/* String object interface */

/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

Type stringobject represents a character string.  An extra zero byte is
reserved at the end to ensure it is zero-terminated, but a size is
present so strings with null bytes in them can be represented.  This
is an immutable object type.

There are functions to create new string objects, to test
an object for string-ness, and to get the
string value.  The latter function returns a null pointer
if the object is not of the proper type.
There is a variant that takes an explicit size as well as a
variant that assumes a zero-terminated string.  Note that none of the
functions should be applied to nil objects.
*/

/* NB The type is revealed here only because it is used in dictobject.c */

typedef struct {
	OB_VARHEAD
	char ob_sval[1];
} stringobject;

extern typeobject Stringtype;

#define is_stringobject(op) ((op)->ob_type == &Stringtype)

extern object *newsizedstringobject PROTO((char *, int));
extern object *newstringobject PROTO((char *));
extern unsigned int getstringsize PROTO((object *));
extern char *getstringvalue PROTO((object *));
extern void joinstring PROTO((object **, object *));
extern int resizestring PROTO((object **, int));

/* Macro, trading safety for speed */
#define GETSTRINGVALUE(op) ((op)->ob_sval)
