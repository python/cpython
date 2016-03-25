import builtins
import locale
import logging
import os
import shutil
import sys
import sysconfig
import warnings
from test import support
try:
    import threading
except ImportError:
    threading = None
try:
    import _multiprocessing, multiprocessing.process
except ImportError:
    multiprocessing = None


class Resource:
    name = None

    def __init__(self):
        self.original = self.get()
        self.current = None

    def decode_value(self, value):
        return value

    def display_diff(self):
        original = self.decode_value(self.original)
        current = self.decode_value(self.current)
        print(f"  Before: {original}", file=sys.stderr)
        print(f"  After:  {current}", file=sys.stderr)


class ModuleAttr(Resource):
    def encode_value(self, value):
        return value

    def get_module(self):
        module_name, attr = self.name.split('.', 1)
        module = globals()[module_name]
        return (module, attr)

    def get(self):
        module, attr = self.get_module()
        value = getattr(module, attr)
        return self.encode_value(value)

    def restore_attr(self, module, attr):
        setattr(module, attr, self.original)

    def restore(self):
        module, attr = self.get_module()
        self.restore_attr(module, attr)


class ModuleAttrList(ModuleAttr):
    def decode_value(self, value):
        return value[2]

    def encode_value(self, value):
        return id(value), value, value.copy()

    def restore_attr(self, module, attr):
        value = self.original[1]
        value[:] = self.original[2]
        setattr(module, attr, value)


class ModuleAttrDict(ModuleAttr):
    _MARKER = object()

    def encode_value(self, value):
        return id(value), value, value.copy()

    def decode_value(self, value):
        return value[2]

    def restore_attr(self, module, attr):
        value = self.original[1]
        value.clear()
        value.update(self.original[2])
        setattr(module, attr, value)

    @classmethod
    def _get_key(cls, data, key):
        value = data.get(key, cls._MARKER)
        if value is not cls._MARKER:
            return repr(value)
        else:
            return '<not set>'

    def display_diff(self):
        old_dict = self.original[2]
        new_dict = self.current[2]

        keys = sorted(dict(old_dict).keys() | dict(new_dict).keys())
        for key in keys:
            old_value = self._get_key(old_dict, key)
            new_value = self._get_key(new_dict, key)
            if old_value == new_value:
                continue

            print(f"  {self.name}[{key!r}]: {old_value} => {new_value}",
                  file=sys.stderr)


class SysArgv(ModuleAttrList):
    name = 'sys.argv'

class SysStdin(ModuleAttr):
    name = 'sys.stdin'

class SysStdout(ModuleAttr):
    name = 'sys.stdout'

class SysStderr(ModuleAttr):
    name = 'sys.stderr'

class SysPath(ModuleAttrList):
    name = 'sys.path'

class SysPathHooks(ModuleAttrList):
    name = 'sys.path_hooks'

class SysWarnOptions(ModuleAttrList):
    name = 'sys.warnoptions'

class SysGettrace(Resource):
    name = 'sys.gettrace'

    def get(self):
        return sys.gettrace()

    def restore(self):
        sys.settrace(self.original)


class OsEnviron(ModuleAttrDict):
    name = 'os.environ'

class ImportFunc(ModuleAttr):
    name = 'builtins.__import__'

class WarningsFilters(ModuleAttrList):
    name = 'warnings.filters'

class WarningsShowWarning(ModuleAttr):
    name = 'warnings.showwarning'




class Cwd(Resource):
    name = 'cwd'

    def get(self):
        return os.getcwd()

    def restore(self):
        os.chdir(self.original)


class AsyncoreSocketMap(Resource):
    name = 'asyncore.socket_map'

    def get(self):
        asyncore = sys.modules.get('asyncore')
        # XXX Making a copy keeps objects alive until __exit__ gets called.
        return asyncore and asyncore.socket_map.copy() or {}

    def restore(self):
        asyncore = sys.modules.get('asyncore')
        if asyncore is not None:
            asyncore.close_all(ignore_all=True)
            asyncore.socket_map.update(self.original)


class ShutilUnpackFormats(ModuleAttrDict):
    name = 'shutil._UNPACK_FORMATS'

class ShutilArchiveFormats(ModuleAttrDict):
    # we could call get_archives_formats() but that only returns the
    # registry keys; we want to check the values too (the functions that
    # are registered)

    name = 'shutil._ARCHIVE_FORMATS'


