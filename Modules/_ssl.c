/* SSL socket module 

   SSL support based on patches by Brian E Gallew and Laszlo Kovacs.

   This module is imported by socket.py. It should *not* be used
   directly.

*/

#include "Python.h"
enum py_ssl_error {
	/* these mirror ssl.h */
	PY_SSL_ERROR_NONE,                 
	PY_SSL_ERROR_SSL,                   
	PY_SSL_ERROR_WANT_READ,             
	PY_SSL_ERROR_WANT_WRITE,            
	PY_SSL_ERROR_WANT_X509_LOOKUP,      
	PY_SSL_ERROR_SYSCALL,     /* look at error stack/return value/errno */
	PY_SSL_ERROR_ZERO_RETURN,           
	PY_SSL_ERROR_WANT_CONNECT,
	/* start of non ssl.h errorcodes */ 
	PY_SSL_ERROR_EOF,         /* special case of SSL_ERROR_SYSCALL */
	PY_SSL_ERROR_INVALID_ERROR_CODE
};

/* Include symbols from _socket module */
#include "socketmodule.h"

#if defined(HAVE_POLL_H) 
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif

/* Include OpenSSL header files */
#include "openssl/rsa.h"
#include "openssl/crypto.h"
#include "openssl/x509.h"
#include "openssl/pem.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/rand.h"

/* SSL error object */
static PyObject *PySSLErrorObject;

/* SSL socket object */

#define X509_NAME_MAXLEN 256

/* RAND_* APIs got added to OpenSSL in 0.9.5 */
#if OPENSSL_VERSION_NUMBER >= 0x0090500fL
# define HAVE_OPENSSL_RAND 1
#else
# undef HAVE_OPENSSL_RAND
#endif

typedef struct {
	PyObject_HEAD
	PySocketSockObject *Socket;	/* Socket on which we're layered */
	SSL_CTX* 	ctx;
	SSL*     	ssl;
	X509*    	server_cert;
	char    	server[X509_NAME_MAXLEN];
	char		issuer[X509_NAME_MAXLEN];

} PySSLObject;

static PyTypeObject PySSL_Type;
static PyObject *PySSL_SSLwrite(PySSLObject *self, PyObject *args);
static PyObject *PySSL_SSLread(PySSLObject *self, PyObject *args);
static int check_socket_and_wait_for_timeout(PySocketSockObject *s, 
					     int writing);

#define PySSLObject_Check(v)	(Py_Type(v) == &PySSL_Type)

typedef enum {
	SOCKET_IS_NONBLOCKING,
	SOCKET_IS_BLOCKING,
	SOCKET_HAS_TIMED_OUT,
	SOCKET_HAS_BEEN_CLOSED,
	SOCKET_TOO_LARGE_FOR_SELECT,
	SOCKET_OPERATION_OK
} timeout_state;

/* XXX It might be helpful to augment the error message generated
   below with the name of the SSL function that generated the error.
   I expect it's obvious most of the time.
*/

