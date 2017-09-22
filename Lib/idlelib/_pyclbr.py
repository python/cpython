# A private copy of 3.7.0a1 pyclbr for use by idlelib.browser
"""Parse a Python module and describe its classes and functions.

Parse enough of a Python file to recognize imports and class and
function definitions, and to find out the superclasses of a class.

The interface consists of a single function:
    readmodule_ex(module, path=None)
where module is the name of a Python module, and path is an optional
list of directories where the module is to be searched.  If present,
path is prepended to the system search path sys.path.  The return value
is a dictionary.  The keys of the dictionary are the names of the
classes and functions defined in the module (including classes that are
defined via the from XXX import YYY construct).  The values are
instances of classes Class and Function.  One special key/value pair is
present for packages: the key '__path__' has a list as its value which
contains the package search path.

Classes and Functions have a common superclass: _Object.  Every instance
has the following attributes:
    module  -- name of the module;
    name    -- name of the object;
    file    -- file in which the object is defined;
    lineno  -- line in the file where the object's definition starts;
    parent  -- parent of this object, if any;
    children -- nested objects contained in this object.
The 'children' attribute is a dictionary mapping names to objects.

Instances of Function describe functions with the attributes from _Object.

Instances of Class describe classes with the attributes from _Object,
plus the following:
    super   -- list of super classes (Class instances if possible);
    methods -- mapping of method names to beginning line numbers.
If the name of a super class is not recognized, the corresponding
entry in the list of super classes is not a class instance but a
string giving the name of the super class.  Since import statements
are recognized and imported modules are scanned as well, this
shouldn't happen often.
"""

import io
import sys
import importlib.util
import tokenize
from token import NAME, DEDENT, OP

__all__ = ["readmodule", "readmodule_ex", "Class", "Function"]

_modules = {}  # Initialize cache of modules we've seen.


class _Object:
    "Informaton about Python class or function."
    def __init__(self, module, name, file, lineno, parent):
        self.module = module
        self.name = name
        self.file = file
        self.lineno = lineno
        self.parent = parent
        self.children = {}

    def _addchild(self, name, obj):
        self.children[name] = obj


class Function(_Object):
    "Information about a Python function, including methods."
    def __init__(self, module, name, file, lineno, parent=None):
        _Object.__init__(self, module, name, file, lineno, parent)


class Class(_Object):
    "Information about a Python class."
    def __init__(self, module, name, super, file, lineno, parent=None):
        _Object.__init__(self, module, name, file, lineno, parent)
        self.super = [] if super is None else super
        self.methods = {}

    def _addmethod(self, name, lineno):
        self.methods[name] = lineno


def _nest_function(ob, func_name, lineno):
    "Return a Function after nesting within ob."
    newfunc = Function(ob.module, func_name, ob.file, lineno, ob)
    ob._addchild(func_name, newfunc)
    if isinstance(ob, Class):
        ob._addmethod(func_name, lineno)
    return newfunc

def _nest_class(ob, class_name, lineno, super=None):
    "Return a Class after nesting within ob."
    newclass = Class(ob.module, class_name, super, ob.file, lineno, ob)
    ob._addchild(class_name, newclass)
    return newclass

def readmodule(module, path=None):
    """Return Class objects for the top-level classes in module.

    This is the original interface, before Functions were added.
    """

    res = {}
    for key, value in _readmodule(module, path or []).items():
        if isinstance(value, Class):
            res[key] = value
    return res

def readmodule_ex(module, path=None):
    """Return a dictionary with all functions and classes in module.

    Search for module in PATH + sys.path.
    If possible, include imported superclasses.
    Do this by reading source, without importing (and executing) it.
    """
    return _readmodule(module, path or [])

