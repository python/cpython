#include "ceval.h"
#include "pycore_long.h"

#include "ceval_macros.h"

#if _Py_TAIL_CALL_INTERP
#define _Py_CEVAL_OPCODE_TARGETS_DEFINE
#include "opcode_targets.h"
#undef _Py_CEVAL_OPCODE_TARGETS_DEFINE
#include "generated_cases.c.h"
#endif
