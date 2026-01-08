#!/bin/sh
#
# Compiles the garbage collector
#
set -e

cd "$(dirname "$0")/.."

# Compile with make
make clean >/dev/null 2>&1 || true
make

echo "✓ Compiled successfully"
exit 0
