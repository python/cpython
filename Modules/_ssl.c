/* SSL socket module 

   SSL support based on patches by Brian E Gallew and Laszlo Kovacs.

   This module is imported by socket.py. It should *not* be used
   directly.

*/

#include "Python.h"

/* Include symbols from _socket module */
#include "socketmodule.h"

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
	BIO*		sbio;
	char    	server[X509_NAME_MAXLEN];
	char		issuer[X509_NAME_MAXLEN];

} PySSLObject;

staticforward PyTypeObject PySSL_Type;
staticforward PyObject *PySSL_SSLwrite(PySSLObject *self, PyObject *args);
staticforward PyObject *PySSL_SSLread(PySSLObject *self, PyObject *args);

#define PySSLObject_Check(v)	((v)->ob_type == &PySSL_Type)

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

	assert(ret <= 0);
    
	err = SSL_get_error(obj->ssl, ret);
	n = PyInt_FromLong(err);
	if (n == NULL)
		return NULL;
	v = PyTuple_New(2);
	if (v == NULL) {
		Py_DECREF(n);
		return NULL;
	}

	switch (SSL_get_error(obj->ssl, ret)) {
	case SSL_ERROR_ZERO_RETURN:
		errstr = "TLS/SSL connection has been closed";
		break;
	case SSL_ERROR_WANT_READ:
		errstr = "The operation did not complete (read)";
		break;
	case SSL_ERROR_WANT_WRITE:
		errstr = "The operation did not complete (write)";
		break;
	case SSL_ERROR_WANT_X509_LOOKUP:
		errstr = "The operation did not complete (X509 lookup)";
		break;
	case SSL_ERROR_SYSCALL:
	case SSL_ERROR_SSL:
	{
		unsigned long e = ERR_get_error();
		if (e == 0) {
			/* an EOF was observed that violates the protocol */
			errstr = "EOF occurred in violation of protocol";
		} else if (e == -1) {
			/* the underlying BIO reported an I/O error */
			Py_DECREF(v);
			Py_DECREF(n);
			return obj->Socket->errorhandler();
		} else {
			/* XXX Protected by global interpreter lock */
			errstr = ERR_error_string(e, NULL);
		}
		break;
	}
	default:
		errstr = "Invalid error code";
	}
	s = PyString_FromString(errstr);
	if (s == NULL) {
		Py_DECREF(v);
		Py_DECREF(n);
	}
	PyTuple_SET_ITEM(v, 0, n);
	PyTuple_SET_ITEM(v, 1, s);
	PyErr_SetObject(PySSLErrorObject, v);
	return NULL;
}

/* This is a C function to be called for new object initialization */
static PySSLObject *
newPySSLObject(PySocketSockObject *Sock, char *key_file, char *cert_file)
{
	PySSLObject *self;
	char *errstr = NULL;
	int ret;

	self = PyObject_New(PySSLObject, &PySSL_Type); /* Create new object */
	if (self == NULL){
		errstr = "newPySSLObject error";
		goto fail;
	}
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

	self->ctx = SSL_CTX_new(SSLv23_method()); /* Set up context */
	if (self->ctx == NULL) {
		errstr = "SSL_CTX_new error";
		goto fail;
	}

	if (key_file) {
		if (SSL_CTX_use_PrivateKey_file(self->ctx, key_file,
						SSL_FILETYPE_PEM) < 1) {
			errstr = "SSL_CTX_use_PrivateKey_file error";
			goto fail;
		}

		if (SSL_CTX_use_certificate_chain_file(self->ctx,
						       cert_file) < 1) {
			errstr = "SSL_CTX_use_certificate_chain_file error";
			goto fail;
		}
	}

	SSL_CTX_set_verify(self->ctx,
			   SSL_VERIFY_NONE, NULL); /* set verify lvl */
	self->ssl = SSL_new(self->ctx); /* New ssl struct */
	SSL_set_fd(self->ssl, Sock->sock_fd);	/* Set the socket for SSL */
	SSL_set_connect_state(self->ssl);

	/* Actually negotiate SSL connection */
	/* XXX If SSL_connect() returns 0, it's also a failure. */
	ret = SSL_connect(self->ssl);
	if (ret <= 0) {
		PySSL_SetError(self, ret);
		goto fail;
	}
	self->ssl->debug = 1;

	if ((self->server_cert = SSL_get_peer_certificate(self->ssl))) {
		X509_NAME_oneline(X509_get_subject_name(self->server_cert),
				  self->server, X509_NAME_MAXLEN);
		X509_NAME_oneline(X509_get_issuer_name(self->server_cert),
				  self->issuer, X509_NAME_MAXLEN);
	}
	self->Socket = Sock;
	Py_INCREF(self->Socket);
	return self;
 fail:
	if (errstr)
		PyErr_SetString(PySSLErrorObject, errstr);
	Py_DECREF(self);
	return NULL;
}

/* This is the Python function called for new object initialization */
static PyObject *
PySocket_ssl(PyObject *self, PyObject *args)
{
	PySSLObject *rv;
	PySocketSockObject *Sock;
	char *key_file = NULL;
	char *cert_file = NULL;

	if (!PyArg_ParseTuple(args, "O!|zz:ssl",
			      PySocketModule.Sock_Type,
			      (PyObject*)&Sock,
			      &key_file, &cert_file))
		return NULL;

	rv = newPySSLObject(Sock, key_file, cert_file);
	if (rv == NULL)
		return NULL;
	return (PyObject *)rv;
}

