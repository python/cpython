/* cryptmodule.c - by Steve Majewski
 */

#include "Python.h"

#include <sys/types.h>


/* Module crypt */


static PyObject *crypt_crypt(PyObject *self, PyObject *args)
{
	char *word, *salt; 
	extern char * crypt(const char *, const char *);

	if (!PyArg_Parse(args, "(ss)", &word, &salt)) {
		return NULL;
	}
	return PyString_FromString( crypt( word, salt ) );

}

static char crypt_crypt__doc__[] = "\
crypt(word, salt) -> string\n\
word will usually be a user's password. salt is a 2-character string\n\
which will be used to select one of 4096 variations of DES. The characters\n\
in salt must be either \".\", \"/\", or an alphanumeric character. Returns\n\
the hashed password as a string, which will be composed of characters from\n\
the same alphabet as the salt.";


static PyMethodDef crypt_methods[] = {
	{"crypt",	crypt_crypt, METH_OLDARGS, crypt_crypt__doc__},
	{NULL,		NULL}		/* sentinel */
};

DL_EXPORT(void)
initcrypt(void)
{
	Py_InitModule("crypt", crypt_methods);
}
