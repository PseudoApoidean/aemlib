#ifndef AEMLIB_TRANSPORT_H
#define AEMLIB_TRANSPORT_H

#include "aemlib/aemlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// DECLARATIONS -----------------------

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

/* Validate the transport interface */
static inline aemlib_status_t aemlib_transport_validate(const aemlib_transport_t *t)
{
    if (!t) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    if (!t->connect || !t->disconnect || !t->read || !t->write) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    return AEMLIB_STATUS_OK;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_TRANSPORT_H */
