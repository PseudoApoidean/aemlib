#ifndef AEMLIB_AEMLIB_H
#define AEMLIB_AEMLIB_H

#include <stddef.h>
#include <stdint.h>

#include "aemlib/status.h"
#include "aemlib/log.h"

#ifdef __cplusplus
extern "C" {
#endif

// VERSION ----------------------------

#define AEMLIB_VERSION_MAJOR 1
#define AEMLIB_VERSION_MINOR 0
#define AEMLIB_VERSION_PATCH 0

// INTERFACES -------------------------

/* Transport interface */
typedef struct {
    aemlib_status_t (*connect)(void *ctx);
    aemlib_status_t (*disconnect)(void *ctx);
    aemlib_status_t (*read)(void *ctx, uint8_t *buf, size_t len, size_t *out_len);
    aemlib_status_t (*write)(void *ctx, const uint8_t *buf, size_t len, size_t *out_len);
    void *ctx; /* user transport context */
} aemlib_transport_t;

/* Time interface */
typedef struct {
    uint64_t (*now_ms)(void *ctx);
    void *ctx;
} aemlib_time_t;

/* Storage interface (optional) */
typedef struct {
    aemlib_status_t (*write_record)(void *ctx, const uint8_t *data, size_t len);
    aemlib_status_t (*read_record)(void *ctx, uint8_t *buf, size_t buf_len, size_t *out_len);
    aemlib_status_t (*delete_record)(void *ctx);
    void *ctx;
} aemlib_storage_t;

/* User callback for inbound PUBLISH messages (QoS 0 only for now).
 * topic/payload point into the client's rx_buffer and are only valid for
 * the duration of the callback; topic is NOT null-terminated. */
typedef void (*aemlib_message_fn)(void *ctx,
                                  const char *topic,
                                  size_t topic_len,
                                  const uint8_t *payload,
                                  size_t payload_len);

/* Internal client states */
typedef enum {
    AEMLIB_STATE_DISCONNECTED = 0,
    AEMLIB_STATE_CONNECTING,
    AEMLIB_STATE_MQTT_CONNECT_SENT,
    AEMLIB_STATE_MQTT_CONNECTED,
    AEMLIB_STATE_MQTT_DISCONNECTING,
} aemlib_state_t;

/* Client instance. Defined here (not opaque) so callers can allocate it
 * statically, in keeping with the library's no-dynamic-allocation design. */
typedef struct aemlib_client {
    /* Current state */
    aemlib_state_t state;

    /* User-provided interfaces */
    aemlib_transport_t transport;
    aemlib_time_t      time;
    aemlib_storage_t   storage;

    /* Buffers (owned by user, referenced here) */
    uint8_t *tx_buffer;
    size_t   tx_buffer_size;

    uint8_t *rx_buffer;
    size_t   rx_buffer_size;

    /* Internal bookkeeping */
    uint64_t last_activity_ms;
    uint64_t keepalive_interval_ms;

    /* MQTT packet ID counter (for QoS1) */
    uint16_t packet_id;

    /* Set once the CONNECT packet has been sent for the current connection
     * attempt, so a slow CONNACK (multiple polls) doesn't re-send it */
    uint8_t connect_sent;

    /* Inbound PUBLISH callback (optional; NULL if unused) */
    aemlib_message_fn on_message;
    void             *on_message_ctx;

} aemlib_client_t;

/* Client configuration */
typedef struct {
    uint8_t *tx_buffer;
    size_t   tx_buffer_size;

    uint8_t *rx_buffer;
    size_t   rx_buffer_size;

    aemlib_transport_t transport;
    aemlib_time_t      time;
    aemlib_storage_t   storage; /* optional; zero-init if unused */

    uint64_t keepalive_interval_ms;

    /* Inbound PUBLISH callback (optional; NULL if unused) */
    aemlib_message_fn on_message;
    void             *on_message_ctx;

} aemlib_config_t;

// API --------------------------------

/* Create a client instance (no dynamic allocation inside) */
aemlib_status_t aemlib_init(aemlib_client_t *client,
                            const aemlib_config_t *config);

/* Poll the client (user-driven, non-blocking) */
aemlib_status_t aemlib_poll(aemlib_client_t *client);

/* Connect / disconnect */
aemlib_status_t aemlib_connect(aemlib_client_t *client);
aemlib_status_t aemlib_disconnect(aemlib_client_t *client);

/* Publish / subscribe (MQTT 3.1.1, QoS 0/1) */
aemlib_status_t aemlib_publish(aemlib_client_t *client,
                               const char *topic,
                               const uint8_t *payload,
                               size_t payload_len,
                               int qos);

aemlib_status_t aemlib_subscribe(aemlib_client_t *client,
                                 const char *topic,
                                 int qos);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_AEMLIB_H */
