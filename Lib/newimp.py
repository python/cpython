"""Prototype of 'import' functionality enhanced to implement packages.

Why packages?  Packages enable module nesting and sibling module
imports.  'Til now, the python module namespace was flat, which
means every module had to have a unique name, in order to not
conflict with names of other modules on the load path.  Furthermore,
suites of modules could not be structurally affiliated with one
another.

With packages, a suite of, eg, email-oriented modules can include a
module named 'mailbox', without conflicting with the, eg, 'mailbox'
module of a shared-memory suite - 'email.mailbox' vs
'shmem.mailbox'.  Packages also enable modules within a suite to
load other modules within their package without having the package
name hard-coded.  Similarly, package suites of modules can be loaded
as a unit, by loading the package that contains them.

Usage: once installed (newimp.install(); newimp.revert() to revert to
the prior __import__ routine), 'import ...' and 'from ... import ...'
can be used to:

  - import modules from the search path, as before.

  - import modules from within other directory "packages" on the search
    path using a '.' dot-delimited nesting syntax.  The nesting is fully
    recursive.

    For example, 'import test.test_types' will import the test_types
    module within the 'test' package.  The calling environment would
    then access the module as 'test.test_types', which is the name of
    the fully-loaded 'test_types' module.  It is found contained within
    the stub (ie, only partially loaded) 'test' module, hence accessed as
    'test.test_types'.

  - import siblings from modules within a package, using '__.' as a shorthand
    prefix to refer to the parent package.  This enables referential
    transparency - package modules need not know their package name.

    The '__' package references are actually names assigned within
    modules, to refer to their containing package.  This means that
    variable references can be made to imported modules, or to variables
    defined via 'import ... from', also using the '__.var' shorthand
    notation.  This establishes a proper equivalence between the import
    reference '__.sibling' and the var reference '__.sibling'.  

  - import an entire package as a unit, by importing the package directory.
    If there is a module named '__init__.py' in the package, it controls the
    load.  Otherwise, all the modules in the dir, including packages, are
    inherently loaded into the package module's namespace.

    For example, 'import test' will load the modules of the entire 'test'
    package, at least until a test failure is encountered.

    In a package, a module with the name '__init__' has a special role.
    If present in a package directory, then it is loaded into the package
    module, instead of loading the contents of the directory.  This
    enables the __init__ module to control the load, possibly loading
    the entire directory deliberately (using 'import __', or even
    'from __ import *', to load all the module contents directly into the
    package module).

  - perform any combination of the above - have a package that contains
    packages, etc.

Modules have a few new attributes in support of packages.  As mentioned
above, '__' is a shorthand attribute denoting the modules' parent package,
also denoted in the module by '__package__'.  Additionally, modules have
associated with them a '__pkgpath__', a path by which sibling modules are
found."""

__version__ = "$Revision$"

# $Id$ First release:
# Ken.Manheimer@nist.gov, 5-Apr-1995, for python 1.2

# Issues (scattered in code - search for three asterisks)
# *** Despite my efforts, 'reload(newimp)' will foul things up.
# *** Normalize_pathname will only work for Unix - which we need to detect.
# *** when a module with the name of the platform (as indicated by
#     to-be-created var sys.platform), the package path gets '.' and the
#     platform dir.
# *** use sys.impadmin for things like an import load-hooks var
# *** Import-load-hook keying module name versus package path, which dictates
#     additions to the default ('.' and os-specific dir) path
# *** Document that the __init__.py can set __.__pkgpath__, in which case that
#     will be used for the package-relative loads.
# *** Add a 'recursive' option to reload, for reload of package constituent
#     modules (including subpackages), as well.  Or maybe that should be the
#     default, and eg stub-completion should override that default.  ???

# Developers Notes:
#
# - 'sys.stub_modules' registers "incidental" (partially loaded) modules.
#   A stub module is promoted to the fully-loaded 'sys.modules' list when it is
#   explicitly loaded as a unit.
# - One load nuance - the actual load of most module types goes into the
#   already-generated stub module.  HOWEVER, eg dynamically loaded modules
#   generate a new module object, which must supplant the existing stub.  One
#   consequence is that the import process must use indirection through
#   sys.stub_modules or sys.modules to track the actual modules across some of
#   the phases.
# - The test routines are cool, including a transient directory
#   hierarchy facility, and a means of skipping to later tests by giving
#   the test routine a numeric arg.
# - There may still be some loose ends, not to mention bugs.  But the full
#   functionality should be there.
# - The ImportStack object is necessary to carry the list of in-process imports
#   across very open-ended recursions, where the state cannot be passed
#   explicitly via the import_module calls; for a primary example, via exec of
#   an 'import' statement within a module.
# - Python's (current) handling of extension modules, via imp.load_dynamic,
#   does too much, some of which needs to be undone.  See comments in
#   load_module.  Among other things, we actually change the __name__ of the
#   module, which conceivably may break something.

try:
    VERBOSE
except NameError:
    VERBOSE = 0				# Will be reset by init(1), also.

import sys, string, regex, types, os, marshal, traceback
import __main__, __builtin__

newimp_globals = vars()

try:
    import imp				# Build on this recent addition
except ImportError:
    raise ImportError, 'Pkg import module depends on optional "imp" module'#==X

from imp import SEARCH_ERROR, PY_SOURCE, PY_COMPILED, C_EXTENSION

def defvar(varNm, envDict, val, override=0):
    """If VARNAME does not have value in DICT, assign VAL to it.  Optional arg
    OVERRIDE means force the assignment in any case."""
    if (not envDict.has_key(varNm)) or override:
	envDict[varNm] = val
    
