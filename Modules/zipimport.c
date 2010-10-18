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
    PyObject *archive;  /* pathname of the Zip archive */
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
static PyObject *get_module_code(ZipImporter *self, char *fullname,
                                 int *p_ispackage, PyObject **p_modpath);


#define ZipImporter_Check(op) PyObject_TypeCheck(op, &ZipImporter_Type)


/* zipimporter.__init__
   Split the "subdirectory" from the Zip archive path, lookup a matching
   entry in sys.path_importer_cache, fetch the file directory from there
   if found, or else read it from the archive. */
static int
zipimporter_init(ZipImporter *self, PyObject *args, PyObject *kwds)
{
    PyObject *pathobj, *files;
    Py_UNICODE *path, *p, *prefix, buf[MAXPATHLEN+2];
    Py_ssize_t len;

    if (!_PyArg_NoKeywords("zipimporter()", kwds))
        return -1;

    if (!PyArg_ParseTuple(args, "O&:zipimporter",
        PyUnicode_FSDecoder, &pathobj))
        return -1;

    /* copy path to buf */
    len = PyUnicode_GET_SIZE(pathobj);
    if (len == 0) {
        PyErr_SetString(ZipImportError, "archive path is empty");
        goto error;
    }
    if (len >= MAXPATHLEN) {
        PyErr_SetString(ZipImportError,
                        "archive path too long");
        goto error;
    }
    Py_UNICODE_strcpy(buf, PyUnicode_AS_UNICODE(pathobj));

#ifdef ALTSEP
    for (p = buf; *p; p++) {
        if (*p == ALTSEP)
            *p = SEP;
    }
#endif

    path = NULL;
    prefix = NULL;
    for (;;) {
        struct stat statbuf;
        int rv;

        if (pathobj == NULL) {
            pathobj = PyUnicode_FromUnicode(buf, len);
            if (pathobj == NULL)
                goto error;
        }
        rv = _Py_stat(pathobj, &statbuf);
        if (rv == 0) {
            /* it exists */
            if (S_ISREG(statbuf.st_mode))
                /* it's a file */
                path = buf;
            break;
        }
        else if (PyErr_Occurred())
            goto error;
        /* back up one path element */
        p = Py_UNICODE_strrchr(buf, SEP);
        if (prefix != NULL)
            *prefix = SEP;
        if (p == NULL)
            break;
        *p = '\0';
        len = p - buf;
        prefix = p;
        Py_CLEAR(pathobj);
    }
    if (path == NULL) {
        PyErr_SetString(ZipImportError, "not a Zip file");
        goto error;
    }

    files = PyDict_GetItem(zip_directory_cache, pathobj);
    if (files == NULL) {
        files = read_directory(pathobj);
        if (files == NULL)
            goto error;
        if (PyDict_SetItem(zip_directory_cache, pathobj, files) != 0)
            goto error;
    }
    else
        Py_INCREF(files);
    self->files = files;

    self->archive = pathobj;
    pathobj = NULL;

    if (prefix != NULL) {
        prefix++;
        len = Py_UNICODE_strlen(prefix);
        if (prefix[len-1] != SEP) {
            /* add trailing SEP */
            prefix[len] = SEP;
            prefix[len + 1] = '\0';
            len++;
        }
    }
    else
        len = 0;
    self->prefix = PyUnicode_FromUnicode(prefix, len);
    if (self->prefix == NULL)
        goto error;

    return 0;

error:
    Py_XDECREF(pathobj);
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
    else if (self->prefix != NULL && PyUnicode_GET_SIZE(self->prefix) != 0)
        return PyUnicode_FromFormat("<zipimporter object \"%.300U%c%.150U\">",
                                    self->archive, SEP, self->prefix);
    else
        return PyUnicode_FromFormat("<zipimporter object \"%.300U\">",
                                    self->archive);
}

/* return fullname.split(".")[-1] */
static char *
get_subname(char *fullname)
{
    char *subname = strrchr(fullname, '.');
    if (subname == NULL)
        subname = fullname;
    else
        subname++;
    return subname;
}

/* Given a (sub)modulename, write the potential file path in the
   archive (without extension) to the path buffer. Return the
   length of the resulting string. */
