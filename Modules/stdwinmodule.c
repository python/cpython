/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

#include "stdwin.h"

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


/* Strongly stdwin-specific argument handlers */

static int
getmousedetail(v, ep)
	object *v;
	EVENT *ep;
{
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 4)
		return err_badarg();
	return getintintarg(gettupleitem(v, 0),
					&ep->u.where.h, &ep->u.where.v) &&
		getintarg(gettupleitem(v, 1), &ep->u.where.clicks) &&
		getintarg(gettupleitem(v, 2), &ep->u.where.button) &&
		getintarg(gettupleitem(v, 3), &ep->u.where.mask);
}

static int
getmenudetail(v, ep)
	object *v;
	EVENT *ep;
{
	object *mp;
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 2)
		return err_badarg();
	mp = gettupleitem(v, 0);
	if (mp == NULL || !is_menuobject(mp))
		return err_badarg();
	ep->u.m.id = ((menuobject *)mp) -> m_id;
	return getintarg(gettupleitem(v, 1), &ep->u.m.item);
}

static int
geteventarg(v, ep)
	object *v;
	EVENT *ep;
{
	object *wp, *detail;
	int a[4];
	if (v == NULL || !is_tupleobject(v) || gettuplesize(v) != 3)
		return err_badarg();
	if (!getintarg(gettupleitem(v, 0), &ep->type))
		return 0;
	wp = gettupleitem(v, 1);
	if (wp == None)
		ep->window = NULL;
	else if (wp == NULL || !is_windowobject(wp))
		return err_badarg();
	else
		ep->window = ((windowobject *)wp) -> w_win;
	detail = gettupleitem(v, 2);
	switch (ep->type) {
	case WE_CHAR:
		if (!is_stringobject(detail) || getstringsize(detail) != 1)
		return err_badarg();
		ep->u.character = getstringvalue(detail)[0];
		return 1;
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
		return getmousedetail(detail, ep);
	case WE_MENU:
		return getmenudetail(detail, ep);
	default:
		return 1;
	}
}


/* Return construction tools */

static object *
makepoint(a, b)
	int a, b;
{
	object *v;
	object *w;
	if ((v = newtupleobject(2)) == NULL)
		return NULL;
	if ((w = newintobject((long)a)) == NULL ||
		settupleitem(v, 0, w) != 0 ||
		(w = newintobject((long)b)) == NULL ||
		settupleitem(v, 1, w) != 0) {
		DECREF(v);
		return NULL;
	}
	return v;
}

static object *
makerect(a, b, c, d)
	int a, b, c, d;
{
	object *v;
	object *w;
	if ((v = newtupleobject(2)) == NULL)
		return NULL;
	if ((w = makepoint(a, b)) == NULL ||
		settupleitem(v, 0, w) != 0 ||
		(w = makepoint(c, d)) == NULL ||
		settupleitem(v, 1, w) != 0) {
		DECREF(v);
		return NULL;
	}
	return v;
}

static object *
makemouse(hor, ver, clicks, button, mask)
	int hor, ver, clicks, button, mask;
{
	object *v;
	object *w;
	if ((v = newtupleobject(4)) == NULL)
		return NULL;
	if ((w = makepoint(hor, ver)) == NULL ||
		settupleitem(v, 0, w) != 0 ||
		(w = newintobject((long)clicks)) == NULL ||
		settupleitem(v, 1, w) != 0 ||
		(w = newintobject((long)button)) == NULL ||
		settupleitem(v, 2, w) != 0 ||
		(w = newintobject((long)mask)) == NULL ||
		settupleitem(v, 3, w) != 0) {
		DECREF(v);
		return NULL;
	}
	return v;
}

static object *
makemenu(mp, item)
	object *mp;
	int item;
{
	object *v;
	object *w;
	if ((v = newtupleobject(2)) == NULL)
		return NULL;
	INCREF(mp);
	if (settupleitem(v, 0, mp) != 0 ||
		(w = newintobject((long)item)) == NULL ||
		settupleitem(v, 1, w) != 0) {
		DECREF(v);
		return NULL;
	}
	return v;
}


