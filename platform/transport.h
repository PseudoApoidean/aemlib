#ifndef AEMLIB_TRANSPORT_H
#define AEMLIB_TRANSPORT_H

#include "aemlib/status.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ------------------------

/*
 * Transport interface:
 *
 * The transport is fully abstracted so the library can run over:
 *   - TCP sockets
 *   - TLS wrappers
 *   - Serial links
 *   - Virtual transports for testing
 *
 * All operations are non-blocking and return immediately.
 * The user is responsible for retrying via aemlib_poll().
 */
typedef struct {
    /* Establish the underlying connection */
    aemlib_status_t (*connect)(void *ctx);

    /* Close the underlying connection */
    aemlib_status_t (*disconnect)(void *ctx);

    /*
     * Read up to buf_len bytes.
     * out_len must be set to the number of bytes actually read.
     *
     * Return codes:
     *   OK            — data read (out_len may be 0)
     *   WOULD_BLOCK   — no data available
     *   CLOSED        — connection closed
     *   IO            — read error
     */
    aemlib_status_t (*read)(void *ctx,
                            uint8_t *buf,
                            size_t buf_len,
                            size_t *out_len);

    /*
     * Write up to len bytes.
     * out_len must be set to the number of bytes actually written.
     *
     * Return codes:
     *   OK            — data written (out_len may be < len)
     *   WOULD_BLOCK   — cannot write now
     *   CLOSED        — connection closed
     *   IO            — write error
     */
    aemlib_status_t (*write)(void *ctx,
                             const uint8_t *buf,
                             size_t len,
                             size_t *out_len);

    /* User-provided context pointer */
    void *ctx;

} aemlib_transport_t;

// DECLARATIONS -----------------------

/* Validate the transport interface (optional helper) */
aemlib_status_t aemlib_transport_validate(const aemlib_transport_t *t);

// IMPLEMENTATION ---------------------

/* Inline helpers for convenience */

static inline aemlib_status_t
aemlib_transport_connect(const aemlib_transport_t *t)
{
    if (!t || !t->connect) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    return t->connect(t->ctx);
}

static inline aemlib_status_t
aemlib_transport_disconnect(const aemlib_transport_t *t)
{
    if (!t || !t->disconnect) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    return t->disconnect(t->ctx);
}

static inline aemlib_status_t
aemlib_transport_read(const aemlib_transport_t *t,
                      uint8_t *buf,
                      size_t buf_len,
                      size_t *out_len)
{
    if (!t || !t->read) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    return t->read(t->ctx, buf, buf_len, out_len);
}

static inline aemlib_status_t
aemlib_transport_write(const aemlib_transport_t *t,
                       const uint8_t *buf,
                       size_t len,
                       size_t *out_len)
{
    if (!t || !t->write) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    return t->write(t->ctx, buf, len, out_len);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_TRANSPORT_H */
