/* Socket module */

/*

This module provides an interface to Berkeley socket IPC.

Limitations:

- Only AF_INET, AF_INET6 and AF_UNIX address families are supported in a
  portable manner, though AF_PACKET, AF_NETLINK and AF_TIPC are supported
  under Linux.
- No read/write operations (use sendall/recv or makefile instead).
- Additional restrictions apply on some non-Unix platforms (compensated
  for by socket.py).

Module interface:

- socket.error: exception raised for socket specific errors
- socket.gaierror: exception raised for getaddrinfo/getnameinfo errors,
    a subclass of socket.error
- socket.herror: exception raised for gethostby* errors,
    a subclass of socket.error
- socket.gethostbyname(hostname) --> host IP address (string: 'dd.dd.dd.dd')
- socket.gethostbyaddr(IP address) --> (hostname, [alias, ...], [IP addr, ...])
- socket.gethostname() --> host name (string: 'spam' or 'spam.domain.com')
- socket.getprotobyname(protocolname) --> protocol number
- socket.getservbyname(servicename[, protocolname]) --> port number
- socket.getservbyport(portnumber[, protocolname]) --> service name
- socket.socket([family[, type [, proto, fileno]]]) --> new socket object
    (fileno specifies a pre-existing socket file descriptor)
- socket.socketpair([family[, type [, proto]]]) --> (socket, socket)
- socket.ntohs(16 bit value) --> new int object
- socket.ntohl(32 bit value) --> new int object
- socket.htons(16 bit value) --> new int object
- socket.htonl(32 bit value) --> new int object
- socket.getaddrinfo(host, port [, family, socktype, proto, flags])
    --> List of (family, socktype, proto, canonname, sockaddr)
- socket.getnameinfo(sockaddr, flags) --> (host, port)
- socket.AF_INET, socket.SOCK_STREAM, etc.: constants from <socket.h>
- socket.has_ipv6: boolean value indicating if IPv6 is supported
- socket.inet_aton(IP address) -> 32-bit packed IP representation
- socket.inet_ntoa(packed IP) -> IP address string
- socket.getdefaulttimeout() -> None | float
- socket.setdefaulttimeout(None | float)
- socket.if_nameindex() -> list of tuples (if_index, if_name)
- socket.if_nametoindex(name) -> corresponding interface index
- socket.if_indextoname(index) -> corresponding interface name
- an Internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket address is a string specifying the pathname
- an AF_PACKET socket address is a tuple containing a string
  specifying the ethernet interface and an integer specifying
  the Ethernet protocol number to be received. For example:
  ("eth0",0x1234).  Optional 3rd,4th,5th elements in the tuple
  specify packet-type and ha-type/addr.
- an AF_TIPC socket address is expressed as
 (addr_type, v1, v2, v3 [, scope]); where addr_type can be one of:
    TIPC_ADDR_NAMESEQ, TIPC_ADDR_NAME, and TIPC_ADDR_ID;
  and scope can be one of:
    TIPC_ZONE_SCOPE, TIPC_CLUSTER_SCOPE, and TIPC_NODE_SCOPE.
  The meaning of v1, v2 and v3 depends on the value of addr_type:
    if addr_type is TIPC_ADDR_NAME:
        v1 is the server type
        v2 is the port identifier
        v3 is ignored
    if addr_type is TIPC_ADDR_NAMESEQ:
        v1 is the server type
        v2 is the lower port number
        v3 is the upper port number
    if addr_type is TIPC_ADDR_ID:
        v1 is the node
        v2 is the ref
        v3 is ignored


Local naming conventions:

- names starting with sock_ are socket object methods
- names starting with socket_ are module-level functions
- names starting with PySocket are exported through socketmodule.h

*/

#ifdef __APPLE__
  /*
   * inet_aton is not available on OSX 10.3, yet we want to use a binary
   * that was build on 10.4 or later to work on that release, weak linking
   * comes to the rescue.
   */
# pragma weak inet_aton
#endif

#include "Python.h"
#include "structmember.h"

#undef MAX
#define MAX(x, y) ((x) < (y) ? (y) : (x))

/* Socket object documentation */
PyDoc_STRVAR(sock_doc,
"socket([family[, type[, proto]]]) -> socket object\n\
\n\
Open a socket of the given type.  The family argument specifies the\n\
address family; it defaults to AF_INET.  The type argument specifies\n\
whether this is a stream (SOCK_STREAM, this is the default)\n\
or datagram (SOCK_DGRAM) socket.  The protocol argument defaults to 0,\n\
specifying the default protocol.  Keyword arguments are accepted.\n\
\n\
A socket object represents one endpoint of a network connection.\n\
\n\
Methods of socket objects (keyword arguments not allowed):\n\
\n\
_accept() -- accept connection, returning new socket fd and client address\n\
bind(addr) -- bind the socket to a local address\n\
close() -- close the socket\n\
connect(addr) -- connect the socket to a remote address\n\
connect_ex(addr) -- connect, return an error code instead of an exception\n\
_dup() -- return a new socket fd duplicated from fileno()\n\
fileno() -- return underlying file descriptor\n\
getpeername() -- return remote address [*]\n\
getsockname() -- return local address\n\
getsockopt(level, optname[, buflen]) -- get socket options\n\
gettimeout() -- return timeout or None\n\
listen(n) -- start listening for incoming connections\n\
recv(buflen[, flags]) -- receive data\n\
recv_into(buffer[, nbytes[, flags]]) -- receive data (into a buffer)\n\
recvfrom(buflen[, flags]) -- receive data and sender\'s address\n\
recvfrom_into(buffer[, nbytes, [, flags])\n\
  -- receive data and sender\'s address (into a buffer)\n\
sendall(data[, flags]) -- send all data\n\
send(data[, flags]) -- send data, may not send all of it\n\
sendto(data[, flags], addr) -- send data to a given address\n\
setblocking(0 | 1) -- set or clear the blocking I/O flag\n\
setsockopt(level, optname, value) -- set socket options\n\
settimeout(None | float) -- set or clear the timeout\n\
shutdown(how) -- shut down traffic in one or both directions\n\
if_nameindex() -- return all network interface indices and names\n\
if_nametoindex(name) -- return the corresponding interface index\n\
if_indextoname(index) -- return the corresponding interface name\n\
\n\
 [*] not available on all platforms!");

/* XXX This is a terrible mess of platform-dependent preprocessor hacks.
   I hope some day someone can clean this up please... */

/* Hacks for gethostbyname_r().  On some non-Linux platforms, the configure
   script doesn't get this right, so we hardcode some platform checks below.
   On the other hand, not all Linux versions agree, so there the settings
   computed by the configure script are needed! */

#ifndef linux
# undef HAVE_GETHOSTBYNAME_R_3_ARG
# undef HAVE_GETHOSTBYNAME_R_5_ARG
# undef HAVE_GETHOSTBYNAME_R_6_ARG
#endif

#if defined(__OpenBSD__)
# include <sys/uio.h>
#endif

#ifndef WITH_THREAD
# undef HAVE_GETHOSTBYNAME_R
#endif

#ifdef HAVE_GETHOSTBYNAME_R
# if defined(_AIX)
#  define HAVE_GETHOSTBYNAME_R_3_ARG
# elif defined(__sun) || defined(__sgi)
#  define HAVE_GETHOSTBYNAME_R_5_ARG
# elif defined(linux)
/* Rely on the configure script */
# else
#  undef HAVE_GETHOSTBYNAME_R
# endif
#endif

#if !defined(HAVE_GETHOSTBYNAME_R) && defined(WITH_THREAD) && \
    !defined(MS_WINDOWS)
# define USE_GETHOSTBYNAME_LOCK
#endif

/* To use __FreeBSD_version */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
/* On systems on which getaddrinfo() is believed to not be thread-safe,
   (this includes the getaddrinfo emulation) protect access with a lock. */
#if defined(WITH_THREAD) && (defined(__APPLE__) || \
    (defined(__FreeBSD__) && __FreeBSD_version+0 < 503000) || \
    defined(__OpenBSD__) || defined(__NetBSD__) || \
    defined(__VMS) || !defined(HAVE_GETADDRINFO))
#define USE_GETADDRINFO_LOCK
#endif

#ifdef USE_GETADDRINFO_LOCK
#define ACQUIRE_GETADDRINFO_LOCK PyThread_acquire_lock(netdb_lock, 1);
#define RELEASE_GETADDRINFO_LOCK PyThread_release_lock(netdb_lock);
#else
#define ACQUIRE_GETADDRINFO_LOCK
#define RELEASE_GETADDRINFO_LOCK
#endif

#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
# include "pythread.h"
#endif

#if defined(PYCC_VACPP)
# include <types.h>
# include <io.h>
# include <sys/ioctl.h>
# include <utils.h>
# include <ctype.h>
#endif

#if defined(__VMS)
#  include <ioctl.h>
#endif

#ifdef __APPLE__
# include <sys/ioctl.h>
#endif


#if defined(PYOS_OS2)
# define  INCL_DOS
# define  INCL_DOSERRORS
# define  INCL_NOPMAPI
# include <os2.h>
#endif

#if defined(__sgi) && _COMPILER_VERSION>700 && !_SGIAPI
/* make sure that the reentrant (gethostbyaddr_r etc)
   functions are declared correctly if compiling with
   MIPSPro 7.x in ANSI C mode (default) */

/* XXX Using _SGIAPI is the wrong thing,
   but I don't know what the right thing is. */
#undef _SGIAPI /* to avoid warning */
#define _SGIAPI 1

#undef _XOPEN_SOURCE
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#ifdef _SS_ALIGNSIZE
#define HAVE_GETADDRINFO 1
#define HAVE_GETNAMEINFO 1
#endif

#define HAVE_INET_PTON
#include <netdb.h>
#endif

/* Irix 6.5 fails to define this variable at all. This is needed
   for both GCC and SGI's compiler. I'd say that the SGI headers
   are just busted. Same thing for Solaris. */
#if (defined(__sgi) || defined(sun)) && !defined(INET_ADDRSTRLEN)
#define INET_ADDRSTRLEN 16
#endif

/* Generic includes */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

/* Generic socket object definitions and includes */
#define PySocket_BUILDING_SOCKET
#include "socketmodule.h"

/* Addressing includes */

#ifndef MS_WINDOWS

/* Non-MS WINDOWS includes */
# include <netdb.h>
# include <unistd.h>

/* Headers needed for inet_ntoa() and inet_addr() */
# if defined(PYOS_OS2) && defined(PYCC_VACPP)
#  include <netdb.h>
typedef size_t socklen_t;
# else
#   include <arpa/inet.h>
# endif

#  include <fcntl.h>

#else

/* MS_WINDOWS includes */
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif

#endif

#include <stddef.h>

#ifndef offsetof
# define offsetof(type, member) ((size_t)(&((type *)0)->member))
#endif

#ifndef O_NONBLOCK
# define O_NONBLOCK O_NDELAY
#endif

/* include Python's addrinfo.h unless it causes trouble */
#if defined(__sgi) && _COMPILER_VERSION>700 && defined(_SS_ALIGNSIZE)
  /* Do not include addinfo.h on some newer IRIX versions.
   * _SS_ALIGNSIZE is defined in sys/socket.h by 6.5.21,
   * for example, but not by 6.5.10.
   */
#elif defined(_MSC_VER) && _MSC_VER>1201
  /* Do not include addrinfo.h for MSVC7 or greater. 'addrinfo' and
   * EAI_* constants are defined in (the already included) ws2tcpip.h.
   */
#else
#  include "addrinfo.h"
#endif

#ifndef HAVE_INET_PTON
#if !defined(NTDDI_VERSION) || (NTDDI_VERSION < NTDDI_LONGHORN)
int inet_pton(int af, const char *src, void *dst);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
#endif
#endif

#ifdef __APPLE__
/* On OS X, getaddrinfo returns no error indication of lookup
   failure, so we must use the emulation instead of the libinfo
   implementation. Unfortunately, performing an autoconf test
   for this bug would require DNS access for the machine performing
   the configuration, which is not acceptable. Therefore, we
   determine the bug just by checking for __APPLE__. If this bug
   gets ever fixed, perhaps checking for sys/version.h would be
   appropriate, which is 10/0 on the system with the bug. */
#ifndef HAVE_GETNAMEINFO
/* This bug seems to be fixed in Jaguar. Ths easiest way I could
   Find to check for Jaguar is that it has getnameinfo(), which
   older releases don't have */
#undef HAVE_GETADDRINFO
#endif

#ifdef HAVE_INET_ATON
#define USE_INET_ATON_WEAKLINK
#endif

#endif

/* I know this is a bad practice, but it is the easiest... */
#if !defined(HAVE_GETADDRINFO)
/* avoid clashes with the C library definition of the symbol. */
#define getaddrinfo fake_getaddrinfo
#define gai_strerror fake_gai_strerror
#define freeaddrinfo fake_freeaddrinfo
#include "getaddrinfo.c"
#endif
#if !defined(HAVE_GETNAMEINFO)
#define getnameinfo fake_getnameinfo
#include "getnameinfo.c"
#endif

#ifdef MS_WINDOWS
/* On Windows a socket is really a handle not an fd */
static SOCKET
dup_socket(SOCKET handle)
{
    WSAPROTOCOL_INFO info;

    if (WSADuplicateSocket(handle, GetCurrentProcessId(), &info))
        return INVALID_SOCKET;

    return WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                     FROM_PROTOCOL_INFO, &info, 0, WSA_FLAG_OVERLAPPED);
}
#define SOCKETCLOSE closesocket
#else
/* On Unix we can use dup to duplicate the file descriptor of a socket*/
#define dup_socket(fd) dup(fd)
#endif

#ifdef MS_WIN32
#undef EAFNOSUPPORT
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#define snprintf _snprintf
#endif

#if defined(PYOS_OS2) && !defined(PYCC_GCC)
#define SOCKETCLOSE soclose
#define NO_DUP /* Sockets are Not Actual File Handles under OS/2 */
#endif

#ifndef SOCKETCLOSE
#define SOCKETCLOSE close
#endif

#if (defined(HAVE_BLUETOOTH_H) || defined(HAVE_BLUETOOTH_BLUETOOTH_H)) && !defined(__NetBSD__) && !defined(__DragonFly__)
#define USE_BLUETOOTH 1
#if defined(__FreeBSD__)
#define BTPROTO_L2CAP BLUETOOTH_PROTO_L2CAP
#define BTPROTO_RFCOMM BLUETOOTH_PROTO_RFCOMM
#define BTPROTO_HCI BLUETOOTH_PROTO_HCI
#define SOL_HCI SOL_HCI_RAW
#define HCI_FILTER SO_HCI_RAW_FILTER
#define sockaddr_l2 sockaddr_l2cap
#define sockaddr_rc sockaddr_rfcomm
#define hci_dev hci_node
#define _BT_L2_MEMB(sa, memb) ((sa)->l2cap_##memb)
#define _BT_RC_MEMB(sa, memb) ((sa)->rfcomm_##memb)
#define _BT_HCI_MEMB(sa, memb) ((sa)->hci_##memb)
#elif defined(__NetBSD__) || defined(__DragonFly__)
#define sockaddr_l2 sockaddr_bt
#define sockaddr_rc sockaddr_bt
#define sockaddr_hci sockaddr_bt
#define sockaddr_sco sockaddr_bt
#define SOL_HCI BTPROTO_HCI
#define HCI_DATA_DIR SO_HCI_DIRECTION
#define _BT_L2_MEMB(sa, memb) ((sa)->bt_##memb)
#define _BT_RC_MEMB(sa, memb) ((sa)->bt_##memb)
#define _BT_HCI_MEMB(sa, memb) ((sa)->bt_##memb)
#define _BT_SCO_MEMB(sa, memb) ((sa)->bt_##memb)
#else
#define _BT_L2_MEMB(sa, memb) ((sa)->l2_##memb)
#define _BT_RC_MEMB(sa, memb) ((sa)->rc_##memb)
#define _BT_HCI_MEMB(sa, memb) ((sa)->hci_##memb)
#define _BT_SCO_MEMB(sa, memb) ((sa)->sco_##memb)
#endif
#endif

#ifdef __VMS
/* TCP/IP Services for VMS uses a maximum send/recv buffer length */
#define SEGMENT_SIZE (32 * 1024 -1)
#endif

/* Convert "sock_addr_t *" to "struct sockaddr *". */
#define SAS2SA(x)       (&((x)->sa))

/*
 * Constants for getnameinfo()
 */
#if !defined(NI_MAXHOST)
#define NI_MAXHOST 1025
#endif
#if !defined(NI_MAXSERV)
#define NI_MAXSERV 32
#endif

#ifndef INVALID_SOCKET /* MS defines this */
#define INVALID_SOCKET (-1)
#endif

/* XXX There's a problem here: *static* functions are not supposed to have
   a Py prefix (or use CapitalizedWords).  Later... */

/* Global variable holding the exception type for errors detected
   by this module (but not argument type or memory errors, etc.). */
static PyObject *socket_herror;
static PyObject *socket_gaierror;
static PyObject *socket_timeout;

/* A forward reference to the socket type object.
   The sock_type variable contains pointers to various functions,
   some of which call new_sockobject(), which uses sock_type, so
   there has to be a circular reference. */
static PyTypeObject sock_type;

#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif

/* Largest value to try to store in a socklen_t (used when handling
   ancillary data).  POSIX requires socklen_t to hold at least
   (2**31)-1 and recommends against storing larger values, but
   socklen_t was originally int in the BSD interface, so to be on the
   safe side we use the smaller of (2**31)-1 and INT_MAX. */
#if INT_MAX > 0x7fffffff
#define SOCKLEN_T_LIMIT 0x7fffffff
#else
#define SOCKLEN_T_LIMIT INT_MAX
#endif

#ifdef HAVE_POLL
/* Instead of select(), we'll use poll() since poll() works on any fd. */
#define IS_SELECTABLE(s) 1
/* Can we call select() with this socket without a buffer overrun? */
#else
/* If there's no timeout left, we don't have to call select, so it's a safe,
 * little white lie. */
#define IS_SELECTABLE(s) (_PyIsSelectable_fd((s)->sock_fd) || (s)->sock_timeout <= 0.0)
#endif

static PyObject*
select_error(void)
{
    PyErr_SetString(PyExc_OSError, "unable to select on socket");
    return NULL;
}

