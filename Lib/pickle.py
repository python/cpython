#     $Id$
#
#     Copyright 
#
#       Copyright 1996 Digital Creations, L.C., 910 Princess Anne
#       Street, Suite 300, Fredericksburg, Virginia 22401 U.S.A. All
#       rights reserved.  Copyright in this software is owned by DCLC,
#       unless otherwise indicated. Permission to use, copy and
#       distribute this software is hereby granted, provided that the
#       above copyright notice appear in all copies and that both that
#       copyright notice and this permission notice appear. Note that
#       any product, process or technology described in this software
#       may be the subject of other Intellectual Property rights
#       reserved by Digital Creations, L.C. and are not licensed
#       hereunder.
#
#     Trademarks 
#
#       Digital Creations & DCLC, are trademarks of Digital Creations, L.C..
#       All other trademarks are owned by their respective companies. 
#
#     No Warranty 
#
#       The software is provided "as is" without warranty of any kind,
#       either express or implied, including, but not limited to, the
#       implied warranties of merchantability, fitness for a particular
#       purpose, or non-infringement. This software could include
#       technical inaccuracies or typographical errors. Changes are
#       periodically made to the software; these changes will be
#       incorporated in new editions of the software. DCLC may make
#       improvements and/or changes in this software at any time
#       without notice.
#
#     Limitation Of Liability 
#
#       In no event will DCLC be liable for direct, indirect, special,
#       incidental, economic, cover, or consequential damages arising
#       out of the use of or inability to use this software even if
#       advised of the possibility of such damages. Some states do not
#       allow the exclusion or limitation of implied warranties or
#       limitation of liability for incidental or consequential
#       damages, so the above limitation or exclusion may not apply to
#       you.
#  
#
# If you have questions regarding this software,
# contact:
#
#   Jim Fulton, jim@digicool.com
#
#   (540) 371-6909
#

