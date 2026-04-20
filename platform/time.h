#ifndef AEMLIB_TIME_H
#define AEMLIB_TIME_H

#include "aemlib/aemlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// DECLARATIONS -----------------------

// IMPLEMENTATION ---------------------

/* Validate the time interface */
static inline aemlib_status_t aemlib_time_validate(const aemlib_time_t *time)
{
    if (!time || !time->now_ms) {
        return AEMLIB_STATUS(AEMLIB_LAYER_SYSTEM, AEMLIB_CODE_INVALID_ARG);
    }
    return AEMLIB_STATUS_OK;
}

/* Inline helper: safe call to now_ms() */
static inline uint64_t aemlib_time_now(const aemlib_time_t *time) {
    if (!time || !time->now_ms) {
        return 0;
    }
    return time->now_ms(time->ctx);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_TIME_H */
