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

  - import an entire package as a unit, by importing the package directory.
    If there is a module named '__main__.py' in the package, it controls the
    load.  Otherwise, all the modules in the dir, including packages, are
    inherently loaded into the package module's namespace.

    __main__.py can load the entire directory, by loading the package
    itself, via eg 'import __', or even 'from __ import *'.  The benefit
    is (1) the ability to do additional things before and after the loads
    of the other modules, and (2) the ability to populate the package
    module with the *contents* of the component modules, ie with a
    'from __ import *'.)

  - import siblings from modules within a package, using '__.' as a shorthand
    prefix to refer to the parent package.  This enables referential
    transparency - package modules need not know their package name.

  - The '__' package references are actually names assigned within
    modules, to refer to their containing package.  This means that
    variable references can be made to imported modules, or variables
    defined via 'import ... from' of the modules, also using the '__.var'
    shorthand notation.  This establishes an proper equivalence between
    the import reference '__.sibling' and the var reference '__.sibling'. 

Modules have a few new attributes, in support of packages.  As
mentioned above, '__' is a shorthand attribute denoting the
modules' parent package, also denoted in the module by
'__package__'.  Additionally, modules have associated with them a
'__pkgpath__', a path by which sibling modules are found."""

__version__ = "$Revision$"

# $Id$
# First release:  Ken.Manheimer@nist.gov,  5-Apr-1995, for python 1.2

# Developers Notes:
#
# - 'sys.stub_modules' registers "incidental" (partially loaded) modules.
#   A stub module is promoted to the fully-loaded 'sys.modules' list when it is
#   explicitly loaded as a unit.
# - The __main__ loads of '__' have not yet been tested.
# - The test routines are cool, including a transient directory
#   hierarchy facility, and a means of skipping to later tests by giving
#   the test routine a numeric arg.
# - This could be substantially optimized, and there are many loose ends
#   lying around, since i wanted to get this released for 1.2.

VERBOSE = 0

import sys, string, regex, types, os, marshal, new, __main__
try:
    import imp				# Build on this recent addition
except ImportError:
    raise ImportError, 'Pkg import module depends on optional "imp" module'

from imp import SEARCH_ERROR, PY_SOURCE, PY_COMPILED, C_EXTENSION
PY_PACKAGE = 4				# In addition to above PY_*

modes = {SEARCH_ERROR: 'SEARCH_ERROR',
	 PY_SOURCE: 'PY_SOURCE',
	 PY_COMPILED: 'PY_COMPILED',
	 C_EXTENSION: 'C_EXTENSION',
	 PY_PACKAGE: 'PY_PACKAGE'}

# sys.stub_modules tracks modules partially loaded modules, ie loaded only
# incidental to load of nested components.

try: sys.stub_modules
except AttributeError:
    sys.stub_modules = {}

# Environment setup - "root" module, '__python__'

# Establish root package '__python__' in __main__ and newimp envs.

PKG_MAIN_NM = '__main__'		# 'pkg/__main__.py' master, if present.
PKG_NM = '__package__'			# Longhand for module's container.
PKG_SHORT_NM = '__'			# Shorthand for module's container.
PKG_SHORT_NM_LEN = len(PKG_SHORT_NM)
PKG_PATH = '__pkgpath__'		# Var holding package search path,
					# usually just the path of the pkg dir.
__python__ = __main__
sys.modules['__python__'] = __python__	# Register as an importable module.
__python__.__dict__[PKG_PATH] = sys.path

origImportFunc = None
def install():
    """Install newimp import_module() routine, for package support.

    newimp.revert() reverts to __import__ routine that was superceded."""
    global origImportFunc
    if not origImportFunc:
	try:
	    import __builtin__
	    origImportFunc = __builtin__.__import__
	except AttributeError:
	    pass
    __builtin__.__import__ = import_module
    print 'Enhanced import functionality installed.'
def revert():
    """Revert to original __builtin__.__import__ func, if newimp.install() has
    been executed."""
    if origImportFunc:
	import __builtin__
	__builtin__.__import__ = origImportFunc
	print 'Original import routine back in place.'

def import_module(name,
		  envLocals=None, envGlobals=None,
		  froms=None,
		  inPkg=None):
    """Primary service routine implementing 'import' with package nesting."""

    # The job is divided into a few distinct steps:
    #
    # - Look for either an already loaded module or a file to be loaded.
    #   * if neither loaded module nor prospect file is found, raise an error.
    #   - If we have a file, not an already loaded module:
    #     - Load the file into a module.
    #     - Register the new module and intermediate package stubs.
    # (We have a module at this point...)
    # - Bind requested syms (module or specified 'from' defs) in calling env.
    # - Return the appropriate component.

    note("import_module: seeking '%s'%s" %
	 (name, ((inPkg and ' (in package %s)' % inPkg.__name__) or '')))

    # We need callers environment dict for local path and resulting module
    # binding.
    if not (envLocals or envGlobals):
	envLocals, envGlobals = exterior()

    modList = theMod = absNm = container = None

    # Get module obj if one already established, or else module file if not:

    if inPkg:
	# We've been invoked with a specific containing package:
	pkg, pkgPath, pkgNm = inPkg, inPkg.__dict__[PKG_PATH], inPkg.__name__
	relNm = name
	absNm = pkgNm + '.' + name
	
    elif name[:PKG_SHORT_NM_LEN+1] != PKG_SHORT_NM + '.':
	# name is NOT '__.something' - setup to seek according to specified
	# absolute name.
	pkg = __python__
	pkgPath = sys.path
	absNm = name
	relNm = absNm

    else:
	# name IS '__.' + something - setup to seek according to relative name,
	# in current package.

	relNm = name[len(PKG_SHORT_NM)+1:]	# Relative portion of name.
	try:
	    pkg = envGlobals[PKG_NM]	# The immediately containing package.
	    pkgPath = pkg.__dict__[PKG_PATH]
	    if pkg == __python__:	# At outermost package.
		absNm = relNm
	    else:
		absNm = (pkg.__name__ + '.' + relNm)
	except KeyError:		# Missing package, path, or name.
	    note("Can't identify parent package, package name, or pkgpath")
	    pass							# ==v

    # Try to find existing module:
    if sys.modules.has_key(absNm):
	note('found ' + absNm + ' already imported')
	theMod = sys.modules[absNm]
    else:
	# Try for builtin or frozen first:
	theMod = imp.init_builtin(absNm)
	if theMod:
	    note('found builtin ' + absNm)
	else:
	    theMod = imp.init_frozen(absNm)
	    if theMod:
		note('found frozen ' + absNm)
	if not theMod:
	    if type(pkgPath) == types.StringType:
		pkgPath = [pkgPath]
	    modList = find_module(relNm, pkgPath, absNm)
	    if not modList:
		raise ImportError, "module '%s' not found" % absNm	# ===X
	    # We have a list of successively nested files leading to the
	    # module, register them as stubs:
	    container = register_module_nesting(modList, pkg)

	    # Load from file if necessary and possible:
	    modNm, modf, path, ty = modList[-1]
	    note('found type ' + modes[ty[2]] + ' - ' + absNm)

	    # Do the load:
	    theMod = load_module(absNm, ty[2], modf, inPkg)

	    # Loaded successfully - promote module to full module status:
	    register_module(theMod, theMod.__name__, pkgPath, pkg)

    # Have a loaded module, impose designated components, and return
    # appropriate thing - according to guido:
    # "Note that for "from spam.ham import bacon" your function should
    #  return the object denoted by 'spam.ham', while for "import
    #  spam.ham" it should return the object denoted by 'spam' -- the
    #  STORE instructions following the import statement expect it this
    #  way."
    if not froms:
	# Establish the module defs in the importing name space:
	(envLocals or envGlobals)[name] = theMod
	return (container or theMod)
    else:
	# Implement 'from': Populate immediate env with module defs:
	if froms == '*':
	    froms = theMod.__dict__.keys()	# resolve '*'
	for item in froms:
	    (envLocals or envGlobals)[item] = theMod.__dict__[item]
	return theMod

def unload(module):
    """Remove registration for a module, so import will do a fresh load."""
    if type(module) == types.ModuleType:
	module = module.__name__
    for m in [sys.modules, sys.stub_modules]:
	try:
	    del m[module]
	except KeyError:
	    pass

def find_module(name, path, absNm=''):
    """Locate module NAME on PATH.  PATH is pathname string or a list of them.

    Note that up-to-date compiled versions of a module are preferred to plain
    source, and compilation is automatically performed when necessary and
    possible.

    Returns a list of the tuples returned by 'find_module_file' (cf), one for
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

	    got = find_module_file(curPath, absNm)
	    if got:
		note('using %s' % got[2], 2)
		return [got]						# ===>

	else:

	    # Composite name specifying nested module:

	    gotList = []; nameAccume = expNm[0]

	    got = find_module_file(curPath, nameAccume)
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
		    break						# ==v^
		if nameAccume:
		    nameAccume = nameAccume + '.' + component
		else:
		    nameAccume = component
		got = find_module_file(fullPath, nameAccume)
		if got:
		    gotList.append(got)
		    # ** have to return the *full* name here:
		    nm, file, fullPath, ty = got
		else:
		    # Clear state vars:
		    gotList, got, nameAccume = [], None, ''
		    break						# ==v^
	    # Found nesting all the way to the specified tip:
	    if got:
		return gotList						# ===>

    # Failed.
    return None