def init(full_reset=0):
    """Do environment initialization, including retrofitting sys.modules with
    module attributes."""
    # Retrofit all existing modules with package attributes, under auspices of
    # __root__:

    locals, globals = vars(), newimp_globals

    if full_reset:
	global VERBOSE
	VERBOSE = 0

    # sys.stub_modules tracks modules partially loaded modules, ie loaded only
    # incidental to load of nested components.  Together with sys.modules and
    # the import stack, it serves as part of the module registration mechanism.
    defvar('stub_modules', sys.__dict__, {}, full_reset)

    # Environment setup - "root" module, '__root__'
    # Establish root package '__root__' in __main__ and newimp envs.

    # Longhand for name of variable identifying module's containing package:
    defvar('PKG_NM', globals, "__package__", full_reset)
    # Shorthand for module's container:
    defvar('PKG_SHORT_NM', globals, "__", full_reset)
    defvar('PKG_SHORT_NM_LEN', globals, len(PKG_SHORT_NM), full_reset)

    # Name of controlling module for a package, if any:
    defvar('INIT_MOD_NM', globals, "__init__", full_reset)

    # Paths eventually will be extended to accomodate non-filesystem media -
    # eg, URLs, composite objects, who knows.

    # Name assigned in sys for general import administration:
    defvar('IMP_SYS_NM', globals, "imp_admin", full_reset)
    defvar('MOD_LOAD_HOOKS', globals, "mod_load_hooks", full_reset)
    if full_reset:
	defvar(IMP_SYS_NM, sys.__dict__, {MOD_LOAD_HOOKS: {}}, full_reset)
    

    # Name assigned in each module to tuple describing module import attrs:
    defvar('IMP_ADMIN', globals, "__impadmin__", full_reset)
    # The load-path obtaining for this package.  Not defined for non-packages.
    # If not set, package directory is used.  If no package directory
    # registered, sys.path is used.
    defvar('PKG_PATH', globals, 0, full_reset)
    # File from which module was loaded - may be None, eg, for __root__:
    defvar('MOD_TYPE', globals, 1, full_reset)
    # Exact path from which the module was loaded:
    defvar('MOD_PATHNAME', globals, 2, full_reset)
    # Package within which the module was found:
    defvar('MOD_PACKAGE', globals, 3, full_reset)
    defvar('USE_PATH', globals, 'either PKG_PATH or my dir', full_reset)
    
    # We're aliasing the top-level __main__ module as '__root__':
    defvar('__root__', globals, __main__, full_reset)
    defvar('ROOT_MOD_NM', globals, "__root__", full_reset)
    if not sys.modules.has_key('__root__') or full_reset:
	# and register it as an imported module:
	sys.modules[ROOT_MOD_NM] = __root__

    # Register package information in all existing top-level modules - they'll
    # the None's mean, among other things, that their USE_PATH's all defer to
    # sys.path.
    for aMod in sys.modules.values():
	if (not aMod.__dict__.has_key(PKG_NM)) or full_reset:
	    set_mod_attrs(aMod, None, __root__, None, None)

    try:
	__builtin__.__import__
	defvar('origImportFunc', globals, __builtin__.__import__)
	defvar('origReloadFunc', globals, __builtin__.reload)
    except AttributeError:
	pass

    defvar('PY_PACKAGE', globals, 4, full_reset)
    defvar('PY_FROZEN', globals, 5, full_reset)
    defvar('PY_BUILTIN', globals, 6, full_reset)

    # Establish lookup table from mod-type "constants" to names:
    defvar('mod_types', globals,
	   {SEARCH_ERROR: 'SEARCH_ERROR',
	    PY_SOURCE: 'PY_SOURCE',
	    PY_COMPILED: 'PY_COMPILED',
	    C_EXTENSION: 'C_EXTENSION',
	    PY_PACKAGE: 'PY_PACKAGE',
	    PY_FROZEN: 'PY_FROZEN',
	    PY_BUILTIN: 'PY_BUILTIN'},
	   full_reset)

    defvar('stack', globals, ImportStack(), full_reset)

def install():
    """Install newimp import_module() routine, for package support.

    newimp.revert() reverts to __import__ routine that was superceded."""
    __builtin__.__import__ = import_module
    __builtin__.reload = reload
    __builtin__.unload = unload
    __builtin__.bypass = bypass
    return 'Enhanced import functionality in place.'
def revert():
    """Revert to original __builtin__.__import__ func, if newimp.install() has
    been executed."""
    if not (origImportFunc and origReloadFunc):
	raise SystemError, "Can't find original import and reload funcs." # ==X
    __builtin__.__import__ = origImportFunc
    __builtin__.reload = origReloadFunc
    del __builtin__.unload, __builtin__.bypass
    return 'Original import routines back in place.'

def import_module(name,
		  envLocals=None, envGlobals=None,
		  froms=None,
		  inPkg=None):
    """Primary service routine implementing 'import' with package nesting.

    NAME:		name as specified to 'import NAME' or 'from NAME...'
    LOCALS, GLOBALS:	local and global dicts obtaining for import
    FROMS:		list of strings of "..." in 'import blat from ...'
    INPKG:		package to which the name search is restricted, for use
    			by recursive package loads (from import_module()).

    A subtle difference from the old import - modules that do fail
    initialization will not be registered in sys.modules, ie will not, in
    effect, be registered as being loaded.  Note further that packages which
    fail their overall load, but have successfully loaded constituent modules,
    will be accessible in the importing namespace as stub modules.

    A new routine, 'newimp.bypass()', provides the means to circumvent
    constituent modules that fail their load, in order to enable load of the
    remainder of a package."""

    rootMod = sys.modules[ROOT_MOD_NM]

    note("import_module: seeking '%s'" % name, 1)

    # We need callers environment dict for local path and resulting module
    # binding.
    if not envGlobals:
	# This should not happen, but does for imports called from within
	# functions.
	envLocals, envGlobals = exterior()

    if inPkg:
	pkg = inPkg
    elif envGlobals.has_key(PKG_NM):
	pkg = envGlobals[PKG_NM]
    else:
	# ** KLUDGE - cover for modules that lack package attributes:
	pkg = rootMod

    if pkg != rootMod:
	note(' - relative to package %s' % pkg)

    modList = theMod = absNm = nesting = None

    # Normalize
    #  - absNm is absolute w.r.t. __root__
    #  - relNm is relative w.r.t. pkg.
    if inPkg:
	absNm, relNm = pkg.__name__ + '.' + name, name
    else:
	absNm, relNm, pkg = normalize_import_ref(name, pkg)
    note("Normalized: %s%s" % (absNm, (((relNm != absNm)
					and (" ('%s' in %s)" % (relNm, pkg)))
				       or '')), 3)

    pkgPath = get_mod_attrs(pkg, USE_PATH)

    try:			# try...finally guards import stack integrity.

	if stack.push(absNm):
	    # We're nested inside a containing import of this module, perhaps
	    # indirectly.  Avoid infinite recursion at this point by using the
	    # existing stub module, for now.  Load of it will be completed by
	    # the superior import.
	    note('recursion on in-process module %s, punting with stub' %
		 absNm)
	    theMod = stack.mod(absNm)

	else:

	    # Try to find already-imported:
	    if sys.modules.has_key(absNm):
		note('found ' + absNm + ' already imported')
		theMod = sys.modules[absNm]
		stack.mod(absNm, theMod)

	    else:		# Actually do load, of one sort or another:

		# Seek builtin or frozen first:
		theMod = imp.init_builtin(absNm)
		if theMod:
		    set_mod_attrs(theMod, None, pkg, None, PY_BUILTIN)
		    stack.mod(absNm, theMod)
		    note('found builtin ' + absNm)
		else:
		    theMod = imp.init_frozen(absNm)
		    if theMod:
			set_mod_attrs(theMod, None, pkg, None, PY_FROZEN)
			stack.mod(absNm, theMod)
			note('found frozen ' + absNm)

		if not theMod:
		    # Not already-loaded, in-process, builtin, or frozen -
		    # we're seeking in the outside world (filesystem):

		    if sys.stub_modules.has_key(absNm):

			# A package for which we have a stub:
			theMod = reload(sys.stub_modules[absNm], inPkg)

		    else:

			# Now we actually search the fs.

			if type(pkgPath) == types.StringType:
			    pkgPath = [pkgPath]

			# Find a path leading to the module:
			modList = find_module(relNm, pkgPath, absNm)
			if not modList:
			    raise ImportError, ("module '%s' not found" % #==X
						absNm)

			# We have a list of successively nested dirs leading
			# to the module, register with import admin, as stubs:
			nesting = register_mod_nesting(modList, pkg)

			# Load from file if necessary and possible:
			modNm, modf, path, ty = modList[-1]
			note('found type %s - %s' % (mod_types[ty[2]], absNm))

			# Establish the module object in question:
			theMod = procure_module(absNm)
			stack.mod(absNm, theMod)
			
			# Do the load:
			theMod = load_module(theMod, ty[2], modf, inPkg)

			commit_mod_containment(absNm)

		    # Successful load - promote to fully-imported status:
		    register_module(theMod, theMod.__name__)


	# We have a loaded module (perhaps stub): situate specified components,
	# and return appropriate thing.  According to guido:
	#
	# "Note that for "from spam.ham import bacon" your function should
	#  return the object denoted by 'spam.ham', while for "import
	#  spam.ham" it should return the object denoted by 'spam' -- the
	#  STORE instructions following the import statement expect it this
	#  way."
	# *** The above rationale should probably be reexamined, since newimp
	#     actually takes care of populating the caller's namespace.

	if not froms:

	    # Return the outermost container, possibly stub:
	    if nesting:
		return find_mod_registration(nesting[0][0])
	    else:
		return find_mod_registration(string.splitfields(absNm,'.')[0])
	else:

	    return theMod

    finally:				# Decrement stack registration:
	stack.pop(absNm)
	    

