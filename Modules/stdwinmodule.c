/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

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

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

#ifdef macintosh
#include ":::src:stdwin:H:stdwin.h"
#else /* !macintosh */
#include "stdwin.h"
#endif /* !macintosh */

#ifdef USE_THREAD

#include "thread.h"

static type_lock StdwinLock; /* Lock held when interpreter not locked */

#define BGN_STDWIN BGN_SAVE acquire_lock(StdwinLock, 1);
#define RET_STDWIN release_lock(StdwinLock); RET_SAVE
#define END_STDWIN release_lock(StdwinLock); END_SAVE

#else

#define BGN_STDWIN BGN_SAVE
#define RET_STDWIN RET_SAVE
#define END_STDWIN END_SAVE

#endif

#define getpointarg(v, a) getargs(v, "(ii)", a, (a)+1)
#define get3pointarg(v, a) getargs(v, "((ii)(ii)(ii))", \
				a, a+1, a+2, a+3, a+4, a+5)
#define getrectarg(v, a) getargs(v, "((ii)(ii))", a, a+1, a+2, a+3)
#define getrectintarg(v, a) getargs(v, "(((ii)(ii))i)", a, a+1, a+2, a+3, a+4)
#define getpointintarg(v, a) getargs(v, "((ii)i)", a, a+1, a+2)
#define getrectpointarg(v, a) getargs(v, "(((ii)(ii))(ii))", \
				a, a+1, a+2, a+3, a+4, a+5)

static object *StdwinError; /* Exception stdwin.error */

/* Window and menu object types declared here because of forward references */

typedef struct {
	OB_HEAD
	object	*w_title;
	WINDOW	*w_win;
	object	*w_attr;	/* Attributes dictionary */
} windowobject;

extern typeobject Windowtype;	/* Really static, forward */

#define is_windowobject(wp) ((wp)->ob_type == &Windowtype)

typedef struct {
	OB_HEAD
	MENU	*m_menu;
	int	 m_id;
	object	*m_attr;	/* Attributes dictionary */
} menuobject;

extern typeobject Menutype;	/* Really static, forward */

#define is_menuobject(mp) ((mp)->ob_type == &Menutype)

typedef struct {
	OB_HEAD
	BITMAP	*b_bitmap;
	object	*b_attr;	/* Attributes dictionary */
} bitmapobject;

extern typeobject Bitmaptype;	/* Really static, forward */

#define is_bitmapobject(mp) ((mp)->ob_type == &Bitmaptype)


/* Strongly stdwin-specific argument handlers */

static int
getmenudetail(v, ep)
	object *v;
	EVENT *ep;
{
	menuobject *mp;
	if (!getargs(v, "(Oi)", &mp, &ep->u.m.item))
		return 0;
	if (!is_menuobject(mp))
		return err_badarg();
	ep->u.m.id = mp->m_id;
	return 1;
}

static int
geteventarg(v, ep)
	object *v;
	EVENT *ep;
{
	object *wp, *detail;
	int a[4];
	if (!getargs(v, "(iOO)", &ep->type, &wp, &detail))
		return 0;
	if (is_windowobject(wp))
		ep->window = ((windowobject *)wp) -> w_win;
	else if (wp == None)
		ep->window = NULL;
	else
		return err_badarg();
	switch (ep->type) {
	case WE_CHAR: {
			char c;
			if (!getargs(detail, "c", &c))
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
		return getargs(detail, "((ii)iii)",
				&ep->u.where.h, &ep->u.where.v,
				&ep->u.where.clicks,
				&ep->u.where.button,
				&ep->u.where.mask);
	case WE_MENU:
		return getmenudetail(detail, ep);
	case WE_KEY:
		return getargs(detail, "(ii)",
			       &ep->u.key.code, &ep->u.key.mask);
	default:
		return 1;
	}
}


/* Return construction tools */

static object *
makepoint(a, b)
	int a, b;
{
	return mkvalue("(ii)", a, b);
}

static object *
makerect(a, b, c, d)
	int a, b, c, d;
{
	return mkvalue("((ii)(ii))", a, b, c, d);
}


/* Drawing objects */

typedef struct {
	OB_HEAD
	windowobject	*d_ref;
} drawingobject;

static drawingobject *Drawing; /* Set to current drawing object, or NULL */

/* Drawing methods */

static object *
drawing_close(dp)
	drawingobject *dp;
{
	if (dp->d_ref != NULL) {
		wenddrawing(dp->d_ref->w_win);
		Drawing = NULL;
		DECREF(dp->d_ref);
		dp->d_ref = NULL;
	}
	INCREF(None);
	return None;
}

static void
drawing_dealloc(dp)
	drawingobject *dp;
{
	if (dp->d_ref != NULL) {
		wenddrawing(dp->d_ref->w_win);
		Drawing = NULL;
		DECREF(dp->d_ref);
		dp->d_ref = NULL;
	}
	free((char *)dp);
}

static object *
drawing_generic(dp, args, func)
	drawingobject *dp;
	object *args;
	void (*func) FPROTO((int, int, int, int));
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	(*func)(a[0], a[1], a[2], a[3]);
	INCREF(None);
	return None;
}

static object *
drawing_line(dp, args)
	drawingobject *dp;
	object *args;
{
	return drawing_generic(dp, args, wdrawline);
}

static object *
drawing_xorline(dp, args)
	drawingobject *dp;
	object *args;
{
	return drawing_generic(dp, args, wxorline);
}

static object *
drawing_circle(dp, args)
	drawingobject *dp;
	object *args;
{
	int a[3];
	if (!getpointintarg(args, a))
		return NULL;
	wdrawcircle(a[0], a[1], a[2]);
	INCREF(None);
	return None;
}

static object *
drawing_fillcircle(dp, args)
	drawingobject *dp;
	object *args;
{
	int a[3];
	if (!getpointintarg(args, a))
		return NULL;
	wfillcircle(a[0], a[1], a[2]);
	INCREF(None);
	return None;
}

static object *
drawing_xorcircle(dp, args)
	drawingobject *dp;
	object *args;
{
	int a[3];
	if (!getpointintarg(args, a))
		return NULL;
	wxorcircle(a[0], a[1], a[2]);
	INCREF(None);
	return None;
}

