/***********************************************************
Copyright 1994 by Lance Ellinghouse,
Cathedral City, California Republic, United States of America.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Lance Ellinghouse
not be used in advertising or publicity pertaining to distribution 
of the software without specific, written prior permission.

LANCE ELLINGHOUSE DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL LANCE ELLINGHOUSE BE LIABLE FOR ANY SPECIAL, 
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING 
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/******************************************************************
This is a curses implementation. I have tried to be as complete
as possible. If there are functions you need that are not included,
please let me know and/or send me some diffs.

There are 3 basic types exported by this module:
   1) Screen - This is not currently used
   2) Window - This is the basic type. This is equivalent to "WINDOW *".
   3) Pad    - This is similar to Window, but works with Pads as defined
               in curses.

Most of the routines can be looked up using the curses man page.

Here is a list of the currently supported methods and attributes
in the curses module:

Return Value      Func/Attr            Description
--------------------------------------------------------------------------
StringObject      version              A string representing the current
                                       version of this module.
WindowObject      initscr()            This initializes the screen for use
None              endwin()             Closes down the screen and returns
                                       things as they were before calling
                                       initscr()
True/FalseObject  isendwin()           Has endwin() been called?
None              doupdate()           Updates screen
WindowObject      newwin(nlines,ncols,begin_y,begin_x)
                  newwin(begin_y,begin_x)
                                       newwin() creates and returns
                                       a new window.
None              beep()               Beep the screen if possible
None              flash()              Flash the screen if possible
None              ungetch(int)         Push the int back so next getch()
                                       will return it.
                                       Note: argument is an INT, not a CHAR
None              flushinp()           Flush all input buffers
None              cbreak()             Enter cbreak mode
None              nocbreak()           Leave cbreak mode
None              echo()               Enter echo mode
None              noecho()             Leave echo mode
None              nl()                 Enter nl mode
None              nonl()               Leave nl mode
None              raw()                Enter raw mode
None              noraw()              Leave raw mode
None              intrflush(int)       Set or reset interruptable flush
                                       mode, int=1 if set, 0 if notset.
None              meta(int)            Allow 8 bit or 7 bit chars.
                                       int=1 is 8 bit, int=0 is 7 bit
StringObject      keyname(int)         return the text representation
                                       of a KEY_ value. (see below)

Here is a list of the currently supported methods and attributes
in the WindowObject:

Return Value      Func/Attr            Description
--------------------------------------------------------------------------
None              refresh()            Do refresh
None              nooutrefresh()       Mark for refresh but wait
None              mvwin(new_y,new_x)   Move Window
None              move(new_y,new_x)    Move Cursor
WindowObject      subwin(nlines,ncols,begin_y,begin_x)
                  subwin(begin_y,begin_x)
None              addch(y,x,ch,attr)
                  addch(y,x,ch)
                  addch(ch,attr)
                  addch(ch)
None              insch(y,x,ch,attr)
                  insch(y,x,ch)
                  insch(ch,attr)
                  insch(ch)
None              delch(y,x)
                  delch()
None              echochar(ch,attr)
                  echochar(ch)
None              addstr(y,x,str,attr)
                  addstr(y,x,str)
                  addstr(str,attr)
                  addstr(str)
None              attron(attr)
None              attroff(attr)
None              attrset(sttr)
None              standend()
None              standout()
None              border(ls,rs,ts,bs,tl,tr,bl,br)   (accepts 0-8 INT args)
None              box(vertch,horch)    vertch and horch are INTS
                  box()
None              hline(y,x,ch,n)
                  hline(ch,n)
None              vline(y,x,ch,n)
                  vline(ch,n)
None              erase()
None              deleteln()
None              insertln()
(y,x)             getyx()
(y,x)             getbegyx()
(y,x)             getmaxyx()
None              clear()
None              clrtobot()
None              clrtoeol()
None              scroll()
None              touchwin()
None              touchline(start,count)
IntObject         getch(y,x)
                  getch()
StringObject      getstr(y,x)
                  getstr()
IntObject         inch(y,x)
                  inch()
None              clearok(int)      int=0 or int=1
None              idlok(int)        int=0 or int=1
None              leaveok(int)      int=0 or int=1
None              scrollok(int)     int=0 or int=1
None              setscrreg(top,bottom)
None              keypad(int)       int=0 or int=1
None              nodelay(int)      int=0 or int=1
None              notimeout(int)    int=0 or int=1
******************************************************************/


/* curses module */

#include "Python.h"