"""\
Pickling Algorithm
------------------

This module implements a basic but powerful algorithm for "pickling" (a.k.a.
serializing, marshalling or flattening) nearly arbitrary Python objects.
This is a more primitive notion than persistency -- although pickle
reads and writes file objects, it does not handle the issue of naming
persistent objects, nor the (even more complicated) area of concurrent
access to persistent objects.  The pickle module can transform a complex
object into a byte stream and it can transform the byte stream into
an object with the same internal structure.  The most obvious thing to
do with these byte streams is to write them onto a file, but it is also
conceivable to send them across a network or store them in a database.

Unlike the built-in marshal module, pickle handles the following correctly:

- recursive objects
- pointer sharing
- classes and class instances

Pickle is Python-specific.  This has the advantage that there are no
restrictions imposed by external standards such as CORBA (which probably
can't represent pointer sharing or recursive objects); however it means
that non-Python programs may not be able to reconstruct pickled Python
objects.

Pickle uses a printable ASCII representation.  This is slightly more
voluminous than a binary representation.  However, small integers actually
take *less* space when represented as minimal-size decimal strings than
when represented as 32-bit binary numbers, and strings are only much longer
if they contain control characters or 8-bit characters.  The big advantage
of using printable ASCII (and of some other characteristics of pickle's
representation) is that for debugging or recovery purposes it is possible
for a human to read the pickled file with a standard text editor.  (I could
have gone a step further and used a notation like S-expressions, but the
parser would have been considerably more complicated and slower, and the
files would probably have become much larger.)

Pickle doesn't handle code objects, which marshal does.
I suppose pickle could, and maybe it should, but there's probably no
great need for it right now (as long as marshal continues to be used
for reading and writing code objects), and at least this avoids
the possibility of smuggling Trojan horses into a program.

For the benefit of persistency modules written using pickle, it supports
the notion of a reference to an object outside the pickled data stream.
Such objects are referenced by a name, which is an arbitrary string of
printable ASCII characters.  The resolution of such names is not defined
by the pickle module -- the persistent object module will have to implement
a method "persistent_load".  To write references to persistent objects,
the persistent module must define a method "persistent_id" which returns
either None or the persistent ID of the object.

There are some restrictions on the pickling of class instances.

First of all, the class must be defined at the top level in a module.

Next, it must normally be possible to create class instances by
calling the class without arguments.  Usually, this is best
accomplished by providing default values for all arguments to its
__init__ method (if it has one).  If this is undesirable, the
class can define a method __getinitargs__, which should return a
*tuple* containing the arguments to be passed to the class
constructor.

Classes can influence how their instances are pickled -- if the class defines
the method __getstate__, it is called and the return state is pickled
as the contents for the instance, and if the class defines the
method __setstate__, it is called with the unpickled state.  (Note
that these methods can also be used to implement copying class instances.)
If there is no __getstate__ method, the instance's __dict__
is pickled.  If there is no __setstate__ method, the pickled object
must be a dictionary and its items are assigned to the new instance's
dictionary.  (If a class defines both __getstate__ and __setstate__,
the state object needn't be a dictionary -- these methods can do what they
want.)

Note that when class instances are pickled, their class's code and data
is not pickled along with them.  Only the instance data is pickled.
This is done on purpose, so you can fix bugs in a class or add methods and
still load objects that were created with an earlier version of the
class.  If you plan to have long-lived objects that will see many versions
of a class, it may be worth to put a version number in the objects so
that suitable conversions can be made by the class's __setstate__ method.

The interface is as follows:

To pickle an object x onto a file f, open for writing:

    p = pickle.Pickler(f)
    p.dump(x)

To unpickle an object x from a file f, open for reading:

    u = pickle.Unpickler(f)
    x = u.load()

The Pickler class only calls the method f.write with a string argument
(XXX possibly the interface should pass f.write instead of f).
The Unpickler calls the methods f.read(with an integer argument)
and f.readline(without argument), both returning a string.
It is explicitly allowed to pass non-file objects here, as long as they
have the right methods.

The following types can be pickled:

- None
- integers, long integers, floating point numbers
- strings
- tuples, lists and dictionaries containing only picklable objects
- class instances whose __dict__ or __setstate__() is picklable
- classes

Attempts to pickle unpicklable objects will raise an exception
after having written an unspecified number of bytes to the file argument.

It is possible to make multiple calls to Pickler.dump() or to
Unpickler.load(), as long as there is a one-to-one correspondence
between pickler and Unpickler objects and between dump and load calls
for any pair of corresponding Pickler and Unpicklers.  WARNING: this
is intended for pickleing multiple objects without intervening modifications
to the objects or their parts.  If you modify an object and then pickle
it again using the same Pickler instance, the object is not pickled
again -- a reference to it is pickled and the Unpickler will return
the old value, not the modified one.  (XXX There are two problems here:
(a) detecting changes, and (b) marshalling a minimal set of changes.
I have no answers.  Garbage Collection may also become a problem here.)
"""

__version__ = "1.7"                     # Code version

from types import *
from copy_reg import *
import string, marshal

format_version = "1.2"                  # File format version we write
compatible_formats = ["1.0", "1.1"]     # Old format versions we can read

mdumps = marshal.dumps
mloads = marshal.loads

PicklingError = "pickle.PicklingError"
UnpicklingError = "pickle.UnpicklingError"

MARK            = '('
STOP            = '.'
POP             = '0'
POP_MARK        = '1'
DUP             = '2'
FLOAT           = 'F'
INT             = 'I'
BININT          = 'J'
BININT1         = 'K'
LONG            = 'L'
BININT2         = 'M'
NONE            = 'N'
PERSID          = 'P'
BINPERSID       = 'Q'
REDUCE          = 'R'
STRING          = 'S'
BINSTRING       = 'T'
SHORT_BINSTRING = 'U'
APPEND          = 'a'
BUILD           = 'b'
GLOBAL          = 'c'
DICT            = 'd'
EMPTY_DICT      = '}'
APPENDS         = 'e'
GET             = 'g'
BINGET          = 'h'
INST            = 'i'
LONG_BINGET     = 'j'
LIST            = 'l'
EMPTY_LIST      = ']'
OBJ             = 'o'
PUT             = 'p'
BINPUT          = 'q'
LONG_BINPUT     = 'r'
SETITEM         = 's'
TUPLE           = 't'
EMPTY_TUPLE     = ')'
SETITEMS        = 'u'