static object *
drawing_elarc(dp, args)
	drawingobject *dp;
	object *args;
{
	int a[6];
	if (!get3pointarg(args, a))
		return NULL;
	wdrawelarc(a[0], a[1], a[2], a[3], a[4], a[5]);
	INCREF(None);
	return None;
}

static object *
drawing_fillelarc(dp, args)
	drawingobject *dp;
	object *args;
{
	int a[6];
	if (!get3pointarg(args, a))
		return NULL;
	wfillelarc(a[0], a[1], a[2], a[3], a[4], a[5]);
	INCREF(None);
	return None;
}

static object *
drawing_xorelarc(dp, args)
	drawingobject *dp;
	object *args;
{
	int a[6];
	if (!get3pointarg(args, a))
		return NULL;
	wxorelarc(a[0], a[1], a[2], a[3], a[4], a[5]);
	INCREF(None);
	return None;
}

static object *
drawing_box(dp, args)
	drawingobject *dp;
	object *args;
{
	return drawing_generic(dp, args, wdrawbox);
}

static object *
drawing_erase(dp, args)
	drawingobject *dp;
	object *args;
{
	return drawing_generic(dp, args, werase);
}

static object *
drawing_paint(dp, args)
	drawingobject *dp;
	object *args;
{
	return drawing_generic(dp, args, wpaint);
}

static object *
drawing_invert(dp, args)
	drawingobject *dp;
	object *args;
{
	return drawing_generic(dp, args, winvert);
}

static POINT *
getpointsarray(v, psize)
	object *v;
	int *psize;
{
	int n = -1;
	object * (*getitem) PROTO((object *, int));
	int i;
	POINT *points;

	if (v == NULL)
		;
	else if (is_listobject(v)) {
		n = getlistsize(v);
		getitem = getlistitem;
	}
	else if (is_tupleobject(v)) {
		n = gettuplesize(v);
		getitem = gettupleitem;
	}

	if (n <= 0) {
		(void) err_badarg();
		return NULL;
	}

	points = NEW(POINT, n);
	if (points == NULL) {
		(void) err_nomem();
		return NULL;
	}

	for (i = 0; i < n; i++) {
		object *w = (*getitem)(v, i);
		int a[2];
		if (!getpointarg(w, a)) {
			DEL(points);
			return NULL;
		}
		points[i].h = a[0];
		points[i].v = a[1];
	}

	*psize = n;
	return points;
}

static object *
drawing_poly(dp, args)
	drawingobject *dp;
	object *args;
{
	int n;
	POINT *points = getpointsarray(args, &n);
	if (points == NULL)
		return NULL;
	wdrawpoly(n, points);
	DEL(points);
	INCREF(None);
	return None;
}

static object *
drawing_fillpoly(dp, args)
	drawingobject *dp;
	object *args;
{
	int n;
	POINT *points = getpointsarray(args, &n);
	if (points == NULL)
		return NULL;
	wfillpoly(n, points);
	DEL(points);
	INCREF(None);
	return None;
}

static object *
drawing_xorpoly(dp, args)
	drawingobject *dp;
	object *args;
{
	int n;
	POINT *points = getpointsarray(args, &n);
	if (points == NULL)
		return NULL;
	wxorpoly(n, points);
	DEL(points);
	INCREF(None);
	return None;
}

static object *
drawing_cliprect(dp, args)
	drawingobject *dp;
	object *args;
{
	return drawing_generic(dp, args, wcliprect);
}

static object *
drawing_noclip(dp, args)
	drawingobject *dp;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	wnoclip();
	INCREF(None);
	return None;
}

static object *
drawing_shade(dp, args)
	drawingobject *dp;
	object *args;
{
	int a[5];
	if (!getrectintarg(args, a))
		return NULL;
	wshade(a[0], a[1], a[2], a[3], a[4]);
	INCREF(None);
	return None;
}

static object *
drawing_text(dp, args)
	drawingobject *dp;
	object *args;
{
	int h, v, size;
	char *text;
	if (!getargs(args, "((ii)s#)", &h, &v, &text, &size))
		return NULL;
	wdrawtext(h, v, text, size);
	INCREF(None);
	return None;
}

/* The following four are also used as stdwin functions */

static object *
drawing_lineheight(dp, args)
	drawingobject *dp;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)wlineheight());
}

static object *
drawing_baseline(dp, args)
	drawingobject *dp;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)wbaseline());
}

static object *
drawing_textwidth(dp, args)
	drawingobject *dp;
	object *args;
{
	char *text;
	int size;
	if (!getargs(args, "s#", &text, &size))
		return NULL;
	return newintobject((long)wtextwidth(text, size));
}

static object *
drawing_textbreak(dp, args)
	drawingobject *dp;
	object *args;
{
	char *text;
	int size, width;
	if (!getargs(args, "(s#i)", &text, &size, &width))
		return NULL;
	return newintobject((long)wtextbreak(text, size, width));
}

static object *
drawing_setfont(self, args)
	drawingobject *self;
	object *args;
{
	char *font;
	char style = '\0';
	int size = 0;
	if (args == NULL || !is_tupleobject(args)) {
		if (!getargs(args, "z", &font))
			return NULL;
	}
	else {
		int n = gettuplesize(args);
		if (n == 2) {
			if (!getargs(args, "(zi)", &font, &size))
				return NULL;
		}
		else if (!getargs(args, "(zic)", &font, &size, &style)) {
			err_clear();
			if (!getargs(args, "(zci)", &font, &style, &size))
				return NULL;
		}
	}
	if (font != NULL) {
		if (!wsetfont(font)) {
			err_setstr(StdwinError, "font not found");
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
	INCREF(None);
	return None;
}

static object *
drawing_getbgcolor(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)wgetbgcolor());
}

static object *
drawing_getfgcolor(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long)wgetfgcolor());
}

static object *
drawing_setbgcolor(self, args)
	object *self;
	object *args;
{
	long color;
	if (!getlongarg(args, &color))
		return NULL;
	wsetbgcolor((COLOR)color);
	INCREF(None);
	return None;
}

static object *
drawing_setfgcolor(self, args)
	object *self;
	object *args;
{
	long color;
	if (!getlongarg(args, &color))
		return NULL;
	wsetfgcolor((COLOR)color);
	INCREF(None);
	return None;
}