/* Drawing objects */

typedef struct {
	OB_HEAD
	windowobject	*d_ref;
} drawingobject;

static drawingobject *Drawing; /* Set to current drawing object, or NULL */

/* Drawing methods */

static void
drawing_dealloc(dp)
	drawingobject *dp;
{
	wenddrawing(dp->d_ref->w_win);
	Drawing = NULL;
	DECREF(dp->d_ref);
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
	drawing_generic(dp, args, wdrawline);
}

static object *
drawing_xorline(dp, args)
	drawingobject *dp;
	object *args;
{
	drawing_generic(dp, args, wxorline);
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
drawing_box(dp, args)
	drawingobject *dp;
	object *args;
{
	drawing_generic(dp, args, wdrawbox);
}

static object *
drawing_erase(dp, args)
	drawingobject *dp;
	object *args;
{
	drawing_generic(dp, args, werase);
}

static object *
drawing_paint(dp, args)
	drawingobject *dp;
	object *args;
{
	drawing_generic(dp, args, wpaint);
}

static object *
drawing_invert(dp, args)
	drawingobject *dp;
	object *args;
{
	drawing_generic(dp, args, winvert);
}

static object *
drawing_cliprect(dp, args)
	drawingobject *dp;
	object *args;
{
	drawing_generic(dp, args, wcliprect);
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
	int a[2];
	object *s;
	if (!getpointstrarg(args, a, &s))
		return NULL;
	wdrawtext(a[0], a[1], getstringvalue(s), (int)getstringsize(s));
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
	object *s;
	if (!getstrarg(args, &s))
		return NULL;
	return newintobject(
		(long)wtextwidth(getstringvalue(s), (int)getstringsize(s)));
}

static object *
drawing_textbreak(dp, args)
	drawingobject *dp;
	object *args;
{
	object *s;
	int a;
	if (!getstrintarg(args, &s, &a))
		return NULL;
	return newintobject(
		(long)wtextbreak(getstringvalue(s), (int)getstringsize(s), a));
}

static struct methodlist drawing_methods[] = {
	{"box",		drawing_box},
	{"circle",	drawing_circle},
	{"cliprect",	drawing_cliprect},
	{"elarc",	drawing_elarc},
	{"erase",	drawing_erase},
	{"invert",	drawing_invert},
	{"line",	drawing_line},
	{"noclip",	drawing_noclip},
	{"paint",	drawing_paint},
	{"shade",	drawing_shade},
	{"text",	drawing_text},
	{"xorline",	drawing_xorline},
	
