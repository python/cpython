
/* ========================== Module _Snd =========================== */

#include "Python.h"



#include "pymactoolbox.h"

/* Macro to test whether a weak-loaded CFM function exists */
#define PyMac_PRECHECK(rtn) do { if ( &rtn == NULL )  {\
        PyErr_SetString(PyExc_NotImplementedError, \
        "Not available in this shared library/OS version"); \
        return NULL; \
    }} while(0)


#include <Carbon/Carbon.h>

/* Convert a SndCommand argument */
static int
SndCmd_Convert(PyObject *v, SndCommand *pc)
{
	int len;
	pc->param1 = 0;
	pc->param2 = 0;
	if (PyTuple_Check(v)) {
		if (PyArg_ParseTuple(v, "h|hl", &pc->cmd, &pc->param1, &pc->param2))
			return 1;
		PyErr_Clear();
		return PyArg_ParseTuple(v, "Hhs#", &pc->cmd, &pc->param1, &pc->param2, &len);
	}
	return PyArg_Parse(v, "H", &pc->cmd);
}

static pascal void SndCh_UserRoutine(SndChannelPtr chan, SndCommand *cmd); /* Forward */
static pascal void SPB_completion(SPBPtr my_spb); /* Forward */

static PyObject *Snd_Error;

/* --------------------- Object type SndChannel --------------------- */

static PyTypeObject SndChannel_Type;

#define SndCh_Check(x) ((x)->ob_type == &SndChannel_Type || PyObject_TypeCheck((x), &SndChannel_Type))

typedef struct SndChannelObject {
	PyObject_HEAD
	SndChannelPtr ob_itself;
	/* Members used to implement callbacks: */
	PyObject *ob_callback;
	long ob_A5;
	SndCommand ob_cmd;
} SndChannelObject;

static PyObject *SndCh_New(SndChannelPtr itself)
{
	SndChannelObject *it;
	it = PyObject_NEW(SndChannelObject, &SndChannel_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_callback = NULL;
	it->ob_A5 = SetCurrentA5();
	return (PyObject *)it;
}

static void SndCh_dealloc(SndChannelObject *self)
{
	SndDisposeChannel(self->ob_itself, 1);
	Py_XDECREF(self->ob_callback);
	PyObject_Free((PyObject *)self);
}

static PyObject *SndCh_SndDoCommand(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SndCommand cmd;
	Boolean noWait;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      SndCmd_Convert, &cmd,
	                      &noWait))
		return NULL;
	_err = SndDoCommand(_self->ob_itself,
	                    &cmd,
	                    noWait);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *SndCh_SndDoImmediate(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SndCommand cmd;
	if (!PyArg_ParseTuple(_args, "O&",
	                      SndCmd_Convert, &cmd))
		return NULL;
	_err = SndDoImmediate(_self->ob_itself,
	                      &cmd);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *SndCh_SndPlay(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SndListHandle sndHandle;
	Boolean async;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      ResObj_Convert, &sndHandle,
	                      &async))
		return NULL;
	_err = SndPlay(_self->ob_itself,
	               sndHandle,
	               async);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *SndCh_SndChannelStatus(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short theLength;
	SCStatus theStatus__out__;
	if (!PyArg_ParseTuple(_args, "h",
	                      &theLength))
		return NULL;
	_err = SndChannelStatus(_self->ob_itself,
	                        theLength,
	                        &theStatus__out__);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("s#",
	                     (char *)&theStatus__out__, (int)sizeof(SCStatus));
	return _res;
}

