/*
 * Proof-of-concept: a real aemlib client talking over a virtual (in-memory)
 * transport to a tiny fake broker also defined in this file. Demonstrates
 * that the connect handshake, SUBSCRIBE/SUBACK, and inbound PUBLISH decode
 * are all real - the client only ever sees bytes arrive over the transport,
 * exactly as it would with a real broker.
 */
#include <aemlib/aemlib.h>
#include <proto.h>
#include <transport.h>
#include <virtual_transport.h>

#include <stdio.h>
#include <string.h>

#define DEMO_TOPIC   "demo/topic"
#define DEMO_PAYLOAD "hello from aemlib"

// FAKE BROKER -----------------------------------------------------------

static uint8_t broker_topic[128];
static size_t  broker_topic_len = 0;
static int     broker_subscribed = 0;

static void broker_process_packet(aemlib_transport_t *broker_transport,
                                  const uint8_t *buf,
                                  const aemlib_mqtt_fixed_header_t *header,
                                  size_t packet_len)
{
    switch (header->type) {
    case AEMLIB_MQTT_PKT_CONNECT: {
        static const uint8_t connack[] = {0x20, 0x02, 0x00, 0x00};
        size_t written = 0;
        aemlib_transport_write(broker_transport, connack, sizeof(connack), &written);
        printf("[broker] CONNECT received, sent CONNACK\n");
        break;
    }

    case AEMLIB_MQTT_PKT_SUBSCRIBE: {
        size_t offset = header->header_size;
        uint16_t packet_id = (uint16_t)(((uint16_t)buf[offset] << 8) | buf[offset + 1]);
        offset += 2;

        size_t topic_len = ((size_t)buf[offset] << 8) | buf[offset + 1];
        offset += 2;
        const uint8_t *topic = &buf[offset];

        if (topic_len <= sizeof(broker_topic)) {
            memcpy(broker_topic, topic, topic_len);
            broker_topic_len = topic_len;
            broker_subscribed = 1;
        }
        printf("[broker] SUBSCRIBE received for topic '%.*s'\n", (int)topic_len, topic);

        uint8_t suback[] = {0x90, 0x03, (uint8_t)(packet_id >> 8), (uint8_t)(packet_id & 0xFF), 0x00};
        size_t written = 0;
        aemlib_transport_write(broker_transport, suback, sizeof(suback), &written);
        printf("[broker] sent SUBACK\n");
        break;
    }

    case AEMLIB_MQTT_PKT_PUBLISH: {
        const char *topic;
        size_t topic_len;
        const uint8_t *payload;
        size_t payload_len;

        aemlib_status_t status = aemlib_proto_decode_publish(buf, packet_len, header,
                                                             &topic, &topic_len, &payload, &payload_len);
        if (status != AEMLIB_STATUS_OK) {
            return;
        }
        printf("[broker] PUBLISH received: topic='%.*s' payload='%.*s'\n",
               (int)topic_len, topic, (int)payload_len, (const char *)payload);

        if (broker_subscribed && topic_len == broker_topic_len &&
            memcmp(topic, broker_topic, topic_len) == 0) {
            size_t written = 0;
            aemlib_transport_write(broker_transport, buf, packet_len, &written);
            printf("[broker] echoing PUBLISH back to subscriber\n");
        }
        break;
    }

    default:
        break;
    }
}

/* Accumulates across calls and processes every complete packet currently
 * buffered, so packets that arrive coalesced in one read (e.g. a SUBSCRIBE
 * immediately followed by a PUBLISH) are never silently dropped. */
static void broker_tick(aemlib_transport_t *broker_transport)
{
    static uint8_t buf[256];
    static size_t buf_len = 0;

    size_t read = 0;
    aemlib_status_t status = aemlib_transport_read(broker_transport, buf + buf_len, sizeof(buf) - buf_len, &read);
    if (status != AEMLIB_STATUS_OK && AEMLIB_STATUS_CODE(status) != AEMLIB_CODE_WOULD_BLOCK) {
        return;
    }
    buf_len += read;

    for (;;) {
        if (buf_len == 0) {
            return;
        }

        aemlib_mqtt_fixed_header_t header;
        status = aemlib_proto_decode_fixed_header(buf, buf_len, &header);
        if (status != AEMLIB_STATUS_OK) {
            return; /* incomplete header or malformed; wait for more or give up */
        }

        size_t packet_len = header.header_size + header.remaining_length;
        if (packet_len > buf_len) {
            return; /* full packet not buffered yet */
        }

        broker_process_packet(broker_transport, buf, &header, packet_len);

        buf_len -= packet_len;
        if (buf_len > 0) {
            memmove(buf, buf + packet_len, buf_len);
        }
    }
}