static PyObject *
PySSL_SetError(PySSLObject *obj, int ret)
{
	PyObject *v, *n, *s;
	char *errstr;
	int err;
	enum py_ssl_error p;

	assert(ret <= 0);
    
	err = SSL_get_error(obj->ssl, ret);

	switch (err) {
	case SSL_ERROR_ZERO_RETURN:
		errstr = "TLS/SSL connection has been closed";
		p = PY_SSL_ERROR_ZERO_RETURN;
		break;
	case SSL_ERROR_WANT_READ:
		errstr = "The operation did not complete (read)";
		p = PY_SSL_ERROR_WANT_READ;
		break;
	case SSL_ERROR_WANT_WRITE:
		p = PY_SSL_ERROR_WANT_WRITE;
		errstr = "The operation did not complete (write)";
		break;
	case SSL_ERROR_WANT_X509_LOOKUP:
		p = PY_SSL_ERROR_WANT_X509_LOOKUP;
		errstr = "The operation did not complete (X509 lookup)";
		break;
	case SSL_ERROR_WANT_CONNECT:
		p = PY_SSL_ERROR_WANT_CONNECT;
		errstr = "The operation did not complete (connect)";
		break;
	case SSL_ERROR_SYSCALL:
	{
		unsigned long e = ERR_get_error();
		if (e == 0) {
			if (ret == 0 || !obj->Socket) {
				p = PY_SSL_ERROR_EOF;
				errstr = "EOF occurred in violation of protocol";
			} else if (ret == -1) {
				/* the underlying BIO reported an I/O error */
				return obj->Socket->errorhandler();
			} else {  /* possible? */
				p = PY_SSL_ERROR_SYSCALL;
				errstr = "Some I/O error occurred";
			}
		} else {
			p = PY_SSL_ERROR_SYSCALL;
			/* XXX Protected by global interpreter lock */
			errstr = ERR_error_string(e, NULL);
		}
		break;
	}   
	case SSL_ERROR_SSL:
	{
		unsigned long e = ERR_get_error();
		p = PY_SSL_ERROR_SSL;
		if (e != 0) 
			/* XXX Protected by global interpreter lock */
			errstr = ERR_error_string(e, NULL);
		else { /* possible? */
			errstr = "A failure in the SSL library occurred";
		}
		break;
	}
	default:
		p = PY_SSL_ERROR_INVALID_ERROR_CODE;
		errstr = "Invalid error code";
	}
	n = PyInt_FromLong((long) p);
	if (n == NULL)
		return NULL;
	v = PyTuple_New(2);
	if (v == NULL) {
		Py_DECREF(n);
		return NULL;
	}

	s = PyUnicode_FromString(errstr);
	if (s == NULL) {
		Py_DECREF(v);
		Py_DECREF(n);
	}
	PyTuple_SET_ITEM(v, 0, n);
	PyTuple_SET_ITEM(v, 1, s);
	PyErr_SetObject(PySSLErrorObject, v);
	Py_DECREF(v);
	return NULL;
}

static PySSLObject *
newPySSLObject(PySocketSockObject *Sock, char *key_file, char *cert_file)
{
	PySSLObject *self;
	char *errstr = NULL;
	int ret;
	int err;
	int sockstate;

	self = PyObject_New(PySSLObject, &PySSL_Type); /* Create new object */
	if (self == NULL)
		return NULL;
	memset(self->server, '\0', sizeof(char) * X509_NAME_MAXLEN);
	memset(self->issuer, '\0', sizeof(char) * X509_NAME_MAXLEN);
	self->server_cert = NULL;
	self->ssl = NULL;
	self->ctx = NULL;
	self->Socket = NULL;

	if ((key_file && !cert_file) || (!key_file && cert_file)) {
		errstr = "Both the key & certificate files must be specified";
		goto fail;
	}

	Py_BEGIN_ALLOW_THREADS
	self->ctx = SSL_CTX_new(SSLv23_method()); /* Set up context */
	Py_END_ALLOW_THREADS
	if (self->ctx == NULL) {
		errstr = "SSL_CTX_new error";
		goto fail;
	}

	if (key_file) {
		Py_BEGIN_ALLOW_THREADS
		ret = SSL_CTX_use_PrivateKey_file(self->ctx, key_file,
						SSL_FILETYPE_PEM);
		Py_END_ALLOW_THREADS
		if (ret < 1) {
			errstr = "SSL_CTX_use_PrivateKey_file error";
			goto fail;
		}

		Py_BEGIN_ALLOW_THREADS
		ret = SSL_CTX_use_certificate_chain_file(self->ctx,
						       cert_file);
		Py_END_ALLOW_THREADS
		SSL_CTX_set_options(self->ctx, SSL_OP_ALL); /* ssl compatibility */
		if (ret < 1) {
			errstr = "SSL_CTX_use_certificate_chain_file error";
			goto fail;
		}
	}

	Py_BEGIN_ALLOW_THREADS
	SSL_CTX_set_verify(self->ctx,
			   SSL_VERIFY_NONE, NULL); /* set verify lvl */
	self->ssl = SSL_new(self->ctx); /* New ssl struct */
	Py_END_ALLOW_THREADS
	SSL_set_fd(self->ssl, Sock->sock_fd);	/* Set the socket for SSL */

	/* If the socket is in non-blocking mode or timeout mode, set the BIO
	 * to non-blocking mode (blocking is the default)
	 */
	if (Sock->sock_timeout >= 0.0) {
		/* Set both the read and write BIO's to non-blocking mode */
		BIO_set_nbio(SSL_get_rbio(self->ssl), 1);
		BIO_set_nbio(SSL_get_wbio(self->ssl), 1);
	}

	Py_BEGIN_ALLOW_THREADS
	SSL_set_connect_state(self->ssl);
	Py_END_ALLOW_THREADS

	/* Actually negotiate SSL connection */
	/* XXX If SSL_connect() returns 0, it's also a failure. */
	sockstate = 0;
	do {
		Py_BEGIN_ALLOW_THREADS
		ret = SSL_connect(self->ssl);
		err = SSL_get_error(self->ssl, ret);
		Py_END_ALLOW_THREADS
		if(PyErr_CheckSignals()) {
                        goto fail;
		}
		if (err == SSL_ERROR_WANT_READ) {
			sockstate = check_socket_and_wait_for_timeout(Sock, 0);
		} else if (err == SSL_ERROR_WANT_WRITE) {
			sockstate = check_socket_and_wait_for_timeout(Sock, 1);
		} else {
			sockstate = SOCKET_OPERATION_OK;
		}
	        if (sockstate == SOCKET_HAS_TIMED_OUT) {
			PyErr_SetString(PySSLErrorObject, "The connect operation timed out");
			goto fail;
		} else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
			PyErr_SetString(PySSLErrorObject, "Underlying socket has been closed.");
			goto fail;
		} else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
			PyErr_SetString(PySSLErrorObject, "Underlying socket too large for select().");
			goto fail;
		} else if (sockstate == SOCKET_IS_NONBLOCKING) {
			break;
		}
	} while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
	if (ret <= 0) {
		PySSL_SetError(self, ret);
		goto fail;
	}
	self->ssl->debug = 1;

	Py_BEGIN_ALLOW_THREADS
	if ((self->server_cert = SSL_get_peer_certificate(self->ssl))) {
		X509_NAME_oneline(X509_get_subject_name(self->server_cert),
				  self->server, X509_NAME_MAXLEN);
		X509_NAME_oneline(X509_get_issuer_name(self->server_cert),
				  self->issuer, X509_NAME_MAXLEN);
	}
	Py_END_ALLOW_THREADS
	self->Socket = Sock;
	Py_INCREF(self->Socket);
	return self;
 fail:
	if (errstr)
		PyErr_SetString(PySSLErrorObject, errstr);
	Py_DECREF(self);
	return NULL;
}

