#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
rm -f test_log.txt
echo "Installing node packages" | tee test_log.txt
npm ci >> test_log.txt 2>&1
echo "Installing playwright browsers" | tee test_log.txt
npx playwright install 2>> test_log.txt
export PORT=$(npx get-port-cli)
echo "Running tests with webserver on port $PORT" | tee test_log.txt
CI=1 npx playwright test | tee test_log.txt
