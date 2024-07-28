/*
 * This file contains various macro definitions in order to reduce the
 * number of lines in translate.c. Do not use them for something else.
 */

#ifndef _FNMATCH_MACROS_H
#define _FNMATCH_MACROS_H

// ==== Macro definitions =====================================================

/*
 * Check that STATUS is >= 0 or execute 'goto abort'.
 *
 * This macro is provided for convenience and should be
 * carefully used if more resources should be released
 * before jumping to the 'abort' label.
 */
#define CHECK_RET_CODE_OR_ABORT(STATUS) \
    do {                                \
        if ((STATUS) < 0) {             \
            assert(PyErr_Occurred());   \
            goto abort;                 \
        }                               \
    } while (0)

/*
 * Check that OBJ is not NULL or execute 'goto abort'.
 *
 * This macro is provided for convenience and should be
 * carefully used if more resources should be released
 * before jumping to the 'abort' label.
 */
#define CHECK_NOT_NULL_OR_ABORT(OBJ)    \
    do {                                \
        if ((OBJ) == NULL) {            \
            goto abort;                 \
        }                               \
    } while (0)

// The following _WRITE_* and _WRITE_*_OR macros do NOT check their inputs
// since they directly delegate to the _PyUnicodeWriter_Write* underlying
// function. In particular, the caller is responsible for type safety.

/* Cast WRITER and call _PyUnicodeWriter_WriteChar(). */
#define _WRITE_CHAR(WRITER, CHAR) \
    _PyUnicodeWriter_WriteChar((_PyUnicodeWriter *)(WRITER), (CHAR))

/* Cast WRITER and call _PyUnicodeWriter_WriteASCIIString(). */
#define _WRITE_ASCII(WRITER, STRING, LENGTH)                        \
    _PyUnicodeWriter_WriteASCIIString((_PyUnicodeWriter *)(WRITER), \
                                      (STRING), (LENGTH))

/* Cast WRITER and call _PyUnicodeWriter_WriteStr(). */
#define _WRITE_STRING(WRITER, STRING) \
    _PyUnicodeWriter_WriteStr((_PyUnicodeWriter *)(WRITER), (STRING))

/* Cast WRITER and call _PyUnicodeWriter_WriteSubstring(). */
#define _WRITE_SUBSTRING(WRITER, STRING, START, STOP)               \
    _PyUnicodeWriter_WriteSubstring((_PyUnicodeWriter *)(WRITER),   \
                                    (STRING), (START), (STOP))

// ----------------------------------------------------------------------------

/* Write the character CHAR or jump to the 'abort' label on failure. */
#define WRITE_CHAR_OR_ABORT(WRITER, CHAR) \
    CHECK_RET_CODE_OR_ABORT(_WRITE_CHAR((WRITER), (CHAR)))

/*
 * Write an ASCII string STRING of given length LENGTH,
 * or jump to the 'abort' label on failure.
 */
#define WRITE_ASCII_OR_ABORT(WRITER, ASCII, LENGTH)  \
    CHECK_RET_CODE_OR_ABORT(_WRITE_ASCII((WRITER), (ASCII), (LENGTH)))

/* Write the string STRING or jump to the 'abort' label on failure. */
#define WRITE_STRING_OR_ABORT(WRITER, STRING)  \
    CHECK_RET_CODE_OR_ABORT(_WRITE_STRING((WRITER), (STRING)))

/*
 * Write the substring STRING[START:STOP] (no-op if empty)
 * or jump to the 'abort' label on failure.
 */
#define WRITE_SUBSTRING_OR_ABORT(WRITER, STRING, START, STOP)           \
    do {                                                                \
        const Py_ssize_t _START = (START);                              \
        const Py_ssize_t _STOP = (STOP);                                \
        int _RC = _WRITE_SUBSTRING((WRITER), (STRING), _START, _STOP);  \
        CHECK_RET_CODE_OR_ABORT(_RC);                                   \
    } while (0)

// ----------------------------------------------------------------------------

/* Replace backslashes in STRING by escaped backslashes. */
#define BACKSLASH_REPLACE(STATE, STRING)    \
    PyObject_CallMethodObjArgs(             \
        (STRING),                           \
        &_Py_ID(replace),                   \
        (STATE)->backslash_str,             \
        (STATE)->backslash_esc_str,         \
        NULL                                \
    )

/* Replace hyphens in STRING by escaped hyphens. */
#define HYPHEN_REPLACE(STATE, STRING)       \
    PyObject_CallMethodObjArgs(             \
        (STRING),                           \
        &_Py_ID(replace),                   \
        (STATE)->hyphen_str,                \
        (STATE)->hyphen_esc_str,            \
        NULL                                \
    )

/*
 * Escape set operations in STRING using re.sub().
 *
 * SETOPS_RE_SUB_METH is a reference to re.compile('([&~|])').sub().
 */
#define SETOPS_REPLACE(STATE, STRING, SETOPS_RE_SUB_METH)   \
    PyObject_CallFunctionObjArgs(                           \
        (SETOPS_RE_SUB_METH),                               \
        (STATE)->setops_repl_str,                           \
        (STRING),                                           \
        NULL                                                \
    )

#endif // _FNMATCH_MACROS_H
