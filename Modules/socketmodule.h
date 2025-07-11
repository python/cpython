/* Socket module header file */

/* Includes needed for the sockaddr_* symbols below */
#ifndef MS_WINDOWS
#ifdef __VMS
#   include <socket.h>
# else
#   include <sys/socket.h>
# endif
# include <netinet/in.h>
# include <netinet/tcp.h>

#else /* MS_WINDOWS */
# include <winsock2.h>

/*
 * If Windows has bluetooth support, include bluetooth constants.
 */
#ifdef AF_BTH
# include <ws2bth.h>
# include <pshpack1.h>

/*
 * The current implementation assumes the bdaddr in the sockaddr structs
 * will be a bdaddr_t. We treat this as an opaque type: on *nix systems, it
 * will be a struct with a single member (an array of six bytes). On windows,
 * we typedef this to ULONGLONG to match the Windows definition.
 */
typedef ULONGLONG bdaddr_t;

/*
 * Redefine SOCKADDR_BTH to provide names compatible with _BT_RC_MEMB() macros.
 */
struct SOCKADDR_BTH_REDEF {
    union {
        USHORT    addressFamily;
        USHORT    family;
    };

    union {
        ULONGLONG btAddr;
        bdaddr_t  bdaddr;
    };

    GUID      serviceClassId;

    union {
        ULONG     port;
        ULONG     channel;
    };

};
# include <poppack.h>
#endif

/* Windows 'supports' CMSG_LEN, but does not follow the POSIX standard
 * interface at all, so there is no point including the code that
 * attempts to use it.
 */
# ifdef PySocket_BUILDING_SOCKET
#  undef CMSG_LEN
# endif
# include <ws2tcpip.h>
/* VC6 is shipped with old platform headers, and does not have MSTcpIP.h
 * Separate SDKs have all the functions we want, but older ones don't have
 * any version information.
 * I use SIO_GET_MULTICAST_FILTER to detect a decent SDK.
 */
# ifdef SIO_GET_MULTICAST_FILTER
#  include <mstcpip.h> /* for SIO_RCVALL */
#  define HAVE_ADDRINFO
#  define HAVE_SOCKADDR_STORAGE
#  define HAVE_GETADDRINFO
#  define HAVE_GETNAMEINFO
#  define ENABLE_IPV6
# else
typedef int socklen_t;
# endif /* IPPROTO_IPV6 */

/* Remove ifdef once Py_WINVER >= 0x0604
 * socket.h only defines AF_HYPERV if _WIN32_WINNT is at that level or higher
 * so for now it's just manually defined.
 */
# ifndef AF_HYPERV
#  define AF_HYPERV 34
# endif
# include <hvsocket.h>
#endif /* MS_WINDOWS */

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#else
# undef AF_UNIX
#endif

#ifdef HAVE_LINUX_NETLINK_H
# ifdef HAVE_ASM_TYPES_H
#  include <asm/types.h>
# endif
# include <linux/netlink.h>
#elif defined(HAVE_NETLINK_NETLINK_H)
# include <netlink/netlink.h>
#else
#  undef AF_NETLINK
#endif

#ifdef HAVE_LINUX_QRTR_H
# ifdef HAVE_ASM_TYPES_H
#  include <asm/types.h>
# endif
# include <linux/qrtr.h>
#else
#  undef AF_QIPCRTR
#endif

#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/sco.h>
#include <bluetooth/hci.h>
#endif

#ifdef HAVE_BLUETOOTH_H
#ifdef __FreeBSD__
#define L2CAP_SOCKET_CHECKED
#endif
#include <bluetooth.h>
#endif

#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif

#ifdef HAVE_NETPACKET_PACKET_H
# include <sys/ioctl.h>
# include <netpacket/packet.h>
#endif

#ifdef HAVE_LINUX_TIPC_H
# include <linux/tipc.h>
#endif

#ifdef HAVE_LINUX_CAN_H
# include <linux/can.h>
#elif defined(HAVE_NETCAN_CAN_H)
# include <netcan/can.h>
#else
# undef AF_CAN
# undef PF_CAN
#endif

#ifdef HAVE_LINUX_CAN_RAW_H
#include <linux/can/raw.h>
#endif

