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
remove_unneeded_uops(_PyUOpInstruction *buffer, int buffer_size)
{
    // Note that we don't enter stubs, those SET_IPs are needed.
    int last_set_ip = -1;
    bool need_ip = true;
    for (int pc = 0; pc < buffer_size; pc++) {
        int opcode = buffer[pc].opcode;
        if (opcode == _SET_IP) {
            if (!need_ip && last_set_ip >= 0) {
                buffer[last_set_ip].opcode = NOP;
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
    _PyUOpInstruction *buffer,
    int buffer_size,
    int curr_stacklen
)
{
    remove_unneeded_uops(buffer, buffer_size);
    return 0;
}
