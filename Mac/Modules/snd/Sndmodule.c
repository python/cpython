
/* =========================== Module Snd =========================== */

#include "Python.h"



#include "macglue.h"
#include "pymactoolbox.h"

#ifdef WITHOUT_FRAMEWORKS
#include <Sound.h>
#include <OSUtils.h> /* for Set(Current)A5 */
#else
#include <Carbon/Carbon.h>
#endif


/* Create a SndCommand object (an (int, int, int) tuple) */
static PyObject *
SndCmd_New(SndCommand *pc)
{
	return Py_BuildValue("hhl", pc->cmd, pc->param1, pc->param2);
}

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
#if !TARGET_API_MAC_CARBON
static pascal void SPB_interrupt(SPBPtr my_spb); /* Forward */
#endif

static PyObject *Snd_Error;

/* --------------------- Object type SndChannel --------------------- */

staticforward PyTypeObject SndChannel_Type;

#define SndCh_Check(x) ((x)->ob_type == &SndChannel_Type)

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
static SndCh_Convert(PyObject *v, SndChannelPtr *p_itself)
{
	if (!SndCh_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "SndChannel required");
		return 0;
	}
	*p_itself = ((SndChannelObject *)v)->ob_itself;
	return 1;
}

static void SndCh_dealloc(SndChannelObject *self)
{
	SndDisposeChannel(self->ob_itself, 1);
	Py_XDECREF(self->ob_callback);
	PyMem_DEL(self);
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

#if !TARGET_API_MAC_CARBON

static PyObject *SndCh_SndStartFilePlay(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short fRefNum;
	short resNum;
	long bufferSize;
	Boolean async;
	if (!PyArg_ParseTuple(_args, "hhlb",
	                      &fRefNum,
	                      &resNum,
	                      &bufferSize,
	                      &async))
		return NULL;
	_err = SndStartFilePlay(_self->ob_itself,
	                        fRefNum,
	                        resNum,
	                        bufferSize,
	                        0,
	                        0,
	                        0,
	                        async);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *SndCh_SndPauseFilePlay(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_err = SndPauseFilePlay(_self->ob_itself);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *SndCh_SndStopFilePlay(SndChannelObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	Boolean quietNow;
	if (!PyArg_ParseTuple(_args, "b",
	                      &quietNow))
		return NULL;
	_err = SndStopFilePlay(_self->ob_itself,
	                       quietNow);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

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
 theStatus__error__: ;
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
	 "(SndCommand cmd, Boolean noWait) -> None"},
	{"SndDoImmediate", (PyCFunction)SndCh_SndDoImmediate, 1,
	 "(SndCommand cmd) -> None"},
	{"SndPlay", (PyCFunction)SndCh_SndPlay, 1,
	 "(SndListHandle sndHandle, Boolean async) -> None"},

#if !TARGET_API_MAC_CARBON
	{"SndStartFilePlay", (PyCFunction)SndCh_SndStartFilePlay, 1,
	 "(short fRefNum, short resNum, long bufferSize, Boolean async) -> None"},
#endif

#if !TARGET_API_MAC_CARBON
	{"SndPauseFilePlay", (PyCFunction)SndCh_SndPauseFilePlay, 1,
	 "() -> None"},
#endif

#if !TARGET_API_MAC_CARBON
	{"SndStopFilePlay", (PyCFunction)SndCh_SndStopFilePlay, 1,
	 "(Boolean quietNow) -> None"},
#endif
	{"SndChannelStatus", (PyCFunction)SndCh_SndChannelStatus, 1,
	 "(short theLength) -> (SCStatus theStatus)"},
	{"SndGetInfo", (PyCFunction)SndCh_SndGetInfo, 1,
	 "(OSType selector, void * infoPtr) -> None"},
	{"SndSetInfo", (PyCFunction)SndCh_SndSetInfo, 1,
	 "(OSType selector, void * infoPtr) -> None"},
	{NULL, NULL, 0}
};

static PyMethodChain SndCh_chain = { SndCh_methods, NULL };

static PyObject *SndCh_getattr(SndChannelObject *self, char *name)
{
	return Py_FindMethodInChain(&SndCh_chain, (PyObject *)self, name);
}

#define SndCh_setattr NULL

#define SndCh_compare NULL

#define SndCh_repr NULL

#define SndCh_hash NULL

staticforward PyTypeObject SndChannel_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"SndChannel", /*tp_name*/
	sizeof(SndChannelObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) SndCh_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) SndCh_getattr, /*tp_getattr*/
	(setattrfunc) SndCh_setattr, /*tp_setattr*/
	(cmpfunc) SndCh_compare, /*tp_compare*/
	(reprfunc) SndCh_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) SndCh_hash, /*tp_hash*/
};

