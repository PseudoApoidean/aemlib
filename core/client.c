#include "aemlib/aemlib.h"
#include "aemlib/log.h"
#include "aemlib/status.h"
#include "aemlib/config.h"

#include "core.h"
#include "proto.h"

#include <string.h>

/*
 * CLIENT LAYER OVERVIEW
 * ---------------------
 * - Drives transport I/O (send/recv)
 * - Encodes outbound MQTT packets using proto.c
 * - Decodes inbound MQTT packets using proto.c
 * - Dispatches decoded packets to core state machine
 * - Implements public API: connect/publish/subscribe/poll
 * - No dynamic allocation, cooperative polling only
 */

// DECLARATIONS -----------------------

static aemlib_status_t client_send(aemlib_client_t *client,
                                   const uint8_t *data,
                                   size_t len);
static aemlib_status_t client_flush_pending(aemlib_client_t *client);
static aemlib_status_t client_dispatch_packet(aemlib_client_t *client,
                                              const aemlib_mqtt_fixed_header_t *header,
                                              size_t packet_len);
static aemlib_status_t client_read(aemlib_client_t *client);
static aemlib_status_t client_send_connect(aemlib_client_t *client);
static aemlib_status_t client_send_ping(aemlib_client_t *client);

// IMPLEMENTATION ---------------------

/* data is always client->tx_buffer at every call site in this file - a
 * partial write is staged back at tx_buffer[0] and finished later by
 * client_flush_pending(), rather than silently dropping the remainder. */
static aemlib_status_t client_send(aemlib_client_t *client,
                                   const uint8_t *data,
                                   size_t len)
{
    if (client->tx_pending_len > 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_WOULD_BLOCK);
    }

    size_t written = 0;
    aemlib_status_t status = aemlib_transport_write(&client->transport, data, len, &written);

    if (status != AEMLIB_STATUS_OK && AEMLIB_STATUS_CODE(status) != AEMLIB_CODE_WOULD_BLOCK) {
        return status;
    }

    if (AEMLIB_STATUS_CODE(status) == AEMLIB_CODE_WOULD_BLOCK) {
        written = 0;
    }

    if (written < len) {
        size_t remaining = len - written;
        memmove(client->tx_buffer, data + written, remaining);
        client->tx_pending_len = remaining;
    }

    return AEMLIB_STATUS_OK;
}

/* Resume a previous partial write, if any. Must run before any new send. */
static aemlib_status_t client_flush_pending(aemlib_client_t *client)
{
    if (client->tx_pending_len == 0) {
        return AEMLIB_STATUS_OK;
    }

    size_t written = 0;
    aemlib_status_t status = aemlib_transport_write(&client->transport,
                                                    client->tx_buffer,
                                                    client->tx_pending_len,
                                                    &written);

    if (status != AEMLIB_STATUS_OK && AEMLIB_STATUS_CODE(status) != AEMLIB_CODE_WOULD_BLOCK) {
        return status;
    }

    if (AEMLIB_STATUS_CODE(status) == AEMLIB_CODE_WOULD_BLOCK) {
        written = 0;
    }

    if (written > 0) {
        client->tx_pending_len -= written;
        if (client->tx_pending_len > 0) {
            memmove(client->tx_buffer, client->tx_buffer + written, client->tx_pending_len);
        }
    }

    return AEMLIB_STATUS_OK;
}

static aemlib_status_t client_dispatch_packet(aemlib_client_t *client,
                                              const aemlib_mqtt_fixed_header_t *header,
                                              size_t packet_len)
{
    switch (header->type) {
    case AEMLIB_MQTT_PKT_PUBLISH: {
        const char *topic = NULL;
        size_t topic_len = 0;
        const uint8_t *payload = NULL;
        size_t payload_len = 0;

        aemlib_status_t decode_status = aemlib_proto_decode_publish(client->rx_buffer, packet_len, header,
                                                                     &topic, &topic_len, &payload, &payload_len);
        if (decode_status != AEMLIB_STATUS_OK) {
            AEMLIB_LOG_ERROR(AEMLIB_LOG_MODULE_CLIENT, "PUBLISH decode error");
            return decode_status;
        }

        if (client->on_message) {
            client->on_message(client->on_message_ctx, topic, topic_len, payload, payload_len);
        }
        return AEMLIB_STATUS_OK;
    }

    case AEMLIB_MQTT_PKT_PINGRESP:
        AEMLIB_LOG_DEBUG(AEMLIB_LOG_MODULE_CLIENT, "PINGRESP received");
        return AEMLIB_STATUS_OK;

    case AEMLIB_MQTT_PKT_SUBACK:
        AEMLIB_LOG_DEBUG(AEMLIB_LOG_MODULE_CLIENT, "SUBACK received");
        return AEMLIB_STATUS_OK;

    default:
        AEMLIB_LOG_DEBUG(AEMLIB_LOG_MODULE_CLIENT, "unhandled packet type: %d", header->type);
        return AEMLIB_STATUS_OK;
    }
}

/* Appends whatever the transport has to rx_buffer, then processes every
 * complete packet now buffered. A partial packet (or partial fixed header)
 * just waits in rx_buffer for the bytes it's missing on a later poll. */