static PyObject *SndCh_SndGetInfo(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType selector;
	void * infoPtr;
	if (!PyArg_ParseTuple(_args, "O&w",
	                      PyMac_GetOSType, &selector,
	                      &infoPtr))
		return NULL;
	_err = SndGetInfo(_self->ob_itself,
	                  selector,
	                  infoPtr);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *SndCh_SndSetInfo(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType selector;
	void * infoPtr;
	if (!PyArg_ParseTuple(_args, "O&w",
	                      PyMac_GetOSType, &selector,
	                      &infoPtr))
		return NULL;
	_err = SndSetInfo(_self->ob_itself,
	                  selector,
	                  infoPtr);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyMethodDef SndCh_methods[] = {
	{"SndDoCommand", (PyCFunction)SndCh_SndDoCommand, 1,
	 PyDoc_STR("(SndCommand cmd, Boolean noWait) -> None")},
	{"SndDoImmediate", (PyCFunction)SndCh_SndDoImmediate, 1,
	 PyDoc_STR("(SndCommand cmd) -> None")},
	{"SndPlay", (PyCFunction)SndCh_SndPlay, 1,
	 PyDoc_STR("(SndListHandle sndHandle, Boolean async) -> None")},
	{"SndChannelStatus", (PyCFunction)SndCh_SndChannelStatus, 1,
	 PyDoc_STR("(short theLength) -> (SCStatus theStatus)")},
	{"SndGetInfo", (PyCFunction)SndCh_SndGetInfo, 1,
	 PyDoc_STR("(OSType selector, void * infoPtr) -> None")},
	{"SndSetInfo", (PyCFunction)SndCh_SndSetInfo, 1,
	 PyDoc_STR("(OSType selector, void * infoPtr) -> None")},
	{NULL, NULL, 0}
};

#define SndCh_getsetlist NULL


#define SndCh_compare NULL

#define SndCh_repr NULL

#define SndCh_hash NULL

static PyTypeObject SndChannel_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Snd.SndChannel", /*tp_name*/
	sizeof(SndChannelObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) SndCh_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) SndCh_compare, /*tp_compare*/
	(reprfunc) SndCh_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) SndCh_hash, /*tp_hash*/
	0, /*tp_call*/
	0, /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	PyObject_GenericSetAttr, /*tp_setattro */
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT, /* tp_flags */
	0, /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	SndCh_methods, /* tp_methods */
	0, /*tp_members*/
	SndCh_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	0, /*tp_init*/
	0, /*tp_alloc*/
	0, /*tp_new*/
	0, /*tp_free*/
};

/* ------------------- End object type SndChannel ------------------- */


/* ------------------------ Object type SPB ------------------------- */

static PyTypeObject SPB_Type;

#define SPBObj_Check(x) ((x)->ob_type == &SPB_Type || PyObject_TypeCheck((x), &SPB_Type))

typedef struct SPBObject {
	PyObject_HEAD
	/* Members used to implement callbacks: */
	PyObject *ob_completion;
	PyObject *ob_interrupt;
	PyObject *ob_thiscallback;
	long ob_A5;
	SPB ob_spb;
} SPBObject;

static PyObject *SPBObj_New(void)
{
	SPBObject *it;
	it = PyObject_NEW(SPBObject, &SPB_Type);
	if (it == NULL) return NULL;
	it->ob_completion = NULL;
	it->ob_interrupt = NULL;
	it->ob_thiscallback = NULL;
	it->ob_A5 = SetCurrentA5();
	memset((char *)&it->ob_spb, 0, sizeof(it->ob_spb));
	it->ob_spb.userLong = (long)it;
	return (PyObject *)it;
}
static int SPBObj_Convert(PyObject *v, SPBPtr *p_itself)
{
	if (!SPBObj_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "SPB required");
		return 0;
	}
	*p_itself = &((SPBObject *)v)->ob_spb;
	return 1;
}

static void SPBObj_dealloc(SPBObject *self)
{
	/* Cleanup of self->ob_itself goes here */
	self->ob_spb.userLong = 0;
	self->ob_thiscallback = 0;
	Py_XDECREF(self->ob_completion);
	Py_XDECREF(self->ob_interrupt);
	PyObject_Free((PyObject *)self);
}

static PyMethodDef SPBObj_methods[] = {
	{NULL, NULL, 0}
};

static PyObject *SPBObj_get_inRefNum(SPBObject *self, void *closure)
{
	return Py_BuildValue("l", self->ob_spb.inRefNum);
}

static int SPBObj_set_inRefNum(SPBObject *self, PyObject *v, void *closure)
{
	return -1 + PyArg_Parse(v, "l", &self->ob_spb.inRefNum);
	return 0;
}

static PyObject *SPBObj_get_count(SPBObject *self, void *closure)
{
	return Py_BuildValue("l", self->ob_spb.count);
}

static int SPBObj_set_count(SPBObject *self, PyObject *v, void *closure)
{
	return -1 + PyArg_Parse(v, "l", &self->ob_spb.count);
	return 0;
}

static PyObject *SPBObj_get_milliseconds(SPBObject *self, void *closure)
{
	return Py_BuildValue("l", self->ob_spb.milliseconds);
}

