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
# This module is maintained by Greg and is available at:
#    http://www.lyra.org/greg/python/imputil.py
#
# Since this isn't in the Python distribution yet, we'll use the CVS ID
# for tracking:
#   $Id$
#

# note: avoid importing non-builtin modules
import imp
import sys
import strop
import __builtin__

# for the DirectoryImporter
import struct
import marshal

_StringType = type('')
_ModuleType = type(sys)

class ImportManager:
  "Manage the import process."

  def install(self):
    ### warning: Python 1.6 will have a different hook mechanism; this
    ### code will need to change.
    self.__chain_import = __builtin__.__import__
    self.__chain_reload = __builtin__.reload
    __builtin__.__import__ = self._import_hook
    ### fix this
    #__builtin__.reload = None
    #__builtin__.reload = self._reload_hook

  def add_suffix(self, suffix, importer):
    assert isinstance(importer, SuffixImporter)
    self.suffixes.append((suffix, importer))

  ######################################################################
  #
  # PRIVATE METHODS
  #
  def __init__(self):
    # we're definitely going to be importing something in the future,
    # so let's just load the OS-related facilities.
    if not _os_stat:
      _os_bootstrap()

    # Initialize the set of suffixes that we recognize and import.
    # The default will import dynamic-load modules first, followed by
    # .py files (or a .py file's cached bytecode)
    self.suffixes = [ ]
    for desc in imp.get_suffixes():
      if desc[2] == imp.C_EXTENSION:
        self.suffixes.append((desc[0], DynLoadSuffixImporter(desc)))
    self.suffixes.append(('.py', PySuffixImporter()))

    # This is the importer that we use for grabbing stuff from the
    # filesystem. It defines one more method (import_from_dir) for our use.
    self.fs_imp = _FilesystemImporter(self.suffixes)

  def _import_hook(self, fqname, globals=None, locals=None, fromlist=None):
    """Python calls this hook to locate and import a module."""

    parts = strop.split(fqname, '.')

    # determine the context of this import
    parent = self._determine_import_context(globals)

    # if there is a parent, then its importer should manage this import
    if parent:
      module = parent.__importer__._do_import(parent, parts, fromlist)
      if module:
        return module

    # has the top module already been imported?
    try:
      top_module = sys.modules[parts[0]]
    except KeyError:

      # look for the topmost module
      top_module = self._import_top_module(parts[0])
      if not top_module:
        # the topmost module wasn't found at all.
        raise ImportError, 'No module named ' + fqname
        return self.__chain_import(name, globals, locals, fromlist)

    # fast-path simple imports
    if len(parts) == 1:
      if not fromlist:
        return top_module

      if not top_module.__dict__.get('__ispkg__'):
        # __ispkg__ isn't defined (the module was not imported by us), or
        # it is zero.
        #
        # In the former case, there is no way that we could import
        # sub-modules that occur in the fromlist (but we can't raise an
        # error because it may just be names) because we don't know how
        # to deal with packages that were imported by other systems.
        #
        # In the latter case (__ispkg__ == 0), there can't be any sub-
        # modules present, so we can just return.
        #
        # In both cases, since len(parts) == 1, the top_module is also
        # the "bottom" which is the defined return when a fromlist exists.
        return top_module

    importer = top_module.__dict__.get('__importer__')
    if importer:
      return importer._finish_import(top_module, parts[1:], fromlist)

    # If the importer does not exist, then we have to bail. A missing importer
    # means that something else imported the module, and we have no knowledge
    # of how to get sub-modules out of the thing.
    raise ImportError, 'No module named ' + fqname
    return self.__chain_import(name, globals, locals, fromlist)

  def _determine_import_context(self, globals):
    """Returns the context in which a module should be imported.

    The context could be a loaded (package) module and the imported module
    will be looked for within that package. The context could also be None,
    meaning there is no context -- the module should be looked for as a
    "top-level" module.
    """

    if not globals or not globals.get('__importer__'):
      # globals does not refer to one of our modules or packages. That
      # implies there is no relative import context (as far as we are
      # concerned), and it should just pick it off the standard path.
      return None

    # The globals refer to a module or package of ours. It will define
    # the context of the new import. Get the module/package fqname.
    parent_fqname = globals['__name__']

    # if a package is performing the import, then return itself (imports
    # refer to pkg contents)
    if globals['__ispkg__']:
      parent = sys.modules[parent_fqname]
      assert globals is parent.__dict__
      return parent

    i = strop.rfind(parent_fqname, '.')

    # a module outside of a package has no particular import context
    if i == -1:
      return None

    # if a module in a package is performing the import, then return the
    # package (imports refer to siblings)
    parent_fqname = parent_fqname[:i]
    parent = sys.modules[parent_fqname]
    assert parent.__name__ == parent_fqname
    return parent

  def _import_top_module(self, name):
    # scan sys.path looking for a location in the filesystem that contains
    # the module, or an Importer object that can import the module.
    for item in sys.path:
      if type(item) == _StringType:
        module = self.fs_imp.import_from_dir(item, name)
      else:
        module = item.import_top(name)
      if module:
        return module
    return None

  def _reload_hook(self, module):
    "Python calls this hook to reload a module."

    # reloading of a module may or may not be possible (depending on the
    # importer), but at least we can validate that it's ours to reload
    importer = module.__dict__.get('__importer__')
    if not importer:
      return self.__chain_reload(module)

    # okay. it is using the imputil system, and we must delegate it, but
    # we don't know what to do (yet)
    ### we should blast the module dict and do another get_code(). need to
    ### flesh this out and add proper docco...
    raise SystemError, "reload not yet implemented"


