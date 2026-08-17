#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
rm -f test_log.txt
echo "Installing node packages" | tee -a test_log.txt
npm ci >> test_log.txt 2>&1
echo "Installing playwright browsers" | tee -a test_log.txt
npx playwright install 2>> test_log.txt
# Disable color so FORCE_COLOR=1 (set in CI) doesn't wrap the port number in
# ANSI escape codes, which would produce an invalid baseURL in Playwright.
export PORT=$(FORCE_COLOR=0 npx get-port-cli)
echo "Running tests with webserver on port $PORT" | tee -a test_log.txt
CI=1 npx playwright test | tee -a test_log.txt