#ifdef HAVE_NCURSES_H
/* Now let's hope there aren't systems that have a broken ncurses.h */
#include <ncurses.h>
#else
#include <curses.h>
#endif

typedef struct {
	PyObject_HEAD
	SCREEN *scr;
} PyCursesScreenObject;

typedef struct {
	PyObject_HEAD
	WINDOW *win;
	WINDOW *parent;
} PyCursesWindowObject;

typedef struct {
	PyObject_HEAD
	WINDOW *pad;
} PyCursesPadObject;

#if 0
staticforward PyTypeObject PyCursesScreen_Type;
#endif
staticforward PyTypeObject PyCursesWindow_Type;
#if 0
staticforward PyTypeObject PyCursesPad_Type;
#endif

#define PyCursesScreen_Check(v)	 ((v)->ob_type == &PyCursesScreen_Type)
#define PyCursesWindow_Check(v)	 ((v)->ob_type == &PyCursesWindow_Type)
#define PyCursesPad_Check(v)	 ((v)->ob_type == &PyCursesPad_Type)

/* Defines */
static PyObject *PyCursesError;		/* For exception curses.error */

/* Catch-all error messages */
static char *catchall_ERR  = "curses function returned ERR";
static char *catchall_NULL = "curses function returned NULL";

/* Tells whether initscr() has been called to initialise curses  */
static int initialised = FALSE;

#define ARG_COUNT(X) \
	(((X) == NULL) ? 0 : (PyTuple_Check(X) ? PyTuple_Size(X) : 1))

/******************************************************************

Change Log:

Version 1.2: 95/02/23 (Steve Clift)
    Fixed several potential core-dumping bugs.
    Reworked arg parsing where variable arg lists are used.
    Generate exceptions when ERR or NULL is returned by curses functions.
    Changed return types to match SysV Curses manual descriptions.
    Added keypad() to window method list.
    Added border(), hline() and vline() window methods.

Version 1.1: 94/08/31:
    Minor fixes given by Guido.
    Changed 'ncurses' to 'curses'
    Changed '__version__' to 'version'
    Added PyErr_Clear() where needed
    Moved ACS_* attribute initialization to PyCurses_InitScr() to fix
        crash on SGI

Version 1.0: 94/08/30:
    This is the first release of this software.
    Released to the Internet via python-list@cwi.nl

******************************************************************/

static char *PyCursesVersion = "1.2";

/*
 * Check the return code from a curses function and return None 
 * or raise an exception as appropriate.
 */

static PyObject *
PyCursesCheckERR(code, fname)
     int code;
     char *fname;
{
  char buf[100];

  if (code != ERR) {
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    if (fname == NULL) {
      PyErr_SetString(PyCursesError, catchall_ERR);
    } else {
      strcpy(buf, fname);
      strcat(buf, "() returned ERR");
      PyErr_SetString(PyCursesError, buf);
    }
    return NULL;
  }
}


static int
PyCursesInitialised()
{
  if (initialised == TRUE)
    return 1;
  else {
    PyErr_SetString(PyCursesError, "must call initscr() first");
    return 0;
  }
}


/* ------------- SCREEN routines --------------- */

#ifdef NOT_YET
static PyObject *
PyCursesScreen_New(arg)
	PyObject * arg;
{
        char *term_type;
	PyFileObject *in_fo;
	PyFileObject *out_fo;
	PyCursesScreenObject *xp;
	xp = PyObject_NEW(PyCursesScreenObject, &PyCursesScreen_Type);
	if (xp == NULL)
		return NULL;
	return (PyObject *)xp;
}
#endif


/* ------------- WINDOW routines --------------- */

static PyObject *
PyCursesWindow_New(win)
	WINDOW *win;
{
	PyCursesWindowObject *wo;

	wo = PyObject_NEW(PyCursesWindowObject, &PyCursesWindow_Type);
	if (wo == NULL)
		return NULL;
	wo->win = win;
	wo->parent = (WINDOW *)NULL;
	return (PyObject *)wo;
}

static void
PyCursesWindow_Dealloc(wo)
	PyCursesWindowObject *wo;
{
  if (wo->win != stdscr)
    delwin(wo->win);
  PyMem_DEL(wo);
}

static PyObject *
PyCursesWindow_Refresh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return NULL;
  return PyCursesCheckERR(wrefresh(self->win), "wrefresh");
}

static PyObject *
PyCursesWindow_NoOutRefresh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return NULL;
  return PyCursesCheckERR(wnoutrefresh(self->win), "wnoutrefresh");
}

