#ifndef Py_SYMTABLE_H
#define Py_SYMTABLE_H
#ifdef __cplusplus
extern "C" {
#endif

/* A symbol table is constructed each time PyNode_Compile() is
   called.  The table walks the entire parse tree and identifies each
   use or definition of a variable. 

   The symbol table contains a dictionary for each code block in a
   module: The symbol dictionary for the block.  They keys of these
   dictionaries are the name of all variables used or defined in the
   block; the integer values are used to store several flags,
   e.g. DEF_PARAM indicates that a variable is a parameter to a
   function. 
*/

struct _symtable_entry;

struct symtable {
	int st_pass;             /* pass == 1 or 2 */
	const char *st_filename; /* name of file being compiled */
	struct _symtable_entry *st_cur; /* current symbol table entry */
	PyObject *st_symbols;    /* dictionary of symbol table entries */
        PyObject *st_stack;      /* stack of namespace info */
	PyObject *st_global;     /* borrowed ref to MODULE in st_symbols */
	int st_nscopes;          /* number of scopes */
	int st_errors;           /* number of errors */
	char *st_private;        /* name of current class or NULL */
	PyFutureFeatures *st_future; /* module's future features */
};

typedef struct _symtable_entry {
	PyObject_HEAD
	PyObject *ste_id;        /* int: key in st_symbols) */
	PyObject *ste_symbols;   /* dict: name to flags) */
	PyObject *ste_name;      /* string: name of scope */
	PyObject *ste_varnames;  /* list of variable names */
	PyObject *ste_children;  /* list of child ids */
	int ste_type;            /* module, class, or function */
	int ste_lineno;          /* first line of scope */
	int ste_optimized;       /* true if namespace can't be optimized */
	int ste_nested;          /* true if scope is nested */
	int ste_child_free;      /* true if a child scope has free variables,
				    including free refs to globals */
	int ste_generator;       /* true if namespace is a generator */
	int ste_opt_lineno;      /* lineno of last exec or import * */
	int ste_tmpname;         /* temporary name counter */
	struct symtable *ste_table;
} PySymtableEntryObject;

PyAPI_DATA(PyTypeObject) PySymtableEntry_Type;

#define PySymtableEntry_Check(op) ((op)->ob_type == &PySymtableEntry_Type)

PyAPI_FUNC(PyObject *) PySymtableEntry_New(struct symtable *,
						 char *, int, int);

PyAPI_FUNC(struct symtable *) PyNode_CompileSymtable(struct _node *, const char *);
PyAPI_FUNC(void) PySymtable_Free(struct symtable *);


#define TOP "global"

/* Flags for def-use information */

#define DEF_GLOBAL 1           /* global stmt */
#define DEF_LOCAL 2            /* assignment in code block */
#define DEF_PARAM 2<<1         /* formal parameter */
#define USE 2<<2               /* name is used */
#define DEF_STAR 2<<3          /* parameter is star arg */
#define DEF_DOUBLESTAR 2<<4    /* parameter is star-star arg */
#define DEF_INTUPLE 2<<5       /* name defined in tuple in parameters */
#define DEF_FREE 2<<6          /* name used but not defined in nested scope */
#define DEF_FREE_GLOBAL 2<<7   /* free variable is actually implicit global */
#define DEF_FREE_CLASS 2<<8    /* free variable from class's method */
#define DEF_IMPORT 2<<9        /* assignment occurred via import */

#define DEF_BOUND (DEF_LOCAL | DEF_PARAM | DEF_IMPORT)

#define TYPE_FUNCTION 1
#define TYPE_CLASS 2
#define TYPE_MODULE 3

#define LOCAL 1
#define GLOBAL_EXPLICIT 2
#define GLOBAL_IMPLICIT 3
#define FREE 4
#define CELL 5

#define OPT_IMPORT_STAR 1
#define OPT_EXEC 2
#define OPT_BARE_EXEC 4

#define GENERATOR 1
#define GENERATOR_EXPRESSION 2

#ifdef __cplusplus
}
#endif
#endif /* !Py_SYMTABLE_H */
