source = '''# line 1
'A module docstring.'

import sys, inspect
# line 5

# line 7
def spam(a, b, c, d=3, (e, (f,))=(4, (5,)), *g, **h):
    eggs(b + d, c + f)

# line 11
def eggs(x, y):
    "A docstring."
    global fr, st
    fr = inspect.currentframe()
    st = inspect.stack()
    p = x
    q = y / 0

# line 20
class StupidGit:
    """A longer,

    indented

    docstring."""
# line 27

    def abuse(self, a, b, c):
        """Another

\tdocstring

        containing

\ttabs
\t
        """
        self.argue(a, b, c)
# line 40
    def argue(self, a, b, c):
        try:
            spam(a, b, c)
        except:
            self.ex = sys.exc_info()
            self.tr = inspect.trace()

# line 48
class MalodorousPervert(StupidGit):
    pass

class ParrotDroppings:
    pass

class FesteringGob(MalodorousPervert, ParrotDroppings):
    pass
'''

# Functions tested in this suite:
# ismodule, isclass, ismethod, isfunction, istraceback, isframe, iscode,
# isbuiltin, isroutine, getmembers, getdoc, getfile, getmodule,
# getsourcefile, getcomments, getsource, getclasstree, getargspec,
# getargvalues, formatargspec, formatargvalues, currentframe, stack, trace

from test_support import TestFailed, TESTFN
import sys, imp, os, string

def test(assertion, message, *args):
    if not assertion:
        raise TestFailed, message % args

import inspect

file = open(TESTFN, 'w')
file.write(source)
file.close()

# Note that load_source creates file TESTFN+'c' or TESTFN+'o'.
mod = imp.load_source('testmod', TESTFN)
files_to_clean_up = [TESTFN, TESTFN + 'c', TESTFN + 'o']

def istest(func, exp):
    obj = eval(exp)
    test(func(obj), '%s(%s)' % (func.__name__, exp))
    for other in [inspect.isbuiltin, inspect.isclass, inspect.iscode,
                  inspect.isframe, inspect.isfunction, inspect.ismethod,
                  inspect.ismodule, inspect.istraceback]:
        if other is not func:
            test(not other(obj), 'not %s(%s)' % (other.__name__, exp))

git = mod.StupidGit()
try:
    1/0
except:
    tb = sys.exc_traceback

istest(inspect.isbuiltin, 'sys.exit')
istest(inspect.isbuiltin, '[].append')
istest(inspect.isclass, 'mod.StupidGit')
istest(inspect.iscode, 'mod.spam.func_code')
istest(inspect.isframe, 'tb.tb_frame')
istest(inspect.isfunction, 'mod.spam')
istest(inspect.ismethod, 'mod.StupidGit.abuse')
istest(inspect.ismethod, 'git.argue')
istest(inspect.ismodule, 'mod')
istest(inspect.istraceback, 'tb')
test(inspect.isroutine(mod.spam), 'isroutine(mod.spam)')
test(inspect.isroutine([].count), 'isroutine([].count)')

classes = inspect.getmembers(mod, inspect.isclass)
test(classes ==
     [('FesteringGob', mod.FesteringGob),
      ('MalodorousPervert', mod.MalodorousPervert),
      ('ParrotDroppings', mod.ParrotDroppings),
      ('StupidGit', mod.StupidGit)], 'class list')
tree = inspect.getclasstree(map(lambda x: x[1], classes), 1)
test(tree ==
     [(mod.ParrotDroppings, ()),
      (mod.StupidGit, ()),
      [(mod.MalodorousPervert, (mod.StupidGit,)),
       [(mod.FesteringGob, (mod.MalodorousPervert, mod.ParrotDroppings))
       ]
      ]
     ], 'class tree')

functions = inspect.getmembers(mod, inspect.isfunction)
test(functions == [('eggs', mod.eggs), ('spam', mod.spam)], 'function list')

test(inspect.getdoc(mod) == 'A module docstring.', 'getdoc(mod)')
test(inspect.getcomments(mod) == '# line 1\n', 'getcomments(mod)')
test(inspect.getmodule(mod.StupidGit) == mod, 'getmodule(mod.StupidGit)')
test(inspect.getfile(mod.StupidGit) == TESTFN, 'getfile(mod.StupidGit)')
test(inspect.getsourcefile(mod.spam) == TESTFN, 'getsourcefile(mod.spam)')
test(inspect.getsourcefile(git.abuse) == TESTFN, 'getsourcefile(git.abuse)')