static PyObject *
PyCursesWindow_MoveWin(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_Parse(arg,"(ii);y,x", &y, &x))
    return NULL;
  return PyCursesCheckERR(mvwin(self->win,y,x), "mvwin");
}

static PyObject *
PyCursesWindow_Move(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_Parse(arg,"(ii);y,x", &y, &x))
    return NULL;
  return PyCursesCheckERR(wmove(self->win,y,x), "wmove");
}

static PyObject *
PyCursesWindow_SubWin(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  WINDOW *win;
  PyCursesWindowObject *rtn_win;
  int nlines, ncols, begin_y, begin_x;

  nlines = 0;
  ncols  = 0;
  switch (ARG_COUNT(arg)) {
  case 2:
    if (!PyArg_Parse(arg,"(ii);begin_y,begin_x",&begin_y,&begin_x))
      return NULL;
    break;
  case 4:
    if (!PyArg_Parse(arg, "(iiii);nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "subwin requires 2 or 4 arguments");
    return NULL;
  }
  win = subwin(self->win,nlines,ncols,begin_y,begin_x);
  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }
  rtn_win = (PyCursesWindowObject *)PyCursesWindow_New(win);
  rtn_win->parent = self->win;
  return (PyObject *)rtn_win;
}

static PyObject *
PyCursesWindow_AddCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  int ch;
  int attr, attr_old = 0;
  int use_xy = FALSE, use_attr = FALSE;

  switch (ARG_COUNT(arg)) {
    case 1:
      if (!PyArg_Parse(arg, "i;ch", &ch))
        return NULL;
      break;
    case 2:
      if (!PyArg_Parse(arg,"(ii);ch,attr", &ch, &attr))
        return NULL;
      use_attr = TRUE;
      break;
    case 3:
      if (!PyArg_Parse(arg,"(iii);y,x,ch", &y, &x, &ch))
        return NULL;
      use_xy = TRUE;
      break;
    case 4:
      if (!PyArg_Parse(arg,"(iiii);y,x,ch,attr", &y, &x, &ch, &attr))
        return NULL;
      use_xy = use_attr = TRUE;
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "addch requires 1 to 4 arguments");
      return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwaddch(self->win,y,x,ch);
  else
    rtn = waddch(self->win,ch);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);

  return PyCursesCheckERR(rtn, "[mv]waddch");
}

static PyObject *
PyCursesWindow_InsCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  int ch;
  int attr, attr_old = 0;
  int use_xy = TRUE, use_attr = FALSE;

  switch (ARG_COUNT(arg)) {
    case 1:
      if (!PyArg_Parse(arg, "i;ch", &ch))
        return NULL;
      break;
    case 2:
      if (!PyArg_Parse(arg,"(ii);ch,attr", &ch, &attr))
        return NULL;
      use_attr = TRUE;
      break;
    case 3:
      if (!PyArg_Parse(arg,"(iii);y,x,ch", &y, &x, &ch))
        return NULL;
      use_xy = TRUE;
      break;
    case 4:
      if (!PyArg_Parse(arg,"(iiii);y,x,ch,attr", &y, &x, &ch, &attr))
        return NULL;
      use_xy = use_attr = TRUE;
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "insch requires 1 to 4 arguments");
      return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwinsch(self->win,y,x,ch);
  else
    rtn = winsch(self->win,ch);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);

  return PyCursesCheckERR(rtn, "[mv]winsch");
}

static PyObject *
PyCursesWindow_DelCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn = wdelch(self->win);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x", &y, &x))
      return NULL;
    rtn = mvwdelch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "delch requires 0 or 2 arguments");
    return NULL;
  }

  return PyCursesCheckERR(rtn, "[mv]wdelch");
}

static PyObject *
PyCursesWindow_EchoChar(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int ch;
  int attr, attr_old;

  switch (ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg,"i;ch", &ch))
      return NULL;
    rtn = wechochar(self->win,ch);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);ch,attr", &ch, &attr))
      return NULL;
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
    rtn = wechochar(self->win,ch);
    wattrset(self->win,attr_old);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "echochar requires 1 or 2 arguments");
    return NULL;
  }

  return PyCursesCheckERR(rtn, "wechochar");
}