static int SPBObj_set_milliseconds(SPBObject *self, PyObject *v, void *closure)
{
	return -1 + PyArg_Parse(v, "l", &self->ob_spb.milliseconds);
	return 0;
}

static PyObject *SPBObj_get_error(SPBObject *self, void *closure)
{
	return Py_BuildValue("h", self->ob_spb.error);
}

#define SPBObj_set_error NULL

#define SPBObj_get_completionRoutine NULL

static int SPBObj_set_completionRoutine(SPBObject *self, PyObject *v, void *closure)
{
	self->ob_spb.completionRoutine = NewSICompletionUPP(SPB_completion);
			self->ob_completion = v;
			Py_INCREF(v);
			return 0;
	return 0;
}

static PyGetSetDef SPBObj_getsetlist[] = {
	{"inRefNum", (getter)SPBObj_get_inRefNum, (setter)SPBObj_set_inRefNum, NULL},
	{"count", (getter)SPBObj_get_count, (setter)SPBObj_set_count, NULL},
	{"milliseconds", (getter)SPBObj_get_milliseconds, (setter)SPBObj_set_milliseconds, NULL},
	{"error", (getter)SPBObj_get_error, (setter)SPBObj_set_error, NULL},
	{"completionRoutine", (getter)SPBObj_get_completionRoutine, (setter)SPBObj_set_completionRoutine, NULL},
	{NULL, NULL, NULL, NULL},
};


#define SPBObj_compare NULL

#define SPBObj_repr NULL

#define SPBObj_hash NULL

static PyTypeObject SPB_Type = {
	PyObject_HEAD_INIT(NULL)
	0, /*ob_size*/
	"_Snd.SPB", /*tp_name*/
	sizeof(SPBObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) SPBObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc)0, /*tp_getattr*/
	(setattrfunc)0, /*tp_setattr*/
	(cmpfunc) SPBObj_compare, /*tp_compare*/
	(reprfunc) SPBObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) SPBObj_hash, /*tp_hash*/
	0, /*tp_call*/
	0, /*tp_str*/
	PyObject_GenericGetAttr, /*tp_getattro*/
	PyObject_GenericSetAttr, /*tp_setattro */
	0, /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT, /* tp_flags */
	0, /*tp_doc*/
	0, /*tp_traverse*/
	0, /*tp_clear*/
	0, /*tp_richcompare*/
	0, /*tp_weaklistoffset*/
	0, /*tp_iter*/
	0, /*tp_iternext*/
	SPBObj_methods, /* tp_methods */
	0, /*tp_members*/
	SPBObj_getsetlist, /*tp_getset*/
	0, /*tp_base*/
	0, /*tp_dict*/
	0, /*tp_descr_get*/
	0, /*tp_descr_set*/
	0, /*tp_dictoffset*/
	0, /*tp_init*/
	0, /*tp_alloc*/
	0, /*tp_new*/
	0, /*tp_free*/
};

/* ---------------------- End object type SPB ----------------------- */


static PyObject *Snd_SPB(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	_res = SPBObj_New(); return _res;
}

static PyObject *Snd_SysBeep(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short duration;
	if (!PyArg_ParseTuple(_args, "h",
	                      &duration))
		return NULL;
	SysBeep(duration);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SndNewChannel(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SndChannelPtr chan = 0;
	short synth;
	long init;
	PyObject* userRoutine;
	if (!PyArg_ParseTuple(_args, "hlO",
	                      &synth,
	                      &init,
	                      &userRoutine))
		return NULL;
	if (userRoutine != Py_None && !PyCallable_Check(userRoutine))
	{
		PyErr_SetString(PyExc_TypeError, "callback must be callable");
		goto userRoutine__error__;
	}
	_err = SndNewChannel(&chan,
	                     synth,
	                     init,
	                     NewSndCallBackUPP(SndCh_UserRoutine));
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     SndCh_New, chan);
	if (_res != NULL && userRoutine != Py_None)
	{
		SndChannelObject *p = (SndChannelObject *)_res;
		p->ob_itself->userInfo = (long)p;
		Py_INCREF(userRoutine);
		p->ob_callback = userRoutine;
	}
 userRoutine__error__: ;
	return _res;
}

static PyObject *Snd_SndSoundManagerVersion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	NumVersion _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = SndSoundManagerVersion();
	_res = Py_BuildValue("O&",
	                     PyMac_BuildNumVersion, _rv);
	return _res;
}