class Importer:
  "Base class for replacing standard import functions."

  def install(self):
    sys.path.insert(0, self)

  def import_top(self, name):
    "Import a top-level module."
    return self._import_one(None, name, name)

  ######################################################################
  #
  # PRIVATE METHODS
  #
  def _finish_import(self, top, parts, fromlist):
    # if "a.b.c" was provided, then load the ".b.c" portion down from
    # below the top-level module.
    bottom = self._load_tail(top, parts)

    # if the form is "import a.b.c", then return "a"
    if not fromlist:
      # no fromlist: return the top of the import tree
      return top

    # the top module was imported by self.
    #
    # this means that the bottom module was also imported by self (just
    # now, or in the past and we fetched it from sys.modules).
    #
    # since we imported/handled the bottom module, this means that we can
    # also handle its fromlist (and reliably use __ispkg__).

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
    if bottom.__ispkg__:
      self._import_fromlist(bottom, fromlist)

    # if the form is "from a.b import c, d" then return "b"
    return bottom

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

    ### backwards-compat
    if len(result) == 2:
      result = result + ({},)

    module = self._process_result(result, fqname)

    # insert the module into its parent
    if parent:
      setattr(parent, modname, module)
    return module

  def _process_result(self, (ispkg, code, values), fqname):
    # did get_code() return an actual module? (rather than a code object)
    is_module = type(code) is _ModuleType

    # use the returned module, or create a new one to exec code into
    if is_module:
      module = code
    else:
      module = imp.new_module(fqname)

    ### record packages a bit differently??
    module.__importer__ = self
    module.__ispkg__ = ispkg

    # insert additional values into the module (before executing the code)
    module.__dict__.update(values)

    # the module is almost ready... make it visible
    sys.modules[fqname] = module

    # execute the code within the module's namespace
    if not is_module:
      exec code in module.__dict__

    return module

  def _load_tail(self, m, parts):
    """Import the rest of the modules, down from the top-level module.

    Returns the last module in the dotted list of modules.
    """
    for part in parts:
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

  def _do_import(self, parent, parts, fromlist):
    """Attempt to import the module relative to parent.

    This method is used when the import context specifies that <self>
    imported the parent module.
    """
    top_name = parts[0]
    top_fqname = parent.__name__ + '.' + top_name
    top_module = self._import_one(parent, top_name, top_fqname)
    if not top_module:
      # this importer and parent could not find the module (relatively)
      return None

    return self._finish_import(top_module, parts[1:], fromlist)

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

    This method should return None, or a 3-tuple.

    * If the module was not found, then None should be returned.

    * The first item of the 2- or 3-tuple should be the integer 0 or 1,
      specifying whether the module that was found is a package or not.

    * The second item is the code object for the module (it will be
      executed within the new module's namespace). This item can also
      be a fully-loaded module object (e.g. loaded from a shared lib).

    * The third item is a dictionary of name/value pairs that will be
      inserted into new module before the code object is executed. This
      is provided in case the module's code expects certain values (such
      as where the module was found). When the second item is a module
      object, then these names/values will be inserted *after* the module
      has been loaded/initialized.
    """
    raise RuntimeError, "get_code not implemented"


######################################################################
#
# Some handy stuff for the Importers
#

# byte-compiled file suffic character
_suffix_char = __debug__ and 'c' or 'o'

# byte-compiled file suffix
_suffix = '.py' + _suffix_char

# the C_EXTENSION suffixes
_c_suffixes = filter(lambda x: x[2] == imp.C_EXTENSION, imp.get_suffixes())

def _compile(pathname, timestamp):
  """Compile (and cache) a Python source file.

  The file specified by <pathname> is compiled to a code object and
  returned.

  Presuming the appropriate privileges exist, the bytecodes will be
  saved back to the filesystem for future imports. The source file's
  modification timestamp must be provided as a Long value.
  """
  codestring = open(pathname, 'r').read()
  if codestring and codestring[-1] != '\n':
    codestring = codestring + '\n'
  code = __builtin__.compile(codestring, pathname, 'exec')

  # try to cache the compiled code
  try:
    f = open(pathname + _suffix_char, 'wb')
  except IOError:
    pass
  else:
    f.write('\0\0\0\0')
    f.write(struct.pack('<I', timestamp))
    marshal.dump(code, f)
    f.flush()
    f.seek(0, 0)
    f.write(imp.get_magic())
    f.close()

  return code

_os_stat = _os_path_join = None
def _os_bootstrap():
  "Set up 'os' module replacement functions for use during import bootstrap."

  names = sys.builtin_module_names

  join = None
  if 'posix' in names:
    sep = '/'
    from posix import stat
  elif 'nt' in names:
    sep = '\\'
    from nt import stat
  elif 'dos' in names:
    sep = '\\'
    from dos import stat
  elif 'os2' in names:
    sep = '\\'
    from os2 import stat
  elif 'mac' in names:
    from mac import stat
    def join(a, b):
      if a == '':
        return b
      path = s
      if ':' not in a:
        a = ':' + a
      if a[-1:] <> ':':
        a = a + ':'
      return a + b
  else:
    raise ImportError, 'no os specific module found'
  
  if join is None:
    def join(a, b, sep=sep):
      if a == '':
        return b
      lastchar = a[-1:]
      if lastchar == '/' or lastchar == sep:
        return a + b
      return a + sep + b

  global _os_stat
  _os_stat = stat

  global _os_path_join
  _os_path_join = join

def _os_path_isdir(pathname):
  "Local replacement for os.path.isdir()."
  try:
    s = _os_stat(pathname)
  except OSError:
    return None
  return (s[0] & 0170000) == 0040000

def _timestamp(pathname):
  "Return the file modification time as a Long."
  try:
    s = _os_stat(pathname)
  except OSError:
    return None
  return long(s[8])

def _fs_import(dir, modname, fqname):
  "Fetch a module from the filesystem."

  pathname = _os_path_join(dir, modname)
  if _os_path_isdir(pathname):
    values = { '__pkgdir__' : pathname, '__path__' : [ pathname ] }
    ispkg = 1
    pathname = _os_path_join(pathname, '__init__')
  else:
    values = { }
    ispkg = 0

    # look for dynload modules
    for desc in _c_suffixes:
      file = pathname + desc[0]
      try:
        fp = open(file, desc[1])
      except IOError:
        pass
      else:
        module = imp.load_module(fqname, fp, file, desc)
        values['__file__'] = file
        return 0, module, values

  t_py = _timestamp(pathname + '.py')
  t_pyc = _timestamp(pathname + _suffix)
  if t_py is None and t_pyc is None:
    return None
  code = None
  if t_py is None or (t_pyc is not None and t_pyc >= t_py):
    file = pathname + _suffix
    f = open(file, 'rb')
    if f.read(4) == imp.get_magic():
      t = struct.unpack('<I', f.read(4))[0]
      if t == t_py:
        code = marshal.load(f)
    f.close()
  if code is None:
    file = pathname + '.py'
    code = _compile(file, t_py)

  values['__file__'] = file
  return ispkg, code, values


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
  """Importer subclass to import from (file) archives.

  This Importer handles imports of the style <archive>.<subfile>, where
  <archive> can be located using a subclass-specific mechanism and the
  <subfile> is found in the archive using a subclass-specific mechanism.

  This class defines two hooks for subclasses: one to locate an archive
  (and possibly return some context for future subfile lookups), and one
  to locate subfiles.
  """

  def get_code(self, parent, modname, fqname):
    if parent:
      # the Importer._finish_import logic ensures that we handle imports
      # under the top level module (package / archive).
      assert parent.__importer__ == self

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

  def get_code(self, parent, modname, fqname):
    if parent:
      dir = parent.__pkgdir__
    else:
      dir = self.dir

    # defer the loading of OS-related facilities
    if not _os_stat:
      _os_bootstrap()

    # Return the module (and other info) if found in the specified
    # directory. Otherwise, return None.
    return _fs_import(dir, modname, fqname)

  def __repr__(self):
    return '<%s.%s for "%s" at 0x%x>' % (self.__class__.__module__,
                                         self.__class__.__name__,
                                         self.dir,
                                         id(self))

######################################################################
#
# Emulate the standard path-style import mechanism
#
class PathImporter(Importer):
  def __init__(self, path=sys.path):
    self.path = path

    # we're definitely going to be importing something in the future,
    # so let's just load the OS-related facilities.
    if not _os_stat:
      _os_bootstrap()

  def get_code(self, parent, modname, fqname):
    if parent:
      # we are looking for a module inside of a specific package
      return _fs_import(parent.__pkgdir__, modname, fqname)

    # scan sys.path, looking for the requested module
    for dir in self.path:
      result = _fs_import(dir, modname, fqname)
      if result:
        return result

    # not found
    return None


######################################################################
#
# Emulate the import mechanism for builtin and frozen modules
#
class BuiltinImporter(Importer):
  def get_code(self, parent, modname, fqname):
    if parent:
      # these modules definitely do not occur within a package context
      return None

    # look for the module
    if imp.is_builtin(modname):
      type = imp.C_BUILTIN
    elif imp.is_frozen(modname):
      type = imp.PY_FROZEN
    else:
      # not found
      return None

    # got it. now load and return it.
    module = imp.load_module(modname, None, modname, ('', '', type))
    return 0, module, { }


######################################################################
#
# Internal importer used for importing from the filesystem
#
class _FilesystemImporter(Importer):
  def __init__(self, suffixes):
    # this list is shared with the ImportManager.
    self.suffixes = suffixes

  def import_from_dir(self, dir, fqname):
    result = self._import_pathname(_os_path_join(dir, fqname), fqname)
    if result:
      return self._process_result(result, fqname)
    return None

  def get_code(self, parent, modname, fqname):
    # This importer is never used with an empty parent. Its existence is
    # private to the ImportManager. The ImportManager uses the
    # import_from_dir() method to import top-level modules/packages.
    # This method is only used when we look for a module within a package.
    assert parent

    return self._import_pathname(_os_path_join(parent.__pkgdir__, modname),
                                 fqname)

  def _import_pathname(self, pathname, fqname):
    if _os_path_isdir(pathname):
      result = self._import_pathname(_os_path_join(pathname, '__init__'),
                                     fqname)
      if result:
        values = result[2]
        values['__pkgdir__'] = pathname
        values['__path__'] = [ pathname ]
        return 1, result[1], values
      return None

    for suffix, importer in self.suffixes:
      filename = pathname + suffix
      try:
        finfo = _os_stat(filename)
      except OSError:
        pass
      else:
        return importer.import_file(filename, finfo, fqname)
    return None

######################################################################
#
# SUFFIX-BASED IMPORTERS
#

class SuffixImporter:
  def import_file(self, filename, finfo, fqname):
    raise RuntimeError

class PySuffixImporter(SuffixImporter):
  def import_file(self, filename, finfo, fqname):
    file = filename[:-3] + _suffix
    t_py = long(finfo[8])
    t_pyc = _timestamp(file)

    code = None
    if t_pyc is not None and t_pyc >= t_py:
      f = open(file, 'rb')
      if f.read(4) == imp.get_magic():
        t = struct.unpack('<I', f.read(4))[0]
        if t == t_py:
          code = marshal.load(f)
      f.close()
    if code is None:
      file = filename
      code = _compile(file, t_py)

    return 0, code, { '__file__' : file }

class DynLoadSuffixImporter(SuffixImporter):
  def __init__(self, desc):
    self.desc = desc

  def import_file(self, filename, finfo, fqname):
    fp = open(filename, self.desc[1])
    module = imp.load_module(fqname, fp, filename, self.desc)
    module.__file__ = filename
    return 0, module, { }


######################################################################

def _test_dir():
  "Debug/test function to create DirectoryImporters from sys.path."
  path = sys.path[:]
  path.reverse()
  for d in path:
    DirectoryImporter(d).install()

def _test_revamp():
  "Debug/test function for the revamped import system."
  PathImporter().install()
  BuiltinImporter().install()

def _print_importers():
  items = sys.modules.items()
  items.sort()
  for name, module in items:
    if module:
      print name, module.__dict__.get('__importer__', '-- no importer')
    else:
      print name, '-- non-existent module'

def _test_revamp():
  ImportManager().install()
  sys.path.insert(0, BuiltinImporter())

######################################################################