static PyObject *
PyCursesWindow_AddStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  char *str;
  int attr, attr_old = 0;
  int use_xy = FALSE, use_attr = FALSE;

  switch (ARG_COUNT(arg)) {
  case 1:
    if (!PyArg_Parse(arg,"s;str", &str))
      return NULL;
    break;
  case 2:
    if (!PyArg_Parse(arg,"(si);str,attr", &str, &attr))
      return NULL;
    use_attr = TRUE;
    break;
  case 3:
    if (!PyArg_Parse(arg,"(iis);y,x,str", &y, &x, &str))
      return NULL;
    use_xy = TRUE;
    break;
  case 4:
    if (!PyArg_Parse(arg,"(iisi);y,x,str,attr", &y, &x, &str, &attr))
      return NULL;
    use_xy = use_attr = TRUE;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "addstr requires 1 to 4 arguments");
    return NULL;
  }

  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  if (use_xy == TRUE)
    rtn = mvwaddstr(self->win,y,x,str);
  else
    rtn = waddstr(self->win,str);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);

  return PyCursesCheckERR(rtn, "[mv]waddstr");
}

static PyObject *
PyCursesWindow_AttrOn(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;attr", &ch))
      return NULL;
  wattron(self->win,ch);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_AttrOff(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;attr", &ch))
      return NULL;
  wattroff(self->win,ch);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_AttrSet(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;attr", &ch))
      return NULL;
  wattrset(self->win,ch);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_StandEnd(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  wstandend(self->win);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_StandOut(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  wstandout(self->win);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_Border(self, args)
     PyCursesWindowObject *self;
     PyObject *args;
{
  int ls, rs, ts, bs, tl, tr, bl, br;
  ls = rs = ts = bs = tl = tr = bl = br = 0;
  if (!PyArg_ParseTuple(args,"|iiiiiiii;ls,rs,ts,bs,tl,tr,bl,br",
                        &ls, &rs, &ts, &bs, &tl, &tr, &bl, &br))
    return NULL;
  wborder(self->win, ls, rs, ts, bs, tl, tr, bl, br);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_Box(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int ch1=0,ch2=0;
  if (!PyArg_NoArgs(arg)) {
    PyErr_Clear();
    if (!PyArg_Parse(arg,"(ii);vertch,horch", &ch1, &ch2))
      return NULL;
  }
  box(self->win,ch1,ch2);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_Hline(self, args)
     PyCursesWindowObject *self;
     PyObject *args;
{
  int ch, n, x, y, code = OK;
  switch (ARG_COUNT(args)) {
  case 2:
    if (!PyArg_Parse(args, "(ii);ch,n", &ch, &n))
      return NULL;
    break;
  case 4:
    if (!PyArg_Parse(args, "(iiii);y,x,ch,n", &y, &x, &ch, &n))
      return NULL;
    code = wmove(self->win, y, x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "hline requires 2 or 4 arguments");
    return NULL;
  }
  if (code != ERR)
    whline(self->win, ch, n);
  return PyCursesCheckERR(code, "wmove");
}

static PyObject *
PyCursesWindow_Vline(self, args)
     PyCursesWindowObject *self;
     PyObject *args;
{
  int ch, n, x, y, code = OK;
  switch (ARG_COUNT(args)) {
  case 2:
    if (!PyArg_Parse(args, "(ii);ch,n", &ch, &n))
      return NULL;
    break;
  case 4:
    if (!PyArg_Parse(args, "(iiii);y,x,ch,n", &y, &x, &ch, &n))
      return NULL;
    code = wmove(self->win, y, x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "vline requires 2 or 4 arguments");
    return NULL;
  }
  if (code != ERR)
    wvline(self->win, ch, n);
  return PyCursesCheckERR(code, "wmove");
}

static PyObject *
PyCursesWindow_Erase(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  werase(self->win);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_DeleteLine(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  return PyCursesCheckERR(wdeleteln(self->win), "wdeleteln");
}

static PyObject *
PyCursesWindow_InsertLine(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  return PyCursesCheckERR(winsertln(self->win), "winsertln");
}

static PyObject *
PyCursesWindow_GetYX(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_NoArgs(arg))
      return NULL;
  getyx(self->win,y,x);
  return Py_BuildValue("(ii)", y, x);
}

static PyObject *
PyCursesWindow_GetBegYX(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_NoArgs(arg))
      return NULL;
  getbegyx(self->win,y,x);
  return Py_BuildValue("(ii)", y, x);
}

static PyObject *
PyCursesWindow_GetMaxYX(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_NoArgs(arg))
      return NULL;
  getmaxyx(self->win,y,x);
  return Py_BuildValue("(ii)", y, x);
}

static PyObject *
PyCursesWindow_Clear(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  wclear(self->win);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_ClearToBottom(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  wclrtobot(self->win);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_ClearToEOL(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  wclrtoeol(self->win);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_Scroll(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  return PyCursesCheckERR(scroll(self->win), "scroll");
}

static PyObject *
PyCursesWindow_TouchWin(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return NULL;
  return PyCursesCheckERR(touchwin(self->win), "touchwin");
}

static PyObject *
PyCursesWindow_TouchLine(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int st, cnt;
  if (!PyArg_Parse(arg,"(ii);start,count",&st,&cnt))
      return NULL;
  return PyCursesCheckERR(touchline(self->win,st,cnt), "touchline");
}

static PyObject *
PyCursesWindow_GetCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  int rtn;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn = wgetch(self->win);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
      return NULL;
    rtn = mvwgetch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "getch requires 0 or 2 arguments");
    return NULL;
  }

  return PyInt_FromLong((long) rtn);
}

static PyObject *
PyCursesWindow_GetStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  char rtn[1024]; /* This should be big enough.. I hope */
  int rtn2;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn2 = wgetstr(self->win,rtn);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
      return NULL;
    rtn2 = mvwgetstr(self->win,y,x,rtn);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "getstr requires 0 or 2 arguments");
    return NULL;
  }

  if (rtn2 == ERR)
    rtn[0] = 0;
  return PyString_FromString(rtn);
}

static PyObject *
PyCursesWindow_InCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y, rtn;

  switch (ARG_COUNT(arg)) {
  case 0:
    rtn = winch(self->win);
    break;
  case 2:
    if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
      return NULL;
    rtn = mvwinch(self->win,y,x);
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "inch requires 0 or 2 arguments");
    return NULL;
  }

  return PyInt_FromLong((long) rtn);
}