static int
make_filename(PyObject *prefix_obj, char *name, char *path)
{
    size_t len;
    char *p;
    PyObject *prefix;

    prefix = PyUnicode_EncodeFSDefault(prefix_obj);
    if (prefix == NULL)
        return -1;
    len = PyBytes_GET_SIZE(prefix);

    /* self.prefix + name [+ SEP + "__init__"] + ".py[co]" */
    if (len + strlen(name) + 13 >= MAXPATHLEN) {
        PyErr_SetString(ZipImportError, "path too long");
        Py_DECREF(prefix);
        return -1;
    }

    strcpy(path, PyBytes_AS_STRING(prefix));
    Py_DECREF(prefix);
    strcpy(path + len, name);
    for (p = path + len; *p; p++) {
        if (*p == '.')
            *p = SEP;
    }
    len += strlen(name);
    assert(len < INT_MAX);
    return (int)len;
}

enum zi_module_info {
    MI_ERROR,
    MI_NOT_FOUND,
    MI_MODULE,
    MI_PACKAGE
};

/* Return some information about a module. */
static enum zi_module_info
get_module_info(ZipImporter *self, char *fullname)
{
    char *subname, path[MAXPATHLEN + 1];
    int len;
    struct st_zip_searchorder *zso;

    subname = get_subname(fullname);

    len = make_filename(self->prefix, subname, path);
    if (len < 0)
        return MI_ERROR;

    for (zso = zip_searchorder; *zso->suffix; zso++) {
        strcpy(path + len, zso->suffix);
        if (PyDict_GetItemString(self->files, path) != NULL) {
            if (zso->type & IS_PACKAGE)
                return MI_PACKAGE;
            else
                return MI_MODULE;
        }
    }
    return MI_NOT_FOUND;
}

/* Check whether we can satisfy the import of the module named by
   'fullname'. Return self if we can, None if we can't. */
