import os
import errno
import py_compile
import shutil
import unittest
import tempfile
import sys
from importlib.machinery import SOURCE_SUFFIXES, BYTECODE_SUFFIXES, ModuleSpec

from test import support

import modulefinder

TEST_DIR = tempfile.mkdtemp()
TEST_PATH = [TEST_DIR, os.path.dirname(tempfile.__file__)]

# Test descriptions will be a list of 5 items:
#
# 1. a module name that will be imported by modulefinder
# 2. a list of module names that modulefinder is required to find
# 3. a list of module names that modulefinder should complain
#    about because they are not found
# 4. a list of module names that modulefinder should complain
#    about because they MAY be not found
# 5. a string specifying packages to create; the format is obvious imo.
#
# Each package will be created in TEST_DIR, and TEST_DIR will be
# removed after the tests again.
# Modulefinder searches in a path that contains TEST_DIR, plus
# the standard Lib directory.

def open_file(path):
    dirname = os.path.dirname(path)
    try:
        os.makedirs(dirname)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise
    return open(path, "w")


def create_package(source):
    ofi = None
    try:
        for line in source.splitlines():
            if line.startswith(" ") or line.startswith("\t"):
                ofi.write(line.strip() + "\n")
            else:
                if ofi:
                    ofi.close()
                ofi = open_file(os.path.join(TEST_DIR, line.strip()))
    finally:
        if ofi:
            ofi.close()


def _do_test(test_case, info, report=False, debug=0, replace_paths=[]):
    import_this, modules, missing, maybe_missing, source = info
    if source is not None:
        create_package(source)
    try:
        mf = modulefinder.ModuleFinder(path=TEST_PATH, debug=debug,
                                       replace_paths=replace_paths)
        mf.import_hook(import_this)
        if report:
            mf.report()
##          # This wouldn't work in general when executed several times:
##          opath = sys.path[:]
##          sys.path = TEST_PATH
##          try:
##              __import__(import_this)
##          except:
##              import traceback; traceback.print_exc()
##          sys.path = opath
##          return
        modules = sorted(set(modules))
        found = sorted(mf.modules)
        # check if we found what we expected, not more, not less
        test_case.assertEqual(found, modules)

        # check for missing and maybe missing modules
        bad, maybe = mf.any_missing_maybe()
        test_case.assertEqual(bad, missing)
        test_case.assertEqual(maybe, maybe_missing)
    finally:
        if source is not None:
            shutil.rmtree(TEST_DIR)



maybe_test = [
    "a.module",
    ["a", "a.module", "sys",
     "b"],
    ["c"], ["b.something"],
    """\
a/__init__.py
a/module.py
                                from b import something
                                from c import something
b/__init__.py
                                from sys import *
"""]

maybe_test_new = [
    "a.module",
    ["a", "a.module", "sys",
     "b", "__future__"],
    ["c"], ["b.something"],
    """\
a/__init__.py
a/module.py
                                from b import something
                                from c import something
b/__init__.py
                                from __future__ import absolute_import
                                from sys import *
"""]

package_test = [
    "a.module",
    ["a", "a.b", "a.c", "a.module", "mymodule", "sys"],
    ["blahblah", "c"], [],
    """\
mymodule.py
a/__init__.py
                                import blahblah
                                from a import b
                                import c
a/module.py
                                import sys
                                from a import b as x
                                from a.c import sillyname
a/b.py
a/c.py
                                from a.module import x
                                import mymodule as sillyname
                                from sys import version_info
"""]

absolute_import_test = [
    "a.module",
    ["a", "a.module",
     "b", "b.x", "b.y", "b.z",
     "__future__", "sys", "gc"],
    ["blahblah", "z"], [],
    """\
mymodule.py
a/__init__.py
a/module.py
                                from __future__ import absolute_import
                                import sys # sys
                                import blahblah # fails
                                import gc # gc
                                import b.x # b.x
                                from b import y # b.y
                                from b.z import * # b.z.*
a/gc.py
a/sys.py
                                import mymodule
a/b/__init__.py
a/b/x.py
a/b/y.py
a/b/z.py
b/__init__.py
                                import z
b/unused.py
b/x.py
b/y.py
b/z.py
"""]

