/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(zipimport_zipimporter___init____doc__,
"zipimporter(archivepath, /)\n"
"--\n"
"\n"
"Create a new zipimporter instance.\n"
"\n"
"  archivepath\n"
"    A path-like object to a zipfile, or to a specific path inside\n"
"    a zipfile.\n"
"\n"
"\'archivepath\' must be a path-like object to a zipfile, or to a specific path\n"
"inside a zipfile. For example, it can be \'/tmp/myimport.zip\', or\n"
"\'/tmp/myimport.zip/mydirectory\', if mydirectory is a valid directory inside\n"
"the archive.\n"
"\n"
"\'ZipImportError\' is raised if \'archivepath\' doesn\'t point to a valid Zip\n"
"archive.\n"
"\n"
"The \'archive\' attribute of the zipimporter object contains the name of the\n"
"zipfile targeted.");

static int
zipimport_zipimporter___init___impl(ZipImporter *self, PyObject *path);

static int
zipimport_zipimporter___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    PyObject *path;

    if ((Py_TYPE(self) == &ZipImporter_Type) &&
        !_PyArg_NoKeywords("zipimporter", kwargs)) {
        goto exit;
    }
    if (!PyArg_ParseTuple(args, "O&:zipimporter",
        PyUnicode_FSDecoder, &path)) {
        goto exit;
    }
    return_value = zipimport_zipimporter___init___impl((ZipImporter *)self, path);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_find_module__doc__,
"find_module($self, fullname, path=None, /)\n"
"--\n"
"\n"
"Search for a module specified by \'fullname\'.\n"
"\n"
"\'fullname\' must be the fully qualified (dotted) module name. It returns the\n"
"zipimporter instance itself if the module was found, or None if it wasn\'t.\n"
"The optional \'path\' argument is ignored -- it\'s there for compatibility\n"
"with the importer protocol.");

#define ZIPIMPORT_ZIPIMPORTER_FIND_MODULE_METHODDEF    \
    {"find_module", (PyCFunction)zipimport_zipimporter_find_module, METH_FASTCALL, zipimport_zipimporter_find_module__doc__},

static PyObject *
zipimport_zipimporter_find_module_impl(ZipImporter *self, PyObject *fullname,
                                       PyObject *path);