static PyObject *
PySocket_ssl(PyObject *self, PyObject *args)
{
	PySSLObject *rv;
	PySocketSockObject *Sock;
	char *key_file = NULL;
	char *cert_file = NULL;

	if (!PyArg_ParseTuple(args, "O!|zz:ssl",
			      PySocketModule.Sock_Type,
			      &Sock,
			      &key_file, &cert_file))
		return NULL;

	rv = newPySSLObject(Sock, key_file, cert_file);
	if (rv == NULL)
		return NULL;
	return (PyObject *)rv;
}

PyDoc_STRVAR(ssl_doc,
"ssl(socket, [keyfile, certfile]) -> sslobject");

/* SSL object methods */

static PyObject *
PySSL_server(PySSLObject *self)
{
	return PyUnicode_FromString(self->server);
}

static PyObject *
PySSL_issuer(PySSLObject *self)
{
	return PyUnicode_FromString(self->issuer);
}


static void PySSL_dealloc(PySSLObject *self)
{
	if (self->server_cert)	/* Possible not to have one? */
		X509_free (self->server_cert);
	if (self->ssl)
	    SSL_free(self->ssl);
	if (self->ctx)
	    SSL_CTX_free(self->ctx);
	Py_XDECREF(self->Socket);
	PyObject_Del(self);
}

/* If the socket has a timeout, do a select()/poll() on the socket.
   The argument writing indicates the direction.
   Returns one of the possibilities in the timeout_state enum (above).
 */

