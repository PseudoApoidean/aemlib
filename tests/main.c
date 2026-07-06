#include "unity.h"

// Test files
#include "test_core.c"
#include "test_client.c"
#include "test_proto.c"
#include "test_virtual_transport.c"

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
    RUN_TEST(test_aemlib_core_connect_invalid_transitions);
    RUN_TEST(test_aemlib_core_disconnect_invalid_transitions);
    RUN_TEST(test_aemlib_core_poll_invalid_state);
    RUN_TEST(test_aemlib_core_state_machine_connect_flow);
    RUN_TEST(test_aemlib_core_connect_sent_connack_rejected);
    RUN_TEST(test_aemlib_core_connect_sent_connack_timeout);
    RUN_TEST(test_aemlib_core_connect_sent_preserves_bytes_after_connack);

    // Client tests
    RUN_TEST(test_aemlib_poll_sends_connect_only_once_while_waiting_for_connack);
    RUN_TEST(test_aemlib_poll_retries_partial_write_without_data_loss);
    RUN_TEST(test_aemlib_poll_processes_multiple_packets_from_one_read);
    RUN_TEST(test_aemlib_poll_reassembles_packet_split_across_reads);

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
    RUN_TEST(test_aemlib_proto_encode_connect_buffer_too_small);
    RUN_TEST(test_aemlib_proto_encode_publish_buffer_too_small);
    RUN_TEST(test_aemlib_proto_encode_subscribe_buffer_too_small);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_connect);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_pingreq);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_invalid_flags_connect);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_invalid_flags_pingreq);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_invalid_flags_subscribe);
    RUN_TEST(test_aemlib_proto_decode_fixed_header_invalid_packet_type);
    RUN_TEST(test_aemlib_proto_decode_publish_normal);
    RUN_TEST(test_aemlib_proto_decode_publish_truncated_topic);
    RUN_TEST(test_aemlib_proto_decode_publish_qos1_unsupported);
    RUN_TEST(test_aemlib_proto_decode_connack);
    RUN_TEST(test_aemlib_proto_decode_suback);

    // Virtual transport tests
    RUN_TEST(test_aemlib_virtual_transport_a_to_b);
    RUN_TEST(test_aemlib_virtual_transport_wraparound);
    RUN_TEST(test_aemlib_virtual_transport_write_full_would_block);
    RUN_TEST(test_aemlib_virtual_transport_disconnected_read_write);

    return UNITY_END();
}
