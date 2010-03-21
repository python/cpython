import sys
import imp
import os
import unittest
from test import test_support


test_src = """\
def get_name():
    return __name__
def get_file():
    return __file__
"""

absimp = "import sub\n"
relimp = "from . import sub\n"
deeprelimp = "from .... import sub\n"
futimp = "from __future__ import absolute_import\n"

reload_src = test_src+"""\
reloaded = True
"""

test_co = compile(test_src, "<???>", "exec")
reload_co = compile(reload_src, "<???>", "exec")

test2_oldabs_co = compile(absimp + test_src, "<???>", "exec")
test2_newabs_co = compile(futimp + absimp + test_src, "<???>", "exec")
test2_newrel_co = compile(relimp + test_src, "<???>", "exec")
test2_deeprel_co = compile(deeprelimp + test_src, "<???>", "exec")
test2_futrel_co = compile(futimp + relimp + test_src, "<???>", "exec")

test_path = "!!!_test_!!!"


class TestImporter:

    modules = {
        "hooktestmodule": (False, test_co),
        "hooktestpackage": (True, test_co),
        "hooktestpackage.sub": (True, test_co),
        "hooktestpackage.sub.subber": (True, test_co),
        "hooktestpackage.oldabs": (False, test2_oldabs_co),
        "hooktestpackage.newabs": (False, test2_newabs_co),
        "hooktestpackage.newrel": (False, test2_newrel_co),
        "hooktestpackage.sub.subber.subest": (True, test2_deeprel_co),
        "hooktestpackage.futrel": (False, test2_futrel_co),
        "sub": (False, test_co),
        "reloadmodule": (False, test_co),
    }

    def __init__(self, path=test_path):
        if path != test_path:
            # if out class is on sys.path_hooks, we must raise
            # ImportError for any path item that we can't handle.
            raise ImportError
        self.path = path

    def _get__path__(self):
        raise NotImplementedError

    def find_module(self, fullname, path=None):
        if fullname in self.modules:
            return self
        else:
            return None

    def load_module(self, fullname):
        ispkg, code = self.modules[fullname]
        mod = sys.modules.setdefault(fullname,imp.new_module(fullname))
        mod.__file__ = "<%s>" % self.__class__.__name__
        mod.__loader__ = self
        if ispkg:
            mod.__path__ = self._get__path__()
        exec code in mod.__dict__
        return mod


class MetaImporter(TestImporter):
    def _get__path__(self):
        return []

class PathImporter(TestImporter):
    def _get__path__(self):
        return [self.path]


class ImportBlocker:
    """Place an ImportBlocker instance on sys.meta_path and you
    can be sure the modules you specified can't be imported, even
    if it's a builtin."""
    def __init__(self, *namestoblock):
        self.namestoblock = dict.fromkeys(namestoblock)
    def find_module(self, fullname, path=None):
        if fullname in self.namestoblock:
            return self
        return None
    def load_module(self, fullname):
        raise ImportError, "I dare you"


class ImpWrapper:

    def __init__(self, path=None):
        if path is not None and not os.path.isdir(path):
            raise ImportError
        self.path = path

    def find_module(self, fullname, path=None):
        subname = fullname.split(".")[-1]
        if subname != fullname and self.path is None:
            return None
        if self.path is None:
            path = None
        else:
            path = [self.path]
        try:
            file, filename, stuff = imp.find_module(subname, path)
        except ImportError:
            return None
        return ImpLoader(file, filename, stuff)


class ImpLoader:

    def __init__(self, file, filename, stuff):
        self.file = file
        self.filename = filename
        self.stuff = stuff

    def load_module(self, fullname):
        mod = imp.load_module(fullname, self.file, self.filename, self.stuff)
        if self.file:
            self.file.close()
        mod.__loader__ = self  # for introspection
        return mod


