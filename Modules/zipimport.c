#include "Python.h"
#include "structmember.h"
#include "osdefs.h"
#include "marshal.h"
#include <time.h>


#define IS_SOURCE   0x0
#define IS_BYTECODE 0x1
#define IS_PACKAGE  0x2

struct st_zip_searchorder {
    char suffix[14];
    int type;
};

/* zip_searchorder defines how we search for a module in the Zip
   archive: we first search for a package __init__, then for
   non-package .pyc, .pyo and .py entries. The .pyc and .pyo entries
   are swapped by initzipimport() if we run in optimized mode. Also,
   '/' is replaced by SEP there. */
static struct st_zip_searchorder zip_searchorder[] = {
    {"/__init__.pyc", IS_PACKAGE | IS_BYTECODE},
    {"/__init__.pyo", IS_PACKAGE | IS_BYTECODE},
    {"/__init__.py", IS_PACKAGE | IS_SOURCE},
    {".pyc", IS_BYTECODE},
    {".pyo", IS_BYTECODE},
    {".py", IS_SOURCE},
    {"", 0}
};

/* zipimporter object definition and support */

typedef struct _zipimporter ZipImporter;

struct _zipimporter {
    PyObject_HEAD
    PyObject *archive;  /* pathname of the Zip archive,
                           decoded from the filesystem encoding */
    PyObject *prefix;   /* file prefix: "a/sub/directory/",
                           encoded to the filesystem encoding */
    PyObject *files;    /* dict with file info {path: toc_entry} */
};

static PyObject *ZipImportError;
/* read_directory() cache */
static PyObject *zip_directory_cache = NULL;

/* forward decls */
static PyObject *read_directory(PyObject *archive);
static PyObject *get_data(PyObject *archive, PyObject *toc_entry);
static PyObject *get_module_code(ZipImporter *self, PyObject *fullname,
                                 int *p_ispackage, PyObject **p_modpath);


#define ZipImporter_Check(op) PyObject_TypeCheck(op, &ZipImporter_Type)


/* zipimporter.__init__
   Split the "subdirectory" from the Zip archive path, lookup a matching
   entry in sys.path_importer_cache, fetch the file directory from there
   if found, or else read it from the archive. */
static int
zipimporter_init(ZipImporter *self, PyObject *args, PyObject *kwds)
{
    PyObject *path, *files, *tmp;
    PyObject *filename = NULL;
    Py_ssize_t len, flen;
#ifdef ALTSEP
    _Py_IDENTIFIER(replace);
#endif

    if (!_PyArg_NoKeywords("zipimporter()", kwds))
        return -1;

    if (!PyArg_ParseTuple(args, "O&:zipimporter",
                          PyUnicode_FSDecoder, &path))
        return -1;

    if (PyUnicode_READY(path) == -1)
        return -1;

    len = PyUnicode_GET_LENGTH(path);
    if (len == 0) {
        PyErr_SetString(ZipImportError, "archive path is empty");
        goto error;
    }

#ifdef ALTSEP
    tmp = _PyObject_CallMethodId(path, &PyId_replace, "CC", ALTSEP, SEP);
    if (!tmp)
        goto error;
    Py_DECREF(path);
    path = tmp;
#endif

    filename = path;
    Py_INCREF(filename);
    flen = len;
    for (;;) {
        struct stat statbuf;
        int rv;

        rv = _Py_stat(filename, &statbuf);
        if (rv == -2)
            goto error;
        if (rv == 0) {
            /* it exists */
            if (!S_ISREG(statbuf.st_mode))
                /* it's a not file */
                Py_CLEAR(filename);
            break;
        }
        Py_CLEAR(filename);
        /* back up one path element */
        flen = PyUnicode_FindChar(path, SEP, 0, flen, -1);
        if (flen == -1)
            break;
        filename = PyUnicode_Substring(path, 0, flen);
    }
    if (filename == NULL) {
        PyErr_SetString(ZipImportError, "not a Zip file");
        goto error;
    }

    if (PyUnicode_READY(filename) < 0)
        goto error;

    files = PyDict_GetItem(zip_directory_cache, filename);
    if (files == NULL) {
        files = read_directory(filename);
        if (files == NULL)
            goto error;
        if (PyDict_SetItem(zip_directory_cache, filename, files) != 0)
            goto error;
    }
    else
        Py_INCREF(files);
    self->files = files;

    /* Transfer reference */
    self->archive = filename;
    filename = NULL;

    /* Check if there is a prefix directory following the filename. */
    if (flen != len) {
        tmp = PyUnicode_Substring(path, flen+1,
                                  PyUnicode_GET_LENGTH(path));
        if (tmp == NULL)
            goto error;
        self->prefix = tmp;
        if (PyUnicode_READ_CHAR(path, len-1) != SEP) {
            /* add trailing SEP */
            tmp = PyUnicode_FromFormat("%U%c", self->prefix, SEP);
            if (tmp == NULL)
                goto error;
            Py_DECREF(self->prefix);
            self->prefix = tmp;
        }
    }
    else
        self->prefix = PyUnicode_New(0, 0);
    Py_DECREF(path);
    return 0;

error:
    Py_DECREF(path);
    Py_XDECREF(filename);
    return -1;
}

/* GC support. */
static int
zipimporter_traverse(PyObject *obj, visitproc visit, void *arg)
{
    ZipImporter *self = (ZipImporter *)obj;
    Py_VISIT(self->files);
    return 0;
}