def reload(module, inPkg = None):
    """Re-parse and re-initialize an already (or partially) imported MODULE.

    The argument can be an already loaded module object or a string name of a
    loaded module or a "stub" module that was partially loaded package module
    incidental to the full load of a contained module.
    
    This is useful if you have edited the module source file using an external
    editor and want to try out the new version without leaving the Python
    interpreter.  The return value is the resulting module object.

    Contrary to the old 'reload', the load is sought from the same location
    where the module was originally found.  If you wish to do a fresh load from
    a different module on the path, do an 'unload()' and then an import.

    When a module is reloaded, its dictionary (containing the module's
    global variables) is retained.  Redefinitions of names will
    override the old definitions, so this is generally not a problem.
    If the new version of a module does not define a name that was
    defined by the old version, the old definition remains.  This
    feature can be used to the module's advantage if it maintains a
    global table or cache of objects -- with a `try' statement it can
    test for the table's presence and skip its initialization if
    desired.

    It is legal though generally not very useful to reload built-in or
    dynamically loaded modules, except for `sys', `__main__' and
    `__builtin__'.  In certain cases, however, extension modules are
    not designed to be initialized more than once, and may fail in
    arbitrary ways when reloaded.

    If a module imports objects from another module using `from' ...
    `import' ..., calling `reload()' for the other module does not
    redefine the objects imported from it -- one way around this is to
    re-execute the `from' statement, another is to use `import' and
    qualified names (MODULE.NAME) instead.

    If a module instantiates instances of a class, reloading the module
    that defines the class does not affect the method definitions of
    the instances, unless they are reinstantiated -- they continue to use the
    old class definition.  The same is true for derived classes."""

    if type(module) == types.StringType:
	theMod = find_mod_registration(module)
    elif type(module) == types.ModuleType:
	theMod = module
    else:
	raise ImportError, '%s not already imported'			# ==X

    if theMod in [sys.modules[ROOT_MOD_NM], sys.modules['__builtin__']]:
	raise ImportError, 'cannot re-init internal module'		# ==X

    try:
	thePath = get_mod_attrs(theMod, MOD_PATHNAME)
    except KeyError:
	thePath = None

    if not thePath:
	# If we have no path for the module, we can only reload it from
	# scratch:
	note('no pathname registered for %s, doing full reload' % theMod)
	unload(theMod)
	envGlobals, envLocals = exterior()
	return import_module(theMod.__name__,
			     envGlobals, envLocals, None, inPkg)
    else:

	stack.mod(theMod.__name__, theMod)
	ty = get_mod_attrs(theMod, MOD_TYPE)
	if ty in [PY_SOURCE, PY_COMPILED]:
	    note('reload invoked for %s %s' % (mod_types[ty], theMod))
	    thePath, ty, openFile = prefer_compiled(thePath, ty)
	else:
	    openFile = open(thePath, get_suffixes(ty)[1])
	return load_module(theMod,					# ==>
			   ty,
			   openFile,
			   inPkg)
def unload(module):
    """Remove registration for a module, so import will do a fresh load.

    Returns the module registries (sys.modules and/or sys.stub_modules) where
    it was found."""
    if type(module) == types.ModuleType:
	module = module.__name__
    gotit = []
    for which in ['sys.modules', 'sys.stub_modules']:
	m = eval(which)
	try:
	    del m[module]
	    gotit.append(which)
	except KeyError:
	    pass
    if not gotit:
	raise ValueError, '%s not a module or a stub' % module		# ==X
    else: return gotit
def bypass(modNm):
    """Register MODULE-NAME so module will be skipped, eg in package load."""
    if sys.modules.has_key(modNm):
	raise ImportError("'%s' already imported, cannot be bypassed." % modNm)
    else:
	sys.modules[modNm] = imp.new_module('bypass()ed module %s' % modNm)
	commit_mod_containment(modNm)
	

def normalize_import_ref(name, pkg):
    """Produce absolute and relative nm and relative pkg given MODNM and origin
    PACKAGE, reducing out all '__'s in the process."""

    # First reduce out all the '__' container-refs we can:
    outwards, inwards = 0, []
    for nm in string.splitfields(name, '.'):
	if nm == PKG_SHORT_NM:
	    if inwards:
		# Pop a containing inwards:
		del inwards[-1]
	    else:
		# (Effectively) leading '__' - notch outwards:
		outwards = outwards + 1
	else:
	    inwards.append(nm)
    inwards = string.joinfields(inwards, '.')

    # Now identify the components:

    if not outwards:
	pkg = sys.modules[ROOT_MOD_NM]
    else:
	while outwards > 1:
	    pkg = pkg.__dict__[PKG_NM]	# We'll just loop at top
	    if pkg == __root__:
		break							# ==v
	    outwards = outwards - 1

    if not inwards:			# Entire package:
	return pkg.__name__, pkg.__name__, pkg				# ==>
    else:				# Name relative to package:
	if pkg == __root__:
	    return inwards, inwards, pkg				# ==>
	else:
	    return pkg.__name__ + '.' + inwards, inwards, pkg		# ==>

