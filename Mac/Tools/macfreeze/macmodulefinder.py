"""macmodulefinder - Find modules used in a script. Only slightly
mac-specific, really."""

import sys
import os

import directives

try:
    # This will work if we are frozen ourselves
    import modulefinder
except ImportError:
    # And this will work otherwise
    _FREEZEDIR=os.path.join(sys.prefix, ':Tools:freeze')
    sys.path.insert(0, _FREEZEDIR)
    import modulefinder

#
# Modules that must be included, and modules that need not be included
# (but are if they are found)
#
MAC_INCLUDE_MODULES=['site']
MAC_MAYMISS_MODULES=['posix', 'os2', 'nt', 'ntpath', 'dos', 'dospath',
                'win32api', 'ce', '_winreg',
                'nturl2path', 'pwd', 'sitecustomize',
                'org.python.core',
                'riscos', 'riscosenviron', 'riscospath'
                ]

# An exception:
Missing="macmodulefinder.Missing"

class Module(modulefinder.Module):

    def gettype(self):
        """Return type of module"""
        if self.__path__:
            return 'package'
        if self.__code__:
            return 'module'
        if self.__file__:
            return 'dynamic'
        return 'builtin'

class ModuleFinder(modulefinder.ModuleFinder):

    def add_module(self, fqname):
        if self.modules.has_key(fqname):
            return self.modules[fqname]
        self.modules[fqname] = m = Module(fqname)
        return m

def process(program, modules=None, module_files=None, debug=0):
    if modules is None:
        modules = []
    if module_files is None:
        module_files = []
    missing = []
    #
    # Add the standard modules needed for startup
    #
    modules = modules + MAC_INCLUDE_MODULES
    #
    # search the main source for directives
    #
    extra_modules, exclude_modules, optional_modules, extra_path = \
                    directives.findfreezedirectives(program)
    for m in extra_modules:
        if os.sep in m:
            # It is a file
            module_files.append(m)
        else:
            modules.append(m)
    # collect all modules of the program
    path = sys.path[:]
    dir = os.path.dirname(program)
    path[0] = dir   # "current dir"
    path = extra_path + path
    #
    # Create the module finder and let it do its work
    #
    modfinder = ModuleFinder(path,
                    excludes=exclude_modules, debug=debug)
    for m in modules:
        modfinder.import_hook(m)
    for m in module_files:
        modfinder.load_file(m)
    modfinder.run_script(program)
    module_dict = modfinder.modules
    #
    # Tell the user about missing modules
    #
    maymiss = exclude_modules + optional_modules + MAC_MAYMISS_MODULES
    for m in modfinder.badmodules.keys():
        if not m in maymiss:
            if debug > 0:
                print 'Missing', m
            missing.append(m)
    #
    # Warn the user about unused builtins
    #
    for m in sys.builtin_module_names:
        if m in ('__main__', '__builtin__'):
            pass
        elif not module_dict.has_key(m):
            if debug > 0:
                print 'Unused', m
        elif module_dict[m].gettype() != 'builtin':
            # XXXX Can this happen?
            if debug > 0:
                print 'Conflict', m
    return module_dict, missing