def sourcerange(top, bottom):
    lines = string.split(source, '\n')
    return string.join(lines[top-1:bottom], '\n') + '\n'

test(inspect.getsource(git.abuse) == sourcerange(29, 39),
     'getsource(git.abuse)')
test(inspect.getsource(mod.StupidGit) == sourcerange(21, 46),
     'getsource(mod.StupidGit)')
test(inspect.getdoc(mod.StupidGit) ==
     'A longer,\n\nindented\n\ndocstring.', 'getdoc(mod.StupidGit)')
test(inspect.getdoc(git.abuse) ==
     'Another\n\ndocstring\n\ncontaining\n\ntabs\n\n', 'getdoc(git.abuse)')
test(inspect.getcomments(mod.StupidGit) == '# line 20\n',
     'getcomments(mod.StupidGit)')

args, varargs, varkw, defaults = inspect.getargspec(mod.eggs)
test(args == ['x', 'y'], 'mod.eggs args')
test(varargs == None, 'mod.eggs varargs')
test(varkw == None, 'mod.eggs varkw')
test(defaults == None, 'mod.eggs defaults')
test(inspect.formatargspec(args, varargs, varkw, defaults) ==
     '(x, y)', 'mod.eggs formatted argspec')
args, varargs, varkw, defaults = inspect.getargspec(mod.spam)
test(args == ['a', 'b', 'c', 'd', ['e', ['f']]], 'mod.spam args')
test(varargs == 'g', 'mod.spam varargs')
test(varkw == 'h', 'mod.spam varkw')
test(defaults == (3, (4, (5,))), 'mod.spam defaults')
test(inspect.formatargspec(args, varargs, varkw, defaults) ==
     '(a, b, c, d=3, (e, (f,))=(4, (5,)), *g, **h)',
     'mod.spam formatted argspec')

git.abuse(7, 8, 9)

istest(inspect.istraceback, 'git.ex[2]')
istest(inspect.isframe, 'mod.fr')

test(len(git.tr) == 3, 'trace() length')
test(git.tr[0][1:] == (TESTFN, 46, 'argue',
                       ['            self.tr = inspect.trace()\n'], 0),
     'trace() row 2')
test(git.tr[1][1:] == (TESTFN, 9, 'spam', ['    eggs(b + d, c + f)\n'], 0),
     'trace() row 2')
test(git.tr[2][1:] == (TESTFN, 18, 'eggs', ['    q = y / 0\n'], 0),
     'trace() row 3')

test(len(mod.st) >= 5, 'stack() length')
test(mod.st[0][1:] ==
     (TESTFN, 16, 'eggs', ['    st = inspect.stack()\n'], 0),
     'stack() row 1')
test(mod.st[1][1:] ==
     (TESTFN, 9, 'spam', ['    eggs(b + d, c + f)\n'], 0),
     'stack() row 2')
test(mod.st[2][1:] ==
     (TESTFN, 43, 'argue', ['            spam(a, b, c)\n'], 0),
     'stack() row 3')
test(mod.st[3][1:] ==
     (TESTFN, 39, 'abuse', ['        self.argue(a, b, c)\n'], 0),
     'stack() row 4')

args, varargs, varkw, locals = inspect.getargvalues(mod.fr)
test(args == ['x', 'y'], 'mod.fr args')
test(varargs == None, 'mod.fr varargs')
test(varkw == None, 'mod.fr varkw')
test(locals == {'x': 11, 'p': 11, 'y': 14}, 'mod.fr locals')
test(inspect.formatargvalues(args, varargs, varkw, locals) ==
     '(x=11, y=14)', 'mod.fr formatted argvalues')

args, varargs, varkw, locals = inspect.getargvalues(mod.fr.f_back)
test(args == ['a', 'b', 'c', 'd', ['e', ['f']]], 'mod.fr.f_back args')
test(varargs == 'g', 'mod.fr.f_back varargs')
test(varkw == 'h', 'mod.fr.f_back varkw')
test(inspect.formatargvalues(args, varargs, varkw, locals) ==
     '(a=7, b=8, c=9, d=3, (e=4, (f=5,)), *g=(), **h={})',
     'mod.fr.f_back formatted argvalues')

for fname in files_to_clean_up:
    try:
        os.unlink(fname)
    except:
        pass

# Test classic-class method resolution order.
class A:    pass
class B(A): pass
class C(A): pass
class D(B, C): pass

expected = (D, B, A, C)
got = inspect.getmro(D)
test(expected == got, "expected %r mro, got %r", expected, got)