static char ssl_doc[] =
"ssl(socket, [keyfile, certfile]) -> sslobject";

/* SSL object methods */

static PyObject *
PySSL_server(PySSLObject *self)
{
	return PyString_FromString(self->server);
}

static PyObject *
PySSL_issuer(PySSLObject *self)
{
	return PyString_FromString(self->issuer);
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

static PyObject *PySSL_SSLwrite(PySSLObject *self, PyObject *args)
{
	char *data;
	int len;

	if (!PyArg_ParseTuple(args, "s#:write", &data, &len))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	len = SSL_write(self->ssl, data, len);
	Py_END_ALLOW_THREADS
	if (len > 0)
		return PyInt_FromLong(len);
	else
		return PySSL_SetError(self, len);
}

static char PySSL_SSLwrite_doc[] =
"write(s) -> len\n\
\n\
Writes the string s into the SSL object.  Returns the number\n\
of bytes written.";

static PyObject *PySSL_SSLread(PySSLObject *self, PyObject *args)
{
	PyObject *buf;
	int count = 0;
	int len = 1024;

	if (!PyArg_ParseTuple(args, "|i:read", &len))
		return NULL;

	if (!(buf = PyString_FromStringAndSize((char *) 0, len)))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	count = SSL_read(self->ssl, PyString_AsString(buf), len);
	Py_END_ALLOW_THREADS
 	if (count <= 0) {
		Py_DECREF(buf);
		return PySSL_SetError(self, count);
	}
	if (count != len && _PyString_Resize(&buf, count) < 0)
		return NULL;
	return buf;
}

static char PySSL_SSLread_doc[] =
"read([len]) -> string\n\
\n\
Read up to len bytes from the SSL socket.";

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

staticforward PyTypeObject PySSL_Type = {
	PyObject_HEAD_INIT(NULL)
	0,				/*ob_size*/
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

static char PySSL_RAND_add_doc[] =
"RAND_add(string, entropy)\n\
\n\
Mix string into the OpenSSL PRNG state.  entropy (a float) is a lower\n\
bound on the entropy contained in string.";

static PyObject *
PySSL_RAND_status(PyObject *self)
{
    return PyInt_FromLong(RAND_status());
}

static char PySSL_RAND_status_doc[] = 
"RAND_status() -> 0 or 1\n\
\n\
Returns 1 if the OpenSSL PRNG has been seeded with enough data and 0 if not.\n\
It is necessary to seed the PRNG with RAND_add() on some platforms before\n\
using the ssl() function.";

static PyObject *
PySSL_RAND_egd(PyObject *self, PyObject *arg)
{
    int bytes;

    if (!PyString_Check(arg))
	return PyErr_Format(PyExc_TypeError,
			    "RAND_egd() expected string, found %s",
			    arg->ob_type->tp_name);
    bytes = RAND_egd(PyString_AS_STRING(arg));
    if (bytes == -1) {
	PyErr_SetString(PySSLErrorObject,
			"EGD connection failed or EGD did not return "
			"enough data to seed the PRNG");
	return NULL;
    }
    return PyInt_FromLong(bytes);
}

static char PySSL_RAND_egd_doc[] = 
"RAND_egd(path) -> bytes\n\
\n\
Queries the entropy gather daemon (EGD) on socket path.  Returns number\n\
of bytes read.  Raises socket.sslerror if connection to EGD fails or\n\
if it does provide enough data to seed PRNG.";

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


static char module_doc[] =
"Implementation module for SSL socket operations.  See the socket module\n\
for documentation.";

DL_EXPORT(void)
init_ssl(void)
{
	PyObject *m, *d;

	PySSL_Type.ob_type = &PyType_Type;

	m = Py_InitModule3("_ssl", PySSL_methods, module_doc);
	d = PyModule_GetDict(m);

	/* Load _socket module and its C API */
	if (PySocketModule_ImportModuleAndAPI())
 	    	return;

	/* Init OpenSSL */
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();

	/* Add symbols to module dict */
	PySSLErrorObject = PyErr_NewException("socket.sslerror", NULL, NULL);
	if (PySSLErrorObject == NULL)
		return;
	PyDict_SetItemString(d, "sslerror", PySSLErrorObject);
	if (PyDict_SetItemString(d, "SSLType",
				 (PyObject *)&PySSL_Type) != 0)
		return;
	PyModule_AddIntConstant(m, "SSL_ERROR_ZERO_RETURN",
				SSL_ERROR_ZERO_RETURN);
	PyModule_AddIntConstant(m, "SSL_ERROR_WANT_READ",
				SSL_ERROR_WANT_READ);
	PyModule_AddIntConstant(m, "SSL_ERROR_WANT_WRITE",
				SSL_ERROR_WANT_WRITE);
	PyModule_AddIntConstant(m, "SSL_ERROR_WANT_X509_LOOKUP",
				SSL_ERROR_WANT_X509_LOOKUP);
	PyModule_AddIntConstant(m, "SSL_ERROR_SYSCALL",
				SSL_ERROR_SYSCALL);
	PyModule_AddIntConstant(m, "SSL_ERROR_SSL",
				SSL_ERROR_SSL);
}
