"""runpy.py - locating and running Python code using the module namespace

Provides support for locating and running Python scripts using the Python
module namespace instead of the native filesystem.

This allows Python code to play nicely with non-filesystem based PEP 302
importers when locating support scripts as well as when importing modules.
"""
# Written by Nick Coghlan <ncoghlan at gmail.com>
#    to implement PEP 338 (Executing Modules as Scripts)

import sys
import imp

__all__ = [
    "run_module",
]

try:
    _get_loader = imp.get_loader
except AttributeError:
    # get_loader() is not provided by the imp module, so emulate it
    # as best we can using the PEP 302 import machinery exposed since
    # Python 2.3. The emulation isn't perfect, but the differences
    # in the way names are shadowed shouldn't matter in practice.
    import os.path
    import marshal                           # Handle compiled Python files

    # This helper is needed in order for the PEP 302 emulation to
    # correctly handle compiled files
    def _read_compiled_file(compiled_file):
        magic = compiled_file.read(4)
        if magic != imp.get_magic():
            return None
        try:
            compiled_file.read(4) # Skip timestamp
            return marshal.load(compiled_file)
        except Exception:
            return None

    class _AbsoluteImporter(object):
        """PEP 302 importer wrapper for top level import machinery"""
        def find_module(self, mod_name, path=None):
            if path is not None:
                return None
            try:
                file, filename, mod_info = imp.find_module(mod_name)
            except ImportError:
                return None
            suffix, mode, mod_type = mod_info
            if mod_type == imp.PY_SOURCE:
                loader = _SourceFileLoader(mod_name, file,
                                           filename, mod_info)
            elif mod_type == imp.PY_COMPILED:
                loader = _CompiledFileLoader(mod_name, file,
                                             filename, mod_info)
            elif mod_type == imp.PKG_DIRECTORY:
                loader = _PackageDirLoader(mod_name, file,
                                           filename, mod_info)
            elif mod_type == imp.C_EXTENSION:
                loader = _FileSystemLoader(mod_name, file,
                                           filename, mod_info)
            else:
                loader = _BasicLoader(mod_name, file,
                                      filename, mod_info)
            return loader


    class _FileSystemImporter(object):
        """PEP 302 importer wrapper for filesystem based imports"""
        def __init__(self, path_item=None):
            if path_item is not None:
                if path_item != '' and not os.path.isdir(path_item):
                    raise ImportError("%s is not a directory" % path_item)
                self.path_dir = path_item
            else:
                raise ImportError("Filesystem importer requires "
                                  "a directory name")

        def find_module(self, mod_name, path=None):
            if path is not None:
                return None
            path_dir = self.path_dir
            if path_dir == '':
                path_dir = os.getcwd()
            sub_name = mod_name.rsplit(".", 1)[-1]
            try:
                file, filename, mod_info = imp.find_module(sub_name,
                                                           [path_dir])
            except ImportError:
                return None
            if not filename.startswith(path_dir):
                return None
            suffix, mode, mod_type = mod_info
            if mod_type == imp.PY_SOURCE:
                loader = _SourceFileLoader(mod_name, file,
                                           filename, mod_info)
            elif mod_type == imp.PY_COMPILED:
                loader = _CompiledFileLoader(mod_name, file,
                                             filename, mod_info)
            elif mod_type == imp.PKG_DIRECTORY:
                loader = _PackageDirLoader(mod_name, file,
                                           filename, mod_info)
            elif mod_type == imp.C_EXTENSION:
                loader = _FileSystemLoader(mod_name, file,
                                           filename, mod_info)
            else:
                loader = _BasicLoader(mod_name, file,
                                      filename, mod_info)
            return loader


    class _BasicLoader(object):
        """PEP 302 loader wrapper for top level import machinery"""
        def __init__(self, mod_name, file, filename, mod_info):
            self.mod_name = mod_name
            self.file = file
            self.filename = filename
            self.mod_info = mod_info

        def _fix_name(self, mod_name):
            if mod_name is None:
                mod_name = self.mod_name
            elif mod_name != self.mod_name:
                raise ImportError("Loader for module %s cannot handle "
                                  "module %s" % (self.mod_name, mod_name))
            return mod_name

        def load_module(self, mod_name=None):
            mod_name = self._fix_name(mod_name)
            mod = imp.load_module(mod_name, self.file,
                                  self.filename, self.mod_info)
            mod.__loader__ = self  # for introspection
            return mod

        def get_code(self, mod_name=None):
            return None

        def get_source(self, mod_name=None):
            return None

        def is_package(self, mod_name=None):
            return False

        def close(self):
            if self.file:
                self.file.close()

        def __del__(self):
            self.close()


    class _FileSystemLoader(_BasicLoader):
        """PEP 302 loader wrapper for filesystem based imports"""
        def get_code(self, mod_name=None):
            mod_name = self._fix_name(mod_name)
            return self._get_code(mod_name)

        def get_data(self, pathname):
            return open(pathname, "rb").read()

        def get_filename(self, mod_name=None):
            mod_name = self._fix_name(mod_name)
            return self._get_filename(mod_name)

        def get_source(self, mod_name=None):
            mod_name = self._fix_name(mod_name)
            return self._get_source(mod_name)

        def is_package(self, mod_name=None):
            mod_name = self._fix_name(mod_name)
            return self._is_package(mod_name)

        def _get_code(self, mod_name):
            return None

        def _get_filename(self, mod_name):
            return self.filename

        def _get_source(self, mod_name):
            return None

        def _is_package(self, mod_name):
            return False

    class _PackageDirLoader(_FileSystemLoader):
        """PEP 302 loader wrapper for PKG_DIRECTORY directories"""
        def _is_package(self, mod_name):
            return True


    class _SourceFileLoader(_FileSystemLoader):
        """PEP 302 loader wrapper for PY_SOURCE modules"""
        def _get_code(self, mod_name):
            return compile(self._get_source(mod_name),
                           self.filename, 'exec')

        def _get_source(self, mod_name):
            f = self.file
            f.seek(0)
            return f.read()


    class _CompiledFileLoader(_FileSystemLoader):
        """PEP 302 loader wrapper for PY_COMPILED modules"""
        def _get_code(self, mod_name):
            f = self.file
            f.seek(0)
            return _read_compiled_file(f)


    def _get_importer(path_item):
        """Retrieve a PEP 302 importer for the given path item

        The returned importer is cached in sys.path_importer_cache
        if it was newly created by a path hook.

        If there is no importer, a wrapper around the basic import
        machinery is returned. This wrapper is never inserted into
        the importer cache (None is inserted instead).

        The cache (or part of it) can be cleared manually if a
        rescan of sys.path_hooks is necessary.
        """
        try:
            importer = sys.path_importer_cache[path_item]
        except KeyError:
            for path_hook in sys.path_hooks:
                try:
                    importer = path_hook(path_item)
                    break
                except ImportError:
                    pass
            else:
                importer = None
            sys.path_importer_cache[path_item] = importer
        if importer is None:
            try:
                importer = _FileSystemImporter(path_item)
            except ImportError:
                pass
        return importer


    def _get_path_loader(mod_name, path=None):
        """Retrieve a PEP 302 loader using a path importer"""
        if path is None:
            path = sys.path
            absolute_loader = _AbsoluteImporter().find_module(mod_name)
            if isinstance(absolute_loader, _FileSystemLoader):
                # Found in filesystem, so scan path hooks
                # before accepting this one as the right one
                loader = None
            else:
                # Not found in filesystem, so use top-level loader
                loader = absolute_loader
        else:
            loader = absolute_loader = None
        if loader is None:
            for path_item in path:
                importer = _get_importer(path_item)
                if importer is not None:
                    loader = importer.find_module(mod_name)
                    if loader is not None:
                        # Found a loader for our module
                        break
            else:
                # No path hook found, so accept the top level loader
                loader = absolute_loader
        return loader

    def _get_package(pkg_name):
        """Retrieve a named package"""
        pkg = __import__(pkg_name)
        sub_pkg_names = pkg_name.split(".")
        for sub_pkg in sub_pkg_names[1:]:
            pkg = getattr(pkg, sub_pkg)
        return pkg

    def _get_loader(mod_name, path=None):
        """Retrieve a PEP 302 loader for the given module or package

        If the module or package is accessible via the normal import
        mechanism, a wrapper around the relevant part of that machinery
        is returned.

        Non PEP 302 mechanisms (e.g. the Windows registry) used by the
        standard import machinery to find files in alternative locations
        are partially supported, but are searched AFTER sys.path. Normally,
        these locations are searched BEFORE sys.path, preventing sys.path
        entries from shadowing them.
        For this to cause a visible difference in behaviour, there must
        be a module or package name that is accessible via both sys.path
        and one of the non PEP 302 file system mechanisms. In this case,
        the emulation will find the former version, while the builtin
        import mechanism will find the latter.
        Items of the following types can be affected by this discrepancy:
            imp.C_EXTENSION
            imp.PY_SOURCE
            imp.PY_COMPILED
            imp.PKG_DIRECTORY
        """
        try:
            loader = sys.modules[mod_name].__loader__
        except (KeyError, AttributeError):
            loader = None
        if loader is None:
            imp.acquire_lock()
            try:
                # Module not in sys.modules, or uses an unhooked loader
                parts = mod_name.rsplit(".", 1)
                if len(parts) == 2:
                    # Sub package, so use parent package's path
                    pkg_name, sub_name = parts
                    if pkg_name and pkg_name[0] != '.':
                        if path is not None:
                            raise ImportError("Path argument must be None "
                                            "for a dotted module name")
                        pkg = _get_package(pkg_name)
                        try:
                            path = pkg.__path__
                        except AttributeError:
                            raise ImportError(pkg_name +
                                            " is not a package")
                    else:
                        raise ImportError("Relative import syntax is not "
                                          "supported by _get_loader()")
                else:
                    # Top level module, so stick with default path
                    sub_name = mod_name

                for importer in sys.meta_path:
                    loader = importer.find_module(mod_name, path)
                    if loader is not None:
                        # Found a metahook to handle the module
                        break
                else:
                    # Handling via the standard path mechanism
                    loader = _get_path_loader(mod_name, path)
            finally:
                imp.release_lock()
        return loader