#ifdef MS_WINDOWS
#ifndef WSAEAGAIN
#define WSAEAGAIN WSAEWOULDBLOCK
#endif
#define CHECK_ERRNO(expected) \
    (WSAGetLastError() == WSA ## expected)
#else
#define CHECK_ERRNO(expected) \
    (errno == expected)
#endif

/* Convenience function to raise an error according to errno
   and return a NULL pointer from a function. */

static PyObject *
set_error(void)
{
#ifdef MS_WINDOWS
    int err_no = WSAGetLastError();
    /* PyErr_SetExcFromWindowsErr() invokes FormatMessage() which
       recognizes the error codes used by both GetLastError() and
       WSAGetLastError */
    if (err_no)
        return PyErr_SetExcFromWindowsErr(PyExc_OSError, err_no);
#endif

#if defined(PYOS_OS2) && !defined(PYCC_GCC)
    if (sock_errno() != NO_ERROR) {
        APIRET rc;
        ULONG  msglen;
        char outbuf[100];
        int myerrorcode = sock_errno();

        /* Retrieve socket-related error message from MPTN.MSG file */
        rc = DosGetMessage(NULL, 0, outbuf, sizeof(outbuf),
                           myerrorcode - SOCBASEERR + 26,
                           "mptn.msg",
                           &msglen);
        if (rc == NO_ERROR) {
            PyObject *v;

            /* OS/2 doesn't guarantee a terminator */
            outbuf[msglen] = '\0';
            if (strlen(outbuf) > 0) {
                /* If non-empty msg, trim CRLF */
                char *lastc = &outbuf[ strlen(outbuf)-1 ];
                while (lastc > outbuf &&
                       Py_ISSPACE(Py_CHARMASK(*lastc))) {
                    /* Trim trailing whitespace (CRLF) */
                    *lastc-- = '\0';
                }
            }
            v = Py_BuildValue("(is)", myerrorcode, outbuf);
            if (v != NULL) {
                PyErr_SetObject(PyExc_OSError, v);
                Py_DECREF(v);
            }
            return NULL;
        }
    }
#endif

    return PyErr_SetFromErrno(PyExc_OSError);
}


static PyObject *
set_herror(int h_error)
{
    PyObject *v;

#ifdef HAVE_HSTRERROR
    v = Py_BuildValue("(is)", h_error, (char *)hstrerror(h_error));
#else
    v = Py_BuildValue("(is)", h_error, "host not found");
#endif
    if (v != NULL) {
        PyErr_SetObject(socket_herror, v);
        Py_DECREF(v);
    }

    return NULL;
}


static PyObject *
set_gaierror(int error)
{
    PyObject *v;

#ifdef EAI_SYSTEM
    /* EAI_SYSTEM is not available on Windows XP. */
    if (error == EAI_SYSTEM)
        return set_error();
#endif

#ifdef HAVE_GAI_STRERROR
    v = Py_BuildValue("(is)", error, gai_strerror(error));
#else
    v = Py_BuildValue("(is)", error, "getaddrinfo failed");
#endif
    if (v != NULL) {
        PyErr_SetObject(socket_gaierror, v);
        Py_DECREF(v);
    }

    return NULL;
}

#ifdef __VMS
/* Function to send in segments */
static int
sendsegmented(int sock_fd, char *buf, int len, int flags)
{
    int n = 0;
    int remaining = len;

    while (remaining > 0) {
        unsigned int segment;

        segment = (remaining >= SEGMENT_SIZE ? SEGMENT_SIZE : remaining);
        n = send(sock_fd, buf, segment, flags);
        if (n < 0) {
            return n;
        }
        remaining -= segment;
        buf += segment;
    } /* end while */

    return len;
}
#endif

/* Function to perform the setting of socket blocking mode
   internally. block = (1 | 0). */
static int
internal_setblocking(PySocketSockObject *s, int block)
{
#ifndef MS_WINDOWS
    int delay_flag;
#endif
#ifdef SOCK_NONBLOCK
    if (block)
        s->sock_type &= (~SOCK_NONBLOCK);
    else
        s->sock_type |= SOCK_NONBLOCK;
#endif

    Py_BEGIN_ALLOW_THREADS
#ifndef MS_WINDOWS
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
    block = !block;
    ioctl(s->sock_fd, FIONBIO, (caddr_t)&block, sizeof(block));
#elif defined(__VMS)
    block = !block;
    ioctl(s->sock_fd, FIONBIO, (unsigned int *)&block);
#else  /* !PYOS_OS2 && !__VMS */
    delay_flag = fcntl(s->sock_fd, F_GETFL, 0);
    if (block)
        delay_flag &= (~O_NONBLOCK);
    else
        delay_flag |= O_NONBLOCK;
    fcntl(s->sock_fd, F_SETFL, delay_flag);
#endif /* !PYOS_OS2 */
#else /* MS_WINDOWS */
    block = !block;
    ioctlsocket(s->sock_fd, FIONBIO, (u_long*)&block);
#endif /* MS_WINDOWS */
    Py_END_ALLOW_THREADS

    /* Since these don't return anything */
    return 1;
}

/* Do a select()/poll() on the socket, if necessary (sock_timeout > 0).
   The argument writing indicates the direction.
   This does not raise an exception; we'll let our caller do that
   after they've reacquired the interpreter lock.
   Returns 1 on timeout, -1 on error, 0 otherwise. */
static int
internal_select_ex(PySocketSockObject *s, int writing, double interval)
{
    int n;

    /* Nothing to do unless we're in timeout mode (not non-blocking) */
    if (s->sock_timeout <= 0.0)
        return 0;

    /* Guard against closed socket */
    if (s->sock_fd < 0)
        return 0;

    /* Handling this condition here simplifies the select loops */
    if (interval < 0.0)
        return 1;

    /* Prefer poll, if available, since you can poll() any fd
     * which can't be done with select(). */
#ifdef HAVE_POLL
    {
        struct pollfd pollfd;
        int timeout;

        pollfd.fd = s->sock_fd;
        pollfd.events = writing ? POLLOUT : POLLIN;

        /* s->sock_timeout is in seconds, timeout in ms */
        timeout = (int)(interval * 1000 + 0.5);
        n = poll(&pollfd, 1, timeout);
    }
#else
    {
        /* Construct the arguments to select */
        fd_set fds;
        struct timeval tv;
        tv.tv_sec = (int)interval;
        tv.tv_usec = (int)((interval - tv.tv_sec) * 1e6);
        FD_ZERO(&fds);
        FD_SET(s->sock_fd, &fds);

        /* See if the socket is ready */
        if (writing)
            n = select(Py_SAFE_DOWNCAST(s->sock_fd+1, SOCKET_T, int),
                       NULL, &fds, NULL, &tv);
        else
            n = select(Py_SAFE_DOWNCAST(s->sock_fd+1, SOCKET_T, int),
                       &fds, NULL, NULL, &tv);
    }
#endif

    if (n < 0)
        return -1;
    if (n == 0)
        return 1;
    return 0;
}

static int
internal_select(PySocketSockObject *s, int writing)
{
    return internal_select_ex(s, writing, s->sock_timeout);
}

/*
   Two macros for automatic retry of select() in case of false positives
   (for example, select() could indicate a socket is ready for reading
    but the data then discarded by the OS because of a wrong checksum).
   Here is an example of use:

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS
    timeout = internal_select_ex(s, 0, interval);
    if (!timeout)
        outlen = recv(s->sock_fd, cbuf, len, flags);
    Py_END_ALLOW_THREADS
    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        return -1;
    }
    END_SELECT_LOOP(s)
*/

#define BEGIN_SELECT_LOOP(s) \
    { \
        _PyTime_timeval now, deadline = {0, 0}; \
        double interval = s->sock_timeout; \
        int has_timeout = s->sock_timeout > 0.0; \
        if (has_timeout) { \
            _PyTime_gettimeofday(&now); \
            deadline = now; \
            _PyTime_ADD_SECONDS(deadline, s->sock_timeout); \
        } \
        while (1) { \
            errno = 0; \

#define END_SELECT_LOOP(s) \
            if (!has_timeout || \
                (!CHECK_ERRNO(EWOULDBLOCK) && !CHECK_ERRNO(EAGAIN))) \
                break; \
            _PyTime_gettimeofday(&now); \
            interval = _PyTime_INTERVAL(now, deadline); \
        } \
    } \

/* Initialize a new socket object. */

static double defaulttimeout = -1.0; /* Default timeout for new sockets */

static void
init_sockobject(PySocketSockObject *s,
                SOCKET_T fd, int family, int type, int proto)
{
    s->sock_fd = fd;
    s->sock_family = family;
    s->sock_type = type;
    s->sock_proto = proto;

    s->errorhandler = &set_error;
#ifdef SOCK_NONBLOCK
    if (type & SOCK_NONBLOCK)
        s->sock_timeout = 0.0;
    else
#endif
    {
        s->sock_timeout = defaulttimeout;
        if (defaulttimeout >= 0.0)
            internal_setblocking(s, 0);
    }

}


/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

static PySocketSockObject *
new_sockobject(SOCKET_T fd, int family, int type, int proto)
{
    PySocketSockObject *s;
    s = (PySocketSockObject *)
        PyType_GenericNew(&sock_type, NULL, NULL);
    if (s != NULL)
        init_sockobject(s, fd, family, type, proto);
    return s;
}


/* Lock to allow python interpreter to continue, but only allow one
   thread to be in gethostbyname or getaddrinfo */
#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
static PyThread_type_lock netdb_lock;
#endif


/* Convert a string specifying a host name or one of a few symbolic
   names to a numeric IP address.  This usually calls gethostbyname()
   to do the work; the names "" and "<broadcast>" are special.
   Return the length (IPv4 should be 4 bytes), or negative if
   an error occurred; then an exception is raised. */

static int
setipaddr(char *name, struct sockaddr *addr_ret, size_t addr_ret_size, int af)
{
    struct addrinfo hints, *res;
    int error;
    int d1, d2, d3, d4;
    char ch;

    memset((void *) addr_ret, '\0', sizeof(*addr_ret));
    if (name[0] == '\0') {
        int siz;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = af;
        hints.ai_socktype = SOCK_DGRAM;         /*dummy*/
        hints.ai_flags = AI_PASSIVE;
        Py_BEGIN_ALLOW_THREADS
        ACQUIRE_GETADDRINFO_LOCK
        error = getaddrinfo(NULL, "0", &hints, &res);
        Py_END_ALLOW_THREADS
        /* We assume that those thread-unsafe getaddrinfo() versions
           *are* safe regarding their return value, ie. that a
           subsequent call to getaddrinfo() does not destroy the
           outcome of the first call. */
        RELEASE_GETADDRINFO_LOCK
        if (error) {
            set_gaierror(error);
            return -1;
        }
        switch (res->ai_family) {
        case AF_INET:
            siz = 4;
            break;
#ifdef ENABLE_IPV6
        case AF_INET6:
            siz = 16;
            break;
#endif
        default:
            freeaddrinfo(res);
            PyErr_SetString(PyExc_OSError,
                "unsupported address family");
            return -1;
        }
        if (res->ai_next) {
            freeaddrinfo(res);
            PyErr_SetString(PyExc_OSError,
                "wildcard resolved to multiple address");
            return -1;
        }
        if (res->ai_addrlen < addr_ret_size)
            addr_ret_size = res->ai_addrlen;
        memcpy(addr_ret, res->ai_addr, addr_ret_size);
        freeaddrinfo(res);
        return siz;
    }
    if (name[0] == '<' && strcmp(name, "<broadcast>") == 0) {
        struct sockaddr_in *sin;
        if (af != AF_INET && af != AF_UNSPEC) {
            PyErr_SetString(PyExc_OSError,
                "address family mismatched");
            return -1;
        }
        sin = (struct sockaddr_in *)addr_ret;
        memset((void *) sin, '\0', sizeof(*sin));
        sin->sin_family = AF_INET;
#ifdef HAVE_SOCKADDR_SA_LEN
        sin->sin_len = sizeof(*sin);
#endif
        sin->sin_addr.s_addr = INADDR_BROADCAST;
        return sizeof(sin->sin_addr);
    }
    if (sscanf(name, "%d.%d.%d.%d%c", &d1, &d2, &d3, &d4, &ch) == 4 &&
        0 <= d1 && d1 <= 255 && 0 <= d2 && d2 <= 255 &&
        0 <= d3 && d3 <= 255 && 0 <= d4 && d4 <= 255) {
        struct sockaddr_in *sin;
        sin = (struct sockaddr_in *)addr_ret;
        sin->sin_addr.s_addr = htonl(
            ((long) d1 << 24) | ((long) d2 << 16) |
            ((long) d3 << 8) | ((long) d4 << 0));
        sin->sin_family = AF_INET;
#ifdef HAVE_SOCKADDR_SA_LEN
        sin->sin_len = sizeof(*sin);
#endif
        return 4;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    Py_BEGIN_ALLOW_THREADS
    ACQUIRE_GETADDRINFO_LOCK
    error = getaddrinfo(name, NULL, &hints, &res);
#if defined(__digital__) && defined(__unix__)
    if (error == EAI_NONAME && af == AF_UNSPEC) {
        /* On Tru64 V5.1, numeric-to-addr conversion fails
           if no address family is given. Assume IPv4 for now.*/
        hints.ai_family = AF_INET;
        error = getaddrinfo(name, NULL, &hints, &res);
    }
#endif
    Py_END_ALLOW_THREADS
    RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
    if (error) {
        set_gaierror(error);
        return -1;
    }
    if (res->ai_addrlen < addr_ret_size)
        addr_ret_size = res->ai_addrlen;
    memcpy((char *) addr_ret, res->ai_addr, addr_ret_size);
    freeaddrinfo(res);
    switch (addr_ret->sa_family) {
    case AF_INET:
        return 4;
#ifdef ENABLE_IPV6
    case AF_INET6:
        return 16;
#endif
    default:
        PyErr_SetString(PyExc_OSError, "unknown address family");
        return -1;
    }
}


/* Create a string object representing an IP address.
   This is always a string of the form 'dd.dd.dd.dd' (with variable
   size numbers). */

static PyObject *
makeipaddr(struct sockaddr *addr, int addrlen)
{
    char buf[NI_MAXHOST];
    int error;

    error = getnameinfo(addr, addrlen, buf, sizeof(buf), NULL, 0,
        NI_NUMERICHOST);
    if (error) {
        set_gaierror(error);
        return NULL;
    }
    return PyUnicode_FromString(buf);
}


#ifdef USE_BLUETOOTH
/* Convert a string representation of a Bluetooth address into a numeric
   address.  Returns the length (6), or raises an exception and returns -1 if
   an error occurred. */

static int
setbdaddr(char *name, bdaddr_t *bdaddr)
{
    unsigned int b0, b1, b2, b3, b4, b5;
    char ch;
    int n;

    n = sscanf(name, "%X:%X:%X:%X:%X:%X%c",
               &b5, &b4, &b3, &b2, &b1, &b0, &ch);
    if (n == 6 && (b0 | b1 | b2 | b3 | b4 | b5) < 256) {
        bdaddr->b[0] = b0;
        bdaddr->b[1] = b1;
        bdaddr->b[2] = b2;
        bdaddr->b[3] = b3;
        bdaddr->b[4] = b4;
        bdaddr->b[5] = b5;
        return 6;
    } else {
        PyErr_SetString(PyExc_OSError, "bad bluetooth address");
        return -1;
    }
}

/* Create a string representation of the Bluetooth address.  This is always a
   string of the form 'XX:XX:XX:XX:XX:XX' where XX is a two digit hexadecimal
   value (zero padded if necessary). */

static PyObject *
makebdaddr(bdaddr_t *bdaddr)
{
    char buf[(6 * 2) + 5 + 1];

    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
        bdaddr->b[5], bdaddr->b[4], bdaddr->b[3],
        bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);
    return PyUnicode_FromString(buf);
}
#endif


/* Create an object representing the given socket address,
   suitable for passing it back to bind(), connect() etc.
   The family field of the sockaddr structure is inspected
   to determine what kind of address it really is. */

/*ARGSUSED*/
static PyObject *
makesockaddr(SOCKET_T sockfd, struct sockaddr *addr, size_t addrlen, int proto)
{
    if (addrlen == 0) {
        /* No address -- may be recvfrom() from known socket */
        Py_INCREF(Py_None);
        return Py_None;
    }

    switch (addr->sa_family) {

    case AF_INET:
    {
        struct sockaddr_in *a;
        PyObject *addrobj = makeipaddr(addr, sizeof(*a));
        PyObject *ret = NULL;
        if (addrobj) {
            a = (struct sockaddr_in *)addr;
            ret = Py_BuildValue("Oi", addrobj, ntohs(a->sin_port));
            Py_DECREF(addrobj);
        }
        return ret;
    }

#if defined(AF_UNIX)
    case AF_UNIX:
    {
        struct sockaddr_un *a = (struct sockaddr_un *) addr;
#ifdef linux
        if (a->sun_path[0] == 0) {  /* Linux abstract namespace */
            addrlen -= offsetof(struct sockaddr_un, sun_path);
            return PyBytes_FromStringAndSize(a->sun_path, addrlen);
        }
        else
#endif /* linux */
        {
            /* regular NULL-terminated string */
            return PyUnicode_DecodeFSDefault(a->sun_path);
        }
    }
#endif /* AF_UNIX */

#if defined(AF_NETLINK)
       case AF_NETLINK:
       {
           struct sockaddr_nl *a = (struct sockaddr_nl *) addr;
           return Py_BuildValue("II", a->nl_pid, a->nl_groups);
       }
#endif /* AF_NETLINK */

#ifdef ENABLE_IPV6
    case AF_INET6:
    {
        struct sockaddr_in6 *a;
        PyObject *addrobj = makeipaddr(addr, sizeof(*a));
        PyObject *ret = NULL;
        if (addrobj) {
            a = (struct sockaddr_in6 *)addr;
            ret = Py_BuildValue("OiII",
                                addrobj,
                                ntohs(a->sin6_port),
                                ntohl(a->sin6_flowinfo),
                                a->sin6_scope_id);
            Py_DECREF(addrobj);
        }
        return ret;
    }
#endif

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
        switch (proto) {

        case BTPROTO_L2CAP:
        {
            struct sockaddr_l2 *a = (struct sockaddr_l2 *) addr;
            PyObject *addrobj = makebdaddr(&_BT_L2_MEMB(a, bdaddr));
            PyObject *ret = NULL;
            if (addrobj) {
                ret = Py_BuildValue("Oi",
                                    addrobj,
                                    _BT_L2_MEMB(a, psm));
                Py_DECREF(addrobj);
            }
            return ret;
        }

        case BTPROTO_RFCOMM:
        {
            struct sockaddr_rc *a = (struct sockaddr_rc *) addr;
            PyObject *addrobj = makebdaddr(&_BT_RC_MEMB(a, bdaddr));
            PyObject *ret = NULL;
            if (addrobj) {
                ret = Py_BuildValue("Oi",
                                    addrobj,
                                    _BT_RC_MEMB(a, channel));
                Py_DECREF(addrobj);
            }
            return ret;
        }

        case BTPROTO_HCI:
        {
            struct sockaddr_hci *a = (struct sockaddr_hci *) addr;
#if defined(__NetBSD__) || defined(__DragonFly__)
            return makebdaddr(&_BT_HCI_MEMB(a, bdaddr));
#else
            PyObject *ret = NULL;
            ret = Py_BuildValue("i", _BT_HCI_MEMB(a, dev));
            return ret;
#endif
        }

#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
        {
            struct sockaddr_sco *a = (struct sockaddr_sco *) addr;
            return makebdaddr(&_BT_SCO_MEMB(a, bdaddr));
        }
#endif

        default:
            PyErr_SetString(PyExc_ValueError,
                            "Unknown Bluetooth protocol");
            return NULL;
        }
#endif

#if defined(HAVE_NETPACKET_PACKET_H) && defined(SIOCGIFNAME)
    case AF_PACKET:
    {
        struct sockaddr_ll *a = (struct sockaddr_ll *)addr;
        char *ifname = "";
        struct ifreq ifr;
        /* need to look up interface name give index */
        if (a->sll_ifindex) {
            ifr.ifr_ifindex = a->sll_ifindex;
            if (ioctl(sockfd, SIOCGIFNAME, &ifr) == 0)
                ifname = ifr.ifr_name;
        }
        return Py_BuildValue("shbhy#",
                             ifname,
                             ntohs(a->sll_protocol),
                             a->sll_pkttype,
                             a->sll_hatype,
                             a->sll_addr,
                             a->sll_halen);
    }
#endif

#ifdef HAVE_LINUX_TIPC_H
    case AF_TIPC:
    {
        struct sockaddr_tipc *a = (struct sockaddr_tipc *) addr;
        if (a->addrtype == TIPC_ADDR_NAMESEQ) {
            return Py_BuildValue("IIIII",
                            a->addrtype,
                            a->addr.nameseq.type,
                            a->addr.nameseq.lower,
                            a->addr.nameseq.upper,
                            a->scope);
        } else if (a->addrtype == TIPC_ADDR_NAME) {
            return Py_BuildValue("IIIII",
                            a->addrtype,
                            a->addr.name.name.type,
                            a->addr.name.name.instance,
                            a->addr.name.name.instance,
                            a->scope);
        } else if (a->addrtype == TIPC_ADDR_ID) {
            return Py_BuildValue("IIIII",
                            a->addrtype,
                            a->addr.id.node,
                            a->addr.id.ref,
                            0,
                            a->scope);
        } else {
            PyErr_SetString(PyExc_ValueError,
                            "Invalid address type");
            return NULL;
        }
    }
#endif

#ifdef AF_CAN
    case AF_CAN:
    {
        struct sockaddr_can *a = (struct sockaddr_can *)addr;
        char *ifname = "";
        struct ifreq ifr;
        /* need to look up interface name given index */
        if (a->can_ifindex) {
            ifr.ifr_ifindex = a->can_ifindex;
            if (ioctl(sockfd, SIOCGIFNAME, &ifr) == 0)
                ifname = ifr.ifr_name;
        }

        return Py_BuildValue("O&h", PyUnicode_DecodeFSDefault,
                                    ifname,
                                    a->can_family);
    }
#endif

#ifdef PF_SYSTEM
    case PF_SYSTEM:
        switch(proto) {
#ifdef SYSPROTO_CONTROL
        case SYSPROTO_CONTROL:
        {
            struct sockaddr_ctl *a = (struct sockaddr_ctl *)addr;
            return Py_BuildValue("(II)", a->sc_id, a->sc_unit);
        }
#endif
        default:
            PyErr_SetString(PyExc_ValueError,
                            "Invalid address type");
            return 0;
        }
#endif

    /* More cases here... */

    default:
        /* If we don't know the address family, don't raise an
           exception -- return it as an (int, bytes) tuple. */
        return Py_BuildValue("iy#",
                             addr->sa_family,
                             addr->sa_data,
                             sizeof(addr->sa_data));

    }
}


/* Parse a socket address argument according to the socket object's
   address family.  Return 1 if the address was in the proper format,
   0 of not.  The address is returned through addr_ret, its length
   through len_ret. */

static int
getsockaddrarg(PySocketSockObject *s, PyObject *args,
               struct sockaddr *addr_ret, int *len_ret)
{
    switch (s->sock_family) {

#if defined(AF_UNIX)
    case AF_UNIX:
    {
        struct sockaddr_un* addr;
        char *path;
        int len;
        int retval = 0;

        /* PEP 383.  Not using PyUnicode_FSConverter since we need to
           allow embedded nulls on Linux. */
        if (PyUnicode_Check(args)) {
            if ((args = PyUnicode_EncodeFSDefault(args)) == NULL)
                return 0;
        }
        else
            Py_INCREF(args);
        if (!PyArg_Parse(args, "y#", &path, &len))
            goto unix_out;

        addr = (struct sockaddr_un*)addr_ret;
#ifdef linux
        if (len > 0 && path[0] == 0) {
            /* Linux abstract namespace extension */
            if (len > sizeof addr->sun_path) {
                PyErr_SetString(PyExc_OSError,
                                "AF_UNIX path too long");
                goto unix_out;
            }
        }
        else
#endif /* linux */
        {
            /* regular NULL-terminated string */
            if (len >= sizeof addr->sun_path) {
                PyErr_SetString(PyExc_OSError,
                                "AF_UNIX path too long");
                goto unix_out;
            }
            addr->sun_path[len] = 0;
        }
        addr->sun_family = s->sock_family;
        memcpy(addr->sun_path, path, len);
#if defined(PYOS_OS2)
        *len_ret = sizeof(*addr);
#else
        *len_ret = len + offsetof(struct sockaddr_un, sun_path);
#endif
        retval = 1;
    unix_out:
        Py_DECREF(args);
        return retval;
    }
#endif /* AF_UNIX */

#if defined(AF_NETLINK)
    case AF_NETLINK:
    {
        struct sockaddr_nl* addr;
        int pid, groups;
        addr = (struct sockaddr_nl *)addr_ret;
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_NETLINK address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args, "II:getsockaddrarg", &pid, &groups))
            return 0;
        addr->nl_family = AF_NETLINK;
        addr->nl_pid = pid;
        addr->nl_groups = groups;
        *len_ret = sizeof(*addr);
        return 1;
    }
#endif

#ifdef AF_RDS
    case AF_RDS:
        /* RDS sockets use sockaddr_in: fall-through */
#endif

    case AF_INET:
    {
        struct sockaddr_in* addr;
        char *host;
        int port, result;
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_INET address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args, "eti:getsockaddrarg",
                              "idna", &host, &port))
            return 0;
        addr=(struct sockaddr_in*)addr_ret;
        result = setipaddr(host, (struct sockaddr *)addr,
                           sizeof(*addr),  AF_INET);
        PyMem_Free(host);
        if (result < 0)
            return 0;
        if (port < 0 || port > 0xffff) {
            PyErr_SetString(
                PyExc_OverflowError,
                "getsockaddrarg: port must be 0-65535.");
            return 0;
        }
        addr->sin_family = AF_INET;
        addr->sin_port = htons((short)port);
        *len_ret = sizeof *addr;
        return 1;
    }

#ifdef ENABLE_IPV6
    case AF_INET6:
    {
        struct sockaddr_in6* addr;
        char *host;
        int port, result;
        unsigned int flowinfo, scope_id;
        flowinfo = scope_id = 0;
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_INET6 address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args, "eti|II",
                              "idna", &host, &port, &flowinfo,
                              &scope_id)) {
            return 0;
        }
        addr = (struct sockaddr_in6*)addr_ret;
        result = setipaddr(host, (struct sockaddr *)addr,
                           sizeof(*addr), AF_INET6);
        PyMem_Free(host);
        if (result < 0)
            return 0;
        if (port < 0 || port > 0xffff) {
            PyErr_SetString(
                PyExc_OverflowError,
                "getsockaddrarg: port must be 0-65535.");
            return 0;
        }
        if (flowinfo > 0xfffff) {
            PyErr_SetString(
                PyExc_OverflowError,
                "getsockaddrarg: flowinfo must be 0-1048575.");
            return 0;
        }
        addr->sin6_family = s->sock_family;
        addr->sin6_port = htons((short)port);
        addr->sin6_flowinfo = htonl(flowinfo);
        addr->sin6_scope_id = scope_id;
        *len_ret = sizeof *addr;
        return 1;
    }
#endif

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
    {
        switch (s->sock_proto) {
        case BTPROTO_L2CAP:
        {
            struct sockaddr_l2 *addr;
            char *straddr;

            addr = (struct sockaddr_l2 *)addr_ret;
            memset(addr, 0, sizeof(struct sockaddr_l2));
            _BT_L2_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "si", &straddr,
                                  &_BT_L2_MEMB(addr, psm))) {
                PyErr_SetString(PyExc_OSError, "getsockaddrarg: "
                                "wrong format");
                return 0;
            }
            if (setbdaddr(straddr, &_BT_L2_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
        case BTPROTO_RFCOMM:
        {
            struct sockaddr_rc *addr;
            char *straddr;

            addr = (struct sockaddr_rc *)addr_ret;
            _BT_RC_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "si", &straddr,
                                  &_BT_RC_MEMB(addr, channel))) {
                PyErr_SetString(PyExc_OSError, "getsockaddrarg: "
                                "wrong format");
                return 0;
            }
            if (setbdaddr(straddr, &_BT_RC_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
        case BTPROTO_HCI:
        {
            struct sockaddr_hci *addr = (struct sockaddr_hci *)addr_ret;
#if defined(__NetBSD__) || defined(__DragonFly__)
                        char *straddr = PyBytes_AS_STRING(args);

                        _BT_HCI_MEMB(addr, family) = AF_BLUETOOTH;
            if (straddr == NULL) {
                PyErr_SetString(PyExc_OSError, "getsockaddrarg: "
                    "wrong format");
                return 0;
            }
            if (setbdaddr(straddr, &_BT_HCI_MEMB(addr, bdaddr)) < 0)
                return 0;
#else
            _BT_HCI_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "i", &_BT_HCI_MEMB(addr, dev))) {
                PyErr_SetString(PyExc_OSError, "getsockaddrarg: "
                                "wrong format");
                return 0;
            }
#endif
            *len_ret = sizeof *addr;
            return 1;
        }
#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
        {
            struct sockaddr_sco *addr;
            char *straddr;

            addr = (struct sockaddr_sco *)addr_ret;
            _BT_SCO_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyBytes_Check(args)) {
                PyErr_SetString(PyExc_OSError, "getsockaddrarg: "
                                "wrong format");
                return 0;
            }
            straddr = PyBytes_AS_STRING(args);
            if (setbdaddr(straddr, &_BT_SCO_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
#endif
        default:
            PyErr_SetString(PyExc_OSError, "getsockaddrarg: unknown Bluetooth protocol");
            return 0;
        }
    }
#endif

#if defined(HAVE_NETPACKET_PACKET_H) && defined(SIOCGIFINDEX)
    case AF_PACKET:
    {
        struct sockaddr_ll* addr;
        struct ifreq ifr;
        char *interfaceName;
        int protoNumber;
        int hatype = 0;
        int pkttype = 0;
        char *haddr = NULL;
        unsigned int halen = 0;

        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_PACKET address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args, "si|iiy#", &interfaceName,
                              &protoNumber, &pkttype, &hatype,
                              &haddr, &halen))
            return 0;
        strncpy(ifr.ifr_name, interfaceName, sizeof(ifr.ifr_name));
        ifr.ifr_name[(sizeof(ifr.ifr_name))-1] = '\0';
        if (ioctl(s->sock_fd, SIOCGIFINDEX, &ifr) < 0) {
            s->errorhandler();
            return 0;
        }
        if (halen > 8) {
          PyErr_SetString(PyExc_ValueError,
                          "Hardware address must be 8 bytes or less");
          return 0;
        }
        if (protoNumber < 0 || protoNumber > 0xffff) {
            PyErr_SetString(
                PyExc_OverflowError,
                "getsockaddrarg: protoNumber must be 0-65535.");
            return 0;
        }
        addr = (struct sockaddr_ll*)addr_ret;
        addr->sll_family = AF_PACKET;
        addr->sll_protocol = htons((short)protoNumber);
        addr->sll_ifindex = ifr.ifr_ifindex;
        addr->sll_pkttype = pkttype;
        addr->sll_hatype = hatype;
        if (halen != 0) {
          memcpy(&addr->sll_addr, haddr, halen);
        }
        addr->sll_halen = halen;
        *len_ret = sizeof *addr;
        return 1;
    }
#endif

#ifdef HAVE_LINUX_TIPC_H
    case AF_TIPC:
    {
        unsigned int atype, v1, v2, v3;
        unsigned int scope = TIPC_CLUSTER_SCOPE;
        struct sockaddr_tipc *addr;

        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_TIPC address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }

        if (!PyArg_ParseTuple(args,
                                "IIII|I;Invalid TIPC address format",
                                &atype, &v1, &v2, &v3, &scope))
            return 0;

        addr = (struct sockaddr_tipc *) addr_ret;
        memset(addr, 0, sizeof(struct sockaddr_tipc));

        addr->family = AF_TIPC;
        addr->scope = scope;
        addr->addrtype = atype;

        if (atype == TIPC_ADDR_NAMESEQ) {
            addr->addr.nameseq.type = v1;
            addr->addr.nameseq.lower = v2;
            addr->addr.nameseq.upper = v3;
        } else if (atype == TIPC_ADDR_NAME) {
            addr->addr.name.name.type = v1;
            addr->addr.name.name.instance = v2;
        } else if (atype == TIPC_ADDR_ID) {
            addr->addr.id.node = v1;
            addr->addr.id.ref = v2;
        } else {
            /* Shouldn't happen */
            PyErr_SetString(PyExc_TypeError, "Invalid address type");
            return 0;
        }

        *len_ret = sizeof(*addr);

        return 1;
    }
#endif

#ifdef AF_CAN
    case AF_CAN:
        switch (s->sock_proto) {
        case CAN_RAW:
        {
            struct sockaddr_can *addr;
            PyObject *interfaceName;
            struct ifreq ifr;
            addr = (struct sockaddr_can *)addr_ret;
            Py_ssize_t len;

            if (!PyArg_ParseTuple(args, "O&", PyUnicode_FSConverter,
                                              &interfaceName))
                return 0;

            len = PyBytes_GET_SIZE(interfaceName);

            if (len == 0) {
                ifr.ifr_ifindex = 0;
            } else if (len < sizeof(ifr.ifr_name)) {
                strncpy(ifr.ifr_name, PyBytes_AS_STRING(interfaceName), sizeof(ifr.ifr_name));
                ifr.ifr_name[(sizeof(ifr.ifr_name))-1] = '\0';
                if (ioctl(s->sock_fd, SIOCGIFINDEX, &ifr) < 0) {
                    s->errorhandler();
                    Py_DECREF(interfaceName);
                    return 0;
                }
            } else {
                PyErr_SetString(PyExc_OSError,
                                "AF_CAN interface name too long");
                Py_DECREF(interfaceName);
                return 0;
            }

            addr->can_family = AF_CAN;
            addr->can_ifindex = ifr.ifr_ifindex;

            *len_ret = sizeof(*addr);
            Py_DECREF(interfaceName);
            return 1;
        }
        default:
            PyErr_SetString(PyExc_OSError,
                            "getsockaddrarg: unsupported CAN protocol");
            return 0;
        }
#endif
            
#ifdef PF_SYSTEM
    case PF_SYSTEM:
        switch (s->sock_proto) {
#ifdef SYSPROTO_CONTROL
        case SYSPROTO_CONTROL:
        {
            struct sockaddr_ctl *addr;
            
            addr = (struct sockaddr_ctl *)addr_ret;
            addr->sc_family = AF_SYSTEM;
            addr->ss_sysaddr = AF_SYS_CONTROL; 

            if (PyUnicode_Check(args)) {
                struct ctl_info info;
                PyObject *ctl_name;

                if (!PyArg_Parse(args, "O&",
                                PyUnicode_FSConverter, &ctl_name)) {
                    return 0;
                }

                if (PyBytes_GET_SIZE(ctl_name) > sizeof(info.ctl_name)) {
                    PyErr_SetString(PyExc_ValueError,
                                    "provided string is too long");
                    Py_DECREF(ctl_name);
                    return 0;
                }
                strncpy(info.ctl_name, PyBytes_AS_STRING(ctl_name),
                        sizeof(info.ctl_name));
                Py_DECREF(ctl_name);

                if (ioctl(s->sock_fd, CTLIOCGINFO, &info)) {
                    PyErr_SetString(PyExc_OSError,
                          "cannot find kernel control with provided name");
                    return 0;
                }
                
                addr->sc_id = info.ctl_id;
                addr->sc_unit = 0;
            } else if (!PyArg_ParseTuple(args, "II",
                                         &(addr->sc_id), &(addr->sc_unit))) {
                PyErr_SetString(PyExc_TypeError, "getsockaddrarg: "
                                "expected str or tuple of two ints");
                
                return 0;
            }
              
            *len_ret = sizeof(*addr);
            return 1;
        }
#endif
        default:
            PyErr_SetString(PyExc_OSError,
                            "getsockaddrarg: unsupported PF_SYSTEM protocol");
            return 0;
        }
#endif

    /* More cases here... */

    default:
        PyErr_SetString(PyExc_OSError, "getsockaddrarg: bad family");
        return 0;

    }
}


/* Get the address length according to the socket object's address family.
   Return 1 if the family is known, 0 otherwise.  The length is returned
   through len_ret. */

static int
getsockaddrlen(PySocketSockObject *s, socklen_t *len_ret)
{
    switch (s->sock_family) {

#if defined(AF_UNIX)
    case AF_UNIX:
    {
        *len_ret = sizeof (struct sockaddr_un);
        return 1;
    }
#endif /* AF_UNIX */
#if defined(AF_NETLINK)
       case AF_NETLINK:
       {
           *len_ret = sizeof (struct sockaddr_nl);
           return 1;
       }
#endif

#ifdef AF_RDS
    case AF_RDS:
        /* RDS sockets use sockaddr_in: fall-through */
#endif

    case AF_INET:
    {
        *len_ret = sizeof (struct sockaddr_in);
        return 1;
    }

#ifdef ENABLE_IPV6
    case AF_INET6:
    {
        *len_ret = sizeof (struct sockaddr_in6);
        return 1;
    }
#endif

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
    {
        switch(s->sock_proto)
        {

        case BTPROTO_L2CAP:
            *len_ret = sizeof (struct sockaddr_l2);
            return 1;
        case BTPROTO_RFCOMM:
            *len_ret = sizeof (struct sockaddr_rc);
            return 1;
        case BTPROTO_HCI:
            *len_ret = sizeof (struct sockaddr_hci);
            return 1;
#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
            *len_ret = sizeof (struct sockaddr_sco);
            return 1;
#endif
        default:
            PyErr_SetString(PyExc_OSError, "getsockaddrlen: "
                            "unknown BT protocol");
            return 0;

        }
    }
#endif

#ifdef HAVE_NETPACKET_PACKET_H
    case AF_PACKET:
    {
        *len_ret = sizeof (struct sockaddr_ll);
        return 1;
    }
#endif

#ifdef HAVE_LINUX_TIPC_H
    case AF_TIPC:
    {
        *len_ret = sizeof (struct sockaddr_tipc);
        return 1;
    }
#endif

#ifdef AF_CAN
    case AF_CAN:
    {
        *len_ret = sizeof (struct sockaddr_can);
        return 1;
    }
#endif
            
#ifdef PF_SYSTEM
    case PF_SYSTEM:
        switch(s->sock_proto) {
#ifdef SYSPROTO_CONTROL
        case SYSPROTO_CONTROL:
            *len_ret = sizeof (struct sockaddr_ctl);
            return 1;
#endif
        default:
            PyErr_SetString(PyExc_OSError, "getsockaddrlen: "
                            "unknown PF_SYSTEM protocol");
            return 0;
        }
#endif

    /* More cases here... */

    default:
        PyErr_SetString(PyExc_OSError, "getsockaddrlen: bad family");
        return 0;

    }
}