def _readmodule(module, path, inpackage=None):
    """Do the hard work for readmodule[_ex].

    If inpackage is given, it must be the dotted name of the package in
    which we are searching for a submodule, and then PATH must be the
    package search path; otherwise, we are searching for a top-level
    module, and path is combined with sys.path.
    """
    # Compute the full module name (prepending inpackage if set).
    if inpackage is not None:
        fullmodule = "%s.%s" % (inpackage, module)
    else:
        fullmodule = module

    # Check in the cache.
    if fullmodule in _modules:
        return _modules[fullmodule]

    # Initialize the dict for this module's contents.
    tree = {}

    # Check if it is a built-in module; we don't do much for these.
    if module in sys.builtin_module_names and inpackage is None:
        _modules[module] = tree
        return tree

    # Check for a dotted module name.
    i = module.rfind('.')
    if i >= 0:
        package = module[:i]
        submodule = module[i+1:]
        parent = _readmodule(package, path, inpackage)
        if inpackage is not None:
            package = "%s.%s" % (inpackage, package)
        if not '__path__' in parent:
            raise ImportError('No package named {}'.format(package))
        return _readmodule(submodule, parent['__path__'], package)

    # Search the path for the module.
    f = None
    if inpackage is not None:
        search_path = path
    else:
        search_path = path + sys.path
    spec = importlib.util._find_spec_from_path(fullmodule, search_path)
    _modules[fullmodule] = tree
    # Is module a package?
    if spec.submodule_search_locations is not None:
        tree['__path__'] = spec.submodule_search_locations
    try:
        source = spec.loader.get_source(fullmodule)
        if source is None:
            return tree
    except (AttributeError, ImportError):
        # If module is not Python source, we cannot do anything.
        return tree

    fname = spec.loader.get_filename(fullmodule)
    return _create_tree(fullmodule, path, fname, source, tree, inpackage)


def _create_tree(fullmodule, path, fname, source, tree, inpackage):
    """Return the tree for a particular module.

    fullmodule (full module name), inpackage+module, becomes o.module.
    path is passed to recursive calls of _readmodule.
    fname becomes o.file.
    source is tokenized.  Imports cause recursive calls to _readmodule.
    tree is {} or {'__path__': <submodule search locations>}.
    inpackage, None or string, is passed to recursive calls of _readmodule.

    The effect of recursive calls is mutation of global _modules.
    """
    f = io.StringIO(source)

    stack = [] # Initialize stack of (class, indent) pairs.

    g = tokenize.generate_tokens(f.readline)
    try:
        for tokentype, token, start, _end, _line in g:
            if tokentype == DEDENT:
                lineno, thisindent = start
                # Close previous nested classes and defs.
                while stack and stack[-1][1] >= thisindent:
                    del stack[-1]
            elif token == 'def':
                lineno, thisindent = start
                # Close previous nested classes and defs.
                while stack and stack[-1][1] >= thisindent:
                    del stack[-1]
                tokentype, func_name, start = next(g)[0:3]
                if tokentype != NAME:
                    continue  # Skip def with syntax error.
                cur_func = None
                if stack:
                    cur_obj = stack[-1][0]
                    cur_func = _nest_function(cur_obj, func_name, lineno)
                else:
                    # It is just a function.
                    cur_func = Function(fullmodule, func_name, fname, lineno)
                    tree[func_name] = cur_func
                stack.append((cur_func, thisindent))
            elif token == 'class':
                lineno, thisindent = start
                # Close previous nested classes and defs.
                while stack and stack[-1][1] >= thisindent:
                    del stack[-1]
                tokentype, class_name, start = next(g)[0:3]
                if tokentype != NAME:
                    continue # Skip class with syntax error.
                # Parse what follows the class name.
                tokentype, token, start = next(g)[0:3]
                inherit = None
                if token == '(':
                    names = [] # Initialize list of superclasses.
                    level = 1
                    super = [] # Tokens making up current superclass.
                    while True:
                        tokentype, token, start = next(g)[0:3]
                        if token in (')', ',') and level == 1:
                            n = "".join(super)
                            if n in tree:
                                # We know this super class.
                                n = tree[n]
                            else:
                                c = n.split('.')
                                if len(c) > 1:
                                    # Super class form is module.class:
                                    # look in module for class.
                                    m = c[-2]
                                    c = c[-1]
                                    if m in _modules:
                                        d = _modules[m]
                                        if c in d:
                                            n = d[c]
                            names.append(n)
                            super = []
                        if token == '(':
                            level += 1
                        elif token == ')':
                            level -= 1
                            if level == 0:
                                break
                        elif token == ',' and level == 1:
                            pass
                        # Only use NAME and OP (== dot) tokens for type name.
                        elif tokentype in (NAME, OP) and level == 1:
                            super.append(token)
                        # Expressions in the base list are not supported.
                    inherit = names
                if stack:
                    cur_obj = stack[-1][0]
                    cur_class = _nest_class(
                            cur_obj, class_name, lineno, inherit)
                else:
                    cur_class = Class(fullmodule, class_name, inherit,
                                      fname, lineno)
                    tree[class_name] = cur_class
                stack.append((cur_class, thisindent))
            elif token == 'import' and start[1] == 0:
                modules = _getnamelist(g)
                for mod, _mod2 in modules:
                    try:
                        # Recursively read the imported module.
                        if inpackage is None:
                            _readmodule(mod, path)
                        else:
                            try:
                                _readmodule(mod, path, inpackage)
                            except ImportError:
                                _readmodule(mod, [])
                    except:
                        # If we can't find or parse the imported module,
                        # too bad -- don't die here.
                        pass
            elif token == 'from' and start[1] == 0:
                mod, token = _getname(g)
                if not mod or token != "import":
                    continue
                names = _getnamelist(g)
                try:
                    # Recursively read the imported module.
                    d = _readmodule(mod, path, inpackage)
                except:
                    # If we can't find or parse the imported module,
                    # too bad -- don't die here.
                    continue
                # Add any classes that were defined in the imported module
                # to our name space if they were mentioned in the list.
                for n, n2 in names:
                    if n in d:
                        tree[n2 or n] = d[n]
                    elif n == '*':
                        # Don't add names that start with _.
                        for n in d:
                            if n[0] != '_':
                                tree[n] = d[n]
    except StopIteration:
        pass

    f.close()
    return tree