class ImportStack:
    """Provide judicious support for mutually recursive import loops.

    Mutually recursive imports, eg a module that imports the package that
    contains it, which in turn imports the module, are not uncommon, and must
    be supported judiciously.  This class is used to track cycles, so a module
    already in the process of being imported (via 'stack.push(module)', and
    concluded via 'stack.release(module)') is not redundantly pursued; *except*
    when a module master '__init__.py' loads the module, in which case it is
    'stack.relax(module)'ed, so the full import is pursued."""

    def __init__(self):
	self._cycles = {}
	self._mods = {}
	self._looped = []
    def in_process(self, modNm):
	"""1 if modNm load already in process, 0 otherwise."""
	return self._cycles.has_key(modNm)				# ==>
    def looped(self, modNm):
	"""1 if modNm load has looped once or more, 0 otherwise."""
	return modNm in self._looped
    def push(self, modNm):
	"""1 if modNm already in process and not 'relax'ed, 0 otherwise.
	(Note that the 'looped' status remains even when the cycle count
	returns to 1.  This is so error messages can indicate that it was, at
	some point, looped during the import process.)"""
	if self.in_process(modNm):
	    self._looped.append(modNm)
	    self._cycles[modNm] = self._cycles[modNm] + 1
	    return 1							# ==>
	else:
	    self._cycles[modNm] = 1
	    return 0							# ==>
    def mod(self, modNm, mod=None):
	"""Associate MOD-NAME with MODULE, for easy reference."""
	if mod:
	    self._mods[modNm] = mod
	else:
	    try:
		return self._mods[modNm]				# ==>
	    except KeyError:
		return None
    def pop(self, modNm):
	"""Decrement stack count of MODNM"""
	if self.in_process(modNm):
	    amt = self._cycles[modNm] = self._cycles[modNm] - 1
	    if amt < 1:
		del self._cycles[modNm]
		if modNm in self._looped:
		    self._looped.remove(modNm)
		if self._mods.has_key(modNm):
		    del self._mods[modNm]
    def relax(self, modNm):
	"""Enable modNm load despite being registered as already in-process."""
	if self._cycles.has_key(modNm):
	    del self._cycles[modNm]

def find_module(name, path, absNm=''):
    """Locate module NAME on PATH.  PATH is pathname string or a list of them.

    Note that up-to-date compiled versions of a module are preferred to plain
    source, and compilation is automatically performed when necessary and
    possible.

    Returns a list of the tuples returned by 'find_mod_file()', one for
    each nested level, deepest last."""

    checked = []			# For avoiding redundant dir lists.

    if not absNm: absNm = name

    # Parse name into list of nested components, 
    expNm = string.splitfields(name, '.')

    for curPath in path:

	if (type(curPath) != types.StringType) or (curPath in checked):
	    # Disregard bogus or already investigated path elements:
	    continue							# ==^
	else:
	    # Register it for subsequent disregard.
	    checked.append(curPath)

	if len(expNm) == 1:

	    # Non-nested module name:

	    got = find_mod_file(curPath, absNm)
	    if got:
		note('using %s' % got[2], 3)
		return [got]						# ==>

	else:

	    # Composite name specifying nested module:

	    gotList = []; nameAccume = expNm[0]

	    got = find_mod_file(curPath, nameAccume)
	    if not got:			# Continue to next prospective path.
		continue						# ==^
	    else:
		gotList.append(got)
		nm, file, fullPath, ty = got

	    # Work on successively nested components:
	    for component in expNm[1:]:
		# 'ty'pe of containing component must be package:
		if ty[2] != PY_PACKAGE:
		    gotList, got = [], None
		    break						# ==v
		if nameAccume:
		    nameAccume = nameAccume + '.' + component
		else:
		    nameAccume = component
		got = find_mod_file(fullPath, nameAccume)
		if got:
		    gotList.append(got)
		    nm, file, fullPath, ty = got
		else:
		    # Clear state vars:
		    gotList, got, nameAccume = [], None, ''
		    break						# ==v
	    # Found nesting all the way to the specified tip:
	    if got:
		return gotList						# ==>

    # Failed.
    return None

def find_mod_file(pathNm, modname):
    """Find right module file given DIR and module NAME, compiling if needed.

    If successful, returns quadruple consisting of:
     - mod name,
     - file object,
     - full pathname for the found file,
     - a description triple as contained in the list returned by get_suffixes.

    Otherwise, returns None.

    Note that up-to-date compiled versions of a module are preferred to plain
    source, and compilation is automatically performed, when necessary and
    possible."""

    relNm = modname[1 + string.rfind(modname, '.'):]

    for suff, mode, ty in get_suffixes():
	fullPath = os.path.join(pathNm, relNm + suff)
	note('trying ' + fullPath + '...', 4)
	try:
	    modf = open(fullPath, mode)
	except IOError:
	    # ** ?? Skip unreadable ones:
	    continue							# ==^

	if ty == PY_PACKAGE:
	    # Enforce directory characteristic:
	    if not os.path.isdir(fullPath):
		note('Skipping non-dir match ' + fullPath, 3)
		continue						# ==^
	    else:
		return (modname, modf, fullPath, (suff, mode, ty))	# ==>
	    

	elif ty in [PY_SOURCE, PY_COMPILED]:
	    usePath, useTy, openFile = prefer_compiled(fullPath, ty)
	    return (modname,						# ==>
		    openFile,
		    usePath,
		    get_suffixes(useTy))

	elif ty == C_EXTENSION:
	    note('found C_EXTENSION ' + fullPath, 3)
	    return (modname, modf, fullPath, (suff, mode, ty))		# ==>

	else:
	    raise SystemError, 'Unanticipated module type encountered'	# ==X

    return None

def prefer_compiled(path, ty, modf=None):
    """Given a path to a .py or .pyc file, attempt to return a path to a
    current pyc file, compiling the .py in the process if necessary.  Returns
    the path to the most current version we can get."""

    if ty == PY_SOURCE:
	if not modf:
	    try:
		modf = open(path, 'r')
	    except IOError:
		pass
	note('working from PY_SOURCE', 3)
	# Try for a compiled version:
	pyc = path + 'c'	# Sadly, we're presuming '.py' suff.
	if (not os.path.exists(pyc) or
	    (os.stat(path)[8] > os.stat(pyc)[8])):
	    # Try to compile:
	    pyc = compile_source(path, modf)
	if pyc and not (os.stat(path)[8] > os.stat(pyc)[8]):
	    # Either pyc was already newer or we just made it so; in either
	    # case it's what we crave:
	    note('but got newer compiled, ' + pyc, 3)
	    try:
		return (pyc, PY_COMPILED, open(pyc, 'rb'))		# ==>
	    except IOError:
		if modf:
		    return (path, PY_SOURCE, modf)			# ==>
		else:
		    raise ImportError, 'Failed acces to .py and .pyc'	# ==X
	else:
	    note("couldn't get newer compiled, using PY_SOURCE", 3)
	    if modf:
		return (path, PY_SOURCE, modf)				# ==>
	    else:
		raise ImportError, 'Failed acces to .py and .pyc'	# ==X

    elif ty == PY_COMPILED:
	note('working from PY_COMPILED', 3)
	if not modf:
	    try:
		modf = open(path, 'rb')
	    except IOError:
		return prefer_compiled(path[:-1], PY_SOURCE)
	# Make sure it is current, trying to compile if necessary, and
	# prefer source failing that:
	note('found compiled ' + path, 3)
	py = path[:-1]			# ** Presuming '.pyc' suffix
	if not os.path.exists(py):
	    note('pyc SANS py: ' + path, 3)
	    return (path, PY_COMPILED, open(py, 'r'))			# ==>
	elif (os.stat(py)[8] > os.stat(path)[8]):
	    note('Forced to compile: ' + py, 3)
	    pyc = compile_source(py, open(py, 'r'))
	    if pyc:
		return (pyc, PY_COMPILED, modf)				# ==>
	    else:
		note('failed compile - must use more recent .py', 3)
		return (py, PY_SOURCE, open(py, 'r'))			# ==>
	else:
	    return (path, PY_COMPILED, modf)				# ==>

