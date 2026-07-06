#include "posix_tcp_transport.h"
#include <aemlib/status.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// DECLARATIONS -----------------------

static aemlib_status_t tcp_connect(void *ctx);
static aemlib_status_t tcp_disconnect(void *ctx);
static aemlib_status_t tcp_read(void *ctx, uint8_t *buf, size_t len, size_t *out_len);
static aemlib_status_t tcp_write(void *ctx, const uint8_t *buf, size_t len, size_t *out_len);

static aemlib_status_t tcp_start_connect(aemlib_posix_tcp_t *t);
static aemlib_status_t tcp_poll_connect(aemlib_posix_tcp_t *t);

// IMPLEMENTATION ---------------------

void aemlib_posix_tcp_init(aemlib_posix_tcp_t *t, const char *host, uint16_t port)
{
    t->fd         = -1;
    t->host       = host;
    t->port       = port;
    t->connecting = 0;
}

void aemlib_posix_tcp_bind(aemlib_transport_t *transport, aemlib_posix_tcp_t *t)
{
    transport->connect    = tcp_connect;
    transport->disconnect = tcp_disconnect;
    transport->read       = tcp_read;
    transport->write      = tcp_write;
    transport->ctx        = t;
}

// CONNECT (non-blocking, multi-poll) -----------------------------------

static aemlib_status_t tcp_start_connect(aemlib_posix_tcp_t *t)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)t->port);

    struct addrinfo *res = NULL;
    if (getaddrinfo(t->host, port_str, &hints, &res) != 0 || !res) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CONNECT);
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_IO);
    }

    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    int rc = connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (rc == 0) {
        t->fd = fd;
        t->connecting = 0;
        return AEMLIB_STATUS_OK;
    }

    if (errno == EINPROGRESS) {
        t->fd = fd;
        t->connecting = 1;
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_WOULD_BLOCK);
    }

    close(fd);
    return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CONNECT);
}

static aemlib_status_t tcp_poll_connect(aemlib_posix_tcp_t *t)
{
    struct pollfd pfd;
    pfd.fd     = t->fd;
    pfd.events = POLLOUT;

    int rv = poll(&pfd, 1, 0);
    if (rv == 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_WOULD_BLOCK);
    }
    if (rv < 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CONNECT);
    }

    int err = 0;
    socklen_t err_len = sizeof(err);
    if (getsockopt(t->fd, SOL_SOCKET, SO_ERROR, &err, &err_len) != 0 || err != 0) {
        close(t->fd);
        t->fd = -1;
        t->connecting = 0;
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CONNECT);
    }

    t->connecting = 0;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t tcp_connect(void *ctx)
{
    aemlib_posix_tcp_t *t = (aemlib_posix_tcp_t *)ctx;

    if (t->connecting) {
        return tcp_poll_connect(t);
    }
    return tcp_start_connect(t);
}

static aemlib_status_t tcp_disconnect(void *ctx)
{
    aemlib_posix_tcp_t *t = (aemlib_posix_tcp_t *)ctx;
    if (t->fd >= 0) {
        close(t->fd);
        t->fd = -1;
    }
    t->connecting = 0;
    return AEMLIB_STATUS_OK;
}

// READ / WRITE ------------------------------------------------------------

static aemlib_status_t tcp_read(void *ctx, uint8_t *buf, size_t len, size_t *out_len)
{
    aemlib_posix_tcp_t *t = (aemlib_posix_tcp_t *)ctx;

    ssize_t n = recv(t->fd, buf, len, MSG_DONTWAIT);
    if (n > 0) {
        *out_len = (size_t)n;
        return AEMLIB_STATUS_OK;
    }
    if (n == 0) {
        *out_len = 0;
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CLOSED);
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        *out_len = 0;
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_WOULD_BLOCK);
    }
    *out_len = 0;
    return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_IO);
}

static aemlib_status_t tcp_write(void *ctx, const uint8_t *buf, size_t len, size_t *out_len)
{
    aemlib_posix_tcp_t *t = (aemlib_posix_tcp_t *)ctx;

    ssize_t n = send(t->fd, buf, len, MSG_NOSIGNAL | MSG_DONTWAIT);
    if (n >= 0) {
        *out_len = (size_t)n;
        return AEMLIB_STATUS_OK;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        *out_len = 0;
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_WOULD_BLOCK);
    }
    *out_len = 0;
    return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_IO);
}
