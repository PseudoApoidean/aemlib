#include "unity.h"
#include "core.h"
#include "aemlib/status.h"
#include "proto.h"
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

/* Canned CONNACK, accepted (session_present=0, return_code=0) */
static aemlib_status_t mock_transport_read_connack_accepted(void *ctx, uint8_t *buf, size_t len, size_t *out_len) {
    static const uint8_t connack[] = {0x20, 0x02, 0x00, 0x00};
    size_t n = sizeof(connack) < len ? sizeof(connack) : len;
    memcpy(buf, connack, n);
    *out_len = n;
    return AEMLIB_STATUS_OK;
}

/* Canned CONNACK, rejected (return_code=5, "not authorized") */
static aemlib_status_t mock_transport_read_connack_rejected(void *ctx, uint8_t *buf, size_t len, size_t *out_len) {
    static const uint8_t connack[] = {0x20, 0x02, 0x00, 0x05};
    size_t n = sizeof(connack) < len ? sizeof(connack) : len;
    memcpy(buf, connack, n);
    *out_len = n;
    return AEMLIB_STATUS_OK;
}

/* CONNACK immediately followed by another packet's bytes in the same read(),
 * to verify the trailing bytes survive for client_read() rather than being
 * discarded along with the consumed CONNACK. */
static uint8_t g_connack_plus_extra[64];
static size_t  g_connack_plus_extra_len = 0;

static aemlib_status_t mock_transport_read_connack_plus_extra(void *ctx, uint8_t *buf, size_t len, size_t *out_len) {
    size_t n = g_connack_plus_extra_len < len ? g_connack_plus_extra_len : len;
    memcpy(buf, g_connack_plus_extra, n);
    *out_len = n;
    return AEMLIB_STATUS_OK;
}

/* Fake clock that a test can advance to exercise timeouts */
static uint64_t g_fake_time_ms;

static uint64_t mock_time_now_advancing(void *ctx) {
    return g_fake_time_ms;
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
    uint8_t dummy_rx_buf[10];
    aemlib_core_config_t config = {
        .tx_buffer = NULL,
        .tx_buffer_size = 0,
        .rx_buffer = dummy_rx_buf,
        .rx_buffer_size = sizeof(dummy_rx_buf),
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

// Test state machine protocol conformance
void test_aemlib_core_connect_invalid_transitions(void) {
    // Test connect from CONNECTING state (invalid)
    aemlib_client_t client = {
        .state = AEMLIB_STATE_CONNECTING
    };
    aemlib_status_t status = aemlib_core_connect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);

    // Test connect from MQTT_CONNECT_SENT state (invalid)
    client.state = AEMLIB_STATE_MQTT_CONNECT_SENT;
    status = aemlib_core_connect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);

    // Test connect from MQTT_DISCONNECTING state (invalid)
    client.state = AEMLIB_STATE_MQTT_DISCONNECTING;
    status = aemlib_core_connect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);
}

void test_aemlib_core_disconnect_invalid_transitions(void) {
    // Test disconnect from DISCONNECTED state (invalid)
    aemlib_client_t client = {
        .state = AEMLIB_STATE_DISCONNECTED
    };
    aemlib_status_t status = aemlib_core_disconnect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);

    // Test disconnect from CONNECTING state (invalid)
    client.state = AEMLIB_STATE_CONNECTING;
    status = aemlib_core_disconnect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);

    // Test disconnect from MQTT_CONNECT_SENT state (invalid)
    client.state = AEMLIB_STATE_MQTT_CONNECT_SENT;
    status = aemlib_core_disconnect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);
}

void test_aemlib_core_poll_invalid_state(void) {
    aemlib_client_t client = {
        .state = (aemlib_state_t)999 // deliberately invalid state
    };
    aemlib_status_t status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_STATE_INVALID), status);
}

