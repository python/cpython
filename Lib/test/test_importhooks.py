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

reload_src = test_src+"""\
reloaded = True
"""

test_co = compile(test_src, "<???>", "exec")
reload_co = compile(reload_src, "<???>", "exec")

test_path = "!!!_test_!!!"


class ImportTracker:
    """Importer that only tracks attempted imports."""
    def __init__(self):
        self.imports = []
    def find_module(self, fullname, path=None):
        self.imports.append(fullname)
        return None


class TestImporter:

    modules = {
        "hooktestmodule": (False, test_co),
        "hooktestpackage": (True, test_co),
        "hooktestpackage.sub": (True, test_co),
        "hooktestpackage.sub.subber": (False, test_co),
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
        self.tracker = ImportTracker()
        sys.meta_path.insert(0, self.tracker)

    def tearDown(self):
        sys.path[:] = self.path
        sys.meta_path[:] = self.meta_path
        sys.path_hooks[:] = self.path_hooks
        sys.path_importer_cache.clear()
        for fullname in self.tracker.imports:
            if fullname in sys.modules:
                del sys.modules[fullname]


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
        self.failIf(hasattr(reloadmodule,'reloaded'))

        TestImporter.modules['reloadmodule'] = (False, reload_co)
        reload(reloadmodule)
        self.failUnless(hasattr(reloadmodule,'reloaded'))

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
        if mname in sys.modules:
            del sys.modules[mname]
        sys.meta_path.append(ImportBlocker(mname))
        try:
            __import__(mname)
        except ImportError:
            pass
        else:
            self.fail("'%s' was not supposed to be importable" % mname)

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
        for mname in mnames:
            m = __import__(mname, globals(), locals(), ["__dummy__"])
            m.__loader__  # to make sure we actually handled the import

def test_main():
    test_support.run_unittest(ImportHooksTestCase)

if __name__ == "__main__":
    test_main()
