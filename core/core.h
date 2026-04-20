#ifndef AEMLIB_CORE_H
#define AEMLIB_CORE_H

#include "aemlib/status.h"
#include "../platform/transport.h"
#include "../platform/time.h"
#include "../platform/storage.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ------------------------

/* Internal client states */
typedef enum {
    AEMLIB_STATE_DISCONNECTED = 0,
    AEMLIB_STATE_CONNECTING,
    AEMLIB_STATE_MQTT_CONNECT_SENT,
    AEMLIB_STATE_MQTT_CONNECTED,
    AEMLIB_STATE_MQTT_DISCONNECTING,
} aemlib_state_t;

/* Forward declaration of the opaque client */
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

} aemlib_client_t;

/* Configuration passed to aemlib_init() */
typedef struct {
    uint8_t *tx_buffer;
    size_t   tx_buffer_size;

    uint8_t *rx_buffer;
    size_t   rx_buffer_size;

    aemlib_transport_t transport;
    aemlib_time_t      time;
    aemlib_storage_t   storage; /* optional; zero-init if unused */

    uint64_t keepalive_interval_ms;

} aemlib_core_config_t;

// DECLARATIONS -----------------------

/* Initialize the client (no dynamic allocation inside) */
aemlib_status_t aemlib_core_init(aemlib_client_t *client,
                                 const aemlib_core_config_t *config);

/* Advance the state machine (non-blocking, user-driven) */
aemlib_status_t aemlib_core_poll(aemlib_client_t *client);

/* Request a connection */
aemlib_status_t aemlib_core_connect(aemlib_client_t *client);

/* Request a disconnect */
aemlib_status_t aemlib_core_disconnect(aemlib_client_t *client);

/* Internal helper: generate next MQTT packet ID */
uint16_t aemlib_core_next_packet_id(aemlib_client_t *client);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_CORE_H */