static object *
drawing_bitmap(self, args)
	object *self;
	object *args;
{
	int h, v;
	object *bp;
	object *mask = NULL;
	if (!getargs(args, "((ii)O)", &h, &v, &bp)) {
		err_clear();
		if (!getargs(args, "((ii)OO)", &h, &v, &bp, &mask))
			return NULL;
		if (mask == None)
			mask = NULL;
		else if (!is_bitmapobject(mask)) {
			err_badarg();
			return NULL;
		}
	}
	if (!is_bitmapobject(bp)) {
		err_badarg();
		return NULL;
	}
	if (((bitmapobject *)bp)->b_bitmap == NULL ||
	    mask != NULL && ((bitmapobject *)mask)->b_bitmap == NULL) {
		err_setstr(StdwinError, "bitmap object already close");
		return NULL;
	}
	if (mask == NULL)
		wdrawbitmap(h, v, ((bitmapobject *)bp)->b_bitmap, ALLBITS);
	else
		wdrawbitmap(h, v,
			    ((bitmapobject *)bp)->b_bitmap,
			    ((bitmapobject *)bp)->b_bitmap);
	INCREF(None);
	return None;
}

static struct methodlist drawing_methods[] = {
	{"bitmap",	drawing_bitmap},
	{"box",		drawing_box},
	{"circle",	drawing_circle},
	{"cliprect",	drawing_cliprect},
	{"close",	drawing_close},
	{"elarc",	drawing_elarc},
	{"enddrawing",	drawing_close},
	{"erase",	drawing_erase},
	{"fillcircle",	drawing_fillcircle},
	{"fillelarc",	drawing_fillelarc},
	{"fillpoly",	drawing_fillpoly},
	{"invert",	drawing_invert},
	{"line",	drawing_line},
	{"noclip",	drawing_noclip},
	{"paint",	drawing_paint},
	{"poly",	drawing_poly},
	{"shade",	drawing_shade},
	{"text",	drawing_text},
	{"xorcircle",	drawing_xorcircle},
	{"xorelarc",	drawing_xorelarc},
	{"xorline",	drawing_xorline},
	{"xorpoly",	drawing_xorpoly},
	
	/* Text measuring methods: */
	{"baseline",	drawing_baseline},
	{"lineheight",	drawing_lineheight},
	{"textbreak",	drawing_textbreak},
	{"textwidth",	drawing_textwidth},

	/* Font setting methods: */
	{"setfont",	drawing_setfont},
	
	/* Color methods: */
	{"getbgcolor",	drawing_getbgcolor},
	{"getfgcolor",	drawing_getfgcolor},
	{"setbgcolor",	drawing_setbgcolor},
	{"setfgcolor",	drawing_setfgcolor},

	{NULL,		NULL}		/* sentinel */
};

static object *
drawing_getattr(dp, name)
	drawingobject *dp;
	char *name;
{
	if (dp->d_ref == NULL) {
		err_setstr(StdwinError, "drawing object already closed");
		return NULL;
	}
	return findmethod(drawing_methods, (object *)dp, name);
}

typeobject Drawingtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"drawing",		/*tp_name*/
	sizeof(drawingobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	drawing_dealloc,	/*tp_dealloc*/
	0,			/*tp_print*/
	drawing_getattr,	/*tp_getattr*/
	0,			/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};


/* Text(edit) objects */

typedef struct {
	OB_HEAD
	TEXTEDIT	*t_text;
	windowobject	*t_ref;
	object		*t_attr;	/* Attributes dictionary */
} textobject;

extern typeobject Texttype;	/* Really static, forward */

static textobject *
newtextobject(wp, left, top, right, bottom)
	windowobject *wp;
	int left, top, right, bottom;
{
	textobject *tp;
	tp = NEWOBJ(textobject, &Texttype);
	if (tp == NULL)
		return NULL;
	tp->t_attr = NULL;
	INCREF(wp);
	tp->t_ref = wp;
	tp->t_text = tecreate(wp->w_win, left, top, right, bottom);
	if (tp->t_text == NULL) {
		DECREF(tp);
		return (textobject *) err_nomem();
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
	XDECREF(tp->t_attr);
	XDECREF(tp->t_ref);
	DEL(tp);
}

static object *
text_close(tp, args)
	textobject *tp;
	object *args;
{
	if (tp->t_text != NULL) {
		tefree(tp->t_text);
		tp->t_text = NULL;
	}
	if (tp->t_attr != NULL) {
		DECREF(tp->t_attr);
		tp->t_attr = NULL;
	}
	if (tp->t_ref != NULL) {
		DECREF(tp->t_ref);
		tp->t_ref = NULL;
	}
	INCREF(None);
	return None;
}

static object *
text_arrow(self, args)
	textobject *self;
	object *args;
{
	int code;
	if (!getintarg(args, &code))
		return NULL;
	tearrow(self->t_text, code);
	INCREF(None);
	return None;
}

static object *
text_draw(self, args)
	textobject *self;
	object *args;
{
	register TEXTEDIT *tp = self->t_text;
	int a[4];
	int left, top, right, bottom;
	if (!getrectarg(args, a))
		return NULL;
	if (Drawing != NULL) {
		err_setstr(StdwinError, "already drawing");
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
	INCREF(None);
	return None;
}

static object *
text_event(self, args)
	textobject *self;
	object *args;
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
	return newintobject((long) teevent(tp, &e));
}

static object *
text_getfocus(self, args)
	textobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return makepoint(tegetfoc1(self->t_text), tegetfoc2(self->t_text));
}

static object *
text_getfocustext(self, args)
	textobject *self;
	object *args;
{
	int f1, f2;
	char *text;
	if (!getnoarg(args))
		return NULL;
	f1 = tegetfoc1(self->t_text);
	f2 = tegetfoc2(self->t_text);
	text = tegettext(self->t_text);
	return newsizedstringobject(text + f1, f2-f1);
}

static object *
text_getrect(self, args)
	textobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return makerect(tegetleft(self->t_text),
			tegettop(self->t_text),
			tegetright(self->t_text),
			tegetbottom(self->t_text));
}