	/* Text measuring methods: */
	{"baseline",	drawing_baseline},
	{"lineheight",	drawing_lineheight},
	{"textbreak",	drawing_textbreak},
	{"textwidth",	drawing_textwidth},
	{NULL,		NULL}		/* sentinel */
};

static object *
drawing_getattr(wp, name)
	drawingobject *wp;
	char *name;
{
	return findmethod(drawing_methods, (object *)wp, name);
}

static typeobject Drawingtype = {
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
	if (tp->t_attr != NULL)
		DECREF(tp->t_attr);
	DECREF(tp->t_ref);
	DEL(tp);
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
		err_setstr(RuntimeError, "not drawing");
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
		/* Hide/show focus around draw call; these are undocumented,
		   but required here to get the highlighting correct.
		   The call to werase is also required for this reason.
		   Finally, this forces us to require (above) that we are NOT
		   already drawing. */
		tehidefocus(tp);
		wbegindrawing(self->t_ref->w_win);
		werase(a[0], a[1], a[2], a[3]);
		tedrawnew(tp, a[0], a[1], a[2], a[3]);
		wenddrawing(self->t_ref->w_win);
		teshowfocus(tp);
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
text_replace(self, args)
	textobject *self;
	object *args;
{
	object *text;
	if (!getstrarg(args, &text))
		return NULL;
	tereplace(self->t_text, getstringvalue(text));
	INCREF(None);
	return None;
}

static struct methodlist text_methods[] = {
	"arrow",	text_arrow,
	"draw",		text_draw,
	"event",	text_event,
	"getfocus",	text_getfocus,
	"getfocustext",	text_getfocustext,
	"getrect",	text_getrect,
	"gettext",	text_gettext,
	"move",		text_move,
	"replace",	text_replace,
	"setfocus",	text_setfocus,
	{NULL,		NULL}		/* sentinel */
};

static object *
text_getattr(tp, name)
	textobject *tp;
	char *name;
{
	if (tp->t_attr != NULL) {
		object *v = dictlookup(tp->t_attr, name);
		if (v != NULL) {
			INCREF(v);
			return v;
		}
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
	if (v == NULL)
		return dictremove(tp->t_attr, name);
	else
		return dictinsert(tp->t_attr, name, v);
}

static typeobject Texttype = {
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
#define MAXNMENU 20		/* Max #menus we allow */
static menuobject *menulist[MAXNMENU];

static menuobject *
newmenuobject(title)
	object *title;
{
	int id;
	MENU *menu;
	menuobject *mp;
	for (id = 0; id < MAXNMENU; id++) {
		if (menulist[id] == NULL)
			break;
	}
	if (id >= MAXNMENU)
		return (menuobject *) err_nomem();
	menu = wmenucreate(id + IDOFFSET, getstringvalue(title));
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
	wmenudelete(mp->m_menu);
	if (mp->m_attr != NULL)
		DECREF(mp->m_attr);
	DEL(mp);
}

static object *
menu_additem(self, args)
	menuobject *self;
	object *args;
{
	object *text;
	int shortcut;
	if (is_tupleobject(args)) {
		object *v;
		if (!getstrstrarg(args, &text, &v))
			return NULL;
		if (getstringsize(v) != 1) {
			err_badarg();
			return NULL;
		}
		shortcut = *getstringvalue(v) & 0xff;
	}
	else {
		if (!getstrarg(args, &text))
			return NULL;
		shortcut = -1;
	}
	wmenuadditem(self->m_menu, getstringvalue(text), shortcut);
	INCREF(None);
	return None;
}

static object *
menu_setitem(self, args)
	menuobject *self;
	object *args;
{
	int index;
	object *text;
	if (!getintstrarg(args, &index, &text))
		return NULL;
	wmenusetitem(self->m_menu, index, getstringvalue(text));
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
	if (!getintintarg(args, &index, &flag))
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
	if (!getintintarg(args, &index, &flag))
		return NULL;
	wmenucheck(self->m_menu, index, flag);
	INCREF(None);
	return None;
}

static struct methodlist menu_methods[] = {
	"additem",	menu_additem,
	"setitem",	menu_setitem,
	"enable",	menu_enable,
	"check",	menu_check,
	{NULL,		NULL}		/* sentinel */
};

static object *
menu_getattr(mp, name)
	menuobject *mp;
	char *name;
{
	if (mp->m_attr != NULL) {
		object *v = dictlookup(mp->m_attr, name);
		if (v != NULL) {
			INCREF(v);
			return v;
		}
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
	if (v == NULL)
		return dictremove(mp->m_attr, name);
	else
		return dictinsert(mp->m_attr, name, v);
}

static typeobject Menutype = {
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

static void
window_print(wp, fp, flags)
	windowobject *wp;
	FILE *fp;
	int flags;
{
	fprintf(fp, "<window titled '%s'>", getstringvalue(wp->w_title));
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
		err_setstr(RuntimeError, "already drawing");
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
	if (!getstrarg(args, &title))
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
	object *title;
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
	int sel;
	object *str;
	int ok;
	if (!getintstrarg(args, &sel, &str))
		return NULL;
	ok = wsetselection(self->w_win, sel,
		getstringvalue(str), (int)getstringsize(str));
	return newintobject(ok);
}

static object *
window_setwincursor(self, args)
	windowobject *self;
	object *args;
{
	object *str;
	CURSOR *c;
	if (!getstrarg(args, &str))
		return NULL;
	c = wfetchcursor(getstringvalue(str));
	if (c == NULL) {
		err_setstr(RuntimeError, "no such cursor");
		return NULL;
	}
	wsetwincursor(self->w_win, c);
	INCREF(None);
	return None;
}

static struct methodlist window_methods[] = {
	{"begindrawing",window_begindrawing},
	{"change",	window_change},
	{"getdocsize",	window_getdocsize},
	{"getorigin",	window_getorigin},
	{"gettitle",	window_gettitle},
	{"getwinsize",	window_getwinsize},
	{"menucreate",	window_menucreate},
	{"scroll",	window_scroll},
	{"setwincursor",window_setwincursor},
	{"setdocsize",	window_setdocsize},
	{"setorigin",	window_setorigin},
	{"setselection",window_setselection},
	{"settimer",	window_settimer},
	{"settitle",	window_settitle},
	{"show",	window_show},
	{"textcreate",	window_textcreate},
	{NULL,		NULL}		/* sentinel */
};

static object *
window_getattr(wp, name)
	windowobject *wp;
	char *name;
{
	if (wp->w_attr != NULL) {
		object *v = dictlookup(wp->w_attr, name);
		if (v != NULL) {
			INCREF(v);
			return v;
		}
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
	if (v == NULL)
		return dictremove(wp->w_attr, name);
	else
		return dictinsert(wp->w_attr, name, v);
}

static typeobject Windowtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"window",		/*tp_name*/
	sizeof(windowobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	window_dealloc,		/*tp_dealloc*/
	window_print,		/*tp_print*/
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
	if (!getstrarg(args, &title))
		return NULL;
	for (tag = 0; tag < MAXNWIN; tag++) {
		if (windowlist[tag] == NULL)
			break;
	}
	if (tag >= MAXNWIN)
		return err_nomem();
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
stdwin_get_poll_event(poll, args)
	int poll;
	object *args;
{
	EVENT e;
	object *v, *w;
	if (!getnoarg(args))
		return NULL;
	if (Drawing != NULL) {
		err_setstr(RuntimeError, "cannot getevent() while drawing");
		return NULL;
	}
/* again: */
	if (poll) {
		if (!wpollevent(&e)) {
			INCREF(None);
			return None;
		}
	}
	else
		wgetevent(&e);
	if (e.type == WE_COMMAND && e.u.command == WC_CANCEL) {
		/* Turn keyboard interrupts into exceptions */
		err_set(KeyboardInterrupt);
		return NULL;
	}
/*
	if (e.window == NULL && (e.type == WE_COMMAND || e.type == WE_CHAR))
		goto again;
*/
	if (e.type == WE_COMMAND && e.u.command == WC_CLOSE) {
		/* Turn WC_CLOSE commands into WE_CLOSE events */
		e.type = WE_CLOSE;
	}
	v = newtupleobject(3);
	if (v == NULL)
		return NULL;
	if ((w = newintobject((long)e.type)) == NULL) {
		DECREF(v);
		return NULL;
	}
	settupleitem(v, 0, w);
	if (e.window == NULL)
		w = None;
	else {
		int tag = wgettag(e.window);
		if (tag < 0 || tag >= MAXNWIN || windowlist[tag] == NULL)
			w = None;
		else
			w = (object *)windowlist[tag];
#ifdef sgi
		/* XXX Trap for unexplained weird bug */
		if ((long)w == (long)0x80000001) {
			err_setstr(SystemError,
				"bad pointer in stdwin.getevent()");
			return NULL;
		}
#endif
	}
	INCREF(w);
	settupleitem(v, 1, w);
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
		w = makemouse(e.u.where.h, e.u.where.v,
				e.u.where.clicks,
				e.u.where.button,
				e.u.where.mask);
		break;
	case WE_MENU:
		if (e.u.m.id >= IDOFFSET && e.u.m.id < IDOFFSET+MAXNMENU &&
				menulist[e.u.m.id - IDOFFSET] != NULL)
			w = (object *)menulist[e.u.m.id - IDOFFSET];
		else
			w = None;
		w = makemenu(w, e.u.m.item);
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
	settupleitem(v, 2, w);
	return v;
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
stdwin_getdefwinpos(wp, args)
	windowobject *wp;
	object *args;
{
	int h, v;
	if (!getnoarg(args))
		return NULL;
	wgetdefwinpos(&h, &v);
	return makepoint(h, v);
}

static object *
stdwin_getdefwinsize(wp, args)
	windowobject *wp;
	object *args;
{
	int width, height;
	if (!getnoarg(args))
		return NULL;
	wgetdefwinsize(&width, &height);
	return makepoint(width, height);
}

static object *
stdwin_menucreate(self, args)
	object *self;
	object *args;
{
	object *title;
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
	object *prompt, *dflt;
	int new, ret;
	char buf[256];
	if (!getstrstrintarg(args, &prompt, &dflt, &new))
		return NULL;
	strncpy(buf, getstringvalue(dflt), sizeof buf);
	buf[sizeof buf - 1] = '\0';
	ret = waskfile(getstringvalue(prompt), buf, sizeof buf, new);
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
	object *prompt;
	int new, ret;
	if (!getstrintarg(args, &prompt, &new))
		return NULL;
	ret = waskync(getstringvalue(prompt), new);
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
	object *prompt, *dflt;
	int ret;
	char buf[256];
	if (!getstrstrarg(args, &prompt, &dflt))
		return NULL;
	strncpy(buf, getstringvalue(dflt), sizeof buf);
	buf[sizeof buf - 1] = '\0';
	ret = waskstr(getstringvalue(prompt), buf, sizeof buf);
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
	object *msg;
	if (!getstrarg(args, &msg))
		return NULL;
	wmessage(getstringvalue(msg));
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
	int i;
	object *str;
	if (!getintstrarg(args, &i, &str))
		return NULL;
	wsetcutbuffer(i, getstringvalue(str), getstringsize(str));
	INCREF(None);
	return None;
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

static struct methodlist stdwin_methods[] = {
	{"askfile",		stdwin_askfile},
	{"askstr",		stdwin_askstr},
	{"askync",		stdwin_askync},
	{"fleep",		stdwin_fleep},
	{"getselection",	stdwin_getselection},
	{"getcutbuffer",	stdwin_getcutbuffer},
	{"getdefwinpos",	stdwin_getdefwinpos},
	{"getdefwinsize",	stdwin_getdefwinsize},
	{"getevent",		stdwin_getevent},
	{"menucreate",		stdwin_menucreate},
	{"message",		stdwin_message},
	{"open",		stdwin_open},
	{"pollevent",		stdwin_pollevent},
	{"resetselection",	stdwin_resetselection},
	{"rotatecutbuffers",	stdwin_rotatecutbuffers},
	{"setcutbuffer",	stdwin_setcutbuffer},
	{"setdefwinpos",	stdwin_setdefwinpos},
	{"setdefwinsize",	stdwin_setdefwinsize},
	
	/* Text measuring methods borrow code from drawing objects: */
	{"baseline",		drawing_baseline},
	{"lineheight",		drawing_lineheight},
	{"textbreak",		drawing_textbreak},
	{"textwidth",		drawing_textwidth},
	{NULL,			NULL}		/* sentinel */
};

void
initstdwin()
{
	static int inited;
	if (!inited) {
		winit();
		inited = 1;
	}
	initmodule("stdwin", stdwin_methods);
}