def _getnamelist(g):
    """Return list of (dotted-name, as-name or None) tuples for token source g.

    An as-name is the name that follows 'as' in an as clause.
    """
    names = []
    while True:
        name, token = _getname(g)
        if not name:
            break
        if token == 'as':
            name2, token = _getname(g)
        else:
            name2 = None
        names.append((name, name2))
        while token != "," and "\n" not in token:
            token = next(g)[1]
        if token != ",":
            break
    return names


def _getname(g):
    "Return (dotted-name or None, next-token) tuple for token source g."
    parts = []
    tokentype, token = next(g)[0:2]
    if tokentype != NAME and token != '*':
        return (None, token)
    parts.append(token)
    while True:
        tokentype, token = next(g)[0:2]
        if token != '.':
            break
        tokentype, token = next(g)[0:2]
        if tokentype != NAME:
            break
        parts.append(token)
    return (".".join(parts), token)


def _main():
    "Print module output (default this file) for quick visual check."
    import os
    try:
        mod = sys.argv[1]
    except:
        mod = __file__
    if os.path.exists(mod):
        path = [os.path.dirname(mod)]
        mod = os.path.basename(mod)
        if mod.lower().endswith(".py"):
            mod = mod[:-3]
    else:
        path = []
    tree = readmodule_ex(mod, path)
    lineno_key = lambda a: getattr(a, 'lineno', 0)
    objs = sorted(tree.values(), key=lineno_key, reverse=True)
    indent_level = 2
    while objs:
        obj = objs.pop()
        if isinstance(obj, list):
            # Value is a __path__ key.
            continue
        if not hasattr(obj, 'indent'):
            obj.indent = 0

        if isinstance(obj, _Object):
            new_objs = sorted(obj.children.values(),
                              key=lineno_key, reverse=True)
            for ob in new_objs:
                ob.indent = obj.indent + indent_level
            objs.extend(new_objs)
        if isinstance(obj, Class):
            print("{}class {} {} {}"
                  .format(' ' * obj.indent, obj.name, obj.super, obj.lineno))
        elif isinstance(obj, Function):
            print("{}def {} {}".format(' ' * obj.indent, obj.name, obj.lineno))

if __name__ == "__main__":
    _main()