def load_module(theMod, ty, theFile, fromMod):
    """Load module NAME, of TYPE, from FILE, within MODULE.

    Optional arg fromMod indicates the module from which the load is being done
    - necessary for detecting import of __ from a package's __init__ module.

    Return the populated module object."""

    # Note: we mint and register intermediate package directories, as necessary
    
    name = theMod.__name__
    nameTail = name[1 + string.rfind(name, '.'):]
    thePath = theFile.name

    if ty == PY_SOURCE:
	exec_into(theFile, theMod, theFile.name)

    elif ty == PY_COMPILED:
	pyc = open(theFile.name, 'rb').read()
	if pyc[0:4] != imp.get_magic():
	    raise ImportError, 'bad magic number: ' + theFile.name	# ==X
	code = marshal.loads(pyc[8:])
	exec_into(code, theMod, theFile.name)

    elif ty == C_EXTENSION:
	# Dynamically loaded C_EXTENSION modules do too much import admin,
	# themselves, which we need to *undo* in order to integrate them with
	# the new import scheme.
	# 1 They register themselves in sys.modules, registering themselves
	#   under their top-level names.  Have to rectify that.
	# 2 The produce their own module objects, *unless* they find an
	#   existing module already registered a la 1, above.  We employ this
	#   quirk to make it use the already generated module.
	try:
	    # Stash a ref to any module that is already registered under the
	    # dyamic module's simple name (nameTail), so we can reestablish it
	    # after the dynamic takes over its' slot:
	    protMod = None
	    if nameTail != name:
		if sys.modules.has_key(nameTail):
		    protMod = sys.modules[nameTail]
	    # Trick the dynamic load, by registering the module we generated
	    # under the nameTail of the module we're loading, so the one we're
	    # loading will use that established module, rather than producing a
	    # new one:
	    sys.modules[nameTail] = theMod
	    theMod = imp.load_dynamic(nameTail, thePath, theFile)
	    theMod.__name__ = name
	    # Cleanup dynamic mod's bogus self-registration, if necessary:
	    if nameTail != name:
		if protMod:
		    # ... reinstating the one that was already there...
		    sys.modules[nameTail] = protMod
		else:
		    if sys.modules.has_key(nameTail):
			# Certain, as long os dynamics continue to misbehave.
			del sys.modules[nameTail]
	    stack.mod(name, theMod)
	    if sys.stub_modules.has_key(name):
		sys.stub_modules[name] = theMod
	    elif sys.modules.has_key(name):
		sys.modules[name] = theMod
	except:
	    # Provide import-nesting info, including signs of circularity:
		raise sys.exc_type, import_trail_msg(str(sys.exc_value),# ==X
						     sys.exc_traceback,
						     name)
    elif ty == PY_PACKAGE:
	# Load package constituents, doing the controlling module *if* it
	# exists *and* it isn't already in process:

	init_mod_f = init_mod = None
	if not stack.in_process(name + '.' + INIT_MOD_NM):
	    # Not already doing __init__ - check for it:
	    init_mod_f = find_mod_file(thePath, INIT_MOD_NM)
	else:
	    note('skipping already-in-process %s.%s' % (theMod.__name__,
							INIT_MOD_NM))
	got = {}
	if init_mod_f:
	    note("Found package's __init__: " + init_mod_f[2])
	    # Enable full continuance of containing-package-load from __init__:
	    if stack.in_process(theMod.__name__):
		stack.relax(theMod.__name__)
	    init_mod = import_module(INIT_MOD_NM,
				     theMod.__dict__, theMod.__dict__,
				     None,
				     theMod)
	else:
	    # ... or else recursively load all constituent modules, except
	    # __init__:
	    for prospect in mod_prospects(thePath):
		if prospect != INIT_MOD_NM:
		    import_module(prospect,
				  theMod.__dict__, theMod.__dict__,
				  None,
				  theMod)

    else:
	raise ImportError, 'Unimplemented import type: %s' % ty		# ==X

    return theMod

def exec_into(obj, module, path):
    """Helper for load_module, execfile/exec path or code OBJ within MODULE."""

    # This depends on ability of exec and execfile to mutilate, erhm, mutate
    # the __dict__ of a module.  It will not work if/when this becomes
    # disallowed, as it is for normal assignments.

    try:
	if type(obj) == types.FileType:
	    execfile(path, module.__dict__, module.__dict__)
	elif type(obj) in [types.CodeType, types.StringType]:
	    exec obj in module.__dict__, module.__dict__
    except:
	    # Make the error message nicer?
	    raise sys.exc_type, import_trail_msg(str(sys.exc_value),	# ==X
						 sys.exc_traceback,
						 module.__name__)
	

def mod_prospects(path):
    """Return a list of prospective modules within directory PATH.

    We actually return the distinct names resulting from stripping the dir
    entries (excluding os.curdir and os.pardir) of their suffixes (as
    represented by 'get_suffixes').

    (Note that matches for the PY_PACKAGE type with null suffix are
    implicitly constrained to be directories.)"""

    # We actually strip the longest matching suffixes, so eg 'dbmmodule.so'
    # mates with 'module.so' rather than '.so'.

    dirList = os.listdir(path)
    excludes = [os.curdir, os.pardir]
    sortedSuffs = sorted_suffixes()
    entries = []
    for item in dirList:
	if item in excludes: continue					# ==^
	for suff in sortedSuffs:
	    # *** ?? maybe platform-specific:
	    sub = -1 * len(suff)
	    if sub == 0:
		if os.path.isdir(os.path.join(path, item)):
		    entries.append(item)
	    elif item[sub:] == suff:
		it = item[:sub]
		if not it in entries:
		    entries.append(it)
		break							# ==v
    return entries
		


def procure_module(name):
    """Return an established or else new module object having NAME.

    First checks sys.modules, then sys.stub_modules."""

    if sys.modules.has_key(name):
	return sys.modules[name]					# ==>
    elif sys.stub_modules.has_key(name):
	return sys.stub_modules[name]					# ==>
    else:
	return (stack.mod(name) or imp.new_module(name))		# ==>

def commit_mod_containment(name):
    """Bind a module object and its containers within their respective
    containers."""
    cume, pkg = '', find_mod_registration(ROOT_MOD_NM)
    for next in string.splitfields(name, '.'):
	if cume:
	    cume = cume + '.' + next
	else:
	    cume = next
	cumeMod = find_mod_registration(cume)
	pkg.__dict__[next] = cumeMod
	pkg = cumeMod

