/* Socket module */

/*

This module provides an interface to Berkeley socket IPC.

Limitations:

- Only AF_INET, AF_INET6 and AF_UNIX address families are supported in a
  portable manner, though AF_PACKET, AF_NETLINK, AF_QIPCRTR and AF_TIPC are
  supported under Linux.
- No read/write operations (use sendall/recv or makefile instead).
- Additional restrictions apply on some non-Unix platforms (compensated
  for by socket.py).

Module interface:

- socket.error: exception raised for socket specific errors, alias for OSError
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
- socket.getaddrinfo(host, port [, family, type, proto, flags])
    --> List of (family, type, proto, canonname, sockaddr)
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
- an internet socket address is a pair (hostname, port)
  where hostname can be anything recognized by gethostbyname()
  (including the dd.dd.dd.dd notation) and port is in host byte order
- where a hostname is returned, the dd.dd.dd.dd notation is used
- a UNIX domain socket address is a string specifying the pathname
- an AF_PACKET socket address is a tuple containing a string
  specifying the ethernet interface and an integer specifying
  the Ethernet protocol number to be received. For example:
  ("eth0",0x1234).  Optional 3rd,4th,5th elements in the tuple
  specify packet-type and ha-type/addr.
- an AF_QIPCRTR socket address is a (node, port) tuple where the
  node and port are non-negative integers.
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

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#ifdef __APPLE__
// Issue #35569: Expose RFC 3542 socket options.
#define __APPLE_USE_RFC_3542 1
#include <AvailabilityMacros.h>
/* for getaddrinfo thread safety test on old versions of OS X */
#ifndef MAC_OS_X_VERSION_10_5
#define MAC_OS_X_VERSION_10_5 1050
#endif
  /*
   * inet_aton is not available on OSX 10.3, yet we want to use a binary
   * that was build on 10.4 or later to work on that release, weak linking
   * comes to the rescue.
   */
# pragma weak inet_aton
#endif

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "pycore_fileutils.h"     // _Py_set_inheritable()
#include "pycore_moduleobject.h"  // _PyModule_GetState
#include "structmember.h"         // PyMemberDef

#ifdef _Py_MEMORY_SANITIZER
# include <sanitizer/msan_interface.h>
#endif

/* Socket object documentation */
PyDoc_STRVAR(sock_doc,
"socket(family=AF_INET, type=SOCK_STREAM, proto=0) -> socket object\n\
socket(family=-1, type=-1, proto=-1, fileno=None) -> socket object\n\
\n\
Open a socket of the given type.  The family argument specifies the\n\
address family; it defaults to AF_INET.  The type argument specifies\n\
whether this is a stream (SOCK_STREAM, this is the default)\n\
or datagram (SOCK_DGRAM) socket.  The protocol argument defaults to 0,\n\
specifying the default protocol.  Keyword arguments are accepted.\n\
The socket is created as non-inheritable.\n\
\n\
When a fileno is passed in, family, type and proto are auto-detected,\n\
unless they are explicitly set.\n\
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
dup() -- return a new socket fd duplicated from fileno()\n\
fileno() -- return underlying file descriptor\n\
getpeername() -- return remote address [*]\n\
getsockname() -- return local address\n\
getsockopt(level, optname[, buflen]) -- get socket options\n\
gettimeout() -- return timeout or None\n\
listen([n]) -- start listening for incoming connections\n\
recv(buflen[, flags]) -- receive data\n\
recv_into(buffer[, nbytes[, flags]]) -- receive data (into a buffer)\n\
recvfrom(buflen[, flags]) -- receive data and sender\'s address\n\
recvfrom_into(buffer[, nbytes, [, flags])\n\
  -- receive data and sender\'s address (into a buffer)\n\
sendall(data[, flags]) -- send all data\n\
send(data[, flags]) -- send data, may not send all of it\n\
sendto(data[, flags], addr) -- send data to a given address\n\
setblocking(bool) -- set or clear the blocking I/O flag\n\
getblocking() -- return True if socket is blocking, False if non-blocking\n\
setsockopt(level, optname, value[, optlen]) -- set socket options\n\
settimeout(None | float) -- set or clear the timeout\n\
shutdown(how) -- shut down traffic in one or both directions\n\
\n\
 [*] not available on all platforms!");

/* XXX This is a terrible mess of platform-dependent preprocessor hacks.
   I hope some day someone can clean this up please... */

/* Hacks for gethostbyname_r().  On some non-Linux platforms, the configure
   script doesn't get this right, so we hardcode some platform checks below.
   On the other hand, not all Linux versions agree, so there the settings
   computed by the configure script are needed! */

#ifndef __linux__
# undef HAVE_GETHOSTBYNAME_R_3_ARG
# undef HAVE_GETHOSTBYNAME_R_5_ARG
# undef HAVE_GETHOSTBYNAME_R_6_ARG
#endif

#if defined(__OpenBSD__)
# include <sys/uio.h>
#endif

#if defined(__ANDROID__) && __ANDROID_API__ < 23
# undef HAVE_GETHOSTBYNAME_R
#endif

#ifdef HAVE_GETHOSTBYNAME_R
# if defined(_AIX) && !defined(_LINUX_SOURCE_COMPAT)
#  define HAVE_GETHOSTBYNAME_R_3_ARG
# elif defined(__sun) || defined(__sgi)
#  define HAVE_GETHOSTBYNAME_R_5_ARG
# elif defined(__linux__)
/* Rely on the configure script */
# elif defined(_LINUX_SOURCE_COMPAT) /* Linux compatibility on AIX */
#  define HAVE_GETHOSTBYNAME_R_6_ARG
# else
#  undef HAVE_GETHOSTBYNAME_R
# endif
#endif

#if !defined(HAVE_GETHOSTBYNAME_R) && !defined(MS_WINDOWS)
# define USE_GETHOSTBYNAME_LOCK
#endif

#if defined(__APPLE__) || defined(__CYGWIN__) || defined(__NetBSD__)
#  include <sys/ioctl.h>
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
#endif // __sgi

/* Solaris fails to define this variable at all. */
#if (defined(__sun) && defined(__SVR4)) && !defined(INET_ADDRSTRLEN)
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

#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif

/* Generic socket object definitions and includes */
#define PySocket_BUILDING_SOCKET
#include "socketmodule.h"

/* Addressing includes */

#ifndef MS_WINDOWS

/* Non-MS WINDOWS includes */
#ifdef HAVE_NETDB_H
#  include <netdb.h>
#endif
# include <unistd.h>

/* Headers needed for inet_ntoa() and inet_addr() */
#   include <arpa/inet.h>

#  include <fcntl.h>

#else /* MS_WINDOWS */

/* MS_WINDOWS includes */
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif

/* Helpers needed for AF_HYPERV */
# include <Rpc.h>

/* Macros based on the IPPROTO enum, see: https://bugs.python.org/issue29515 */
#define IPPROTO_ICMP IPPROTO_ICMP
#define IPPROTO_IGMP IPPROTO_IGMP
#define IPPROTO_GGP IPPROTO_GGP
#define IPPROTO_TCP IPPROTO_TCP
#define IPPROTO_PUP IPPROTO_PUP
#define IPPROTO_UDP IPPROTO_UDP
#define IPPROTO_IDP IPPROTO_IDP
#define IPPROTO_ND IPPROTO_ND
#define IPPROTO_RAW IPPROTO_RAW
#define IPPROTO_MAX IPPROTO_MAX
#define IPPROTO_HOPOPTS IPPROTO_HOPOPTS
#define IPPROTO_IPV4 IPPROTO_IPV4
#define IPPROTO_IPV6 IPPROTO_IPV6
#define IPPROTO_ROUTING IPPROTO_ROUTING
#define IPPROTO_FRAGMENT IPPROTO_FRAGMENT
#define IPPROTO_ESP IPPROTO_ESP
#define IPPROTO_AH IPPROTO_AH
#define IPPROTO_ICMPV6 IPPROTO_ICMPV6
#define IPPROTO_NONE IPPROTO_NONE
#define IPPROTO_DSTOPTS IPPROTO_DSTOPTS
#define IPPROTO_EGP IPPROTO_EGP
#define IPPROTO_PIM IPPROTO_PIM
#define IPPROTO_ICLFXBM IPPROTO_ICLFXBM  // WinSock2 only
#define IPPROTO_ST IPPROTO_ST  // WinSock2 only
#define IPPROTO_CBT IPPROTO_CBT  // WinSock2 only
#define IPPROTO_IGP IPPROTO_IGP  // WinSock2 only
#define IPPROTO_RDP IPPROTO_RDP  // WinSock2 only
#define IPPROTO_PGM IPPROTO_PGM  // WinSock2 only
#define IPPROTO_L2TP IPPROTO_L2TP  // WinSock2 only
#define IPPROTO_SCTP IPPROTO_SCTP  // WinSock2 only

/* Provides the IsWindows7SP1OrGreater() function */
#include <versionhelpers.h>
// For if_nametoindex() and if_indextoname()
#include <iphlpapi.h>

/* remove some flags on older version Windows during run-time.
   https://msdn.microsoft.com/en-us/library/windows/desktop/ms738596.aspx */
typedef struct {
    DWORD build_number;  /* available starting with this Win10 BuildNumber */
    const char flag_name[20];
} FlagRuntimeInfo;

/* IMPORTANT: make sure the list ordered by descending build_number */
static FlagRuntimeInfo win_runtime_flags[] = {
    /* available starting with Windows 10 1709 */
    {16299, "TCP_KEEPIDLE"},
    {16299, "TCP_KEEPINTVL"},
    /* available starting with Windows 10 1703 */
    {15063, "TCP_KEEPCNT"},
    /* available starting with Windows 10 1607 */
    {14393, "TCP_FASTOPEN"}
};

/*[clinic input]
module _socket
class _socket.socket "PySocketSockObject *" "clinic_state()->sock_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2db2489bd2219fd8]*/

static int
remove_unusable_flags(PyObject *m)
{
    PyObject *dict;
    OSVERSIONINFOEX info;

    dict = PyModule_GetDict(m);
    if (dict == NULL) {
        return -1;
    }
#ifndef MS_WINDOWS_DESKTOP
    info.dwOSVersionInfoSize = sizeof(info);
    if (!GetVersionEx((OSVERSIONINFO*) &info)) {
        PyErr_SetFromWindowsErr(0);
        return -1;
    }
#else
    /* set to Windows 10, except BuildNumber. */
    memset(&info, 0, sizeof(info));
    info.dwOSVersionInfoSize = sizeof(info);
    info.dwMajorVersion = 10;
    info.dwMinorVersion = 0;

    /* set Condition Mask */
    DWORDLONG dwlConditionMask = 0;
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(dwlConditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
#endif

    for (int i=0; i<sizeof(win_runtime_flags)/sizeof(FlagRuntimeInfo); i++) {
#ifdef MS_WINDOWS_DESKTOP
        info.dwBuildNumber = win_runtime_flags[i].build_number;
        /* greater than or equal to the specified version?
           Compatibility Mode will not cheat VerifyVersionInfo(...) */
        BOOL isSupported = VerifyVersionInfo(
            &info,
            VER_MAJORVERSION|VER_MINORVERSION|VER_BUILDNUMBER,
            dwlConditionMask);
#else
        /* note in this case 'info' is the actual OS version, whereas above
           it is the version to compare against. */
        BOOL isSupported = info.dwMajorVersion > 10 ||
            (info.dwMajorVersion == 10 && info.dwMinorVersion > 0) ||
            (info.dwMajorVersion == 10 && info.dwMinorVersion == 0 &&
            info.dwBuildNumber >= win_runtime_flags[i].build_number);
#endif
        if (isSupported) {
            break;
        }
        else {
            PyObject *flag_name = PyUnicode_FromString(win_runtime_flags[i].flag_name);
            if (flag_name == NULL) {
                return -1;
            }
            PyObject *v = _PyDict_Pop(dict, flag_name, Py_None);
            Py_DECREF(flag_name);
            if (v == NULL) {
                return -1;
            }
            Py_DECREF(v);
        }
    }
    return 0;
}

#endif

#include <stddef.h>

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
/* This bug seems to be fixed in Jaguar. The easiest way I could
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
#endif // HAVE_GETNAMEINFO

#ifdef MS_WINDOWS
#define SOCKETCLOSE closesocket
#endif

#ifdef MS_WIN32
#  undef EAFNOSUPPORT
#  define EAFNOSUPPORT WSAEAFNOSUPPORT
#endif

#ifndef SOCKETCLOSE
#  define SOCKETCLOSE close
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

#ifdef MS_WINDOWS_DESKTOP
#define sockaddr_rc SOCKADDR_BTH_REDEF

#define USE_BLUETOOTH 1
#define AF_BLUETOOTH AF_BTH
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM
#define _BT_RC_MEMB(sa, memb) ((sa)->memb)
#endif /* MS_WINDOWS_DESKTOP */

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

#ifndef INADDR_NONE
#define INADDR_NONE (-1)
#endif

typedef struct _socket_state {
    /* The sock_type variable contains pointers to various functions,
       some of which call new_sockobject(), which uses sock_type, so
       there has to be a circular reference. */
    PyTypeObject *sock_type;

    /* Global variable holding the exception type for errors detected
       by this module (but not argument type or memory errors, etc.). */
    PyObject *socket_herror;
    PyObject *socket_gaierror;

    /* Default timeout for new sockets */
    _PyTime_t defaulttimeout;

#if defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4)
#if defined(HAVE_ACCEPT4) && defined(SOCK_CLOEXEC)
    /* accept4() is available on Linux 2.6.28+ and glibc 2.10 */
    int accept4_works;
#endif
#endif

#ifdef SOCK_CLOEXEC
    /* socket() and socketpair() fail with EINVAL on Linux kernel older
     * than 2.6.27 if SOCK_CLOEXEC flag is set in the socket type. */
    int sock_cloexec_works;
#endif
} socket_state;

static inline socket_state *
get_module_state(PyObject *mod)
{
    void *state = _PyModule_GetState(mod);
    assert(state != NULL);
    return (socket_state *)state;
}

static struct PyModuleDef socketmodule;

static inline socket_state *
find_module_state_by_def(PyTypeObject *type)
{
    PyObject *mod = PyType_GetModuleByDef(type, &socketmodule);
    assert(mod != NULL);
    return get_module_state(mod);
}

#define clinic_state() (find_module_state_by_def(type))
#include "clinic/socketmodule.c.h"
#undef clinic_state

/* XXX There's a problem here: *static* functions are not supposed to have
   a Py prefix (or use CapitalizedWords).  Later... */

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
#define IS_SELECTABLE(s) (_PyIsSelectable_fd((s)->sock_fd) || (s)->sock_timeout <= 0)
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

#ifdef MS_WINDOWS
#  define GET_SOCK_ERROR WSAGetLastError()
#  define SET_SOCK_ERROR(err) WSASetLastError(err)
#  define SOCK_TIMEOUT_ERR WSAEWOULDBLOCK
#  define SOCK_INPROGRESS_ERR WSAEWOULDBLOCK
#else
#  define GET_SOCK_ERROR errno
#  define SET_SOCK_ERROR(err) do { errno = err; } while (0)
#  define SOCK_TIMEOUT_ERR EWOULDBLOCK
#  define SOCK_INPROGRESS_ERR EINPROGRESS
#endif

#ifdef _MSC_VER
#  define SUPPRESS_DEPRECATED_CALL __pragma(warning(suppress: 4996))
#else
#  define SUPPRESS_DEPRECATED_CALL
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

    return PyErr_SetFromErrno(PyExc_OSError);
}


#if defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYNAME) || defined (HAVE_GETHOSTBYADDR)
static PyObject *
set_herror(socket_state *state, int h_error)
{
    PyObject *v;

#ifdef HAVE_HSTRERROR
    v = Py_BuildValue("(is)", h_error, hstrerror(h_error));
#else
    v = Py_BuildValue("(is)", h_error, "host not found");
#endif
    if (v != NULL) {
        PyErr_SetObject(state->socket_herror, v);
        Py_DECREF(v);
    }

    return NULL;
}
#endif


#ifdef HAVE_GETADDRINFO
static PyObject *
set_gaierror(socket_state *state, int error)
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
        PyErr_SetObject(state->socket_gaierror, v);
        Py_DECREF(v);
    }

    return NULL;
}
#endif

/* Function to perform the setting of socket blocking mode
   internally. block = (1 | 0). */
static int
internal_setblocking(PySocketSockObject *s, int block)
{
    int result = -1;
#ifdef MS_WINDOWS
    u_long arg;
#endif
#if !defined(MS_WINDOWS) \
    && !((defined(HAVE_SYS_IOCTL_H) && defined(FIONBIO)))
    int delay_flag, new_delay_flag;
#endif

    Py_BEGIN_ALLOW_THREADS
#ifndef MS_WINDOWS
#if (defined(HAVE_SYS_IOCTL_H) && defined(FIONBIO))
    block = !block;
    if (ioctl(s->sock_fd, FIONBIO, (unsigned int *)&block) == -1)
        goto done;
#else
    delay_flag = fcntl(s->sock_fd, F_GETFL, 0);
    if (delay_flag == -1)
        goto done;
    if (block)
        new_delay_flag = delay_flag & (~O_NONBLOCK);
    else
        new_delay_flag = delay_flag | O_NONBLOCK;
    if (new_delay_flag != delay_flag)
        if (fcntl(s->sock_fd, F_SETFL, new_delay_flag) == -1)
            goto done;
#endif
#else /* MS_WINDOWS */
    arg = !block;
    if (ioctlsocket(s->sock_fd, FIONBIO, &arg) != 0)
        goto done;
#endif /* MS_WINDOWS */

    result = 0;

  done:
    Py_END_ALLOW_THREADS

    if (result) {
#ifndef MS_WINDOWS
        PyErr_SetFromErrno(PyExc_OSError);
#else
        PyErr_SetExcFromWindowsErr(PyExc_OSError, WSAGetLastError());
#endif
    }

    return result;
}

static int
internal_select(PySocketSockObject *s, int writing, _PyTime_t interval,
                int connect)
{
    int n;
#ifdef HAVE_POLL
    struct pollfd pollfd;
    _PyTime_t ms;
#else
    fd_set fds, efds;
    struct timeval tv, *tvp;
#endif

    /* must be called with the GIL held */
    assert(PyGILState_Check());

    /* Error condition is for output only */
    assert(!(connect && !writing));

    /* Guard against closed socket */
    if (s->sock_fd == INVALID_SOCKET)
        return 0;

    /* Prefer poll, if available, since you can poll() any fd
     * which can't be done with select(). */
#ifdef HAVE_POLL
    pollfd.fd = s->sock_fd;
    pollfd.events = writing ? POLLOUT : POLLIN;
    if (connect) {
        /* On Windows, the socket becomes writable on connection success,
           but a connection failure is notified as an error. On POSIX, the
           socket becomes writable on connection success or on connection
           failure. */
        pollfd.events |= POLLERR;
    }

    /* s->sock_timeout is in seconds, timeout in ms */
    ms = _PyTime_AsMilliseconds(interval, _PyTime_ROUND_CEILING);
    assert(ms <= INT_MAX);

    /* On some OSes, typically BSD-based ones, the timeout parameter of the
       poll() syscall, when negative, must be exactly INFTIM, where defined,
       or -1. See issue 37811. */
    if (ms < 0) {
#ifdef INFTIM
        ms = INFTIM;
#else
        ms = -1;
#endif
    }

    Py_BEGIN_ALLOW_THREADS;
    n = poll(&pollfd, 1, (int)ms);
    Py_END_ALLOW_THREADS;
#else
    if (interval >= 0) {
        _PyTime_AsTimeval_clamp(interval, &tv, _PyTime_ROUND_CEILING);
        tvp = &tv;
    }
    else
        tvp = NULL;

    FD_ZERO(&fds);
    FD_SET(s->sock_fd, &fds);
    FD_ZERO(&efds);
    if (connect) {
        /* On Windows, the socket becomes writable on connection success,
           but a connection failure is notified as an error. On POSIX, the
           socket becomes writable on connection success or on connection
           failure. */
        FD_SET(s->sock_fd, &efds);
    }

    /* See if the socket is ready */
    Py_BEGIN_ALLOW_THREADS;
    if (writing)
        n = select(Py_SAFE_DOWNCAST(s->sock_fd+1, SOCKET_T, int),
                   NULL, &fds, &efds, tvp);
    else
        n = select(Py_SAFE_DOWNCAST(s->sock_fd+1, SOCKET_T, int),
                   &fds, NULL, &efds, tvp);
    Py_END_ALLOW_THREADS;
#endif

    if (n < 0)
        return -1;
    if (n == 0)
        return 1;
    return 0;
}

/* Call a socket function.

   On error, raise an exception and return -1 if err is set, or fill err and
   return -1 otherwise. If a signal was received and the signal handler raised
   an exception, return -1, and set err to -1 if err is set.

   On success, return 0, and set err to 0 if err is set.

   If the socket has a timeout, wait until the socket is ready before calling
   the function: wait until the socket is writable if writing is nonzero, wait
   until the socket received data otherwise.

   If the socket function is interrupted by a signal (failed with EINTR): retry
   the function, except if the signal handler raised an exception (PEP 475).

   When the function is retried, recompute the timeout using a monotonic clock.

   sock_call_ex() must be called with the GIL held. The socket function is
   called with the GIL released. */
static int
sock_call_ex(PySocketSockObject *s,
             int writing,
             int (*sock_func) (PySocketSockObject *s, void *data),
             void *data,
             int connect,
             int *err,
             _PyTime_t timeout)
{
    int has_timeout = (timeout > 0);
    _PyTime_t deadline = 0;
    int deadline_initialized = 0;
    int res;

    /* sock_call() must be called with the GIL held. */
    assert(PyGILState_Check());

    /* outer loop to retry select() when select() is interrupted by a signal
       or to retry select()+sock_func() on false positive (see above) */
    while (1) {
        /* For connect(), poll even for blocking socket. The connection
           runs asynchronously. */
        if (has_timeout || connect) {
            if (has_timeout) {
                _PyTime_t interval;

                if (deadline_initialized) {
                    /* recompute the timeout */
                    interval = _PyDeadline_Get(deadline);
                }
                else {
                    deadline_initialized = 1;
                    deadline = _PyDeadline_Init(timeout);
                    interval = timeout;
                }

                if (interval >= 0) {
                    res = internal_select(s, writing, interval, connect);
                }
                else {
                    res = 1;
                }
            }
            else {
                res = internal_select(s, writing, timeout, connect);
            }

            if (res == -1) {
                if (err)
                    *err = GET_SOCK_ERROR;

                if (CHECK_ERRNO(EINTR)) {
                    /* select() was interrupted by a signal */
                    if (PyErr_CheckSignals()) {
                        if (err)
                            *err = -1;
                        return -1;
                    }

                    /* retry select() */
                    continue;
                }

                /* select() failed */
                s->errorhandler();
                return -1;
            }

            if (res == 1) {
                if (err)
                    *err = SOCK_TIMEOUT_ERR;
                else
                    PyErr_SetString(PyExc_TimeoutError, "timed out");
                return -1;
            }

            /* the socket is ready */
        }

        /* inner loop to retry sock_func() when sock_func() is interrupted
           by a signal */
        while (1) {
            Py_BEGIN_ALLOW_THREADS
            res = sock_func(s, data);
            Py_END_ALLOW_THREADS

            if (res) {
                /* sock_func() succeeded */
                if (err)
                    *err = 0;
                return 0;
            }

            if (err)
                *err = GET_SOCK_ERROR;

            if (!CHECK_ERRNO(EINTR))
                break;

            /* sock_func() was interrupted by a signal */
            if (PyErr_CheckSignals()) {
                if (err)
                    *err = -1;
                return -1;
            }

            /* retry sock_func() */
        }

        if (s->sock_timeout > 0
            && (CHECK_ERRNO(EWOULDBLOCK) || CHECK_ERRNO(EAGAIN))) {
            /* False positive: sock_func() failed with EWOULDBLOCK or EAGAIN.

               For example, select() could indicate a socket is ready for
               reading, but the data then discarded by the OS because of a
               wrong checksum.

               Loop on select() to recheck for socket readiness. */
            continue;
        }

        /* sock_func() failed */
        if (!err)
            s->errorhandler();
        /* else: err was already set before */
        return -1;
    }
}

static int
sock_call(PySocketSockObject *s,
          int writing,
          int (*func) (PySocketSockObject *s, void *data),
          void *data)
{
    return sock_call_ex(s, writing, func, data, 0, NULL, s->sock_timeout);
}


/* Initialize a new socket object. */

static int
init_sockobject(socket_state *state, PySocketSockObject *s,
                SOCKET_T fd, int family, int type, int proto)
{
    s->sock_fd = fd;
    s->sock_family = family;

    s->sock_type = type;

    /* It's possible to pass SOCK_NONBLOCK and SOCK_CLOEXEC bit flags
       on some OSes as part of socket.type.  We want to reset them here,
       to make socket.type be set to the same value on all platforms.
       Otherwise, simple code like 'if sock.type == SOCK_STREAM' is
       not portable.
    */
#ifdef SOCK_NONBLOCK
    s->sock_type = s->sock_type & ~SOCK_NONBLOCK;
#endif
#ifdef SOCK_CLOEXEC
    s->sock_type = s->sock_type & ~SOCK_CLOEXEC;
#endif

    s->sock_proto = proto;

    s->errorhandler = &set_error;
#ifdef SOCK_NONBLOCK
    if (type & SOCK_NONBLOCK)
        s->sock_timeout = 0;
    else
#endif
    {
        s->sock_timeout = state->defaulttimeout;
        if (state->defaulttimeout >= 0) {
            if (internal_setblocking(s, 0) == -1) {
                return -1;
            }
        }
    }
    s->state = state;
    return 0;
}


#ifdef HAVE_SOCKETPAIR
/* Create a new socket object.
   This just creates the object and initializes it.
   If the creation fails, return NULL and set an exception (implicit
   in NEWOBJ()). */

static PySocketSockObject *
new_sockobject(socket_state *state, SOCKET_T fd, int family, int type,
               int proto)
{
    PyTypeObject *tp = state->sock_type;
    PySocketSockObject *s = (PySocketSockObject *)tp->tp_alloc(tp, 0);
    if (s == NULL) {
        return NULL;
    }
    if (init_sockobject(state, s, fd, family, type, proto) == -1) {
        Py_DECREF(s);
        return NULL;
    }
    return s;
}
#endif


/* Lock to allow python interpreter to continue, but only allow one
   thread to be in gethostbyname or getaddrinfo */
#if defined(USE_GETHOSTBYNAME_LOCK)
static PyThread_type_lock netdb_lock;
#endif


#ifdef HAVE_GETADDRINFO
/* Convert a string specifying a host name or one of a few symbolic
   names to a numeric IP address.  This usually calls gethostbyname()
   to do the work; the names "" and "<broadcast>" are special.
   Return the length (IPv4 should be 4 bytes), or negative if
   an error occurred; then an exception is raised. */