static PyObject *Snd_SndManagerStatus(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short theLength;
	SMStatus theStatus__out__;
	if (!PyArg_ParseTuple(_args, "h",
	                      &theLength))
		return NULL;
	_err = SndManagerStatus(theLength,
	                        &theStatus__out__);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("s#",
	                     (char *)&theStatus__out__, (int)sizeof(SMStatus));
	return _res;
}

static PyObject *Snd_SndGetSysBeepState(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	short sysBeepState;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	SndGetSysBeepState(&sysBeepState);
	_res = Py_BuildValue("h",
	                     sysBeepState);
	return _res;
}

static PyObject *Snd_SndSetSysBeepState(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short sysBeepState;
	if (!PyArg_ParseTuple(_args, "h",
	                      &sysBeepState))
		return NULL;
	_err = SndSetSysBeepState(sysBeepState);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_GetSysBeepVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long level;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetSysBeepVolume(&level);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     level);
	return _res;
}

static PyObject *Snd_SetSysBeepVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long level;
	if (!PyArg_ParseTuple(_args, "l",
	                      &level))
		return NULL;
	_err = SetSysBeepVolume(level);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_GetDefaultOutputVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long level;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = GetDefaultOutputVolume(&level);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     level);
	return _res;
}

static PyObject *Snd_SetDefaultOutputVolume(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long level;
	if (!PyArg_ParseTuple(_args, "l",
	                      &level))
		return NULL;
	_err = SetDefaultOutputVolume(level);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_GetSoundHeaderOffset(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SndListHandle sndHandle;
	long offset;
	if (!PyArg_ParseTuple(_args, "O&",
	                      ResObj_Convert, &sndHandle))
		return NULL;
	_err = GetSoundHeaderOffset(sndHandle,
	                            &offset);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     offset);
	return _res;
}

static PyObject *Snd_GetCompressionInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short compressionID;
	OSType format;
	short numChannels;
	short sampleSize;
	CompressionInfo cp__out__;
	if (!PyArg_ParseTuple(_args, "hO&hh",
	                      &compressionID,
	                      PyMac_GetOSType, &format,
	                      &numChannels,
	                      &sampleSize))
		return NULL;
	_err = GetCompressionInfo(compressionID,
	                          format,
	                          numChannels,
	                          sampleSize,
	                          &cp__out__);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("s#",
	                     (char *)&cp__out__, (int)sizeof(CompressionInfo));
	return _res;
}

static PyObject *Snd_SetSoundPreference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType theType;
	Str255 name;
	Handle settings;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theType,
	                      ResObj_Convert, &settings))
		return NULL;
	_err = SetSoundPreference(theType,
	                          name,
	                          settings);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, name);
	return _res;
}

static PyObject *Snd_GetSoundPreference(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType theType;
	Str255 name;
	Handle settings;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetOSType, &theType,
	                      ResObj_Convert, &settings))
		return NULL;
	_err = GetSoundPreference(theType,
	                          name,
	                          settings);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, name);
	return _res;
}

static PyObject *Snd_GetCompressionName(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	OSType compressionType;
	Str255 compressionName;
	if (!PyArg_ParseTuple(_args, "O&",
	                      PyMac_GetOSType, &compressionType))
		return NULL;
	_err = GetCompressionName(compressionType,
	                          compressionName);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     PyMac_BuildStr255, compressionName);
	return _res;
}

static PyObject *Snd_SPBVersion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	NumVersion _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = SPBVersion();
	_res = Py_BuildValue("O&",
	                     PyMac_BuildNumVersion, _rv);
	return _res;
}

static PyObject *Snd_SndRecord(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Point corner;
	OSType quality;
	SndListHandle sndHandle;
	if (!PyArg_ParseTuple(_args, "O&O&",
	                      PyMac_GetPoint, &corner,
	                      PyMac_GetOSType, &quality))
		return NULL;
	_err = SndRecord((ModalFilterUPP)0,
	                 corner,
	                 quality,
	                 &sndHandle);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     ResObj_New, sndHandle);
	return _res;
}

