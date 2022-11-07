#include "Python.h"
#include "pycore_asdl.h"

GENERATE_ASDL_SEQ_CONSTRUCTOR(generic, void*);
GENERATE_ASDL_SEQ_CONSTRUCTOR(identifier, PyObject*);
GENERATE_ASDL_SEQ_CONSTRUCTOR(int, int);
