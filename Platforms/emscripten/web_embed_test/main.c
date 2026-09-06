/* Smoke test for libpython linked into a program whose main() is not Python's.
 *
 * Linked without -sMAIN_MODULE and without the interpreter's
 * -sEXPORTED_FUNCTIONS, so it fails if libpython depends on either.
 */

#include <Python.h>
#include <stdio.h>

// Imports come from the preloaded zip, len() goes through the call trampoline.
static const char *SCRIPT =
    "import json, select\n"
    "select.poll().poll(0)\n"
    "print(json.dumps({'embedded': len('ok')}))\n";

int main(void)
{
    PyStatus status;
    PyConfig config;

    PyConfig_InitIsolatedConfig(&config);
    config.write_bytecode = 0;
    config.module_search_paths_set = 1;
    PyWideStringList_Append(&config.module_search_paths, L"/lib/stdlib.zip");
    PyConfig_SetBytesString(&config, &config.executable, "/embed");

    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        puts("web_embed_test: Py_InitializeFromConfig failed");
        return 1;
    }

    if (PyRun_SimpleString(SCRIPT) != 0) {
        puts("web_embed_test: script failed");
        return 1;
    }

    Py_Finalize();
    puts("web_embed_test: ok");
    return 0;
}
