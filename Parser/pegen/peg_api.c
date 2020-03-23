#include <pegen_interface.h>

#include "../tokenizer.h"
#include "pegen.h"

mod_ty
PyPegen_ASTFromString(const char *str, PyArena *arena)
{
    return run_parser_from_string(str, parse_start, RAW_AST_OBJECT, arena);
}

mod_ty
PyPegen_ASTFromFile(const char *filename, PyArena *arena)
{
    return run_parser_from_file(filename, parse_start, RAW_AST_OBJECT, arena);
}

PyCodeObject *
PyPegen_CodeObjectFromString(const char *str, PyArena *arena)
{
    return run_parser_from_string(str, parse_start, CODE_OBJECT, arena);
}

PyCodeObject *
PyPegen_CodeObjectFromFile(const char *file, PyArena *arena)
{
    return run_parser_from_file(file, parse_start, CODE_OBJECT, arena);
}
