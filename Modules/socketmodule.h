/* Socket module header file */

/* Includes needed for the sockaddr_* symbols below */
#ifndef MS_WINDOWS
# include <sys/socket.h>
# include <netinet/in.h>
# if !(defined(__BEOS__) || defined(__CYGWIN__) || (defined(PYOS_OS2) && defined(PYCC_VACPP)))
#  include <netinet/tcp.h>
# endif

#else /* MS_WINDOWS */
# include <winsock.h>
#endif

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#else
# undef AF_UNIX
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

/* The object holding a socket.  It holds some extra information,
   like the address family, which is used to decode socket address
   arguments properly. */

typedef struct {
	PyObject_HEAD
	SOCKET_T sock_fd;	/* Socket file descriptor */
	int sock_family;	/* Address family, e.g., AF_INET */
	int sock_type;		/* Socket type, e.g., SOCK_STREAM */
	int sock_proto;		/* Protocol type, usually 0 */
	union sock_addr {
		struct sockaddr_in in;
#ifdef AF_UNIX
		struct sockaddr_un un;
#endif
#ifdef ENABLE_IPV6
		struct sockaddr_in6 in6;
		struct sockaddr_storage storage;
#endif
#ifdef HAVE_NETPACKET_PACKET_H
		struct sockaddr_ll ll;
#endif
	} sock_addr;
	PyObject *(*errorhandler)(void); /* Error handler; checks
					    errno, returns NULL and
					    sets a Python exception */
} PySocketSockObject;

/* --- C API ----------------------------------------------------*/

/* C API for usage by other Python modules */
typedef struct {
	PyTypeObject *Sock_Type;
} PySocketModule_APIObject;

/* XXX The net effect of the following appears to be to define a function
   XXX named PySocketModule_APIObject in _ssl.c.  It's unclear why it isn't
   XXX defined there directly. */
#ifndef PySocket_BUILDING_SOCKET

/* --- C API ----------------------------------------------------*/

/* Interfacestructure to C API for other modules.
   Call PySocket_ImportModuleAPI() to initialize this
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
	mod = PyImport_ImportModule(apimodule);
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
