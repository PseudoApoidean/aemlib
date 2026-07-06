#include "unity.h"
#include "virtual_transport.h"
#include "aemlib/status.h"
#include <string.h>

// Test aemlib_virtual_transport basic loopback
void test_aemlib_virtual_transport_a_to_b(void) {
    uint8_t chan_buf_ab[8];
    uint8_t chan_buf_ba[8];
    aemlib_virtual_channel_t chan_ab, chan_ba;
    aemlib_virtual_channel_init(&chan_ab, chan_buf_ab, sizeof(chan_buf_ab));
    aemlib_virtual_channel_init(&chan_ba, chan_buf_ba, sizeof(chan_buf_ba));

    aemlib_virtual_transport_t vt_a, vt_b;
    aemlib_virtual_transport_pair_init(&vt_a, &chan_ab, &vt_b, &chan_ba);

    aemlib_transport_t a, b;
    aemlib_virtual_transport_bind(&a, &vt_a);
    aemlib_virtual_transport_bind(&b, &vt_b);

    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, aemlib_transport_connect(&a));
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, aemlib_transport_connect(&b));

    const uint8_t msg[] = {1, 2, 3};
    size_t written = 0;
    aemlib_status_t status = aemlib_transport_write(&a, msg, sizeof(msg), &written);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(3, written);

    uint8_t out[8] = {0};
    size_t read = 0;
    status = aemlib_transport_read(&b, out, sizeof(out), &read);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(3, read);
    TEST_ASSERT_EQUAL_MEMORY(msg, out, 3);

    // Nothing left to read on b, and a hasn't received anything from b
    read = 0;
    status = aemlib_transport_read(&b, out, sizeof(out), &read);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(0, read);
}

void test_aemlib_virtual_transport_wraparound(void) {
    uint8_t chan_buf_ab[4];
    uint8_t chan_buf_ba[4];
    aemlib_virtual_channel_t chan_ab, chan_ba;
    aemlib_virtual_channel_init(&chan_ab, chan_buf_ab, sizeof(chan_buf_ab));
    aemlib_virtual_channel_init(&chan_ba, chan_buf_ba, sizeof(chan_buf_ba));

    aemlib_virtual_transport_t vt_a, vt_b;
    aemlib_virtual_transport_pair_init(&vt_a, &chan_ab, &vt_b, &chan_ba);

    aemlib_transport_t a, b;
    aemlib_virtual_transport_bind(&a, &vt_a);
    aemlib_virtual_transport_bind(&b, &vt_b);
    aemlib_transport_connect(&a);
    aemlib_transport_connect(&b);

    // Fill, drain partially, then refill to force the ring buffer to wrap
    const uint8_t first[] = {1, 2, 3};
    size_t written = 0;
    aemlib_transport_write(&a, first, sizeof(first), &written);
    TEST_ASSERT_EQUAL(3, written);

    uint8_t out[4] = {0};
    size_t read = 0;
    aemlib_transport_read(&b, out, 2, &read);
    TEST_ASSERT_EQUAL(2, read);

    const uint8_t second[] = {4, 5, 6};
    aemlib_transport_write(&a, second, sizeof(second), &written);
    TEST_ASSERT_EQUAL(3, written);

    read = 0;
    aemlib_status_t status = aemlib_transport_read(&b, out, sizeof(out), &read);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(4, read);
    TEST_ASSERT_EQUAL(3, out[0]); // last byte left over from "first"
    TEST_ASSERT_EQUAL(4, out[1]);
    TEST_ASSERT_EQUAL(5, out[2]);
    TEST_ASSERT_EQUAL(6, out[3]);
}

void test_aemlib_virtual_transport_write_full_would_block(void) {
    uint8_t chan_buf_ab[2];
    uint8_t chan_buf_ba[2];
    aemlib_virtual_channel_t chan_ab, chan_ba;
    aemlib_virtual_channel_init(&chan_ab, chan_buf_ab, sizeof(chan_buf_ab));
    aemlib_virtual_channel_init(&chan_ba, chan_buf_ba, sizeof(chan_buf_ba));

    aemlib_virtual_transport_t vt_a, vt_b;
    aemlib_virtual_transport_pair_init(&vt_a, &chan_ab, &vt_b, &chan_ba);

    aemlib_transport_t a;
    aemlib_virtual_transport_bind(&a, &vt_a);
    aemlib_transport_connect(&a);

    const uint8_t msg[] = {1, 2};
    size_t written = 0;
    aemlib_transport_write(&a, msg, sizeof(msg), &written);
    TEST_ASSERT_EQUAL(2, written);

    // Channel is now full; a further write can accept nothing
    written = 0;
    aemlib_status_t status = aemlib_transport_write(&a, msg, sizeof(msg), &written);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_WOULD_BLOCK), status);
    TEST_ASSERT_EQUAL(0, written);
}

void test_aemlib_virtual_transport_disconnected_read_write(void) {
    uint8_t chan_buf_ab[4];
    uint8_t chan_buf_ba[4];
    aemlib_virtual_channel_t chan_ab, chan_ba;
    aemlib_virtual_channel_init(&chan_ab, chan_buf_ab, sizeof(chan_buf_ab));
    aemlib_virtual_channel_init(&chan_ba, chan_buf_ba, sizeof(chan_buf_ba));

    aemlib_virtual_transport_t vt_a, vt_b;
    aemlib_virtual_transport_pair_init(&vt_a, &chan_ab, &vt_b, &chan_ba);

    aemlib_transport_t a;
    aemlib_virtual_transport_bind(&a, &vt_a);
    // Not connected yet

    uint8_t buf[4];
    size_t out_len = 0;
    aemlib_status_t status = aemlib_transport_read(&a, buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CLOSED), status);

    status = aemlib_transport_write(&a, buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CLOSED), status);
}
