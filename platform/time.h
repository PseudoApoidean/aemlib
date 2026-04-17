#ifndef AEMLIB_TIME_H
#define AEMLIB_TIME_H

#include "aemlib/status.h"

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ------------------------

/* Time interface: user provides a millisecond clock */
typedef struct {
    uint64_t (*now_ms)(void *ctx);
    void *ctx;
} aemlib_time_t;

// DECLARATIONS -----------------------

/* Validate the time interface (optional helper) */
aemlib_status_t aemlib_time_validate(const aemlib_time_t *time);

// IMPLEMENTATION ---------------------

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
