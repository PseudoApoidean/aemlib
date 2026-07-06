/*
 * Proof-of-concept: aemlib talking to a real MQTT broker over a real POSIX
 * TCP socket (test.mosquitto.org:1883) - connects, subscribes, publishes to
 * its own topic, and waits for the broker to echo the message back.
 */
#include <aemlib/aemlib.h>
#include "posix_tcp_transport.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BROKER_HOST     "test.mosquitto.org"
#define BROKER_PORT     1883
#define DEMO_PAYLOAD    "hello from aemlib over real TCP"
#define OVERALL_TIMEOUT_MS 15000

static uint64_t real_time_now(void *ctx) {
    (void)ctx;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)(ts.tv_nsec / 1000000);
}

static int g_received = 0;

static void on_message(void *ctx,
                       const char *topic,
                       size_t topic_len,
                       const uint8_t *payload,
                       size_t payload_len)
{
    (void)ctx;
    printf("[client] received PUBLISH: topic='%.*s' payload='%.*s'\n",
           (int)topic_len, topic, (int)payload_len, (const char *)payload);
    g_received = 1;
}

int main(void)
{
    /* Unique-ish topic so we don't collide with other test.mosquitto.org traffic */
    char topic[128];
    snprintf(topic, sizeof(topic), "aemlib/demo/%d/%llu",
             (int)getpid(), (unsigned long long)real_time_now(NULL));

    aemlib_posix_tcp_t tcp;
    aemlib_posix_tcp_init(&tcp, BROKER_HOST, BROKER_PORT);

    aemlib_transport_t transport;
    aemlib_posix_tcp_bind(&transport, &tcp);

    uint8_t tx_buf[512];
    uint8_t rx_buf[512];

    aemlib_client_t client;
    aemlib_config_t config = {
        .tx_buffer = tx_buf,
        .tx_buffer_size = sizeof(tx_buf),
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = transport,
        .time = {
            .now_ms = real_time_now,
            .ctx    = NULL
        },
        .keepalive_interval_ms = 60000,
        .on_message     = on_message,
        .on_message_ctx = NULL
    };

    aemlib_status_t status = aemlib_init(&client, &config);
    if (status != AEMLIB_STATUS_OK) {
        fprintf(stderr, "aemlib_init failed (0x%08x)\n", status);
        return 1;
    }

    printf("connecting to %s:%d ...\n", BROKER_HOST, BROKER_PORT);
    status = aemlib_connect(&client);
    if (status != AEMLIB_STATUS_OK) {
        fprintf(stderr, "aemlib_connect failed (0x%08x)\n", status);
        return 1;
    }

    int subscribed = 0;
    int published = 0;
    uint64_t subscribe_time = 0;
    uint64_t start = real_time_now(NULL);

    while (real_time_now(NULL) - start < OVERALL_TIMEOUT_MS) {
        status = aemlib_poll(&client);
        if (status != AEMLIB_STATUS_OK) {
            fprintf(stderr, "poll error (0x%08x) in state %d\n", status, client.state);
            return 1;
        }

        if (client.state == AEMLIB_STATE_MQTT_CONNECTED && !subscribed) {
            aemlib_subscribe(&client, topic, 0);
            subscribed = 1;
            subscribe_time = real_time_now(NULL);
            printf("subscribed to '%s'\n", topic);
        }

        /* Give the broker a moment to process SUBSCRIBE before we PUBLISH */
        if (subscribed && !published && real_time_now(NULL) - subscribe_time > 500) {
            aemlib_publish(&client, topic, (const uint8_t *)DEMO_PAYLOAD, strlen(DEMO_PAYLOAD), 0);
            published = 1;
            printf("published '%s'\n", DEMO_PAYLOAD);
        }

        if (g_received) {
            break;
        }

        usleep(20000); /* 20ms poll cadence */
    }

    aemlib_disconnect(&client);
    aemlib_poll(&client);

    if (!g_received) {
        fprintf(stderr, "demo FAILED: never received the echoed PUBLISH from the real broker\n");
        return 1;
    }

    printf("mqtt_broker_demo OK: connected over real TCP, subscribed, published, and received the message back\n");
    return 0;
}