static void
zipimporter_dealloc(ZipImporter *self)
{
    PyObject_GC_UnTrack(self);
    Py_XDECREF(self->archive);
    Py_XDECREF(self->prefix);
    Py_XDECREF(self->files);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
zipimporter_repr(ZipImporter *self)
{
    if (self->archive == NULL)
        return PyUnicode_FromString("<zipimporter object \"???\">");
    else if (self->prefix != NULL && PyUnicode_GET_LENGTH(self->prefix) != 0)
        return PyUnicode_FromFormat("<zipimporter object \"%U%c%U\">",
                                    self->archive, SEP, self->prefix);
    else
        return PyUnicode_FromFormat("<zipimporter object \"%U\">",
                                    self->archive);
}

/* return fullname.split(".")[-1] */
static PyObject *
get_subname(PyObject *fullname)
{
    Py_ssize_t len, dot;
    if (PyUnicode_READY(fullname) < 0)
        return NULL;
    len = PyUnicode_GET_LENGTH(fullname);
    dot = PyUnicode_FindChar(fullname, '.', 0, len, -1);
    if (dot == -1) {
        Py_INCREF(fullname);
        return fullname;
    } else
        return PyUnicode_Substring(fullname, dot+1, len);
}

/* Given a (sub)modulename, write the potential file path in the
   archive (without extension) to the path buffer. Return the
   length of the resulting string.

   return self.prefix + name.replace('.', os.sep) */
static PyObject*
make_filename(PyObject *prefix, PyObject *name)
{
    PyObject *pathobj;
    Py_UCS4 *p, *buf;
    Py_ssize_t len;

    len = PyUnicode_GET_LENGTH(prefix) + PyUnicode_GET_LENGTH(name) + 1;
    p = buf = PyMem_Malloc(sizeof(Py_UCS4) * len);
    if (buf == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    if (!PyUnicode_AsUCS4(prefix, p, len, 0)) {
        PyMem_Free(buf);
        return NULL;
    }
    p += PyUnicode_GET_LENGTH(prefix);
    len -= PyUnicode_GET_LENGTH(prefix);
    if (!PyUnicode_AsUCS4(name, p, len, 1)) {
        PyMem_Free(buf);
        return NULL;
    }
    for (; *p; p++) {
        if (*p == '.')
            *p = SEP;
    }
    pathobj = PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND,
                                        buf, p-buf);
    PyMem_Free(buf);
    return pathobj;
}

enum zi_module_info {
    MI_ERROR,
    MI_NOT_FOUND,
    MI_MODULE,
    MI_PACKAGE
};

/* Does this path represent a directory?
   on error, return < 0
   if not a dir, return 0
   if a dir, return 1
*/
static int
check_is_directory(ZipImporter *self, PyObject* prefix, PyObject *path)
{
    PyObject *dirpath;
    int res;

    /* See if this is a "directory". If so, it's eligible to be part
       of a namespace package. We test by seeing if the name, with an
       appended path separator, exists. */
    dirpath = PyUnicode_FromFormat("%U%U%c", prefix, path, SEP);
    if (dirpath == NULL)
        return -1;
    /* If dirpath is present in self->files, we have a directory. */
    res = PyDict_Contains(self->files, dirpath);
    Py_DECREF(dirpath);
    return res;
}

/* Return some information about a module. */
static enum zi_module_info
get_module_info(ZipImporter *self, PyObject *fullname)
{
    PyObject *subname;
    PyObject *path, *fullpath, *item;
    struct st_zip_searchorder *zso;

    subname = get_subname(fullname);
    if (subname == NULL)
        return MI_ERROR;

    path = make_filename(self->prefix, subname);
    Py_DECREF(subname);
    if (path == NULL)
        return MI_ERROR;

    for (zso = zip_searchorder; *zso->suffix; zso++) {
        fullpath = PyUnicode_FromFormat("%U%s", path, zso->suffix);
        if (fullpath == NULL) {
            Py_DECREF(path);
            return MI_ERROR;
        }
        item = PyDict_GetItem(self->files, fullpath);
        Py_DECREF(fullpath);
        if (item != NULL) {
            Py_DECREF(path);
            if (zso->type & IS_PACKAGE)
                return MI_PACKAGE;
            else
                return MI_MODULE;
        }
    }
    Py_DECREF(path);
    return MI_NOT_FOUND;
}

typedef enum {
    FL_ERROR,
    FL_NOT_FOUND,
    FL_MODULE_FOUND,
    FL_NS_FOUND
} find_loader_result;

/* The guts of "find_loader" and "find_module". Return values:
   -1: error
    0: no loader or namespace portions found
    1: module/package found
    2: namespace portion found: *namespace_portion will point to the name
*/
static find_loader_result
find_loader(ZipImporter *self, PyObject *fullname, PyObject **namespace_portion)
{
    enum zi_module_info mi;

    *namespace_portion = NULL;

    mi = get_module_info(self, fullname);
    if (mi == MI_ERROR)
        return FL_ERROR;
    if (mi == MI_NOT_FOUND) {
        /* Not a module or regular package. See if this is a directory, and
           therefore possibly a portion of a namespace package. */
        int is_dir = check_is_directory(self, self->prefix, fullname);
        if (is_dir < 0)
            return -1;
        if (is_dir) {
            /* This is possibly a portion of a namespace
               package. Return the string representing its path,
               without a trailing separator. */
            *namespace_portion = PyUnicode_FromFormat("%U%c%U%U",
                                                      self->archive, SEP,
                                                      self->prefix, fullname);
            if (*namespace_portion == NULL)
                return FL_ERROR;
            return FL_NS_FOUND;
        }
        return FL_NOT_FOUND;
    }
    /* This is a module or package. */
    return FL_MODULE_FOUND;
}


/* Check whether we can satisfy the import of the module named by
   'fullname'. Return self if we can, None if we can't. */
static PyObject *
zipimporter_find_module(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *path = NULL;
    PyObject *fullname;
    PyObject *namespace_portion = NULL;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "U|O:zipimporter.find_module", &fullname, &path))
        return NULL;

    switch (find_loader(self, fullname, &namespace_portion)) {
    case FL_ERROR:
        return NULL;
    case FL_NS_FOUND:
        /* A namespace portion is not allowed via find_module, so return None. */
        Py_DECREF(namespace_portion);
        /* FALL THROUGH */
    case FL_NOT_FOUND:
        result = Py_None;
        break;
    case FL_MODULE_FOUND:
        result = (PyObject *)self;
        break;
    }
    Py_INCREF(result);
    return result;
}


