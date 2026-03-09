#include "aemlib/status.h"

#ifndef AEMLIB_AEMLIB_H
#define AEMLIB_AEMLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/* Version macros */
#define AEMLIB_VERSION_MAJOR 1
#define AEMLIB_VERSION_MINOR 0
#define AEMLIB_VERSION_PATCH 0

/* API */
aemlib_status_t aemlib_init(void);
aemlib_status_t aemlib_poll(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_AEMLIB_H */
