
/* =========================== Module Snd =========================== */

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

#include <Sound.h>

#ifndef HAVE_UNIVERSAL_HEADERS
#define SndCallBackUPP ProcPtr
#define NewSndCallBackProc(x) ((SndCallBackProcPtr)(x))
#define SndListHandle Handle
#endif

#include <OSUtils.h> /* for Set(Current)A5 */

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
		return PyArg_ParseTuple(v, "hhs#", &pc->cmd, &pc->param1, &pc->param2, &len);
	}
	return PyArg_Parse(v, "h", &pc->cmd);
}

/* Create a NumVersion object (a quintuple of integers) */
static PyObject *
NumVer_New(NumVersion nv)
{
	return Py_BuildValue("iiiii",
	                     nv.majorRev,
#ifdef THINK_C
	                     nv.minorRev,
	                     nv.bugFixRev,
#else
	                     (nv.minorAndBugRev>>4) & 0xf,
	                     nv.minorAndBugRev & 0xf,
#endif
	                     nv.stage,
	                     nv.nonRelRev);
}

static pascal void SndCh_UserRoutine(SndChannelPtr chan, SndCommand *cmd); /* Forward */

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

static PyObject *SndCh_New(itself)
	SndChannelPtr itself;
{
	SndChannelObject *it;
	it = PyObject_NEW(SndChannelObject, &SndChannel_Type);
	if (it == NULL) return NULL;
	it->ob_itself = itself;
	it->ob_callback = NULL;
	it->ob_A5 = SetCurrentA5();
	return (PyObject *)it;
}
static SndCh_Convert(v, p_itself)
	PyObject *v;
	SndChannelPtr *p_itself;
{
	if (!SndCh_Check(v))
	{
		PyErr_SetString(PyExc_TypeError, "SndChannel required");
		return 0;
	}
	*p_itself = ((SndChannelObject *)v)->ob_itself;
	return 1;
}

static void SndCh_dealloc(self)
	SndChannelObject *self;
{
	SndDisposeChannel(self->ob_itself, 1);
	Py_XDECREF(self->ob_callback);
	PyMem_DEL(self);
}

static PyObject *SndCh_SndDoCommand(_self, _args)
	SndChannelObject *_self;
	PyObject *_args;
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

static PyObject *SndCh_SndDoImmediate(_self, _args)
	SndChannelObject *_self;
	PyObject *_args;
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

static PyObject *SndCh_SndPlay(_self, _args)
	SndChannelObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	OSErr _err;
	SndListHandle sndHdl;
	Boolean async;
	if (!PyArg_ParseTuple(_args, "O&b",
	                      ResObj_Convert, &sndHdl,
	                      &async))
		return NULL;
	_err = SndPlay(_self->ob_itself,
	               sndHdl,
	               async);
	if (_err != noErr) return PyMac_Error(_err);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *SndCh_SndStartFilePlay(_self, _args)
	SndChannelObject *_self;
	PyObject *_args;
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

static PyObject *SndCh_SndPauseFilePlay(_self, _args)
	SndChannelObject *_self;
	PyObject *_args;
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

static PyObject *SndCh_SndStopFilePlay(_self, _args)
	SndChannelObject *_self;
	PyObject *_args;
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

static PyObject *SndCh_SndChannelStatus(_self, _args)
	SndChannelObject *_self;
	PyObject *_args;
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

static PyMethodDef SndCh_methods[] = {
	{"SndDoCommand", (PyCFunction)SndCh_SndDoCommand, 1,
	 "(SndCommand cmd, Boolean noWait) -> None"},
	{"SndDoImmediate", (PyCFunction)SndCh_SndDoImmediate, 1,
	 "(SndCommand cmd) -> None"},
	{"SndPlay", (PyCFunction)SndCh_SndPlay, 1,
	 "(SndListHandle sndHdl, Boolean async) -> None"},
	{"SndStartFilePlay", (PyCFunction)SndCh_SndStartFilePlay, 1,
	 "(short fRefNum, short resNum, long bufferSize, Boolean async) -> None"},
	{"SndPauseFilePlay", (PyCFunction)SndCh_SndPauseFilePlay, 1,
	 "() -> None"},
	{"SndStopFilePlay", (PyCFunction)SndCh_SndStopFilePlay, 1,
	 "(Boolean quietNow) -> None"},
	{"SndChannelStatus", (PyCFunction)SndCh_SndChannelStatus, 1,
	 "(short theLength) -> (SCStatus theStatus)"},
	{NULL, NULL, 0}
};

static PyMethodChain SndCh_chain = { SndCh_methods, NULL };

static PyObject *SndCh_getattr(self, name)
	SndChannelObject *self;
	char *name;
{
	return Py_FindMethodInChain(&SndCh_chain, (PyObject *)self, name);
}

#define SndCh_setattr NULL

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
};

