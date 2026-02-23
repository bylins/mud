#!/bin/sh
# Install opentelemetry-cpp SDK required for building Bylins MUD with WITH_OTEL=ON.
#
# Usage:
#   ./tools/install-otel-sdk.sh            # vcpkg (default)
#   ./tools/install-otel-sdk.sh --source   # build from source
#
# After installation, build the server with:
#   cmake -S . -B build_otel \
#     -DCMAKE_BUILD_TYPE=Release \
#     -DWITH_OTEL=ON \
#     -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake \
#     -DCMAKE_PREFIX_PATH=~/vcpkg/installed/x64-linux
#   make -C build_otel -j$(($(nproc)/2))
#
# Note: if you only deploy a pre-built binary, this script is not needed —
# opentelemetry-cpp is a build-time dependency only.

set -e

OTEL_VERSION="1.24.0"
VCPKG_DIR="${VCPKG_DIR:-$HOME/vcpkg}"
OTEL_INSTALL_PREFIX="${OTEL_INSTALL_PREFIX:-/usr/local}"

METHOD="vcpkg"
if [ "$1" = "--source" ]; then
    METHOD="source"
fi

# ── vcpkg (primary) ──────────────────────────────────────────────────────────
install_via_vcpkg() {
    if [ ! -f "$VCPKG_DIR/vcpkg" ]; then
        echo "Installing vcpkg to $VCPKG_DIR ..."
        git clone https://github.com/microsoft/vcpkg "$VCPKG_DIR"
        "$VCPKG_DIR/bootstrap-vcpkg.sh" -disableMetrics
    else
        echo "vcpkg found at $VCPKG_DIR"
    fi

    echo "Installing opentelemetry-cpp $OTEL_VERSION via vcpkg ..."
    "$VCPKG_DIR/vcpkg" install "opentelemetry-cpp:x64-linux"

    echo ""
    echo "Done. Build the server with:"
    echo "  cmake -S . -B build_otel \\"
    echo "    -DCMAKE_BUILD_TYPE=Release \\"
    echo "    -DWITH_OTEL=ON \\"
    echo "    -DCMAKE_TOOLCHAIN_FILE=$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake \\"
    echo "    -DCMAKE_PREFIX_PATH=$VCPKG_DIR/installed/x64-linux"
}

# ── From source (alternative) ────────────────────────────────────────────────
install_from_source() {
    echo "Installing build dependencies ..."
    sudo apt-get install -y \
        build-essential cmake \
        libssl-dev libcurl4-gnutls-dev \
        zlib1g-dev

    WORKDIR=$(mktemp -d)
    trap 'rm -rf "$WORKDIR"' EXIT

    echo "Downloading opentelemetry-cpp $OTEL_VERSION ..."
    wget -q -P "$WORKDIR" \
        "https://github.com/open-telemetry/opentelemetry-cpp/archive/refs/tags/v${OTEL_VERSION}.tar.gz"
    tar xzf "$WORKDIR/v${OTEL_VERSION}.tar.gz" -C "$WORKDIR"

    echo "Building (this takes ~15 minutes) ..."
    cmake -S "$WORKDIR/opentelemetry-cpp-${OTEL_VERSION}" -B "$WORKDIR/build" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$OTEL_INSTALL_PREFIX" \
        -DWITH_OTLP_HTTP=ON \
        -DWITH_OTLP_GRPC=OFF \
        -DBUILD_TESTING=OFF \
        -DWITH_BENCHMARK=OFF \
        -DWITH_EXAMPLES=OFF
    cmake --build "$WORKDIR/build" -j"$(nproc)"
    sudo cmake --install "$WORKDIR/build"

    echo ""
    echo "Done. Build the server with:"
    echo "  cmake -S . -B build_otel \\"
    echo "    -DCMAKE_BUILD_TYPE=Release \\"
    echo "    -DWITH_OTEL=ON"
    echo "  (opentelemetry-cpp installed to $OTEL_INSTALL_PREFIX, found automatically)"
}

case "$METHOD" in
    vcpkg)  install_via_vcpkg ;;
    source) install_from_source ;;
esac
