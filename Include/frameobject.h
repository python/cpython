/* Frame object interface */

typedef struct {
	int b_type;		/* what kind of block this is */
	int b_handler;		/* where to jump to find handler */
	int b_level;		/* value stack level to pop to */
} block;

typedef struct _frame {
	OB_HEAD
	struct _frame *f_back;	/* previous frame, or NULL */
	codeobject *f_code;	/* code segment */
	object *f_globals;	/* global symbol table (dictobject) */
	object *f_locals;	/* local symbol table (dictobject) */
	object **f_valuestack;	/* malloc'ed array */
	block *f_blockstack;	/* malloc'ed array */
	int f_nvalues;		/* size of f_valuestack */
	int f_nblocks;		/* size of f_blockstack */
	int f_iblock;		/* index in f_blockstack */
} frameobject;


/* Standard object interface */

extern typeobject Frametype;

#define is_frameobject(op) ((op)->ob_type == &Frametype)

frameobject * newframeobject PROTO(
	(frameobject *, codeobject *, object *, object *, int, int));


/* The rest of the interface is specific for frame objects */

/* List access macros */

#ifdef NDEBUG
#define GETITEM(v, i) GETLISTITEM((listobject *)(v), (i))
#define GETITEMNAME(v, i) GETSTRINGVALUE((stringobject *)GETITEM((v), (i)))
#else
#define GETITEM(v, i) getlistitem((v), (i))
#define GETITEMNAME(v, i) getstringvalue(getlistitem((v), (i)))
#endif

#define GETUSTRINGVALUE(s) ((unsigned char *)GETSTRINGVALUE(s))

/* Code access macros */

#define Getconst(f, i)	(GETITEM((f)->f_code->co_consts, (i)))
#define Getname(f, i)	(GETITEMNAME((f)->f_code->co_names, (i)))


/* Block management functions */

void setup_block PROTO((frameobject *, int, int, int));
block *pop_block PROTO((frameobject *));