/* ------------------- End object type SndChannel ------------------- */


/* ------------------------ Object type SPB ------------------------- */

staticforward PyTypeObject SPB_Type;

#define SPBObj_Check(x) ((x)->ob_type == &SPB_Type)

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
static SPBObj_Convert(PyObject *v, SPBPtr *p_itself)
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
	PyMem_DEL(self);
}

static PyMethodDef SPBObj_methods[] = {
	{NULL, NULL, 0}
};

static PyMethodChain SPBObj_chain = { SPBObj_methods, NULL };

static PyObject *SPBObj_getattr(SPBObject *self, char *name)
{

				if (strcmp(name, "inRefNum") == 0)
					return Py_BuildValue("l", self->ob_spb.inRefNum);
				else if (strcmp(name, "count") == 0)
					return Py_BuildValue("l", self->ob_spb.count);
				else if (strcmp(name, "milliseconds") == 0)
					return Py_BuildValue("l", self->ob_spb.milliseconds);
				else if (strcmp(name, "error") == 0)
					return Py_BuildValue("h", self->ob_spb.error);
	return Py_FindMethodInChain(&SPBObj_chain, (PyObject *)self, name);
}

static int SPBObj_setattr(SPBObject *self, char *name, PyObject *value)
{

		int rv = 0;
		
		if (strcmp(name, "inRefNum") == 0)
			rv = PyArg_Parse(value, "l", &self->ob_spb.inRefNum);
		else if (strcmp(name, "count") == 0)
			rv = PyArg_Parse(value, "l", &self->ob_spb.count);
		else if (strcmp(name, "milliseconds") == 0)
			rv = PyArg_Parse(value, "l", &self->ob_spb.milliseconds);
		else if (strcmp(name, "buffer") == 0)
			rv = PyArg_Parse(value, "w#", &self->ob_spb.bufferPtr, &self->ob_spb.bufferLength);
		else if (strcmp(name, "completionRoutine") == 0) {
			self->ob_spb.completionRoutine = NewSICompletionUPP(SPB_completion);
			self->ob_completion = value;
			Py_INCREF(value);
			rv = 1;
#if !TARGET_API_MAC_CARBON
		} else if (strcmp(name, "interruptRoutine") == 0) {
			self->ob_spb.completionRoutine = NewSIInterruptUPP(SPB_interrupt);
			self->ob_interrupt = value;
			Py_INCREF(value);
			rv = 1;
#endif
		}
		if ( rv ) return 0;
		else return -1;
}

#define SPBObj_compare NULL

#define SPBObj_repr NULL

#define SPBObj_hash NULL

staticforward PyTypeObject SPB_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0, /*ob_size*/
	"SPB", /*tp_name*/
	sizeof(SPBObject), /*tp_basicsize*/
	0, /*tp_itemsize*/
	/* methods */
	(destructor) SPBObj_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	(getattrfunc) SPBObj_getattr, /*tp_getattr*/
	(setattrfunc) SPBObj_setattr, /*tp_setattr*/
	(cmpfunc) SPBObj_compare, /*tp_compare*/
	(reprfunc) SPBObj_repr, /*tp_repr*/
	(PyNumberMethods *)0, /* tp_as_number */
	(PySequenceMethods *)0, /* tp_as_sequence */
	(PyMappingMethods *)0, /* tp_as_mapping */
	(hashfunc) SPBObj_hash, /*tp_hash*/
};

/* ---------------------- End object type SPB ----------------------- */


