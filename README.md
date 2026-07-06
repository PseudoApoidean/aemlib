# AEMlib

**A**poidea's **E**mbedded **M**QTT **lib**rary — a small MQTT 3.1.1 client written in C99 for embedded/bare-metal targets, with no dynamic allocation and a cooperative, user-driven polling model.

## Design principles

- **Handle-centric, no hidden state.** `aemlib_client_t` is a plain, non-opaque struct the caller owns and can allocate statically.
- **No dynamic allocation.** The caller provides all buffers (TX/RX) up front; the library only ever references them.
- **Cooperative polling.** Nothing happens except inside `aemlib_poll()`. No internal threads, no blocking calls — safe to call from a bare-metal loop or an RTOS task on a timer tick.
- **Pluggable everything.** Transport, time, and storage are plain structs of function pointers plus a `void *ctx` — swap in a real socket, a virtual/loopback transport, or a fake for tests without touching the client.

## Status

MQTT 3.1.1, QoS 0 only, for now. Working:

- Connect handshake with real CONNACK wait + timeout
- Publish / subscribe (QoS 0)
- Inbound PUBLISH delivered via an `on_message` callback
- Keepalive (PINGREQ)
- Clean disconnect

**Known limitations**:
- QoS 1 (packet IDs, retries, PUBACK) and the offline/persistent queue are not implemented yet.

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

- **`smoke_test`** — only includes `<aemlib/aemlib.h>`; exists purely to catch public-API/implementation drift, wired into `ctest`.
- **`pubsub_demo`** — a real client talking to a tiny fake broker over the virtual transport; proves connect → subscribe → publish → receive end to end. Also wired into `ctest`.
- **`mqtt_broker_demo`** — connects to a real public broker (`test.mosquitto.org:1883`) over a POSIX TCP transport. Built by default but *not* part of `ctest` (not hermetic — talks to the network).

## Testing

Unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity), fetched via CMake `FetchContent`. Run everything with `ctest --preset dev`.