static PyObject *
PyCursesWindow_ClearOk(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int val;
  if (!PyArg_Parse(arg,"i;True(1) or False(0)",&val))
    return NULL;
  clearok(self->win,val);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_IdlOk(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int val;
  if (!PyArg_Parse(arg,"i;True(1) or False(0)",&val))
    return NULL;
  idlok(self->win,val);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_LeaveOk(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int val;
  if (!PyArg_Parse(arg,"i;True(1) or False(0)",&val))
    return NULL;
  leaveok(self->win,val);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_ScrollOk(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int val;
  if (!PyArg_Parse(arg,"i;True(1) or False(0)",&val))
    return NULL;
  scrollok(self->win,val);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_SetScrollRegion(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_Parse(arg,"(ii);top, bottom",&y,&x))
    return NULL;
  return PyCursesCheckERR(wsetscrreg(self->win,y,x), "wsetscrreg");
}

static PyObject *
PyCursesWindow_KeyPad(self,arg)
     PyCursesWindowObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return NULL;
  keypad(self->win,ch);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_NoDelay(self,arg)
     PyCursesWindowObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return NULL;
  nodelay(self->win,ch);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCursesWindow_NoTimeout(self,arg)
     PyCursesWindowObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return NULL;
  notimeout(self->win,ch);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef PyCursesWindow_Methods[] = {
	{"refresh",         (PyCFunction)PyCursesWindow_Refresh},
	{"nooutrefresh",    (PyCFunction)PyCursesWindow_NoOutRefresh},
	{"mvwin",           (PyCFunction)PyCursesWindow_MoveWin},
	{"move",            (PyCFunction)PyCursesWindow_Move},
	{"subwin",          (PyCFunction)PyCursesWindow_SubWin},
	{"addch",           (PyCFunction)PyCursesWindow_AddCh},
	{"insch",           (PyCFunction)PyCursesWindow_InsCh},
	{"delch",           (PyCFunction)PyCursesWindow_DelCh},
	{"echochar",        (PyCFunction)PyCursesWindow_EchoChar},
	{"addstr",          (PyCFunction)PyCursesWindow_AddStr},
	{"attron",          (PyCFunction)PyCursesWindow_AttrOn},
	{"attroff",         (PyCFunction)PyCursesWindow_AttrOff},
	{"attrset",         (PyCFunction)PyCursesWindow_AttrSet},
	{"standend",        (PyCFunction)PyCursesWindow_StandEnd},
	{"standout",        (PyCFunction)PyCursesWindow_StandOut},
	{"border",          (PyCFunction)PyCursesWindow_Border, METH_VARARGS},
	{"box",             (PyCFunction)PyCursesWindow_Box},
	{"hline",           (PyCFunction)PyCursesWindow_Hline},
	{"vline",           (PyCFunction)PyCursesWindow_Vline},
	{"erase",           (PyCFunction)PyCursesWindow_Erase},
	{"deleteln",        (PyCFunction)PyCursesWindow_DeleteLine},
	{"insertln",        (PyCFunction)PyCursesWindow_InsertLine},
	{"getyx",           (PyCFunction)PyCursesWindow_GetYX},
	{"getbegyx",        (PyCFunction)PyCursesWindow_GetBegYX},
	{"getmaxyx",        (PyCFunction)PyCursesWindow_GetMaxYX},
	{"clear",           (PyCFunction)PyCursesWindow_Clear},
	{"clrtobot",        (PyCFunction)PyCursesWindow_ClearToBottom},
	{"clrtoeol",        (PyCFunction)PyCursesWindow_ClearToEOL},
	{"scroll",          (PyCFunction)PyCursesWindow_Scroll},
	{"touchwin",        (PyCFunction)PyCursesWindow_TouchWin},
	{"touchline",       (PyCFunction)PyCursesWindow_TouchLine},
	{"getch",           (PyCFunction)PyCursesWindow_GetCh},
	{"getstr",          (PyCFunction)PyCursesWindow_GetStr},
	{"inch",            (PyCFunction)PyCursesWindow_InCh},
	{"clearok",         (PyCFunction)PyCursesWindow_ClearOk},
	{"idlok",           (PyCFunction)PyCursesWindow_IdlOk},
	{"leaveok",         (PyCFunction)PyCursesWindow_LeaveOk},
	{"scrollok",        (PyCFunction)PyCursesWindow_ScrollOk},
	{"setscrreg",       (PyCFunction)PyCursesWindow_SetScrollRegion},
	{"keypad",          (PyCFunction)PyCursesWindow_KeyPad},
	{"nodelay",         (PyCFunction)PyCursesWindow_NoDelay},
	{"notimeout",       (PyCFunction)PyCursesWindow_NoTimeout},
	{NULL,		        NULL}   /* sentinel */
};

static PyObject *
PyCursesWindow_GetAttr(self, name)
	PyCursesWindowObject *self;
	char *name;
{
  return Py_FindMethod(PyCursesWindow_Methods, (PyObject *)self, name);
}


/* --------------- PAD routines ---------------- */

#ifdef NOT_YET
static PyObject *
PyCursesPad_New(pad)
	WINDOW *pad;
{
	PyCursesPadObject *po;
	po = PyObject_NEW(PyCursesPadObject, &PyCursesPad_Type);
	if (po == NULL)
		return NULL;
	po->pad = pad;
	return (PyObject *)po;
}
#endif


/* -------------------------------------------------------*/

#if 0
static PyTypeObject PyCursesScreen_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"curses screen",	/*tp_name*/
	sizeof(PyCursesScreenObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)0 /*PyCursesScreen_Dealloc*/, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};
#endif

static PyTypeObject PyCursesWindow_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"curses window",	/*tp_name*/
	sizeof(PyCursesWindowObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)PyCursesWindow_Dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)PyCursesWindow_GetAttr, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};

