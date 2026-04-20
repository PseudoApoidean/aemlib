#ifndef AEMLIB_STORAGE_H
#define AEMLIB_STORAGE_H

#include "aemlib/aemlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// DECLARATIONS -----------------------

/* Validate the storage interface */
static inline aemlib_status_t aemlib_storage_validate(const aemlib_storage_t *storage)
{
    if (!storage) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    /* Storage is optional, so only check if provided */
    if (storage->write_record || storage->read_record || storage->delete_record) {
        /* If any function is provided, all should be */
        if (!storage->write_record || !storage->read_record || !storage->delete_record) {
            return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
        }
    }
    return AEMLIB_STATUS_OK;
}

/* Inline helpers for convenience */

static inline aemlib_status_t
aemlib_storage_write(const aemlib_storage_t *s,
                     const uint8_t *data,
                     size_t len)
{
    if (!s || !s->write_record) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
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
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    return s->read_record(s->ctx, buf, buf_len, out_len);
}

static inline aemlib_status_t
aemlib_storage_delete(const aemlib_storage_t *s)
{
    if (!s || !s->delete_record) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_INVALID_ARG);
    }
    return s->delete_record(s->ctx);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_STORAGE_H */