static PyObject *Snd_SPBSignInDevice(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short deviceRefNum;
	Str255 deviceName;
	if (!PyArg_ParseTuple(_args, "hO&",
	                      &deviceRefNum,
	                      PyMac_GetStr255, deviceName))
		return NULL;
	_err = SPBSignInDevice(deviceRefNum,
	                       deviceName);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBSignOutDevice(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short deviceRefNum;
	if (!PyArg_ParseTuple(_args, "h",
	                      &deviceRefNum))
		return NULL;
	_err = SPBSignOutDevice(deviceRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBGetIndexedDevice(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short count;
	Str255 deviceName;
	Handle deviceIconHandle;
	if (!PyArg_ParseTuple(_args, "h",
	                      &count))
		return NULL;
	_err = SPBGetIndexedDevice(count,
	                           deviceName,
	                           &deviceIconHandle);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&O&",
	                     PyMac_BuildStr255, deviceName,
	                     ResObj_New, deviceIconHandle);
	return _res;
}

static PyObject *Snd_SPBOpenDevice(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Str255 deviceName;
	short permission;
	long inRefNum;
	if (!PyArg_ParseTuple(_args, "O&h",
	                      PyMac_GetStr255, deviceName,
	                      &permission))
		return NULL;
	_err = SPBOpenDevice(deviceName,
	                     permission,
	                     &inRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     inRefNum);
	return _res;
}

static PyObject *Snd_SPBCloseDevice(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inRefNum))
		return NULL;
	_err = SPBCloseDevice(inRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBRecord(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	SPBPtr inParamPtr;
	Boolean asynchFlag;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      SPBObj_Convert, &inParamPtr,
	                      &asynchFlag))
		return NULL;
	_err = SPBRecord(inParamPtr,
	                 asynchFlag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBPauseRecording(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inRefNum))
		return NULL;
	_err = SPBPauseRecording(inRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBResumeRecording(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inRefNum))
		return NULL;
	_err = SPBResumeRecording(inRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBStopRecording(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inRefNum))
		return NULL;
	_err = SPBStopRecording(inRefNum);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBGetRecordingStatus(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	short recordingStatus;
	short meterLevel;
	unsigned long totalSamplesToRecord;
	unsigned long numberOfSamplesRecorded;
	unsigned long totalMsecsToRecord;
	unsigned long numberOfMsecsRecorded;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inRefNum))
		return NULL;
	_err = SPBGetRecordingStatus(inRefNum,
	                             &recordingStatus,
	                             &meterLevel,
	                             &totalSamplesToRecord,
	                             &numberOfSamplesRecorded,
	                             &totalMsecsToRecord,
	                             &numberOfMsecsRecorded);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("hhllll",
	                     recordingStatus,
	                     meterLevel,
	                     totalSamplesToRecord,
	                     numberOfSamplesRecorded,
	                     totalMsecsToRecord,
	                     numberOfMsecsRecorded);
	return _res;
}

static PyObject *Snd_SPBGetDeviceInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	OSType infoType;
	void * infoData;
	if (!PyArg_ParseTuple(_args, "lO&w",
	                      &inRefNum,
	                      PyMac_GetOSType, &infoType,
	                      &infoData))
		return NULL;
	_err = SPBGetDeviceInfo(inRefNum,
	                        infoType,
	                        infoData);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBSetDeviceInfo(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	OSType infoType;
	void * infoData;
	if (!PyArg_ParseTuple(_args, "lO&w",
	                      &inRefNum,
	                      PyMac_GetOSType, &infoType,
	                      &infoData))
		return NULL;
	_err = SPBSetDeviceInfo(inRefNum,
	                        infoType,
	                        infoData);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_SPBMillisecondsToBytes(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	long milliseconds;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inRefNum))
		return NULL;
	_err = SPBMillisecondsToBytes(inRefNum,
	                              &milliseconds);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     milliseconds);
	return _res;
}

static PyObject *Snd_SPBBytesToMilliseconds(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	long inRefNum;
	long byteCount;
	if (!PyArg_ParseTuple(_args, "l",
	                      &inRefNum))
		return NULL;
	_err = SPBBytesToMilliseconds(inRefNum,
	                              &byteCount);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("l",
	                     byteCount);
	return _res;
}