/* Support functions for the sendmsg() and recvmsg[_into]() methods.
   Currently, these methods are only compiled if the RFC 2292/3542
   CMSG_LEN() macro is available.  Older systems seem to have used
   sizeof(struct cmsghdr) + (length) where CMSG_LEN() is used now, so
   it may be possible to define CMSG_LEN() that way if it's not
   provided.  Some architectures might need extra padding after the
   cmsghdr, however, and CMSG_LEN() would have to take account of
   this. */
#ifdef CMSG_LEN
/* If length is in range, set *result to CMSG_LEN(length) and return
   true; otherwise, return false. */
static int
get_CMSG_LEN(size_t length, size_t *result)
{
    size_t tmp;

    if (length > (SOCKLEN_T_LIMIT - CMSG_LEN(0)))
        return 0;
    tmp = CMSG_LEN(length);
    if (tmp > SOCKLEN_T_LIMIT || tmp < length)
        return 0;
    *result = tmp;
    return 1;
}

#ifdef CMSG_SPACE
/* If length is in range, set *result to CMSG_SPACE(length) and return
   true; otherwise, return false. */
static int
get_CMSG_SPACE(size_t length, size_t *result)
{
    size_t tmp;

    /* Use CMSG_SPACE(1) here in order to take account of the padding
       necessary before *and* after the data. */
    if (length > (SOCKLEN_T_LIMIT - CMSG_SPACE(1)))
        return 0;
    tmp = CMSG_SPACE(length);
    if (tmp > SOCKLEN_T_LIMIT || tmp < length)
        return 0;
    *result = tmp;
    return 1;
}
#endif

/* Return true iff msg->msg_controllen is valid, cmsgh is a valid
   pointer in msg->msg_control with at least "space" bytes after it,
   and its cmsg_len member inside the buffer. */
static int
cmsg_min_space(struct msghdr *msg, struct cmsghdr *cmsgh, size_t space)
{
    size_t cmsg_offset;
    static const size_t cmsg_len_end = (offsetof(struct cmsghdr, cmsg_len) +
                                        sizeof(cmsgh->cmsg_len));

    /* Note that POSIX allows msg_controllen to be of signed type. */
    if (cmsgh == NULL || msg->msg_control == NULL || msg->msg_controllen < 0)
        return 0;
    if (space < cmsg_len_end)
        space = cmsg_len_end;
    cmsg_offset = (char *)cmsgh - (char *)msg->msg_control;
    return (cmsg_offset <= (size_t)-1 - space &&
            cmsg_offset + space <= msg->msg_controllen);
}

/* If pointer CMSG_DATA(cmsgh) is in buffer msg->msg_control, set
   *space to number of bytes following it in the buffer and return
   true; otherwise, return false.  Assumes cmsgh, msg->msg_control and
   msg->msg_controllen are valid. */
static int
get_cmsg_data_space(struct msghdr *msg, struct cmsghdr *cmsgh, size_t *space)
{
    size_t data_offset;
    char *data_ptr;

    if ((data_ptr = (char *)CMSG_DATA(cmsgh)) == NULL)
        return 0;
    data_offset = data_ptr - (char *)msg->msg_control;
    if (data_offset > msg->msg_controllen)
        return 0;
    *space = msg->msg_controllen - data_offset;
    return 1;
}

/* If cmsgh is invalid or not contained in the buffer pointed to by
   msg->msg_control, return -1.  If cmsgh is valid and its associated
   data is entirely contained in the buffer, set *data_len to the
   length of the associated data and return 0.  If only part of the
   associated data is contained in the buffer but cmsgh is otherwise
   valid, set *data_len to the length contained in the buffer and
   return 1. */
static int
get_cmsg_data_len(struct msghdr *msg, struct cmsghdr *cmsgh, size_t *data_len)
{
    size_t space, cmsg_data_len;

    if (!cmsg_min_space(msg, cmsgh, CMSG_LEN(0)) ||
        cmsgh->cmsg_len < CMSG_LEN(0))
        return -1;
    cmsg_data_len = cmsgh->cmsg_len - CMSG_LEN(0);
    if (!get_cmsg_data_space(msg, cmsgh, &space))
        return -1;
    if (space >= cmsg_data_len) {
        *data_len = cmsg_data_len;
        return 0;
    }
    *data_len = space;
    return 1;
}
#endif    /* CMSG_LEN */


/* s._accept() -> (fd, address) */

static PyObject *
sock_accept(PySocketSockObject *s)
{
    sock_addr_t addrbuf;
    SOCKET_T newfd = INVALID_SOCKET;
    socklen_t addrlen;
    PyObject *sock = NULL;
    PyObject *addr = NULL;
    PyObject *res = NULL;
    int timeout;
    if (!getsockaddrlen(s, &addrlen))
        return NULL;
    memset(&addrbuf, 0, addrlen);

    if (!IS_SELECTABLE(s))
        return select_error();

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS
    timeout = internal_select_ex(s, 0, interval);
    if (!timeout) {
        newfd = accept(s->sock_fd, SAS2SA(&addrbuf), &addrlen);
    }
    Py_END_ALLOW_THREADS

    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        return NULL;
    }
    END_SELECT_LOOP(s)

    if (newfd == INVALID_SOCKET)
        return s->errorhandler();

    sock = PyLong_FromSocket_t(newfd);
    if (sock == NULL) {
        SOCKETCLOSE(newfd);
        goto finally;
    }

    addr = makesockaddr(s->sock_fd, SAS2SA(&addrbuf),
                        addrlen, s->sock_proto);
    if (addr == NULL)
        goto finally;

    res = PyTuple_Pack(2, sock, addr);

finally:
    Py_XDECREF(sock);
    Py_XDECREF(addr);
    return res;
}

PyDoc_STRVAR(accept_doc,
"_accept() -> (integer, address info)\n\
\n\
Wait for an incoming connection.  Return a new socket file descriptor\n\
representing the connection, and the address of the client.\n\
For IP sockets, the address info is a pair (hostaddr, port).");

/* s.setblocking(flag) method.  Argument:
   False -- non-blocking mode; same as settimeout(0)
   True -- blocking mode; same as settimeout(None)
*/

static PyObject *
sock_setblocking(PySocketSockObject *s, PyObject *arg)
{
    long block;

    block = PyLong_AsLong(arg);
    if (block == -1 && PyErr_Occurred())
        return NULL;

    s->sock_timeout = block ? -1.0 : 0.0;
    internal_setblocking(s, block);

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(setblocking_doc,
"setblocking(flag)\n\
\n\
Set the socket to blocking (flag is true) or non-blocking (false).\n\
setblocking(True) is equivalent to settimeout(None);\n\
setblocking(False) is equivalent to settimeout(0.0).");

/* s.settimeout(timeout) method.  Argument:
   None -- no timeout, blocking mode; same as setblocking(True)
   0.0  -- non-blocking mode; same as setblocking(False)
   > 0  -- timeout mode; operations time out after timeout seconds
   < 0  -- illegal; raises an exception
*/
static PyObject *
sock_settimeout(PySocketSockObject *s, PyObject *arg)
{
    double timeout;

    if (arg == Py_None)
        timeout = -1.0;
    else {
        timeout = PyFloat_AsDouble(arg);
        if (timeout < 0.0) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError,
                                "Timeout value out of range");
            return NULL;
        }
    }

    s->sock_timeout = timeout;
    internal_setblocking(s, timeout < 0.0);

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(settimeout_doc,
"settimeout(timeout)\n\
\n\
Set a timeout on socket operations.  'timeout' can be a float,\n\
giving in seconds, or None.  Setting a timeout of None disables\n\
the timeout feature and is equivalent to setblocking(1).\n\
Setting a timeout of zero is the same as setblocking(0).");

/* s.gettimeout() method.
   Returns the timeout associated with a socket. */
static PyObject *
sock_gettimeout(PySocketSockObject *s)
{
    if (s->sock_timeout < 0.0) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else
        return PyFloat_FromDouble(s->sock_timeout);
}

PyDoc_STRVAR(gettimeout_doc,
"gettimeout() -> timeout\n\
\n\
Returns the timeout in seconds (float) associated with socket \n\
operations. A timeout of None indicates that timeouts on socket \n\
operations are disabled.");

/* s.setsockopt() method.
   With an integer third argument, sets an integer option.
   With a string third argument, sets an option from a buffer;
   use optional built-in module 'struct' to encode the string. */

static PyObject *
sock_setsockopt(PySocketSockObject *s, PyObject *args)
{
    int level;
    int optname;
    int res;
    char *buf;
    int buflen;
    int flag;

    if (PyArg_ParseTuple(args, "iii:setsockopt",
                         &level, &optname, &flag)) {
        buf = (char *) &flag;
        buflen = sizeof flag;
    }
    else {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "iiy#:setsockopt",
                              &level, &optname, &buf, &buflen))
            return NULL;
    }
    res = setsockopt(s->sock_fd, level, optname, (void *)buf, buflen);
    if (res < 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(setsockopt_doc,
"setsockopt(level, option, value)\n\
\n\
Set a socket option.  See the Unix manual for level and option.\n\
The value argument can either be an integer or a string.");


/* s.getsockopt() method.
   With two arguments, retrieves an integer option.
   With a third integer argument, retrieves a string buffer of that size;
   use optional built-in module 'struct' to decode the string. */

static PyObject *
sock_getsockopt(PySocketSockObject *s, PyObject *args)
{
    int level;
    int optname;
    int res;
    PyObject *buf;
    socklen_t buflen = 0;

    if (!PyArg_ParseTuple(args, "ii|i:getsockopt",
                          &level, &optname, &buflen))
        return NULL;

    if (buflen == 0) {
        int flag = 0;
        socklen_t flagsize = sizeof flag;
        res = getsockopt(s->sock_fd, level, optname,
                         (void *)&flag, &flagsize);
        if (res < 0)
            return s->errorhandler();
        return PyLong_FromLong(flag);
    }
#ifdef __VMS
    /* socklen_t is unsigned so no negative test is needed,
       test buflen == 0 is previously done */
    if (buflen > 1024) {
#else
    if (buflen <= 0 || buflen > 1024) {
#endif
        PyErr_SetString(PyExc_OSError,
                        "getsockopt buflen out of range");
        return NULL;
    }
    buf = PyBytes_FromStringAndSize((char *)NULL, buflen);
    if (buf == NULL)
        return NULL;
    res = getsockopt(s->sock_fd, level, optname,
                     (void *)PyBytes_AS_STRING(buf), &buflen);
    if (res < 0) {
        Py_DECREF(buf);
        return s->errorhandler();
    }
    _PyBytes_Resize(&buf, buflen);
    return buf;
}

PyDoc_STRVAR(getsockopt_doc,
"getsockopt(level, option[, buffersize]) -> value\n\
\n\
Get a socket option.  See the Unix manual for level and option.\n\
If a nonzero buffersize argument is given, the return value is a\n\
string of that length; otherwise it is an integer.");


/* s.bind(sockaddr) method */

static PyObject *
sock_bind(PySocketSockObject *s, PyObject *addro)
{
    sock_addr_t addrbuf;
    int addrlen;
    int res;

    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = bind(s->sock_fd, SAS2SA(&addrbuf), addrlen);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(bind_doc,
"bind(address)\n\
\n\
Bind the socket to a local address.  For IP sockets, the address is a\n\
pair (host, port); the host must refer to the local host. For raw packet\n\
sockets the address is a tuple (ifname, proto [,pkttype [,hatype]])");


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static PyObject *
sock_close(PySocketSockObject *s)
{
    SOCKET_T fd;

    if ((fd = s->sock_fd) != -1) {
        s->sock_fd = -1;
        Py_BEGIN_ALLOW_THREADS
        (void) SOCKETCLOSE(fd);
        Py_END_ALLOW_THREADS
    }
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(close_doc,
"close()\n\
\n\
Close the socket.  It cannot be used after this call.");

static PyObject *
sock_detach(PySocketSockObject *s)
{
    SOCKET_T fd = s->sock_fd;
    s->sock_fd = -1;
    return PyLong_FromSocket_t(fd);
}

PyDoc_STRVAR(detach_doc,
"detach()\n\
\n\
Close the socket object without closing the underlying file descriptor.\
The object cannot be used after this call, but the file descriptor\
can be reused for other purposes.  The file descriptor is returned.");

static int
internal_connect(PySocketSockObject *s, struct sockaddr *addr, int addrlen,
                 int *timeoutp)
{
    int res, timeout;

    timeout = 0;
    res = connect(s->sock_fd, addr, addrlen);

#ifdef MS_WINDOWS

    if (s->sock_timeout > 0.0) {
        if (res < 0 && WSAGetLastError() == WSAEWOULDBLOCK &&
            IS_SELECTABLE(s)) {
            /* This is a mess.  Best solution: trust select */
            fd_set fds;
            fd_set fds_exc;
            struct timeval tv;
            tv.tv_sec = (int)s->sock_timeout;
            tv.tv_usec = (int)((s->sock_timeout - tv.tv_sec) * 1e6);
            FD_ZERO(&fds);
            FD_SET(s->sock_fd, &fds);
            FD_ZERO(&fds_exc);
            FD_SET(s->sock_fd, &fds_exc);
            res = select(Py_SAFE_DOWNCAST(s->sock_fd+1, SOCKET_T, int),
                         NULL, &fds, &fds_exc, &tv);
            if (res == 0) {
                res = WSAEWOULDBLOCK;
                timeout = 1;
            } else if (res > 0) {
                if (FD_ISSET(s->sock_fd, &fds))
                    /* The socket is in the writable set - this
                       means connected */
                    res = 0;
                else {
                    /* As per MS docs, we need to call getsockopt()
                       to get the underlying error */
                    int res_size = sizeof res;
                    /* It must be in the exception set */
                    assert(FD_ISSET(s->sock_fd, &fds_exc));
                    if (0 == getsockopt(s->sock_fd, SOL_SOCKET, SO_ERROR,
                                        (char *)&res, &res_size))
                        /* getsockopt also clears WSAGetLastError,
                           so reset it back. */
                        WSASetLastError(res);
                    else
                        res = WSAGetLastError();
                }
            }
            /* else if (res < 0) an error occurred */
        }
    }

    if (res < 0)
        res = WSAGetLastError();

#else

    if (s->sock_timeout > 0.0) {
        if (res < 0 && errno == EINPROGRESS && IS_SELECTABLE(s)) {
            timeout = internal_select(s, 1);
            if (timeout == 0) {
                /* Bug #1019808: in case of an EINPROGRESS,
                   use getsockopt(SO_ERROR) to get the real
                   error. */
                socklen_t res_size = sizeof res;
                (void)getsockopt(s->sock_fd, SOL_SOCKET,
                                 SO_ERROR, &res, &res_size);
                if (res == EISCONN)
                    res = 0;
                errno = res;
            }
            else if (timeout == -1) {
                res = errno;            /* had error */
            }
            else
                res = EWOULDBLOCK;                      /* timed out */
        }
    }

    if (res < 0)
        res = errno;

#endif
    *timeoutp = timeout;

    return res;
}

/* s.connect(sockaddr) method */

static PyObject *
sock_connect(PySocketSockObject *s, PyObject *addro)
{
    sock_addr_t addrbuf;
    int addrlen;
    int res;
    int timeout;

    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = internal_connect(s, SAS2SA(&addrbuf), addrlen, &timeout);
    Py_END_ALLOW_THREADS

    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        return NULL;
    }
    if (res != 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(connect_doc,
"connect(address)\n\
\n\
Connect the socket to a remote address.  For IP sockets, the address\n\
is a pair (host, port).");


/* s.connect_ex(sockaddr) method */

static PyObject *
sock_connect_ex(PySocketSockObject *s, PyObject *addro)
{
    sock_addr_t addrbuf;
    int addrlen;
    int res;
    int timeout;

    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = internal_connect(s, SAS2SA(&addrbuf), addrlen, &timeout);
    Py_END_ALLOW_THREADS

    /* Signals are not errors (though they may raise exceptions).  Adapted
       from PyErr_SetFromErrnoWithFilenameObject(). */
#ifdef EINTR
    if (res == EINTR && PyErr_CheckSignals())
        return NULL;
#endif

    return PyLong_FromLong((long) res);
}

PyDoc_STRVAR(connect_ex_doc,
"connect_ex(address) -> errno\n\
\n\
This is like connect(address), but returns an error code (the errno value)\n\
instead of raising an exception when an error occurs.");


/* s.fileno() method */

static PyObject *
sock_fileno(PySocketSockObject *s)
{
    return PyLong_FromSocket_t(s->sock_fd);
}

PyDoc_STRVAR(fileno_doc,
"fileno() -> integer\n\
\n\
Return the integer file descriptor of the socket.");


/* s.getsockname() method */

static PyObject *
sock_getsockname(PySocketSockObject *s)
{
    sock_addr_t addrbuf;
    int res;
    socklen_t addrlen;

    if (!getsockaddrlen(s, &addrlen))
        return NULL;
    memset(&addrbuf, 0, addrlen);
    Py_BEGIN_ALLOW_THREADS
    res = getsockname(s->sock_fd, SAS2SA(&addrbuf), &addrlen);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    return makesockaddr(s->sock_fd, SAS2SA(&addrbuf), addrlen,
                        s->sock_proto);
}

PyDoc_STRVAR(getsockname_doc,
"getsockname() -> address info\n\
\n\
Return the address of the local endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");


#ifdef HAVE_GETPEERNAME         /* Cray APP doesn't have this :-( */
/* s.getpeername() method */

static PyObject *
sock_getpeername(PySocketSockObject *s)
{
    sock_addr_t addrbuf;
    int res;
    socklen_t addrlen;

    if (!getsockaddrlen(s, &addrlen))
        return NULL;
    memset(&addrbuf, 0, addrlen);
    Py_BEGIN_ALLOW_THREADS
    res = getpeername(s->sock_fd, SAS2SA(&addrbuf), &addrlen);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    return makesockaddr(s->sock_fd, SAS2SA(&addrbuf), addrlen,
                        s->sock_proto);
}

PyDoc_STRVAR(getpeername_doc,
"getpeername() -> address info\n\
\n\
Return the address of the remote endpoint.  For IP sockets, the address\n\
info is a pair (hostaddr, port).");

#endif /* HAVE_GETPEERNAME */


/* s.listen(n) method */

static PyObject *
sock_listen(PySocketSockObject *s, PyObject *arg)
{
    int backlog;
    int res;

    backlog = _PyLong_AsInt(arg);
    if (backlog == -1 && PyErr_Occurred())
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    /* To avoid problems on systems that don't allow a negative backlog
     * (which doesn't make sense anyway) we force a minimum value of 0. */
    if (backlog < 0)
        backlog = 0;
    res = listen(s->sock_fd, backlog);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(listen_doc,
"listen(backlog)\n\
\n\
Enable a server to accept connections.  The backlog argument must be at\n\
least 0 (if it is lower, it is set to 0); it specifies the number of\n\
unaccepted connections that the system will allow before refusing new\n\
connections.");


/*
 * This is the guts of the recv() and recv_into() methods, which reads into a
 * char buffer.  If you have any inc/dec ref to do to the objects that contain
 * the buffer, do it in the caller.  This function returns the number of bytes
 * successfully read.  If there was an error, it returns -1.  Note that it is
 * also possible that we return a number of bytes smaller than the request
 * bytes.
 */

static Py_ssize_t
sock_recv_guts(PySocketSockObject *s, char* cbuf, Py_ssize_t len, int flags)
{
    Py_ssize_t outlen = -1;
    int timeout;
#ifdef __VMS
    int remaining;
    char *read_buf;
#endif

    if (!IS_SELECTABLE(s)) {
        select_error();
        return -1;
    }
    if (len == 0) {
        /* If 0 bytes were requested, do nothing. */
        return 0;
    }

#ifndef __VMS
    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS
    timeout = internal_select_ex(s, 0, interval);
    if (!timeout)
        outlen = recv(s->sock_fd, cbuf, len, flags);
    Py_END_ALLOW_THREADS

    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        return -1;
    }
    END_SELECT_LOOP(s)
    if (outlen < 0) {
        /* Note: the call to errorhandler() ALWAYS indirectly returned
           NULL, so ignore its return value */
        s->errorhandler();
        return -1;
    }
#else
    read_buf = cbuf;
    remaining = len;
    while (remaining != 0) {
        unsigned int segment;
        int nread = -1;

        segment = remaining /SEGMENT_SIZE;
        if (segment != 0) {
            segment = SEGMENT_SIZE;
        }
        else {
            segment = remaining;
        }

        BEGIN_SELECT_LOOP(s)
        Py_BEGIN_ALLOW_THREADS
        timeout = internal_select_ex(s, 0, interval);
        if (!timeout)
            nread = recv(s->sock_fd, read_buf, segment, flags);
        Py_END_ALLOW_THREADS
        if (timeout == 1) {
            PyErr_SetString(socket_timeout, "timed out");
            return -1;
        }
        END_SELECT_LOOP(s)

        if (nread < 0) {
            s->errorhandler();
            return -1;
        }
        if (nread != remaining) {
            read_buf += nread;
            break;
        }

        remaining -= segment;
        read_buf += segment;
    }
    outlen = read_buf - cbuf;
#endif /* !__VMS */

    return outlen;
}


/* s.recv(nbytes [,flags]) method */

static PyObject *
sock_recv(PySocketSockObject *s, PyObject *args)
{
    Py_ssize_t recvlen, outlen;
    int flags = 0;
    PyObject *buf;

    if (!PyArg_ParseTuple(args, "n|i:recv", &recvlen, &flags))
        return NULL;

    if (recvlen < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recv");
        return NULL;
    }

    /* Allocate a new string. */
    buf = PyBytes_FromStringAndSize((char *) 0, recvlen);
    if (buf == NULL)
        return NULL;

    /* Call the guts */
    outlen = sock_recv_guts(s, PyBytes_AS_STRING(buf), recvlen, flags);
    if (outlen < 0) {
        /* An error occurred, release the string and return an
           error. */
        Py_DECREF(buf);
        return NULL;
    }
    if (outlen != recvlen) {
        /* We did not read as many bytes as we anticipated, resize the
           string if possible and be successful. */
        _PyBytes_Resize(&buf, outlen);
    }

    return buf;
}

PyDoc_STRVAR(recv_doc,
"recv(buffersize[, flags]) -> data\n\
\n\
Receive up to buffersize bytes from the socket.  For the optional flags\n\
argument, see the Unix manual.  When no data is available, block until\n\
at least one byte is available or until the remote end is closed.  When\n\
the remote end is closed and all data is read, return the empty string.");


/* s.recv_into(buffer, [nbytes [,flags]]) method */

static PyObject*
sock_recv_into(PySocketSockObject *s, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"buffer", "nbytes", "flags", 0};

    int flags = 0;
    Py_buffer pbuf;
    char *buf;
    Py_ssize_t buflen, readlen, recvlen = 0;

    /* Get the buffer's memory */
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "w*|ni:recv_into", kwlist,
                                     &pbuf, &recvlen, &flags))
        return NULL;
    buf = pbuf.buf;
    buflen = pbuf.len;

    if (recvlen < 0) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recv_into");
        return NULL;
    }
    if (recvlen == 0) {
        /* If nbytes was not specified, use the buffer's length */
        recvlen = buflen;
    }

    /* Check if the buffer is large enough */
    if (buflen < recvlen) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_ValueError,
                        "buffer too small for requested bytes");
        return NULL;
    }

    /* Call the guts */
    readlen = sock_recv_guts(s, buf, recvlen, flags);
    if (readlen < 0) {
        /* Return an error. */
        PyBuffer_Release(&pbuf);
        return NULL;
    }

    PyBuffer_Release(&pbuf);
    /* Return the number of bytes read.  Note that we do not do anything
       special here in the case that readlen < recvlen. */
    return PyLong_FromSsize_t(readlen);
}

PyDoc_STRVAR(recv_into_doc,
"recv_into(buffer, [nbytes[, flags]]) -> nbytes_read\n\
\n\
A version of recv() that stores its data into a buffer rather than creating \n\
a new string.  Receive up to buffersize bytes from the socket.  If buffersize \n\
is not specified (or 0), receive up to the size available in the given buffer.\n\
\n\
See recv() for documentation about the flags.");


/*
 * This is the guts of the recvfrom() and recvfrom_into() methods, which reads
 * into a char buffer.  If you have any inc/def ref to do to the objects that
 * contain the buffer, do it in the caller.  This function returns the number
 * of bytes successfully read.  If there was an error, it returns -1.  Note
 * that it is also possible that we return a number of bytes smaller than the
 * request bytes.
 *
 * 'addr' is a return value for the address object.  Note that you must decref
 * it yourself.
 */
static Py_ssize_t
sock_recvfrom_guts(PySocketSockObject *s, char* cbuf, Py_ssize_t len, int flags,
                   PyObject** addr)
{
    sock_addr_t addrbuf;
    int timeout;
    Py_ssize_t n = -1;
    socklen_t addrlen;

    *addr = NULL;

    if (!getsockaddrlen(s, &addrlen))
        return -1;

    if (!IS_SELECTABLE(s)) {
        select_error();
        return -1;
    }

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS
    memset(&addrbuf, 0, addrlen);
    timeout = internal_select_ex(s, 0, interval);
    if (!timeout) {
#ifndef MS_WINDOWS
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
        n = recvfrom(s->sock_fd, cbuf, len, flags,
                     SAS2SA(&addrbuf), &addrlen);
#else
        n = recvfrom(s->sock_fd, cbuf, len, flags,
                     (void *) &addrbuf, &addrlen);
#endif
#else
        n = recvfrom(s->sock_fd, cbuf, len, flags,
                     SAS2SA(&addrbuf), &addrlen);
#endif
    }
    Py_END_ALLOW_THREADS

    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        return -1;
    }
    END_SELECT_LOOP(s)
    if (n < 0) {
        s->errorhandler();
        return -1;
    }

    if (!(*addr = makesockaddr(s->sock_fd, SAS2SA(&addrbuf),
                               addrlen, s->sock_proto)))
        return -1;

    return n;
}

/* s.recvfrom(nbytes [,flags]) method */

static PyObject *
sock_recvfrom(PySocketSockObject *s, PyObject *args)
{
    PyObject *buf = NULL;
    PyObject *addr = NULL;
    PyObject *ret = NULL;
    int flags = 0;
    Py_ssize_t recvlen, outlen;

    if (!PyArg_ParseTuple(args, "n|i:recvfrom", &recvlen, &flags))
        return NULL;

    if (recvlen < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recvfrom");
        return NULL;
    }

    buf = PyBytes_FromStringAndSize((char *) 0, recvlen);
    if (buf == NULL)
        return NULL;

    outlen = sock_recvfrom_guts(s, PyBytes_AS_STRING(buf),
                                recvlen, flags, &addr);
    if (outlen < 0) {
        goto finally;
    }

    if (outlen != recvlen) {
        /* We did not read as many bytes as we anticipated, resize the
           string if possible and be successful. */
        if (_PyBytes_Resize(&buf, outlen) < 0)
            /* Oopsy, not so successful after all. */
            goto finally;
    }

    ret = PyTuple_Pack(2, buf, addr);

finally:
    Py_XDECREF(buf);
    Py_XDECREF(addr);
    return ret;
}

