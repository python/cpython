
/* ========================== Module Menu =========================== */

#include "Python.h"



#define SystemSevenOrLater 1

#include "macglue.h"
#include <Memory.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Controls.h>

extern PyObject *ResObj_New(Handle);
extern int ResObj_Convert(PyObject *, Handle *);

extern PyObject *WinObj_New(WindowPtr);
extern int WinObj_Convert(PyObject *, WindowPtr *);

extern PyObject *DlgObj_New(DialogPtr);
extern int DlgObj_Convert(PyObject *, DialogPtr *);
extern PyTypeObject Dialog_Type;
#define DlgObj_Check(x) ((x)->ob_type == &Dialog_Type)

extern PyObject *MenuObj_New(MenuHandle);
extern int MenuObj_Convert(PyObject *, MenuHandle *);

extern PyObject *CtlObj_New(ControlHandle);
extern int CtlObj_Convert(PyObject *, ControlHandle *);

extern PyObject *WinObj_WhichWindow(WindowPtr);

#include <Devices.h> /* Defines OpenDeskAcc in universal headers */
#include <Desk.h> /* Defines OpenDeskAcc in old headers */
#include <Menus.h>

#define resNotFound -192 /* Can't include <Errors.h> because of Python's "errors.h" */

static PyObject *Menu_Error;

/* ------------------------ Object type Menu ------------------------ */

PyTypeObject Menu_Type;

#define MenuObj_Check(x) ((x)->ob_type == &Menu_Type)

typedef struct MenuObject {
	PyObject_HEAD
	MenuHandle ob_itself;
} MenuObject;

PyObject *MenuObj_New(itself)
	MenuHandle itself;
{
	MenuObject *it;
	it = PyObject_NEW(MenuObject, &Menu_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}
MenuObj_Convert(v, p_itself)
	PyObject *v;
	MenuHandle *p_itself;
{
	if (!MenuObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Menu required");
		return 0;
	}
	*p_itself = ((MenuObject *)v)->ob_itself;
	return 1;
}

static void MenuObj_dealloc(self)
	MenuObject *self;
{
	/* Cleanup of self->ob_itself goes here */
	PyMem_DEL(self);
}

static PyObject *MenuObj_DisposeMenu(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DisposeMenu(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_AppendMenu(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 data;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, data))
		return NULL;
	AppendMenu(_self->ob_itself,
	           data);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_AppendResMenu(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ResType theType;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theType))
		return NULL;
	AppendResMenu(_self->ob_itself,
	              theType);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertResMenu(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ResType theType;
	short afterItem;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetOSType, &theType,
	                      &afterItem))
		return NULL;
	InsertResMenu(_self->ob_itself,
	              theType,
	              afterItem);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertMenu(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short beforeID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &beforeID))
		return NULL;
	InsertMenu(_self->ob_itself,
	           beforeID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertMenuItem(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 itemString;
	short afterItem;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetStr255, itemString,
	                      &afterItem))
		return NULL;
	InsertMenuItem(_self->ob_itself,
	               itemString,
	               afterItem);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_DeleteMenuItem(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	DeleteMenuItem(_self->ob_itself,
	               item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_SetMenuItemText(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	Str255 itemString;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &item,
	                      PyMac_GetStr255, itemString))
		return NULL;
	SetMenuItemText(_self->ob_itself,
	                item,
	                itemString);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemText(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	Str255 itemString;
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	GetMenuItemText(_self->ob_itself,
	                item,
	                itemString);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, itemString);
	return _res;
}

