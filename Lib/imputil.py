#
# imputil.py
#
# Written by Greg Stein. Public Domain.
# No Copyright, no Rights Reserved, and no Warranties.
#
# Utilities to help out with custom import mechanisms.
#
# Additional modifications were contribed by Marc-Andre Lemburg and
# Gordon McMillan.
#

__version__ = '0.3'

# note: avoid importing non-builtin modules
import imp
import sys
import strop
import __builtin__	### why this instead of just using __builtins__ ??

# for the DirectoryImporter
import struct
import marshal

class Importer:
  "Base class for replacing standard import functions."

  def install(self):
    self.__chain_import = __builtin__.__import__
    self.__chain_reload = __builtin__.reload
    __builtin__.__import__ = self._import_hook
    __builtin__.reload = self._reload_hook

  ######################################################################
  #
  # PRIVATE METHODS
  #
  def _import_hook(self, name, globals=None, locals=None, fromlist=None):
    """Python calls this hook to locate and import a module.

    This method attempts to load the (dotted) module name. If it cannot
    find it, then it delegates the import to the next import hook in the
    chain (where "next" is defined as the import hook that was in place
    at the time this Importer instance was installed).
    """

    ### insert a fast-path check for whether the module is already
    ### loaded? use a variant of _determine_import_context() which
    ### returns a context regardless of Importer used. generate an
    ### fqname and look in sys.modules for it.

    # determine the context of this import
    parent = self._determine_import_context(globals)

    # import the module within the context, or from the default context
    top, tail = self._import_top_module(parent, name)
    if top is None:
      # the module was not found; delegate to the next import hook
      return self.__chain_import(name, globals, locals, fromlist)

    # the top module may be under the control of a different importer.
    # if so, then defer to that importer for completion of the import.
    # note it may be self, or is undefined so we (self) may as well
    # finish the import.
    importer = top.__dict__.get('__importer__', self)
    return importer._finish_import(top, tail, fromlist)

  def _finish_import(self, top, tail, fromlist):
    # if "a.b.c" was provided, then load the ".b.c" portion down from
    # below the top-level module.
    bottom = self._load_tail(top, tail)

    # if the form is "import a.b.c", then return "a"
    if not fromlist:
      # no fromlist: return the top of the import tree
      return top

    # the top module was imported by self, or it was not imported through
    # the Importer mechanism and self is simply handling the import of
    # the sub-modules and fromlist.
    #
    # this means that the bottom module was also imported by self, or we
    # are handling things in the absence of a prior Importer
    #
    # ### why the heck are we handling it? what is the example scenario
    # ### where this happens? note that we can't determine is_package()
    # ### for non-Importer modules.
    #
    # since we imported/handled the bottom module, this means that we can
    # also handle its fromlist (and reliably determine is_package()).

    # if the bottom node is a package, then (potentially) import some modules.
    #
    # note: if it is not a package, then "fromlist" refers to names in
    #       the bottom module rather than modules.
    # note: for a mix of names and modules in the fromlist, we will
    #       import all modules and insert those into the namespace of
    #       the package module. Python will pick up all fromlist names
    #       from the bottom (package) module; some will be modules that
    #       we imported and stored in the namespace, others are expected
    #       to be present already.
    if self._is_package(bottom.__dict__):
      self._import_fromlist(bottom, fromlist)

    # if the form is "from a.b import c, d" then return "b"
    return bottom

  def _reload_hook(self, module):
    "Python calls this hook to reload a module."

    # reloading of a module may or may not be possible (depending on the
    # importer), but at least we can validate that it's ours to reload
    importer = module.__dict__.get('__importer__', None)
    if importer is not self:
      return self.__chain_reload(module)

    # okay. it is ours, but we don't know what to do (yet)
    ### we should blast the module dict and do another get_code(). need to
    ### flesh this out and add proper docco...
    raise SystemError, "reload not yet implemented"

  def _determine_import_context(self, globals):
    """Returns the context in which a module should be imported.

    The context could be a loaded (package) module and the imported module
    will be looked for within that package. The context could also be None,
    meaning there is no context -- the module should be looked for as a
    "top-level" module.
    """

    if not globals or \
       globals.get('__importer__', None) is not self:
      # globals does not refer to one of our modules or packages.
      # That implies there is no relative import context, and it
      # should just pick it off the standard path.
      return None

    # The globals refer to a module or package of ours. It will define
    # the context of the new import. Get the module/package fqname.
    parent_fqname = globals['__name__']

    # for a package, return itself (imports refer to pkg contents)
    if self._is_package(globals):
      parent = sys.modules[parent_fqname]
      assert globals is parent.__dict__
      return parent

    i = strop.rfind(parent_fqname, '.')

    # a module outside of a package has no particular import context
    if i == -1:
      return None

    # for a module in a package, return the package (imports refer to siblings)
    parent_fqname = parent_fqname[:i]
    parent = sys.modules[parent_fqname]
    assert parent.__name__ == parent_fqname
    return parent

  def _import_top_module(self, parent, name):
    """Locate the top of the import tree (relative or absolute).

    parent defines the context in which the import should occur. See
    _determine_import_context() for details.

    Returns a tuple (module, tail). module is the loaded (top-level) module,
    or None if the module is not found. tail is the remaining portion of
    the dotted name.
    """
    i = strop.find(name, '.')
    if i == -1:
      head = name
      tail = ""
    else:
      head = name[:i]
      tail = name[i+1:]
    if parent:
      fqname = "%s.%s" % (parent.__name__, head)
    else:
      fqname = head
    module = self._import_one(parent, head, fqname)
    if module:
      # the module was relative, or no context existed (the module was
      # simply found on the path).
      return module, tail
    if parent:
      # we tried relative, now try an absolute import (from the path)
      module = self._import_one(None, head, head)
      if module:
        return module, tail

    # the module wasn't found
    return None, None

  def _import_one(self, parent, modname, fqname):
    "Import a single module."

    # has the module already been imported?
    try:
      return sys.modules[fqname]
    except KeyError:
      pass

    # load the module's code, or fetch the module itself
    result = self.get_code(parent, modname, fqname)
    if result is None:
      return None

    # did get_code() return an actual module? (rather than a code object)
    is_module = type(result[1]) is type(sys)

    # use the returned module, or create a new one to exec code into
    if is_module:
      module = result[1]
    else:
      module = imp.new_module(fqname)

    ### record packages a bit differently??
    module.__importer__ = self
    module.__ispkg__ = result[0]

    # if present, the third item is a set of values to insert into the module
    if len(result) > 2:
      module.__dict__.update(result[2])

    # the module is almost ready... make it visible
    sys.modules[fqname] = module

    # execute the code within the module's namespace
    if not is_module:
      exec result[1] in module.__dict__

    # insert the module into its parent
    if parent:
      setattr(parent, modname, module)
    return module

  def _load_tail(self, m, tail):
    """Import the rest of the modules, down from the top-level module.

    Returns the last module in the dotted list of modules.
    """
    if tail:
      for part in strop.splitfields(tail, '.'):
        fqname = "%s.%s" % (m.__name__, part)
        m = self._import_one(m, part, fqname)
        if not m:
          raise ImportError, "No module named " + fqname
    return m

  def _import_fromlist(self, package, fromlist):
    'Import any sub-modules in the "from" list.'

    # if '*' is present in the fromlist, then look for the '__all__' variable
    # to find additional items (modules) to import.
    if '*' in fromlist:
      fromlist = list(fromlist) + list(package.__dict__.get('__all__', []))

    for sub in fromlist:
      # if the name is already present, then don't try to import it (it
      # might not be a module!).
      if sub != '*' and not hasattr(package, sub):
        subname = "%s.%s" % (package.__name__, sub)
        submod = self._import_one(package, sub, subname)
        if not submod:
          raise ImportError, "cannot import name " + subname

  def _is_package(self, module_dict):
    """Determine if a given module (dictionary) specifies a package.

    The package status is in the module-level name __ispkg__. The module
    must also have been imported by self, so that we can reliably apply
    semantic meaning to __ispkg__.

    ### weaken the test to issubclass(Importer)?
    """
    return module_dict.get('__importer__', None) is self and \
           module_dict['__ispkg__']

  ######################################################################
  #
  # METHODS TO OVERRIDE
  #
  def get_code(self, parent, modname, fqname):
    """Find and retrieve the code for the given module.

    parent specifies a parent module to define a context for importing. It
    may be None, indicating no particular context for the search.

    modname specifies a single module (not dotted) within the parent.

    fqname specifies the fully-qualified module name. This is a (potentially)
    dotted name from the "root" of the module namespace down to the modname.
    If there is no parent, then modname==fqname.

    This method should return None, a 2-tuple, or a 3-tuple.

    * If the module was not found, then None should be returned.

    * The first item of the 2- or 3-tuple should be the integer 0 or 1,
      specifying whether the module that was found is a package or not.

    * The second item is the code object for the module (it will be
      executed within the new module's namespace). This item can also
      be a fully-loaded module object (e.g. loaded from a shared lib).

    * If present, the third item is a dictionary of name/value pairs that
      will be inserted into new module before the code object is executed.
      This provided in case the module's code expects certain values (such
      as where the module was found). When the second item is a module
      object, then these names/values will be inserted *after* the module
      has been loaded/initialized.
    """
    raise RuntimeError, "get_code not implemented"