def find_module_file(pathNm, modname):
    """Find module file given dir PATHNAME and module NAME.

    If successful, returns quadruple consisting of a mod name, file object,
    PATHNAME for the found file, and a description triple as contained in the
    list returned by get_suffixes.

    Otherwise, returns None.

    Note that up-to-date compiled versions of a module are preferred to plain
    source, and compilation is automatically performed, when necessary and
    possible."""

    relNm = string.splitfields(modname,'.')[-1]

    if pathNm[-1] != '/': pathNm = pathNm + '/'

    for suff, mode, ty in get_suffixes():
	note('trying ' + pathNm + relNm + suff + '...', 3)
	fullPath = pathNm + relNm + suff
	try:
	    modf = open(fullPath, mode)
	except IOError:
	    # ?? Skip unreadable ones.
	    continue							# ==^

	if ty == PY_PACKAGE:
	    # Enforce directory characteristic:
	    if not os.path.isdir(fullPath):
		note('Skipping non-dir match ' + fullPath)
		continue						# ==^
	    else:
		return (modname, modf, fullPath, (suff, mode, ty))	# ===>
	    

	elif ty == PY_SOURCE:
	    # Try for a compiled version:
	    note('found source ' + fullPath, 2)
	    pyc = fullPath + 'c'	# Sadly, we're presuming '.py' suff.
	    if (not os.path.exists(pyc) or
		(os.stat(fullPath)[8] > os.stat(pyc)[8])):
		# Try to compile:
		pyc = compile_source(fullPath, modf)
	    if pyc and (os.stat(fullPath)[8] < os.stat(pyc)[8]):
		# Either pyc was already newer or we just made it so; in either
		# case it's what we crave:
		return (modname, open(pyc, 'rb'), pyc,			# ===>
			('.pyc', 'rb', PY_COMPILED))
	    # Couldn't get a compiled version - return the source:
	    return (modname, modf, fullPath, (suff, mode, ty))		# ===>

	elif ty == PY_COMPILED:
	    # Make sure it is current, trying to compile if necessary, and
	    # prefer source failing that:
	    note('found compiled ' + fullPath, 2)
	    py = fullPath[:-1]		# Sadly again, presuming '.pyc' suff.
	    if not os.path.exists(py):
		note('found pyc sans py: ' + fullPath)
		return (modname, modf, fullPath, (suff, mode, ty))	# ===>
	    elif (os.stat(py)[8] > os.stat(fullPath)[8]):
		note('forced to try compiling: ' + py)
		pyc = compile_source(py, modf)
		if pyc:
		    return (modname, modf, fullPath, (suff, mode, ty))	# ===>
		else:
		    note('failed compile - must use more recent .py')
		    return (modname,					# ===>
			    open(py, 'r'), py, ('.py', 'r', PY_SOURCE))
	    else:
		return (modname, modf, fullPath, (suff, mode, ty))	# ===>

	elif ty == C_EXTENSION:
	    note('found extension ' + fullPath, 2)
	    return (modname, modf, fullPath, (suff, mode, ty))		# ===>

	else:
	    raise SystemError, 'Unanticipated (new?) module type encountered'

    return None


