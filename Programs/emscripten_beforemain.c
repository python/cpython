/* Emscripten setup for when Python is the program. Linked into the
 * interpreter only, never into libpython.
 *
 * Runs main() under WebAssembly.promising so that libpython can suspend the
 * wasm stack in a syscall (see Python/emscripten_syscalls.c).
 */

void _PyEmscripten_BeforeMain_js(void);

__attribute__((constructor)) void
_PyEmscripten_BeforeMain(void)
{
    _PyEmscripten_BeforeMain_js();
}