######################################################################
#
# Simple function-based importer
#
class FuncImporter(Importer):
  "Importer subclass to use a supplied function rather than method overrides."
  def __init__(self, func):
    self.func = func
  def get_code(self, parent, modname, fqname):
    return self.func(parent, modname, fqname)

def install_with(func):
  FuncImporter(func).install()


######################################################################
#
# Base class for archive-based importing
#
class PackageArchiveImporter(Importer):
  "Importer subclass to import from (file) archives."

  def get_code(self, parent, modname, fqname):
    if parent:
      # if a parent "package" is provided, then we are importing a sub-file
      # from the archive.
      result = self.get_subfile(parent.__archive__, modname)
      if result is None:
        return None
      if type(result) == type(()):
        return (0,) + result
      return 0, result

    # no parent was provided, so the archive should exist somewhere on the
    # default "path".
    archive = self.get_archive(modname)
    if archive is None:
      return None
    return 1, "", {'__archive__':archive}

  def get_archive(self, modname):
    """Get an archive of modules.

    This method should locate an archive and return a value which can be
    used by get_subfile to load modules from it. The value may be a simple
    pathname, an open file, or a complex object that caches information
    for future imports.

    Return None if the archive was not found.
    """
    raise RuntimeError, "get_archive not implemented"

  def get_subfile(self, archive, modname):
    """Get code from a subfile in the specified archive.

    Given the specified archive (as returned by get_archive()), locate
    and return a code object for the specified module name.

    A 2-tuple may be returned, consisting of a code object and a dict
    of name/values to place into the target module.

    Return None if the subfile was not found.
    """
    raise RuntimeError, "get_subfile not implemented"