/* ------------------- End object type SndChannel ------------------- */


static PyObject *Snd_SetSoundVol(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short level;
	if (!PyArg_ParseTuple(_args, "h",
	                      &level))
		return NULL;
	SetSoundVol(level);
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
}

static PyObject *Snd_GetSoundVol(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	short level;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	GetSoundVol(&level);
	_res = Py_BuildValue("h",
	                     level);
	return _res;
}

static PyObject *Snd_SndNewChannel(_self, _args)
	PyObject *_self;
	PyObject *_args;
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
	                     NewSndCallBackProc(SndCh_UserRoutine));
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

static PyObject *Snd_SndControl(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Snd_SndSoundManagerVersion(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	NumVersion _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = SndSoundManagerVersion();
	_res = Py_BuildValue("O&",
	                     NumVer_New, _rv);
	return _res;
}

static PyObject *Snd_SndManagerStatus(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Snd_SndGetSysBeepState(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Snd_SndSetSysBeepState(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Snd_MACEVersion(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	NumVersion _rv;
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	_rv = MACEVersion();
	_res = Py_BuildValue("O&",
	                     NumVer_New, _rv);
	return _res;
}

static PyObject *Snd_Comp3to1(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	char *buffer__in__;
	char *buffer__out__;
	long buffer__len__;
	int buffer__in_len__;
	char *state__in__;
	char state__out__[128];
	int state__len__;
	int state__in_len__;
	unsigned long numChannels;
	unsigned long whichChannel;
	if (!PyArg_ParseTuple(_args, "s#s#ll",
	                      &buffer__in__, &buffer__in_len__,
	                      &state__in__, &state__in_len__,
	                      &numChannels,
	                      &whichChannel))
		return NULL;
	if ((buffer__out__ = malloc(buffer__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto buffer__error__;
	}
	buffer__len__ = buffer__in_len__;
	if (state__in_len__ != 128)
	{
		PyErr_SetString(PyExc_TypeError, "buffer length should be 128");
		goto state__error__;
	}
	state__len__ = state__in_len__;
	Comp3to1(buffer__in__, buffer__out__, (long)buffer__len__,
	         state__in__, state__out__,
	         numChannels,
	         whichChannel);
	_res = Py_BuildValue("s#s#",
	                     buffer__out__, (int)buffer__len__,
	                     state__out__, (int)128);
 state__error__: ;
	free(buffer__out__);
 buffer__error__: ;
	return _res;
}

static PyObject *Snd_Exp1to3(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	char *buffer__in__;
	char *buffer__out__;
	long buffer__len__;
	int buffer__in_len__;
	char *state__in__;
	char state__out__[128];
	int state__len__;
	int state__in_len__;
	unsigned long numChannels;
	unsigned long whichChannel;
	if (!PyArg_ParseTuple(_args, "s#s#ll",
	                      &buffer__in__, &buffer__in_len__,
	                      &state__in__, &state__in_len__,
	                      &numChannels,
	                      &whichChannel))
		return NULL;
	if ((buffer__out__ = malloc(buffer__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto buffer__error__;
	}
	buffer__len__ = buffer__in_len__;
	if (state__in_len__ != 128)
	{
		PyErr_SetString(PyExc_TypeError, "buffer length should be 128");
		goto state__error__;
	}
	state__len__ = state__in_len__;
	Exp1to3(buffer__in__, buffer__out__, (long)buffer__len__,
	        state__in__, state__out__,
	        numChannels,
	        whichChannel);
	_res = Py_BuildValue("s#s#",
	                     buffer__out__, (int)buffer__len__,
	                     state__out__, (int)128);
 state__error__: ;
	free(buffer__out__);
 buffer__error__: ;
	return _res;
}

static PyObject *Snd_Comp6to1(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	char *buffer__in__;
	char *buffer__out__;
	long buffer__len__;
	int buffer__in_len__;
	char *state__in__;
	char state__out__[128];
	int state__len__;
	int state__in_len__;
	unsigned long numChannels;
	unsigned long whichChannel;
	if (!PyArg_ParseTuple(_args, "s#s#ll",
	                      &buffer__in__, &buffer__in_len__,
	                      &state__in__, &state__in_len__,
	                      &numChannels,
	                      &whichChannel))
		return NULL;
	if ((buffer__out__ = malloc(buffer__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto buffer__error__;
	}
	buffer__len__ = buffer__in_len__;
	if (state__in_len__ != 128)
	{
		PyErr_SetString(PyExc_TypeError, "buffer length should be 128");
		goto state__error__;
	}
	state__len__ = state__in_len__;
	Comp6to1(buffer__in__, buffer__out__, (long)buffer__len__,
	         state__in__, state__out__,
	         numChannels,
	         whichChannel);
	_res = Py_BuildValue("s#s#",
	                     buffer__out__, (int)buffer__len__,
	                     state__out__, (int)128);
 state__error__: ;
	free(buffer__out__);
 buffer__error__: ;
	return _res;
}

static PyObject *Snd_Exp1to6(_self, _args)
	PyObject *_self;
	PyObject *_args;
{
	PyObject *_res = NULL;
	char *buffer__in__;
	char *buffer__out__;
	long buffer__len__;
	int buffer__in_len__;
	char *state__in__;
	char state__out__[128];
	int state__len__;
	int state__in_len__;
	unsigned long numChannels;
	unsigned long whichChannel;
	if (!PyArg_ParseTuple(_args, "s#s#ll",
	                      &buffer__in__, &buffer__in_len__,
	                      &state__in__, &state__in_len__,
	                      &numChannels,
	                      &whichChannel))
		return NULL;
	if ((buffer__out__ = malloc(buffer__in_len__)) == NULL)
	{
		PyErr_NoMemory();
		goto buffer__error__;
	}
	buffer__len__ = buffer__in_len__;
	if (state__in_len__ != 128)
	{
		PyErr_SetString(PyExc_TypeError, "buffer length should be 128");
		goto state__error__;
	}
	state__len__ = state__in_len__;
	Exp1to6(buffer__in__, buffer__out__, (long)buffer__len__,
	        state__in__, state__out__,
	        numChannels,
	        whichChannel);
	_res = Py_BuildValue("s#s#",
	                     buffer__out__, (int)buffer__len__,
	                     state__out__, (int)128);
 state__error__: ;
	free(buffer__out__);
 buffer__error__: ;
	return _res;
}

static PyObject *Snd_GetSysBeepVolume(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Snd_SetSysBeepVolume(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Snd_GetDefaultOutputVolume(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Snd_SetDefaultOutputVolume(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyObject *Snd_GetSoundHeaderOffset(_self, _args)
	PyObject *_self;
	PyObject *_args;
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

static PyMethodDef Snd_methods[] = {
	{"SetSoundVol", (PyCFunction)Snd_SetSoundVol, 1,
	 "(short level) -> None"},
	{"GetSoundVol", (PyCFunction)Snd_GetSoundVol, 1,
	 "() -> (short level)"},
	{"SndNewChannel", (PyCFunction)Snd_SndNewChannel, 1,
	 "(short synth, long init, PyObject* userRoutine) -> (SndChannelPtr chan)"},
	{"SndControl", (PyCFunction)Snd_SndControl, 1,
	 "(short id) -> (SndCommand cmd)"},
	{"SndSoundManagerVersion", (PyCFunction)Snd_SndSoundManagerVersion, 1,
	 "() -> (NumVersion _rv)"},
	{"SndManagerStatus", (PyCFunction)Snd_SndManagerStatus, 1,
	 "(short theLength) -> (SMStatus theStatus)"},
	{"SndGetSysBeepState", (PyCFunction)Snd_SndGetSysBeepState, 1,
	 "() -> (short sysBeepState)"},
	{"SndSetSysBeepState", (PyCFunction)Snd_SndSetSysBeepState, 1,
	 "(short sysBeepState) -> None"},
	{"MACEVersion", (PyCFunction)Snd_MACEVersion, 1,
	 "() -> (NumVersion _rv)"},
	{"Comp3to1", (PyCFunction)Snd_Comp3to1, 1,
	 "(Buffer buffer, Buffer state, unsigned long numChannels, unsigned long whichChannel) -> (Buffer buffer, Buffer state)"},
	{"Exp1to3", (PyCFunction)Snd_Exp1to3, 1,
	 "(Buffer buffer, Buffer state, unsigned long numChannels, unsigned long whichChannel) -> (Buffer buffer, Buffer state)"},
	{"Comp6to1", (PyCFunction)Snd_Comp6to1, 1,
	 "(Buffer buffer, Buffer state, unsigned long numChannels, unsigned long whichChannel) -> (Buffer buffer, Buffer state)"},
	{"Exp1to6", (PyCFunction)Snd_Exp1to6, 1,
	 "(Buffer buffer, Buffer state, unsigned long numChannels, unsigned long whichChannel) -> (Buffer buffer, Buffer state)"},
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
	{NULL, NULL, 0}
};



/* Routine passed to Py_AddPendingCall -- call the Python callback */
static int
SndCh_CallCallBack(arg)
	void *arg;
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


void initSnd()
{
	PyObject *m;
	PyObject *d;





	m = Py_InitModule("Snd", Snd_methods);
	d = PyModule_GetDict(m);
	Snd_Error = PyMac_GetOSErrException();
	if (Snd_Error == NULL ||
	    PyDict_SetItemString(d, "Error", Snd_Error) != 0)
		Py_FatalError("can't initialize Snd.Error");
}

/* ========================= End module Snd ========================= */