static int
setipaddr(socket_state *state, const char *name, struct sockaddr *addr_ret,
          size_t addr_ret_size, int af)
{
    struct addrinfo hints, *res;
    int error;

    memset((void *) addr_ret, '\0', sizeof(*addr_ret));
    if (name[0] == '\0') {
        int siz;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = af;
        hints.ai_socktype = SOCK_DGRAM;         /*dummy*/
        hints.ai_flags = AI_PASSIVE;
        Py_BEGIN_ALLOW_THREADS
        error = getaddrinfo(NULL, "0", &hints, &res);
        Py_END_ALLOW_THREADS
        /* We assume that those thread-unsafe getaddrinfo() versions
           *are* safe regarding their return value, ie. that a
           subsequent call to getaddrinfo() does not destroy the
           outcome of the first call. */
        if (error) {
            res = NULL;  // no-op, remind us that it is invalid; gh-100795
            set_gaierror(state, error);
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
    /* special-case broadcast - inet_addr() below can return INADDR_NONE for
     * this */
    if (strcmp(name, "255.255.255.255") == 0 ||
        strcmp(name, "<broadcast>") == 0) {
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

    /* avoid a name resolution in case of numeric address */
#ifdef HAVE_INET_PTON
    /* check for an IPv4 address */
    if (af == AF_UNSPEC || af == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *)addr_ret;
        memset(sin, 0, sizeof(*sin));
        if (inet_pton(AF_INET, name, &sin->sin_addr) > 0) {
            sin->sin_family = AF_INET;
#ifdef HAVE_SOCKADDR_SA_LEN
            sin->sin_len = sizeof(*sin);
#endif
            return 4;
        }
    }
#ifdef ENABLE_IPV6
    /* check for an IPv6 address - if the address contains a scope ID, we
     * fallback to getaddrinfo(), which can handle translation from interface
     * name to interface index */
    if ((af == AF_UNSPEC || af == AF_INET6) && !strchr(name, '%')) {
        struct sockaddr_in6 *sin = (struct sockaddr_in6 *)addr_ret;
        memset(sin, 0, sizeof(*sin));
        if (inet_pton(AF_INET6, name, &sin->sin6_addr) > 0) {
            sin->sin6_family = AF_INET6;
#ifdef HAVE_SOCKADDR_SA_LEN
            sin->sin6_len = sizeof(*sin);
#endif
            return 16;
        }
    }
#endif /* ENABLE_IPV6 */
#else /* HAVE_INET_PTON */
    /* check for an IPv4 address */
    if (af == AF_INET || af == AF_UNSPEC) {
        struct sockaddr_in *sin = (struct sockaddr_in *)addr_ret;
        memset(sin, 0, sizeof(*sin));
        if ((sin->sin_addr.s_addr = inet_addr(name)) != INADDR_NONE) {
            sin->sin_family = AF_INET;
#ifdef HAVE_SOCKADDR_SA_LEN
            sin->sin_len = sizeof(*sin);
#endif
            return 4;
        }
    }
#endif /* HAVE_INET_PTON */

    /* perform a name resolution */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    Py_BEGIN_ALLOW_THREADS
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
    if (error) {
        res = NULL;  // no-op, remind us that it is invalid; gh-100795
        set_gaierror(state, error);
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
#endif // HAVE_GETADDRINFO

/* Convert IPv4 sockaddr to a Python str. */

static PyObject *
make_ipv4_addr(const struct sockaddr_in *addr)
{
    char buf[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf)) == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return PyUnicode_FromString(buf);
}

#ifdef ENABLE_IPV6
/* Convert IPv6 sockaddr to a Python str. */

static PyObject *
make_ipv6_addr(const struct sockaddr_in6 *addr)
{
    char buf[INET6_ADDRSTRLEN];
    if (inet_ntop(AF_INET6, &addr->sin6_addr, buf, sizeof(buf)) == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return PyUnicode_FromString(buf);
}
#endif

#ifdef USE_BLUETOOTH
/* Convert a string representation of a Bluetooth address into a numeric
   address.  Returns the length (6), or raises an exception and returns -1 if
   an error occurred. */

static int
setbdaddr(const char *name, bdaddr_t *bdaddr)
{
    unsigned int b0, b1, b2, b3, b4, b5;
    char ch;
    int n;

    n = sscanf(name, "%X:%X:%X:%X:%X:%X%c",
               &b5, &b4, &b3, &b2, &b1, &b0, &ch);
    if (n == 6 && (b0 | b1 | b2 | b3 | b4 | b5) < 256) {

#ifdef MS_WINDOWS
        *bdaddr = (ULONGLONG)(b0 & 0xFF);
        *bdaddr |= ((ULONGLONG)(b1 & 0xFF) << 8);
        *bdaddr |= ((ULONGLONG)(b2 & 0xFF) << 16);
        *bdaddr |= ((ULONGLONG)(b3 & 0xFF) << 24);
        *bdaddr |= ((ULONGLONG)(b4 & 0xFF) << 32);
        *bdaddr |= ((ULONGLONG)(b5 & 0xFF) << 40);
#else
        bdaddr->b[0] = b0;
        bdaddr->b[1] = b1;
        bdaddr->b[2] = b2;
        bdaddr->b[3] = b3;
        bdaddr->b[4] = b4;
        bdaddr->b[5] = b5;
#endif

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
#ifdef MS_WINDOWS
    int i;
    unsigned int octets[6];

    for (i = 0; i < 6; ++i) {
        octets[i] = ((*bdaddr) >> (8 * i)) & 0xFF;
    }

    return PyUnicode_FromFormat("%02X:%02X:%02X:%02X:%02X:%02X",
        octets[5], octets[4], octets[3],
        octets[2], octets[1], octets[0]);
#else
    return PyUnicode_FromFormat("%02X:%02X:%02X:%02X:%02X:%02X",
        bdaddr->b[5], bdaddr->b[4], bdaddr->b[3],
        bdaddr->b[2], bdaddr->b[1], bdaddr->b[0]);
#endif
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
        Py_RETURN_NONE;
    }

    switch (addr->sa_family) {

    case AF_INET:
    {
        const struct sockaddr_in *a = (const struct sockaddr_in *)addr;
        PyObject *addrobj = make_ipv4_addr(a);
        PyObject *ret = NULL;
        if (addrobj) {
            ret = Py_BuildValue("Oi", addrobj, ntohs(a->sin_port));
            Py_DECREF(addrobj);
        }
        return ret;
    }

#if defined(AF_UNIX)
    case AF_UNIX:
    {
        struct sockaddr_un *a = (struct sockaddr_un *) addr;
#ifdef __linux__
        size_t linuxaddrlen = addrlen - offsetof(struct sockaddr_un, sun_path);
        if (linuxaddrlen > 0 && a->sun_path[0] == 0) {  /* Linux abstract namespace */
            return PyBytes_FromStringAndSize(a->sun_path, linuxaddrlen);
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

#if defined(AF_QIPCRTR)
       case AF_QIPCRTR:
       {
           struct sockaddr_qrtr *a = (struct sockaddr_qrtr *) addr;
           return Py_BuildValue("II", a->sq_node, a->sq_port);
       }
#endif /* AF_QIPCRTR */

#if defined(AF_VSOCK)
       case AF_VSOCK:
       {
           struct sockaddr_vm *a = (struct sockaddr_vm *) addr;
           return Py_BuildValue("II", a->svm_cid, a->svm_port);
       }
#endif /* AF_VSOCK */

#ifdef ENABLE_IPV6
    case AF_INET6:
    {
        const struct sockaddr_in6 *a = (const struct sockaddr_in6 *)addr;
        PyObject *addrobj = make_ipv6_addr(a);
        PyObject *ret = NULL;
        if (addrobj) {
            ret = Py_BuildValue("OiII",
                                addrobj,
                                ntohs(a->sin6_port),
                                ntohl(a->sin6_flowinfo),
                                a->sin6_scope_id);
            Py_DECREF(addrobj);
        }
        return ret;
    }
#endif /* ENABLE_IPV6 */

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
        switch (proto) {

#ifdef BTPROTO_L2CAP
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

#endif /* BTPROTO_L2CAP */

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

#ifdef BTPROTO_HCI
        case BTPROTO_HCI:
        {
            struct sockaddr_hci *a = (struct sockaddr_hci *) addr;
#if defined(__NetBSD__) || defined(__DragonFly__)
            return makebdaddr(&_BT_HCI_MEMB(a, bdaddr));
#else /* __NetBSD__ || __DragonFly__ */
            PyObject *ret = NULL;
            ret = Py_BuildValue("i", _BT_HCI_MEMB(a, dev));
            return ret;
#endif /* !(__NetBSD__ || __DragonFly__) */
        }

#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
        {
            struct sockaddr_sco *a = (struct sockaddr_sco *) addr;
            return makebdaddr(&_BT_SCO_MEMB(a, bdaddr));
        }
#endif /* !__FreeBSD__ */
#endif /* BTPROTO_HCI */

        default:
            PyErr_SetString(PyExc_ValueError,
                            "Unknown Bluetooth protocol");
            return NULL;
        }
#endif /* USE_BLUETOOTH */

#if defined(HAVE_NETPACKET_PACKET_H) && defined(SIOCGIFNAME)
    case AF_PACKET:
    {
        struct sockaddr_ll *a = (struct sockaddr_ll *)addr;
        const char *ifname = "";
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
                             (Py_ssize_t)a->sll_halen);
    }
#endif /* HAVE_NETPACKET_PACKET_H && SIOCGIFNAME */

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
#endif /* HAVE_LINUX_TIPC_H */

#if defined(AF_CAN) && defined(SIOCGIFNAME)
    case AF_CAN:
    {
        struct sockaddr_can *a = (struct sockaddr_can *)addr;
        const char *ifname = "";
        struct ifreq ifr;
        /* need to look up interface name given index */
        if (a->can_ifindex) {
            ifr.ifr_ifindex = a->can_ifindex;
            if (ioctl(sockfd, SIOCGIFNAME, &ifr) == 0)
                ifname = ifr.ifr_name;
        }

        switch (proto) {
#ifdef CAN_ISOTP
          case CAN_ISOTP:
          {
              return Py_BuildValue("O&kk", PyUnicode_DecodeFSDefault,
                                          ifname,
                                          a->can_addr.tp.rx_id,
                                          a->can_addr.tp.tx_id);
          }
#endif /* CAN_ISOTP */
#ifdef CAN_J1939
          case CAN_J1939:
          {
              return Py_BuildValue("O&KIB", PyUnicode_DecodeFSDefault,
                                          ifname,
                                          (unsigned long long)a->can_addr.j1939.name,
                                          (unsigned int)a->can_addr.j1939.pgn,
                                          a->can_addr.j1939.addr);
          }
#endif /* CAN_J1939 */
          default:
          {
              return Py_BuildValue("(O&)", PyUnicode_DecodeFSDefault,
                                        ifname);
          }
        }
    }
#endif /* AF_CAN && SIOCGIFNAME */

#ifdef PF_SYSTEM
    case PF_SYSTEM:
        switch(proto) {
#ifdef SYSPROTO_CONTROL
        case SYSPROTO_CONTROL:
        {
            struct sockaddr_ctl *a = (struct sockaddr_ctl *)addr;
            return Py_BuildValue("(II)", a->sc_id, a->sc_unit);
        }
#endif /* SYSPROTO_CONTROL */
        default:
            PyErr_SetString(PyExc_ValueError,
                            "Invalid address type");
            return 0;
        }
#endif /* PF_SYSTEM */

#ifdef HAVE_SOCKADDR_ALG
    case AF_ALG:
    {
        struct sockaddr_alg *a = (struct sockaddr_alg *)addr;
        return Py_BuildValue("s#s#HH",
            a->salg_type,
            strnlen((const char*)a->salg_type,
                    sizeof(a->salg_type)),
            a->salg_name,
            strnlen((const char*)a->salg_name,
                    sizeof(a->salg_name)),
            a->salg_feat,
            a->salg_mask);
    }
#endif /* HAVE_SOCKADDR_ALG */

#ifdef HAVE_AF_HYPERV
    case AF_HYPERV:
    {
        SOCKADDR_HV *a = (SOCKADDR_HV *) addr;

        wchar_t *guidStr;
        RPC_STATUS res = UuidToStringW(&a->VmId, &guidStr);
        if (res != RPC_S_OK) {
            PyErr_SetFromWindowsErr(res);
            return 0;
        }
        PyObject *vmId = PyUnicode_FromWideChar(guidStr, -1);
        res = RpcStringFreeW(&guidStr);
        assert(res == RPC_S_OK);

        res = UuidToStringW(&a->ServiceId, &guidStr);
        if (res != RPC_S_OK) {
            Py_DECREF(vmId);
            PyErr_SetFromWindowsErr(res);
            return 0;
        }
        PyObject *serviceId = PyUnicode_FromWideChar(guidStr, -1);
        res = RpcStringFreeW(&guidStr);
        assert(res == RPC_S_OK);

        return Py_BuildValue("NN", vmId, serviceId);
    }
#endif /* AF_HYPERV */

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

#if defined(HAVE_BIND) || defined(HAVE_CONNECTTO) || defined(CMSG_LEN)
/* Helper for getsockaddrarg: bypass IDNA for ASCII-only host names
   (in particular, numeric IP addresses). */
struct maybe_idna {
    PyObject *obj;
    char *buf;
};

static void
idna_cleanup(struct maybe_idna *data)
{
    Py_CLEAR(data->obj);
}

static int
idna_converter(PyObject *obj, struct maybe_idna *data)
{
    size_t len;
    PyObject *obj2;
    if (obj == NULL) {
        idna_cleanup(data);
        return 1;
    }
    data->obj = NULL;
    len = -1;
    if (PyBytes_Check(obj)) {
        data->buf = PyBytes_AsString(obj);
        len = PyBytes_Size(obj);
    }
    else if (PyByteArray_Check(obj)) {
        data->buf = PyByteArray_AsString(obj);
        len = PyByteArray_Size(obj);
    }
    else if (PyUnicode_Check(obj)) {
        if (PyUnicode_READY(obj) == -1) {
            return 0;
        }
        if (PyUnicode_IS_COMPACT_ASCII(obj)) {
            data->buf = PyUnicode_DATA(obj);
            len = PyUnicode_GET_LENGTH(obj);
        }
        else {
            obj2 = PyUnicode_AsEncodedString(obj, "idna", NULL);
            if (!obj2) {
                PyErr_SetString(PyExc_TypeError, "encoding of hostname failed");
                return 0;
            }
            assert(PyBytes_Check(obj2));
            data->obj = obj2;
            data->buf = PyBytes_AS_STRING(obj2);
            len = PyBytes_GET_SIZE(obj2);
        }
    }
    else {
        PyErr_Format(PyExc_TypeError, "str, bytes or bytearray expected, not %s",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }
    if (strlen(data->buf) != len) {
        Py_CLEAR(data->obj);
        PyErr_SetString(PyExc_TypeError, "host name must not contain null character");
        return 0;
    }
    return Py_CLEANUP_SUPPORTED;
}

/* Parse a socket address argument according to the socket object's
   address family.  Return 1 if the address was in the proper format,
   0 of not.  The address is returned through addr_ret, its length
   through len_ret. */

static int
getsockaddrarg(PySocketSockObject *s, PyObject *args,
               sock_addr_t *addrbuf, int *len_ret, const char *caller)
{
    switch (s->sock_family) {

#if defined(AF_UNIX)
    case AF_UNIX:
    {
        Py_buffer path;
        int retval = 0;

        /* PEP 383.  Not using PyUnicode_FSConverter since we need to
           allow embedded nulls on Linux. */
        if (PyUnicode_Check(args)) {
            if ((args = PyUnicode_EncodeFSDefault(args)) == NULL)
                return 0;
        }
        else
            Py_INCREF(args);
        if (!PyArg_Parse(args, "y*", &path)) {
            Py_DECREF(args);
            return retval;
        }
        assert(path.len >= 0);

        struct sockaddr_un* addr = &addrbuf->un;
#ifdef __linux__
        if (path.len == 0 || *(const char *)path.buf == 0) {
            /* Linux abstract namespace extension:
               - Empty address auto-binding to an abstract address
               - Address that starts with null byte */
            if ((size_t)path.len > sizeof addr->sun_path) {
                PyErr_SetString(PyExc_OSError,
                                "AF_UNIX path too long");
                goto unix_out;
            }

            *len_ret = path.len + offsetof(struct sockaddr_un, sun_path);
        }
        else
#endif /* linux */
        {
            /* regular NULL-terminated string */
            if ((size_t)path.len >= sizeof addr->sun_path) {
                PyErr_SetString(PyExc_OSError,
                                "AF_UNIX path too long");
                goto unix_out;
            }
            addr->sun_path[path.len] = 0;

            /* including the tailing NUL */
            *len_ret = path.len + offsetof(struct sockaddr_un, sun_path) + 1;
        }
        addr->sun_family = s->sock_family;
        memcpy(addr->sun_path, path.buf, path.len);

        retval = 1;
    unix_out:
        PyBuffer_Release(&path);
        Py_DECREF(args);
        return retval;
    }
#endif /* AF_UNIX */

#if defined(AF_NETLINK)
    case AF_NETLINK:
    {
        int pid, groups;
        struct sockaddr_nl* addr = &addrbuf->nl;
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "%s(): AF_NETLINK address must be tuple, not %.500s",
                caller, Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args,
                              "II;AF_NETLINK address must be a pair "
                              "(pid, groups)",
                              &pid, &groups))
        {
            return 0;
        }
        addr->nl_family = AF_NETLINK;
        addr->nl_pid = pid;
        addr->nl_groups = groups;
        *len_ret = sizeof(*addr);
        return 1;
    }
#endif /* AF_NETLINK */

#if defined(AF_QIPCRTR)
    case AF_QIPCRTR:
    {
        unsigned int node, port;
        struct sockaddr_qrtr* addr = &addrbuf->sq;
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_QIPCRTR address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args, "II:getsockaddrarg", &node, &port))
            return 0;
        addr->sq_family = AF_QIPCRTR;
        addr->sq_node = node;
        addr->sq_port = port;
        *len_ret = sizeof(*addr);
        return 1;
    }
#endif /* AF_QIPCRTR */

#if defined(AF_VSOCK)
    case AF_VSOCK:
    {
        struct sockaddr_vm* addr = &addrbuf->vm;
        int port, cid;
        memset(addr, 0, sizeof(struct sockaddr_vm));
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "getsockaddrarg: "
                "AF_VSOCK address must be tuple, not %.500s",
                Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args, "II:getsockaddrarg", &cid, &port))
            return 0;
        addr->svm_family = s->sock_family;
        addr->svm_port = port;
        addr->svm_cid = cid;
        *len_ret = sizeof(*addr);
        return 1;
    }
#endif /* AF_VSOCK */


#ifdef AF_RDS
    case AF_RDS:
        /* RDS sockets use sockaddr_in: fall-through */
#endif /* AF_RDS */

#ifdef AF_DIVERT
    case AF_DIVERT:
        /* FreeBSD divert(4) sockets use sockaddr_in: fall-through */
#endif /* AF_DIVERT */

    case AF_INET:
    {
        struct maybe_idna host = {NULL, NULL};
        int port, result;
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "%s(): AF_INET address must be tuple, not %.500s",
                caller, Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args,
                              "O&i;AF_INET address must be a pair "
                              "(host, port)",
                              idna_converter, &host, &port))
        {
            assert(PyErr_Occurred());
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                PyErr_Format(PyExc_OverflowError,
                             "%s(): port must be 0-65535.", caller);
            }
            return 0;
        }
        struct sockaddr_in* addr = &addrbuf->in;
        result = setipaddr(s->state, host.buf, (struct sockaddr *)addr,
                           sizeof(*addr),  AF_INET);
        idna_cleanup(&host);
        if (result < 0)
            return 0;
        if (port < 0 || port > 0xffff) {
            PyErr_Format(
                PyExc_OverflowError,
                "%s(): port must be 0-65535.", caller);
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
        struct maybe_idna host = {NULL, NULL};
        int port, result;
        unsigned int flowinfo, scope_id;
        flowinfo = scope_id = 0;
        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "%s(): AF_INET6 address must be tuple, not %.500s",
                caller, Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args,
                              "O&i|II;AF_INET6 address must be a tuple "
                              "(host, port[, flowinfo[, scopeid]])",
                              idna_converter, &host, &port, &flowinfo,
                              &scope_id))
        {
            assert(PyErr_Occurred());
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                PyErr_Format(PyExc_OverflowError,
                             "%s(): port must be 0-65535.", caller);
            }
            return 0;
        }
        struct sockaddr_in6* addr = &addrbuf->in6;
        result = setipaddr(s->state, host.buf, (struct sockaddr *)addr,
                           sizeof(*addr), AF_INET6);
        idna_cleanup(&host);
        if (result < 0)
            return 0;
        if (port < 0 || port > 0xffff) {
            PyErr_Format(
                PyExc_OverflowError,
                "%s(): port must be 0-65535.", caller);
            return 0;
        }
        if (flowinfo > 0xfffff) {
            PyErr_Format(
                PyExc_OverflowError,
                "%s(): flowinfo must be 0-1048575.", caller);
            return 0;
        }
        addr->sin6_family = s->sock_family;
        addr->sin6_port = htons((short)port);
        addr->sin6_flowinfo = htonl(flowinfo);
        addr->sin6_scope_id = scope_id;
        *len_ret = sizeof *addr;
        return 1;
    }
#endif /* ENABLE_IPV6 */

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
    {
        switch (s->sock_proto) {
#ifdef BTPROTO_L2CAP
        case BTPROTO_L2CAP:
        {
            const char *straddr;

            struct sockaddr_l2 *addr = &addrbuf->bt_l2;
            memset(addr, 0, sizeof(struct sockaddr_l2));
            _BT_L2_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "si", &straddr,
                                  &_BT_L2_MEMB(addr, psm))) {
                PyErr_Format(PyExc_OSError,
                             "%s(): wrong format", caller);
                return 0;
            }
            if (setbdaddr(straddr, &_BT_L2_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
#endif /* BTPROTO_L2CAP */
        case BTPROTO_RFCOMM:
        {
            const char *straddr;
            struct sockaddr_rc *addr = &addrbuf->bt_rc;
            _BT_RC_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "si", &straddr,
                                  &_BT_RC_MEMB(addr, channel))) {
                PyErr_Format(PyExc_OSError,
                             "%s(): wrong format", caller);
                return 0;
            }
            if (setbdaddr(straddr, &_BT_RC_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
#ifdef BTPROTO_HCI
        case BTPROTO_HCI:
        {
            struct sockaddr_hci *addr = &addrbuf->bt_hci;
#if defined(__NetBSD__) || defined(__DragonFly__)
            const char *straddr;
            _BT_HCI_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyBytes_Check(args)) {
                PyErr_Format(PyExc_OSError, "%s: "
                             "wrong format", caller);
                return 0;
            }
            straddr = PyBytes_AS_STRING(args);
            if (setbdaddr(straddr, &_BT_HCI_MEMB(addr, bdaddr)) < 0)
                return 0;
#else  /* __NetBSD__ || __DragonFly__ */
            _BT_HCI_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyArg_ParseTuple(args, "i", &_BT_HCI_MEMB(addr, dev))) {
                PyErr_Format(PyExc_OSError,
                             "%s(): wrong format", caller);
                return 0;
            }
#endif /* !(__NetBSD__ || __DragonFly__) */
            *len_ret = sizeof *addr;
            return 1;
        }
#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
        {
            const char *straddr;

            struct sockaddr_sco *addr = &addrbuf->bt_sco;
            _BT_SCO_MEMB(addr, family) = AF_BLUETOOTH;
            if (!PyBytes_Check(args)) {
                PyErr_Format(PyExc_OSError,
                             "%s(): wrong format", caller);
                return 0;
            }
            straddr = PyBytes_AS_STRING(args);
            if (setbdaddr(straddr, &_BT_SCO_MEMB(addr, bdaddr)) < 0)
                return 0;

            *len_ret = sizeof *addr;
            return 1;
        }
#endif /* !__FreeBSD__ */
#endif /* BTPROTO_HCI */
        default:
            PyErr_Format(PyExc_OSError,
                         "%s(): unknown Bluetooth protocol", caller);
            return 0;
        }
    }
#endif /* USE_BLUETOOTH */

#if defined(HAVE_NETPACKET_PACKET_H) && defined(SIOCGIFINDEX)
    case AF_PACKET:
    {
        struct ifreq ifr;
        const char *interfaceName;
        int protoNumber;
        int hatype = 0;
        int pkttype = PACKET_HOST;
        Py_buffer haddr = {NULL, NULL};

        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "%s(): AF_PACKET address must be tuple, not %.500s",
                caller, Py_TYPE(args)->tp_name);
            return 0;
        }
        /* XXX: improve the default error message according to the
           documentation of AF_PACKET, which would be added as part
           of bpo-25041. */
        if (!PyArg_ParseTuple(args,
                              "si|iiy*;AF_PACKET address must be a tuple of "
                              "two to five elements",
                              &interfaceName, &protoNumber, &pkttype, &hatype,
                              &haddr))
        {
            assert(PyErr_Occurred());
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                PyErr_Format(PyExc_OverflowError,
                             "%s(): address argument out of range", caller);
            }
            return 0;
        }
        strncpy(ifr.ifr_name, interfaceName, sizeof(ifr.ifr_name));
        ifr.ifr_name[(sizeof(ifr.ifr_name))-1] = '\0';
        if (ioctl(s->sock_fd, SIOCGIFINDEX, &ifr) < 0) {
            s->errorhandler();
            PyBuffer_Release(&haddr);
            return 0;
        }
        if (haddr.buf && haddr.len > 8) {
            PyErr_SetString(PyExc_ValueError,
                            "Hardware address must be 8 bytes or less");
            PyBuffer_Release(&haddr);
            return 0;
        }
        if (protoNumber < 0 || protoNumber > 0xffff) {
            PyErr_Format(
                PyExc_OverflowError,
                "%s(): proto must be 0-65535.", caller);
            PyBuffer_Release(&haddr);
            return 0;
        }
        struct sockaddr_ll* addr = &addrbuf->ll;
        addr->sll_family = AF_PACKET;
        addr->sll_protocol = htons((short)protoNumber);
        addr->sll_ifindex = ifr.ifr_ifindex;
        addr->sll_pkttype = pkttype;
        addr->sll_hatype = hatype;
        if (haddr.buf) {
            memcpy(&addr->sll_addr, haddr.buf, haddr.len);
            addr->sll_halen = haddr.len;
        }
        else
            addr->sll_halen = 0;
        *len_ret = sizeof *addr;
        PyBuffer_Release(&haddr);
        return 1;
    }
#endif /* HAVE_NETPACKET_PACKET_H && SIOCGIFINDEX */