/* Check whether we can satisfy the import of the module named by
   'fullname', or whether it could be a portion of a namespace
   package. Return self if we can load it, a string containing the
   full path if it's a possible namespace portion, None if we
   can't load it. */
static PyObject *
zipimporter_find_loader(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *path = NULL;
    PyObject *fullname;
    PyObject *result = NULL;
    PyObject *namespace_portion = NULL;

    if (!PyArg_ParseTuple(args, "U|O:zipimporter.find_module", &fullname, &path))
        return NULL;

    switch (find_loader(self, fullname, &namespace_portion)) {
    case FL_ERROR:
        return NULL;
    case FL_NOT_FOUND:        /* Not found, return (None, []) */
        result = Py_BuildValue("O[]", Py_None);
        break;
    case FL_MODULE_FOUND:     /* Return (self, []) */
        result = Py_BuildValue("O[]", self);
        break;
    case FL_NS_FOUND:         /* Return (None, [namespace_portion]) */
        result = Py_BuildValue("O[O]", Py_None, namespace_portion);
        Py_DECREF(namespace_portion);
        return result;
    }
    return result;
}

/* Load and return the module named by 'fullname'. */
static PyObject *
zipimporter_load_module(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *code = NULL, *mod, *dict;
    PyObject *fullname;
    PyObject *modpath = NULL;
    int ispackage;

    if (!PyArg_ParseTuple(args, "U:zipimporter.load_module",
                          &fullname))
        return NULL;
    if (PyUnicode_READY(fullname) == -1)
        return NULL;

    code = get_module_code(self, fullname, &ispackage, &modpath);
    if (code == NULL)
        goto error;

    mod = PyImport_AddModuleObject(fullname);
    if (mod == NULL)
        goto error;
    dict = PyModule_GetDict(mod);

    /* mod.__loader__ = self */
    if (PyDict_SetItemString(dict, "__loader__", (PyObject *)self) != 0)
        goto error;

    if (ispackage) {
        /* add __path__ to the module *before* the code gets
           executed */
        PyObject *pkgpath, *fullpath;
        PyObject *subname = get_subname(fullname);
        int err;

        fullpath = PyUnicode_FromFormat("%U%c%U%U",
                                self->archive, SEP,
                                self->prefix, subname);
        Py_DECREF(subname);
        if (fullpath == NULL)
            goto error;

        pkgpath = Py_BuildValue("[N]", fullpath);
        if (pkgpath == NULL)
            goto error;
        err = PyDict_SetItemString(dict, "__path__", pkgpath);
        Py_DECREF(pkgpath);
        if (err != 0)
            goto error;
    }
    mod = PyImport_ExecCodeModuleObject(fullname, code, modpath, NULL);
    Py_CLEAR(code);
    if (mod == NULL)
        goto error;

    if (Py_VerboseFlag)
        PySys_FormatStderr("import %U # loaded from Zip %U\n",
                           fullname, modpath);
    Py_DECREF(modpath);
    return mod;
error:
    Py_XDECREF(code);
    Py_XDECREF(modpath);
    return NULL;
}

/* Return a string matching __file__ for the named module */
static PyObject *
zipimporter_get_filename(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *fullname, *code, *modpath;
    int ispackage;

    if (!PyArg_ParseTuple(args, "U:zipimporter.get_filename",
                          &fullname))
        return NULL;

    /* Deciding the filename requires working out where the code
       would come from if the module was actually loaded */
    code = get_module_code(self, fullname, &ispackage, &modpath);
    if (code == NULL)
        return NULL;
    Py_DECREF(code); /* Only need the path info */

    return modpath;
}

/* Return a bool signifying whether the module is a package or not. */
static PyObject *
zipimporter_is_package(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *fullname;
    enum zi_module_info mi;

    if (!PyArg_ParseTuple(args, "U:zipimporter.is_package",
                          &fullname))
        return NULL;

    mi = get_module_info(self, fullname);
    if (mi == MI_ERROR)
        return NULL;
    if (mi == MI_NOT_FOUND) {
        PyErr_Format(ZipImportError, "can't find module %R", fullname);
        return NULL;
    }
    return PyBool_FromLong(mi == MI_PACKAGE);
}


static PyObject *
zipimporter_get_data(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *path, *key;
#ifdef ALTSEP
    _Py_IDENTIFIER(replace);
#endif
    PyObject *toc_entry;
    Py_ssize_t path_start, path_len, len;

    if (!PyArg_ParseTuple(args, "U:zipimporter.get_data", &path))
        return NULL;

#ifdef ALTSEP
    path = _PyObject_CallMethodId(path, &PyId_replace, "CC", ALTSEP, SEP);
    if (!path)
        return NULL;
#else
    Py_INCREF(path);
#endif
    if (PyUnicode_READY(path) == -1)
        goto error;

    path_len = PyUnicode_GET_LENGTH(path);

    len = PyUnicode_GET_LENGTH(self->archive);
    path_start = 0;
    if (PyUnicode_Tailmatch(path, self->archive, 0, len, -1)
        && PyUnicode_READ_CHAR(path, len) == SEP) {
        path_start = len + 1;
    }

    key = PyUnicode_Substring(path, path_start, path_len);
    if (key == NULL)
        goto error;
    toc_entry = PyDict_GetItem(self->files, key);
    if (toc_entry == NULL) {
        PyErr_SetFromErrnoWithFilenameObject(PyExc_IOError, key);
        Py_DECREF(key);
        goto error;
    }
    Py_DECREF(key);
    Py_DECREF(path);
    return get_data(self->archive, toc_entry);
  error:
    Py_DECREF(path);
    return NULL;
}

