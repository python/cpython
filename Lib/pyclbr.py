"""Parse a Python file and retrieve classes and methods.

Parse enough of a Python file to recognize class and method
definitions and to find out the superclasses of a class.

The interface consists of a single function:
        readmodule_ex(module [, path[, inpackage]])
module is the name of a Python module, path is an optional list of
directories where the module is to be searched.  If present, path is
prepended to the system search path sys.path.  (inpackage is used
internally to search for a submodule of a package.)
The return value is a dictionary.  The keys of the dictionary are
the names of the classes defined in the module (including classes
that are defined via the from XXX import YYY construct).  The values
are class instances of the class Class defined here.

A class is described by the class Class in this module.  Instances
of this class have the following instance variables:
        name -- the name of the class
        super -- a list of super classes (Class instances)
        methods -- a dictionary of methods
        file -- the file in which the class was defined
        lineno -- the line in the file on which the class statement occurred
The dictionary of methods uses the method names as keys and the line
numbers on which the method was defined as values.
If the name of a super class is not recognized, the corresponding
entry in the list of super classes is not a class instance but a
string giving the name of the super class.  Since import statements
are recognized and imported modules are scanned as well, this
shouldn't happen often.

XXX describe the Function class.

BUGS
- Nested classes and functions can confuse it.

PACKAGE RELATED BUGS
- If you have a package and a module inside that or another package
  with the same name, module caching doesn't work properly since the
  key is the base name of the module/package.
- The only entry that is returned when you readmodule a package is a
  __path__ whose value is a list which confuses certain class browsers.
- When code does:
  from package import subpackage
  class MyClass(subpackage.SuperClass):
    ...
  It can't locate the parent.  It probably needs to have the same
  hairy logic that the import locator already does.  (This logic
  exists coded in Python in the freeze package.)
"""

import sys
import imp
import tokenize # Python tokenizer
from token import NAME

__all__ = ["readmodule"]

_modules = {}                           # cache of modules we've seen

# each Python class is represented by an instance of this class
class Class:
    '''Class to represent a Python class.'''
    def __init__(self, module, name, super, file, lineno):
        self.module = module
        self.name = name
        if super is None:
            super = []
        self.super = super
        self.methods = {}
        self.file = file
        self.lineno = lineno

    def _addmethod(self, name, lineno):
        self.methods[name] = lineno

class Function(Class):
    '''Class to represent a top-level Python function'''
    def __init__(self, module, name, file, lineno):
        Class.__init__(self, module, name, None, file, lineno)
    def _addmethod(self, name, lineno):
        assert 0, "Function._addmethod() shouldn't be called"

def readmodule(module, path=[], inpackage=False):
    '''Backwards compatible interface.

    Like readmodule_ex() but strips Function objects from the
    resulting dictionary.'''

    dict = readmodule_ex(module, path, inpackage)
    res = {}
    for key, value in dict.items():
        if not isinstance(value, Function):
            res[key] = value
    return res