static object *
text_gettext(self, args)
	textobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newsizedstringobject(tegettext(self->t_text),
					tegetlen(self->t_text));
}

static object *
text_move(self, args)
	textobject *self;
	object *args;
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	temovenew(self->t_text, a[0], a[1], a[2], a[3]);
	INCREF(None);
	return None;
}

static object *
text_replace(self, args)
	textobject *self;
	object *args;
{
	char *text;
	if (!getstrarg(args, &text))
		return NULL;
	tereplace(self->t_text, text);
	INCREF(None);
	return None;
}

static object *
text_setactive(self, args)
	textobject *self;
	object *args;
{
	int flag;
	if (!getintarg(args, &flag))
		return NULL;
	tesetactive(self->t_text, flag);
	INCREF(None);
	return None;
}

static object *
text_setfocus(self, args)
	textobject *self;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	tesetfocus(self->t_text, a[0], a[1]);
	INCREF(None);
	return None;
}

static object *
text_settext(self, args)
	textobject *self;
	object *args;
{
	char *text;
	char *buf;
	int size;
	if (!getargs(args, "s#", &text, &size))
		return NULL;
	if ((buf = NEW(char, size)) == NULL) {
		return err_nomem();
	}
	memcpy(buf, text, size);
	tesetbuf(self->t_text, buf, size); /* Becomes owner of buffer */
	INCREF(None);
	return None;
}

static object *
text_setview(self, args)
	textobject *self;
	object *args;
{
	int a[4];
	if (args == None)
		tenoview(self->t_text);
	else {
		if (!getrectarg(args, a))
			return NULL;
		tesetview(self->t_text, a[0], a[1], a[2], a[3]);
	}
	INCREF(None);
	return None;
}

static struct methodlist text_methods[] = {
	{"arrow",	text_arrow},
	{"close",	text_close},
	{"draw",	text_draw},
	{"event",	text_event},
	{"getfocus",	text_getfocus},
	{"getfocustext",text_getfocustext},
	{"getrect",	text_getrect},
	{"gettext",	text_gettext},
	{"move",	text_move},
	{"replace",	text_replace},
	{"setactive",	text_setactive},
	{"setfocus",	text_setfocus},
	{"settext",	text_settext},
	{"setview",	text_setview},
	{NULL,		NULL}		/* sentinel */
};

static object *
text_getattr(tp, name)
	textobject *tp;
	char *name;
{
	object *v = NULL;
	if (tp->t_ref == NULL) {
		err_setstr(StdwinError, "text object already closed");
		return NULL;
	}
	if (strcmp(name, "__dict__") == 0) {
		v = tp->t_attr;
		if (v == NULL)
			v = None;
	}
	else if (tp->t_attr != NULL) {
		v = dictlookup(tp->t_attr, name);
	}
	if (v != NULL) {
		INCREF(v);
		return v;
	}
	return findmethod(text_methods, (object *)tp, name);
}

static int
text_setattr(tp, name, v)
	textobject *tp;
	char *name;
	object *v;
{
	if (tp->t_attr == NULL) {
		tp->t_attr = newdictobject();
		if (tp->t_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = dictremove(tp->t_attr, name);
		if (rv < 0)
			err_setstr(AttributeError,
			        "delete non-existing text object attribute");
		return rv;
	}
	else
		return dictinsert(tp->t_attr, name, v);
}

typeobject Texttype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"textedit",		/*tp_name*/
	sizeof(textobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	text_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	text_getattr,		/*tp_getattr*/
	text_setattr,		/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};


/* Menu objects */

#define IDOFFSET 10		/* Menu IDs we use start here */
#define MAXNMENU 200		/* Max #menus we allow */
static menuobject *menulist[MAXNMENU];

static menuobject *newmenuobject PROTO((char *));
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
		err_setstr(StdwinError, "creating too many menus");
		return NULL;
	}
	menu = wmenucreate(id + IDOFFSET, title);
	if (menu == NULL)
		return (menuobject *) err_nomem();
	mp = NEWOBJ(menuobject, &Menutype);
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
	XDECREF(mp->m_attr);
	DEL(mp);
}

static object *
menu_close(mp, args)
	menuobject *mp;
	object *args;
{
	int id = mp->m_id - IDOFFSET;
	if (id >= 0 && id < MAXNMENU && menulist[id] == mp) {
		menulist[id] = NULL;
	}
	mp->m_id = -1;
	if (mp->m_menu != NULL)
		wmenudelete(mp->m_menu);
	mp->m_menu = NULL;
	XDECREF(mp->m_attr);
	mp->m_attr = NULL;
	INCREF(None);
	return None;
}

static object *
menu_additem(self, args)
	menuobject *self;
	object *args;
{
	char *text;
	int shortcut = -1;
	if (is_tupleobject(args)) {
		char c;
		if (!getargs(args, "(sc)", &text, &c))
			return NULL;
		shortcut = c;
	}
	else if (!getstrarg(args, &text))
		return NULL;
	wmenuadditem(self->m_menu, text, shortcut);
	INCREF(None);
	return None;
}

static object *
menu_setitem(self, args)
	menuobject *self;
	object *args;
{
	int index;
	char *text;
	if (!getargs(args, "(is)", &index, &text))
		return NULL;
	wmenusetitem(self->m_menu, index, text);
	INCREF(None);
	return None;
}

static object *
menu_enable(self, args)
	menuobject *self;
	object *args;
{
	int index;
	int flag;
	if (!getargs(args, "(ii)", &index, &flag))
		return NULL;
	wmenuenable(self->m_menu, index, flag);
	INCREF(None);
	return None;
}

static object *
menu_check(self, args)
	menuobject *self;
	object *args;
{
	int index;
	int flag;
	if (!getargs(args, "(ii)", &index, &flag))
		return NULL;
	wmenucheck(self->m_menu, index, flag);
	INCREF(None);
	return None;
}

static struct methodlist menu_methods[] = {
	{"additem",	menu_additem},
	{"setitem",	menu_setitem},
	{"enable",	menu_enable},
	{"check",	menu_check},
	{"close",	menu_close},
	{NULL,		NULL}		/* sentinel */
};

