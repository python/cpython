// To enable signal handling, the embedder should:
// 1. set Module.Py_EmscriptenSignalBuffer = some_shared_array_buffer;
// 2. set the Py_EMSCRIPTEN_SIGNAL_HANDLING flag to 1 as follows:
//    Module.HEAP8[Module._Py_EMSCRIPTEN_SIGNAL_HANDLING] = 1
//
// The address &Py_EMSCRIPTEN_SIGNAL_HANDLING is exported as
// Module._Py_EMSCRIPTEN_SIGNAL_HANDLING.
#include <emscripten.h>
#include "Python.h"

EM_JS(int, _Py_CheckEmscriptenSignals_Helper, (void), {
    if (!Module.Py_EmscriptenSignalBuffer) {
        return 0;
    }
    try {
        let result = Module.Py_EmscriptenSignalBuffer[0];
        Module.Py_EmscriptenSignalBuffer[0] = 0;
        return result;
    } catch(e) {
#if !defined(NDEBUG)
        console.warn("Error occurred while trying to read signal buffer:", e);
#endif
        return 0;
    }
});

EMSCRIPTEN_KEEPALIVE int Py_EMSCRIPTEN_SIGNAL_HANDLING = 0;

void
_Py_CheckEmscriptenSignals(void)
{
    if (!Py_EMSCRIPTEN_SIGNAL_HANDLING) {
        return;
    }
    int signal = _Py_CheckEmscriptenSignals_Helper();
    if (signal) {
        PyErr_SetInterruptEx(signal);
    }
}


#define PY_EMSCRIPTEN_SIGNAL_INTERVAL 50
static int emscripten_signal_clock = PY_EMSCRIPTEN_SIGNAL_INTERVAL;

void
_Py_CheckEmscriptenSignalsPeriodically(void)
{
    if (!Py_EMSCRIPTEN_SIGNAL_HANDLING) {
        return;
    }
    emscripten_signal_clock--;
    if (emscripten_signal_clock == 0) {
        emscripten_signal_clock = PY_EMSCRIPTEN_SIGNAL_INTERVAL;
        _Py_CheckEmscriptenSignals();
    }
}
