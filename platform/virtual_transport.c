#include "virtual_transport.h"
#include "aemlib/status.h"

// DECLARATIONS -----------------------

static aemlib_status_t channel_write(aemlib_virtual_channel_t *ch,
                                     const uint8_t *data,
                                     size_t len,
                                     size_t *out_len);
static aemlib_status_t channel_read(aemlib_virtual_channel_t *ch,
                                    uint8_t *buf,
                                    size_t len,
                                    size_t *out_len);

static aemlib_status_t vt_connect(void *ctx);
static aemlib_status_t vt_disconnect(void *ctx);
static aemlib_status_t vt_read(void *ctx, uint8_t *buf, size_t len, size_t *out_len);
static aemlib_status_t vt_write(void *ctx, const uint8_t *buf, size_t len, size_t *out_len);

// IMPLEMENTATION ---------------------

void aemlib_virtual_channel_init(aemlib_virtual_channel_t *channel,
                                 uint8_t *buf,
                                 size_t buf_size)
{
    channel->buf      = buf;
    channel->buf_size = buf_size;
    channel->head     = 0;
    channel->len      = 0;
}

void aemlib_virtual_transport_pair_init(aemlib_virtual_transport_t *a,
                                        aemlib_virtual_channel_t *a_to_b,
                                        aemlib_virtual_transport_t *b,
                                        aemlib_virtual_channel_t *b_to_a)
{
    a->tx        = a_to_b;
    a->rx        = b_to_a;
    a->connected = 0;

    b->tx        = b_to_a;
    b->rx        = a_to_b;
    b->connected = 0;
}

void aemlib_virtual_transport_bind(aemlib_transport_t *transport,
                                   aemlib_virtual_transport_t *vt)
{
    transport->connect    = vt_connect;
    transport->disconnect = vt_disconnect;
    transport->read       = vt_read;
    transport->write      = vt_write;
    transport->ctx        = vt;
}

// CHANNEL (RING BUFFER) --------------

static aemlib_status_t channel_write(aemlib_virtual_channel_t *ch,
                                     const uint8_t *data,
                                     size_t len,
                                     size_t *out_len)
{
    size_t space = ch->buf_size - ch->len;
    size_t n = len < space ? len : space;
    size_t tail = (ch->head + ch->len) % ch->buf_size;

    for (size_t i = 0; i < n; i++) {
        ch->buf[(tail + i) % ch->buf_size] = data[i];
    }

    ch->len += n;
    *out_len = n;

    if (n == 0 && len > 0) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_WOULD_BLOCK);
    }
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t channel_read(aemlib_virtual_channel_t *ch,
                                    uint8_t *buf,
                                    size_t len,
                                    size_t *out_len)
{
    size_t n = len < ch->len ? len : ch->len;

    for (size_t i = 0; i < n; i++) {
        buf[i] = ch->buf[(ch->head + i) % ch->buf_size];
    }

    ch->head = (ch->head + n) % ch->buf_size;
    ch->len -= n;
    *out_len = n;

    return AEMLIB_STATUS_OK;
}

// TRANSPORT VTABLE --------------------

static aemlib_status_t vt_connect(void *ctx)
{
    aemlib_virtual_transport_t *vt = (aemlib_virtual_transport_t *)ctx;
    vt->connected = 1;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t vt_disconnect(void *ctx)
{
    aemlib_virtual_transport_t *vt = (aemlib_virtual_transport_t *)ctx;
    vt->connected = 0;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t vt_read(void *ctx, uint8_t *buf, size_t len, size_t *out_len)
{
    aemlib_virtual_transport_t *vt = (aemlib_virtual_transport_t *)ctx;
    if (!vt->connected) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CLOSED);
    }
    return channel_read(vt->rx, buf, len, out_len);
}

static aemlib_status_t vt_write(void *ctx, const uint8_t *buf, size_t len, size_t *out_len)
{
    aemlib_virtual_transport_t *vt = (aemlib_virtual_transport_t *)ctx;
    if (!vt->connected) {
        return AEMLIB_STATUS(AEMLIB_LAYER_IO, AEMLIB_CODE_CLOSED);
    }
    return channel_write(vt->tx, buf, len, out_len);
}
