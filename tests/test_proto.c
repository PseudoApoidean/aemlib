#include "unity.h"
#include "proto.h"
#include "aemlib/status.h"
#include <string.h>

// Test aemlib_proto_encode_remaining_length
void test_aemlib_proto_encode_remaining_length_zero(void) {
    uint8_t buf[4];
    size_t out_len;
    aemlib_status_t status = aemlib_proto_encode_remaining_length(0, buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(1, out_len);
    TEST_ASSERT_EQUAL(0, buf[0]);
}

void test_aemlib_proto_encode_remaining_length_small(void) {
    uint8_t buf[4];
    size_t out_len;
    aemlib_status_t status = aemlib_proto_encode_remaining_length(127, buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(1, out_len);
    TEST_ASSERT_EQUAL(127, buf[0]);
}

void test_aemlib_proto_encode_remaining_length_medium(void) {
    uint8_t buf[4];
    size_t out_len;
    aemlib_status_t status = aemlib_proto_encode_remaining_length(128, buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(2, out_len);
    TEST_ASSERT_EQUAL(0x80, buf[0]);
    TEST_ASSERT_EQUAL(0x01, buf[1]);
}

void test_aemlib_proto_encode_remaining_length_large(void) {
    uint8_t buf[4];
    size_t out_len;
    aemlib_status_t status = aemlib_proto_encode_remaining_length(2097151, buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(3, out_len);
    TEST_ASSERT_EQUAL(0xFF, buf[0]);
    TEST_ASSERT_EQUAL(0xFF, buf[1]);
    TEST_ASSERT_EQUAL(0x7F, buf[2]);
}

void test_aemlib_proto_encode_remaining_length_buffer_too_small(void) {
    uint8_t buf[1];
    size_t out_len;
    aemlib_status_t status = aemlib_proto_encode_remaining_length(2097151, buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL), status);
}

// Test aemlib_proto_decode_remaining_length
void test_aemlib_proto_decode_remaining_length_zero(void) {
    uint8_t buf[] = {0};
    uint32_t value;
    size_t consumed;
    aemlib_status_t status = aemlib_proto_decode_remaining_length(buf, sizeof(buf), &value, &consumed);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(0, value);
    TEST_ASSERT_EQUAL(1, consumed);
}

void test_aemlib_proto_decode_remaining_length_small(void) {
    uint8_t buf[] = {127};
    uint32_t value;
    size_t consumed;
    aemlib_status_t status = aemlib_proto_decode_remaining_length(buf, sizeof(buf), &value, &consumed);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(127, value);
    TEST_ASSERT_EQUAL(1, consumed);
}

void test_aemlib_proto_decode_remaining_length_medium(void) {
    uint8_t buf[] = {0x80, 0x01};
    uint32_t value;
    size_t consumed;
    aemlib_status_t status = aemlib_proto_decode_remaining_length(buf, sizeof(buf), &value, &consumed);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(128, value);
    TEST_ASSERT_EQUAL(2, consumed);
}

void test_aemlib_proto_decode_remaining_length_large(void) {
    uint8_t buf[] = {0xFF, 0xFF, 0x7F};
    uint32_t value;
    size_t consumed;
    aemlib_status_t status = aemlib_proto_decode_remaining_length(buf, sizeof(buf), &value, &consumed);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(2097151, value);
    TEST_ASSERT_EQUAL(3, consumed);
}

void test_aemlib_proto_decode_remaining_length_incomplete(void) {
    uint8_t buf[] = {0x80};
    uint32_t value;
    size_t consumed;
    aemlib_status_t status = aemlib_proto_decode_remaining_length(buf, sizeof(buf), &value, &consumed);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL), status);
}

// Test aemlib_proto_write_utf8
void test_aemlib_proto_write_utf8_normal(void) {
    uint8_t buf[10];
    size_t offset = 0;
    const char *str = "hello";
    aemlib_status_t status = aemlib_proto_write_utf8(buf, sizeof(buf), &offset, str);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(7,   offset);
    TEST_ASSERT_EQUAL(0,   buf[0]); // length MSB
    TEST_ASSERT_EQUAL(5,   buf[1]); // length LSB
    TEST_ASSERT_EQUAL('h', buf[2]);
    TEST_ASSERT_EQUAL('e', buf[3]);
    TEST_ASSERT_EQUAL('l', buf[4]);
    TEST_ASSERT_EQUAL('l', buf[5]);
    TEST_ASSERT_EQUAL('o', buf[6]);
}

void test_aemlib_proto_write_utf8_buffer_too_small(void) {
    uint8_t buf[5];
    size_t offset = 0;
    const char *str = "hello";
    aemlib_status_t status = aemlib_proto_write_utf8(buf, sizeof(buf), &offset, str);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL), status);
}

