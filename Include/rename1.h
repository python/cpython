#ifndef Py_RENAME1_H
#define Py_RENAME1_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
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

/* This file contains a bunch of #defines that make it possible to use
   "new style" names (e.g. PyObject) with the old style Python source
   distribution. */

/* Remove some symbols (these conflict with X11 symbols) */
#undef True
#undef False
#undef None

typedef ANY *PyUnivPtr;
typedef struct methodlist PyMethodDef;

#define Py_NO_DEBUG NDEBUG
#define Py_TRACE_REFS TRACE_REFS
#define Py_REF_DEBUG REF_DEBUG
#define Py_HAVE_PROTOTYPES HAVE_PROTOTYPES
#define Py_HAVE_STDLIB HAVE_STDLIB
#define _Py_ZeroStruct FalseObject
#define _Py_NoneStruct NoObject
#define _Py_TrueStruct TrueObject
#define Py_DebugFlag debugging
#define _PyParser_Grammar gram
#define _PySys_ProfileFunc sys_profile
#define _PySys_TraceFunc sys_trace
#define _PyThread_Started threads_started
#define _PyParser_TokenNames tok_name
#define Py_VerboseFlag verbose
#define PyExc_AttributeError AttributeError
#define PyExc_EOFError EOFError
#define PyExc_IOError IOError
#define PyExc_ImportError ImportError
#define PyExc_IndexError IndexError
#define PyExc_KeyError KeyError
#define PyExc_MemoryError MemoryError
#define PyExc_NameError NameError
#define PyExc_OverflowError OverflowError
#define PyExc_RuntimeError RuntimeError
#define PyExc_SyntaxError SyntaxError
#define PyExc_SystemError SystemError
#define PyExc_TypeError TypeError
#define PyExc_ValueError ValueError
#define PyExc_ZeroDivisionError ZeroDivisionError
#define PyExc_KeyboardInterrupt KeyboardInterrupt
#define PyExc_SystemExit SystemExit
#define PyFloat_Type Floattype
#define PyInt_Type Inttype
#define PyLong_Type Longtype
#define PyNothing_Type Notype
#define PyString_Type Stringtype
#define PyType_Type Typetype
#define PyList_Type Listtype
#define PyDict_Type Dicttype
#define PyTuple_Type Tupletype
#define PyFile_Type Filetype
#define PyClass_Type Classtype
#define PyFunction_Type Functype
#define PyMethod_Type Instancemethodtype
#define PyInstance_Type Instancetype
#define PyCFunction_Type Methodtype
#define PyModule_Type Moduletype
#define PyCode_Type Codetype
#define PyFrame_Type Frametype
#define PyFloatObject floatobject
#define PyIntObject intobject
#define PyLongObject longobject
#define PyNothingObject noobject
#define PyObject object
#define PyStringObject stringobject
#define PyTypeObject typeobject
#define PyListObject listobject
#define PyDictObject dictobject
#define PyTupleObject tupleobject
#define PyFileObject fileobject
#define PyClassObject classobject
#define PyCodeObject codeobject
#define PyFrameObject frameobject
#define PyFunctionObject funcobject
#define PyMethodObject instancemethodobject
#define PyInstanceObject instanceobject
#define PyCFunctionObject methodobject
#define PyModuleObject moduleobject
#define PyNumberMethods number_methods
#define PySequenceMethods sequence_methods
#define PyMappingMethods mapping_methods
#define PyObject_HEAD OB_HEAD
#define PyObject_VAR_HEAD OB_VARHEAD
#define PyObject_HEAD_INIT OB_HEAD_INIT
#define PyObject_NEW NEWOBJ
#define PyObject_NEW_VAR NEWVAROBJ
#define Py_PROTO PROTO
#define Py_FPROTO PROTO
#define PyMem_NEW NEW
#define PyMem_RESIZE RESIZE
#define PyMem_DEL DEL
#define PyMem_XDEL XDEL
#define Py_BEGIN_ALLOW_THREADS BGN_SAVE
#define Py_BLOCK_THREADS RET_SAVE
#define Py_UNBLOCK_THREADS RES_SAVE
#define Py_END_ALLOW_THREADS END_SAVE
#define PyFloat_Check is_floatobject
#define PyInt_Check is_intobject
#define PyLong_Check is_longobject
#define PyNothing_Check is_noobject
#define PyString_Check is_stringobject
#define PyType_Check is_typeobject
#define PyList_Check is_listobject
#define PyDict_Check is_dictobject
#define PyTuple_Check is_tupleobject
#define PyFile_Check is_fileobject
#define PyClass_Check is_classobject
#define PyCode_Check is_codeobject
#define PyFrame_Check is_frameobject
#define PyFunction_Check is_funcobject
#define PyMethod_Check is_instancemethodobject
#define PyInstance_Check is_instanceobject
#define PyCFunction_Check is_methodobject
#define PyModule_Check is_moduleobject
#define Py_INCREF INCREF
#define Py_DECREF DECREF
#define Py_XINCREF XINCREF
#define Py_XDECREF XDECREF
#define _Py_NewReference NEWREF
#define _Py_Dealloc DELREF
#define _Py_ForgetReference UNREF
#define Py_None (&_Py_NoneStruct)
#define Py_False ((object *) &_Py_ZeroStruct)
#define Py_True ((object *) &_Py_TrueStruct)
#define PyObject_Compare cmpobject
#define PyObject_GetAttrString getattr
#define PyObject_GetAttr getattro
#define PyObject_Hash hashobject
#define _PyObject_New newobject
#define _PyObject_NewVar newvarobject
#define PyObject_Print printobject
#define PyObject_Repr reprobject
#define PyObject_SetAttrString setattr
#define PyObject_SetAttr setattro
#define PyObject_IsTrue testbool
#define Py_PRINT_RAW PRINT_RAW
#define PyFloat_AsString float_buf_repr
#define PyFloat_AsDouble getfloatvalue
#define PyFloat_AS_DOUBLE GETFLOATVALUE
#define PyFloat_FromDouble newfloatobject
#define PyInt_AsLong getintvalue
#define PyInt_AS_LONG GETINTVALUE
#define PyInt_FromLong newintobject
#define _PyLong_New alloclongobject
#define PyLong_AsDouble dgetlongvalue
#define PyLong_FromDouble dnewlongobject
#define PyLong_AsLong getlongvalue
#define PyLong_FromString long_scan
#define PyLong_FromLong newlongobject
#define PyString_Format formatstring
#define PyString_Size getstringsize
#define PyString_AsString getstringvalue
#define PyString_AS_STRING GETSTRINGVALUE
#define PyString_Concat joinstring
#define PyString_FromStringAndSize newsizedstringobject
#define PyString_FromString newstringobject
#define _PyString_Resize resizestring
#define PyList_Append addlistitem
#define PyList_GetItem getlistitem
#define PyList_GET_ITEM GETLISTITEM
#define PyList_Size getlistsize
#define PyList_GetSlice getlistslice
#define PyList_Insert inslistitem
#define PyList_New newlistobject
#define PyList_SetItem setlistitem
#define PyList_SetSlice setlistslice
#define PyList_Sort sortlist
#define PyDict_SetItemString dictinsert
#define PyDict_GetItemString dictlookup
#define PyDict_DelItemString dictremove
#define PyDict_Items getmappingitems
#define PyDict_Keys getmappingkeys
#define PyDict_Values getmappingvalues
#define PyDict_Clear mappingclear
#define PyDict_Next mappinggetnext
#define PyDict_SetItem mappinginsert
#define PyDict_GetItem mappinglookup
#define PyDict_DelItem mappingremove
#define PyDict_New newmappingobject
#define PyTuple_GetItem gettupleitem
#define PyTuple_GET_ITEM GETTUPLEITEM
#define PyTuple_Size gettuplesize
#define PyTuple_GetSlice gettupleslice
#define PyTuple_New newtupleobject
#define PyTuple_SetItem settupleitem
#define PyFile_GetLine filegetline
#define PyFile_AsFile getfilefile
#define PyFile_FromString newfileobject
#define PyFile_FromFile newopenfileobject
#define PyFile_SoftSpace softspace
#define PyFile_WriteObject writeobject
#define PyFile_WriteString writestring
#define PyMethod_Class instancemethodgetclass
#define PyMethod_Function instancemethodgetfunc
#define PyMethod_Self instancemethodgetself
#define PyClass_IsSubclass issubclass
#define PyClass_New newclassobject
#define PyMethod_New newinstancemethodobject
#define PyInstance_New newinstanceobject
#define PyTryBlock block
#define PyFrame_ExtendStack extend_stack
#define PyFrame_New newframeobject
#define PyFrame_BlockPop pop_block
#define PyFrame_BlockSetup setup_block
#define PyFunction_GetCode getfunccode
#define PyFunction_GetGlobals getfuncglobals
#define PyFunction_New newfuncobject
#define PyCFunction method
#define Py_FindMethod findmethod
#define PyCFunction_GetFunction getmethod
#define PyCFunction_GetSelf getself
#define PyCFunction_IsVarArgs getvarargs
#define PyCFunction_New newmethodobject
#define PyModule_GetDict getmoduledict
#define PyModule_GetName getmodulename
#define PyModule_New newmoduleobject
#define PyGrammar_AddAccelerators addaccelerators
#define PyGrammar_FindDFA finddfa
#define PyGrammar_LabelRepr labelrepr
#define PyNode_ListTree listtree
#define PyNode_AddChild addchild
#define PyNode_Free freetree
#define PyNode_New newtree
#define PyParser_AddToken addtoken
#define PyParser_Delete delparser
#define PyParser_New newparser
#define PyParser_ParseFile parsefile
#define PyParser_ParseString parsestring
#define PyToken_OneChar tok_1char
#define PyToken_TwoChars tok_2char
#define PyTokenizer_Free tok_free
#define PyTokenizer_Get tok_get
#define PyTokenizer_FromFile tok_setupf
#define PyTokenizer_FromString tok_setups
#define PyNode_Compile compile
#define PyCode_New newcodeobject
#define PyEval_CallObject call_object
#define PyEval_EvalCode eval_code
#define Py_FlushLine flushline
#define PyEval_GetGlobals getglobals
#define PyEval_GetLocals getlocals
#define PyEval_InitThreads init_save_thread
#define PyErr_PrintTraceBack printtraceback
#define PyEval_RestoreThread restore_thread
#define PyEval_SaveThread save_thread
#define PyTraceBack_Fetch tb_fetch
#define PyTraceBack_Here tb_here
#define PyTraceBack_Print tb_print
#define PyTraceBack_Store tb_store
#define PyImport_AddModule add_module
#define PyImport_Cleanup doneimport
#define PyImport_GetModuleDict get_modules
#define PyImport_ImportModule import_module
#define PyImport_ImportFrozenModule init_frozen
#define PyImport_Init initimport
#define PyImport_ReloadModule reload_module
#define PyNumber_Coerce coerce
#define PyBuiltin_GetObject getbuiltin
#define PyBuiltin_Init initbuiltin
#define PyMarshal_Init initmarshal
#define PyMarshal_ReadLongFromFile rd_long
#define PyMarshal_ReadObjectFromFile rd_object
#define PyMarshal_ReadObjectFromString rds_object
#define PyMarshal_WriteLongToFile wr_long
#define PyMarshal_WriteObjectToFile wr_object
#define PySys_Init initsys
#define PySys_SetArgv setpythonargv
#define PySys_SetPath setpythonpath
#define PySys_GetObject sysget
#define PySys_GetFile sysgetfile
#define PySys_SetObject sysset
#define Py_CompileString compile_string
#define Py_FatalError fatal
#define Py_Exit goaway
#define Py_Initialize initall
#define PyErr_Print print_error
#define PyParser_SimpleParseFile parse_file
#define PyParser_SimpleParseString parse_string
#define PyRun_AnyFile run
#define PyRun_SimpleFile run_script
#define PyRun_SimpleString run_command
#define PyRun_File run_file
#define PyRun_String run_string
#define PyRun_InteractiveOne run_tty_1
#define PyRun_InteractiveLoop run_tty_loop
#define PyMember_Get getmember
#define PyMember_Set setmember
#define Py_InitModule initmodule
#define Py_BuildValue mkvalue
#define Py_VaBuildValue vmkvalue
#define PyArg_Parse getargs
#define PyArg_NoArgs(v) getargs(v, "")
#define PyArg_GetChar getichararg
#define PyArg_GetDoubleArray getidoublearray
#define PyArg_GetFloat getifloatarg
#define PyArg_GetFloatArray getifloatarray
#define PyArg_GetInt getintarg
#define PyArg_GetLong getilongarg
#define PyArg_GetLongArray getilongarray
#define PyArg_GetLongArraySize getilongarraysize
#define PyArg_GetObject getiobjectarg
#define PyArg_GetShort getishortarg
#define PyArg_GetShortArray getishortarray
#define PyArg_GetShortArraySize getishortarraysize
#define PyArg_GetString getistringarg
#define PyErr_BadArgument err_badarg
#define PyErr_BadInternalCall err_badcall
#define PyErr_Input err_input
#define PyErr_NoMemory err_nomem
#define PyErr_SetFromErrno err_errno
#define PyErr_SetNone err_set
#define PyErr_SetString err_setstr
#define PyErr_SetObject err_setval
#define PyErr_Occurred err_occurred
#define PyErr_GetAndClear err_get
#define PyErr_Clear err_clear
#define PyOS_InterruptableGetString fgets_intr
#define PyOS_InitInterrupts initintr
#define PyOS_InterruptOccurred intrcheck
#define PyOS_GetLastModificationTime getmtime

#ifdef __cplusplus
}
#endif
#endif /* !Py_RENAME1_H */
