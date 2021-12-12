import builtins
import locale
import os
import sys
import threading
from test import support
from test.support import os_helper
from test.libregrtest.utils import print_warning


class SkipTestEnvironment(Exception):
    pass


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
    the saved items was changed by the test. The support.environment_altered
    attribute is set to True if a change is detected.

    If verbose is more than 1, the before and after state of changed
    items is also printed.
    """

    def __init__(self, testname, verbose=0, quiet=False, *, pgo=False):
        self.testname = testname
        self.verbose = verbose
        self.quiet = quiet
        self.pgo = pgo

    # To add things to save and restore, add a name XXX to the resources list
    # and add corresponding get_XXX/restore_XXX functions.  get_XXX should
    # return the value to be saved and compared against a second call to the
    # get function when test execution completes.  restore_XXX should accept
    # the saved value and restore the resource using it.  It will be called if
    # and only if a change in the value is detected.
    #
    # Note: XXX will have any '.' replaced with '_' characters when determining
    # the corresponding method names.

    resources = ('sys.argv', 'cwd', 'sys.stdin', 'sys.stdout', 'sys.stderr',
                 'os.environ', 'sys.path', 'sys.path_hooks', '__import__',
                 'warnings.filters', 'asyncore.socket_map',
                 'logging._handlers', 'logging._handlerList', 'sys.gettrace',
                 'sys.warnoptions',
                 # multiprocessing.process._cleanup() may release ref
                 # to a thread, so check processes first.
                 'multiprocessing.process._dangling', 'threading._dangling',
                 'sysconfig._CONFIG_VARS', 'sysconfig._INSTALL_SCHEMES',
                 'files', 'locale', 'warnings.showwarning',
                 'shutil_archive_formats', 'shutil_unpack_formats',
                 'asyncio.events._event_loop_policy',
                 'urllib.requests._url_tempfiles', 'urllib.requests._opener',
                )

    def get_module(self, name):
        # function for restore() methods
        return sys.modules[name]

    def try_get_module(self, name):
        # function for get() methods
        try:
            return self.get_module(name)
        except KeyError:
            raise SkipTestEnvironment

    def get_urllib_requests__url_tempfiles(self):
        urllib_request = self.try_get_module('urllib.request')
        return list(urllib_request._url_tempfiles)
    def restore_urllib_requests__url_tempfiles(self, tempfiles):
        for filename in tempfiles:
            os_helper.unlink(filename)

    def get_urllib_requests__opener(self):
        urllib_request = self.try_get_module('urllib.request')
        return urllib_request._opener
    def restore_urllib_requests__opener(self, opener):
        urllib_request = self.get_module('urllib.request')
        urllib_request._opener = opener

    def get_asyncio_events__event_loop_policy(self):
        self.try_get_module('asyncio')
        return support.maybe_get_event_loop_policy()
    def restore_asyncio_events__event_loop_policy(self, policy):
        asyncio = self.get_module('asyncio')
        asyncio.set_event_loop_policy(policy)

    def get_sys_argv(self):
        return id(sys.argv), sys.argv, sys.argv[:]
    def restore_sys_argv(self, saved_argv):
        sys.argv = saved_argv[1]
        sys.argv[:] = saved_argv[2]

    def get_cwd(self):
        return os.getcwd()
    def restore_cwd(self, saved_cwd):
        os.chdir(saved_cwd)

    def get_sys_stdout(self):
        return sys.stdout
    def restore_sys_stdout(self, saved_stdout):
        sys.stdout = saved_stdout

    def get_sys_stderr(self):
        return sys.stderr
    def restore_sys_stderr(self, saved_stderr):
        sys.stderr = saved_stderr

    def get_sys_stdin(self):
        return sys.stdin
    def restore_sys_stdin(self, saved_stdin):
        sys.stdin = saved_stdin

    def get_os_environ(self):
        return id(os.environ), os.environ, dict(os.environ)
    def restore_os_environ(self, saved_environ):
        os.environ = saved_environ[1]
        os.environ.clear()
        os.environ.update(saved_environ[2])

    def get_sys_path(self):
        return id(sys.path), sys.path, sys.path[:]
    def restore_sys_path(self, saved_path):
        sys.path = saved_path[1]
        sys.path[:] = saved_path[2]

    def get_sys_path_hooks(self):
        return id(sys.path_hooks), sys.path_hooks, sys.path_hooks[:]
    def restore_sys_path_hooks(self, saved_hooks):
        sys.path_hooks = saved_hooks[1]
        sys.path_hooks[:] = saved_hooks[2]

    def get_sys_gettrace(self):
        return sys.gettrace()
    def restore_sys_gettrace(self, trace_fxn):
        sys.settrace(trace_fxn)

    def get___import__(self):
        return builtins.__import__
    def restore___import__(self, import_):
        builtins.__import__ = import_

    def get_warnings_filters(self):
        warnings = self.try_get_module('warnings')
        return id(warnings.filters), warnings.filters, warnings.filters[:]
    def restore_warnings_filters(self, saved_filters):
        warnings = self.get_module('warnings')
        warnings.filters = saved_filters[1]
        warnings.filters[:] = saved_filters[2]

    def get_asyncore_socket_map(self):
        asyncore = sys.modules.get('asyncore')
        # XXX Making a copy keeps objects alive until __exit__ gets called.
        return asyncore and asyncore.socket_map.copy() or {}
    def restore_asyncore_socket_map(self, saved_map):
        asyncore = sys.modules.get('asyncore')
        if asyncore is not None:
            asyncore.close_all(ignore_all=True)
            asyncore.socket_map.update(saved_map)

    def get_shutil_archive_formats(self):
        shutil = self.try_get_module('shutil')
        # we could call get_archives_formats() but that only returns the
        # registry keys; we want to check the values too (the functions that
        # are registered)
        return shutil._ARCHIVE_FORMATS, shutil._ARCHIVE_FORMATS.copy()
    def restore_shutil_archive_formats(self, saved):
        shutil = self.get_module('shutil')
        shutil._ARCHIVE_FORMATS = saved[0]
        shutil._ARCHIVE_FORMATS.clear()
        shutil._ARCHIVE_FORMATS.update(saved[1])

    def get_shutil_unpack_formats(self):
        shutil = self.try_get_module('shutil')
        return shutil._UNPACK_FORMATS, shutil._UNPACK_FORMATS.copy()
    def restore_shutil_unpack_formats(self, saved):
        shutil = self.get_module('shutil')
        shutil._UNPACK_FORMATS = saved[0]
        shutil._UNPACK_FORMATS.clear()
        shutil._UNPACK_FORMATS.update(saved[1])

    def get_logging__handlers(self):
        logging = self.try_get_module('logging')
        # _handlers is a WeakValueDictionary
        return id(logging._handlers), logging._handlers, logging._handlers.copy()
    def restore_logging__handlers(self, saved_handlers):
        # Can't easily revert the logging state
        pass

    def get_logging__handlerList(self):
        logging = self.try_get_module('logging')
        # _handlerList is a list of weakrefs to handlers
        return id(logging._handlerList), logging._handlerList, logging._handlerList[:]
    def restore_logging__handlerList(self, saved_handlerList):
        # Can't easily revert the logging state
        pass

    def get_sys_warnoptions(self):
        return id(sys.warnoptions), sys.warnoptions, sys.warnoptions[:]
    def restore_sys_warnoptions(self, saved_options):
        sys.warnoptions = saved_options[1]
        sys.warnoptions[:] = saved_options[2]

    # Controlling dangling references to Thread objects can make it easier
    # to track reference leaks.
    def get_threading__dangling(self):
        # This copies the weakrefs without making any strong reference
        return threading._dangling.copy()
    def restore_threading__dangling(self, saved):
        threading._dangling.clear()
        threading._dangling.update(saved)

    # Same for Process objects
    def get_multiprocessing_process__dangling(self):
        multiprocessing_process = self.try_get_module('multiprocessing.process')
        # Unjoined process objects can survive after process exits
        multiprocessing_process._cleanup()
        # This copies the weakrefs without making any strong reference
        return multiprocessing_process._dangling.copy()
    def restore_multiprocessing_process__dangling(self, saved):
        multiprocessing_process = self.get_module('multiprocessing.process')
        multiprocessing_process._dangling.clear()
        multiprocessing_process._dangling.update(saved)

    def get_sysconfig__CONFIG_VARS(self):
        # make sure the dict is initialized
        sysconfig = self.try_get_module('sysconfig')
        sysconfig.get_config_var('prefix')
        return (id(sysconfig._CONFIG_VARS), sysconfig._CONFIG_VARS,
                dict(sysconfig._CONFIG_VARS))
    def restore_sysconfig__CONFIG_VARS(self, saved):
        sysconfig = self.get_module('sysconfig')
        sysconfig._CONFIG_VARS = saved[1]
        sysconfig._CONFIG_VARS.clear()
        sysconfig._CONFIG_VARS.update(saved[2])

    def get_sysconfig__INSTALL_SCHEMES(self):
        sysconfig = self.try_get_module('sysconfig')
        return (id(sysconfig._INSTALL_SCHEMES), sysconfig._INSTALL_SCHEMES,
                sysconfig._INSTALL_SCHEMES.copy())
    def restore_sysconfig__INSTALL_SCHEMES(self, saved):
        sysconfig = self.get_module('sysconfig')
        sysconfig._INSTALL_SCHEMES = saved[1]
        sysconfig._INSTALL_SCHEMES.clear()
        sysconfig._INSTALL_SCHEMES.update(saved[2])

    def get_files(self):
        return sorted(fn + ('/' if os.path.isdir(fn) else '')
                      for fn in os.listdir())
    def restore_files(self, saved_value):
        fn = os_helper.TESTFN
        if fn not in saved_value and (fn + '/') not in saved_value:
            if os.path.isfile(fn):
                os_helper.unlink(fn)
            elif os.path.isdir(fn):
                os_helper.rmtree(fn)

    _lc = [getattr(locale, lc) for lc in dir(locale)
           if lc.startswith('LC_')]
    def get_locale(self):
        pairings = []
        for lc in self._lc:
            try:
                pairings.append((lc, locale.setlocale(lc, None)))
            except (TypeError, ValueError):
                continue
        return pairings
    def restore_locale(self, saved):
        for lc, setting in saved:
            locale.setlocale(lc, setting)

    def get_warnings_showwarning(self):
        warnings = self.try_get_module('warnings')
        return warnings.showwarning
    def restore_warnings_showwarning(self, fxn):
        warnings = self.get_module('warnings')
        warnings.showwarning = fxn

    def resource_info(self):
        for name in self.resources:
            method_suffix = name.replace('.', '_')
            get_name = 'get_' + method_suffix
            restore_name = 'restore_' + method_suffix
            yield name, getattr(self, get_name), getattr(self, restore_name)

    def __enter__(self):
        self.saved_values = []
        for name, get, restore in self.resource_info():
            try:
                original = get()
            except SkipTestEnvironment:
                continue

            self.saved_values.append((name, get, restore, original))
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        saved_values = self.saved_values
        self.saved_values = None

        # Some resources use weak references
        support.gc_collect()

        for name, get, restore, original in saved_values:
            current = get()
            # Check for changes to the resource's value
            if current != original:
                support.environment_altered = True
                restore(original)
                if not self.quiet and not self.pgo:
                    print_warning(
                        f"{name} was modified by {self.testname}\n"
                        f"  Before: {original}\n"
                        f"  After:  {current} ")
        return False
