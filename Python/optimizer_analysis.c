#include "Python.h"
#include "opcode.h"
#include "pycore_interp.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_uops.h"
#include "pycore_long.h"
#include "cpython/optimizer.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "pycore_optimizer.h"


static void
remove_unneeded_uops(_PyUOpInstruction *trace, int trace_length)
{
    // Note that we don't enter stubs, those SET_IPs are needed.
    int last_set_ip = -1;
    bool need_ip = true;
    for (int pc = 0; pc < trace_length; pc++) {
        int opcode = trace[pc].opcode;
        if (opcode == _SET_IP) {
            if (!need_ip && last_set_ip >= 0) {
                trace[last_set_ip].opcode = NOP;
            }
            need_ip = false;
            last_set_ip = pc;
        }
        else if (opcode == _JUMP_TO_TOP || opcode == _EXIT_TRACE) {
            break;
        }
        else {
            // If opcode has ERROR or DEOPT, set need_ip to true
            if (_PyOpcode_opcode_metadata[opcode].flags & (HAS_ERROR_FLAG | HAS_DEOPT_FLAG) || opcode == _PUSH_FRAME) {
                need_ip = true;
            }
        }
    }
}


int
_Py_uop_analyze_and_optimize(
    PyCodeObject *co,
    _PyUOpInstruction *trace,
    int trace_len,
    int curr_stacklen
)
{
    remove_unneeded_uops(trace, trace_len);
    return trace_len;
}
