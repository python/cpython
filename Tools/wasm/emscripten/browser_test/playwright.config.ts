import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
  testDir: '.',
  forbidOnly: true,
  retries: 2,
  reporter: process.env.CI ? 'dot' : 'html',
  use: {
    baseURL: 'http://localhost:8787',
    trace: 'on-first-retry',
  },
  projects: [
    {
      name: 'chromium',
      use: { ...devices['Desktop Chrome'] },
    },
  ],
  webServer: {
    command: 'npx http-server ../../../../cross-build/wasm32-emscripten/build/python/web_example_pyrepl_jspi/ -p 8787',
    url: 'http://localhost:8787',
  },
});