static PyObject *Snd_SPB(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	return SPBObj_New();
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

#if !TARGET_API_MAC_CARBON

static PyObject *Snd_SndControl(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short id;
	SndCommand cmd;
	if (!PyArg_ParseTuple(_args, "h",
	                      &id))
		return NULL;
	_err = SndControl(id,
	                  &cmd);
	if (_err != noErr) return PyMac_Error(_err);
	_res = Py_BuildValue("O&",
	                     SndCmd_New, &cmd);
	return _res;
}
#endif

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
 theStatus__error__: ;
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

#if !TARGET_API_MAC_CARBON

static PyObject *Snd_MACEVersion(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	NumVersion _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MACEVersion();
	_res = Py_BuildValue("O&",
	                     PyMac_BuildNumVersion, _rv);
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Snd_Comp3to1(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *buffer__in__;
	char *buffer__out__;
	long buffer__len__;
	int buffer__in_len__;
	StateBlock *state__in__;
	StateBlock state__out__;
	int state__in_len__;
	unsigned long numChannels;
	unsigned long whichChannel;
	if (!PyArg_ParseTuple(_args, "s#s#ll",
	                      &buffer__in__, &buffer__in_len__,
	                      (char **)&state__in__, &state__in_len__,
	                      &numChannels,
	                      &whichChannel))
		return NULL;
	if ((buffer__out__ = malloc(buffer__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto buffer__error__;
	}
	buffer__len__ = buffer__in_len__;
	if (state__in_len__ != sizeof(StateBlock))
	{
		PyErr_SetString(PyExc_TypeError, "buffer length should be sizeof(StateBlock)");
		goto state__error__;
	}
	Comp3to1(buffer__in__, buffer__out__, (long)buffer__len__,
	         state__in__, &state__out__,
	         numChannels,
	         whichChannel);
	_res = Py_BuildValue("s#s#",
	                     buffer__out__, (int)buffer__len__,
	                     (char *)&state__out__, (int)sizeof(StateBlock));
 state__error__: ;
	free(buffer__out__);
 buffer__error__: ;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Snd_Exp1to3(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *buffer__in__;
	char *buffer__out__;
	long buffer__len__;
	int buffer__in_len__;
	StateBlock *state__in__;
	StateBlock state__out__;
	int state__in_len__;
	unsigned long numChannels;
	unsigned long whichChannel;
	if (!PyArg_ParseTuple(_args, "s#s#ll",
	                      &buffer__in__, &buffer__in_len__,
	                      (char **)&state__in__, &state__in_len__,
	                      &numChannels,
	                      &whichChannel))
		return NULL;
	if ((buffer__out__ = malloc(buffer__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto buffer__error__;
	}
	buffer__len__ = buffer__in_len__;
	if (state__in_len__ != sizeof(StateBlock))
	{
		PyErr_SetString(PyExc_TypeError, "buffer length should be sizeof(StateBlock)");
		goto state__error__;
	}
	Exp1to3(buffer__in__, buffer__out__, (long)buffer__len__,
	        state__in__, &state__out__,
	        numChannels,
	        whichChannel);
	_res = Py_BuildValue("s#s#",
	                     buffer__out__, (int)buffer__len__,
	                     (char *)&state__out__, (int)sizeof(StateBlock));
 state__error__: ;
	free(buffer__out__);
 buffer__error__: ;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Snd_Comp6to1(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *buffer__in__;
	char *buffer__out__;
	long buffer__len__;
	int buffer__in_len__;
	StateBlock *state__in__;
	StateBlock state__out__;
	int state__in_len__;
	unsigned long numChannels;
	unsigned long whichChannel;
	if (!PyArg_ParseTuple(_args, "s#s#ll",
	                      &buffer__in__, &buffer__in_len__,
	                      (char **)&state__in__, &state__in_len__,
	                      &numChannels,
	                      &whichChannel))
		return NULL;
	if ((buffer__out__ = malloc(buffer__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto buffer__error__;
	}
	buffer__len__ = buffer__in_len__;
	if (state__in_len__ != sizeof(StateBlock))
	{
		PyErr_SetString(PyExc_TypeError, "buffer length should be sizeof(StateBlock)");
		goto state__error__;
	}
	Comp6to1(buffer__in__, buffer__out__, (long)buffer__len__,
	         state__in__, &state__out__,
	         numChannels,
	         whichChannel);
	_res = Py_BuildValue("s#s#",
	                     buffer__out__, (int)buffer__len__,
	                     (char *)&state__out__, (int)sizeof(StateBlock));
 state__error__: ;
	free(buffer__out__);
 buffer__error__: ;
	return _res;
}
#endif

#if !TARGET_API_MAC_CARBON

static PyObject *Snd_Exp1to6(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	char *buffer__in__;
	char *buffer__out__;
	long buffer__len__;
	int buffer__in_len__;
	StateBlock *state__in__;
	StateBlock state__out__;
	int state__in_len__;
	unsigned long numChannels;
	unsigned long whichChannel;
	if (!PyArg_ParseTuple(_args, "s#s#ll",
	                      &buffer__in__, &buffer__in_len__,
	                      (char **)&state__in__, &state__in_len__,
	                      &numChannels,
	                      &whichChannel))
		return NULL;
	if ((buffer__out__ = malloc(buffer__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto buffer__error__;
	}
	buffer__len__ = buffer__in_len__;
	if (state__in_len__ != sizeof(StateBlock))
	{
		PyErr_SetString(PyExc_TypeError, "buffer length should be sizeof(StateBlock)");
		goto state__error__;
	}
	Exp1to6(buffer__in__, buffer__out__, (long)buffer__len__,
	        state__in__, &state__out__,
	        numChannels,
	        whichChannel);
	_res = Py_BuildValue("s#s#",
	                     buffer__out__, (int)buffer__len__,
	                     (char *)&state__out__, (int)sizeof(StateBlock));
 state__error__: ;
	free(buffer__out__);
 buffer__error__: ;
	return _res;
}
#endif

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
 cp__error__: ;
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

#if !TARGET_API_MAC_CARBON

static PyObject *Snd_SPBRecordToFile(PyObject *_self, PyObject *_args)
{
	PyObject *_res = NULL;
	OSErr _err;
	short fRefNum;
	SPBPtr inParamPtr;
	Boolean asynchFlag;
	if (!PyArg_ParseTuple(_args, "hO&b",
	                      &fRefNum,
	                      SPBObj_Convert, &inParamPtr,
	                      &asynchFlag))
		return NULL;
	_err = SPBRecordToFile(fRefNum,
	                       inParamPtr,
	                       asynchFlag);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}
#endif

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
	 NULL},
	{"SysBeep", (PyCFunction)Snd_SysBeep, 1,
	 "(short duration) -> None"},
	{"SndNewChannel", (PyCFunction)Snd_SndNewChannel, 1,
	 "(short synth, long init, PyObject* userRoutine) -> (SndChannelPtr chan)"},

#if !TARGET_API_MAC_CARBON
	{"SndControl", (PyCFunction)Snd_SndControl, 1,
	 "(short id) -> (SndCommand cmd)"},
#endif
	{"SndSoundManagerVersion", (PyCFunction)Snd_SndSoundManagerVersion, 1,
	 "() -> (NumVersion _rv)"},
	{"SndManagerStatus", (PyCFunction)Snd_SndManagerStatus, 1,
	 "(short theLength) -> (SMStatus theStatus)"},
	{"SndGetSysBeepState", (PyCFunction)Snd_SndGetSysBeepState, 1,
	 "() -> (short sysBeepState)"},
	{"SndSetSysBeepState", (PyCFunction)Snd_SndSetSysBeepState, 1,
	 "(short sysBeepState) -> None"},

#if !TARGET_API_MAC_CARBON
	{"MACEVersion", (PyCFunction)Snd_MACEVersion, 1,
	 "() -> (NumVersion _rv)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"Comp3to1", (PyCFunction)Snd_Comp3to1, 1,
	 "(Buffer buffer, StateBlock state, unsigned long numChannels, unsigned long whichChannel) -> (Buffer buffer, StateBlock state)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"Exp1to3", (PyCFunction)Snd_Exp1to3, 1,
	 "(Buffer buffer, StateBlock state, unsigned long numChannels, unsigned long whichChannel) -> (Buffer buffer, StateBlock state)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"Comp6to1", (PyCFunction)Snd_Comp6to1, 1,
	 "(Buffer buffer, StateBlock state, unsigned long numChannels, unsigned long whichChannel) -> (Buffer buffer, StateBlock state)"},
#endif

#if !TARGET_API_MAC_CARBON
	{"Exp1to6", (PyCFunction)Snd_Exp1to6, 1,
	 "(Buffer buffer, StateBlock state, unsigned long numChannels, unsigned long whichChannel) -> (Buffer buffer, StateBlock state)"},
#endif
	{"GetSysBeepVolume", (PyCFunction)Snd_GetSysBeepVolume, 1,
	 "() -> (long level)"},
	{"SetSysBeepVolume", (PyCFunction)Snd_SetSysBeepVolume, 1,
	 "(long level) -> None"},
	{"GetDefaultOutputVolume", (PyCFunction)Snd_GetDefaultOutputVolume, 1,
	 "() -> (long level)"},
	{"SetDefaultOutputVolume", (PyCFunction)Snd_SetDefaultOutputVolume, 1,
	 "(long level) -> None"},
	{"GetSoundHeaderOffset", (PyCFunction)Snd_GetSoundHeaderOffset, 1,
	 "(SndListHandle sndHandle) -> (long offset)"},
	{"GetCompressionInfo", (PyCFunction)Snd_GetCompressionInfo, 1,
	 "(short compressionID, OSType format, short numChannels, short sampleSize) -> (CompressionInfo cp)"},
	{"SetSoundPreference", (PyCFunction)Snd_SetSoundPreference, 1,
	 "(OSType theType, Handle settings) -> (Str255 name)"},
	{"GetSoundPreference", (PyCFunction)Snd_GetSoundPreference, 1,
	 "(OSType theType, Handle settings) -> (Str255 name)"},
	{"GetCompressionName", (PyCFunction)Snd_GetCompressionName, 1,
	 "(OSType compressionType) -> (Str255 compressionName)"},
	{"SPBVersion", (PyCFunction)Snd_SPBVersion, 1,
	 "() -> (NumVersion _rv)"},
	{"SPBSignInDevice", (PyCFunction)Snd_SPBSignInDevice, 1,
	 "(short deviceRefNum, Str255 deviceName) -> None"},
	{"SPBSignOutDevice", (PyCFunction)Snd_SPBSignOutDevice, 1,
	 "(short deviceRefNum) -> None"},
	{"SPBGetIndexedDevice", (PyCFunction)Snd_SPBGetIndexedDevice, 1,
	 "(short count) -> (Str255 deviceName, Handle deviceIconHandle)"},
	{"SPBOpenDevice", (PyCFunction)Snd_SPBOpenDevice, 1,
	 "(Str255 deviceName, short permission) -> (long inRefNum)"},
	{"SPBCloseDevice", (PyCFunction)Snd_SPBCloseDevice, 1,
	 "(long inRefNum) -> None"},
	{"SPBRecord", (PyCFunction)Snd_SPBRecord, 1,
	 "(SPBPtr inParamPtr, Boolean asynchFlag) -> None"},

#if !TARGET_API_MAC_CARBON
	{"SPBRecordToFile", (PyCFunction)Snd_SPBRecordToFile, 1,
	 "(short fRefNum, SPBPtr inParamPtr, Boolean asynchFlag) -> None"},
#endif
	{"SPBPauseRecording", (PyCFunction)Snd_SPBPauseRecording, 1,
	 "(long inRefNum) -> None"},
	{"SPBResumeRecording", (PyCFunction)Snd_SPBResumeRecording, 1,
	 "(long inRefNum) -> None"},
	{"SPBStopRecording", (PyCFunction)Snd_SPBStopRecording, 1,
	 "(long inRefNum) -> None"},
	{"SPBGetRecordingStatus", (PyCFunction)Snd_SPBGetRecordingStatus, 1,
	 "(long inRefNum) -> (short recordingStatus, short meterLevel, unsigned long totalSamplesToRecord, unsigned long numberOfSamplesRecorded, unsigned long totalMsecsToRecord, unsigned long numberOfMsecsRecorded)"},
	{"SPBGetDeviceInfo", (PyCFunction)Snd_SPBGetDeviceInfo, 1,
	 "(long inRefNum, OSType infoType, void * infoData) -> None"},
	{"SPBSetDeviceInfo", (PyCFunction)Snd_SPBSetDeviceInfo, 1,
	 "(long inRefNum, OSType infoType, void * infoData) -> None"},
	{"SPBMillisecondsToBytes", (PyCFunction)Snd_SPBMillisecondsToBytes, 1,
	 "(long inRefNum) -> (long milliseconds)"},
	{"SPBBytesToMilliseconds", (PyCFunction)Snd_SPBBytesToMilliseconds, 1,
	 "(long inRefNum) -> (long byteCount)"},
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

#if !TARGET_API_MAC_CARBON
static pascal void
SPB_interrupt(SPBPtr my_spb)
{
	SPBObject *p = (SPBObject *)(my_spb->userLong);
	
	if (p && p->ob_interrupt) {
		long A5 = SetA5(p->ob_A5);
		p->ob_thiscallback = p->ob_interrupt;	/* Hope we cannot get two at the same time */
		Py_AddPendingCall(SPB_CallCallBack, (void *)p);
		SetA5(A5);
	}
}
#endif


void initSnd(void)
{
	PyObject *m;
	PyObject *d;





	m = Py_InitModule("Snd", Snd_methods);
	d = PyModule_GetDict(m);
	Snd_Error = PyMac_GetOSErrException();
	if (Snd_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Snd_Error) != 0)
		return;
	SndChannel_Type.ob_type = &PyType_Type;
	Py_INCREF(&SndChannel_Type);
	if (PyDict_SetItemString(d, "SndChannelType", (PyObject *)&SndChannel_Type) != 0)
		Py_FatalError("can't initialize SndChannelType");
	SPB_Type.ob_type = &PyType_Type;
	Py_INCREF(&SPB_Type);
	if (PyDict_SetItemString(d, "SPBType", (PyObject *)&SPB_Type) != 0)
		Py_FatalError("can't initialize SPBType");
}

/* ========================= End module Snd ========================= */

