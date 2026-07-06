#include "unity.h"
#include "aemlib/aemlib.h"
#include "aemlib/status.h"
#include <string.h>

// Mock implementations for testing aemlib_poll() (client.c) behavior

static int g_write_count = 0;

static aemlib_status_t client_mock_transport_connect(void *ctx) {
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t client_mock_transport_disconnect(void *ctx) {
    return AEMLIB_STATUS_OK;
}

/* No CONNACK ever arrives, simulating a slow/unresponsive broker */
static aemlib_status_t client_mock_transport_read_no_data(void *ctx, uint8_t *buf, size_t len, size_t *out_len) {
    *out_len = 0;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t client_mock_transport_write_counting(void *ctx, const uint8_t *buf, size_t len, size_t *out_len) {
    g_write_count++;
    *out_len = len;
    return AEMLIB_STATUS_OK;
}

static uint64_t client_mock_time_now(void *ctx) {
    return 1000; // frozen clock: never trips the CONNACK timeout
}

// Test aemlib_poll

void test_aemlib_poll_sends_connect_only_once_while_waiting_for_connack(void) {
    g_write_count = 0;

    uint8_t tx_buf[64];
    uint8_t rx_buf[64];
    aemlib_client_t client;
    aemlib_config_t config = {
        .tx_buffer = tx_buf,
        .tx_buffer_size = sizeof(tx_buf),
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect = client_mock_transport_connect,
            .disconnect = client_mock_transport_disconnect,
            .read = client_mock_transport_read_no_data,
            .write = client_mock_transport_write_counting,
            .ctx = NULL
        },
        .time = {
            .now_ms = client_mock_time_now,
            .ctx = NULL
        },
        .keepalive_interval_ms = 60000
    };

    aemlib_status_t status = aemlib_init(&client, &config);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);

    status = aemlib_connect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);

    // A real broker can take several polls to answer with CONNACK; re-sending
    // CONNECT on each of those polls is a protocol violation (found by testing
    // against a real broker on 2026-07-06 - the virtual transport's one-tick
    // CONNACK response had been masking this).
    for (int i = 0; i < 5; i++) {
        status = aemlib_poll(&client);
        TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    }

    TEST_ASSERT_EQUAL(AEMLIB_STATE_MQTT_CONNECT_SENT, client.state);
    TEST_ASSERT_EQUAL(1, g_write_count);
}
