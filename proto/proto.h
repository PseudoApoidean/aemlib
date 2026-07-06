#ifndef AEMLIB_PROTO_H
#define AEMLIB_PROTO_H

#include "aemlib/status.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ------------------------

// Remaining Length Encoding Constants
/** @brief Base value for MQTT remaining length variable-length encoding */
#define AEMLIB_MQTT_REMAINING_LENGTH_BASE 128
/** @brief Bitmask to extract the lower 7 bits of a remaining length byte */
#define AEMLIB_MQTT_REMAINING_LENGTH_MASK 0x7F
/** @brief Continuation bit indicating more remaining length bytes follow */
#define AEMLIB_MQTT_REMAINING_LENGTH_CONTINUATION 0x80
/** @brief Maximum multiplier value for remaining length decoding (prevents overflow) */
#define AEMLIB_MQTT_MAX_REMAINING_LENGTH_MULTIPLIER (AEMLIB_MQTT_REMAINING_LENGTH_BASE * AEMLIB_MQTT_REMAINING_LENGTH_BASE * AEMLIB_MQTT_REMAINING_LENGTH_BASE)
/** @brief Maximum number of bytes needed to encode any remaining length value */
#define AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES 4

// Fixed Header Constants
/** @brief Minimum size of an MQTT fixed header in bytes */
#define AEMLIB_MQTT_FIXED_HEADER_MIN_SIZE 2
/** @brief Bit shift to extract packet type from the first byte of fixed header */
#define AEMLIB_MQTT_PACKET_TYPE_SHIFT 4
/** @brief Bitmask to extract flags from the first byte of fixed header */
#define AEMLIB_MQTT_FLAGS_MASK 0x0F

// UTF-8 String Constants
/** @brief Size of the length prefix for UTF-8 encoded strings in MQTT packets */
#define AEMLIB_MQTT_UTF8_LENGTH_PREFIX_SIZE 2

// CONNECT Packet Constants
/** @brief MQTT protocol level for version 3.1.1 */
#define AEMLIB_MQTT_PROTOCOL_LEVEL 4
/** @brief Connect flag indicating a clean session should be started */
#define AEMLIB_MQTT_CONNECT_CLEAN_SESSION 0x02
/** @brief Number of bytes used to encode the keep-alive interval */
#define AEMLIB_MQTT_KEEPALIVE_BYTES 2

// General Byte Operation Constants
/** @brief Bit shift for extracting the high byte from a 16-bit value */
#define AEMLIB_MQTT_BYTE_SHIFT 8
/** @brief Bitmask to extract the low byte from a 16-bit value */
#define AEMLIB_MQTT_LOW_BYTE_MASK 0xFF

// PINGREQ Packet Constants
/** @brief Total length of a PINGREQ packet in bytes */
#define AEMLIB_MQTT_PINGREQ_LENGTH 2
/** @brief Remaining length field value for PINGREQ packets (always 0) */
#define AEMLIB_MQTT_PINGREQ_REMAINING_LENGTH 0

// DISCONNECT Packet Constants
/** @brief Total length of a DISCONNECT packet in bytes */
#define AEMLIB_MQTT_DISCONNECT_LENGTH 2
/** @brief Remaining length field value for DISCONNECT packets (always 0) */
#define AEMLIB_MQTT_DISCONNECT_REMAINING_LENGTH 0

// PUBLISH Packet Constants
/** @brief QoS 0 flags for PUBLISH packets */
#define AEMLIB_MQTT_PUBLISH_FLAGS_QOS0 0x00

// SUBSCRIBE Packet Constants
/** @brief Required flags for SUBSCRIBE packets (reserved bits = 0b0010) */
#define AEMLIB_MQTT_SUBSCRIBE_FLAGS 0x02
/** @brief QoS value in SUBSCRIBE payload (QoS 0) */
#define AEMLIB_MQTT_SUBSCRIBE_PAYLOAD_QOS 0
/** @brief Size of packet ID field in bytes */
#define AEMLIB_MQTT_PACKET_ID_SIZE 2

// CONNACK Packet Constants
/** @brief Minimum size of CONNACK packet in bytes */
#define AEMLIB_MQTT_CONNACK_MIN_SIZE 4
/** @brief Bitmask for session present flag in CONNACK */
#define AEMLIB_MQTT_CONNACK_SESSION_PRESENT_MASK 0x01
/** @brief Index of session present flags byte in CONNACK packet */
#define AEMLIB_MQTT_CONNACK_SESSION_BYTE_INDEX 2
/** @brief Index of return code byte in CONNACK packet */
#define AEMLIB_MQTT_CONNACK_RETURN_CODE_INDEX 3

