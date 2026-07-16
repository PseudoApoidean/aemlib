# AEMlib

**A**poidea's **E**mbedded **M**QTT **lib**rary

A small MQTT 3.1.1 client written in C99 for embedded/bare-metal targets, with no dynamic allocation and a cooperative, user-driven polling model.

## Design principles

- **Handle-centric, no hidden state.** `aemlib_client_t` is a plain, non-opaque struct the caller owns and can allocate statically.
- **No dynamic allocation.** The caller provides all buffers (TX/RX) up front; the library only ever references them.
- **Cooperative polling.** Nothing happens except inside `aemlib_poll()`. Safe to call from either a bare-metal loop or an RTOS task on a timer tick.
- **Pluggable everything.** Transport, time, and storage are plain structs of function pointers plus a `void *ctx`. Swap in a real socket, a virtual/loopback transport, or a fake for tests without touching the client.

## Building

Requires CMake 3.20+ and a C99 compiler. A `dev` preset (Ninja, `-Wall -Wextra`) is provided:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev --output-on-failure
```

Or without presets:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Build options (both default `ON`): `AEMLIB_BUILD_TESTS`, `AEMLIB_BUILD_EXAMPLES`.

## Examples

- **`smoke_test`**: Includes `<aemlib/aemlib.h>` to test for API compatibility. Wired into `ctest`.
- **`pubsub_demo`**: A real client talking to a tiny fake broker over the virtual transport; proves connect → subscribe → publish → receive end to end. Wired into `ctest`.
- **`mqtt_broker_demo`**: Connects to a real public broker (`test.mosquitto.org:1883`) over a POSIX TCP transport. Built by default but *not* part of `ctest` (not hermetic).

## Testing

Unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity), fetched via CMake `FetchContent`. Run everything with `ctest --preset dev`.