static aemlib_status_t client_read(aemlib_client_t *client)
{
    size_t read = 0;
    aemlib_status_t status = aemlib_transport_read(&client->transport,
                                                  client->rx_buffer + client->rx_len,
                                                  client->rx_buffer_size - client->rx_len,
                                                  &read);

    if (status != AEMLIB_STATUS_OK && AEMLIB_STATUS_CODE(status) != AEMLIB_CODE_WOULD_BLOCK) {
        return status;
    }

    if (AEMLIB_STATUS_CODE(status) == AEMLIB_CODE_WOULD_BLOCK) {
        read = 0;
    }

    client->rx_len += read;

    for (;;) {
        if (client->rx_len == 0) {
            return AEMLIB_STATUS_OK;
        }

        aemlib_mqtt_fixed_header_t header;
        aemlib_status_t decode_status = aemlib_proto_decode_fixed_header(client->rx_buffer, client->rx_len, &header);

        if (AEMLIB_STATUS_CODE(decode_status) == AEMLIB_CODE_BUFFER_TOO_SMALL) {
            if (client->rx_len == client->rx_buffer_size) {
                AEMLIB_LOG_ERROR(AEMLIB_LOG_MODULE_CLIENT, "rx buffer full without a complete fixed header");
                return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
            }
            return AEMLIB_STATUS_OK; /* wait for more bytes */
        }

        if (decode_status != AEMLIB_STATUS_OK) {
            AEMLIB_LOG_ERROR(AEMLIB_LOG_MODULE_CLIENT, "decode error");
            return decode_status;
        }

        size_t packet_len = header.header_size + header.remaining_length;

        if (packet_len > client->rx_buffer_size) {
            AEMLIB_LOG_ERROR(AEMLIB_LOG_MODULE_CLIENT, "packet too large for rx buffer");
            return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
        }

        if (packet_len > client->rx_len) {
            return AEMLIB_STATUS_OK; /* full packet not buffered yet */
        }

        decode_status = client_dispatch_packet(client, &header, packet_len);
        if (decode_status != AEMLIB_STATUS_OK) {
            return decode_status;
        }

        client->rx_len -= packet_len;
        if (client->rx_len > 0) {
            memmove(client->rx_buffer, client->rx_buffer + packet_len, client->rx_len);
        }
    }
}

/* Build and send CONNECT packet */
static aemlib_status_t client_send_connect(aemlib_client_t *client)
{
    size_t out_len = 0;
    uint16_t keepalive_sec = (uint16_t)(client->keepalive_interval_ms / 1000);

    aemlib_status_t status = aemlib_proto_encode_connect(
        client->tx_buffer,
        client->tx_buffer_size,
        &out_len,
        "",
        keepalive_sec);

    AEMLIB_CHECK_STATUS(status);

    return client_send(client, client->tx_buffer, out_len);
}

/* Build and send PINGREQ */
static aemlib_status_t client_send_ping(aemlib_client_t *client)
{
    size_t out_len = 0;
    aemlib_status_t status = aemlib_proto_encode_pingreq(
        client->tx_buffer,
        client->tx_buffer_size,
        &out_len);

    if (status != AEMLIB_STATUS_OK) {
        return status;
    }

    return client_send(client, client->tx_buffer, out_len);
}

/* PUBLIC API ------------------------------------------------------------ */

aemlib_status_t aemlib_init(aemlib_client_t *client,
                            const aemlib_config_t *cfg)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    memset(client, 0, sizeof(*client));
    return aemlib_core_init(client, cfg);
}

aemlib_status_t aemlib_connect(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    /* Discard anything left over from a previous connection attempt */
    client->tx_pending_len = 0;
    client->rx_len = 0;

    return aemlib_core_connect(client);
}

aemlib_status_t aemlib_disconnect(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    return aemlib_core_disconnect(client);
}

aemlib_status_t aemlib_publish(aemlib_client_t *client,
                               const char *topic,
                               const uint8_t *payload,
                               size_t payload_len,
                               int qos)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    if (qos != 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_UNSUPPORTED);
    }

    size_t out_len = 0;

    aemlib_status_t status = aemlib_proto_encode_publish(
        client->tx_buffer,
        client->tx_buffer_size,
        &out_len,
        topic,
        payload,
        payload_len);

    AEMLIB_CHECK_STATUS(status);

    return client_send(client, client->tx_buffer, out_len);
}

aemlib_status_t aemlib_subscribe(aemlib_client_t *client,
                                 const char *topic,
                                 int qos)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    if (qos != 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_UNSUPPORTED);
    }

    size_t out_len = 0;
    uint16_t packet_id = aemlib_core_next_packet_id(client);

    aemlib_status_t status = aemlib_proto_encode_subscribe(
        client->tx_buffer,
        client->tx_buffer_size,
        &out_len,
        packet_id,
        topic);

    AEMLIB_CHECK_STATUS(status);

    return client_send(client, client->tx_buffer, out_len);
}

/*
 * Cooperative poll:
 * - Drive core state machine
 * - Perform I/O
 * - Handle keepalive
 */
aemlib_status_t aemlib_poll(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    aemlib_status_t status = client_flush_pending(client);
    AEMLIB_CHECK_STATUS(status);

    status = aemlib_core_poll(client);
    AEMLIB_CHECK_STATUS(status);

    switch (client->state) {

    case AEMLIB_STATE_MQTT_CONNECT_SENT:
        /* Send CONNECT once; a real broker can take several polls to reply
         * with CONNACK, and re-sending CONNECT while waiting is a protocol
         * violation that gets the connection dropped. */
        if (!client->connect_sent) {
            status = client_send_connect(client);
            if (status != AEMLIB_STATUS_OK) {
                return status;
            }
            client->connect_sent = 1;
        }
        break;

    case AEMLIB_STATE_MQTT_CONNECTED:
        /* Read inbound packets */
        status = client_read(client);
        AEMLIB_CHECK_STATUS(status);

        /* Keepalive */
        uint64_t now = aemlib_time_now(&client->time);
        if (now - client->last_activity_ms >= client->keepalive_interval_ms) {
            status = client_send_ping(client);
            AEMLIB_CHECK_STATUS(status);
            client->last_activity_ms = now;
        }
        break;

    default:
        break;
    }

    return AEMLIB_STATUS_OK;
}
