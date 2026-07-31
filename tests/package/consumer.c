#include <aemlib/aemlib.h>

#include <stdio.h>

static aemlib_status_t stub_connect(void *ctx) {
    (void)ctx;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t stub_disconnect(void *ctx) {
    (void)ctx;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t stub_read(void *ctx, uint8_t *buf, size_t len, size_t *out_len) {
    (void)ctx;
    (void)buf;
    (void)len;
    *out_len = 0;
    return AEMLIB_STATUS_OK;
}

static aemlib_status_t stub_write(void *ctx, const uint8_t *buf, size_t len, size_t *out_len) {
    (void)ctx;
    (void)buf;
    *out_len = len;
    return AEMLIB_STATUS_OK;
}

static uint64_t stub_time_now(void *ctx) {
    (void)ctx;
    return 0;
}

int main(void) {
    uint8_t tx_buf[256];
    uint8_t rx_buf[256];

    aemlib_client_t client;
    aemlib_config_t config = {
        .tx_buffer = tx_buf,
        .tx_buffer_size = sizeof(tx_buf),
        .rx_buffer = rx_buf,
        .rx_buffer_size = sizeof(rx_buf),
        .transport = {
            .connect    = stub_connect,
            .disconnect = stub_disconnect,
            .read       = stub_read,
            .write      = stub_write,
            .ctx        = NULL
        },
        .time = {
            .now_ms = stub_time_now,
            .ctx    = NULL
        },
        .keepalive_interval_ms = 60000
    };

    aemlib_status_t status = aemlib_init(&client, &config);
    if (status != AEMLIB_STATUS_OK) {
        fprintf(stderr, "consumer FAILED: aemlib_init returned 0x%08x\n", status);
        return 1;
    }

    printf("consumer OK: linked aemlib::aemlib and called aemlib_init\n");
    return 0;
}