static PyObject *
zipimporter_get_code(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *fullname;

    if (!PyArg_ParseTuple(args, "U:zipimporter.get_code", &fullname))
        return NULL;

    return get_module_code(self, fullname, NULL, NULL);
}

static PyObject *
zipimporter_get_source(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *toc_entry;
    PyObject *fullname, *subname, *path, *fullpath;
    enum zi_module_info mi;

    if (!PyArg_ParseTuple(args, "U:zipimporter.get_source", &fullname))
        return NULL;

    mi = get_module_info(self, fullname);
    if (mi == MI_ERROR)
        return NULL;
    if (mi == MI_NOT_FOUND) {
        PyErr_Format(ZipImportError, "can't find module %R", fullname);
        return NULL;
    }

    subname = get_subname(fullname);
    if (subname == NULL)
        return NULL;

    path = make_filename(self->prefix, subname);
    Py_DECREF(subname);
    if (path == NULL)
        return NULL;

    if (mi == MI_PACKAGE)
        fullpath = PyUnicode_FromFormat("%U%c__init__.py", path, SEP);
    else
        fullpath = PyUnicode_FromFormat("%U.py", path);
    Py_DECREF(path);
    if (fullpath == NULL)
        return NULL;

    toc_entry = PyDict_GetItem(self->files, fullpath);
    Py_DECREF(fullpath);
    if (toc_entry != NULL) {
        PyObject *res, *bytes;
        bytes = get_data(self->archive, toc_entry);
        if (bytes == NULL)
            return NULL;
        res = PyUnicode_FromStringAndSize(PyBytes_AS_STRING(bytes),
                                          PyBytes_GET_SIZE(bytes));
        Py_DECREF(bytes);
        return res;
    }

    /* we have the module, but no source */
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(doc_find_module,
"find_module(fullname, path=None) -> self or None.\n\
\n\
Search for a module specified by 'fullname'. 'fullname' must be the\n\
fully qualified (dotted) module name. It returns the zipimporter\n\
instance itself if the module was found, or None if it wasn't.\n\
The optional 'path' argument is ignored -- it's there for compatibility\n\
with the importer protocol.");

PyDoc_STRVAR(doc_find_loader,
"find_loader(fullname, path=None) -> self, str or None.\n\
\n\
Search for a module specified by 'fullname'. 'fullname' must be the\n\
fully qualified (dotted) module name. It returns the zipimporter\n\
instance itself if the module was found, a string containing the\n\
full path name if it's possibly a portion of a namespace package,\n\
or None otherwise. The optional 'path' argument is ignored -- it's\n\
 there for compatibility with the importer protocol.");

PyDoc_STRVAR(doc_load_module,
"load_module(fullname) -> module.\n\
\n\
Load the module specified by 'fullname'. 'fullname' must be the\n\
fully qualified (dotted) module name. It returns the imported\n\
module, or raises ZipImportError if it wasn't found.");

PyDoc_STRVAR(doc_get_data,
"get_data(pathname) -> string with file data.\n\
\n\
Return the data associated with 'pathname'. Raise IOError if\n\
the file wasn't found.");

PyDoc_STRVAR(doc_is_package,
"is_package(fullname) -> bool.\n\
\n\
Return True if the module specified by fullname is a package.\n\
Raise ZipImportError if the module couldn't be found.");

PyDoc_STRVAR(doc_get_code,
"get_code(fullname) -> code object.\n\
\n\
Return the code object for the specified module. Raise ZipImportError\n\
if the module couldn't be found.");

PyDoc_STRVAR(doc_get_source,
"get_source(fullname) -> source string.\n\
\n\
Return the source code for the specified module. Raise ZipImportError\n\
if the module couldn't be found, return None if the archive does\n\
contain the module, but has no source for it.");


PyDoc_STRVAR(doc_get_filename,
"get_filename(fullname) -> filename string.\n\
\n\
Return the filename for the specified module.");

static PyMethodDef zipimporter_methods[] = {
    {"find_module", zipimporter_find_module, METH_VARARGS,
     doc_find_module},
    {"find_loader", zipimporter_find_loader, METH_VARARGS,
     doc_find_loader},
    {"load_module", zipimporter_load_module, METH_VARARGS,
     doc_load_module},
    {"get_data", zipimporter_get_data, METH_VARARGS,
     doc_get_data},
    {"get_code", zipimporter_get_code, METH_VARARGS,
     doc_get_code},
    {"get_source", zipimporter_get_source, METH_VARARGS,
     doc_get_source},
    {"get_filename", zipimporter_get_filename, METH_VARARGS,
     doc_get_filename},
    {"is_package", zipimporter_is_package, METH_VARARGS,
     doc_is_package},
    {NULL,              NULL}   /* sentinel */
};

static PyMemberDef zipimporter_members[] = {
    {"archive",  T_OBJECT, offsetof(ZipImporter, archive),  READONLY},
    {"prefix",   T_OBJECT, offsetof(ZipImporter, prefix),   READONLY},
    {"_files",   T_OBJECT, offsetof(ZipImporter, files),    READONLY},
    {NULL}
};

PyDoc_STRVAR(zipimporter_doc,
"zipimporter(archivepath) -> zipimporter object\n\
\n\
Create a new zipimporter instance. 'archivepath' must be a path to\n\
a zipfile, or to a specific path inside a zipfile. For example, it can be\n\
'/tmp/myimport.zip', or '/tmp/myimport.zip/mydirectory', if mydirectory is a\n\
valid directory inside the archive.\n\
\n\
'ZipImportError is raised if 'archivepath' doesn't point to a valid Zip\n\
archive.\n\
\n\
The 'archive' attribute of zipimporter objects contains the name of the\n\
zipfile targeted.");

#define DEFERRED_ADDRESS(ADDR) 0

static PyTypeObject ZipImporter_Type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "zipimport.zipimporter",
    sizeof(ZipImporter),
    0,                                          /* tp_itemsize */
    (destructor)zipimporter_dealloc,            /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)zipimporter_repr,                 /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_HAVE_GC,                     /* tp_flags */
    zipimporter_doc,                            /* tp_doc */
    zipimporter_traverse,                       /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    zipimporter_methods,                        /* tp_methods */
    zipimporter_members,                        /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)zipimporter_init,                 /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};