# The same w/ new-class MRO.
class A(object):    pass
class B(A): pass
class C(A): pass
class D(B, C): pass

expected = (D, B, C, A, object)
got = inspect.getmro(D)
test(expected == got, "expected %r mro, got %r", expected, got)

# Test classify_class_attrs.
def attrs_wo_objs(cls):
    return [t[:3] for t in inspect.classify_class_attrs(cls)]

class A:
    def s(): pass
    s = staticmethod(s)

    def c(cls): pass
    c = classmethod(c)

    def getp(self): pass
    p = property(getp)

    def m(self): pass

    def m1(self): pass

    datablob = '1'

attrs = attrs_wo_objs(A)
test(('s', 'static method', A) in attrs, 'missing static method')
test(('c', 'class method', A) in attrs, 'missing class method')
test(('p', 'property', A) in attrs, 'missing property')
test(('m', 'method', A) in attrs, 'missing plain method')
test(('m1', 'method', A) in attrs, 'missing plain method')
test(('datablob', 'data', A) in attrs, 'missing data')

class B(A):
    def m(self): pass

attrs = attrs_wo_objs(B)
test(('s', 'static method', A) in attrs, 'missing static method')
test(('c', 'class method', A) in attrs, 'missing class method')
test(('p', 'property', A) in attrs, 'missing property')
test(('m', 'method', B) in attrs, 'missing plain method')
test(('m1', 'method', A) in attrs, 'missing plain method')
test(('datablob', 'data', A) in attrs, 'missing data')


class C(A):
    def m(self): pass
    def c(self): pass

attrs = attrs_wo_objs(C)
test(('s', 'static method', A) in attrs, 'missing static method')
test(('c', 'method', C) in attrs, 'missing plain method')
test(('p', 'property', A) in attrs, 'missing property')
test(('m', 'method', C) in attrs, 'missing plain method')
test(('m1', 'method', A) in attrs, 'missing plain method')
test(('datablob', 'data', A) in attrs, 'missing data')

class D(B, C):
    def m1(self): pass

attrs = attrs_wo_objs(D)
test(('s', 'static method', A) in attrs, 'missing static method')
test(('c', 'class method', A) in attrs, 'missing class method')
test(('p', 'property', A) in attrs, 'missing property')
test(('m', 'method', B) in attrs, 'missing plain method')
test(('m1', 'method', D) in attrs, 'missing plain method')
test(('datablob', 'data', A) in attrs, 'missing data')

# Repeat all that, but w/ new-style classes.

class A(object):

    def s(): pass
    s = staticmethod(s)

    def c(cls): pass
    c = classmethod(c)

    def getp(self): pass
    p = property(getp)

    def m(self): pass

    def m1(self): pass

    datablob = '1'

attrs = attrs_wo_objs(A)
test(('s', 'static method', A) in attrs, 'missing static method')
test(('c', 'class method', A) in attrs, 'missing class method')
test(('p', 'property', A) in attrs, 'missing property')
test(('m', 'method', A) in attrs, 'missing plain method')
test(('m1', 'method', A) in attrs, 'missing plain method')
test(('datablob', 'data', A) in attrs, 'missing data')

class B(A):

    def m(self): pass

attrs = attrs_wo_objs(B)
test(('s', 'static method', A) in attrs, 'missing static method')
test(('c', 'class method', A) in attrs, 'missing class method')
test(('p', 'property', A) in attrs, 'missing property')
test(('m', 'method', B) in attrs, 'missing plain method')
test(('m1', 'method', A) in attrs, 'missing plain method')
test(('datablob', 'data', A) in attrs, 'missing data')


class C(A):

    def m(self): pass
    def c(self): pass

attrs = attrs_wo_objs(C)
test(('s', 'static method', A) in attrs, 'missing static method')
test(('c', 'method', C) in attrs, 'missing plain method')
test(('p', 'property', A) in attrs, 'missing property')
test(('m', 'method', C) in attrs, 'missing plain method')
test(('m1', 'method', A) in attrs, 'missing plain method')
test(('datablob', 'data', A) in attrs, 'missing data')

class D(B, C):

    def m1(self): pass

attrs = attrs_wo_objs(D)
test(('s', 'static method', A) in attrs, 'missing static method')
test(('c', 'method', C) in attrs, 'missing plain method')
test(('p', 'property', A) in attrs, 'missing property')
test(('m', 'method', B) in attrs, 'missing plain method')
test(('m1', 'method', D) in attrs, 'missing plain method')
test(('datablob', 'data', A) in attrs, 'missing data')