def load_module(name, ty, theFile, fromMod=None):
    """Load module NAME, type TYPE, from file FILE.

    Optional arg fromMod indicated the module from which the load is being done
    - necessary for detecting import of __ from a package's __main__ module.

    Return the populated module object."""

    # Note: we mint and register intermediate package directories, as necessary
    
    # Determine packagepath extension:

    # Establish the module object in question:
    theMod = procure_module(name)
    nameTail = string.splitfields(name, '.')[-1]
    thePath = theFile.name

    if ty == PY_SOURCE:
	exec_into(theFile, theMod, theFile.name)

    elif ty == PY_COMPILED:
	pyc = open(theFile.name, 'rb').read()
	if pyc[0:4] != imp.get_magic():
	    raise ImportError, 'bad magic number: ' + theFile.name	# ===>
	code = marshal.loads(pyc[8:])
	exec_into(code, theMod, theFile.name)

    elif ty == C_EXTENSION:
	try:
	    theMod = imp.load_dynamic(nameTail, thePath, theFile)
	except:
	    # ?? Ok to embellish the error message?
	    raise sys.exc_type, ('%s (from %s)' %
				 (str(sys.exc_value), theFile.name))

    elif ty == PY_PACKAGE:
	# Load constituents:
	if (os.path.exists(thePath + '/' + PKG_MAIN_NM) and
	    # pkg has a __main__, and this import not already from __main__, so
	    # __main__ can 'import __', or even better, 'from __ import *'
	    ((theMod.__name__ != PKG_MAIN_NM) and (fromMod.__ == theMod))):
	    exec_into(thePath + '/' + PKG_MAIN_NM, theMod, theFile.name)
	else:
	    # ... or else recursively load constituent modules.
	    prospects = mod_prospects(thePath)
	    for item in prospects:
		theMod.__dict__[item] = import_module(item,
						      theMod.__dict__,
						      theMod.__dict__,
						      None,
						      theMod)
		
    else:
	raise ImportError, 'Unimplemented import type: %s' % ty		# ===>
	
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
	# ?? Ok to embellish the error message?
	raise sys.exc_type, ('%s (from %s)' %
			     (str(sys.exc_value), path))
	

