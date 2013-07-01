/*
 * A type which wraps a socket
 *
 * socket_connection.c
 *
 * Copyright (c) 2006-2008, R Oudkerk --- see COPYING.txt
 */

#include "multiprocessing.h"

#if defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)
#  include "poll.h"
#endif

#ifdef MS_WINDOWS
#  define WRITE(h, buffer, length) send((SOCKET)h, buffer, length, 0)
#  define READ(h, buffer, length) recv((SOCKET)h, buffer, length, 0)
#  define CLOSE(h) closesocket((SOCKET)h)
#else
#  define WRITE(h, buffer, length) write(h, buffer, length)
#  define READ(h, buffer, length) read(h, buffer, length)
#  define CLOSE(h) close(h)
#endif

/*
 * Wrapper for PyErr_CheckSignals() which can be called without the GIL
 */

static int
check_signals(void)
{
    PyGILState_STATE state;
    int res;
    state = PyGILState_Ensure();
    res = PyErr_CheckSignals();
    PyGILState_Release(state);
    return res;
}

/*
 * Send string to file descriptor
 */

static Py_ssize_t
_conn_sendall(HANDLE h, char *string, size_t length)
{
    char *p = string;
    Py_ssize_t res;

    while (length > 0) {
        res = WRITE(h, p, length);
        if (res < 0) {
            if (errno == EINTR) {
                if (check_signals() < 0)
                    return MP_EXCEPTION_HAS_BEEN_SET;
                continue;
            }
            return MP_SOCKET_ERROR;
        }
        length -= res;
        p += res;
    }

    return MP_SUCCESS;
}

/*
 * Receive string of exact length from file descriptor
 */

static Py_ssize_t
_conn_recvall(HANDLE h, char *buffer, size_t length)
{
    size_t remaining = length;
    Py_ssize_t temp;
    char *p = buffer;

    while (remaining > 0) {
        temp = READ(h, p, remaining);
        if (temp < 0) {
            if (errno == EINTR) {
                if (check_signals() < 0)
                    return MP_EXCEPTION_HAS_BEEN_SET;
                continue;
            }
            return temp;
        }
        else if (temp == 0) {
            return remaining == length ? MP_END_OF_FILE : MP_EARLY_END_OF_FILE;
        }
        remaining -= temp;
        p += temp;
    }

    return MP_SUCCESS;
}

/*
 * Send a string prepended by the string length in network byte order
 */

static Py_ssize_t
conn_send_string(ConnectionObject *conn, char *string, size_t length)
{
    Py_ssize_t res;
    /* The "header" of the message is a 32 bit unsigned number (in
       network order) which specifies the length of the "body".  If
       the message is shorter than about 16kb then it is quicker to
       combine the "header" and the "body" of the message and send
       them at once. */
    if (length < (16*1024)) {
        char *message;

        message = PyMem_Malloc(length+4);
        if (message == NULL)
            return MP_MEMORY_ERROR;

        *(UINT32*)message = htonl((UINT32)length);
        memcpy(message+4, string, length);
        Py_BEGIN_ALLOW_THREADS
        res = _conn_sendall(conn->handle, message, length+4);
        Py_END_ALLOW_THREADS
        PyMem_Free(message);
    } else {
        UINT32 lenbuff;

        if (length > MAX_MESSAGE_LENGTH)
            return MP_BAD_MESSAGE_LENGTH;

        lenbuff = htonl((UINT32)length);
        Py_BEGIN_ALLOW_THREADS
        res = _conn_sendall(conn->handle, (char*)&lenbuff, 4) ||
            _conn_sendall(conn->handle, string, length);
        Py_END_ALLOW_THREADS
    }
    return res;
}

/*
 * Attempts to read into buffer, or failing that into *newbuffer
 *
 * Returns number of bytes read.
 */

