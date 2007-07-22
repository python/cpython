
/* ========================== Module _Menu ========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>


#ifdef USE_TOOLBOX_OBJECT_GLUE

extern PyObject *_MenuObj_New(MenuHandle);
extern int _MenuObj_Convert(PyObject *, MenuHandle *);

#define MenuObj_New _MenuObj_New
#define MenuObj_Convert _MenuObj_Convert
#endif

#define as_Menu(h) ((MenuHandle)h)
#define as_Resource(h) ((Handle)h)


/* Alternative version of MenuObj_New, which returns None for NULL argument */
PyObject *OptMenuObj_New(MenuRef itself)
{
        if (itself == NULL) {
                Py_INCREF(Py_None);
                return Py_None;
        }
        return MenuObj_New(itself);
}

/* Alternative version of MenuObj_Convert, which returns NULL for a None argument */
int OptMenuObj_Convert(PyObject *v, MenuRef *p_itself)
{
        if ( v == Py_None ) {
                *p_itself = NULL;
                return 1;
        }
        return MenuObj_Convert(v, p_itself);
}

static PyObject *Menu_Error;

/* ------------------------ Object type Menu ------------------------ */

PyTypeObject Menu_Type;

#define MenuObj_Check(x) (Py_Type(x) == &Menu_Type || PyObject_TypeCheck((x), &Menu_Type))

typedef struct MenuObject {
	PyObject_HEAD
	MenuHandle ob_itself;
} MenuObject;

PyObject *MenuObj_New(MenuHandle itself)
{
	MenuObject *it;
	it = PyObject_NEW(MenuObject, &Menu_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	return (PyObject *)it;
}

int MenuObj_Convert(PyObject *v, MenuHandle *p_itself)
{
	if (!MenuObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "Menu required");
		return 0;
	}
	*p_itself = ((MenuObject *)v)->ob_itself;
	return 1;
}

static void MenuObj_dealloc(MenuObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	Py_Type(self)->tp_free((PyObject *)self);
}