class LoggingHandlers(ModuleAttrDict):
    # _handlers is a WeakValueDictionary
    name = 'logging._handlers'

    def restore(self):
        # Can't easily revert the logging state
        pass

class LoggingHandlerList(ModuleAttrList):
    # _handlerList is a list of weakrefs to handlers
    name = 'logging._handlerList'

    def restore(self):
        # Can't easily revert the logging state
        pass


class ThreadingDangling(Resource):
    # Controlling dangling references to Thread objects can make it easier
    # to track reference leaks.

    name = 'threading._dangling'

    def get(self):
        if not threading:
            return None
        # This copies the weakrefs without making any strong reference
        return threading._dangling.copy()

    def restore(self):
        if not threading:
            return
        threading._dangling.clear()
        threading._dangling.update(self.original)


if not multiprocessing:
    class MultiprocessingProcessDangling(Resource):
        # Same for Process objects

        name = 'multiprocessing.process._dangling'

        def get(self):
            # Unjoined process objects can survive after process exits
            multiprocessing.process._cleanup()
            # This copies the weakrefs without making any strong reference
            return multiprocessing.process._dangling.copy()

        def restore(self):
            multiprocessing.process._dangling.clear()
            multiprocessing.process._dangling.update(self.original)


class SysconfigInstallSchemes(ModuleAttrDict):
    name = 'sysconfig._INSTALL_SCHEMES'

class SysconfigConfigVars(ModuleAttrDict):
    name = 'sysconfig._CONFIG_VARS'

    def get(self):
        # make sure the dict is initialized
        sysconfig.get_config_var('prefix')
        return super().get()


class Files(Resource):
    name = 'files'

    def get(self):
        return sorted(fn + ('/' if os.path.isdir(fn) else '')
                      for fn in os.listdir())

    def restore(self):
        fn = support.TESTFN
        if fn not in self.original and (fn + '/') not in self.original:
            if os.path.isfile(fn):
                support.unlink(fn)
            elif os.path.isdir(fn):
                support.rmtree(fn)


class Locale(Resource):
    name = 'locale'

    _lc = [getattr(locale, lc) for lc in dir(locale)
           if lc.startswith('LC_')]

    def get(self):
        pairings = []
        for lc in self._lc:
            try:
                pairings.append((lc, locale.setlocale(lc, None)))
            except (TypeError, ValueError):
                continue
        return pairings

    def restore(self):
        for lc, setting in self.original:
            locale.setlocale(lc, setting)


def _get_resources(parent_cls, resources):
    for cls in parent_cls.__subclasses__():
        if cls.name is not None:
            resources.append(cls)
        _get_resources(cls, resources)

def get_resources(resources=None):
    resources = []
    _get_resources(Resource, resources)
    return resources



# Unit tests are supposed to leave the execution environment unchanged
# once they complete.  But sometimes tests have bugs, especially when
# tests fail, and the changes to environment go on to mess up other
# tests.  This can cause issues with buildbot stability, since tests
# are run in random order and so problems may appear to come and go.
# There are a few things we can save and restore to mitigate this, and
# the following context manager handles this task.

class saved_test_environment:
    """Save bits of the test environment and restore them at block exit.

        with saved_test_environment(testname, verbose, quiet):
            #stuff

    Unless quiet is True, a warning is printed to stderr if any of
    the saved items was changed by the test.  The attribute 'changed'
    is initially False, but is set to True if a change is detected.

    If verbose is more than 1, the before and after state of changed
    items is also printed.
    """

    changed = False

    def __init__(self, testname, verbose=0, quiet=False, *, pgo=False):
        self.testname = testname
        self.verbose = verbose
        self.quiet = quiet
        self.pgo = pgo
        self.resources = None

    def __enter__(self):
        self.resources = [resource_class()
                          for resource_class in get_resources()]
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # clear resources since they keep strong references to many objects
        resources = self.resources
        self.resources = None

        for resource in resources:
            resource.current = resource.get()

            # Check for changes to the resource's value
            if resource.current == resource.original:
                continue

            self.changed = True
            resource.restore()
            if self.quiet or self.pgo:
                continue

            print(f"Warning -- {resource.name} was modified by {self.testname}",
                  file=sys.stderr)
            resource.display_diff()
        return False