/* implementation */

/* Given a buffer, return the long that is represented by the first
   4 bytes, encoded as little endian. This partially reimplements
   marshal.c:r_long() */
static long
get_long(unsigned char *buf) {
    long x;
    x =  buf[0];
    x |= (long)buf[1] <<  8;
    x |= (long)buf[2] << 16;
    x |= (long)buf[3] << 24;
#if SIZEOF_LONG > 4
    /* Sign extension for 64-bit machines */
    x |= -(x & 0x80000000L);
#endif
    return x;
}

/*
   read_directory(archive) -> files dict (new reference)

   Given a path to a Zip archive, build a dict, mapping file names
   (local to the archive, using SEP as a separator) to toc entries.

   A toc_entry is a tuple:

   (__file__,      # value to use for __file__, available for all files,
                   # encoded to the filesystem encoding
    compress,      # compression kind; 0 for uncompressed
    data_size,     # size of compressed data on disk
    file_size,     # size of decompressed data
    file_offset,   # offset of file header from start of archive
    time,          # mod time of file (in dos format)
    date,          # mod data of file (in dos format)
    crc,           # crc checksum of the data
   )

   Directories can be recognized by the trailing SEP in the name,
   data_size and file_offset are 0.
*/
static PyObject *
read_directory(PyObject *archive)
{
    PyObject *files = NULL;
    FILE *fp;
    unsigned short flags;
    short compress, time, date, name_size;
    long crc, data_size, file_size, header_size;
    Py_ssize_t file_offset, header_position, header_offset;
    long l, count;
    Py_ssize_t i;
    char name[MAXPATHLEN + 5];
    char dummy[8]; /* Buffer to read unused header values into */
    PyObject *nameobj = NULL;
    char *p, endof_central_dir[22];
    Py_ssize_t arc_offset;  /* Absolute offset to start of the zip-archive. */
    PyObject *path;
    const char *charset;
    int bootstrap;

    fp = _Py_fopen(archive, "rb");
    if (fp == NULL) {
        if (!PyErr_Occurred())
            PyErr_Format(ZipImportError, "can't open Zip file: %R", archive);
        return NULL;
    }

    if (fseek(fp, -22, SEEK_END) == -1) {
        fclose(fp);
        PyErr_Format(ZipImportError, "can't read Zip file: %R", archive);
        return NULL;
    }
    header_position = ftell(fp);
    if (fread(endof_central_dir, 1, 22, fp) != 22) {
        fclose(fp);
        PyErr_Format(ZipImportError, "can't read Zip file: %R", archive);
        return NULL;
    }
    if (get_long((unsigned char *)endof_central_dir) != 0x06054B50) {
        /* Bad: End of Central Dir signature */
        fclose(fp);
        PyErr_Format(ZipImportError, "not a Zip file: %R", archive);
        return NULL;
    }

    header_size = get_long((unsigned char *)endof_central_dir + 12);
    header_offset = get_long((unsigned char *)endof_central_dir + 16);
    arc_offset = header_position - header_offset - header_size;
    header_offset += arc_offset;

    files = PyDict_New();
    if (files == NULL)
        goto error;

    /* Start of Central Directory */
    count = 0;
    if (fseek(fp, header_offset, 0) == -1)
        goto file_error;
    for (;;) {
        PyObject *t;
        int err;

        /* Start of file header */
        l = PyMarshal_ReadLongFromFile(fp);
        if (l != 0x02014B50)
            break;              /* Bad: Central Dir File Header */

        /* On Windows, calling fseek to skip over the fields we don't use is
        slower than reading the data into a dummy buffer because fseek flushes
        stdio's internal buffers. See issue #8745. */
        if (fread(dummy, 1, 4, fp) != 4) /* Skip unused fields, avoid fseek */
            goto file_error;

        flags = (unsigned short)PyMarshal_ReadShortFromFile(fp);
        compress = PyMarshal_ReadShortFromFile(fp);
        time = PyMarshal_ReadShortFromFile(fp);
        date = PyMarshal_ReadShortFromFile(fp);
        crc = PyMarshal_ReadLongFromFile(fp);
        data_size = PyMarshal_ReadLongFromFile(fp);
        file_size = PyMarshal_ReadLongFromFile(fp);
        name_size = PyMarshal_ReadShortFromFile(fp);
        header_size = name_size +
           PyMarshal_ReadShortFromFile(fp) +
           PyMarshal_ReadShortFromFile(fp);
        if (fread(dummy, 1, 8, fp) != 8) /* Skip unused fields, avoid fseek */
            goto file_error;
        file_offset = PyMarshal_ReadLongFromFile(fp) + arc_offset;
        if (name_size > MAXPATHLEN)
            name_size = MAXPATHLEN;

        p = name;
        for (i = 0; i < (Py_ssize_t)name_size; i++) {
            *p = (char)getc(fp);
            if (*p == '/')
                *p = SEP;
            p++;
        }
        *p = 0;         /* Add terminating null byte */
        for (; i < header_size; i++) /* Skip the rest of the header */
            if(getc(fp) == EOF) /* Avoid fseek */
                goto file_error;

        bootstrap = 0;
        if (flags & 0x0800)
            charset = "utf-8";
        else if (!PyThreadState_GET()->interp->codecs_initialized) {
            /* During bootstrap, we may need to load the encodings
               package from a ZIP file. But the cp437 encoding is implemented
               in Python in the encodings package.

               Break out of this dependency by assuming that the path to
               the encodings module is ASCII-only. */
            charset = "ascii";
            bootstrap = 1;
        }
        else
            charset = "cp437";
        nameobj = PyUnicode_Decode(name, name_size, charset, NULL);
        if (nameobj == NULL) {
            if (bootstrap)
                PyErr_Format(PyExc_NotImplementedError,
                    "bootstrap issue: python%i%i.zip contains non-ASCII "
                    "filenames without the unicode flag",
                    PY_MAJOR_VERSION, PY_MINOR_VERSION);
            goto error;
        }
        if (PyUnicode_READY(nameobj) == -1)
            goto error;
        path = PyUnicode_FromFormat("%U%c%U", archive, SEP, nameobj);
        if (path == NULL)
            goto error;
        t = Py_BuildValue("Nhllnhhl", path, compress, data_size,
                          file_size, file_offset, time, date, crc);
        if (t == NULL)
            goto error;
        err = PyDict_SetItem(files, nameobj, t);
        Py_CLEAR(nameobj);
        Py_DECREF(t);
        if (err != 0)
            goto error;
        count++;
    }
    fclose(fp);
    if (Py_VerboseFlag)
        PySys_FormatStderr("# zipimport: found %ld names in %R\n",
                           count, archive);
    return files;
file_error:
    fclose(fp);
    Py_XDECREF(files);
    Py_XDECREF(nameobj);
    PyErr_Format(ZipImportError, "can't read Zip file: %R", archive);
    return NULL;
error:
    fclose(fp);
    Py_XDECREF(files);
    Py_XDECREF(nameobj);
    return NULL;
}