def register_mod_nesting(modList, pkg):
    """Given find_module()-style NEST-LIST and parent PACKAGE, register new
    package components as stub modules, and return list of nested
    module/relative-name pairs.

    Note that the modules objects are not situated in their containing packages
    here - that is left 'til after a successful load, and done by
    commit_mod_nesting()."""
    nesting = []

    for modNm, modF, path, ty in modList:

	relNm = modNm[1 + string.rfind(modNm, '.'):]

	if sys.modules.has_key(modNm):
	    theMod = sys.modules[modNm]	# Nestle in containing package
	    pkg = theMod		# Set as parent for next in sequence.
	elif sys.stub_modules.has_key(modNm):
	    # Similar to above...
	    theMod = sys.stub_modules[modNm]
	    pkg = theMod
	else:
	    theMod = procure_module(modNm)
	    stack.mod(modNm, theMod)
	    # *** ??? Should we be using 'path' instead of modF.name?  If not,
	    # should we get rid of the 'path' return val?
	    set_mod_attrs(theMod, normalize_pathname(modF.name),
			  pkg, None, ty[2])
	    if ty[2] == PY_PACKAGE:
		# Register as a stub:
		register_module(theMod, modNm, 1)
	    pkg = theMod
	nesting.append((theMod.__name__,relNm))

    return nesting

def register_module(theMod, name, stub=0):
    """Properly register MODULE, NAME, and optional STUB qualification."""
    
    if stub:
	sys.stub_modules[name] = theMod
    else:
	sys.modules[name] = theMod
	if sys.stub_modules.has_key(name):
	    del sys.stub_modules[name]

def find_mod_registration(name):
    """Find module named NAME sys.modules, .stub_modules, or on the stack."""
    if sys.stub_modules.has_key(name):
	return sys.stub_modules[name]					# ==>
    elif sys.modules.has_key(name):
	return sys.modules[name]					# ==>
    else:
	if stack.in_process(name):
	    it = stack.mod(name)
	    if it:
		return it						# ==>
	    else:
		raise ValueError, '%s %s in %s or %s' % (name,		# ==X
							 'not registered',
							 'sys.modules',
							 'sys.stub_modules')

def get_mod_attrs(theMod, which = None):
    """Get MODULE object's path, containing-package, and designated path.

    Virtual attribute USE_PATH is derived from PKG_PATH, MOD_PATHNAME,
    and/or sys.path, depending on the module type and settings."""
    it = theMod.__dict__[IMP_ADMIN]
    if which:
	# Load path is either the explicitly designated load path for the
	# package, or else the directory in which it resides:
	if which == USE_PATH:
	    if it[PKG_PATH]:
		# Return explicitly designated path:
		return it[PKG_PATH]					# ==>
	    if it[MOD_PATHNAME]: 
		if it[MOD_TYPE] == PY_PACKAGE:
		    # Return the package's directory path:
		    return [it[MOD_PATHNAME]]				# ==>
		else:
		    # Return the directory where the module resides:
		    return [os.path.split(it[MOD_PATHNAME])[0]]		# ==>
	    # No explicitly designated path - use sys.path, eg for system
	    # modules, etc:
	    return sys.path
	return it[which]						# ==>
    else:
	return it							# ==>

def set_mod_attrs(theMod, path, pkg, pkgPath, ty):
    """Register MOD import attrs PATH, PKG container, and PKGPATH, linking
    the package container into the module along the way."""
    theDict = theMod.__dict__
    try:
	# Get existing one, if any:
	it = theDict[IMP_ADMIN]
    except KeyError:
	# None existing, gen a new one:
	it = [None] * 4
    for fld, val in ((MOD_PATHNAME, path), (MOD_PACKAGE, pkg),
		     (PKG_PATH, pkgPath), (MOD_TYPE, ty)):
	if val:
	    it[fld] = val

    theDict[IMP_ADMIN] = it
    if pkg:
	theDict[PKG_NM] = theDict[PKG_SHORT_NM] = pkg
    return it								# ==>

def format_tb_msg(tb, recursive):
    """This should be in traceback.py, and traceback.print_tb() should use it
    and traceback.extract_tb(), instead of print_tb() and extract_tb() having
    so much redundant code!"""
    tb_lines, formed = traceback.extract_tb(tb), ''
    for line in tb_lines:
	f, lno, nm, ln = line
	if f[-1 * (len(__name__) + 3):] == __name__ + '.py':
	    # Skip newimp notices - agregious hack, justified only by the fact
	    # that this functionality will be properly doable in new impending
	    # exception mechanism:
	    continue
	formed = formed + ('\n%s File "%s", line %d, in %s%s' %
			   (((recursive and '*') or ' '),
			    f, lno, nm,
			    ((ln and '\n    ' + string.strip(ln)) or '')))
    return formed

def import_trail_msg(msg, tb, modNm):
    """Doctor an error message to include the path of the current import, and
    a sign that it is a circular import, if so."""
    return (msg +
	    format_tb_msg(tb,
			  (stack.looped(modNm) and stack.in_process(modNm))))

def compile_source(sourcePath, sourceFile):
    """Given python code source path and file obj, Create a compiled version.

    Return path of compiled version, or None if file creation is not
    successful.  (Compilation errors themselves are passed without restraint.)

    This is an import-private interface, and not well-behaved for general use.
    
    In particular, we presume the validity of the sourcePath, and that it
    includes a '.py' extension."""

    compiledPath = sourcePath[:-3] + '.pyc'
    try:
	compiledFile = open(compiledPath, 'wb')
    except IOError:
	note("write permission denied to " + compiledPath, 3)
	return None
    mtime = os.stat(sourcePath)[8]

    try:
	compiled = compile(sourceFile.read(), sourcePath, 'exec')
    except SyntaxError:
	    # Doctor the exception a bit, to include the source file name in
	    # the report, and then reraise the doctored version.
	    os.unlink(compiledFile.name)
	    sys.exc_value = ((sys.exc_value[0] + ' in ' + sourceFile.name,)
			     + sys.exc_value[1:])
	    raise sys.exc_type, sys.exc_value				# ==X

    # Ok, we have a valid compilation.
    try:
	compiledFile.write(imp.get_magic())		# compiled magic number
	compiledFile.seek(8, 0)				# mtime space holder
	marshal.dump(compiled, compiledFile)		# write the code obj
	compiledFile.seek(4, 0)				# position for mtime
	compiledFile.write(marshal.dumps(mtime)[1:])	# register mtime
	compiledFile.flush()
	compiledFile.close()
	return compiledPath
    except IOError:
	return None							# ==>