static PyMethodDef Snd_methods[] = {
	{"SPB", (PyCFunction)Snd_SPB, 1,
	 PyDoc_STR(NULL)},
	{"SysBeep", (PyCFunction)Snd_SysBeep, 1,
	 PyDoc_STR("(short duration) -> None")},
	{"SndNewChannel", (PyCFunction)Snd_SndNewChannel, 1,
	 PyDoc_STR("(short synth, long init, PyObject* userRoutine) -> (SndChannelPtr chan)")},
	{"SndSoundManagerVersion", (PyCFunction)Snd_SndSoundManagerVersion, 1,
	 PyDoc_STR("() -> (NumVersion _rv)")},
	{"SndManagerStatus", (PyCFunction)Snd_SndManagerStatus, 1,
	 PyDoc_STR("(short theLength) -> (SMStatus theStatus)")},
	{"SndGetSysBeepState", (PyCFunction)Snd_SndGetSysBeepState, 1,
	 PyDoc_STR("() -> (short sysBeepState)")},
	{"SndSetSysBeepState", (PyCFunction)Snd_SndSetSysBeepState, 1,
	 PyDoc_STR("(short sysBeepState) -> None")},
	{"GetSysBeepVolume", (PyCFunction)Snd_GetSysBeepVolume, 1,
	 PyDoc_STR("() -> (long level)")},
	{"SetSysBeepVolume", (PyCFunction)Snd_SetSysBeepVolume, 1,
	 PyDoc_STR("(long level) -> None")},
	{"GetDefaultOutputVolume", (PyCFunction)Snd_GetDefaultOutputVolume, 1,
	 PyDoc_STR("() -> (long level)")},
	{"SetDefaultOutputVolume", (PyCFunction)Snd_SetDefaultOutputVolume, 1,
	 PyDoc_STR("(long level) -> None")},
	{"GetSoundHeaderOffset", (PyCFunction)Snd_GetSoundHeaderOffset, 1,
	 PyDoc_STR("(SndListHandle sndHandle) -> (long offset)")},
	{"GetCompressionInfo", (PyCFunction)Snd_GetCompressionInfo, 1,
	 PyDoc_STR("(short compressionID, OSType format, short numChannels, short sampleSize) -> (CompressionInfo cp)")},
	{"SetSoundPreference", (PyCFunction)Snd_SetSoundPreference, 1,
	 PyDoc_STR("(OSType theType, Handle settings) -> (Str255 name)")},
	{"GetSoundPreference", (PyCFunction)Snd_GetSoundPreference, 1,
	 PyDoc_STR("(OSType theType, Handle settings) -> (Str255 name)")},
	{"GetCompressionName", (PyCFunction)Snd_GetCompressionName, 1,
	 PyDoc_STR("(OSType compressionType) -> (Str255 compressionName)")},
	{"SPBVersion", (PyCFunction)Snd_SPBVersion, 1,
	 PyDoc_STR("() -> (NumVersion _rv)")},
	{"SndRecord", (PyCFunction)Snd_SndRecord, 1,
	 PyDoc_STR("(Point corner, OSType quality) -> (SndListHandle sndHandle)")},
	{"SPBSignInDevice", (PyCFunction)Snd_SPBSignInDevice, 1,
	 PyDoc_STR("(short deviceRefNum, Str255 deviceName) -> None")},
	{"SPBSignOutDevice", (PyCFunction)Snd_SPBSignOutDevice, 1,
	 PyDoc_STR("(short deviceRefNum) -> None")},
	{"SPBGetIndexedDevice", (PyCFunction)Snd_SPBGetIndexedDevice, 1,
	 PyDoc_STR("(short count) -> (Str255 deviceName, Handle deviceIconHandle)")},
	{"SPBOpenDevice", (PyCFunction)Snd_SPBOpenDevice, 1,
	 PyDoc_STR("(Str255 deviceName, short permission) -> (long inRefNum)")},
	{"SPBCloseDevice", (PyCFunction)Snd_SPBCloseDevice, 1,
	 PyDoc_STR("(long inRefNum) -> None")},
	{"SPBRecord", (PyCFunction)Snd_SPBRecord, 1,
	 PyDoc_STR("(SPBPtr inParamPtr, Boolean asynchFlag) -> None")},
	{"SPBPauseRecording", (PyCFunction)Snd_SPBPauseRecording, 1,
	 PyDoc_STR("(long inRefNum) -> None")},
	{"SPBResumeRecording", (PyCFunction)Snd_SPBResumeRecording, 1,
	 PyDoc_STR("(long inRefNum) -> None")},
	{"SPBStopRecording", (PyCFunction)Snd_SPBStopRecording, 1,
	 PyDoc_STR("(long inRefNum) -> None")},
	{"SPBGetRecordingStatus", (PyCFunction)Snd_SPBGetRecordingStatus, 1,
	 PyDoc_STR("(long inRefNum) -> (short recordingStatus, short meterLevel, unsigned long totalSamplesToRecord, unsigned long numberOfSamplesRecorded, unsigned long totalMsecsToRecord, unsigned long numberOfMsecsRecorded)")},
	{"SPBGetDeviceInfo", (PyCFunction)Snd_SPBGetDeviceInfo, 1,
	 PyDoc_STR("(long inRefNum, OSType infoType, void * infoData) -> None")},
	{"SPBSetDeviceInfo", (PyCFunction)Snd_SPBSetDeviceInfo, 1,
	 PyDoc_STR("(long inRefNum, OSType infoType, void * infoData) -> None")},
	{"SPBMillisecondsToBytes", (PyCFunction)Snd_SPBMillisecondsToBytes, 1,
	 PyDoc_STR("(long inRefNum) -> (long milliseconds)")},
	{"SPBBytesToMilliseconds", (PyCFunction)Snd_SPBBytesToMilliseconds, 1,
	 PyDoc_STR("(long inRefNum) -> (long byteCount)")},
	{NULL, NULL, 0}
};



