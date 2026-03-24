import { defineConfig, devices } from '@playwright/test';

const port = process.env.PORT ?? "8787";

export default defineConfig({
  testDir: '.',
  forbidOnly: true,
  retries: 2,
  reporter: process.env.CI ? 'dot' : 'html',
  use: {
    baseURL: `http://localhost:${port}`,
    trace: 'on-first-retry',
  },
  projects: [
    {
      name: 'chromium',
      use: { ...devices['Desktop Chrome'] },
    },
  ],
  webServer: {
    command: `npx http-server ../../../cross-build/wasm32-emscripten/build/python/web_example_pyrepl_jspi/ -p ${port}`,
    url: `http://localhost:${port}`,
  },
});
