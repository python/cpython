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
StringObject      __version__          This returns a string representing
                                       the current version of this module
WindowObject      initscr()            This initializes the screen for use
None              endwin()             Closes down the screen and returns
                                       things as they were before calling
                                       initscr()
True/FalseObject  isendwin()           Has endwin() been called?
IntObject         doupdate()           Updates screen and returns number
                                       of bytes written to screen
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
IntObject         refresh()            Do refresh
IntObject         nooutrefresh()       Mark for refresh but wait
True/False        mvwin(new_y,new_x)   Move Window
True/False        move(new_y,new_x)    Move Cursor
WindowObject      subwin(nlines,ncols,begin_y,begin_x)
                  subwin(begin_y,begin_x)
True/False        addch(y,x,ch,attr)
                  addch(y,x,ch)
                  addch(ch,attr)
                  addch(ch)
True/False        insch(y,x,ch,attr)
                  insch(y,x,ch)
                  insch(ch,attr)
                  insch(ch)
True/False        delch(y,x)
                  delch()
True/False        echochar(ch,attr)
                  echochar(ch)
True/False        addstr(y,x,str,attr)
                  addstr(y,x,str)
                  addstr(str,attr)
                  addstr(str)
True/False        attron(attr)
True/False        attroff(attr)
True/False        attrset(sttr)
True/False        standend()
True/False        standout()
True/False        box(vertch,horch)    vertch and horch are INTS
                  box()
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
None              nodelay(int)      int=0 or int=1
None              notimeout(int)    int=0 or int=1
******************************************************************/


/* curses module */

#include "Python.h"

#include <curses.h>

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

staticforward PyTypeObject PyCursesScreen_Type;
staticforward PyTypeObject PyCursesWindow_Type;
staticforward PyTypeObject PyCursesPad_Type;

#define PyCursesScreen_Check(v)	 ((v)->ob_type == &PyCursesScreen_Type)
#define PyCursesWindow_Check(v)	 ((v)->ob_type == &PyCursesWindow_Type)
#define PyCursesPad_Check(v)	 ((v)->ob_type == &PyCursesPad_Type)

/* Defines */
PyObject *PyCurses_OK;
PyObject *PyCurses_ERR;

