#include "Python.h"

#include "cursor.h"

static void
set_line(_PyTok_Cursor *cursor, int lineno, _PyTok_Off start,
         _PyTok_Off end)
{
    cursor->pos = start;
    cursor->line_start = start;
    cursor->line_end = end;
    cursor->lineno = lineno;
}

int
_PyTok_CursorSetLine(_PyTok_Cursor *cursor, int lineno)
{
    if (cursor->source == NULL) {
        PyErr_SetString(PyExc_SystemError, "cursor has no tokenizer source");
        return -1;
    }
    const _PyTok_SourceText *source = cursor->source;
    if (lineno > 0 && cursor->lineno == lineno - 1 &&
            lineno <= source->nlines) {
        _PyTok_Off start = cursor->line_end;
        _PyTok_Off end = source->len;
        if (lineno < source->nlines) {
            end = _PyTok_SourceFindLineEnd(source, start);
            if (end < 0) {
                return -1;
            }
        }
        set_line(cursor, lineno, start, end);
        return 0;
    }

    _PyTok_Line line;
    if (_PyTok_SourceLine(source, lineno, &line) < 0) {
        return -1;
    }
    set_line(cursor, lineno, line.start, line.end);
    return 0;
}

int
_PyTok_CursorSetOffset(_PyTok_Cursor *cursor, _PyTok_Off offset)
{
    if (cursor->source == NULL) {
        PyErr_SetString(PyExc_SystemError, "cursor has no tokenizer source");
        return -1;
    }
    const _PyTok_SourceText *source = cursor->source;
    int stays_on_line = cursor->lineno > 0 &&
        offset >= cursor->line_start && offset < cursor->line_end;
    if (!stays_on_line && cursor->lineno > 0 &&
            offset == cursor->line_end && offset == source->len &&
            (offset == 0 || source->bytes[offset - 1] != '\n')) {
        stays_on_line = 1;
    }
    if (stays_on_line) {
        cursor->pos = offset;
        return 0;
    }

    _PyTok_Loc loc;
    if (_PyTok_SourceLocation(
            source, offset, _PYTOK_AFFINITY_RIGHT, &loc) < 0) {
        return -1;
    }
    _PyTok_Off start = offset - loc.byte_col;
    _PyTok_Off end = source->len;
    if (loc.lineno < source->nlines) {
        end = _PyTok_SourceFindLineEnd(source, start);
        if (end < 0) {
            return -1;
        }
    }
    set_line(cursor, loc.lineno, start, end);
    cursor->pos = offset;
    return 0;
}
