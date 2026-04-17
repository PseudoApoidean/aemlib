#ifndef AEMLIB_LOG_H
#define AEMLIB_LOG_H

#include "aemlib/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ------------------------

/* Logging levels */
typedef enum aemlib_log_level {
    AEMLIB_LOG_NONE  = 0,
    AEMLIB_LOG_ERROR = 1,
    AEMLIB_LOG_INFO  = 2,
    AEMLIB_LOG_DEBUG = 3,
} aemlib_log_level_t;

/* User-provided logging callback */
typedef void (*aemlib_log_fn)(
    aemlib_log_level_t level,
    const char *module,
    const char *msg
);

/* Default compile-time log level */
#ifndef AEMLIB_LOG_LEVEL
#define AEMLIB_LOG_LEVEL AEMLIB_LOG_INFO
#endif

/* Module identifiers */
#define AEMLIB_LOG_MODULE_CORE      "core"
#define AEMLIB_LOG_MODULE_PROTO     "proto"
#define AEMLIB_LOG_MODULE_TRANSPORT "transport"
#define AEMLIB_LOG_MODULE_STORAGE   "storage"
#define AEMLIB_LOG_MODULE_TIME      "time"

// DECLARATIONS -----------------------

/* Set the global logging callback */
void aemlib_set_log_callback(aemlib_log_fn fn);

/* Emit a log message (unprefixed) */
void aemlib_log_emit(aemlib_log_level_t level,
                     const char *module,
                     const char *fmt, ...);

/* Emit a log message with "[LEVEL] module: msg" prefix */
void aemlib_log_emit_prefixed(aemlib_log_level_t level,
                              const char *module,
                              const char *fmt, ...);

// IMPLEMENTATION MACROS --------------

#if AEMLIB_LOG_LEVEL >= AEMLIB_LOG_ERROR
#define AEMLIB_LOG_ERROR(module, fmt, ...) \
    aemlib_log_emit(AEMLIB_LOG_ERROR, module, fmt, ##__VA_ARGS__)
#else
#define AEMLIB_LOG_ERROR(module, fmt, ...) ((void)0)
#endif

#if AEMLIB_LOG_LEVEL >= AEMLIB_LOG_INFO
#define AEMLIB_LOG_INFO(module, fmt, ...) \
    aemlib_log_emit(AEMLIB_LOG_INFO, module, fmt, ##__VA_ARGS__)
#else
#define AEMLIB_LOG_INFO(module, fmt, ...) ((void)0)
#endif

#if AEMLIB_LOG_LEVEL >= AEMLIB_LOG_DEBUG
#define AEMLIB_LOG_DEBUG(module, fmt, ...) \
    aemlib_log_emit(AEMLIB_LOG_DEBUG, module, fmt, ##__VA_ARGS__)
#else
#define AEMLIB_LOG_DEBUG(module, fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_LOG_H */