/******************************************************************

Change Log:

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
char *PyCursesVersion = "1.1";

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
	xp = (PyObject *)PyObject_NEW(PyCursesScreenObject, &PyCursesScreen_Type);
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
	wo = (PyCursesWindowObject *)PyObject_NEW(PyCursesWindowObject, &PyCursesWindow_Type);
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
    return (PyObject *)NULL;
  return (PyObject *)PyInt_FromLong(wrefresh(self->win));
}

static PyObject *
PyCursesWindow_NoOutRefresh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  return (PyObject *)PyInt_FromLong(wnoutrefresh(self->win));
}

static PyObject *
PyCursesWindow_MoveWin(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  if (!PyArg_Parse(arg,"(ii)", &y, &x))
    return (PyObject *)NULL;
  rtn = mvwin(self->win,y,x);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_Move(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  if (!PyArg_Parse(arg,"(ii)", &y, &x))
    return (PyObject *)NULL;
  rtn = wmove(self->win,y,x);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
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
  if (!PyArg_Parse(arg,
		   "(iiii);nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
    if (!PyArg_Parse(arg,"(ii)",&begin_y,&begin_x))
      return (PyObject *)NULL;
  win = subwin(self->win,nlines,ncols,begin_y,begin_x);
  if (win == NULL) {
    Py_INCREF(Py_None);
    return (PyObject *)Py_None;
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
  int attr;
  int attr_old;
  int use_xy = TRUE;
  int use_attr = TRUE;
  switch (PyTuple_Size(arg)) {
    case 2:
    case 4:
      use_attr = TRUE;
      break;
    default:
      use_attr = FALSE;
    }
  if (!PyArg_Parse(arg,"(iiii);y,x,ch,attr", &y, &x, &ch, &attr)) {
    if (!PyArg_Parse(arg,"(iii);y,x,ch", &y, &x, &ch)) {
      use_xy = FALSE;
      if (!PyArg_Parse(arg,"(ii);ch,attr", &ch, &attr))
	if (!PyArg_Parse(arg,"i;ch", &ch))
	  return (PyObject *)NULL;
    }
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
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_InsCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  int ch;
  int use_xy = TRUE;
  int attr, attr_old, use_attr = FALSE;
  switch (PyTuple_Size(arg)) {
    case 2:
    case 4:
      use_attr = TRUE;
      break;
    default:
      use_attr = FALSE;
    }
  if (!PyArg_Parse(arg,"(iiii);y,x,ch,attr", &y, &x, &ch, &attr)) {
    if (!PyArg_Parse(arg,"(iii);y,x,ch", &y, &x, &ch)) {
      use_xy = FALSE;
      if (!PyArg_Parse(arg,"(ii);ch,attr", &ch, &attr))
	if (!PyArg_Parse(arg,"i;ch", &ch))
	  return (PyObject *)NULL;
    }
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
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_DelCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  int use_xy = TRUE;
  if (!PyArg_Parse(arg,"(ii);y,x", &y, &x))
    use_xy = FALSE;
  PyErr_Clear();
  if (use_xy == TRUE)
    rtn = mvwdelch(self->win,y,x);
  else
    rtn = wdelch(self->win);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_EchoChar(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int ch;
  int attr, attr_old, use_attr = TRUE;
  if (!PyArg_Parse(arg,"(ii);ch,attr", &ch, &attr)) {
    use_attr = FALSE;
    if (!PyArg_Parse(arg,"i;ch", &ch))
      return (PyObject *)NULL;
  }
  if (use_attr == TRUE) {
    attr_old = getattrs(self->win);
    wattrset(self->win,attr);
  }
  rtn = wechochar(self->win,ch);
  if (use_attr == TRUE)
    wattrset(self->win,attr_old);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_AddStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int x, y;
  char *str;
  int use_xy = TRUE;
  int attr, attr_old, use_attr = TRUE;
    switch (PyTuple_Size(arg)) {
    case 2:
    case 4:
      use_attr = TRUE;
      break;
    default:
      use_attr = FALSE;
    }
  if (!PyArg_Parse(arg,"(iisi);y,x,str,attr", &y, &x, &str, &attr)) {
    if (!PyArg_Parse(arg,"(iis);y,x,str", &y, &x, &str)) {
      use_xy = FALSE;
      if (!PyArg_Parse(arg,"(si);str,attr", &str, &attr))
	if (!PyArg_Parse(arg,"s;str", &str))
	  return (PyObject *)NULL;
    }
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
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_AttrOn(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int ch;
  if (!PyArg_Parse(arg,"i;attr", &ch))
      return (PyObject *)NULL;
  rtn = wattron(self->win,ch);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_AttrOff(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int ch;
  if (!PyArg_Parse(arg,"i;attr", &ch))
      return (PyObject *)NULL;
  rtn = wattroff(self->win,ch);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_AttrSet(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int ch;
  if (!PyArg_Parse(arg,"i;attr", &ch))
      return (PyObject *)NULL;
  rtn = wattrset(self->win,ch);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_StandEnd(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  rtn = wstandend(self->win);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_StandOut(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  rtn = wstandout(self->win);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_Box(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int rtn;
  int ch1=0,ch2=0;
  if (!PyArg_Parse(arg,"(ii);vertch,horch", &ch1, &ch2))
    PyErr_Clear();
  rtn = box(self->win,ch1,ch2);
  if (rtn == OK) {
    Py_INCREF(PyCurses_OK);
    return (PyObject *)PyCurses_OK;
  }
  Py_INCREF(PyCurses_ERR);
  return (PyObject *)PyCurses_ERR;
}

static PyObject *
PyCursesWindow_Erase(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  werase(self->win);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_DeleteLine(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  wdeleteln(self->win);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_InsertLine(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  winsertln(self->win);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_GetYX(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  getyx(self->win,y,x);
  return (PyObject *)Py_BuildValue("(ii)", y, x);
}

static PyObject *
PyCursesWindow_GetBegYX(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  getbegyx(self->win,y,x);
  return (PyObject *)Py_BuildValue("(ii)", y, x);
}

static PyObject *
PyCursesWindow_GetMaxYX(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  getmaxyx(self->win,y,x);
  return (PyObject *)Py_BuildValue("(ii)", y, x);
}

static PyObject *
PyCursesWindow_Clear(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  wclear(self->win);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_ClearToBottom(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  wclrtobot(self->win);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_ClearToEOL(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  wclrtoeol(self->win);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_Scroll(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  scroll(self->win);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_TouchWin(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
      return (PyObject *)NULL;
  touchwin(self->win);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_TouchLine(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int st, cnt;
  if (!PyArg_Parse(arg,"(ii);start, count",&st,&cnt))
      return (PyObject *)NULL;
  touchline(self->win,st,cnt);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_GetCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  int use_xy = TRUE;
  int rtn;
  if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
    use_xy = FALSE;
  PyErr_Clear();
  if (use_xy == TRUE)
    rtn = mvwgetch(self->win,y,x);
  else
    rtn = wgetch(self->win);
  return (PyObject *)PyInt_FromLong(rtn);
}

static PyObject *
PyCursesWindow_GetStr(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  int use_xy = TRUE;
  char rtn[1024]; /* This should be big enough.. I hope */
  int rtn2;
  if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
    use_xy = FALSE;
  PyErr_Clear();
  if (use_xy == TRUE)
    rtn2 = mvwgetstr(self->win,y,x,rtn);
  else
    rtn2 = wgetstr(self->win,rtn);
  if (rtn2 == ERR)
    rtn[0] = 0;
  return (PyObject *)PyString_FromString(rtn);
}

