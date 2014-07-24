bits shared by the bytesobject and unicodeobject implementations (and
possibly other modules, in a not too distant future).

the stuff in here is included into relevant places; see the individual
source files for details.

--------------------------------------------------------------------
the following defines used by the different modules:

STRINGLIB_CHAR

    the type used to hold a character (char or Py_UNICODE)

STRINGLIB_EMPTY

    a PyObject representing the empty string, only to be used if
    STRINGLIB_MUTABLE is 0

Py_ssize_t STRINGLIB_LEN(PyObject*)

    returns the length of the given string object (which must be of the
    right type)

PyObject* STRINGLIB_NEW(STRINGLIB_CHAR*, Py_ssize_t)

    creates a new string object

STRINGLIB_CHAR* STRINGLIB_STR(PyObject*)

    returns the pointer to the character data for the given string
    object (which must be of the right type)

int STRINGLIB_CHECK_EXACT(PyObject *)

    returns true if the object is an instance of our type, not a subclass

STRINGLIB_MUTABLE

    must be 0 or 1 to tell the cpp macros in stringlib code if the object
    being operated on is mutable or not