/* Return the zlib.decompress function object, or NULL if zlib couldn't
   be imported. The function is cached when found, so subsequent calls
   don't import zlib again. */
static PyObject *
get_decompress_func(void)
{
    static int importing_zlib = 0;
    PyObject *zlib;
    PyObject *decompress;
    _Py_IDENTIFIER(decompress);

    if (importing_zlib != 0)
        /* Someone has a zlib.py[co] in their Zip file;
           let's avoid a stack overflow. */
        return NULL;
    importing_zlib = 1;
    zlib = PyImport_ImportModuleNoBlock("zlib");
    importing_zlib = 0;
    if (zlib != NULL) {
        decompress = _PyObject_GetAttrId(zlib,
                                         &PyId_decompress);
        Py_DECREF(zlib);
    }
    else {
        PyErr_Clear();
        decompress = NULL;
    }
    if (Py_VerboseFlag)
        PySys_WriteStderr("# zipimport: zlib %s\n",
            zlib != NULL ? "available": "UNAVAILABLE");
    return decompress;
}

/* Given a path to a Zip file and a toc_entry, return the (uncompressed)
   data as a new reference. */
static PyObject *
get_data(PyObject *archive, PyObject *toc_entry)
{
    PyObject *raw_data, *data = NULL, *decompress;
    char *buf;
    FILE *fp;
    int err;
    Py_ssize_t bytes_read = 0;
    long l;
    PyObject *datapath;
    long compress, data_size, file_size, file_offset, bytes_size;
    long time, date, crc;

    if (!PyArg_ParseTuple(toc_entry, "Olllllll", &datapath, &compress,
                          &data_size, &file_size, &file_offset, &time,
                          &date, &crc)) {
        return NULL;
    }

    fp = _Py_fopen(archive, "rb");
    if (!fp) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_IOError,
               "zipimport: can not open file %U", archive);
        return NULL;
    }

    /* Check to make sure the local file header is correct */
    if (fseek(fp, file_offset, 0) == -1) {
        fclose(fp);
        PyErr_Format(ZipImportError, "can't read Zip file: %R", archive);
        return NULL;
    }

    l = PyMarshal_ReadLongFromFile(fp);
    if (l != 0x04034B50) {
        /* Bad: Local File Header */
        PyErr_Format(ZipImportError,
                     "bad local file header in %U",
                     archive);
        fclose(fp);
        return NULL;
    }
    if (fseek(fp, file_offset + 26, 0) == -1) {
        fclose(fp);
        PyErr_Format(ZipImportError, "can't read Zip file: %R", archive);
        return NULL;
    }

    l = 30 + PyMarshal_ReadShortFromFile(fp) +
        PyMarshal_ReadShortFromFile(fp);        /* local header size */
    file_offset += l;           /* Start of file data */

    bytes_size = compress == 0 ? data_size : data_size + 1;
    if (bytes_size == 0)
        bytes_size++;
    raw_data = PyBytes_FromStringAndSize((char *)NULL, bytes_size);

    if (raw_data == NULL) {
        fclose(fp);
        return NULL;
    }
    buf = PyBytes_AsString(raw_data);

    err = fseek(fp, file_offset, 0);
    if (err == 0) {
        bytes_read = fread(buf, 1, data_size, fp);
    } else {
        fclose(fp);
        PyErr_Format(ZipImportError, "can't read Zip file: %R", archive);
        return NULL;
    }
    fclose(fp);
    if (err || bytes_read != data_size) {
        PyErr_SetString(PyExc_IOError,
                        "zipimport: can't read data");
        Py_DECREF(raw_data);
        return NULL;
    }

    if (compress != 0) {
        buf[data_size] = 'Z';  /* saw this in zipfile.py */
        data_size++;
    }
    buf[data_size] = '\0';

    if (compress == 0) {  /* data is not compressed */
        data = PyBytes_FromStringAndSize(buf, data_size);
        Py_DECREF(raw_data);
        return data;
    }

    /* Decompress with zlib */
    decompress = get_decompress_func();
    if (decompress == NULL) {
        PyErr_SetString(ZipImportError,
                        "can't decompress data; "
                        "zlib not available");
        goto error;
    }
    data = PyObject_CallFunction(decompress, "Oi", raw_data, -15);
    Py_DECREF(decompress);
