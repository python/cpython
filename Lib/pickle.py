"""Create portable serialized representations of Python objects.

See module cPickle for a (much) faster implementation.
See module copy_reg for a mechanism for registering custom picklers.

Classes:

    Pickler
    Unpickler

Functions:

    dump(object, file)
    dumps(object) -> string
    load(file) -> object
    loads(string) -> object

Misc variables:

    __version__
    format_version
    compatible_formats

"""

__version__ = "$Revision$"       # Code version

from types import *
from copy_reg import dispatch_table, safe_constructors
import string
import marshal
import sys
import struct

format_version = "1.3"                     # File format version we write
compatible_formats = ["1.0", "1.1", "1.2"] # Old format versions we can read

mdumps = marshal.dumps
mloads = marshal.loads

class PickleError(Exception): pass
class PicklingError(PickleError): pass
class UnpicklingError(PickleError): pass

try:
    from org.python.core import PyStringMap
except ImportError:
    PyStringMap = None

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
UNICODE         = 'V'
BINUNICODE      = 'X'
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
BINFLOAT        = 'G'

class Pickler:

    def __init__(self, file, bin = 0):
        self.write = file.write
        self.memo = {}
        self.bin = bin

    def dump(self, object):
        self.save(object)
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
                        "can't pickle %s object: %s" % (`t.__name__`,
                                                         `object`)
                else:
                    tup = reduce()
            else:
                tup = reduce(object)

            if type(tup) is StringType:
                self.save_global(object, tup)
                return

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

            if type(arg_tup) is not TupleType and arg_tup is not None:
                raise PicklingError, "Second element of tuple returned " \
                                     "by %s must be a tuple" % reduce

            self.save_reduce(callable, arg_tup, state) 
            memo_len = len(memo)
            self.write(self.put(memo_len))
            memo[d] = (memo_len, object)
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

    def save_float(self, object, pack=struct.pack):
        if self.bin:
            self.write(BINFLOAT + pack('>d', object))
        else:
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

    def save_unicode(self, object):
        d = id(object)
        memo = self.memo

        if (self.bin):
            encoding = object.encode('utf-8')
            l = len(encoding)
            s = mdumps(l)[1:]
            self.write(BINUNICODE + s + encoding)
        else:
            object = object.replace(u"\\", u"\\u005c")
            object = object.replace(u"\n", u"\\u000a")
            self.write(UNICODE + object.encode('raw-unicode-escape') + '\n')

        memo_len = len(memo)
        self.write(self.put(memo_len))
        memo[d] = (memo_len, object)
    dispatch[UnicodeType] = save_unicode

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
           
            write(POP * (len(object) + 1) + self.get(memo[d][0]))
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
    if not PyStringMap is None:
        dispatch[PyStringMap] = save_dict

    def save_inst(self, object):
        d = id(object)
        cls = object.__class__

        memo  = self.memo
        write = self.write
        save  = self.save

        if hasattr(object, '__getinitargs__'):
            args = object.__getinitargs__()
            len(args) # XXX Assert it's a sequence
            _keep_alive(args, memo)
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
            write(INST + cls.__module__ + '\n' + cls.__name__ + '\n' +
                self.put(memo_len))

        memo[d] = (memo_len, object)

        try:
            getstate = object.__getstate__
        except AttributeError:
            stuff = object.__dict__
        else:
            stuff = getstate()
            _keep_alive(stuff, memo)
        save(stuff)
        write(BUILD)
    dispatch[InstanceType] = save_inst

    def save_global(self, object, name = None):
        write = self.write
        memo = self.memo

        if (name is None):
            name = object.__name__

        try:
            module = object.__module__
        except AttributeError:
            module = whichmodule(object, name)

        memo_len = len(memo)
        write(GLOBAL + module + '\n' + name + '\n' +
            self.put(memo_len))
        memo[id(object)] = (memo_len, object)
    dispatch[ClassType] = save_global
    dispatch[FunctionType] = save_global
    dispatch[BuiltinFunctionType] = save_global


def _keep_alive(x, memo):
    """Keeps a reference to the object x in the memo.

    Because we remember objects by their id, we have
    to assure that possibly temporary objects are kept
    alive by referencing them.
    We store a reference at the id of the memo, which should
    normally not be used unless someone tries to deepcopy
    the memo itself...
    """
    try:
        memo[id(memo)].append(x)
    except KeyError:
        # aha, this is the first one :-)
        memo[id(memo)]=[x]


classmap = {}