# This helper is needed due to a missing component in the PEP 302
# loader protocol (specifically, "get_filename" is non-standard)
def _get_filename(loader, mod_name):
    try:
        get_filename = loader.get_filename
    except AttributeError:
        return None
    else:
        return get_filename(mod_name)

# ------------------------------------------------------------
# Done with the import machinery emulation, on with the code!

def _run_code(code, run_globals, init_globals,
              mod_name, mod_fname, mod_loader):
    """Helper for _run_module_code"""
    if init_globals is not None:
        run_globals.update(init_globals)
    run_globals.update(__name__ = mod_name,
                       __file__ = mod_fname,
                       __loader__ = mod_loader)
    exec code in run_globals
    return run_globals

def _run_module_code(code, init_globals=None,
                    mod_name=None, mod_fname=None,
                    mod_loader=None, alter_sys=False):
    """Helper for run_module"""
    # Set up the top level namespace dictionary
    if alter_sys:
        # Modify sys.argv[0] and sys.module[mod_name]
        temp_module = imp.new_module(mod_name)
        mod_globals = temp_module.__dict__
        saved_argv0 = sys.argv[0]
        restore_module = mod_name in sys.modules
        if restore_module:
            saved_module = sys.modules[mod_name]
        imp.acquire_lock()
        try:
            sys.argv[0] = mod_fname
            sys.modules[mod_name] = temp_module
            try:
                _run_code(code, mod_globals, init_globals,
                          mod_name, mod_fname, mod_loader)
            finally:
                sys.argv[0] = saved_argv0
                if restore_module:
                    sys.modules[mod_name] = saved_module
                else:
                    del sys.modules[mod_name]
        finally:
            imp.release_lock()
        # Copy the globals of the temporary module, as they
        # may be cleared when the temporary module goes away
        return mod_globals.copy()
    else:
        # Leave the sys module alone
        return _run_code(code, {}, init_globals,
                         mod_name, mod_fname, mod_loader)


def run_module(mod_name, init_globals=None,
                         run_name=None, alter_sys=False):
    """Execute a module's code without importing it

       Returns the resulting top level namespace dictionary
    """
    loader = _get_loader(mod_name)
    if loader is None:
        raise ImportError("No module named " + mod_name)
    code = loader.get_code(mod_name)
    if code is None:
        raise ImportError("No code object available for " + mod_name)
    filename = _get_filename(loader, mod_name)
    if run_name is None:
        run_name = mod_name
    return _run_module_code(code, init_globals, run_name,
                            filename, loader, alter_sys)


if __name__ == "__main__":
    # Run the module specified as the next command line argument
    if len(sys.argv) < 2:
        print >> sys.stderr, "No module specified for execution"
    else:
        del sys.argv[0] # Make the requested module sys.argv[0]
        run_module(sys.argv[0], run_name="__main__", alter_sys=True)