class Pickler:

    def __init__(self, file, bin = 0):
        self.write = file.write
        self.memo = {}
        self.bin = bin

    def dump(self, object):
        self.save(object)
        self.write(STOP)

    def dump_special(self, callable, args, state = None):
	if (type(args) is not TupleType):
            raise PicklingError, "Second argument to dump_special " \
                                 "must be a tuple"

        self.save_reduce(callable, args, state)
        self.write(STOP)

    def put(self, i):
        if (self.bin):
            s = mdumps(i)[1:]
            if (i < 256):
                return BINPUT + s[0]

            return LONG_BINPUT + s

        return PUT + `i` + '\n'

    def get(self, i):
        if (self.bin):
            s = mdumps(i)[1:]

            if (i < 256):
                return BINGET + s[0]

            return LONG_BINGET + s

        return GET + `i` + '\n'
        
    def save(self, object, pers_save = 0):
        memo = self.memo

        if (not pers_save):
	    pid = self.persistent_id(object)
	    if (pid is not None):
                self.save_pers(pid)
                return

        d = id(object)
 
        t = type(object)

	if ((t is TupleType) and (len(object) == 0)):
	    if (self.bin):
                self.save_empty_tuple(object)
            else:
                self.save_tuple(object)
            return

        if memo.has_key(d):
            self.write(self.get(memo[d][0]))
            return

        try:
            f = self.dispatch[t]
        except KeyError:
            pid = self.inst_persistent_id(object)
            if pid is not None:
                self.save_pers(pid)
                return

            try:
                reduce = dispatch_table[t]
            except KeyError:
                try:
                    reduce = object.__reduce__
                except AttributeError:
                    raise PicklingError, \
                        "can't pickle %s objects" % `t.__name__`
                else:
                    tup = reduce()
            else:
                tup = reduce(object)

            if (type(tup) is not TupleType):
                raise PicklingError, "Value returned by %s must be a " \
                                     "tuple" % reduce

            l = len(tup)
   
	    if ((l != 2) and (l != 3)):
                raise PicklingError, "tuple returned by %s must contain " \
                                     "only two or three elements" % reduce

            callable = tup[0]
            arg_tup  = tup[1]
          
            if (l > 2):
                state = tup[2]
            else:
                state = None

            if (type(arg_tup) is not TupleType):
                raise PicklingError, "Second element of tuple returned " \
                                     "by %s must be a tuple" % reduce

            self.save_reduce(callable, arg_tup, state) 
            return

        f(self, object)

    def persistent_id(self, object):
        return None

    def inst_persistent_id(self, object):
        return None

    def save_pers(self, pid):
        if (not self.bin):
            self.write(PERSID + str(pid) + '\n')
        else:
            self.save(pid, 1)
            self.write(BINPERSID)

    def save_reduce(self, callable, arg_tup, state = None):
        write = self.write
        save = self.save

        save(callable)
        save(arg_tup)
        write(REDUCE)
        
	if (state is not None):
            save(state)
            write(BUILD)

    dispatch = {}

    def save_none(self, object):
        self.write(NONE)
    dispatch[NoneType] = save_none

    def save_int(self, object):
        if (self.bin):
            i = mdumps(object)[1:]
            if (i[-2:] == '\000\000'):
                if (i[-3] == '\000'):
                    self.write(BININT1 + i[:-3])
                    return

                self.write(BININT2 + i[:-2])
                return

            self.write(BININT + i)
        else:
            self.write(INT + `object` + '\n')
    dispatch[IntType] = save_int

    def save_long(self, object):
        self.write(LONG + `object` + '\n')
    dispatch[LongType] = save_long

    def save_float(self, object):
        self.write(FLOAT + `object` + '\n')
    dispatch[FloatType] = save_float

    def save_string(self, object):
        d = id(object)
        memo = self.memo

        if (self.bin):
            l = len(object)
            s = mdumps(l)[1:]
            if (l < 256):
                self.write(SHORT_BINSTRING + s[0] + object)
            else:
                self.write(BINSTRING + s + object)
        else:
            self.write(STRING + `object` + '\n')

        memo_len = len(memo)
        self.write(self.put(memo_len))
        memo[d] = (memo_len, object)
    dispatch[StringType] = save_string

    def save_tuple(self, object):

        write = self.write
        save  = self.save
        memo  = self.memo

        d = id(object)

        write(MARK)

        for element in object:
            save(element)

        if (len(object) and memo.has_key(d)):
	    if (self.bin):
                write(POP_MARK + self.get(memo[d][0]))
                return
           
            write(POP * (len(object) + 1) + self.get(mem[d][0]))
            return

        memo_len = len(memo)
        self.write(TUPLE + self.put(memo_len))
        memo[d] = (memo_len, object)
    dispatch[TupleType] = save_tuple

    def save_empty_tuple(self, object):
        self.write(EMPTY_TUPLE)

    def save_list(self, object):
        d = id(object)

        write = self.write
        save  = self.save
        memo  = self.memo

	if (self.bin):
            write(EMPTY_LIST)
        else:
            write(MARK + LIST)

        memo_len = len(memo)
        write(self.put(memo_len))
        memo[d] = (memo_len, object)

        using_appends = (self.bin and (len(object) > 1))

        if (using_appends):
            write(MARK)

        for element in object:
            save(element)
  
            if (not using_appends):
                write(APPEND)

        if (using_appends):
            write(APPENDS)
    dispatch[ListType] = save_list

    def save_dict(self, object):
        d = id(object)

        write = self.write
        save  = self.save
        memo  = self.memo

	if (self.bin):
            write(EMPTY_DICT)
        else:
            write(MARK + DICT)

        memo_len = len(memo)
        self.write(self.put(memo_len))
        memo[d] = (memo_len, object)

        using_setitems = (self.bin and (len(object) > 1))

        if (using_setitems):
            write(MARK)

        items = object.items()
        for key, value in items:
            save(key)
            save(value)

            if (not using_setitems):
                write(SETITEM)

        if (using_setitems):
            write(SETITEMS)

    dispatch[DictionaryType] = save_dict

    def save_inst(self, object):
        d = id(object)
        cls = object.__class__

        memo  = self.memo
        write = self.write
        save  = self.save

        if hasattr(object, '__getinitargs__'):
            args = object.__getinitargs__()
            len(args) # XXX Assert it's a sequence
        else:
            args = ()

        write(MARK)

        if (self.bin):
            save(cls)

        for arg in args:
            save(arg)

        memo_len = len(memo)
        if (self.bin):
            write(OBJ + self.put(memo_len))
        else:
            module = whichmodule(cls, cls.__name__)
            name = cls.__name__
            write(INST + module + '\n' + name + '\n' +
                self.put(memo_len))

        memo[d] = (memo_len, object)

        try:
            getstate = object.__getstate__
        except AttributeError:
            stuff = object.__dict__
        else:
            stuff = getstate()
        save(stuff)
        write(BUILD)
    dispatch[InstanceType] = save_inst

    def save_global(self, object, name = None):
        write = self.write
        memo = self.memo

        if (name is None):
            name = object.__name__

        module = whichmodule(object, name)

        memo_len = len(memo)
        write(GLOBAL + module + '\n' + name + '\n' +
            self.put(memo_len))
        memo[id(object)] = (memo_len, object)
    dispatch[ClassType] = save_global
    dispatch[FunctionType] = save_global
    dispatch[BuiltinFunctionType] = save_global


