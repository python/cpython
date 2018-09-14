#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Originally created by Jeethu Rao <jeethu@jeethurao.com>
# Ported to Python 3.8 by Neil Schemenauer

"""Convert Python code objects into static C structures.
"""
import sys
import argparse
import ast
import inspect
import marshal
import os
import pathlib
import py_compile
import subprocess
import importlib.util
import _serializer

from typing import Any, List, Optional, Tuple


MACROS = """
#define PyObject_HEAD_INIT_RC(rc, type)  \
    { _PyObject_EXTRA_INIT               \
      rc, type },

#define PyVarObject_HEAD_INIT_RC(rc, type, size)  \
    { PyObject_HEAD_INIT_RC(rc, type) size },

#define FROM_GC(g) ((PyObject *)(((PyGC_Head *)g)+1))
#define _GC_UNTRACKED (0)
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

    def _run_py_command(self, cmd: str) -> Any:
        proc = subprocess.run([sys.executable, "-c", cmd],
                              stdout=subprocess.PIPE)
        return ast.literal_eval(proc.stdout.decode("utf-8"))

    def _get_startup_modules(self,
                             extra_modules: Optional[List[str]]=None):
        cmd_print_modules = (
            'import sys\n'
            'print([m for m in sys.modules])\n'
            )
        startup_modules = self._run_py_command(cmd_print_modules)
        if extra_modules:
            startup_modules.extend(extra_modules)
        bad_modules = {
            #'encodings',
            #'collections',
            #'collections.abc',
            '_collections_abc',
            #'_frozen_importlib',
            #'_frozen_importlib_external',
        }
        modules = []
        for mod_name in startup_modules:
            if mod_name in bad_modules:
                continue
            try:
                spec = importlib.util.find_spec(mod_name)
            except ValueError:
                print('skipping', mod_name, 'no spec')
                continue
            if spec is None:
                print('skipping', mod_name, 'no spec')
                continue
            if spec.origin is None:
                print('skipping', mod_name, 'missing origin')
                continue
            co = spec.loader.get_code(spec.name)
            if co is None:
                print('skipping', mod_name, 'missing code')
                continue
            modules.append((mod_name, spec))
        for mod, spec in modules:
            co = spec.loader.get_code(spec.name)
            yield mod, co, spec.origin, spec.cached

    def _get_module_code(self, mod_name):
        spec = importlib.util.find_spec(mod_name)
        return spec.loader.get_code(spec.name)

    def freeze_startup_modules(self,
                               output_filename: Optional[str]=None,
                               include_bootstrap: bool=True,
                               extra_modules: Optional[List[str]]=None) -> str:
        """
        Freeze modules required at startup
        """
        template = PYTHON_FROZEN_MODULES_TEMPLATE.lstrip()
        d = {}
        modules = {}
        startup_modules = self._get_startup_modules(extra_modules)
        for mod_name, co, origin, cached in startup_modules:
            print('freezing startup module', mod_name)
            modules[mod_name] = origin
            try:
                d[mod_name] = (self._serialize(co), origin, cached)
            except TypeError as e:
                print(py_file)
                raise e

        self.finalize()
        init_lines = []
        padding_space = " " * 31
        for k, (code_ptr, _origin, _cached) in d.items():
            file_name = _origin
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
            print(k, file_name, needs_path)
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


PYTHON_FROZEN_MODULES_TEMPLATE = """
/* Autogenerated from the following files:
    {files}
*/

#include "Python.h"
#include <stdbool.h>
#include "pycore_gc.h" // PyGC_Head

{fwd_declarations}

static _Bool frozen_modules_initialized = false;

extern int _PyFrozenModule_AddModule(const char *name, PyObject *co,
                                _Bool needs_path, const char *parent_name,
                                const char *origin, const char *cached);

void _PyFrozenModules_Init(void) {{
    if (frozen_modules_initialized)
        return;
    frozen_modules_initialized = true;

    intitialize_hashes(_ToHash);

{extra_refs}

{init_lines}
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

def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(help='Functions')

    parser_1 = subparsers.add_parser('freeze-py',
                                     help="Freeze a single python script")
    parser_1.add_argument('py_file', type=str,
                          help='The python file to freeze')
    parser_1.add_argument('-o', '--out-file', type=str,
                          default='out.c', nargs='?',
                          help="The output filename (defaults to 'out.c')")
    parser_1.set_defaults(freeze_py=True)

    parser_2 = subparsers.add_parser('freeze-startup-modules',
                                     help="Freeze startup modules "
                                          "from a python interpreter")
    parser_2.set_defaults(freeze_startup_modules=True)
    parser_2.add_argument('-o', '--out-file', type=str,
                          default='out.c', nargs='?',
                          help="The output filename (defaults to 'out.c')")
    parser_2.add_argument('-x', '--extra-module', action='append',
                           help="Extra modules to include")

    args = parser.parse_args()
    serializer = Serializer()
    if getattr(args, 'freeze_py', False):
        serializer.freeze_py(args.py_file, output_filename=args.out_file)
    elif getattr(args, 'freeze_startup_modules', False):
        serializer.freeze_startup_modules(args.out_file,
                                          extra_modules=args.extra_module)

if __name__ == '__main__':
    main()