got_suffixes = None
got_suffixes_dict = {}
def get_suffixes(ty=None):
    """Produce a list of triples, each describing a type of import file.

    Triples have the form '(SUFFIX, MODE, TYPE)', where:

    SUFFIX is a string found appended to a module name to make a filename for
    that type of import file.

    MODE is the mode string to be passed to the built-in 'open' function - "r"
    for text files, "rb" for binary.

    TYPE is the file type:

     PY_SOURCE:		python source code,
     PY_COMPILED:	byte-compiled python source,
     C_EXTENSION:	compiled-code object file,
     PY_PACKAGE:	python library directory, or
     SEARCH_ERROR:	no module found. """

    # Note: sorted_suffixes() depends on this function's value being invariant.
    # sorted_suffixes() must be revised if this becomes untrue.
    
    global got_suffixes, got_suffixes_dict

    if not got_suffixes:
	# Ensure that the .pyc suffix precedes the .py:
	got_suffixes = [('', 'r', PY_PACKAGE)]
	got_suffixes_dict[PY_PACKAGE] = ('', 'r', PY_PACKAGE)
	py = pyc = None
	for suff in imp.get_suffixes():
	    got_suffixes_dict[suff[2]] = suff
	    if suff[0] == '.py':
		py = suff
	    elif suff[0] == '.pyc':
		pyc = suff
	    else:
		got_suffixes.append(suff)
	got_suffixes.append(pyc)
	got_suffixes.append(py)
    if ty:
	return got_suffixes_dict[ty]					# ==>
    else:
	return got_suffixes						# ==>
		

sortedSuffs = []			# State vars for sorted_suffixes().  Go
def sorted_suffixes():
    """Helper function ~efficiently~ tracks sorted list of module suffixes."""

    # Produce sortedSuffs once - this presumes that get_suffixes does not
    # change from call to call during a python session.  Needs to be
    # corrected if that becomes no longer true.

    global sortedsuffs
    if not sortedSuffs:			# do compute only the "first" time
	for item in get_suffixes():
	    sortedSuffs.append(item[0])
	# Sort them in descending order:
	sortedSuffs.sort(lambda x, y: (((len(x) > len(y)) and 1) or
				       ((len(x) < len(y)) and -1)))
	sortedSuffs.reverse()
    return sortedSuffs


def normalize_pathname(path):
    """Given PATHNAME, return an absolute pathname relative to cwd, reducing
    unnecessary components where convenient (eg, on Unix)."""

    # We do a lot more when we have posix-style paths, eg os.sep == '/'.

    if os.sep != '/':
	return os.path.join(os.getcwd, path)				# ==>

    outwards, inwards = 0, []
    for nm in string.splitfields(path, os.sep):
	if nm != os.curdir:
	    if nm == os.pardir:
		# Translate parent-dir entries to outward notches:
		if inwards:
		    # Pop a containing inwards:
		    del inwards[-1]
		else:
		    # Register leading outward notches:
		    outwards = outwards + 1
	    else:
		inwards.append(nm)
    inwards = string.joinfields(inwards, os.sep)

    if (not inwards) or (inwards[0] != os.sep):
	# Relative path - join with current working directory, (ascending
	# outwards to account for leading parent-dir components):
	cwd = os.getcwd()
	if outwards:
	    cwd = string.splitfields(cwd, os.sep)
	    cwd = string.joinfields(cwd[:len(cwd) - outwards], os.sep)
	if inwards:
	    return os.path.join(cwd, inwards)				# ==>
	else:
	    return cwd							# ==>
    else:
	return inwards							# ==>


# exterior(): Utility routine, obtain local and global dicts of environment
#	      containing/outside the callers environment, ie that of the
#	      caller's caller.  Routines can use exterior() to determine the
#	      environment from which they were called. 

def exterior():
    """Return dyad containing locals and globals of caller's caller.

    Locals will be None if same as globals, ie env is global env."""

    bogus = 'bogus'			# A locally usable exception
    try: raise bogus			# Force an exception object
    except bogus:
	at = sys.exc_traceback.tb_frame.f_back		# The external frame.
	if at.f_back: at = at.f_back			# And further, if any.
	globals, locals = at.f_globals, at.f_locals
	if locals == globals:				# Exterior is global?
	    locals = None
	return (locals, globals)

#########################################################################
#			      TESTING FACILITIES			#

def note(msg, threshold=2):
    if VERBOSE >= threshold: sys.stderr.write('(import: ' + msg + ')\n')

class TestDirHier:
    """Populate a transient directory hierarchy according to a definition
    template - so we can create package/module hierarchies with which to
    exercise the new import facilities..."""

    def __init__(self, template, where='/var/tmp'):
	"""Establish a dir hierarchy, according to TEMPLATE, that will be
	deleted upon deletion of this object (or deliberate invocation of the
	__del__ method)."""
	self.PKG_NM = 'tdh_'
	rev = 0
	while os.path.exists(os.path.join(where, self.PKG_NM+str(rev))):
	    rev = rev + 1
	sys.exc_traceback = None	# Ensure Discard of try/except obj ref
	self.PKG_NM = self.PKG_NM + str(rev)
	self.root = os.path.join(where, self.PKG_NM)
	self.createDir(self.root)
	self.add(template)

    def __del__(self):
	"""Cleanup the test hierarchy."""
	self.remove()
    def add(self, template, root=None):
	"""Populate directory according to template dictionary.

	Keys indicate file names, possibly directories themselves.

	String values dictate contents of flat files.

	Dictionary values dictate recursively embedded dictionary templates."""
	if root == None: root = self.root
	for key, val in template.items():
	    name = os.path.join(root, key)
	    if type(val) == types.StringType:	# flat file
		self.createFile(name, val)
	    elif type(val) == types.DictionaryType:	# embedded dir
		self.createDir(name)
		self.add(val, name)
	    else:
		raise ValueError, ('invalid file-value type, %s' %	# ==X
				   type(val))
    def remove(self, name=''):
	"""Dispose of the NAME (or keys in dictionary), using 'rm -r'."""
	name = os.path.join(self.root, name)
	sys.exc_traceback = None	# Ensure Discard of try/except obj ref
	if os.path.exists(name):
	    print '(TestDirHier: eradicating %s)' % name
	    os.system('rm -r ' + name)
	else:
	    raise IOError, "can't remove non-existent " + name		# ==X
    def createFile(self, name, contents=None):
	"""Establish file NAME with CONTENTS.

	If no contents specfied, contents will be 'print NAME'."""
	f = open(name, 'w')
	if not contents:
	    f.write("print '" + name + "'\n")
	else:
	    f.write(contents)
	f.close
    def createDir(self, name):
	"""Create dir with NAME."""
	return os.mkdir(name, 0755)

skipToTest = 0
atTest = 1
def testExec(msg, execList, locals, globals):
    global skipToTest, atTest
    print 'Import Test:', '(' + str(atTest) + ')', msg, '...'
    atTest = atTest + 1
    if skipToTest > (atTest - 1):
	print ' ... skipping til test', skipToTest
	return
    else:
	print ''
    for stmt in execList:
	exec stmt in locals, globals

