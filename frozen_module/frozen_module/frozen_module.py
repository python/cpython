#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Main module."""
import ast
import inspect
import marshal
import os
import pathlib
import py_compile
import subprocess

from . import _serializer

from typing import Any, List, Optional, Tuple


MACROS = """
#define PyObject_HEAD_INIT_RC(rc, type)  \
    { _PyObject_EXTRA_INIT               \
      rc, type },

#define PyVarObject_HEAD_INIT_RC(rc, type, size)  \
    { PyObject_HEAD_INIT_RC(rc, type) size },

#define FROM_GC(g) ((PyObject *)(((PyGC_Head *)g)+1))
#define _GC_UNTRACKED \
    ((0 & ~_PyGC_REFS_MASK) | \
    (((size_t)(_PyGC_REFS_UNTRACKED)) << _PyGC_REFS_SHIFT))
""".lstrip()

TO_HASH_TEMPLATE = """
struct _to_hash {{
    PyObject *obj;
    Py_ssize_t offset;
}};

static void intitialize_hashes(const struct _to_hash *th)
{{
    PySetObject *obj;
    setentry *se;
    Py_hash_t hash;;
    const struct _to_hash *p;
    for (p = th; ;p++) {{
        if (p->obj == NULL)
            break;
        obj = (PySetObject*)p->obj;
        se = obj->table + p->offset;
        hash = PyObject_Hash(se->key);
        se->hash = hash;
    }}
}}

static const struct _to_hash _ToHash[] = {to_hash};
"""


class Refcounted:
    def __init__(self):
        self.ref_count = 0

    def inc_ref(self) -> None:
        self.ref_count += 1

    def ensure_nonzero(self):
        if self.ref_count == 0:
            self.inc_ref()


class RefcountedFwdDeclaration(Refcounted):
    def __init__(self, template):
        super().__init__()
        self.template = template

    def __str__(self) -> str:
        return self.template % (self.ref_count, )