static PyObject *MenuObj_DisposeMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef DisposeMenu
	PyMac_PRECHECK(DisposeMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DisposeMenu(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_CalcMenuSize(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef CalcMenuSize
	PyMac_PRECHECK(CalcMenuSize);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	CalcMenuSize(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_CountMenuItems(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt16 _rv;
#ifndef CountMenuItems
	PyMac_PRECHECK(CountMenuItems);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = CountMenuItems(_self->ob_itself);
	_res = Py_BuildValue("H",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_GetMenuFont(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 outFontID;
	UInt16 outFontSize;
#ifndef GetMenuFont
	PyMac_PRECHECK(GetMenuFont);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMenuFont(_self->ob_itself,
	                   &outFontID,
	                   &outFontSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hH",
	                     outFontID,
	                     outFontSize);
	return _res;
}

static PyObject *MenuObj_SetMenuFont(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	SInt16 inFontID;
	UInt16 inFontSize;
#ifndef SetMenuFont
	PyMac_PRECHECK(SetMenuFont);
#endif
	if (!PyArg_ParseTuple(_args, "hH",
	                      &inFontID,
	                      &inFontSize))
		return NULL;
	_err = SetMenuFont(_self->ob_itself,
	                   inFontID,
	                   inFontSize);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuExcludesMarkColumn(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef GetMenuExcludesMarkColumn
	PyMac_PRECHECK(GetMenuExcludesMarkColumn);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMenuExcludesMarkColumn(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_SetMenuExcludesMarkColumn(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Boolean excludesMark;
#ifndef SetMenuExcludesMarkColumn
	PyMac_PRECHECK(SetMenuExcludesMarkColumn);
#endif
	if (!PyArg_ParseTuple(_args, "b",
	                      &excludesMark))
		return NULL;
	_err = SetMenuExcludesMarkColumn(_self->ob_itself,
	                                 excludesMark);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_IsValidMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsValidMenu
	PyMac_PRECHECK(IsValidMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsValidMenu(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_GetMenuRetainCount(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ItemCount _rv;
#ifndef GetMenuRetainCount
	PyMac_PRECHECK(GetMenuRetainCount);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMenuRetainCount(_self->ob_itself);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_RetainMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef RetainMenu
	PyMac_PRECHECK(RetainMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = RetainMenu(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_ReleaseMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef ReleaseMenu
	PyMac_PRECHECK(ReleaseMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = ReleaseMenu(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_DuplicateMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle outMenu;
#ifndef DuplicateMenu
	PyMac_PRECHECK(DuplicateMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = DuplicateMenu(_self->ob_itself,
	                     &outMenu);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, outMenu);
	return _res;
}

static PyObject *MenuObj_CopyMenuTitleAsCFString(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef outString;
#ifndef CopyMenuTitleAsCFString
	PyMac_PRECHECK(CopyMenuTitleAsCFString);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = CopyMenuTitleAsCFString(_self->ob_itself,
	                               &outString);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, outString);
	return _res;
}

static PyObject *MenuObj_SetMenuTitleWithCFString(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inString;
#ifndef SetMenuTitleWithCFString
	PyMac_PRECHECK(SetMenuTitleWithCFString);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      CFStringRefObj_Convert, &inString))
		return NULL;
	_err = SetMenuTitleWithCFString(_self->ob_itself,
	                                inString);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InvalidateMenuSize(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef InvalidateMenuSize
	PyMac_PRECHECK(InvalidateMenuSize);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = InvalidateMenuSize(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_IsMenuSizeInvalid(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsMenuSizeInvalid
	PyMac_PRECHECK(IsMenuSizeInvalid);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsMenuSizeInvalid(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_MacAppendMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 data;
#ifndef MacAppendMenu
	PyMac_PRECHECK(MacAppendMenu);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, data))
		return NULL;
	MacAppendMenu(_self->ob_itself,
	              data);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertResMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ResType theType;
	short afterItem;
#ifndef InsertResMenu
	PyMac_PRECHECK(InsertResMenu);
#endif
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

static PyObject *MenuObj_AppendResMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ResType theType;
#ifndef AppendResMenu
	PyMac_PRECHECK(AppendResMenu);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &theType))
		return NULL;
	AppendResMenu(_self->ob_itself,
	              theType);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_MacInsertMenuItem(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 itemString;
	short afterItem;
#ifndef MacInsertMenuItem
	PyMac_PRECHECK(MacInsertMenuItem);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetStr255, itemString,
	                      &afterItem))
		return NULL;
	MacInsertMenuItem(_self->ob_itself,
	                  itemString,
	                  afterItem);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_DeleteMenuItem(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
#ifndef DeleteMenuItem
	PyMac_PRECHECK(DeleteMenuItem);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	DeleteMenuItem(_self->ob_itself,
	               item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertFontResMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short afterItem;
	short scriptFilter;
#ifndef InsertFontResMenu
	PyMac_PRECHECK(InsertFontResMenu);
#endif
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

static PyObject *MenuObj_InsertIntlResMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ResType theType;
	short afterItem;
	short scriptFilter;
#ifndef InsertIntlResMenu
	PyMac_PRECHECK(InsertIntlResMenu);
#endif
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

static PyObject *MenuObj_AppendMenuItemText(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Str255 inString;
#ifndef AppendMenuItemText
	PyMac_PRECHECK(AppendMenuItemText);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, inString))
		return NULL;
	_err = AppendMenuItemText(_self->ob_itself,
	                          inString);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertMenuItemText(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	Str255 inString;
	MenuItemIndex afterItem;
#ifndef InsertMenuItemText
	PyMac_PRECHECK(InsertMenuItemText);
#endif
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetStr255, inString,
	                      &afterItem))
		return NULL;
	_err = InsertMenuItemText(_self->ob_itself,
	                          inString,
	                          afterItem);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_CopyMenuItems(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inFirstItem;
	ItemCount inNumItems;
	MenuHandle inDestMenu;
	MenuItemIndex inInsertAfter;
#ifndef CopyMenuItems
	PyMac_PRECHECK(CopyMenuItems);
#endif
	if (!PyArg_ParseTuple(_args, "hlO&h",
	                      &inFirstItem,
	                      &inNumItems,
	                      MenuObj_Convert, &inDestMenu,
	                      &inInsertAfter))
		return NULL;
	_err = CopyMenuItems(_self->ob_itself,
	                     inFirstItem,
	                     inNumItems,
	                     inDestMenu,
	                     inInsertAfter);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_DeleteMenuItems(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inFirstItem;
	ItemCount inNumItems;
#ifndef DeleteMenuItems
	PyMac_PRECHECK(DeleteMenuItems);
#endif
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inFirstItem,
	                      &inNumItems))
		return NULL;
	_err = DeleteMenuItems(_self->ob_itself,
	                       inFirstItem,
	                       inNumItems);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_AppendMenuItemTextWithCFString(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inString;
	MenuItemAttributes inAttributes;
	MenuCommand inCommandID;
	MenuItemIndex outNewItem;
#ifndef AppendMenuItemTextWithCFString
	PyMac_PRECHECK(AppendMenuItemTextWithCFString);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      CFStringRefObj_Convert, &inString,
	                      &inAttributes,
	                      &inCommandID))
		return NULL;
	_err = AppendMenuItemTextWithCFString(_self->ob_itself,
	                                      inString,
	                                      inAttributes,
	                                      inCommandID,
	                                      &outNewItem);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outNewItem);
	return _res;
}

static PyObject *MenuObj_InsertMenuItemTextWithCFString(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	CFStringRef inString;
	MenuItemIndex inAfterItem;
	MenuItemAttributes inAttributes;
	MenuCommand inCommandID;
#ifndef InsertMenuItemTextWithCFString
	PyMac_PRECHECK(InsertMenuItemTextWithCFString);
#endif
	if (!PyArg_ParseTuple(_args, "O&hll",
	                      CFStringRefObj_Convert, &inString,
	                      &inAfterItem,
	                      &inAttributes,
	                      &inCommandID))
		return NULL;
	_err = InsertMenuItemTextWithCFString(_self->ob_itself,
	                                      inString,
	                                      inAfterItem,
	                                      inAttributes,
	                                      inCommandID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_PopUpMenuSelect(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	short top;
	short left;
	short popUpItem;
#ifndef PopUpMenuSelect
	PyMac_PRECHECK(PopUpMenuSelect);
#endif
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

static PyObject *MenuObj_InvalidateMenuEnabling(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef InvalidateMenuEnabling
	PyMac_PRECHECK(InvalidateMenuEnabling);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = InvalidateMenuEnabling(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_IsMenuBarInvalid(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsMenuBarInvalid
	PyMac_PRECHECK(IsMenuBarInvalid);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsMenuBarInvalid(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_MacInsertMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuID beforeID;
#ifndef MacInsertMenu
	PyMac_PRECHECK(MacInsertMenu);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &beforeID))
		return NULL;
	MacInsertMenu(_self->ob_itself,
	              beforeID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_SetRootMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef SetRootMenu
	PyMac_PRECHECK(SetRootMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = SetRootMenu(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_MacCheckMenuItem(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	Boolean checked;
#ifndef MacCheckMenuItem
	PyMac_PRECHECK(MacCheckMenuItem);
#endif
	if (!PyArg_ParseTuple(_args, "hb",
	                      &item,
	                      &checked))
		return NULL;
	MacCheckMenuItem(_self->ob_itself,
	                 item,
	                 checked);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_SetMenuItemText(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	Str255 itemString;
#ifndef SetMenuItemText
	PyMac_PRECHECK(SetMenuItemText);
#endif
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

static PyObject *MenuObj_GetMenuItemText(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	Str255 itemString;
#ifndef GetMenuItemText
	PyMac_PRECHECK(GetMenuItemText);
#endif
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

static PyObject *MenuObj_SetItemMark(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	CharParameter markChar;
#ifndef SetItemMark
	PyMac_PRECHECK(SetItemMark);
#endif
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

static PyObject *MenuObj_GetItemMark(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	CharParameter markChar;
#ifndef GetItemMark
	PyMac_PRECHECK(GetItemMark);
#endif
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

static PyObject *MenuObj_SetItemCmd(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	CharParameter cmdChar;
#ifndef SetItemCmd
	PyMac_PRECHECK(SetItemCmd);
#endif
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

static PyObject *MenuObj_GetItemCmd(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	CharParameter cmdChar;
#ifndef GetItemCmd
	PyMac_PRECHECK(GetItemCmd);
#endif
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

static PyObject *MenuObj_SetItemIcon(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	short iconIndex;
#ifndef SetItemIcon
	PyMac_PRECHECK(SetItemIcon);
#endif
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

static PyObject *MenuObj_GetItemIcon(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	short iconIndex;
#ifndef GetItemIcon
	PyMac_PRECHECK(GetItemIcon);
#endif
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

static PyObject *MenuObj_SetItemStyle(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	StyleParameter chStyle;
#ifndef SetItemStyle
	PyMac_PRECHECK(SetItemStyle);
#endif
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

static PyObject *MenuObj_GetItemStyle(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	Style chStyle;
#ifndef GetItemStyle
	PyMac_PRECHECK(GetItemStyle);
#endif
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

static PyObject *MenuObj_SetMenuItemCommandID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	MenuCommand inCommandID;
#ifndef SetMenuItemCommandID
	PyMac_PRECHECK(SetMenuItemCommandID);
#endif
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inItem,
	                      &inCommandID))
		return NULL;
	_err = SetMenuItemCommandID(_self->ob_itself,
	                            inItem,
	                            inCommandID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemCommandID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	MenuCommand outCommandID;
#ifndef GetMenuItemCommandID
	PyMac_PRECHECK(GetMenuItemCommandID);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemCommandID(_self->ob_itself,
	                            inItem,
	                            &outCommandID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outCommandID);
	return _res;
}

static PyObject *MenuObj_SetMenuItemModifiers(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	UInt8 inModifiers;
#ifndef SetMenuItemModifiers
	PyMac_PRECHECK(SetMenuItemModifiers);
#endif
	if (!PyArg_ParseTuple(_args, "hb",
	                      &inItem,
	                      &inModifiers))
		return NULL;
	_err = SetMenuItemModifiers(_self->ob_itself,
	                            inItem,
	                            inModifiers);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemModifiers(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	UInt8 outModifiers;
#ifndef GetMenuItemModifiers
	PyMac_PRECHECK(GetMenuItemModifiers);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemModifiers(_self->ob_itself,
	                            inItem,
	                            &outModifiers);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("b",
	                     outModifiers);
	return _res;
}

static PyObject *MenuObj_SetMenuItemIconHandle(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	UInt8 inIconType;
	Handle inIconHandle;
#ifndef SetMenuItemIconHandle
	PyMac_PRECHECK(SetMenuItemIconHandle);
#endif
	if (!PyArg_ParseTuple(_args, "hbO&",
	                      &inItem,
	                      &inIconType,
	                      ResObj_Convert, &inIconHandle))
		return NULL;
	_err = SetMenuItemIconHandle(_self->ob_itself,
	                             inItem,
	                             inIconType,
	                             inIconHandle);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemIconHandle(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	UInt8 outIconType;
	Handle outIconHandle;
#ifndef GetMenuItemIconHandle
	PyMac_PRECHECK(GetMenuItemIconHandle);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemIconHandle(_self->ob_itself,
	                             inItem,
	                             &outIconType,
	                             &outIconHandle);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("bO&",
	                     outIconType,
	                     ResObj_New, outIconHandle);
	return _res;
}

static PyObject *MenuObj_SetMenuItemTextEncoding(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	TextEncoding inScriptID;
#ifndef SetMenuItemTextEncoding
	PyMac_PRECHECK(SetMenuItemTextEncoding);
#endif
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inItem,
	                      &inScriptID))
		return NULL;
	_err = SetMenuItemTextEncoding(_self->ob_itself,
	                               inItem,
	                               inScriptID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemTextEncoding(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	TextEncoding outScriptID;
#ifndef GetMenuItemTextEncoding
	PyMac_PRECHECK(GetMenuItemTextEncoding);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemTextEncoding(_self->ob_itself,
	                               inItem,
	                               &outScriptID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outScriptID);
	return _res;
}

static PyObject *MenuObj_SetMenuItemHierarchicalID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	MenuID inHierID;
#ifndef SetMenuItemHierarchicalID
	PyMac_PRECHECK(SetMenuItemHierarchicalID);
#endif
	if (!PyArg_ParseTuple(_args, "hh",
	                      &inItem,
	                      &inHierID))
		return NULL;
	_err = SetMenuItemHierarchicalID(_self->ob_itself,
	                                 inItem,
	                                 inHierID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemHierarchicalID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	MenuID outHierID;
#ifndef GetMenuItemHierarchicalID
	PyMac_PRECHECK(GetMenuItemHierarchicalID);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemHierarchicalID(_self->ob_itself,
	                                 inItem,
	                                 &outHierID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outHierID);
	return _res;
}

static PyObject *MenuObj_SetMenuItemFontID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	SInt16 inFontID;
#ifndef SetMenuItemFontID
	PyMac_PRECHECK(SetMenuItemFontID);
#endif
	if (!PyArg_ParseTuple(_args, "hh",
	                      &inItem,
	                      &inFontID))
		return NULL;
	_err = SetMenuItemFontID(_self->ob_itself,
	                         inItem,
	                         inFontID);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemFontID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	SInt16 outFontID;
#ifndef GetMenuItemFontID
	PyMac_PRECHECK(GetMenuItemFontID);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemFontID(_self->ob_itself,
	                         inItem,
	                         &outFontID);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outFontID);
	return _res;
}

static PyObject *MenuObj_SetMenuItemRefCon(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	UInt32 inRefCon;
#ifndef SetMenuItemRefCon
	PyMac_PRECHECK(SetMenuItemRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inItem,
	                      &inRefCon))
		return NULL;
	_err = SetMenuItemRefCon(_self->ob_itself,
	                         inItem,
	                         inRefCon);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemRefCon(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	UInt32 outRefCon;
#ifndef GetMenuItemRefCon
	PyMac_PRECHECK(GetMenuItemRefCon);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemRefCon(_self->ob_itself,
	                         inItem,
	                         &outRefCon);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outRefCon);
	return _res;
}

static PyObject *MenuObj_SetMenuItemKeyGlyph(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	SInt16 inGlyph;
#ifndef SetMenuItemKeyGlyph
	PyMac_PRECHECK(SetMenuItemKeyGlyph);
#endif
	if (!PyArg_ParseTuple(_args, "hh",
	                      &inItem,
	                      &inGlyph))
		return NULL;
	_err = SetMenuItemKeyGlyph(_self->ob_itself,
	                           inItem,
	                           inGlyph);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemKeyGlyph(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SInt16 inItem;
	SInt16 outGlyph;
#ifndef GetMenuItemKeyGlyph
	PyMac_PRECHECK(GetMenuItemKeyGlyph);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemKeyGlyph(_self->ob_itself,
	                           inItem,
	                           &outGlyph);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outGlyph);
	return _res;
}

static PyObject *MenuObj_MacEnableMenuItem(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuItemIndex item;
#ifndef MacEnableMenuItem
	PyMac_PRECHECK(MacEnableMenuItem);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	MacEnableMenuItem(_self->ob_itself,
	                  item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_DisableMenuItem(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuItemIndex item;
#ifndef DisableMenuItem
	PyMac_PRECHECK(DisableMenuItem);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	DisableMenuItem(_self->ob_itself,
	                item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_IsMenuItemEnabled(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	MenuItemIndex item;
#ifndef IsMenuItemEnabled
	PyMac_PRECHECK(IsMenuItemEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	_rv = IsMenuItemEnabled(_self->ob_itself,
	                        item);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_EnableMenuItemIcon(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuItemIndex item;
#ifndef EnableMenuItemIcon
	PyMac_PRECHECK(EnableMenuItemIcon);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	EnableMenuItemIcon(_self->ob_itself,
	                   item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_DisableMenuItemIcon(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuItemIndex item;
#ifndef DisableMenuItemIcon
	PyMac_PRECHECK(DisableMenuItemIcon);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	DisableMenuItemIcon(_self->ob_itself,
	                    item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_IsMenuItemIconEnabled(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	MenuItemIndex item;
#ifndef IsMenuItemIconEnabled
	PyMac_PRECHECK(IsMenuItemIconEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	_rv = IsMenuItemIconEnabled(_self->ob_itself,
	                            item);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_SetMenuItemHierarchicalMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inItem;
	MenuHandle inHierMenu;
#ifndef SetMenuItemHierarchicalMenu
	PyMac_PRECHECK(SetMenuItemHierarchicalMenu);
#endif
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &inItem,
	                      MenuObj_Convert, &inHierMenu))
		return NULL;
	_err = SetMenuItemHierarchicalMenu(_self->ob_itself,
	                                   inItem,
	                                   inHierMenu);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemHierarchicalMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inItem;
	MenuHandle outHierMenu;
#ifndef GetMenuItemHierarchicalMenu
	PyMac_PRECHECK(GetMenuItemHierarchicalMenu);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemHierarchicalMenu(_self->ob_itself,
	                                   inItem,
	                                   &outHierMenu);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     OptMenuObj_New, outHierMenu);
	return _res;
}

static PyObject *MenuObj_CopyMenuItemTextAsCFString(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inItem;
	CFStringRef outString;
#ifndef CopyMenuItemTextAsCFString
	PyMac_PRECHECK(CopyMenuItemTextAsCFString);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = CopyMenuItemTextAsCFString(_self->ob_itself,
	                                  inItem,
	                                  &outString);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     CFStringRefObj_New, outString);
	return _res;
}

static PyObject *MenuObj_SetMenuItemTextWithCFString(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inItem;
	CFStringRef inString;
#ifndef SetMenuItemTextWithCFString
	PyMac_PRECHECK(SetMenuItemTextWithCFString);
#endif
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &inItem,
	                      CFStringRefObj_Convert, &inString))
		return NULL;
	_err = SetMenuItemTextWithCFString(_self->ob_itself,
	                                   inItem,
	                                   inString);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemIndent(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inItem;
	UInt32 outIndent;
#ifndef GetMenuItemIndent
	PyMac_PRECHECK(GetMenuItemIndent);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_err = GetMenuItemIndent(_self->ob_itself,
	                         inItem,
	                         &outIndent);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outIndent);
	return _res;
}

static PyObject *MenuObj_SetMenuItemIndent(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inItem;
	UInt32 inIndent;
#ifndef SetMenuItemIndent
	PyMac_PRECHECK(SetMenuItemIndent);
#endif
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inItem,
	                      &inIndent))
		return NULL;
	_err = SetMenuItemIndent(_self->ob_itself,
	                         inItem,
	                         inIndent);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemCommandKey(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inItem;
	Boolean inGetVirtualKey;
	UInt16 outKey;
#ifndef GetMenuItemCommandKey
	PyMac_PRECHECK(GetMenuItemCommandKey);
#endif
	if (!PyArg_ParseTuple(_args, "hb",
	                      &inItem,
	                      &inGetVirtualKey))
		return NULL;
	_err = GetMenuItemCommandKey(_self->ob_itself,
	                             inItem,
	                             inGetVirtualKey,
	                             &outKey);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     outKey);
	return _res;
}

static PyObject *MenuObj_SetMenuItemCommandKey(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inItem;
	Boolean inSetVirtualKey;
	UInt16 inKey;
#ifndef SetMenuItemCommandKey
	PyMac_PRECHECK(SetMenuItemCommandKey);
#endif
	if (!PyArg_ParseTuple(_args, "hbH",
	                      &inItem,
	                      &inSetVirtualKey,
	                      &inKey))
		return NULL;
	_err = SetMenuItemCommandKey(_self->ob_itself,
	                             inItem,
	                             inSetVirtualKey,
	                             inKey);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemPropertyAttributes(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex item;
	OSType propertyCreator;
	OSType propertyTag;
	UInt32 attributes;
#ifndef GetMenuItemPropertyAttributes
	PyMac_PRECHECK(GetMenuItemPropertyAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "hO&O&",
	                      &item,
	                      PyMac_GetOSType, &propertyCreator,
	                      PyMac_GetOSType, &propertyTag))
		return NULL;
	_err = GetMenuItemPropertyAttributes(_self->ob_itself,
	                                     item,
	                                     propertyCreator,
	                                     propertyTag,
	                                     &attributes);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     attributes);
	return _res;
}

static PyObject *MenuObj_ChangeMenuItemPropertyAttributes(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex item;
	OSType propertyCreator;
	OSType propertyTag;
	UInt32 attributesToSet;
	UInt32 attributesToClear;
#ifndef ChangeMenuItemPropertyAttributes
	PyMac_PRECHECK(ChangeMenuItemPropertyAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "hO&O&ll",
	                      &item,
	                      PyMac_GetOSType, &propertyCreator,
	                      PyMac_GetOSType, &propertyTag,
	                      &attributesToSet,
	                      &attributesToClear))
		return NULL;
	_err = ChangeMenuItemPropertyAttributes(_self->ob_itself,
	                                        item,
	                                        propertyCreator,
	                                        propertyTag,
	                                        attributesToSet,
	                                        attributesToClear);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuAttributes(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuAttributes outAttributes;
#ifndef GetMenuAttributes
	PyMac_PRECHECK(GetMenuAttributes);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMenuAttributes(_self->ob_itself,
	                         &outAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outAttributes);
	return _res;
}

static PyObject *MenuObj_ChangeMenuAttributes(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuAttributes setTheseAttributes;
	MenuAttributes clearTheseAttributes;
#ifndef ChangeMenuAttributes
	PyMac_PRECHECK(ChangeMenuAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &setTheseAttributes,
	                      &clearTheseAttributes))
		return NULL;
	_err = ChangeMenuAttributes(_self->ob_itself,
	                            setTheseAttributes,
	                            clearTheseAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuItemAttributes(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex item;
	MenuItemAttributes outAttributes;
#ifndef GetMenuItemAttributes
	PyMac_PRECHECK(GetMenuItemAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	_err = GetMenuItemAttributes(_self->ob_itself,
	                             item,
	                             &outAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outAttributes);
	return _res;
}

static PyObject *MenuObj_ChangeMenuItemAttributes(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex item;
	MenuItemAttributes setTheseAttributes;
	MenuItemAttributes clearTheseAttributes;
#ifndef ChangeMenuItemAttributes
	PyMac_PRECHECK(ChangeMenuItemAttributes);
#endif
	if (!PyArg_ParseTuple(_args, "hll",
	                      &item,
	                      &setTheseAttributes,
	                      &clearTheseAttributes))
		return NULL;
	_err = ChangeMenuItemAttributes(_self->ob_itself,
	                                item,
	                                setTheseAttributes,
	                                clearTheseAttributes);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_DisableAllMenuItems(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef DisableAllMenuItems
	PyMac_PRECHECK(DisableAllMenuItems);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DisableAllMenuItems(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_EnableAllMenuItems(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef EnableAllMenuItems
	PyMac_PRECHECK(EnableAllMenuItems);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	EnableAllMenuItems(_self->ob_itself);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_MenuHasEnabledItems(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef MenuHasEnabledItems
	PyMac_PRECHECK(MenuHasEnabledItems);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MenuHasEnabledItems(_self->ob_itself);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_GetMenuType(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	UInt16 outType;
#ifndef GetMenuType
	PyMac_PRECHECK(GetMenuType);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetMenuType(_self->ob_itself,
	                   &outType);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("H",
	                     outType);
	return _res;
}

static PyObject *MenuObj_CountMenuItemsWithCommandID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ItemCount _rv;
	MenuCommand inCommandID;
#ifndef CountMenuItemsWithCommandID
	PyMac_PRECHECK(CountMenuItemsWithCommandID);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inCommandID))
		return NULL;
	_rv = CountMenuItemsWithCommandID(_self->ob_itself,
	                                  inCommandID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_GetIndMenuItemWithCommandID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuCommand inCommandID;
	UInt32 inItemIndex;
	MenuHandle outMenu;
	MenuItemIndex outIndex;
#ifndef GetIndMenuItemWithCommandID
	PyMac_PRECHECK(GetIndMenuItemWithCommandID);
#endif
	if (!PyArg_ParseTuple(_args, "ll",
	                      &inCommandID,
	                      &inItemIndex))
		return NULL;
	_err = GetIndMenuItemWithCommandID(_self->ob_itself,
	                                   inCommandID,
	                                   inItemIndex,
	                                   &outMenu,
	                                   &outIndex);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&h",
	                     MenuObj_New, outMenu,
	                     outIndex);
	return _res;
}

static PyObject *MenuObj_EnableMenuCommand(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuCommand inCommandID;
#ifndef EnableMenuCommand
	PyMac_PRECHECK(EnableMenuCommand);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inCommandID))
		return NULL;
	EnableMenuCommand(_self->ob_itself,
	                  inCommandID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_DisableMenuCommand(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuCommand inCommandID;
#ifndef DisableMenuCommand
	PyMac_PRECHECK(DisableMenuCommand);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inCommandID))
		return NULL;
	DisableMenuCommand(_self->ob_itself,
	                   inCommandID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_IsMenuCommandEnabled(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	MenuCommand inCommandID;
#ifndef IsMenuCommandEnabled
	PyMac_PRECHECK(IsMenuCommandEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inCommandID))
		return NULL;
	_rv = IsMenuCommandEnabled(_self->ob_itself,
	                           inCommandID);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_SetMenuCommandMark(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuCommand inCommandID;
	UniChar inMark;
#ifndef SetMenuCommandMark
	PyMac_PRECHECK(SetMenuCommandMark);
#endif
	if (!PyArg_ParseTuple(_args, "lh",
	                      &inCommandID,
	                      &inMark))
		return NULL;
	_err = SetMenuCommandMark(_self->ob_itself,
	                          inCommandID,
	                          inMark);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_GetMenuCommandMark(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuCommand inCommandID;
	UniChar outMark;
#ifndef GetMenuCommandMark
	PyMac_PRECHECK(GetMenuCommandMark);
#endif
	if (!PyArg_ParseTuple(_args, "l",
	                      &inCommandID))
		return NULL;
	_err = GetMenuCommandMark(_self->ob_itself,
	                          inCommandID,
	                          &outMark);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outMark);
	return _res;
}

static PyObject *MenuObj_GetMenuCommandPropertySize(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuCommand inCommandID;
	OSType inPropertyCreator;
	OSType inPropertyTag;
	ByteCount outSize;
#ifndef GetMenuCommandPropertySize
	PyMac_PRECHECK(GetMenuCommandPropertySize);
#endif
	if (!PyArg_ParseTuple(_args, "lO&O&",
	                      &inCommandID,
	                      PyMac_GetOSType, &inPropertyCreator,
	                      PyMac_GetOSType, &inPropertyTag))
		return NULL;
	_err = GetMenuCommandPropertySize(_self->ob_itself,
	                                  inCommandID,
	                                  inPropertyCreator,
	                                  inPropertyTag,
	                                  &outSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outSize);
	return _res;
}

static PyObject *MenuObj_RemoveMenuCommandProperty(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuCommand inCommandID;
	OSType inPropertyCreator;
	OSType inPropertyTag;
#ifndef RemoveMenuCommandProperty
	PyMac_PRECHECK(RemoveMenuCommandProperty);
#endif
	if (!PyArg_ParseTuple(_args, "lO&O&",
	                      &inCommandID,
	                      PyMac_GetOSType, &inPropertyCreator,
	                      PyMac_GetOSType, &inPropertyTag))
		return NULL;
	_err = RemoveMenuCommandProperty(_self->ob_itself,
	                                 inCommandID,
	                                 inPropertyCreator,
	                                 inPropertyTag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_IsMenuItemInvalid(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	MenuItemIndex inItem;
#ifndef IsMenuItemInvalid
	PyMac_PRECHECK(IsMenuItemInvalid);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &inItem))
		return NULL;
	_rv = IsMenuItemInvalid(_self->ob_itself,
	                        inItem);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_InvalidateMenuItems(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex inFirstItem;
	ItemCount inNumItems;
#ifndef InvalidateMenuItems
	PyMac_PRECHECK(InvalidateMenuItems);
#endif
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inFirstItem,
	                      &inNumItems))
		return NULL;
	_err = InvalidateMenuItems(_self->ob_itself,
	                           inFirstItem,
	                           inNumItems);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_UpdateInvalidMenuItems(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef UpdateInvalidMenuItems
	PyMac_PRECHECK(UpdateInvalidMenuItems);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = UpdateInvalidMenuItems(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_CreateStandardFontMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex afterItem;
	MenuID firstHierMenuID;
	OptionBits options;
	ItemCount outHierMenuCount;
#ifndef CreateStandardFontMenu
	PyMac_PRECHECK(CreateStandardFontMenu);
#endif
	if (!PyArg_ParseTuple(_args, "hhl",
	                      &afterItem,
	                      &firstHierMenuID,
	                      &options))
		return NULL;
	_err = CreateStandardFontMenu(_self->ob_itself,
	                              afterItem,
	                              firstHierMenuID,
	                              options,
	                              &outHierMenuCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outHierMenuCount);
	return _res;
}

static PyObject *MenuObj_UpdateStandardFontMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	ItemCount outHierMenuCount;
#ifndef UpdateStandardFontMenu
	PyMac_PRECHECK(UpdateStandardFontMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = UpdateStandardFontMenu(_self->ob_itself,
	                              &outHierMenuCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outHierMenuCount);
	return _res;
}

static PyObject *MenuObj_GetFontFamilyFromMenuSelection(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuItemIndex item;
	FMFontFamily outFontFamily;
	FMFontStyle outStyle;
#ifndef GetFontFamilyFromMenuSelection
	PyMac_PRECHECK(GetFontFamilyFromMenuSelection);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &item))
		return NULL;
	_err = GetFontFamilyFromMenuSelection(_self->ob_itself,
	                                      item,
	                                      &outFontFamily,
	                                      &outStyle);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hh",
	                     outFontFamily,
	                     outStyle);
	return _res;
}

static PyObject *MenuObj_GetMenuID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuID _rv;
#ifndef GetMenuID
	PyMac_PRECHECK(GetMenuID);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMenuID(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_GetMenuWidth(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
#ifndef GetMenuWidth
	PyMac_PRECHECK(GetMenuWidth);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMenuWidth(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_GetMenuHeight(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
#ifndef GetMenuHeight
	PyMac_PRECHECK(GetMenuHeight);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMenuHeight(_self->ob_itself);
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *MenuObj_SetMenuID(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuID menuID;
#ifndef SetMenuID
	PyMac_PRECHECK(SetMenuID);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	SetMenuID(_self->ob_itself,
	          menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_SetMenuWidth(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 width;
#ifndef SetMenuWidth
	PyMac_PRECHECK(SetMenuWidth);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &width))
		return NULL;
	SetMenuWidth(_self->ob_itself,
	             width);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_SetMenuHeight(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 height;
#ifndef SetMenuHeight
	PyMac_PRECHECK(SetMenuHeight);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &height))
		return NULL;
	SetMenuHeight(_self->ob_itself,
	              height);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_as_Resource(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Handle _rv;
#ifndef as_Resource
	PyMac_PRECHECK(as_Resource);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = as_Resource(_self->ob_itself);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *MenuObj_AppendMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 data;
#ifndef AppendMenu
	PyMac_PRECHECK(AppendMenu);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetStr255, data))
		return NULL;
	AppendMenu(_self->ob_itself,
	           data);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertMenu(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short beforeID;
#ifndef InsertMenu
	PyMac_PRECHECK(InsertMenu);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &beforeID))
		return NULL;
	InsertMenu(_self->ob_itself,
	           beforeID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_InsertMenuItem(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Str255 itemString;
	short afterItem;
#ifndef InsertMenuItem
	PyMac_PRECHECK(InsertMenuItem);
#endif
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

static PyObject *MenuObj_EnableMenuItem(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt16 item;
#ifndef EnableMenuItem
	PyMac_PRECHECK(EnableMenuItem);
#endif
	if (!PyArg_ParseTuple(_args, "H",
	                      &item))
		return NULL;
	EnableMenuItem(_self->ob_itself,
	               item);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *MenuObj_CheckMenuItem(MenuObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short item;
	Boolean checked;
#ifndef CheckMenuItem
	PyMac_PRECHECK(CheckMenuItem);
#endif
	if (!PyArg_ParseTuple(_args, "hb",
	                      &item,
	                      &checked))
		return NULL;
	CheckMenuItem(_self->ob_itself,
	              item,
	              checked);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef MenuObj_methods[] = {
	{"DisposeMenu", (PyCFunction)MenuObj_DisposeMenu, 1,
	 PyDoc_STR("() -> None")},
	{"CalcMenuSize", (PyCFunction)MenuObj_CalcMenuSize, 1,
	 PyDoc_STR("() -> None")},
	{"CountMenuItems", (PyCFunction)MenuObj_CountMenuItems, 1,
	 PyDoc_STR("() -> (UInt16 _rv)")},
	{"GetMenuFont", (PyCFunction)MenuObj_GetMenuFont, 1,
	 PyDoc_STR("() -> (SInt16 outFontID, UInt16 outFontSize)")},
	{"SetMenuFont", (PyCFunction)MenuObj_SetMenuFont, 1,
	 PyDoc_STR("(SInt16 inFontID, UInt16 inFontSize) -> None")},
	{"GetMenuExcludesMarkColumn", (PyCFunction)MenuObj_GetMenuExcludesMarkColumn, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"SetMenuExcludesMarkColumn", (PyCFunction)MenuObj_SetMenuExcludesMarkColumn, 1,
	 PyDoc_STR("(Boolean excludesMark) -> None")},
	{"IsValidMenu", (PyCFunction)MenuObj_IsValidMenu, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"GetMenuRetainCount", (PyCFunction)MenuObj_GetMenuRetainCount, 1,
	 PyDoc_STR("() -> (ItemCount _rv)")},
	{"RetainMenu", (PyCFunction)MenuObj_RetainMenu, 1,
	 PyDoc_STR("() -> None")},
	{"ReleaseMenu", (PyCFunction)MenuObj_ReleaseMenu, 1,
	 PyDoc_STR("() -> None")},
	{"DuplicateMenu", (PyCFunction)MenuObj_DuplicateMenu, 1,
	 PyDoc_STR("() -> (MenuHandle outMenu)")},
	{"CopyMenuTitleAsCFString", (PyCFunction)MenuObj_CopyMenuTitleAsCFString, 1,
	 PyDoc_STR("() -> (CFStringRef outString)")},
	{"SetMenuTitleWithCFString", (PyCFunction)MenuObj_SetMenuTitleWithCFString, 1,
	 PyDoc_STR("(CFStringRef inString) -> None")},
	{"InvalidateMenuSize", (PyCFunction)MenuObj_InvalidateMenuSize, 1,
	 PyDoc_STR("() -> None")},
	{"IsMenuSizeInvalid", (PyCFunction)MenuObj_IsMenuSizeInvalid, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"MacAppendMenu", (PyCFunction)MenuObj_MacAppendMenu, 1,
	 PyDoc_STR("(Str255 data) -> None")},
	{"InsertResMenu", (PyCFunction)MenuObj_InsertResMenu, 1,
	 PyDoc_STR("(ResType theType, short afterItem) -> None")},
	{"AppendResMenu", (PyCFunction)MenuObj_AppendResMenu, 1,
	 PyDoc_STR("(ResType theType) -> None")},
	{"MacInsertMenuItem", (PyCFunction)MenuObj_MacInsertMenuItem, 1,
	 PyDoc_STR("(Str255 itemString, short afterItem) -> None")},
	{"DeleteMenuItem", (PyCFunction)MenuObj_DeleteMenuItem, 1,
	 PyDoc_STR("(short item) -> None")},
	{"InsertFontResMenu", (PyCFunction)MenuObj_InsertFontResMenu, 1,
	 PyDoc_STR("(short afterItem, short scriptFilter) -> None")},
	{"InsertIntlResMenu", (PyCFunction)MenuObj_InsertIntlResMenu, 1,
	 PyDoc_STR("(ResType theType, short afterItem, short scriptFilter) -> None")},
	{"AppendMenuItemText", (PyCFunction)MenuObj_AppendMenuItemText, 1,
	 PyDoc_STR("(Str255 inString) -> None")},
	{"InsertMenuItemText", (PyCFunction)MenuObj_InsertMenuItemText, 1,
	 PyDoc_STR("(Str255 inString, MenuItemIndex afterItem) -> None")},
	{"CopyMenuItems", (PyCFunction)MenuObj_CopyMenuItems, 1,
	 PyDoc_STR("(MenuItemIndex inFirstItem, ItemCount inNumItems, MenuHandle inDestMenu, MenuItemIndex inInsertAfter) -> None")},
	{"DeleteMenuItems", (PyCFunction)MenuObj_DeleteMenuItems, 1,
	 PyDoc_STR("(MenuItemIndex inFirstItem, ItemCount inNumItems) -> None")},
	{"AppendMenuItemTextWithCFString", (PyCFunction)MenuObj_AppendMenuItemTextWithCFString, 1,
	 PyDoc_STR("(CFStringRef inString, MenuItemAttributes inAttributes, MenuCommand inCommandID) -> (MenuItemIndex outNewItem)")},
	{"InsertMenuItemTextWithCFString", (PyCFunction)MenuObj_InsertMenuItemTextWithCFString, 1,
	 PyDoc_STR("(CFStringRef inString, MenuItemIndex inAfterItem, MenuItemAttributes inAttributes, MenuCommand inCommandID) -> None")},
	{"PopUpMenuSelect", (PyCFunction)MenuObj_PopUpMenuSelect, 1,
	 PyDoc_STR("(short top, short left, short popUpItem) -> (long _rv)")},
	{"InvalidateMenuEnabling", (PyCFunction)MenuObj_InvalidateMenuEnabling, 1,
	 PyDoc_STR("() -> None")},
	{"IsMenuBarInvalid", (PyCFunction)MenuObj_IsMenuBarInvalid, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"MacInsertMenu", (PyCFunction)MenuObj_MacInsertMenu, 1,
	 PyDoc_STR("(MenuID beforeID) -> None")},
	{"SetRootMenu", (PyCFunction)MenuObj_SetRootMenu, 1,
	 PyDoc_STR("() -> None")},
	{"MacCheckMenuItem", (PyCFunction)MenuObj_MacCheckMenuItem, 1,
	 PyDoc_STR("(short item, Boolean checked) -> None")},
	{"SetMenuItemText", (PyCFunction)MenuObj_SetMenuItemText, 1,
	 PyDoc_STR("(short item, Str255 itemString) -> None")},
	{"GetMenuItemText", (PyCFunction)MenuObj_GetMenuItemText, 1,
	 PyDoc_STR("(short item) -> (Str255 itemString)")},
	{"SetItemMark", (PyCFunction)MenuObj_SetItemMark, 1,
	 PyDoc_STR("(short item, CharParameter markChar) -> None")},
	{"GetItemMark", (PyCFunction)MenuObj_GetItemMark, 1,
	 PyDoc_STR("(short item) -> (CharParameter markChar)")},
	{"SetItemCmd", (PyCFunction)MenuObj_SetItemCmd, 1,
	 PyDoc_STR("(short item, CharParameter cmdChar) -> None")},
	{"GetItemCmd", (PyCFunction)MenuObj_GetItemCmd, 1,
	 PyDoc_STR("(short item) -> (CharParameter cmdChar)")},
	{"SetItemIcon", (PyCFunction)MenuObj_SetItemIcon, 1,
	 PyDoc_STR("(short item, short iconIndex) -> None")},
	{"GetItemIcon", (PyCFunction)MenuObj_GetItemIcon, 1,
	 PyDoc_STR("(short item) -> (short iconIndex)")},
	{"SetItemStyle", (PyCFunction)MenuObj_SetItemStyle, 1,
	 PyDoc_STR("(short item, StyleParameter chStyle) -> None")},
	{"GetItemStyle", (PyCFunction)MenuObj_GetItemStyle, 1,
	 PyDoc_STR("(short item) -> (Style chStyle)")},
	{"SetMenuItemCommandID", (PyCFunction)MenuObj_SetMenuItemCommandID, 1,
	 PyDoc_STR("(SInt16 inItem, MenuCommand inCommandID) -> None")},
	{"GetMenuItemCommandID", (PyCFunction)MenuObj_GetMenuItemCommandID, 1,
	 PyDoc_STR("(SInt16 inItem) -> (MenuCommand outCommandID)")},
	{"SetMenuItemModifiers", (PyCFunction)MenuObj_SetMenuItemModifiers, 1,
	 PyDoc_STR("(SInt16 inItem, UInt8 inModifiers) -> None")},
	{"GetMenuItemModifiers", (PyCFunction)MenuObj_GetMenuItemModifiers, 1,
	 PyDoc_STR("(SInt16 inItem) -> (UInt8 outModifiers)")},
	{"SetMenuItemIconHandle", (PyCFunction)MenuObj_SetMenuItemIconHandle, 1,
	 PyDoc_STR("(SInt16 inItem, UInt8 inIconType, Handle inIconHandle) -> None")},
	{"GetMenuItemIconHandle", (PyCFunction)MenuObj_GetMenuItemIconHandle, 1,
	 PyDoc_STR("(SInt16 inItem) -> (UInt8 outIconType, Handle outIconHandle)")},
	{"SetMenuItemTextEncoding", (PyCFunction)MenuObj_SetMenuItemTextEncoding, 1,
	 PyDoc_STR("(SInt16 inItem, TextEncoding inScriptID) -> None")},
	{"GetMenuItemTextEncoding", (PyCFunction)MenuObj_GetMenuItemTextEncoding, 1,
	 PyDoc_STR("(SInt16 inItem) -> (TextEncoding outScriptID)")},
	{"SetMenuItemHierarchicalID", (PyCFunction)MenuObj_SetMenuItemHierarchicalID, 1,
	 PyDoc_STR("(SInt16 inItem, MenuID inHierID) -> None")},
	{"GetMenuItemHierarchicalID", (PyCFunction)MenuObj_GetMenuItemHierarchicalID, 1,
	 PyDoc_STR("(SInt16 inItem) -> (MenuID outHierID)")},
	{"SetMenuItemFontID", (PyCFunction)MenuObj_SetMenuItemFontID, 1,
	 PyDoc_STR("(SInt16 inItem, SInt16 inFontID) -> None")},
	{"GetMenuItemFontID", (PyCFunction)MenuObj_GetMenuItemFontID, 1,
	 PyDoc_STR("(SInt16 inItem) -> (SInt16 outFontID)")},
	{"SetMenuItemRefCon", (PyCFunction)MenuObj_SetMenuItemRefCon, 1,
	 PyDoc_STR("(SInt16 inItem, UInt32 inRefCon) -> None")},
	{"GetMenuItemRefCon", (PyCFunction)MenuObj_GetMenuItemRefCon, 1,
	 PyDoc_STR("(SInt16 inItem) -> (UInt32 outRefCon)")},
	{"SetMenuItemKeyGlyph", (PyCFunction)MenuObj_SetMenuItemKeyGlyph, 1,
	 PyDoc_STR("(SInt16 inItem, SInt16 inGlyph) -> None")},
	{"GetMenuItemKeyGlyph", (PyCFunction)MenuObj_GetMenuItemKeyGlyph, 1,
	 PyDoc_STR("(SInt16 inItem) -> (SInt16 outGlyph)")},
	{"MacEnableMenuItem", (PyCFunction)MenuObj_MacEnableMenuItem, 1,
	 PyDoc_STR("(MenuItemIndex item) -> None")},
	{"DisableMenuItem", (PyCFunction)MenuObj_DisableMenuItem, 1,
	 PyDoc_STR("(MenuItemIndex item) -> None")},
	{"IsMenuItemEnabled", (PyCFunction)MenuObj_IsMenuItemEnabled, 1,
	 PyDoc_STR("(MenuItemIndex item) -> (Boolean _rv)")},
	{"EnableMenuItemIcon", (PyCFunction)MenuObj_EnableMenuItemIcon, 1,
	 PyDoc_STR("(MenuItemIndex item) -> None")},
	{"DisableMenuItemIcon", (PyCFunction)MenuObj_DisableMenuItemIcon, 1,
	 PyDoc_STR("(MenuItemIndex item) -> None")},
	{"IsMenuItemIconEnabled", (PyCFunction)MenuObj_IsMenuItemIconEnabled, 1,
	 PyDoc_STR("(MenuItemIndex item) -> (Boolean _rv)")},
	{"SetMenuItemHierarchicalMenu", (PyCFunction)MenuObj_SetMenuItemHierarchicalMenu, 1,
	 PyDoc_STR("(MenuItemIndex inItem, MenuHandle inHierMenu) -> None")},
	{"GetMenuItemHierarchicalMenu", (PyCFunction)MenuObj_GetMenuItemHierarchicalMenu, 1,
	 PyDoc_STR("(MenuItemIndex inItem) -> (MenuHandle outHierMenu)")},
	{"CopyMenuItemTextAsCFString", (PyCFunction)MenuObj_CopyMenuItemTextAsCFString, 1,
	 PyDoc_STR("(MenuItemIndex inItem) -> (CFStringRef outString)")},
	{"SetMenuItemTextWithCFString", (PyCFunction)MenuObj_SetMenuItemTextWithCFString, 1,
	 PyDoc_STR("(MenuItemIndex inItem, CFStringRef inString) -> None")},
	{"GetMenuItemIndent", (PyCFunction)MenuObj_GetMenuItemIndent, 1,
	 PyDoc_STR("(MenuItemIndex inItem) -> (UInt32 outIndent)")},
	{"SetMenuItemIndent", (PyCFunction)MenuObj_SetMenuItemIndent, 1,
	 PyDoc_STR("(MenuItemIndex inItem, UInt32 inIndent) -> None")},
	{"GetMenuItemCommandKey", (PyCFunction)MenuObj_GetMenuItemCommandKey, 1,
	 PyDoc_STR("(MenuItemIndex inItem, Boolean inGetVirtualKey) -> (UInt16 outKey)")},
	{"SetMenuItemCommandKey", (PyCFunction)MenuObj_SetMenuItemCommandKey, 1,
	 PyDoc_STR("(MenuItemIndex inItem, Boolean inSetVirtualKey, UInt16 inKey) -> None")},
	{"GetMenuItemPropertyAttributes", (PyCFunction)MenuObj_GetMenuItemPropertyAttributes, 1,
	 PyDoc_STR("(MenuItemIndex item, OSType propertyCreator, OSType propertyTag) -> (UInt32 attributes)")},
	{"ChangeMenuItemPropertyAttributes", (PyCFunction)MenuObj_ChangeMenuItemPropertyAttributes, 1,
	 PyDoc_STR("(MenuItemIndex item, OSType propertyCreator, OSType propertyTag, UInt32 attributesToSet, UInt32 attributesToClear) -> None")},
	{"GetMenuAttributes", (PyCFunction)MenuObj_GetMenuAttributes, 1,
	 PyDoc_STR("() -> (MenuAttributes outAttributes)")},
	{"ChangeMenuAttributes", (PyCFunction)MenuObj_ChangeMenuAttributes, 1,
	 PyDoc_STR("(MenuAttributes setTheseAttributes, MenuAttributes clearTheseAttributes) -> None")},
	{"GetMenuItemAttributes", (PyCFunction)MenuObj_GetMenuItemAttributes, 1,
	 PyDoc_STR("(MenuItemIndex item) -> (MenuItemAttributes outAttributes)")},
	{"ChangeMenuItemAttributes", (PyCFunction)MenuObj_ChangeMenuItemAttributes, 1,
	 PyDoc_STR("(MenuItemIndex item, MenuItemAttributes setTheseAttributes, MenuItemAttributes clearTheseAttributes) -> None")},
	{"DisableAllMenuItems", (PyCFunction)MenuObj_DisableAllMenuItems, 1,
	 PyDoc_STR("() -> None")},
	{"EnableAllMenuItems", (PyCFunction)MenuObj_EnableAllMenuItems, 1,
	 PyDoc_STR("() -> None")},
	{"MenuHasEnabledItems", (PyCFunction)MenuObj_MenuHasEnabledItems, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"GetMenuType", (PyCFunction)MenuObj_GetMenuType, 1,
	 PyDoc_STR("() -> (UInt16 outType)")},
	{"CountMenuItemsWithCommandID", (PyCFunction)MenuObj_CountMenuItemsWithCommandID, 1,
	 PyDoc_STR("(MenuCommand inCommandID) -> (ItemCount _rv)")},
	{"GetIndMenuItemWithCommandID", (PyCFunction)MenuObj_GetIndMenuItemWithCommandID, 1,
	 PyDoc_STR("(MenuCommand inCommandID, UInt32 inItemIndex) -> (MenuHandle outMenu, MenuItemIndex outIndex)")},
	{"EnableMenuCommand", (PyCFunction)MenuObj_EnableMenuCommand, 1,
	 PyDoc_STR("(MenuCommand inCommandID) -> None")},
	{"DisableMenuCommand", (PyCFunction)MenuObj_DisableMenuCommand, 1,
	 PyDoc_STR("(MenuCommand inCommandID) -> None")},
	{"IsMenuCommandEnabled", (PyCFunction)MenuObj_IsMenuCommandEnabled, 1,
	 PyDoc_STR("(MenuCommand inCommandID) -> (Boolean _rv)")},
	{"SetMenuCommandMark", (PyCFunction)MenuObj_SetMenuCommandMark, 1,
	 PyDoc_STR("(MenuCommand inCommandID, UniChar inMark) -> None")},
	{"GetMenuCommandMark", (PyCFunction)MenuObj_GetMenuCommandMark, 1,
	 PyDoc_STR("(MenuCommand inCommandID) -> (UniChar outMark)")},
	{"GetMenuCommandPropertySize", (PyCFunction)MenuObj_GetMenuCommandPropertySize, 1,
	 PyDoc_STR("(MenuCommand inCommandID, OSType inPropertyCreator, OSType inPropertyTag) -> (ByteCount outSize)")},
	{"RemoveMenuCommandProperty", (PyCFunction)MenuObj_RemoveMenuCommandProperty, 1,
	 PyDoc_STR("(MenuCommand inCommandID, OSType inPropertyCreator, OSType inPropertyTag) -> None")},
	{"IsMenuItemInvalid", (PyCFunction)MenuObj_IsMenuItemInvalid, 1,
	 PyDoc_STR("(MenuItemIndex inItem) -> (Boolean _rv)")},
	{"InvalidateMenuItems", (PyCFunction)MenuObj_InvalidateMenuItems, 1,
	 PyDoc_STR("(MenuItemIndex inFirstItem, ItemCount inNumItems) -> None")},
	{"UpdateInvalidMenuItems", (PyCFunction)MenuObj_UpdateInvalidMenuItems, 1,
	 PyDoc_STR("() -> None")},
	{"CreateStandardFontMenu", (PyCFunction)MenuObj_CreateStandardFontMenu, 1,
	 PyDoc_STR("(MenuItemIndex afterItem, MenuID firstHierMenuID, OptionBits options) -> (ItemCount outHierMenuCount)")},
	{"UpdateStandardFontMenu", (PyCFunction)MenuObj_UpdateStandardFontMenu, 1,
	 PyDoc_STR("() -> (ItemCount outHierMenuCount)")},
	{"GetFontFamilyFromMenuSelection", (PyCFunction)MenuObj_GetFontFamilyFromMenuSelection, 1,
	 PyDoc_STR("(MenuItemIndex item) -> (FMFontFamily outFontFamily, FMFontStyle outStyle)")},
	{"GetMenuID", (PyCFunction)MenuObj_GetMenuID, 1,
	 PyDoc_STR("() -> (MenuID _rv)")},
	{"GetMenuWidth", (PyCFunction)MenuObj_GetMenuWidth, 1,
	 PyDoc_STR("() -> (SInt16 _rv)")},
	{"GetMenuHeight", (PyCFunction)MenuObj_GetMenuHeight, 1,
	 PyDoc_STR("() -> (SInt16 _rv)")},
	{"SetMenuID", (PyCFunction)MenuObj_SetMenuID, 1,
	 PyDoc_STR("(MenuID menuID) -> None")},
	{"SetMenuWidth", (PyCFunction)MenuObj_SetMenuWidth, 1,
	 PyDoc_STR("(SInt16 width) -> None")},
	{"SetMenuHeight", (PyCFunction)MenuObj_SetMenuHeight, 1,
	 PyDoc_STR("(SInt16 height) -> None")},
	{"as_Resource", (PyCFunction)MenuObj_as_Resource, 1,
	 PyDoc_STR("() -> (Handle _rv)")},
	{"AppendMenu", (PyCFunction)MenuObj_AppendMenu, 1,
	 PyDoc_STR("(Str255 data) -> None")},
	{"InsertMenu", (PyCFunction)MenuObj_InsertMenu, 1,
	 PyDoc_STR("(short beforeID) -> None")},
	{"InsertMenuItem", (PyCFunction)MenuObj_InsertMenuItem, 1,
	 PyDoc_STR("(Str255 itemString, short afterItem) -> None")},
	{"EnableMenuItem", (PyCFunction)MenuObj_EnableMenuItem, 1,
	 PyDoc_STR("(UInt16 item) -> None")},
	{"CheckMenuItem", (PyCFunction)MenuObj_CheckMenuItem, 1,
	 PyDoc_STR("(short item, Boolean checked) -> None")},
	{NULL, NULL, 0}
};

#define MenuObj_getsetlist NULL


#define MenuObj_compare NULL

#define MenuObj_repr NULL

#define MenuObj_hash NULL
#define MenuObj_tp_init 0

#define MenuObj_tp_alloc PyType_GenericAlloc

static PyObject *MenuObj_tp_new(PyTypeObject *type, PyObject *_args, PyObject *_kwds)
{
	PyObject *_self;
	MenuHandle itself;
	char *kw[] = {"itself", 0};

	if (!PyArg_ParseTupleAndKeywords(_args, _kwds, "O&", kw, MenuObj_Convert, &itself)) return NULL;
	if ((_self = type->tp_alloc(type, 0)) == NULL) return NULL;
	((MenuObject *)_self)->ob_itself = itself;
	return _self;
}

#define MenuObj_tp_free PyObject_Del


PyTypeObject Menu_Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"_Menu.Menu", /*tp_name*/
	sizeof(MenuObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) MenuObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) MenuObj_compare, /*tp_compare*/
	(reprfunc) MenuObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) MenuObj_hash, /*tp_hash*/
	0, /*tp_call*/
	0, /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	PyObject_GenericSetAttr, /*tp_setattro */
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE, /* tp_flags */
	0, /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	MenuObj_methods, /* tp_methods */
	0, /*tp_members*/
	MenuObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	MenuObj_tp_init, /* tp_init */
	MenuObj_tp_alloc, /* tp_alloc */
	MenuObj_tp_new, /* tp_new */
	MenuObj_tp_free, /* tp_free */
};

/* ---------------------- End object type Menu ---------------------- */


static PyObject *Menu_NewMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	MenuID menuID;
	Str255 menuTitle;
#ifndef NewMenu
	PyMac_PRECHECK(NewMenu);
#endif
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

static PyObject *Menu_MacGetMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	short resourceID;
#ifndef MacGetMenu
	PyMac_PRECHECK(MacGetMenu);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &resourceID))
		return NULL;
	_rv = MacGetMenu(resourceID);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}

static PyObject *Menu_CreateNewMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuID inMenuID;
	MenuAttributes inMenuAttributes;
	MenuHandle outMenuRef;
#ifndef CreateNewMenu
	PyMac_PRECHECK(CreateNewMenu);
#endif
	if (!PyArg_ParseTuple(_args, "hl",
	                      &inMenuID,
	                      &inMenuAttributes))
		return NULL;
	_err = CreateNewMenu(inMenuID,
	                     inMenuAttributes,
	                     &outMenuRef);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, outMenuRef);
	return _res;
}

static PyObject *Menu_MenuKey(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	CharParameter ch;
#ifndef MenuKey
	PyMac_PRECHECK(MenuKey);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &ch))
		return NULL;
	_rv = MenuKey(ch);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Menu_MenuSelect(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
	Point startPt;
#ifndef MenuSelect
	PyMac_PRECHECK(MenuSelect);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetPoint, &startPt))
		return NULL;
	_rv = MenuSelect(startPt);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Menu_MenuChoice(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	long _rv;
#ifndef MenuChoice
	PyMac_PRECHECK(MenuChoice);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MenuChoice();
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Menu_MenuEvent(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	UInt32 _rv;
	EventRecord inEvent;
#ifndef MenuEvent
	PyMac_PRECHECK(MenuEvent);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &inEvent))
		return NULL;
	_rv = MenuEvent(&inEvent);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Menu_GetMBarHeight(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short _rv;
#ifndef GetMBarHeight
	PyMac_PRECHECK(GetMBarHeight);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMBarHeight();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Menu_MacDrawMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef MacDrawMenuBar
	PyMac_PRECHECK(MacDrawMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	MacDrawMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_InvalMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef InvalMenuBar
	PyMac_PRECHECK(InvalMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	InvalMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_HiliteMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuID menuID;
#ifndef HiliteMenu
	PyMac_PRECHECK(HiliteMenu);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	HiliteMenu(menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_GetNewMBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuBarHandle _rv;
	short menuBarID;
#ifndef GetNewMBar
	PyMac_PRECHECK(GetNewMBar);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuBarID))
		return NULL;
	_rv = GetNewMBar(menuBarID);
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Menu_GetMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuBarHandle _rv;
#ifndef GetMenuBar
	PyMac_PRECHECK(GetMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = GetMenuBar();
	_res = Py_BuildValue("O&",
	                     ResObj_New, _rv);
	return _res;
}

static PyObject *Menu_SetMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuBarHandle mbar;
#ifndef SetMenuBar
	PyMac_PRECHECK(SetMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &mbar))
		return NULL;
	SetMenuBar(mbar);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_DuplicateMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuBarHandle inMbar;
	MenuBarHandle outMbar;
#ifndef DuplicateMenuBar
	PyMac_PRECHECK(DuplicateMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &inMbar))
		return NULL;
	_err = DuplicateMenuBar(inMbar,
	                        &outMbar);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, outMbar);
	return _res;
}

static PyObject *Menu_DisposeMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuBarHandle inMbar;
#ifndef DisposeMenuBar
	PyMac_PRECHECK(DisposeMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &inMbar))
		return NULL;
	_err = DisposeMenuBar(inMbar);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_GetMenuHandle(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	MenuID menuID;
#ifndef GetMenuHandle
	PyMac_PRECHECK(GetMenuHandle);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	_rv = GetMenuHandle(menuID);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}

static PyObject *Menu_MacDeleteMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuID menuID;
#ifndef MacDeleteMenu
	PyMac_PRECHECK(MacDeleteMenu);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	MacDeleteMenu(menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_ClearMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ClearMenuBar
	PyMac_PRECHECK(ClearMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ClearMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_SetMenuFlashCount(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short count;
#ifndef SetMenuFlashCount
	PyMac_PRECHECK(SetMenuFlashCount);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &count))
		return NULL;
	SetMenuFlashCount(count);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_FlashMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuID menuID;
#ifndef FlashMenuBar
	PyMac_PRECHECK(FlashMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	FlashMenuBar(menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_IsMenuBarVisible(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
#ifndef IsMenuBarVisible
	PyMac_PRECHECK(IsMenuBarVisible);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = IsMenuBarVisible();
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Menu_ShowMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef ShowMenuBar
	PyMac_PRECHECK(ShowMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	ShowMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_HideMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef HideMenuBar
	PyMac_PRECHECK(HideMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	HideMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_AcquireRootMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle _rv;
#ifndef AcquireRootMenu
	PyMac_PRECHECK(AcquireRootMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = AcquireRootMenu();
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}

static PyObject *Menu_DeleteMCEntries(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuID menuID;
	short menuItem;
#ifndef DeleteMCEntries
	PyMac_PRECHECK(DeleteMCEntries);
#endif
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

static PyObject *Menu_InitContextualMenus(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
#ifndef InitContextualMenus
	PyMac_PRECHECK(InitContextualMenus);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = InitContextualMenus();
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_IsShowContextualMenuClick(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	EventRecord inEvent;
#ifndef IsShowContextualMenuClick
	PyMac_PRECHECK(IsShowContextualMenuClick);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetEventRecord, &inEvent))
		return NULL;
	_rv = IsShowContextualMenuClick(&inEvent);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Menu_LMGetTheMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	SInt16 _rv;
#ifndef LMGetTheMenu
	PyMac_PRECHECK(LMGetTheMenu);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = LMGetTheMenu();
	_res = Py_BuildValue("h",
	                     _rv);
	return _res;
}

static PyObject *Menu_as_Menu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	Handle h;
#ifndef as_Menu
	PyMac_PRECHECK(as_Menu);
#endif
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &h))
		return NULL;
	_rv = as_Menu(h);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}

static PyObject *Menu_GetMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle _rv;
	short resourceID;
#ifndef GetMenu
	PyMac_PRECHECK(GetMenu);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &resourceID))
		return NULL;
	_rv = GetMenu(resourceID);
	_res = Py_BuildValue("O&",
	                     MenuObj_New, _rv);
	return _res;
}

static PyObject *Menu_DeleteMenu(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short menuID;
#ifndef DeleteMenu
	PyMac_PRECHECK(DeleteMenu);
#endif
	if (!PyArg_ParseTuple(_args, "h",
	                      &menuID))
		return NULL;
	DeleteMenu(menuID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_DrawMenuBar(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
#ifndef DrawMenuBar
	PyMac_PRECHECK(DrawMenuBar);
#endif
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	DrawMenuBar();
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_CountMenuItemsWithCommandID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	ItemCount _rv;
	MenuHandle inMenu;
	MenuCommand inCommandID;
#ifndef CountMenuItemsWithCommandID
	PyMac_PRECHECK(CountMenuItemsWithCommandID);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID))
		return NULL;
	_rv = CountMenuItemsWithCommandID(inMenu,
	                                  inCommandID);
	_res = Py_BuildValue("l",
	                     _rv);
	return _res;
}

static PyObject *Menu_GetIndMenuItemWithCommandID(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle inMenu;
	MenuCommand inCommandID;
	UInt32 inItemIndex;
	MenuHandle outMenu;
	MenuItemIndex outIndex;
#ifndef GetIndMenuItemWithCommandID
	PyMac_PRECHECK(GetIndMenuItemWithCommandID);
#endif
	if (!PyArg_ParseTuple(_args, "O&ll",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID,
	                      &inItemIndex))
		return NULL;
	_err = GetIndMenuItemWithCommandID(inMenu,
	                                   inCommandID,
	                                   inItemIndex,
	                                   &outMenu,
	                                   &outIndex);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&h",
	                     MenuObj_New, outMenu,
	                     outIndex);
	return _res;
}

static PyObject *Menu_EnableMenuCommand(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle inMenu;
	MenuCommand inCommandID;
#ifndef EnableMenuCommand
	PyMac_PRECHECK(EnableMenuCommand);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID))
		return NULL;
	EnableMenuCommand(inMenu,
	                  inCommandID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_DisableMenuCommand(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	MenuHandle inMenu;
	MenuCommand inCommandID;
#ifndef DisableMenuCommand
	PyMac_PRECHECK(DisableMenuCommand);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID))
		return NULL;
	DisableMenuCommand(inMenu,
	                   inCommandID);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_IsMenuCommandEnabled(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	Boolean _rv;
	MenuHandle inMenu;
	MenuCommand inCommandID;
#ifndef IsMenuCommandEnabled
	PyMac_PRECHECK(IsMenuCommandEnabled);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID))
		return NULL;
	_rv = IsMenuCommandEnabled(inMenu,
	                           inCommandID);
	_res = Py_BuildValue("b",
	                     _rv);
	return _res;
}

static PyObject *Menu_SetMenuCommandMark(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle inMenu;
	MenuCommand inCommandID;
	UniChar inMark;
#ifndef SetMenuCommandMark
	PyMac_PRECHECK(SetMenuCommandMark);
#endif
	if (!PyArg_ParseTuple(_args, "O&lh",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID,
	                      &inMark))
		return NULL;
	_err = SetMenuCommandMark(inMenu,
	                          inCommandID,
	                          inMark);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Menu_GetMenuCommandMark(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle inMenu;
	MenuCommand inCommandID;
	UniChar outMark;
#ifndef GetMenuCommandMark
	PyMac_PRECHECK(GetMenuCommandMark);
#endif
	if (!PyArg_ParseTuple(_args, "O&l",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID))
		return NULL;
	_err = GetMenuCommandMark(inMenu,
	                          inCommandID,
	                          &outMark);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("h",
	                     outMark);
	return _res;
}

static PyObject *Menu_GetMenuCommandPropertySize(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle inMenu;
	MenuCommand inCommandID;
	OSType inPropertyCreator;
	OSType inPropertyTag;
	ByteCount outSize;
#ifndef GetMenuCommandPropertySize
	PyMac_PRECHECK(GetMenuCommandPropertySize);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&O&",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID,
	                      PyMac_GetOSType, &inPropertyCreator,
	                      PyMac_GetOSType, &inPropertyTag))
		return NULL;
	_err = GetMenuCommandPropertySize(inMenu,
	                                  inCommandID,
	                                  inPropertyCreator,
	                                  inPropertyTag,
	                                  &outSize);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     outSize);
	return _res;
}

static PyObject *Menu_RemoveMenuCommandProperty(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSStatus _err;
	MenuHandle inMenu;
	MenuCommand inCommandID;
	OSType inPropertyCreator;
	OSType inPropertyTag;
#ifndef RemoveMenuCommandProperty
	PyMac_PRECHECK(RemoveMenuCommandProperty);
#endif
	if (!PyArg_ParseTuple(_args, "O&lO&O&",
	                      OptMenuObj_Convert, &inMenu,
	                      &inCommandID,
	                      PyMac_GetOSType, &inPropertyCreator,
	                      PyMac_GetOSType, &inPropertyTag))
		return NULL;
	_err = RemoveMenuCommandProperty(inMenu,
	                                 inCommandID,
	                                 inPropertyCreator,
	                                 inPropertyTag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef Menu_methods[] = {
	{"NewMenu", (PyCFunction)Menu_NewMenu, 1,
	 PyDoc_STR("(MenuID menuID, Str255 menuTitle) -> (MenuHandle _rv)")},
	{"MacGetMenu", (PyCFunction)Menu_MacGetMenu, 1,
	 PyDoc_STR("(short resourceID) -> (MenuHandle _rv)")},
	{"CreateNewMenu", (PyCFunction)Menu_CreateNewMenu, 1,
	 PyDoc_STR("(MenuID inMenuID, MenuAttributes inMenuAttributes) -> (MenuHandle outMenuRef)")},
	{"MenuKey", (PyCFunction)Menu_MenuKey, 1,
	 PyDoc_STR("(CharParameter ch) -> (long _rv)")},
	{"MenuSelect", (PyCFunction)Menu_MenuSelect, 1,
	 PyDoc_STR("(Point startPt) -> (long _rv)")},
	{"MenuChoice", (PyCFunction)Menu_MenuChoice, 1,
	 PyDoc_STR("() -> (long _rv)")},
	{"MenuEvent", (PyCFunction)Menu_MenuEvent, 1,
	 PyDoc_STR("(EventRecord inEvent) -> (UInt32 _rv)")},
	{"GetMBarHeight", (PyCFunction)Menu_GetMBarHeight, 1,
	 PyDoc_STR("() -> (short _rv)")},
	{"MacDrawMenuBar", (PyCFunction)Menu_MacDrawMenuBar, 1,
	 PyDoc_STR("() -> None")},
	{"InvalMenuBar", (PyCFunction)Menu_InvalMenuBar, 1,
	 PyDoc_STR("() -> None")},
	{"HiliteMenu", (PyCFunction)Menu_HiliteMenu, 1,
	 PyDoc_STR("(MenuID menuID) -> None")},
	{"GetNewMBar", (PyCFunction)Menu_GetNewMBar, 1,
	 PyDoc_STR("(short menuBarID) -> (MenuBarHandle _rv)")},
	{"GetMenuBar", (PyCFunction)Menu_GetMenuBar, 1,
	 PyDoc_STR("() -> (MenuBarHandle _rv)")},
	{"SetMenuBar", (PyCFunction)Menu_SetMenuBar, 1,
	 PyDoc_STR("(MenuBarHandle mbar) -> None")},
	{"DuplicateMenuBar", (PyCFunction)Menu_DuplicateMenuBar, 1,
	 PyDoc_STR("(MenuBarHandle inMbar) -> (MenuBarHandle outMbar)")},
	{"DisposeMenuBar", (PyCFunction)Menu_DisposeMenuBar, 1,
	 PyDoc_STR("(MenuBarHandle inMbar) -> None")},
	{"GetMenuHandle", (PyCFunction)Menu_GetMenuHandle, 1,
	 PyDoc_STR("(MenuID menuID) -> (MenuHandle _rv)")},
	{"MacDeleteMenu", (PyCFunction)Menu_MacDeleteMenu, 1,
	 PyDoc_STR("(MenuID menuID) -> None")},
	{"ClearMenuBar", (PyCFunction)Menu_ClearMenuBar, 1,
	 PyDoc_STR("() -> None")},
	{"SetMenuFlashCount", (PyCFunction)Menu_SetMenuFlashCount, 1,
	 PyDoc_STR("(short count) -> None")},
	{"FlashMenuBar", (PyCFunction)Menu_FlashMenuBar, 1,
	 PyDoc_STR("(MenuID menuID) -> None")},
	{"IsMenuBarVisible", (PyCFunction)Menu_IsMenuBarVisible, 1,
	 PyDoc_STR("() -> (Boolean _rv)")},
	{"ShowMenuBar", (PyCFunction)Menu_ShowMenuBar, 1,
	 PyDoc_STR("() -> None")},
	{"HideMenuBar", (PyCFunction)Menu_HideMenuBar, 1,
	 PyDoc_STR("() -> None")},
	{"AcquireRootMenu", (PyCFunction)Menu_AcquireRootMenu, 1,
	 PyDoc_STR("() -> (MenuHandle _rv)")},
	{"DeleteMCEntries", (PyCFunction)Menu_DeleteMCEntries, 1,
	 PyDoc_STR("(MenuID menuID, short menuItem) -> None")},
	{"InitContextualMenus", (PyCFunction)Menu_InitContextualMenus, 1,
	 PyDoc_STR("() -> None")},
	{"IsShowContextualMenuClick", (PyCFunction)Menu_IsShowContextualMenuClick, 1,
	 PyDoc_STR("(EventRecord inEvent) -> (Boolean _rv)")},
	{"LMGetTheMenu", (PyCFunction)Menu_LMGetTheMenu, 1,
	 PyDoc_STR("() -> (SInt16 _rv)")},
	{"as_Menu", (PyCFunction)Menu_as_Menu, 1,
	 PyDoc_STR("(Handle h) -> (MenuHandle _rv)")},
	{"GetMenu", (PyCFunction)Menu_GetMenu, 1,
	 PyDoc_STR("(short resourceID) -> (MenuHandle _rv)")},
	{"DeleteMenu", (PyCFunction)Menu_DeleteMenu, 1,
	 PyDoc_STR("(short menuID) -> None")},
	{"DrawMenuBar", (PyCFunction)Menu_DrawMenuBar, 1,
	 PyDoc_STR("() -> None")},
	{"CountMenuItemsWithCommandID", (PyCFunction)Menu_CountMenuItemsWithCommandID, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID) -> (ItemCount _rv)")},
	{"GetIndMenuItemWithCommandID", (PyCFunction)Menu_GetIndMenuItemWithCommandID, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID, UInt32 inItemIndex) -> (MenuHandle outMenu, MenuItemIndex outIndex)")},
	{"EnableMenuCommand", (PyCFunction)Menu_EnableMenuCommand, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID) -> None")},
	{"DisableMenuCommand", (PyCFunction)Menu_DisableMenuCommand, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID) -> None")},
	{"IsMenuCommandEnabled", (PyCFunction)Menu_IsMenuCommandEnabled, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID) -> (Boolean _rv)")},
	{"SetMenuCommandMark", (PyCFunction)Menu_SetMenuCommandMark, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID, UniChar inMark) -> None")},
	{"GetMenuCommandMark", (PyCFunction)Menu_GetMenuCommandMark, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID) -> (UniChar outMark)")},
	{"GetMenuCommandPropertySize", (PyCFunction)Menu_GetMenuCommandPropertySize, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID, OSType inPropertyCreator, OSType inPropertyTag) -> (ByteCount outSize)")},
	{"RemoveMenuCommandProperty", (PyCFunction)Menu_RemoveMenuCommandProperty, 1,
	 PyDoc_STR("(MenuHandle inMenu, MenuCommand inCommandID, OSType inPropertyCreator, OSType inPropertyTag) -> None")},
	{NULL, NULL, 0}
};




void init_Menu(void)
{
	PyObject *m;
	PyObject *d;



	        PyMac_INIT_TOOLBOX_OBJECT_NEW(MenuHandle, MenuObj_New);
	        PyMac_INIT_TOOLBOX_OBJECT_CONVERT(MenuHandle, MenuObj_Convert);


	m = Py_InitModule("_Menu", Menu_methods);
	d = PyModule_GetDict(m);
	Menu_Error = PyMac_GetOSErrException();
	if (Menu_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Menu_Error) != 0)
		return;
	Py_Type(&Menu_Type) = &PyType_Type;
	if (PyType_Ready(&Menu_Type) < 0) return;
	Py_INCREF(&Menu_Type);
	PyModule_AddObject(m, "Menu", (PyObject *)&Menu_Type);
	/* Backward-compatible name */
	Py_INCREF(&Menu_Type);
	PyModule_AddObject(m, "MenuType", (PyObject *)&Menu_Type);
}

/* ======================== End module _Menu ======================== */