static PyObject *
zipimport_zipimporter_find_module(ZipImporter *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *fullname;
    PyObject *path = Py_None;

    if (!_PyArg_ParseStack(args, nargs, "U|O:find_module",
        &fullname, &path)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_find_module_impl(self, fullname, path);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_find_loader__doc__,
"find_loader($self, fullname, path=None, /)\n"
"--\n"
"\n"
"Search for a module specified by \'fullname\'.\n"
"\n"
"\'fullname\' must be the fully qualified (dotted) module name. It returns the\n"
"zipimporter instance itself if the module was found, a string containing the\n"
"full path name if it\'s possibly a portion of a namespace package,\n"
"or None otherwise. The optional \'path\' argument is ignored -- it\'s\n"
"there for compatibility with the importer protocol.");

#define ZIPIMPORT_ZIPIMPORTER_FIND_LOADER_METHODDEF    \
    {"find_loader", (PyCFunction)zipimport_zipimporter_find_loader, METH_FASTCALL, zipimport_zipimporter_find_loader__doc__},

static PyObject *
zipimport_zipimporter_find_loader_impl(ZipImporter *self, PyObject *fullname,
                                       PyObject *path);

static PyObject *
zipimport_zipimporter_find_loader(ZipImporter *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *fullname;
    PyObject *path = Py_None;

    if (!_PyArg_ParseStack(args, nargs, "U|O:find_loader",
        &fullname, &path)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_find_loader_impl(self, fullname, path);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_load_module__doc__,
"load_module($self, fullname, /)\n"
"--\n"
"\n"
"Load the module specified by \'fullname\'.\n"
"\n"
"\'fullname\' must be the fully qualified (dotted) module name. It returns the\n"
"imported module, or raises ZipImportError if it wasn\'t found.");

#define ZIPIMPORT_ZIPIMPORTER_LOAD_MODULE_METHODDEF    \
    {"load_module", (PyCFunction)zipimport_zipimporter_load_module, METH_O, zipimport_zipimporter_load_module__doc__},

static PyObject *
zipimport_zipimporter_load_module_impl(ZipImporter *self, PyObject *fullname);

static PyObject *
zipimport_zipimporter_load_module(ZipImporter *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *fullname;

    if (!PyArg_Parse(arg, "U:load_module", &fullname)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_load_module_impl(self, fullname);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_get_filename__doc__,
"get_filename($self, fullname, /)\n"
"--\n"
"\n"
"Return the filename for the specified module.");

#define ZIPIMPORT_ZIPIMPORTER_GET_FILENAME_METHODDEF    \
    {"get_filename", (PyCFunction)zipimport_zipimporter_get_filename, METH_O, zipimport_zipimporter_get_filename__doc__},

static PyObject *
zipimport_zipimporter_get_filename_impl(ZipImporter *self,
                                        PyObject *fullname);

static PyObject *
zipimport_zipimporter_get_filename(ZipImporter *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *fullname;

    if (!PyArg_Parse(arg, "U:get_filename", &fullname)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_get_filename_impl(self, fullname);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_is_package__doc__,
"is_package($self, fullname, /)\n"
"--\n"
"\n"
"Return True if the module specified by fullname is a package.\n"
"\n"
"Raise ZipImportError if the module couldn\'t be found.");

#define ZIPIMPORT_ZIPIMPORTER_IS_PACKAGE_METHODDEF    \
    {"is_package", (PyCFunction)zipimport_zipimporter_is_package, METH_O, zipimport_zipimporter_is_package__doc__},

static PyObject *
zipimport_zipimporter_is_package_impl(ZipImporter *self, PyObject *fullname);

static PyObject *
zipimport_zipimporter_is_package(ZipImporter *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *fullname;

    if (!PyArg_Parse(arg, "U:is_package", &fullname)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_is_package_impl(self, fullname);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_get_data__doc__,
"get_data($self, pathname, /)\n"
"--\n"
"\n"
"Return the data associated with \'pathname\'.\n"
"\n"
"Raise OSError if the file was not found.");

#define ZIPIMPORT_ZIPIMPORTER_GET_DATA_METHODDEF    \
    {"get_data", (PyCFunction)zipimport_zipimporter_get_data, METH_O, zipimport_zipimporter_get_data__doc__},

static PyObject *
zipimport_zipimporter_get_data_impl(ZipImporter *self, PyObject *path);

static PyObject *
zipimport_zipimporter_get_data(ZipImporter *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *path;

    if (!PyArg_Parse(arg, "U:get_data", &path)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_get_data_impl(self, path);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_get_code__doc__,
"get_code($self, fullname, /)\n"
"--\n"
"\n"
"Return the code object for the specified module.\n"
"\n"
"Raise ZipImportError if the module couldn\'t be found.");

#define ZIPIMPORT_ZIPIMPORTER_GET_CODE_METHODDEF    \
    {"get_code", (PyCFunction)zipimport_zipimporter_get_code, METH_O, zipimport_zipimporter_get_code__doc__},

static PyObject *
zipimport_zipimporter_get_code_impl(ZipImporter *self, PyObject *fullname);

static PyObject *
zipimport_zipimporter_get_code(ZipImporter *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *fullname;

    if (!PyArg_Parse(arg, "U:get_code", &fullname)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_get_code_impl(self, fullname);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_get_source__doc__,
"get_source($self, fullname, /)\n"
"--\n"
"\n"
"Return the source code for the specified module.\n"
"\n"
"Raise ZipImportError if the module couldn\'t be found, return None if the\n"
"archive does contain the module, but has no source for it.");

#define ZIPIMPORT_ZIPIMPORTER_GET_SOURCE_METHODDEF    \
    {"get_source", (PyCFunction)zipimport_zipimporter_get_source, METH_O, zipimport_zipimporter_get_source__doc__},

static PyObject *
zipimport_zipimporter_get_source_impl(ZipImporter *self, PyObject *fullname);

static PyObject *
zipimport_zipimporter_get_source(ZipImporter *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *fullname;

    if (!PyArg_Parse(arg, "U:get_source", &fullname)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_get_source_impl(self, fullname);

exit:
    return return_value;
}

PyDoc_STRVAR(zipimport_zipimporter_get_resource_reader__doc__,
"get_resource_reader($self, fullname, /)\n"
"--\n"
"\n"
"Return the ResourceReader for a package in a zip file.\n"
"\n"
"If \'fullname\' is a package within the zip file, return the \'ResourceReader\'\n"
"object for the package.  Otherwise return None.");

#define ZIPIMPORT_ZIPIMPORTER_GET_RESOURCE_READER_METHODDEF    \
    {"get_resource_reader", (PyCFunction)zipimport_zipimporter_get_resource_reader, METH_O, zipimport_zipimporter_get_resource_reader__doc__},

static PyObject *
zipimport_zipimporter_get_resource_reader_impl(ZipImporter *self,
                                               PyObject *fullname);

static PyObject *
zipimport_zipimporter_get_resource_reader(ZipImporter *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *fullname;

    if (!PyArg_Parse(arg, "U:get_resource_reader", &fullname)) {
        goto exit;
    }
    return_value = zipimport_zipimporter_get_resource_reader_impl(self, fullname);

exit:
    return return_value;
}
/*[clinic end generated code: output=0b57adfe21373512 input=a9049054013a1b77]*/