# This is no longer used to find classes, but still for functions
def whichmodule(cls, clsname):
    """Figure out the module in which a class occurs.

    Search sys.modules for the module.
    Cache in classmap.
    Return a module name.
    If the class cannot be found, return __main__.
    """
    if classmap.has_key(cls):
        return classmap[cls]

    for name, module in sys.modules.items():
        if name != '__main__' and \
            hasattr(module, clsname) and \
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
        self.append(string.atoi(self.readline()[:-1]))
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

    def load_binfloat(self, unpack=struct.unpack):
        self.append(unpack('>d', self.read(8))[0])
    dispatch[BINFLOAT] = load_binfloat

    def load_string(self):
        rep = self.readline()[:-1]
        if not self._is_string_secure(rep):
            raise ValueError, "insecure string pickle"
        self.append(eval(rep,
                         {'__builtins__': {}})) # Let's be careful
    dispatch[STRING] = load_string

    def _is_string_secure(self, s):
        """Return true if s contains a string that is safe to eval

        The definition of secure string is based on the implementation
        in cPickle.  s is secure as long as it only contains a quoted
        string and optional trailing whitespace.
        """
        q = s[0]
        if q not in ("'", '"'):
            return 0
        # find the closing quote
        offset = 1
        i = None
        while 1:
            try:
                i = s.index(q, offset)
            except ValueError:
                # if there is an error the first time, there is no
                # close quote
                if offset == 1:
                    return 0
            if s[i-1] != '\\':
                break
            # check to see if this one is escaped
            nslash = 0
            j = i - 1
            while j >= offset and s[j] == '\\':
                j = j - 1
                nslash = nslash + 1
            if nslash % 2 == 0:
                break
            offset = i + 1
        for c in s[i+1:]:
            if ord(c) > 32:
                return 0
        return 1

    def load_binstring(self):
        len = mloads('i' + self.read(4))
        self.append(self.read(len))
    dispatch[BINSTRING] = load_binstring

    def load_unicode(self):
        self.append(unicode(self.readline()[:-1],'raw-unicode-escape'))
    dispatch[UNICODE] = load_unicode

    def load_binunicode(self):
        len = mloads('i' + self.read(4))
        self.append(unicode(self.read(len),'utf-8'))
    dispatch[BINUNICODE] = load_binunicode

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
        instantiated = 0
        if (not args and type(klass) is ClassType and
            not hasattr(klass, "__getinitargs__")):
            try:
                value = _EmptyClass()
                value.__class__ = klass
                instantiated = 1
            except RuntimeError:
                # In restricted execution, assignment to inst.__class__ is
                # prohibited
                pass
        if not instantiated:
            try:
                value = apply(klass, args)
            except TypeError, err:
                raise TypeError, "in constructor for %s: %s" % (
                    klass.__name__, str(err)), sys.exc_info()[2]
        self.append(value)
    dispatch[INST] = load_inst

    def load_obj(self):
        stack = self.stack
        k = self.marker()
        klass = stack[k + 1]
        del stack[k + 1]
        args = tuple(stack[k + 1:]) 
        del stack[k:]
        instantiated = 0
        if (not args and type(klass) is ClassType and
            not hasattr(klass, "__getinitargs__")):
            try:
                value = _EmptyClass()
                value.__class__ = klass
                instantiated = 1
            except RuntimeError:
                # In restricted execution, assignment to inst.__class__ is
                # prohibited
                pass
        if not instantiated:
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
        try:
            __import__(module)
            mod = sys.modules[module]
            klass = getattr(mod, name)
        except (ImportError, KeyError, AttributeError):
            raise SystemError, \
                  "Failed to import class %s from module %s" % \
                  (name, module)
        return klass

    def load_reduce(self):
        stack = self.stack

        callable = stack[-2]
        arg_tup  = stack[-1]
        del stack[-2:]

        if type(callable) is not ClassType:
            if not safe_constructors.has_key(callable):
                try:
                    safe = callable.__safe_for_unpickling__
                except AttributeError:
                    safe = None

                if (not safe):
                   raise UnpicklingError, "%s is not safe for " \
                                          "unpickling" % callable

        if arg_tup is None:
            value = callable.__basicnew__()
        else:
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
        self.append(self.stack[-1])
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
            try:
                inst.__dict__.update(value)
            except RuntimeError:
                # XXX In restricted execution, the instance's __dict__ is not
                # accessible.  Use the old way of unpickling the instance
                # variables.  This is a semantic different when unpickling in
                # restricted vs. unrestricted modes.
                for k, v in value.items():
                    setattr(inst, k, v)
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

# Helper class for load_inst/load_obj

class _EmptyClass:
    pass

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