static Py_ssize_t
conn_recv_string(ConnectionObject *conn, char *buffer,
                 size_t buflength, char **newbuffer, size_t maxlength)
{
    Py_ssize_t res;
    UINT32 ulength;

    *newbuffer = NULL;

    Py_BEGIN_ALLOW_THREADS
    res = _conn_recvall(conn->handle, (char*)&ulength, 4);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return res;

    ulength = ntohl(ulength);
    if (ulength > maxlength)
        return MP_BAD_MESSAGE_LENGTH;

    if (ulength > buflength) {
        *newbuffer = buffer = PyMem_Malloc((size_t)ulength);
        if (buffer == NULL)
            return MP_MEMORY_ERROR;
    }

    Py_BEGIN_ALLOW_THREADS
    res = _conn_recvall(conn->handle, buffer, (size_t)ulength);
    Py_END_ALLOW_THREADS

    if (res >= 0) {
        res = (Py_ssize_t)ulength;
    } else if (*newbuffer != NULL) {
        PyMem_Free(*newbuffer);
        *newbuffer = NULL;
    }
    return res;
}

/*
 * Check whether any data is available for reading -- neg timeout blocks
 */

static int
conn_poll(ConnectionObject *conn, double timeout, PyThreadState *_save)
{
#if defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)
    int res;
    struct pollfd p;

    p.fd = (int)conn->handle;
    p.events = POLLIN | POLLPRI;
    p.revents = 0;

    if (timeout < 0) {
        do {
            res = poll(&p, 1, -1);
        } while (res < 0 && errno == EINTR);
    } else {
        res = poll(&p, 1, (int)(timeout * 1000 + 0.5));
        if (res < 0 && errno == EINTR) {
            /* We were interrupted by a signal.  Just indicate a
               timeout even though we are early. */
            return FALSE;
        }
    }

    if (res < 0) {
        return MP_SOCKET_ERROR;
    } else if (p.revents & (POLLNVAL|POLLERR)) {
        Py_BLOCK_THREADS
        PyErr_SetString(PyExc_IOError, "poll() gave POLLNVAL or POLLERR");
        Py_UNBLOCK_THREADS
        return MP_EXCEPTION_HAS_BEEN_SET;
    } else if (p.revents != 0) {
        return TRUE;
    } else {
        assert(res == 0);
        return FALSE;
    }
#else
    int res;
    fd_set rfds;

    /*
     * Verify the handle, issue 3321. Not required for windows.
     */
    #ifndef MS_WINDOWS
        if (((int)conn->handle) < 0 || ((int)conn->handle) >= FD_SETSIZE) {
            Py_BLOCK_THREADS
            PyErr_SetString(PyExc_IOError, "handle out of range in select()");
            Py_UNBLOCK_THREADS
            return MP_EXCEPTION_HAS_BEEN_SET;
        }
    #endif

    FD_ZERO(&rfds);
    FD_SET((SOCKET)conn->handle, &rfds);

    if (timeout < 0.0) {
        do {
            res = select((int)conn->handle+1, &rfds, NULL, NULL, NULL);
        } while (res < 0 && errno == EINTR);
    } else {
        struct timeval tv;
        tv.tv_sec = (long)timeout;
        tv.tv_usec = (long)((timeout - tv.tv_sec) * 1e6 + 0.5);
        res = select((int)conn->handle+1, &rfds, NULL, NULL, &tv);
        if (res < 0 && errno == EINTR) {
            /* We were interrupted by a signal.  Just indicate a
               timeout even though we are early. */
            return FALSE;
        }
    }

    if (res < 0) {
        return MP_SOCKET_ERROR;
    } else if (FD_ISSET(conn->handle, &rfds)) {
        return TRUE;
    } else {
        assert(res == 0);
        return FALSE;
    }
#endif
}

/*
 * "connection.h" defines the Connection type using defs above
 */

#define CONNECTION_NAME "Connection"
#define CONNECTION_TYPE ConnectionType

#include "connection.h"
