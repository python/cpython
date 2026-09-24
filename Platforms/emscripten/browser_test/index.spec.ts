import { test, expect } from '@playwright/test';

test('has title', async ({ page }) => {
  await page.goto('/');

  await expect(page).toHaveTitle("Emscripten PyRepl Example");
  const xterm = await page.locator('css=#terminal');
  await expect(xterm).toHaveText(/Python.*on emscripten.*Type.*for more information/);
  const xtermInput = await page.getByRole('textbox');
  await xtermInput.pressSequentially(`def f():\nprint("hello", "emscripten repl!")\n\n`);
  await xtermInput.pressSequentially(`f()\n`);
  await expect(xterm).toHaveText(/hello emscripten repl!/);

});