classmap = {}

def whichmodule(cls, clsname):
    """Figure out the module in which a class occurs.

    Search sys.modules for the module.
    Cache in classmap.
    Return a module name.
    If the class cannot be found, return __main__.
    """
    if classmap.has_key(cls):
        return classmap[cls]
    import sys

    for name, module in sys.modules.items():
        if hasattr(module, clsname) and \
            getattr(module, clsname) is cls:
            break
    else:
        name = '__main__'
    classmap[cls] = name
    return name


class Unpickler:

    def __init__(self, file):
        self.readline = file.readline
        self.read = file.read
        self.memo = {}

    def load(self):
        self.mark = ['spam'] # Any new unique object
        self.stack = []
        self.append = self.stack.append
        read = self.read
        dispatch = self.dispatch
        try:
            while 1:
                key = read(1)
                dispatch[key](self)
        except STOP, value:
            return value

    def marker(self):
        stack = self.stack
        mark = self.mark
        k = len(stack)-1
        while stack[k] is not mark: k = k-1
        return k

    dispatch = {}

    def load_eof(self):
        raise EOFError
    dispatch[''] = load_eof

    def load_persid(self):
        pid = self.readline()[:-1]
        self.append(self.persistent_load(pid))
    dispatch[PERSID] = load_persid

    def load_binpersid(self):
        stack = self.stack
         
        pid = stack[-1]
        del stack[-1]

        self.append(self.persistent_load(pid))
    dispatch[BINPERSID] = load_binpersid

    def load_none(self):
        self.append(None)
    dispatch[NONE] = load_none

    def load_int(self):
        self.append(string.atoi(self.readline()[:-1], 0))
    dispatch[INT] = load_int

    def load_binint(self):
        self.append(mloads('i' + self.read(4)))
    dispatch[BININT] = load_binint

    def load_binint1(self):
        self.append(mloads('i' + self.read(1) + '\000\000\000'))
    dispatch[BININT1] = load_binint1

    def load_binint2(self):
        self.append(mloads('i' + self.read(2) + '\000\000'))
    dispatch[BININT2] = load_binint2
 
    def load_long(self):
        self.append(string.atol(self.readline()[:-1], 0))
    dispatch[LONG] = load_long

    def load_float(self):
        self.append(string.atof(self.readline()[:-1]))
    dispatch[FLOAT] = load_float

    def load_string(self):
        self.append(eval(self.readline()[:-1],
                         {'__builtins__': {}})) # Let's be careful
    dispatch[STRING] = load_string

    def load_binstring(self):
        len = mloads('i' + self.read(4))
        self.append(self.read(len))
    dispatch[BINSTRING] = load_binstring

    def load_short_binstring(self):
        len = mloads('i' + self.read(1) + '\000\000\000')
        self.append(self.read(len))
    dispatch[SHORT_BINSTRING] = load_short_binstring

    def load_tuple(self):
        k = self.marker()
        self.stack[k:] = [tuple(self.stack[k+1:])]
    dispatch[TUPLE] = load_tuple

    def load_empty_tuple(self):
        self.stack.append(())
    dispatch[EMPTY_TUPLE] = load_empty_tuple

    def load_empty_list(self):
        self.stack.append([])
    dispatch[EMPTY_LIST] = load_empty_list

    def load_empty_dictionary(self):
        self.stack.append({})
    dispatch[EMPTY_DICT] = load_empty_dictionary

    def load_list(self):
        k = self.marker()
        self.stack[k:] = [self.stack[k+1:]]
    dispatch[LIST] = load_list

    def load_dict(self):
        k = self.marker()
        d = {}
        items = self.stack[k+1:]
        for i in range(0, len(items), 2):
            key = items[i]
            value = items[i+1]
            d[key] = value
        self.stack[k:] = [d]
    dispatch[DICT] = load_dict

    def load_inst(self):
        k = self.marker()
        args = tuple(self.stack[k+1:])
        del self.stack[k:]
        module = self.readline()[:-1]
        name = self.readline()[:-1]
        klass = self.find_class(module, name)
	if (type(klass) is not ClassType):
            raise SystemError, "Imported object %s from module %s is " \
                               "not a class" % (name, module)

        value = apply(klass, args)
        self.append(value)
    dispatch[INST] = load_inst

    def load_obj(self):
        stack = self.stack
        k = self.marker()
        klass = stack[k + 1]
        del stack[k + 1]
        args = tuple(stack[k + 1:]) 
        del stack[k:]
        value = apply(klass, args)
        self.append(value)
    dispatch[OBJ] = load_obj                

    def load_global(self):
        module = self.readline()[:-1]
        name = self.readline()[:-1]
        klass = self.find_class(module, name)
        self.append(klass)
    dispatch[GLOBAL] = load_global

    def find_class(self, module, name):
        env = {}

        try:
            exec 'from %s import %s' % (module, name) in env
        except ImportError:
            raise SystemError, \
                  "Failed to import class %s from module %s" % \
                  (name, module)
        klass = env[name]
        return klass

    def load_reduce(self):
        stack = self.stack

        callable = stack[-2]
        arg_tup  = stack[-1]
	del stack[-2:]

	if (type(callable) is not ClassType):
	    if (not safe_constructors.has_key(callable)):
		try:
                    safe = callable.__safe_for_unpickling__
                except AttributeError:
                    safe = None

                if (not safe):
                   raise UnpicklingError, "%s is not safe for " \
                                          "unpickling" % callable

        value = apply(callable, arg_tup)
        self.append(value)
    dispatch[REDUCE] = load_reduce

    def load_pop(self):
        del self.stack[-1]
    dispatch[POP] = load_pop

    def load_pop_mark(self):
        k = self.marker()
	del self.stack[k:]
    dispatch[POP_MARK] = load_pop_mark

    def load_dup(self):
        self.append(stack[-1])
    dispatch[DUP] = load_dup

    def load_get(self):
        self.append(self.memo[self.readline()[:-1]])
    dispatch[GET] = load_get

    def load_binget(self):
        i = mloads('i' + self.read(1) + '\000\000\000')
        self.append(self.memo[`i`])
    dispatch[BINGET] = load_binget

    def load_long_binget(self):
        i = mloads('i' + self.read(4))
        self.append(self.memo[`i`])
    dispatch[LONG_BINGET] = load_long_binget

    def load_put(self):
        self.memo[self.readline()[:-1]] = self.stack[-1]
    dispatch[PUT] = load_put

    def load_binput(self):
        i = mloads('i' + self.read(1) + '\000\000\000')
        self.memo[`i`] = self.stack[-1]
    dispatch[BINPUT] = load_binput

    def load_long_binput(self):
        i = mloads('i' + self.read(4))
        self.memo[`i`] = self.stack[-1]
    dispatch[LONG_BINPUT] = load_long_binput

    def load_append(self):
        stack = self.stack
        value = stack[-1]
        del stack[-1]
        list = stack[-1]
        list.append(value)
    dispatch[APPEND] = load_append

    def load_appends(self):
        stack = self.stack
        mark = self.marker()
        list = stack[mark - 1]
	for i in range(mark + 1, len(stack)):
            list.append(stack[i])

        del stack[mark:]
    dispatch[APPENDS] = load_appends
           
    def load_setitem(self):
        stack = self.stack
        value = stack[-1]
        key = stack[-2]
        del stack[-2:]
        dict = stack[-1]
        dict[key] = value
    dispatch[SETITEM] = load_setitem

    def load_setitems(self):
        stack = self.stack
        mark = self.marker()
        dict = stack[mark - 1]
	for i in range(mark + 1, len(stack), 2):
            dict[stack[i]] = stack[i + 1]

        del stack[mark:]
    dispatch[SETITEMS] = load_setitems

    def load_build(self):
        stack = self.stack
        value = stack[-1]
        del stack[-1]
        inst = stack[-1]
        try:
            setstate = inst.__setstate__
        except AttributeError:
            for key in value.keys():
                setattr(inst, key, value[key])
        else:
            setstate(value)
    dispatch[BUILD] = load_build

    def load_mark(self):
        self.append(self.mark)
    dispatch[MARK] = load_mark

    def load_stop(self):
        value = self.stack[-1]
        del self.stack[-1]
        raise STOP, value
    dispatch[STOP] = load_stop


# Shorthands

from StringIO import StringIO

def dump(object, file, bin = 0):
    Pickler(file, bin).dump(object)

def dumps(object, bin = 0):
    file = StringIO()
    Pickler(file, bin).dump(object)
    return file.getvalue()

def load(file):
    return Unpickler(file).load()

def loads(str):
    file = StringIO(str)
    return Unpickler(file).load()


# The rest is used for testing only

class C:
    def __cmp__(self, other):
        return cmp(self.__dict__, other.__dict__)

def test():
    fn = 'out'
    c = C()
    c.foo = 1
    c.bar = 2
    x = [0, 1, 2, 3]
    y = ('abc', 'abc', c, c)
    x.append(y)
    x.append(y)
    x.append(5)
    f = open(fn, 'w')
    F = Pickler(f)
    F.dump(x)
    f.close()
    f = open(fn, 'r')
    U = Unpickler(f)
    x2 = U.load()
    print x
    print x2
    print x == x2
    print map(id, x)
    print map(id, x2)
    print F.memo
    print U.memo

if __name__ == '__main__':
    test()
