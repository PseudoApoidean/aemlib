#ifndef AEMLIB_VIRTUAL_TRANSPORT_H
#define AEMLIB_VIRTUAL_TRANSPORT_H

#include "aemlib/aemlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ------------------------

/* Single-direction byte pipe backing a virtual transport endpoint (ring buffer
 * over caller-provided storage; no dynamic allocation). */
typedef struct {
    uint8_t *buf;
    size_t   buf_size;
    size_t   head;
    size_t   len;
} aemlib_virtual_channel_t;

/* One endpoint of an in-memory loopback transport. Two endpoints sharing a
 * pair of channels (one per direction) let a client and a fake broker/peer
 * talk to each other without any real I/O. */
typedef struct {
    aemlib_virtual_channel_t *tx;
    aemlib_virtual_channel_t *rx;
    uint8_t connected;
} aemlib_virtual_transport_t;

// DECLARATIONS -----------------------

/* Initialize a byte channel over caller-provided storage */
void aemlib_virtual_channel_init(aemlib_virtual_channel_t *channel,
                                 uint8_t *buf,
                                 size_t buf_size);

/* Wire up a loopback pair: what one endpoint writes, the other reads */
void aemlib_virtual_transport_pair_init(aemlib_virtual_transport_t *a,
                                        aemlib_virtual_channel_t *a_to_b,
                                        aemlib_virtual_transport_t *b,
                                        aemlib_virtual_channel_t *b_to_a);

/* Bind an aemlib_transport_t vtable to a virtual transport endpoint */
void aemlib_virtual_transport_bind(aemlib_transport_t *transport,
                                   aemlib_virtual_transport_t *vt);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_VIRTUAL_TRANSPORT_H */
