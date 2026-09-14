/* Smoke test for libpython linked into a program whose main() is not Python's. */

#include <Python.h>
#include <stdio.h>

// Imports come from the preloaded zip, len() goes through the call trampoline
// and poll() through the syscall overrides.
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
    status = PyWideStringList_Append(&config.module_search_paths,
                                     L"/lib/stdlib.zip");
    if (PyStatus_Exception(status)) {
        goto exception;
    }
    status = PyConfig_SetBytesString(&config, &config.executable, "/embed");
    if (PyStatus_Exception(status)) {
        goto exception;
    }
    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        goto exception;
    }
    PyConfig_Clear(&config);

    if (PyRun_SimpleString(SCRIPT) != 0) {
        puts("web_embed_test: script failed");
        return 1;
    }
    if (Py_FinalizeEx() < 0) {
        puts("web_embed_test: Py_FinalizeEx failed");
        return 1;
    }
    puts("web_embed_test: ok");
    return 0;

exception:
    PyConfig_Clear(&config);
    Py_ExitStatusException(status);
}
