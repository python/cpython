#ifndef Py_OLDNAMES_H
#define Py_OLDNAMES_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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
   "old style" names (e.g. object) with the new style Python source
   distribution. */

#define True Py_True
#define False Py_False
#define None Py_None

/* typedef ANY *PyUnivPtr; */
#define methodlist PyMethodDef
#define methodchain PyMethodChain

#define None Py_None
#define False Py_False
#define True Py_True

#define Accesstype PyAccess_Type
#define is_accessobject PyAccess_Check
#define newaccessobject PyAccess_FromValue
#define getaccessvalue PyAccess_AsValue
#define setaccessvalue PyAccess_SetValue
#define setaccessowner PyAccess_SetOwner
#define cloneaccessobject PyAccess_Clone
#define hasaccessvalue PyAccess_HasValue
#define Anynumbertype PyAnyNumber_Type
#define Anysequencetype PyAnySequence_Type
#define Anymappingtype PyAnyMapping_Type

#ifdef Py_TRACE_REFS
#define TRACE_REFS
#endif

#ifdef Py_REF_DEBUG
#define REF_DEBUG
#endif

#define FalseObject _Py_ZeroStruct
#define NoObject _Py_NoneStruct
#define TrueObject _Py_TrueStruct
#define debugging Py_DebugFlag
#define gram _PyParser_Grammar
#define sys_profile _PySys_ProfileFunc
#define sys_trace _PySys_TraceFunc
#define sys_checkinterval _PySys_CheckInterval
#define threads_started _PyThread_Started
#define tok_name _PyParser_TokenNames
#define verbose Py_VerboseFlag
#define suppress_print Py_SuppressPrintingFlag
#define AccessError PyExc_AccessError
#define AttributeError PyExc_AttributeError
#define ConflictError PyExc_ConflictError
#define EOFError PyExc_EOFError
#define IOError PyExc_IOError
#define ImportError PyExc_ImportError
#define IndexError PyExc_IndexError
#define KeyError PyExc_KeyError
#define MemoryError PyExc_MemoryError
#define NameError PyExc_NameError
#define OverflowError PyExc_OverflowError
#define RuntimeError PyExc_RuntimeError
#define SyntaxError PyExc_SyntaxError
#define SystemError PyExc_SystemError
#define TypeError PyExc_TypeError
#define ValueError PyExc_ValueError
#define ZeroDivisionError PyExc_ZeroDivisionError
#define KeyboardInterrupt PyExc_KeyboardInterrupt
#define SystemExit PyExc_SystemExit
#define Floattype PyFloat_Type
#define Inttype PyInt_Type
#define Longtype PyLong_Type
#define Notype PyNothing_Type
#define Stringtype PyString_Type
#define Typetype PyType_Type
#define Listtype PyList_Type
#define Dicttype PyDict_Type
#define Mappingtype PyDict_Type
#define Tupletype PyTuple_Type
#define Filetype PyFile_Type
#define Classtype PyClass_Type
#define Functype PyFunction_Type
#define Instancemethodtype PyMethod_Type
#define Instancetype PyInstance_Type
#define Methodtype PyCFunction_Type
#define Moduletype PyModule_Type
#define Codetype PyCode_Type
#define Frametype PyFrame_Type
#define Rangetype PyRange_Type
#define floatobject PyFloatObject
#define intobject PyIntObject
#define longobject PyLongObject
#define noobject PyNothingObject
#define object PyObject
#define stringobject PyStringObject
#define typeobject PyTypeObject
#define listobject PyListObject
#define dictobject PyDictObject
#define tupleobject PyTupleObject
#define fileobject PyFileObject
#define classobject PyClassObject
#define codeobject PyCodeObject
#define frameobject PyFrameObject
#define funcobject PyFunctionObject
#define instancemethodobject PyMethodObject
#define instanceobject PyInstanceObject
#define methodobject PyCFunctionObject
#define moduleobject PyModuleObject
#define number_methods PyNumberMethods
#define sequence_methods PySequenceMethods
#define mapping_methods PyMappingMethods
#define OB_HEAD PyObject_HEAD
#define OB_VARHEAD PyObject_VAR_HEAD
#define OB_HEAD_INIT PyObject_HEAD_INIT
#define NEWOBJ PyObject_NEW
#define NEWVAROBJ PyObject_NEW_VAR
#define PROTO Py_PROTO
#define FPROTO Py_FPROTO
#define NEW PyMem_NEW
#define RESIZE PyMem_RESIZE
#define DEL PyMem_DEL
#define XDEL PyMem_XDEL
#define BGN_SAVE Py_BEGIN_ALLOW_THREADS
#define RET_SAVE Py_BLOCK_THREADS
#define RES_SAVE Py_UNBLOCK_THREADS
#define END_SAVE Py_END_ALLOW_THREADS
#define is_floatobject PyFloat_Check
#define is_intobject PyInt_Check
#define is_longobject PyLong_Check
#define is_noobject PyNothing_Check
#define is_stringobject PyString_Check
#define is_typeobject PyType_Check
#define is_listobject PyList_Check
#define is_dictobject PyDict_Check
#define is_mappingobject PyDict_Check
#define is_tupleobject PyTuple_Check
#define is_fileobject PyFile_Check
#define is_classobject PyClass_Check
#define is_codeobject PyCode_Check
#define is_frameobject PyFrame_Check
#define is_funcobject PyFunction_Check
#define is_instancemethodobject PyMethod_Check
#define is_instanceobject PyInstance_Check
#define is_methodobject PyCFunction_Check
#define is_moduleobject PyModule_Check
#define INCREF Py_INCREF
#define DECREF Py_DECREF
#define XINCREF Py_XINCREF
#define XDECREF Py_XDECREF
#define NEWREF _Py_NewReference
#define DELREF _Py_Dealloc
#define UNREF _Py_ForgetReference
#define cmpobject PyObject_Compare
#define getattr PyObject_GetAttrString
#define getattro PyObject_GetAttr
#define hasattr PyObject_HasAttrString
#define hasattro PyObject_HasAttr
#define hashobject PyObject_Hash
#define newobject _PyObject_New
#define newvarobject _PyObject_NewVar
#define printobject PyObject_Print
#define reprobject PyObject_Repr
#define strobject PyObject_Str
#define setattr PyObject_SetAttrString
#define setattro PyObject_SetAttr
#define testbool PyObject_IsTrue
#define PRINT_RAW Py_PRINT_RAW
#define float_buf_repr PyFloat_AsString
#define getfloatvalue PyFloat_AsDouble
#define GETFLOATVALUE PyFloat_AS_DOUBLE
#define newfloatobject PyFloat_FromDouble
#define getintvalue PyInt_AsLong
#define GETINTVALUE PyInt_AS_LONG
#define getmaxint PyInt_GetMax
#define newintobject PyInt_FromLong
#define alloclongobject _PyLong_New
#define dgetlongvalue PyLong_AsDouble
#define dnewlongobject PyLong_FromDouble
#define getlongvalue PyLong_AsLong
#define long_escan PyLong_FromString
#define long_scan(a, b) PyLong_FromString((a), (char **)0, (b))
#define newlongobject PyLong_FromLong
#define formatstring PyString_Format
#define getstringsize PyString_Size
#define getstringvalue PyString_AsString
#define GETSTRINGVALUE PyString_AS_STRING
#define joinstring PyString_Concat
#define joinstring_decref PyString_ConcatAndDel
#define newsizedstringobject PyString_FromStringAndSize
#define newstringobject PyString_FromString
#define resizestring _PyString_Resize
#define addlistitem PyList_Append
#define getlistitem PyList_GetItem
#define GETLISTITEM PyList_GET_ITEM
#define getlistsize PyList_Size
#define getlistslice PyList_GetSlice
#define inslistitem PyList_Insert
#define newlistobject PyList_New
#define setlistitem PyList_SetItem
#define setlistslice PyList_SetSlice
#define sortlist PyList_Sort
#define reverselist PyList_Reverse
#define listtuple PyList_AsTuple
#define dictinsert PyDict_SetItemString
#define dictlookup PyDict_GetItemString
#define dictremove PyDict_DelItemString
#define getmappingitems PyDict_Items
#define getdictitems PyDict_Items
#define getmappingkeys PyDict_Keys
#define getdictkeys PyDict_Keys
#define getmappingvalues PyDict_Values
#define getdictvalues PyDict_Values
#define getmappingsize PyDict_Size
#define getdictsize PyDict_Size
#define mappingclear PyDict_Clear
#define mappinggetnext PyDict_Next
#define mappinginsert PyDict_SetItem
#define dict2insert PyDict_SetItem
#define mappinglookup PyDict_GetItem
#define dict2lookup PyDict_GetItem
#define mappingremove PyDict_DelItem
#define dict2remove PyDict_DelItem
#define newmappingobject PyDict_New
#define newdictobject PyDict_New
#define gettupleitem PyTuple_GetItem
#define GETTUPLEITEM PyTuple_GET_ITEM
#define gettuplesize PyTuple_Size
#define gettupleslice PyTuple_GetSlice
#define newtupleobject PyTuple_New
#define settupleitem PyTuple_SetItem
#define resizetuple _PyTuple_Resize
#define filegetline PyFile_GetLine
#define getfilefile PyFile_AsFile
#define getfilename PyFile_Name
#define setfilebufsize PyFile_SetBufSize
#define newfileobject PyFile_FromString
#define newopenfileobject PyFile_FromFile
#define softspace PyFile_SoftSpace
#define writeobject PyFile_WriteObject
#define writestring PyFile_WriteString
#define instancemethodgetclass PyMethod_Class
#define instancemethodgetfunc PyMethod_Function
#define instancemethodgetself PyMethod_Self
#define issubclass PyClass_IsSubclass
#define newclassobject PyClass_New
#define newinstancemethodobject PyMethod_New
#define newinstanceobject PyInstance_New
#define instancebinop PyInstance_DoBinOp
#define block PyTryBlock
#define extend_stack PyFrame_ExtendStack
#define newframeobject PyFrame_New
#define pop_block PyFrame_BlockPop
#define setup_block PyFrame_BlockSetup
#define fast_2_locals PyFrame_FastToLocals
#define locals_2_fast PyFrame_LocalsToFast
#define getfunccode PyFunction_GetCode
#define getfuncglobals PyFunction_GetGlobals
#define getfuncargstuff PyFunction_GetArgStuff
#define setfuncargstuff PyFunction_SetArgStuff
#define mystrtol PyOS_strtol
#define mystrtoul PyOS_strtoul
#define newfuncobject PyFunction_New
#define newrangeobject PyRange_New
#define method PyCFunction
#define findmethod Py_FindMethod
#define findmethodinchain Py_FindMethodInChain
#define getmethod PyCFunction_GetFunction
#define getself PyCFunction_GetSelf
#define getvarargs PyCFunction_IsVarArgs
#define newmethodobject PyCFunction_New
#define getmoduledict PyModule_GetDict
#define getmodulename PyModule_GetName
#define newmoduleobject PyModule_New
#define addaccelerators PyGrammar_AddAccelerators
#define finddfa PyGrammar_FindDFA
#define labelrepr PyGrammar_LabelRepr
#define listtree PyNode_ListTree
#define addchild PyNode_AddChild
#define freetree PyNode_Free
#define newtree PyNode_New
#define addtoken PyParser_AddToken
#define delparser PyParser_Delete
#define newparser PyParser_New
#define parsefile PyParser_ParseFile
#define parsestring PyParser_ParseString
#define tok_1char PyToken_OneChar
#define tok_2char PyToken_TwoChars
#define tok_free PyTokenizer_Free
#define tok_get PyTokenizer_Get
#define tok_setupf PyTokenizer_FromFile
#define tok_setups PyTokenizer_FromString
#define compile PyNode_Compile
#define newcodeobject PyCode_New
#define call_object PyEval_CallObject
#define eval_code PyEval_EvalCode
#define flushline Py_FlushLine
#define getbuiltins PyEval_GetBuiltins
#define getglobals PyEval_GetGlobals
#define getlocals PyEval_GetLocals
#define getowner PyEval_GetOwner
#define getframe PyEval_GetFrame
#define getrestricted PyEval_GetRestricted
#define init_save_thread PyEval_InitThreads
#define printtraceback PyErr_PrintTraceBack
#define restore_thread PyEval_RestoreThread
#define save_thread PyEval_SaveThread
#define tb_fetch PyTraceBack_Fetch
#define tb_here PyTraceBack_Here
#define tb_print PyTraceBack_Print
#define tb_store PyTraceBack_Store
#define add_module PyImport_AddModule
#define doneimport PyImport_Cleanup
#define get_modules PyImport_GetModuleDict
#define get_pyc_magic PyImport_GetMagicNumber
#define import_module PyImport_ImportModule
#define init_frozen PyImport_ImportFrozenModule
#define initimport PyImport_Init
#define reload_module PyImport_ReloadModule
#define coerce PyNumber_Coerce
#define getbuiltin PyBuiltin_GetObject
#define initbuiltin PyBuiltin_Init
#define initmarshal PyMarshal_Init
#define initmodule Py_InitModule
#define initmodule4 Py_InitModule4
#define rd_long PyMarshal_ReadLongFromFile
#define rd_short PyMarshal_ReadShortFromFile
#define rd_object PyMarshal_ReadObjectFromFile
#define rds_object PyMarshal_ReadObjectFromString
#define wr_long PyMarshal_WriteLongToFile
#define wr_short PyMarshal_WriteShortToFile
#define wr_object PyMarshal_WriteObjectToFile
#define initsys PySys_Init
#define setpythonargv PySys_SetArgv
#define setpythonpath PySys_SetPath
#define sysget PySys_GetObject
#define sysgetfile PySys_GetFile
#define sysset PySys_SetObject
#define compile_string Py_CompileString
#define fatal Py_FatalError
#define goaway Py_Exit
#define cleanup Py_Cleanup
#define initall Py_Initialize
#define print_error PyErr_Print
#define parse_file PyParser_SimpleParseFile
#define parse_string PyParser_SimpleParseString
#define run PyRun_AnyFile
#define run_script PyRun_SimpleFile
#define run_command PyRun_SimpleString
#define run_file PyRun_File
#define run_string PyRun_String
#define run_tty_1 PyRun_InteractiveOne
#define run_tty_loop PyRun_InteractiveLoop
#define getmember PyMember_Get
#define setmember PyMember_Set
#define mkvalue Py_BuildValue
#define vmkvalue Py_VaBuildValue
#define getargs PyArg_Parse
#define vgetargs PyArgs_VaParse
#define newgetargs PyArg_ParseTuple
#define getichararg PyArg_GetChar
#define getidoublearray PyArg_GetDoubleArray
#define getifloatarg PyArg_GetFloat
#define getifloatarray PyArg_GetFloatArray
#define getnoarg(v) PyArg_NoArgs(v)
#define getintarg(v,a) getargs((v),"i",(a))
#define getlongarg(v,a) getargs((v),"l",(a))
#define getstrarg(v,a) getargs((v),"s",(a))
#define getilongarg PyArg_GetLong
#define getilongarray PyArg_GetLongArray
#define getilongarraysize PyArg_GetLongArraySize
#define getiobjectarg PyArg_GetObject
#define getishortarg PyArg_GetShort
#define getishortarray PyArg_GetShortArray
#define getishortarraysize PyArg_GetShortArraySize
#define getistringarg PyArg_GetString
#define err_badarg PyErr_BadArgument
#define err_badcall PyErr_BadInternalCall
#define err_input PyErr_Input
#define err_nomem PyErr_NoMemory
#define err_errno PyErr_SetFromErrno
#define err_set PyErr_SetNone
#define err_setstr PyErr_SetString
#define err_setval PyErr_SetObject
#define err_occurred PyErr_Occurred
#define err_fetch PyErr_Fetch
#define err_restore PyErr_Restore
#define err_clear PyErr_Clear
#define fgets_intr PyOS_InterruptableGetString
#define initintr PyOS_InitInterrupts
#define intrcheck PyOS_InterruptOccurred
#define getmtime PyOS_GetLastModificationTime
#define my_readline PyOS_Readline
#define realmain Py_Main
#define ref_total _Py_RefTotal
#define sigcheck PyErr_CheckSignals

#ifdef __cplusplus
}
#endif
#endif /* !Py_OLDNAMES_H */