static object *
menu_getattr(mp, name)
	menuobject *mp;
	char *name;
{
	object *v = NULL;
	if (mp->m_menu == NULL) {
		err_setstr(StdwinError, "menu object already closed");
		return NULL;
	}
	if (strcmp(name, "__dict__") == 0) {
		v = mp->m_attr;
		if (v == NULL)
			v = None;
	}
	else if (mp->m_attr != NULL) {
		v = dictlookup(mp->m_attr, name);
	}
	if (v != NULL) {
		INCREF(v);
		return v;
	}
	return findmethod(menu_methods, (object *)mp, name);
}

static int
menu_setattr(mp, name, v)
	menuobject *mp;
	char *name;
	object *v;
{
	if (mp->m_attr == NULL) {
		mp->m_attr = newdictobject();
		if (mp->m_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = dictremove(mp->m_attr, name);
		if (rv < 0)
			err_setstr(AttributeError,
			        "delete non-existing menu object attribute");
		return rv;
	}
	else
		return dictinsert(mp->m_attr, name, v);
}

typeobject Menutype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"menu",			/*tp_name*/
	sizeof(menuobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	menu_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	menu_getattr,		/*tp_getattr*/
	menu_setattr,		/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};


/* Bitmaps objects */

static bitmapobject *newbitmapobject PROTO((int, int));
static bitmapobject *
newbitmapobject(width, height)
	int width, height;
{
	BITMAP *bitmap;
	bitmapobject *bp;
	bitmap = wnewbitmap(width, height);
	if (bitmap == NULL)
		return (bitmapobject *) err_nomem();
	bp = NEWOBJ(bitmapobject, &Bitmaptype);
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
	XDECREF(bp->b_attr);
	DEL(bp);
}

static object *
bitmap_close(bp, args)
	bitmapobject *bp;
	object *args;
{
	if (bp->b_bitmap != NULL)
		wfreebitmap(bp->b_bitmap);
	bp->b_bitmap = NULL;
	XDECREF(bp->b_attr);
	bp->b_attr = NULL;
	INCREF(None);
	return None;
}

static object *
bitmap_setbit(self, args)
	bitmapobject *self;
	object *args;
{
	int a[3];
	if (!getpointintarg(args, a))
		return NULL;
	wsetbit(self->b_bitmap, a[0], a[1], a[2]);
	INCREF(None);
	return None;
}

static object *
bitmap_getbit(self, args)
	bitmapobject *self;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	return newintobject((long) wgetbit(self->b_bitmap, a[0], a[1]));
}

static object *
bitmap_getsize(self, args)
	bitmapobject *self;
	object *args;
{
	int width, height;
	if (!getnoarg(args))
		return NULL;
	wgetbitmapsize(self->b_bitmap, &width, &height);
	return mkvalue("(ii)", width, height);
}

static struct methodlist bitmap_methods[] = {
	{"close",	bitmap_close},
	{"getsize",	bitmap_getsize},
	{"getbit",	bitmap_getbit},
	{"setbit",	bitmap_setbit},
	{NULL,		NULL}		/* sentinel */
};

static object *
bitmap_getattr(bp, name)
	bitmapobject *bp;
	char *name;
{
	object *v = NULL;
	if (bp->b_bitmap == NULL) {
		err_setstr(StdwinError, "bitmap object already closed");
		return NULL;
	}
	if (strcmp(name, "__dict__") == 0) {
		v = bp->b_attr;
		if (v == NULL)
			v = None;
	}
	else if (bp->b_attr != NULL) {
		v = dictlookup(bp->b_attr, name);
	}
	if (v != NULL) {
		INCREF(v);
		return v;
	}
	return findmethod(bitmap_methods, (object *)bp, name);
}

static int
bitmap_setattr(bp, name, v)
	bitmapobject *bp;
	char *name;
	object *v;
{
	if (bp->b_attr == NULL) {
		bp->b_attr = newdictobject();
		if (bp->b_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = dictremove(bp->b_attr, name);
		if (rv < 0)
			err_setstr(AttributeError,
			        "delete non-existing bitmap object attribute");
		return rv;
	}
	else
		return dictinsert(bp->b_attr, name, v);
}

typeobject Bitmaptype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"bitmap",			/*tp_name*/
	sizeof(bitmapobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	bitmap_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	bitmap_getattr,		/*tp_getattr*/
	bitmap_setattr,		/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};


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
	DECREF(wp->w_title);
	if (wp->w_attr != NULL)
		DECREF(wp->w_attr);
	free((char *)wp);
}

static object *
window_close(wp, args)
	windowobject *wp;
	object *args;
{
	if (wp->w_win != NULL) {
		int tag = wgettag(wp->w_win);
		if (tag >= 0 && tag < MAXNWIN)
			windowlist[tag] = NULL;
		wclose(wp->w_win);
		wp->w_win = NULL;
	}
	INCREF(None);
	return None;
}

static object *
window_begindrawing(wp, args)
	windowobject *wp;
	object *args;
{
	drawingobject *dp;
	if (!getnoarg(args))
		return NULL;
	if (Drawing != NULL) {
		err_setstr(StdwinError, "already drawing");
		return NULL;
	}
	dp = NEWOBJ(drawingobject, &Drawingtype);
	if (dp == NULL)
		return NULL;
	Drawing = dp;
	INCREF(wp);
	dp->d_ref = wp;
	wbegindrawing(wp->w_win);
	return (object *)dp;
}

static object *
window_change(wp, args)
	windowobject *wp;
	object *args;
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	wchange(wp->w_win, a[0], a[1], a[2], a[3]);
	INCREF(None);
	return None;
}

static object *
window_gettitle(wp, args)
	windowobject *wp;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	INCREF(wp->w_title);
	return wp->w_title;
}

static object *
window_getwinpos(wp, args)
	windowobject *wp;
	object *args;
{
	int h, v;
	if (!getnoarg(args))
		return NULL;
	wgetwinpos(wp->w_win, &h, &v);
	return makepoint(h, v);
}

static object *
window_getwinsize(wp, args)
	windowobject *wp;
	object *args;
{
	int width, height;
	if (!getnoarg(args))
		return NULL;
	wgetwinsize(wp->w_win, &width, &height);
	return makepoint(width, height);
}