#ifdef HAVE_LINUX_CAN_BCM_H
#include <linux/can/bcm.h>
#endif

#ifdef HAVE_LINUX_CAN_J1939_H
#include <linux/can/j1939.h>
#endif

#ifdef HAVE_SYS_SYS_DOMAIN_H
#include <sys/sys_domain.h>
#endif
#ifdef HAVE_SYS_KERN_CONTROL_H
#include <sys/kern_control.h>
#endif

#ifdef HAVE_LINUX_VM_SOCKETS_H
# include <linux/vm_sockets.h>
#else
# undef AF_VSOCK
#endif

#ifdef HAVE_LINUX_NETFILTER_IPV4_H
# include <linux/netfilter_ipv4.h>
#endif

#ifdef HAVE_SOCKADDR_ALG

# include <linux/if_alg.h>
# ifndef AF_ALG
#  define AF_ALG 38
# endif
# ifndef SOL_ALG
#  define SOL_ALG 279
# endif

/* Linux 3.19 */
# ifndef ALG_SET_AEAD_ASSOCLEN
#  define ALG_SET_AEAD_ASSOCLEN           4
# endif
# ifndef ALG_SET_AEAD_AUTHSIZE
#  define ALG_SET_AEAD_AUTHSIZE           5
# endif
/* Linux 4.8 */
# ifndef ALG_SET_PUBKEY
#  define ALG_SET_PUBKEY                  6
# endif

# ifndef ALG_OP_SIGN
#  define ALG_OP_SIGN                     2
# endif
# ifndef ALG_OP_VERIFY
#  define ALG_OP_VERIFY                   3
# endif

#endif /* HAVE_SOCKADDR_ALG */

#ifdef __EMSCRIPTEN__
// wasm32-emscripten sockets only support subset of IPv4 and IPv6.
// SCTP protocol crashes runtime.
#ifdef IPPROTO_SCTP
#  undef IPPROTO_SCTP
#endif
// setsockopt() fails with ENOPROTOOPT, getsockopt only supports SO_ERROR.
// undef SO_REUSEADDR and SO_REUSEPORT so they cannot be used.
#ifdef SO_REUSEADDR
#  undef SO_REUSEADDR
#endif
#ifdef SO_REUSEPORT
#  undef SO_REUSEPORT
#endif
#endif // __EMSCRIPTEN__

