/* drawf  DrawFile functions */

#include "oslib/os.h"
#include "oslib/osfile.h"
#include "oslib/drawfile.h"
#include "Python.h"

#include <limits.h>

#define PyDrawF_Check(op) ((op)->ob_type == &PyDrawFType)
#define HDRSIZE 40
#define GRPHDRSIZE 36
#define DRAWTYPE 0xaff
#define NEXT(d) ((drawfile_object*)((char*)(d)+(d)->size))

typedef struct
{ PyObject_HEAD
  drawfile_diagram *drawf;
  int size; /*length in bytes*/
  int nobjs;  /* no of objects */
} PyDrawFObject;

typedef struct dheader
{  char tag [4];
   int major_version;
   int minor_version;
   char source [12];
   os_box bbox;
} dheader;

static PyObject *DrawFError; /* Exception drawf.error */
static os_error *e;
static PyTypeObject PyDrawFType;

static dheader header=
{ {'D','r','a','w'},
  201,0,
  {'P','y','t','h','o','n',' ',' ',' ',' ',' ',' '},
  {INT_MAX,INT_MAX,INT_MIN,INT_MIN}
};

static PyObject *drawf_oserror(void)
{ PyErr_SetString(DrawFError,e->errmess);
  return 0;
}

static PyObject *drawf_error(char *s)
{ PyErr_SetString(DrawFError,s);
  return 0;
}

/* DrawFile commands */

static void countobjs(PyDrawFObject *pd)
{ int k=0,q;
  drawfile_diagram *dd=pd->drawf;
  drawfile_object *d=dd->objects;
  char *end=(char*)dd+pd->size;
  pd->nobjs=-1;
  while((char*)d<end)
  { k++;
    q=d->size;
    if(q<=0||q&3) return ;
    d=NEXT(d);
  }
  if ((char*)d==end) pd->nobjs=k;
}

static drawfile_object *findobj(PyDrawFObject *pd,int n)
{ drawfile_diagram *dd=pd->drawf;
  drawfile_object *d=dd->objects;
  for(;n>0;n--) d=NEXT(d);
  return d;
}

static PyDrawFObject* new(int size)
{ PyDrawFObject *b=PyObject_NEW(PyDrawFObject,&PyDrawFType);
  if(!b) return NULL;
  size+=HDRSIZE;
  b->drawf=malloc(size);
  if(!b->drawf)
  { Py_DECREF(b);
    return (PyDrawFObject*)PyErr_NoMemory();
  }
  b->size=size;
  return b;
}

static void extend(os_box *b,os_box *c)
{ if(b->x0>c->x0) b->x0=c->x0;
  if(b->y0>c->y0) b->y0=c->y0;
  if(b->x1<c->x1) b->x1=c->x1;
  if(b->y1<c->y1) b->y1=c->y1;
}

static PyObject *DrawF_New(PyObject *self,PyObject *args)
{ PyDrawFObject *b;
  if(!PyArg_ParseTuple(args,"")) return NULL;
  b=new(0);
  if(!b) return NULL;
  *((dheader*)b->drawf)=header;
  b->nobjs=0;
  return (PyObject *)b;
}

static PyObject *DrawF_Load(PyObject *self,PyObject *args)
{ int size;
  char *fname;
  PyDrawFObject *b;
  fileswitch_object_type ot;
  if(!PyArg_ParseTuple(args,"s",&fname)) return NULL;
  e=xosfile_read_no_path(fname,&ot,0,0,&size,0);
  if(e) return drawf_oserror();
  size-=HDRSIZE;
  if(ot!=osfile_IS_FILE||size<0||size&3) return drawf_error("Bad draw file");
  b=new(size);
  if(!b) return NULL;
  e=xosfile_load_stamped_no_path(fname,(byte*)(b->drawf),0,0,0,0,0);
  if(e)
  { Py_DECREF(b);
    return drawf_oserror();
  }
  countobjs(b);
  if(b->nobjs>=0) return (PyObject *)b;
  Py_DECREF(b);
  return drawf_error("Corrupt draw file");
}