relative_import_test = [
    "a.module",
    ["__future__",
     "a", "a.module",
     "a.b", "a.b.y", "a.b.z",
     "a.b.c", "a.b.c.moduleC",
     "a.b.c.d", "a.b.c.e",
     "a.b.x",
     "gc"],
    [], [],
    """\
mymodule.py
a/__init__.py
                                from .b import y, z # a.b.y, a.b.z
a/module.py
                                from __future__ import absolute_import # __future__
                                import gc # gc
a/gc.py
a/sys.py
a/b/__init__.py
                                from ..b import x # a.b.x
                                #from a.b.c import moduleC
                                from .c import moduleC # a.b.moduleC
a/b/x.py
a/b/y.py
a/b/z.py
a/b/g.py
a/b/c/__init__.py
                                from ..c import e # a.b.c.e
a/b/c/moduleC.py
                                from ..c import d # a.b.c.d
a/b/c/d.py
a/b/c/e.py
a/b/c/x.py
"""]

relative_import_test_2 = [
    "a.module",
    ["a", "a.module",
     "a.sys",
     "a.b", "a.b.y", "a.b.z",
     "a.b.c", "a.b.c.d",
     "a.b.c.e",
     "a.b.c.moduleC",
     "a.b.c.f",
     "a.b.x",
     "a.another"],
    [], [],
    """\
mymodule.py
a/__init__.py
                                from . import sys # a.sys
a/another.py
a/module.py
                                from .b import y, z # a.b.y, a.b.z
a/gc.py
a/sys.py
a/b/__init__.py
                                from .c import moduleC # a.b.c.moduleC
                                from .c import d # a.b.c.d
a/b/x.py
a/b/y.py
a/b/z.py
a/b/c/__init__.py
                                from . import e # a.b.c.e
a/b/c/moduleC.py
                                #
                                from . import f   # a.b.c.f
                                from .. import x  # a.b.x
                                from ... import another # a.another
a/b/c/d.py
a/b/c/e.py
a/b/c/f.py
"""]

relative_import_test_3 = [
    "a.module",
    ["a", "a.module"],
    ["a.bar"],
    [],
    """\
a/__init__.py
                                def foo(): pass
a/module.py
                                from . import foo
                                from . import bar
"""]

relative_import_test_4 = [
    "a.module",
    ["a", "a.module"],
    [],
    [],
    """\
a/__init__.py
                                def foo(): pass
a/module.py
                                from . import *
"""]

bytecode_test = [
    "a",
    ["a"],
    [],
    [],
    ""
]

syntax_error_test = [
    "a.module",
    ["a", "a.module", "b"],
    ["b.module"], [],
    """\
a/__init__.py
a/module.py
                                import b.module
b/__init__.py
b/module.py
                                ?  # SyntaxError: invalid syntax
"""]


same_name_as_bad_test = [
    "a.module",
    ["a", "a.module", "b", "b.c"],
    ["c"], [],
    """\
a/__init__.py
a/module.py
                                import c
                                from b import c
b/__init__.py
b/c.py
"""]


second_dir = os.path.join(TEST_DIR, "second_dir")
third_dir = os.path.join(TEST_DIR, "third_dir")
modulefinder.AddPackagePath("extended_package", second_dir)
modulefinder.AddPackagePath("extended_package", third_dir)

namespace_package_test = [
    "a",
    ["a", "extended_package", "extended_package.b",
     "extended_package.b.c", "extended_package.b.e",
     "extended_package.b.f", "extended_package.d"],
    ["extended_package.b.arglebargle", "extended_package.b.b"], [],
    """\
a.py
                                import extended_package.b.c
                                import extended_package.b.e
extended_package/__init__.py
extended_package/b/c.py
                                import extended_package.b.arglebargle
extended_package/d.py
second_dir/b/e.py
                                from . import f
                                from .. import d
third_dir/b/f.py
                                import extended_package.b.b
"""]

namespace_package_test_2 = [
    "a",
    ["a", "extended_package", "extended_package.b",
     "extended_package.b.c"],
    ["extended_package.b.a"], [],
    """\
a.py
                                # Should load from second_dir, since it's
                                # earlier on the path
                                import extended_package.b.c
extended_package/__init__.py
second_dir/b/c.py
                                import extended_package.b.a
third_dir/b/c.py
"""]


