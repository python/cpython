/* RISCOS module implementation */

#include "h.osfscontrol"
#include "h.osgbpb"
#include "h.os"
#include "h.osfile"

#include "Python.h"

#include <errno.h>

static os_error *e;

static PyObject *RiscosError; /* Exception riscos.error */

static PyObject *riscos_oserror(void)
{ PyErr_SetString(RiscosError,e->errmess);
  return 0;
}

static PyObject *riscos_error(char *s) { PyErr_SetString(RiscosError,s);return 0;}

/* RISCOS file commands */

static PyObject *riscos_remove(PyObject *self,PyObject *args)
{       char *path1;
	if (!PyArg_Parse(args, "s", &path1)) return NULL;
	if (remove(path1)) return PyErr_SetFromErrno(RiscosError);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *riscos_rename(PyObject *self,PyObject *args)
{	char *path1, *path2;
	if (!PyArg_Parse(args, "(ss)", &path1, &path2)) return NULL;
	if (rename(path1,path2)) return PyErr_SetFromErrno(RiscosError);;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *riscos_system(PyObject *self,PyObject *args)
{	char *command;
	if (!PyArg_Parse(args, "s", &command)) return NULL;
	return PyInt_FromLong(system(command));
}

static PyObject *riscos_chdir(PyObject *self,PyObject *args)
{	char *path;
	if (!PyArg_Parse(args, "s", &path)) return NULL;
	e=xosfscontrol_dir(path);
	if(e) return riscos_oserror();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *canon(char *path)
{ int len;
  PyObject *obj;
  char *buf;
  e=xosfscontrol_canonicalise_path(path,0,0,0,0,&len);
  if(e) return riscos_oserror();
  obj=PyString_FromStringAndSize(NULL,-len);
  if(obj==NULL) return NULL;
  buf=PyString_AsString(obj);
  e=xosfscontrol_canonicalise_path(path,buf,0,0,1-len,&len);
  if(len!=1) return riscos_error("Error expanding path");
  if(!e) return obj;
  Py_DECREF(obj);
  return riscos_oserror();
}

static PyObject *riscos_getcwd(PyObject *self,PyObject *args)
{ if(!PyArg_NoArgs(args)) return NULL;
  return canon("@");
}

static PyObject *riscos_expand(PyObject *self,PyObject *args)
{	char *path;
	if (!PyArg_Parse(args, "s", &path)) return NULL;
        return canon(path);
}

static PyObject *riscos_mkdir(PyObject *self,PyObject *args)
{	char *path;
        int mode;
        if (!PyArg_ParseTuple(args, "s|i", &path, &mode)) return NULL;
        e=xosfile_create_dir(path,0);
        if(e) return riscos_oserror();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *riscos_listdir(PyObject *self,PyObject *args)
{	char *path,buf[256];
        PyObject *d, *v;
        int c=0,count;
	if (!PyArg_Parse(args, "s", &path)) return NULL;
	d=PyList_New(0);
	if(!d) return NULL;
	for(;;)
	{ e=xosgbpb_dir_entries(path,(osgbpb_string_list*)buf,
	                             1,c,256,0,&count,&c);
	  if(e)
	  { Py_DECREF(d);return riscos_oserror();
	  }
	  if(count)
	  { v=PyString_FromString(buf);
	    if(!v) { Py_DECREF(d);return 0;}
	    if(PyList_Append(d,v)) {Py_DECREF(d);Py_DECREF(v);return 0;}
	  }
	  if(c==-1) break;
	}
	return d;
}

static PyObject *riscos_stat(PyObject *self,PyObject *args)
{	char *path;
        int ob,len;
        bits t=0;
        bits ld,ex,at,ft,mode;
	if (!PyArg_Parse(args, "s", &path)) return NULL;
	e=xosfile_read_stamped_no_path(path,&ob,&ld,&ex,&len,&at,&ft);
	if(e) return riscos_oserror();
	switch (ob)
	{ case osfile_IS_FILE:mode=0100000;break;  /* OCTAL */
	  case osfile_IS_DIR:mode=040000;break;
	  case osfile_IS_IMAGE:mode=0140000;break;
	  default:return riscos_error("Not found");
	}
	if(ft!=-1) t=unixtime(ld,ex);
	mode|=(at&7)<<6;
	mode|=((at&112)*9)>>4;
        return Py_BuildValue("(lllllllllllll)",
		    (long)mode,/*st_mode*/
		    0,/*st_ino*/
		    0,/*st_dev*/
		    0,/*st_nlink*/
		    0,/*st_uid*/
		    0,/*st_gid*/
		    (long)len,/*st_size*/
		    (long)t,/*st_atime*/
		    (long)t,/*st_mtime*/
		    (long)t,/*st_ctime*/
		    (long)ft,/*file type*/
		    (long)at,/*attributes*/
		    (long)ob/*object type*/
		    );
}

static PyObject *riscos_chmod(PyObject *self,PyObject *args)
{	char *path;
        bits mode;
        bits attr;
        attr=(mode&0x700)>>8;
        attr|=(mode&7)<<4;
	if (!PyArg_Parse(args, "(si)", &path,(int*)&mode)) return NULL;
        e=xosfile_write_attr(path,attr);
        if(e) return riscos_oserror();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *riscos_utime(PyObject *self,PyObject *args)
{	char *path;
        int x,y;
	if (!PyArg_Parse(args, "(s(ii))", &path,&x,&y)) return NULL;
        e=xosfile_stamp(path);
        if(e) return riscos_oserror();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *riscos_settype(PyObject *self,PyObject *args)
{	char *path,*name;
        int type;
	if (!PyArg_Parse(args, "(si)", &path,&type))
	{ PyErr_Clear();
	  if (!PyArg_Parse(args, "(ss)", &path,&name)) return NULL;
	  e=xosfscontrol_file_type_from_string(name,(bits*)&type);
	  if(e) return riscos_oserror();
	}
        e=xosfile_set_type(path,type);
        if(e) return riscos_oserror();
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *riscos_getenv(PyObject *self,PyObject *args)
{ char *name,*value;
  if(!PyArg_Parse(args,"s",&name)) return NULL;
  value=getenv(name);
  if(value) return PyString_FromString(value);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *riscos_putenv(PyObject *self,PyObject *args)
{ char *name,*value;
  int len;
  os_var_type type=os_VARTYPE_LITERAL_STRING;
  if(!PyArg_ParseTuple(args,"ss|i",&name,&value,&type)) return NULL;
  if(type!=os_VARTYPE_STRING&&type!=os_VARTYPE_MACRO&&type!=os_VARTYPE_EXPANDED
                            &&type!=os_VARTYPE_LITERAL_STRING)
    return riscos_error("Bad putenv type");
  len=strlen(value);
  if(type!=os_VARTYPE_LITERAL_STRING) len++;
                          /* Other types need null terminator! */
  e=xos_set_var_val(name,(byte*)value,len,0,type,0,0);
  if(e) return riscos_oserror();
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *riscos_delenv(PyObject *self,PyObject *args)
{ char *name;
  if(!PyArg_Parse(args,"s",&name)) return NULL;
  e=xos_set_var_val(name,NULL,-1,0,0,0,0);
  if(e) return riscos_oserror();
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *riscos_getenvdict(PyObject *self,PyObject *args)
{ PyObject *dict;
  char value[257];
  char *which="*";
  int size;
  char *context=NULL;
  if(!PyArg_ParseTuple(args,"|s",&which)) return NULL;
  dict = PyDict_New();
  if (!dict) return NULL;
  /* XXX This part ignores errors */
  while(!xos_read_var_val(which,value,sizeof(value)-1,(int)context,
         os_VARTYPE_EXPANDED,&size,(int *)&context,0))
  { PyObject *v;
    value[size]='\0';
    v = PyString_FromString(value);
    if (v == NULL) continue;
    PyDict_SetItemString(dict, context, v);
    Py_DECREF(v);
  }
  return dict;
}

static PyMethodDef riscos_methods[] = {

	{"unlink",	riscos_remove},
        {"remove",      riscos_remove},
	{"rename",	riscos_rename},
	{"system",	riscos_system},
	{"rmdir",	riscos_remove},
	{"chdir",	riscos_chdir},
	{"getcwd",	riscos_getcwd},
	{"expand",      riscos_expand},
	{"mkdir",	riscos_mkdir,1},
	{"listdir",	riscos_listdir},
	{"stat",	riscos_stat},
	{"lstat",	riscos_stat},
        {"chmod",	riscos_chmod},
	{"utime",	riscos_utime},
	{"settype",	riscos_settype},
	{"getenv",      riscos_getenv},
	{"putenv",      riscos_putenv},
	{"delenv",      riscos_delenv},
	{"getenvdict",  riscos_getenvdict,1},
	{NULL,		NULL}		 /* Sentinel */
};



void
initriscos()
{
	PyObject *m, *d;

	m = Py_InitModule("riscos", riscos_methods);
	d = PyModule_GetDict(m);

	/* Initialize riscos.error exception */
	RiscosError = PyString_FromString("riscos.error");
	if (RiscosError == NULL || PyDict_SetItemString(d, "error", RiscosError) != 0)
		Py_FatalError("can't define riscos.error");
}