class PackageArchive(PackageArchiveImporter):
  "PackageArchiveImporter subclass that refers to a specific archive."

  def __init__(self, modname, archive_pathname):
    self.__modname = modname
    self.__path = archive_pathname

  def get_archive(self, modname):
    if modname == self.__modname:
      return self.__path
    return None

  # get_subfile is passed the full pathname of the archive


######################################################################
#
# Emulate the standard directory-based import mechanism
#

class DirectoryImporter(Importer):
  "Importer subclass to emulate the standard importer."

  def __init__(self, dir):
    self.dir = dir
    self.ext_char = __debug__ and 'c' or 'o'
    self.ext = '.py' + self.ext_char

  def get_code(self, parent, modname, fqname):
    if parent:
      dir = parent.__pkgdir__
    else:
      dir = self.dir

    # pull the os module from our instance data. we don't do this at the
    # top-level, because it isn't a builtin module (and we want to defer
    # loading non-builtins until as late as possible).
    try:
      os = self.os
    except AttributeError:
      import os
      self.os = os

    pathname = os.path.join(dir, modname)
    if os.path.isdir(pathname):
      values = { '__pkgdir__' : pathname }
      ispkg = 1
      pathname = os.path.join(pathname, '__init__')
    else:
      values = { }
      ispkg = 0

    t_py = self._timestamp(pathname + '.py')
    t_pyc = self._timestamp(pathname + self.ext)
    if t_py is None and t_pyc is None:
      return None
    code = None
    if t_py is None or (t_pyc is not None and t_pyc >= t_py):
      f = open(pathname + self.ext, 'rb')
      if f.read(4) == imp.get_magic():
        t = struct.unpack('<I', f.read(4))[0]
        if t == t_py:
          code = marshal.load(f)
      f.close()
    if code is None:
      code = self._compile(pathname + '.py', t_py)
    return ispkg, code, values

  def _timestamp(self, pathname):
    try:
      s = self.os.stat(pathname)
    except OSError:
      return None
    return long(s[8])

  def _compile(self, pathname, timestamp):
    codestring = open(pathname, 'r').read()
    if codestring and codestring[-1] != '\n':
      codestring = codestring + '\n'
    code = __builtin__.compile(codestring, pathname, 'exec')

    # try to cache the compiled code
    try:
      f = open(pathname + self.ext_char, 'wb')
      f.write('\0\0\0\0')
      f.write(struct.pack('<I', timestamp))
      marshal.dump(code, f)
      f.flush()
      f.seek(0, 0)
      f.write(imp.get_magic())
      f.close()
    except OSError:
      pass

    return code

  def __repr__(self):
    return '<%s.%s for "%s" at 0x%x>' % (self.__class__.__module__,
                                         self.__class__.__name__,
                                         self.dir,
                                         id(self))

def _test_dir():
  "Debug/test function to create DirectoryImporters from sys.path."
  path = sys.path[:]
  path.reverse()
  for d in path:
    DirectoryImporter(d).install()

######################################################################