error:
    Py_DECREF(raw_data);
    return data;
}

/* Lenient date/time comparison function. The precision of the mtime
   in the archive is lower than the mtime stored in a .pyc: we
   must allow a difference of at most one second. */
static int
eq_mtime(time_t t1, time_t t2)
{
    time_t d = t1 - t2;
    if (d < 0)
        d = -d;
    /* dostime only stores even seconds, so be lenient */
    return d <= 1;
}

/* Given the contents of a .py[co] file in a buffer, unmarshal the data
   and return the code object. Return None if it the magic word doesn't
   match (we do this instead of raising an exception as we fall back
   to .py if available and we don't want to mask other errors).
   Returns a new reference. */
static PyObject *
unmarshal_code(PyObject *pathname, PyObject *data, time_t mtime)
{
    PyObject *code;
    char *buf = PyBytes_AsString(data);
    Py_ssize_t size = PyBytes_Size(data);

    if (size <= 9) {
        PyErr_SetString(ZipImportError,
                        "bad pyc data");
        return NULL;
    }

    if (get_long((unsigned char *)buf) != PyImport_GetMagicNumber()) {
        if (Py_VerboseFlag)
            PySys_FormatStderr("# %R has bad magic\n",
                               pathname);
        Py_INCREF(Py_None);
        return Py_None;  /* signal caller to try alternative */
    }

    if (mtime != 0 && !eq_mtime(get_long((unsigned char *)buf + 4),
                                mtime)) {
        if (Py_VerboseFlag)
            PySys_FormatStderr("# %R has bad mtime\n",
                               pathname);
        Py_INCREF(Py_None);
        return Py_None;  /* signal caller to try alternative */
    }

    /* XXX the pyc's size field is ignored; timestamp collisions are probably
       unimportant with zip files. */
    code = PyMarshal_ReadObjectFromString(buf + 12, size - 12);
    if (code == NULL)
        return NULL;
    if (!PyCode_Check(code)) {
        Py_DECREF(code);
        PyErr_Format(PyExc_TypeError,
             "compiled module %R is not a code object",
             pathname);
        return NULL;
    }
    return code;
}

/* Replace any occurances of "\r\n?" in the input string with "\n".
   This converts DOS and Mac line endings to Unix line endings.
   Also append a trailing "\n" to be compatible with
   PyParser_SimpleParseFile(). Returns a new reference. */
static PyObject *
normalize_line_endings(PyObject *source)
{
    char *buf, *q, *p;
    PyObject *fixed_source;
    int len = 0;

    p = PyBytes_AsString(source);
    if (p == NULL) {
        return PyBytes_FromStringAndSize("\n\0", 2);
    }

    /* one char extra for trailing \n and one for terminating \0 */
    buf = (char *)PyMem_Malloc(PyBytes_Size(source) + 2);
    if (buf == NULL) {
        PyErr_SetString(PyExc_MemoryError,
                        "zipimport: no memory to allocate "
                        "source buffer");
        return NULL;
    }
    /* replace "\r\n?" by "\n" */
    for (q = buf; *p != '\0'; p++) {
        if (*p == '\r') {
            *q++ = '\n';
            if (*(p + 1) == '\n')
                p++;
        }
        else
            *q++ = *p;
        len++;
    }
    *q++ = '\n';  /* add trailing \n */
    *q = '\0';
    fixed_source = PyBytes_FromStringAndSize(buf, len + 2);
    PyMem_Free(buf);
    return fixed_source;
}

/* Given a string buffer containing Python source code, compile it
   return and return a code object as a new reference. */
static PyObject *
compile_source(PyObject *pathname, PyObject *source)
{
    PyObject *code, *fixed_source, *pathbytes;

    pathbytes = PyUnicode_EncodeFSDefault(pathname);
    if (pathbytes == NULL)
        return NULL;

    fixed_source = normalize_line_endings(source);
    if (fixed_source == NULL) {
        Py_DECREF(pathbytes);
        return NULL;
    }

    code = Py_CompileString(PyBytes_AsString(fixed_source),
                            PyBytes_AsString(pathbytes),
                            Py_file_input);
    Py_DECREF(pathbytes);
    Py_DECREF(fixed_source);
    return code;
}

/* Convert the date/time values found in the Zip archive to a value
   that's compatible with the time stamp stored in .pyc files. */
static time_t
parse_dostime(int dostime, int dosdate)
{
    struct tm stm;

    memset((void *) &stm, '\0', sizeof(stm));

    stm.tm_sec   =  (dostime        & 0x1f) * 2;
    stm.tm_min   =  (dostime >> 5)  & 0x3f;
    stm.tm_hour  =  (dostime >> 11) & 0x1f;
    stm.tm_mday  =   dosdate        & 0x1f;
    stm.tm_mon   = ((dosdate >> 5)  & 0x0f) - 1;
    stm.tm_year  = ((dosdate >> 9)  & 0x7f) + 80;
    stm.tm_isdst =   -1; /* wday/yday is ignored */

    return mktime(&stm);
}

/* Given a path to a .pyc or .pyo file in the archive, return the
   modification time of the matching .py file, or 0 if no source
   is available. */
static time_t
get_mtime_of_source(ZipImporter *self, PyObject *path)
{
    PyObject *toc_entry, *stripped;
    time_t mtime;

    /* strip 'c' or 'o' from *.py[co] */
    if (PyUnicode_READY(path) == -1)
        return (time_t)-1;
    stripped = PyUnicode_FromKindAndData(PyUnicode_KIND(path),
                                         PyUnicode_DATA(path),
                                         PyUnicode_GET_LENGTH(path) - 1);
    if (stripped == NULL)
        return (time_t)-1;

    toc_entry = PyDict_GetItem(self->files, stripped);
    Py_DECREF(stripped);
    if (toc_entry != NULL && PyTuple_Check(toc_entry) &&
        PyTuple_Size(toc_entry) == 8) {
        /* fetch the time stamp of the .py file for comparison
           with an embedded pyc time stamp */
        int time, date;
        time = PyLong_AsLong(PyTuple_GetItem(toc_entry, 5));
        date = PyLong_AsLong(PyTuple_GetItem(toc_entry, 6));
        mtime = parse_dostime(time, date);
    } else
        mtime = 0;
    return mtime;
}

