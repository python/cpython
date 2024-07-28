/*
 * This file contains various macro definitions in order to reduce the
 * number of lines in translate.c. Do not use them for something else.
 */

#ifndef _FNMATCH_MACROS_H
#define _FNMATCH_MACROS_H

// ==== Macro definitions =====================================================

/*
 * Check that INTVAL is >= 0 or execute 'goto abort'.
 *
 * This macro is provided for convenience and should be
 * carefully used if more resources should be released
 * before jumping to the 'abort' label.
 */
#define CHECK_INTVAL_OR_ABORT(INTVAL)   \
    do {                                \
        if ((INTVAL) < 0) {             \
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
#define CHECK_NON_NULL_OR_ABORT(OBJ)  \
    do {                        \
        if ((OBJ) == NULL) {    \
            goto abort;         \
        }                       \
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
#define _WRITE_BLOCK(WRITER, STRING, START, STOP)                   \
    _PyUnicodeWriter_WriteSubstring((_PyUnicodeWriter *)(WRITER),   \
                                    (STRING), (START), (STOP))

// ----------------------------------------------------------------------------

/* Write a character CHAR or execute the ON_ERROR statements if it fails. */
#define _WRITE_CHAR_OR(WRITER, CHAR, ON_ERROR)      \
    do {                                            \
        if (_WRITE_CHAR((WRITER), (CHAR)) < 0) {    \
            ON_ERROR;                               \
        }                                           \
    } while (0)

/*
 * Write an ASCII string STRING of given length LENGTH,
 * or execute the ON_ERROR statements if it fails.
 */
#define _WRITE_ASCII_OR(WRITER, ASCII, LENGTH, ON_ERROR)        \
    do {                                                        \
        if (_WRITE_ASCII((WRITER), (ASCII), (LENGTH)) < 0) {    \
            ON_ERROR;                                           \
        }                                                       \
    } while (0)

/* Write the string STRING or execute the ON_ERROR statements if it fails. */
#define _WRITE_STRING_OR(WRITER, STRING, ON_ERROR)      \
    do {                                                \
        if (_WRITE_STRING((WRITER), (STRING)) < 0) {    \
            ON_ERROR;                                   \
        }                                               \
    } while (0)

/*
 * Write the substring STRING[START:STOP] (no-op if the substring is empty)
 * or execute the ON_ERROR statements if it fails.
 */
#define _WRITE_SUBSTRING_OR(WRITER, STRING, START, STOP, ON_ERROR)      \
    do {                                                                \
        /* intermediate variables to allow in-place operations */       \
        Py_ssize_t _i = (START), _j = (STOP);                           \
        if (_i < _j && _WRITE_BLOCK((WRITER), (STRING), _i, _j) < 0) {  \
            ON_ERROR;                                                   \
        }                                                               \
    } while (0)

// ----------------------------------------------------------------------------

// Macros which execute "goto abort" if an error occurs.

#define WRITE_CHAR_OR_ABORT(WRITER, CHAR) \
    _WRITE_CHAR_OR((WRITER), (CHAR), goto abort)
#define WRITE_ASCII_OR_ABORT(WRITER, STRING, LENGTH) \
    _WRITE_ASCII_OR((WRITER), (STRING), (LENGTH), goto abort)
#define WRITE_STRING_OR_ABORT(WRITER, STRING) \
    _WRITE_STRING_OR((WRITER), (STRING), goto abort)
#define WRITE_BLOCK_OR_ABORT(WRITER, STRING, START, STOP) \
    _WRITE_SUBSTRING_OR((WRITER), (STRING), (START), (STOP), goto abort)

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
