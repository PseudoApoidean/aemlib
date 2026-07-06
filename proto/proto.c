#include "proto.h"
#include <string.h>

// DEFINITIONS ---------------------------------------------------------------

// INTERNAL HELPERS ----------------------------------------------------------

static inline aemlib_status_t ensure_space(size_t needed,
                                           size_t offset,
                                           size_t buf_len)
{
    if (offset + needed > buf_len) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
    }
    return AEMLIB_STATUS_OK;
}

static inline aemlib_status_t reserve_space(size_t *offset,
                                            size_t size,
                                            size_t buf_len)
{
    aemlib_status_t status = ensure_space(size, *offset, buf_len);
    AEMLIB_CHECK_STATUS(status);
    *offset += size;
    return AEMLIB_STATUS_OK;
}

static inline aemlib_status_t append_byte(uint8_t *buf,
                                          size_t *offset,
                                          size_t buf_len,
                                          uint8_t value)
{
    aemlib_status_t status = ensure_space(1, *offset, buf_len);
    AEMLIB_CHECK_STATUS(status);
    buf[(*offset)++] = value;
    return AEMLIB_STATUS_OK;
}

static inline aemlib_status_t append_uint16_be(uint8_t *buf,
                                               size_t *offset,
                                               size_t buf_len,
                                               uint16_t value)
{
    aemlib_status_t status = ensure_space(2, *offset, buf_len);
    AEMLIB_CHECK_STATUS(status);
    buf[(*offset)++] = (uint8_t)(value >> AEMLIB_MQTT_BYTE_SHIFT);
    buf[(*offset)++] = (uint8_t)(value & AEMLIB_MQTT_LOW_BYTE_MASK);
    return AEMLIB_STATUS_OK;
}

/* Validate MQTT packet flags according to MQTT 3.1.1 specification */
static aemlib_status_t validate_mqtt_flags(aemlib_mqtt_packet_type_t type, uint8_t flags)
{
    switch (type) {
    case AEMLIB_MQTT_PKT_CONNECT:
    case AEMLIB_MQTT_PKT_CONNACK:
    case AEMLIB_MQTT_PKT_PUBACK:
    case AEMLIB_MQTT_PKT_SUBACK:
    case AEMLIB_MQTT_PKT_PINGREQ:
    case AEMLIB_MQTT_PKT_PINGRESP:
    case AEMLIB_MQTT_PKT_DISCONNECT:
        if (flags != 0x00) {
            return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_PROTOCOL);
        }
        break;

    case AEMLIB_MQTT_PKT_PUBLISH: {
        /* DUP (bit 3) and RETAIN (bit 0) are independent of QoS and may be
         * legitimately set by the broker (e.g. retained messages); only the
         * QoS bits (2-1) are constrained here, since 0b11 is reserved. */
        uint8_t qos = (flags >> 1) & 0x03;
        if (qos == 0x03) {
            return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_PROTOCOL);
        }
        break;
    }

    case AEMLIB_MQTT_PKT_SUBSCRIBE:
        if (flags != 0x02) {
            return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_PROTOCOL);
        }
        break;

    default:
        // Unknown packet type - this is also invalid
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_PROTOCOL);
    }

    return AEMLIB_STATUS_OK;
}

// REMAINING LENGTH ----------------------------------------------------------

aemlib_status_t aemlib_proto_encode_remaining_length(uint32_t value,
                                                     uint8_t *buf,
                                                     size_t buf_len,
                                                     size_t *out_len)
{
    size_t i = 0;

    do {
        if (i >= buf_len) {
            return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
        }

        uint8_t encoded = value % AEMLIB_MQTT_REMAINING_LENGTH_BASE;
        value /= AEMLIB_MQTT_REMAINING_LENGTH_BASE;

        if (value > 0) {
            encoded |= AEMLIB_MQTT_REMAINING_LENGTH_CONTINUATION;
        }

        buf[i++] = encoded;

    } while (value > 0);

    *out_len = i;
    return AEMLIB_STATUS_OK;
}