class BasicTests(unittest.TestCase):

    def test_package(self):
        _do_test(self, package_test)

    def test_maybe(self):
        _do_test(self, maybe_test)

    def test_maybe_new(self):
        _do_test(self, maybe_test_new)

    def test_absolute_imports(self):
        _do_test(self, absolute_import_test)

    def test_relative_imports(self):
        _do_test(self, relative_import_test)

    def test_relative_imports_2(self):
        _do_test(self, relative_import_test_2)

    def test_relative_imports_3(self):
        _do_test(self, relative_import_test_3)

    def test_relative_imports_4(self):
        _do_test(self, relative_import_test_4)

    def test_syntax_error(self):
        _do_test(self, syntax_error_test)

    def test_same_name_as_bad(self):
        _do_test(self, same_name_as_bad_test)

    def test_bytecode(self):
        base_path = os.path.join(TEST_DIR, 'a')
        source_path = base_path + SOURCE_SUFFIXES[0]
        bytecode_path = base_path + BYTECODE_SUFFIXES[0]
        with open_file(source_path) as file:
            file.write('testing_modulefinder = True\n')
        py_compile.compile(source_path, cfile=bytecode_path)
        os.remove(source_path)
        _do_test(self, bytecode_test)

    def test_replace_paths(self):
        old_path = os.path.join(TEST_DIR, 'a', 'module.py')
        new_path = os.path.join(TEST_DIR, 'a', 'spam.py')
        with support.captured_stdout() as output:
            _do_test(self, maybe_test, debug=2,
                     replace_paths=[(old_path, new_path)])
        output = output.getvalue()
        expected = "co_filename %r changed to %r" % (old_path, new_path)
        self.assertIn(expected, output)

    def test_extended_opargs(self):
        extended_opargs_test = [
            "a",
            ["a", "b"],
            [], [],
            """\
a.py
                                %r
                                import b
b.py
""" % list(range(2**16))]  # 2**16 constants
        _do_test(self, extended_opargs_test)

    def test_namespace_package(self):
        _do_test(self, namespace_package_test)

    def test_namespace_package_2(self):
        _do_test(self, namespace_package_test_2)


class BasicLoader:
    def create_module(self, spec):
        return

    def exec_module(self, module):
        raise RuntimeError("This method should not be run.")
    
    def get_code(self, name):
        return compile(modules[name], "<string>", "exec")


modules = {"a": "import b\nimport b.c",
           "b": "from . import d",
           "b.c": "from . import e",
           "b.d": "import f"}


class BasicFinder:
    def __init__(self, loader_class):
        self.loader_class = loader_class

    def find_spec(self, name, path, target=None):
        if name not in modules:
            return None
        return ModuleSpec(name, self.loader_class(), is_package=(name == "b"))
        

basic_test = ["a", ["a", "b", "b.c", "b.d"], ["b.e", "f"], [], None]


class BaseImportHookCase(unittest.TestCase):
    def setUp(self):
        self.real_finders = sys.meta_path
    
    def tearDown(self):
        sys.meta_path = self.real_finders

class HookTests(BaseImportHookCase):
    def test_normal_hooks(self):
        # This produces virtually the same results as
        # any of the default SourceLoaders served by
        # PathFinder using FileFinder.

        sys.meta_path = [BasicFinder(BasicLoader)]
        _do_test(self, basic_test)

    def test_deprecated_hooks(self):
        # As above, but with the deprecated APIs instead.

        class Loader:
            def load_module(self, name):
                raise RuntimeError("This method cannot be run either, "
                                   "since it will execute the module.")

            def is_package(self, name):
                # We can't stick this information in the spec,
                # since we aren't creating a spec.
                # modulefinder should accept information
                # in this form, but it is more likely that older loaders
                # will not provide this function.
                # Nothing we can do about that.
                return name == "b"
            
            def get_code(self, name):
                return compile(modules[name], "<string>", "exec")

        class Finder:
            def find_module(self, name, path=None):
                if name not in modules:
                    return None
                return Loader()

        sys.meta_path = [Finder()]
        _do_test(self, basic_test)

    def test_create_module(self):
        class Module:
            pass
        
        class Loader(BasicLoader):
            def create_module(self, spec):
                self.module = Module()
                return self.module

            def get_code(self, name):
                assert sys.modules[name] is self.module
                return super().get_code(name)

        sys.meta_path = [BasicFinder(Loader)]
        _do_test(self, basic_test)


class NoneFinder:
    def __init__(self, attr):
        self.attr = attr

    def find_spec(self, name, path, target=None):
        spec = ModuleSpec(name, BasicLoader(), is_package=(name == "b"))
        if name == "b":
            setattr(spec, self.attr, None)
        return spec


class NoneTests(BaseImportHookCase):
    def test_loader_None(self):        
        sys.meta_path = [NoneFinder("loader")]
        _do_test(self, ["a", ["a", "b", "b.c"], ["b.e"], [], None])

    def test_name_None(self):
        sys.meta_path = [NoneFinder("name")]
        _do_test(self, ["a", ["a"], ["b", "b.c"], [], None])

    # Other attributes are usually None anyway


if __name__ == "__main__":
    unittest.main()