// CLIENT ------------------------------------------------------------------

static uint64_t stub_time_now(void *ctx) {
    (void)ctx;
    return 0;
}

static void on_message(void *ctx,
                       const char *topic,
                       size_t topic_len,
                       const uint8_t *payload,
                       size_t payload_len)
{
    int *received = (int *)ctx;
    printf("[client] received PUBLISH: topic='%.*s' payload='%.*s'\n",
           (int)topic_len, topic, (int)payload_len, (const char *)payload);
    *received = 1;
}

int main(void)
{
    uint8_t chan_c2b[512];
    uint8_t chan_b2c[512];
    aemlib_virtual_channel_t ch_c2b, ch_b2c;
    aemlib_virtual_channel_init(&ch_c2b, chan_c2b, sizeof(chan_c2b));
    aemlib_virtual_channel_init(&ch_b2c, chan_b2c, sizeof(chan_b2c));

    aemlib_virtual_transport_t vt_client, vt_broker;
    aemlib_virtual_transport_pair_init(&vt_client, &ch_c2b, &vt_broker, &ch_b2c);

    aemlib_transport_t client_transport, broker_transport;
    aemlib_virtual_transport_bind(&client_transport, &vt_client);
    aemlib_virtual_transport_bind(&broker_transport, &vt_broker);
    aemlib_transport_connect(&broker_transport); /* broker is always listening */

    int received = 0;

    uint8_t tx_buf[256];
    uint8_t rx_buf[256];
    aemlib_client_t client;
    aemlib_config_t config = {
        .tx_buffer = tx_buf,
        .tx_buffer_size = sizeof(tx_buf),
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = client_transport,
        .time = {
            .now_ms = stub_time_now,
            .ctx    = NULL
        },
        .keepalive_interval_ms = 60000,
        .on_message     = on_message,
        .on_message_ctx = &received
    };

    aemlib_status_t status = aemlib_init(&client, &config);
    if (status != AEMLIB_STATUS_OK) {
        fprintf(stderr, "aemlib_init failed (0x%08x)\n", status);
        return 1;
    }

    status = aemlib_connect(&client);
    if (status != AEMLIB_STATUS_OK) {
        fprintf(stderr, "aemlib_connect failed (0x%08x)\n", status);
        return 1;
    }

    int subscribed = 0;
    int published = 0;

    for (int tick = 0; tick < 20 && !received; tick++) {
        status = aemlib_poll(&client);
        if (status != AEMLIB_STATUS_OK) {
            fprintf(stderr, "aemlib_poll failed (0x%08x)\n", status);
            return 1;
        }

        broker_tick(&broker_transport);

        if (client.state == AEMLIB_STATE_MQTT_CONNECTED && !subscribed) {
            aemlib_subscribe(&client, DEMO_TOPIC, 0);
            subscribed = 1;
            printf("[client] subscribed to '%s'\n", DEMO_TOPIC);
        }

        if (subscribed && !published) {
            aemlib_publish(&client, DEMO_TOPIC, (const uint8_t *)DEMO_PAYLOAD, strlen(DEMO_PAYLOAD), 0);
            published = 1;
            printf("[client] published '%s' to '%s'\n", DEMO_PAYLOAD, DEMO_TOPIC);
        }
    }

    aemlib_disconnect(&client);
    aemlib_poll(&client);

    if (!received) {
        fprintf(stderr, "demo FAILED: never received the echoed PUBLISH\n");
        return 1;
    }

    printf("pubsub demo OK: connect -> subscribe -> publish -> receive, all over a real (virtual) transport\n");
    return 0;
}
