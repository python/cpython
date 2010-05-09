/*
 * A type which wraps a pipe handle in message oriented mode
 *
 * pipe_connection.c
 *
 * Copyright (c) 2006-2008, R Oudkerk --- see COPYING.txt
 */

#include "multiprocessing.h"

#define CLOSE(h) CloseHandle(h)

/*
 * Send string to the pipe; assumes in message oriented mode
 */

static Py_ssize_t
conn_send_string(ConnectionObject *conn, char *string, size_t length)
{
    DWORD amount_written;
    BOOL ret;

    Py_BEGIN_ALLOW_THREADS
    ret = WriteFile(conn->handle, string, length, &amount_written, NULL);
    Py_END_ALLOW_THREADS

    if (ret == 0 && GetLastError() == ERROR_NO_SYSTEM_RESOURCES) {
        PyErr_Format(PyExc_ValueError, "Cannnot send %" PY_FORMAT_SIZE_T "d bytes over connection", length);
        return MP_STANDARD_ERROR;
    }

    return ret ? MP_SUCCESS : MP_STANDARD_ERROR;
}

/*
 * Attempts to read into buffer, or if buffer too small into *newbuffer.
 *
 * Returns number of bytes read.  Assumes in message oriented mode.
 */

static Py_ssize_t
conn_recv_string(ConnectionObject *conn, char *buffer,
                 size_t buflength, char **newbuffer, size_t maxlength)
{
    DWORD left, length, full_length, err;
    BOOL ret;
    *newbuffer = NULL;

    Py_BEGIN_ALLOW_THREADS
    ret = ReadFile(conn->handle, buffer, MIN(buflength, maxlength),
                  &length, NULL);
    Py_END_ALLOW_THREADS
    if (ret)
        return length;

    err = GetLastError();
    if (err != ERROR_MORE_DATA) {
        if (err == ERROR_BROKEN_PIPE)
            return MP_END_OF_FILE;
        return MP_STANDARD_ERROR;
    }

    if (!PeekNamedPipe(conn->handle, NULL, 0, NULL, NULL, &left))
        return MP_STANDARD_ERROR;

    full_length = length + left;
    if (full_length > maxlength)
        return MP_BAD_MESSAGE_LENGTH;

    *newbuffer = PyMem_Malloc(full_length);
    if (*newbuffer == NULL)
        return MP_MEMORY_ERROR;

    memcpy(*newbuffer, buffer, length);

    Py_BEGIN_ALLOW_THREADS
    ret = ReadFile(conn->handle, *newbuffer+length, left, &length, NULL);
    Py_END_ALLOW_THREADS
    if (ret) {
        assert(length == left);
        return full_length;
    } else {
        PyMem_Free(*newbuffer);
        return MP_STANDARD_ERROR;
    }
}

/*
 * Check whether any data is available for reading
 */

static int
conn_poll(ConnectionObject *conn, double timeout, PyThreadState *_save)
{
    DWORD bytes, deadline, delay;
    int difference, res;
    BOOL block = FALSE;

    if (!PeekNamedPipe(conn->handle, NULL, 0, NULL, &bytes, NULL))
        return MP_STANDARD_ERROR;

    if (timeout == 0.0)
        return bytes > 0;

    if (timeout < 0.0)
        block = TRUE;
    else
        /* XXX does not check for overflow */
        deadline = GetTickCount() + (DWORD)(1000 * timeout + 0.5);

    Sleep(0);

    for (delay = 1 ; ; delay += 1) {
        if (!PeekNamedPipe(conn->handle, NULL, 0, NULL, &bytes, NULL))
            return MP_STANDARD_ERROR;
        else if (bytes > 0)
            return TRUE;

        if (!block) {
            difference = deadline - GetTickCount();
            if (difference < 0)
                return FALSE;
            if ((int)delay > difference)
                delay = difference;
        }

        if (delay > 20)
            delay = 20;

        Sleep(delay);

        /* check for signals */
        Py_BLOCK_THREADS
        res = PyErr_CheckSignals();
        Py_UNBLOCK_THREADS

        if (res)
            return MP_EXCEPTION_HAS_BEEN_SET;
    }
}

/*
 * "connection.h" defines the PipeConnection type using the definitions above
 */

#define CONNECTION_NAME "PipeConnection"
#define CONNECTION_TYPE PipeConnectionType

#include "connection.h"
