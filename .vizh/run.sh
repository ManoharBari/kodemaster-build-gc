#!/bin/sh
#
# Runs the garbage collector demo program
#
set -e

cd "$(dirname "$0")/.."

# Ensure compiled
if [ ! -f "./your_program" ]; then
    make >/dev/null 2>&1
fi

# Execute the program
exec ./your_program "$@"
