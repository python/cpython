/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Stdwin module */

/* Stdwin itself is a module, not a separate object type.
   Object types defined here:
	wp: a window
	dp: a drawing structure (only one can exist at a time)
	mp: a menu
	tp: a textedit block
	bp: a bitmap
*/

/* Rules for translating C stdwin function calls into Python stwin:
   - All names drop their initial letter 'w'
   - Functions with a window as first parameter are methods of window objects
   - There is no equivalent for wclose(); just delete the window object
     (all references to it!)  (XXX maybe this is a bad idea)
   - w.begindrawing() returns a drawing object
   - There is no equivalent for wenddrawing(win); just delete the drawing
      object (all references to it!)  (XXX maybe this is a bad idea)
   - Functions that may only be used inside wbegindrawing / wendddrawing
     are methods of the drawing object; this includes the text measurement
     functions (which however have doubles as module functions).
   - Methods of the drawing object drop an initial 'draw' from their name
     if they have it, e.g., wdrawline() --> d.line()
   - The obvious type conversions: int --> intobject; string --> stringobject
   - A text parameter followed by a length parameter is only a text (string)
     parameter in Python
   - A point or other pair of horizontal and vertical coordinates is always
     a pair of integers in Python
   - Two points forming a rectangle or endpoints of a line segment are a
     pair of points in Python
   - The arguments to d.elarc() are three points.
   - The functions wgetclip() and wsetclip() are translated into
     stdwin.getcutbuffer() and stdwin.setcutbuffer(); 'clip' is really
     a bad word for what these functions do (clipping has a different
     meaning in the drawing world), while cutbuffer is standard X jargon.
     XXX This must change again in the light of changes to stdwin!
   - For textedit, similar rules hold, but they are less strict.
   XXX more?
*/

#include "Python.h"

#ifdef macintosh
#include "macglue.h"
#endif

#ifdef macintosh
#include ":::stdwin:H:stdwin.h"
#else /* !macintosh */
#include "stdwin.h"
#define HAVE_BITMAPS
#endif /* !macintosh */

#ifdef WITH_THREAD

#include "pythread.h"

static PyThread_type_lock StdwinLock; /* Lock held when interpreter not locked */

#define BGN_STDWIN Py_BEGIN_ALLOW_THREADS PyThread_acquire_lock(StdwinLock, 1);
#define RET_STDWIN PyThread_release_lock(StdwinLock); Py_BLOCK_THREADS
#define END_STDWIN PyThread_release_lock(StdwinLock); Py_END_ALLOW_THREADS

#else

#define BGN_STDWIN Py_BEGIN_ALLOW_THREADS
#define RET_STDWIN Py_BLOCK_THREADS
#define END_STDWIN Py_END_ALLOW_THREADS

#endif

#define getintarg(v,a) PyArg_Parse(v, "i", a)
#define getlongarg(v,a) PyArg_Parse(v, "l", a)
#define getstrarg(v,a) PyArg_Parse(v, "s", a)
#define getpointarg(v, a) PyArg_Parse(v, "(ii)", a, (a)+1)
#define get3pointarg(v, a) PyArg_Parse(v, "((ii)(ii)(ii))", \
                                       a, a+1, a+2, a+3, a+4, a+5)
#define getrectarg(v, a) PyArg_Parse(v, "((ii)(ii))", a, a+1, a+2, a+3)
#define getrectintarg(v, a) PyArg_Parse(v, "(((ii)(ii))i)", \
                                        a, a+1, a+2, a+3, a+4)
#define getpointintarg(v, a) PyArg_Parse(v, "((ii)i)", a, a+1, a+2)
#define getrectpointarg(v, a) PyArg_Parse(v, "(((ii)(ii))(ii))", \
                                          a, a+1, a+2, a+3, a+4, a+5)

static PyObject *StdwinError; /* Exception stdwin.error */

/* Window and menu object types declared here because of forward references */

typedef struct {
	PyObject_HEAD
	PyObject	*w_title;
	WINDOW  	*w_win;
	PyObject	*w_attr;             /* Attributes dictionary */
} windowobject;

staticforward PyTypeObject Windowtype;

#define is_windowobject(wp) ((wp)->ob_type == &Windowtype)

typedef struct {
	PyObject_HEAD
	MENU    	*m_menu;
	int     	 m_id;
	PyObject	*m_attr;             /* Attributes dictionary */
} menuobject;

staticforward PyTypeObject Menutype;

#define is_menuobject(mp) ((mp)->ob_type == &Menutype)

typedef struct {
	PyObject_HEAD
	BITMAP  	*b_bitmap;
	PyObject	*b_attr;             /* Attributes dictionary */
} bitmapobject;

staticforward PyTypeObject Bitmaptype;

#define is_bitmapobject(mp) ((mp)->ob_type == &Bitmaptype)


/* Strongly stdwin-specific argument handlers */

static int
getmenudetail(v, ep)
	PyObject *v;
	EVENT *ep;
{
	menuobject *mp;
	if (!PyArg_Parse(v, "(Oi)", &mp, &ep->u.m.item))
		return 0;
	if (!is_menuobject(mp))
		return PyErr_BadArgument();
	ep->u.m.id = mp->m_id;
	return 1;
}

static int
geteventarg(v, ep)
	PyObject *v;
	EVENT *ep;
{
	PyObject *wp, *detail;
	int a[4];
	if (!PyArg_Parse(v, "(iOO)", &ep->type, &wp, &detail))
		return 0;
	if (is_windowobject(wp))
		ep->window = ((windowobject *)wp) -> w_win;
	else if (wp == Py_None)
		ep->window = NULL;
	else
		return PyErr_BadArgument();
	switch (ep->type) {
	case WE_CHAR: {
			char c;
			if (!PyArg_Parse(detail, "c", &c))
				return 0;
			ep->u.character = c;
			return 1;
		}
	case WE_COMMAND:
		return getintarg(detail, &ep->u.command);
	case WE_DRAW:
		if (!getrectarg(detail, a))
			return 0;
		ep->u.area.left = a[0];
		ep->u.area.top = a[1];
		ep->u.area.right = a[2];
		ep->u.area.bottom = a[3];
		return 1;
	case WE_MOUSE_DOWN:
	case WE_MOUSE_UP:
	case WE_MOUSE_MOVE:
		return PyArg_Parse(detail, "((ii)iii)",
                                   &ep->u.where.h, &ep->u.where.v,
                                   &ep->u.where.clicks,
                                   &ep->u.where.button,
                                   &ep->u.where.mask);
	case WE_MENU:
		return getmenudetail(detail, ep);
	case WE_KEY:
		return PyArg_Parse(detail, "(ii)",
                                   &ep->u.key.code, &ep->u.key.mask);
	default:
		return 1;
	}
}


/* Return construction tools */

static PyObject *
makepoint(a, b)
	int a, b;
{
	return Py_BuildValue("(ii)", a, b);
}

static PyObject *
makerect(a, b, c, d)
	int a, b, c, d;
{
	return Py_BuildValue("((ii)(ii))", a, b, c, d);
}


/* Drawing objects */

typedef struct {
	PyObject_HEAD
	windowobject	*d_ref;
} drawingobject;

static drawingobject *Drawing; /* Set to current drawing object, or NULL */

/* Drawing methods */