static object *
window_setwinpos(wp, args)
	windowobject *wp;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetwinpos(wp->w_win, a[0], a[1]);
	INCREF(None);
	return None;
}

static object *
window_setwinsize(wp, args)
	windowobject *wp;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetwinsize(wp->w_win, a[0], a[1]);
	INCREF(None);
	return None;
}

static object *
window_getdocsize(wp, args)
	windowobject *wp;
	object *args;
{
	int width, height;
	if (!getnoarg(args))
		return NULL;
	wgetdocsize(wp->w_win, &width, &height);
	return makepoint(width, height);
}

static object *
window_getorigin(wp, args)
	windowobject *wp;
	object *args;
{
	int width, height;
	if (!getnoarg(args))
		return NULL;
	wgetorigin(wp->w_win, &width, &height);
	return makepoint(width, height);
}

static object *
window_scroll(wp, args)
	windowobject *wp;
	object *args;
{
	int a[6];
	if (!getrectpointarg(args, a))
		return NULL;
	wscroll(wp->w_win, a[0], a[1], a[2], a[3], a[4], a[5]);
	INCREF(None);
	return None;
}

static object *
window_setdocsize(wp, args)
	windowobject *wp;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetdocsize(wp->w_win, a[0], a[1]);
	INCREF(None);
	return None;
}

static object *
window_setorigin(wp, args)
	windowobject *wp;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetorigin(wp->w_win, a[0], a[1]);
	INCREF(None);
	return None;
}

static object *
window_settitle(wp, args)
	windowobject *wp;
	object *args;
{
	object *title;
	if (!getargs(args, "S", &title))
		return NULL;
	DECREF(wp->w_title);
	INCREF(title);
	wp->w_title = title;
	wsettitle(wp->w_win, getstringvalue(title));
	INCREF(None);
	return None;
}

static object *
window_show(wp, args)
	windowobject *wp;
	object *args;
{
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	wshow(wp->w_win, a[0], a[1], a[2], a[3]);
	INCREF(None);
	return None;
}

static object *
window_settimer(wp, args)
	windowobject *wp;
	object *args;
{
	int a;
	if (!getintarg(args, &a))
		return NULL;
	wsettimer(wp->w_win, a);
	INCREF(None);
	return None;
}

static object *
window_menucreate(self, args)
	windowobject *self;
	object *args;
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
	return (object *)mp;
}

static object *
window_textcreate(self, args)
	windowobject *self;
	object *args;
{
	textobject *tp;
	int a[4];
	if (!getrectarg(args, a))
		return NULL;
	return (object *)
		newtextobject(self, a[0], a[1], a[2], a[3]);
}

static object *
window_setselection(self, args)
	windowobject *self;
	object *args;
{
	int sel, size, ok;
	char *text;
	if (!getargs(args, "(is#)", &sel, &text, &size))
		return NULL;
	ok = wsetselection(self->w_win, sel, text, size);
	return newintobject(ok);
}

static object *
window_setwincursor(self, args)
	windowobject *self;
	object *args;
{
	char *name;
	CURSOR *c;
	if (!getargs(args, "z", &name))
		return NULL;
	if (name == NULL)
		c = NULL;
	else {
		c = wfetchcursor(name);
		if (c == NULL) {
			err_setstr(StdwinError, "no such cursor");
			return NULL;
		}
	}
	wsetwincursor(self->w_win, c);
	INCREF(None);
	return None;
}

static object *
window_setactive(self, args)
	windowobject *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	wsetactive(self->w_win);
	INCREF(None);
	return None;
}

#ifdef CWI_HACKS
static object *
window_getxwindowid(self, args)
	windowobject *self;
	object *args;
{
	long wid = wgetxwindowid(self->w_win);
	return newintobject(wid);
}
#endif

static struct methodlist window_methods[] = {
	{"begindrawing",window_begindrawing},
	{"change",	window_change},
	{"close",	window_close},
	{"getdocsize",	window_getdocsize},
	{"getorigin",	window_getorigin},
	{"gettitle",	window_gettitle},
	{"getwinpos",	window_getwinpos},
	{"getwinsize",	window_getwinsize},
	{"menucreate",	window_menucreate},
	{"scroll",	window_scroll},
	{"setactive",	window_setactive},
	{"setdocsize",	window_setdocsize},
	{"setorigin",	window_setorigin},
	{"setselection",window_setselection},
	{"settimer",	window_settimer},
	{"settitle",	window_settitle},
	{"setwincursor",window_setwincursor},
	{"setwinpos",	window_setwinpos},
	{"setwinsize",	window_setwinsize},
	{"show",	window_show},
	{"textcreate",	window_textcreate},
#ifdef CWI_HACKS
	{"getxwindowid",window_getxwindowid},
#endif
	{NULL,		NULL}		/* sentinel */
};

static object *
window_getattr(wp, name)
	windowobject *wp;
	char *name;
{
	object *v = NULL;
	if (wp->w_win == NULL) {
		err_setstr(StdwinError, "window already closed");
		return NULL;
	}
	if (strcmp(name, "__dict__") == 0) {
		v = wp->w_attr;
		if (v == NULL)
			v = None;
	}
	else if (wp->w_attr != NULL) {
		v = dictlookup(wp->w_attr, name);
	}
	if (v != NULL) {
		INCREF(v);
		return v;
	}
	return findmethod(window_methods, (object *)wp, name);
}

static int
window_setattr(wp, name, v)
	windowobject *wp;
	char *name;
	object *v;
{
	if (wp->w_attr == NULL) {
		wp->w_attr = newdictobject();
		if (wp->w_attr == NULL)
			return -1;
	}
	if (v == NULL) {
		int rv = dictremove(wp->w_attr, name);
		if (rv < 0)
			err_setstr(AttributeError,
			        "delete non-existing menu object attribute");
		return rv;
	}
	else
		return dictinsert(wp->w_attr, name, v);
}