/* Routine passed to Py_AddPendingCall -- call the Python callback */
static int
SndCh_CallCallBack(void *arg)
{
	SndChannelObject *p = (SndChannelObject *)arg;
	PyObject *args;
	PyObject *res;
	args = Py_BuildValue("(O(hhl))",
	                     p, p->ob_cmd.cmd, p->ob_cmd.param1, p->ob_cmd.param2);
	res = PyEval_CallObject(p->ob_callback, args);
	Py_DECREF(args);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

/* Routine passed to NewSndChannel -- schedule a call to SndCh_CallCallBack */
static pascal void
SndCh_UserRoutine(SndChannelPtr chan, SndCommand *cmd)
{
	SndChannelObject *p = (SndChannelObject *)(chan->userInfo);
	if (p->ob_callback != NULL) {
		long A5 = SetA5(p->ob_A5);
		p->ob_cmd = *cmd;
		Py_AddPendingCall(SndCh_CallCallBack, (void *)p);
		SetA5(A5);
	}
}

/* SPB callbacks - Schedule callbacks to Python */
static int
SPB_CallCallBack(void *arg)
{
	SPBObject *p = (SPBObject *)arg;
	PyObject *args;
	PyObject *res;
	
	if ( p->ob_thiscallback == 0 ) return 0;
	args = Py_BuildValue("(O)", p);
	res = PyEval_CallObject(p->ob_thiscallback, args);
	p->ob_thiscallback = 0;
	Py_DECREF(args);
	if (res == NULL)
		return -1;
	Py_DECREF(res);
	return 0;
}

static pascal void
SPB_completion(SPBPtr my_spb)
{
	SPBObject *p = (SPBObject *)(my_spb->userLong);
	
	if (p && p->ob_completion) {
		long A5 = SetA5(p->ob_A5);
		p->ob_thiscallback = p->ob_completion;	/* Hope we cannot get two at the same time */
		Py_AddPendingCall(SPB_CallCallBack, (void *)p);
		SetA5(A5);
	}
}



void init_Snd(void)
{
	PyObject *m;
	PyObject *d;





	m = Py_InitModule("_Snd", Snd_methods);
	d = PyModule_GetDict(m);
	Snd_Error = PyMac_GetOSErrException();
	if (Snd_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Snd_Error) != 0)
		return;
	SndChannel_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&SndChannel_Type) < 0) return;
	Py_INCREF(&SndChannel_Type);
	PyModule_AddObject(m, "SndChannel", (PyObject *)&SndChannel_Type);
	/* Backward-compatible name */
	Py_INCREF(&SndChannel_Type);
	PyModule_AddObject(m, "SndChannelType", (PyObject *)&SndChannel_Type);
	SPB_Type.ob_type = &PyType_Type;
	if (PyType_Ready(&SPB_Type) < 0) return;
	Py_INCREF(&SPB_Type);
	PyModule_AddObject(m, "SPB", (PyObject *)&SPB_Type);
	/* Backward-compatible name */
	Py_INCREF(&SPB_Type);
	PyModule_AddObject(m, "SPBType", (PyObject *)&SPB_Type);
}

/* ======================== End module _Snd ========================= */