void test_aemlib_core_state_machine_connect_flow(void) {
    uint8_t rx_buf[16];
    aemlib_client_t client = {
        .state = AEMLIB_STATE_DISCONNECTED,
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect = mock_transport_connect,
            .disconnect = mock_transport_disconnect,
            .read = mock_transport_read_connack_accepted,
            .write = mock_transport_write,
            .ctx = NULL
        },
        .time = {
            .now_ms = mock_time_now,
            .ctx = NULL
        }
    };

    // Start connection
    aemlib_status_t status = aemlib_core_connect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_CONNECTING, client.state);

    // Poll while connecting (transport connect succeeds)
    status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_MQTT_CONNECT_SENT, client.state);

    // Poll while waiting for CONNACK (broker accepts the connection)
    status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_MQTT_CONNECTED, client.state);

    // Now disconnect should work
    status = aemlib_core_disconnect(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_MQTT_DISCONNECTING, client.state);

    // Poll while disconnecting
    status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_DISCONNECTED, client.state);
}

void test_aemlib_core_connect_sent_connack_rejected(void) {
    uint8_t rx_buf[16];
    aemlib_client_t client = {
        .state = AEMLIB_STATE_MQTT_CONNECT_SENT,
        .last_activity_ms = 1000,
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect = mock_transport_connect,
            .disconnect = mock_transport_disconnect,
            .read = mock_transport_read_connack_rejected,
            .write = mock_transport_write,
            .ctx = NULL
        },
        .time = {
            .now_ms = mock_time_now,
            .ctx = NULL
        }
    };

    aemlib_status_t status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_CONNECT), status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_DISCONNECTED, client.state);
}

void test_aemlib_core_connect_sent_connack_timeout(void) {
    g_fake_time_ms = 1000;

    aemlib_client_t client = {
        .state = AEMLIB_STATE_MQTT_CONNECT_SENT,
        .last_activity_ms = 1000,
        .transport = {
            .connect = mock_transport_connect,
            .disconnect = mock_transport_disconnect,
            .read = mock_transport_read, // no data available
            .write = mock_transport_write,
            .ctx = NULL
        },
        .time = {
            .now_ms = mock_time_now_advancing,
            .ctx = NULL
        }
    };

    // Still within the CONNACK timeout window
    aemlib_status_t status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_MQTT_CONNECT_SENT, client.state);

    // Past the CONNACK timeout window (matches AEMLIB_CORE_CONNACK_TIMEOUT_MS in core.c)
    g_fake_time_ms = 1000 + 5000;
    status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_TIMEOUT), status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_DISCONNECTED, client.state);
}

void test_aemlib_core_connect_sent_preserves_bytes_after_connack(void) {
    static const uint8_t connack[] = {0x20, 0x02, 0x00, 0x00};
    memcpy(g_connack_plus_extra, connack, sizeof(connack));

    size_t extra_len = 0;
    aemlib_status_t encode_status = aemlib_proto_encode_publish(
        g_connack_plus_extra + sizeof(connack),
        sizeof(g_connack_plus_extra) - sizeof(connack),
        &extra_len, "x", (const uint8_t *)"y", 1);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, encode_status);
    g_connack_plus_extra_len = sizeof(connack) + extra_len;

    uint8_t rx_buf[64];
    aemlib_client_t client = {
        .state = AEMLIB_STATE_MQTT_CONNECT_SENT,
        .last_activity_ms = 1000,
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect = mock_transport_connect,
            .disconnect = mock_transport_disconnect,
            .read = mock_transport_read_connack_plus_extra,
            .write = mock_transport_write,
            .ctx = NULL
        },
        .time = {
            .now_ms = mock_time_now,
            .ctx = NULL
        }
    };

    // AEM-7: CONNACK immediately followed by another packet's bytes in the
    // same read() used to discard everything past the CONNACK.
    aemlib_status_t status = aemlib_core_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_STATE_MQTT_CONNECTED, client.state);
    TEST_ASSERT_EQUAL(extra_len, client.rx_len);
}