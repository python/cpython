/* Socket module header file */

/* Includes needed for the sockaddr_* symbols below */
#ifndef MS_WINDOWS
#ifdef __VMS
#   include <socket.h>
# else
#   include <sys/socket.h>
# endif
# include <netinet/in.h>
# if !(defined(__CYGWIN__) || (defined(PYOS_OS2) && defined(PYCC_VACPP)))
#  include <netinet/tcp.h>
# endif

#else /* MS_WINDOWS */
#if _MSC_VER >= 1300
# include <winsock2.h>
# include <ws2tcpip.h>
# include <MSTcpIP.h> /* for SIO_RCVALL */
# define HAVE_ADDRINFO
# define HAVE_SOCKADDR_STORAGE
# define HAVE_GETADDRINFO
# define HAVE_GETNAMEINFO
# define ENABLE_IPV6
#else
# define WIN32_LEAN_AND_MEAN
# include <winsock.h>
#endif
#endif

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
#else
#  undef AF_NETLINK
#endif

#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/sco.h>
#include <bluetooth/hci.h>
#endif

#ifdef HAVE_BLUETOOTH_H
#include <bluetooth.h>
#endif

#ifdef HAVE_NETPACKET_PACKET_H
# include <sys/ioctl.h>
# include <net/if.h>
# include <netpacket/packet.h>
#endif

#ifndef Py__SOCKET_H
#define Py__SOCKET_H
#ifdef __cplusplus
extern "C" {
#endif

/* Python module and C API name */
#define PySocket_MODULE_NAME	"_socket"
#define PySocket_CAPI_NAME	"CAPI"

/* Abstract the socket file descriptor type */
#ifdef MS_WINDOWS
typedef SOCKET SOCKET_T;
#	ifdef MS_WIN64
#		define SIZEOF_SOCKET_T 8
#	else
#		define SIZEOF_SOCKET_T 4
#	endif
#else
typedef int SOCKET_T;
#	define SIZEOF_SOCKET_T SIZEOF_INT
#endif

/* Socket address */
typedef union sock_addr {
	struct sockaddr_in in;
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
#ifdef HAVE_BLUETOOTH_BLUETOOTH_H
	struct sockaddr_l2 bt_l2;
	struct sockaddr_rc bt_rc;
	struct sockaddr_sco bt_sco;
	struct sockaddr_hci bt_hci;
#endif
#ifdef HAVE_NETPACKET_PACKET_H
	struct sockaddr_ll ll;
#endif
} sock_addr_t;

/* The object holding a socket.  It holds some extra information,
   like the address family, which is used to decode socket address
   arguments properly. */

typedef struct {
	PyObject_HEAD
	SOCKET_T sock_fd;	/* Socket file descriptor */
	int sock_family;	/* Address family, e.g., AF_INET */
	int sock_type;		/* Socket type, e.g., SOCK_STREAM */
	int sock_proto;		/* Protocol type, usually 0 */
	PyObject *(*errorhandler)(void); /* Error handler; checks
					    errno, returns NULL and
					    sets a Python exception */
	double sock_timeout;		 /* Operation timeout in seconds;
					    0.0 means non-blocking */
} PySocketSockObject;

/* --- C API ----------------------------------------------------*/

/* Short explanation of what this C API export mechanism does
   and how it works:

    The _ssl module needs access to the type object defined in 
    the _socket module. Since cross-DLL linking introduces a lot of
    problems on many platforms, the "trick" is to wrap the
    C API of a module in a struct which then gets exported to
    other modules via a PyCObject.

    The code in socketmodule.c defines this struct (which currently
    only contains the type object reference, but could very
    well also include other C APIs needed by other modules)
    and exports it as PyCObject via the module dictionary
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

/* C API for usage by other Python modules */
typedef struct {
	PyTypeObject *Sock_Type;
        PyObject *error;
} PySocketModule_APIObject;

/* XXX The net effect of the following appears to be to define a function
   XXX named PySocketModule_APIObject in _ssl.c.  It's unclear why it isn't
   XXX defined there directly. 

   >>> It's defined here because other modules might also want to use
   >>> the C API.

*/
#ifndef PySocket_BUILDING_SOCKET

/* --- C API ----------------------------------------------------*/

/* Interfacestructure to C API for other modules.
   Call PySocketModule_ImportModuleAndAPI() to initialize this
   structure. After that usage is simple:

   if (!PyArg_ParseTuple(args, "O!|zz:ssl",
                         &PySocketModule.Sock_Type, (PyObject*)&Sock,
	 		 &key_file, &cert_file))
 	 return NULL;
   ...
*/

static
PySocketModule_APIObject PySocketModule;

/* You *must* call this before using any of the functions in
   PySocketModule and check its outcome; otherwise all accesses will
   result in a segfault. Returns 0 on success. */

#ifndef DPRINTF
# define DPRINTF if (0) printf
#endif

static
int PySocketModule_ImportModuleAndAPI(void)
{
	PyObject *mod = 0, *v = 0;
	char *apimodule = PySocket_MODULE_NAME;
	char *apiname = PySocket_CAPI_NAME;
	void *api;

	DPRINTF("Importing the %s C API...\n", apimodule);
	mod = PyImport_ImportModuleNoBlock(apimodule);
	if (mod == NULL)
		goto onError;
	DPRINTF(" %s package found\n", apimodule);
	v = PyObject_GetAttrString(mod, apiname);
	if (v == NULL)
		goto onError;
	Py_DECREF(mod);
	DPRINTF(" API object %s found\n", apiname);
	api = PyCObject_AsVoidPtr(v);
	if (api == NULL)
		goto onError;
	Py_DECREF(v);
	memcpy(&PySocketModule, api, sizeof(PySocketModule));
	DPRINTF(" API object loaded and initialized.\n");
	return 0;

 onError:
	DPRINTF(" not found.\n");
	Py_XDECREF(mod);
	Py_XDECREF(v);
	return -1;
}

#endif /* !PySocket_BUILDING_SOCKET */

#ifdef __cplusplus
}
#endif
#endif /* !Py__SOCKET_H */