static PyObject *MenuObj_DisableItem(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	DisableItem(_self->ob_itself,
	            item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_EnableItem(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	EnableItem(_self->ob_itself,
	           item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_CheckItem(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	Boolean checked;
	if (!PyArg_ParseTuple(_args, "hb",
	                      &item,
	                      &checked))
		return NULL;
	CheckItem(_self->ob_itself,
	          item,
	          checked);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_SetItemMark(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	short markChar;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &item,
	                      &markChar))
		return NULL;
	SetItemMark(_self->ob_itself,
	            item,
	            markChar);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetItemMark(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	short markChar;
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	GetItemMark(_self->ob_itself,
	            item,
	            &markChar);
	_res = Py_BuildValue("h",
	                     markChar);
	return _res;
}

static PyObject *MenuObj_SetItemIcon(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	short iconIndex;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &item,
	                      &iconIndex))
		return NULL;
	SetItemIcon(_self->ob_itself,
	            item,
	            iconIndex);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetItemIcon(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	short iconIndex;
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	GetItemIcon(_self->ob_itself,
	            item,
	            &iconIndex);
	_res = Py_BuildValue("h",
	                     iconIndex);
	return _res;
}

static PyObject *MenuObj_SetItemStyle(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	short chStyle;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &item,
	                      &chStyle))
		return NULL;
	SetItemStyle(_self->ob_itself,
	             item,
	             chStyle);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetItemStyle(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	unsigned char chStyle;
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	GetItemStyle(_self->ob_itself,
	             item,
	             &chStyle);
	_res = Py_BuildValue("b",
	                     chStyle);
	return _res;
}

static PyObject *MenuObj_CalcMenuSize(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CalcMenuSize(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_CountMItems(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CountMItems(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_GetItemCmd(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	short cmdChar;
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	GetItemCmd(_self->ob_itself,
	           item,
	           &cmdChar);
	_res = Py_BuildValue("h",
	                     cmdChar);
	return _res;
}

static PyObject *MenuObj_SetItemCmd(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short item;
	short cmdChar;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &item,
	                      &cmdChar))
		return NULL;
	SetItemCmd(_self->ob_itself,
	           item,
	           cmdChar);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_PopUpMenuSelect(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	short top;
	short left;
	short popUpItem;
	if (!PyArg_ParseTuple(_args, "hhh",
	                      &top,
	                      &left,
	                      &popUpItem))
		return NULL;
	_rv = PopUpMenuSelect(_self->ob_itself,
	                      top,
	                      left,
	                      popUpItem);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_InsertFontResMenu(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short afterItem;
	short scriptFilter;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &afterItem,
	                      &scriptFilter))
		return NULL;
	InsertFontResMenu(_self->ob_itself,
	                  afterItem,
	                  scriptFilter);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertIntlResMenu(_self, _args)
	MenuObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	ResType theType;
	short afterItem;
	short scriptFilter;
	if (!PyArg_ParseTuple(_args, "O&hh",
	                      PyMac_GetOSType, &theType,
	                      &afterItem,
	                      &scriptFilter))
		return NULL;
	InsertIntlResMenu(_self->ob_itself,
	                  theType,
	                  afterItem,
	                  scriptFilter);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef MenuObj_methods[] = {
	{"DisposeMenu", (PyCFunction)MenuObj_DisposeMenu, 1,
	 "() -> None"},
	{"AppendMenu", (PyCFunction)MenuObj_AppendMenu, 1,
	 "(Str255 data) -> None"},
	{"AppendResMenu", (PyCFunction)MenuObj_AppendResMenu, 1,
	 "(ResType theType) -> None"},
	{"InsertResMenu", (PyCFunction)MenuObj_InsertResMenu, 1,
	 "(ResType theType, short afterItem) -> None"},
	{"InsertMenu", (PyCFunction)MenuObj_InsertMenu, 1,
	 "(short beforeID) -> None"},
	{"InsertMenuItem", (PyCFunction)MenuObj_InsertMenuItem, 1,
	 "(Str255 itemString, short afterItem) -> None"},
	{"DeleteMenuItem", (PyCFunction)MenuObj_DeleteMenuItem, 1,
	 "(short item) -> None"},
	{"SetMenuItemText", (PyCFunction)MenuObj_SetMenuItemText, 1,
	 "(short item, Str255 itemString) -> None"},
	{"GetMenuItemText", (PyCFunction)MenuObj_GetMenuItemText, 1,
	 "(short item) -> (Str255 itemString)"},
	{"DisableItem", (PyCFunction)MenuObj_DisableItem, 1,
	 "(short item) -> None"},
	{"EnableItem", (PyCFunction)MenuObj_EnableItem, 1,
	 "(short item) -> None"},
	{"CheckItem", (PyCFunction)MenuObj_CheckItem, 1,
	 "(short item, Boolean checked) -> None"},
	{"SetItemMark", (PyCFunction)MenuObj_SetItemMark, 1,
	 "(short item, short markChar) -> None"},
	{"GetItemMark", (PyCFunction)MenuObj_GetItemMark, 1,
	 "(short item) -> (short markChar)"},
	{"SetItemIcon", (PyCFunction)MenuObj_SetItemIcon, 1,
	 "(short item, short iconIndex) -> None"},
	{"GetItemIcon", (PyCFunction)MenuObj_GetItemIcon, 1,
	 "(short item) -> (short iconIndex)"},
	{"SetItemStyle", (PyCFunction)MenuObj_SetItemStyle, 1,
	 "(short item, short chStyle) -> None"},
	{"GetItemStyle", (PyCFunction)MenuObj_GetItemStyle, 1,
	 "(short item) -> (unsigned char chStyle)"},
	{"CalcMenuSize", (PyCFunction)MenuObj_CalcMenuSize, 1,
	 "() -> None"},
	{"CountMItems", (PyCFunction)MenuObj_CountMItems, 1,
	 "() -> (short _rv)"},
	{"GetItemCmd", (PyCFunction)MenuObj_GetItemCmd, 1,
	 "(short item) -> (short cmdChar)"},
	{"SetItemCmd", (PyCFunction)MenuObj_SetItemCmd, 1,
	 "(short item, short cmdChar) -> None"},
	{"PopUpMenuSelect", (PyCFunction)MenuObj_PopUpMenuSelect, 1,
	 "(short top, short left, short popUpItem) -> (long _rv)"},
	{"InsertFontResMenu", (PyCFunction)MenuObj_InsertFontResMenu, 1,
	 "(short afterItem, short scriptFilter) -> None"},
	{"InsertIntlResMenu", (PyCFunction)MenuObj_InsertIntlResMenu, 1,
	 "(ResType theType, short afterItem, short scriptFilter) -> None"},
	{NULL, NULL, 0}
};

PyMethodChain MenuObj_chain = { MenuObj_methods, NULL };

static PyObject *MenuObj_getattr(self, name)
	MenuObject *self;
	char *name;
{
	return Py_FindMethodInChain(&MenuObj_chain, (PyObject *)self, name);
}

#define MenuObj_setattr NULL

PyTypeObject Menu_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"Menu", /*tp_name*/
	sizeof(MenuObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) MenuObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) MenuObj_getattr, /*tp_getattr*/
	(setattrfunc) MenuObj_setattr, /*tp_setattr*/
};

/* ---------------------- End object type Menu ---------------------- */


static PyObject *Menu_GetMBarHeight(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMBarHeight();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Menu_InitMenus(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	InitMenus();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_NewMenu(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	short menuID;
	Str255 menuTitle;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &menuID,
	                      PyMac_GetStr255, menuTitle))
		return NULL;
	_rv = NewMenu(menuID,
	              menuTitle);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}

static PyObject *Menu_GetMenu(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	short resourceID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &resourceID))
		return NULL;
	_rv = GetMenu(resourceID);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}

static PyObject *Menu_DrawMenuBar(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DrawMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_InvalMenuBar(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	InvalMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_DeleteMenu(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short menuID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	DeleteMenu(menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_ClearMenuBar(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClearMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_GetNewMBar(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	short menuBarID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuBarID))
		return NULL;
	_rv = GetNewMBar(menuBarID);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Menu_GetMenuBar(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMenuBar();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Menu_SetMenuBar(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Handle menuList;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &menuList))
		return NULL;
	SetMenuBar(menuList);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_MenuKey(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	short ch;
	if (!PyArg_ParseTuple(_args, "h",
	                      &ch))
		return NULL;
	_rv = MenuKey(ch);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Menu_HiliteMenu(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short menuID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	HiliteMenu(menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_GetMenuHandle(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	short menuID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	_rv = GetMenuHandle(menuID);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}

static PyObject *Menu_FlashMenuBar(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short menuID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	FlashMenuBar(menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_SetMenuFlash(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short count;
	if (!PyArg_ParseTuple(_args, "h",
	                      &count))
		return NULL;
	SetMenuFlash(count);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_MenuSelect(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	Point startPt;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &startPt))
		return NULL;
	_rv = MenuSelect(startPt);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Menu_InitProcMenu(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short resID;
	if (!PyArg_ParseTuple(_args, "h",
	                      &resID))
		return NULL;
	InitProcMenu(resID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_MenuChoice(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MenuChoice();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Menu_DeleteMCEntries(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short menuID;
	short menuItem;
	if (!PyArg_ParseTuple(_args, "hh",
	                      &menuID,
	                      &menuItem))
		return NULL;
	DeleteMCEntries(menuID,
	                menuItem);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_SystemEdit(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Boolean _rv;
	short editCmd;
	if (!PyArg_ParseTuple(_args, "h",
	                      &editCmd))
		return NULL;
	_rv = SystemEdit(editCmd);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Menu_SystemMenu(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	long menuResult;
	if (!PyArg_ParseTuple(_args, "l",
	                      &menuResult))
		return NULL;
	SystemMenu(menuResult);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_OpenDeskAcc(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	Str255 name;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, name))
		return NULL;
	OpenDeskAcc(name);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef Menu_methods[] = {
	{"GetMBarHeight", (PyCFunction)Menu_GetMBarHeight, 1,
	 "() -> (short _rv)"},
	{"InitMenus", (PyCFunction)Menu_InitMenus, 1,
	 "() -> None"},
	{"NewMenu", (PyCFunction)Menu_NewMenu, 1,
	 "(short menuID, Str255 menuTitle) -> (MenuHandle _rv)"},
	{"GetMenu", (PyCFunction)Menu_GetMenu, 1,
	 "(short resourceID) -> (MenuHandle _rv)"},
	{"DrawMenuBar", (PyCFunction)Menu_DrawMenuBar, 1,
	 "() -> None"},
	{"InvalMenuBar", (PyCFunction)Menu_InvalMenuBar, 1,
	 "() -> None"},
	{"DeleteMenu", (PyCFunction)Menu_DeleteMenu, 1,
	 "(short menuID) -> None"},
	{"ClearMenuBar", (PyCFunction)Menu_ClearMenuBar, 1,
	 "() -> None"},
	{"GetNewMBar", (PyCFunction)Menu_GetNewMBar, 1,
	 "(short menuBarID) -> (Handle _rv)"},
	{"GetMenuBar", (PyCFunction)Menu_GetMenuBar, 1,
	 "() -> (Handle _rv)"},
	{"SetMenuBar", (PyCFunction)Menu_SetMenuBar, 1,
	 "(Handle menuList) -> None"},
	{"MenuKey", (PyCFunction)Menu_MenuKey, 1,
	 "(short ch) -> (long _rv)"},
	{"HiliteMenu", (PyCFunction)Menu_HiliteMenu, 1,
	 "(short menuID) -> None"},
	{"GetMenuHandle", (PyCFunction)Menu_GetMenuHandle, 1,
	 "(short menuID) -> (MenuHandle _rv)"},
	{"FlashMenuBar", (PyCFunction)Menu_FlashMenuBar, 1,
	 "(short menuID) -> None"},
	{"SetMenuFlash", (PyCFunction)Menu_SetMenuFlash, 1,
	 "(short count) -> None"},
	{"MenuSelect", (PyCFunction)Menu_MenuSelect, 1,
	 "(Point startPt) -> (long _rv)"},
	{"InitProcMenu", (PyCFunction)Menu_InitProcMenu, 1,
	 "(short resID) -> None"},
	{"MenuChoice", (PyCFunction)Menu_MenuChoice, 1,
	 "() -> (long _rv)"},
	{"DeleteMCEntries", (PyCFunction)Menu_DeleteMCEntries, 1,
	 "(short menuID, short menuItem) -> None"},
	{"SystemEdit", (PyCFunction)Menu_SystemEdit, 1,
	 "(short editCmd) -> (Boolean _rv)"},
	{"SystemMenu", (PyCFunction)Menu_SystemMenu, 1,
	 "(long menuResult) -> None"},
	{"OpenDeskAcc", (PyCFunction)Menu_OpenDeskAcc, 1,
	 "(Str255 name) -> None"},
	{NULL, NULL, 0}
};




void initMenu()
{
	PyObject *m;
	PyObject *d;




	m = Py_InitModule("Menu", Menu_methods);
	d = PyModule_GetDict(m);
	Menu_Error = PyMac_GetOSErrException();
	if (Menu_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Menu_Error) != 0)
		Py_FatalError("can't initialize Menu.Error");
}

/* ======================== End module Menu ========================= */