aemlib_status_t aemlib_proto_decode_remaining_length(const uint8_t *buf,
                                                     size_t buf_len,
                                                     uint32_t *value,
                                                     size_t *consumed)
{
    uint32_t multiplier = 1;
    uint32_t result = 0;
    size_t i = 0;

    while (i < buf_len) {
        uint8_t byte = buf[i++];
        result += (byte & AEMLIB_MQTT_REMAINING_LENGTH_MASK) * multiplier;

        if (multiplier > AEMLIB_MQTT_MAX_REMAINING_LENGTH_MULTIPLIER) {
            return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_PROTOCOL);
        }

        if ((byte & AEMLIB_MQTT_REMAINING_LENGTH_CONTINUATION) == 0) {
            *value = result;
            *consumed = i;
            return AEMLIB_STATUS_OK;
        }

        multiplier *= AEMLIB_MQTT_REMAINING_LENGTH_BASE;
    }

    return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
}

// UTF-8 STRING --------------------------------------------------------------

aemlib_status_t aemlib_proto_write_utf8(uint8_t *buf,
                                        size_t buf_len,
                                        size_t *offset,
                                        const char *str)
{
    size_t len = strlen(str);

    aemlib_status_t status = ensure_space(AEMLIB_MQTT_UTF8_LENGTH_PREFIX_SIZE + len, *offset, buf_len);
    AEMLIB_CHECK_STATUS(status);

    buf[*offset + 0] = (uint8_t)((len >> AEMLIB_MQTT_BYTE_SHIFT) & AEMLIB_MQTT_LOW_BYTE_MASK);
    buf[*offset + 1] = (uint8_t)(len & AEMLIB_MQTT_LOW_BYTE_MASK);

    memcpy(&buf[*offset + AEMLIB_MQTT_UTF8_LENGTH_PREFIX_SIZE], str, len);

    *offset += AEMLIB_MQTT_UTF8_LENGTH_PREFIX_SIZE + len;
    return AEMLIB_STATUS_OK;
}

// FIXED HEADER DECODER ------------------------------------------------------

aemlib_status_t aemlib_proto_decode_fixed_header(const uint8_t *buf,
                                                 size_t buf_len,
                                                 aemlib_mqtt_fixed_header_t *out)
{
    if (buf_len < AEMLIB_MQTT_FIXED_HEADER_MIN_SIZE) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
    }

    uint8_t header_byte = buf[0];
    out->type = (aemlib_mqtt_packet_type_t)(header_byte >> AEMLIB_MQTT_PACKET_TYPE_SHIFT);
    out->flags = header_byte & AEMLIB_MQTT_FLAGS_MASK;

    // Validate flags according to MQTT specification
    aemlib_status_t status = validate_mqtt_flags(out->type, out->flags);
    AEMLIB_CHECK_STATUS(status);

    uint32_t remaining_len = 0;
    size_t consumed = 0;

    status = aemlib_proto_decode_remaining_length(&buf[1], buf_len - 1, &remaining_len, &consumed);
    AEMLIB_CHECK_STATUS(status);

    out->remaining_length = remaining_len;
    out->header_size = 1 + consumed;

    return AEMLIB_STATUS_OK;
}

// CONNECT -------------------------------------------------------------------

aemlib_status_t aemlib_proto_encode_connect(uint8_t *buf,
                                            size_t buf_len,
                                            size_t *out_len,
                                            const char *client_id,
                                            uint16_t keepalive_sec)
{
    size_t offset = 0;
    aemlib_status_t status;

    status = append_byte(buf, &offset, buf_len, (uint8_t)(AEMLIB_MQTT_PKT_CONNECT << AEMLIB_MQTT_PACKET_TYPE_SHIFT));
    AEMLIB_CHECK_STATUS(status);

    size_t rl_pos = offset;
    status = reserve_space(&offset, AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES, buf_len);
    AEMLIB_CHECK_STATUS(status);

    status = aemlib_proto_write_utf8(buf, buf_len, &offset, "MQTT");
    AEMLIB_CHECK_STATUS(status);

    status = append_byte(buf, &offset, buf_len, AEMLIB_MQTT_PROTOCOL_LEVEL);
    AEMLIB_CHECK_STATUS(status);

    status = append_byte(buf, &offset, buf_len, AEMLIB_MQTT_CONNECT_CLEAN_SESSION);
    AEMLIB_CHECK_STATUS(status);

    status = append_uint16_be(buf, &offset, buf_len, keepalive_sec);
    AEMLIB_CHECK_STATUS(status);

    status = aemlib_proto_write_utf8(buf, buf_len, &offset, client_id);
    AEMLIB_CHECK_STATUS(status);

    uint32_t rl = (uint32_t)(offset - (rl_pos + AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES));
    uint8_t rl_buf[AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES];
    size_t rl_len = 0;

    status = aemlib_proto_encode_remaining_length(rl, rl_buf, sizeof(rl_buf), &rl_len);
    AEMLIB_CHECK_STATUS(status);

    memmove(&buf[rl_pos + rl_len], &buf[rl_pos + AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES], rl);
    memcpy(&buf[rl_pos], rl_buf, rl_len);

    *out_len = rl_pos + rl_len + rl;
    return AEMLIB_STATUS_OK;
}

