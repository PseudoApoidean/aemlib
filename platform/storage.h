#ifndef AEMLIB_STORAGE_H
#define AEMLIB_STORAGE_H

#include "include/aemlib/status.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ------------------------

/*
 * Storage interface:
 * A simple record-store abstraction for offline queueing.
 *
 * The client writes one record at a time, reads it back later,
 * and deletes it when processed.
 */
typedef struct {
    aemlib_status_t (*write_record)(void *ctx,
                                    const uint8_t *data,
                                    size_t len);

    aemlib_status_t (*read_record)(void *ctx,
                                   uint8_t *buf,
                                   size_t buf_len,
                                   size_t *out_len);

    aemlib_status_t (*delete_record)(void *ctx);

    void *ctx;
} aemlib_storage_t;

// DECLARATIONS -----------------------

/* Validate the storage interface (optional helper) */
aemlib_status_t aemlib_storage_validate(const aemlib_storage_t *storage);

// IMPLEMENTATION ---------------------

/* Inline helpers for convenience */

static inline aemlib_status_t
aemlib_storage_write(const aemlib_storage_t *s,
                     const uint8_t *data,
                     size_t len)
{
    if (!s || !s->write_record) {
        return AEMLIB_STATUS(AEMLIB_LAYER_SYSTEM, AEMLIB_CODE_INVALID_ARG);
    }
    return s->write_record(s->ctx, data, len);
}

static inline aemlib_status_t
aemlib_storage_read(const aemlib_storage_t *s,
                    uint8_t *buf,
                    size_t buf_len,
                    size_t *out_len)
{
    if (!s || !s->read_record) {
        return AEMLIB_STATUS(AEMLIB_LAYER_SYSTEM, AEMLIB_CODE_INVALID_ARG);
    }
    return s->read_record(s->ctx, buf, buf_len, out_len);
}

static inline aemlib_status_t
aemlib_storage_delete(const aemlib_storage_t *s)
{
    if (!s || !s->delete_record) {
        return AEMLIB_STATUS(AEMLIB_LAYER_SYSTEM, AEMLIB_CODE_INVALID_ARG);
    }
    return s->delete_record(s->ctx);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_STORAGE_H */