static PyObject *DrawF_Save(PyDrawFObject *self,PyObject *arg)
{ char *fname;
  if(!PyArg_ParseTuple(arg,"s",&fname)) return NULL;
  e=xosfile_save_stamped(fname,DRAWTYPE,
  (byte*)(self->drawf),(byte*)(self->drawf)+self->size);
  if (e) return drawf_oserror();
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *DrawF_Render(PyDrawFObject *self,PyObject *arg)
{ int flags,trans,clip,flat;
  if(!PyArg_ParseTuple(arg,"iiii",&flags,&trans,&clip,&flat)) return NULL;
  e=xdrawfile_render((drawfile_render_flags)flags,self->drawf,self->size,
  (os_trfm*)trans,(os_box*)clip,flat);
  if(e) return drawf_oserror();
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *DrawF_Path(PyDrawFObject *self,PyObject *arg)
{ PyObject *pl;
  PyObject *dp=0;
  os_colour fill;
  os_colour outline;
  int width,start=0;
  drawfile_path_style style;
  int size=40;
  int n,i,element_count;
  drawfile_diagram *diag;
  drawfile_object *dobj;
  drawfile_path *dpath;
  draw_path *thepath;
  draw_line_style line_style;
  draw_dash_pattern *dash_pattern=0;
  os_box *box;
  long *pe;
  if(!PyArg_ParseTuple(arg,"O!(iiii)|O!i",&PyList_Type,&pl,(int*)&fill,
     (int*)&outline,&width,(int*)&style,&PyTuple_Type,&dp,&start)) return NULL;
  if(dp)
  { element_count=PyTuple_Size(dp);
    size+=4*element_count+8;
    style.flags|=drawfile_PATH_DASHED;
  }
  else style.flags&=~drawfile_PATH_DASHED;
  n=PyList_Size(pl);
  size+=4*n+8;
  for(i=0;i<n;i++) if(!PyInt_Check(PyList_GetItem(pl,i))) size+=4;
  diag=realloc(self->drawf,self->size+size);
  if(!diag) return PyErr_NoMemory();
  self->drawf=diag;
  dobj=(drawfile_object*)((char*)diag+self->size);
  dobj->type=drawfile_TYPE_PATH;
  dobj->size=size;
  dpath=&dobj->data.path;
  self->size+=size;
  self->nobjs++;
  box=&dpath->bbox;
  dpath->fill=fill;dpath->outline=outline;
  dpath->width=width;dpath->style=style;
  pe=(long*)&(dpath->path);
  if(dp)
  { dash_pattern=&(((drawfile_path_with_pattern*)dpath)->pattern);
    dash_pattern->start=start;
    dash_pattern->element_count=element_count;
    for(i=0;i<element_count;i++)
    { dash_pattern->elements[i]=(int)PyInt_AsLong(PyTuple_GetItem(dp,i));
    }
    pe+=element_count+2;
  }
  thepath=(draw_path*)pe;
  *pe++=draw_MOVE_TO;
  for(i=0;i<n;i++)
  { PyObject *p=PyList_GetItem(pl,i);
    if(PyInt_Check(p))
      *pe++=PyInt_AsLong(p);
    else
    {
      *pe++=PyInt_AsLong(PyTuple_GetItem(p,0));
      *pe++=PyInt_AsLong(PyTuple_GetItem(p,1));
    }
  }
  *pe=draw_END_PATH;
  line_style.join_style=style.flags&3;
  line_style.end_cap_style=(style.flags&3)>>2;
  line_style.start_cap_style=(style.flags&3)>>4;
  line_style.reserved=0;
  line_style.mitre_limit=10;
  line_style.start_cap_width=style.cap_width;
  line_style.end_cap_width=style.cap_width;
  line_style.start_cap_length=style.cap_length;
  line_style.end_cap_length=style.cap_length;
  e=xdraw_process_path(thepath,0x70000000,0,0,width,&line_style,dash_pattern,
  (draw_output_path)((char*)box+0x80000000),0);
  if(e) return drawf_oserror();

  /* draw_process_path seems to have a bug:
     If the bounding box size is zero, it returns 0x7FFFFFFF, ..., 0x80000000 instead of the
     correct size. */
  if (box->x0==0x7FFFFFFF)
  {
      /* path has zero extension, so forget it; it would be invisible anyway */
      self->size-=size;
      self->nobjs--;
      diag=realloc(self->drawf,self->size);
      if(!diag) return PyErr_NoMemory();
      self->drawf=diag;
  }
  else
      extend(&(diag->bbox),box);
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *DrawF_Text(PyDrawFObject *self,PyObject *arg)
{ os_colour fill,bg_hint;
  drawfile_text_style style;
  int xsize,ysize,x,y;
  int size,len;
  char *text;
  drawfile_diagram *diag;
  drawfile_object *dobj;
  drawfile_text *dtext;
  os_box *box;
  if(!PyArg_ParseTuple(arg,"(ii)s(iiiii)",&x,&y,&text,
     (int*)&fill,(int*)&bg_hint,(int*)&style,&xsize,&ysize)) return NULL;
  len=strlen(text);
  size=((len+4)&(~3))+52;
  diag=realloc(self->drawf,self->size+size);
  if(!diag) return PyErr_NoMemory();
  self->drawf=diag;
  dobj=(drawfile_object*)((char*)diag+self->size);
  dobj->type=drawfile_TYPE_TEXT;
  dobj->size=size;
  dtext=&dobj->data.text;
  self->size+=size;
  self->nobjs++;
  dtext->fill=fill;
  dtext->bg_hint=bg_hint;
  dtext->style=style;
  dtext->xsize=xsize;
  dtext->ysize=ysize;
  dtext->base.x=x;
  dtext->base.y=y;
  memset(dtext->text,0,(len+4)&(~3));
  sprintf(dtext->text,"%s",text);
  box=&(dtext->bbox);
  box->x0=x;box->y0=y;box->x1=x+len*xsize;box->y1=y+ysize;
  extend(&(diag->bbox),box);
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *DrawF_TText(PyDrawFObject *self,PyObject *arg)
{ os_colour fill,bg_hint;
  drawfile_text_style style;
  int xsize,ysize,x,y;
  int t1,t2,t3,t4,t5,t6;
  int size,len;
  char *text;
  drawfile_diagram *diag;
  drawfile_object *dobj;
  drawfile_trfm_text *dtext;
  os_box *box;
  t1=1<<16;t2=0;
  t3=0;t4=1<<16;
  t5=t6=0;
  if(!PyArg_ParseTuple(arg,"(ii)s(iiiii)|(iiiiii)",&x,&y,&text,
     (int*)&fill,(int*)&bg_hint,(int*)&style,&xsize,&ysize,&t1,&t2,&t3,&t4,&t5,&t6)) return NULL;
  len=strlen(text);
  size=((len+4)&(~3))+52+28;
  diag=realloc(self->drawf,self->size+size);
  if(!diag) return PyErr_NoMemory();
  self->drawf=diag;
  dobj=(drawfile_object*)((char*)diag+self->size);
  dobj->type=drawfile_TYPE_TRFM_TEXT;
  dobj->size=size;
  dtext=&dobj->data.trfm_text;
  self->size+=size;
  self->nobjs++;
  dtext->trfm.entries[0][0]=t1;
  dtext->trfm.entries[0][1]=t2;
  dtext->trfm.entries[1][0]=t3;
  dtext->trfm.entries[1][1]=t4;
  dtext->trfm.entries[2][0]=t5;
  dtext->trfm.entries[2][1]=t6;
  dtext->flags=0;
  dtext->fill=fill;
  dtext->bg_hint=bg_hint;
  dtext->style=style;
  dtext->xsize=xsize;
  dtext->ysize=ysize;
  dtext->base.x=x;
  dtext->base.y=y;
  memset(dtext->text,0,(len+4)&(~3));
  sprintf(dtext->text,"%s",text);
  box=&(dtext->bbox);
  box->x0=x;box->y0=y;box->x1=x+len*xsize;box->y1=y+ysize;
  extend(&(diag->bbox),box);
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *DrawF_FontTable(PyDrawFObject *self,PyObject *arg)
{ int size=8,n=0;
  PyObject *d,*key,*value;
  drawfile_diagram *diag;
  drawfile_object *dobj;
  char *dtable;
  if(!PyArg_ParseTuple(arg,"O!",&PyDict_Type,&d)) return NULL;
  while(PyDict_Next(d,&n,&key,&value))
  { int m=PyString_Size(value);
    if(m<0||!PyInt_Check(key)) return NULL;
    size+=m+2;
  }
  size=(size+3)&(~3);
  diag=realloc(self->drawf,self->size+size);
  if(!diag) return PyErr_NoMemory();
  self->drawf=diag;
  dobj=(drawfile_object*)((char*)diag+self->size);
  dobj->type=drawfile_TYPE_FONT_TABLE;
  dobj->size=size;
  dtable=(char*)(&dobj->data.font_table);
  self->size+=size;
  self->nobjs++;
  memset(dtable,0,size-8);
  n=0;
  while(PyDict_Next(d,&n,&key,&value))
  { int m=PyString_Size(value);
    *dtable=(char)PyInt_AsLong(key);
    strcpy(dtable+1,PyString_AsString(value));
    dtable+=m+2;
  }
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *DrawF_Group(PyDrawFObject *self,PyObject *arg)
{ int size,n;
  PyDrawFObject *g;
  char *name="";
  drawfile_diagram *diag;
  drawfile_object *dobj;
  drawfile_group *dgroup;
  if(!PyArg_ParseTuple(arg,"O!|s",&PyDrawFType,(PyObject*)&g,&name))
       return NULL;
  size=g->size-4;
  diag=realloc(self->drawf,self->size+size);
  if(!diag) return PyErr_NoMemory();
  self->drawf=diag;
  self->nobjs++;
  dobj=(drawfile_object*)((char*)diag+self->size);
  self->size+=size;
  dobj->type=drawfile_TYPE_GROUP;
  dobj->size=g->size-4;
  dgroup=&dobj->data.group;
  dgroup->bbox=g->drawf->bbox;
  memset(dgroup->name,' ',12);
  n=strlen(name);
  if(n>12) n=12;
  memcpy(dgroup->name,name,n);
  memcpy((char*)dgroup->objects,(char*)g->drawf+40,g->size-40);
  extend(&(diag->bbox),&(dgroup->bbox));
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *DrawF_Find(PyDrawFObject *self,PyObject *arg)
{ int x,y,n=0;
  int r=-1;
  drawfile_object *d;
  if(!PyArg_ParseTuple(arg,"ii|i",&x,&y,&n)) return NULL;
  if(n<self->nobjs&&n>=0)
  { d=findobj(self,n);
    while(n<self->nobjs)
    { if(x>=d->data.text.bbox.x0&&x<=d->data.text.bbox.x1&&
         y>=d->data.text.bbox.y0&&y<=d->data.text.bbox.y1) { r=n;break;}
      n++;
      d=NEXT(d);
    }
  }
  return PyInt_FromLong(r);
}


static PyObject *DrawF_Box(PyDrawFObject *self,PyObject *arg)
{ int n=-1;
  os_box *box;
  if(!PyArg_ParseTuple(arg,"|i",&n)) return NULL;
  if(n>=self->nobjs|n<0) box=&(self->drawf->bbox);
  else box=&(findobj(self,n)->data.text.bbox);
  return Py_BuildValue("iiii",box->x0,box->y0,box->x1,box->y1);
}

static void SetBlock(drawfile_object *d,int size,int type,int offset,int value)
{ char *end=(char*)d+size;
  printf("s%d t%d o%d v%d\n",size,type,offset,value);
  for(;(char*)d<end;d=NEXT(d))
    if(d->type==type) ((int*)(d))[offset]=value;
    else if(d->type==drawfile_TYPE_GROUP)
           SetBlock((drawfile_object*)&d->data.group.objects,
                    d->size,type,offset,value);
  printf("SetBlock Done\n");
}

static PyObject *SetWord(PyDrawFObject *self,PyObject *arg,int type,int offset)
{ int n=PyTuple_Size(arg);
  int m,value,q;
  PyObject *par;
  drawfile_object *e,*d=self->drawf->objects;
  if(n==0) return  drawf_error("Value Required");
  par=PyTuple_GetItem(arg,0);
  if(!PyInt_Check(par))
  { PyErr_SetString(PyExc_TypeError,"Int Required");
    return 0;
  }
  value=(int)PyInt_AsLong(par);
  if(n==1) SetBlock(d,self->size-HDRSIZE,type,offset,value);
  else
  { for(m=1;m<n;m++)
    { par=PyTuple_GetItem(arg,m);
      if(!PyInt_Check(par))
      { PyErr_SetString(PyExc_TypeError,"Int Required");
        return 0;
      }
      q=(int)PyInt_AsLong(par);
      if(q<0||q>=self->nobjs)
      { PyErr_SetString(PyExc_ValueError,"Object out of range");
        return 0;
      }
      e=d;
      for(;q>0;q--) e=NEXT(e);
      if(e->type==type)
      { ((int*)(e))[offset]=value;
      }
      else if(e->type==drawfile_TYPE_GROUP)
             SetBlock((drawfile_object*)&e->data.group.objects,
                      e->size-GRPHDRSIZE,type,offset,value);
    }
  }
  Py_INCREF(Py_None);return Py_None;
}

static PyObject *DrawF_PathFill(PyDrawFObject *self,PyObject *arg)
{ return SetWord(self,arg,drawfile_TYPE_PATH,6);
}

static PyObject *DrawF_PathColour(PyDrawFObject *self,PyObject *arg)
{ return SetWord(self,arg,drawfile_TYPE_PATH,7);
}

static PyObject *DrawF_TextColour(PyDrawFObject *self,PyObject *arg)
{ return SetWord(self,arg,drawfile_TYPE_TEXT,6);
}

static PyObject *DrawF_TextBackground(PyDrawFObject *self,PyObject *arg)
{ return SetWord(self,arg,drawfile_TYPE_TEXT,7);
}

static struct PyMethodDef PyDrawF_Methods[]=
{
  { "render",(PyCFunction)DrawF_Render,1},
  { "save",(PyCFunction)DrawF_Save,1},
  { "path",(PyCFunction)DrawF_Path,1},
  { "text",(PyCFunction)DrawF_Text,1},
  { "ttext",(PyCFunction)DrawF_TText,1},
  { "fonttable",(PyCFunction)DrawF_FontTable,1},
  { "group",(PyCFunction)DrawF_Group,1},
  { "find",(PyCFunction)DrawF_Find,1},
  { "box",(PyCFunction)DrawF_Box,1},
  { "pathfill",(PyCFunction)DrawF_PathFill,1},
  { "pathcolour",(PyCFunction)DrawF_PathColour,1},
  { "textcolour",(PyCFunction)DrawF_TextColour,1},
  { "textbackground",(PyCFunction)DrawF_TextBackground,1},
  { NULL,NULL}		/* sentinel */
};

static int drawf_len(PyDrawFObject *b)
{ return b->nobjs;
}

static PyObject *drawf_concat(PyDrawFObject *b,PyDrawFObject *c)
{ int size=b->size+c->size-2*HDRSIZE;
  PyDrawFObject *p=new(size);
  drawfile_diagram *dd;
  drawfile_object *d;
  int n;
  if(!p) return NULL;
  dd=p->drawf;
  d=(drawfile_object*)((char*)dd+b->size);
  memcpy((char*)dd,(char*)(b->drawf),b->size);
  memcpy(d,(char*)(c->drawf)+HDRSIZE,c->size-HDRSIZE);
  p->nobjs=b->nobjs+c->nobjs;
  for(n=c->nobjs;n>0;n--)
  { extend(&(dd->bbox),&(d->data.path.bbox));
    d=NEXT(d);
  }
  return (PyObject*)p;
}

static PyObject *drawf_repeat(PyDrawFObject *b,int i)
{ PyErr_SetString(PyExc_IndexError,"drawf repetition not implemented");
  return NULL;
}

static PyObject *drawf_item(PyDrawFObject *b,int i)
{ PyDrawFObject *c;
  int size;
  drawfile_diagram *dd;
  drawfile_object *d;
  if(i<0||i>=b->nobjs)
  { PyErr_SetString(PyExc_IndexError,"drawf index out of range");
    return NULL;
  }
  d=findobj(b,i);
  size=(char*)findobj(b,i+1)-(char*)d;
  c=new(size);
  if(!c) return NULL;
  dd=c->drawf;
  *((dheader*)dd)=header;
  memcpy(dd->objects,d,size);
  c->nobjs=1;
  extend(&(dd->bbox),&(d->data.path.bbox));
  return (PyObject*)c;
}

static PyObject *drawf_slice(PyDrawFObject *b,int i,int j)
{ PyDrawFObject *c;
  int size,n;
  drawfile_diagram *dd;
  drawfile_object *d;
  if(i<0||j>b->nobjs)
  { PyErr_SetString(PyExc_IndexError,"drawf index out of range");
    return NULL;
  }
  d=findobj(b,i);
  size=(char*)findobj(b,j)-(char*)d;
  c=new(size);
  if(!c) return NULL;
  dd=c->drawf;
  *((dheader*)dd)=header;
  memcpy(dd->objects,d,size);
  c->nobjs=j-i;
  for(n=j-i;n>0;n--)
  { extend(&(dd->bbox),&(d->data.path.bbox));
    d=NEXT(d);
  }
  return (PyObject*)c;
}

static int drawf_ass_item(PyDrawFObject *b,int i,PyObject *v)
{ PyErr_SetString(PyExc_IndexError,"drawf ass not implemented");
  return NULL;
}
/*{ if(i<0||4*i>=b->length)
  { PyErr_SetString(PyExc_IndexError,"drawf index out of range");
    return -1;
  }
  if(!PyInt_Check(v))
  { PyErr_SetString(PyExc_TypeError,"drawf item must be integer");
    return -1;
  }
  ((long*)(b->drawf))[i]=PyInt_AsLong(v);
  return 0;
}
*/

static int drawf_ass_slice(PyDrawFObject *b,int i,int j,PyObject *v)
{ PyErr_SetString(PyExc_IndexError,"drawf ass_slice not implemented");
  return NULL;
}

static PySequenceMethods drawf_as_sequence=
{ (inquiry)drawf_len,
  (binaryfunc)drawf_concat,
  (intargfunc)drawf_repeat,
  (intargfunc)drawf_item,
  (intintargfunc)drawf_slice,
  (intobjargproc)drawf_ass_item,
  (intintobjargproc)drawf_ass_slice,
};

static PyObject *PyDrawF_GetAttr(PyDrawFObject *s,char *name)
{
  if (!strcmp(name, "size")) return PyInt_FromLong((long)s->size);
  if (!strcmp(name, "start")) return PyInt_FromLong((long)s->drawf);
  if (!strcmp(name, "__members__"))
  { PyObject *list = PyList_New(2);
    if (list)
    { PyList_SetItem(list, 0, PyString_FromString("size"));
      PyList_SetItem(list, 1, PyString_FromString("start"));
      if (PyErr_Occurred()) { Py_DECREF(list);list = NULL;}
    }
    return list;
  }
  return Py_FindMethod(PyDrawF_Methods, (PyObject*) s,name);
}

static void PyDrawF_Dealloc(PyDrawFObject *b)
{
    if (b->drawf)
        ;
    else
        PyMem_DEL(b->drawf);
  PyMem_DEL(b);
}

static PyTypeObject PyDrawFType=
{ PyObject_HEAD_INIT(&PyType_Type)
  0,				/*ob_size*/
  "drawf",			/*tp_name*/
  sizeof(PyDrawFObject),	/*tp_size*/
  0,				/*tp_itemsize*/
	/* methods */
  (destructor)PyDrawF_Dealloc,	/*tp_dealloc*/
  0,				/*tp_print*/
  (getattrfunc)PyDrawF_GetAttr,	/*tp_getattr*/
  0,				/*tp_setattr*/
  0,				/*tp_compare*/
  0,				/*tp_repr*/
  0,				/*tp_as_number*/
  &drawf_as_sequence,		/*tp_as_sequence*/
  0,				/*tp_as_mapping*/
  0,                            /*tp_hash*/
};



static PyMethodDef DrawFMethods[]=
{
  { "load",DrawF_Load,1},
  { "new",DrawF_New,1},
  { NULL,NULL}		 /* Sentinel */
};

void initdrawf()
{ PyObject *m, *d;
  m = Py_InitModule("drawf", DrawFMethods);
  d = PyModule_GetDict(m);
  DrawFError=PyString_FromString("drawf.error");
  PyDict_SetItemString(d,"error",DrawFError);
}