// PINGREQ -------------------------------------------------------------------

aemlib_status_t aemlib_proto_encode_pingreq(uint8_t *buf,
                                            size_t buf_len,
                                            size_t *out_len)
{
    if (buf_len < AEMLIB_MQTT_PINGREQ_LENGTH) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
    }

    buf[0] = (AEMLIB_MQTT_PKT_PINGREQ << AEMLIB_MQTT_PACKET_TYPE_SHIFT);
    buf[1] = AEMLIB_MQTT_PINGREQ_REMAINING_LENGTH;

    *out_len = AEMLIB_MQTT_PINGREQ_LENGTH;
    return AEMLIB_STATUS_OK;
}

// DISCONNECT ----------------------------------------------------------------

aemlib_status_t aemlib_proto_encode_disconnect(uint8_t *buf,
                                               size_t buf_len,
                                               size_t *out_len)
{
    if (buf_len < AEMLIB_MQTT_DISCONNECT_LENGTH) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
    }

    buf[0] = (AEMLIB_MQTT_PKT_DISCONNECT << AEMLIB_MQTT_PACKET_TYPE_SHIFT);
    buf[1] = AEMLIB_MQTT_DISCONNECT_REMAINING_LENGTH;

    *out_len = AEMLIB_MQTT_DISCONNECT_LENGTH;
    return AEMLIB_STATUS_OK;
}

// PUBLISH (QoS 0) -----------------------------------------------------------

aemlib_status_t aemlib_proto_encode_publish(uint8_t *buf,
                                            size_t buf_len,
                                            size_t *out_len,
                                            const char *topic,
                                            const uint8_t *payload,
                                            size_t payload_len)
{
    size_t offset = 0;

    // Fixed header (flags = QoS0)
    aemlib_status_t status = append_byte(buf, &offset, buf_len,
                                          (AEMLIB_MQTT_PKT_PUBLISH << AEMLIB_MQTT_PACKET_TYPE_SHIFT) | AEMLIB_MQTT_PUBLISH_FLAGS_QOS0);
    AEMLIB_CHECK_STATUS(status);

    // Reserve RL
    size_t rl_pos = offset;
    status = reserve_space(&offset, AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES, buf_len);
    AEMLIB_CHECK_STATUS(status);

    // Topic
    status = aemlib_proto_write_utf8(buf, buf_len, &offset, topic);
    AEMLIB_CHECK_STATUS(status);

    // Payload
    status = ensure_space(payload_len, offset, buf_len);
    AEMLIB_CHECK_STATUS(status);
    memcpy(&buf[offset], payload, payload_len);
    offset += payload_len;

    // Compute RL
    uint32_t rl = (uint32_t)(offset - (rl_pos + AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES));
    uint8_t rl_buf[AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES];
    size_t rl_len = 0;

    status = aemlib_proto_encode_remaining_length(rl, rl_buf, sizeof(rl_buf), &rl_len);
    AEMLIB_CHECK_STATUS(status);

    memmove(&buf[rl_pos + rl_len], &buf[rl_pos + AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES], rl);
    memcpy(&buf[rl_pos], rl_buf, rl_len);

    *out_len = rl_pos + rl_len + rl;
    return AEMLIB_STATUS_OK;
}

aemlib_status_t aemlib_proto_decode_publish(const uint8_t *buf,
                                            size_t buf_len,
                                            const aemlib_mqtt_fixed_header_t *header,
                                            const char **topic,
                                            size_t *topic_len,
                                            const uint8_t **payload,
                                            size_t *payload_len)
{
    if (!buf || !header || !topic || !topic_len || !payload || !payload_len) {
        return AEMLIB_STATUS(AEMLIB_LAYER_GENERAL, AEMLIB_CODE_INVALID_ARG);
    }

    /* QoS 1/2 PUBLISH carry a packet identifier we don't parse yet */
    uint8_t qos = (header->flags >> 1) & 0x03;
    if (qos != 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_UNSUPPORTED);
    }

    size_t offset = header->header_size;
    size_t packet_end = header->header_size + header->remaining_length;

    if (packet_end > buf_len) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
    }

    if (offset + AEMLIB_MQTT_UTF8_LENGTH_PREFIX_SIZE > packet_end) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_MALFORMED);
    }

    size_t topic_length = ((size_t)buf[offset] << AEMLIB_MQTT_BYTE_SHIFT) | buf[offset + 1];
    offset += AEMLIB_MQTT_UTF8_LENGTH_PREFIX_SIZE;

    if (offset + topic_length > packet_end) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_MALFORMED);
    }

    *topic = (const char *)&buf[offset];
    *topic_len = topic_length;
    offset += topic_length;

    *payload = &buf[offset];
    *payload_len = packet_end - offset;

    return AEMLIB_STATUS_OK;
}