// Test aemlib_proto_encode_pingreq
void test_aemlib_proto_encode_pingreq(void) {
    uint8_t buf[10];
    size_t out_len;
    aemlib_status_t status = aemlib_proto_encode_pingreq(buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(2, out_len);
    TEST_ASSERT_EQUAL(0xC0, buf[0]); // PINGREQ type and flags
    TEST_ASSERT_EQUAL(0x00, buf[1]); // remaining length
}

// Test aemlib_proto_encode_disconnect
void test_aemlib_proto_encode_disconnect(void) {
    uint8_t buf[10];
    size_t out_len;
    aemlib_status_t status = aemlib_proto_encode_disconnect(buf, sizeof(buf), &out_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(2, out_len);
    TEST_ASSERT_EQUAL(0xE0, buf[0]); // DISCONNECT type and flags
    TEST_ASSERT_EQUAL(0x00, buf[1]); // remaining length
}

// Test aemlib_proto_encode_connect
void test_aemlib_proto_encode_connect(void) {
    uint8_t buf[50];
    size_t out_len;
    const char *client_id = "test_client";
    uint16_t keepalive = 60;
    aemlib_status_t status = aemlib_proto_encode_connect(buf, sizeof(buf), &out_len, client_id, keepalive);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(25,   out_len);
    TEST_ASSERT_EQUAL(0x10, buf[0]); // CONNECT packet type and flags
    TEST_ASSERT_EQUAL(23,   buf[1]); // remaining length
    TEST_ASSERT_EQUAL(0x00, buf[2]);
    TEST_ASSERT_EQUAL(0x04, buf[3]);
    TEST_ASSERT_EQUAL('M',  buf[4]);
    TEST_ASSERT_EQUAL('Q',  buf[5]);
    TEST_ASSERT_EQUAL('T',  buf[6]);
    TEST_ASSERT_EQUAL('T',  buf[7]);
    TEST_ASSERT_EQUAL(0x04, buf[8]);
    TEST_ASSERT_EQUAL(0x02, buf[9]);
    TEST_ASSERT_EQUAL(0x00, buf[10]);
    TEST_ASSERT_EQUAL(0x3C, buf[11]);
    TEST_ASSERT_EQUAL(0x00, buf[12]);
    TEST_ASSERT_EQUAL(0x0B, buf[13]);
    TEST_ASSERT_EQUAL('t',  buf[14]);
    TEST_ASSERT_EQUAL('e',  buf[15]);
    TEST_ASSERT_EQUAL('s',  buf[16]);
    TEST_ASSERT_EQUAL('t',  buf[17]);
    TEST_ASSERT_EQUAL('_',  buf[18]);
    TEST_ASSERT_EQUAL('c',  buf[19]);
    TEST_ASSERT_EQUAL('l',  buf[20]);
    TEST_ASSERT_EQUAL('i',  buf[21]);
    TEST_ASSERT_EQUAL('e',  buf[22]);
    TEST_ASSERT_EQUAL('n',  buf[23]);
    TEST_ASSERT_EQUAL('t',  buf[24]);
}

// Test aemlib_proto_encode_publish
void test_aemlib_proto_encode_publish(void) {
    uint8_t buf[50];
    size_t out_len;
    const char *topic = "test/topic";
    const uint8_t payload[] = "hello";
    size_t payload_len = 5;
    aemlib_status_t status = aemlib_proto_encode_publish(buf, sizeof(buf), &out_len, topic, payload, payload_len);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(19,   out_len);
    TEST_ASSERT_EQUAL(0x30, buf[0]); // PUBLISH packet type and flags
    TEST_ASSERT_EQUAL(17,   buf[1]); // remaining length
    TEST_ASSERT_EQUAL(0x00, buf[2]);
    TEST_ASSERT_EQUAL(0x0A, buf[3]);
    TEST_ASSERT_EQUAL('t',  buf[4]);
    TEST_ASSERT_EQUAL('e',  buf[5]);
    TEST_ASSERT_EQUAL('s',  buf[6]);
    TEST_ASSERT_EQUAL('t',  buf[7]);
    TEST_ASSERT_EQUAL('/',  buf[8]);
    TEST_ASSERT_EQUAL('t',  buf[9]);
    TEST_ASSERT_EQUAL('o',  buf[10]);
    TEST_ASSERT_EQUAL('p',  buf[11]);
    TEST_ASSERT_EQUAL('i',  buf[12]);
    TEST_ASSERT_EQUAL('c',  buf[13]);
    TEST_ASSERT_EQUAL('h',  buf[14]);
    TEST_ASSERT_EQUAL('e',  buf[15]);
    TEST_ASSERT_EQUAL('l',  buf[16]);
    TEST_ASSERT_EQUAL('l',  buf[17]);
    TEST_ASSERT_EQUAL('o',  buf[18]);
}

// Test aemlib_proto_encode_subscribe
void test_aemlib_proto_encode_subscribe(void) {
    uint8_t buf[50];
    size_t out_len;
    uint16_t packet_id = 1;
    const char *topic = "test/topic";
    aemlib_status_t status = aemlib_proto_encode_subscribe(buf, sizeof(buf), &out_len, packet_id, topic);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(17,   out_len);
    TEST_ASSERT_EQUAL(0x82, buf[0]); // SUBSCRIBE packet type and flags
    TEST_ASSERT_EQUAL(15,   buf[1]); // remaining length
    TEST_ASSERT_EQUAL(0x00, buf[2]);
    TEST_ASSERT_EQUAL(0x01, buf[3]);
    TEST_ASSERT_EQUAL(0x00, buf[4]);
    TEST_ASSERT_EQUAL(0x0A, buf[5]);
    TEST_ASSERT_EQUAL('t',  buf[6]);
    TEST_ASSERT_EQUAL('e',  buf[7]);
    TEST_ASSERT_EQUAL('s',  buf[8]);
    TEST_ASSERT_EQUAL('t',  buf[9]);
    TEST_ASSERT_EQUAL('/',  buf[10]);
    TEST_ASSERT_EQUAL('t',  buf[11]);
    TEST_ASSERT_EQUAL('o',  buf[12]);
    TEST_ASSERT_EQUAL('p',  buf[13]);
    TEST_ASSERT_EQUAL('i',  buf[14]);
    TEST_ASSERT_EQUAL('c',  buf[15]);
    TEST_ASSERT_EQUAL(0x00, buf[16]);
}

// Test aemlib_proto_decode_fixed_header
void test_aemlib_proto_decode_fixed_header_connect(void) {
    uint8_t buf[] = {0x10, 0x00}; // CONNECT packet, remaining length 0
    aemlib_mqtt_fixed_header_t header;
    aemlib_status_t status = aemlib_proto_decode_fixed_header(buf, sizeof(buf), &header);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_MQTT_PKT_CONNECT, header.type);
    TEST_ASSERT_EQUAL(0, header.flags);
    TEST_ASSERT_EQUAL(0, header.remaining_length);
    TEST_ASSERT_EQUAL(2, header.header_size);
}

void test_aemlib_proto_decode_fixed_header_pingreq(void) {
    uint8_t buf[] = {0xC0, 0x00}; // PINGREQ packet
    aemlib_mqtt_fixed_header_t header;
    aemlib_status_t status = aemlib_proto_decode_fixed_header(buf, sizeof(buf), &header);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(AEMLIB_MQTT_PKT_PINGREQ, header.type);
    TEST_ASSERT_EQUAL(0, header.flags);
    TEST_ASSERT_EQUAL(0, header.remaining_length);
    TEST_ASSERT_EQUAL(2, header.header_size);
}

// Test aemlib_proto_decode_connack
void test_aemlib_proto_decode_connack(void) {
    uint8_t buf[] = {0x20, 0x02, 0x00, 0x00}; // CONNACK packet
    uint8_t session_present, return_code;
    aemlib_status_t status = aemlib_proto_decode_connack(buf, sizeof(buf), &session_present, &return_code);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(0, session_present);
    TEST_ASSERT_EQUAL(0, return_code);
}

// Test aemlib_proto_decode_suback
void test_aemlib_proto_decode_suback(void) {
    uint8_t buf[] = {0x90, 0x03, 0x00, 0x01, 0x00}; // SUBACK packet
    uint16_t packet_id;
    uint8_t granted_qos;
    aemlib_status_t status = aemlib_proto_decode_suback(buf, sizeof(buf), &packet_id, &granted_qos);
    TEST_ASSERT_EQUAL(AEMLIB_STATUS_OK, status);
    TEST_ASSERT_EQUAL(1, packet_id);
    TEST_ASSERT_EQUAL(0, granted_qos);
}