class Serializer:
    def __init__(self):
        self.symbol_map = {}
        self.symbol_idx = 0
        self._none_refs = 0
        self._true_refs = 0
        self._false_refs = 0
        self._ellipsis_refs = 0
        self._to_hash = []
        self._forward_declarations = [
            MACROS,
        ]
        self._constants_map = {}

    @property
    def forward_declarations(self):
        for fwd in self._forward_declarations:
            if isinstance(fwd, RefcountedFwdDeclaration):
                fwd.ensure_nonzero()
                yield str(fwd)
            else:
                yield fwd

    def _gensym_incr(self, typ: str) -> None:
        if typ not in self.symbol_map:
            self.symbol_map[typ] = 1
        else:
            self.symbol_map[typ] += 1

    def gensym(self, typ: str) -> str:
        self._gensym_incr(typ)
        return f"_{typ}_obj_{self.symbol_map[typ]}"

    def store_ref(self, obj, sym) -> None:
        self._constants_map[(obj, type(obj))] = (
            sym, self._forward_declarations[-1], obj)

    def load_ref(self, obj) -> str:
        sym, fwd_declaration, _ = self._constants_map[(obj, type(obj))]
        fwd_declaration.inc_ref()
        return f"&{sym}"

    def has_ref(self, obj) -> bool:
        if (obj, type(obj)) in self._constants_map:
            if not isinstance(obj, tuple):
                return True
            _, _, obj1 = self._constants_map[(obj, type(obj))]
            return all(isinstance(x[0], type(x[1])) for x in zip(obj, obj1))
        return False

    def _get_int_ref(self, obj) -> str:
        if not self.has_ref(obj):
            ndigits, long_repr = _serializer.serialize_long(obj)
            sym = self.gensym('long')
            if ndigits <= 0:
                self._forward_declarations.append(
                    RefcountedFwdDeclaration(
                        f"/* {obj!r} */\n"
                        f"static struct _longobject {sym}"
                        f" = {long_repr};\n"))
            else:
                self._forward_declarations.append(
                    RefcountedFwdDeclaration(
                        f"/* {obj!r} */\n"
                        f"static struct {{\n"
                        f"PyObject_VAR_HEAD "
                        f"digit ob_digit[{ndigits}]; \n}} "
                        f"{sym} = {long_repr};\n"))
            self.store_ref(obj, sym)
        return self.load_ref(obj)

    def _get_float_ref(self, obj) -> str:
        if not self.has_ref(obj):
            float_repr = _serializer.serialize_float(obj)
            sym = self.gensym('float')
            self._forward_declarations.append(
                RefcountedFwdDeclaration(
                    f"/* {obj!r} */\n"
                    f"static PyFloatObject {sym} = {float_repr};"))
            self.store_ref(obj, sym)
        return self.load_ref(obj)

    def _get_bytes_ref(self, obj) -> str:
        if not self.has_ref(obj):
            nbytes, bytes_repr = _serializer.serialize_bytes(obj)
            sym = self.gensym('bytes')
            if nbytes <= 1:
                self._forward_declarations.append(
                    RefcountedFwdDeclaration(
                        f"/* {self._repr_comment(obj)} */\n"
                        f"static PyBytesObject "
                        f"{sym} = {bytes_repr};\n"))
            else:
                self._forward_declarations.append(
                    RefcountedFwdDeclaration(
                        f"/* {self._repr_comment(obj)} */\n"
                        f"static struct {{\n"
                        f"  PyObject_VAR_HEAD;\n"
                        f"  Py_hash_t ob_shash;" "\n"
                        f"  char ob_sval[{nbytes}];\n}} "
                        f"{sym} = {bytes_repr};\n"))
            self.store_ref(obj, sym)
        return self.load_ref(obj)

    @staticmethod
    def _repr_comment(obj) -> str:
        if isinstance(obj, str):
            if '/*' in obj or '*/' in obj or '//' in obj:
                return 'str(...)'
        return repr(obj).replace('%', '%%')

    _GC_PREFIX = (
        "static struct {\n"
        "    struct {\n"
        "      void *next;\n"
        "      void *prev;\n"
        "      Py_ssize_t gc_refs;\n"
        "    };\n")

    def _get_tuple_ref(self, obj) -> str:
        hashable = True
        try:
            hash(obj)
        except TypeError:
            hashable = False
        if not hashable or not self.has_ref(obj):
            nitems, gc_head_size, tuple_repr = _serializer.serialize_tuple(
                obj, self._get_ref)
            sym = self.gensym('tuple')
            if nitems == 0:
                prefix = (
                    self._GC_PREFIX +
                    f"    PyTupleObject obj;\n}} ")
            else:
                prefix = (
                    self._GC_PREFIX +
                    f"    PyObject_VAR_HEAD;\n"
                    f"    PyObject *ob_item[{nitems}];\n}} ")
            self._forward_declarations.append(
                RefcountedFwdDeclaration(
                    prefix + f"{sym} = {tuple_repr};\n"))
            if hashable:
                self.store_ref(obj, sym)
        return f"FROM_GC({self.load_ref(obj)})"

    def _get_frozenset_ref(self, obj) -> str:
        if not self.has_ref(obj):
            sym = self.gensym('frozenset')
            to_hash, fz_set_repr = \
                _serializer.serialize_frozenset(sym, obj,
                                                self._get_ref,
                                                self.static_hash)
            if to_hash:
                self._to_hash.extend(to_hash)
            self._forward_declarations.append(
                RefcountedFwdDeclaration(
                    self._GC_PREFIX +
                    f"    PySetObject obj;\n}} "
                    f"{sym} = {fz_set_repr};\n"))
            self.store_ref(obj, sym)
        return f"FROM_GC({self.load_ref(obj)})"

    def _get_str_ref(self, obj) -> str:
        if not self.has_ref(obj):
            sym = self.gensym('unicode')
            obj_type, extra_char_type, extra_char_count, body = \
                _serializer.serialize_unicode(obj)
            self._forward_declarations.append(
                RefcountedFwdDeclaration(
                    f"/* {self._repr_comment(obj)} */\n"
                    f"static struct {{\n"
                    f"  {obj_type} data;\n"
                    f"  {extra_char_type} extra_data[{extra_char_count}];\n}} "
                    f"{sym} = {body};\n"))
            self.store_ref(obj, sym)
        return self.load_ref(obj)

    def _get_code_ref(self, obj) -> str:
        if not self.has_ref(obj):
            body = _serializer.serialize_code(obj, self._get_ref)
            sym = self.gensym('code')
            self._forward_declarations.append(
                RefcountedFwdDeclaration(
                    f"static PyCodeObject "
                    f"{sym} = {body};\n"))
            self.store_ref(obj, sym)
        return self.load_ref(obj)

    def _get_complex_ref(self, obj) -> str:
        if not self.has_ref(obj):
            complex_repr = _serializer.serialize_complex(obj)
            sym = self.gensym('complex')
            self._forward_declarations.append(
                RefcountedFwdDeclaration(
                    f"/* {obj!r} */\n"
                    f"static PyComplexObject {sym} = {complex_repr};"))
            self.store_ref(obj, sym)
        return self.load_ref(obj)

    def _serialize(self, obj) -> str:
        return self._get_ref(obj)

    def _get_ref(self, obj) -> str:
        t = type(obj)
        if obj is None:
            self._none_refs += 1
            s = "Py_None"
        elif obj is Ellipsis:
            self._ellipsis_refs += 1
            s = "Py_Ellipsis"
        elif obj is True:
            self._true_refs += 1
            s = "Py_True"
        elif obj is False:
            self._false_refs += 1
            s = "Py_False"
        elif t is int:
            s = self._get_int_ref(obj)
        elif t is float:
            s = self._get_float_ref(obj)
        elif t is bytes:
            s = self._get_bytes_ref(obj)
        elif t is tuple:
            s = self._get_tuple_ref(obj)
        elif t is str:
            s = self._get_str_ref(obj)
        elif t is complex:
            s = self._get_complex_ref(obj)
        elif t is frozenset:
            s = self._get_frozenset_ref(obj)
        elif inspect.iscode(obj):
            s = self._get_code_ref(obj)
        else:
            raise TypeError(f"Cannot serialize {obj}")
        return s

    @classmethod
    def can_statically_hash(cls, obj: Any) -> bool:
        """
        Returns True if the hash value of an object isn't randomized
        at python startup. Returns True for all hashable objects that are not
        strings or bytes and don't contain strings or bytes.
        """
        if isinstance(obj, (str, bytes)) or inspect.iscode(obj):
            return False
        if isinstance(obj, (tuple, frozenset)):
            return all(cls.can_statically_hash(x) for x in obj)
        try:
            hash(obj)
        except TypeError:
            return False
        return True

    @classmethod
    def static_hash(cls, obj: Any) -> int:
        if cls.can_statically_hash(obj):
            return hash(obj)
        return -1

    @property
    def extra_refs(self) -> List[str]:
        refs = []
        spacing = "    "
        if self._none_refs > 0:
            refs.append(f"{spacing}_Py_NoneStruct.ob_refcnt += "
                        f"{self._none_refs};")
        if self._true_refs > 0:
            refs.append(f"{spacing}((PyObject *)Py_True)->ob_refcnt += "
                        f"{self._true_refs};")
        if self._false_refs > 0:
            refs.append(f"{spacing}((PyObject *)Py_False)->ob_refcnt += "
                        f"{self._false_refs};")
        if self._ellipsis_refs > 0:
            refs.append(f"{spacing}_Py_EllipsisObject.ob_refcnt += "
                        f"{self._ellipsis_refs};")
        return refs

    @property
    def to_hash(self):
        if not self._to_hash:
            return "{{0,0}}"
        else:
            s = ", \n    ".join(self._to_hash)
            return f"{{\n{s},\n    {{0,0}}\n}}"

    def finalize(self):
        self._forward_declarations.append(
            TO_HASH_TEMPLATE.format(to_hash=self.to_hash))

    @classmethod
    def serialize(cls, o) -> Tuple[str, str]:
        obj = cls()
        s = obj._serialize(o)
        obj.finalize()
        return "\n".join(obj.forward_declarations), s

    @staticmethod
    def get_pyc_file(py_file_name: str) -> str:
        if py_file_name.endswith(".pyc"):
            return py_file_name
        else:
            return py_compile.compile(py_file_name)

    _INIT_FILE_NAMES = ('__init__.py', '__init__.pyc', '__init__.pyo')

    def _run_target_py_command(self, py_exe_file: str, cmd: str) -> Any:
        py_exe_path = pathlib.Path(py_exe_file)
        if not py_exe_path.is_file():
            raise RuntimeError(f"Could not find python binary {py_exe_file}")
        proc = subprocess.run([py_exe_file, "-c", cmd],
                              stdout=subprocess.PIPE)
        return ast.literal_eval(proc.stdout.decode("utf-8"))

    def _get_spec(self, py_exe_file: str, mod_name: str) -> Tuple[str, str]:
        cmds = [
            "from importlib.util import find_spec",
            f"spec = find_spec(\'{mod_name}\')",
            "print(repr((spec.origin, spec.cached)))"
        ]
        return self._run_target_py_command(py_exe_file, ";".join(cmds))

    def _get_startup_modules(self, py_exe_file: str,
                             extra_modules: Optional[List[str]]=None):
        bad_modules = {
            'encodings',
            'collections',
            'collections.abc',
            '_collections_abc',
        }
        cmds = [
            "import sys",
            "print("
            "{x: sys.modules[x].__file__ "
            "for x in sys.modules if "
            "getattr(sys.modules[x], '__file__', '').endswith("
            "('.py', '.pyc', '.pyo'))})"
        ]
        if extra_modules is not None:
            for mod in extra_modules:
                cmds.insert(0, f"import {mod}")
        modules = self._run_target_py_command(py_exe_file, ";".join(cmds))
        for mod_name, py_file in modules.items():
            spec = self._get_spec(py_exe_file, mod_name)
            if mod_name in bad_modules:
                continue
            yield mod_name, py_file, spec

    def _get_bootstrap_root_path(self, py_exe_file: str):
        py_exe_path = pathlib.Path(py_exe_file)
        if not py_exe_path.is_file():
            raise RuntimeError(f"Could not find python binary {py_exe_file}")

        bootstrap_dir = py_exe_path.parent / "Lib" / "importlib"
        if bootstrap_dir.is_dir():
            return bootstrap_dir
        else:
            bootstrap_dir = pathlib.Path.cwd() / "Lib" / "importlib"
            return bootstrap_dir

    def _get_bootstrap_modules(self, py_exe_file: str):
        bootstrap_dir = self._get_bootstrap_root_path(py_exe_file)
        bootstrap_path = bootstrap_dir / "_bootstrap.py"
        bootstrap_external_path = bootstrap_dir / "_bootstrap_external.py"
        if bootstrap_path.is_file():
            with open(bootstrap_path, 'r') as f:
                bootstrap_code = _serializer.compile_code(
                    "<frozen importlib._bootstrap>", f.read())
                bootstrap = self._serialize(bootstrap_code)
        if bootstrap_external_path.is_file():
            with open(bootstrap_external_path, 'r') as f:
                bootstrap_external_code = _serializer.compile_code(
                    "<frozen importlib._bootstrap_external>", f.read())
                bootstrap_external = self._serialize(
                    bootstrap_external_code)
        return bootstrap, bootstrap_external

    def freeze_startup_modules(self, py_exe_file: str,
                               output_filename: Optional[str]=None,
                               include_bootstrap: bool=True,
                               extra_modules: Optional[List[str]]=None) -> str:
        """
        Freeze modules required at startup
        """
        template = PYTHON_FROZEN_MODULES_TEMPLATE.lstrip()
        d = {}
        modules = {}
        startup_modules = self._get_startup_modules(py_exe_file,
                                                    extra_modules)
        for mod_name, py_file, (origin, cached) in startup_modules:
            modules[mod_name] = py_file
            pyc_file = self.get_pyc_file(py_file)
            with open(pyc_file, "rb") as f:
                f.read(12)
                code = marshal.load(f)
                try:
                    d[mod_name] = (py_file, self._serialize(code),
                                   origin, cached)
                except TypeError as e:
                    print(py_file)
                    raise e

        bootstrap = bootstrap_external = None
        if include_bootstrap:
            bootstrap, bootstrap_external = self._get_bootstrap_modules(
                py_exe_file)

        self.finalize()
        init_lines = []
        padding_space = " " * 31
        for k, (file_name, code_ptr, _origin, _cached) in d.items():
            if '.' in k:
                parent_module = f"\"{k.rsplit('.', 1)[0]}\""
            else:
                parent_module = "NULL"
            origin = "NULL" if _origin is None else f"\"{_origin}\""
            cached = "NULL" if _cached is None else f"\"{_cached}\""
            if file_name.endswith(self._INIT_FILE_NAMES):
                needs_path = "true"
            else:
                needs_path = "false"
            init_lines.append(f"    _PyFrozenModule_AddModule("
                              f"\"{k}\",\n{padding_space}"
                              f"(PyObject*){code_ptr}, "
                              f"{needs_path}, {parent_module},"
                              f"\n{padding_space}{origin},"
                              f"\n{padding_space}{cached});")
        output = template.format(
            files=" ".join(
                sorted(os.path.split(x[1])[1]
                       for x in modules.items())),
            fwd_declarations="\n".join(self.forward_declarations),
            extra_refs="\n".join(self.extra_refs),
            init_lines="\n".join(init_lines))
        if bootstrap and bootstrap_external:
            output += PYTHON_BOOTSTRAP_FN_TEMPLATE.format(
                bootstrap=bootstrap,
                bootstrap_external=bootstrap_external)
        if output_filename is not None:
            with open(output_filename, 'w') as f:
                f.write(output)
        return output

    def freeze_py(self, py_file_name: str,
                  output_filename: Optional[str]=None) -> str:
        """
        Freeze a single python file
        """
        template = FROZEN_PY_TEMPLATE.lstrip()
        pyc_file_name = self.get_pyc_file(py_file_name)
        with open(pyc_file_name, "rb") as f:
            f.read(12)
            code = marshal.load(f)
            fwd_declarations, obj_ref = self.serialize(code)
            c_code = template.format(fwd_declarations=fwd_declarations,
                                     obj_ref=obj_ref)
        if output_filename is not None:
            with open(output_filename, "w") as f:
                f.write(c_code)
        return c_code


