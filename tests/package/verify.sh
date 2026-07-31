#!/usr/bin/env bash
#
# Installs AEMlib into a throwaway prefix and builds an out-of-tree project
# against it with find_package. An install that nobody has linked against is
# not verified, so this builds and runs a real consumer.

set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

log="$work_dir/log"

# Quiet while it works, but a failing step has to say why: ninja writes compiler
# errors to stdout, so discarding stdout outright leaves a CI failure with no
# diagnostic at all.
run() {
    if ! "$@" >"$log" 2>&1; then
        echo "FAILED: $*" >&2
        cat "$log" >&2
        exit 1
    fi
}

fail() {
    echo "FAILED: $1" >&2
    exit 1
}

# $1 dir, $2.. names allowed at its top level
assert_only() {
    local dir="$1"
    shift

    [ -d "$dir" ] || fail "$dir was not created by the install"

    local entry
    for entry in "$dir"/*; do
        local name
        name=$(basename "$entry")

        local allowed
        for allowed in "$@"; do
            # shellcheck disable=SC2053 # the pattern is meant to glob
            if [[ $name == $allowed ]]; then
                continue 2
            fi
        done

        fail "unexpected entry installed in ${dir#"$prefix"}: $name"
    done
}

check_prefix() {
    assert_only "$prefix/include" aemlib
    assert_only "$prefix/lib" 'libaem.*' cmake
    assert_only "$prefix/lib/cmake" aemlib

    local internal
    for internal in core.h client.h proto.h transport.h time_iface.h storage.h virtual_transport.h; do
        if find "$prefix/include" -name "$internal" | grep -q .; then
            fail "internal header $internal was installed"
        fi
    done
}

# Two configurations, because they fail differently. Tests OFF is how a consumer
# actually builds the package. Tests ON is the only way a Unity leak into the
# prefix can show up at all - with tests off, Unity is never even fetched.
for tests in OFF ON; do
    prefix="$work_dir/prefix-$tests"

    echo "== building and installing aemlib (AEMLIB_BUILD_TESTS=$tests) to $prefix"
    run cmake -S "$repo_root" -B "$work_dir/build-$tests" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DAEMLIB_BUILD_TESTS="$tests" \
        -DAEMLIB_BUILD_EXAMPLES=OFF \
        -DCMAKE_INSTALL_PREFIX="$prefix"
    run cmake --build "$work_dir/build-$tests"
    run cmake --install "$work_dir/build-$tests"

    echo "== checking the prefix holds the package and nothing else"
    check_prefix
done

# The consumer builds against the tests-off prefix, the one a consumer would get.
prefix="$work_dir/prefix-OFF"

echo "== building the out-of-tree consumer against the installed package"
run cmake -S "$repo_root/tests/package" -B "$work_dir/consumer" -G Ninja \
    -DCMAKE_PREFIX_PATH="$prefix"
run cmake --build "$work_dir/consumer"

echo "== running it"
"$work_dir/consumer/consumer"

echo "== package verification passed"
