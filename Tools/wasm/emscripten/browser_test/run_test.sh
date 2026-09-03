#!/bin/bash
# Redirect to new location
exec "$(dirname "$0")/../../../../Platforms/emscripten/browser_test/run_test.sh" "$@"