typeobject Windowtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"window",		/*tp_name*/
	sizeof(windowobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	window_dealloc,		/*tp_dealloc*/
	0,			/*tp_print*/
	window_getattr,		/*tp_getattr*/
	window_setattr,		/*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
};

/* Stdwin methods */

static object *
stdwin_open(sw, args)
	object *sw;
	object *args;
{
	int tag;
	object *title;
	windowobject *wp;
	if (!getargs(args, "S", &title))
		return NULL;
	for (tag = 0; tag < MAXNWIN; tag++) {
		if (windowlist[tag] == NULL)
			break;
	}
	if (tag >= MAXNWIN) {
		err_setstr(StdwinError, "creating too many windows");
		return NULL;
	}
	wp = NEWOBJ(windowobject, &Windowtype);
	if (wp == NULL)
		return NULL;
	INCREF(title);
	wp->w_title = title;
	wp->w_win = wopen(getstringvalue(title), (void (*)()) NULL);
	wp->w_attr = NULL;
	if (wp->w_win == NULL) {
		DECREF(wp);
		return NULL;
	}
	windowlist[tag] = wp;
	wsettag(wp->w_win, tag);
	return (object *)wp;
}

static object *
window2object(win)
	WINDOW *win;
{
	object *w;
	if (win == NULL)
		w = None;
	else {
		int tag = wgettag(win);
		if (tag < 0 || tag >= MAXNWIN || windowlist[tag] == NULL ||
			windowlist[tag]->w_win != win)
			w = None;
		else
			w = (object *)windowlist[tag];
	}
	INCREF(w);
	return w;
}

static object *
stdwin_get_poll_event(poll, args)
	int poll;
	object *args;
{
	EVENT e;
	object *u, *v, *w;
	if (!getnoarg(args))
		return NULL;
	if (Drawing != NULL) {
		err_setstr(StdwinError, "cannot getevent() while drawing");
		return NULL;
	}
 again:
	BGN_STDWIN
	if (poll) {
		if (!wpollevent(&e)) {
			RET_STDWIN
			INCREF(None);
			return None;
		}
	}
	else
		wgetevent(&e);
	END_STDWIN
	if (e.type == WE_COMMAND && e.u.command == WC_CANCEL) {
		/* Turn keyboard interrupts into exceptions */
		err_set(KeyboardInterrupt);
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
			w = newsizedstringobject(c, 1);
		}
		break;
	case WE_COMMAND:
		w = newintobject((long)e.u.command);
		break;
	case WE_DRAW:
		w = makerect(e.u.area.left, e.u.area.top,
				e.u.area.right, e.u.area.bottom);
		break;
	case WE_MOUSE_DOWN:
	case WE_MOUSE_MOVE:
	case WE_MOUSE_UP:
		w = mkvalue("((ii)iii)",
				e.u.where.h, e.u.where.v,
				e.u.where.clicks,
				e.u.where.button,
				e.u.where.mask);
		break;
	case WE_MENU:
		if (e.u.m.id >= IDOFFSET && e.u.m.id < IDOFFSET+MAXNMENU &&
				menulist[e.u.m.id - IDOFFSET] != NULL)
			w = mkvalue("(Oi)",
				    menulist[e.u.m.id - IDOFFSET], e.u.m.item);
		else {
			/* Ghost menu event.
			   Can occur only on the Mac if another part
			   of the aplication has installed a menu;
			   like the THINK C console library. */
			DECREF(v);
			goto again;
		}
		break;
	case WE_KEY:
		w = mkvalue("(ii)", e.u.key.code, e.u.key.mask);
		break;
	case WE_LOST_SEL:
		w = newintobject((long)e.u.sel);
		break;
	default:
		w = None;
		INCREF(w);
		break;
	}
	if (w == NULL) {
		DECREF(v);
		return NULL;
	}
	u = mkvalue("(iOO)", e.type, v, w);
	XDECREF(v);
	XDECREF(w);
	return u;
}

static object *
stdwin_getevent(sw, args)
	object *sw;
	object *args;
{
	return stdwin_get_poll_event(0, args);
}

static object *
stdwin_pollevent(sw, args)
	object *sw;
	object *args;
{
	return stdwin_get_poll_event(1, args);
}

static object *
stdwin_setdefwinpos(sw, args)
	object *sw;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetdefwinpos(a[0], a[1]);
	INCREF(None);
	return None;
}

static object *
stdwin_setdefwinsize(sw, args)
	object *sw;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetdefwinsize(a[0], a[1]);
	INCREF(None);
	return None;
}

static object *
stdwin_setdefscrollbars(sw, args)
	object *sw;
	object *args;
{
	int a[2];
	if (!getpointarg(args, a))
		return NULL;
	wsetdefscrollbars(a[0], a[1]);
	INCREF(None);
	return None;
}

static object *
stdwin_getdefwinpos(self, args)
	object *self;
	object *args;
{
	int h, v;
	if (!getnoarg(args))
		return NULL;
	wgetdefwinpos(&h, &v);
	return makepoint(h, v);
}

static object *
stdwin_getdefwinsize(self, args)
	object *self;
	object *args;
{
	int width, height;
	if (!getnoarg(args))
		return NULL;
	wgetdefwinsize(&width, &height);
	return makepoint(width, height);
}

static object *
stdwin_getdefscrollbars(self, args)
	object *self;
	object *args;
{
	int h, v;
	if (!getnoarg(args))
		return NULL;
	wgetdefscrollbars(&h, &v);
	return makepoint(h, v);
}

static object *
stdwin_menucreate(self, args)
	object *self;
	object *args;
{
	char *title;
	if (!getstrarg(args, &title))
		return NULL;
	wmenusetdeflocal(0);
	return (object *)newmenuobject(title);
}

static object *
stdwin_askfile(self, args)
	object *self;
	object *args;
{
	char *prompt, *dflt;
	int new, ret;
	char buf[256];
	if (!getargs(args, "(ssi)", &prompt, &dflt, &new))
		return NULL;
	strncpy(buf, dflt, sizeof buf);
	buf[sizeof buf - 1] = '\0';
	BGN_STDWIN
	ret = waskfile(prompt, buf, sizeof buf, new);
	END_STDWIN
	if (!ret) {
		err_set(KeyboardInterrupt);
		return NULL;
	}
	return newstringobject(buf);
}