static PyObject *
drawing_close(dp)
	drawingobject *dp;
{
	if (dp->d_ref != NULL) {
		wenddrawing(dp->d_ref->w_win);
		Drawing = NULL;
		Py_DECREF(dp->d_ref);
		dp->d_ref = NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static void
drawing_dealloc(dp)
	drawingobject *dp;
{
	if (dp->d_ref != NULL) {
		wenddrawing(dp->d_ref->w_win);
		Drawing = NULL;
		Py_DECREF(dp->d_ref);
		dp->d_ref = NULL;
	}
	free((char *)dp);
}

static PyObject *
drawing_generic(dp, args, func)
	drawingobject *dp;
	PyObject *args;
	void (*func) Py_FPROTO((int, int, int, int));
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	(*func)(a[0], a[1], a[2], a[3]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_line(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	return drawing_generic(dp, args, wdrawline);
}

static PyObject *
drawing_xorline(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	return drawing_generic(dp, args, wxorline);
}

static PyObject *
drawing_circle(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int a[3];
	if (!getpointintarg(args, a))
		return NULL;
	wdrawcircle(a[0], a[1], a[2]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_fillcircle(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int a[3];
	if (!getpointintarg(args, a))
		return NULL;
	wfillcircle(a[0], a[1], a[2]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_xorcircle(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int a[3];
	if (!getpointintarg(args, a))
		return NULL;
	wxorcircle(a[0], a[1], a[2]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_elarc(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int a[6];
	if (!get3pointarg(args, a))
		return NULL;
	wdrawelarc(a[0], a[1], a[2], a[3], a[4], a[5]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_fillelarc(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int a[6];
	if (!get3pointarg(args, a))
		return NULL;
	wfillelarc(a[0], a[1], a[2], a[3], a[4], a[5]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_xorelarc(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int a[6];
	if (!get3pointarg(args, a))
		return NULL;
	wxorelarc(a[0], a[1], a[2], a[3], a[4], a[5]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_box(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	return drawing_generic(dp, args, wdrawbox);
}

static PyObject *
drawing_erase(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	return drawing_generic(dp, args, werase);
}

static PyObject *
drawing_paint(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	return drawing_generic(dp, args, wpaint);
}

static PyObject *
drawing_invert(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	return drawing_generic(dp, args, winvert);
}

static POINT *
getpointsarray(v, psize)
	PyObject *v;
	int *psize;
{
	int n = -1;
	PyObject * (*getitem) Py_PROTO((PyObject *, int));
	int i;
	POINT *points;

	if (v == NULL)
		;
	else if (PyList_Check(v)) {
		n = PyList_Size(v);
		getitem = PyList_GetItem;
	}
	else if (PyTuple_Check(v)) {
		n = PyTuple_Size(v);
		getitem = PyTuple_GetItem;
	}

	if (n <= 0) {
		(void) PyErr_BadArgument();
		return NULL;
	}

	points = PyMem_NEW(POINT, n);
	if (points == NULL) {
		(void) PyErr_NoMemory();
		return NULL;
	}

	for (i = 0; i < n; i++) {
		PyObject *w = (*getitem)(v, i);
		int a[2];
		if (!getpointarg(w, a)) {
			PyMem_DEL(points);
			return NULL;
		}
		points[i].h = a[0];
		points[i].v = a[1];
	}

	*psize = n;
	return points;
}

static PyObject *
drawing_poly(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int n;
	POINT *points = getpointsarray(args, &n);
	if (points == NULL)
		return NULL;
	wdrawpoly(n, points);
	PyMem_DEL(points);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_fillpoly(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int n;
	POINT *points = getpointsarray(args, &n);
	if (points == NULL)
		return NULL;
	wfillpoly(n, points);
	PyMem_DEL(points);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_xorpoly(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int n;
	POINT *points = getpointsarray(args, &n);
	if (points == NULL)
		return NULL;
	wxorpoly(n, points);
	PyMem_DEL(points);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_cliprect(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	return drawing_generic(dp, args, wcliprect);
}

static PyObject *
drawing_noclip(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	wnoclip();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_shade(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int a[5];
	if (!getrectintarg(args, a))
		return NULL;
	wshade(a[0], a[1], a[2], a[3], a[4]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_text(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	int h, v, size;
	char *text;
	if (!PyArg_Parse(args, "((ii)t#)", &h, &v, &text, &size))
		return NULL;
	wdrawtext(h, v, text, size);
	Py_INCREF(Py_None);
	return Py_None;
}

/* The following four are also used as stdwin functions */

static PyObject *
drawing_lineheight(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)wlineheight());
}

static PyObject *
drawing_baseline(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)wbaseline());
}

static PyObject *
drawing_textwidth(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	char *text;
	int size;
	if (!PyArg_Parse(args, "t#", &text, &size))
		return NULL;
	return PyInt_FromLong((long)wtextwidth(text, size));
}

static PyObject *
drawing_textbreak(dp, args)
	drawingobject *dp;
	PyObject *args;
{
	char *text;
	int size, width;
	if (!PyArg_Parse(args, "(t#i)", &text, &size, &width))
		return NULL;
	return PyInt_FromLong((long)wtextbreak(text, size, width));
}

static PyObject *
drawing_setfont(self, args)
	drawingobject *self;
	PyObject *args;
{
	char *font;
	char style = '\0';
	int size = 0;
	if (args == NULL || !PyTuple_Check(args)) {
		if (!PyArg_Parse(args, "z", &font))
			return NULL;
	}
	else {
		int n = PyTuple_Size(args);
		if (n == 2) {
			if (!PyArg_Parse(args, "(zi)", &font, &size))
				return NULL;
		}
		else if (!PyArg_Parse(args, "(zic)", &font, &size, &style)) {
			PyErr_Clear();
			if (!PyArg_Parse(args, "(zci)", &font, &style, &size))
				return NULL;
		}
	}
	if (font != NULL) {
		if (!wsetfont(font)) {
			PyErr_SetString(StdwinError, "font not found");
			return NULL;
		}
	}
	if (size != 0)
		wsetsize(size);
	switch (style) {
	case 'b':
		wsetbold();
		break;
	case 'i':
		wsetitalic();
		break;
	case 'o':
		wsetbolditalic();
		break;
	case 'u':
		wsetunderline();
		break;
	case 'p':
		wsetplain();
		break;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_getbgcolor(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)wgetbgcolor());
}

static PyObject *
drawing_getfgcolor(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long)wgetfgcolor());
}

static PyObject *
drawing_setbgcolor(self, args)
	PyObject *self;
	PyObject *args;
{
	long color;
	if (!getlongarg(args, &color))
		return NULL;
	wsetbgcolor((COLOR)color);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
drawing_setfgcolor(self, args)
	PyObject *self;
	PyObject *args;
{
	long color;
	if (!getlongarg(args, &color))
		return NULL;
	wsetfgcolor((COLOR)color);
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef HAVE_BITMAPS

static PyObject *
drawing_bitmap(self, args)
	PyObject *self;
	PyObject *args;
{
	int h, v;
	PyObject *bp;
	PyObject *mask = NULL;
	if (!PyArg_Parse(args, "((ii)O)", &h, &v, &bp)) {
		PyErr_Clear();
		if (!PyArg_Parse(args, "((ii)OO)", &h, &v, &bp, &mask))
			return NULL;
		if (mask == Py_None)
			mask = NULL;
		else if (!is_bitmapobject(mask)) {
			PyErr_BadArgument();
			return NULL;
		}
	}
	if (!is_bitmapobject(bp)) {
		PyErr_BadArgument();
		return NULL;
	}
	if (((bitmapobject *)bp)->b_bitmap == NULL ||
	    mask != NULL && ((bitmapobject *)mask)->b_bitmap == NULL) {
		PyErr_SetString(StdwinError, "bitmap object already close");
		return NULL;
	}
	if (mask == NULL)
		wdrawbitmap(h, v, ((bitmapobject *)bp)->b_bitmap, ALLBITS);
	else
		wdrawbitmap(h, v,
			    ((bitmapobject *)bp)->b_bitmap,
			    ((bitmapobject *)bp)->b_bitmap);
	Py_INCREF(Py_None);
	return Py_None;
}

#endif /* HAVE_BITMAPS */

static PyMethodDef drawing_methods[] = {
#ifdef HAVE_BITMAPS
	{"bitmap",	(PyCFunction)drawing_bitmap},
#endif
	{"box",		(PyCFunction)drawing_box},
	{"circle",	(PyCFunction)drawing_circle},
	{"cliprect",	(PyCFunction)drawing_cliprect},
	{"close",	(PyCFunction)drawing_close},
	{"elarc",	(PyCFunction)drawing_elarc},
	{"enddrawing",	(PyCFunction)drawing_close},
	{"erase",	(PyCFunction)drawing_erase},
	{"fillcircle",	(PyCFunction)drawing_fillcircle},
	{"fillelarc",	(PyCFunction)drawing_fillelarc},
	{"fillpoly",	(PyCFunction)drawing_fillpoly},
	{"invert",	(PyCFunction)drawing_invert},
	{"line",	(PyCFunction)drawing_line},
	{"noclip",	(PyCFunction)drawing_noclip},
	{"paint",	(PyCFunction)drawing_paint},
	{"poly",	(PyCFunction)drawing_poly},
	{"shade",	(PyCFunction)drawing_shade},
	{"text",	(PyCFunction)drawing_text},
	{"xorcircle",	(PyCFunction)drawing_xorcircle},
	{"xorelarc",	(PyCFunction)drawing_xorelarc},
	{"xorline",	(PyCFunction)drawing_xorline},
	{"xorpoly",	(PyCFunction)drawing_xorpoly},
	
	/* Text measuring methods: */
	{"baseline",	(PyCFunction)drawing_baseline},
	{"lineheight",	(PyCFunction)drawing_lineheight},
	{"textbreak",	(PyCFunction)drawing_textbreak},
	{"textwidth",	(PyCFunction)drawing_textwidth},

	/* Font setting methods: */
	{"setfont",	(PyCFunction)drawing_setfont},
	
	/* Color methods: */
	{"getbgcolor",	(PyCFunction)drawing_getbgcolor},
	{"getfgcolor",	(PyCFunction)drawing_getfgcolor},
	{"setbgcolor",	(PyCFunction)drawing_setbgcolor},
	{"setfgcolor",	(PyCFunction)drawing_setfgcolor},

	{NULL,		NULL}		/* sentinel */
};

static PyObject *
drawing_getattr(dp, name)
	drawingobject *dp;
	char *name;
{
	if (dp->d_ref == NULL) {
		PyErr_SetString(StdwinError, "drawing object already closed");
		return NULL;
	}
	return Py_FindMethod(drawing_methods, (PyObject *)dp, name);
}

PyTypeObject Drawingtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"drawing",		/*tp_name*/
	sizeof(drawingobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)drawing_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)drawing_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};


/* Text(edit) objects */

typedef struct {
	PyObject_HEAD
	TEXTEDIT	*t_text;
	windowobject	*t_ref;
	PyObject	*t_attr;	     /* Attributes dictionary */
} textobject;

staticforward PyTypeObject Texttype;

static textobject *
newtextobject(wp, left, top, right, bottom)
	windowobject *wp;
	int left, top, right, bottom;
{
	textobject *tp;
	tp = PyObject_NEW(textobject, &Texttype);
	if (tp == NULL)
		return NULL;
	tp->t_attr = NULL;
	Py_INCREF(wp);
	tp->t_ref = wp;
	tp->t_text = tecreate(wp->w_win, left, top, right, bottom);
	if (tp->t_text == NULL) {
		Py_DECREF(tp);
		return (textobject *) PyErr_NoMemory();
	}
	return tp;
}

/* Text(edit) methods */

static void
text_dealloc(tp)
	textobject *tp;
{
	if (tp->t_text != NULL)
		tefree(tp->t_text);
	Py_XDECREF(tp->t_attr);
	Py_XDECREF(tp->t_ref);
	PyMem_DEL(tp);
}

static PyObject *
text_close(tp, args)
	textobject *tp;
	PyObject *args;
{
	if (tp->t_text != NULL) {
		tefree(tp->t_text);
		tp->t_text = NULL;
	}
	if (tp->t_attr != NULL) {
		Py_DECREF(tp->t_attr);
		tp->t_attr = NULL;
	}
	if (tp->t_ref != NULL) {
		Py_DECREF(tp->t_ref);
		tp->t_ref = NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
text_arrow(self, args)
	textobject *self;
	PyObject *args;
{
	int code;
	if (!getintarg(args, &code))
		return NULL;
	tearrow(self->t_text, code);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
text_draw(self, args)
	textobject *self;
	PyObject *args;
{
	register TEXTEDIT *tp = self->t_text;
	int a[4];
	int left, top, right, bottom;
	if (!getrectarg(args, a))
		return NULL;
	if (Drawing != NULL) {
		PyErr_SetString(StdwinError, "already drawing");
		return NULL;
	}
	/* Clip to text area and ignore if area is empty */
	left = tegetleft(tp);
	top = tegettop(tp);
	right = tegetright(tp);
	bottom = tegetbottom(tp);
	if (a[0] < left) a[0] = left;
	if (a[1] < top) a[1] = top;
	if (a[2] > right) a[2] = right;
	if (a[3] > bottom) a[3] = bottom;
	if (a[0] < a[2] && a[1] < a[3]) {
		wbegindrawing(self->t_ref->w_win);
		tedrawnew(tp, a[0], a[1], a[2], a[3]);
		wenddrawing(self->t_ref->w_win);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
text_event(self, args)
	textobject *self;
	PyObject *args;
{
	register TEXTEDIT *tp = self->t_text;
	EVENT e;
	if (!geteventarg(args, &e))
		return NULL;
	if (e.type == WE_MOUSE_DOWN) {
		/* Cheat at the margins */
		int width, height;
		wgetdocsize(e.window, &width, &height);
		if (e.u.where.h < 0 && tegetleft(tp) == 0)
			e.u.where.h = 0;
		else if (e.u.where.h > width && tegetright(tp) == width)
			e.u.where.h = width;
		if (e.u.where.v < 0 && tegettop(tp) == 0)
			e.u.where.v = 0;
		else if (e.u.where.v > height && tegetright(tp) == height)
			e.u.where.v = height;
	}
	return PyInt_FromLong((long) teevent(tp, &e));
}

static PyObject *
text_getfocus(self, args)
	textobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return makepoint(tegetfoc1(self->t_text), tegetfoc2(self->t_text));
}

static PyObject *
text_getfocustext(self, args)
	textobject *self;
	PyObject *args;
{
	int f1, f2;
	char *text;
	if (!PyArg_NoArgs(args))
		return NULL;
	f1 = tegetfoc1(self->t_text);
	f2 = tegetfoc2(self->t_text);
	text = tegettext(self->t_text);
	return PyString_FromStringAndSize(text + f1, f2-f1);
}

static PyObject *
text_getrect(self, args)
	textobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return makerect(tegetleft(self->t_text),
			tegettop(self->t_text),
			tegetright(self->t_text),
			tegetbottom(self->t_text));
}

static PyObject *
text_gettext(self, args)
	textobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyString_FromStringAndSize(tegettext(self->t_text),
					  tegetlen(self->t_text));
}

static PyObject *
text_move(self, args)
	textobject *self;
	PyObject *args;
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	temovenew(self->t_text, a[0], a[1], a[2], a[3]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
text_replace(self, args)
	textobject *self;
	PyObject *args;
{
	char *text;
	if (!getstrarg(args, &text))
		return NULL;
	tereplace(self->t_text, text);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
text_setactive(self, args)
	textobject *self;
	PyObject *args;
{
	int flag;
	if (!getintarg(args, &flag))
		return NULL;
	tesetactive(self->t_text, flag);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
text_setfocus(self, args)
	textobject *self;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	tesetfocus(self->t_text, a[0], a[1]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
text_settext(self, args)
	textobject *self;
	PyObject *args;
{
	char *text;
	char *buf;
	int size;
	if (!PyArg_Parse(args, "t#", &text, &size))
		return NULL;
	if ((buf = PyMem_NEW(char, size)) == NULL) {
		return PyErr_NoMemory();
	}
	memcpy(buf, text, size);
	tesetbuf(self->t_text, buf, size); /* Becomes owner of buffer */
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
text_setview(self, args)
	textobject *self;
	PyObject *args;
{
	int a[4];
	if (args == Py_None)
		tenoview(self->t_text);
	else {
		if (!getrectarg(args, a))
			return NULL;
		tesetview(self->t_text, a[0], a[1], a[2], a[3]);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef text_methods[] = {
	{"arrow",	(PyCFunction)text_arrow},
	{"close",	(PyCFunction)text_close},
	{"draw",	(PyCFunction)text_draw},
	{"event",	(PyCFunction)text_event},
	{"getfocus",	(PyCFunction)text_getfocus},
	{"getfocustext",(PyCFunction)text_getfocustext},
	{"getrect",	(PyCFunction)text_getrect},
	{"gettext",	(PyCFunction)text_gettext},
	{"move",	(PyCFunction)text_move},
	{"replace",	(PyCFunction)text_replace},
	{"setactive",	(PyCFunction)text_setactive},
	{"setfocus",	(PyCFunction)text_setfocus},
	{"settext",	(PyCFunction)text_settext},
	{"setview",	(PyCFunction)text_setview},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
text_getattr(tp, name)
	textobject *tp;
	char *name;
{
	PyObject *v = NULL;
	if (tp->t_ref == NULL) {
		PyErr_SetString(StdwinError, "text object already closed");
		return NULL;
	}
	if (strcmp(name, "__dict__") == 0) {
		v = tp->t_attr;
		if (v == NULL)
			v = Py_None;
	}
	else if (tp->t_attr != NULL) {
		v = PyDict_GetItemString(tp->t_attr, name);
	}
	if (v != NULL) {
		Py_INCREF(v);
		return v;
	}
	return Py_FindMethod(text_methods, (PyObject *)tp, name);
}

static int
text_setattr(tp, name, v)
	textobject *tp;
	char *name;
	PyObject *v;
{
	if (tp->t_attr == NULL) {
		tp->t_attr = PyDict_New();
		if (tp->t_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(tp->t_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
				  "delete non-existing text object attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(tp->t_attr, name, v);
}

statichere PyTypeObject Texttype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"textedit",		/*tp_name*/
	sizeof(textobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)text_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)text_getattr, /*tp_getattr*/
	(setattrfunc)text_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};


/* Menu objects */

#define IDOFFSET 10		/* Menu IDs we use start here */
#define MAXNMENU 200		/* Max #menus we allow */
static menuobject *menulist[MAXNMENU];

static menuobject *newmenuobject Py_PROTO((char *));
static menuobject *
newmenuobject(title)
	char *title;
{
	int id;
	MENU *menu;
	menuobject *mp;
	for (id = 0; id < MAXNMENU; id++) {
		if (menulist[id] == NULL)
			break;
	}
	if (id >= MAXNMENU) {
		PyErr_SetString(StdwinError, "creating too many menus");
		return NULL;
	}
	menu = wmenucreate(id + IDOFFSET, title);
	if (menu == NULL)
		return (menuobject *) PyErr_NoMemory();
	mp = PyObject_NEW(menuobject, &Menutype);
	if (mp != NULL) {
		mp->m_menu = menu;
		mp->m_id = id + IDOFFSET;
		mp->m_attr = NULL;
		menulist[id] = mp;
	}
	else
		wmenudelete(menu);
	return mp;
}

/* Menu methods */

static void
menu_dealloc(mp)
	menuobject *mp;
{
	
	int id = mp->m_id - IDOFFSET;
	if (id >= 0 && id < MAXNMENU && menulist[id] == mp) {
		menulist[id] = NULL;
	}
	if (mp->m_menu != NULL)
		wmenudelete(mp->m_menu);
	Py_XDECREF(mp->m_attr);
	PyMem_DEL(mp);
}

static PyObject *
menu_close(mp, args)
	menuobject *mp;
	PyObject *args;
{
	int id = mp->m_id - IDOFFSET;
	if (id >= 0 && id < MAXNMENU && menulist[id] == mp) {
		menulist[id] = NULL;
	}
	mp->m_id = -1;
	if (mp->m_menu != NULL)
		wmenudelete(mp->m_menu);
	mp->m_menu = NULL;
	Py_XDECREF(mp->m_attr);
	mp->m_attr = NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
menu_additem(self, args)
	menuobject *self;
	PyObject *args;
{
	char *text;
	int shortcut = -1;
	if (PyTuple_Check(args)) {
		char c;
		if (!PyArg_Parse(args, "(sc)", &text, &c))
			return NULL;
		shortcut = c;
	}
	else if (!getstrarg(args, &text))
		return NULL;
	wmenuadditem(self->m_menu, text, shortcut);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
menu_setitem(self, args)
	menuobject *self;
	PyObject *args;
{
	int index;
	char *text;
	if (!PyArg_Parse(args, "(is)", &index, &text))
		return NULL;
	wmenusetitem(self->m_menu, index, text);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
menu_enable(self, args)
	menuobject *self;
	PyObject *args;
{
	int index;
	int flag;
	if (!PyArg_Parse(args, "(ii)", &index, &flag))
		return NULL;
	wmenuenable(self->m_menu, index, flag);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
menu_check(self, args)
	menuobject *self;
	PyObject *args;
{
	int index;
	int flag;
	if (!PyArg_Parse(args, "(ii)", &index, &flag))
		return NULL;
	wmenucheck(self->m_menu, index, flag);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef menu_methods[] = {
	{"additem",	(PyCFunction)menu_additem},
	{"setitem",	(PyCFunction)menu_setitem},
	{"enable",	(PyCFunction)menu_enable},
	{"check",	(PyCFunction)menu_check},
	{"close",	(PyCFunction)menu_close},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
menu_getattr(mp, name)
	menuobject *mp;
	char *name;
{
	PyObject *v = NULL;
	if (mp->m_menu == NULL) {
		PyErr_SetString(StdwinError, "menu object already closed");
		return NULL;
	}
	if (strcmp(name, "__dict__") == 0) {
		v = mp->m_attr;
		if (v == NULL)
			v = Py_None;
	}
	else if (mp->m_attr != NULL) {
		v = PyDict_GetItemString(mp->m_attr, name);
	}
	if (v != NULL) {
		Py_INCREF(v);
		return v;
	}
	return Py_FindMethod(menu_methods, (PyObject *)mp, name);
}

static int
menu_setattr(mp, name, v)
	menuobject *mp;
	char *name;
	PyObject *v;
{
	if (mp->m_attr == NULL) {
		mp->m_attr = PyDict_New();
		if (mp->m_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(mp->m_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
				  "delete non-existing menu object attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(mp->m_attr, name, v);
}

statichere PyTypeObject Menutype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"menu",			/*tp_name*/
	sizeof(menuobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)menu_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)menu_getattr, /*tp_getattr*/
	(setattrfunc)menu_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};


#ifdef HAVE_BITMAPS

/* Bitmaps objects */

static bitmapobject *newbitmapobject Py_PROTO((int, int));
static bitmapobject *
newbitmapobject(width, height)
	int width, height;
{
	BITMAP *bitmap;
	bitmapobject *bp;
	bitmap = wnewbitmap(width, height);
	if (bitmap == NULL)
		return (bitmapobject *) PyErr_NoMemory();
	bp = PyObject_NEW(bitmapobject, &Bitmaptype);
	if (bp != NULL) {
		bp->b_bitmap = bitmap;
		bp->b_attr = NULL;
	}
	else
		wfreebitmap(bitmap);
	return bp;
}

/* Bitmap methods */

static void
bitmap_dealloc(bp)
	bitmapobject *bp;
{
	if (bp->b_bitmap != NULL)
		wfreebitmap(bp->b_bitmap);
	Py_XDECREF(bp->b_attr);
	PyMem_DEL(bp);
}

static PyObject *
bitmap_close(bp, args)
	bitmapobject *bp;
	PyObject *args;
{
	if (bp->b_bitmap != NULL)
		wfreebitmap(bp->b_bitmap);
	bp->b_bitmap = NULL;
	Py_XDECREF(bp->b_attr);
	bp->b_attr = NULL;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
bitmap_setbit(self, args)
	bitmapobject *self;
	PyObject *args;
{
	int a[3];
	if (!getpointintarg(args, a))
		return NULL;
	wsetbit(self->b_bitmap, a[0], a[1], a[2]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
bitmap_getbit(self, args)
	bitmapobject *self;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	return PyInt_FromLong((long) wgetbit(self->b_bitmap, a[0], a[1]));
}

static PyObject *
bitmap_getsize(self, args)
	bitmapobject *self;
	PyObject *args;
{
	int width, height;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetbitmapsize(self->b_bitmap, &width, &height);
	return Py_BuildValue("(ii)", width, height);
}

static PyMethodDef bitmap_methods[] = {
	{"close",	(PyCFunction)bitmap_close},
	{"getsize",	(PyCFunction)bitmap_getsize},
	{"getbit",	(PyCFunction)bitmap_getbit},
	{"setbit",	(PyCFunction)bitmap_setbit},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
bitmap_getattr(bp, name)
	bitmapobject *bp;
	char *name;
{
	PyObject *v = NULL;
	if (bp->b_bitmap == NULL) {
		PyErr_SetString(StdwinError, "bitmap object already closed");
		return NULL;
	}
	if (strcmp(name, "__dict__") == 0) {
		v = bp->b_attr;
		if (v == NULL)
			v = Py_None;
	}
	else if (bp->b_attr != NULL) {
		v = PyDict_GetItemString(bp->b_attr, name);
	}
	if (v != NULL) {
		Py_INCREF(v);
		return v;
	}
	return Py_FindMethod(bitmap_methods, (PyObject *)bp, name);
}

static int
bitmap_setattr(bp, name, v)
	bitmapobject *bp;
	char *name;
	PyObject *v;
{
	if (bp->b_attr == NULL) {
		bp->b_attr = PyDict_New();
		if (bp->b_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(bp->b_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
			        "delete non-existing bitmap object attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(bp->b_attr, name, v);
}

statichere PyTypeObject Bitmaptype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"bitmap",			/*tp_name*/
	sizeof(bitmapobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)bitmap_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)bitmap_getattr, /*tp_getattr*/
	(setattrfunc)bitmap_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

#endif /* HAVE_BITMAPS */


/* Windows */

#define MAXNWIN 50
static windowobject *windowlist[MAXNWIN];

/* Window methods */

static void
window_dealloc(wp)
	windowobject *wp;
{
	if (wp->w_win != NULL) {
		int tag = wgettag(wp->w_win);
		if (tag >= 0 && tag < MAXNWIN)
			windowlist[tag] = NULL;
		else
			fprintf(stderr, "XXX help! tag %d in window_dealloc\n",
				tag);
		wclose(wp->w_win);
	}
	Py_DECREF(wp->w_title);
	if (wp->w_attr != NULL)
		Py_DECREF(wp->w_attr);
	free((char *)wp);
}

static PyObject *
window_close(wp, args)
	windowobject *wp;
	PyObject *args;
{
	if (wp->w_win != NULL) {
		int tag = wgettag(wp->w_win);
		if (tag >= 0 && tag < MAXNWIN)
			windowlist[tag] = NULL;
		wclose(wp->w_win);
		wp->w_win = NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_begindrawing(wp, args)
	windowobject *wp;
	PyObject *args;
{
	drawingobject *dp;
	if (!PyArg_NoArgs(args))
		return NULL;
	if (Drawing != NULL) {
		PyErr_SetString(StdwinError, "already drawing");
		return NULL;
	}
	dp = PyObject_NEW(drawingobject, &Drawingtype);
	if (dp == NULL)
		return NULL;
	Drawing = dp;
	Py_INCREF(wp);
	dp->d_ref = wp;
	wbegindrawing(wp->w_win);
	return (PyObject *)dp;
}

static PyObject *
window_change(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	wchange(wp->w_win, a[0], a[1], a[2], a[3]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_gettitle(wp, args)
	windowobject *wp;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	Py_INCREF(wp->w_title);
	return wp->w_title;
}

static PyObject *
window_getwinpos(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int h, v;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetwinpos(wp->w_win, &h, &v);
	return makepoint(h, v);
}

static PyObject *
window_getwinsize(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int width, height;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetwinsize(wp->w_win, &width, &height);
	return makepoint(width, height);
}

static PyObject *
window_setwinpos(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetwinpos(wp->w_win, a[0], a[1]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_setwinsize(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetwinsize(wp->w_win, a[0], a[1]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_getdocsize(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int width, height;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetdocsize(wp->w_win, &width, &height);
	return makepoint(width, height);
}

static PyObject *
window_getorigin(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int width, height;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetorigin(wp->w_win, &width, &height);
	return makepoint(width, height);
}

static PyObject *
window_scroll(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int a[6];
	if (!getrectpointarg(args, a))
		return NULL;
	wscroll(wp->w_win, a[0], a[1], a[2], a[3], a[4], a[5]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_setdocsize(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetdocsize(wp->w_win, a[0], a[1]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_setorigin(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetorigin(wp->w_win, a[0], a[1]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_settitle(wp, args)
	windowobject *wp;
	PyObject *args;
{
	PyObject *title;
	if (!PyArg_Parse(args, "S", &title))
		return NULL;
	Py_DECREF(wp->w_title);
	Py_INCREF(title);
	wp->w_title = title;
	wsettitle(wp->w_win, PyString_AsString(title));
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_show(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	wshow(wp->w_win, a[0], a[1], a[2], a[3]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_settimer(wp, args)
	windowobject *wp;
	PyObject *args;
{
	int a;
	if (!getintarg(args, &a))
		return NULL;
	wsettimer(wp->w_win, a);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_menucreate(self, args)
	windowobject *self;
	PyObject *args;
{
	menuobject *mp;
	char *title;
	if (!getstrarg(args, &title))
		return NULL;
	wmenusetdeflocal(1);
	mp = newmenuobject(title);
	if (mp == NULL)
		return NULL;
	wmenuattach(self->w_win, mp->m_menu);
	return (PyObject *)mp;
}

static PyObject *
window_textcreate(self, args)
	windowobject *self;
	PyObject *args;
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	return (PyObject *)newtextobject(self, a[0], a[1], a[2], a[3]);
}

static PyObject *
window_setselection(self, args)
	windowobject *self;
	PyObject *args;
{
	int sel, size, ok;
	char *text;
	if (!PyArg_Parse(args, "(it#)", &sel, &text, &size))
		return NULL;
	ok = wsetselection(self->w_win, sel, text, size);
	return PyInt_FromLong(ok);
}

static PyObject *
window_setwincursor(self, args)
	windowobject *self;
	PyObject *args;
{
	char *name;
	CURSOR *c;
	if (!PyArg_Parse(args, "z", &name))
		return NULL;
	if (name == NULL)
		c = NULL;
	else {
		c = wfetchcursor(name);
		if (c == NULL) {
			PyErr_SetString(StdwinError, "no such cursor");
			return NULL;
		}
	}
	wsetwincursor(self->w_win, c);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
window_setactive(self, args)
	windowobject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	wsetactive(self->w_win);
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef CWI_HACKS
static PyObject *
window_getxwindowid(self, args)
	windowobject *self;
	PyObject *args;
{
	long wid = wgetxwindowid(self->w_win);
	return PyInt_FromLong(wid);
}
#endif

static PyMethodDef window_methods[] = {
	{"begindrawing",(PyCFunction)window_begindrawing},
	{"change",	(PyCFunction)window_change},
	{"close",	(PyCFunction)window_close},
	{"getdocsize",	(PyCFunction)window_getdocsize},
	{"getorigin",	(PyCFunction)window_getorigin},
	{"gettitle",	(PyCFunction)window_gettitle},
	{"getwinpos",	(PyCFunction)window_getwinpos},
	{"getwinsize",	(PyCFunction)window_getwinsize},
	{"menucreate",	(PyCFunction)window_menucreate},
	{"scroll",	(PyCFunction)window_scroll},
	{"setactive",	(PyCFunction)window_setactive},
	{"setdocsize",	(PyCFunction)window_setdocsize},
	{"setorigin",	(PyCFunction)window_setorigin},
	{"setselection",(PyCFunction)window_setselection},
	{"settimer",	(PyCFunction)window_settimer},
	{"settitle",	(PyCFunction)window_settitle},
	{"setwincursor",(PyCFunction)window_setwincursor},
	{"setwinpos",	(PyCFunction)window_setwinpos},
	{"setwinsize",	(PyCFunction)window_setwinsize},
	{"show",	(PyCFunction)window_show},
	{"textcreate",	(PyCFunction)window_textcreate},
#ifdef CWI_HACKS
	{"getxwindowid",(PyCFunction)window_getxwindowid},
#endif
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
window_getattr(wp, name)
	windowobject *wp;
	char *name;
{
	PyObject *v = NULL;
	if (wp->w_win == NULL) {
		PyErr_SetString(StdwinError, "window already closed");
		return NULL;
	}
	if (strcmp(name, "__dict__") == 0) {
		v = wp->w_attr;
		if (v == NULL)
			v = Py_None;
	}
	else if (wp->w_attr != NULL) {
		v = PyDict_GetItemString(wp->w_attr, name);
	}
	if (v != NULL) {
		Py_INCREF(v);
		return v;
	}
	return Py_FindMethod(window_methods, (PyObject *)wp, name);
}

static int
window_setattr(wp, name, v)
	windowobject *wp;
	char *name;
	PyObject *v;
{
	if (wp->w_attr == NULL) {
		wp->w_attr = PyDict_New();
		if (wp->w_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = PyDict_DelItemString(wp->w_attr, name);
		if (rv < 0)
			PyErr_SetString(PyExc_AttributeError,
			          "delete non-existing menu object attribute");
		return rv;
	}
	else
		return PyDict_SetItemString(wp->w_attr, name, v);
}

statichere PyTypeObject Windowtype = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"window",		/*tp_name*/
	sizeof(windowobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)window_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)window_getattr, /*tp_getattr*/
	(setattrfunc)window_setattr, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

/* Stdwin methods */

static PyObject *
stdwin_done(sw, args)
	PyObject *sw;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	wdone();
	/* XXX There is no protection against continued use of
	   XXX stdwin functions or objects after this call is made.
	   XXX Use at own risk */
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_open(sw, args)
	PyObject *sw;
	PyObject *args;
{
	int tag;
	PyObject *title;
	windowobject *wp;
	if (!PyArg_Parse(args, "S", &title))
		return NULL;
	for (tag = 0; tag < MAXNWIN; tag++) {
		if (windowlist[tag] == NULL)
			break;
	}
	if (tag >= MAXNWIN) {
		PyErr_SetString(StdwinError, "creating too many windows");
		return NULL;
	}
	wp = PyObject_NEW(windowobject, &Windowtype);
	if (wp == NULL)
		return NULL;
	Py_INCREF(title);
	wp->w_title = title;
	wp->w_win = wopen(PyString_AsString(title), (void (*)()) NULL);
	wp->w_attr = NULL;
	if (wp->w_win == NULL) {
		Py_DECREF(wp);
		return NULL;
	}
	windowlist[tag] = wp;
	wsettag(wp->w_win, tag);
	return (PyObject *)wp;
}

static PyObject *
window2object(win)
	WINDOW *win;
{
	PyObject *w;
	if (win == NULL)
		w = Py_None;
	else {
		int tag = wgettag(win);
		if (tag < 0 || tag >= MAXNWIN || windowlist[tag] == NULL ||
			windowlist[tag]->w_win != win)
			w = Py_None;
		else
			w = (PyObject *)windowlist[tag];
	}
	Py_INCREF(w);
	return w;
}

static PyObject *
stdwin_get_poll_event(poll, args)
	int poll;
	PyObject *args;
{
	EVENT e;
	PyObject *u, *v, *w;
	if (!PyArg_NoArgs(args))
		return NULL;
	if (Drawing != NULL) {
		PyErr_SetString(StdwinError,
				"cannot getevent() while drawing");
		return NULL;
	}
 again:
	BGN_STDWIN
	if (poll) {
		if (!wpollevent(&e)) {
			RET_STDWIN
			Py_INCREF(Py_None);
			return Py_None;
		}
	}
	else
		wgetevent(&e);
	END_STDWIN
	if (e.type == WE_COMMAND && e.u.command == WC_CANCEL) {
		/* Turn keyboard interrupts into exceptions */
		PyErr_SetNone(PyExc_KeyboardInterrupt);
		return NULL;
	}
	if (e.type == WE_COMMAND && e.u.command == WC_CLOSE) {
		/* Turn WC_CLOSE commands into WE_CLOSE events */
		e.type = WE_CLOSE;
	}
	v = window2object(e.window);
	switch (e.type) {
	case WE_CHAR:
		{
			char c[1];
			c[0] = e.u.character;
			w = PyString_FromStringAndSize(c, 1);
		}
		break;
	case WE_COMMAND:
		w = PyInt_FromLong((long)e.u.command);
		break;
	case WE_DRAW:
		w = makerect(e.u.area.left, e.u.area.top,
				e.u.area.right, e.u.area.bottom);
		break;
	case WE_MOUSE_DOWN:
	case WE_MOUSE_MOVE:
	case WE_MOUSE_UP:
		w = Py_BuildValue("((ii)iii)",
				  e.u.where.h, e.u.where.v,
				  e.u.where.clicks,
				  e.u.where.button,
				  e.u.where.mask);
		break;
	case WE_MENU:
		if (e.u.m.id >= IDOFFSET &&
		    e.u.m.id < IDOFFSET+MAXNMENU &&
		    menulist[e.u.m.id - IDOFFSET] != NULL)
		{
			w = Py_BuildValue("(Oi)",
					  menulist[e.u.m.id - IDOFFSET],
					  e.u.m.item);
		}
		else {
			/* Ghost menu event.
			   Can occur only on the Mac if another part
			   of the aplication has installed a menu;
			   like the THINK C console library. */
			Py_DECREF(v);
			goto again;
		}
		break;
	case WE_KEY:
		w = Py_BuildValue("(ii)", e.u.key.code, e.u.key.mask);
		break;
	case WE_LOST_SEL:
		w = PyInt_FromLong((long)e.u.sel);
		break;
	default:
		w = Py_None;
		Py_INCREF(w);
		break;
	}
	if (w == NULL) {
		Py_DECREF(v);
		return NULL;
	}
	u = Py_BuildValue("(iOO)", e.type, v, w);
	Py_XDECREF(v);
	Py_XDECREF(w);
	return u;
}

static PyObject *
stdwin_getevent(sw, args)
	PyObject *sw;
	PyObject *args;
{
	return stdwin_get_poll_event(0, args);
}

static PyObject *
stdwin_pollevent(sw, args)
	PyObject *sw;
	PyObject *args;
{
	return stdwin_get_poll_event(1, args);
}

static PyObject *
stdwin_setdefwinpos(sw, args)
	PyObject *sw;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetdefwinpos(a[0], a[1]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_setdefwinsize(sw, args)
	PyObject *sw;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetdefwinsize(a[0], a[1]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_setdefscrollbars(sw, args)
	PyObject *sw;
	PyObject *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetdefscrollbars(a[0], a[1]);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_getdefwinpos(self, args)
	PyObject *self;
	PyObject *args;
{
	int h, v;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetdefwinpos(&h, &v);
	return makepoint(h, v);
}

static PyObject *
stdwin_getdefwinsize(self, args)
	PyObject *self;
	PyObject *args;
{
	int width, height;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetdefwinsize(&width, &height);
	return makepoint(width, height);
}

static PyObject *
stdwin_getdefscrollbars(self, args)
	PyObject *self;
	PyObject *args;
{
	int h, v;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetdefscrollbars(&h, &v);
	return makepoint(h, v);
}

static PyObject *
stdwin_menucreate(self, args)
	PyObject *self;
	PyObject *args;
{
	char *title;
	if (!getstrarg(args, &title))
		return NULL;
	wmenusetdeflocal(0);
	return (PyObject *)newmenuobject(title);
}

static PyObject *
stdwin_askfile(self, args)
	PyObject *self;
	PyObject *args;
{
	char *prompt, *dflt;
	int new, ret;
	char buf[256];
	if (!PyArg_Parse(args, "(ssi)", &prompt, &dflt, &new))
		return NULL;
	strncpy(buf, dflt, sizeof buf);
	buf[sizeof buf - 1] = '\0';
	BGN_STDWIN
	ret = waskfile(prompt, buf, sizeof buf, new);
	END_STDWIN
	if (!ret) {
		PyErr_SetNone(PyExc_KeyboardInterrupt);
		return NULL;
	}
	return PyString_FromString(buf);
}

static PyObject *
stdwin_askync(self, args)
	PyObject *self;
	PyObject *args;
{
	char *prompt;
	int new, ret;
	if (!PyArg_Parse(args, "(si)", &prompt, &new))
		return NULL;
	BGN_STDWIN
	ret = waskync(prompt, new);
	END_STDWIN
	if (ret < 0) {
		PyErr_SetNone(PyExc_KeyboardInterrupt);
		return NULL;
	}
	return PyInt_FromLong((long)ret);
}

static PyObject *
stdwin_askstr(self, args)
	PyObject *self;
	PyObject *args;
{
	char *prompt, *dflt;
	int ret;
	char buf[256];
	if (!PyArg_Parse(args, "(ss)", &prompt, &dflt))
		return NULL;
	strncpy(buf, dflt, sizeof buf);
	buf[sizeof buf - 1] = '\0';
	BGN_STDWIN
	ret = waskstr(prompt, buf, sizeof buf);
	END_STDWIN
	if (!ret) {
		PyErr_SetNone(PyExc_KeyboardInterrupt);
		return NULL;
	}
	return PyString_FromString(buf);
}

static PyObject *
stdwin_message(self, args)
	PyObject *self;
	PyObject *args;
{
	char *msg;
	if (!getstrarg(args, &msg))
		return NULL;
	BGN_STDWIN
	wmessage(msg);
	END_STDWIN
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_fleep(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	wfleep();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_setcutbuffer(self, args)
	PyObject *self;
	PyObject *args;
{
	int i, size;
	char *str;
	if (!PyArg_Parse(args, "(it#)", &i, &str, &size))
		return NULL;
	wsetcutbuffer(i, str, size);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_getactive(self, args)
	PyObject *self;
	PyObject *args;
{
	return window2object(wgetactive());
}

static PyObject *
stdwin_getcutbuffer(self, args)
	PyObject *self;
	PyObject *args;
{
	int i;
	char *str;
	int len;
	if (!getintarg(args, &i))
		return NULL;
	str = wgetcutbuffer(i, &len);
	if (str == NULL) {
		str = "";
		len = 0;
	}
	return PyString_FromStringAndSize(str, len);
}

static PyObject *
stdwin_rotatecutbuffers(self, args)
	PyObject *self;
	PyObject *args;
{
	int i;
	if (!getintarg(args, &i))
		return NULL;
	wrotatecutbuffers(i);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_getselection(self, args)
	PyObject *self;
	PyObject *args;
{
	int sel;
	char *data;
	int len;
	if (!getintarg(args, &sel))
		return NULL;
	data = wgetselection(sel, &len);
	if (data == NULL) {
		data = "";
		len = 0;
	}
	return PyString_FromStringAndSize(data, len);
}

static PyObject *
stdwin_resetselection(self, args)
	PyObject *self;
	PyObject *args;
{
	int sel;
	if (!getintarg(args, &sel))
		return NULL;
	wresetselection(sel);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
stdwin_fetchcolor(self, args)
	PyObject *self;
	PyObject *args;
{
	char *colorname;
	COLOR color;
	if (!getstrarg(args, &colorname))
		return NULL;
	color = wfetchcolor(colorname);
#ifdef BADCOLOR
	if (color == BADCOLOR) {
		PyErr_SetString(StdwinError, "color name not found");
		return NULL;
	}
#endif
	return PyInt_FromLong((long)color);
}

static PyObject *
stdwin_getscrsize(self, args)
	PyObject *self;
	PyObject *args;
{
	int width, height;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetscrsize(&width, &height);
	return makepoint(width, height);
}

static PyObject *
stdwin_getscrmm(self, args)
	PyObject *self;
	PyObject *args;
{
	int width, height;
	if (!PyArg_NoArgs(args))
		return NULL;
	wgetscrmm(&width, &height);
	return makepoint(width, height);
}

#ifdef unix
static PyObject *
stdwin_connectionnumber(self, args)
	PyObject *self;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	return PyInt_FromLong((long) wconnectionnumber());
}
#endif

static PyObject *
stdwin_listfontnames(self, args)
	PyObject *self;
	PyObject *args;
{
	char *pattern;
	char **fontnames;
	int count;
	PyObject *list;
	if (!PyArg_Parse(args, "z", &pattern))
		return NULL;
	fontnames = wlistfontnames(pattern, &count);
	list = PyList_New(count);
	if (list != NULL) {
		int i;
		for (i = 0; i < count; i++) {
			PyObject *v = PyString_FromString(fontnames[i]);
			if (v == NULL) {
				Py_DECREF(list);
				list = NULL;
				break;
			}
			PyList_SetItem(list, i, v);
		}
	}
	return list;
}

#ifdef HAVE_BITMAPS
static PyObject *
stdwin_newbitmap(self, args)
	PyObject *self;
	PyObject *args;
{
	int width, height;
	bitmapobject *bp;
	if (!PyArg_Parse(args, "(ii)", &width, &height))
		return NULL;
	return (PyObject *)newbitmapobject(width, height);
}
#endif

static PyMethodDef stdwin_methods[] = {
	{"askfile",		stdwin_askfile},
	{"askstr",		stdwin_askstr},
	{"askync",		stdwin_askync},
	{"done",		stdwin_done},
	{"fetchcolor",		stdwin_fetchcolor},
#ifdef unix
	{"fileno",		stdwin_connectionnumber},
	{"connectionnumber",	stdwin_connectionnumber},
#endif
	{"fleep",		stdwin_fleep},
	{"getactive",		stdwin_getactive},
	{"getcutbuffer",	stdwin_getcutbuffer},
	{"getdefscrollbars",	stdwin_getdefscrollbars},
	{"getdefwinpos",	stdwin_getdefwinpos},
	{"getdefwinsize",	stdwin_getdefwinsize},
	{"getevent",		stdwin_getevent},
	{"getscrmm",		stdwin_getscrmm},
	{"getscrsize",		stdwin_getscrsize},
	{"getselection",	stdwin_getselection},
	{"listfontnames",	stdwin_listfontnames},
	{"menucreate",		stdwin_menucreate},
	{"message",		stdwin_message},
#ifdef HAVE_BITMAPS
	{"newbitmap",		stdwin_newbitmap},
#endif
	{"open",		stdwin_open},
	{"pollevent",		stdwin_pollevent},
	{"resetselection",	stdwin_resetselection},
	{"rotatecutbuffers",	stdwin_rotatecutbuffers},
	{"setcutbuffer",	stdwin_setcutbuffer},
	{"setdefscrollbars",	stdwin_setdefscrollbars},
	{"setdefwinpos",	stdwin_setdefwinpos},
	{"setdefwinsize",	stdwin_setdefwinsize},
	
	/* Text measuring methods borrow code from drawing objects: */
	{"baseline",		(PyCFunction)drawing_baseline},
	{"lineheight",		(PyCFunction)drawing_lineheight},
	{"textbreak",		(PyCFunction)drawing_textbreak},
	{"textwidth",		(PyCFunction)drawing_textwidth},

	/* Same for font setting methods: */
	{"setfont",		(PyCFunction)drawing_setfont},

	/* Same for color setting/getting methods: */
	{"getbgcolor",		(PyCFunction)drawing_getbgcolor},
	{"getfgcolor",		(PyCFunction)drawing_getfgcolor},
	{"setbgcolor",		(PyCFunction)drawing_setbgcolor},
	{"setfgcolor",		(PyCFunction)drawing_setfgcolor},

	{NULL,			NULL}		/* sentinel */
};

#ifndef macintosh
static int
checkstringlist(args, ps, pn)
	PyObject *args;
	char ***ps;
	int *pn;
{
	int i, n;
	char **s;
	if (!PyList_Check(args)) {
		PyErr_SetString(PyExc_TypeError, "list of strings expected");
		return 0;
	}
	n = PyList_Size(args);
	s = PyMem_NEW(char *, n+1);
	if (s == NULL) {
		PyErr_NoMemory();
		return 0;
	}
	for (i = 0; i < n; i++) {
		PyObject *item = PyList_GetItem(args, i);
		if (!PyString_Check(item)) {
			PyErr_SetString(PyExc_TypeError,
					"list of strings expected");
			return 0;
		}
		s[i] = PyString_AsString(item);
	}
	s[n] = NULL; /* In case caller wants a NULL-terminated list */
	*ps = s;
	*pn = n;
	return 1;
}

static int
putbackstringlist(list, s, n)
	PyObject *list;
	char **s;
	int n;
{
	int oldsize = PyList_Size(list);
	PyObject *newlist;
	int i;
	if (n == oldsize)
		return 1;
	newlist = PyList_New(n);
	for (i = 0; i < n && newlist != NULL; i++) {
		PyObject *item = PyString_FromString(s[i]);
		if (item == NULL) {
			Py_DECREF(newlist);
			newlist = NULL;
		}
		else
			PyList_SetItem(newlist, i, item);
	}
	if (newlist == NULL)
		return 0;
	(*list->ob_type->tp_as_sequence->sq_ass_slice)
		(list, 0, oldsize, newlist);
	Py_DECREF(newlist);
	return 1;
}
#endif /* macintosh */

DL_EXPORT(void)
initstdwin()
{
	PyObject *m, *d;
	static int inited = 0;

	if (!inited) {
#ifdef macintosh
		winit();
		PyMac_DoYieldEnabled = 0;
#else
		char buf[1000];
		int argc = 0;
		char **argv = NULL;
		PyObject *sys_argv = PySys_GetObject("argv");
		if (sys_argv != NULL) {
			if (!checkstringlist(sys_argv, &argv, &argc))
				PyErr_Clear();
		}
		if (argc > 0) {
			/* If argv[0] has a ".py" suffix, remove the suffix */
			char *p = strrchr(argv[0], '.');
			if (p != NULL && strcmp(p, ".py") == 0) {
				int n = p - argv[0];
				if (n >= sizeof(buf))
					n = sizeof(buf)-1;
				strncpy(buf, argv[0], n);
				buf[n] = '\0';
				argv[0] = buf;
			}
		}
		winitargs(&argc, &argv);
		if (argv != NULL) {
			if (!putbackstringlist(sys_argv, argv, argc))
				PyErr_Clear();
		}
#endif
		inited = 1;
	}
	m = Py_InitModule("stdwin", stdwin_methods);
	d = PyModule_GetDict(m);
	
	/* Initialize stdwin.error exception */
	StdwinError = PyErr_NewException("stdwin.error", NULL, NULL);
	if (StdwinError == NULL ||
	    PyDict_SetItemString(d, "error", StdwinError) != 0)
		return;
#ifdef WITH_THREAD
	StdwinLock = PyThread_allocate_lock();
#endif
}
