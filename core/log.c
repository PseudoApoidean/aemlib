#include "aemlib/aemlib.h"
#include "aemlib/log.h"
#include "aemlib/config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// DEFINITIONS ------------------------

/* Global logging callback */
static aemlib_log_fn g_aemlib_log_callback = NULL;

/* Optional: convert log level to string */
static const char *aemlib_log_level_str(aemlib_log_level_t level) {
    switch (level) {
        case AEMLIB_LOG_ERROR: return "ERROR";
        case AEMLIB_LOG_INFO:  return "INFO";
        case AEMLIB_LOG_DEBUG: return "DEBUG";
        case AEMLIB_LOG_NONE:
        default:               return "NONE";
    }
}

// DECLARATIONS -----------------------

static void aemlib_log_emit_internal(aemlib_log_level_t level,
                                     const char *module,
                                     const char *fmt,
                                     va_list args);

// IMPLEMENTATION ---------------------

void aemlib_set_log_callback(aemlib_log_fn fn) {
    g_aemlib_log_callback = fn;
}

void aemlib_log_emit(aemlib_log_level_t level,
                     const char *module,
                     const char *fmt, ...) {

#if AEMLIB_LOG_LEVEL == AEMLIB_LOG_NONE
    (void)level;
    (void)module;
    (void)fmt;
    return;
#else
    if (!g_aemlib_log_callback) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    aemlib_log_emit_internal(level, module, fmt, args);
    va_end(args);
#endif
}

static void aemlib_log_emit_internal(aemlib_log_level_t level,
                                     const char *module,
                                     const char *fmt,
                                     va_list args) {

    char buffer[128];

    int n = vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (n < 0) {
        /* Formatting failed — fallback */
        g_aemlib_log_callback(level,
                              module ? module : "",
                              "log formatting error");
        return;
    }

    buffer[sizeof(buffer) - 1] = '\0';

    g_aemlib_log_callback(level,
                          module ? module : "",
                          buffer);
}

/* Optional helper: prefixed log output */
void aemlib_log_emit_prefixed(aemlib_log_level_t level,
                              const char *module,
                              const char *fmt, ...) {

#if AEMLIB_LOG_LEVEL == AEMLIB_LOG_NONE
    (void)level;
    (void)module;
    (void)fmt;
    return;
#else
    if (!g_aemlib_log_callback) {
        return;
    }

    char msg_buf[96];
    char full_buf[128];

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    va_end(args);

    if (n < 0) {
        g_aemlib_log_callback(level,
                              module ? module : "",
                              "log formatting error");
        return;
    }

    msg_buf[sizeof(msg_buf) - 1] = '\0';

    snprintf(full_buf, sizeof(full_buf),
             "[%s] %s: %s",
             aemlib_log_level_str(level),
             module ? module : "",
             msg_buf);

    full_buf[sizeof(full_buf) - 1] = '\0';

    g_aemlib_log_callback(level,
                          module ? module : "",
                          full_buf);
#endif
}