#ifndef Py__SOCKET_H
#define Py__SOCKET_H
#ifdef __cplusplus
extern "C" {
#endif

/* Python module and C API name */
#define PySocket_MODULE_NAME    "_socket"
#define PySocket_CAPI_NAME      "CAPI"
#define PySocket_CAPSULE_NAME   PySocket_MODULE_NAME "." PySocket_CAPI_NAME

/* Abstract the socket file descriptor type */
#ifdef MS_WINDOWS
typedef SOCKET SOCKET_T;
#       ifdef MS_WIN64
#               define SIZEOF_SOCKET_T 8
#       else
#               define SIZEOF_SOCKET_T 4
#       endif
#else
typedef int SOCKET_T;
#       define SIZEOF_SOCKET_T SIZEOF_INT
#endif

#if SIZEOF_SOCKET_T <= SIZEOF_LONG
#define PyLong_FromSocket_t(fd) PyLong_FromLong((SOCKET_T)(fd))
#define PyLong_AsSocket_t(fd) (SOCKET_T)PyLong_AsLong(fd)
#else
#define PyLong_FromSocket_t(fd) PyLong_FromLongLong((SOCKET_T)(fd))
#define PyLong_AsSocket_t(fd) (SOCKET_T)PyLong_AsLongLong(fd)
#endif

// AF_HYPERV is only supported on Windows
#if defined(AF_HYPERV) && (defined(MS_WINDOWS_APP) || defined(MS_WINDOWS_SYSTEM))
#  define HAVE_AF_HYPERV
#endif

/* Socket address */
typedef union sock_addr {
    struct sockaddr_in in;
    struct sockaddr sa;
#ifdef AF_UNIX
    struct sockaddr_un un;
#endif
#ifdef AF_NETLINK
    struct sockaddr_nl nl;
#endif
#ifdef ENABLE_IPV6
    struct sockaddr_in6 in6;
    struct sockaddr_storage storage;
#endif
#if defined(MS_WINDOWS)
    struct SOCKADDR_BTH_REDEF bt_rc;
#elif defined(HAVE_BLUETOOTH_BLUETOOTH_H) // Linux
    struct sockaddr_l2 bt_l2;
    struct sockaddr_rc bt_rc;
    struct sockaddr_sco bt_sco;
    struct sockaddr_hci bt_hci;
#elif defined(HAVE_BLUETOOTH_H)
# if defined(__FreeBSD__)
    struct sockaddr_l2cap bt_l2;
    struct sockaddr_rfcomm bt_rc;
    struct sockaddr_sco bt_sco;
    struct sockaddr_hci bt_hci;
# else // NetBSD, DragonFly BSD
    struct sockaddr_bt bt;
# endif
#endif
#ifdef HAVE_NETPACKET_PACKET_H
    struct sockaddr_ll ll;
#endif
#if defined(HAVE_LINUX_CAN_H) || defined(HAVE_NETCAN_CAN_H)
    struct sockaddr_can can;
#endif
#ifdef HAVE_SYS_KERN_CONTROL_H
    struct sockaddr_ctl ctl;
#endif
#ifdef HAVE_SOCKADDR_ALG
    struct sockaddr_alg alg;
#endif
#ifdef AF_QIPCRTR
    struct sockaddr_qrtr sq;
#endif
#ifdef AF_VSOCK
    struct sockaddr_vm vm;
#endif
#ifdef HAVE_LINUX_TIPC_H
    struct sockaddr_tipc tipc;
#endif
#ifdef HAVE_AF_HYPERV
    SOCKADDR_HV hv;
#endif
} sock_addr_t;

/* The object holding a socket.  It holds some extra information,
   like the address family, which is used to decode socket address
   arguments properly. */

typedef struct {
    PyObject_HEAD
    SOCKET_T sock_fd;           /* Socket file descriptor */
    int sock_family;            /* Address family, e.g., AF_INET */
    int sock_type;              /* Socket type, e.g., SOCK_STREAM */
    int sock_proto;             /* Protocol type, usually 0 */
    PyObject *(*errorhandler)(void); /* Error handler; checks
                                        errno, returns NULL and
                                        sets a Python exception */
    PyTime_t sock_timeout;     /* Operation timeout in seconds;
                                        0.0 means non-blocking */
    struct _socket_state *state;
#ifdef MS_WINDOWS
    int quickack;
#endif
} PySocketSockObject;

/* --- C API ----------------------------------------------------*/

/* Short explanation of what this C API export mechanism does
   and how it works:

    The _ssl module needs access to the type object defined in
    the _socket module. Since cross-DLL linking introduces a lot of
    problems on many platforms, the "trick" is to wrap the
    C API of a module in a struct which then gets exported to
    other modules via a PyCapsule.

    The code in socketmodule.c defines this struct (which currently
    only contains the type object reference, but could very
    well also include other C APIs needed by other modules)
    and exports it as PyCapsule via the module dictionary
    under the name "CAPI".

    Other modules can now include the socketmodule.h file
    which defines the needed C APIs to import and set up
    a static copy of this struct in the importing module.

    After initialization, the importing module can then
    access the C APIs from the _socket module by simply
    referring to the static struct, e.g.

    Load _socket module and its C API; this sets up the global
    PySocketModule:

    if (PySocketModule_ImportModuleAndAPI())
        return;


    Now use the C API as if it were defined in the using
    module:

    if (!PyArg_ParseTuple(args, "O!|zz:ssl",

                          PySocketModule.Sock_Type,

                          (PyObject*)&Sock,
                          &key_file, &cert_file))
        return NULL;

    Support could easily be extended to export more C APIs/symbols
    this way. Currently, only the type object is exported,
    other candidates would be socket constructors and socket
    access functions.

*/

/* C API for usage by other Python modules.
 * Always add new things to the end for binary compatibility. */
typedef struct {
    PyTypeObject *Sock_Type;
    PyObject *error;
    PyObject *timeout_error;
} PySocketModule_APIObject;

#define PySocketModule_ImportModuleAndAPI() PyCapsule_Import(PySocket_CAPSULE_NAME, 1)

#ifdef __cplusplus
}
#endif
#endif /* !Py__SOCKET_H */