static PyObject *
PyCursesWindow_InCh(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  int use_xy = TRUE;
  int rtn;
  if (!PyArg_Parse(arg,"(ii);y,x",&y,&x))
    use_xy = FALSE;
  PyErr_Clear();
  if (use_xy == TRUE)
    rtn = mvwinch(self->win,y,x);
  else
    rtn = winch(self->win);
  return (PyObject *)PyInt_FromLong(rtn);
}

static PyObject *
PyCursesWindow_ClearOk(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int val;
  if (!PyArg_Parse(arg,"i;True(1) or False(0)",&val))
    return (PyObject *)NULL;
  clearok(self->win,val);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_IdlOk(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int val;
  if (!PyArg_Parse(arg,"i;True(1) or False(0)",&val))
    return (PyObject *)NULL;
  idlok(self->win,val);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_LeaveOk(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int val;
  if (!PyArg_Parse(arg,"i;True(1) or False(0)",&val))
    return (PyObject *)NULL;
  leaveok(self->win,val);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_ScrollOk(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int val;
  if (!PyArg_Parse(arg,"i;True(1) or False(0)",&val))
    return (PyObject *)NULL;
  scrollok(self->win,val);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_SetScrollRegion(self,arg)
     PyCursesWindowObject *self;
     PyObject * arg;
{
  int x, y;
  if (!PyArg_Parse(arg,"(ii);top, bottom",&y,&x))
    return (PyObject *)NULL;
  wsetscrreg(self->win,y,x);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_KeyPad(self,arg)
     PyCursesWindowObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return (PyObject *)NULL;
  keypad(self->win,ch);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_NoDelay(self,arg)
     PyCursesWindowObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return (PyObject *)NULL;
  nodelay(self->win,ch);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCursesWindow_NoTimeout(self,arg)
     PyCursesWindowObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return (PyObject *)NULL;
  notimeout(self->win,ch);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyMethodDef PyCursesWindow_Methods[] = {
	{"refresh",             (PyCFunction)PyCursesWindow_Refresh},
	{"nooutrefresh",        (PyCFunction)PyCursesWindow_NoOutRefresh},
	{"mvwin",               (PyCFunction)PyCursesWindow_MoveWin},
	{"move",                (PyCFunction)PyCursesWindow_Move},
	{"subwin",              (PyCFunction)PyCursesWindow_SubWin},
	{"addch",               (PyCFunction)PyCursesWindow_AddCh},
	{"insch",               (PyCFunction)PyCursesWindow_InsCh},
	{"delch",               (PyCFunction)PyCursesWindow_DelCh},
	{"echochar",            (PyCFunction)PyCursesWindow_EchoChar},
	{"addstr",              (PyCFunction)PyCursesWindow_AddStr},
	{"attron",              (PyCFunction)PyCursesWindow_AttrOn},
	{"attroff",             (PyCFunction)PyCursesWindow_AttrOff},
	{"attrset",             (PyCFunction)PyCursesWindow_AttrSet},
	{"standend",            (PyCFunction)PyCursesWindow_StandEnd},
	{"standout",            (PyCFunction)PyCursesWindow_StandOut},
	{"box",                 (PyCFunction)PyCursesWindow_Box},
	{"erase",               (PyCFunction)PyCursesWindow_Erase},
	{"deleteln",            (PyCFunction)PyCursesWindow_DeleteLine},
	{"insertln",            (PyCFunction)PyCursesWindow_InsertLine},
	{"getyx",               (PyCFunction)PyCursesWindow_GetYX},
	{"getbegyx",            (PyCFunction)PyCursesWindow_GetBegYX},
	{"getmaxyx",            (PyCFunction)PyCursesWindow_GetMaxYX},
	{"clear",               (PyCFunction)PyCursesWindow_Clear},
	{"clrtobot",            (PyCFunction)PyCursesWindow_ClearToBottom},
	{"clrtoeol",            (PyCFunction)PyCursesWindow_ClearToEOL},
	{"scroll",              (PyCFunction)PyCursesWindow_Scroll},
	{"touchwin",            (PyCFunction)PyCursesWindow_TouchWin},
	{"touchline",           (PyCFunction)PyCursesWindow_TouchLine},
	{"getch",               (PyCFunction)PyCursesWindow_GetCh},
	{"getstr",              (PyCFunction)PyCursesWindow_GetStr},
	{"inch",                (PyCFunction)PyCursesWindow_InCh},
	{"clearok",             (PyCFunction)PyCursesWindow_ClearOk},
	{"idlok",               (PyCFunction)PyCursesWindow_IdlOk},
	{"leaveok",             (PyCFunction)PyCursesWindow_LeaveOk},
	{"scrollok",            (PyCFunction)PyCursesWindow_ScrollOk},
	{"setscrreg",           (PyCFunction)PyCursesWindow_SetScrollRegion},
	{"keypad",              (PyCFunction)PyCursesWindow_KeyPad},
	{"nodelay",             (PyCFunction)PyCursesWindow_NoDelay},
	{"notimeout",           (PyCFunction)PyCursesWindow_NoTimeout},
	{NULL,		        (PyCFunction)NULL}   /* sentinel */
};

static PyObject *
PyCursesWindow_GetAttr(self, name)
	PyCursesWindowObject *self;
	char *name;
{
  return Py_FindMethod(PyCursesWindow_Methods, (PyObject *)self, name);
}

/* --------------- PAD routines ---------------- */
static PyObject *
PyCursesPad_New(pad)
	WINDOW *pad;
{
	PyCursesPadObject *po;
	po = (PyCursesPadObject *)PyObject_NEW(PyCursesPadObject, &PyCursesPad_Type);
	if (po == NULL)
		return NULL;
	po->pad = pad;
	return (PyObject *)po;
}

/* -------------------------------------------------------*/
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

/* -------------------------------------------------------*/

static PyObject *ModDict;

static PyObject * 
PyCurses_InitScr(self, args)
     PyObject * self;
     PyObject * args;
{
  static int already_inited = FALSE;
  WINDOW *win;
  if (!PyArg_NoArgs(args))
    return (PyObject *)NULL;
  if (already_inited == TRUE) {
    wrefresh(stdscr);
    return (PyObject *)PyCursesWindow_New(stdscr);
  }
  already_inited = TRUE;

  win = initscr();

/* This was moved from initcurses() because core dumped on SGI */
#define SetDictChar(string,ch) \
	PyDict_SetItemString(ModDict,string,PyInt_FromLong(ch));
 
	/* Here are some graphic symbols you can use */
        SetDictChar("ACS_ULCORNER",(ACS_ULCORNER));
	SetDictChar("ACS_ULCORNER",(ACS_ULCORNER));
	SetDictChar("ACS_LLCORNER",(ACS_LLCORNER));
	SetDictChar("ACS_URCORNER",(ACS_URCORNER));
	SetDictChar("ACS_LRCORNER",(ACS_LRCORNER));
	SetDictChar("ACS_RTEE",    (ACS_RTEE));
	SetDictChar("ACS_LTEE",    (ACS_LTEE));
	SetDictChar("ACS_BTEE",    (ACS_BTEE));
	SetDictChar("ACS_TTEE",    (ACS_TTEE));
	SetDictChar("ACS_HLINE",   (ACS_HLINE));
	SetDictChar("ACS_VLINE",   (ACS_VLINE));
	SetDictChar("ACS_PLUS",    (ACS_PLUS));
	SetDictChar("ACS_S1",      (ACS_S1));
	SetDictChar("ACS_S9",      (ACS_S9));
	SetDictChar("ACS_DIAMOND", (ACS_DIAMOND));
	SetDictChar("ACS_CKBOARD", (ACS_CKBOARD));
	SetDictChar("ACS_DEGREE",  (ACS_DEGREE));
	SetDictChar("ACS_PLMINUS", (ACS_PLMINUS));
	SetDictChar("ACS_BULLET",  (ACS_BULLET));
	SetDictChar("ACS_LARROW",  (ACS_RARROW));
	SetDictChar("ACS_DARROW",  (ACS_DARROW));
	SetDictChar("ACS_UARROW",  (ACS_UARROW));
	SetDictChar("ACS_BOARD",   (ACS_BOARD));
	SetDictChar("ACS_LANTERN", (ACS_LANTERN));
	SetDictChar("ACS_BLOCK",   (ACS_BLOCK));

  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject * 
PyCurses_EndWin(self, args)
     PyObject * self;
     PyObject * args;
{
  if (!PyArg_NoArgs(args))
    return (PyObject *)NULL;
  endwin();
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject * 
PyCurses_IsEndWin(self, args)
     PyObject * self;
     PyObject * args;
{
  if (!PyArg_NoArgs(args))
    return (PyObject *)NULL;
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
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  return (PyObject *)PyInt_FromLong(doupdate());
}

static PyObject *
PyCurses_NewWindow(self,arg)
     PyObject * self;
     PyObject * arg;
{
  WINDOW *win;
  int nlines, ncols, begin_y, begin_x;
  nlines = 0;
  ncols  = 0;
  if (!PyArg_Parse(arg,
		   "(iiii);nlines,ncols,begin_y,begin_x",
		   &nlines,&ncols,&begin_y,&begin_x))
    if (!PyArg_Parse(arg,"(ii)",&begin_y,&begin_x))
      return (PyObject *)NULL;
  win = newwin(nlines,ncols,begin_y,begin_x);
  if (win == NULL) {
    Py_INCREF(Py_None);
    return (PyObject *)Py_None;
  }
  return (PyObject *)PyCursesWindow_New(win);
}

static PyObject *
PyCurses_Beep(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  beep();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_Flash(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  flash();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_UngetCh(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;integer",&ch))
    return (PyObject *)NULL;
  ungetch(ch);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_FlushInp(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  flushinp();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_CBreak(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  cbreak();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_NoCBreak(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  nocbreak();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_Echo(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  echo();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_NoEcho(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  noecho();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_Nl(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  nl();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_NoNl(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  nonl();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_Raw(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  raw();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_NoRaw(self,arg)
     PyObject * self;
     PyObject * arg;
{
  if (!PyArg_NoArgs(arg))
    return (PyObject *)NULL;
  noraw();
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_IntrFlush(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return (PyObject *)NULL;
  intrflush(NULL,ch);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_Meta(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i;True(1), False(0)",&ch))
    return (PyObject *)NULL;
  meta(NULL,ch);
  Py_INCREF(Py_None);
  return (PyObject *)Py_None;
}

static PyObject *
PyCurses_KeyName(self,arg)
     PyObject * self;
     PyObject * arg;
{
  int ch;
  if (!PyArg_Parse(arg,"i",&ch))
    return (PyObject *)NULL;
  return PyString_FromString((char *)keyname(ch));
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

void
initcurses()
{
	PyObject *m, *d, *x;

	/* Create the module and add the functions */
	m = Py_InitModule("curses", PyCurses_methods);

	PyCurses_OK  = Py_True;
	PyCurses_ERR = Py_False;
	Py_INCREF(PyCurses_OK);
	Py_INCREF(PyCurses_ERR);
	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ModDict = d; /* For PyCurses_InitScr */

	/* Make the version available */
	PyDict_SetItemString(d,"version",
			     PyString_FromString(PyCursesVersion));

	/* Here are some defines */
	PyDict_SetItemString(d,"OK", PyCurses_OK);
	PyDict_SetItemString(d,"ERR",PyCurses_ERR);

	/* Here are some attributes you can add to chars to print */
	PyDict_SetItemString(d, "A_NORMAL",    PyInt_FromLong(A_NORMAL));
	PyDict_SetItemString(d, "A_STANDOUT",  PyInt_FromLong(A_STANDOUT));
	PyDict_SetItemString(d, "A_UNDERLINE", PyInt_FromLong(A_UNDERLINE));
	PyDict_SetItemString(d, "A_REVERSE",   PyInt_FromLong(A_REVERSE));
	PyDict_SetItemString(d, "A_BLINK",     PyInt_FromLong(A_BLINK));
	PyDict_SetItemString(d, "A_DIM",       PyInt_FromLong(A_DIM));
	PyDict_SetItemString(d, "A_BOLD",      PyInt_FromLong(A_BOLD));
	PyDict_SetItemString(d, "A_ALTCHARSET",PyInt_FromLong(A_ALTCHARSET));

	/* Now set everything up for KEY_ variables */
	{
	  int key;
	  char *key_n;
	  char *key_n2;
	  for (key=KEY_MIN;key < KEY_MAX; key++) {
	    key_n = (char *)keyname(key);
	    if (strcmp(key_n,"UNKNOWN KEY")==0)
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
	    PyDict_SetItemString(d,key_n2,PyInt_FromLong(key));
	    if (key_n2 != key_n)
	      free(key_n2);
	  }
	SetDictChar("KEY_MIN",KEY_MIN);
	SetDictChar("KEY_MAX",KEY_MAX);
	}

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module syslog");
}
