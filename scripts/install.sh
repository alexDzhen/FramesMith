#!/usr/bin/env sh
# Builds framesmith if needed, installs it, and grants the capability that
# lets --send work without sudo. Usage: scripts/install.sh [prefix]
set -eu

cd "$(dirname "$0")/.."
PREFIX="${1:-/usr/local}"

if [ ! -x build/framesmith ]; then
    ./scripts/build.sh
fi

sudo cmake --install build --prefix "$PREFIX"

if sudo setcap cap_net_raw+ep "$PREFIX/bin/framesmith"; then
    echo "Installed $PREFIX/bin/framesmith with CAP_NET_RAW (so --send needs no sudo)."
else
    echo "Installed $PREFIX/bin/framesmith, but the capability could not be set."
    echo "Install setcap (package libcap2-bin) and rerun, or use sudo when sending."
fi