static PyObject *
zipimporter_find_module(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *path = NULL;
    char *fullname;
    enum zi_module_info mi;

    if (!PyArg_ParseTuple(args, "s|O:zipimporter.find_module",
                          &fullname, &path))
        return NULL;

    mi = get_module_info(self, fullname);
    if (mi == MI_ERROR)
        return NULL;
    if (mi == MI_NOT_FOUND) {
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_INCREF(self);
    return (PyObject *)self;
}

/* Load and return the module named by 'fullname'. */
static PyObject *
zipimporter_load_module(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *code = NULL, *mod, *dict;
    char *fullname;
    PyObject *modpath = NULL, *modpath_bytes;
    int ispackage;

    if (!PyArg_ParseTuple(args, "s:zipimporter.load_module",
                          &fullname))
        return NULL;

    code = get_module_code(self, fullname, &ispackage, &modpath);
    if (code == NULL)
        goto error;

    mod = PyImport_AddModule(fullname);
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
        char *subname = get_subname(fullname);
        int err;

        fullpath = PyUnicode_FromFormat("%U%c%U%s",
                                self->archive, SEP,
                                self->prefix, subname);
        if (fullpath == NULL)
            goto error;

        pkgpath = Py_BuildValue("[O]", fullpath);
        Py_DECREF(fullpath);
        if (pkgpath == NULL)
            goto error;
        err = PyDict_SetItemString(dict, "__path__", pkgpath);
        Py_DECREF(pkgpath);
        if (err != 0)
            goto error;
    }
    modpath_bytes = PyUnicode_EncodeFSDefault(modpath);
    if (modpath_bytes == NULL)
        goto error;
    mod = PyImport_ExecCodeModuleEx(fullname, code,
                                    PyBytes_AS_STRING(modpath_bytes));
    Py_DECREF(modpath_bytes);
    Py_CLEAR(code);
    if (mod == NULL)
        goto error;

    if (Py_VerboseFlag)
        PySys_FormatStderr("import %s # loaded from Zip %U\n",
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
    PyObject *code;
    char *fullname;
    PyObject *modpath;
    int ispackage;

    if (!PyArg_ParseTuple(args, "s:zipimporter.get_filename",
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
    char *fullname;
    enum zi_module_info mi;

    if (!PyArg_ParseTuple(args, "s:zipimporter.is_package",
                          &fullname))
        return NULL;

    mi = get_module_info(self, fullname);
    if (mi == MI_ERROR)
        return NULL;
    if (mi == MI_NOT_FOUND) {
        PyErr_Format(ZipImportError, "can't find module '%.200s'",
                     fullname);
        return NULL;
    }
    return PyBool_FromLong(mi == MI_PACKAGE);
}

static PyObject *
zipimporter_get_data(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *pathobj, *key;
    const Py_UNICODE *path;
#ifdef ALTSEP
    Py_UNICODE *p, buf[MAXPATHLEN + 1];
#endif
    Py_UNICODE *archive;
    PyObject *toc_entry;
    Py_ssize_t path_len, len;

    if (!PyArg_ParseTuple(args, "U:zipimporter.get_data", &pathobj))
        return NULL;

    path_len = PyUnicode_GET_SIZE(pathobj);
    path = PyUnicode_AS_UNICODE(pathobj);
#ifdef ALTSEP
    if (path_len >= MAXPATHLEN) {
        PyErr_SetString(ZipImportError, "path too long");
        return NULL;
    }
    Py_UNICODE_strcpy(buf, path);
    for (p = buf; *p; p++) {
        if (*p == ALTSEP)
            *p = SEP;
    }
    path = buf;
#endif
    archive = PyUnicode_AS_UNICODE(self->archive);
    len = PyUnicode_GET_SIZE(self->archive);
    if ((size_t)len < Py_UNICODE_strlen(path) &&
        Py_UNICODE_strncmp(path, archive, len) == 0 &&
        path[len] == SEP) {
        path += len + 1;
        path_len -= len + 1;
    }

    key = PyUnicode_FromUnicode(path, path_len);
    if (key == NULL)
        return NULL;
    toc_entry = PyDict_GetItem(self->files, key);
    if (toc_entry == NULL) {
        PyErr_SetFromErrnoWithFilenameObject(PyExc_IOError, key);
        Py_DECREF(key);
        return NULL;
    }
    Py_DECREF(key);
    return get_data(self->archive, toc_entry);
}

static PyObject *
zipimporter_get_code(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    char *fullname;

    if (!PyArg_ParseTuple(args, "s:zipimporter.get_code", &fullname))
        return NULL;

    return get_module_code(self, fullname, NULL, NULL);
}

static PyObject *
zipimporter_get_source(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *toc_entry;
    char *fullname, *subname, path[MAXPATHLEN+1];
    int len;
    enum zi_module_info mi;

    if (!PyArg_ParseTuple(args, "s:zipimporter.get_source", &fullname))
        return NULL;

    mi = get_module_info(self, fullname);
    if (mi == MI_ERROR)
        return NULL;
    if (mi == MI_NOT_FOUND) {
        PyErr_Format(ZipImportError, "can't find module '%.200s'",
                     fullname);
        return NULL;
    }
    subname = get_subname(fullname);

    len = make_filename(self->prefix, subname, path);
    if (len < 0)
        return NULL;

    if (mi == MI_PACKAGE) {
        path[len] = SEP;
        strcpy(path + len + 1, "__init__.py");
    }
    else
        strcpy(path + len, ".py");

    toc_entry = PyDict_GetItemString(self->files, path);
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
read_directory(PyObject *archive_obj)
{
    /* FIXME: work on Py_UNICODE* instead of char* */
    PyObject *files = NULL;
    FILE *fp;
    unsigned short flags;
    long compress, crc, data_size, file_size, file_offset, date, time;
    long header_offset, name_size, header_size, header_position;
    long i, l, count;
    size_t length;
    Py_UNICODE path[MAXPATHLEN + 5];
    char name[MAXPATHLEN + 5];
    PyObject *nameobj = NULL;
    char *p, endof_central_dir[22];
    long arc_offset; /* offset from beginning of file to start of zip-archive */
    PyObject *pathobj;
    const char *charset;

    if (PyUnicode_GET_SIZE(archive_obj) > MAXPATHLEN) {
        PyErr_SetString(PyExc_OverflowError,
                        "Zip path name is too long");
        return NULL;
    }
    Py_UNICODE_strcpy(path, PyUnicode_AS_UNICODE(archive_obj));

    fp = _Py_fopen(archive_obj, "rb");
    if (fp == NULL) {
        PyErr_Format(ZipImportError, "can't open Zip file: "
                     "'%.200U'", archive_obj);
        return NULL;
    }
    fseek(fp, -22, SEEK_END);
    header_position = ftell(fp);
    if (fread(endof_central_dir, 1, 22, fp) != 22) {
        fclose(fp);
        PyErr_Format(ZipImportError, "can't read Zip file: "
                     "'%.200U'", archive_obj);
        return NULL;
    }
    if (get_long((unsigned char *)endof_central_dir) != 0x06054B50) {
        /* Bad: End of Central Dir signature */
        fclose(fp);
        PyErr_Format(ZipImportError, "not a Zip file: "
                     "'%.200U'", archive_obj);
        return NULL;
    }

    header_size = get_long((unsigned char *)endof_central_dir + 12);
    header_offset = get_long((unsigned char *)endof_central_dir + 16);
    arc_offset = header_position - header_offset - header_size;
    header_offset += arc_offset;

    files = PyDict_New();
    if (files == NULL)
        goto error;

    length = Py_UNICODE_strlen(path);
    path[length] = SEP;

    /* Start of Central Directory */
    count = 0;
    for (;;) {
        PyObject *t;
        int err;

        fseek(fp, header_offset, 0);  /* Start of file header */
        l = PyMarshal_ReadLongFromFile(fp);
        if (l != 0x02014B50)
            break;              /* Bad: Central Dir File Header */
        fseek(fp, header_offset + 8, 0);
        flags = (unsigned short)PyMarshal_ReadShortFromFile(fp);
        compress = PyMarshal_ReadShortFromFile(fp);
        time = PyMarshal_ReadShortFromFile(fp);
        date = PyMarshal_ReadShortFromFile(fp);
        crc = PyMarshal_ReadLongFromFile(fp);
        data_size = PyMarshal_ReadLongFromFile(fp);
        file_size = PyMarshal_ReadLongFromFile(fp);
        name_size = PyMarshal_ReadShortFromFile(fp);
        header_size = 46 + name_size +
           PyMarshal_ReadShortFromFile(fp) +
           PyMarshal_ReadShortFromFile(fp);
        fseek(fp, header_offset + 42, 0);
        file_offset = PyMarshal_ReadLongFromFile(fp) + arc_offset;
        if (name_size > MAXPATHLEN)
            name_size = MAXPATHLEN;

        p = name;
        for (i = 0; i < name_size; i++) {
            *p = (char)getc(fp);
            if (*p == '/')
                *p = SEP;
            p++;
        }
        *p = 0;         /* Add terminating null byte */
        header_offset += header_size;

        if (flags & 0x0800)
            charset = "utf-8";
        else
            charset = "cp437";
        nameobj = PyUnicode_Decode(name, name_size, charset, NULL);
        if (nameobj == NULL)
            goto error;
        Py_UNICODE_strncpy(path + length + 1, PyUnicode_AS_UNICODE(nameobj), MAXPATHLEN - length - 1);

        pathobj = PyUnicode_FromUnicode(path, Py_UNICODE_strlen(path));
        if (pathobj == NULL)
            goto error;
        t = Py_BuildValue("Niiiiiii", pathobj, compress, data_size,
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
        PySys_FormatStderr("# zipimport: found %ld names in %U\n",
            count, archive_obj);
    return files;
error:
    fclose(fp);
    Py_XDECREF(files);
    Py_XDECREF(nameobj);
    return NULL;
}

/* Return the zlib.decompress function object, or NULL if zlib couldn't
   be imported. The function is cached when found, so subsequent calls
   don't import zlib again. Returns a *borrowed* reference.
   XXX This makes zlib.decompress immortal. */
static PyObject *
get_decompress_func(void)
{
    static PyObject *decompress = NULL;

    if (decompress == NULL) {
        PyObject *zlib;
        static int importing_zlib = 0;

        if (importing_zlib != 0)
            /* Someone has a zlib.py[co] in their Zip file;
               let's avoid a stack overflow. */
            return NULL;
        importing_zlib = 1;
        zlib = PyImport_ImportModuleNoBlock("zlib");
        importing_zlib = 0;
        if (zlib != NULL) {
            decompress = PyObject_GetAttrString(zlib,
                                                "decompress");
            Py_DECREF(zlib);
        }
        else
            PyErr_Clear();
        if (Py_VerboseFlag)
            PySys_WriteStderr("# zipimport: zlib %s\n",
                zlib != NULL ? "available": "UNAVAILABLE");
    }
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
        PyErr_Format(PyExc_IOError,
           "zipimport: can not open file %U", archive);
        return NULL;
    }

    /* Check to make sure the local file header is correct */
    fseek(fp, file_offset, 0);
    l = PyMarshal_ReadLongFromFile(fp);
    if (l != 0x04034B50) {
        /* Bad: Local File Header */
        PyErr_Format(ZipImportError,
                     "bad local file header in %U",
                     archive);
        fclose(fp);
        return NULL;
    }
    fseek(fp, file_offset + 26, 0);
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
    if (err == 0)
        bytes_read = fread(buf, 1, data_size, fp);
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
unmarshal_code(char *pathname, PyObject *data, time_t mtime)
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
            PySys_WriteStderr("# %s has bad magic\n",
                              pathname);
        Py_INCREF(Py_None);
        return Py_None;  /* signal caller to try alternative */
    }

    if (mtime != 0 && !eq_mtime(get_long((unsigned char *)buf + 4),
                                mtime)) {
        if (Py_VerboseFlag)
            PySys_WriteStderr("# %s has bad mtime\n",
                              pathname);
        Py_INCREF(Py_None);
        return Py_None;  /* signal caller to try alternative */
    }

    code = PyMarshal_ReadObjectFromString(buf + 8, size - 8);
    if (code == NULL)
        return NULL;
    if (!PyCode_Check(code)) {
        Py_DECREF(code);
        PyErr_Format(PyExc_TypeError,
             "compiled module %.200s is not a code object",
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
    char *buf, *q, *p = PyBytes_AsString(source);
    PyObject *fixed_source;
    int len = 0;

    if (!p) {
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
compile_source(char *pathname, PyObject *source)
{
    PyObject *code, *fixed_source;

    fixed_source = normalize_line_endings(source);
    if (fixed_source == NULL)
        return NULL;

    code = Py_CompileString(PyBytes_AsString(fixed_source), pathname,
                            Py_file_input);
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
   modifictaion time of the matching .py file, or 0 if no source
   is available. */
static time_t
get_mtime_of_source(ZipImporter *self, char *path)
{
    PyObject *toc_entry;
    time_t mtime = 0;
    Py_ssize_t lastchar = strlen(path) - 1;
    char savechar = path[lastchar];
    path[lastchar] = '\0';  /* strip 'c' or 'o' from *.py[co] */
    toc_entry = PyDict_GetItemString(self->files, path);
    if (toc_entry != NULL && PyTuple_Check(toc_entry) &&
        PyTuple_Size(toc_entry) == 8) {
        /* fetch the time stamp of the .py file for comparison
           with an embedded pyc time stamp */
        int time, date;
        time = PyLong_AsLong(PyTuple_GetItem(toc_entry, 5));
        date = PyLong_AsLong(PyTuple_GetItem(toc_entry, 6));
        mtime = parse_dostime(time, date);
    }
    path[lastchar] = savechar;
    return mtime;
}

/* Return the code object for the module named by 'fullname' from the
   Zip archive as a new reference. */
static PyObject *
get_code_from_data(ZipImporter *self, int ispackage, int isbytecode,
                   time_t mtime, PyObject *toc_entry)
{
    PyObject *data, *code;
    PyObject *modpath;

    data = get_data(self->archive, toc_entry);
    if (data == NULL)
        return NULL;

    modpath = PyUnicode_EncodeFSDefault(PyTuple_GetItem(toc_entry, 0));
    if (modpath == NULL) {
        Py_DECREF(data);
        return NULL;
    }

    if (isbytecode)
        code = unmarshal_code(PyBytes_AS_STRING(modpath), data, mtime);
    else
        code = compile_source(PyBytes_AS_STRING(modpath), data);
    Py_DECREF(modpath);
    Py_DECREF(data);
    return code;
}

/* Get the code object assoiciated with the module specified by
   'fullname'. */
static PyObject *
get_module_code(ZipImporter *self, char *fullname,
                int *p_ispackage, PyObject **p_modpath)
{
    PyObject *toc_entry;
    char *subname, path[MAXPATHLEN + 1];
    int len;
    struct st_zip_searchorder *zso;

    subname = get_subname(fullname);

    len = make_filename(self->prefix, subname, path);
    if (len < 0)
        return NULL;

    for (zso = zip_searchorder; *zso->suffix; zso++) {
        PyObject *code = NULL;

        strcpy(path + len, zso->suffix);
        if (Py_VerboseFlag > 1)
            PySys_FormatStderr("# trying %U%c%s\n",
                               self->archive, (int)SEP, path);
        toc_entry = PyDict_GetItemString(self->files, path);
        if (toc_entry != NULL) {
            time_t mtime = 0;
            int ispackage = zso->type & IS_PACKAGE;
            int isbytecode = zso->type & IS_BYTECODE;

            if (isbytecode)
                mtime = get_mtime_of_source(self, path);
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
            return code;
        }
    }
    PyErr_Format(ZipImportError, "can't find module '%.200s'", fullname);
    return NULL;
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