def mod_prospects(path):
    """Return a list of prospective modules within directory PATH.

    We actually return the distinct names resulting from stripping the dir
    entries (excluding '.' and '..') of their suffixes (as represented by
    'get_suffixes').

    (Note that matches for the PY_PACKAGE type with null suffix are
    implicitly constrained to be directories.)"""

    # We actually strip the longest matching suffixes, so eg 'dbmmodule.so'
    # mates with 'module.so' rather than '.so'.

    dirList = os.listdir(path)
    excludes = ['.', '..']
    sortedSuffs = sorted_suffixes()
    entries = []
    for item in dirList:
	if item in excludes: continue					# ==^
	for suff in sortedSuffs:
	    sub = -1 * len(suff)
	    if sub == 0:
		if os.path.isdir(os.path.join(path, item)):
		    entries.append(item)
	    elif item[sub:] == suff:
		it = item[:sub]
		if not it in entries:
		    entries.append(it)
		break							# ==v^
    return entries
		


def procure_module(name):
    """Return an established or else new module object having NAME.

    First checks sys.modules, then sys.stub_modules."""

    if sys.modules.has_key(name):
	it = sys.modules[name]
    elif sys.stub_modules.has_key(name):
	it = sys.stub_modules[name]
    else:
	it = new.module(name)
    return it								# ===>

def register_module_nesting(modList, pkg):
    """Given a find_module()-style NESTING and a parent PACKAGE, register
    components as stub modules."""
    container = None
    for stubModNm, stubModF, stubPath, stubTy in modList:
	relStubNm = string.splitfields(stubModNm, '.')[-1]
	if sys.modules.has_key(stubModNm):
	    # Nestle in containing package:
	    stubMod = sys.modules[stubModNm]
	    pkg.__dict__[relStubNm] = stubMod
	    pkg = stubMod	# will be parent for next in sequence.
	elif sys.stub_modules.has_key(stubModNm):
	    stubMod = sys.stub_modules[stubModNm]
	    pkg.__dict__[relStubNm] = stubMod
	    pkg = stubMod
	else:
	    stubMod = procure_module(stubModNm)
	    # Register as a stub:
	    register_module(stubMod, stubModNm, stubPath, pkg, 1)
	    pkg.__dict__[relStubNm] = stubMod
	    pkg = stubMod
	if not container:
	    container = stubMod
    return container

