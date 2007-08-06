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
try:
    from imp import get_loader
except ImportError:
    from pkgutil import get_loader

__all__ = [
    "run_module",
]


def _run_code(code, run_globals, init_globals,
              mod_name, mod_fname, mod_loader):
    """Helper for _run_module_code"""
    if init_globals is not None:
        run_globals.update(init_globals)
    run_globals.update(__name__ = mod_name,
                       __file__ = mod_fname,
                       __loader__ = mod_loader)
    exec(code, run_globals)
    return run_globals

def _run_module_code(code, init_globals=None,
                     mod_name=None, mod_fname=None,
                     mod_loader=None, alter_sys=False):
    """Helper for run_module"""
    # Set up the top level namespace dictionary
    if alter_sys:
        # Modify sys.argv[0] and sys.modules[mod_name]
        sys.argv[0] = mod_fname
        module = imp.new_module(mod_name)
        sys.modules[mod_name] = module
        mod_globals = module.__dict__
    else:
        # Leave the sys module alone
        mod_globals = {}
    return _run_code(code, mod_globals, init_globals,
                     mod_name, mod_fname, mod_loader)


# This helper is needed due to a missing component in the PEP 302
# loader protocol (specifically, "get_filename" is non-standard)
def _get_filename(loader, mod_name):
    try:
        get_filename = loader.get_filename
    except AttributeError:
        return None
    else:
        return get_filename(mod_name)


def run_module(mod_name, init_globals=None,
                         run_name=None, alter_sys=False):
    """Execute a module's code without importing it

       Returns the resulting top level namespace dictionary
    """
    loader = get_loader(mod_name)
    if loader is None:
        raise ImportError("No module named %s" % mod_name)
    if loader.is_package(mod_name):
        raise ImportError(("%s is a package and cannot " +
                          "be directly executed") % mod_name)
    code = loader.get_code(mod_name)
    if code is None:
        raise ImportError("No code object available for %s" % mod_name)
    filename = _get_filename(loader, mod_name)
    if run_name is None:
        run_name = mod_name
    return _run_module_code(code, init_globals, run_name,
                            filename, loader, alter_sys)


if __name__ == "__main__":
    # Run the module specified as the next command line argument
    if len(sys.argv) < 2:
        print("No module specified for execution", file=sys.stderr)
    else:
        del sys.argv[0] # Make the requested module sys.argv[0]
        run_module(sys.argv[0], run_name="__main__", alter_sys=True)
