/* cryptmodule.c - by Steve Majewski
 */

#include "Python.h"

#include <sys/types.h>


/* Module crypt */


static PyObject *crypt_crypt(self, args)
	PyObject *self, *args;
{
	char *word, *salt; 
	extern char * crypt();

	if (!PyArg_Parse(args, "(ss)", &word, &salt)) {
		return NULL;
	}
	return PyString_FromString( crypt( word, salt ) );

}

static PyMethodDef crypt_methods[] = {
	{"crypt",	crypt_crypt},
	{NULL,		NULL}		/* sentinel */
};

DL_EXPORT(void)
initcrypt()
{
	Py_InitModule("crypt", crypt_methods);
}
