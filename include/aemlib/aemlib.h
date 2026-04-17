#ifndef AEMLIB_AEMLIB_H
#define AEMLIB_AEMLIB_H

#include "aemlib/status.h"
#include "aemlib/log.h"

#ifdef __cplusplus
extern "C" {
#endif

// VERSION ----------------------------

#define AEMLIB_VERSION_MAJOR 1
#define AEMLIB_VERSION_MINOR 0
#define AEMLIB_VERSION_PATCH 0

// FORWARD DECLARATIONS ---------------

/* Opaque client handle */
typedef struct aemlib_client aemlib_client_t;

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

/* Client configuration */
typedef struct {
    uint8_t *tx_buffer;
    size_t   tx_buffer_size;

    uint8_t *rx_buffer;
    size_t   rx_buffer_size;

    aemlib_transport_t transport;
    aemlib_time_t      time;
    aemlib_storage_t   storage; /* optional; zero-init if unused */
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