static object *
stdwin_askync(self, args)
	object *self;
	object *args;
{
	char *prompt;
	int new, ret;
	if (!getargs(args, "(si)", &prompt, &new))
		return NULL;
	BGN_STDWIN
	ret = waskync(prompt, new);
	END_STDWIN
	if (ret < 0) {
		err_set(KeyboardInterrupt);
		return NULL;
	}
	return newintobject((long)ret);
}

static object *
stdwin_askstr(self, args)
	object *self;
	object *args;
{
	char *prompt, *dflt;
	int ret;
	char buf[256];
	if (!getargs(args, "(ss)", &prompt, &dflt))
		return NULL;
	strncpy(buf, dflt, sizeof buf);
	buf[sizeof buf - 1] = '\0';
	BGN_STDWIN
	ret = waskstr(prompt, buf, sizeof buf);
	END_STDWIN
	if (!ret) {
		err_set(KeyboardInterrupt);
		return NULL;
	}
	return newstringobject(buf);
}

static object *
stdwin_message(self, args)
	object *self;
	object *args;
{
	char *msg;
	if (!getstrarg(args, &msg))
		return NULL;
	BGN_STDWIN
	wmessage(msg);
	END_STDWIN
	INCREF(None);
	return None;
}

static object *
stdwin_fleep(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	wfleep();
	INCREF(None);
	return None;
}

static object *
stdwin_setcutbuffer(self, args)
	object *self;
	object *args;
{
	int i, size;
	char *str;
	if (!getargs(args, "(is#)", &i, &str, &size))
		return NULL;
	wsetcutbuffer(i, str, size);
	INCREF(None);
	return None;
}

static object *
stdwin_getactive(self, args)
	object *self;
	object *args;
{
	return window2object(wgetactive());
}

static object *
stdwin_getcutbuffer(self, args)
	object *self;
	object *args;
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
	return newsizedstringobject(str, len);
}

static object *
stdwin_rotatecutbuffers(self, args)
	object *self;
	object *args;
{
	int i;
	if (!getintarg(args, &i))
		return NULL;
	wrotatecutbuffers(i);
	INCREF(None);
	return None;
}

static object *
stdwin_getselection(self, args)
	object *self;
	object *args;
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
	return newsizedstringobject(data, len);
}

static object *
stdwin_resetselection(self, args)
	object *self;
	object *args;
{
	int sel;
	if (!getintarg(args, &sel))
		return NULL;
	wresetselection(sel);
	INCREF(None);
	return None;
}

static object *
stdwin_fetchcolor(self, args)
	object *self;
	object *args;
{
	char *colorname;
	COLOR color;
	if (!getstrarg(args, &colorname))
		return NULL;
	color = wfetchcolor(colorname);
#ifdef BADCOLOR
	if (color == BADCOLOR) {
		err_setstr(StdwinError, "color name not found");
		return NULL;
	}
#endif
	return newintobject((long)color);
}

static object *
stdwin_getscrsize(self, args)
	object *self;
	object *args;
{
	int width, height;
	if (!getnoarg(args))
		return NULL;
	wgetscrsize(&width, &height);
	return makepoint(width, height);
}

static object *
stdwin_getscrmm(self, args)
	object *self;
	object *args;
{
	int width, height;
	if (!getnoarg(args))
		return NULL;
	wgetscrmm(&width, &height);
	return makepoint(width, height);
}

#ifdef unix
static object *
stdwin_connectionnumber(self, args)
	object *self;
	object *args;
{
	if (!getnoarg(args))
		return NULL;
	return newintobject((long) wconnectionnumber());
}
#endif

static object *
stdwin_listfontnames(self, args)
	object *self;
	object *args;
{
	char *pattern;
	char **fontnames;
	int count;
	object *list;
	if (!getargs(args, "z", &pattern))
		return NULL;
	fontnames = wlistfontnames(pattern, &count);
	list = newlistobject(count);
	if (list != NULL) {
		int i;
		for (i = 0; i < count; i++) {
			object *v = newstringobject(fontnames[i]);
			if (v == NULL) {
				DECREF(list);
				list = NULL;
				break;
			}
			setlistitem(list, i, v);
		}
	}
	return list;
}

static object *
stdwin_newbitmap(self, args)
	object *self;
	object *args;
{
	int width, height;
	bitmapobject *bp;
	if (!getargs(args, "(ii)", &width, &height))
		return NULL;
	return (object *)newbitmapobject(width, height);
}

static struct methodlist stdwin_methods[] = {
	{"askfile",		stdwin_askfile},
	{"askstr",		stdwin_askstr},
	{"askync",		stdwin_askync},
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
	{"newbitmap",		stdwin_newbitmap},
	{"open",		stdwin_open},
	{"pollevent",		stdwin_pollevent},
	{"resetselection",	stdwin_resetselection},
	{"rotatecutbuffers",	stdwin_rotatecutbuffers},
	{"setcutbuffer",	stdwin_setcutbuffer},
	{"setdefscrollbars",	stdwin_setdefscrollbars},
	{"setdefwinpos",	stdwin_setdefwinpos},
	{"setdefwinsize",	stdwin_setdefwinsize},
	
	/* Text measuring methods borrow code from drawing objects: */
	{"baseline",		drawing_baseline},
	{"lineheight",		drawing_lineheight},
	{"textbreak",		drawing_textbreak},
	{"textwidth",		drawing_textwidth},

	/* Same for font setting methods: */
	{"setfont",		drawing_setfont},

	/* Same for color setting/getting methods: */
	{"getbgcolor",		drawing_getbgcolor},
	{"getfgcolor",		drawing_getfgcolor},
	{"setbgcolor",		drawing_setbgcolor},
	{"setfgcolor",		drawing_setfgcolor},

	{NULL,			NULL}		/* sentinel */
};

void
initstdwin()
{
	object *m, *d;
	static int inited = 0;

	if (!inited) {
		winit();
		inited = 1;
	}
	m = initmodule("stdwin", stdwin_methods);
	d = getmoduledict(m);
	
	/* Initialize stdwin.error exception */
	StdwinError = newstringobject("stdwin.error");
	if (StdwinError == NULL || dictinsert(d, "error", StdwinError) != 0)
		fatal("can't define stdwin.error");
#ifdef USE_THREAD
	StdwinLock = allocate_lock();
	if (StdwinLock == NULL)
		fatal("can't allocate stdwin lock");
#endif
}