static int
check_socket_and_wait_for_timeout(PySocketSockObject *s, int writing)
{
	fd_set fds;
	struct timeval tv;
	int rc;

	/* Nothing to do unless we're in timeout mode (not non-blocking) */
	if (s->sock_timeout < 0.0)
		return SOCKET_IS_BLOCKING;
	else if (s->sock_timeout == 0.0)
		return SOCKET_IS_NONBLOCKING;

	/* Guard against closed socket */
	if (s->sock_fd < 0)
		return SOCKET_HAS_BEEN_CLOSED;

	/* Prefer poll, if available, since you can poll() any fd
	 * which can't be done with select(). */
#ifdef HAVE_POLL
	{
		struct pollfd pollfd;
		int timeout;

		pollfd.fd = s->sock_fd;
		pollfd.events = writing ? POLLOUT : POLLIN;

		/* s->sock_timeout is in seconds, timeout in ms */
		timeout = (int)(s->sock_timeout * 1000 + 0.5);
		Py_BEGIN_ALLOW_THREADS
		rc = poll(&pollfd, 1, timeout);
		Py_END_ALLOW_THREADS

		goto normal_return;
	}
#endif

	/* Guard against socket too large for select*/
#ifndef Py_SOCKET_FD_CAN_BE_GE_FD_SETSIZE
	if (s->sock_fd >= FD_SETSIZE)
		return SOCKET_TOO_LARGE_FOR_SELECT;
#endif

	/* Construct the arguments to select */
	tv.tv_sec = (int)s->sock_timeout;
	tv.tv_usec = (int)((s->sock_timeout - tv.tv_sec) * 1e6);
	FD_ZERO(&fds);
	FD_SET(s->sock_fd, &fds);

	/* See if the socket is ready */
	Py_BEGIN_ALLOW_THREADS
	if (writing)
		rc = select(s->sock_fd+1, NULL, &fds, NULL, &tv);
	else
		rc = select(s->sock_fd+1, &fds, NULL, NULL, &tv);
	Py_END_ALLOW_THREADS

normal_return:
	/* Return SOCKET_TIMED_OUT on timeout, SOCKET_OPERATION_OK otherwise
	   (when we are able to write or when there's something to read) */
	return rc == 0 ? SOCKET_HAS_TIMED_OUT : SOCKET_OPERATION_OK;
}

static PyObject *PySSL_SSLwrite(PySSLObject *self, PyObject *args)
{
	char *data;
	int len;
	int count;
	int sockstate;
	int err;

	if (!PyArg_ParseTuple(args, "s#:write", &data, &count))
		return NULL;

	sockstate = check_socket_and_wait_for_timeout(self->Socket, 1);
	if (sockstate == SOCKET_HAS_TIMED_OUT) {
		PyErr_SetString(PySSLErrorObject, "The write operation timed out");
		return NULL;
	} else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
		PyErr_SetString(PySSLErrorObject, "Underlying socket has been closed.");
		return NULL;
	} else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
		PyErr_SetString(PySSLErrorObject, "Underlying socket too large for select().");
		return NULL;
	}
	do {
		err = 0;
		Py_BEGIN_ALLOW_THREADS
		len = SSL_write(self->ssl, data, count);
		err = SSL_get_error(self->ssl, len);
		Py_END_ALLOW_THREADS
		if(PyErr_CheckSignals()) {
			return NULL;
		}
		if (err == SSL_ERROR_WANT_READ) {
			sockstate = check_socket_and_wait_for_timeout(self->Socket, 0);
		} else if (err == SSL_ERROR_WANT_WRITE) {
			sockstate = check_socket_and_wait_for_timeout(self->Socket, 1);
		} else {
			sockstate = SOCKET_OPERATION_OK;
		}
	        if (sockstate == SOCKET_HAS_TIMED_OUT) {
			PyErr_SetString(PySSLErrorObject, "The write operation timed out");
			return NULL;
		} else if (sockstate == SOCKET_HAS_BEEN_CLOSED) {
			PyErr_SetString(PySSLErrorObject, "Underlying socket has been closed.");
			return NULL;
		} else if (sockstate == SOCKET_IS_NONBLOCKING) {
			break;
		}
	} while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
	if (len > 0)
		return PyInt_FromLong(len);
	else
		return PySSL_SetError(self, len);
}

PyDoc_STRVAR(PySSL_SSLwrite_doc,
"write(s) -> len\n\
\n\
Writes the string s into the SSL object.  Returns the number\n\
of bytes written.");

