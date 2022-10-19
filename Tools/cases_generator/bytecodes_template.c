#include "Python.h"
#include "opcode.h"

#define inst(name, stack_effect) case name:

#define family(name) static int family_##name

static void
dummy_func(unsigned char opcode)
{
    switch (opcode) {

// BEGIN BYTECODES //
// INSERT CASES HERE //
// END BYTECODES //

    }
}

// Families go below this point //

