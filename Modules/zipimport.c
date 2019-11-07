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
    PyObject *prefix;   /* file prefix: "a/sub/directory/" */
    PyObject *files;    /* dict with file info {path: toc_entry} */
};

static PyObject *ZipImportError;
static PyObject *zip_directory_cache = NULL;

/* forward decls */
static PyObject *read_directory(const char *archive);
static PyObject *get_data(const char *archive, PyObject *toc_entry);
static PyObject *get_module_code(ZipImporter *self, char *fullname,
                                 int *p_ispackage, char **p_modpath);


#define ZipImporter_Check(op) PyObject_TypeCheck(op, &ZipImporter_Type)


/* zipimporter.__init__
   Split the "subdirectory" from the Zip archive path, lookup a matching
   entry in sys.path_importer_cache, fetch the file directory from there
   if found, or else read it from the archive. */
static int
zipimporter_init(ZipImporter *self, PyObject *args, PyObject *kwds)
{
    char *path, *p, *prefix, buf[MAXPATHLEN+2];
    size_t len;

    if (!_PyArg_NoKeywords("zipimporter()", kwds))
        return -1;

    if (!PyArg_ParseTuple(args, "s:zipimporter",
                          &path))
        return -1;

    len = strlen(path);
    if (len == 0) {
        PyErr_SetString(ZipImportError, "archive path is empty");
        return -1;
    }
    if (len >= MAXPATHLEN) {
        PyErr_SetString(ZipImportError,
                        "archive path too long");
        return -1;
    }
    strcpy(buf, path);

#ifdef ALTSEP
    for (p = buf; *p; p++) {
        if (*p == ALTSEP)
            *p = SEP;
    }
#endif

    path = NULL;
    prefix = NULL;
    for (;;) {
#ifndef RISCOS
        struct stat statbuf;
        int rv;

        rv = stat(buf, &statbuf);
        if (rv == 0) {
            /* it exists */
            if (S_ISREG(statbuf.st_mode))
                /* it's a file */
                path = buf;
            break;
        }
#else
        if (object_exists(buf)) {
            /* it exists */
            if (isfile(buf))
                /* it's a file */
                path = buf;
            break;
        }
#endif
        /* back up one path element */
        p = strrchr(buf, SEP);
        if (prefix != NULL)
            *prefix = SEP;
        if (p == NULL)
            break;
        *p = '\0';
        prefix = p;
    }
    if (path != NULL) {
        PyObject *files;
        files = PyDict_GetItemString(zip_directory_cache, path);
        if (files == NULL) {
            files = read_directory(buf);
            if (files == NULL)
                return -1;
            if (PyDict_SetItemString(zip_directory_cache, path,
                                     files) != 0)
                return -1;
        }
        else
            Py_INCREF(files);
        self->files = files;
    }
    else {
        PyErr_SetString(ZipImportError, "not a Zip file");
        return -1;
    }

    if (prefix == NULL)
        prefix = "";
    else {
        prefix++;
        len = strlen(prefix);
        if (prefix[len-1] != SEP) {
            /* add trailing SEP */
            prefix[len] = SEP;
            prefix[len + 1] = '\0';
        }
    }

    self->archive = PyString_FromString(buf);
    if (self->archive == NULL)
        return -1;

    self->prefix = PyString_FromString(prefix);
    if (self->prefix == NULL)
        return -1;

    return 0;
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
    char buf[500];
    char *archive = "???";
    char *prefix = "";

    if (self->archive != NULL && PyString_Check(self->archive))
        archive = PyString_AsString(self->archive);
    if (self->prefix != NULL && PyString_Check(self->prefix))
        prefix = PyString_AsString(self->prefix);
    if (prefix != NULL && *prefix)
        PyOS_snprintf(buf, sizeof(buf),
                      "<zipimporter object \"%.300s%c%.150s\">",
                      archive, SEP, prefix);
    else
        PyOS_snprintf(buf, sizeof(buf),
                      "<zipimporter object \"%.300s\">",
                      archive);
    return PyString_FromString(buf);
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
make_filename(char *prefix, char *name, char *path)
{
    size_t len;
    char *p;

    len = strlen(prefix);

    /* self.prefix + name [+ SEP + "__init__"] + ".py[co]" */
    if (len + strlen(name) + 13 >= MAXPATHLEN) {
        PyErr_SetString(ZipImportError, "path too long");
        return -1;
    }

    strcpy(path, prefix);
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

    len = make_filename(PyString_AsString(self->prefix), subname, path);
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
    PyObject *code, *mod, *dict;
    char *fullname, *modpath;
    int ispackage;

    if (!PyArg_ParseTuple(args, "s:zipimporter.load_module",
                          &fullname))
        return NULL;

    code = get_module_code(self, fullname, &ispackage, &modpath);
    if (code == NULL)
        return NULL;

    mod = PyImport_AddModule(fullname);
    if (mod == NULL) {
        Py_DECREF(code);
        return NULL;
    }
    dict = PyModule_GetDict(mod);

    /* mod.__loader__ = self */
    if (PyDict_SetItemString(dict, "__loader__", (PyObject *)self) != 0)
        goto error;

    if (ispackage) {
        /* add __path__ to the module *before* the code gets
           executed */
        PyObject *pkgpath, *fullpath;
        char *prefix = PyString_AsString(self->prefix);
        char *subname = get_subname(fullname);
        int err;

        fullpath = PyString_FromFormat("%s%c%s%s",
                                PyString_AsString(self->archive),
                                SEP,
                                *prefix ? prefix : "",
                                subname);
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
    mod = PyImport_ExecCodeModuleEx(fullname, code, modpath);
    Py_DECREF(code);
    if (Py_VerboseFlag)
        PySys_WriteStderr("import %s # loaded from Zip %s\n",
                          fullname, modpath);
    return mod;
error:
    Py_DECREF(code);
    Py_DECREF(mod);
    return NULL;
}

/* Return a string matching __file__ for the named module */
static PyObject *
zipimporter_get_filename(PyObject *obj, PyObject *args)
{
    ZipImporter *self = (ZipImporter *)obj;
    PyObject *code;
    char *fullname, *modpath;
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

    return PyString_FromString(modpath);
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
    char *path;
#ifdef ALTSEP
    char *p, buf[MAXPATHLEN + 1];
#endif
    PyObject *toc_entry;
    Py_ssize_t len;

    if (!PyArg_ParseTuple(args, "s:zipimporter.get_data", &path))
        return NULL;

#ifdef ALTSEP
    if (strlen(path) >= MAXPATHLEN) {
        PyErr_SetString(ZipImportError, "path too long");
        return NULL;
    }
    strcpy(buf, path);
    for (p = buf; *p; p++) {
        if (*p == ALTSEP)
            *p = SEP;
    }
    path = buf;
#endif
    len = PyString_Size(self->archive);
    if ((size_t)len < strlen(path) &&
        strncmp(path, PyString_AsString(self->archive), len) == 0 &&
        path[len] == SEP) {
        path = path + len + 1;
    }

    toc_entry = PyDict_GetItemString(self->files, path);
    if (toc_entry == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, path);
        return NULL;
    }
    return get_data(PyString_AsString(self->archive), toc_entry);
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

    len = make_filename(PyString_AsString(self->prefix), subname, path);
    if (len < 0)
        return NULL;

    if (mi == MI_PACKAGE) {
        path[len] = SEP;
        strcpy(path + len + 1, "__init__.py");
    }
    else
        strcpy(path + len, ".py");

    toc_entry = PyDict_GetItemString(self->files, path);
    if (toc_entry != NULL)
        return get_data(PyString_AsString(self->archive), toc_entry);

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
    0,                                          /* tp_compare */
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

/* Given a buffer, return the unsigned int that is represented by the first
   4 bytes, encoded as little endian. This partially reimplements
   marshal.c:r_long() */
static unsigned int
get_uint32(const unsigned char *buf)
{
    unsigned int x;
    x =  buf[0];
    x |= (unsigned int)buf[1] <<  8;
    x |= (unsigned int)buf[2] << 16;
    x |= (unsigned int)buf[3] << 24;
    return x;
}

/* Given a buffer, return the unsigned int that is represented by the first
   2 bytes, encoded as little endian. This partially reimplements
   marshal.c:r_short() */
static unsigned short
get_uint16(const unsigned char *buf)
{
    unsigned short x;
    x =  buf[0];
    x |= (unsigned short)buf[1] <<  8;
    return x;
}

static void
set_file_error(const char *archive, int eof)
{
    if (eof) {
        PyErr_SetString(PyExc_EOFError, "EOF read where not expected");
    }
    else {
        PyErr_SetFromErrnoWithFilename(PyExc_IOError, archive);
    }
}

/*
   read_directory(archive) -> files dict (new reference)

   Given a path to a Zip archive, build a dict, mapping file names
   (local to the archive, using SEP as a separator) to toc entries.

   A toc_entry is a tuple:

   (__file__,      # value to use for __file__, available for all files
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
read_directory(const char *archive)
{
    PyObject *files = NULL;
    FILE *fp;
    unsigned short compress, time, date, name_size;
    unsigned int crc, data_size, file_size, header_size, header_offset;
    unsigned long file_offset, header_position;
    unsigned long arc_offset;  /* Absolute offset to start of the zip-archive. */
    unsigned int count, i;
    unsigned char buffer[46];
    size_t length;
    char name[MAXPATHLEN + 1];
    char path[2*MAXPATHLEN + 2]; /* archive + SEP + name + '\0' */
    const char *errmsg = NULL;

    if (strlen(archive) > MAXPATHLEN) {
        PyErr_SetString(PyExc_OverflowError,
                        "Zip path name is too long");
        return NULL;
    }
    strcpy(path, archive);

    fp = fopen(archive, "rb");
    if (fp == NULL) {
        PyErr_Format(ZipImportError, "can't open Zip file: "
                     "'%.200s'", archive);
        return NULL;
    }

    if (fseek(fp, -22, SEEK_END) == -1) {
        goto file_error;
    }
    header_position = (unsigned long)ftell(fp);
    if (header_position == (unsigned long)-1) {
        goto file_error;
    }
    assert(header_position <= (unsigned long)LONG_MAX);
    if (fread(buffer, 1, 22, fp) != 22) {
        goto file_error;
    }
    if (get_uint32(buffer) != 0x06054B50u) {
        /* Bad: End of Central Dir signature */
        errmsg = "not a Zip file";
        goto invalid_header;
    }

    header_size = get_uint32(buffer + 12);
    header_offset = get_uint32(buffer + 16);
    if (header_position < header_size) {
        errmsg = "bad central directory size";
        goto invalid_header;
    }
    if (header_position < header_offset) {
        errmsg = "bad central directory offset";
        goto invalid_header;
    }
    if (header_position - header_size < header_offset) {
        errmsg = "bad central directory size or offset";
        goto invalid_header;
    }
    header_position -= header_size;
    arc_offset = header_position - header_offset;

    files = PyDict_New();
    if (files == NULL) {
        goto error;
    }

    length = (long)strlen(path);
    path[length] = SEP;

    /* Start of Central Directory */
    count = 0;
    if (fseek(fp, (long)header_position, 0) == -1) {
        goto file_error;
    }
    for (;;) {
        PyObject *t;
        size_t n;
        int err;

        n = fread(buffer, 1, 46, fp);
        if (n < 4) {
            goto eof_error;
        }
        /* Start of file header */
        if (get_uint32(buffer) != 0x02014B50u) {
            break;              /* Bad: Central Dir File Header */
        }
        if (n != 46) {
            goto eof_error;
        }
        compress = get_uint16(buffer + 10);
        time = get_uint16(buffer + 12);
        date = get_uint16(buffer + 14);
        crc = get_uint32(buffer + 16);
        data_size = get_uint32(buffer + 20);
        file_size = get_uint32(buffer + 24);
        name_size = get_uint16(buffer + 28);
        header_size = (unsigned int)name_size +
           get_uint16(buffer + 30) /* extra field */ +
           get_uint16(buffer + 32) /* comment */;

        file_offset = get_uint32(buffer + 42);
        if (file_offset > header_offset) {
            errmsg = "bad local header offset";
            goto invalid_header;
        }
        file_offset += arc_offset;

        if (name_size > MAXPATHLEN) {
            name_size = MAXPATHLEN;
        }
        if (fread(name, 1, name_size, fp) != name_size) {
            goto file_error;
        }
        name[name_size] = '\0';  /* Add terminating null byte */
        if (SEP != '/') {
            for (i = 0; i < name_size; i++) {
                if (name[i] == '/') {
                    name[i] = SEP;
                }
            }
        }
        /* Skip the rest of the header.
         * On Windows, calling fseek to skip over the fields we don't use is
         * slower than reading the data because fseek flushes stdio's
         * internal buffers.  See issue #8745. */
        assert(header_size <= 3*0xFFFFu);
        for (i = name_size; i < header_size; i++) {
            if (getc(fp) == EOF) {
                goto file_error;
            }
        }

        memcpy(path + length + 1, name, name_size + 1);

        t = Py_BuildValue("sHIIkHHI", path, compress, data_size,
                          file_size, file_offset, time, date, crc);
        if (t == NULL) {
            goto error;
        }
        err = PyDict_SetItemString(files, name, t);
        Py_DECREF(t);
        if (err != 0) {
            goto error;
        }
        count++;
    }
    fclose(fp);
    if (Py_VerboseFlag) {
        PySys_WriteStderr("# zipimport: found %u names in %.200s\n",
                           count, archive);
    }
    return files;

eof_error:
    set_file_error(archive, !ferror(fp));
    goto error;

file_error:
    PyErr_Format(ZipImportError, "can't read Zip file: %.200s", archive);
    goto error;

invalid_header:
    assert(errmsg != NULL);
    PyErr_Format(ZipImportError, "%s: %.200s", errmsg, archive);
    goto error;

error:
    fclose(fp);
    Py_XDECREF(files);
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
get_data(const char *archive, PyObject *toc_entry)
{
    PyObject *raw_data = NULL, *data, *decompress;
    char *buf;
    FILE *fp;
    const char *datapath;
    unsigned short compress, time, date;
    unsigned int crc;
    Py_ssize_t data_size, file_size;
    long file_offset, header_size;
    unsigned char buffer[30];
    const char *errmsg = NULL;

    if (!PyArg_ParseTuple(toc_entry, "sHnnlHHI", &datapath, &compress,
                          &data_size, &file_size, &file_offset, &time,
                          &date, &crc)) {
        return NULL;
    }
    if (data_size < 0) {
        PyErr_Format(ZipImportError, "negative data size");
        return NULL;
    }

    fp = fopen(archive, "rb");
    if (!fp) {
        PyErr_Format(PyExc_IOError,
           "zipimport: can not open file %s", archive);
        return NULL;
    }

    /* Check to make sure the local file header is correct */
    if (fseek(fp, file_offset, 0) == -1) {
        goto file_error;
    }
    if (fread(buffer, 1, 30, fp) != 30) {
        goto eof_error;
    }
    if (get_uint32(buffer) != 0x04034B50u) {
        /* Bad: Local File Header */
        errmsg = "bad local file header";
        goto invalid_header;
    }

    header_size = (unsigned int)30 +
        get_uint16(buffer + 26) /* file name */ +
        get_uint16(buffer + 28) /* extra field */;
    if (file_offset > LONG_MAX - header_size) {
        errmsg = "bad local file header size";
        goto invalid_header;
    }
    file_offset += header_size;  /* Start of file data */

    if (data_size > LONG_MAX - 1) {
        fclose(fp);
        PyErr_NoMemory();
        return NULL;
    }
    raw_data = PyString_FromStringAndSize((char *)NULL, compress == 0 ?
                                          data_size : data_size + 1);

    if (raw_data == NULL) {
        goto error;
    }
    buf = PyString_AsString(raw_data);

    if (fseek(fp, file_offset, 0) == -1) {
        goto file_error;
    }
    if (fread(buf, 1, data_size, fp) != (size_t)data_size) {
        PyErr_SetString(PyExc_IOError,
                        "zipimport: can't read data");
        goto error;
    }

    fclose(fp);
    fp = NULL;

    if (compress != 0) {
        buf[data_size] = 'Z';  /* saw this in zipfile.py */
        data_size++;
    }
    buf[data_size] = '\0';

    if (compress == 0)  /* data is not compressed */
        return raw_data;

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
    Py_DECREF(raw_data);
    return data;

eof_error:
    set_file_error(archive, !ferror(fp));
    goto error;

file_error:
    PyErr_Format(ZipImportError, "can't read Zip file: %.200s", archive);
    goto error;

invalid_header:
    assert(errmsg != NULL);
    PyErr_Format(ZipImportError, "%s: %.200s", errmsg, archive);
    goto error;

error:
    if (fp != NULL) {
        fclose(fp);
    }
    Py_XDECREF(raw_data);
    return NULL;
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
unmarshal_code(const char *pathname, PyObject *data, time_t mtime)
{
    PyObject *code;
    unsigned char *buf = (unsigned char *)PyString_AsString(data);
    Py_ssize_t size = PyString_Size(data);

    if (size < 8) {
        PyErr_SetString(ZipImportError,
                        "bad pyc data");
        return NULL;
    }

    if (get_uint32(buf) != (unsigned int)PyImport_GetMagicNumber()) {
        if (Py_VerboseFlag) {
            PySys_WriteStderr("# %s has bad magic\n",
                              pathname);
        }
        Py_INCREF(Py_None);
        return Py_None;  /* signal caller to try alternative */
    }

    if (mtime != 0 && !eq_mtime(get_uint32(buf + 4), mtime)) {
        if (Py_VerboseFlag) {
            PySys_WriteStderr("# %s has bad mtime\n",
                              pathname);
        }
        Py_INCREF(Py_None);
        return Py_None;  /* signal caller to try alternative */
    }

    code = PyMarshal_ReadObjectFromString((char *)buf + 8, size - 8);
    if (code == NULL) {
        return NULL;
    }
    if (!PyCode_Check(code)) {
        Py_DECREF(code);
        PyErr_Format(PyExc_TypeError,
             "compiled module %.200s is not a code object",
             pathname);
        return NULL;
    }
    return code;
}

/* Replace any occurrences of "\r\n?" in the input string with "\n".
   This converts DOS and Mac line endings to Unix line endings.
   Also append a trailing "\n" to be compatible with
   PyParser_SimpleParseFile(). Returns a new reference. */
static PyObject *
normalize_line_endings(PyObject *source)
{
    char *buf, *q, *p = PyString_AsString(source);
    PyObject *fixed_source;

    if (!p)
        return NULL;

    /* one char extra for trailing \n and one for terminating \0 */
    buf = (char *)PyMem_Malloc(PyString_Size(source) + 2);
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
    }
    *q++ = '\n';  /* add trailing \n */
    *q = '\0';
    fixed_source = PyString_FromString(buf);
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

    code = Py_CompileString(PyString_AsString(fixed_source), pathname,
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
   modification time of the matching .py file, or 0 if no source
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
        time = PyInt_AsLong(PyTuple_GetItem(toc_entry, 5));
        date = PyInt_AsLong(PyTuple_GetItem(toc_entry, 6));
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
    char *modpath;
    char *archive = PyString_AsString(self->archive);

    if (archive == NULL)
        return NULL;

    data = get_data(archive, toc_entry);
    if (data == NULL)
        return NULL;

    modpath = PyString_AsString(PyTuple_GetItem(toc_entry, 0));

    if (isbytecode) {
        code = unmarshal_code(modpath, data, mtime);
    }
    else {
        code = compile_source(modpath, data);
    }
    Py_DECREF(data);
    return code;
}

/* Get the code object associated with the module specified by
   'fullname'. */
static PyObject *
get_module_code(ZipImporter *self, char *fullname,
                int *p_ispackage, char **p_modpath)
{
    PyObject *toc_entry;
    char *subname, path[MAXPATHLEN + 1];
    int len;
    struct st_zip_searchorder *zso;

    subname = get_subname(fullname);

    len = make_filename(PyString_AsString(self->prefix), subname, path);
    if (len < 0)
        return NULL;

    for (zso = zip_searchorder; *zso->suffix; zso++) {
        PyObject *code = NULL;

        strcpy(path + len, zso->suffix);
        if (Py_VerboseFlag > 1)
            PySys_WriteStderr("# trying %s%c%s\n",
                              PyString_AsString(self->archive),
                              SEP, path);
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
            if (code != NULL && p_modpath != NULL)
                *p_modpath = PyString_AsString(
                    PyTuple_GetItem(toc_entry, 0));
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

PyMODINIT_FUNC
initzipimport(void)
{
    PyObject *mod;

    if (PyType_Ready(&ZipImporter_Type) < 0)
        return;

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

    mod = Py_InitModule4("zipimport", NULL, zipimport_doc,
                         NULL, PYTHON_API_VERSION);
    if (mod == NULL)
        return;

    ZipImportError = PyErr_NewException("zipimport.ZipImportError",
                                        PyExc_ImportError, NULL);
    if (ZipImportError == NULL)
        return;

    Py_INCREF(ZipImportError);
    if (PyModule_AddObject(mod, "ZipImportError",
                           ZipImportError) < 0)
        return;

    Py_INCREF(&ZipImporter_Type);
    if (PyModule_AddObject(mod, "zipimporter",
                           (PyObject *)&ZipImporter_Type) < 0)
        return;

    zip_directory_cache = PyDict_New();
    if (zip_directory_cache == NULL)
        return;
    Py_INCREF(zip_directory_cache);
    if (PyModule_AddObject(mod, "_zip_directory_cache",
                           zip_directory_cache) < 0)
        return;
}