#if 0
static PyTypeObject PyCursesPad_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"curses pad",	/*tp_name*/
	sizeof(PyCursesPadObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)0 /*PyCursesPad_Dealloc*/, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};
#endif


/* -------------------------------------------------------*/

static PyObject *ModDict;

static PyObject * 
PyCurses_InitScr(self, args)
     PyObject * self;
     PyObject * args;
{
  WINDOW *win;
  if (!PyArg_NoArgs(args))
    return NULL;
  if (initialised == TRUE) {
    wrefresh(stdscr);
    return (PyObject *)PyCursesWindow_New(stdscr);
  }

  win = initscr();
  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  initialised = TRUE;

/* This was moved from initcurses() because core dumped on SGI */
/* Also, they are probably not defined until you've called initscr() */
#define SetDictInt(string,ch) \
	PyDict_SetItemString(ModDict,string,PyInt_FromLong((long) (ch)));
 
	/* Here are some graphic symbols you can use */
        SetDictInt("ACS_ULCORNER",(ACS_ULCORNER));
	SetDictInt("ACS_ULCORNER",(ACS_ULCORNER));
	SetDictInt("ACS_LLCORNER",(ACS_LLCORNER));
	SetDictInt("ACS_URCORNER",(ACS_URCORNER));
	SetDictInt("ACS_LRCORNER",(ACS_LRCORNER));
	SetDictInt("ACS_RTEE",    (ACS_RTEE));
	SetDictInt("ACS_LTEE",    (ACS_LTEE));
	SetDictInt("ACS_BTEE",    (ACS_BTEE));
	SetDictInt("ACS_TTEE",    (ACS_TTEE));
	SetDictInt("ACS_HLINE",   (ACS_HLINE));
	SetDictInt("ACS_VLINE",   (ACS_VLINE));
	SetDictInt("ACS_PLUS",    (ACS_PLUS));
	SetDictInt("ACS_S1",      (ACS_S1));
	SetDictInt("ACS_S9",      (ACS_S9));
	SetDictInt("ACS_DIAMOND", (ACS_DIAMOND));
	SetDictInt("ACS_CKBOARD", (ACS_CKBOARD));
	SetDictInt("ACS_DEGREE",  (ACS_DEGREE));
	SetDictInt("ACS_PLMINUS", (ACS_PLMINUS));
	SetDictInt("ACS_BULLET",  (ACS_BULLET));
	SetDictInt("ACS_LARROW",  (ACS_RARROW));
	SetDictInt("ACS_DARROW",  (ACS_DARROW));
	SetDictInt("ACS_UARROW",  (ACS_UARROW));
	SetDictInt("ACS_BOARD",   (ACS_BOARD));
	SetDictInt("ACS_LANTERN", (ACS_LANTERN));
	SetDictInt("ACS_BLOCK",   (ACS_BLOCK));

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject * 
PyCurses_EndWin(self, args)
     PyObject * self;
     PyObject * args;
{
  if (!PyArg_NoArgs(args) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(endwin(), "endwin");
}

static PyObject * 
PyCurses_IsEndWin(self, args)
     PyObject * self;
     PyObject * args;
{
  if (!PyArg_NoArgs(args))
    return NULL;
  if (isendwin() == FALSE) {
    Py_INCREF(Py_False);
    return Py_False;
  }
  Py_INCREF(Py_True);
  return Py_True;
}

static PyObject *
PyCurses_DoUpdate(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(doupdate(), "doupdate");
}

static PyObject *
PyCurses_NewWindow(self,arg)
     PyObject * self;
     PyObject * arg;
{
  WINDOW *win;
  int nlines, ncols, begin_y, begin_x;

  if (!PyCursesInitialised())
    return NULL;
  nlines = ncols = 0;
  switch (ARG_COUNT(arg)) {
  case 2:
    if (!PyArg_Parse(arg,"(ii);begin)_y,begin_x",&begin_y,&begin_x))
      return NULL;
    break;
  case 4:
    if (!PyArg_Parse(arg, "(iiii);nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
      return NULL;
    break;
  default:
    PyErr_SetString(PyExc_TypeError, "newwin requires 2 or 4 arguments");
    return NULL;
  }

  win = newwin(nlines,ncols,begin_y,begin_x);
  if (win == NULL) {
    PyErr_SetString(PyCursesError, catchall_NULL);
    return NULL;
  }

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCurses_Beep(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  beep();
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCurses_Flash(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  flash();
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCurses_UngetCh(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;integer",&ch) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(ungetch(ch), "ungetch");
}

static PyObject *
PyCurses_FlushInp(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  flushinp();
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyCurses_CBreak(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(cbreak(), "cbreak");
}

static PyObject *
PyCurses_NoCBreak(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(nocbreak(), "nocbreak");
}

static PyObject *
PyCurses_Echo(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(echo(), "echo");
}

static PyObject *
PyCurses_NoEcho(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(noecho(), "noecho");
}

static PyObject *
PyCurses_Nl(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(nl(), "nl");
}

static PyObject *
PyCurses_NoNl(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(nonl(), "nonl");
}

static PyObject *
PyCurses_Raw(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(raw(), "raw");
}

static PyObject *
PyCurses_NoRaw(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(noraw(), "noraw");
}

static PyObject *
PyCurses_IntrFlush(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return NULL;
  return PyCursesCheckERR(intrflush(NULL,ch), "intrflush");
}

static PyObject *
PyCurses_Meta(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch) || !PyCursesInitialised())
    return NULL;
  return PyCursesCheckERR(meta(stdscr, ch), "meta");
}

static PyObject *
PyCurses_KeyName(self,arg)
     PyObject * self;
     PyObject * arg;
{
  const char *knp;
  int ch;
  if (!PyArg_Parse(arg,"i",&ch))
    return NULL;
  knp = keyname(ch);
  return PyString_FromString((knp == NULL) ? "" : knp);
}

#ifdef NOT_YET
static PyObject * 
PyCurses_NewTerm(self, args)
     PyObject * self;
     PyObject * args;
{
}

static PyObject * 
PyCurses_SetTerm(self, args)
     PyObject * self;
     PyObject * args;
{
}
#endif

/* List of functions defined in the module */

static PyMethodDef PyCurses_methods[] = {
	{"initscr",             (PyCFunction)PyCurses_InitScr},
	{"endwin",              (PyCFunction)PyCurses_EndWin},
	{"isendwin",            (PyCFunction)PyCurses_IsEndWin},
	{"doupdate",            (PyCFunction)PyCurses_DoUpdate},
	{"newwin",              (PyCFunction)PyCurses_NewWindow},
	{"beep",                (PyCFunction)PyCurses_Beep},
	{"flash",               (PyCFunction)PyCurses_Flash},
	{"ungetch",             (PyCFunction)PyCurses_UngetCh},
	{"flushinp",            (PyCFunction)PyCurses_FlushInp},
	{"cbreak",              (PyCFunction)PyCurses_CBreak},
	{"nocbreak",            (PyCFunction)PyCurses_NoCBreak},
	{"echo",                (PyCFunction)PyCurses_Echo},
	{"noecho",              (PyCFunction)PyCurses_NoEcho},
	{"nl",                  (PyCFunction)PyCurses_Nl},
	{"nonl",                (PyCFunction)PyCurses_NoNl},
	{"raw",                 (PyCFunction)PyCurses_Raw},
	{"noraw",               (PyCFunction)PyCurses_NoRaw},
	{"intrflush",           (PyCFunction)PyCurses_IntrFlush},
	{"meta",                (PyCFunction)PyCurses_Meta},
	{"keyname",             (PyCFunction)PyCurses_KeyName},
#ifdef NOT_YET
	{"newterm",             (PyCFunction)PyCurses_NewTerm},
	{"set_term",            (PyCFunction)PyCurses_SetTerm},
#endif
	{NULL,		NULL}		/* sentinel */
};

/* Initialization function for the module */

DL_EXPORT(void)
initcurses()
{
	PyObject *m, *d, *v;

	/* Create the module and add the functions */
	m = Py_InitModule("curses", PyCurses_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ModDict = d; /* For PyCurses_InitScr */

	/* For exception curses.error */
	PyCursesError = PyErr_NewException("curses.error", NULL, NULL);
	PyDict_SetItemString(d, "error", PyCursesError);

	/* Make the version available */
	v = PyString_FromString(PyCursesVersion);
	PyDict_SetItemString(d, "version", v);
	PyDict_SetItemString(d, "__version__", v);
	Py_DECREF(v);

	/* Here are some attributes you can add to chars to print */
	SetDictInt("A_NORMAL",		A_NORMAL);
	SetDictInt("A_STANDOUT",	A_STANDOUT);
	SetDictInt("A_UNDERLINE",	A_UNDERLINE);
	SetDictInt("A_REVERSE",		A_REVERSE);
	SetDictInt("A_BLINK",		A_BLINK);
	SetDictInt("A_DIM",		A_DIM);
	SetDictInt("A_BOLD",		A_BOLD);
	SetDictInt("A_ALTCHARSET",	A_ALTCHARSET);

	/* Now set everything up for KEY_ variables */
	{
	  int key;
	  char *key_n;
	  char *key_n2;
	  for (key=KEY_MIN;key < KEY_MAX; key++) {
	    key_n = (char *)keyname(key);
	    if (key_n == NULL || strcmp(key_n,"UNKNOWN KEY")==0)
	      continue;
	    if (strncmp(key_n,"KEY_F(",6)==0) {
	      char *p1, *p2;
	      key_n2 = malloc(strlen(key_n)+1);
	      p1 = key_n;
	      p2 = key_n2;
	      while (*p1) {
		if (*p1 != '(' && *p1 != ')') {
		  *p2 = *p1;
		  p2++;
		}
		p1++;
	      }
	      *p2 = (char)0;
	    } else
	      key_n2 = key_n;
	    PyDict_SetItemString(d,key_n2,PyInt_FromLong((long) key));
	    if (key_n2 != key_n)
	      free(key_n2);
	  }
	  SetDictInt("KEY_MIN", KEY_MIN);
	  SetDictInt("KEY_MAX", KEY_MAX);
	}
}
