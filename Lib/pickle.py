"""Create portable serialized representations of Python objects.

See module cPickle for a (much) faster implementation.
See module copy_reg for a mechanism for registering custom picklers.
See module pickletools source for extensive comments.

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
import marshal
import sys
import struct
import re

__all__ = ["PickleError", "PicklingError", "UnpicklingError", "Pickler",
           "Unpickler", "dump", "dumps", "load", "loads"]

# These are purely informational; no code usues these
format_version = "2.0"                  # File format version we write
compatible_formats = ["1.0",            # Original protocol 0
                      "1.1",            # Protocol 0 with class supprt added
                      "1.2",            # Original protocol 1
                      "1.3",            # Protocol 1 with BINFLOAT added
                      "2.0",            # Protocol 2
                      ]                 # Old format versions we can read

# Why use struct.pack() for pickling but marshal.loads() for
# unpickling?  struct.pack() is 40% faster than marshal.loads(), but
# marshal.loads() is twice as fast as struct.unpack()!
mloads = marshal.loads

class PickleError(Exception):
    """A common base class for the other pickling exceptions."""
    pass

class PicklingError(PickleError):
    """This exception is raised when an unpicklable object is passed to the
    dump() method.

    """
    pass

class UnpicklingError(PickleError):
    """This exception is raised when there is a problem unpickling an object,
    such as a security violation.

    Note that other exceptions may also be raised during unpickling, including
    (but not necessarily limited to) AttributeError, EOFError, ImportError,
    and IndexError.

    """
    pass

class _Stop(Exception):
    def __init__(self, value):
        self.value = value

try:
    from org.python.core import PyStringMap
except ImportError:
    PyStringMap = None

try:
    UnicodeType
except NameError:
    UnicodeType = None

# Pickle opcodes.  See pickletools.py for extensive docs.  The listing
# here is in kind-of alphabetical order of 1-character pickle code.
# pickletools groups them by purpose.

MARK            = '('   # push special markobject on stack
STOP            = '.'   # every pickle ends with STOP
POP             = '0'   # discard topmost stack item
POP_MARK        = '1'   # discard stack top through topmost markobject
DUP             = '2'   # duplicate top stack item
FLOAT           = 'F'   # push float object; decimal string argument
INT             = 'I'   # push integer or bool; decimal string argument
BININT          = 'J'   # push four-byte signed int
BININT1         = 'K'   # push 1-byte unsigned int
LONG            = 'L'   # push long; decimal string argument
BININT2         = 'M'   # push 2-byte unsigned int
NONE            = 'N'   # push None
PERSID          = 'P'   # push persistent object; id is taken from string arg
BINPERSID       = 'Q'   #  "       "         "  ;  "  "   "     "  stack
REDUCE          = 'R'   # apply callable to argtuple, both on stack
STRING          = 'S'   # push string; NL-terminated string argument
BINSTRING       = 'T'   # push string; counted binary string argument
SHORT_BINSTRING = 'U'   #  "     "   ;    "      "       "      " < 256 bytes
UNICODE         = 'V'   # push Unicode string; raw-unicode-escaped'd argument
BINUNICODE      = 'X'   #   "     "       "  ; counted UTF-8 string argument
APPEND          = 'a'   # append stack top to list below it
BUILD           = 'b'   # call __setstate__ or __dict__.update()
GLOBAL          = 'c'   # push self.find_class(modname, name); 2 string args
DICT            = 'd'   # build a dict from stack items
EMPTY_DICT      = '}'   # push empty dict
APPENDS         = 'e'   # extend list on stack by topmost stack slice
GET             = 'g'   # push item from memo on stack; index is string arg
BINGET          = 'h'   #   "    "    "    "   "   "  ;   "    " 1-byte arg
INST            = 'i'   # build & push class instance
LONG_BINGET     = 'j'   # push item from memo on stack; index is 4-byte arg
LIST            = 'l'   # build list from topmost stack items
EMPTY_LIST      = ']'   # push empty list
OBJ             = 'o'   # build & push class instance
PUT             = 'p'   # store stack top in memo; index is string arg
BINPUT          = 'q'   #   "     "    "   "   " ;   "    " 1-byte arg
LONG_BINPUT     = 'r'   #   "     "    "   "   " ;   "    " 4-byte arg
SETITEM         = 's'   # add key+value pair to dict
TUPLE           = 't'   # build tuple from topmost stack items
EMPTY_TUPLE     = ')'   # push empty tuple
SETITEMS        = 'u'   # modify dict by adding topmost key+value pairs
BINFLOAT        = 'G'   # push float; arg is 8-byte float encoding

TRUE            = 'I01\n'  # not an opcode; see INT docs in pickletools.py
FALSE           = 'I00\n'  # not an opcode; see INT docs in pickletools.py

# Protocol 2 (not yet implemented).

PROTO           = '\x80'  # identify pickle protocol
NEWOBJ          = '\x81'  # build object by applying cls.__new__ to argtuple
EXT1            = '\x82'  # push object from extension registry; 1-byte index
EXT2            = '\x83'  # ditto, but 2-byte index
EXT4            = '\x84'  # ditto, but 4-byte index
TUPLE1          = '\x85'  # build 1-tuple from stack top
TUPLE2          = '\x86'  # build 2-tuple from two topmost stack items
TUPLE3          = '\x87'  # build 3-tuple from three topmost stack items
NEWTRUE         = '\x88'  # push True
NEWFALSE        = '\x89'  # push False
LONG1           = '\x8a'  # push long from < 256 bytes
LONG4           = '\x8b'  # push really big long

_tuplesize2code = [EMPTY_TUPLE, TUPLE1, TUPLE2, TUPLE3]


__all__.extend([x for x in dir() if re.match("[A-Z][A-Z0-9_]+$",x)])
del x

_quotes = ["'", '"']

class Pickler:

    def __init__(self, file, proto=1):
        """This takes a file-like object for writing a pickle data stream.

        The optional proto argument tells the pickler to use the given
        protocol; supported protocols are 0, 1, 2.  The default
        protocol is 1 (in previous Python versions the default was 0).

        Protocol 1 is more efficient than protocol 0; protocol 2 is
        more efficient than protocol 1.  Protocol 2 is not the default
        because it is not supported by older Python versions.

        XXX Protocol 2 is not yet implemented.

        The file parameter must have a write() method that accepts a single
        string argument.  It can thus be an open file object, a StringIO
        object, or any other custom object that meets this interface.

        """
        if not 0 <= proto <= 2:
            raise ValueError, "pickle protocol must be 0, 1 or 2"
        self.write = file.write
        self.memo = {}
        self.proto = proto
        self.bin = proto >= 1

    def clear_memo(self):
        """Clears the pickler's "memo".

        The memo is the data structure that remembers which objects the
        pickler has already seen, so that shared or recursive objects are
        pickled by reference and not by value.  This method is useful when
        re-using picklers.

        """
        self.memo.clear()

    def dump(self, object):
        """Write a pickled representation of object to the open file object.

        Either the binary or ASCII format will be used, depending on the
        value of the bin flag passed to the constructor.

        """
        if self.proto >= 2:
            self.write(PROTO + chr(self.proto))
        self.save(object)
        self.write(STOP)

    def memoize(self, obj):
        """Store an object in the memo."""

        # The Pickler memo is a dictionary mapping object ids to 2-tuples
        # that contain the Unpickler memo key and the object being memoized.
        # The memo key is written to the pickle and will become
        # the key in the Unpickler's memo.  The object is stored in the
        # Pickler memo so that transient objects are kept alive during
        # pickling.

        # The use of the Unpickler memo length as the memo key is just a
        # convention.  The only requirement is that the memo values be unique.
        # But there appears no advantage to any other scheme, and this
        # scheme allows the Unpickler memo to be implemented as a plain (but
        # growable) array, indexed by memo key.
        memo_len = len(self.memo)
        self.write(self.put(memo_len))
        self.memo[id(obj)] = memo_len, obj

    # Return a PUT (BINPUT, LONG_BINPUT) opcode string, with argument i.
    def put(self, i, pack=struct.pack):
        if self.bin:
            if i < 256:
                return BINPUT + chr(i)
            else:
                return LONG_BINPUT + pack("<i", i)

        return PUT + `i` + '\n'

    # Return a GET (BINGET, LONG_BINGET) opcode string, with argument i.
    def get(self, i, pack=struct.pack):
        if self.bin:
            if i < 256:
                return BINGET + chr(i)
            else:
                return LONG_BINGET + pack("<i", i)

        return GET + `i` + '\n'

    def save(self, object):
        memo = self.memo

        pid = self.persistent_id(object)
        if pid is not None:
            self.save_pers(pid)
            return

        d = id(object)

        t = type(object)

        if d in memo:
            self.write(self.get(memo[d][0]))
            return

        try:
            f = self.dispatch[t]
        except KeyError:
            pass
        else:
            f(self, object)
            return

        # The dispatch table doesn't know about type t.
        try:
            issc = issubclass(t, TypeType)
        except TypeError: # t is not a class
            issc = 0
        if issc:
            self.save_global(object)
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

        if type(tup) is not TupleType:
            raise PicklingError, "Value returned by %s must be a " \
                                 "tuple" % reduce

        l = len(tup)

        if (l != 2) and (l != 3):
            raise PicklingError, "tuple returned by %s must contain " \
                                 "only two or three elements" % reduce

        callable = tup[0]
        arg_tup  = tup[1]

        if l > 2:
            state = tup[2]
        else:
            state = None

        if type(arg_tup) is not TupleType and arg_tup is not None:
            raise PicklingError, "Second element of tuple returned " \
                                 "by %s must be a tuple" % reduce

        self.save_reduce(callable, arg_tup, state)
        self.memoize(object)

    def persistent_id(self, object):
        return None

    def save_pers(self, pid):
        if self.bin:
            self.save(pid)
            self.write(BINPERSID)
        else:
            self.write(PERSID + str(pid) + '\n')

    def save_reduce(self, acallable, arg_tup, state = None):
        write = self.write
        save = self.save

        if not callable(acallable):
            raise PicklingError("__reduce__() must return callable as "
                                "first argument, not %s" % `acallable`)

        save(acallable)
        save(arg_tup)
        write(REDUCE)

        if state is not None:
            save(state)
            write(BUILD)

    dispatch = {}

    def save_none(self, object):
        self.write(NONE)
    dispatch[NoneType] = save_none

    def save_bool(self, object):
        if self.proto >= 2:
            self.write(object and NEWTRUE or NEWFALSE)
        else:
            self.write(object and TRUE or FALSE)
    dispatch[bool] = save_bool

    def save_int(self, object, pack=struct.pack):
        if self.bin:
            # If the int is small enough to fit in a signed 4-byte 2's-comp
            # format, we can store it more efficiently than the general
            # case.
            # First one- and two-byte unsigned ints:
            if object >= 0:
                if object <= 0xff:
                    self.write(BININT1 + chr(object))
                    return
                if object <= 0xffff:
                    self.write(BININT2 + chr(object&0xff) + chr(object>>8))
                    return
            # Next check for 4-byte signed ints:
            high_bits = object >> 31  # note that Python shift sign-extends
            if high_bits == 0 or high_bits == -1:
                # All high bits are copies of bit 2**31, so the value
                # fits in a 4-byte signed int.
                self.write(BININT + pack("<i", object))
                return
        # Text pickle, or int too big to fit in signed 4-byte format.
        self.write(INT + `object` + '\n')
    dispatch[IntType] = save_int

    def save_long(self, object, pack=struct.pack):
        if self.proto >= 2:
            bytes = encode_long(object)
            n = len(bytes)
            if n < 256:
                self.write(LONG1 + chr(n) + bytes)
            else:
                self.write(LONG4 + pack("<i", n) + bytes)
        self.write(LONG + `object` + '\n')
    dispatch[LongType] = save_long

    def save_float(self, object, pack=struct.pack):
        if self.bin:
            self.write(BINFLOAT + pack('>d', object))
        else:
            self.write(FLOAT + `object` + '\n')
    dispatch[FloatType] = save_float

    def save_string(self, object, pack=struct.pack):
        if self.bin:
            n = len(object)
            if n < 256:
                self.write(SHORT_BINSTRING + chr(n) + object)
            else:
                self.write(BINSTRING + pack("<i", n) + object)
        else:
            self.write(STRING + `object` + '\n')
        self.memoize(object)
    dispatch[StringType] = save_string

    def save_unicode(self, object, pack=struct.pack):
        if self.bin:
            encoding = object.encode('utf-8')
            n = len(encoding)
            self.write(BINUNICODE + pack("<i", n) + encoding)
        else:
            object = object.replace("\\", "\\u005c")
            object = object.replace("\n", "\\u000a")
            self.write(UNICODE + object.encode('raw-unicode-escape') + '\n')
        self.memoize(object)
    dispatch[UnicodeType] = save_unicode

    if StringType == UnicodeType:
        # This is true for Jython
        def save_string(self, object, pack=struct.pack):
            unicode = object.isunicode()

            if self.bin:
                if unicode:
                    object = object.encode("utf-8")
                l = len(object)
                if l < 256 and not unicode:
                    self.write(SHORT_BINSTRING + chr(l) + object)
                else:
                    s = pack("<i", l)
                    if unicode:
                        self.write(BINUNICODE + s + object)
                    else:
                        self.write(BINSTRING + s + object)
            else:
                if unicode:
                    object = object.replace("\\", "\\u005c")
                    object = object.replace("\n", "\\u000a")
                    object = object.encode('raw-unicode-escape')
                    self.write(UNICODE + object + '\n')
                else:
                    self.write(STRING + `object` + '\n')
            self.memoize(object)
        dispatch[StringType] = save_string

    def save_tuple(self, object):
        write = self.write
        save  = self.save
        memo  = self.memo
        proto = self.proto

        if proto >= 1:
            n = len(object)
            if n <= 3:
                if not object:
                    write(EMPTY_TUPLE)
                    return
                if proto >= 2:
                    for element in object:
                        save(element)
                    # Subtle.  Same as in the big comment below.
                    if id(object) in memo:
                        get = self.get(memo[id(object)][0])
                        write(POP * n + get)
                    else:
                        write(_tuplesize2code[n])
                        self.memoize(object)
                    return

        # proto 0, or proto 1 and tuple isn't empty, or proto > 1 and tuple
        # has more than 3 elements.
        write(MARK)
        for element in object:
            save(element)

        if object and id(object) in memo:
            # Subtle.  d was not in memo when we entered save_tuple(), so
            # the process of saving the tuple's elements must have saved
            # the tuple itself:  the tuple is recursive.  The proper action
            # now is to throw away everything we put on the stack, and
            # simply GET the tuple (it's already constructed).  This check
            # could have been done in the "for element" loop instead, but
            # recursive tuples are a rare thing.
            get = self.get(memo[id(object)][0])
            if proto:
                write(POP_MARK + get)
            else:   # proto 0 -- POP_MARK not available
                write(POP * (len(object) + 1) + get)
            return

        # No recursion (including the empty-tuple case for protocol 0).
        self.write(TUPLE)
        self.memoize(object) # XXX shouldn't memoize empty tuple?!

    dispatch[TupleType] = save_tuple

    def save_empty_tuple(self, object):
        self.write(EMPTY_TUPLE)

    def save_list(self, object):
        write = self.write
        save  = self.save

        if self.bin:
            write(EMPTY_LIST)
            self.memoize(object)
            n = len(object)
            if n > 1:
                write(MARK)
                for element in object:
                    save(element)
                write(APPENDS)
            elif n:
                assert n == 1
                save(object[0])
                write(APPEND)
            # else the list is empty, and we're already done

        else:   # proto 0 -- can't use EMPTY_LIST or APPENDS
            write(MARK + LIST)
            self.memoize(object)
            for element in object:
                save(element)
                write(APPEND)

    dispatch[ListType] = save_list

    def save_dict(self, object):
        write = self.write
        save  = self.save
        items = object.iteritems()

        if self.bin:
            write(EMPTY_DICT)
            self.memoize(object)
            if len(object) > 1:
                write(MARK)
                for key, value in items:
                    save(key)
                    save(value)
                write(SETITEMS)
                return

        else:   # proto 0 -- can't use EMPTY_DICT or SETITEMS
            write(MARK + DICT)
            self.memoize(object)

        # proto 0 or len(object) < 2
        for key, value in items:
            save(key)
            save(value)
            write(SETITEM)

    dispatch[DictionaryType] = save_dict
    if not PyStringMap is None:
        dispatch[PyStringMap] = save_dict

    def save_inst(self, object):
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

        if self.bin:
            save(cls)
            for arg in args:
                save(arg)
            write(OBJ)
        else:
            for arg in args:
                save(arg)
            write(INST + cls.__module__ + '\n' + cls.__name__ + '\n')

        self.memoize(object)

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

        if name is None:
            name = object.__name__

        try:
            module = object.__module__
        except AttributeError:
            module = whichmodule(object, name)

        try:
            __import__(module)
            mod = sys.modules[module]
            klass = getattr(mod, name)
        except (ImportError, KeyError, AttributeError):
            raise PicklingError(
                "Can't pickle %r: it's not found as %s.%s" %
                (object, module, name))
        else:
            if klass is not object:
                raise PicklingError(
                    "Can't pickle %r: it's not the same object as %s.%s" %
                    (object, module, name))

        write(GLOBAL + module + '\n' + name + '\n')
        self.memoize(object)

    dispatch[ClassType] = save_global
    dispatch[FunctionType] = save_global
    dispatch[BuiltinFunctionType] = save_global
    dispatch[TypeType] = save_global


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


classmap = {} # called classmap for backwards compatibility

def whichmodule(func, funcname):
    """Figure out the module in which a function occurs.

    Search sys.modules for the module.
    Cache in classmap.
    Return a module name.
    If the function cannot be found, return __main__.
    """
    if func in classmap:
        return classmap[func]

    for name, module in sys.modules.items():
        if module is None:
            continue # skip dummy package entries
        if name != '__main__' and \
            hasattr(module, funcname) and \
            getattr(module, funcname) is func:
            break
    else:
        name = '__main__'
    classmap[func] = name
    return name


class Unpickler:

    def __init__(self, file):
        """This takes a file-like object for reading a pickle data stream.

        This class automatically determines whether the data stream was
        written in binary mode or not, so it does not need a flag as in
        the Pickler class factory.

        The file-like object must have two methods, a read() method that
        takes an integer argument, and a readline() method that requires no
        arguments.  Both methods should return a string.  Thus file-like
        object can be a file object opened for reading, a StringIO object,
        or any other custom object that meets this interface.

        """
        self.readline = file.readline
        self.read = file.read
        self.memo = {}

    def load(self):
        """Read a pickled object representation from the open file object.

        Return the reconstituted object hierarchy specified in the file
        object.

        """
        self.mark = object() # any new unique object
        self.stack = []
        self.append = self.stack.append
        read = self.read
        dispatch = self.dispatch
        try:
            while 1:
                key = read(1)
                dispatch[key](self)
        except _Stop, stopinst:
            return stopinst.value

    # Return largest index k such that self.stack[k] is self.mark.
    # If the stack doesn't contain a mark, eventually raises IndexError.
    # This could be sped by maintaining another stack, of indices at which
    # the mark appears.  For that matter, the latter stack would suffice,
    # and we wouldn't need to push mark objects on self.stack at all.
    # Doing so is probably a good thing, though, since if the pickle is
    # corrupt (or hostile) we may get a clue from finding self.mark embedded
    # in unpickled objects.
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

    def load_proto(self):
        proto = ord(self.read(1))
        if not 0 <= proto <= 2:
            raise ValueError, "unsupported pickle protocol: %d" % proto
    dispatch[PROTO] = load_proto

    def load_persid(self):
        pid = self.readline()[:-1]
        self.append(self.persistent_load(pid))
    dispatch[PERSID] = load_persid

    def load_binpersid(self):
        pid = self.stack.pop()
        self.append(self.persistent_load(pid))
    dispatch[BINPERSID] = load_binpersid

    def load_none(self):
        self.append(None)
    dispatch[NONE] = load_none

    def load_false(self):
        self.append(False)
    dispatch[NEWFALSE] = load_false

    def load_true(self):
        self.append(True)
    dispatch[NEWTRUE] = load_true

    def load_int(self):
        data = self.readline()
        if data == FALSE[1:]:
            val = False
        elif data == TRUE[1:]:
            val = True
        else:
            try:
                val = int(data)
            except ValueError:
                val = long(data)
        self.append(val)
    dispatch[INT] = load_int

    def load_binint(self):
        self.append(mloads('i' + self.read(4)))
    dispatch[BININT] = load_binint

    def load_binint1(self):
        self.append(ord(self.read(1)))
    dispatch[BININT1] = load_binint1

    def load_binint2(self):
        self.append(mloads('i' + self.read(2) + '\000\000'))
    dispatch[BININT2] = load_binint2

    def load_long(self):
        self.append(long(self.readline()[:-1], 0))
    dispatch[LONG] = load_long

    def load_long1(self):
        n = ord(self.read(1))
        bytes = self.read(n)
        return decode_long(bytes)
    dispatch[LONG1] = load_long1

    def load_long4(self):
        n = mloads('i' + self.read(4))
        bytes = self.read(n)
        return decode_long(bytes)
    dispatch[LONG4] = load_long4

    def load_float(self):
        self.append(float(self.readline()[:-1]))
    dispatch[FLOAT] = load_float

    def load_binfloat(self, unpack=struct.unpack):
        self.append(unpack('>d', self.read(8))[0])
    dispatch[BINFLOAT] = load_binfloat

    def load_string(self):
        rep = self.readline()[:-1]
        for q in _quotes:
            if rep.startswith(q):
                if not rep.endswith(q):
                    raise ValueError, "insecure string pickle"
                rep = rep[len(q):-len(q)]
                break
        else:
            raise ValueError, "insecure string pickle"
        self.append(rep.decode("string-escape"))
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
        len = ord(self.read(1))
        self.append(self.read(len))
    dispatch[SHORT_BINSTRING] = load_short_binstring

    def load_tuple(self):
        k = self.marker()
        self.stack[k:] = [tuple(self.stack[k+1:])]
    dispatch[TUPLE] = load_tuple

    def load_empty_tuple(self):
        self.stack.append(())
    dispatch[EMPTY_TUPLE] = load_empty_tuple

    def load_tuple1(self):
        self.stack[-1] = (self.stack[-1],)
    dispatch[TUPLE1] = load_tuple1

    def load_tuple2(self):
        self.stack[-2:] = [(self.stack[-2], self.stack[-1])]
    dispatch[TUPLE2] = load_tuple2

    def load_tuple3(self):
        self.stack[-3:] = [(self.stack[-3], self.stack[-2], self.stack[-1])]
    dispatch[TUPLE3] = load_tuple3

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
                if not hasattr(klass, '__safe_for_unpickling__'):
                    raise UnpicklingError('%s is not safe for unpickling' %
                                          klass)
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
        __import__(module)
        mod = sys.modules[module]
        klass = getattr(mod, name)
        return klass

    def load_reduce(self):
        stack = self.stack

        callable = stack[-2]
        arg_tup  = stack[-1]
        del stack[-2:]

        if type(callable) is not ClassType:
            if not callable in safe_constructors:
                try:
                    safe = callable.__safe_for_unpickling__
                except AttributeError:
                    safe = None

                if not safe:
                    raise UnpicklingError, "%s is not safe for " \
                                           "unpickling" % callable

        if arg_tup is None:
            import warnings
            warnings.warn("The None return argument form of __reduce__  is "
                          "deprecated. Return a tuple of arguments instead.",
                          DeprecationWarning)
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
        i = ord(self.read(1))
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
        i = ord(self.read(1))
        self.memo[`i`] = self.stack[-1]
    dispatch[BINPUT] = load_binput

    def load_long_binput(self):
        i = mloads('i' + self.read(4))
        self.memo[`i`] = self.stack[-1]
    dispatch[LONG_BINPUT] = load_long_binput

    def load_append(self):
        stack = self.stack
        value = stack.pop()
        list = stack[-1]
        list.append(value)
    dispatch[APPEND] = load_append

    def load_appends(self):
        stack = self.stack
        mark = self.marker()
        list = stack[mark - 1]
        list.extend(stack[mark + 1:])
        del stack[mark:]
    dispatch[APPENDS] = load_appends

    def load_setitem(self):
        stack = self.stack
        value = stack.pop()
        key = stack.pop()
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
        value = stack.pop()
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
        value = self.stack.pop()
        raise _Stop(value)
    dispatch[STOP] = load_stop

# Helper class for load_inst/load_obj

class _EmptyClass:
    pass

# Encode/decode longs.

def encode_long(x):
    r"""Encode a long to a two's complement little-ending binary string.
    >>> encode_long(255L)
    '\xff\x00'
    >>> encode_long(32767L)
    '\xff\x7f'
    >>> encode_long(-256L)
    '\x00\xff'
    >>> encode_long(-32768L)
    '\x00\x80'
    >>> encode_long(-128L)
    '\x80'
    >>> encode_long(127L)
    '\x7f'
    >>>
    """
    digits = []
    while not -128 <= x < 128:
        digits.append(x & 0xff)
        x >>= 8
    digits.append(x & 0xff)
    return "".join(map(chr, digits))

def decode_long(data):
    r"""Decode a long from a two's complement little-endian binary string.
    >>> decode_long("\xff\x00")
    255L
    >>> decode_long("\xff\x7f")
    32767L
    >>> decode_long("\x00\xff")
    -256L
    >>> decode_long("\x00\x80")
    -32768L
    >>> decode_long("\x80")
    -128L
    >>> decode_long("\x7f")
    127L
    """
    x = 0L
    i = 0L
    for c in data:
        x |= long(ord(c)) << i
        i += 8L
    if data and ord(c) >= 0x80:
        x -= 1L << i
    return x

# Shorthands

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

def dump(object, file, proto=1):
    Pickler(file, proto).dump(object)

def dumps(object, proto=1):
    file = StringIO()
    Pickler(file, proto).dump(object)
    return file.getvalue()

def load(file):
    return Unpickler(file).load()

def loads(str):
    file = StringIO(str)
    return Unpickler(file).load()

# Doctest

def _test():
    import doctest
    return doctest.testmod()

if __name__ == "__main__":
    _test()
