#include "unity.h"

// Test files
#include "test_core.c"
#include "test_proto.c"

int main(void) {
    UNITY_BEGIN();

    // Core tests
    RUN_TEST(test_aemlib_core_init_null_client);
    RUN_TEST(test_aemlib_core_init_null_config);
    RUN_TEST(test_aemlib_core_init_invalid_config);
    RUN_TEST(test_aemlib_core_init_valid_config);
    RUN_TEST(test_aemlib_core_poll_null_client);
    RUN_TEST(test_aemlib_core_poll_disconnected_state);
    RUN_TEST(test_aemlib_core_connect_null_client);
    RUN_TEST(test_aemlib_core_connect_from_disconnected);
    RUN_TEST(test_aemlib_core_connect_invalid_state);
    RUN_TEST(test_aemlib_core_disconnect_null_client);
    RUN_TEST(test_aemlib_core_disconnect_from_connected);
    RUN_TEST(test_aemlib_core_disconnect_invalid_state);
    RUN_TEST(test_aemlib_core_next_packet_id_null_client);
    RUN_TEST(test_aemlib_core_next_packet_id_normal);
    RUN_TEST(test_aemlib_core_next_packet_id_wraparound);

    // Proto tests
    RUN_TEST(test_aemlib_proto_encode_remaining_length_zero);
    RUN_TEST(test_aemlib_proto_encode_remaining_length_small);
    RUN_TEST(test_aemlib_proto_encode_remaining_length_medium);
    RUN_TEST(test_aemlib_proto_encode_remaining_length_large);
    RUN_TEST(test_aemlib_proto_encode_remaining_length_buffer_too_small);
    RUN_TEST(test_aemlib_proto_decode_remaining_length_zero);
    RUN_TEST(test_aemlib_proto_decode_remaining_length_small);
    RUN_TEST(test_aemlib_proto_decode_remaining_length_medium);
    RUN_TEST(test_aemlib_proto_decode_remaining_length_large);
    RUN_TEST(test_aemlib_proto_decode_remaining_length_incomplete);
    RUN_TEST(test_aemlib_proto_write_utf8_normal);
    RUN_TEST(test_aemlib_proto_write_utf8_buffer_too_small);
    RUN_TEST(test_aemlib_proto_encode_pingreq);
    RUN_TEST(test_aemlib_proto_encode_disconnect);
    RUN_TEST(test_aemlib_proto_encode_connect);
    RUN_TEST(test_aemlib_proto_encode_publish);
    RUN_TEST(test_aemlib_proto_encode_subscribe);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_connect);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_pingreq);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_invalid_flags_connect);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_invalid_flags_pingreq);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_invalid_flags_subscribe);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_invalid_packet_type);
    RUN_TEST(test_aemlib_proto_decode_connack);
    RUN_TEST(test_aemlib_proto_decode_suback);

    return UNITY_END();
}