PYTHON_BOOTSTRAP_FN_TEMPLATE = """
int _PyFrozenModules_ImportBootstrap(void)
{{
    if (import_bootstrap_module("_frozen_importlib_external",
                                (PyObject*){bootstrap_external}) <= 0)
        return -1;
    return import_bootstrap_module("_frozen_importlib",
                                   (PyObject*){bootstrap});
}}
"""

PYTHON_FROZEN_MODULES_TEMPLATE = """
/* Autogenerated from the following files:
    {files}
*/
#include "Python.h"
#include <stdbool.h>

{fwd_declarations}

static _Bool frozen_modules_initialized = false;
static _Bool frozen_modules_disable = false;
static PyObject* frozen_code_objects = NULL;

/* Remove name from sys.modules, if it's there. */
static void
remove_module(PyObject *name)
{{
    PyObject *modules = PyImport_GetModuleDict();
    if (PyDict_GetItem(modules, name) == NULL)
        return;
    if (PyDict_DelItem(modules, name) < 0)
        Py_FatalError("import:  deleting existing key in"
                      "sys.modules failed");
}}

static PyObject *
module_dict_for_exec(PyObject *name)
{{
    PyObject *m, *d = NULL;

    m = PyImport_AddModuleObject(name);
    if (m == NULL)
        return NULL;
    /* If the module is being reloaded, we get the old module back
       and re-use its dict to exec the new code. */
    d = PyModule_GetDict(m);
    if (PyDict_GetItemString(d, "__builtins__") == NULL) {{
        if (PyDict_SetItemString(d, "__builtins__",
                                 PyEval_GetBuiltins()) != 0) {{
            remove_module(name);
            return NULL;
        }}
    }}

    return d;  /* Return a borrowed reference. */
}}

static PyObject *
exec_code_in_module(PyObject *name,
                    PyObject *module_dict, PyObject *code_object)
{{
    PyObject *modules = PyImport_GetModuleDict();
    PyObject *v, *m;

    v = PyEval_EvalCode(code_object, module_dict, module_dict);
    if (v == NULL) {{
        remove_module(name);
        return NULL;
    }}
    Py_DECREF(v);

    if ((m = PyDict_GetItem(modules, name)) == NULL) {{
        PyErr_Format(PyExc_ImportError,
                     "Loaded module %R not found in sys.modules",
                     name);
        return NULL;
    }}

    Py_INCREF(m);

    return m;
}}

static int import_bootstrap_module(char * name, PyObject* co) {{
    PyObject *m, *d, *name_obj = PyUnicode_FromString(name);
    if (!PyCode_Check(co)) {{
        PyErr_Format(PyExc_TypeError,
                     "frozen object %R is not a code object",
                     "_frozen_importlib");
        goto _import_botstrap_err_return;
    }}
    d = module_dict_for_exec(name_obj);
    if (d == NULL) {{
        goto _import_botstrap_err_return;
    }}
    m = exec_code_in_module(name_obj, d, co);
    if (m == NULL)
        goto _import_botstrap_err_return;
    Py_DECREF(name_obj);
    Py_DECREF(m);
    return 1;
_import_botstrap_err_return:
    Py_DECREF(name_obj);
    return -1;
}}


int _PyFrozenModule_AddModule(const char *name, PyObject *co,
                                _Bool needs_path, const char *parent_name,
                                const char *origin, const char *cached)
{{
    PyObject *t, *_needs_path, *_parent_name,
             *_origin, *_cached;
    int r;
    _needs_path = needs_path ? Py_True : Py_False;
    Py_INCREF(_needs_path);

    if(parent_name == NULL) {{
        _parent_name = Py_None;
        Py_INCREF(Py_None);
    }} else {{
        _parent_name = PyUnicode_FromString(parent_name);
    }}
    if(origin == NULL) {{
        _origin = Py_None;
        Py_INCREF(_origin);
    }} else {{
        _origin = PyUnicode_FromString(origin);
    }}
    if(cached == NULL) {{
        _cached = Py_None;
        Py_INCREF(_cached);
    }} else {{
        _cached = PyUnicode_FromString(cached);
    }}
    t = PyTuple_Pack(5, co, _needs_path, _parent_name, _origin, _cached);
    if (t == NULL) {{
        Py_DECREF(_needs_path);
        Py_DECREF(_parent_name);
        return -1;
    }}
    r = PyDict_SetItemString(frozen_code_objects, name, t);
    Py_DECREF(_needs_path);
    Py_DECREF(_parent_name);
    Py_DECREF(t);
    return r;
}}

void _PyFrozenModules_Init(void) {{
    if (frozen_modules_initialized)
        return;
    frozen_modules_initialized = true;

    intitialize_hashes(_ToHash);

{extra_refs}

    frozen_code_objects = PyDict_New();

{init_lines}
}}

void _PyFrozenModules_Finalize(void) {{
    if(frozen_code_objects) {{
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(frozen_code_objects, &pos, &key, &value)) {{
            PyCodeObject *co = (PyCodeObject*)PyTuple_GetItem(value, 0);
            if (co->co_zombieframe != NULL) {{
                PyObject_GC_Del(co->co_zombieframe);
                co->co_zombieframe = NULL;
            }}
            if (co->co_weakreflist != NULL) {{
                PyObject_ClearWeakRefs((PyObject*)co);
                co->co_weakreflist = NULL;
            }}
        }}
        PyDict_Clear(frozen_code_objects);
        Py_DECREF(frozen_code_objects);
        frozen_code_objects = NULL;
    }}
}}

void _PyFrozenModules_Disable(void) {{
    frozen_modules_disable = true;
}}

void _PyFrozenModules_Enable(void) {{
    frozen_modules_disable = false;
}}


PyObject* _PyFrozenModule_Lookup(PyObject* name) {{
    if(frozen_code_objects != NULL && !frozen_modules_disable) {{
        PyObject *co, *needs_path, *parent_name,
                 *origin, *cached, *path_str,
                 *module, *module_dict, *t =
            PyDict_GetItem(frozen_code_objects, name);
        if (t == NULL) {{
            return NULL;
        }}
        co = PyTuple_GetItem(t, 0);
        needs_path = PyTuple_GetItem(t, 1);
        parent_name = PyTuple_GetItem(t, 2);
        origin = PyTuple_GetItem(t, 3);
        cached = PyTuple_GetItem(t, 4);
        if (co != NULL) {{
            if (needs_path == Py_True) {{
                PyObject *path_sep = PyUnicode_FromString("/");
                Py_ssize_t sz = PyUnicode_Find(origin, path_sep,
                                               0, PyUnicode_GetLength(origin),
                                               -1);
                Py_DECREF(path_sep);
                if (sz == -2) {{
                    Py_DECREF(origin);
                    Py_DECREF(cached);
                    return NULL;
                }}
                else if (sz == -1) {{
                    path_str = origin;
                    Py_INCREF(path_str);
                }}
                else {{
                    path_str = PyUnicode_Substring(origin, 0, sz);
                }}
                module = PyImport_AddModuleObject(name);
                module_dict = PyModule_GetDict(module);
                PyObject *l = PyList_New(1);
                if (l == NULL) {{
                    Py_DECREF(origin);
                    Py_DECREF(cached);
                    Py_DECREF(path_str);
                    return NULL;
                }}
                PyList_SetItem(l, 0, path_str);
                int err = PyDict_SetItemString(module_dict, "__path__", l);
                Py_DECREF(l);
                if (err != 0) {{
                    Py_DECREF(origin);
                    Py_DECREF(cached);
                    Py_DECREF(path_str);
                    return NULL;
                }}
                Py_DECREF(path_str);
            }}
            module = PyImport_ExecCodeModuleObject(
                name, co, origin, cached);
            Py_DECREF(origin);
            Py_DECREF(cached);
            return module;
        }}
    }}
    return NULL;
}}
"""


FROZEN_PY_TEMPLATE = """
#include <Python.h>

{fwd_declarations}

int main(int argc, char* argv[]) {{
    int i;
    wchar_t **argv_copy;
    PyObject *rez, *obj = (PyObject*){obj_ref};
    argv_copy = (wchar_t **)PyMem_RawMalloc(sizeof(wchar_t*) * (argc+1));
    if (!argv_copy) {{
        fprintf(stderr, "out of memory\\n");
        return 1;
    }}
    for (i = 0; i < argc; i++) {{
        argv_copy[i] = Py_DecodeLocale(argv[i], NULL);
        if (!argv_copy[i]) {{
            fprintf(stderr, "Fatal Python error: "
                "unable to decode the command line argument #%i\\n",
                            i + 1);
            return 1;
        }}
    }}
    argv_copy[argc] = NULL;
    Py_Initialize();
    intitialize_hashes(_ToHash);
    PySys_SetArgv(argc, argv_copy);
    rez = PyImport_ExecCodeModule("__main__", obj);
    if(rez)
        Py_DECREF(rez);
    else
        PyErr_Print();
    return Py_FinalizeEx();
}}
"""
