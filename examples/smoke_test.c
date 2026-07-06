/*
 * Permanent smoke test for the public API. Only includes <aemlib/aemlib.h>
 * (no internal headers), so it fails to link the moment the public API and
 * the client implementation drift apart - the exact bug found and fixed on
 * 2026-07-05, when aemlib.h declared functions client.c never defined.
 */
#include <aemlib/aemlib.h>

#include <stdio.h>
#include <string.h>

static int g_connack_sent = 0;

static aemlib_status_t stub_connect(void *ctx) {
    (void)ctx;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t stub_disconnect(void *ctx) {
    (void)ctx;
    return AEMLIB_STATUS_OK;
}

/* Hands back a canned CONNACK once, so the state machine can really reach
 * MQTT_CONNECTED instead of the test having to fake that transition. */
static aemlib_status_t stub_read(void *ctx, uint8_t *buf, size_t len, size_t *out_len) {
    (void)ctx;

    if (!g_connack_sent && len >= 4) {
        static const uint8_t connack[] = {0x20, 0x02, 0x00, 0x00};
        memcpy(buf, connack, sizeof(connack));
        *out_len = sizeof(connack);
        g_connack_sent = 1;
        return AEMLIB_STATUS_OK;
    }

    *out_len = 0;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t stub_write(void *ctx, const uint8_t *buf, size_t len, size_t *out_len) {
    (void)ctx;
    (void)buf;
    *out_len = len;
    return AEMLIB_STATUS_OK;
}

static uint64_t stub_time_now(void *ctx) {
    (void)ctx;
    return 0;
}

static int fail(const char *step, aemlib_status_t status) {
    fprintf(stderr, "smoke test FAILED at %s (status=0x%08x)\n", step, status);
    return 1;
}

int main(void) {
    uint8_t tx_buf[256];
    uint8_t rx_buf[256];

    aemlib_client_t client;
    aemlib_config_t config = {
        .tx_buffer = tx_buf,
        .tx_buffer_size = sizeof(tx_buf),
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect    = stub_connect,
            .disconnect = stub_disconnect,
            .read       = stub_read,
            .write      = stub_write,
            .ctx        = NULL
        },
        .time = {
            .now_ms = stub_time_now,
            .ctx    = NULL
        },
        .keepalive_interval_ms = 60000
    };

    aemlib_status_t status = aemlib_init(&client, &config);
    if (status != AEMLIB_STATUS_OK) {
        return fail("aemlib_init", status);
    }

    status = aemlib_connect(&client);
    if (status != AEMLIB_STATUS_OK) {
        return fail("aemlib_connect", status);
    }

    for (int i = 0; i < 4; i++) {
        status = aemlib_poll(&client);
        if (status != AEMLIB_STATUS_OK) {
            return fail("aemlib_poll (connecting)", status);
        }
    }

    if (client.state != AEMLIB_STATE_MQTT_CONNECTED) {
        fprintf(stderr, "smoke test FAILED: did not reach MQTT_CONNECTED (state=%d)\n", client.state);
        return 1;
    }

    const uint8_t payload[] = "hello";
    status = aemlib_publish(&client, "smoke/topic", payload, sizeof(payload) - 1, 0);
    if (status != AEMLIB_STATUS_OK) {
        return fail("aemlib_publish", status);
    }

    status = aemlib_subscribe(&client, "smoke/topic", 0);
    if (status != AEMLIB_STATUS_OK) {
        return fail("aemlib_subscribe", status);
    }

    status = aemlib_poll(&client);
    if (status != AEMLIB_STATUS_OK) {
        return fail("aemlib_poll (connected)", status);
    }

    status = aemlib_disconnect(&client);
    if (status != AEMLIB_STATUS_OK) {
        return fail("aemlib_disconnect", status);
    }

    status = aemlib_poll(&client);
    if (status != AEMLIB_STATUS_OK) {
        return fail("aemlib_poll (disconnecting)", status);
    }

    if (client.state != AEMLIB_STATE_DISCONNECTED) {
        fprintf(stderr, "smoke test FAILED: did not reach DISCONNECTED (state=%d)\n", client.state);
        return 1;
    }

    printf("smoke test OK: init -> connect -> poll -> publish -> subscribe -> poll -> disconnect\n");
    return 0;
}