// SUBACK Packet Constants
/** @brief Minimum size of SUBACK packet in bytes */
#define AEMLIB_MQTT_SUBACK_MIN_SIZE 5
/** @brief Index of first byte of packet ID in SUBACK packet */
#define AEMLIB_MQTT_SUBACK_PACKET_ID_BYTE_1 2
/** @brief Index of second byte of packet ID in SUBACK packet */
#define AEMLIB_MQTT_SUBACK_PACKET_ID_BYTE_2 3
/** @brief Index of granted QoS byte in SUBACK packet */
#define AEMLIB_MQTT_SUBACK_GRANTED_QOS_INDEX 4

/* MQTT Control Packet Types (MQTT 3.1.1) */
typedef enum {
    AEMLIB_MQTT_PKT_CONNECT     = 1,
    AEMLIB_MQTT_PKT_CONNACK     = 2,
    AEMLIB_MQTT_PKT_PUBLISH     = 3,
    AEMLIB_MQTT_PKT_PUBACK      = 4,
    AEMLIB_MQTT_PKT_SUBSCRIBE   = 8,
    AEMLIB_MQTT_PKT_SUBACK      = 9,
    AEMLIB_MQTT_PKT_PINGREQ     = 12,
    AEMLIB_MQTT_PKT_PINGRESP    = 13,
    AEMLIB_MQTT_PKT_DISCONNECT  = 14
} aemlib_mqtt_packet_type_t;

/* Parsed fixed header */
typedef struct {
    aemlib_mqtt_packet_type_t type;
    uint8_t flags;
    uint32_t remaining_length;
    size_t header_size; /* bytes consumed by fixed header */
} aemlib_mqtt_fixed_header_t;

// DECLARATIONS -----------------------

/* Encode CONNECT packet */
aemlib_status_t aemlib_proto_encode_connect(uint8_t *buf,
                                            size_t buf_len,
                                            size_t *out_len,
                                            const char *client_id,
                                            uint16_t keepalive_sec);

/* Encode PINGREQ */
aemlib_status_t aemlib_proto_encode_pingreq(uint8_t *buf,
                                            size_t buf_len,
                                            size_t *out_len);

/* Encode DISCONNECT */
aemlib_status_t aemlib_proto_encode_disconnect(uint8_t *buf,
                                               size_t buf_len,
                                               size_t *out_len);

/* Encode PUBLISH (QoS 0 only for now) */
aemlib_status_t aemlib_proto_encode_publish(uint8_t *buf,
                                            size_t buf_len,
                                            size_t *out_len,
                                            const char *topic,
                                            const uint8_t *payload,
                                            size_t payload_len);

/* Encode SUBSCRIBE (single topic, QoS 0 for now) */
aemlib_status_t aemlib_proto_encode_subscribe(uint8_t *buf,
                                              size_t buf_len,
                                              size_t *out_len,
                                              uint16_t packet_id,
                                              const char *topic);

/* Decode fixed header */
aemlib_status_t aemlib_proto_decode_fixed_header(const uint8_t *buf,
                                                 size_t buf_len,
                                                 aemlib_mqtt_fixed_header_t *out);

/* Decode PUBLISH (QoS 0 only for now). buf/buf_len is the whole packet
 * (fixed header included); topic/payload are returned as pointers into buf. */
aemlib_status_t aemlib_proto_decode_publish(const uint8_t *buf,
                                            size_t buf_len,
                                            const aemlib_mqtt_fixed_header_t *header,
                                            const char **topic,
                                            size_t *topic_len,
                                            const uint8_t **payload,
                                            size_t *payload_len);

/* Decode CONNACK */
aemlib_status_t aemlib_proto_decode_connack(const uint8_t *buf,
                                            size_t buf_len,
                                            uint8_t *session_present,
                                            uint8_t *return_code);

/* Decode SUBACK (single topic) */
aemlib_status_t aemlib_proto_decode_suback(const uint8_t *buf,
                                           size_t buf_len,
                                           uint16_t *packet_id,
                                           uint8_t *granted_qos);

// HELPERS ----------------------------

/* Encode MQTT Remaining Length */
aemlib_status_t aemlib_proto_encode_remaining_length(uint32_t value,
                                                     uint8_t *buf,
                                                     size_t buf_len,
                                                     size_t *out_len);

/* Decode MQTT Remaining Length */
aemlib_status_t aemlib_proto_decode_remaining_length(const uint8_t *buf,
                                                     size_t buf_len,
                                                     uint32_t *value,
                                                     size_t *consumed);

/* Write UTF-8 string (2-byte length prefix) */
aemlib_status_t aemlib_proto_write_utf8(uint8_t *buf,
                                        size_t buf_len,
                                        size_t *offset,
                                        const char *str);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_PROTO_H */
