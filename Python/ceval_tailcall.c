#include "pyconfig.h"  // _Py_TAIL_CALL_INTERP

#if _Py_TAIL_CALL_INTERP
#include "ceval.h"
#include "pycore_long.h"

#include "ceval_macros.h"

#define _Py_CEVAL_OPCODE_TARGETS_DEFINE
#include "opcode_targets.h"
#undef _Py_CEVAL_OPCODE_TARGETS_DEFINE
#include "generated_cases.c.h"
#endif