static PyObject *PySSL_SSLread(PySSLObject *self, PyObject *args)
{
	PyObject *buf;
	int count = 0;
	int len = 1024;
	int sockstate;
	int err;

	if (!PyArg_ParseTuple(args, "|i:read", &len))
		return NULL;

	if (!(buf = PyBytes_FromStringAndSize((char *) 0, len)))
		return NULL;
	
	/* first check if there are bytes ready to be read */
	Py_BEGIN_ALLOW_THREADS
	count = SSL_pending(self->ssl);
	Py_END_ALLOW_THREADS

	if (!count) {
		sockstate = check_socket_and_wait_for_timeout(self->Socket, 0);
		if (sockstate == SOCKET_HAS_TIMED_OUT) {
			PyErr_SetString(PySSLErrorObject, "The read operation timed out");
			Py_DECREF(buf);
			return NULL;
		} else if (sockstate == SOCKET_TOO_LARGE_FOR_SELECT) {
			PyErr_SetString(PySSLErrorObject, "Underlying socket too large for select().");
			return NULL;
		}
	}
	do {
		err = 0;
		Py_BEGIN_ALLOW_THREADS
		count = SSL_read(self->ssl, PyBytes_AS_STRING(buf), len);
		err = SSL_get_error(self->ssl, count);
		Py_END_ALLOW_THREADS
		if(PyErr_CheckSignals()) {
			Py_DECREF(buf);
			return NULL;
		}
		if (err == SSL_ERROR_WANT_READ) {
			sockstate = check_socket_and_wait_for_timeout(self->Socket, 0);
		} else if (err == SSL_ERROR_WANT_WRITE) {
			sockstate = check_socket_and_wait_for_timeout(self->Socket, 1);
		} else {
			sockstate = SOCKET_OPERATION_OK;
		}
	        if (sockstate == SOCKET_HAS_TIMED_OUT) {
			PyErr_SetString(PySSLErrorObject, "The read operation timed out");
			Py_DECREF(buf);
			return NULL;
		} else if (sockstate == SOCKET_IS_NONBLOCKING) {
			break;
		}
	} while (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE);
 	if (count <= 0) {
		Py_DECREF(buf);
		return PySSL_SetError(self, count);
	}
	if (count != len)
		if (PyBytes_Resize(buf, count) < 0) {
                        Py_DECREF(buf);
                        return NULL;
                }
	return buf;
}

PyDoc_STRVAR(PySSL_SSLread_doc,
"read([len]) -> bytes\n\
\n\
Read up to len bytes from the SSL socket.");

static PyMethodDef PySSLMethods[] = {
	{"write", (PyCFunction)PySSL_SSLwrite, METH_VARARGS,
	          PySSL_SSLwrite_doc},
	{"read", (PyCFunction)PySSL_SSLread, METH_VARARGS,
	          PySSL_SSLread_doc},
	{"server", (PyCFunction)PySSL_server, METH_NOARGS},
	{"issuer", (PyCFunction)PySSL_issuer, METH_NOARGS},
	{NULL, NULL}
};

static PyObject *PySSL_getattr(PySSLObject *self, char *name)
{
	return Py_FindMethod(PySSLMethods, (PyObject *)self, name);
}

static PyTypeObject PySSL_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"socket.SSL",			/*tp_name*/
	sizeof(PySSLObject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)PySSL_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)PySSL_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
	0,				/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	0,				/*tp_hash*/
};

#ifdef HAVE_OPENSSL_RAND