// SUBSCRIBE (single topic, QoS 0) ------------------------------------------

aemlib_status_t aemlib_proto_encode_subscribe(uint8_t *buf,
                                              size_t buf_len,
                                              size_t *out_len,
                                              uint16_t packet_id,
                                              const char *topic)
{
    size_t offset = 0;

    // Fixed header
    aemlib_status_t status = append_byte(buf, &offset, buf_len,
                                          (AEMLIB_MQTT_PKT_SUBSCRIBE << AEMLIB_MQTT_PACKET_TYPE_SHIFT) | AEMLIB_MQTT_SUBSCRIBE_FLAGS);
    AEMLIB_CHECK_STATUS(status);

    // Reserve RL
    size_t rl_pos = offset;
    status = reserve_space(&offset, AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES, buf_len);
    AEMLIB_CHECK_STATUS(status);

    // Packet ID
    status = append_uint16_be(buf, &offset, buf_len, packet_id);
    AEMLIB_CHECK_STATUS(status);

    // Topic
    status = aemlib_proto_write_utf8(buf, buf_len, &offset, topic);
    AEMLIB_CHECK_STATUS(status);

    // Requested QoS
    status = append_byte(buf, &offset, buf_len, AEMLIB_MQTT_SUBSCRIBE_PAYLOAD_QOS);
    AEMLIB_CHECK_STATUS(status);

    // Compute RL
    uint32_t rl = (uint32_t)(offset - (rl_pos + AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES));
    uint8_t rl_buf[AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES];
    size_t rl_len = 0;

    status = aemlib_proto_encode_remaining_length(rl, rl_buf, sizeof(rl_buf), &rl_len);
    AEMLIB_CHECK_STATUS(status);

    memmove(&buf[rl_pos + rl_len], &buf[rl_pos + AEMLIB_MQTT_REMAINING_LENGTH_MAX_BYTES], rl);
    memcpy(&buf[rl_pos], rl_buf, rl_len);

    *out_len = rl_pos + rl_len + rl;
    return AEMLIB_STATUS_OK;
}

// CONNACK -------------------------------------------------------------------

aemlib_status_t aemlib_proto_decode_connack(const uint8_t *buf,
                                            size_t buf_len,
                                            uint8_t *session_present,
                                            uint8_t *return_code)
{
    if (buf_len < AEMLIB_MQTT_CONNACK_MIN_SIZE) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
    }

    *session_present = buf[AEMLIB_MQTT_CONNACK_SESSION_BYTE_INDEX] & AEMLIB_MQTT_CONNACK_SESSION_PRESENT_MASK;
    *return_code     = buf[AEMLIB_MQTT_CONNACK_RETURN_CODE_INDEX];

    return AEMLIB_STATUS_OK;
}

// SUBACK --------------------------------------------------------------------

aemlib_status_t aemlib_proto_decode_suback(const uint8_t *buf,
                                           size_t buf_len,
                                           uint16_t *packet_id,
                                           uint8_t *granted_qos)
{
    if (buf_len < AEMLIB_MQTT_SUBACK_MIN_SIZE) {
        return AEMLIB_STATUS(AEMLIB_LAYER_PROTOCOL, AEMLIB_CODE_BUFFER_TOO_SMALL);
    }

    *packet_id   = (uint16_t)(buf[AEMLIB_MQTT_SUBACK_PACKET_ID_BYTE_1] << AEMLIB_MQTT_BYTE_SHIFT) | buf[AEMLIB_MQTT_SUBACK_PACKET_ID_BYTE_2];
    *granted_qos = buf[AEMLIB_MQTT_SUBACK_GRANTED_QOS_INDEX];

    return AEMLIB_STATUS_OK;
}