PyDoc_STRVAR(recvfrom_doc,
"recvfrom(buffersize[, flags]) -> (data, address info)\n\
\n\
Like recv(buffersize, flags) but also return the sender's address info.");


/* s.recvfrom_into(buffer[, nbytes [,flags]]) method */

static PyObject *
sock_recvfrom_into(PySocketSockObject *s, PyObject *args, PyObject* kwds)
{
    static char *kwlist[] = {"buffer", "nbytes", "flags", 0};

    int flags = 0;
    Py_buffer pbuf;
    char *buf;
    Py_ssize_t readlen, buflen, recvlen = 0;

    PyObject *addr = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "w*|ni:recvfrom_into",
                                     kwlist, &pbuf,
                                     &recvlen, &flags))
        return NULL;
    buf = pbuf.buf;
    buflen = pbuf.len;
    assert(buf != 0 && buflen > 0);

    if (recvlen < 0) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recvfrom_into");
        return NULL;
    }
    if (recvlen == 0) {
        /* If nbytes was not specified, use the buffer's length */
        recvlen = buflen;
    }

    readlen = sock_recvfrom_guts(s, buf, recvlen, flags, &addr);
    if (readlen < 0) {
        PyBuffer_Release(&pbuf);
        /* Return an error */
        Py_XDECREF(addr);
        return NULL;
    }

    PyBuffer_Release(&pbuf);
    /* Return the number of bytes read and the address.  Note that we do
       not do anything special here in the case that readlen < recvlen. */
    return Py_BuildValue("nN", readlen, addr);
}

PyDoc_STRVAR(recvfrom_into_doc,
"recvfrom_into(buffer[, nbytes[, flags]]) -> (nbytes, address info)\n\
\n\
Like recv_into(buffer[, nbytes[, flags]]) but also return the sender's address info.");


/* The sendmsg() and recvmsg[_into]() methods require a working
   CMSG_LEN().  See the comment near get_CMSG_LEN(). */
#ifdef CMSG_LEN
/*
 * Call recvmsg() with the supplied iovec structures, flags, and
 * ancillary data buffer size (controllen).  Returns the tuple return
 * value for recvmsg() or recvmsg_into(), with the first item provided
 * by the supplied makeval() function.  makeval() will be called with
 * the length read and makeval_data as arguments, and must return a
 * new reference (which will be decrefed if there is a subsequent
 * error).  On error, closes any file descriptors received via
 * SCM_RIGHTS.
 */
static PyObject *
sock_recvmsg_guts(PySocketSockObject *s, struct iovec *iov, int iovlen,
                  int flags, Py_ssize_t controllen,
                  PyObject *(*makeval)(ssize_t, void *), void *makeval_data)
{
    ssize_t bytes_received = -1;
    int timeout;
    sock_addr_t addrbuf;
    socklen_t addrbuflen;
    struct msghdr msg = {0};
    PyObject *cmsg_list = NULL, *retval = NULL;
    void *controlbuf = NULL;
    struct cmsghdr *cmsgh;
    size_t cmsgdatalen = 0;
    int cmsg_status;

    /* XXX: POSIX says that msg_name and msg_namelen "shall be
       ignored" when the socket is connected (Linux fills them in
       anyway for AF_UNIX sockets at least).  Normally msg_namelen
       seems to be set to 0 if there's no address, but try to
       initialize msg_name to something that won't be mistaken for a
       real address if that doesn't happen. */
    if (!getsockaddrlen(s, &addrbuflen))
        return NULL;
    memset(&addrbuf, 0, addrbuflen);
    SAS2SA(&addrbuf)->sa_family = AF_UNSPEC;

    if (controllen < 0 || controllen > SOCKLEN_T_LIMIT) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid ancillary data buffer length");
        return NULL;
    }
    if (controllen > 0 && (controlbuf = PyMem_Malloc(controllen)) == NULL)
        return PyErr_NoMemory();

    /* Make the system call. */
    if (!IS_SELECTABLE(s)) {
        select_error();
        goto finally;
    }

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS;
    msg.msg_name = SAS2SA(&addrbuf);
    msg.msg_namelen = addrbuflen;
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;
    msg.msg_control = controlbuf;
    msg.msg_controllen = controllen;
    timeout = internal_select_ex(s, 0, interval);
    if (!timeout)
        bytes_received = recvmsg(s->sock_fd, &msg, flags);
    Py_END_ALLOW_THREADS;
    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        goto finally;
    }
    END_SELECT_LOOP(s)

    if (bytes_received < 0) {
        s->errorhandler();
        goto finally;
    }

    /* Make list of (level, type, data) tuples from control messages. */
    if ((cmsg_list = PyList_New(0)) == NULL)
        goto err_closefds;
    /* Check for empty ancillary data as old CMSG_FIRSTHDR()
       implementations didn't do so. */
    for (cmsgh = ((msg.msg_controllen > 0) ? CMSG_FIRSTHDR(&msg) : NULL);
         cmsgh != NULL; cmsgh = CMSG_NXTHDR(&msg, cmsgh)) {
        PyObject *bytes, *tuple;
        int tmp;

        cmsg_status = get_cmsg_data_len(&msg, cmsgh, &cmsgdatalen);
        if (cmsg_status != 0) {
            if (PyErr_WarnEx(PyExc_RuntimeWarning,
                             "received malformed or improperly-truncated "
                             "ancillary data", 1) == -1)
                goto err_closefds;
        }
        if (cmsg_status < 0)
            break;
        if (cmsgdatalen > PY_SSIZE_T_MAX) {
            PyErr_SetString(PyExc_OSError, "control message too long");
            goto err_closefds;
        }

        bytes = PyBytes_FromStringAndSize((char *)CMSG_DATA(cmsgh),
                                          cmsgdatalen);
        tuple = Py_BuildValue("iiN", (int)cmsgh->cmsg_level,
                              (int)cmsgh->cmsg_type, bytes);
        if (tuple == NULL)
            goto err_closefds;
        tmp = PyList_Append(cmsg_list, tuple);
        Py_DECREF(tuple);
        if (tmp != 0)
            goto err_closefds;

        if (cmsg_status != 0)
            break;
    }

    retval = Py_BuildValue("NOiN",
                           (*makeval)(bytes_received, makeval_data),
                           cmsg_list,
                           (int)msg.msg_flags,
                           makesockaddr(s->sock_fd, SAS2SA(&addrbuf),
                                        ((msg.msg_namelen > addrbuflen) ?
                                         addrbuflen : msg.msg_namelen),
                                        s->sock_proto));
    if (retval == NULL)
        goto err_closefds;

finally:
    Py_XDECREF(cmsg_list);
    PyMem_Free(controlbuf);
    return retval;

err_closefds:
#ifdef SCM_RIGHTS
    /* Close all descriptors coming from SCM_RIGHTS, so they don't leak. */
    for (cmsgh = ((msg.msg_controllen > 0) ? CMSG_FIRSTHDR(&msg) : NULL);
         cmsgh != NULL; cmsgh = CMSG_NXTHDR(&msg, cmsgh)) {
        cmsg_status = get_cmsg_data_len(&msg, cmsgh, &cmsgdatalen);
        if (cmsg_status < 0)
            break;
        if (cmsgh->cmsg_level == SOL_SOCKET &&
            cmsgh->cmsg_type == SCM_RIGHTS) {
            size_t numfds;
            int *fdp;

            numfds = cmsgdatalen / sizeof(int);
            fdp = (int *)CMSG_DATA(cmsgh);
            while (numfds-- > 0)
                close(*fdp++);
        }
        if (cmsg_status != 0)
            break;
    }
#endif /* SCM_RIGHTS */
    goto finally;
}


static PyObject *
makeval_recvmsg(ssize_t received, void *data)
{
    PyObject **buf = data;

    if (received < PyBytes_GET_SIZE(*buf))
        _PyBytes_Resize(buf, received);
    Py_XINCREF(*buf);
    return *buf;
}

/* s.recvmsg(bufsize[, ancbufsize[, flags]]) method */

static PyObject *
sock_recvmsg(PySocketSockObject *s, PyObject *args)
{
    Py_ssize_t bufsize, ancbufsize = 0;
    int flags = 0;
    struct iovec iov;
    PyObject *buf = NULL, *retval = NULL;

    if (!PyArg_ParseTuple(args, "n|ni:recvmsg", &bufsize, &ancbufsize, &flags))
        return NULL;

    if (bufsize < 0) {
        PyErr_SetString(PyExc_ValueError, "negative buffer size in recvmsg()");
        return NULL;
    }
    if ((buf = PyBytes_FromStringAndSize(NULL, bufsize)) == NULL)
        return NULL;
    iov.iov_base = PyBytes_AS_STRING(buf);
    iov.iov_len = bufsize;

    /* Note that we're passing a pointer to *our pointer* to the bytes
       object here (&buf); makeval_recvmsg() may incref the object, or
       deallocate it and set our pointer to NULL. */
    retval = sock_recvmsg_guts(s, &iov, 1, flags, ancbufsize,
                               &makeval_recvmsg, &buf);
    Py_XDECREF(buf);
    return retval;
}

PyDoc_STRVAR(recvmsg_doc,
"recvmsg(bufsize[, ancbufsize[, flags]]) -> (data, ancdata, msg_flags, address)\n\
\n\
Receive normal data (up to bufsize bytes) and ancillary data from the\n\
socket.  The ancbufsize argument sets the size in bytes of the\n\
internal buffer used to receive the ancillary data; it defaults to 0,\n\
meaning that no ancillary data will be received.  Appropriate buffer\n\
sizes for ancillary data can be calculated using CMSG_SPACE() or\n\
CMSG_LEN(), and items which do not fit into the buffer might be\n\
truncated or discarded.  The flags argument defaults to 0 and has the\n\
same meaning as for recv().\n\
\n\
The return value is a 4-tuple: (data, ancdata, msg_flags, address).\n\
The data item is a bytes object holding the non-ancillary data\n\
received.  The ancdata item is a list of zero or more tuples\n\
(cmsg_level, cmsg_type, cmsg_data) representing the ancillary data\n\
(control messages) received: cmsg_level and cmsg_type are integers\n\
specifying the protocol level and protocol-specific type respectively,\n\
and cmsg_data is a bytes object holding the associated data.  The\n\
msg_flags item is the bitwise OR of various flags indicating\n\
conditions on the received message; see your system documentation for\n\
details.  If the receiving socket is unconnected, address is the\n\
address of the sending socket, if available; otherwise, its value is\n\
unspecified.\n\
\n\
If recvmsg() raises an exception after the system call returns, it\n\
will first attempt to close any file descriptors received via the\n\
SCM_RIGHTS mechanism.");


static PyObject *
makeval_recvmsg_into(ssize_t received, void *data)
{
    return PyLong_FromSsize_t(received);
}

/* s.recvmsg_into(buffers[, ancbufsize[, flags]]) method */

static PyObject *
sock_recvmsg_into(PySocketSockObject *s, PyObject *args)
{
    Py_ssize_t ancbufsize = 0;
    int flags = 0;
    struct iovec *iovs = NULL;
    Py_ssize_t i, nitems, nbufs = 0;
    Py_buffer *bufs = NULL;
    PyObject *buffers_arg, *fast, *retval = NULL;

    if (!PyArg_ParseTuple(args, "O|ni:recvmsg_into",
                          &buffers_arg, &ancbufsize, &flags))
        return NULL;

    if ((fast = PySequence_Fast(buffers_arg,
                                "recvmsg_into() argument 1 must be an "
                                "iterable")) == NULL)
        return NULL;
    nitems = PySequence_Fast_GET_SIZE(fast);
    if (nitems > INT_MAX) {
        PyErr_SetString(PyExc_OSError, "recvmsg_into() argument 1 is too long");
        goto finally;
    }

    /* Fill in an iovec for each item, and save the Py_buffer
       structs to release afterwards. */
    if (nitems > 0 && ((iovs = PyMem_New(struct iovec, nitems)) == NULL ||
                       (bufs = PyMem_New(Py_buffer, nitems)) == NULL)) {
        PyErr_NoMemory();
        goto finally;
    }
    for (; nbufs < nitems; nbufs++) {
        if (!PyArg_Parse(PySequence_Fast_GET_ITEM(fast, nbufs),
                         "w*;recvmsg_into() argument 1 must be an iterable "
                         "of single-segment read-write buffers",
                         &bufs[nbufs]))
            goto finally;
        iovs[nbufs].iov_base = bufs[nbufs].buf;
        iovs[nbufs].iov_len = bufs[nbufs].len;
    }

    retval = sock_recvmsg_guts(s, iovs, nitems, flags, ancbufsize,
                               &makeval_recvmsg_into, NULL);
finally:
    for (i = 0; i < nbufs; i++)
        PyBuffer_Release(&bufs[i]);
    PyMem_Free(bufs);
    PyMem_Free(iovs);
    Py_DECREF(fast);
    return retval;
}

PyDoc_STRVAR(recvmsg_into_doc,
"recvmsg_into(buffers[, ancbufsize[, flags]]) -> (nbytes, ancdata, msg_flags, address)\n\
\n\
Receive normal data and ancillary data from the socket, scattering the\n\
non-ancillary data into a series of buffers.  The buffers argument\n\
must be an iterable of objects that export writable buffers\n\
(e.g. bytearray objects); these will be filled with successive chunks\n\
of the non-ancillary data until it has all been written or there are\n\
no more buffers.  The ancbufsize argument sets the size in bytes of\n\
the internal buffer used to receive the ancillary data; it defaults to\n\
0, meaning that no ancillary data will be received.  Appropriate\n\
buffer sizes for ancillary data can be calculated using CMSG_SPACE()\n\
or CMSG_LEN(), and items which do not fit into the buffer might be\n\
truncated or discarded.  The flags argument defaults to 0 and has the\n\
same meaning as for recv().\n\
\n\
The return value is a 4-tuple: (nbytes, ancdata, msg_flags, address).\n\
The nbytes item is the total number of bytes of non-ancillary data\n\
written into the buffers.  The ancdata item is a list of zero or more\n\
tuples (cmsg_level, cmsg_type, cmsg_data) representing the ancillary\n\
data (control messages) received: cmsg_level and cmsg_type are\n\
integers specifying the protocol level and protocol-specific type\n\
respectively, and cmsg_data is a bytes object holding the associated\n\
data.  The msg_flags item is the bitwise OR of various flags\n\
indicating conditions on the received message; see your system\n\
documentation for details.  If the receiving socket is unconnected,\n\
address is the address of the sending socket, if available; otherwise,\n\
its value is unspecified.\n\
\n\
If recvmsg_into() raises an exception after the system call returns,\n\
it will first attempt to close any file descriptors received via the\n\
SCM_RIGHTS mechanism.");
#endif    /* CMSG_LEN */


/* s.send(data [,flags]) method */

static PyObject *
sock_send(PySocketSockObject *s, PyObject *args)
{
    char *buf;
    Py_ssize_t len, n = -1;
    int flags = 0, timeout;
    Py_buffer pbuf;

    if (!PyArg_ParseTuple(args, "y*|i:send", &pbuf, &flags))
        return NULL;

    if (!IS_SELECTABLE(s)) {
        PyBuffer_Release(&pbuf);
        return select_error();
    }
    buf = pbuf.buf;
    len = pbuf.len;

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS
    timeout = internal_select_ex(s, 1, interval);
    if (!timeout)
#ifdef __VMS
        n = sendsegmented(s->sock_fd, buf, len, flags);
#else
        n = send(s->sock_fd, buf, len, flags);
#endif
    Py_END_ALLOW_THREADS
    if (timeout == 1) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(socket_timeout, "timed out");
        return NULL;
    }
    END_SELECT_LOOP(s)

    PyBuffer_Release(&pbuf);
    if (n < 0)
        return s->errorhandler();
    return PyLong_FromSsize_t(n);
}

PyDoc_STRVAR(send_doc,
"send(data[, flags]) -> count\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  Return the number of bytes\n\
sent; this may be less than len(data) if the network is busy.");


/* s.sendall(data [,flags]) method */

static PyObject *
sock_sendall(PySocketSockObject *s, PyObject *args)
{
    char *buf;
    Py_ssize_t len, n = -1;
    int flags = 0, timeout, saved_errno;
    Py_buffer pbuf;

    if (!PyArg_ParseTuple(args, "y*|i:sendall", &pbuf, &flags))
        return NULL;
    buf = pbuf.buf;
    len = pbuf.len;

    if (!IS_SELECTABLE(s)) {
        PyBuffer_Release(&pbuf);
        return select_error();
    }

    do {
        Py_BEGIN_ALLOW_THREADS
        timeout = internal_select(s, 1);
        n = -1;
        if (!timeout) {
#ifdef __VMS
            n = sendsegmented(s->sock_fd, buf, len, flags);
#else
            n = send(s->sock_fd, buf, len, flags);
#endif
        }
        Py_END_ALLOW_THREADS
        if (timeout == 1) {
            PyBuffer_Release(&pbuf);
            PyErr_SetString(socket_timeout, "timed out");
            return NULL;
        }
        /* PyErr_CheckSignals() might change errno */
        saved_errno = errno;
        /* We must run our signal handlers before looping again.
           send() can return a successful partial write when it is
           interrupted, so we can't restrict ourselves to EINTR. */
        if (PyErr_CheckSignals()) {
            PyBuffer_Release(&pbuf);
            return NULL;
        }
        if (n < 0) {
            /* If interrupted, try again */
            if (saved_errno == EINTR)
                continue;
            else
                break;
        }
        buf += n;
        len -= n;
    } while (len > 0);
    PyBuffer_Release(&pbuf);

    if (n < 0)
        return s->errorhandler();

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(sendall_doc,
"sendall(data[, flags])\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  This calls send() repeatedly\n\
until all data is sent.  If an error occurs, it's impossible\n\
to tell how much data has been sent.");


/* s.sendto(data, [flags,] sockaddr) method */

static PyObject *
sock_sendto(PySocketSockObject *s, PyObject *args)
{
    Py_buffer pbuf;
    PyObject *addro;
    char *buf;
    Py_ssize_t len, arglen;
    sock_addr_t addrbuf;
    int addrlen, n = -1, flags, timeout;

    flags = 0;
    arglen = PyTuple_Size(args);
    switch (arglen) {
        case 2:
            PyArg_ParseTuple(args, "y*O:sendto", &pbuf, &addro);
            break;
        case 3:
            PyArg_ParseTuple(args, "y*iO:sendto",
                             &pbuf, &flags, &addro);
            break;
        default:
            PyErr_Format(PyExc_TypeError,
                         "sendto() takes 2 or 3 arguments (%d given)",
                         arglen);
            return NULL;
    }
    if (PyErr_Occurred())
        return NULL;

    buf = pbuf.buf;
    len = pbuf.len;

    if (!IS_SELECTABLE(s)) {
        PyBuffer_Release(&pbuf);
        return select_error();
    }

    if (!getsockaddrarg(s, addro, SAS2SA(&addrbuf), &addrlen)) {
        PyBuffer_Release(&pbuf);
        return NULL;
    }

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS
    timeout = internal_select_ex(s, 1, interval);
    if (!timeout)
        n = sendto(s->sock_fd, buf, len, flags, SAS2SA(&addrbuf), addrlen);
    Py_END_ALLOW_THREADS

    if (timeout == 1) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(socket_timeout, "timed out");
        return NULL;
    }
    END_SELECT_LOOP(s)
    PyBuffer_Release(&pbuf);
    if (n < 0)
        return s->errorhandler();
    return PyLong_FromSsize_t(n);
}

PyDoc_STRVAR(sendto_doc,
"sendto(data[, flags], address) -> count\n\
\n\
Like send(data, flags) but allows specifying the destination address.\n\
For IP sockets, the address is a pair (hostaddr, port).");


/* The sendmsg() and recvmsg[_into]() methods require a working
   CMSG_LEN().  See the comment near get_CMSG_LEN(). */
#ifdef CMSG_LEN
/* s.sendmsg(buffers[, ancdata[, flags[, address]]]) method */