/* helper routines for seeding the SSL PRNG */
static PyObject *
PySSL_RAND_add(PyObject *self, PyObject *args)
{
    char *buf;
    int len;
    double entropy;

    if (!PyArg_ParseTuple(args, "s#d:RAND_add", &buf, &len, &entropy))
	return NULL;
    RAND_add(buf, len, entropy);
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(PySSL_RAND_add_doc,
"RAND_add(string, entropy)\n\
\n\
Mix string into the OpenSSL PRNG state.  entropy (a float) is a lower\n\
bound on the entropy contained in string.");

static PyObject *
PySSL_RAND_status(PyObject *self)
{
    return PyBool_FromLong(RAND_status());
}

PyDoc_STRVAR(PySSL_RAND_status_doc,
"RAND_status() -> 0 or 1\n\
\n\
Returns True if the OpenSSL PRNG has been seeded with enough data and\n\
False if not.  It is necessary to seed the PRNG with RAND_add()\n\
on some platforms before using the ssl() function.");

static PyObject *
PySSL_RAND_egd(PyObject *self, PyObject *arg)
{
    int bytes;

    if (!PyString_Check(arg))
	return PyErr_Format(PyExc_TypeError,
			    "RAND_egd() expected string, found %s",
			    Py_Type(arg)->tp_name);
    bytes = RAND_egd(PyString_AS_STRING(arg));
    if (bytes == -1) {
	PyErr_SetString(PySSLErrorObject,
			"EGD connection failed or EGD did not return "
			"enough data to seed the PRNG");
	return NULL;
    }
    return PyInt_FromLong(bytes);
}

PyDoc_STRVAR(PySSL_RAND_egd_doc,
"RAND_egd(path) -> bytes\n\
\n\
Queries the entropy gather daemon (EGD) on socket path.  Returns number\n\
of bytes read.  Raises socket.sslerror if connection to EGD fails or\n\
if it does provide enough data to seed PRNG.");

#endif

/* List of functions exported by this module. */

static PyMethodDef PySSL_methods[] = {
	{"ssl",			PySocket_ssl,
	 METH_VARARGS, ssl_doc},
#ifdef HAVE_OPENSSL_RAND
	{"RAND_add",            PySSL_RAND_add, METH_VARARGS, 
	 PySSL_RAND_add_doc},
	{"RAND_egd",            PySSL_RAND_egd, METH_O,
	 PySSL_RAND_egd_doc},
	{"RAND_status",         (PyCFunction)PySSL_RAND_status, METH_NOARGS,
	 PySSL_RAND_status_doc},
#endif
	{NULL,			NULL}		 /* Sentinel */
};


PyDoc_STRVAR(module_doc,
"Implementation module for SSL socket operations.  See the socket module\n\
for documentation.");

PyMODINIT_FUNC
init_ssl(void)
{
	PyObject *m, *d;

	Py_Type(&PySSL_Type) = &PyType_Type;

	m = Py_InitModule3("_ssl", PySSL_methods, module_doc);
	if (m == NULL)
		return;
	d = PyModule_GetDict(m);

	/* Load _socket module and its C API */
	if (PySocketModule_ImportModuleAndAPI())
 	    	return;

	/* Init OpenSSL */
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();

	/* Add symbols to module dict */
	PySSLErrorObject = PyErr_NewException("socket.sslerror",
                                               PySocketModule.error,
                                               NULL);
	if (PySSLErrorObject == NULL)
		return;
	PyDict_SetItemString(d, "sslerror", PySSLErrorObject);
	if (PyDict_SetItemString(d, "SSLType",
				 (PyObject *)&PySSL_Type) != 0)
		return;
	PyModule_AddIntConstant(m, "SSL_ERROR_ZERO_RETURN",
				PY_SSL_ERROR_ZERO_RETURN);
	PyModule_AddIntConstant(m, "SSL_ERROR_WANT_READ",
				PY_SSL_ERROR_WANT_READ);
	PyModule_AddIntConstant(m, "SSL_ERROR_WANT_WRITE",
				PY_SSL_ERROR_WANT_WRITE);
	PyModule_AddIntConstant(m, "SSL_ERROR_WANT_X509_LOOKUP",
				PY_SSL_ERROR_WANT_X509_LOOKUP);
	PyModule_AddIntConstant(m, "SSL_ERROR_SYSCALL",
				PY_SSL_ERROR_SYSCALL);
	PyModule_AddIntConstant(m, "SSL_ERROR_SSL",
				PY_SSL_ERROR_SSL);
	PyModule_AddIntConstant(m, "SSL_ERROR_WANT_CONNECT",
				PY_SSL_ERROR_WANT_CONNECT);
	/* non ssl.h errorcodes */
	PyModule_AddIntConstant(m, "SSL_ERROR_EOF",
				PY_SSL_ERROR_EOF);
	PyModule_AddIntConstant(m, "SSL_ERROR_INVALID_ERROR_CODE",
				PY_SSL_ERROR_INVALID_ERROR_CODE);

}
