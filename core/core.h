#ifndef AEMLIB_CORE_H
#define AEMLIB_CORE_H

#include "aemlib/aemlib.h"
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

/* aemlib_state_t, aemlib_client_t, and aemlib_config_t are defined in
 * aemlib/aemlib.h; aemlib_core_config_t is the internal name used by the
 * core/client implementation for the same configuration type. */
typedef aemlib_config_t aemlib_core_config_t;

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
