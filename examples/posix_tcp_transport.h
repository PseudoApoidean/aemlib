#ifndef AEMLIB_POSIX_TCP_TRANSPORT_H
#define AEMLIB_POSIX_TCP_TRANSPORT_H

#include <aemlib/aemlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ------------------------

/* POSIX (BSD sockets) TCP transport. Examples-only: uses getaddrinfo/sockets,
 * not portable to bare-metal/RTOS targets, so it lives outside platform/. */
typedef struct {
    int fd;
    const char *host;
    uint16_t port;
    int connecting; /* non-blocking connect() in progress, waiting on POLLOUT */
} aemlib_posix_tcp_t;

// DECLARATIONS -----------------------

/* Initialize (does not open a socket yet; that happens on first connect()) */
void aemlib_posix_tcp_init(aemlib_posix_tcp_t *t, const char *host, uint16_t port);

/* Bind an aemlib_transport_t vtable to a POSIX TCP transport instance */
void aemlib_posix_tcp_bind(aemlib_transport_t *transport, aemlib_posix_tcp_t *t);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AEMLIB_POSIX_TCP_TRANSPORT_H */