def test(number=0, leaveHiers=0):
    """Exercise import functionality, creating a transient dir hierarchy for
    the purpose.

    We actually install the new import functionality, temporarily, resuming the
    existing function on cleanup."""

    import __builtin__

    global skipToTest, atTest
    skipToTest = number
    hier = None

    def unloadFull(mod):
	"""Unload module and offspring submodules, if any."""
	modMod = ''
	if type(mod) == types.StringType:
	    modNm = mod
	elif type(mod) == types.ModuleType:
	    modNm = modMod.__name__
	for subj in sys.modules.keys() + sys.stub_modules.keys():
	    if subj[0:len(modNm)] == modNm:
		unload(subj)

    try:
	__main__.testMods
    except AttributeError:
	__main__.testMods = []
    testMods = __main__.testMods
	

    # Install the newimp routines, within a try/finally:
    try:
	sys.exc_traceback = None
	wasImport = __builtin__.__import__	# Stash default
	wasPath = sys.path
    except AttributeError:
	wasImport = None
    try:
	hiers = []; modules = []
	global VERBOSE
	wasVerbose, VERBOSE = VERBOSE, 1
	__builtin__.__import__ = import_module	# Install new version

	if testMods:		# Clear out imports from previous tests
	    for m in testMods[:]:
		unloadFull(m)
		testMods.remove(m)

	# ------
	# Test 1
	testExec("already imported module: %s" % sys.modules.keys()[0],
		 ['import ' + sys.modules.keys()[0]],
		 vars(), newimp_globals)
	no_sirree = 'no_sirree_does_not_exist'
	# ------
	# Test 2
	testExec("non-existent module: %s" % no_sirree,
		 ['try: import ' + no_sirree +
		  '\nexcept ImportError: pass'],
		  vars(), newimp_globals)
	got = None

	# ------
	# Test 3
	# Find a module that's not yet loaded, from a list of prospects:
	for mod in ['Complex', 'UserDict', 'UserList', 'calendar',
		    'cmd', 'dis', 'mailbox', 'profile', 'random', 'rfc822']:
	    if not (mod in sys.modules.keys()):
		got = mod
		break							# ==v
	if got:
	    testExec("not-yet loaded module: %s" % mod,
		     ['import ' + mod, 'modules.append(got)'],
		     vars(), newimp_globals)
	else:
	    testExec("not-yet loaded module: list exhausted, never mind",
		     [], vars(), newimp_globals)

	# Now some package stuff.

	# ------
	# Test 4
	# First change the path to include our temp dir, copying so the
	# addition can be revoked on cleanup in the finally, below:
	sys.path = ['/var/tmp'] + sys.path[:]
	# Now create a trivial package:
	stmts = ["hier1 = TestDirHier({'a.py': 'print \"a.py executing\"'})",
		 "hiers.append(hier1)",
		 "base = hier1.PKG_NM",
		 "exec 'import ' + base",
		 "testMods.append(base)"]
	testExec("trivial package, with one module, a.py",
		 stmts, vars(), newimp_globals)

	# ------
	# Test 5
	# Slightly less trivial package - reference to '__':
	stmts = [("hier2 = TestDirHier({'ref.py': 'print \"Pkg __:\", __'})"),
		 "base = hier2.PKG_NM",
		 "hiers.append(hier2)",
		 "exec 'import ' + base",
		 "testMods.append(base)"]
	testExec("trivial package, with module that has pkg shorthand ref",
		 stmts, vars(), newimp_globals)

	# ------
	# Test 6
	# Nested package, plus '__' references:

	complexTemplate = {'ref.py': 'print "ref.py loading..."',
			    'suite': {'s1.py': 'print "s1.py, in pkg:", __',
				      'subsuite': {'sub1.py':
						   'print "sub1.py"'}}}
	stmts = [('print """%s\n%s\n%s\n%s\n%s\n%s"""' %
		  ('.../',
		   '    ref.py\t\t\t"ref.py loading..."',
		   '    suite/',
		   '	    s1.py \t\t"s1.py, in pkg: xxxx.suite"',
		   '	    subsuite/',
		   '		sub1.py		"sub1.py" ')),
		 "hier3 = TestDirHier(complexTemplate)",
		 "base = hier3.PKG_NM",
		 "hiers.append(hier3)",
		 "exec 'import ' + base",
		 "testMods.append(base)"]
	testExec("Significantly nestled package:",
		 stmts, vars(), newimp_globals)

	# ------
	# Test 7
	# Try an elaborate hierarchy which includes an __init__ master in one
	# one portion, a ref across packages within the hierarchies, and an
	# indirect recursive import which cannot be satisfied (and hence,
	# prevents load of part of the hierarchy).
	complexTemplate = {'mid':
			   {'prime':
			    {'__init__.py': 'import __.easy, __.nother',
			     'easy.py': 'print "easy.py:", __name__',
			     'nother.py': ('%s\n%s\n%s\n' %
					   ('import __.easy',
					    'print "nother got __.easy"',
					    # __.__.awry should be found but
					    # should not load successfully,
					    # disrupting nother, but not easy
					    'import __.__.awry'))},
			    # continuing dict 'mid':
			    'awry':
			    {'__init__.py':
			     ('%s\n%s' %
			      ('print "got " + __name__',
			       'from __ import *')),
			     # This mutual recursion (b->a, a->d->b) should be
			     # ok, since a.py sets ax before recursing.
			     'a.py':	'ax = 1; from __.b import bx',
			     'b.py':	'bx = 1; from __.a import ax'}}}
	stmts = ["hier5 = TestDirHier(complexTemplate)",
		 "base = hier5.PKG_NM",
		 "testMods.append(base)",
		 "hiers.append(hier5)",
		 "exec 'import %s.mid.prime' % base",
		 "print eval(base)",		# Verify the base was bound
		 "testMods.append(base)"]
	testExec("Elaborate, clean hierarchy",
		 stmts, vars(), newimp_globals)

	# ------
	# test 8
	# Here we disrupt the mutual recursion in the mid.awry package, so the
	# import should now fail.
	complexTemplate['mid']['awry']['a.py'] = 'from __.b import bx; ax = 1'
	complexTemplate['mid']['awry']['b.py'] = 'from __.a import ax; bx = 1'
	stmts = ["hier6 = TestDirHier(complexTemplate)",
		 "base = hier6.PKG_NM",
		 "testMods.append(base)",
		 "hiers.append(hier6)",
		 "work = ('import %s.mid.prime' % base)",
		 ("try: exec work" +
		  "\nexcept ImportError: print ' -- import failed, as ought'" +
		  "\nelse: raise SystemError, sys.exc_value"),
		 "testMods.append(base)"]
	testExec("Elaborate hier w/ deliberately flawed import recursion",
		 stmts, vars(), newimp_globals)

	sys.exc_traceback = None	# Signify clean conclusion.

    finally:
	skipToTest = 0
	atTest = 1
	sys.path = wasPath
	VERBOSE = wasVerbose
	if wasImport:			# Resurrect prior routine
	    __builtin__.__import__ = wasImport
	else:
	    del __builtin__.__import__
	if leaveHiers:
	    print 'Cleanup inhibited'
	else:
	    if sys.exc_traceback:
		print ' ** Import test FAILURE... cleanup.'
	    else:
		print ' Import test SUCCESS... cleanup'
	    for h in hiers: h.remove(); del h	# Dispose of test directories

init()

if __name__ == '__main__':
    test()
