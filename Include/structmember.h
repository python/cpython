/* Interface to map C struct members to Python object attributes */

/* The offsetof() macro calculates the offset of a structure member
   in its structure.  Unfortunately this cannot be written down
   portably, hence it is provided by a Standard C header file.
   For pre-Standard C compilers, here is a version that usually works
   (but watch out!): */

#ifndef offsetof
#define offsetof(type, member) ( (int) & ((type*)0) -> member )
#endif

/* An array of memberlist structures defines the name, type and offset
   of selected members of a C structure.  These can be read by
   getmember() and set by setmember() (except if their READONLY flag
   is set).  The array must be terminated with an entry whose name
   pointer is NULL. */

struct memberlist {
	char *name;
	int type;
	int offset;
	int readonly;
};

/* Types */
#define T_SHORT		0
#define T_INT		1
#define T_LONG		2
#define T_FLOAT		3
#define T_DOUBLE	4
#define T_STRING	5
#define T_OBJECT	6

/* Readonly flag */
#define READONLY	1
#define RO		READONLY		/* Shorthand */

object *getmember PROTO((char *, struct memberlist *, char *));
int setmember PROTO((char *, struct memberlist *, char *, object *));
