#!/bin/bash
npm ci
npx playwright install
CI=1 npx playwright test