static PyObject *
sock_sendmsg(PySocketSockObject *s, PyObject *args)
{
    Py_ssize_t i, ndataparts, ndatabufs = 0, ncmsgs, ncmsgbufs = 0;
    Py_buffer *databufs = NULL;
    struct iovec *iovs = NULL;
    sock_addr_t addrbuf;
    struct msghdr msg = {0};
    struct cmsginfo {
        int level;
        int type;
        Py_buffer data;
    } *cmsgs = NULL;
    void *controlbuf = NULL;
    size_t controllen, controllen_last;
    ssize_t bytes_sent = -1;
    int addrlen, timeout, flags = 0;
    PyObject *data_arg, *cmsg_arg = NULL, *addr_arg = NULL, *data_fast = NULL,
        *cmsg_fast = NULL, *retval = NULL;

    if (!PyArg_ParseTuple(args, "O|OiO:sendmsg",
                          &data_arg, &cmsg_arg, &flags, &addr_arg))
        return NULL;

    /* Parse destination address. */
    if (addr_arg != NULL && addr_arg != Py_None) {
        if (!getsockaddrarg(s, addr_arg, SAS2SA(&addrbuf), &addrlen))
            goto finally;
        msg.msg_name = &addrbuf;
        msg.msg_namelen = addrlen;
    }

    /* Fill in an iovec for each message part, and save the Py_buffer
       structs to release afterwards. */
    if ((data_fast = PySequence_Fast(data_arg,
                                     "sendmsg() argument 1 must be an "
                                     "iterable")) == NULL)
        goto finally;
    ndataparts = PySequence_Fast_GET_SIZE(data_fast);
    if (ndataparts > INT_MAX) {
        PyErr_SetString(PyExc_OSError, "sendmsg() argument 1 is too long");
        goto finally;
    }
    msg.msg_iovlen = ndataparts;
    if (ndataparts > 0 &&
        ((msg.msg_iov = iovs = PyMem_New(struct iovec, ndataparts)) == NULL ||
         (databufs = PyMem_New(Py_buffer, ndataparts)) == NULL)) {
        PyErr_NoMemory();
        goto finally;
    }
    for (; ndatabufs < ndataparts; ndatabufs++) {
        if (!PyArg_Parse(PySequence_Fast_GET_ITEM(data_fast, ndatabufs),
                         "y*;sendmsg() argument 1 must be an iterable of "
                         "buffer-compatible objects",
                         &databufs[ndatabufs]))
            goto finally;
        iovs[ndatabufs].iov_base = databufs[ndatabufs].buf;
        iovs[ndatabufs].iov_len = databufs[ndatabufs].len;
    }

    if (cmsg_arg == NULL)
        ncmsgs = 0;
    else {
        if ((cmsg_fast = PySequence_Fast(cmsg_arg,
                                         "sendmsg() argument 2 must be an "
                                         "iterable")) == NULL)
            goto finally;
        ncmsgs = PySequence_Fast_GET_SIZE(cmsg_fast);
    }

#ifndef CMSG_SPACE
    if (ncmsgs > 1) {
        PyErr_SetString(PyExc_OSError,
                        "sending multiple control messages is not supported "
                        "on this system");
        goto finally;
    }
#endif
    /* Save level, type and Py_buffer for each control message,
       and calculate total size. */
    if (ncmsgs > 0 && (cmsgs = PyMem_New(struct cmsginfo, ncmsgs)) == NULL) {
        PyErr_NoMemory();
        goto finally;
    }
    controllen = controllen_last = 0;
    while (ncmsgbufs < ncmsgs) {
        size_t bufsize, space;

        if (!PyArg_Parse(PySequence_Fast_GET_ITEM(cmsg_fast, ncmsgbufs),
                         "(iiy*):[sendmsg() ancillary data items]",
                         &cmsgs[ncmsgbufs].level,
                         &cmsgs[ncmsgbufs].type,
                         &cmsgs[ncmsgbufs].data))
            goto finally;
        bufsize = cmsgs[ncmsgbufs++].data.len;

#ifdef CMSG_SPACE
        if (!get_CMSG_SPACE(bufsize, &space)) {
#else
        if (!get_CMSG_LEN(bufsize, &space)) {
#endif
            PyErr_SetString(PyExc_OSError, "ancillary data item too large");
            goto finally;
        }
        controllen += space;
        if (controllen > SOCKLEN_T_LIMIT || controllen < controllen_last) {
            PyErr_SetString(PyExc_OSError, "too much ancillary data");
            goto finally;
        }
        controllen_last = controllen;
    }

    /* Construct ancillary data block from control message info. */
    if (ncmsgbufs > 0) {
        struct cmsghdr *cmsgh = NULL;

        if ((msg.msg_control = controlbuf =
             PyMem_Malloc(controllen)) == NULL) {
            PyErr_NoMemory();
            goto finally;
        }
        msg.msg_controllen = controllen;

        /* Need to zero out the buffer as a workaround for glibc's
           CMSG_NXTHDR() implementation.  After getting the pointer to
           the next header, it checks its (uninitialized) cmsg_len
           member to see if the "message" fits in the buffer, and
           returns NULL if it doesn't.  Zero-filling the buffer
           ensures that this doesn't happen. */
        memset(controlbuf, 0, controllen);

        for (i = 0; i < ncmsgbufs; i++) {
            size_t msg_len, data_len = cmsgs[i].data.len;
            int enough_space = 0;

            cmsgh = (i == 0) ? CMSG_FIRSTHDR(&msg) : CMSG_NXTHDR(&msg, cmsgh);
            if (cmsgh == NULL) {
                PyErr_Format(PyExc_RuntimeError,
                             "unexpected NULL result from %s()",
                             (i == 0) ? "CMSG_FIRSTHDR" : "CMSG_NXTHDR");
                goto finally;
            }
            if (!get_CMSG_LEN(data_len, &msg_len)) {
                PyErr_SetString(PyExc_RuntimeError,
                                "item size out of range for CMSG_LEN()");
                goto finally;
            }
            if (cmsg_min_space(&msg, cmsgh, msg_len)) {
                size_t space;

                cmsgh->cmsg_len = msg_len;
                if (get_cmsg_data_space(&msg, cmsgh, &space))
                    enough_space = (space >= data_len);
            }
            if (!enough_space) {
                PyErr_SetString(PyExc_RuntimeError,
                                "ancillary data does not fit in calculated "
                                "space");
                goto finally;
            }
            cmsgh->cmsg_level = cmsgs[i].level;
            cmsgh->cmsg_type = cmsgs[i].type;
            memcpy(CMSG_DATA(cmsgh), cmsgs[i].data.buf, data_len);
        }
    }

    /* Make the system call. */
    if (!IS_SELECTABLE(s)) {
        select_error();
        goto finally;
    }

    BEGIN_SELECT_LOOP(s)
    Py_BEGIN_ALLOW_THREADS;
    timeout = internal_select_ex(s, 1, interval);
    if (!timeout)
        bytes_sent = sendmsg(s->sock_fd, &msg, flags);
    Py_END_ALLOW_THREADS;
    if (timeout == 1) {
        PyErr_SetString(socket_timeout, "timed out");
        goto finally;
    }
    END_SELECT_LOOP(s)

    if (bytes_sent < 0) {
        s->errorhandler();
        goto finally;
    }
    retval = PyLong_FromSsize_t(bytes_sent);

finally:
    PyMem_Free(controlbuf);
    for (i = 0; i < ncmsgbufs; i++)
        PyBuffer_Release(&cmsgs[i].data);
    PyMem_Free(cmsgs);
    Py_XDECREF(cmsg_fast);
    for (i = 0; i < ndatabufs; i++)
        PyBuffer_Release(&databufs[i]);
    PyMem_Free(databufs);
    PyMem_Free(iovs);
    Py_XDECREF(data_fast);
    return retval;
}

PyDoc_STRVAR(sendmsg_doc,
"sendmsg(buffers[, ancdata[, flags[, address]]]) -> count\n\
\n\
Send normal and ancillary data to the socket, gathering the\n\
non-ancillary data from a series of buffers and concatenating it into\n\
a single message.  The buffers argument specifies the non-ancillary\n\
data as an iterable of buffer-compatible objects (e.g. bytes objects).\n\
The ancdata argument specifies the ancillary data (control messages)\n\
as an iterable of zero or more tuples (cmsg_level, cmsg_type,\n\
cmsg_data), where cmsg_level and cmsg_type are integers specifying the\n\
protocol level and protocol-specific type respectively, and cmsg_data\n\
is a buffer-compatible object holding the associated data.  The flags\n\
argument defaults to 0 and has the same meaning as for send().  If\n\
address is supplied and not None, it sets a destination address for\n\
the message.  The return value is the number of bytes of non-ancillary\n\
data sent.");
#endif    /* CMSG_LEN */


/* s.shutdown(how) method */

static PyObject *
sock_shutdown(PySocketSockObject *s, PyObject *arg)
{
    int how;
    int res;

    how = _PyLong_AsInt(arg);
    if (how == -1 && PyErr_Occurred())
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = shutdown(s->sock_fd, how);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(shutdown_doc,
"shutdown(flag)\n\
\n\
Shut down the reading side of the socket (flag == SHUT_RD), the writing side\n\
of the socket (flag == SHUT_WR), or both ends (flag == SHUT_RDWR).");

#if defined(MS_WINDOWS) && defined(SIO_RCVALL)
static PyObject*
sock_ioctl(PySocketSockObject *s, PyObject *arg)
{
    unsigned long cmd = SIO_RCVALL;
    PyObject *argO;
    DWORD recv;

    if (!PyArg_ParseTuple(arg, "kO:ioctl", &cmd, &argO))
        return NULL;

    switch (cmd) {
    case SIO_RCVALL: {
        unsigned int option = RCVALL_ON;
        if (!PyArg_ParseTuple(arg, "kI:ioctl", &cmd, &option))
            return NULL;
        if (WSAIoctl(s->sock_fd, cmd, &option, sizeof(option),
                         NULL, 0, &recv, NULL, NULL) == SOCKET_ERROR) {
            return set_error();
        }
        return PyLong_FromUnsignedLong(recv); }
    case SIO_KEEPALIVE_VALS: {
        struct tcp_keepalive ka;
        if (!PyArg_ParseTuple(arg, "k(kkk):ioctl", &cmd,
                        &ka.onoff, &ka.keepalivetime, &ka.keepaliveinterval))
            return NULL;
        if (WSAIoctl(s->sock_fd, cmd, &ka, sizeof(ka),
                         NULL, 0, &recv, NULL, NULL) == SOCKET_ERROR) {
            return set_error();
        }
        return PyLong_FromUnsignedLong(recv); }
    default:
        PyErr_Format(PyExc_ValueError, "invalid ioctl command %d", cmd);
        return NULL;
    }
}
PyDoc_STRVAR(sock_ioctl_doc,
"ioctl(cmd, option) -> long\n\
\n\
Control the socket with WSAIoctl syscall. Currently supported 'cmd' values are\n\
SIO_RCVALL:  'option' must be one of the socket.RCVALL_* constants.\n\
SIO_KEEPALIVE_VALS:  'option' is a tuple of (onoff, timeout, interval).");
#endif

#if defined(MS_WINDOWS)
static PyObject*
sock_share(PySocketSockObject *s, PyObject *arg)
{
    WSAPROTOCOL_INFO info;
    DWORD processId;
    int result;

    if (!PyArg_ParseTuple(arg, "I", &processId))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    result = WSADuplicateSocket(s->sock_fd, processId, &info);
    Py_END_ALLOW_THREADS
    if (result == SOCKET_ERROR)
        return set_error();
    return PyBytes_FromStringAndSize((const char*)&info, sizeof(info));
}
PyDoc_STRVAR(sock_share_doc,
"share(process_id) -> bytes\n\
\n\
Share the socket with another process.  The target process id\n\
must be provided and the resulting bytes object passed to the target\n\
process.  There the shared socket can be instantiated by calling\n\
socket.fromshare().");


#endif

/* List of methods for socket objects */

static PyMethodDef sock_methods[] = {
    {"_accept",           (PyCFunction)sock_accept, METH_NOARGS,
                      accept_doc},
    {"bind",              (PyCFunction)sock_bind, METH_O,
                      bind_doc},
    {"close",             (PyCFunction)sock_close, METH_NOARGS,
                      close_doc},
    {"connect",           (PyCFunction)sock_connect, METH_O,
                      connect_doc},
    {"connect_ex",        (PyCFunction)sock_connect_ex, METH_O,
                      connect_ex_doc},
    {"detach",            (PyCFunction)sock_detach, METH_NOARGS,
                      detach_doc},
    {"fileno",            (PyCFunction)sock_fileno, METH_NOARGS,
                      fileno_doc},
#ifdef HAVE_GETPEERNAME
    {"getpeername",       (PyCFunction)sock_getpeername,
                      METH_NOARGS, getpeername_doc},
#endif
    {"getsockname",       (PyCFunction)sock_getsockname,
                      METH_NOARGS, getsockname_doc},
    {"getsockopt",        (PyCFunction)sock_getsockopt, METH_VARARGS,
                      getsockopt_doc},
#if defined(MS_WINDOWS) && defined(SIO_RCVALL)
    {"ioctl",             (PyCFunction)sock_ioctl, METH_VARARGS,
                      sock_ioctl_doc},
#endif
#if defined(MS_WINDOWS)
    {"share",         (PyCFunction)sock_share, METH_VARARGS,
                      sock_share_doc},
#endif
    {"listen",            (PyCFunction)sock_listen, METH_O,
                      listen_doc},
    {"recv",              (PyCFunction)sock_recv, METH_VARARGS,
                      recv_doc},
    {"recv_into",         (PyCFunction)sock_recv_into, METH_VARARGS | METH_KEYWORDS,
                      recv_into_doc},
    {"recvfrom",          (PyCFunction)sock_recvfrom, METH_VARARGS,
                      recvfrom_doc},
    {"recvfrom_into",  (PyCFunction)sock_recvfrom_into, METH_VARARGS | METH_KEYWORDS,
                      recvfrom_into_doc},
    {"send",              (PyCFunction)sock_send, METH_VARARGS,
                      send_doc},
    {"sendall",           (PyCFunction)sock_sendall, METH_VARARGS,
                      sendall_doc},
    {"sendto",            (PyCFunction)sock_sendto, METH_VARARGS,
                      sendto_doc},
    {"setblocking",       (PyCFunction)sock_setblocking, METH_O,
                      setblocking_doc},
    {"settimeout",    (PyCFunction)sock_settimeout, METH_O,
                      settimeout_doc},
    {"gettimeout",    (PyCFunction)sock_gettimeout, METH_NOARGS,
                      gettimeout_doc},
    {"setsockopt",        (PyCFunction)sock_setsockopt, METH_VARARGS,
                      setsockopt_doc},
    {"shutdown",          (PyCFunction)sock_shutdown, METH_O,
                      shutdown_doc},
#ifdef CMSG_LEN
    {"recvmsg",           (PyCFunction)sock_recvmsg, METH_VARARGS,
                      recvmsg_doc},
    {"recvmsg_into",      (PyCFunction)sock_recvmsg_into, METH_VARARGS,
                      recvmsg_into_doc,},
    {"sendmsg",           (PyCFunction)sock_sendmsg, METH_VARARGS,
                      sendmsg_doc},
#endif
    {NULL,                      NULL}           /* sentinel */
};

/* SockObject members */
static PyMemberDef sock_memberlist[] = {
       {"family", T_INT, offsetof(PySocketSockObject, sock_family), READONLY, "the socket family"},
       {"type", T_INT, offsetof(PySocketSockObject, sock_type), READONLY, "the socket type"},
       {"proto", T_INT, offsetof(PySocketSockObject, sock_proto), READONLY, "the socket protocol"},
       {"timeout", T_DOUBLE, offsetof(PySocketSockObject, sock_timeout), READONLY, "the socket timeout"},
       {0},
};

/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

static void
sock_dealloc(PySocketSockObject *s)
{
    if (s->sock_fd != -1) {
        PyObject *exc, *val, *tb;
        Py_ssize_t old_refcount = Py_REFCNT(s);
        ++Py_REFCNT(s);
        PyErr_Fetch(&exc, &val, &tb);
        if (PyErr_WarnFormat(PyExc_ResourceWarning, 1,
                             "unclosed %R", s))
            /* Spurious errors can appear at shutdown */
            if (PyErr_ExceptionMatches(PyExc_Warning))
                PyErr_WriteUnraisable((PyObject *) s);
        PyErr_Restore(exc, val, tb);
        (void) SOCKETCLOSE(s->sock_fd);
        Py_REFCNT(s) = old_refcount;
    }
    Py_TYPE(s)->tp_free((PyObject *)s);
}


static PyObject *
sock_repr(PySocketSockObject *s)
{
#if SIZEOF_SOCKET_T > SIZEOF_LONG
    if (s->sock_fd > LONG_MAX) {
        /* this can occur on Win64, and actually there is a special
           ugly printf formatter for decimal pointer length integer
           printing, only bother if necessary*/
        PyErr_SetString(PyExc_OverflowError,
                        "no printf formatter to display "
                        "the socket descriptor in decimal");
        return NULL;
    }
#endif
    return PyUnicode_FromFormat(
        "<socket object, fd=%ld, family=%d, type=%d, proto=%d>",
        (long)s->sock_fd, s->sock_family,
        s->sock_type,
        s->sock_proto);
}


/* Create a new, uninitialized socket object. */

static PyObject *
sock_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *new;

    new = type->tp_alloc(type, 0);
    if (new != NULL) {
        ((PySocketSockObject *)new)->sock_fd = -1;
        ((PySocketSockObject *)new)->sock_timeout = -1.0;
        ((PySocketSockObject *)new)->errorhandler = &set_error;
    }
    return new;
}


/* Initialize a new socket object. */

/*ARGSUSED*/
static int
sock_initobj(PyObject *self, PyObject *args, PyObject *kwds)
{
    PySocketSockObject *s = (PySocketSockObject *)self;
    PyObject *fdobj = NULL;
    SOCKET_T fd = INVALID_SOCKET;
    int family = AF_INET, type = SOCK_STREAM, proto = 0;
    static char *keywords[] = {"family", "type", "proto", "fileno", 0};

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "|iiiO:socket", keywords,
                                     &family, &type, &proto, &fdobj))
        return -1;

    if (fdobj != NULL && fdobj != Py_None) {
#ifdef MS_WINDOWS
        /* recreate a socket that was duplicated */
        if (PyBytes_Check(fdobj)) {
            WSAPROTOCOL_INFO info;
            if (PyBytes_GET_SIZE(fdobj) != sizeof(info)) {
                PyErr_Format(PyExc_ValueError,
                    "socket descriptor string has wrong size, "
                    "should be %zu bytes.", sizeof(info));
                return -1;
            }
            memcpy(&info, PyBytes_AS_STRING(fdobj), sizeof(info));
            Py_BEGIN_ALLOW_THREADS
            fd = WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                     FROM_PROTOCOL_INFO, &info, 0, WSA_FLAG_OVERLAPPED);
            Py_END_ALLOW_THREADS
            if (fd == INVALID_SOCKET) {
                set_error();
                return -1;
            }
            family = info.iAddressFamily;
            type = info.iSocketType;
            proto = info.iProtocol;
        }
        else
#endif
        {
            fd = PyLong_AsSocket_t(fdobj);
            if (fd == (SOCKET_T)(-1) && PyErr_Occurred())
                return -1;
            if (fd == INVALID_SOCKET) {
                PyErr_SetString(PyExc_ValueError,
                                "can't use invalid socket value");
                return -1;
            }
        }
    }
    else {
        Py_BEGIN_ALLOW_THREADS
        fd = socket(family, type, proto);
        Py_END_ALLOW_THREADS

        if (fd == INVALID_SOCKET) {
            set_error();
            return -1;
        }
    }
    init_sockobject(s, fd, family, type, proto);

    return 0;

}


/* Type object for socket objects. */

static PyTypeObject sock_type = {
    PyVarObject_HEAD_INIT(0, 0)         /* Must fill in type value later */
    "_socket.socket",                           /* tp_name */
    sizeof(PySocketSockObject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)sock_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)sock_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    sock_doc,                                   /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    sock_methods,                               /* tp_methods */
    sock_memberlist,                            /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    sock_initobj,                               /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    sock_new,                                   /* tp_new */
    PyObject_Del,                               /* tp_free */
};


/* Python interface to gethostname(). */

/*ARGSUSED*/
static PyObject *
socket_gethostname(PyObject *self, PyObject *unused)
{
#ifdef MS_WINDOWS
    /* Don't use winsock's gethostname, as this returns the ANSI
       version of the hostname, whereas we need a Unicode string.
       Otherwise, gethostname apparently also returns the DNS name. */
    wchar_t buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = Py_ARRAY_LENGTH(buf);
    wchar_t *name;
    PyObject *result;

    if (GetComputerNameExW(ComputerNamePhysicalDnsHostname, buf, &size))
        return PyUnicode_FromWideChar(buf, size);

    if (GetLastError() != ERROR_MORE_DATA)
        return PyErr_SetFromWindowsErr(0);

    if (size == 0)
        return PyUnicode_New(0, 0);

    /* MSDN says ERROR_MORE_DATA may occur because DNS allows longer
       names */
    name = PyMem_Malloc(size * sizeof(wchar_t));
    if (!name)
        return NULL;
    if (!GetComputerNameExW(ComputerNamePhysicalDnsHostname,
                           name,
                           &size))
    {
        PyMem_Free(name);
        return PyErr_SetFromWindowsErr(0);
    }

    result = PyUnicode_FromWideChar(name, size);
    PyMem_Free(name);
    return result;
#else
    char buf[1024];
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = gethostname(buf, (int) sizeof buf - 1);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return set_error();
    buf[sizeof buf - 1] = '\0';
    return PyUnicode_FromString(buf);
#endif
}

PyDoc_STRVAR(gethostname_doc,
"gethostname() -> string\n\
\n\
Return the current host name.");

#ifdef HAVE_SETHOSTNAME
PyDoc_STRVAR(sethostname_doc,
"sethostname(name)\n\n\
Sets the hostname to name.");

static PyObject *
socket_sethostname(PyObject *self, PyObject *args)
{
    PyObject *hnobj;
    Py_buffer buf;
    int res, flag = 0;

    if (!PyArg_ParseTuple(args, "S:sethostname", &hnobj)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "O&:sethostname",
                PyUnicode_FSConverter, &hnobj))
            return NULL;
        flag = 1;
    }
    res = PyObject_GetBuffer(hnobj, &buf, PyBUF_SIMPLE);
    if (!res) {
        res = sethostname(buf.buf, buf.len);
        PyBuffer_Release(&buf);
    }
    if (flag)
        Py_DECREF(hnobj);
    if (res)
        return set_error();
    Py_RETURN_NONE;
}
#endif

/* Python interface to gethostbyname(name). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyname(PyObject *self, PyObject *args)
{
    char *name;
    sock_addr_t addrbuf;
    PyObject *ret = NULL;

    if (!PyArg_ParseTuple(args, "et:gethostbyname", "idna", &name))
        return NULL;
    if (setipaddr(name, SAS2SA(&addrbuf),  sizeof(addrbuf), AF_INET) < 0)
        goto finally;
    ret = makeipaddr(SAS2SA(&addrbuf), sizeof(struct sockaddr_in));
finally:
    PyMem_Free(name);
    return ret;
}

PyDoc_STRVAR(gethostbyname_doc,
"gethostbyname(host) -> address\n\
\n\
Return the IP address (a string of the form '255.255.255.255') for a host.");


/* Convenience function common to gethostbyname_ex and gethostbyaddr */

static PyObject *
gethost_common(struct hostent *h, struct sockaddr *addr, int alen, int af)
{
    char **pch;
    PyObject *rtn_tuple = (PyObject *)NULL;
    PyObject *name_list = (PyObject *)NULL;
    PyObject *addr_list = (PyObject *)NULL;
    PyObject *tmp;

    if (h == NULL) {
        /* Let's get real error message to return */
        set_herror(h_errno);
        return NULL;
    }

    if (h->h_addrtype != af) {
        /* Let's get real error message to return */
        errno = EAFNOSUPPORT;
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    switch (af) {

    case AF_INET:
        if (alen < sizeof(struct sockaddr_in))
            return NULL;
        break;

#ifdef ENABLE_IPV6
    case AF_INET6:
        if (alen < sizeof(struct sockaddr_in6))
            return NULL;
        break;
#endif

    }

    if ((name_list = PyList_New(0)) == NULL)
        goto err;

    if ((addr_list = PyList_New(0)) == NULL)
        goto err;

    /* SF #1511317: h_aliases can be NULL */
    if (h->h_aliases) {
        for (pch = h->h_aliases; *pch != NULL; pch++) {
            int status;
            tmp = PyUnicode_FromString(*pch);
            if (tmp == NULL)
                goto err;

            status = PyList_Append(name_list, tmp);
            Py_DECREF(tmp);

            if (status)
                goto err;
        }
    }

    for (pch = h->h_addr_list; *pch != NULL; pch++) {
        int status;

        switch (af) {

        case AF_INET:
            {
            struct sockaddr_in sin;
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = af;
#ifdef HAVE_SOCKADDR_SA_LEN
            sin.sin_len = sizeof(sin);
#endif
            memcpy(&sin.sin_addr, *pch, sizeof(sin.sin_addr));
            tmp = makeipaddr((struct sockaddr *)&sin, sizeof(sin));

            if (pch == h->h_addr_list && alen >= sizeof(sin))
                memcpy((char *) addr, &sin, sizeof(sin));
            break;
            }

#ifdef ENABLE_IPV6
        case AF_INET6:
            {
            struct sockaddr_in6 sin6;
            memset(&sin6, 0, sizeof(sin6));
            sin6.sin6_family = af;
#ifdef HAVE_SOCKADDR_SA_LEN
            sin6.sin6_len = sizeof(sin6);
#endif
            memcpy(&sin6.sin6_addr, *pch, sizeof(sin6.sin6_addr));
            tmp = makeipaddr((struct sockaddr *)&sin6,
                sizeof(sin6));

            if (pch == h->h_addr_list && alen >= sizeof(sin6))
                memcpy((char *) addr, &sin6, sizeof(sin6));
            break;
            }
#endif

        default:                /* can't happen */
            PyErr_SetString(PyExc_OSError,
                            "unsupported address family");
            return NULL;
        }

        if (tmp == NULL)
            goto err;

        status = PyList_Append(addr_list, tmp);
        Py_DECREF(tmp);

        if (status)
            goto err;
    }

    rtn_tuple = Py_BuildValue("sOO", h->h_name, name_list, addr_list);

 err:
    Py_XDECREF(name_list);
    Py_XDECREF(addr_list);
    return rtn_tuple;
}


/* Python interface to gethostbyname_ex(name). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyname_ex(PyObject *self, PyObject *args)
{
    char *name;
    struct hostent *h;
    sock_addr_t addr;
    struct sockaddr *sa;
    PyObject *ret = NULL;
#ifdef HAVE_GETHOSTBYNAME_R
    struct hostent hp_allocated;
#ifdef HAVE_GETHOSTBYNAME_R_3_ARG
    struct hostent_data data;
#else
    char buf[16384];
    int buf_len = (sizeof buf) - 1;
    int errnop;
#endif
#ifdef HAVE_GETHOSTBYNAME_R_3_ARG
    int result;
#endif
#endif /* HAVE_GETHOSTBYNAME_R */

    if (!PyArg_ParseTuple(args, "et:gethostbyname_ex", "idna", &name))
        return NULL;
    if (setipaddr(name, SAS2SA(&addr), sizeof(addr), AF_INET) < 0)
        goto finally;
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_GETHOSTBYNAME_R
#if   defined(HAVE_GETHOSTBYNAME_R_6_ARG)
    gethostbyname_r(name, &hp_allocated, buf, buf_len,
                             &h, &errnop);
#elif defined(HAVE_GETHOSTBYNAME_R_5_ARG)
    h = gethostbyname_r(name, &hp_allocated, buf, buf_len, &errnop);
#else /* HAVE_GETHOSTBYNAME_R_3_ARG */
    memset((void *) &data, '\0', sizeof(data));
    result = gethostbyname_r(name, &hp_allocated, &data);
    h = (result != 0) ? NULL : &hp_allocated;
#endif
#else /* not HAVE_GETHOSTBYNAME_R */
#ifdef USE_GETHOSTBYNAME_LOCK
    PyThread_acquire_lock(netdb_lock, 1);
#endif
    h = gethostbyname(name);
#endif /* HAVE_GETHOSTBYNAME_R */
    Py_END_ALLOW_THREADS
    /* Some C libraries would require addr.__ss_family instead of
       addr.ss_family.
       Therefore, we cast the sockaddr_storage into sockaddr to
       access sa_family. */
    sa = SAS2SA(&addr);
    ret = gethost_common(h, SAS2SA(&addr), sizeof(addr),
                         sa->sa_family);
#ifdef USE_GETHOSTBYNAME_LOCK
    PyThread_release_lock(netdb_lock);
#endif
finally:
    PyMem_Free(name);
    return ret;
}

PyDoc_STRVAR(ghbn_ex_doc,
"gethostbyname_ex(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.");


/* Python interface to gethostbyaddr(IP). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyaddr(PyObject *self, PyObject *args)
{
    sock_addr_t addr;
    struct sockaddr *sa = SAS2SA(&addr);
    char *ip_num;
    struct hostent *h;
    PyObject *ret = NULL;
#ifdef HAVE_GETHOSTBYNAME_R
    struct hostent hp_allocated;
#ifdef HAVE_GETHOSTBYNAME_R_3_ARG
    struct hostent_data data;
#else
    /* glibcs up to 2.10 assume that the buf argument to
       gethostbyaddr_r is 8-byte aligned, which at least llvm-gcc
       does not ensure. The attribute below instructs the compiler
       to maintain this alignment. */
    char buf[16384] Py_ALIGNED(8);
    int buf_len = (sizeof buf) - 1;
    int errnop;
#endif
#ifdef HAVE_GETHOSTBYNAME_R_3_ARG
    int result;
#endif
#endif /* HAVE_GETHOSTBYNAME_R */
    char *ap;
    int al;
    int af;

    if (!PyArg_ParseTuple(args, "et:gethostbyaddr", "idna", &ip_num))
        return NULL;
    af = AF_UNSPEC;
    if (setipaddr(ip_num, sa, sizeof(addr), af) < 0)
        goto finally;
    af = sa->sa_family;
    ap = NULL;
    /* al = 0; */
    switch (af) {
    case AF_INET:
        ap = (char *)&((struct sockaddr_in *)sa)->sin_addr;
        al = sizeof(((struct sockaddr_in *)sa)->sin_addr);
        break;
#ifdef ENABLE_IPV6
    case AF_INET6:
        ap = (char *)&((struct sockaddr_in6 *)sa)->sin6_addr;
        al = sizeof(((struct sockaddr_in6 *)sa)->sin6_addr);
        break;
#endif
    default:
        PyErr_SetString(PyExc_OSError, "unsupported address family");
        goto finally;
    }
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_GETHOSTBYNAME_R
#if   defined(HAVE_GETHOSTBYNAME_R_6_ARG)
    gethostbyaddr_r(ap, al, af,
        &hp_allocated, buf, buf_len,
        &h, &errnop);
#elif defined(HAVE_GETHOSTBYNAME_R_5_ARG)
    h = gethostbyaddr_r(ap, al, af,
                        &hp_allocated, buf, buf_len, &errnop);
#else /* HAVE_GETHOSTBYNAME_R_3_ARG */
    memset((void *) &data, '\0', sizeof(data));
    result = gethostbyaddr_r(ap, al, af, &hp_allocated, &data);
    h = (result != 0) ? NULL : &hp_allocated;
#endif
#else /* not HAVE_GETHOSTBYNAME_R */
#ifdef USE_GETHOSTBYNAME_LOCK
    PyThread_acquire_lock(netdb_lock, 1);
#endif
    h = gethostbyaddr(ap, al, af);
#endif /* HAVE_GETHOSTBYNAME_R */
    Py_END_ALLOW_THREADS
    ret = gethost_common(h, SAS2SA(&addr), sizeof(addr), af);
#ifdef USE_GETHOSTBYNAME_LOCK
    PyThread_release_lock(netdb_lock);
#endif
finally:
    PyMem_Free(ip_num);
    return ret;
}

PyDoc_STRVAR(gethostbyaddr_doc,
"gethostbyaddr(host) -> (name, aliaslist, addresslist)\n\
\n\
Return the true host name, a list of aliases, and a list of IP addresses,\n\
for a host.  The host argument is a string giving a host name or IP number.");


/* Python interface to getservbyname(name).
   This only returns the port number, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getservbyname(PyObject *self, PyObject *args)
{
    char *name, *proto=NULL;
    struct servent *sp;
    if (!PyArg_ParseTuple(args, "s|s:getservbyname", &name, &proto))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    sp = getservbyname(name, proto);
    Py_END_ALLOW_THREADS
    if (sp == NULL) {
        PyErr_SetString(PyExc_OSError, "service/proto not found");
        return NULL;
    }
    return PyLong_FromLong((long) ntohs(sp->s_port));
}

PyDoc_STRVAR(getservbyname_doc,
"getservbyname(servicename[, protocolname]) -> integer\n\
\n\
Return a port number from a service name and protocol name.\n\
The optional protocol name, if given, should be 'tcp' or 'udp',\n\
otherwise any protocol will match.");


/* Python interface to getservbyport(port).
   This only returns the service name, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getservbyport(PyObject *self, PyObject *args)
{
    int port;
    char *proto=NULL;
    struct servent *sp;
    if (!PyArg_ParseTuple(args, "i|s:getservbyport", &port, &proto))
        return NULL;
    if (port < 0 || port > 0xffff) {
        PyErr_SetString(
            PyExc_OverflowError,
            "getservbyport: port must be 0-65535.");
        return NULL;
    }
    Py_BEGIN_ALLOW_THREADS
    sp = getservbyport(htons((short)port), proto);
    Py_END_ALLOW_THREADS
    if (sp == NULL) {
        PyErr_SetString(PyExc_OSError, "port/proto not found");
        return NULL;
    }
    return PyUnicode_FromString(sp->s_name);
}

PyDoc_STRVAR(getservbyport_doc,
"getservbyport(port[, protocolname]) -> string\n\
\n\
Return the service name from a port number and protocol name.\n\
The optional protocol name, if given, should be 'tcp' or 'udp',\n\
otherwise any protocol will match.");

/* Python interface to getprotobyname(name).
   This only returns the protocol number, since the other info is
   already known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getprotobyname(PyObject *self, PyObject *args)
{
    char *name;
    struct protoent *sp;
    if (!PyArg_ParseTuple(args, "s:getprotobyname", &name))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    sp = getprotobyname(name);
    Py_END_ALLOW_THREADS
    if (sp == NULL) {
        PyErr_SetString(PyExc_OSError, "protocol not found");
        return NULL;
    }
    return PyLong_FromLong((long) sp->p_proto);
}

PyDoc_STRVAR(getprotobyname_doc,
"getprotobyname(name) -> integer\n\
\n\
Return the protocol number for the named protocol.  (Rarely used.)");


#ifndef NO_DUP
/* dup() function for socket fds */

static PyObject *
socket_dup(PyObject *self, PyObject *fdobj)
{
    SOCKET_T fd, newfd;
    PyObject *newfdobj;


    fd = PyLong_AsSocket_t(fdobj);
    if (fd == (SOCKET_T)(-1) && PyErr_Occurred())
        return NULL;

    newfd = dup_socket(fd);
    if (newfd == INVALID_SOCKET)
        return set_error();

    newfdobj = PyLong_FromSocket_t(newfd);
    if (newfdobj == NULL)
        SOCKETCLOSE(newfd);
    return newfdobj;
}

PyDoc_STRVAR(dup_doc,
"dup(integer) -> integer\n\
\n\
Duplicate an integer socket file descriptor.  This is like os.dup(), but for\n\
sockets; on some platforms os.dup() won't work for socket file descriptors.");
#endif


#ifdef HAVE_SOCKETPAIR
/* Create a pair of sockets using the socketpair() function.
   Arguments as for socket() except the default family is AF_UNIX if
   defined on the platform; otherwise, the default is AF_INET. */

