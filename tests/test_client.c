#include "unity.h"
#include "aemlib/aemlib.h"
#include "aemlib/status.h"
#include "proto.h"
#include <string.h>

// Mock implementations for testing aemlib_poll() (client.c) behavior

static int g_write_count = 0;

static uint8_t g_written_bytes[64];
static size_t  g_written_total = 0;

/* Accepts at most one byte per call, to exercise the partial-write retry path */
static aemlib_status_t client_mock_transport_write_one_byte(void *ctx, const uint8_t *buf, size_t len, size_t *out_len) {
    size_t n = (len > 0) ? 1 : 0;
    if (n > 0) {
        g_written_bytes[g_written_total++] = buf[0];
    }
    *out_len = n;
    return AEMLIB_STATUS_OK;
}

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

static int  g_message_count = 0;
static char g_last_topic[64];
static char g_last_payload[64];

static void client_mock_on_message(void *ctx,
                                   const char *topic,
                                   size_t topic_len,
                                   const uint8_t *payload,
                                   size_t payload_len) {
    g_message_count++;

    size_t tlen = topic_len < sizeof(g_last_topic) - 1 ? topic_len : sizeof(g_last_topic) - 1;
    memcpy(g_last_topic, topic, tlen);
    g_last_topic[tlen] = '\0';

    size_t plen = payload_len < sizeof(g_last_payload) - 1 ? payload_len : sizeof(g_last_payload) - 1;
    memcpy(g_last_payload, payload, plen);
    g_last_payload[plen] = '\0';
}

/* Serves bytes out of a canned buffer, optionally in small chunks, to
 * simulate multiple packets arriving in one read() or a single packet
 * split across several reads(). */
static uint8_t g_read_buf[128];
static size_t  g_read_buf_len = 0;
static size_t  g_read_buf_offset = 0;
static size_t  g_read_chunk_size = 0; /* 0 = deliver everything available */

static aemlib_status_t client_mock_transport_read_from_buf(void *ctx, uint8_t *buf, size_t len, size_t *out_len) {
    size_t remaining = g_read_buf_len - g_read_buf_offset;
    size_t to_copy = remaining;
    if (g_read_chunk_size > 0 && to_copy > g_read_chunk_size) {
        to_copy = g_read_chunk_size;
    }
    if (to_copy > len) {
        to_copy = len;
    }
    memcpy(buf, g_read_buf + g_read_buf_offset, to_copy);
    g_read_buf_offset += to_copy;
    *out_len = to_copy;
    return AEMLIB_STATUS_OK;
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

void test_aemlib_poll_retries_partial_write_without_data_loss(void) {
    g_written_total = 0;

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
            .write = client_mock_transport_write_one_byte,
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

    // The whole CONNECT packet (14 bytes for an empty client ID) trickles out
    // one byte per poll; AEM-6 was client_send() reporting success and
    // dropping everything past the first byte instead of retrying the rest.
    for (int i = 0; i < 20; i++) {
        status = aemlib_poll(&client);
        TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    }

    TEST_ASSERT_EQUAL(0, client.tx_pending_len);

    aemlib_mqtt_fixed_header_t header;
    status = aemlib_proto_decode_fixed_header(g_written_bytes, g_written_total, &header);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_MQTT_PKT_CONNECT, header.type);
    TEST_ASSERT_EQUAL(header.header_size + header.remaining_length, g_written_total);
}

void test_aemlib_poll_processes_multiple_packets_from_one_read(void) {
    g_message_count = 0;
    g_read_buf_offset = 0;
    g_read_chunk_size = 0; /* deliver both packets in a single read() */

    size_t off = 0;
    size_t out_len = 0;
    aemlib_proto_encode_publish(g_read_buf, sizeof(g_read_buf), &out_len, "topic/a", (const uint8_t *)"AAA", 3);
    off += out_len;
    aemlib_proto_encode_publish(g_read_buf + off, sizeof(g_read_buf) - off, &out_len, "topic/b", (const uint8_t *)"BBBB", 4);
    off += out_len;
    g_read_buf_len = off;

    uint8_t tx_buf[64];
    uint8_t rx_buf[128];
    aemlib_client_t client;
    aemlib_config_t config = {
        .tx_buffer = tx_buf,
        .tx_buffer_size = sizeof(tx_buf),
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect = client_mock_transport_connect,
            .disconnect = client_mock_transport_disconnect,
            .read = client_mock_transport_read_from_buf,
            .write = client_mock_transport_write_counting,
            .ctx = NULL
        },
        .time = {
            .now_ms = client_mock_time_now,
            .ctx = NULL
        },
        .keepalive_interval_ms = 60000,
        .on_message = client_mock_on_message
    };

    aemlib_status_t status = aemlib_init(&client, &config);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    client.state = AEMLIB_STATE_MQTT_CONNECTED;
    client.last_activity_ms = 1000;

    // AEM-7: a single read() containing two coalesced PUBLISH packets used to
    // only process the first, silently dropping the second.
    status = aemlib_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);

    TEST_ASSERT_EQUAL(2, g_message_count);
    TEST_ASSERT_EQUAL_STRING("topic/b", g_last_topic);
    TEST_ASSERT_EQUAL_STRING("BBBB", g_last_payload);
    TEST_ASSERT_EQUAL(0, client.rx_len);
}

void test_aemlib_poll_reassembles_packet_split_across_reads(void) {
    g_message_count = 0;
    g_read_buf_offset = 0;
    g_read_chunk_size = 3; /* deliver only 3 bytes per read(), forcing reassembly */

    size_t out_len = 0;
    aemlib_proto_encode_publish(g_read_buf, sizeof(g_read_buf), &out_len, "t", (const uint8_t *)"hello", 5);
    g_read_buf_len = out_len;

    uint8_t tx_buf[64];
    uint8_t rx_buf[128];
    aemlib_client_t client;
    aemlib_config_t config = {
        .tx_buffer = tx_buf,
        .tx_buffer_size = sizeof(tx_buf),
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect = client_mock_transport_connect,
            .disconnect = client_mock_transport_disconnect,
            .read = client_mock_transport_read_from_buf,
            .write = client_mock_transport_write_counting,
            .ctx = NULL
        },
        .time = {
            .now_ms = client_mock_time_now,
            .ctx = NULL
        },
        .keepalive_interval_ms = 60000,
        .on_message = client_mock_on_message
    };

    aemlib_status_t status = aemlib_init(&client, &config);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    client.state = AEMLIB_STATE_MQTT_CONNECTED;
    client.last_activity_ms = 1000;

    // First poll only has 3 of the packet's bytes; must not fire early.
    status = aemlib_poll(&client);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(0, g_message_count);

    for (int i = 0; i < 10 && g_message_count == 0; i++) {
        status = aemlib_poll(&client);
        TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    }

    TEST_ASSERT_EQUAL(1, g_message_count);
    TEST_ASSERT_EQUAL_STRING("t", g_last_topic);
    TEST_ASSERT_EQUAL_STRING("hello", g_last_payload);
    TEST_ASSERT_EQUAL(0, client.rx_len);
}