def register_module(theMod, name, path, package, stub=0):
    """Properly register MODULE, w/ name, path, package, opt, as stub."""
    
    if stub:
	sys.stub_modules[name] = theMod
    else:
	sys.modules[name] = theMod
	if sys.stub_modules.has_key(name):
	    del sys.stub_modules[name]
    theMod.__ = theMod.__dict__[PKG_NM] = package
    theMod.__dict__[PKG_PATH] = path


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
	note("write permission denied to " + compiledPath)
	return None
    mtime = os.stat(sourcePath)[8]
    sourceFile.seek(0)			# rewind
    try:
	compiledFile.write(imp.get_magic())		# compiled magic number
	compiledFile.seek(8, 0)				# mtime space holder
	# We let compilation errors go their own way...
	compiled = compile(sourceFile.read(), sourcePath, 'exec')
	marshal.dump(compiled, compiledFile)		# write the code obj
	compiledFile.seek(4, 0)				# position for mtime
	compiledFile.write(marshal.dumps(mtime)[1:])	# register mtime
	compiledFile.flush()
	compiledFile.close()
	return compiledPath
    except IOError:
	return None


def PathExtension(locals, globals):	# Probably obsolete.
    """Determine import search path extension vis-a-vis __pkgpath__ entries.

    local dict __pkgpath__ will preceed global dict __pkgpath__."""

    pathadd = []
    if globals and globals.has_key(PKG_PATH):
	pathadd = PrependPath(pathadd, globals[PKG_PATH], 'global')
    if locals and locals.has_key(PKG_PATH):
	pathadd = PrependPath(pathadd, locals[PKG_PATH], 'local')
    if pathadd:
	note(PKG_PATH + ' extension: ' + pathadd)
    return pathadd

def PrependPath(path, pre, whence):	# Probably obsolete
    """Return copy of PATH list with string or list-of-strings PRE prepended.

    If PRE is neither a string nor list-of-strings, print warning that
    locality WHENCE has malformed value."""

    # (There is probably a better way to handle malformed PREs, but raising an
    # error seems too severe - in that case, a bad setting for
    # sys.__pkgpath__ would prevent any imports!)

    if type(pre) == types.StringType: return [pre] + path[:]		# ===>
    elif type(pre) == types.ListType: return pre + path[:]		# ===>
    else:
	print "**Ignoring '%s' bad %s value**" % (whence, PKG_PATH)
	return path							# ===>

got_suffixes = None
def get_suffixes():
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
    
    global got_suffixes

    if got_suffixes:
	return got_suffixes
    else:
	# Ensure that the .pyc suffix precedes the .py:
	got_suffixes = [('', 'r', PY_PACKAGE)]
	py = pyc = None
	for suff in imp.get_suffixes():
	    if suff[0] == '.py':
		py = suff
	    elif suff[0] == '.pyc':
		pyc = suff
	    else:
		got_suffixes.append(suff)
	got_suffixes.append(pyc)
	got_suffixes.append(py)
	return got_suffixes
		

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

def note(msg, threshold=1):
    if VERBOSE >= threshold: print '(import:', msg, ')'

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
		raise ValueError, 'invalid file-value type, %s' % type(val)
    def remove(self, name=''):
	"""Dispose of the NAME (or keys in dictionary), using 'rm -r'."""
	name = os.path.join(self.root, name)
	sys.exc_traceback = None	# Ensure Discard of try/except obj ref
	if os.path.exists(name):
	    print '(TestDirHier: deleting %s)' % name
	    os.system('rm -r ' + name)
	else:
	    raise IOError, "can't remove non-existent " + name
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