/*ARGSUSED*/
static PyObject *
socket_socketpair(PyObject *self, PyObject *args)
{
    PySocketSockObject *s0 = NULL, *s1 = NULL;
    SOCKET_T sv[2];
    int family, type = SOCK_STREAM, proto = 0;
    PyObject *res = NULL;

#if defined(AF_UNIX)
    family = AF_UNIX;
#else
    family = AF_INET;
#endif
    if (!PyArg_ParseTuple(args, "|iii:socketpair",
                          &family, &type, &proto))
        return NULL;
    /* Create a pair of socket fds */
    if (socketpair(family, type, proto, sv) < 0)
        return set_error();
    s0 = new_sockobject(sv[0], family, type, proto);
    if (s0 == NULL)
        goto finally;
    s1 = new_sockobject(sv[1], family, type, proto);
    if (s1 == NULL)
        goto finally;
    res = PyTuple_Pack(2, s0, s1);

finally:
    if (res == NULL) {
        if (s0 == NULL)
            SOCKETCLOSE(sv[0]);
        if (s1 == NULL)
            SOCKETCLOSE(sv[1]);
    }
    Py_XDECREF(s0);
    Py_XDECREF(s1);
    return res;
}

PyDoc_STRVAR(socketpair_doc,
"socketpair([family[, type[, proto]]]) -> (socket object, socket object)\n\
\n\
Create a pair of socket objects from the sockets returned by the platform\n\
socketpair() function.\n\
The arguments are the same as for socket() except the default family is\n\
AF_UNIX if defined on the platform; otherwise, the default is AF_INET.");

#endif /* HAVE_SOCKETPAIR */


static PyObject *
socket_ntohs(PyObject *self, PyObject *args)
{
    int x1, x2;

    if (!PyArg_ParseTuple(args, "i:ntohs", &x1)) {
        return NULL;
    }
    if (x1 < 0) {
        PyErr_SetString(PyExc_OverflowError,
            "can't convert negative number to unsigned long");
        return NULL;
    }
    x2 = (unsigned int)ntohs((unsigned short)x1);
    return PyLong_FromLong(x2);
}

PyDoc_STRVAR(ntohs_doc,
"ntohs(integer) -> integer\n\
\n\
Convert a 16-bit integer from network to host byte order.");


static PyObject *
socket_ntohl(PyObject *self, PyObject *arg)
{
    unsigned long x;

    if (PyLong_Check(arg)) {
        x = PyLong_AsUnsignedLong(arg);
        if (x == (unsigned long) -1 && PyErr_Occurred())
            return NULL;
#if SIZEOF_LONG > 4
        {
            unsigned long y;
            /* only want the trailing 32 bits */
            y = x & 0xFFFFFFFFUL;
            if (y ^ x)
                return PyErr_Format(PyExc_OverflowError,
                            "long int larger than 32 bits");
            x = y;
        }
#endif
    }
    else
        return PyErr_Format(PyExc_TypeError,
                            "expected int/long, %s found",
                            Py_TYPE(arg)->tp_name);
    if (x == (unsigned long) -1 && PyErr_Occurred())
        return NULL;
    return PyLong_FromUnsignedLong(ntohl(x));
}

PyDoc_STRVAR(ntohl_doc,
"ntohl(integer) -> integer\n\
\n\
Convert a 32-bit integer from network to host byte order.");


static PyObject *
socket_htons(PyObject *self, PyObject *args)
{
    int x1, x2;

    if (!PyArg_ParseTuple(args, "i:htons", &x1)) {
        return NULL;
    }
    if (x1 < 0) {
        PyErr_SetString(PyExc_OverflowError,
            "can't convert negative number to unsigned long");
        return NULL;
    }
    x2 = (unsigned int)htons((unsigned short)x1);
    return PyLong_FromLong(x2);
}

PyDoc_STRVAR(htons_doc,
"htons(integer) -> integer\n\
\n\
Convert a 16-bit integer from host to network byte order.");


static PyObject *
socket_htonl(PyObject *self, PyObject *arg)
{
    unsigned long x;

    if (PyLong_Check(arg)) {
        x = PyLong_AsUnsignedLong(arg);
        if (x == (unsigned long) -1 && PyErr_Occurred())
            return NULL;
#if SIZEOF_LONG > 4
        {
            unsigned long y;
            /* only want the trailing 32 bits */
            y = x & 0xFFFFFFFFUL;
            if (y ^ x)
                return PyErr_Format(PyExc_OverflowError,
                            "long int larger than 32 bits");
            x = y;
        }
#endif
    }
    else
        return PyErr_Format(PyExc_TypeError,
                            "expected int/long, %s found",
                            Py_TYPE(arg)->tp_name);
    return PyLong_FromUnsignedLong(htonl((unsigned long)x));
}

PyDoc_STRVAR(htonl_doc,
"htonl(integer) -> integer\n\
\n\
Convert a 32-bit integer from host to network byte order.");

/* socket.inet_aton() and socket.inet_ntoa() functions. */

PyDoc_STRVAR(inet_aton_doc,
"inet_aton(string) -> bytes giving packed 32-bit IP representation\n\
\n\
Convert an IP address in string format (123.45.67.89) to the 32-bit packed\n\
binary format used in low-level network functions.");

static PyObject*
socket_inet_aton(PyObject *self, PyObject *args)
{
#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif
#ifdef HAVE_INET_ATON
    struct in_addr buf;
#endif

#if !defined(HAVE_INET_ATON) || defined(USE_INET_ATON_WEAKLINK)
#if (SIZEOF_INT != 4)
#error "Not sure if in_addr_t exists and int is not 32-bits."
#endif
    /* Have to use inet_addr() instead */
    unsigned int packed_addr;
#endif
    char *ip_addr;

    if (!PyArg_ParseTuple(args, "s:inet_aton", &ip_addr))
        return NULL;


#ifdef HAVE_INET_ATON

#ifdef USE_INET_ATON_WEAKLINK
    if (inet_aton != NULL) {
#endif
    if (inet_aton(ip_addr, &buf))
        return PyBytes_FromStringAndSize((char *)(&buf),
                                          sizeof(buf));

    PyErr_SetString(PyExc_OSError,
                    "illegal IP address string passed to inet_aton");
    return NULL;

#ifdef USE_INET_ATON_WEAKLINK
   } else {
#endif

#endif

#if !defined(HAVE_INET_ATON) || defined(USE_INET_ATON_WEAKLINK)

    /* special-case this address as inet_addr might return INADDR_NONE
     * for this */
    if (strcmp(ip_addr, "255.255.255.255") == 0) {
        packed_addr = 0xFFFFFFFF;
    } else {

        packed_addr = inet_addr(ip_addr);

        if (packed_addr == INADDR_NONE) {               /* invalid address */
            PyErr_SetString(PyExc_OSError,
                "illegal IP address string passed to inet_aton");
            return NULL;
        }
    }
    return PyBytes_FromStringAndSize((char *) &packed_addr,
                                      sizeof(packed_addr));

#ifdef USE_INET_ATON_WEAKLINK
   }
#endif

#endif
}

PyDoc_STRVAR(inet_ntoa_doc,
"inet_ntoa(packed_ip) -> ip_address_string\n\
\n\
Convert an IP address from 32-bit packed binary format to string format");

static PyObject*
socket_inet_ntoa(PyObject *self, PyObject *args)
{
    char *packed_str;
    int addr_len;
    struct in_addr packed_addr;

    if (!PyArg_ParseTuple(args, "y#:inet_ntoa", &packed_str, &addr_len)) {
        return NULL;
    }

    if (addr_len != sizeof(packed_addr)) {
        PyErr_SetString(PyExc_OSError,
            "packed IP wrong length for inet_ntoa");
        return NULL;
    }

    memcpy(&packed_addr, packed_str, addr_len);

    return PyUnicode_FromString(inet_ntoa(packed_addr));
}

#ifdef HAVE_INET_PTON

PyDoc_STRVAR(inet_pton_doc,
"inet_pton(af, ip) -> packed IP address string\n\
\n\
Convert an IP address from string format to a packed string suitable\n\
for use with low-level network functions.");

static PyObject *
socket_inet_pton(PyObject *self, PyObject *args)
{
    int af;
    char* ip;
    int retval;
#ifdef ENABLE_IPV6
    char packed[MAX(sizeof(struct in_addr), sizeof(struct in6_addr))];
#else
    char packed[sizeof(struct in_addr)];
#endif
    if (!PyArg_ParseTuple(args, "is:inet_pton", &af, &ip)) {
        return NULL;
    }

#if !defined(ENABLE_IPV6) && defined(AF_INET6)
    if(af == AF_INET6) {
        PyErr_SetString(PyExc_OSError,
                        "can't use AF_INET6, IPv6 is disabled");
        return NULL;
    }
#endif

    retval = inet_pton(af, ip, packed);
    if (retval < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    } else if (retval == 0) {
        PyErr_SetString(PyExc_OSError,
            "illegal IP address string passed to inet_pton");
        return NULL;
    } else if (af == AF_INET) {
        return PyBytes_FromStringAndSize(packed,
                                          sizeof(struct in_addr));
#ifdef ENABLE_IPV6
    } else if (af == AF_INET6) {
        return PyBytes_FromStringAndSize(packed,
                                          sizeof(struct in6_addr));
#endif
    } else {
        PyErr_SetString(PyExc_OSError, "unknown address family");
        return NULL;
    }
}

PyDoc_STRVAR(inet_ntop_doc,
"inet_ntop(af, packed_ip) -> string formatted IP address\n\
\n\
Convert a packed IP address of the given family to string format.");

static PyObject *
socket_inet_ntop(PyObject *self, PyObject *args)
{
    int af;
    char* packed;
    int len;
    const char* retval;
#ifdef ENABLE_IPV6
    char ip[MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1];
#else
    char ip[INET_ADDRSTRLEN + 1];
#endif

    /* Guarantee NUL-termination for PyUnicode_FromString() below */
    memset((void *) &ip[0], '\0', sizeof(ip));

    if (!PyArg_ParseTuple(args, "iy#:inet_ntop", &af, &packed, &len)) {
        return NULL;
    }

    if (af == AF_INET) {
        if (len != sizeof(struct in_addr)) {
            PyErr_SetString(PyExc_ValueError,
                "invalid length of packed IP address string");
            return NULL;
        }
#ifdef ENABLE_IPV6
    } else if (af == AF_INET6) {
        if (len != sizeof(struct in6_addr)) {
            PyErr_SetString(PyExc_ValueError,
                "invalid length of packed IP address string");
            return NULL;
        }
#endif
    } else {
        PyErr_Format(PyExc_ValueError,
            "unknown address family %d", af);
        return NULL;
    }

    retval = inet_ntop(af, packed, ip, sizeof(ip));
    if (!retval) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    } else {
        return PyUnicode_FromString(retval);
    }

    /* NOTREACHED */
    PyErr_SetString(PyExc_RuntimeError, "invalid handling of inet_ntop");
    return NULL;
}

#endif /* HAVE_INET_PTON */

/* Python interface to getaddrinfo(host, port). */

/*ARGSUSED*/
static PyObject *
socket_getaddrinfo(PyObject *self, PyObject *args, PyObject* kwargs)
{
    static char* kwnames[] = {"host", "port", "family", "type", "proto",
                              "flags", 0};
    struct addrinfo hints, *res;
    struct addrinfo *res0 = NULL;
    PyObject *hobj = NULL;
    PyObject *pobj = (PyObject *)NULL;
    char pbuf[30];
    char *hptr, *pptr;
    int family, socktype, protocol, flags;
    int error;
    PyObject *all = (PyObject *)NULL;
    PyObject *idna = NULL;

    family = socktype = protocol = flags = 0;
    family = AF_UNSPEC;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|iiii:getaddrinfo",
                          kwnames, &hobj, &pobj, &family, &socktype,
                          &protocol, &flags)) {
        return NULL;
    }
    if (hobj == Py_None) {
        hptr = NULL;
    } else if (PyUnicode_Check(hobj)) {
        _Py_IDENTIFIER(encode);

        idna = _PyObject_CallMethodId(hobj, &PyId_encode, "s", "idna");
        if (!idna)
            return NULL;
        assert(PyBytes_Check(idna));
        hptr = PyBytes_AS_STRING(idna);
    } else if (PyBytes_Check(hobj)) {
        hptr = PyBytes_AsString(hobj);
    } else {
        PyErr_SetString(PyExc_TypeError,
                        "getaddrinfo() argument 1 must be string or None");
        return NULL;
    }
    if (PyLong_CheckExact(pobj)) {
        long value = PyLong_AsLong(pobj);
        if (value == -1 && PyErr_Occurred())
            goto err;
        PyOS_snprintf(pbuf, sizeof(pbuf), "%ld", value);
        pptr = pbuf;
    } else if (PyUnicode_Check(pobj)) {
        pptr = _PyUnicode_AsString(pobj);
        if (pptr == NULL)
            goto err;
    } else if (PyBytes_Check(pobj)) {
        pptr = PyBytes_AS_STRING(pobj);
    } else if (pobj == Py_None) {
        pptr = (char *)NULL;
    } else {
        PyErr_SetString(PyExc_OSError, "Int or String expected");
        goto err;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = flags;
    Py_BEGIN_ALLOW_THREADS
    ACQUIRE_GETADDRINFO_LOCK
    error = getaddrinfo(hptr, pptr, &hints, &res0);
    Py_END_ALLOW_THREADS
    RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
    if (error) {
        set_gaierror(error);
        goto err;
    }

    if ((all = PyList_New(0)) == NULL)
        goto err;
    for (res = res0; res; res = res->ai_next) {
        PyObject *single;
        PyObject *addr =
            makesockaddr(-1, res->ai_addr, res->ai_addrlen, protocol);
        if (addr == NULL)
            goto err;
        single = Py_BuildValue("iiisO", res->ai_family,
            res->ai_socktype, res->ai_protocol,
            res->ai_canonname ? res->ai_canonname : "",
            addr);
        Py_DECREF(addr);
        if (single == NULL)
            goto err;

        if (PyList_Append(all, single))
            goto err;
        Py_XDECREF(single);
    }
    Py_XDECREF(idna);
    if (res0)
        freeaddrinfo(res0);
    return all;
 err:
    Py_XDECREF(all);
    Py_XDECREF(idna);
    if (res0)
        freeaddrinfo(res0);
    return (PyObject *)NULL;
}

PyDoc_STRVAR(getaddrinfo_doc,
"getaddrinfo(host, port [, family, socktype, proto, flags])\n\
    -> list of (family, socktype, proto, canonname, sockaddr)\n\
\n\
Resolve host and port into addrinfo struct.");

/* Python interface to getnameinfo(sa, flags). */

/*ARGSUSED*/
static PyObject *
socket_getnameinfo(PyObject *self, PyObject *args)
{
    PyObject *sa = (PyObject *)NULL;
    int flags;
    char *hostp;
    int port;
    unsigned int flowinfo, scope_id;
    char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
    struct addrinfo hints, *res = NULL;
    int error;
    PyObject *ret = (PyObject *)NULL;

    flags = flowinfo = scope_id = 0;
    if (!PyArg_ParseTuple(args, "Oi:getnameinfo", &sa, &flags))
        return NULL;
    if (!PyTuple_Check(sa)) {
        PyErr_SetString(PyExc_TypeError,
                        "getnameinfo() argument 1 must be a tuple");
        return NULL;
    }
    if (!PyArg_ParseTuple(sa, "si|II",
                          &hostp, &port, &flowinfo, &scope_id))
        return NULL;
    if (flowinfo > 0xfffff) {
        PyErr_SetString(PyExc_OverflowError,
                        "getsockaddrarg: flowinfo must be 0-1048575.");
        return NULL;
    }
    PyOS_snprintf(pbuf, sizeof(pbuf), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;     /* make numeric port happy */
    hints.ai_flags = AI_NUMERICHOST;    /* don't do any name resolution */
    Py_BEGIN_ALLOW_THREADS
    ACQUIRE_GETADDRINFO_LOCK
    error = getaddrinfo(hostp, pbuf, &hints, &res);
    Py_END_ALLOW_THREADS
    RELEASE_GETADDRINFO_LOCK  /* see comment in setipaddr() */
    if (error) {
        set_gaierror(error);
        goto fail;
    }
    if (res->ai_next) {
        PyErr_SetString(PyExc_OSError,
            "sockaddr resolved to multiple addresses");
        goto fail;
    }
    switch (res->ai_family) {
    case AF_INET:
        {
        if (PyTuple_GET_SIZE(sa) != 2) {
            PyErr_SetString(PyExc_OSError,
                "IPv4 sockaddr must be 2 tuple");
            goto fail;
        }
        break;
        }
#ifdef ENABLE_IPV6
    case AF_INET6:
        {
        struct sockaddr_in6 *sin6;
        sin6 = (struct sockaddr_in6 *)res->ai_addr;
        sin6->sin6_flowinfo = htonl(flowinfo);
        sin6->sin6_scope_id = scope_id;
        break;
        }
#endif
    }
    error = getnameinfo(res->ai_addr, (socklen_t) res->ai_addrlen,
                    hbuf, sizeof(hbuf), pbuf, sizeof(pbuf), flags);
    if (error) {
        set_gaierror(error);
        goto fail;
    }
    ret = Py_BuildValue("ss", hbuf, pbuf);

fail:
    if (res)
        freeaddrinfo(res);
    return ret;
}

PyDoc_STRVAR(getnameinfo_doc,
"getnameinfo(sockaddr, flags) --> (host, port)\n\
\n\
Get host and port for a sockaddr.");


/* Python API to getting and setting the default timeout value. */

static PyObject *
socket_getdefaulttimeout(PyObject *self)
{
    if (defaulttimeout < 0.0) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    else
        return PyFloat_FromDouble(defaulttimeout);
}

PyDoc_STRVAR(getdefaulttimeout_doc,
"getdefaulttimeout() -> timeout\n\
\n\
Returns the default timeout in seconds (float) for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");

static PyObject *
socket_setdefaulttimeout(PyObject *self, PyObject *arg)
{
    double timeout;

    if (arg == Py_None)
        timeout = -1.0;
    else {
        timeout = PyFloat_AsDouble(arg);
        if (timeout < 0.0) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError,
                                "Timeout value out of range");
            return NULL;
        }
    }

    defaulttimeout = timeout;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(setdefaulttimeout_doc,
"setdefaulttimeout(timeout)\n\
\n\
Set the default timeout in seconds (float) for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");

#ifdef HAVE_IF_NAMEINDEX
/* Python API for getting interface indices and names */