class ImportHooksBaseTestCase(unittest.TestCase):

    def setUp(self):
        self.path = sys.path[:]
        self.meta_path = sys.meta_path[:]
        self.path_hooks = sys.path_hooks[:]
        sys.path_importer_cache.clear()
        self.modules_before = sys.modules.copy()

    def tearDown(self):
        sys.path[:] = self.path
        sys.meta_path[:] = self.meta_path
        sys.path_hooks[:] = self.path_hooks
        sys.path_importer_cache.clear()
        sys.modules.clear()
        sys.modules.update(self.modules_before)


class ImportHooksTestCase(ImportHooksBaseTestCase):

    def doTestImports(self, importer=None):
        import hooktestmodule
        import hooktestpackage
        import hooktestpackage.sub
        import hooktestpackage.sub.subber
        self.assertEqual(hooktestmodule.get_name(),
                         "hooktestmodule")
        self.assertEqual(hooktestpackage.get_name(),
                         "hooktestpackage")
        self.assertEqual(hooktestpackage.sub.get_name(),
                         "hooktestpackage.sub")
        self.assertEqual(hooktestpackage.sub.subber.get_name(),
                         "hooktestpackage.sub.subber")
        if importer:
            self.assertEqual(hooktestmodule.__loader__, importer)
            self.assertEqual(hooktestpackage.__loader__, importer)
            self.assertEqual(hooktestpackage.sub.__loader__, importer)
            self.assertEqual(hooktestpackage.sub.subber.__loader__, importer)

        TestImporter.modules['reloadmodule'] = (False, test_co)
        import reloadmodule
        self.assertFalse(hasattr(reloadmodule,'reloaded'))

        TestImporter.modules['reloadmodule'] = (False, reload_co)
        imp.reload(reloadmodule)
        self.assertTrue(hasattr(reloadmodule,'reloaded'))

        import hooktestpackage.oldabs
        self.assertEqual(hooktestpackage.oldabs.get_name(),
                         "hooktestpackage.oldabs")
        self.assertEqual(hooktestpackage.oldabs.sub,
                         hooktestpackage.sub)

        import hooktestpackage.newrel
        self.assertEqual(hooktestpackage.newrel.get_name(),
                         "hooktestpackage.newrel")
        self.assertEqual(hooktestpackage.newrel.sub,
                         hooktestpackage.sub)

        import hooktestpackage.sub.subber.subest as subest
        self.assertEqual(subest.get_name(),
                         "hooktestpackage.sub.subber.subest")
        self.assertEqual(subest.sub,
                         hooktestpackage.sub)

        import hooktestpackage.futrel
        self.assertEqual(hooktestpackage.futrel.get_name(),
                         "hooktestpackage.futrel")
        self.assertEqual(hooktestpackage.futrel.sub,
                         hooktestpackage.sub)

        import sub
        self.assertEqual(sub.get_name(), "sub")

        import hooktestpackage.newabs
        self.assertEqual(hooktestpackage.newabs.get_name(),
                         "hooktestpackage.newabs")
        self.assertEqual(hooktestpackage.newabs.sub, sub)

    def testMetaPath(self):
        i = MetaImporter()
        sys.meta_path.append(i)
        self.doTestImports(i)

    def testPathHook(self):
        sys.path_hooks.append(PathImporter)
        sys.path.append(test_path)
        self.doTestImports()

    def testBlocker(self):
        mname = "exceptions"  # an arbitrary harmless builtin module
        test_support.unload(mname)
        sys.meta_path.append(ImportBlocker(mname))
        self.assertRaises(ImportError, __import__, mname)

    def testImpWrapper(self):
        i = ImpWrapper()
        sys.meta_path.append(i)
        sys.path_hooks.append(ImpWrapper)
        mnames = ("colorsys", "urlparse", "distutils.core", "compiler.misc")
        for mname in mnames:
            parent = mname.split(".")[0]
            for n in sys.modules.keys():
                if n.startswith(parent):
                    del sys.modules[n]
        with test_support.check_warnings(("The compiler package is deprecated "
                                          "and removed", DeprecationWarning)):
            for mname in mnames:
                m = __import__(mname, globals(), locals(), ["__dummy__"])
                m.__loader__  # to make sure we actually handled the import


def test_main():
    test_support.run_unittest(ImportHooksTestCase)

if __name__ == "__main__":
    test_main()