def readmodule_ex(module, path=[], inpackage=False):
    '''Read a module file and return a dictionary of classes.

    Search for MODULE in PATH and sys.path, read and parse the
    module and return a dictionary with one entry for each class
    found in the module.'''

    dict = {}

    i = module.rfind('.')
    if i >= 0:
        # Dotted module name
        package = module[:i].strip()
        submodule = module[i+1:].strip()
        parent = readmodule_ex(package, path, inpackage)
        child = readmodule_ex(submodule, parent['__path__'], True)
        return child

    if module in _modules:
        # we've seen this module before...
        return _modules[module]
    if module in sys.builtin_module_names:
        # this is a built-in module
        _modules[module] = dict
        return dict

    # search the path for the module
    f = None
    if inpackage:
        try:
            f, file, (suff, mode, type) = \
                    imp.find_module(module, path)
        except ImportError:
            f = None
    if f is None:
        fullpath = list(path) + sys.path
        f, file, (suff, mode, type) = imp.find_module(module, fullpath)
    if type == imp.PKG_DIRECTORY:
        dict['__path__'] = [file]
        _modules[module] = dict
        path = [file] + path
        f, file, (suff, mode, type) = \
                        imp.find_module('__init__', [file])
    if type != imp.PY_SOURCE:
        # not Python source, can't do anything with this module
        f.close()
        _modules[module] = dict
        return dict

    _modules[module] = dict
    classstack = [] # stack of (class, indent) pairs

    g = tokenize.generate_tokens(f.readline)
    try:
        for tokentype, token, start, end, line in g:
            if token == 'def':
                lineno, thisindent = start
                tokentype, meth_name, start, end, line = g.next()
                if tokentype != NAME:
                    continue # Syntax error
                # close all classes indented at least as much
                while classstack and \
                      classstack[-1][1] >= thisindent:
                    del classstack[-1]
                if classstack:
                    # it's a class method
                    cur_class = classstack[-1][0]
                    cur_class._addmethod(meth_name, lineno)
                else:
                    # it's a function
                    dict[meth_name] = Function(module, meth_name, file, lineno)
            elif token == 'class':
                lineno, thisindent = start
                tokentype, class_name, start, end, line = g.next()
                if tokentype != NAME:
                    continue # Syntax error
                # close all classes indented at least as much
                while classstack and \
                      classstack[-1][1] >= thisindent:
                    del classstack[-1]
                # parse what follows the class name
                tokentype, token, start, end, line = g.next()
                inherit = None
                if token == '(':
                    names = [] # List of superclasses
                    # there's a list of superclasses
                    level = 1
                    super = [] # Tokens making up current superclass
                    while True:
                        tokentype, token, start, end, line = g.next()
                        if token in (')', ',') and level == 1:
                            n = "".join(super)
                            if n in dict:
                                # we know this super class
                                n = dict[n]
                            else:
                                c = n.split('.')
                                if len(c) > 1:
                                    # super class is of the form
                                    # module.class: look in module for
                                    # class
                                    m = c[-2]
                                    c = c[-1]
                                    if m in _modules:
                                        d = _modules[m]
                                        if c in d:
                                            n = d[c]
                            names.append(n)
                        if token == '(':
                            level += 1
                        elif token == ')':
                            level -= 1
                            if level == 0:
                                break
                        elif token == ',' and level == 1:
                            pass
                        else:
                            super.append(token)
                    inherit = names
                cur_class = Class(module, class_name, inherit, file, lineno)
                dict[class_name] = cur_class
                classstack.append((cur_class, thisindent))
            elif token == 'import' and start[1] == 0:
                modules = _getnamelist(g)
                for mod, mod2 in modules:
                    readmodule_ex(mod, path, inpackage)
            elif token == 'from' and start[1] == 0:
                mod, token = _getname(g)
                if not mod or token != "import":
                    continue
                names = _getnamelist(g)
                try:
                    # recursively read the imported module
                    d = readmodule_ex(mod, path, inpackage)
                except:
                    continue
                # add any classes that were defined in the imported module
                # to our name space if they were mentioned in the list
                for n, n2 in names:
                    if n in d:
                        dict[n2 or n] = d[n]
                    elif n == '*':
                        # only add a name if not already there (to mimic
                        # what Python does internally) also don't add
                        # names that start with _
                        for n in d:
                            if n[0] != '_' and not n in dict:
                                dict[n] = d[n]
    except StopIteration:
        pass

    f.close()
    return dict

def _getnamelist(g):
    # Helper to get a comma-separated list of dotted names plus 'as'
    # clauses.  Return a list of pairs (name, name2) where name2 is
    # the 'as' name, or None if there is no 'as' clause.
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
            tokentype, token, start, end, line = g.next()
        if token != ",":
            break
    return names

def _getname(g):
    # Helper to get a dotted name, return a pair (name, token) where
    # name is the dotted name, or None if there was no dotted name,
    # and token is the next input token.
    parts = []
    tokentype, token, start, end, line = g.next()
    if tokentype != NAME and token != '*':
        return (None, token)
    parts.append(token)
    while True:
        tokentype, token, start, end, line = g.next()
        if token != '.':
            break
        tokentype, token, start, end, line = g.next()
        if tokentype != NAME:
            break
        parts.append(token)
    return (".".join(parts), token)