static PyObject *
socket_if_nameindex(PyObject *self, PyObject *arg)
{
    PyObject *list;
    int i;
    struct if_nameindex *ni;

    ni = if_nameindex();
    if (ni == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    list = PyList_New(0);
    if (list == NULL) {
        if_freenameindex(ni);
        return NULL;
    }

    for (i = 0; ni[i].if_index != 0 && i < INT_MAX; i++) {
        PyObject *ni_tuple = Py_BuildValue("IO&",
                ni[i].if_index, PyUnicode_DecodeFSDefault, ni[i].if_name);

        if (ni_tuple == NULL || PyList_Append(list, ni_tuple) == -1) {
            Py_XDECREF(ni_tuple);
            Py_DECREF(list);
            if_freenameindex(ni);
            return NULL;
        }
        Py_DECREF(ni_tuple);
    }

    if_freenameindex(ni);
    return list;
}

PyDoc_STRVAR(if_nameindex_doc,
"if_nameindex()\n\
\n\
Returns a list of network interface information (index, name) tuples.");

static PyObject *
socket_if_nametoindex(PyObject *self, PyObject *args)
{
    PyObject *oname;
    unsigned long index;

    if (!PyArg_ParseTuple(args, "O&:if_nametoindex",
                          PyUnicode_FSConverter, &oname))
        return NULL;

    index = if_nametoindex(PyBytes_AS_STRING(oname));
    Py_DECREF(oname);
    if (index == 0) {
        /* if_nametoindex() doesn't set errno */
        PyErr_SetString(PyExc_OSError, "no interface with this name");
        return NULL;
    }

    return PyLong_FromUnsignedLong(index);
}

PyDoc_STRVAR(if_nametoindex_doc,
"if_nametoindex(if_name)\n\
\n\
Returns the interface index corresponding to the interface name if_name.");

static PyObject *
socket_if_indextoname(PyObject *self, PyObject *arg)
{
    unsigned long index;
    char name[IF_NAMESIZE + 1];

    index = PyLong_AsUnsignedLong(arg);
    if (index == (unsigned long) -1)
        return NULL;

    if (if_indextoname(index, name) == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    return PyUnicode_DecodeFSDefault(name);
}

PyDoc_STRVAR(if_indextoname_doc,
"if_indextoname(if_index)\n\
\n\
Returns the interface name corresponding to the interface index if_index.");

#endif  /* HAVE_IF_NAMEINDEX */


#ifdef CMSG_LEN
/* Python interface to CMSG_LEN(length). */

static PyObject *
socket_CMSG_LEN(PyObject *self, PyObject *args)
{
    Py_ssize_t length;
    size_t result;

    if (!PyArg_ParseTuple(args, "n:CMSG_LEN", &length))
        return NULL;
    if (length < 0 || !get_CMSG_LEN(length, &result)) {
        PyErr_Format(PyExc_OverflowError, "CMSG_LEN() argument out of range");
        return NULL;
    }
    return PyLong_FromSize_t(result);
}

PyDoc_STRVAR(CMSG_LEN_doc,
"CMSG_LEN(length) -> control message length\n\
\n\
Return the total length, without trailing padding, of an ancillary\n\
data item with associated data of the given length.  This value can\n\
often be used as the buffer size for recvmsg() to receive a single\n\
item of ancillary data, but RFC 3542 requires portable applications to\n\
use CMSG_SPACE() and thus include space for padding, even when the\n\
item will be the last in the buffer.  Raises OverflowError if length\n\
is outside the permissible range of values.");


#ifdef CMSG_SPACE
/* Python interface to CMSG_SPACE(length). */

static PyObject *
socket_CMSG_SPACE(PyObject *self, PyObject *args)
{
    Py_ssize_t length;
    size_t result;

    if (!PyArg_ParseTuple(args, "n:CMSG_SPACE", &length))
        return NULL;
    if (length < 0 || !get_CMSG_SPACE(length, &result)) {
        PyErr_SetString(PyExc_OverflowError,
                        "CMSG_SPACE() argument out of range");
        return NULL;
    }
    return PyLong_FromSize_t(result);
}

PyDoc_STRVAR(CMSG_SPACE_doc,
"CMSG_SPACE(length) -> buffer size\n\
\n\
Return the buffer size needed for recvmsg() to receive an ancillary\n\
data item with associated data of the given length, along with any\n\
trailing padding.  The buffer space needed to receive multiple items\n\
is the sum of the CMSG_SPACE() values for their associated data\n\
lengths.  Raises OverflowError if length is outside the permissible\n\
range of values.");
#endif    /* CMSG_SPACE */
#endif    /* CMSG_LEN */


/* List of functions exported by this module. */

static PyMethodDef socket_methods[] = {
    {"gethostbyname",           socket_gethostbyname,
     METH_VARARGS, gethostbyname_doc},
    {"gethostbyname_ex",        socket_gethostbyname_ex,
     METH_VARARGS, ghbn_ex_doc},
    {"gethostbyaddr",           socket_gethostbyaddr,
     METH_VARARGS, gethostbyaddr_doc},
    {"gethostname",             socket_gethostname,
     METH_NOARGS,  gethostname_doc},
#ifdef HAVE_SETHOSTNAME
    {"sethostname",             socket_sethostname,
     METH_VARARGS,  sethostname_doc},
#endif
    {"getservbyname",           socket_getservbyname,
     METH_VARARGS, getservbyname_doc},
    {"getservbyport",           socket_getservbyport,
     METH_VARARGS, getservbyport_doc},
    {"getprotobyname",          socket_getprotobyname,
     METH_VARARGS, getprotobyname_doc},
#ifndef NO_DUP
    {"dup",                     socket_dup,
     METH_O, dup_doc},
#endif
#ifdef HAVE_SOCKETPAIR
    {"socketpair",              socket_socketpair,
     METH_VARARGS, socketpair_doc},
#endif
    {"ntohs",                   socket_ntohs,
     METH_VARARGS, ntohs_doc},
    {"ntohl",                   socket_ntohl,
     METH_O, ntohl_doc},
    {"htons",                   socket_htons,
     METH_VARARGS, htons_doc},
    {"htonl",                   socket_htonl,
     METH_O, htonl_doc},
    {"inet_aton",               socket_inet_aton,
     METH_VARARGS, inet_aton_doc},
    {"inet_ntoa",               socket_inet_ntoa,
     METH_VARARGS, inet_ntoa_doc},
#ifdef HAVE_INET_PTON
    {"inet_pton",               socket_inet_pton,
     METH_VARARGS, inet_pton_doc},
    {"inet_ntop",               socket_inet_ntop,
     METH_VARARGS, inet_ntop_doc},
#endif
    {"getaddrinfo",             (PyCFunction)socket_getaddrinfo,
     METH_VARARGS | METH_KEYWORDS, getaddrinfo_doc},
    {"getnameinfo",             socket_getnameinfo,
     METH_VARARGS, getnameinfo_doc},
    {"getdefaulttimeout",       (PyCFunction)socket_getdefaulttimeout,
     METH_NOARGS, getdefaulttimeout_doc},
    {"setdefaulttimeout",       socket_setdefaulttimeout,
     METH_O, setdefaulttimeout_doc},
#ifdef HAVE_IF_NAMEINDEX
    {"if_nameindex", socket_if_nameindex,
     METH_NOARGS, if_nameindex_doc},
    {"if_nametoindex", socket_if_nametoindex,
     METH_VARARGS, if_nametoindex_doc},
    {"if_indextoname", socket_if_indextoname,
     METH_O, if_indextoname_doc},
#endif
#ifdef CMSG_LEN
    {"CMSG_LEN",                socket_CMSG_LEN,
     METH_VARARGS, CMSG_LEN_doc},
#ifdef CMSG_SPACE
    {"CMSG_SPACE",              socket_CMSG_SPACE,
     METH_VARARGS, CMSG_SPACE_doc},
#endif
#endif
    {NULL,                      NULL}            /* Sentinel */
};


#ifdef MS_WINDOWS
#define OS_INIT_DEFINED

/* Additional initialization and cleanup for Windows */

static void
os_cleanup(void)
{
    WSACleanup();
}

static int
os_init(void)
{
    WSADATA WSAData;
    int ret;
    ret = WSAStartup(0x0101, &WSAData);
    switch (ret) {
    case 0:     /* No error */
        Py_AtExit(os_cleanup);
        return 1; /* Success */
    case WSASYSNOTREADY:
        PyErr_SetString(PyExc_ImportError,
                        "WSAStartup failed: network not ready");
        break;
    case WSAVERNOTSUPPORTED:
    case WSAEINVAL:
        PyErr_SetString(
            PyExc_ImportError,
            "WSAStartup failed: requested version not supported");
        break;
    default:
        PyErr_Format(PyExc_ImportError, "WSAStartup failed: error code %d", ret);
        break;
    }
    return 0; /* Failure */
}

#endif /* MS_WINDOWS */


#ifdef PYOS_OS2
#define OS_INIT_DEFINED

/* Additional initialization for OS/2 */

static int
os_init(void)
{
#ifndef PYCC_GCC
    int rc = sock_init();

    if (rc == 0) {
        return 1; /* Success */
    }

    PyErr_Format(PyExc_ImportError, "OS/2 TCP/IP Error# %d", sock_errno());

    return 0;  /* Failure */
#else
    /* No need to initialize sockets with GCC/EMX */
    return 1; /* Success */
#endif
}

#endif /* PYOS_OS2 */


#ifndef OS_INIT_DEFINED
static int
os_init(void)
{
    return 1; /* Success */
}
#endif


/* C API table - always add new things to the end for binary
   compatibility. */
static
PySocketModule_APIObject PySocketModuleAPI =
{
    &sock_type,
    NULL,
    NULL
};


/* Initialize the _socket module.

   This module is actually called "_socket", and there's a wrapper
   "socket.py" which implements some additional functionality.
   The import of "_socket" may fail with an ImportError exception if
   os-specific initialization fails.  On Windows, this does WINSOCK
   initialization.  When WINSOCK is initialized successfully, a call to
   WSACleanup() is scheduled to be made at exit time.
*/

PyDoc_STRVAR(socket_doc,
"Implementation module for socket operations.\n\
\n\
See the socket module for documentation.");

static struct PyModuleDef socketmodule = {
    PyModuleDef_HEAD_INIT,
    PySocket_MODULE_NAME,
    socket_doc,
    -1,
    socket_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__socket(void)
{
    PyObject *m, *has_ipv6;

    if (!os_init())
        return NULL;

    Py_TYPE(&sock_type) = &PyType_Type;
    m = PyModule_Create(&socketmodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(PyExc_OSError);
    PySocketModuleAPI.error = PyExc_OSError;
    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);
    socket_herror = PyErr_NewException("socket.herror",
                                       PyExc_OSError, NULL);
    if (socket_herror == NULL)
        return NULL;
    Py_INCREF(socket_herror);
    PyModule_AddObject(m, "herror", socket_herror);
    socket_gaierror = PyErr_NewException("socket.gaierror", PyExc_OSError,
        NULL);
    if (socket_gaierror == NULL)
        return NULL;
    Py_INCREF(socket_gaierror);
    PyModule_AddObject(m, "gaierror", socket_gaierror);
    socket_timeout = PyErr_NewException("socket.timeout",
                                        PyExc_OSError, NULL);
    if (socket_timeout == NULL)
        return NULL;
    PySocketModuleAPI.timeout_error = socket_timeout;
    Py_INCREF(socket_timeout);
    PyModule_AddObject(m, "timeout", socket_timeout);
    Py_INCREF((PyObject *)&sock_type);
    if (PyModule_AddObject(m, "SocketType",
                           (PyObject *)&sock_type) != 0)
        return NULL;
    Py_INCREF((PyObject *)&sock_type);
    if (PyModule_AddObject(m, "socket",
                           (PyObject *)&sock_type) != 0)
        return NULL;

#ifdef ENABLE_IPV6
    has_ipv6 = Py_True;
#else
    has_ipv6 = Py_False;
#endif
    Py_INCREF(has_ipv6);
    PyModule_AddObject(m, "has_ipv6", has_ipv6);

    /* Export C API */
    if (PyModule_AddObject(m, PySocket_CAPI_NAME,
           PyCapsule_New(&PySocketModuleAPI, PySocket_CAPSULE_NAME, NULL)
                             ) != 0)
        return NULL;

    /* Address families (we only support AF_INET and AF_UNIX) */
#ifdef AF_UNSPEC
    PyModule_AddIntConstant(m, "AF_UNSPEC", AF_UNSPEC);
#endif
    PyModule_AddIntConstant(m, "AF_INET", AF_INET);
#ifdef AF_INET6
    PyModule_AddIntConstant(m, "AF_INET6", AF_INET6);
#endif /* AF_INET6 */
#if defined(AF_UNIX)
    PyModule_AddIntConstant(m, "AF_UNIX", AF_UNIX);
#endif /* AF_UNIX */
#ifdef AF_AX25
    /* Amateur Radio AX.25 */
    PyModule_AddIntConstant(m, "AF_AX25", AF_AX25);
#endif
#ifdef AF_IPX
    PyModule_AddIntConstant(m, "AF_IPX", AF_IPX); /* Novell IPX */
#endif
#ifdef AF_APPLETALK
    /* Appletalk DDP */
    PyModule_AddIntConstant(m, "AF_APPLETALK", AF_APPLETALK);
#endif
#ifdef AF_NETROM
    /* Amateur radio NetROM */
    PyModule_AddIntConstant(m, "AF_NETROM", AF_NETROM);
#endif
#ifdef AF_BRIDGE
    /* Multiprotocol bridge */
    PyModule_AddIntConstant(m, "AF_BRIDGE", AF_BRIDGE);
#endif
#ifdef AF_ATMPVC
    /* ATM PVCs */
    PyModule_AddIntConstant(m, "AF_ATMPVC", AF_ATMPVC);
#endif
#ifdef AF_AAL5
    /* Reserved for Werner's ATM */
    PyModule_AddIntConstant(m, "AF_AAL5", AF_AAL5);
#endif
#ifdef AF_X25
    /* Reserved for X.25 project */
    PyModule_AddIntConstant(m, "AF_X25", AF_X25);
#endif
#ifdef AF_INET6
    PyModule_AddIntConstant(m, "AF_INET6", AF_INET6); /* IP version 6 */
#endif
#ifdef AF_ROSE
    /* Amateur Radio X.25 PLP */
    PyModule_AddIntConstant(m, "AF_ROSE", AF_ROSE);
#endif
#ifdef AF_DECnet
    /* Reserved for DECnet project */
    PyModule_AddIntConstant(m, "AF_DECnet", AF_DECnet);
#endif
#ifdef AF_NETBEUI
    /* Reserved for 802.2LLC project */
    PyModule_AddIntConstant(m, "AF_NETBEUI", AF_NETBEUI);
#endif
#ifdef AF_SECURITY
    /* Security callback pseudo AF */
    PyModule_AddIntConstant(m, "AF_SECURITY", AF_SECURITY);
#endif
#ifdef AF_KEY
    /* PF_KEY key management API */
    PyModule_AddIntConstant(m, "AF_KEY", AF_KEY);
#endif
#ifdef AF_NETLINK
    /*  */
    PyModule_AddIntConstant(m, "AF_NETLINK", AF_NETLINK);
    PyModule_AddIntConstant(m, "NETLINK_ROUTE", NETLINK_ROUTE);
#ifdef NETLINK_SKIP
    PyModule_AddIntConstant(m, "NETLINK_SKIP", NETLINK_SKIP);
#endif
#ifdef NETLINK_W1
    PyModule_AddIntConstant(m, "NETLINK_W1", NETLINK_W1);
#endif
    PyModule_AddIntConstant(m, "NETLINK_USERSOCK", NETLINK_USERSOCK);
    PyModule_AddIntConstant(m, "NETLINK_FIREWALL", NETLINK_FIREWALL);
#ifdef NETLINK_TCPDIAG
    PyModule_AddIntConstant(m, "NETLINK_TCPDIAG", NETLINK_TCPDIAG);
#endif
#ifdef NETLINK_NFLOG
    PyModule_AddIntConstant(m, "NETLINK_NFLOG", NETLINK_NFLOG);
#endif
#ifdef NETLINK_XFRM
    PyModule_AddIntConstant(m, "NETLINK_XFRM", NETLINK_XFRM);
#endif
#ifdef NETLINK_ARPD
    PyModule_AddIntConstant(m, "NETLINK_ARPD", NETLINK_ARPD);
#endif
#ifdef NETLINK_ROUTE6
    PyModule_AddIntConstant(m, "NETLINK_ROUTE6", NETLINK_ROUTE6);
#endif
    PyModule_AddIntConstant(m, "NETLINK_IP6_FW", NETLINK_IP6_FW);
#ifdef NETLINK_DNRTMSG
    PyModule_AddIntConstant(m, "NETLINK_DNRTMSG", NETLINK_DNRTMSG);
#endif
#ifdef NETLINK_TAPBASE
    PyModule_AddIntConstant(m, "NETLINK_TAPBASE", NETLINK_TAPBASE);
#endif
#endif /* AF_NETLINK */
#ifdef AF_ROUTE
    /* Alias to emulate 4.4BSD */
    PyModule_AddIntConstant(m, "AF_ROUTE", AF_ROUTE);
#endif
#ifdef AF_ASH
    /* Ash */
    PyModule_AddIntConstant(m, "AF_ASH", AF_ASH);
#endif
#ifdef AF_ECONET
    /* Acorn Econet */
    PyModule_AddIntConstant(m, "AF_ECONET", AF_ECONET);
#endif
#ifdef AF_ATMSVC
    /* ATM SVCs */
    PyModule_AddIntConstant(m, "AF_ATMSVC", AF_ATMSVC);
#endif
#ifdef AF_SNA
    /* Linux SNA Project (nutters!) */
    PyModule_AddIntConstant(m, "AF_SNA", AF_SNA);
#endif
#ifdef AF_IRDA
    /* IRDA sockets */
    PyModule_AddIntConstant(m, "AF_IRDA", AF_IRDA);
#endif
#ifdef AF_PPPOX
    /* PPPoX sockets */
    PyModule_AddIntConstant(m, "AF_PPPOX", AF_PPPOX);
#endif
#ifdef AF_WANPIPE
    /* Wanpipe API Sockets */
    PyModule_AddIntConstant(m, "AF_WANPIPE", AF_WANPIPE);
#endif
#ifdef AF_LLC
    /* Linux LLC */
    PyModule_AddIntConstant(m, "AF_LLC", AF_LLC);
#endif

#ifdef USE_BLUETOOTH
    PyModule_AddIntConstant(m, "AF_BLUETOOTH", AF_BLUETOOTH);
    PyModule_AddIntConstant(m, "BTPROTO_L2CAP", BTPROTO_L2CAP);
    PyModule_AddIntConstant(m, "BTPROTO_HCI", BTPROTO_HCI);
    PyModule_AddIntConstant(m, "SOL_HCI", SOL_HCI);
#if !defined(__NetBSD__) && !defined(__DragonFly__)
    PyModule_AddIntConstant(m, "HCI_FILTER", HCI_FILTER);
#endif
#if !defined(__FreeBSD__)
#if !defined(__NetBSD__) && !defined(__DragonFly__)
    PyModule_AddIntConstant(m, "HCI_TIME_STAMP", HCI_TIME_STAMP);
#endif
    PyModule_AddIntConstant(m, "HCI_DATA_DIR", HCI_DATA_DIR);
    PyModule_AddIntConstant(m, "BTPROTO_SCO", BTPROTO_SCO);
#endif
    PyModule_AddIntConstant(m, "BTPROTO_RFCOMM", BTPROTO_RFCOMM);
    PyModule_AddStringConstant(m, "BDADDR_ANY", "00:00:00:00:00:00");
    PyModule_AddStringConstant(m, "BDADDR_LOCAL", "00:00:00:FF:FF:FF");
#endif

#ifdef AF_CAN
    /* Controller Area Network */
    PyModule_AddIntConstant(m, "AF_CAN", AF_CAN);
#endif
#ifdef PF_CAN
    /* Controller Area Network */
    PyModule_AddIntConstant(m, "PF_CAN", PF_CAN);
#endif

/* Reliable Datagram Sockets */
#ifdef AF_RDS
    PyModule_AddIntConstant(m, "AF_RDS", AF_RDS);
#endif
#ifdef PF_RDS
    PyModule_AddIntConstant(m, "PF_RDS", PF_RDS);
#endif

/* Kernel event messages */
#ifdef PF_SYSTEM
    PyModule_AddIntConstant(m, "PF_SYSTEM", PF_SYSTEM);
#endif
#ifdef AF_SYSTEM
    PyModule_AddIntConstant(m, "AF_SYSTEM", AF_SYSTEM);
#endif

#ifdef AF_PACKET
    PyModule_AddIntMacro(m, AF_PACKET);
#endif
#ifdef PF_PACKET
    PyModule_AddIntMacro(m, PF_PACKET);
#endif
#ifdef PACKET_HOST
    PyModule_AddIntMacro(m, PACKET_HOST);
#endif
#ifdef PACKET_BROADCAST
    PyModule_AddIntMacro(m, PACKET_BROADCAST);
#endif
#ifdef PACKET_MULTICAST
    PyModule_AddIntMacro(m, PACKET_MULTICAST);
#endif
#ifdef PACKET_OTHERHOST
    PyModule_AddIntMacro(m, PACKET_OTHERHOST);
#endif
#ifdef PACKET_OUTGOING
    PyModule_AddIntMacro(m, PACKET_OUTGOING);
#endif
#ifdef PACKET_LOOPBACK
    PyModule_AddIntMacro(m, PACKET_LOOPBACK);
#endif
#ifdef PACKET_FASTROUTE
    PyModule_AddIntMacro(m, PACKET_FASTROUTE);
#endif

#ifdef HAVE_LINUX_TIPC_H
    PyModule_AddIntConstant(m, "AF_TIPC", AF_TIPC);

    /* for addresses */
    PyModule_AddIntConstant(m, "TIPC_ADDR_NAMESEQ", TIPC_ADDR_NAMESEQ);
    PyModule_AddIntConstant(m, "TIPC_ADDR_NAME", TIPC_ADDR_NAME);
    PyModule_AddIntConstant(m, "TIPC_ADDR_ID", TIPC_ADDR_ID);

    PyModule_AddIntConstant(m, "TIPC_ZONE_SCOPE", TIPC_ZONE_SCOPE);
    PyModule_AddIntConstant(m, "TIPC_CLUSTER_SCOPE", TIPC_CLUSTER_SCOPE);
    PyModule_AddIntConstant(m, "TIPC_NODE_SCOPE", TIPC_NODE_SCOPE);

    /* for setsockopt() */
    PyModule_AddIntConstant(m, "SOL_TIPC", SOL_TIPC);
    PyModule_AddIntConstant(m, "TIPC_IMPORTANCE", TIPC_IMPORTANCE);
    PyModule_AddIntConstant(m, "TIPC_SRC_DROPPABLE", TIPC_SRC_DROPPABLE);
    PyModule_AddIntConstant(m, "TIPC_DEST_DROPPABLE",
                    TIPC_DEST_DROPPABLE);
    PyModule_AddIntConstant(m, "TIPC_CONN_TIMEOUT", TIPC_CONN_TIMEOUT);

    PyModule_AddIntConstant(m, "TIPC_LOW_IMPORTANCE",
                    TIPC_LOW_IMPORTANCE);
    PyModule_AddIntConstant(m, "TIPC_MEDIUM_IMPORTANCE",
                    TIPC_MEDIUM_IMPORTANCE);
    PyModule_AddIntConstant(m, "TIPC_HIGH_IMPORTANCE",
                    TIPC_HIGH_IMPORTANCE);
    PyModule_AddIntConstant(m, "TIPC_CRITICAL_IMPORTANCE",
                    TIPC_CRITICAL_IMPORTANCE);

    /* for subscriptions */
    PyModule_AddIntConstant(m, "TIPC_SUB_PORTS", TIPC_SUB_PORTS);
    PyModule_AddIntConstant(m, "TIPC_SUB_SERVICE", TIPC_SUB_SERVICE);
#ifdef TIPC_SUB_CANCEL
    /* doesn't seem to be available everywhere */
    PyModule_AddIntConstant(m, "TIPC_SUB_CANCEL", TIPC_SUB_CANCEL);
#endif
    PyModule_AddIntConstant(m, "TIPC_WAIT_FOREVER", TIPC_WAIT_FOREVER);
    PyModule_AddIntConstant(m, "TIPC_PUBLISHED", TIPC_PUBLISHED);
    PyModule_AddIntConstant(m, "TIPC_WITHDRAWN", TIPC_WITHDRAWN);
    PyModule_AddIntConstant(m, "TIPC_SUBSCR_TIMEOUT", TIPC_SUBSCR_TIMEOUT);
    PyModule_AddIntConstant(m, "TIPC_CFG_SRV", TIPC_CFG_SRV);
    PyModule_AddIntConstant(m, "TIPC_TOP_SRV", TIPC_TOP_SRV);
#endif

    /* Socket types */
    PyModule_AddIntConstant(m, "SOCK_STREAM", SOCK_STREAM);
    PyModule_AddIntConstant(m, "SOCK_DGRAM", SOCK_DGRAM);
/* We have incomplete socket support. */
    PyModule_AddIntConstant(m, "SOCK_RAW", SOCK_RAW);
    PyModule_AddIntConstant(m, "SOCK_SEQPACKET", SOCK_SEQPACKET);
#if defined(SOCK_RDM)
    PyModule_AddIntConstant(m, "SOCK_RDM", SOCK_RDM);
#endif
#ifdef SOCK_CLOEXEC
    PyModule_AddIntConstant(m, "SOCK_CLOEXEC", SOCK_CLOEXEC);
#endif
#ifdef SOCK_NONBLOCK
    PyModule_AddIntConstant(m, "SOCK_NONBLOCK", SOCK_NONBLOCK);
#endif

#ifdef  SO_DEBUG
    PyModule_AddIntConstant(m, "SO_DEBUG", SO_DEBUG);
#endif
#ifdef  SO_ACCEPTCONN
    PyModule_AddIntConstant(m, "SO_ACCEPTCONN", SO_ACCEPTCONN);
#endif
#ifdef  SO_REUSEADDR
    PyModule_AddIntConstant(m, "SO_REUSEADDR", SO_REUSEADDR);
#endif
#ifdef SO_EXCLUSIVEADDRUSE
    PyModule_AddIntConstant(m, "SO_EXCLUSIVEADDRUSE", SO_EXCLUSIVEADDRUSE);
#endif

#ifdef  SO_KEEPALIVE
    PyModule_AddIntConstant(m, "SO_KEEPALIVE", SO_KEEPALIVE);
#endif
#ifdef  SO_DONTROUTE
    PyModule_AddIntConstant(m, "SO_DONTROUTE", SO_DONTROUTE);
#endif
#ifdef  SO_BROADCAST
    PyModule_AddIntConstant(m, "SO_BROADCAST", SO_BROADCAST);
#endif
#ifdef  SO_USELOOPBACK
    PyModule_AddIntConstant(m, "SO_USELOOPBACK", SO_USELOOPBACK);
#endif
#ifdef  SO_LINGER
    PyModule_AddIntConstant(m, "SO_LINGER", SO_LINGER);
#endif
#ifdef  SO_OOBINLINE
    PyModule_AddIntConstant(m, "SO_OOBINLINE", SO_OOBINLINE);
#endif
#ifdef  SO_REUSEPORT
    PyModule_AddIntConstant(m, "SO_REUSEPORT", SO_REUSEPORT);
#endif
#ifdef  SO_SNDBUF
    PyModule_AddIntConstant(m, "SO_SNDBUF", SO_SNDBUF);
#endif
#ifdef  SO_RCVBUF
    PyModule_AddIntConstant(m, "SO_RCVBUF", SO_RCVBUF);
#endif
#ifdef  SO_SNDLOWAT
    PyModule_AddIntConstant(m, "SO_SNDLOWAT", SO_SNDLOWAT);
#endif
#ifdef  SO_RCVLOWAT
    PyModule_AddIntConstant(m, "SO_RCVLOWAT", SO_RCVLOWAT);
#endif
#ifdef  SO_SNDTIMEO
    PyModule_AddIntConstant(m, "SO_SNDTIMEO", SO_SNDTIMEO);
#endif
#ifdef  SO_RCVTIMEO
    PyModule_AddIntConstant(m, "SO_RCVTIMEO", SO_RCVTIMEO);
#endif
#ifdef  SO_ERROR
    PyModule_AddIntConstant(m, "SO_ERROR", SO_ERROR);
#endif
#ifdef  SO_TYPE
    PyModule_AddIntConstant(m, "SO_TYPE", SO_TYPE);
#endif
#ifdef  SO_SETFIB
    PyModule_AddIntConstant(m, "SO_SETFIB", SO_SETFIB);
#endif
#ifdef  SO_PASSCRED
    PyModule_AddIntConstant(m, "SO_PASSCRED", SO_PASSCRED);
#endif
#ifdef  SO_PEERCRED
    PyModule_AddIntConstant(m, "SO_PEERCRED", SO_PEERCRED);
#endif
#ifdef  LOCAL_PEERCRED
    PyModule_AddIntConstant(m, "LOCAL_PEERCRED", LOCAL_PEERCRED);
#endif
#ifdef  SO_BINDTODEVICE
    PyModule_AddIntMacro(m, SO_BINDTODEVICE);
#endif

    /* Maximum number of connections for "listen" */
#ifdef  SOMAXCONN
    PyModule_AddIntConstant(m, "SOMAXCONN", SOMAXCONN);
#else
    PyModule_AddIntConstant(m, "SOMAXCONN", 5); /* Common value */
#endif

    /* Ancilliary message types */
#ifdef  SCM_RIGHTS
    PyModule_AddIntConstant(m, "SCM_RIGHTS", SCM_RIGHTS);
#endif
#ifdef  SCM_CREDENTIALS
    PyModule_AddIntConstant(m, "SCM_CREDENTIALS", SCM_CREDENTIALS);
#endif
#ifdef  SCM_CREDS
    PyModule_AddIntConstant(m, "SCM_CREDS", SCM_CREDS);
#endif

    /* Flags for send, recv */
#ifdef  MSG_OOB
    PyModule_AddIntConstant(m, "MSG_OOB", MSG_OOB);
#endif
#ifdef  MSG_PEEK
    PyModule_AddIntConstant(m, "MSG_PEEK", MSG_PEEK);
#endif
#ifdef  MSG_DONTROUTE
    PyModule_AddIntConstant(m, "MSG_DONTROUTE", MSG_DONTROUTE);
#endif
#ifdef  MSG_DONTWAIT
    PyModule_AddIntConstant(m, "MSG_DONTWAIT", MSG_DONTWAIT);
#endif
#ifdef  MSG_EOR
    PyModule_AddIntConstant(m, "MSG_EOR", MSG_EOR);
#endif
#ifdef  MSG_TRUNC
    PyModule_AddIntConstant(m, "MSG_TRUNC", MSG_TRUNC);
#endif
#ifdef  MSG_CTRUNC
    PyModule_AddIntConstant(m, "MSG_CTRUNC", MSG_CTRUNC);
#endif
#ifdef  MSG_WAITALL
    PyModule_AddIntConstant(m, "MSG_WAITALL", MSG_WAITALL);
#endif
#ifdef  MSG_BTAG
    PyModule_AddIntConstant(m, "MSG_BTAG", MSG_BTAG);
#endif
#ifdef  MSG_ETAG
    PyModule_AddIntConstant(m, "MSG_ETAG", MSG_ETAG);
#endif
#ifdef  MSG_NOSIGNAL
    PyModule_AddIntConstant(m, "MSG_NOSIGNAL", MSG_NOSIGNAL);
#endif
#ifdef  MSG_NOTIFICATION
    PyModule_AddIntConstant(m, "MSG_NOTIFICATION", MSG_NOTIFICATION);
#endif
#ifdef  MSG_CMSG_CLOEXEC
    PyModule_AddIntConstant(m, "MSG_CMSG_CLOEXEC", MSG_CMSG_CLOEXEC);
#endif
#ifdef  MSG_ERRQUEUE
    PyModule_AddIntConstant(m, "MSG_ERRQUEUE", MSG_ERRQUEUE);
#endif
#ifdef  MSG_CONFIRM
    PyModule_AddIntConstant(m, "MSG_CONFIRM", MSG_CONFIRM);
#endif
#ifdef  MSG_MORE
    PyModule_AddIntConstant(m, "MSG_MORE", MSG_MORE);
#endif
#ifdef  MSG_EOF
    PyModule_AddIntConstant(m, "MSG_EOF", MSG_EOF);
#endif
#ifdef  MSG_BCAST
    PyModule_AddIntConstant(m, "MSG_BCAST", MSG_BCAST);
#endif
#ifdef  MSG_MCAST
    PyModule_AddIntConstant(m, "MSG_MCAST", MSG_MCAST);
#endif

    /* Protocol level and numbers, usable for [gs]etsockopt */
#ifdef  SOL_SOCKET
    PyModule_AddIntConstant(m, "SOL_SOCKET", SOL_SOCKET);
#endif
#ifdef  SOL_IP
    PyModule_AddIntConstant(m, "SOL_IP", SOL_IP);
#else
    PyModule_AddIntConstant(m, "SOL_IP", 0);
#endif
#ifdef  SOL_IPX
    PyModule_AddIntConstant(m, "SOL_IPX", SOL_IPX);
#endif
#ifdef  SOL_AX25
    PyModule_AddIntConstant(m, "SOL_AX25", SOL_AX25);
#endif
#ifdef  SOL_ATALK
    PyModule_AddIntConstant(m, "SOL_ATALK", SOL_ATALK);
#endif
#ifdef  SOL_NETROM
    PyModule_AddIntConstant(m, "SOL_NETROM", SOL_NETROM);
#endif
#ifdef  SOL_ROSE
    PyModule_AddIntConstant(m, "SOL_ROSE", SOL_ROSE);
#endif
#ifdef  SOL_TCP
    PyModule_AddIntConstant(m, "SOL_TCP", SOL_TCP);
#else
    PyModule_AddIntConstant(m, "SOL_TCP", 6);
#endif
#ifdef  SOL_UDP
    PyModule_AddIntConstant(m, "SOL_UDP", SOL_UDP);
#else
    PyModule_AddIntConstant(m, "SOL_UDP", 17);
#endif
#ifdef SOL_CAN_BASE
    PyModule_AddIntConstant(m, "SOL_CAN_BASE", SOL_CAN_BASE);
#endif
#ifdef SOL_CAN_RAW
    PyModule_AddIntConstant(m, "SOL_CAN_RAW", SOL_CAN_RAW);
    PyModule_AddIntConstant(m, "CAN_RAW", CAN_RAW);
#endif
#ifdef HAVE_LINUX_CAN_H
    PyModule_AddIntConstant(m, "CAN_EFF_FLAG", CAN_EFF_FLAG);
    PyModule_AddIntConstant(m, "CAN_RTR_FLAG", CAN_RTR_FLAG);
    PyModule_AddIntConstant(m, "CAN_ERR_FLAG", CAN_ERR_FLAG);

    PyModule_AddIntConstant(m, "CAN_SFF_MASK", CAN_SFF_MASK);
    PyModule_AddIntConstant(m, "CAN_EFF_MASK", CAN_EFF_MASK);
    PyModule_AddIntConstant(m, "CAN_ERR_MASK", CAN_ERR_MASK);
#endif
#ifdef HAVE_LINUX_CAN_RAW_H
    PyModule_AddIntConstant(m, "CAN_RAW_FILTER", CAN_RAW_FILTER);
    PyModule_AddIntConstant(m, "CAN_RAW_ERR_FILTER", CAN_RAW_ERR_FILTER);
    PyModule_AddIntConstant(m, "CAN_RAW_LOOPBACK", CAN_RAW_LOOPBACK);
    PyModule_AddIntConstant(m, "CAN_RAW_RECV_OWN_MSGS", CAN_RAW_RECV_OWN_MSGS);
#endif
#ifdef SOL_RDS
    PyModule_AddIntConstant(m, "SOL_RDS", SOL_RDS);
#endif
#ifdef RDS_CANCEL_SENT_TO
    PyModule_AddIntConstant(m, "RDS_CANCEL_SENT_TO", RDS_CANCEL_SENT_TO);
#endif
#ifdef RDS_GET_MR
    PyModule_AddIntConstant(m, "RDS_GET_MR", RDS_GET_MR);
#endif
#ifdef RDS_FREE_MR
    PyModule_AddIntConstant(m, "RDS_FREE_MR", RDS_FREE_MR);
#endif
#ifdef RDS_RECVERR
    PyModule_AddIntConstant(m, "RDS_RECVERR", RDS_RECVERR);
#endif
#ifdef RDS_CONG_MONITOR
    PyModule_AddIntConstant(m, "RDS_CONG_MONITOR", RDS_CONG_MONITOR);
#endif
#ifdef RDS_GET_MR_FOR_DEST
    PyModule_AddIntConstant(m, "RDS_GET_MR_FOR_DEST", RDS_GET_MR_FOR_DEST);
#endif
#ifdef  IPPROTO_IP
    PyModule_AddIntConstant(m, "IPPROTO_IP", IPPROTO_IP);
#else
    PyModule_AddIntConstant(m, "IPPROTO_IP", 0);
#endif
#ifdef  IPPROTO_HOPOPTS
    PyModule_AddIntConstant(m, "IPPROTO_HOPOPTS", IPPROTO_HOPOPTS);
#endif
#ifdef  IPPROTO_ICMP
    PyModule_AddIntConstant(m, "IPPROTO_ICMP", IPPROTO_ICMP);
#else
    PyModule_AddIntConstant(m, "IPPROTO_ICMP", 1);
#endif
#ifdef  IPPROTO_IGMP
    PyModule_AddIntConstant(m, "IPPROTO_IGMP", IPPROTO_IGMP);
#endif
#ifdef  IPPROTO_GGP
    PyModule_AddIntConstant(m, "IPPROTO_GGP", IPPROTO_GGP);
#endif
#ifdef  IPPROTO_IPV4
    PyModule_AddIntConstant(m, "IPPROTO_IPV4", IPPROTO_IPV4);
#endif
#ifdef  IPPROTO_IPV6
    PyModule_AddIntConstant(m, "IPPROTO_IPV6", IPPROTO_IPV6);
#endif
#ifdef  IPPROTO_IPIP
    PyModule_AddIntConstant(m, "IPPROTO_IPIP", IPPROTO_IPIP);
#endif
#ifdef  IPPROTO_TCP
    PyModule_AddIntConstant(m, "IPPROTO_TCP", IPPROTO_TCP);
#else
    PyModule_AddIntConstant(m, "IPPROTO_TCP", 6);
#endif
#ifdef  IPPROTO_EGP
    PyModule_AddIntConstant(m, "IPPROTO_EGP", IPPROTO_EGP);
#endif
#ifdef  IPPROTO_PUP
    PyModule_AddIntConstant(m, "IPPROTO_PUP", IPPROTO_PUP);
#endif
#ifdef  IPPROTO_UDP
    PyModule_AddIntConstant(m, "IPPROTO_UDP", IPPROTO_UDP);
#else
    PyModule_AddIntConstant(m, "IPPROTO_UDP", 17);
#endif
#ifdef  IPPROTO_IDP
    PyModule_AddIntConstant(m, "IPPROTO_IDP", IPPROTO_IDP);
#endif
#ifdef  IPPROTO_HELLO
    PyModule_AddIntConstant(m, "IPPROTO_HELLO", IPPROTO_HELLO);
#endif
#ifdef  IPPROTO_ND
    PyModule_AddIntConstant(m, "IPPROTO_ND", IPPROTO_ND);
#endif
#ifdef  IPPROTO_TP
    PyModule_AddIntConstant(m, "IPPROTO_TP", IPPROTO_TP);
#endif
#ifdef  IPPROTO_IPV6
    PyModule_AddIntConstant(m, "IPPROTO_IPV6", IPPROTO_IPV6);
#endif
#ifdef  IPPROTO_ROUTING
    PyModule_AddIntConstant(m, "IPPROTO_ROUTING", IPPROTO_ROUTING);
#endif
#ifdef  IPPROTO_FRAGMENT
    PyModule_AddIntConstant(m, "IPPROTO_FRAGMENT", IPPROTO_FRAGMENT);
#endif
#ifdef  IPPROTO_RSVP
    PyModule_AddIntConstant(m, "IPPROTO_RSVP", IPPROTO_RSVP);
#endif
#ifdef  IPPROTO_GRE
    PyModule_AddIntConstant(m, "IPPROTO_GRE", IPPROTO_GRE);
#endif
#ifdef  IPPROTO_ESP
    PyModule_AddIntConstant(m, "IPPROTO_ESP", IPPROTO_ESP);
#endif
#ifdef  IPPROTO_AH
    PyModule_AddIntConstant(m, "IPPROTO_AH", IPPROTO_AH);
#endif
#ifdef  IPPROTO_MOBILE
    PyModule_AddIntConstant(m, "IPPROTO_MOBILE", IPPROTO_MOBILE);
#endif
#ifdef  IPPROTO_ICMPV6
    PyModule_AddIntConstant(m, "IPPROTO_ICMPV6", IPPROTO_ICMPV6);
#endif
#ifdef  IPPROTO_NONE
    PyModule_AddIntConstant(m, "IPPROTO_NONE", IPPROTO_NONE);
#endif
#ifdef  IPPROTO_DSTOPTS
    PyModule_AddIntConstant(m, "IPPROTO_DSTOPTS", IPPROTO_DSTOPTS);
#endif
#ifdef  IPPROTO_XTP
    PyModule_AddIntConstant(m, "IPPROTO_XTP", IPPROTO_XTP);
#endif
#ifdef  IPPROTO_EON
    PyModule_AddIntConstant(m, "IPPROTO_EON", IPPROTO_EON);
#endif
#ifdef  IPPROTO_PIM
    PyModule_AddIntConstant(m, "IPPROTO_PIM", IPPROTO_PIM);
#endif
#ifdef  IPPROTO_IPCOMP
    PyModule_AddIntConstant(m, "IPPROTO_IPCOMP", IPPROTO_IPCOMP);
#endif
#ifdef  IPPROTO_VRRP
    PyModule_AddIntConstant(m, "IPPROTO_VRRP", IPPROTO_VRRP);
#endif
#ifdef  IPPROTO_SCTP
    PyModule_AddIntConstant(m, "IPPROTO_SCTP", IPPROTO_SCTP);
#endif
#ifdef  IPPROTO_BIP
    PyModule_AddIntConstant(m, "IPPROTO_BIP", IPPROTO_BIP);
#endif
/**/
#ifdef  IPPROTO_RAW
    PyModule_AddIntConstant(m, "IPPROTO_RAW", IPPROTO_RAW);
#else
    PyModule_AddIntConstant(m, "IPPROTO_RAW", 255);
#endif
#ifdef  IPPROTO_MAX
    PyModule_AddIntConstant(m, "IPPROTO_MAX", IPPROTO_MAX);
#endif

#ifdef  SYSPROTO_CONTROL
    PyModule_AddIntConstant(m, "SYSPROTO_CONTROL", SYSPROTO_CONTROL);
#endif

    /* Some port configuration */
#ifdef  IPPORT_RESERVED
    PyModule_AddIntConstant(m, "IPPORT_RESERVED", IPPORT_RESERVED);
#else
    PyModule_AddIntConstant(m, "IPPORT_RESERVED", 1024);
#endif
#ifdef  IPPORT_USERRESERVED
    PyModule_AddIntConstant(m, "IPPORT_USERRESERVED", IPPORT_USERRESERVED);
#else
    PyModule_AddIntConstant(m, "IPPORT_USERRESERVED", 5000);
#endif

    /* Some reserved IP v.4 addresses */
#ifdef  INADDR_ANY
    PyModule_AddIntConstant(m, "INADDR_ANY", INADDR_ANY);
#else
    PyModule_AddIntConstant(m, "INADDR_ANY", 0x00000000);
#endif
#ifdef  INADDR_BROADCAST
    PyModule_AddIntConstant(m, "INADDR_BROADCAST", INADDR_BROADCAST);
#else
    PyModule_AddIntConstant(m, "INADDR_BROADCAST", 0xffffffff);
#endif
#ifdef  INADDR_LOOPBACK
    PyModule_AddIntConstant(m, "INADDR_LOOPBACK", INADDR_LOOPBACK);
#else
    PyModule_AddIntConstant(m, "INADDR_LOOPBACK", 0x7F000001);
#endif
#ifdef  INADDR_UNSPEC_GROUP
    PyModule_AddIntConstant(m, "INADDR_UNSPEC_GROUP", INADDR_UNSPEC_GROUP);
#else
    PyModule_AddIntConstant(m, "INADDR_UNSPEC_GROUP", 0xe0000000);
#endif
#ifdef  INADDR_ALLHOSTS_GROUP
    PyModule_AddIntConstant(m, "INADDR_ALLHOSTS_GROUP",
                            INADDR_ALLHOSTS_GROUP);
#else
    PyModule_AddIntConstant(m, "INADDR_ALLHOSTS_GROUP", 0xe0000001);
#endif
#ifdef  INADDR_MAX_LOCAL_GROUP
    PyModule_AddIntConstant(m, "INADDR_MAX_LOCAL_GROUP",
                            INADDR_MAX_LOCAL_GROUP);
#else
    PyModule_AddIntConstant(m, "INADDR_MAX_LOCAL_GROUP", 0xe00000ff);
#endif
#ifdef  INADDR_NONE
    PyModule_AddIntConstant(m, "INADDR_NONE", INADDR_NONE);
#else
    PyModule_AddIntConstant(m, "INADDR_NONE", 0xffffffff);
#endif

    /* IPv4 [gs]etsockopt options */
#ifdef  IP_OPTIONS
    PyModule_AddIntConstant(m, "IP_OPTIONS", IP_OPTIONS);
#endif
#ifdef  IP_HDRINCL
    PyModule_AddIntConstant(m, "IP_HDRINCL", IP_HDRINCL);
#endif
#ifdef  IP_TOS
    PyModule_AddIntConstant(m, "IP_TOS", IP_TOS);
#endif
#ifdef  IP_TTL
    PyModule_AddIntConstant(m, "IP_TTL", IP_TTL);
#endif
#ifdef  IP_RECVOPTS
    PyModule_AddIntConstant(m, "IP_RECVOPTS", IP_RECVOPTS);
#endif
#ifdef  IP_RECVRETOPTS
    PyModule_AddIntConstant(m, "IP_RECVRETOPTS", IP_RECVRETOPTS);
#endif
#ifdef  IP_RECVDSTADDR
    PyModule_AddIntConstant(m, "IP_RECVDSTADDR", IP_RECVDSTADDR);
#endif
#ifdef  IP_RETOPTS
    PyModule_AddIntConstant(m, "IP_RETOPTS", IP_RETOPTS);
#endif
#ifdef  IP_MULTICAST_IF
    PyModule_AddIntConstant(m, "IP_MULTICAST_IF", IP_MULTICAST_IF);
#endif
#ifdef  IP_MULTICAST_TTL
    PyModule_AddIntConstant(m, "IP_MULTICAST_TTL", IP_MULTICAST_TTL);
#endif
#ifdef  IP_MULTICAST_LOOP
    PyModule_AddIntConstant(m, "IP_MULTICAST_LOOP", IP_MULTICAST_LOOP);
#endif
#ifdef  IP_ADD_MEMBERSHIP
    PyModule_AddIntConstant(m, "IP_ADD_MEMBERSHIP", IP_ADD_MEMBERSHIP);
#endif
#ifdef  IP_DROP_MEMBERSHIP
    PyModule_AddIntConstant(m, "IP_DROP_MEMBERSHIP", IP_DROP_MEMBERSHIP);
#endif
#ifdef  IP_DEFAULT_MULTICAST_TTL
    PyModule_AddIntConstant(m, "IP_DEFAULT_MULTICAST_TTL",
                            IP_DEFAULT_MULTICAST_TTL);
#endif
#ifdef  IP_DEFAULT_MULTICAST_LOOP
    PyModule_AddIntConstant(m, "IP_DEFAULT_MULTICAST_LOOP",
                            IP_DEFAULT_MULTICAST_LOOP);
#endif
#ifdef  IP_MAX_MEMBERSHIPS
    PyModule_AddIntConstant(m, "IP_MAX_MEMBERSHIPS", IP_MAX_MEMBERSHIPS);
#endif
#ifdef  IP_TRANSPARENT
    PyModule_AddIntConstant(m, "IP_TRANSPARENT", IP_TRANSPARENT);
#endif

    /* IPv6 [gs]etsockopt options, defined in RFC2553 */
#ifdef  IPV6_JOIN_GROUP
    PyModule_AddIntConstant(m, "IPV6_JOIN_GROUP", IPV6_JOIN_GROUP);
#endif
#ifdef  IPV6_LEAVE_GROUP
    PyModule_AddIntConstant(m, "IPV6_LEAVE_GROUP", IPV6_LEAVE_GROUP);
#endif
#ifdef  IPV6_MULTICAST_HOPS
    PyModule_AddIntConstant(m, "IPV6_MULTICAST_HOPS", IPV6_MULTICAST_HOPS);
#endif
#ifdef  IPV6_MULTICAST_IF
    PyModule_AddIntConstant(m, "IPV6_MULTICAST_IF", IPV6_MULTICAST_IF);
#endif
#ifdef  IPV6_MULTICAST_LOOP
    PyModule_AddIntConstant(m, "IPV6_MULTICAST_LOOP", IPV6_MULTICAST_LOOP);
#endif
#ifdef  IPV6_UNICAST_HOPS
    PyModule_AddIntConstant(m, "IPV6_UNICAST_HOPS", IPV6_UNICAST_HOPS);
#endif
    /* Additional IPV6 socket options, defined in RFC 3493 */
#ifdef IPV6_V6ONLY
    PyModule_AddIntConstant(m, "IPV6_V6ONLY", IPV6_V6ONLY);
#endif
    /* Advanced IPV6 socket options, from RFC 3542 */
#ifdef IPV6_CHECKSUM
    PyModule_AddIntConstant(m, "IPV6_CHECKSUM", IPV6_CHECKSUM);
#endif
#ifdef IPV6_DONTFRAG
    PyModule_AddIntConstant(m, "IPV6_DONTFRAG", IPV6_DONTFRAG);
#endif
#ifdef IPV6_DSTOPTS
    PyModule_AddIntConstant(m, "IPV6_DSTOPTS", IPV6_DSTOPTS);
#endif
#ifdef IPV6_HOPLIMIT
    PyModule_AddIntConstant(m, "IPV6_HOPLIMIT", IPV6_HOPLIMIT);
#endif
#ifdef IPV6_HOPOPTS
    PyModule_AddIntConstant(m, "IPV6_HOPOPTS", IPV6_HOPOPTS);
#endif
#ifdef IPV6_NEXTHOP
    PyModule_AddIntConstant(m, "IPV6_NEXTHOP", IPV6_NEXTHOP);
#endif
#ifdef IPV6_PATHMTU
    PyModule_AddIntConstant(m, "IPV6_PATHMTU", IPV6_PATHMTU);
#endif
#ifdef IPV6_PKTINFO
    PyModule_AddIntConstant(m, "IPV6_PKTINFO", IPV6_PKTINFO);
#endif
#ifdef IPV6_RECVDSTOPTS
    PyModule_AddIntConstant(m, "IPV6_RECVDSTOPTS", IPV6_RECVDSTOPTS);
#endif
#ifdef IPV6_RECVHOPLIMIT
    PyModule_AddIntConstant(m, "IPV6_RECVHOPLIMIT", IPV6_RECVHOPLIMIT);
#endif
#ifdef IPV6_RECVHOPOPTS
    PyModule_AddIntConstant(m, "IPV6_RECVHOPOPTS", IPV6_RECVHOPOPTS);
#endif
#ifdef IPV6_RECVPKTINFO
    PyModule_AddIntConstant(m, "IPV6_RECVPKTINFO", IPV6_RECVPKTINFO);
#endif
#ifdef IPV6_RECVRTHDR
    PyModule_AddIntConstant(m, "IPV6_RECVRTHDR", IPV6_RECVRTHDR);
#endif
#ifdef IPV6_RECVTCLASS
    PyModule_AddIntConstant(m, "IPV6_RECVTCLASS", IPV6_RECVTCLASS);
#endif
#ifdef IPV6_RTHDR
    PyModule_AddIntConstant(m, "IPV6_RTHDR", IPV6_RTHDR);
#endif
#ifdef IPV6_RTHDRDSTOPTS
    PyModule_AddIntConstant(m, "IPV6_RTHDRDSTOPTS", IPV6_RTHDRDSTOPTS);
#endif
#ifdef IPV6_RTHDR_TYPE_0
    PyModule_AddIntConstant(m, "IPV6_RTHDR_TYPE_0", IPV6_RTHDR_TYPE_0);
#endif
#ifdef IPV6_RECVPATHMTU
    PyModule_AddIntConstant(m, "IPV6_RECVPATHMTU", IPV6_RECVPATHMTU);
#endif
#ifdef IPV6_TCLASS
    PyModule_AddIntConstant(m, "IPV6_TCLASS", IPV6_TCLASS);
#endif
#ifdef IPV6_USE_MIN_MTU
    PyModule_AddIntConstant(m, "IPV6_USE_MIN_MTU", IPV6_USE_MIN_MTU);
#endif

    /* TCP options */
#ifdef  TCP_NODELAY
    PyModule_AddIntConstant(m, "TCP_NODELAY", TCP_NODELAY);
#endif
#ifdef  TCP_MAXSEG
    PyModule_AddIntConstant(m, "TCP_MAXSEG", TCP_MAXSEG);
#endif
#ifdef  TCP_CORK
    PyModule_AddIntConstant(m, "TCP_CORK", TCP_CORK);
#endif
#ifdef  TCP_KEEPIDLE
    PyModule_AddIntConstant(m, "TCP_KEEPIDLE", TCP_KEEPIDLE);
#endif
#ifdef  TCP_KEEPINTVL
    PyModule_AddIntConstant(m, "TCP_KEEPINTVL", TCP_KEEPINTVL);
#endif
#ifdef  TCP_KEEPCNT
    PyModule_AddIntConstant(m, "TCP_KEEPCNT", TCP_KEEPCNT);
#endif
#ifdef  TCP_SYNCNT
    PyModule_AddIntConstant(m, "TCP_SYNCNT", TCP_SYNCNT);
#endif
#ifdef  TCP_LINGER2
    PyModule_AddIntConstant(m, "TCP_LINGER2", TCP_LINGER2);
#endif
#ifdef  TCP_DEFER_ACCEPT
    PyModule_AddIntConstant(m, "TCP_DEFER_ACCEPT", TCP_DEFER_ACCEPT);
#endif
#ifdef  TCP_WINDOW_CLAMP
    PyModule_AddIntConstant(m, "TCP_WINDOW_CLAMP", TCP_WINDOW_CLAMP);
#endif
#ifdef  TCP_INFO
    PyModule_AddIntConstant(m, "TCP_INFO", TCP_INFO);
#endif
#ifdef  TCP_QUICKACK
    PyModule_AddIntConstant(m, "TCP_QUICKACK", TCP_QUICKACK);
#endif


    /* IPX options */
#ifdef  IPX_TYPE
    PyModule_AddIntConstant(m, "IPX_TYPE", IPX_TYPE);
#endif

/* Reliable Datagram Sockets */
#ifdef RDS_CMSG_RDMA_ARGS
    PyModule_AddIntConstant(m, "RDS_CMSG_RDMA_ARGS", RDS_CMSG_RDMA_ARGS);
#endif
#ifdef RDS_CMSG_RDMA_DEST
    PyModule_AddIntConstant(m, "RDS_CMSG_RDMA_DEST", RDS_CMSG_RDMA_DEST);
#endif
#ifdef RDS_CMSG_RDMA_MAP
    PyModule_AddIntConstant(m, "RDS_CMSG_RDMA_MAP", RDS_CMSG_RDMA_MAP);
#endif
#ifdef RDS_CMSG_RDMA_STATUS
    PyModule_AddIntConstant(m, "RDS_CMSG_RDMA_STATUS", RDS_CMSG_RDMA_STATUS);
#endif
#ifdef RDS_CMSG_RDMA_UPDATE
    PyModule_AddIntConstant(m, "RDS_CMSG_RDMA_UPDATE", RDS_CMSG_RDMA_UPDATE);
#endif
#ifdef RDS_RDMA_READWRITE
    PyModule_AddIntConstant(m, "RDS_RDMA_READWRITE", RDS_RDMA_READWRITE);
#endif
#ifdef RDS_RDMA_FENCE
    PyModule_AddIntConstant(m, "RDS_RDMA_FENCE", RDS_RDMA_FENCE);
#endif
#ifdef RDS_RDMA_INVALIDATE
    PyModule_AddIntConstant(m, "RDS_RDMA_INVALIDATE", RDS_RDMA_INVALIDATE);
#endif
#ifdef RDS_RDMA_USE_ONCE
    PyModule_AddIntConstant(m, "RDS_RDMA_USE_ONCE", RDS_RDMA_USE_ONCE);
#endif
#ifdef RDS_RDMA_DONTWAIT
    PyModule_AddIntConstant(m, "RDS_RDMA_DONTWAIT", RDS_RDMA_DONTWAIT);
#endif
#ifdef RDS_RDMA_NOTIFY_ME
    PyModule_AddIntConstant(m, "RDS_RDMA_NOTIFY_ME", RDS_RDMA_NOTIFY_ME);
#endif
#ifdef RDS_RDMA_SILENT
    PyModule_AddIntConstant(m, "RDS_RDMA_SILENT", RDS_RDMA_SILENT);
#endif

    /* get{addr,name}info parameters */
#ifdef EAI_ADDRFAMILY
    PyModule_AddIntConstant(m, "EAI_ADDRFAMILY", EAI_ADDRFAMILY);
#endif
#ifdef EAI_AGAIN
    PyModule_AddIntConstant(m, "EAI_AGAIN", EAI_AGAIN);
#endif
#ifdef EAI_BADFLAGS
    PyModule_AddIntConstant(m, "EAI_BADFLAGS", EAI_BADFLAGS);
#endif
#ifdef EAI_FAIL
    PyModule_AddIntConstant(m, "EAI_FAIL", EAI_FAIL);
#endif
#ifdef EAI_FAMILY
    PyModule_AddIntConstant(m, "EAI_FAMILY", EAI_FAMILY);
#endif
#ifdef EAI_MEMORY
    PyModule_AddIntConstant(m, "EAI_MEMORY", EAI_MEMORY);
#endif
#ifdef EAI_NODATA
    PyModule_AddIntConstant(m, "EAI_NODATA", EAI_NODATA);
#endif
#ifdef EAI_NONAME
    PyModule_AddIntConstant(m, "EAI_NONAME", EAI_NONAME);
#endif
#ifdef EAI_OVERFLOW
    PyModule_AddIntConstant(m, "EAI_OVERFLOW", EAI_OVERFLOW);
#endif
#ifdef EAI_SERVICE
    PyModule_AddIntConstant(m, "EAI_SERVICE", EAI_SERVICE);
#endif
#ifdef EAI_SOCKTYPE
    PyModule_AddIntConstant(m, "EAI_SOCKTYPE", EAI_SOCKTYPE);
#endif
#ifdef EAI_SYSTEM
    PyModule_AddIntConstant(m, "EAI_SYSTEM", EAI_SYSTEM);
#endif
#ifdef EAI_BADHINTS
    PyModule_AddIntConstant(m, "EAI_BADHINTS", EAI_BADHINTS);
#endif
#ifdef EAI_PROTOCOL
    PyModule_AddIntConstant(m, "EAI_PROTOCOL", EAI_PROTOCOL);
#endif
#ifdef EAI_MAX
    PyModule_AddIntConstant(m, "EAI_MAX", EAI_MAX);
#endif
#ifdef AI_PASSIVE
    PyModule_AddIntConstant(m, "AI_PASSIVE", AI_PASSIVE);
#endif
#ifdef AI_CANONNAME
    PyModule_AddIntConstant(m, "AI_CANONNAME", AI_CANONNAME);
#endif
#ifdef AI_NUMERICHOST
    PyModule_AddIntConstant(m, "AI_NUMERICHOST", AI_NUMERICHOST);
#endif
#ifdef AI_NUMERICSERV
    PyModule_AddIntConstant(m, "AI_NUMERICSERV", AI_NUMERICSERV);
#endif
#ifdef AI_MASK
    PyModule_AddIntConstant(m, "AI_MASK", AI_MASK);
#endif
#ifdef AI_ALL
    PyModule_AddIntConstant(m, "AI_ALL", AI_ALL);
#endif
#ifdef AI_V4MAPPED_CFG
    PyModule_AddIntConstant(m, "AI_V4MAPPED_CFG", AI_V4MAPPED_CFG);
#endif
#ifdef AI_ADDRCONFIG
    PyModule_AddIntConstant(m, "AI_ADDRCONFIG", AI_ADDRCONFIG);
#endif
#ifdef AI_V4MAPPED
    PyModule_AddIntConstant(m, "AI_V4MAPPED", AI_V4MAPPED);
#endif
#ifdef AI_DEFAULT
    PyModule_AddIntConstant(m, "AI_DEFAULT", AI_DEFAULT);
#endif
#ifdef NI_MAXHOST
    PyModule_AddIntConstant(m, "NI_MAXHOST", NI_MAXHOST);
#endif
#ifdef NI_MAXSERV
    PyModule_AddIntConstant(m, "NI_MAXSERV", NI_MAXSERV);
#endif
#ifdef NI_NOFQDN
    PyModule_AddIntConstant(m, "NI_NOFQDN", NI_NOFQDN);
#endif
#ifdef NI_NUMERICHOST
    PyModule_AddIntConstant(m, "NI_NUMERICHOST", NI_NUMERICHOST);
#endif
#ifdef NI_NAMEREQD
    PyModule_AddIntConstant(m, "NI_NAMEREQD", NI_NAMEREQD);
#endif
#ifdef NI_NUMERICSERV
    PyModule_AddIntConstant(m, "NI_NUMERICSERV", NI_NUMERICSERV);
#endif
#ifdef NI_DGRAM
    PyModule_AddIntConstant(m, "NI_DGRAM", NI_DGRAM);
#endif

    /* shutdown() parameters */
#ifdef SHUT_RD
    PyModule_AddIntConstant(m, "SHUT_RD", SHUT_RD);
#elif defined(SD_RECEIVE)
    PyModule_AddIntConstant(m, "SHUT_RD", SD_RECEIVE);
#else
    PyModule_AddIntConstant(m, "SHUT_RD", 0);
#endif
#ifdef SHUT_WR
    PyModule_AddIntConstant(m, "SHUT_WR", SHUT_WR);
#elif defined(SD_SEND)
    PyModule_AddIntConstant(m, "SHUT_WR", SD_SEND);
#else
    PyModule_AddIntConstant(m, "SHUT_WR", 1);
#endif
#ifdef SHUT_RDWR
    PyModule_AddIntConstant(m, "SHUT_RDWR", SHUT_RDWR);
#elif defined(SD_BOTH)
    PyModule_AddIntConstant(m, "SHUT_RDWR", SD_BOTH);
#else
    PyModule_AddIntConstant(m, "SHUT_RDWR", 2);
#endif

#ifdef SIO_RCVALL
    {
        DWORD codes[] = {SIO_RCVALL, SIO_KEEPALIVE_VALS};
        const char *names[] = {"SIO_RCVALL", "SIO_KEEPALIVE_VALS"};
        int i;
        for(i = 0; i<Py_ARRAY_LENGTH(codes); ++i) {
            PyObject *tmp;
            tmp = PyLong_FromUnsignedLong(codes[i]);
            if (tmp == NULL)
                return NULL;
            PyModule_AddObject(m, names[i], tmp);
        }
    }
    PyModule_AddIntConstant(m, "RCVALL_OFF", RCVALL_OFF);
    PyModule_AddIntConstant(m, "RCVALL_ON", RCVALL_ON);
    PyModule_AddIntConstant(m, "RCVALL_SOCKETLEVELONLY", RCVALL_SOCKETLEVELONLY);
#ifdef RCVALL_IPLEVEL
    PyModule_AddIntConstant(m, "RCVALL_IPLEVEL", RCVALL_IPLEVEL);
#endif
#ifdef RCVALL_MAX
    PyModule_AddIntConstant(m, "RCVALL_MAX", RCVALL_MAX);
#endif
#endif /* _MSTCPIP_ */

    /* Initialize gethostbyname lock */
#if defined(USE_GETHOSTBYNAME_LOCK) || defined(USE_GETADDRINFO_LOCK)
    netdb_lock = PyThread_allocate_lock();
#endif
    return m;
}


#ifndef HAVE_INET_PTON
#if !defined(NTDDI_VERSION) || (NTDDI_VERSION < NTDDI_LONGHORN)

/* Simplistic emulation code for inet_pton that only works for IPv4 */
/* These are not exposed because they do not set errno properly */

int
inet_pton(int af, const char *src, void *dst)
{
    if (af == AF_INET) {
#if (SIZEOF_INT != 4)
#error "Not sure if in_addr_t exists and int is not 32-bits."
#endif
        unsigned int packed_addr;
        packed_addr = inet_addr(src);
        if (packed_addr == INADDR_NONE)
            return 0;
        memcpy(dst, &packed_addr, 4);
        return 1;
    }
    /* Should set errno to EAFNOSUPPORT */
    return -1;
}

const char *
inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    if (af == AF_INET) {
        struct in_addr packed_addr;
        if (size < 16)
            /* Should set errno to ENOSPC. */
            return NULL;
        memcpy(&packed_addr, src, sizeof(packed_addr));
        return strncpy(dst, inet_ntoa(packed_addr), size);
    }
    /* Should set errno to EAFNOSUPPORT */
    return NULL;
}

#endif
#endif