#ifdef HAVE_LINUX_TIPC_H
    case AF_TIPC:
    {
        unsigned int atype, v1, v2, v3;
        unsigned int scope = TIPC_CLUSTER_SCOPE;

        if (!PyTuple_Check(args)) {
            PyErr_Format(
                PyExc_TypeError,
                "%s(): AF_TIPC address must be tuple, not %.500s",
                caller, Py_TYPE(args)->tp_name);
            return 0;
        }

        if (!PyArg_ParseTuple(args,
                              "IIII|I;AF_TIPC address must be a tuple "
                              "(addr_type, v1, v2, v3[, scope])",
                              &atype, &v1, &v2, &v3, &scope))
        {
            return 0;
        }

        struct sockaddr_tipc *addr = &addrbuf->tipc;
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
#endif /* HAVE_LINUX_TIPC_H */

#if defined(AF_CAN) && defined(SIOCGIFINDEX)
    case AF_CAN:
        switch (s->sock_proto) {
#ifdef CAN_RAW
        case CAN_RAW:
        /* fall-through */
#endif
#ifdef CAN_BCM
        case CAN_BCM:
#endif
#if defined(CAN_RAW) || defined(CAN_BCM)
        {
            PyObject *interfaceName;
            struct ifreq ifr;
            Py_ssize_t len;
            struct sockaddr_can *addr = &addrbuf->can;

            if (!PyTuple_Check(args)) {
                PyErr_Format(PyExc_TypeError,
                             "%s(): AF_CAN address must be tuple, not %.500s",
                             caller, Py_TYPE(args)->tp_name);
                return 0;
            }
            if (!PyArg_ParseTuple(args,
                                  "O&;AF_CAN address must be a tuple "
                                  "(interface, )",
                                  PyUnicode_FSConverter, &interfaceName))
            {
                return 0;
            }

            len = PyBytes_GET_SIZE(interfaceName);

            if (len == 0) {
                ifr.ifr_ifindex = 0;
            } else if ((size_t)len < sizeof(ifr.ifr_name)) {
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
#endif /* CAN_RAW || CAN_BCM */

#ifdef CAN_ISOTP
        case CAN_ISOTP:
        {
            PyObject *interfaceName;
            struct ifreq ifr;
            Py_ssize_t len;
            unsigned long int rx_id, tx_id;

            struct sockaddr_can *addr = &addrbuf->can;

            if (!PyArg_ParseTuple(args, "O&kk", PyUnicode_FSConverter,
                                              &interfaceName,
                                              &rx_id,
                                              &tx_id))
                return 0;

            len = PyBytes_GET_SIZE(interfaceName);

            if (len == 0) {
                ifr.ifr_ifindex = 0;
            } else if ((size_t)len < sizeof(ifr.ifr_name)) {
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
            addr->can_addr.tp.rx_id = rx_id;
            addr->can_addr.tp.tx_id = tx_id;

            *len_ret = sizeof(*addr);
            Py_DECREF(interfaceName);
            return 1;
        }
#endif /* CAN_ISOTP */
#ifdef CAN_J1939
        case CAN_J1939:
        {
            PyObject *interfaceName;
            struct ifreq ifr;
            Py_ssize_t len;
            unsigned long long j1939_name; /* at least 64 bits */
            unsigned int j1939_pgn; /* at least 32 bits */
            uint8_t j1939_addr;

            struct sockaddr_can *addr = &addrbuf->can;

            if (!PyArg_ParseTuple(args, "O&KIB", PyUnicode_FSConverter,
                                              &interfaceName,
                                              &j1939_name,
                                              &j1939_pgn,
                                              &j1939_addr))
                return 0;

            len = PyBytes_GET_SIZE(interfaceName);

            if (len == 0) {
                ifr.ifr_ifindex = 0;
            } else if ((size_t)len < sizeof(ifr.ifr_name)) {
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
            addr->can_addr.j1939.name = (uint64_t)j1939_name;
            addr->can_addr.j1939.pgn = (uint32_t)j1939_pgn;
            addr->can_addr.j1939.addr = j1939_addr;

            *len_ret = sizeof(*addr);
            Py_DECREF(interfaceName);
            return 1;
        }
#endif /* CAN_J1939 */
        default:
            PyErr_Format(PyExc_OSError,
                         "%s(): unsupported CAN protocol", caller);
            return 0;
        }
#endif /* AF_CAN && SIOCGIFINDEX */

#ifdef PF_SYSTEM
    case PF_SYSTEM:
        switch (s->sock_proto) {
#ifdef SYSPROTO_CONTROL
        case SYSPROTO_CONTROL:
        {
            struct sockaddr_ctl *addr = &addrbuf->ctl;
            addr->sc_family = AF_SYSTEM;
            addr->ss_sysaddr = AF_SYS_CONTROL;

            if (PyUnicode_Check(args)) {
                struct ctl_info info;
                PyObject *ctl_name;

                if (!PyArg_Parse(args, "O&",
                                PyUnicode_FSConverter, &ctl_name)) {
                    return 0;
                }

                if (PyBytes_GET_SIZE(ctl_name) > (Py_ssize_t)sizeof(info.ctl_name)) {
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
                PyErr_Format(PyExc_TypeError,
                             "%s(): PF_SYSTEM address must be a str or "
                             "a pair (id, unit)", caller);
                return 0;
            }

            *len_ret = sizeof(*addr);
            return 1;
        }
#endif /* SYSPROTO_CONTROL */
        default:
            PyErr_Format(PyExc_OSError,
                         "%s(): unsupported PF_SYSTEM protocol", caller);
            return 0;
        }
#endif /* PF_SYSTEM */
#ifdef HAVE_SOCKADDR_ALG
    case AF_ALG:
    {
        const char *type;
        const char *name;
        struct sockaddr_alg *sa = &addrbuf->alg;

        memset(sa, 0, sizeof(*sa));
        sa->salg_family = AF_ALG;

        if (!PyTuple_Check(args)) {
            PyErr_Format(PyExc_TypeError,
                         "%s(): AF_ALG address must be tuple, not %.500s",
                         caller, Py_TYPE(args)->tp_name);
            return 0;
        }
        if (!PyArg_ParseTuple(args,
                              "ss|HH;AF_ALG address must be a tuple "
                              "(type, name[, feat[, mask]])",
                              &type, &name, &sa->salg_feat, &sa->salg_mask))
        {
            return 0;
        }
        /* sockaddr_alg has fixed-sized char arrays for type, and name
         * both must be NULL terminated.
         */
        if (strlen(type) >= sizeof(sa->salg_type)) {
            PyErr_SetString(PyExc_ValueError, "AF_ALG type too long.");
            return 0;
        }
        strncpy((char *)sa->salg_type, type, sizeof(sa->salg_type));
        if (strlen(name) >= sizeof(sa->salg_name)) {
            PyErr_SetString(PyExc_ValueError, "AF_ALG name too long.");
            return 0;
        }
        strncpy((char *)sa->salg_name, name, sizeof(sa->salg_name));

        *len_ret = sizeof(*sa);
        return 1;
    }
#endif /* HAVE_SOCKADDR_ALG */
#ifdef HAVE_AF_HYPERV
    case AF_HYPERV:
    {
        switch (s->sock_proto) {
        case HV_PROTOCOL_RAW:
        {
            PyObject *vm_id_obj = NULL;
            PyObject *service_id_obj = NULL;

            SOCKADDR_HV *addr = &addrbuf->hv;

            memset(addr, 0, sizeof(*addr));
            addr->Family = AF_HYPERV;

            if (!PyTuple_Check(args)) {
                PyErr_Format(PyExc_TypeError,
                    "%s(): AF_HYPERV address must be tuple, not %.500s",
                    caller, Py_TYPE(args)->tp_name);
                return 0;
            }
            if (!PyArg_ParseTuple(args,
                "UU;AF_HYPERV address must be a str tuple (vm_id, service_id)",
                &vm_id_obj, &service_id_obj))
            {
                return 0;
            }

            wchar_t *guid_str = PyUnicode_AsWideCharString(vm_id_obj, NULL);
            if (guid_str == NULL) {
                PyErr_Format(PyExc_ValueError,
                    "%s(): AF_HYPERV address vm_id is not a valid UUID string",
                    caller);
                return 0;
            }
            RPC_STATUS rc = UuidFromStringW(guid_str, &addr->VmId);
            PyMem_Free(guid_str);
            if (rc != RPC_S_OK) {
                PyErr_Format(PyExc_ValueError,
                    "%s(): AF_HYPERV address vm_id is not a valid UUID string",
                    caller);
                return 0;
            }

            guid_str = PyUnicode_AsWideCharString(service_id_obj, NULL);
            if (guid_str == NULL) {
                PyErr_Format(PyExc_ValueError,
                    "%s(): AF_HYPERV address service_id is not a valid UUID string",
                    caller);
                return 0;
            }
            rc = UuidFromStringW(guid_str, &addr->ServiceId);
            PyMem_Free(guid_str);
            if (rc != RPC_S_OK) {
                PyErr_Format(PyExc_ValueError,
                    "%s(): AF_HYPERV address service_id is not a valid UUID string",
                    caller);
                return 0;
            }

            *len_ret = sizeof(*addr);
            return 1;
        }
        default:
            PyErr_Format(PyExc_OSError,
                "%s(): unsupported AF_HYPERV protocol: %d",
                caller, s->sock_proto);
            return 0;
        }
    }
#endif /* HAVE_AF_HYPERV */

    /* More cases here... */

    default:
        PyErr_Format(PyExc_OSError, "%s(): bad family", caller);
        return 0;

    }
}
#endif // defined(HAVE_BIND) || defined(HAVE_CONNECTTO) || defined(CMSG_LEN)


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
#endif /* AF_NETLINK */

#if defined(AF_QIPCRTR)
    case AF_QIPCRTR:
    {
        *len_ret = sizeof (struct sockaddr_qrtr);
        return 1;
    }
#endif /* AF_QIPCRTR */

#if defined(AF_VSOCK)
       case AF_VSOCK:
       {
           *len_ret = sizeof (struct sockaddr_vm);
           return 1;
       }
#endif /* AF_VSOCK */

#ifdef AF_RDS
    case AF_RDS:
        /* RDS sockets use sockaddr_in: fall-through */
#endif /* AF_RDS */

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
#endif /* ENABLE_IPV6 */

#ifdef USE_BLUETOOTH
    case AF_BLUETOOTH:
    {
        switch(s->sock_proto)
        {

#ifdef BTPROTO_L2CAP
        case BTPROTO_L2CAP:
            *len_ret = sizeof (struct sockaddr_l2);
            return 1;
#endif /* BTPROTO_L2CAP */
        case BTPROTO_RFCOMM:
            *len_ret = sizeof (struct sockaddr_rc);
            return 1;
#ifdef BTPROTO_HCI
        case BTPROTO_HCI:
            *len_ret = sizeof (struct sockaddr_hci);
            return 1;
#if !defined(__FreeBSD__)
        case BTPROTO_SCO:
            *len_ret = sizeof (struct sockaddr_sco);
            return 1;
#endif /* !__FreeBSD__ */
#endif /* BTPROTO_HCI */
        default:
            PyErr_SetString(PyExc_OSError, "getsockaddrlen: "
                            "unknown BT protocol");
            return 0;

        }
    }
#endif /* USE_BLUETOOTH */

#ifdef HAVE_NETPACKET_PACKET_H
    case AF_PACKET:
    {
        *len_ret = sizeof (struct sockaddr_ll);
        return 1;
    }
#endif /* HAVE_NETPACKET_PACKET_H */

#ifdef HAVE_LINUX_TIPC_H
    case AF_TIPC:
    {
        *len_ret = sizeof (struct sockaddr_tipc);
        return 1;
    }
#endif /* HAVE_LINUX_TIPC_H */

#ifdef AF_CAN
    case AF_CAN:
    {
        *len_ret = sizeof (struct sockaddr_can);
        return 1;
    }
#endif /* AF_CAN */

#ifdef PF_SYSTEM
    case PF_SYSTEM:
        switch(s->sock_proto) {
#ifdef SYSPROTO_CONTROL
        case SYSPROTO_CONTROL:
            *len_ret = sizeof (struct sockaddr_ctl);
            return 1;
#endif /* SYSPROTO_CONTROL */
        default:
            PyErr_SetString(PyExc_OSError, "getsockaddrlen: "
                            "unknown PF_SYSTEM protocol");
            return 0;
        }
#endif /* PF_SYSTEM */
#ifdef HAVE_SOCKADDR_ALG
    case AF_ALG:
    {
        *len_ret = sizeof (struct sockaddr_alg);
        return 1;
    }
#endif /* HAVE_SOCKADDR_ALG */
#ifdef HAVE_AF_HYPERV
    case AF_HYPERV:
    {
        *len_ret = sizeof (SOCKADDR_HV);
        return 1;
    }
#endif /* HAVE_AF_HYPERV */

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
    if (cmsgh == NULL || msg->msg_control == NULL)
        return 0;
    /* Note that POSIX allows msg_controllen to be of a signed type. This is
       annoying under OS X as it's unsigned there and so it triggers a
       tautological comparison warning under Clang when compared against 0.
       Since the check is valid on other platforms, silence the warning under
       Clang. */
    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wtautological-compare"
    #endif
    #if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5)))
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
    #endif
    if (msg->msg_controllen < 0)
        return 0;
    #if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5)))
    #pragma GCC diagnostic pop
    #endif
    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif
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


struct sock_accept {
    socklen_t *addrlen;
    sock_addr_t *addrbuf;
    SOCKET_T result;
};

#if defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4)

static int
sock_accept_impl(PySocketSockObject *s, void *data)
{
    struct sock_accept *ctx = data;
    struct sockaddr *addr = SAS2SA(ctx->addrbuf);
    socklen_t *paddrlen = ctx->addrlen;
#ifdef HAVE_SOCKADDR_ALG
    /* AF_ALG does not support accept() with addr and raises
     * ECONNABORTED instead. */
    if (s->sock_family == AF_ALG) {
        addr = NULL;
        paddrlen = NULL;
        *ctx->addrlen = 0;
    }
#endif

#if defined(HAVE_ACCEPT4) && defined(SOCK_CLOEXEC)
    socket_state *state = s->state;
    if (state->accept4_works != 0) {
        ctx->result = accept4(s->sock_fd, addr, paddrlen,
                              SOCK_CLOEXEC);
        if (ctx->result == INVALID_SOCKET && state->accept4_works == -1) {
            /* On Linux older than 2.6.28, accept4() fails with ENOSYS */
            state->accept4_works = (errno != ENOSYS);
        }
    }
    if (state->accept4_works == 0)
        ctx->result = accept(s->sock_fd, addr, paddrlen);
#else
    ctx->result = accept(s->sock_fd, addr, paddrlen);
#endif

#ifdef MS_WINDOWS
    return (ctx->result != INVALID_SOCKET);
#else
    return (ctx->result >= 0);
#endif
}

/* s._accept() -> (fd, address) */

static PyObject *
sock_accept(PySocketSockObject *s, PyObject *Py_UNUSED(ignored))
{
    sock_addr_t addrbuf;
    SOCKET_T newfd;
    socklen_t addrlen;
    PyObject *sock = NULL;
    PyObject *addr = NULL;
    PyObject *res = NULL;
    struct sock_accept ctx;

    if (!getsockaddrlen(s, &addrlen))
        return NULL;
    memset(&addrbuf, 0, addrlen);

    if (!IS_SELECTABLE(s))
        return select_error();

    ctx.addrlen = &addrlen;
    ctx.addrbuf = &addrbuf;
    if (sock_call(s, 0, sock_accept_impl, &ctx) < 0)
        return NULL;
    newfd = ctx.result;

#ifdef MS_WINDOWS
#if defined(MS_WINDOWS_APP) || defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)
#ifndef HANDLE_FLAG_INHERIT
#define HANDLE_FLAG_INHERIT 0x00000001
#endif
    if (!SetHandleInformation((HANDLE)newfd, HANDLE_FLAG_INHERIT, 0)) {
        PyErr_SetFromWindowsErr(0);
        SOCKETCLOSE(newfd);
        goto finally;
    }
#endif
#else

#if defined(HAVE_ACCEPT4) && defined(SOCK_CLOEXEC)
    socket_state *state = s->state;
    if (!state->accept4_works)
#endif
    {
        if (_Py_set_inheritable(newfd, 0, NULL) < 0) {
            SOCKETCLOSE(newfd);
            goto finally;
        }
    }
#endif

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
#endif // defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4)


/* s.setblocking(flag) method.  Argument:
   False -- non-blocking mode; same as settimeout(0)
   True -- blocking mode; same as settimeout(None)
*/

static PyObject *
sock_setblocking(PySocketSockObject *s, PyObject *arg)
{
    long block;

    block = PyObject_IsTrue(arg);
    if (block < 0)
        return NULL;

    s->sock_timeout = _PyTime_FromSeconds(block ? -1 : 0);
    if (internal_setblocking(s, block) == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(setblocking_doc,
"setblocking(flag)\n\
\n\
Set the socket to blocking (flag is true) or non-blocking (false).\n\
setblocking(True) is equivalent to settimeout(None);\n\
setblocking(False) is equivalent to settimeout(0.0).");

/* s.getblocking() method.
   Returns True if socket is in blocking mode,
   False if it is in non-blocking mode.
*/
static PyObject *
sock_getblocking(PySocketSockObject *s, PyObject *Py_UNUSED(ignored))
{
    if (s->sock_timeout) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

PyDoc_STRVAR(getblocking_doc,
"getblocking()\n\
\n\
Returns True if socket is in blocking mode, or False if it\n\
is in non-blocking mode.");

static int
socket_parse_timeout(_PyTime_t *timeout, PyObject *timeout_obj)
{
#ifdef MS_WINDOWS
    struct timeval tv;
#endif
#ifndef HAVE_POLL
    _PyTime_t ms;
#endif
    int overflow = 0;

    if (timeout_obj == Py_None) {
        *timeout = _PyTime_FromSeconds(-1);
        return 0;
    }

    if (_PyTime_FromSecondsObject(timeout,
                                  timeout_obj, _PyTime_ROUND_TIMEOUT) < 0)
        return -1;

    if (*timeout < 0) {
        PyErr_SetString(PyExc_ValueError, "Timeout value out of range");
        return -1;
    }

#ifdef MS_WINDOWS
    overflow |= (_PyTime_AsTimeval(*timeout, &tv, _PyTime_ROUND_TIMEOUT) < 0);
#endif
#ifndef HAVE_POLL
    ms = _PyTime_AsMilliseconds(*timeout, _PyTime_ROUND_TIMEOUT);
    overflow |= (ms > INT_MAX);
#endif
    if (overflow) {
        PyErr_SetString(PyExc_OverflowError,
                        "timeout doesn't fit into C timeval");
        return -1;
    }

    return 0;
}

/* s.settimeout(timeout) method.  Argument:
   None -- no timeout, blocking mode; same as setblocking(True)
   0.0  -- non-blocking mode; same as setblocking(False)
   > 0  -- timeout mode; operations time out after timeout seconds
   < 0  -- illegal; raises an exception
*/
static PyObject *
sock_settimeout(PySocketSockObject *s, PyObject *arg)
{
    _PyTime_t timeout;

    if (socket_parse_timeout(&timeout, arg) < 0)
        return NULL;

    s->sock_timeout = timeout;

    int block = timeout < 0;
    /* Blocking mode for a Python socket object means that operations
       like :meth:`recv` or :meth:`sendall` will block the execution of
       the current thread until they are complete or aborted with a
       `TimeoutError` or `socket.error` errors.  When timeout is `None`,
       the underlying FD is in a blocking mode.  When timeout is a positive
       number, the FD is in a non-blocking mode, and socket ops are
       implemented with a `select()` call.

       When timeout is 0.0, the FD is in a non-blocking mode.

       This table summarizes all states in which the socket object and
       its underlying FD can be:

       ==================== ===================== ==============
        `gettimeout()`       `getblocking()`       FD
       ==================== ===================== ==============
        ``None``             ``True``              blocking
        ``0.0``              ``False``             non-blocking
        ``> 0``              ``True``              non-blocking
    */

    if (internal_setblocking(s, block) == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
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
sock_gettimeout(PySocketSockObject *s, PyObject *Py_UNUSED(ignored))
{
    if (s->sock_timeout < 0) {
        Py_RETURN_NONE;
    }
    else {
        double seconds = _PyTime_AsSecondsDouble(s->sock_timeout);
        return PyFloat_FromDouble(seconds);
    }
}

PyDoc_STRVAR(gettimeout_doc,
"gettimeout() -> timeout\n\
\n\
Returns the timeout in seconds (float) associated with socket\n\
operations. A timeout of None indicates that timeouts on socket\n\
operations are disabled.");

#ifdef HAVE_SETSOCKOPT
/* s.setsockopt() method.
   With an integer third argument, sets an integer optval with optlen=4.
   With None as third argument and an integer fourth argument, set
   optval=NULL with unsigned int as optlen.
   With a string third argument, sets an option from a buffer;
   use optional built-in module 'struct' to encode the string.
*/

static PyObject *
sock_setsockopt(PySocketSockObject *s, PyObject *args)
{
    int level;
    int optname;
    int res;
    Py_buffer optval;
    int flag;
    unsigned int optlen;
    PyObject *none;

#ifdef AF_VSOCK
    if (s->sock_family == AF_VSOCK) {
        uint64_t vflag; // Must be set width of 64 bits
        /* setsockopt(level, opt, flag) */
        if (PyArg_ParseTuple(args, "iiK:setsockopt",
                         &level, &optname, &vflag)) {
            // level should always be set to AF_VSOCK
            res = setsockopt(s->sock_fd, level, optname,
                         (void*)&vflag, sizeof vflag);
            goto done;
        }
        return NULL;
    }
#endif

    /* setsockopt(level, opt, flag) */
    if (PyArg_ParseTuple(args, "iii:setsockopt",
                         &level, &optname, &flag)) {
        res = setsockopt(s->sock_fd, level, optname,
                         (char*)&flag, sizeof flag);
        goto done;
    }

    PyErr_Clear();
    /* setsockopt(level, opt, None, flag) */
    if (PyArg_ParseTuple(args, "iiO!I:setsockopt",
                         &level, &optname, Py_TYPE(Py_None), &none, &optlen)) {
        assert(sizeof(socklen_t) >= sizeof(unsigned int));
        res = setsockopt(s->sock_fd, level, optname,
                         NULL, (socklen_t)optlen);
        goto done;
    }

    PyErr_Clear();
    /* setsockopt(level, opt, buffer) */
    if (!PyArg_ParseTuple(args, "iiy*:setsockopt",
                            &level, &optname, &optval))
        return NULL;

#ifdef MS_WINDOWS
    if (optval.len > INT_MAX) {
        PyBuffer_Release(&optval);
        PyErr_Format(PyExc_OverflowError,
                        "socket option is larger than %i bytes",
                        INT_MAX);
        return NULL;
    }
    res = setsockopt(s->sock_fd, level, optname,
                        optval.buf, (int)optval.len);
#else
    res = setsockopt(s->sock_fd, level, optname, optval.buf, optval.len);
#endif
    PyBuffer_Release(&optval);

done:
    if (res < 0) {
        return s->errorhandler();
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(setsockopt_doc,
"setsockopt(level, option, value: int)\n\
setsockopt(level, option, value: buffer)\n\
setsockopt(level, option, None, optlen: int)\n\
\n\
Set a socket option.  See the Unix manual for level and option.\n\
The value argument can either be an integer, a string buffer, or\n\
None, optlen.");
#endif

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
    int flag = 0;
    socklen_t flagsize;

    if (!PyArg_ParseTuple(args, "ii|i:getsockopt",
                          &level, &optname, &buflen))
        return NULL;

    if (buflen == 0) {
#ifdef AF_VSOCK
        if (s->sock_family == AF_VSOCK) {
            uint64_t vflag = 0; // Must be set width of 64 bits
            flagsize = sizeof vflag;
            res = getsockopt(s->sock_fd, level, optname,
                         (void *)&vflag, &flagsize);
            if (res < 0)
                return s->errorhandler();
            return PyLong_FromUnsignedLong(vflag);
        }
#endif
        flagsize = sizeof flag;
        res = getsockopt(s->sock_fd, level, optname,
                         (void *)&flag, &flagsize);
        if (res < 0)
            return s->errorhandler();
        return PyLong_FromLong(flag);
    }
#ifdef AF_VSOCK
    if (s->sock_family == AF_VSOCK) {
        PyErr_SetString(PyExc_OSError,
                        "getsockopt string buffer not allowed");
        return NULL;
        }
#endif
    if (buflen <= 0 || buflen > 1024) {
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


#ifdef HAVE_BIND
/* s.bind(sockaddr) method */

static PyObject *
sock_bind(PySocketSockObject *s, PyObject *addro)
{
    sock_addr_t addrbuf;
    int addrlen;
    int res;

    if (!getsockaddrarg(s, addro, &addrbuf, &addrlen, "bind")) {
        return NULL;
    }

    if (PySys_Audit("socket.bind", "OO", s, addro) < 0) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    res = bind(s->sock_fd, SAS2SA(&addrbuf), addrlen);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return s->errorhandler();
    Py_RETURN_NONE;
}

PyDoc_STRVAR(bind_doc,
"bind(address)\n\
\n\
Bind the socket to a local address.  For IP sockets, the address is a\n\
pair (host, port); the host must refer to the local host. For raw packet\n\
sockets the address is a tuple (ifname, proto [,pkttype [,hatype [,addr]]])");
#endif


/* s.close() method.
   Set the file descriptor to -1 so operations tried subsequently
   will surely fail. */

static PyObject *
sock_close(PySocketSockObject *s, PyObject *Py_UNUSED(ignored))
{
    SOCKET_T fd;
    int res;

    fd = s->sock_fd;
    if (fd != INVALID_SOCKET) {
        s->sock_fd = INVALID_SOCKET;

        /* We do not want to retry upon EINTR: see
           http://lwn.net/Articles/576478/ and
           http://linux.derkeiler.com/Mailing-Lists/Kernel/2005-09/3000.html
           for more details. */
        Py_BEGIN_ALLOW_THREADS
        res = SOCKETCLOSE(fd);
        Py_END_ALLOW_THREADS
        /* bpo-30319: The peer can already have closed the connection.
           Python ignores ECONNRESET on close(). */
        if (res < 0 && errno != ECONNRESET) {
            return s->errorhandler();
        }
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(sock_close_doc,
"close()\n\
\n\
Close the socket.  It cannot be used after this call.");

static PyObject *
sock_detach(PySocketSockObject *s, PyObject *Py_UNUSED(ignored))
{
    SOCKET_T fd = s->sock_fd;
    s->sock_fd = INVALID_SOCKET;
    return PyLong_FromSocket_t(fd);
}

PyDoc_STRVAR(detach_doc,
"detach()\n\
\n\
Close the socket object without closing the underlying file descriptor.\n\
The object cannot be used after this call, but the file descriptor\n\
can be reused for other purposes.  The file descriptor is returned.");

#ifdef HAVE_CONNECT
static int
sock_connect_impl(PySocketSockObject *s, void* Py_UNUSED(data))
{
    int err;
    socklen_t size = sizeof err;

    if (getsockopt(s->sock_fd, SOL_SOCKET, SO_ERROR, (void *)&err, &size)) {
        /* getsockopt() failed */
        return 0;
    }

    if (err == EISCONN)
        return 1;
    if (err != 0) {
        /* sock_call_ex() uses GET_SOCK_ERROR() to get the error code */
        SET_SOCK_ERROR(err);
        return 0;
    }
    return 1;
}

static int
internal_connect(PySocketSockObject *s, struct sockaddr *addr, int addrlen,
                 int raise)
{
    int res, err, wait_connect;

    Py_BEGIN_ALLOW_THREADS
    res = connect(s->sock_fd, addr, addrlen);
    Py_END_ALLOW_THREADS

    if (!res) {
        /* connect() succeeded, the socket is connected */
        return 0;
    }

    /* connect() failed */

    /* save error, PyErr_CheckSignals() can replace it */
    err = GET_SOCK_ERROR;
    if (CHECK_ERRNO(EINTR)) {
        if (PyErr_CheckSignals())
            return -1;

        /* Issue #23618: when connect() fails with EINTR, the connection is
           running asynchronously.

           If the socket is blocking or has a timeout, wait until the
           connection completes, fails or timed out using select(), and then
           get the connection status using getsockopt(SO_ERROR).

           If the socket is non-blocking, raise InterruptedError. The caller is
           responsible to wait until the connection completes, fails or timed
           out (it's the case in asyncio for example). */
        wait_connect = (s->sock_timeout != 0 && IS_SELECTABLE(s));
    }
    else {
        wait_connect = (s->sock_timeout > 0 && err == SOCK_INPROGRESS_ERR
                        && IS_SELECTABLE(s));
    }

    if (!wait_connect) {
        if (raise) {
            /* restore error, maybe replaced by PyErr_CheckSignals() */
            SET_SOCK_ERROR(err);
            s->errorhandler();
            return -1;
        }
        else
            return err;
    }

    if (raise) {
        /* socket.connect() raises an exception on error */
        if (sock_call_ex(s, 1, sock_connect_impl, NULL,
                         1, NULL, s->sock_timeout) < 0)
            return -1;
    }
    else {
        /* socket.connect_ex() returns the error code on error */
        if (sock_call_ex(s, 1, sock_connect_impl, NULL,
                         1, &err, s->sock_timeout) < 0)
            return err;
    }
    return 0;
}

/* s.connect(sockaddr) method */

static PyObject *
sock_connect(PySocketSockObject *s, PyObject *addro)
{
    sock_addr_t addrbuf;
    int addrlen;
    int res;

    if (!getsockaddrarg(s, addro, &addrbuf, &addrlen, "connect")) {
        return NULL;
    }

    if (PySys_Audit("socket.connect", "OO", s, addro) < 0) {
        return NULL;
    }

    res = internal_connect(s, SAS2SA(&addrbuf), addrlen, 1);
    if (res < 0)
        return NULL;

    Py_RETURN_NONE;
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

    if (!getsockaddrarg(s, addro, &addrbuf, &addrlen, "connect_ex")) {
        return NULL;
    }

    if (PySys_Audit("socket.connect", "OO", s, addro) < 0) {
        return NULL;
    }

    res = internal_connect(s, SAS2SA(&addrbuf), addrlen, 0);
    if (res < 0)
        return NULL;

    return PyLong_FromLong((long) res);
}

PyDoc_STRVAR(connect_ex_doc,
"connect_ex(address) -> errno\n\
\n\
This is like connect(address), but returns an error code (the errno value)\n\
instead of raising an exception when an error occurs.");
#endif // HAVE_CONNECT


/* s.fileno() method */

static PyObject *
sock_fileno(PySocketSockObject *s, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromSocket_t(s->sock_fd);
}

PyDoc_STRVAR(fileno_doc,
"fileno() -> integer\n\
\n\
Return the integer file descriptor of the socket.");


#ifdef HAVE_GETSOCKNAME
/* s.getsockname() method */

static PyObject *
sock_getsockname(PySocketSockObject *s, PyObject *Py_UNUSED(ignored))
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
Return the address of the local endpoint. The format depends on the\n\
address family. For IPv4 sockets, the address info is a pair\n\
(hostaddr, port). For IPv6 sockets, the address info is a 4-tuple\n\
(hostaddr, port, flowinfo, scope_id).");
#endif


#ifdef HAVE_GETPEERNAME         /* Cray APP doesn't have this :-( */
/* s.getpeername() method */

static PyObject *
sock_getpeername(PySocketSockObject *s, PyObject *Py_UNUSED(ignored))
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


#ifdef HAVE_LISTEN
/* s.listen(n) method */

static PyObject *
sock_listen(PySocketSockObject *s, PyObject *args)
{
    /* We try to choose a default backlog high enough to avoid connection drops
     * for common workloads, yet not too high to limit resource usage. */
    int backlog = Py_MIN(SOMAXCONN, 128);
    int res;

    if (!PyArg_ParseTuple(args, "|i:listen", &backlog))
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
    Py_RETURN_NONE;
}

PyDoc_STRVAR(listen_doc,
"listen([backlog])\n\
\n\
Enable a server to accept connections.  If backlog is specified, it must be\n\
at least 0 (if it is lower, it is set to 0); it specifies the number of\n\
unaccepted connections that the system will allow before refusing new\n\
connections. If not specified, a default reasonable value is chosen.");
#endif

struct sock_recv {
    char *cbuf;
    Py_ssize_t len;
    int flags;
    Py_ssize_t result;
};

static int
sock_recv_impl(PySocketSockObject *s, void *data)
{
    struct sock_recv *ctx = data;

#ifdef MS_WINDOWS
    if (ctx->len > INT_MAX)
        ctx->len = INT_MAX;
    ctx->result = recv(s->sock_fd, ctx->cbuf, (int)ctx->len, ctx->flags);
#else
    ctx->result = recv(s->sock_fd, ctx->cbuf, ctx->len, ctx->flags);
#endif
    return (ctx->result >= 0);
}


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
    struct sock_recv ctx;

    if (!IS_SELECTABLE(s)) {
        select_error();
        return -1;
    }
    if (len == 0) {
        /* If 0 bytes were requested, do nothing. */
        return 0;
    }

    ctx.cbuf = cbuf;
    ctx.len = len;
    ctx.flags = flags;
    if (sock_call(s, 0, sock_recv_impl, &ctx) < 0)
        return -1;

    return ctx.result;
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
A version of recv() that stores its data into a buffer rather than creating\n\
a new string.  Receive up to buffersize bytes from the socket.  If buffersize\n\
is not specified (or 0), receive up to the size available in the given buffer.\n\
\n\
See recv() for documentation about the flags.");

struct sock_recvfrom {
    char* cbuf;
    Py_ssize_t len;
    int flags;
    socklen_t *addrlen;
    sock_addr_t *addrbuf;
    Py_ssize_t result;
};

#ifdef HAVE_RECVFROM
static int
sock_recvfrom_impl(PySocketSockObject *s, void *data)
{
    struct sock_recvfrom *ctx = data;

    memset(ctx->addrbuf, 0, *ctx->addrlen);

#ifdef MS_WINDOWS
    if (ctx->len > INT_MAX)
        ctx->len = INT_MAX;
    ctx->result = recvfrom(s->sock_fd, ctx->cbuf, (int)ctx->len, ctx->flags,
                           SAS2SA(ctx->addrbuf), ctx->addrlen);
#else
    ctx->result = recvfrom(s->sock_fd, ctx->cbuf, ctx->len, ctx->flags,
                           SAS2SA(ctx->addrbuf), ctx->addrlen);
#endif
    return (ctx->result >= 0);
}


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
    socklen_t addrlen;
    struct sock_recvfrom ctx;

    *addr = NULL;

    if (!getsockaddrlen(s, &addrlen))
        return -1;

    if (!IS_SELECTABLE(s)) {
        select_error();
        return -1;
    }

    ctx.cbuf = cbuf;
    ctx.len = len;
    ctx.flags = flags;
    ctx.addrbuf = &addrbuf;
    ctx.addrlen = &addrlen;
    if (sock_call(s, 0, sock_recvfrom_impl, &ctx) < 0)
        return -1;

    *addr = makesockaddr(s->sock_fd, SAS2SA(&addrbuf), addrlen,
                         s->sock_proto);
    if (*addr == NULL)
        return -1;

    return ctx.result;
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

    if (recvlen < 0) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_ValueError,
                        "negative buffersize in recvfrom_into");
        return NULL;
    }
    if (recvlen == 0) {
        /* If nbytes was not specified, use the buffer's length */
        recvlen = buflen;
    } else if (recvlen > buflen) {
        PyBuffer_Release(&pbuf);
        PyErr_SetString(PyExc_ValueError,
                        "nbytes is greater than the length of the buffer");
        return NULL;
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
#endif

/* The sendmsg() and recvmsg[_into]() methods require a working
   CMSG_LEN().  See the comment near get_CMSG_LEN(). */
#ifdef CMSG_LEN
struct sock_recvmsg {
    struct msghdr *msg;
    int flags;
    ssize_t result;
};

static int
sock_recvmsg_impl(PySocketSockObject *s, void *data)
{
    struct sock_recvmsg *ctx = data;

    ctx->result = recvmsg(s->sock_fd, ctx->msg, ctx->flags);
    return  (ctx->result >= 0);
}

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
    sock_addr_t addrbuf;
    socklen_t addrbuflen;
    struct msghdr msg = {0};
    PyObject *cmsg_list = NULL, *retval = NULL;
    void *controlbuf = NULL;
    struct cmsghdr *cmsgh;
    size_t cmsgdatalen = 0;
    int cmsg_status;
    struct sock_recvmsg ctx;

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

    msg.msg_name = SAS2SA(&addrbuf);
    msg.msg_namelen = addrbuflen;
    msg.msg_iov = iov;
    msg.msg_iovlen = iovlen;
    msg.msg_control = controlbuf;
    msg.msg_controllen = controllen;

    ctx.msg = &msg;
    ctx.flags = flags;
    if (sock_call(s, 0, sock_recvmsg_impl, &ctx) < 0)
        goto finally;

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
                           (*makeval)(ctx.result, makeval_data),
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
    return Py_XNewRef(*buf);
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


struct sock_send {
    char *buf;
    Py_ssize_t len;
    int flags;
    Py_ssize_t result;
};

static int
sock_send_impl(PySocketSockObject *s, void *data)
{
    struct sock_send *ctx = data;

#ifdef MS_WINDOWS
    if (ctx->len > INT_MAX)
        ctx->len = INT_MAX;
    ctx->result = send(s->sock_fd, ctx->buf, (int)ctx->len, ctx->flags);
#else
    ctx->result = send(s->sock_fd, ctx->buf, ctx->len, ctx->flags);
#endif
    return (ctx->result >= 0);
}

/* s.send(data [,flags]) method */

static PyObject *
sock_send(PySocketSockObject *s, PyObject *args)
{
    int flags = 0;
    Py_buffer pbuf;
    struct sock_send ctx;

    if (!PyArg_ParseTuple(args, "y*|i:send", &pbuf, &flags))
        return NULL;

    if (!IS_SELECTABLE(s)) {
        PyBuffer_Release(&pbuf);
        return select_error();
    }
    ctx.buf = pbuf.buf;
    ctx.len = pbuf.len;
    ctx.flags = flags;
    if (sock_call(s, 1, sock_send_impl, &ctx) < 0) {
        PyBuffer_Release(&pbuf);
        return NULL;
    }
    PyBuffer_Release(&pbuf);

    return PyLong_FromSsize_t(ctx.result);
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
    Py_ssize_t len, n;
    int flags = 0;
    Py_buffer pbuf;
    struct sock_send ctx;
    int has_timeout = (s->sock_timeout > 0);
    _PyTime_t timeout = s->sock_timeout;
    _PyTime_t deadline = 0;
    int deadline_initialized = 0;
    PyObject *res = NULL;

    if (!PyArg_ParseTuple(args, "y*|i:sendall", &pbuf, &flags))
        return NULL;
    buf = pbuf.buf;
    len = pbuf.len;

    if (!IS_SELECTABLE(s)) {
        PyBuffer_Release(&pbuf);
        return select_error();
    }

    do {
        if (has_timeout) {
            if (deadline_initialized) {
                /* recompute the timeout */
                timeout = _PyDeadline_Get(deadline);
            }
            else {
                deadline_initialized = 1;
                deadline = _PyDeadline_Init(timeout);
            }

            if (timeout <= 0) {
                PyErr_SetString(PyExc_TimeoutError, "timed out");
                goto done;
            }
        }

        ctx.buf = buf;
        ctx.len = len;
        ctx.flags = flags;
        if (sock_call_ex(s, 1, sock_send_impl, &ctx, 0, NULL, timeout) < 0)
            goto done;
        n = ctx.result;
        assert(n >= 0);

        buf += n;
        len -= n;

        /* We must run our signal handlers before looping again.
           send() can return a successful partial write when it is
           interrupted, so we can't restrict ourselves to EINTR. */
        if (PyErr_CheckSignals())
            goto done;
    } while (len > 0);
    PyBuffer_Release(&pbuf);

    res = Py_NewRef(Py_None);

done:
    PyBuffer_Release(&pbuf);
    return res;
}

PyDoc_STRVAR(sendall_doc,
"sendall(data[, flags])\n\
\n\
Send a data string to the socket.  For the optional flags\n\
argument, see the Unix manual.  This calls send() repeatedly\n\
until all data is sent.  If an error occurs, it's impossible\n\
to tell how much data has been sent.");


#ifdef HAVE_SENDTO
struct sock_sendto {
    char *buf;
    Py_ssize_t len;
    int flags;
    int addrlen;
    sock_addr_t *addrbuf;
    Py_ssize_t result;
};

static int
sock_sendto_impl(PySocketSockObject *s, void *data)
{
    struct sock_sendto *ctx = data;

#ifdef MS_WINDOWS
    if (ctx->len > INT_MAX)
        ctx->len = INT_MAX;
    ctx->result = sendto(s->sock_fd, ctx->buf, (int)ctx->len, ctx->flags,
                         SAS2SA(ctx->addrbuf), ctx->addrlen);
#else
    ctx->result = sendto(s->sock_fd, ctx->buf, ctx->len, ctx->flags,
                         SAS2SA(ctx->addrbuf), ctx->addrlen);
#endif
    return (ctx->result >= 0);
}

/* s.sendto(data, [flags,] sockaddr) method */

static PyObject *
sock_sendto(PySocketSockObject *s, PyObject *args)
{
    Py_buffer pbuf;
    PyObject *addro;
    Py_ssize_t arglen;
    sock_addr_t addrbuf;
    int addrlen, flags;
    struct sock_sendto ctx;

    flags = 0;
    arglen = PyTuple_Size(args);
    switch (arglen) {
        case 2:
            if (!PyArg_ParseTuple(args, "y*O:sendto", &pbuf, &addro)) {
                return NULL;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "y*iO:sendto",
                                  &pbuf, &flags, &addro)) {
                return NULL;
            }
            break;
        default:
            PyErr_Format(PyExc_TypeError,
                         "sendto() takes 2 or 3 arguments (%zd given)",
                         arglen);
            return NULL;
    }

    if (!IS_SELECTABLE(s)) {
        PyBuffer_Release(&pbuf);
        return select_error();
    }

    if (!getsockaddrarg(s, addro, &addrbuf, &addrlen, "sendto")) {
        PyBuffer_Release(&pbuf);
        return NULL;
    }

    if (PySys_Audit("socket.sendto", "OO", s, addro) < 0) {
        return NULL;
    }

    ctx.buf = pbuf.buf;
    ctx.len = pbuf.len;
    ctx.flags = flags;
    ctx.addrlen = addrlen;
    ctx.addrbuf = &addrbuf;
    if (sock_call(s, 1, sock_sendto_impl, &ctx) < 0) {
        PyBuffer_Release(&pbuf);
        return NULL;
    }
    PyBuffer_Release(&pbuf);

    return PyLong_FromSsize_t(ctx.result);
}

PyDoc_STRVAR(sendto_doc,
"sendto(data[, flags], address) -> count\n\
\n\
Like send(data, flags) but allows specifying the destination address.\n\
For IP sockets, the address is a pair (hostaddr, port).");
#endif


/* The sendmsg() and recvmsg[_into]() methods require a working
   CMSG_LEN().  See the comment near get_CMSG_LEN(). */
#ifdef CMSG_LEN
struct sock_sendmsg {
    struct msghdr *msg;
    int flags;
    ssize_t result;
};

static int
sock_sendmsg_iovec(PySocketSockObject *s, PyObject *data_arg,
                   struct msghdr *msg,
                   Py_buffer **databufsout, Py_ssize_t *ndatabufsout) {
    Py_ssize_t ndataparts, ndatabufs = 0;
    int result = -1;
    struct iovec *iovs = NULL;
    PyObject *data_fast = NULL;
    Py_buffer *databufs = NULL;

    /* Fill in an iovec for each message part, and save the Py_buffer
       structs to release afterwards. */
    data_fast = PySequence_Fast(data_arg,
                                "sendmsg() argument 1 must be an "
                                "iterable");
    if (data_fast == NULL) {
        goto finally;
    }

    ndataparts = PySequence_Fast_GET_SIZE(data_fast);
    if (ndataparts > INT_MAX) {
        PyErr_SetString(PyExc_OSError, "sendmsg() argument 1 is too long");
        goto finally;
    }

    msg->msg_iovlen = ndataparts;
    if (ndataparts > 0) {
        iovs = PyMem_New(struct iovec, ndataparts);
        if (iovs == NULL) {
            PyErr_NoMemory();
            goto finally;
        }
        msg->msg_iov = iovs;

        databufs = PyMem_New(Py_buffer, ndataparts);
        if (databufs == NULL) {
            PyErr_NoMemory();
            goto finally;
        }
    }
    for (; ndatabufs < ndataparts; ndatabufs++) {
        if (!PyArg_Parse(PySequence_Fast_GET_ITEM(data_fast, ndatabufs),
                         "y*;sendmsg() argument 1 must be an iterable of "
                         "bytes-like objects",
                         &databufs[ndatabufs]))
            goto finally;
        iovs[ndatabufs].iov_base = databufs[ndatabufs].buf;
        iovs[ndatabufs].iov_len = databufs[ndatabufs].len;
    }
    result = 0;
  finally:
    *databufsout = databufs;
    *ndatabufsout = ndatabufs;
    Py_XDECREF(data_fast);
    return result;
}

static int
sock_sendmsg_impl(PySocketSockObject *s, void *data)
{
    struct sock_sendmsg *ctx = data;

    ctx->result = sendmsg(s->sock_fd, ctx->msg, ctx->flags);
    return (ctx->result >= 0);
}

/* s.sendmsg(buffers[, ancdata[, flags[, address]]]) method */

static PyObject *
sock_sendmsg(PySocketSockObject *s, PyObject *args)
{
    Py_ssize_t i, ndatabufs = 0, ncmsgs, ncmsgbufs = 0;
    Py_buffer *databufs = NULL;
    sock_addr_t addrbuf;
    struct msghdr msg;
    struct cmsginfo {
        int level;
        int type;
        Py_buffer data;
    } *cmsgs = NULL;
    void *controlbuf = NULL;
    size_t controllen, controllen_last;
    int addrlen, flags = 0;
    PyObject *data_arg, *cmsg_arg = NULL, *addr_arg = NULL,
        *cmsg_fast = NULL, *retval = NULL;
    struct sock_sendmsg ctx;

    if (!PyArg_ParseTuple(args, "O|OiO:sendmsg",
                          &data_arg, &cmsg_arg, &flags, &addr_arg)) {
        return NULL;
    }

    memset(&msg, 0, sizeof(msg));

    /* Parse destination address. */
    if (addr_arg != NULL && addr_arg != Py_None) {
        if (!getsockaddrarg(s, addr_arg, &addrbuf, &addrlen,
                            "sendmsg"))
        {
            goto finally;
        }
        if (PySys_Audit("socket.sendmsg", "OO", s, addr_arg) < 0) {
            return NULL;
        }
        msg.msg_name = &addrbuf;
        msg.msg_namelen = addrlen;
    } else {
        if (PySys_Audit("socket.sendmsg", "OO", s, Py_None) < 0) {
            return NULL;
        }
    }

    /* Fill in an iovec for each message part, and save the Py_buffer
       structs to release afterwards. */
    if (sock_sendmsg_iovec(s, data_arg, &msg, &databufs, &ndatabufs) == -1) {
        goto finally;
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

        controlbuf = PyMem_Malloc(controllen);
        if (controlbuf == NULL) {
            PyErr_NoMemory();
            goto finally;
        }
        msg.msg_control = controlbuf;

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

    ctx.msg = &msg;
    ctx.flags = flags;
    if (sock_call(s, 1, sock_sendmsg_impl, &ctx) < 0)
        goto finally;

    retval = PyLong_FromSsize_t(ctx.result);

finally:
    PyMem_Free(controlbuf);
    for (i = 0; i < ncmsgbufs; i++)
        PyBuffer_Release(&cmsgs[i].data);
    PyMem_Free(cmsgs);
    Py_XDECREF(cmsg_fast);
    PyMem_Free(msg.msg_iov);
    for (i = 0; i < ndatabufs; i++) {
        PyBuffer_Release(&databufs[i]);
    }
    PyMem_Free(databufs);
    return retval;
}

PyDoc_STRVAR(sendmsg_doc,
"sendmsg(buffers[, ancdata[, flags[, address]]]) -> count\n\
\n\
Send normal and ancillary data to the socket, gathering the\n\
non-ancillary data from a series of buffers and concatenating it into\n\
a single message.  The buffers argument specifies the non-ancillary\n\
data as an iterable of bytes-like objects (e.g. bytes objects).\n\
The ancdata argument specifies the ancillary data (control messages)\n\
as an iterable of zero or more tuples (cmsg_level, cmsg_type,\n\
cmsg_data), where cmsg_level and cmsg_type are integers specifying the\n\
protocol level and protocol-specific type respectively, and cmsg_data\n\
is a bytes-like object holding the associated data.  The flags\n\
argument defaults to 0 and has the same meaning as for send().  If\n\
address is supplied and not None, it sets a destination address for\n\
the message.  The return value is the number of bytes of non-ancillary\n\
data sent.");
#endif    /* CMSG_LEN */

#ifdef HAVE_SOCKADDR_ALG
static PyObject*
sock_sendmsg_afalg(PySocketSockObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *retval = NULL;

    Py_ssize_t i, ndatabufs = 0;
    Py_buffer *databufs = NULL;
    PyObject *data_arg = NULL;

    Py_buffer iv = {NULL, NULL};

    PyObject *opobj = NULL;
    int op = -1;

    PyObject *assoclenobj = NULL;
    int assoclen = -1;

    unsigned int *uiptr;
    int flags = 0;

    struct msghdr msg;
    struct cmsghdr *header = NULL;
    struct af_alg_iv *alg_iv = NULL;
    struct sock_sendmsg ctx;
    Py_ssize_t controllen;
    void *controlbuf = NULL;
    static char *keywords[] = {"msg", "op", "iv", "assoclen", "flags", 0};

    if (self->sock_family != AF_ALG) {
        PyErr_SetString(PyExc_OSError,
                        "algset is only supported for AF_ALG");
        return NULL;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwds,
                                     "|O$O!y*O!i:sendmsg_afalg", keywords,
                                     &data_arg,
                                     &PyLong_Type, &opobj, &iv,
                                     &PyLong_Type, &assoclenobj, &flags)) {
        return NULL;
    }

    memset(&msg, 0, sizeof(msg));

    /* op is a required, keyword-only argument >= 0 */
    if (opobj != NULL) {
        op = _PyLong_AsInt(opobj);
    }
    if (op < 0) {
        /* override exception from _PyLong_AsInt() */
        PyErr_SetString(PyExc_TypeError,
                        "Invalid or missing argument 'op'");
        goto finally;
    }
    /* assoclen is optional but must be >= 0 */
    if (assoclenobj != NULL) {
        assoclen = _PyLong_AsInt(assoclenobj);
        if (assoclen == -1 && PyErr_Occurred()) {
            goto finally;
        }
        if (assoclen < 0) {
            PyErr_SetString(PyExc_TypeError,
                            "assoclen must be positive");
            goto finally;
        }
    }

    controllen = CMSG_SPACE(4);
    if (iv.buf != NULL) {
        controllen += CMSG_SPACE(sizeof(*alg_iv) + iv.len);
    }
    if (assoclen >= 0) {
        controllen += CMSG_SPACE(4);
    }

    controlbuf = PyMem_Malloc(controllen);
    if (controlbuf == NULL) {
        PyErr_NoMemory();
        goto finally;
    }
    memset(controlbuf, 0, controllen);

    msg.msg_controllen = controllen;
    msg.msg_control = controlbuf;

    /* Fill in an iovec for each message part, and save the Py_buffer
       structs to release afterwards. */
    if (data_arg != NULL) {
        if (sock_sendmsg_iovec(self, data_arg, &msg, &databufs, &ndatabufs) == -1) {
            goto finally;
        }
    }

    /* set operation to encrypt or decrypt */
    header = CMSG_FIRSTHDR(&msg);
    if (header == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "unexpected NULL result from CMSG_FIRSTHDR");
        goto finally;
    }
    header->cmsg_level = SOL_ALG;
    header->cmsg_type = ALG_SET_OP;
    header->cmsg_len = CMSG_LEN(4);
    uiptr = (void*)CMSG_DATA(header);
    *uiptr = (unsigned int)op;

    /* set initialization vector */
    if (iv.buf != NULL) {
        header = CMSG_NXTHDR(&msg, header);
        if (header == NULL) {
            PyErr_SetString(PyExc_RuntimeError,
                            "unexpected NULL result from CMSG_NXTHDR(iv)");
            goto finally;
        }
        header->cmsg_level = SOL_ALG;
        header->cmsg_type = ALG_SET_IV;
        header->cmsg_len = CMSG_SPACE(sizeof(*alg_iv) + iv.len);
        alg_iv = (void*)CMSG_DATA(header);
        alg_iv->ivlen = iv.len;
        memcpy(alg_iv->iv, iv.buf, iv.len);
    }

    /* set length of associated data for AEAD */
    if (assoclen >= 0) {
        header = CMSG_NXTHDR(&msg, header);
        if (header == NULL) {
            PyErr_SetString(PyExc_RuntimeError,
                            "unexpected NULL result from CMSG_NXTHDR(assoc)");
            goto finally;
        }
        header->cmsg_level = SOL_ALG;
        header->cmsg_type = ALG_SET_AEAD_ASSOCLEN;
        header->cmsg_len = CMSG_LEN(4);
        uiptr = (void*)CMSG_DATA(header);
        *uiptr = (unsigned int)assoclen;
    }

    ctx.msg = &msg;
    ctx.flags = flags;
    if (sock_call(self, 1, sock_sendmsg_impl, &ctx) < 0) {
        goto finally;
    }

    retval = PyLong_FromSsize_t(ctx.result);

  finally:
    PyMem_Free(controlbuf);
    if (iv.buf != NULL) {
        PyBuffer_Release(&iv);
    }
    PyMem_Free(msg.msg_iov);
    for (i = 0; i < ndatabufs; i++) {
        PyBuffer_Release(&databufs[i]);
    }
    PyMem_Free(databufs);
    return retval;
}

PyDoc_STRVAR(sendmsg_afalg_doc,
"sendmsg_afalg([msg], *, op[, iv[, assoclen[, flags=MSG_MORE]]])\n\
\n\
Set operation mode, IV and length of associated data for an AF_ALG\n\
operation socket.");
#endif

#ifdef HAVE_SHUTDOWN
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
    Py_RETURN_NONE;
}

PyDoc_STRVAR(shutdown_doc,
"shutdown(flag)\n\
\n\
Shut down the reading side of the socket (flag == SHUT_RD), the writing side\n\
of the socket (flag == SHUT_WR), or both ends (flag == SHUT_RDWR).");
#endif

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
#if defined(SIO_LOOPBACK_FAST_PATH)
    case SIO_LOOPBACK_FAST_PATH: {
        unsigned int option;
        if (!PyArg_ParseTuple(arg, "kI:ioctl", &cmd, &option))
            return NULL;
        if (WSAIoctl(s->sock_fd, cmd, &option, sizeof(option),
                         NULL, 0, &recv, NULL, NULL) == SOCKET_ERROR) {
            return set_error();
        }
        return PyLong_FromUnsignedLong(recv); }
#endif
    default:
        PyErr_Format(PyExc_ValueError, "invalid ioctl command %lu", cmd);
        return NULL;
    }
}
PyDoc_STRVAR(sock_ioctl_doc,
"ioctl(cmd, option) -> long\n\
\n\
Control the socket with WSAIoctl syscall. Currently supported 'cmd' values are\n\
SIO_RCVALL:  'option' must be one of the socket.RCVALL_* constants.\n\
SIO_KEEPALIVE_VALS:  'option' is a tuple of (onoff, timeout, interval).\n\
SIO_LOOPBACK_FAST_PATH: 'option' is a boolean value, and is disabled by default");
#endif

#if defined(MS_WINDOWS)
static PyObject*
sock_share(PySocketSockObject *s, PyObject *arg)
{
    WSAPROTOCOL_INFOW info;
    DWORD processId;
    int result;

    if (!PyArg_ParseTuple(arg, "I", &processId))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    result = WSADuplicateSocketW(s->sock_fd, processId, &info);
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
#if defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4)
    {"_accept",           (PyCFunction)sock_accept, METH_NOARGS,
                      accept_doc},
#endif
#ifdef HAVE_BIND
    {"bind",              (PyCFunction)sock_bind, METH_O,
                      bind_doc},
#endif
    {"close",             (PyCFunction)sock_close, METH_NOARGS,
                      sock_close_doc},
#ifdef HAVE_CONNECT
    {"connect",           (PyCFunction)sock_connect, METH_O,
                      connect_doc},
    {"connect_ex",        (PyCFunction)sock_connect_ex, METH_O,
                      connect_ex_doc},
#endif
    {"detach",            (PyCFunction)sock_detach, METH_NOARGS,
                      detach_doc},
    {"fileno",            (PyCFunction)sock_fileno, METH_NOARGS,
                      fileno_doc},
#ifdef HAVE_GETPEERNAME
    {"getpeername",       (PyCFunction)sock_getpeername,
                      METH_NOARGS, getpeername_doc},
#endif
#ifdef HAVE_GETSOCKNAME
    {"getsockname",       (PyCFunction)sock_getsockname,
                      METH_NOARGS, getsockname_doc},
#endif
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
#ifdef HAVE_LISTEN
    {"listen",            (PyCFunction)sock_listen, METH_VARARGS,
                      listen_doc},
#endif
    {"recv",              (PyCFunction)sock_recv, METH_VARARGS,
                      recv_doc},
    {"recv_into",         _PyCFunction_CAST(sock_recv_into), METH_VARARGS | METH_KEYWORDS,
                      recv_into_doc},
#ifdef HAVE_RECVFROM
    {"recvfrom",          (PyCFunction)sock_recvfrom, METH_VARARGS,
                      recvfrom_doc},
    {"recvfrom_into",  _PyCFunction_CAST(sock_recvfrom_into), METH_VARARGS | METH_KEYWORDS,
                      recvfrom_into_doc},
#endif
    {"send",              (PyCFunction)sock_send, METH_VARARGS,
                      send_doc},
    {"sendall",           (PyCFunction)sock_sendall, METH_VARARGS,
                      sendall_doc},
#ifdef HAVE_SENDTO
    {"sendto",            (PyCFunction)sock_sendto, METH_VARARGS,
                      sendto_doc},
#endif
    {"setblocking",       (PyCFunction)sock_setblocking, METH_O,
                      setblocking_doc},
    {"getblocking",   (PyCFunction)sock_getblocking, METH_NOARGS,
                      getblocking_doc},
    {"settimeout",    (PyCFunction)sock_settimeout, METH_O,
                      settimeout_doc},
    {"gettimeout",    (PyCFunction)sock_gettimeout, METH_NOARGS,
                      gettimeout_doc},
#ifdef HAVE_SETSOCKOPT
    {"setsockopt",        (PyCFunction)sock_setsockopt, METH_VARARGS,
                      setsockopt_doc},
#endif
#ifdef HAVE_SHUTDOWN
    {"shutdown",          (PyCFunction)sock_shutdown, METH_O,
                      shutdown_doc},
#endif
#ifdef CMSG_LEN
    {"recvmsg",           (PyCFunction)sock_recvmsg, METH_VARARGS,
                      recvmsg_doc},
    {"recvmsg_into",      (PyCFunction)sock_recvmsg_into, METH_VARARGS,
                      recvmsg_into_doc,},
    {"sendmsg",           (PyCFunction)sock_sendmsg, METH_VARARGS,
                      sendmsg_doc},
#endif
#ifdef HAVE_SOCKADDR_ALG
    {"sendmsg_afalg",     _PyCFunction_CAST(sock_sendmsg_afalg), METH_VARARGS | METH_KEYWORDS,
                      sendmsg_afalg_doc},
#endif
    {NULL,                      NULL}           /* sentinel */
};

/* SockObject members */
static PyMemberDef sock_memberlist[] = {
       {"family", T_INT, offsetof(PySocketSockObject, sock_family), READONLY, "the socket family"},
       {"type", T_INT, offsetof(PySocketSockObject, sock_type), READONLY, "the socket type"},
       {"proto", T_INT, offsetof(PySocketSockObject, sock_proto), READONLY, "the socket protocol"},
       {0},
};

static PyGetSetDef sock_getsetlist[] = {
    {"timeout", (getter)sock_gettimeout, NULL, PyDoc_STR("the socket timeout")},
    {NULL} /* sentinel */
};

/* Deallocate a socket object in response to the last Py_DECREF().
   First close the file description. */

static void
sock_finalize(PySocketSockObject *s)
{
    SOCKET_T fd;

    /* Save the current exception, if any. */
    PyObject *exc = PyErr_GetRaisedException();

    if (s->sock_fd != INVALID_SOCKET) {
        if (PyErr_ResourceWarning((PyObject *)s, 1, "unclosed %R", s)) {
            /* Spurious errors can appear at shutdown */
            if (PyErr_ExceptionMatches(PyExc_Warning)) {
                PyErr_WriteUnraisable((PyObject *)s);
            }
        }

        /* Only close the socket *after* logging the ResourceWarning warning
           to allow the logger to call socket methods like
           socket.getsockname(). If the socket is closed before, socket
           methods fails with the EBADF error. */
        fd = s->sock_fd;
        s->sock_fd = INVALID_SOCKET;

        /* We do not want to retry upon EINTR: see sock_close() */
        Py_BEGIN_ALLOW_THREADS
        (void) SOCKETCLOSE(fd);
        Py_END_ALLOW_THREADS
    }

    /* Restore the saved exception. */
    PyErr_SetRaisedException(exc);
}

static int
sock_traverse(PySocketSockObject *s, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(s));
    return 0;
}

static void
sock_dealloc(PySocketSockObject *s)
{
    if (PyObject_CallFinalizerFromDealloc((PyObject *)s) < 0) {
        return;
    }
    PyTypeObject *tp = Py_TYPE(s);
    PyObject_GC_UnTrack(s);
    tp->tp_free((PyObject *)s);
    Py_DECREF(tp);
}


static PyObject *
sock_repr(PySocketSockObject *s)
{
    long sock_fd;
    /* On Windows, this test is needed because SOCKET_T is unsigned */
    if (s->sock_fd == INVALID_SOCKET) {
        sock_fd = -1;
    }
#if SIZEOF_SOCKET_T > SIZEOF_LONG
    else if (s->sock_fd > LONG_MAX) {
        /* this can occur on Win64, and actually there is a special
           ugly printf formatter for decimal pointer length integer
           printing, only bother if necessary*/
        PyErr_SetString(PyExc_OverflowError,
                        "no printf formatter to display "
                        "the socket descriptor in decimal");
        return NULL;
    }
#endif
    else
        sock_fd = (long)s->sock_fd;
    return PyUnicode_FromFormat(
        "<socket object, fd=%ld, family=%d, type=%d, proto=%d>",
        sock_fd, s->sock_family,
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
        ((PySocketSockObject *)new)->sock_fd = INVALID_SOCKET;
        ((PySocketSockObject *)new)->sock_timeout = _PyTime_FromSeconds(-1);
        ((PySocketSockObject *)new)->errorhandler = &set_error;
    }
    return new;
}


/* Initialize a new socket object. */

/*ARGSUSED*/

#ifndef HAVE_SOCKET
#define socket stub_socket
static int
socket(int domain, int type, int protocol)
{
    errno = ENOTSUP;
    return INVALID_SOCKET;
}
#endif

/*[clinic input]
_socket.socket.__init__ as sock_initobj
    family: int = -1
    type: int = -1
    proto: int = -1
    fileno as fdobj: object = NULL
[clinic start generated code]*/

static int
sock_initobj_impl(PySocketSockObject *self, int family, int type, int proto,
                  PyObject *fdobj)
/*[clinic end generated code: output=d114d026b9a9a810 input=04cfc32953f5cc25]*/
{

    SOCKET_T fd = INVALID_SOCKET;
    socket_state *state = find_module_state_by_def(Py_TYPE(self));

#ifndef MS_WINDOWS
#ifdef SOCK_CLOEXEC
    int *atomic_flag_works = &state->sock_cloexec_works;
#else
    int *atomic_flag_works = NULL;
#endif
#endif

#ifdef MS_WINDOWS
    /* In this case, we don't use the family, type and proto args */
    if (fdobj == NULL || fdobj == Py_None)
#endif
    {
        if (PySys_Audit("socket.__new__", "Oiii",
                        self, family, type, proto) < 0) {
            return -1;
        }
    }

    if (fdobj != NULL && fdobj != Py_None) {
#ifdef MS_WINDOWS
        /* recreate a socket that was duplicated */
        if (PyBytes_Check(fdobj)) {
            WSAPROTOCOL_INFOW info;
            if (PyBytes_GET_SIZE(fdobj) != sizeof(info)) {
                PyErr_Format(PyExc_ValueError,
                    "socket descriptor string has wrong size, "
                    "should be %zu bytes.", sizeof(info));
                return -1;
            }
            memcpy(&info, PyBytes_AS_STRING(fdobj), sizeof(info));

            if (PySys_Audit("socket.__new__", "Oiii", self,
                            info.iAddressFamily, info.iSocketType,
                            info.iProtocol) < 0) {
                return -1;
            }

            Py_BEGIN_ALLOW_THREADS
            fd = WSASocketW(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                     FROM_PROTOCOL_INFO, &info, 0, WSA_FLAG_OVERLAPPED);
            Py_END_ALLOW_THREADS
            if (fd == INVALID_SOCKET) {
                set_error();
                return -1;
            }

            if (!SetHandleInformation((HANDLE)fd, HANDLE_FLAG_INHERIT, 0)) {
                PyErr_SetFromWindowsErr(0);
                closesocket(fd);
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
#ifdef MS_WINDOWS
            if (fd == INVALID_SOCKET) {
#else
            if (fd < 0) {
#endif
                PyErr_SetString(PyExc_ValueError, "negative file descriptor");
                return -1;
            }

            /* validate that passed file descriptor is valid and a socket. */
            sock_addr_t addrbuf;
            socklen_t addrlen = sizeof(sock_addr_t);

            memset(&addrbuf, 0, addrlen);
#ifdef HAVE_GETSOCKNAME
            if (getsockname(fd, SAS2SA(&addrbuf), &addrlen) == 0) {
                if (family == -1) {
                    family = SAS2SA(&addrbuf)->sa_family;
                }
            } else {
#ifdef MS_WINDOWS
                /* getsockname() on an unbound socket is an error on Windows.
                   Invalid descriptor and not a socket is same error code.
                   Error out if family must be resolved, or bad descriptor. */
                if (family == -1 || CHECK_ERRNO(ENOTSOCK)) {
#else
                /* getsockname() is not supported for SOL_ALG on Linux. */
                if (family == -1 || CHECK_ERRNO(EBADF) || CHECK_ERRNO(ENOTSOCK)) {
#endif
                    set_error();
                    return -1;
                }
            }
#endif // HAVE_GETSOCKNAME
#ifdef SO_TYPE
            if (type == -1) {
                int tmp;
                socklen_t slen = sizeof(tmp);
                if (getsockopt(fd, SOL_SOCKET, SO_TYPE,
                               (void *)&tmp, &slen) == 0)
                {
                    type = tmp;
                } else {
                    set_error();
                    return -1;
                }
            }
#else
            type = SOCK_STREAM;
#endif
#ifdef SO_PROTOCOL
            if (proto == -1) {
                int tmp;
                socklen_t slen = sizeof(tmp);
                if (getsockopt(fd, SOL_SOCKET, SO_PROTOCOL,
                               (void *)&tmp, &slen) == 0)
                {
                    proto = tmp;
                } else {
                    set_error();
                    return -1;
                }
            }
#else
            proto = 0;
#endif
        }
    }
    else {
        /* No fd, default to AF_INET and SOCK_STREAM */
        if (family == -1) {
            family = AF_INET;
        }
        if (type == -1) {
            type = SOCK_STREAM;
        }
        if (proto == -1) {
            proto = 0;
        }
#ifdef MS_WINDOWS
        Py_BEGIN_ALLOW_THREADS
        fd = WSASocketW(family, type, proto,
                        NULL, 0,
                        WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
        Py_END_ALLOW_THREADS

        if (fd == INVALID_SOCKET) {
            set_error();
            return -1;
        }
#else
        /* UNIX */
        Py_BEGIN_ALLOW_THREADS
#ifdef SOCK_CLOEXEC
        if (state->sock_cloexec_works != 0) {
            fd = socket(family, type | SOCK_CLOEXEC, proto);
            if (state->sock_cloexec_works == -1) {
                if (fd >= 0) {
                    state->sock_cloexec_works = 1;
                }
                else if (errno == EINVAL) {
                    /* Linux older than 2.6.27 does not support SOCK_CLOEXEC */
                    state->sock_cloexec_works = 0;
                    fd = socket(family, type, proto);
                }
            }
        }
        else
#endif
        {
            fd = socket(family, type, proto);
        }
        Py_END_ALLOW_THREADS

        if (fd == INVALID_SOCKET) {
            set_error();
            return -1;
        }

        if (_Py_set_inheritable(fd, 0, atomic_flag_works) < 0) {
            SOCKETCLOSE(fd);
            return -1;
        }
#endif
    }
    if (init_sockobject(state, self, fd, family, type, proto) == -1) {
        SOCKETCLOSE(fd);
        return -1;
    }

    return 0;

}


/* Type object for socket objects. */

static PyType_Slot sock_slots[] = {
    {Py_tp_dealloc, sock_dealloc},
    {Py_tp_traverse, sock_traverse},
    {Py_tp_repr, sock_repr},
    {Py_tp_doc, (void *)sock_doc},
    {Py_tp_methods, sock_methods},
    {Py_tp_members, sock_memberlist},
    {Py_tp_getset, sock_getsetlist},
    {Py_tp_init, sock_initobj},
    {Py_tp_new, sock_new},
    {Py_tp_finalize, sock_finalize},
    {0, NULL},
};

static PyType_Spec sock_spec = {
    .name = "_socket.socket",
    .basicsize = sizeof(PySocketSockObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = sock_slots,
};


#ifdef HAVE_GETHOSTNAME
/* Python interface to gethostname(). */

/*ARGSUSED*/
static PyObject *
socket_gethostname(PyObject *self, PyObject *unused)
{
    if (PySys_Audit("socket.gethostname", NULL) < 0) {
        return NULL;
    }

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
    name = PyMem_New(wchar_t, size);
    if (!name) {
        PyErr_NoMemory();
        return NULL;
    }
    if (!GetComputerNameExW(ComputerNamePhysicalDnsHostname,
                           name,
                           &size))
    {
        PyErr_SetFromWindowsErr(0);
        PyMem_Free(name);
        return NULL;
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
    return PyUnicode_DecodeFSDefault(buf);
#endif
}

PyDoc_STRVAR(gethostname_doc,
"gethostname() -> string\n\
\n\
Return the current host name.");
#endif

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

#if defined(_AIX) || (defined(__sun) && defined(__SVR4) && Py_SUNOS_VERSION <= 510)
/* issue #18259, sethostname is not declared in any useful header file on AIX
 * the same is true for Solaris 10 */
extern int sethostname(const char *, size_t);
#endif

    if (!PyArg_ParseTuple(args, "S:sethostname", &hnobj)) {
        PyErr_Clear();
        if (!PyArg_ParseTuple(args, "O&:sethostname",
                PyUnicode_FSConverter, &hnobj))
            return NULL;
        flag = 1;
    }

    if (PySys_Audit("socket.sethostname", "(O)", hnobj) < 0) {
        return NULL;
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

#ifdef HAVE_GETADDRINFO
/* Python interface to gethostbyname(name). */

/*ARGSUSED*/
static PyObject *
socket_gethostbyname(PyObject *self, PyObject *args)
{
    char *name;
    struct sockaddr_in addrbuf;
    PyObject *ret = NULL;

    if (!PyArg_ParseTuple(args, "et:gethostbyname", "idna", &name))
        return NULL;
    if (PySys_Audit("socket.gethostbyname", "O", args) < 0) {
        goto finally;
    }
    socket_state *state = get_module_state(self);
    int rc = setipaddr(state, name, (struct sockaddr *)&addrbuf,
                       sizeof(addrbuf), AF_INET);
    if (rc < 0) {
        goto finally;
    }
    ret = make_ipv4_addr(&addrbuf);
finally:
    PyMem_Free(name);
    return ret;
}

PyDoc_STRVAR(gethostbyname_doc,
"gethostbyname(host) -> address\n\
\n\
Return the IP address (a string of the form '255.255.255.255') for a host.");
#endif


#if defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYNAME) || defined (HAVE_GETHOSTBYADDR)
static PyObject*
sock_decode_hostname(const char *name)
{
#ifdef MS_WINDOWS
    /* Issue #26227: gethostbyaddr() returns a string encoded
     * to the ANSI code page */
    return PyUnicode_DecodeMBCS(name, strlen(name), "surrogatepass");
#else
    /* Decode from UTF-8 */
    return PyUnicode_FromString(name);
#endif
}

/* Convenience function common to gethostbyname_ex and gethostbyaddr */

static PyObject *
gethost_common(socket_state *state, struct hostent *h, struct sockaddr *addr,
               size_t alen, int af)
{
    char **pch;
    PyObject *rtn_tuple = (PyObject *)NULL;
    PyObject *name_list = (PyObject *)NULL;
    PyObject *addr_list = (PyObject *)NULL;
    PyObject *tmp;
    PyObject *name;

    if (h == NULL) {
        /* Let's get real error message to return */
        set_herror(state, h_errno);
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
        for (pch = h->h_aliases; ; pch++) {
            int status;
            char *host_alias;
            // pch can be misaligned
            memcpy(&host_alias, pch, sizeof(host_alias));
            if (host_alias == NULL) {
                break;
            }
            tmp = PyUnicode_FromString(host_alias);
            if (tmp == NULL)
                goto err;

            status = PyList_Append(name_list, tmp);
            Py_DECREF(tmp);

            if (status)
                goto err;
        }
    }

    for (pch = h->h_addr_list; ; pch++) {
        int status;
        char *host_address;
        // pch can be misaligned
        memcpy(&host_address, pch, sizeof(host_address));
        if (host_address == NULL) {
            break;
        }

        switch (af) {

        case AF_INET:
            {
            struct sockaddr_in sin;
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = af;
#ifdef HAVE_SOCKADDR_SA_LEN
            sin.sin_len = sizeof(sin);
#endif
            memcpy(&sin.sin_addr, host_address, sizeof(sin.sin_addr));
            tmp = make_ipv4_addr(&sin);

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
            memcpy(&sin6.sin6_addr, host_address, sizeof(sin6.sin6_addr));
            tmp = make_ipv6_addr(&sin6);

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

    name = sock_decode_hostname(h->h_name);
    if (name == NULL)
        goto err;
    rtn_tuple = Py_BuildValue("NOO", name, name_list, addr_list);

 err:
    Py_XDECREF(name_list);
    Py_XDECREF(addr_list);
    return rtn_tuple;
}
#endif

#if defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYNAME)
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
    if (PySys_Audit("socket.gethostbyname", "O", args) < 0) {
        goto finally;
    }
    socket_state *state = get_module_state(self);
    if (setipaddr(state, name, SAS2SA(&addr), sizeof(addr), AF_INET) < 0) {
        goto finally;
    }
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
    SUPPRESS_DEPRECATED_CALL
    h = gethostbyname(name);
#endif /* HAVE_GETHOSTBYNAME_R */
    Py_END_ALLOW_THREADS
    /* Some C libraries would require addr.__ss_family instead of
       addr.ss_family.
       Therefore, we cast the sockaddr_storage into sockaddr to
       access sa_family. */
    sa = SAS2SA(&addr);
    ret = gethost_common(state, h, SAS2SA(&addr), sizeof(addr),
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
#endif

#if defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYADDR)
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
    const char *ap;
    int al;
    int af;

    if (!PyArg_ParseTuple(args, "et:gethostbyaddr", "idna", &ip_num))
        return NULL;
    if (PySys_Audit("socket.gethostbyaddr", "O", args) < 0) {
        goto finally;
    }
    af = AF_UNSPEC;
    socket_state *state = get_module_state(self);
    if (setipaddr(state, ip_num, sa, sizeof(addr), af) < 0) {
        goto finally;
    }
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
    SUPPRESS_DEPRECATED_CALL
    h = gethostbyaddr(ap, al, af);
#endif /* HAVE_GETHOSTBYNAME_R */
    Py_END_ALLOW_THREADS
    ret = gethost_common(state, h, SAS2SA(&addr), sizeof(addr), af);
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
#endif

#ifdef HAVE_GETSERVBYNAME
/* Python interface to getservbyname(name).
   This only returns the port number, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getservbyname(PyObject *self, PyObject *args)
{
    const char *name, *proto=NULL;
    struct servent *sp;
    if (!PyArg_ParseTuple(args, "s|s:getservbyname", &name, &proto))
        return NULL;

    if (PySys_Audit("socket.getservbyname", "ss", name, proto) < 0) {
        return NULL;
    }

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
#endif

#ifdef HAVE_GETSERVBYPORT
/* Python interface to getservbyport(port).
   This only returns the service name, since the other info is already
   known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getservbyport(PyObject *self, PyObject *args)
{
    int port;
    const char *proto=NULL;
    struct servent *sp;
    if (!PyArg_ParseTuple(args, "i|s:getservbyport", &port, &proto))
        return NULL;
    if (port < 0 || port > 0xffff) {
        PyErr_SetString(
            PyExc_OverflowError,
            "getservbyport: port must be 0-65535.");
        return NULL;
    }

    if (PySys_Audit("socket.getservbyport", "is", port, proto) < 0) {
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
#endif

#ifdef HAVE_GETPROTOBYNAME
/* Python interface to getprotobyname(name).
   This only returns the protocol number, since the other info is
   already known or not useful (like the list of aliases). */

/*ARGSUSED*/
static PyObject *
socket_getprotobyname(PyObject *self, PyObject *args)
{
    const char *name;
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
#endif

static PyObject *
socket_close(PyObject *self, PyObject *fdobj)
{
    SOCKET_T fd;
    int res;

    fd = PyLong_AsSocket_t(fdobj);
    if (fd == (SOCKET_T)(-1) && PyErr_Occurred())
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = SOCKETCLOSE(fd);
    Py_END_ALLOW_THREADS
    /* bpo-30319: The peer can already have closed the connection.
       Python ignores ECONNRESET on close(). */
    if (res < 0 && !CHECK_ERRNO(ECONNRESET)) {
        return set_error();
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(close_doc,
"close(integer) -> None\n\
\n\
Close an integer socket file descriptor.  This is like os.close(), but for\n\
sockets; on some platforms os.close() won't work for socket file descriptors.");

#ifndef NO_DUP
/* dup() function for socket fds */

static PyObject *
socket_dup(PyObject *self, PyObject *fdobj)
{
    SOCKET_T fd, newfd;
    PyObject *newfdobj;
#ifdef MS_WINDOWS
    WSAPROTOCOL_INFOW info;
#endif

    fd = PyLong_AsSocket_t(fdobj);
    if (fd == (SOCKET_T)(-1) && PyErr_Occurred()) {
        return NULL;
    }

#ifdef MS_WINDOWS
    if (WSADuplicateSocketW(fd, GetCurrentProcessId(), &info))
        return set_error();

    newfd = WSASocketW(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
                      FROM_PROTOCOL_INFO,
                      &info, 0, WSA_FLAG_OVERLAPPED);
    if (newfd == INVALID_SOCKET) {
        return set_error();
    }

    if (!SetHandleInformation((HANDLE)newfd, HANDLE_FLAG_INHERIT, 0)) {
        PyErr_SetFromWindowsErr(0);
        closesocket(newfd);
        return NULL;
    }
#else
    /* On UNIX, dup can be used to duplicate the file descriptor of a socket */
    newfd = _Py_dup(fd);
    if (newfd == INVALID_SOCKET) {
        return NULL;
    }
#endif

    newfdobj = PyLong_FromSocket_t(newfd);
    if (newfdobj == NULL) {
        SOCKETCLOSE(newfd);
    }
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
    socket_state *state = get_module_state(self);
#ifdef SOCK_CLOEXEC
    int *atomic_flag_works = &state->sock_cloexec_works;
#else
    int *atomic_flag_works = NULL;
#endif
    int ret;

#if defined(AF_UNIX)
    family = AF_UNIX;
#else
    family = AF_INET;
#endif
    if (!PyArg_ParseTuple(args, "|iii:socketpair",
                          &family, &type, &proto))
        return NULL;

    /* Create a pair of socket fds */
    Py_BEGIN_ALLOW_THREADS
#ifdef SOCK_CLOEXEC
    if (state->sock_cloexec_works != 0) {
        ret = socketpair(family, type | SOCK_CLOEXEC, proto, sv);
        if (state->sock_cloexec_works == -1) {
            if (ret >= 0) {
                state->sock_cloexec_works = 1;
            }
            else if (errno == EINVAL) {
                /* Linux older than 2.6.27 does not support SOCK_CLOEXEC */
                state->sock_cloexec_works = 0;
                ret = socketpair(family, type, proto, sv);
            }
        }
    }
    else
#endif
    {
        ret = socketpair(family, type, proto, sv);
    }
    Py_END_ALLOW_THREADS

    if (ret < 0)
        return set_error();

    if (_Py_set_inheritable(sv[0], 0, atomic_flag_works) < 0)
        goto finally;
    if (_Py_set_inheritable(sv[1], 0, atomic_flag_works) < 0)
        goto finally;

    s0 = new_sockobject(state, sv[0], family, type, proto);
    if (s0 == NULL)
        goto finally;
    s1 = new_sockobject(state, sv[1], family, type, proto);
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
"socketpair([family[, type [, proto]]]) -> (socket object, socket object)\n\
\n\
Create a pair of socket objects from the sockets returned by the platform\n\
socketpair() function.\n\
The arguments are the same as for socket() except the default family is\n\
AF_UNIX if defined on the platform; otherwise, the default is AF_INET.");

#endif /* HAVE_SOCKETPAIR */


static PyObject *
socket_ntohs(PyObject *self, PyObject *args)
{
    int x;

    if (!PyArg_ParseTuple(args, "i:ntohs", &x)) {
        return NULL;
    }
    if (x < 0) {
        PyErr_SetString(PyExc_OverflowError,
                        "ntohs: can't convert negative Python int to C "
                        "16-bit unsigned integer");
        return NULL;
    }
    if (x > 0xffff) {
        PyErr_SetString(PyExc_OverflowError,
                        "ntohs: Python int too large to convert to C "
                        "16-bit unsigned integer");
        return NULL;
    }
    return PyLong_FromUnsignedLong(ntohs((unsigned short)x));
}

PyDoc_STRVAR(ntohs_doc,
"ntohs(integer) -> integer\n\
\n\
Convert a 16-bit unsigned integer from network to host byte order.");


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
                            "int larger than 32 bits");
            x = y;
        }
#endif
    }
    else
        return PyErr_Format(PyExc_TypeError,
                            "expected int, %s found",
                            Py_TYPE(arg)->tp_name);
    return PyLong_FromUnsignedLong(ntohl(x));
}

PyDoc_STRVAR(ntohl_doc,
"ntohl(integer) -> integer\n\
\n\
Convert a 32-bit integer from network to host byte order.");


static PyObject *
socket_htons(PyObject *self, PyObject *args)
{
    int x;

    if (!PyArg_ParseTuple(args, "i:htons", &x)) {
        return NULL;
    }
    if (x < 0) {
        PyErr_SetString(PyExc_OverflowError,
                        "htons: can't convert negative Python int to C "
                        "16-bit unsigned integer");
        return NULL;
    }
    if (x > 0xffff) {
        PyErr_SetString(PyExc_OverflowError,
                        "htons: Python int too large to convert to C "
                        "16-bit unsigned integer");
        return NULL;
    }
    return PyLong_FromUnsignedLong(htons((unsigned short)x));
}

PyDoc_STRVAR(htons_doc,
"htons(integer) -> integer\n\
\n\
Convert a 16-bit unsigned integer from host to network byte order.");


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
                            "int larger than 32 bits");
            x = y;
        }
#endif
    }
    else
        return PyErr_Format(PyExc_TypeError,
                            "expected int, %s found",
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
    const char *ip_addr;

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
        packed_addr = INADDR_BROADCAST;
    } else {

        SUPPRESS_DEPRECATED_CALL
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

#ifdef HAVE_INET_NTOA
PyDoc_STRVAR(inet_ntoa_doc,
"inet_ntoa(packed_ip) -> ip_address_string\n\
\n\
Convert an IP address from 32-bit packed binary format to string format");

static PyObject*
socket_inet_ntoa(PyObject *self, PyObject *args)
{
    Py_buffer packed_ip;
    struct in_addr packed_addr;

    if (!PyArg_ParseTuple(args, "y*:inet_ntoa", &packed_ip)) {
        return NULL;
    }

    if (packed_ip.len != sizeof(packed_addr)) {
        PyErr_SetString(PyExc_OSError,
            "packed IP wrong length for inet_ntoa");
        PyBuffer_Release(&packed_ip);
        return NULL;
    }

    memcpy(&packed_addr, packed_ip.buf, packed_ip.len);
    PyBuffer_Release(&packed_ip);

    SUPPRESS_DEPRECATED_CALL
    return PyUnicode_FromString(inet_ntoa(packed_addr));
}
#endif // HAVE_INET_NTOA

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
    const char* ip;
    int retval;
#ifdef ENABLE_IPV6
    char packed[Py_MAX(sizeof(struct in_addr), sizeof(struct in6_addr))];
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
    Py_buffer packed_ip;
    const char* retval;
#ifdef ENABLE_IPV6
    char ip[Py_MAX(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)];
#else
    char ip[INET_ADDRSTRLEN];
#endif

    if (!PyArg_ParseTuple(args, "iy*:inet_ntop", &af, &packed_ip)) {
        return NULL;
    }

    if (af == AF_INET) {
        if (packed_ip.len != sizeof(struct in_addr)) {
            PyErr_SetString(PyExc_ValueError,
                "invalid length of packed IP address string");
            PyBuffer_Release(&packed_ip);
            return NULL;
        }
#ifdef ENABLE_IPV6
    } else if (af == AF_INET6) {
        if (packed_ip.len != sizeof(struct in6_addr)) {
            PyErr_SetString(PyExc_ValueError,
                "invalid length of packed IP address string");
            PyBuffer_Release(&packed_ip);
            return NULL;
        }
#endif
    } else {
        PyErr_Format(PyExc_ValueError,
            "unknown address family %d", af);
        PyBuffer_Release(&packed_ip);
        return NULL;
    }

    /* inet_ntop guarantee NUL-termination of resulting string. */
    retval = inet_ntop(af, packed_ip.buf, ip, sizeof(ip));
    if (!retval) {
        PyErr_SetFromErrno(PyExc_OSError);
        PyBuffer_Release(&packed_ip);
        return NULL;
    } else {
        PyBuffer_Release(&packed_ip);
        return PyUnicode_FromString(retval);
    }
}

#endif /* HAVE_INET_PTON */

#ifdef HAVE_GETADDRINFO
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
    PyObject *pstr = NULL;
    const char *hptr, *pptr;
    int family, socktype, protocol, flags;
    int error;
    PyObject *all = (PyObject *)NULL;
    PyObject *idna = NULL;

    socktype = protocol = flags = 0;
    family = AF_UNSPEC;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|iiii:getaddrinfo",
                          kwnames, &hobj, &pobj, &family, &socktype,
                          &protocol, &flags)) {
        return NULL;
    }
    if (hobj == Py_None) {
        hptr = NULL;
    } else if (PyUnicode_Check(hobj)) {
        idna = PyUnicode_AsEncodedString(hobj, "idna", NULL);
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
        pstr = PyObject_Str(pobj);
        if (pstr == NULL)
            goto err;
        assert(PyUnicode_Check(pstr));
        pptr = PyUnicode_AsUTF8(pstr);
        if (pptr == NULL)
            goto err;
    } else if (PyUnicode_Check(pobj)) {
        pptr = PyUnicode_AsUTF8(pobj);
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
#if defined(__APPLE__) && defined(AI_NUMERICSERV)
    if ((flags & AI_NUMERICSERV) && (pptr == NULL || (pptr[0] == '0' && pptr[1] == 0))) {
        /* On OSX up to at least OSX 10.8 getaddrinfo crashes
         * if AI_NUMERICSERV is set and the servname is NULL or "0".
         * This workaround avoids a segfault in libsystem.
         */
        pptr = "00";
    }
#endif

    if (PySys_Audit("socket.getaddrinfo", "OOiii",
                    hobj, pobj, family, socktype, protocol) < 0) {
        return NULL;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = flags;
    Py_BEGIN_ALLOW_THREADS
    error = getaddrinfo(hptr, pptr, &hints, &res0);
    Py_END_ALLOW_THREADS
    if (error) {
        res0 = NULL;  // gh-100795
        socket_state *state = get_module_state(self);
        set_gaierror(state, error);
        goto err;
    }

    all = PyList_New(0);
    if (all == NULL)
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

        if (PyList_Append(all, single)) {
            Py_DECREF(single);
            goto err;
        }
        Py_DECREF(single);
    }
    Py_XDECREF(idna);
    Py_XDECREF(pstr);
    if (res0)
        freeaddrinfo(res0);
    return all;
 err:
    Py_XDECREF(all);
    Py_XDECREF(idna);
    Py_XDECREF(pstr);
    if (res0)
        freeaddrinfo(res0);
    return (PyObject *)NULL;
}

PyDoc_STRVAR(getaddrinfo_doc,
"getaddrinfo(host, port [, family, type, proto, flags])\n\
    -> list of (family, type, proto, canonname, sockaddr)\n\
\n\
Resolve host and port into addrinfo struct.");
#endif // HAVE_GETADDRINFO

#ifdef HAVE_GETNAMEINFO
/* Python interface to getnameinfo(sa, flags). */

/*ARGSUSED*/
static PyObject *
socket_getnameinfo(PyObject *self, PyObject *args)
{
    PyObject *sa = (PyObject *)NULL;
    int flags;
    const char *hostp;
    int port;
    unsigned int flowinfo, scope_id;
    char hbuf[NI_MAXHOST], pbuf[NI_MAXSERV];
    struct addrinfo hints, *res = NULL;
    int error;
    PyObject *ret = (PyObject *)NULL;
    PyObject *name;

    flags = flowinfo = scope_id = 0;
    if (!PyArg_ParseTuple(args, "Oi:getnameinfo", &sa, &flags))
        return NULL;
    if (!PyTuple_Check(sa)) {
        PyErr_SetString(PyExc_TypeError,
                        "getnameinfo() argument 1 must be a tuple");
        return NULL;
    }
    if (!PyArg_ParseTuple(sa, "si|II;getnameinfo(): illegal sockaddr argument",
                          &hostp, &port, &flowinfo, &scope_id))
    {
        return NULL;
    }
    if (flowinfo > 0xfffff) {
        PyErr_SetString(PyExc_OverflowError,
                        "getnameinfo(): flowinfo must be 0-1048575.");
        return NULL;
    }

    if (PySys_Audit("socket.getnameinfo", "(O)", sa) < 0) {
        return NULL;
    }

    PyOS_snprintf(pbuf, sizeof(pbuf), "%d", port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;     /* make numeric port happy */
    hints.ai_flags = AI_NUMERICHOST;    /* don't do any name resolution */
    Py_BEGIN_ALLOW_THREADS
    error = getaddrinfo(hostp, pbuf, &hints, &res);
    Py_END_ALLOW_THREADS
    if (error) {
        res = NULL;  // gh-100795
        socket_state *state = get_module_state(self);
        set_gaierror(state, error);
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
    Py_BEGIN_ALLOW_THREADS
    error = getnameinfo(res->ai_addr, (socklen_t) res->ai_addrlen,
                    hbuf, sizeof(hbuf), pbuf, sizeof(pbuf), flags);
    Py_END_ALLOW_THREADS
    if (error) {
        socket_state *state = get_module_state(self);
        set_gaierror(state, error);
        goto fail;
    }

    name = sock_decode_hostname(hbuf);
    if (name == NULL)
        goto fail;
    ret = Py_BuildValue("Ns", name, pbuf);

fail:
    if (res)
        freeaddrinfo(res);
    return ret;
}

PyDoc_STRVAR(getnameinfo_doc,
"getnameinfo(sockaddr, flags) --> (host, port)\n\
\n\
Get host and port for a sockaddr.");
#endif // HAVE_GETNAMEINFO

/* Python API to getting and setting the default timeout value. */

static PyObject *
socket_getdefaulttimeout(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    socket_state *state = get_module_state(self);
    if (state->defaulttimeout < 0) {
        Py_RETURN_NONE;
    }
    else {
        double seconds = _PyTime_AsSecondsDouble(state->defaulttimeout);
        return PyFloat_FromDouble(seconds);
    }
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
    _PyTime_t timeout;

    if (socket_parse_timeout(&timeout, arg) < 0)
        return NULL;

    socket_state *state = get_module_state(self);
    state->defaulttimeout = timeout;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(setdefaulttimeout_doc,
"setdefaulttimeout(timeout)\n\
\n\
Set the default timeout in seconds (float) for new socket objects.\n\
A value of None indicates that new socket objects have no timeout.\n\
When the socket module is first imported, the default is None.");

#if defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS)
/* Python API for getting interface indices and names */

static PyObject *
socket_if_nameindex(PyObject *self, PyObject *arg)
{
    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }
#ifdef MS_WINDOWS
    PMIB_IF_TABLE2 tbl;
    int ret;
    if ((ret = GetIfTable2Ex(MibIfTableRaw, &tbl)) != NO_ERROR) {
        Py_DECREF(list);
        // ret is used instead of GetLastError()
        return PyErr_SetFromWindowsErr(ret);
    }
    for (ULONG i = 0; i < tbl->NumEntries; ++i) {
        MIB_IF_ROW2 r = tbl->Table[i];
        WCHAR buf[NDIS_IF_MAX_STRING_SIZE + 1];
        if ((ret = ConvertInterfaceLuidToNameW(&r.InterfaceLuid, buf,
                                               Py_ARRAY_LENGTH(buf)))) {
            Py_DECREF(list);
            FreeMibTable(tbl);
            // ret is used instead of GetLastError()
            return PyErr_SetFromWindowsErr(ret);
        }
        PyObject *tuple = Py_BuildValue("Iu", r.InterfaceIndex, buf);
        if (tuple == NULL || PyList_Append(list, tuple) == -1) {
            Py_XDECREF(tuple);
            Py_DECREF(list);
            FreeMibTable(tbl);
            return NULL;
        }
        Py_DECREF(tuple);
    }
    FreeMibTable(tbl);
    return list;
#else
    int i;
    struct if_nameindex *ni;

    ni = if_nameindex();
    if (ni == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        Py_DECREF(list);
        return NULL;
    }

#ifdef _Py_MEMORY_SANITIZER
    __msan_unpoison(ni, sizeof(ni));
    __msan_unpoison(&ni[0], sizeof(ni[0]));
#endif
    for (i = 0; ni[i].if_index != 0 && i < INT_MAX; i++) {
#ifdef _Py_MEMORY_SANITIZER
        /* This one isn't the end sentinel, the next one must exist. */
        __msan_unpoison(&ni[i+1], sizeof(ni[0]));
        /* Otherwise Py_BuildValue internals are flagged by MSan when
           they access the not-msan-tracked if_name string data. */
        {
            char *to_sanitize = ni[i].if_name;
            do {
                __msan_unpoison(to_sanitize, 1);
            } while (*to_sanitize++ != '\0');
        }
#endif
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
#endif
}

PyDoc_STRVAR(if_nameindex_doc,
"if_nameindex()\n\
\n\
Returns a list of network interface information (index, name) tuples.");

static PyObject *
socket_if_nametoindex(PyObject *self, PyObject *args)
{
    PyObject *oname;
#ifdef MS_WINDOWS
    NET_IFINDEX index;
#else
    unsigned long index;
#endif
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
    unsigned long index_long = PyLong_AsUnsignedLong(arg);
    if (index_long == (unsigned long) -1 && PyErr_Occurred()) {
        return NULL;
    }

#ifdef MS_WINDOWS
    NET_IFINDEX index = (NET_IFINDEX)index_long;
#else
    unsigned int index = (unsigned int)index_long;
#endif

    if ((unsigned long)index != index_long) {
        PyErr_SetString(PyExc_OverflowError, "index is too large");
        return NULL;
    }

    char name[IF_NAMESIZE + 1];
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

#endif // defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS)


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
#ifdef HAVE_GETADDRINFO
    {"gethostbyname",           socket_gethostbyname,
     METH_VARARGS, gethostbyname_doc},
#endif
#if defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYNAME)
    {"gethostbyname_ex",        socket_gethostbyname_ex,
     METH_VARARGS, ghbn_ex_doc},
#endif
#if defined(HAVE_GETHOSTBYNAME_R) || defined (HAVE_GETHOSTBYADDR)
    {"gethostbyaddr",           socket_gethostbyaddr,
     METH_VARARGS, gethostbyaddr_doc},
#endif
#ifdef HAVE_GETHOSTNAME
    {"gethostname",             socket_gethostname,
     METH_NOARGS,  gethostname_doc},
#endif
#ifdef HAVE_SETHOSTNAME
    {"sethostname",             socket_sethostname,
     METH_VARARGS,  sethostname_doc},
#endif
#ifdef HAVE_GETSERVBYNAME
    {"getservbyname",           socket_getservbyname,
     METH_VARARGS, getservbyname_doc},
#endif
#ifdef HAVE_GETSERVBYPORT
    {"getservbyport",           socket_getservbyport,
     METH_VARARGS, getservbyport_doc},
#endif
#ifdef HAVE_GETPROTOBYNAME
    {"getprotobyname",          socket_getprotobyname,
     METH_VARARGS, getprotobyname_doc},
#endif
    {"close",                   socket_close,
     METH_O, close_doc},
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
#ifdef HAVE_INET_NTOA
    {"inet_ntoa",               socket_inet_ntoa,
     METH_VARARGS, inet_ntoa_doc},
#endif
#ifdef HAVE_INET_PTON
    {"inet_pton",               socket_inet_pton,
     METH_VARARGS, inet_pton_doc},
    {"inet_ntop",               socket_inet_ntop,
     METH_VARARGS, inet_ntop_doc},
#endif
#ifdef HAVE_GETADDRINFO
    {"getaddrinfo",             _PyCFunction_CAST(socket_getaddrinfo),
     METH_VARARGS | METH_KEYWORDS, getaddrinfo_doc},
#endif
#ifdef HAVE_GETNAMEINFO
    {"getnameinfo",             socket_getnameinfo,
     METH_VARARGS, getnameinfo_doc},
#endif
    {"getdefaulttimeout",       socket_getdefaulttimeout,
     METH_NOARGS, getdefaulttimeout_doc},
    {"setdefaulttimeout",       socket_setdefaulttimeout,
     METH_O, setdefaulttimeout_doc},
#if defined(HAVE_IF_NAMEINDEX) || defined(MS_WINDOWS)
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



#ifndef OS_INIT_DEFINED
static int
os_init(void)
{
    return 1; /* Success */
}
#endif

static void
sock_free_api(PySocketModule_APIObject *capi)
{
    Py_DECREF(capi->Sock_Type);
    Py_DECREF(capi->error);
    Py_DECREF(capi->timeout_error);
    PyMem_Free(capi);
}

static void
sock_destroy_api(PyObject *capsule)
{
    void *capi = PyCapsule_GetPointer(capsule, PySocket_CAPSULE_NAME);
    sock_free_api(capi);
}

static PySocketModule_APIObject *
sock_get_api(socket_state *state)
{
    PySocketModule_APIObject *capi = PyMem_Malloc(sizeof(PySocketModule_APIObject));
    if (capi == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    capi->Sock_Type = (PyTypeObject *)Py_NewRef(state->sock_type);
    capi->error = Py_NewRef(PyExc_OSError);
    capi->timeout_error = Py_NewRef(PyExc_TimeoutError);
    return capi;
}


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

static int
socket_exec(PyObject *m)
{
    if (!os_init()) {
        goto error;
    }

    socket_state *state = get_module_state(m);
    state->defaulttimeout = _PYTIME_FROMSECONDS(-1);

#if defined(HAVE_ACCEPT) || defined(HAVE_ACCEPT4)
#if defined(HAVE_ACCEPT4) && defined(SOCK_CLOEXEC)
    state->accept4_works = -1;
#endif
#endif

#ifdef SOCK_CLOEXEC
    state->sock_cloexec_works = -1;
#endif

#define ADD_EXC(MOD, NAME, VAR, BASE) do {                  \
    VAR = PyErr_NewException("socket." NAME, BASE, NULL);   \
    if (VAR == NULL) {                                      \
        goto error;                                         \
    }                                                       \
    if (PyModule_AddObjectRef(MOD, NAME, VAR) < 0) {        \
        goto error;                                         \
    }                                                       \
} while (0)

    ADD_EXC(m, "herror", state->socket_herror, PyExc_OSError);
    ADD_EXC(m, "gaierror", state->socket_gaierror, PyExc_OSError);

#undef ADD_EXC

    if (PyModule_AddObjectRef(m, "error", PyExc_OSError) < 0) {
        goto error;
    }
    if (PyModule_AddObjectRef(m, "timeout", PyExc_TimeoutError) < 0) {
        goto error;
    }

    PyObject *sock_type = PyType_FromMetaclass(NULL, m, &sock_spec, NULL);
    if (sock_type == NULL) {
        goto error;
    }
    state->sock_type = (PyTypeObject *)sock_type;
    if (PyModule_AddObjectRef(m, "SocketType", sock_type) < 0) {
        goto error;
    }
    if (PyModule_AddType(m, state->sock_type) < 0) {
        goto error;
    }

    PyObject *has_ipv6;
#ifdef ENABLE_IPV6
    has_ipv6 = Py_True;
#else
    has_ipv6 = Py_False;
#endif
    if (PyModule_AddObjectRef(m, "has_ipv6", has_ipv6) < 0) {
        goto error;
    }

    /* Export C API */
    PySocketModule_APIObject *capi = sock_get_api(state);
    if (capi == NULL) {
        goto error;
    }
    PyObject *capsule = PyCapsule_New(capi,
                                      PySocket_CAPSULE_NAME,
                                      sock_destroy_api);
    if (capsule == NULL) {
        sock_free_api(capi);
        goto error;
    }
    int rc = PyModule_AddObjectRef(m, PySocket_CAPI_NAME, capsule);
    Py_DECREF(capsule);
    if (rc < 0) {
        goto error;
    }

#define ADD_INT_MACRO(MOD, INT) do {                    \
    if (PyModule_AddIntConstant(MOD, #INT, INT) < 0) {  \
        goto error;                                     \
    }                                                   \
} while (0)

#define ADD_INT_CONST(MOD, NAME, INT) do {              \
    if (PyModule_AddIntConstant(MOD, NAME, INT) < 0) {  \
        goto error;                                     \
    }                                                   \
} while (0)

#define ADD_STR_CONST(MOD, NAME, STR) do {                  \
    if (PyModule_AddStringConstant(MOD, NAME, STR) < 0) {   \
        goto error;                                         \
    }                                                       \
} while (0)

    /* Address families (we only support AF_INET and AF_UNIX) */
#ifdef AF_UNSPEC
    ADD_INT_MACRO(m, AF_UNSPEC);
#endif
    ADD_INT_MACRO(m, AF_INET);
#if defined(AF_UNIX)
    ADD_INT_MACRO(m, AF_UNIX);
#endif /* AF_UNIX */
#ifdef AF_AX25
    /* Amateur Radio AX.25 */
    ADD_INT_MACRO(m, AF_AX25);
#endif
#ifdef AF_IPX
    ADD_INT_MACRO(m, AF_IPX); /* Novell IPX */
#endif
#ifdef AF_APPLETALK
    /* Appletalk DDP */
    ADD_INT_MACRO(m, AF_APPLETALK);
#endif
#ifdef AF_NETROM
    /* Amateur radio NetROM */
    ADD_INT_MACRO(m, AF_NETROM);
#endif
#ifdef AF_BRIDGE
    /* Multiprotocol bridge */
    ADD_INT_MACRO(m, AF_BRIDGE);
#endif
#ifdef AF_ATMPVC
    /* ATM PVCs */
    ADD_INT_MACRO(m, AF_ATMPVC);
#endif
#ifdef AF_AAL5
    /* Reserved for Werner's ATM */
    ADD_INT_MACRO(m, AF_AAL5);
#endif
#ifdef HAVE_SOCKADDR_ALG
    ADD_INT_MACRO(m, AF_ALG); /* Linux crypto */
#endif
#ifdef AF_X25
    /* Reserved for X.25 project */
    ADD_INT_MACRO(m, AF_X25);
#endif
#ifdef AF_INET6
    ADD_INT_MACRO(m, AF_INET6); /* IP version 6 */
#endif
#ifdef AF_ROSE
    /* Amateur Radio X.25 PLP */
    ADD_INT_MACRO(m, AF_ROSE);
#endif
#ifdef AF_DECnet
    /* Reserved for DECnet project */
    ADD_INT_MACRO(m, AF_DECnet);
#endif
#ifdef AF_NETBEUI
    /* Reserved for 802.2LLC project */
    ADD_INT_MACRO(m, AF_NETBEUI);
#endif
#ifdef AF_SECURITY
    /* Security callback pseudo AF */
    ADD_INT_MACRO(m, AF_SECURITY);
#endif
#ifdef AF_KEY
    /* PF_KEY key management API */
    ADD_INT_MACRO(m, AF_KEY);
#endif
#ifdef AF_NETLINK
    /*  */
    ADD_INT_MACRO(m, AF_NETLINK);
    ADD_INT_MACRO(m, NETLINK_ROUTE);
#ifdef NETLINK_SKIP
    ADD_INT_MACRO(m, NETLINK_SKIP);
#endif
#ifdef NETLINK_W1
    ADD_INT_MACRO(m, NETLINK_W1);
#endif
    ADD_INT_MACRO(m, NETLINK_USERSOCK);
    ADD_INT_MACRO(m, NETLINK_FIREWALL);
#ifdef NETLINK_TCPDIAG
    ADD_INT_MACRO(m, NETLINK_TCPDIAG);
#endif
#ifdef NETLINK_NFLOG
    ADD_INT_MACRO(m, NETLINK_NFLOG);
#endif
#ifdef NETLINK_XFRM
    ADD_INT_MACRO(m, NETLINK_XFRM);
#endif
#ifdef NETLINK_ARPD
    ADD_INT_MACRO(m, NETLINK_ARPD);
#endif
#ifdef NETLINK_ROUTE6
    ADD_INT_MACRO(m, NETLINK_ROUTE6);
#endif
    ADD_INT_MACRO(m, NETLINK_IP6_FW);
#ifdef NETLINK_DNRTMSG
    ADD_INT_MACRO(m, NETLINK_DNRTMSG);
#endif
#ifdef NETLINK_TAPBASE
    ADD_INT_MACRO(m, NETLINK_TAPBASE);
#endif
#ifdef NETLINK_CRYPTO
    ADD_INT_MACRO(m, NETLINK_CRYPTO);
#endif
#endif /* AF_NETLINK */

#ifdef AF_QIPCRTR
    /* Qualcomm IPCROUTER */
    ADD_INT_MACRO(m, AF_QIPCRTR);
#endif

#ifdef AF_VSOCK
    ADD_INT_CONST(m, "AF_VSOCK", AF_VSOCK);
    ADD_INT_CONST(m, "SO_VM_SOCKETS_BUFFER_SIZE", 0);
    ADD_INT_CONST(m, "SO_VM_SOCKETS_BUFFER_MIN_SIZE", 1);
    ADD_INT_CONST(m, "SO_VM_SOCKETS_BUFFER_MAX_SIZE", 2);
    ADD_INT_CONST(m, "VMADDR_CID_ANY", 0xffffffff);
    ADD_INT_CONST(m, "VMADDR_PORT_ANY", 0xffffffff);
    ADD_INT_CONST(m, "VMADDR_CID_HOST", 2);
    ADD_INT_CONST(m, "VM_SOCKETS_INVALID_VERSION", 0xffffffff);
    ADD_INT_CONST(m, "IOCTL_VM_SOCKETS_GET_LOCAL_CID",  _IO(7, 0xb9));
#endif

#ifdef AF_ROUTE
    /* Alias to emulate 4.4BSD */
    ADD_INT_MACRO(m, AF_ROUTE);
#endif
#ifdef AF_LINK
    ADD_INT_MACRO(m, AF_LINK);
#endif
#ifdef AF_ASH
    /* Ash */
    ADD_INT_MACRO(m, AF_ASH);
#endif
#ifdef AF_ECONET
    /* Acorn Econet */
    ADD_INT_MACRO(m, AF_ECONET);
#endif
#ifdef AF_ATMSVC
    /* ATM SVCs */
    ADD_INT_MACRO(m, AF_ATMSVC);
#endif
#ifdef AF_SNA
    /* Linux SNA Project (nutters!) */
    ADD_INT_MACRO(m, AF_SNA);
#endif
#ifdef AF_IRDA
    /* IRDA sockets */
    ADD_INT_MACRO(m, AF_IRDA);
#endif
#ifdef AF_PPPOX
    /* PPPoX sockets */
    ADD_INT_MACRO(m, AF_PPPOX);
#endif
#ifdef AF_WANPIPE
    /* Wanpipe API Sockets */
    ADD_INT_MACRO(m, AF_WANPIPE);
#endif
#ifdef AF_LLC
    /* Linux LLC */
    ADD_INT_MACRO(m, AF_LLC);
#endif
#ifdef HAVE_AF_HYPERV
    /* Hyper-V sockets */
    ADD_INT_MACRO(m, AF_HYPERV);

    /* for proto */
    ADD_INT_MACRO(m, HV_PROTOCOL_RAW);

    /* for setsockopt() */
    ADD_INT_MACRO(m, HVSOCKET_CONNECT_TIMEOUT);
    ADD_INT_MACRO(m, HVSOCKET_CONNECT_TIMEOUT_MAX);
    ADD_INT_MACRO(m, HVSOCKET_CONNECTED_SUSPEND);
    ADD_INT_MACRO(m, HVSOCKET_ADDRESS_FLAG_PASSTHRU);

    /* for bind() or connect() */
    ADD_STR_CONST(m, "HV_GUID_ZERO", "00000000-0000-0000-0000-000000000000");
    ADD_STR_CONST(m, "HV_GUID_WILDCARD", "00000000-0000-0000-0000-000000000000");
    ADD_STR_CONST(m, "HV_GUID_BROADCAST", "FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF");
    ADD_STR_CONST(m, "HV_GUID_CHILDREN", "90DB8B89-0D35-4F79-8CE9-49EA0AC8B7CD");
    ADD_STR_CONST(m, "HV_GUID_LOOPBACK", "E0E16197-DD56-4A10-9195-5EE7A155A838");
    ADD_STR_CONST(m, "HV_GUID_PARENT", "A42E7CDA-D03F-480C-9CC2-A4DE20ABB878");
#endif /* HAVE_AF_HYPERV */

#ifdef USE_BLUETOOTH
    ADD_INT_MACRO(m, AF_BLUETOOTH);
#ifdef BTPROTO_L2CAP
    ADD_INT_MACRO(m, BTPROTO_L2CAP);
#endif /* BTPROTO_L2CAP */
#ifdef BTPROTO_HCI
    ADD_INT_MACRO(m, BTPROTO_HCI);
    ADD_INT_MACRO(m, SOL_HCI);
#if !defined(__NetBSD__) && !defined(__DragonFly__)
    ADD_INT_MACRO(m, HCI_FILTER);
#if !defined(__FreeBSD__)
    ADD_INT_MACRO(m, HCI_TIME_STAMP);
    ADD_INT_MACRO(m, HCI_DATA_DIR);
#endif /* !__FreeBSD__ */
#endif /* !__NetBSD__ && !__DragonFly__ */
#endif /* BTPROTO_HCI */
#ifdef BTPROTO_RFCOMM
    ADD_INT_MACRO(m, BTPROTO_RFCOMM);
#endif /* BTPROTO_RFCOMM */
    ADD_STR_CONST(m, "BDADDR_ANY", "00:00:00:00:00:00");
    ADD_STR_CONST(m, "BDADDR_LOCAL", "00:00:00:FF:FF:FF");
#ifdef BTPROTO_SCO
    ADD_INT_MACRO(m, BTPROTO_SCO);
#endif /* BTPROTO_SCO */
#endif /* USE_BLUETOOTH */

#ifdef AF_CAN
    /* Controller Area Network */
    ADD_INT_MACRO(m, AF_CAN);
#endif
#ifdef PF_CAN
    /* Controller Area Network */
    ADD_INT_MACRO(m, PF_CAN);
#endif

/* Reliable Datagram Sockets */
#ifdef AF_RDS
    ADD_INT_MACRO(m, AF_RDS);
#endif
#ifdef PF_RDS
    ADD_INT_MACRO(m, PF_RDS);
#endif

/* Kernel event messages */
#ifdef PF_SYSTEM
    ADD_INT_MACRO(m, PF_SYSTEM);
#endif
#ifdef AF_SYSTEM
    ADD_INT_MACRO(m, AF_SYSTEM);
#endif

/* FreeBSD divert(4) */
#ifdef PF_DIVERT
    ADD_INT_MACRO(m, PF_DIVERT);
#endif
#ifdef AF_DIVERT
    ADD_INT_MACRO(m, AF_DIVERT);
#endif

#ifdef AF_PACKET
    ADD_INT_MACRO(m, AF_PACKET);
#endif
#ifdef PF_PACKET
    ADD_INT_MACRO(m, PF_PACKET);
#endif
#ifdef PACKET_HOST
    ADD_INT_MACRO(m, PACKET_HOST);
#endif
#ifdef PACKET_BROADCAST
    ADD_INT_MACRO(m, PACKET_BROADCAST);
#endif
#ifdef PACKET_MULTICAST
    ADD_INT_MACRO(m, PACKET_MULTICAST);
#endif
#ifdef PACKET_OTHERHOST
    ADD_INT_MACRO(m, PACKET_OTHERHOST);
#endif
#ifdef PACKET_OUTGOING
    ADD_INT_MACRO(m, PACKET_OUTGOING);
#endif
#ifdef PACKET_LOOPBACK
    ADD_INT_MACRO(m, PACKET_LOOPBACK);
#endif
#ifdef PACKET_FASTROUTE
    ADD_INT_MACRO(m, PACKET_FASTROUTE);
#endif

#ifdef HAVE_LINUX_TIPC_H
    ADD_INT_MACRO(m, AF_TIPC);

    /* for addresses */
    ADD_INT_MACRO(m, TIPC_ADDR_NAMESEQ);
    ADD_INT_MACRO(m, TIPC_ADDR_NAME);
    ADD_INT_MACRO(m, TIPC_ADDR_ID);

    ADD_INT_MACRO(m, TIPC_ZONE_SCOPE);
    ADD_INT_MACRO(m, TIPC_CLUSTER_SCOPE);
    ADD_INT_MACRO(m, TIPC_NODE_SCOPE);

    /* for setsockopt() */
    ADD_INT_MACRO(m, SOL_TIPC);
    ADD_INT_MACRO(m, TIPC_IMPORTANCE);
    ADD_INT_MACRO(m, TIPC_SRC_DROPPABLE);
    ADD_INT_MACRO(m, TIPC_DEST_DROPPABLE);
    ADD_INT_MACRO(m, TIPC_CONN_TIMEOUT);

    ADD_INT_MACRO(m, TIPC_LOW_IMPORTANCE);
    ADD_INT_MACRO(m, TIPC_MEDIUM_IMPORTANCE);
    ADD_INT_MACRO(m, TIPC_HIGH_IMPORTANCE);
    ADD_INT_MACRO(m, TIPC_CRITICAL_IMPORTANCE);

    /* for subscriptions */
    ADD_INT_MACRO(m, TIPC_SUB_PORTS);
    ADD_INT_MACRO(m, TIPC_SUB_SERVICE);
#ifdef TIPC_SUB_CANCEL
    /* doesn't seem to be available everywhere */
    ADD_INT_MACRO(m, TIPC_SUB_CANCEL);
#endif
    ADD_INT_MACRO(m, TIPC_WAIT_FOREVER);
    ADD_INT_MACRO(m, TIPC_PUBLISHED);
    ADD_INT_MACRO(m, TIPC_WITHDRAWN);
    ADD_INT_MACRO(m, TIPC_SUBSCR_TIMEOUT);
    ADD_INT_MACRO(m, TIPC_CFG_SRV);
    ADD_INT_MACRO(m, TIPC_TOP_SRV);
#endif

#ifdef HAVE_SOCKADDR_ALG
    /* Socket options */
    ADD_INT_MACRO(m, ALG_SET_KEY);
    ADD_INT_MACRO(m, ALG_SET_IV);
    ADD_INT_MACRO(m, ALG_SET_OP);
    ADD_INT_MACRO(m, ALG_SET_AEAD_ASSOCLEN);
    ADD_INT_MACRO(m, ALG_SET_AEAD_AUTHSIZE);
    ADD_INT_MACRO(m, ALG_SET_PUBKEY);

    /* Operations */
    ADD_INT_MACRO(m, ALG_OP_DECRYPT);
    ADD_INT_MACRO(m, ALG_OP_ENCRYPT);
    ADD_INT_MACRO(m, ALG_OP_SIGN);
    ADD_INT_MACRO(m, ALG_OP_VERIFY);
#endif

/* IEEE 802.3 protocol numbers required for a standard TCP/IP network stack */
#ifdef ETHERTYPE_ARP
    ADD_INT_MACRO(m, ETHERTYPE_ARP);
#endif
#ifdef ETHERTYPE_IP
    ADD_INT_MACRO(m, ETHERTYPE_IP);
#endif
#ifdef ETHERTYPE_IPV6
    ADD_INT_MACRO(m, ETHERTYPE_IPV6);
#endif
#ifdef ETHERTYPE_VLAN
    ADD_INT_MACRO(m, ETHERTYPE_VLAN);
#endif

/* Linux pseudo-protocol for sniffing every packet */
#ifdef ETH_P_ALL
    ADD_INT_MACRO(m, ETH_P_ALL);
#endif

    /* Socket types */
    ADD_INT_MACRO(m, SOCK_STREAM);
    ADD_INT_MACRO(m, SOCK_DGRAM);
/* We have incomplete socket support. */
#ifdef SOCK_RAW
    /* SOCK_RAW is marked as optional in the POSIX specification */
    ADD_INT_MACRO(m, SOCK_RAW);
#endif
#ifdef SOCK_SEQPACKET
    ADD_INT_MACRO(m, SOCK_SEQPACKET);
#endif
#if defined(SOCK_RDM)
    ADD_INT_MACRO(m, SOCK_RDM);
#endif
#ifdef SOCK_CLOEXEC
    ADD_INT_MACRO(m, SOCK_CLOEXEC);
#endif
#ifdef SOCK_NONBLOCK
    ADD_INT_MACRO(m, SOCK_NONBLOCK);
#endif

#ifdef  SO_DEBUG
    ADD_INT_MACRO(m, SO_DEBUG);
#endif
#ifdef  SO_ACCEPTCONN
    ADD_INT_MACRO(m, SO_ACCEPTCONN);
#endif
#ifdef  SO_REUSEADDR
    ADD_INT_MACRO(m, SO_REUSEADDR);
#endif
#ifdef SO_EXCLUSIVEADDRUSE
    ADD_INT_MACRO(m, SO_EXCLUSIVEADDRUSE);
#endif
#ifdef SO_INCOMING_CPU
    ADD_INT_MACRO(m, SO_INCOMING_CPU);
#endif

#ifdef  SO_KEEPALIVE
    ADD_INT_MACRO(m, SO_KEEPALIVE);
#endif
#ifdef  SO_DONTROUTE
    ADD_INT_MACRO(m, SO_DONTROUTE);
#endif
#ifdef  SO_BROADCAST
    ADD_INT_MACRO(m, SO_BROADCAST);
#endif
#ifdef  SO_USELOOPBACK
    ADD_INT_MACRO(m, SO_USELOOPBACK);
#endif
#ifdef  SO_LINGER
    ADD_INT_MACRO(m, SO_LINGER);
#endif
#ifdef  SO_OOBINLINE
    ADD_INT_MACRO(m, SO_OOBINLINE);
#endif
#ifndef __GNU__
#ifdef  SO_REUSEPORT
    ADD_INT_MACRO(m, SO_REUSEPORT);
#endif
#endif
#ifdef  SO_SNDBUF
    ADD_INT_MACRO(m, SO_SNDBUF);
#endif
#ifdef  SO_RCVBUF
    ADD_INT_MACRO(m, SO_RCVBUF);
#endif
#ifdef  SO_SNDLOWAT
    ADD_INT_MACRO(m, SO_SNDLOWAT);
#endif
#ifdef  SO_RCVLOWAT
    ADD_INT_MACRO(m, SO_RCVLOWAT);
#endif
#ifdef  SO_SNDTIMEO
    ADD_INT_MACRO(m, SO_SNDTIMEO);
#endif
#ifdef  SO_RCVTIMEO
    ADD_INT_MACRO(m, SO_RCVTIMEO);
#endif
#ifdef  SO_ERROR
    ADD_INT_MACRO(m, SO_ERROR);
#endif
#ifdef  SO_TYPE
    ADD_INT_MACRO(m, SO_TYPE);
#endif
#ifdef  SO_SETFIB
    ADD_INT_MACRO(m, SO_SETFIB);
#endif
#ifdef  SO_PASSCRED
    ADD_INT_MACRO(m, SO_PASSCRED);
#endif
#ifdef  SO_PEERCRED
    ADD_INT_MACRO(m, SO_PEERCRED);
#endif
#ifdef  LOCAL_PEERCRED
    ADD_INT_MACRO(m, LOCAL_PEERCRED);
#endif
#ifdef  SO_PASSSEC
    ADD_INT_MACRO(m, SO_PASSSEC);
#endif
#ifdef  SO_PEERSEC
    ADD_INT_MACRO(m, SO_PEERSEC);
#endif
#ifdef  SO_BINDTODEVICE
    ADD_INT_MACRO(m, SO_BINDTODEVICE);
#endif
#ifdef  SO_PRIORITY
    ADD_INT_MACRO(m, SO_PRIORITY);
#endif
#ifdef  SO_MARK
    ADD_INT_MACRO(m, SO_MARK);
#endif
#ifdef  SO_USER_COOKIE
    ADD_INT_MACRO(m, SO_USER_COOKIE);
#endif
#ifdef  SO_RTABLE
    ADD_INT_MACRO(m, SO_RTABLE);
#endif
#ifdef SO_DOMAIN
    ADD_INT_MACRO(m, SO_DOMAIN);
#endif
#ifdef SO_PROTOCOL
    ADD_INT_MACRO(m, SO_PROTOCOL);
#endif
#ifdef LOCAL_CREDS
    ADD_INT_MACRO(m, LOCAL_CREDS);
#endif
#ifdef LOCAL_CREDS_PERSISTENT
    ADD_INT_MACRO(m, LOCAL_CREDS_PERSISTENT);
#endif

    /* Maximum number of connections for "listen" */
#ifdef  SOMAXCONN
    ADD_INT_MACRO(m, SOMAXCONN);
#else
    ADD_INT_CONST(m, "SOMAXCONN", 5); /* Common value */
#endif

    /* Ancillary message types */
#ifdef  SCM_RIGHTS
    ADD_INT_MACRO(m, SCM_RIGHTS);
#endif
#ifdef  SCM_CREDENTIALS
    ADD_INT_MACRO(m, SCM_CREDENTIALS);
#endif
#ifdef  SCM_CREDS
    ADD_INT_MACRO(m, SCM_CREDS);
#endif
#ifdef  SCM_CREDS2
    ADD_INT_MACRO(m, SCM_CREDS2);
#endif

    /* Flags for send, recv */
#ifdef  MSG_OOB
    ADD_INT_MACRO(m, MSG_OOB);
#endif
#ifdef  MSG_PEEK
    ADD_INT_MACRO(m, MSG_PEEK);
#endif
#ifdef  MSG_DONTROUTE
    ADD_INT_MACRO(m, MSG_DONTROUTE);
#endif
#ifdef  MSG_DONTWAIT
    ADD_INT_MACRO(m, MSG_DONTWAIT);
#endif
#ifdef  MSG_EOR
    ADD_INT_MACRO(m, MSG_EOR);
#endif
#ifdef  MSG_TRUNC
    // workaround for https://github.com/WebAssembly/wasi-libc/issues/305
    #if defined(__wasi__) && !defined(__WASI_RIFLAGS_RECV_DATA_TRUNCATED)
    #  define __WASI_RIFLAGS_RECV_DATA_TRUNCATED 2
    #endif
    ADD_INT_MACRO(m, MSG_TRUNC);
#endif
#ifdef  MSG_CTRUNC
    ADD_INT_MACRO(m, MSG_CTRUNC);
#endif
#ifdef  MSG_WAITALL
    ADD_INT_MACRO(m, MSG_WAITALL);
#endif
#ifdef  MSG_BTAG
    ADD_INT_MACRO(m, MSG_BTAG);
#endif
#ifdef  MSG_ETAG
    ADD_INT_MACRO(m, MSG_ETAG);
#endif
#ifdef  MSG_NOSIGNAL
    ADD_INT_MACRO(m, MSG_NOSIGNAL);
#endif
#ifdef  MSG_NOTIFICATION
    ADD_INT_MACRO(m, MSG_NOTIFICATION);
#endif
#ifdef  MSG_CMSG_CLOEXEC
    ADD_INT_MACRO(m, MSG_CMSG_CLOEXEC);
#endif
#ifdef  MSG_ERRQUEUE
    ADD_INT_MACRO(m, MSG_ERRQUEUE);
#endif
#ifdef  MSG_CONFIRM
    ADD_INT_MACRO(m, MSG_CONFIRM);
#endif
#ifdef  MSG_MORE
    ADD_INT_MACRO(m, MSG_MORE);
#endif
#ifdef  MSG_EOF
    ADD_INT_MACRO(m, MSG_EOF);
#endif
#ifdef  MSG_BCAST
    ADD_INT_MACRO(m, MSG_BCAST);
#endif
#ifdef  MSG_MCAST
    ADD_INT_MACRO(m, MSG_MCAST);
#endif
#ifdef MSG_FASTOPEN
    ADD_INT_MACRO(m, MSG_FASTOPEN);
#endif

    /* Protocol level and numbers, usable for [gs]etsockopt */
#ifdef  SOL_SOCKET
    ADD_INT_MACRO(m, SOL_SOCKET);
#endif
#ifdef  SOL_IP
    ADD_INT_MACRO(m, SOL_IP);
#else
    ADD_INT_CONST(m, "SOL_IP", 0);
#endif
#ifdef  SOL_IPX
    ADD_INT_MACRO(m, SOL_IPX);
#endif
#ifdef  SOL_AX25
    ADD_INT_MACRO(m, SOL_AX25);
#endif
#ifdef  SOL_ATALK
    ADD_INT_MACRO(m, SOL_ATALK);
#endif
#ifdef  SOL_NETROM
    ADD_INT_MACRO(m, SOL_NETROM);
#endif
#ifdef  SOL_ROSE
    ADD_INT_MACRO(m, SOL_ROSE);
#endif
#ifdef  SOL_TCP
    ADD_INT_MACRO(m, SOL_TCP);
#else
    ADD_INT_CONST(m, "SOL_TCP", 6);
#endif
#ifdef  SOL_UDP
    ADD_INT_MACRO(m, SOL_UDP);
#else
    ADD_INT_CONST(m, "SOL_UDP", 17);
#endif
#ifdef SOL_CAN_BASE
    ADD_INT_MACRO(m, SOL_CAN_BASE);
#endif
#ifdef SOL_CAN_RAW
    ADD_INT_MACRO(m, SOL_CAN_RAW);
    ADD_INT_MACRO(m, CAN_RAW);
#endif
#if defined(HAVE_LINUX_CAN_H) || defined(HAVE_NETCAN_CAN_H)
    ADD_INT_MACRO(m, CAN_EFF_FLAG);
    ADD_INT_MACRO(m, CAN_RTR_FLAG);
    ADD_INT_MACRO(m, CAN_ERR_FLAG);

    ADD_INT_MACRO(m, CAN_SFF_MASK);
    ADD_INT_MACRO(m, CAN_EFF_MASK);
    ADD_INT_MACRO(m, CAN_ERR_MASK);
#ifdef CAN_ISOTP
    ADD_INT_MACRO(m, CAN_ISOTP);
#endif
#ifdef CAN_J1939
    ADD_INT_MACRO(m, CAN_J1939);
#endif
#endif
#if defined(HAVE_LINUX_CAN_RAW_H) || defined(HAVE_NETCAN_CAN_H)
    ADD_INT_MACRO(m, CAN_RAW_FILTER);
#ifdef CAN_RAW_ERR_FILTER
    ADD_INT_MACRO(m, CAN_RAW_ERR_FILTER);
#endif
    ADD_INT_MACRO(m, CAN_RAW_LOOPBACK);
    ADD_INT_MACRO(m, CAN_RAW_RECV_OWN_MSGS);
#endif
#ifdef HAVE_LINUX_CAN_RAW_FD_FRAMES
    ADD_INT_MACRO(m, CAN_RAW_FD_FRAMES);
#endif
#ifdef HAVE_LINUX_CAN_RAW_JOIN_FILTERS
    ADD_INT_MACRO(m, CAN_RAW_JOIN_FILTERS);
#endif
#ifdef HAVE_LINUX_CAN_BCM_H
    ADD_INT_MACRO(m, CAN_BCM);

    /* BCM opcodes */
    ADD_INT_CONST(m, "CAN_BCM_TX_SETUP", TX_SETUP);
    ADD_INT_CONST(m, "CAN_BCM_TX_DELETE", TX_DELETE);
    ADD_INT_CONST(m, "CAN_BCM_TX_READ", TX_READ);
    ADD_INT_CONST(m, "CAN_BCM_TX_SEND", TX_SEND);
    ADD_INT_CONST(m, "CAN_BCM_RX_SETUP", RX_SETUP);
    ADD_INT_CONST(m, "CAN_BCM_RX_DELETE", RX_DELETE);
    ADD_INT_CONST(m, "CAN_BCM_RX_READ", RX_READ);
    ADD_INT_CONST(m, "CAN_BCM_TX_STATUS", TX_STATUS);
    ADD_INT_CONST(m, "CAN_BCM_TX_EXPIRED", TX_EXPIRED);
    ADD_INT_CONST(m, "CAN_BCM_RX_STATUS", RX_STATUS);
    ADD_INT_CONST(m, "CAN_BCM_RX_TIMEOUT", RX_TIMEOUT);
    ADD_INT_CONST(m, "CAN_BCM_RX_CHANGED", RX_CHANGED);

    /* BCM flags */
    ADD_INT_CONST(m, "CAN_BCM_SETTIMER", SETTIMER);
    ADD_INT_CONST(m, "CAN_BCM_STARTTIMER", STARTTIMER);
    ADD_INT_CONST(m, "CAN_BCM_TX_COUNTEVT", TX_COUNTEVT);
    ADD_INT_CONST(m, "CAN_BCM_TX_ANNOUNCE", TX_ANNOUNCE);
    ADD_INT_CONST(m, "CAN_BCM_TX_CP_CAN_ID", TX_CP_CAN_ID);
    ADD_INT_CONST(m, "CAN_BCM_RX_FILTER_ID", RX_FILTER_ID);
    ADD_INT_CONST(m, "CAN_BCM_RX_CHECK_DLC", RX_CHECK_DLC);
    ADD_INT_CONST(m, "CAN_BCM_RX_NO_AUTOTIMER", RX_NO_AUTOTIMER);
    ADD_INT_CONST(m, "CAN_BCM_RX_ANNOUNCE_RESUME", RX_ANNOUNCE_RESUME);
    ADD_INT_CONST(m, "CAN_BCM_TX_RESET_MULTI_IDX", TX_RESET_MULTI_IDX);
    ADD_INT_CONST(m, "CAN_BCM_RX_RTR_FRAME", RX_RTR_FRAME);
#ifdef CAN_FD_FRAME
    /* CAN_FD_FRAME was only introduced in the 4.8.x kernel series */
    ADD_INT_CONST(m, "CAN_BCM_CAN_FD_FRAME", CAN_FD_FRAME);
#endif
#endif
#ifdef HAVE_LINUX_CAN_J1939_H
    ADD_INT_MACRO(m, J1939_MAX_UNICAST_ADDR);
    ADD_INT_MACRO(m, J1939_IDLE_ADDR);
    ADD_INT_MACRO(m, J1939_NO_ADDR);
    ADD_INT_MACRO(m, J1939_NO_NAME);
    ADD_INT_MACRO(m, J1939_PGN_REQUEST);
    ADD_INT_MACRO(m, J1939_PGN_ADDRESS_CLAIMED);
    ADD_INT_MACRO(m, J1939_PGN_ADDRESS_COMMANDED);
    ADD_INT_MACRO(m, J1939_PGN_PDU1_MAX);
    ADD_INT_MACRO(m, J1939_PGN_MAX);
    ADD_INT_MACRO(m, J1939_NO_PGN);

    /* J1939 socket options */
    ADD_INT_MACRO(m, SO_J1939_FILTER);
    ADD_INT_MACRO(m, SO_J1939_PROMISC);
    ADD_INT_MACRO(m, SO_J1939_SEND_PRIO);
    ADD_INT_MACRO(m, SO_J1939_ERRQUEUE);

    ADD_INT_MACRO(m, SCM_J1939_DEST_ADDR);
    ADD_INT_MACRO(m, SCM_J1939_DEST_NAME);
    ADD_INT_MACRO(m, SCM_J1939_PRIO);
    ADD_INT_MACRO(m, SCM_J1939_ERRQUEUE);

    ADD_INT_MACRO(m, J1939_NLA_PAD);
    ADD_INT_MACRO(m, J1939_NLA_BYTES_ACKED);

    ADD_INT_MACRO(m, J1939_EE_INFO_NONE);
    ADD_INT_MACRO(m, J1939_EE_INFO_TX_ABORT);

    ADD_INT_MACRO(m, J1939_FILTER_MAX);
#endif
#ifdef SOL_RDS
    ADD_INT_MACRO(m, SOL_RDS);
#endif
#ifdef HAVE_SOCKADDR_ALG
    ADD_INT_MACRO(m, SOL_ALG);
#endif
#ifdef RDS_CANCEL_SENT_TO
    ADD_INT_MACRO(m, RDS_CANCEL_SENT_TO);
#endif
#ifdef RDS_GET_MR
    ADD_INT_MACRO(m, RDS_GET_MR);
#endif
#ifdef RDS_FREE_MR
    ADD_INT_MACRO(m, RDS_FREE_MR);
#endif
#ifdef RDS_RECVERR
    ADD_INT_MACRO(m, RDS_RECVERR);
#endif
#ifdef RDS_CONG_MONITOR
    ADD_INT_MACRO(m, RDS_CONG_MONITOR);
#endif
#ifdef RDS_GET_MR_FOR_DEST
    ADD_INT_MACRO(m, RDS_GET_MR_FOR_DEST);
#endif
#ifdef  IPPROTO_IP
    ADD_INT_MACRO(m, IPPROTO_IP);
#else
    ADD_INT_CONST(m, "IPPROTO_IP", 0);
#endif
#ifdef  IPPROTO_HOPOPTS
    ADD_INT_MACRO(m, IPPROTO_HOPOPTS);
#endif
#ifdef  IPPROTO_ICMP
    ADD_INT_MACRO(m, IPPROTO_ICMP);
#else
    ADD_INT_CONST(m, "IPPROTO_ICMP", 1);
#endif
#ifdef  IPPROTO_IGMP
    ADD_INT_MACRO(m, IPPROTO_IGMP);
#endif
#ifdef  IPPROTO_GGP
    ADD_INT_MACRO(m, IPPROTO_GGP);
#endif
#ifdef  IPPROTO_IPV4
    ADD_INT_MACRO(m, IPPROTO_IPV4);
#endif
#ifdef  IPPROTO_IPV6
    ADD_INT_MACRO(m, IPPROTO_IPV6);
#endif
#ifdef  IPPROTO_IPIP
    ADD_INT_MACRO(m, IPPROTO_IPIP);
#endif
#ifdef  IPPROTO_TCP
    ADD_INT_MACRO(m, IPPROTO_TCP);
#else
    ADD_INT_CONST(m, "IPPROTO_TCP", 6);
#endif
#ifdef  IPPROTO_EGP
    ADD_INT_MACRO(m, IPPROTO_EGP);
#endif
#ifdef  IPPROTO_PUP
    ADD_INT_MACRO(m, IPPROTO_PUP);
#endif
#ifdef  IPPROTO_UDP
    ADD_INT_MACRO(m, IPPROTO_UDP);
#else
    ADD_INT_CONST(m, "IPPROTO_UDP", 17);
#endif
#ifdef  IPPROTO_UDPLITE
    ADD_INT_MACRO(m, IPPROTO_UDPLITE);
    #ifndef UDPLITE_SEND_CSCOV
        #define UDPLITE_SEND_CSCOV 10
    #endif
    ADD_INT_MACRO(m, UDPLITE_SEND_CSCOV);
    #ifndef UDPLITE_RECV_CSCOV
        #define UDPLITE_RECV_CSCOV 11
    #endif
    ADD_INT_MACRO(m, UDPLITE_RECV_CSCOV);
#endif
#ifdef  IPPROTO_IDP
    ADD_INT_MACRO(m, IPPROTO_IDP);
#endif
#ifdef  IPPROTO_HELLO
    ADD_INT_MACRO(m, IPPROTO_HELLO);
#endif
#ifdef  IPPROTO_ND
    ADD_INT_MACRO(m, IPPROTO_ND);
#endif
#ifdef  IPPROTO_TP
    ADD_INT_MACRO(m, IPPROTO_TP);
#endif
#ifdef  IPPROTO_ROUTING
    ADD_INT_MACRO(m, IPPROTO_ROUTING);
#endif
#ifdef  IPPROTO_FRAGMENT
    ADD_INT_MACRO(m, IPPROTO_FRAGMENT);
#endif
#ifdef  IPPROTO_RSVP
    ADD_INT_MACRO(m, IPPROTO_RSVP);
#endif
#ifdef  IPPROTO_GRE
    ADD_INT_MACRO(m, IPPROTO_GRE);
#endif
#ifdef  IPPROTO_ESP
    ADD_INT_MACRO(m, IPPROTO_ESP);
#endif
#ifdef  IPPROTO_AH
    ADD_INT_MACRO(m, IPPROTO_AH);
#endif
#ifdef  IPPROTO_MOBILE
    ADD_INT_MACRO(m, IPPROTO_MOBILE);
#endif
#ifdef  IPPROTO_ICMPV6
    ADD_INT_MACRO(m, IPPROTO_ICMPV6);
#endif
#ifdef  IPPROTO_NONE
    ADD_INT_MACRO(m, IPPROTO_NONE);
#endif
#ifdef  IPPROTO_DSTOPTS
    ADD_INT_MACRO(m, IPPROTO_DSTOPTS);
#endif
#ifdef  IPPROTO_XTP
    ADD_INT_MACRO(m, IPPROTO_XTP);
#endif
#ifdef  IPPROTO_EON
    ADD_INT_MACRO(m, IPPROTO_EON);
#endif
#ifdef  IPPROTO_PIM
    ADD_INT_MACRO(m, IPPROTO_PIM);
#endif
#ifdef  IPPROTO_IPCOMP
    ADD_INT_MACRO(m, IPPROTO_IPCOMP);
#endif
#ifdef  IPPROTO_VRRP
    ADD_INT_MACRO(m, IPPROTO_VRRP);
#endif
#ifdef  IPPROTO_SCTP
    ADD_INT_MACRO(m, IPPROTO_SCTP);
#endif
#ifdef  IPPROTO_BIP
    ADD_INT_MACRO(m, IPPROTO_BIP);
#endif
#ifdef  IPPROTO_MPTCP
    ADD_INT_MACRO(m, IPPROTO_MPTCP);
#endif
/**/
#ifdef  IPPROTO_RAW
    ADD_INT_MACRO(m, IPPROTO_RAW);
#else
    ADD_INT_CONST(m, "IPPROTO_RAW", 255);
#endif
#ifdef  IPPROTO_MAX
    ADD_INT_MACRO(m, IPPROTO_MAX);
#endif

#ifdef  MS_WINDOWS
    ADD_INT_MACRO(m, IPPROTO_ICLFXBM);
    ADD_INT_MACRO(m, IPPROTO_ST);
    ADD_INT_MACRO(m, IPPROTO_CBT);
    ADD_INT_MACRO(m, IPPROTO_IGP);
    ADD_INT_MACRO(m, IPPROTO_RDP);
    ADD_INT_MACRO(m, IPPROTO_PGM);
    ADD_INT_MACRO(m, IPPROTO_L2TP);
    ADD_INT_MACRO(m, IPPROTO_SCTP);
#endif

#ifdef  SYSPROTO_CONTROL
    ADD_INT_MACRO(m, SYSPROTO_CONTROL);
#endif

    /* Some port configuration */
#ifdef  IPPORT_RESERVED
    ADD_INT_MACRO(m, IPPORT_RESERVED);
#else
    ADD_INT_CONST(m, "IPPORT_RESERVED", 1024);
#endif
#ifdef  IPPORT_USERRESERVED
    ADD_INT_MACRO(m, IPPORT_USERRESERVED);
#else
    ADD_INT_CONST(m, "IPPORT_USERRESERVED", 5000);
#endif

    /* Some reserved IP v.4 addresses */
#ifdef  INADDR_ANY
    ADD_INT_MACRO(m, INADDR_ANY);
#else
    ADD_INT_CONST(m, "INADDR_ANY", 0x00000000);
#endif
#ifdef  INADDR_BROADCAST
    ADD_INT_MACRO(m, INADDR_BROADCAST);
#else
    ADD_INT_CONST(m, "INADDR_BROADCAST", 0xffffffff);
#endif
#ifdef  INADDR_LOOPBACK
    ADD_INT_MACRO(m, INADDR_LOOPBACK);
#else
    ADD_INT_CONST(m, "INADDR_LOOPBACK", 0x7F000001);
#endif
#ifdef  INADDR_UNSPEC_GROUP
    ADD_INT_MACRO(m, INADDR_UNSPEC_GROUP);
#else
    ADD_INT_CONST(m, "INADDR_UNSPEC_GROUP", 0xe0000000);
#endif
#ifdef  INADDR_ALLHOSTS_GROUP
    ADD_INT_CONST(m, "INADDR_ALLHOSTS_GROUP",
                            INADDR_ALLHOSTS_GROUP);
#else
    ADD_INT_CONST(m, "INADDR_ALLHOSTS_GROUP", 0xe0000001);
#endif
#ifdef  INADDR_MAX_LOCAL_GROUP
    ADD_INT_MACRO(m, INADDR_MAX_LOCAL_GROUP);
#else
    ADD_INT_CONST(m, "INADDR_MAX_LOCAL_GROUP", 0xe00000ff);
#endif
#ifdef  INADDR_NONE
    ADD_INT_MACRO(m, INADDR_NONE);
#else
    ADD_INT_CONST(m, "INADDR_NONE", 0xffffffff);
#endif

    /* IPv4 [gs]etsockopt options */
#ifdef  IP_OPTIONS
    ADD_INT_MACRO(m, IP_OPTIONS);
#endif
#ifdef  IP_HDRINCL
    ADD_INT_MACRO(m, IP_HDRINCL);
#endif
#ifdef  IP_TOS
    ADD_INT_MACRO(m, IP_TOS);
#endif
#ifdef  IP_TTL
    ADD_INT_MACRO(m, IP_TTL);
#endif
#ifdef  IP_RECVOPTS
    ADD_INT_MACRO(m, IP_RECVOPTS);
#endif
#ifdef  IP_RECVRETOPTS
    ADD_INT_MACRO(m, IP_RECVRETOPTS);
#endif
#ifdef  IP_RECVTOS
    ADD_INT_MACRO(m, IP_RECVTOS);
#endif
#ifdef  IP_RECVDSTADDR
    ADD_INT_MACRO(m, IP_RECVDSTADDR);
#endif
#ifdef  IP_RETOPTS
    ADD_INT_MACRO(m, IP_RETOPTS);
#endif
#ifdef  IP_MULTICAST_IF
    ADD_INT_MACRO(m, IP_MULTICAST_IF);
#endif
#ifdef  IP_MULTICAST_TTL
    ADD_INT_MACRO(m, IP_MULTICAST_TTL);
#endif
#ifdef  IP_MULTICAST_LOOP
    ADD_INT_MACRO(m, IP_MULTICAST_LOOP);
#endif
#ifdef  IP_ADD_MEMBERSHIP
    ADD_INT_MACRO(m, IP_ADD_MEMBERSHIP);
#endif
#ifdef  IP_DROP_MEMBERSHIP
    ADD_INT_MACRO(m, IP_DROP_MEMBERSHIP);
#endif
#ifdef  IP_DEFAULT_MULTICAST_TTL
    ADD_INT_MACRO(m, IP_DEFAULT_MULTICAST_TTL);
#endif
#ifdef  IP_DEFAULT_MULTICAST_LOOP
    ADD_INT_MACRO(m, IP_DEFAULT_MULTICAST_LOOP);
#endif
#ifdef  IP_MAX_MEMBERSHIPS
    ADD_INT_MACRO(m, IP_MAX_MEMBERSHIPS);
#endif
#ifdef  IP_TRANSPARENT
    ADD_INT_MACRO(m, IP_TRANSPARENT);
#endif
#ifdef  IP_PKTINFO
    ADD_INT_MACRO(m, IP_PKTINFO);
#endif
#ifdef IP_BIND_ADDRESS_NO_PORT
    ADD_INT_MACRO(m, IP_BIND_ADDRESS_NO_PORT);
#endif
#ifdef IP_UNBLOCK_SOURCE
    ADD_INT_MACRO(m, IP_UNBLOCK_SOURCE);
#endif
#ifdef IP_BLOCK_SOURCE
    ADD_INT_MACRO(m, IP_BLOCK_SOURCE);
#endif
#ifdef IP_ADD_SOURCE_MEMBERSHIP
    ADD_INT_MACRO(m, IP_ADD_SOURCE_MEMBERSHIP);
#endif
#ifdef IP_DROP_SOURCE_MEMBERSHIP
    ADD_INT_MACRO(m, IP_DROP_SOURCE_MEMBERSHIP);
#endif

    /* IPv6 [gs]etsockopt options, defined in RFC2553 */
#ifdef  IPV6_JOIN_GROUP
    ADD_INT_MACRO(m, IPV6_JOIN_GROUP);
#endif
#ifdef  IPV6_LEAVE_GROUP
    ADD_INT_MACRO(m, IPV6_LEAVE_GROUP);
#endif
#ifdef  IPV6_MULTICAST_HOPS
    ADD_INT_MACRO(m, IPV6_MULTICAST_HOPS);
#endif
#ifdef  IPV6_MULTICAST_IF
    ADD_INT_MACRO(m, IPV6_MULTICAST_IF);
#endif
#ifdef  IPV6_MULTICAST_LOOP
    ADD_INT_MACRO(m, IPV6_MULTICAST_LOOP);
#endif
#ifdef  IPV6_UNICAST_HOPS
    ADD_INT_MACRO(m, IPV6_UNICAST_HOPS);
#endif
    /* Additional IPV6 socket options, defined in RFC 3493 */
#ifdef IPV6_V6ONLY
    ADD_INT_MACRO(m, IPV6_V6ONLY);
#endif
    /* Advanced IPV6 socket options, from RFC 3542 */
#ifdef IPV6_CHECKSUM
    ADD_INT_MACRO(m, IPV6_CHECKSUM);
#endif
#ifdef IPV6_DONTFRAG
    ADD_INT_MACRO(m, IPV6_DONTFRAG);
#endif
#ifdef IPV6_DSTOPTS
    ADD_INT_MACRO(m, IPV6_DSTOPTS);
#endif
#ifdef IPV6_HOPLIMIT
    ADD_INT_MACRO(m, IPV6_HOPLIMIT);
#endif
#ifdef IPV6_HOPOPTS
    ADD_INT_MACRO(m, IPV6_HOPOPTS);
#endif
#ifdef IPV6_NEXTHOP
    ADD_INT_MACRO(m, IPV6_NEXTHOP);
#endif
#ifdef IPV6_PATHMTU
    ADD_INT_MACRO(m, IPV6_PATHMTU);
#endif
#ifdef IPV6_PKTINFO
    ADD_INT_MACRO(m, IPV6_PKTINFO);
#endif
#ifdef IPV6_RECVDSTOPTS
    ADD_INT_MACRO(m, IPV6_RECVDSTOPTS);
#endif
#ifdef IPV6_RECVHOPLIMIT
    ADD_INT_MACRO(m, IPV6_RECVHOPLIMIT);
#endif
#ifdef IPV6_RECVHOPOPTS
    ADD_INT_MACRO(m, IPV6_RECVHOPOPTS);
#endif
#ifdef IPV6_RECVPKTINFO
    ADD_INT_MACRO(m, IPV6_RECVPKTINFO);
#endif
#ifdef IPV6_RECVRTHDR
    ADD_INT_MACRO(m, IPV6_RECVRTHDR);
#endif
#ifdef IPV6_RECVTCLASS
    ADD_INT_MACRO(m, IPV6_RECVTCLASS);
#endif
#ifdef IPV6_RTHDR
    ADD_INT_MACRO(m, IPV6_RTHDR);
#endif
#ifdef IPV6_RTHDRDSTOPTS
    ADD_INT_MACRO(m, IPV6_RTHDRDSTOPTS);
#endif
#ifdef IPV6_RTHDR_TYPE_0
    ADD_INT_MACRO(m, IPV6_RTHDR_TYPE_0);
#endif
#ifdef IPV6_RECVPATHMTU
    ADD_INT_MACRO(m, IPV6_RECVPATHMTU);
#endif
#ifdef IPV6_TCLASS
    ADD_INT_MACRO(m, IPV6_TCLASS);
#endif
#ifdef IPV6_USE_MIN_MTU
    ADD_INT_MACRO(m, IPV6_USE_MIN_MTU);
#endif

    /* TCP options */
#ifdef  TCP_NODELAY
    ADD_INT_MACRO(m, TCP_NODELAY);
#endif
#ifdef  TCP_MAXSEG
    ADD_INT_MACRO(m, TCP_MAXSEG);
#endif
#ifdef  TCP_CORK
    ADD_INT_MACRO(m, TCP_CORK);
#endif
#ifdef  TCP_KEEPIDLE
    ADD_INT_MACRO(m, TCP_KEEPIDLE);
#endif
    /* TCP_KEEPALIVE is OSX's TCP_KEEPIDLE equivalent */
#if defined(__APPLE__) && defined(TCP_KEEPALIVE)
    ADD_INT_MACRO(m, TCP_KEEPALIVE);
#endif
#ifdef  TCP_KEEPINTVL
    ADD_INT_MACRO(m, TCP_KEEPINTVL);
#endif
#ifdef  TCP_KEEPCNT
    ADD_INT_MACRO(m, TCP_KEEPCNT);
#endif
#ifdef  TCP_SYNCNT
    ADD_INT_MACRO(m, TCP_SYNCNT);
#endif
#ifdef  TCP_LINGER2
    ADD_INT_MACRO(m, TCP_LINGER2);
#endif
#ifdef  TCP_DEFER_ACCEPT
    ADD_INT_MACRO(m, TCP_DEFER_ACCEPT);
#endif
#ifdef  TCP_WINDOW_CLAMP
    ADD_INT_MACRO(m, TCP_WINDOW_CLAMP);
#endif
#ifdef  TCP_INFO
    ADD_INT_MACRO(m, TCP_INFO);
#endif
#ifdef  TCP_CONNECTION_INFO
    ADD_INT_MACRO(m, TCP_CONNECTION_INFO);
#endif
#ifdef  TCP_QUICKACK
    ADD_INT_MACRO(m, TCP_QUICKACK);
#endif
#ifdef  TCP_CONGESTION
    ADD_INT_MACRO(m, TCP_CONGESTION);
#endif
#ifdef  TCP_MD5SIG
    ADD_INT_MACRO(m, TCP_MD5SIG);
#endif
#ifdef  TCP_THIN_LINEAR_TIMEOUTS
    ADD_INT_MACRO(m, TCP_THIN_LINEAR_TIMEOUTS);
#endif
#ifdef  TCP_THIN_DUPACK
    ADD_INT_MACRO(m, TCP_THIN_DUPACK);
#endif
#ifdef  TCP_USER_TIMEOUT
    ADD_INT_MACRO(m, TCP_USER_TIMEOUT);
#endif
#ifdef  TCP_REPAIR
    ADD_INT_MACRO(m, TCP_REPAIR);
#endif
#ifdef  TCP_REPAIR_QUEUE
    ADD_INT_MACRO(m, TCP_REPAIR_QUEUE);
#endif
#ifdef  TCP_QUEUE_SEQ
    ADD_INT_MACRO(m, TCP_QUEUE_SEQ);
#endif
#ifdef  TCP_REPAIR_OPTIONS
    ADD_INT_MACRO(m, TCP_REPAIR_OPTIONS);
#endif
#ifdef  TCP_FASTOPEN
    ADD_INT_MACRO(m, TCP_FASTOPEN);
#endif
#ifdef  TCP_TIMESTAMP
    ADD_INT_MACRO(m, TCP_TIMESTAMP);
#endif
#ifdef  TCP_NOTSENT_LOWAT
    ADD_INT_MACRO(m, TCP_NOTSENT_LOWAT);
#endif
#ifdef  TCP_CC_INFO
    ADD_INT_MACRO(m, TCP_CC_INFO);
#endif
#ifdef  TCP_SAVE_SYN
    ADD_INT_MACRO(m, TCP_SAVE_SYN);
#endif
#ifdef  TCP_SAVED_SYN
    ADD_INT_MACRO(m, TCP_SAVED_SYN);
#endif
#ifdef  TCP_REPAIR_WINDOW
    ADD_INT_MACRO(m, TCP_REPAIR_WINDOW);
#endif
#ifdef  TCP_FASTOPEN_CONNECT
    ADD_INT_MACRO(m, TCP_FASTOPEN_CONNECT);
#endif
#ifdef  TCP_ULP
    ADD_INT_MACRO(m, TCP_ULP);
#endif
#ifdef  TCP_MD5SIG_EXT
    ADD_INT_MACRO(m, TCP_MD5SIG_EXT);
#endif
#ifdef  TCP_FASTOPEN_KEY
    ADD_INT_MACRO(m, TCP_FASTOPEN_KEY);
#endif
#ifdef  TCP_FASTOPEN_NO_COOKIE
    ADD_INT_MACRO(m, TCP_FASTOPEN_NO_COOKIE);
#endif
#ifdef  TCP_ZEROCOPY_RECEIVE
    ADD_INT_MACRO(m, TCP_ZEROCOPY_RECEIVE);
#endif
#ifdef  TCP_INQ
    ADD_INT_MACRO(m, TCP_INQ);
#endif
#ifdef  TCP_TX_DELAY
    ADD_INT_MACRO(m, TCP_TX_DELAY);
#endif

    /* IPX options */
#ifdef  IPX_TYPE
    ADD_INT_MACRO(m, IPX_TYPE);
#endif

/* Reliable Datagram Sockets */
#ifdef RDS_CMSG_RDMA_ARGS
    ADD_INT_MACRO(m, RDS_CMSG_RDMA_ARGS);
#endif
#ifdef RDS_CMSG_RDMA_DEST
    ADD_INT_MACRO(m, RDS_CMSG_RDMA_DEST);
#endif
#ifdef RDS_CMSG_RDMA_MAP
    ADD_INT_MACRO(m, RDS_CMSG_RDMA_MAP);
#endif
#ifdef RDS_CMSG_RDMA_STATUS
    ADD_INT_MACRO(m, RDS_CMSG_RDMA_STATUS);
#endif
#ifdef RDS_CMSG_RDMA_UPDATE
    ADD_INT_MACRO(m, RDS_CMSG_RDMA_UPDATE);
#endif
#ifdef RDS_RDMA_READWRITE
    ADD_INT_MACRO(m, RDS_RDMA_READWRITE);
#endif
#ifdef RDS_RDMA_FENCE
    ADD_INT_MACRO(m, RDS_RDMA_FENCE);
#endif
#ifdef RDS_RDMA_INVALIDATE
    ADD_INT_MACRO(m, RDS_RDMA_INVALIDATE);
#endif
#ifdef RDS_RDMA_USE_ONCE
    ADD_INT_MACRO(m, RDS_RDMA_USE_ONCE);
#endif
#ifdef RDS_RDMA_DONTWAIT
    ADD_INT_MACRO(m, RDS_RDMA_DONTWAIT);
#endif
#ifdef RDS_RDMA_NOTIFY_ME
    ADD_INT_MACRO(m, RDS_RDMA_NOTIFY_ME);
#endif
#ifdef RDS_RDMA_SILENT
    ADD_INT_MACRO(m, RDS_RDMA_SILENT);
#endif

    /* get{addr,name}info parameters */
#ifdef EAI_ADDRFAMILY
    ADD_INT_MACRO(m, EAI_ADDRFAMILY);
#endif
#ifdef EAI_AGAIN
    ADD_INT_MACRO(m, EAI_AGAIN);
#endif
#ifdef EAI_BADFLAGS
    ADD_INT_MACRO(m, EAI_BADFLAGS);
#endif
#ifdef EAI_FAIL
    ADD_INT_MACRO(m, EAI_FAIL);
#endif
#ifdef EAI_FAMILY
    ADD_INT_MACRO(m, EAI_FAMILY);
#endif
#ifdef EAI_MEMORY
    ADD_INT_MACRO(m, EAI_MEMORY);
#endif
#ifdef EAI_NODATA
    ADD_INT_MACRO(m, EAI_NODATA);
#endif
#ifdef EAI_NONAME
    ADD_INT_MACRO(m, EAI_NONAME);
#endif
#ifdef EAI_OVERFLOW
    ADD_INT_MACRO(m, EAI_OVERFLOW);
#endif
#ifdef EAI_SERVICE
    ADD_INT_MACRO(m, EAI_SERVICE);
#endif
#ifdef EAI_SOCKTYPE
    ADD_INT_MACRO(m, EAI_SOCKTYPE);
#endif
#ifdef EAI_SYSTEM
    ADD_INT_MACRO(m, EAI_SYSTEM);
#endif
#ifdef EAI_BADHINTS
    ADD_INT_MACRO(m, EAI_BADHINTS);
#endif
#ifdef EAI_PROTOCOL
    ADD_INT_MACRO(m, EAI_PROTOCOL);
#endif
#ifdef EAI_MAX
    ADD_INT_MACRO(m, EAI_MAX);
#endif
#ifdef AI_PASSIVE
    ADD_INT_MACRO(m, AI_PASSIVE);
#endif
#ifdef AI_CANONNAME
    ADD_INT_MACRO(m, AI_CANONNAME);
#endif
#ifdef AI_NUMERICHOST
    ADD_INT_MACRO(m, AI_NUMERICHOST);
#endif
#ifdef AI_NUMERICSERV
    ADD_INT_MACRO(m, AI_NUMERICSERV);
#endif
#ifdef AI_MASK
    ADD_INT_MACRO(m, AI_MASK);
#endif
#ifdef AI_ALL
    ADD_INT_MACRO(m, AI_ALL);
#endif
#ifdef AI_V4MAPPED_CFG
    ADD_INT_MACRO(m, AI_V4MAPPED_CFG);
#endif
#ifdef AI_ADDRCONFIG
    ADD_INT_MACRO(m, AI_ADDRCONFIG);
#endif
#ifdef AI_V4MAPPED
    ADD_INT_MACRO(m, AI_V4MAPPED);
#endif
#ifdef AI_DEFAULT
    ADD_INT_MACRO(m, AI_DEFAULT);
#endif
#ifdef NI_MAXHOST
    ADD_INT_MACRO(m, NI_MAXHOST);
#endif
#ifdef NI_MAXSERV
    ADD_INT_MACRO(m, NI_MAXSERV);
#endif
#ifdef NI_NOFQDN
    ADD_INT_MACRO(m, NI_NOFQDN);
#endif
#ifdef NI_NUMERICHOST
    ADD_INT_MACRO(m, NI_NUMERICHOST);
#endif
#ifdef NI_NAMEREQD
    ADD_INT_MACRO(m, NI_NAMEREQD);
#endif
#ifdef NI_NUMERICSERV
    ADD_INT_MACRO(m, NI_NUMERICSERV);
#endif
#ifdef NI_DGRAM
    ADD_INT_MACRO(m, NI_DGRAM);
#endif

    /* shutdown() parameters */
#ifdef SHUT_RD
    ADD_INT_MACRO(m, SHUT_RD);
#elif defined(SD_RECEIVE)
    ADD_INT_CONST(m, "SHUT_RD", SD_RECEIVE);
#else
    ADD_INT_CONST(m, "SHUT_RD", 0);
#endif
#ifdef SHUT_WR
    ADD_INT_MACRO(m, SHUT_WR);
#elif defined(SD_SEND)
    ADD_INT_CONST(m, "SHUT_WR", SD_SEND);
#else
    ADD_INT_CONST(m, "SHUT_WR", 1);
#endif
#ifdef SHUT_RDWR
    ADD_INT_MACRO(m, SHUT_RDWR);
#elif defined(SD_BOTH)
    ADD_INT_CONST(m, "SHUT_RDWR", SD_BOTH);
#else
    ADD_INT_CONST(m, "SHUT_RDWR", 2);
#endif

#ifdef SIO_RCVALL
    {
        DWORD codes[] = {SIO_RCVALL, SIO_KEEPALIVE_VALS,
#if defined(SIO_LOOPBACK_FAST_PATH)
            SIO_LOOPBACK_FAST_PATH
#endif
        };
        const char *names[] = {"SIO_RCVALL", "SIO_KEEPALIVE_VALS",
#if defined(SIO_LOOPBACK_FAST_PATH)
            "SIO_LOOPBACK_FAST_PATH"
#endif
        };
        int i;
        for (i = 0; i < Py_ARRAY_LENGTH(codes); ++i) {
            PyObject *tmp = PyLong_FromUnsignedLong(codes[i]);
            if (tmp == NULL) {
                goto error;
            }
            int rc = PyModule_AddObjectRef(m, names[i], tmp);
            Py_DECREF(tmp);
            if (rc < 0) {
                goto error;
            }
        }
    }
    ADD_INT_MACRO(m, RCVALL_OFF);
    ADD_INT_MACRO(m, RCVALL_ON);
    ADD_INT_MACRO(m, RCVALL_SOCKETLEVELONLY);
#ifdef RCVALL_IPLEVEL
    ADD_INT_MACRO(m, RCVALL_IPLEVEL);
#endif
#ifdef RCVALL_MAX
    ADD_INT_MACRO(m, RCVALL_MAX);
#endif
#endif /* _MSTCPIP_ */

    /* Initialize gethostbyname lock */
#if defined(USE_GETHOSTBYNAME_LOCK)
    netdb_lock = PyThread_allocate_lock();
#endif

#ifdef MS_WINDOWS
    /* remove some flags on older version Windows during run-time */
    if (remove_unusable_flags(m) < 0) {
        goto error;
    }
#endif

#undef ADD_INT_MACRO
#undef ADD_INT_CONST
#undef ADD_STR_CONST

    return 0;

error:
    return -1;
}

static struct PyModuleDef_Slot socket_slots[] = {
    {Py_mod_exec, socket_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL},
};

static int
socket_traverse(PyObject *mod, visitproc visit, void *arg)
{
    socket_state *state = get_module_state(mod);
    Py_VISIT(state->sock_type);
    Py_VISIT(state->socket_herror);
    Py_VISIT(state->socket_gaierror);
    return 0;
}

static int
socket_clear(PyObject *mod)
{
    socket_state *state = get_module_state(mod);
    Py_CLEAR(state->sock_type);
    Py_CLEAR(state->socket_herror);
    Py_CLEAR(state->socket_gaierror);
    return 0;
}

static void
socket_free(void *mod)
{
    (void)socket_clear((PyObject *)mod);
}

static struct PyModuleDef socketmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = PySocket_MODULE_NAME,
    .m_doc = socket_doc,
    .m_size = sizeof(socket_state),
    .m_methods = socket_methods,
    .m_slots = socket_slots,
    .m_traverse = socket_traverse,
    .m_clear = socket_clear,
    .m_free = socket_free,
};

PyMODINIT_FUNC
PyInit__socket(void)
{
    return PyModuleDef_Init(&socketmodule);
}
