#include "unity.h"
#include "core.h"
#include "aemlib/status.h"
#include <string.h>

// Mock implementations for testing
static aemlib_status_t mock_transport_connect(void *ctx) {
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t mock_transport_disconnect(void *ctx) {
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t mock_transport_read(void *ctx, uint8_t *buf, size_t len, size_t *out_len) {
    *out_len = 0;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t mock_transport_write(void *ctx, const uint8_t *buf, size_t len, size_t *out_len) {
    *out_len = len;
    return AEMLIB_STATUS_OK;
}

static uint64_t mock_time_now(void *ctx) {
    return 1000; // Fixed time for testing
}

static aemlib_status_t mock_storage_write(void *ctx, const uint8_t *data, size_t len) {
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t mock_storage_read(void *ctx, uint8_t *buf, size_t buf_len, size_t *out_len) {
    *out_len = 0;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t mock_storage_delete(void *ctx) {
    return AEMLIB_STATUS_OK;
}

// Test setup
void setUp(void) {
    // Set up test fixtures
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test aemlib_core_init
void test_aemlib_core_init_null_client(void) {
    aemlib_core_config_t config = {0};
    aemlib_status_t status = aemlib_core_init(NULL, &config);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG), status);
}

void test_aemlib_core_init_null_config(void) {
    aemlib_client_t client;
    aemlib_status_t status = aemlib_core_init(&client, NULL);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG), status);
}

void test_aemlib_core_init_invalid_config(void) {
    aemlib_client_t client;
    aemlib_core_config_t config = {
        .tx_buffer = NULL,
        .tx_buffer_size = 0,
        .rx_buffer = (uint8_t*)1,
        .rx_buffer_size = 10,
        .transport = {
            .connect = mock_transport_connect,
            .disconnect = mock_transport_disconnect,
            .read = mock_transport_read,
            .write = mock_transport_write,
            .ctx = NULL
        },
        .time = {
            .now_ms = mock_time_now,
            .ctx = NULL
        },
        .storage = {0},
        .keepalive_interval_ms = 60000
    };

    aemlib_status_t status = aemlib_core_init(&client, &config);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG), status);
}

void test_aemlib_core_init_valid_config(void) {
    aemlib_client_t client;
    uint8_t tx_buf[100];
    uint8_t rx_buf[100];

    aemlib_core_config_t config = {
        .tx_buffer = tx_buf,
        .tx_buffer_size = sizeof(tx_buf),
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect = mock_transport_connect,
            .disconnect = mock_transport_disconnect,
            .read = mock_transport_read,
            .write = mock_transport_write,
            .ctx = NULL
        },
        .time = {
            .now_ms = mock_time_now,
            .ctx = NULL
        },
        .storage = {
            .write_record = mock_storage_write,
            .read_record = mock_storage_read,
            .delete_record = mock_storage_delete,
            .ctx = NULL
        },
        .keepalive_interval_ms = 60000
    };

    aemlib_status_t status = aemlib_core_init(&client, &config);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_DISCONNECTED, client.state);
    TEST_ASSERT_EQUAL(1, client.packet_id);
    TEST_ASSERT_EQUAL(1000, client.last_activity_ms);
}

// Test aemlib_core_poll
void test_aemlib_core_poll_null_client(void) {
    aemlib_status_t status = aemlib_core_poll(NULL);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG), status);
}

void test_aemlib_core_poll_disconnected_state(void) {
    aemlib_client_t client = {
        .state = AEMLIB_STATE_DISCONNECTED,
        .transport = {
            .connect = mock_transport_connect,
            .disconnect = mock_transport_disconnect,
            .read = mock_transport_read,
            .write = mock_transport_write,
            .ctx = NULL
        },
        .time = {
            .now_ms = mock_time_now,
            .ctx = NULL
        }
    };

    aemlib_status_t status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
}

// Test aemlib_core_connect
void test_aemlib_core_connect_null_client(void) {
    aemlib_status_t status = aemlib_core_connect(NULL);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG), status);
}

void test_aemlib_core_connect_from_disconnected(void) {
    aemlib_client_t client = {
        .state = AEMLIB_STATE_DISCONNECTED
    };

    aemlib_status_t status = aemlib_core_connect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_CONNECTING, client.state);
}

void test_aemlib_core_connect_invalid_state(void) {
    aemlib_client_t client = {
        .state = AEMLIB_STATE_MQTT_CONNECTED
    };

    aemlib_status_t status = aemlib_core_connect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);
}

// Test aemlib_core_disconnect
void test_aemlib_core_disconnect_null_client(void) {
    aemlib_status_t status = aemlib_core_disconnect(NULL);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG), status);
}

void test_aemlib_core_disconnect_from_connected(void) {
    aemlib_client_t client = {
        .state = AEMLIB_STATE_MQTT_CONNECTED
    };

    aemlib_status_t status = aemlib_core_disconnect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_MQTT_DISCONNECTING, client.state);
}

void test_aemlib_core_disconnect_invalid_state(void) {
    aemlib_client_t client = {
        .state = AEMLIB_STATE_DISCONNECTED
    };

    aemlib_status_t status = aemlib_core_disconnect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);
}

// Test aemlib_core_next_packet_id
void test_aemlib_core_next_packet_id_null_client(void) {
    uint16_t id = aemlib_core_next_packet_id(NULL);
    TEST_ASSERT_EQUAL(1, id);
}

void test_aemlib_core_next_packet_id_normal(void) {
    aemlib_client_t client = {
        .packet_id = 5
    };

    uint16_t id = aemlib_core_next_packet_id(&client);
    TEST_ASSERT_EQUAL(6, id);
    TEST_ASSERT_EQUAL(6, client.packet_id);
}

void test_aemlib_core_next_packet_id_wraparound(void) {
    aemlib_client_t client = {
        .packet_id = 65535
    };

    uint16_t id = aemlib_core_next_packet_id(&client);
    TEST_ASSERT_EQUAL(1, id);
    TEST_ASSERT_EQUAL(1, client.packet_id);
}