def test(number=0):
    """Exercise import functionality, creating a transient dir hierarchy for
    the purpose.

    We actually install the new import functionality, temporarily, resuming the
    existing function on cleanup."""

    import __builtin__

    global skipToTest, atTest
    skipToTest = number
    hier = None

    def confPkgVars(mod, locals, globals):
	if not sys.modules.has_key(mod):
	    print 'import test: missing module "%s"' % mod
	else:
	    modMod = sys.modules[mod]
	    if not modMod.__dict__.has_key(PKG_SHORT_NM):
		print ('import test: module "%s" missing %s pkg shorthand' %
		       (mod, PKG_SHORT_NM))
	    if not modMod.__dict__.has_key(PKG_PATH):
		print ('import test: module "%s" missing %s package path' %
		       (mod, PKG_PATH))
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

    # First, get the globals and locals to pass to our testExec():
    exec 'import ' + __name__
    globals, locals = eval(__name__ + '.__dict__'), vars()

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
	wasVerbose, VERBOSE = VERBOSE, 2
	__builtin__.__import__ = import_module	# Install new version

	if testMods:		# Clear out imports from previous tests
	    for m in testMods[:]:
		unloadFull(m)
		testMods.remove(m)

	testExec("already imported module: %s" % sys.modules.keys()[0],
		 ['import ' + sys.modules.keys()[0]],
		 locals, globals)
	try:
	    no_sirree = 'no_sirree_does_not_exist'
	    testExec("non-existent module: %s" % no_sirree,
		     ['import ' + no_sirree],
		     locals, globals)
	except ImportError:
	    testExec("ok", ['pass'], locals, globals)
	got = None
	for mod in ['Complex', 'UserDict', 'UserList', 'calendar',
		    'cmd', 'dis', 'mailbox', 'profile', 'random', 'rfc822']:
	    if not (mod in sys.modules.keys()):
		got = mod
		break							# ==v
	if got:
	    testExec("not-yet loaded module: %s" % mod,
		     ['import ' + mod, 'modules.append(got)'],
		     locals, globals)
	else:
	    print "Import Test: couldn't find unimported module from list"

	# Now some package stuff.
	
	# First change the path to include our temp dir, copying so the
	# addition can be revoked on cleanup in the finally, below:
	sys.path = ['/var/tmp'] + sys.path[:]
	# Now create a trivial package:
	stmts = ["hier1 = TestDirHier({'a.py': 'print \"a.py executing\"'})",
		 "hiers.append(hier1)",
		 "root = hier1.PKG_NM",
		 "exec 'import ' + root",
		 "testMods.append(root)",
		 "confPkgVars(sys.modules[root].__name__, locals, globals)",
		 "confPkgVars(sys.modules[root].__name__+'.a',locals,globals)"]
	testExec("trivial package, with one module, a.py",
		 stmts, locals, globals)
	# Slightly less trivial package - reference to '__':
	stmts = [("hier2 = TestDirHier({'ref.py': 'print \"Pkg __:\", __'})"),
		 "root = hier2.PKG_NM",
		 "hiers.append(hier2)",
		 "exec 'import ' + root",
		 "testMods.append(root)"]
	testExec("trivial package, with module that has pkg shorthand ref",
		 stmts, locals, globals)
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
		 "root = hier3.PKG_NM",
		 "hiers.append(hier3)",
		 "exec 'import ' + root",
		 "testMods.append(root)"]
	testExec("Significantly nestled package:",
		 stmts, locals, globals)

	# Now try to do an embedded sibling import, using '__' shorthand -
	# alter our complexTemplate for a new dirHier:
	complexTemplate['suite']['s1.py'] = 'import __.subsuite'
	stmts = ["hier4 = TestDirHier(complexTemplate)",
		 "root = hier4.PKG_NM",
		 "testMods.append(root)",
		 "hiers.append(hier4)",
		 "exec 'import %s.suite.s1' % root",
		 "testMods.append(root)"]
	testExec("Similar structure, but suite/s1.py imports '__.subsuite'",
		 stmts, locals, globals)

	sys.exc_traceback = None	# Signify clean conclusion.

    finally:
	if sys.exc_traceback:
	    print ' ** Import test FAILURE... cleanup.'
	else:
	    print ' Import test SUCCESS... cleanup'
	VERBOSE = wasVerbose
	skipToTest = 0
	atTest = 1
	sys.path = wasPath
	for h in hiers: h.remove(); del h	# Dispose of test directories
	if wasImport:				# Resurrect prior routine
	    __builtin__.__import__ = wasImport
	else:
	    del __builtin__.__import__