/* Return the code object for the module named by 'fullname' from the
   Zip archive as a new reference. */
static PyObject *
get_code_from_data(ZipImporter *self, int ispackage, int isbytecode,
                   time_t mtime, PyObject *toc_entry)
{
    PyObject *data, *modpath, *code;

    data = get_data(self->archive, toc_entry);
    if (data == NULL)
        return NULL;

    modpath = PyTuple_GetItem(toc_entry, 0);
    if (isbytecode)
        code = unmarshal_code(modpath, data, mtime);
    else
        code = compile_source(modpath, data);
    Py_DECREF(data);
    return code;
}

/* Get the code object associated with the module specified by
   'fullname'. */
static PyObject *
get_module_code(ZipImporter *self, PyObject *fullname,
                int *p_ispackage, PyObject **p_modpath)
{
    PyObject *code = NULL, *toc_entry, *subname;
    PyObject *path, *fullpath = NULL;
    struct st_zip_searchorder *zso;

    subname = get_subname(fullname);
    if (subname == NULL)
        return NULL;

    path = make_filename(self->prefix, subname);
    Py_DECREF(subname);
    if (path == NULL)
        return NULL;

    for (zso = zip_searchorder; *zso->suffix; zso++) {
        code = NULL;

        fullpath = PyUnicode_FromFormat("%U%s", path, zso->suffix);
        if (fullpath == NULL)
            goto exit;

        if (Py_VerboseFlag > 1)
            PySys_FormatStderr("# trying %U%c%U\n",
                               self->archive, (int)SEP, fullpath);
        toc_entry = PyDict_GetItem(self->files, fullpath);
        if (toc_entry != NULL) {
            time_t mtime = 0;
            int ispackage = zso->type & IS_PACKAGE;
            int isbytecode = zso->type & IS_BYTECODE;

            if (isbytecode) {
                mtime = get_mtime_of_source(self, fullpath);
                if (mtime == (time_t)-1 && PyErr_Occurred()) {
                    goto exit;
                }
            }
            Py_CLEAR(fullpath);
            if (p_ispackage != NULL)
                *p_ispackage = ispackage;
            code = get_code_from_data(self, ispackage,
                                      isbytecode, mtime,
                                      toc_entry);
            if (code == Py_None) {
                /* bad magic number or non-matching mtime
                   in byte code, try next */
                Py_DECREF(code);
                continue;
            }
            if (code != NULL && p_modpath != NULL) {
                *p_modpath = PyTuple_GetItem(toc_entry, 0);
                Py_INCREF(*p_modpath);
            }
            goto exit;
        }
        else
            Py_CLEAR(fullpath);
    }
    PyErr_Format(ZipImportError, "can't find module %R", fullname);
exit:
    Py_DECREF(path);
    Py_XDECREF(fullpath);
    return code;
}


/* Module init */

PyDoc_STRVAR(zipimport_doc,
"zipimport provides support for importing Python modules from Zip archives.\n\
\n\
This module exports three objects:\n\
- zipimporter: a class; its constructor takes a path to a Zip archive.\n\
- ZipImportError: exception raised by zipimporter objects. It's a\n\
  subclass of ImportError, so it can be caught as ImportError, too.\n\
- _zip_directory_cache: a dict, mapping archive paths to zip directory\n\
  info dicts, as used in zipimporter._files.\n\
\n\
It is usually not needed to use the zipimport module explicitly; it is\n\
used by the builtin import mechanism for sys.path items that are paths\n\
to Zip archives.");

static struct PyModuleDef zipimportmodule = {
    PyModuleDef_HEAD_INIT,
    "zipimport",
    zipimport_doc,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_zipimport(void)
{
    PyObject *mod;

    if (PyType_Ready(&ZipImporter_Type) < 0)
        return NULL;

    /* Correct directory separator */
    zip_searchorder[0].suffix[0] = SEP;
    zip_searchorder[1].suffix[0] = SEP;
    zip_searchorder[2].suffix[0] = SEP;
    if (Py_OptimizeFlag) {
        /* Reverse *.pyc and *.pyo */
        struct st_zip_searchorder tmp;
        tmp = zip_searchorder[0];
        zip_searchorder[0] = zip_searchorder[1];
        zip_searchorder[1] = tmp;
        tmp = zip_searchorder[3];
        zip_searchorder[3] = zip_searchorder[4];
        zip_searchorder[4] = tmp;
    }

    mod = PyModule_Create(&zipimportmodule);
    if (mod == NULL)
        return NULL;

    ZipImportError = PyErr_NewException("zipimport.ZipImportError",
                                        PyExc_ImportError, NULL);
    if (ZipImportError == NULL)
        return NULL;

    Py_INCREF(ZipImportError);
    if (PyModule_AddObject(mod, "ZipImportError",
                           ZipImportError) < 0)
        return NULL;

    Py_INCREF(&ZipImporter_Type);
    if (PyModule_AddObject(mod, "zipimporter",
                           (PyObject *)&ZipImporter_Type) < 0)
        return NULL;

    zip_directory_cache = PyDict_New();
    if (zip_directory_cache == NULL)
        return NULL;
    Py_INCREF(zip_directory_cache);
    if (PyModule_AddObject(mod, "_zip_directory_cache",
                           zip_directory_cache) < 0)
        return NULL;
    return mod;
}
