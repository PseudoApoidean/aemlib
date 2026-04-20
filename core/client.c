#include "aemlib/aemlib.h"
#include "aemlib/log.h"
#include "aemlib/status.h"
#include "aemlib/config.h"

#include "core.h"
#include "proto.h"

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
static aemlib_status_t client_read(aemlib_client_t *client);
static aemlib_status_t client_send_connect(aemlib_client_t *client);
static aemlib_status_t client_send_ping(aemlib_client_t *client);

// IMPLEMENTATION ---------------------

static aemlib_status_t client_send(aemlib_client_t *client,
                                   const uint8_t *data,
                                   size_t len)
{
    size_t written = 0;
    aemlib_status_t status = aemlib_transport_send(&client->transport, data, len, &written);

    if (status == AEMLIB_STATUS_OK && written == len) {
        return AEMLIB_STATUS_OK;
    }

    if (AEMLIB_STATUS_CODE(status) == AEMLIB_CODE_WOULD_BLOCK) {
        return AEMLIB_STATUS_OK;
    }

    return status;
}

static aemlib_status_t client_read(aemlib_client_t *client)
{
    size_t read = 0;
    aemlib_status_t status = aemlib_transport_recv(&client->transport,
                                                  client->rx_buffer,
                                                  client->rx_buffer_size,
                                                  &read);

    if (status == AEMLIB_STATUS_OK && read > 0) {
        size_t consumed = 0;
        aemlib_status_t decode_status = aemlib_proto_decode(client->rx_buffer, read, &consumed, client);

        if (decode_status != AEMLIB_STATUS_OK) {
            AEMLIB_LOG_ERROR(AEMLIB_LOG_MODULE_CLIENT, "decode error");
            return decode_status;
        }
    }

    if (AEMLIB_STATUS_CODE(status) == AEMLIB_CODE_WOULD_BLOCK) {
        return AEMLIB_STATUS_OK;
    }

    return status;
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

aemlib_status_t aemlib_client_init(aemlib_client_t *client,
                                   const aemlib_core_config_t *cfg)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    memset(client, 0, sizeof(*client));
    return aemlib_core_init(client, cfg);
}

aemlib_status_t aemlib_client_connect(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    return aemlib_core_connect(client);
}

aemlib_status_t aemlib_client_disconnect(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    return aemlib_core_disconnect(client);
}

aemlib_status_t aemlib_client_publish(aemlib_client_t *client,
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

/*
 * Cooperative poll:
 * - Drive core state machine
 * - Perform I/O
 * - Handle keepalive
 */
aemlib_status_t aemlib_client_poll(aemlib_client_t *client)
{
    if (!client) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    aemlib_status_t status = aemlib_core_poll(client);
    AEMLIB_CHECK_STATUS(status);

    switch (client->state) {

    case AEMLIB_STATE_MQTT_CONNECT_SENT:
        /* After transport connect, send CONNECT packet */
        status = client_send_connect(client);
        if (status != AEMLIB_STATUS_OK) {
            return status;
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
