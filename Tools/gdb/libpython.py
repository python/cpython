#!/usr/bin/python
'''
From gdb 7 onwards, gdb's build can be configured --with-python, allowing gdb
to be extended with Python code e.g. for library-specific data visualizations,
such as for the C++ STL types.  Documentation on this API can be seen at:
http://sourceware.org/gdb/current/onlinedocs/gdb/Python-API.html


This python module deals with the case when the process being debugged (the
"inferior process" in gdb parlance) is itself python, or more specifically,
linked against libpython.  In this situation, almost every item of data is a
(PyObject*), and having the debugger merely print their addresses is not very
enlightening.

This module embeds knowledge about the implementation details of libpython so
that we can emit useful visualizations e.g. a string, a list, a dict, a frame
giving file/line information and the state of local variables

In particular, given a gdb.Value corresponding to a PyObject* in the inferior
process, we can generate a "proxy value" within the gdb process.  For example,
given a PyObject* in the inferior process that is in fact a PyListObject*
holding three PyObject* that turn out to be PyBytesObject* instances, we can
generate a proxy value within the gdb process that is a list of bytes
instances:
  [b"foo", b"bar", b"baz"]

Doing so can be expensive for complicated graphs of objects, and could take
some time, so we also have a "write_repr" method that writes a representation
of the data to a file-like object.  This allows us to stop the traversal by
having the file-like object raise an exception if it gets too much data.

With both "proxyval" and "write_repr" we keep track of the set of all addresses
visited so far in the traversal, to avoid infinite recursion due to cycles in
the graph of object references.

We try to defer gdb.lookup_type() invocations for python types until as late as
possible: for a dynamically linked python binary, when the process starts in
the debugger, the libpython.so hasn't been dynamically loaded yet, so none of
the type names are known to the debugger

The module also extends gdb with some python-specific commands.
'''

import gdb
import os
import locale
import sys


# Look up the gdb.Type for some standard types:
# Those need to be refreshed as types (pointer sizes) may change when
# gdb loads different executables

def _type_char_ptr():
    return gdb.lookup_type('char').pointer()  # char*


def _type_unsigned_char_ptr():
    return gdb.lookup_type('unsigned char').pointer()  # unsigned char*


def _type_unsigned_short_ptr():
    return gdb.lookup_type('unsigned short').pointer()


def _type_unsigned_int_ptr():
    return gdb.lookup_type('unsigned int').pointer()


def _sizeof_void_p():
    return gdb.lookup_type('void').pointer().sizeof


Py_TPFLAGS_MANAGED_DICT      = (1 << 4)
Py_TPFLAGS_HEAPTYPE          = (1 << 9)
Py_TPFLAGS_LONG_SUBCLASS     = (1 << 24)
Py_TPFLAGS_LIST_SUBCLASS     = (1 << 25)
Py_TPFLAGS_TUPLE_SUBCLASS    = (1 << 26)
Py_TPFLAGS_BYTES_SUBCLASS    = (1 << 27)
Py_TPFLAGS_UNICODE_SUBCLASS  = (1 << 28)
Py_TPFLAGS_DICT_SUBCLASS     = (1 << 29)
Py_TPFLAGS_BASE_EXC_SUBCLASS = (1 << 30)
Py_TPFLAGS_TYPE_SUBCLASS     = (1 << 31)

#From pycore_frame.h
FRAME_OWNED_BY_CSTACK = 3

MAX_OUTPUT_LEN=1024

hexdigits = "0123456789abcdef"

ENCODING = locale.getpreferredencoding()

FRAME_INFO_OPTIMIZED_OUT = '(frame information optimized out)'
UNABLE_READ_INFO_PYTHON_FRAME = 'Unable to read information on python frame'
EVALFRAME = '_PyEval_EvalFrameDefault'

class NullPyObjectPtr(RuntimeError):
    pass


def safety_limit(val):
    # Given an integer value from the process being debugged, limit it to some
    # safety threshold so that arbitrary breakage within said process doesn't
    # break the gdb process too much (e.g. sizes of iterations, sizes of lists)
    return min(val, 1000)


def safe_range(val):
    # As per range, but don't trust the value too much: cap it to a safety
    # threshold in case the data was corrupted
    return range(safety_limit(int(val)))

class StringTruncated(RuntimeError):
    pass

class TruncatedStringIO(object):
    '''Similar to io.StringIO, but can truncate the output by raising a
    StringTruncated exception'''
    def __init__(self, maxlen=None):
        self._val = ''
        self.maxlen = maxlen

    def write(self, data):
        if self.maxlen:
            if len(data) + len(self._val) > self.maxlen:
                # Truncation:
                self._val += data[0:self.maxlen - len(self._val)]
                raise StringTruncated()

        self._val += data

    def getvalue(self):
        return self._val

class PyObjectPtr(object):
    """
    Class wrapping a gdb.Value that's either a (PyObject*) within the
    inferior process, or some subclass pointer e.g. (PyBytesObject*)

    There will be a subclass for every refined PyObject type that we care
    about.

    Note that at every stage the underlying pointer could be NULL, point
    to corrupt data, etc; this is the debugger, after all.
    """
    _typename = 'PyObject'

    def __init__(self, gdbval, cast_to=None):
        if cast_to:
            self._gdbval = gdbval.cast(cast_to)
        else:
            self._gdbval = gdbval

    def field(self, name):
        '''
        Get the gdb.Value for the given field within the PyObject.

        Various libpython types are defined using the "PyObject_HEAD" and
        "PyObject_VAR_HEAD" macros.

        In Python, this is defined as an embedded PyVarObject type thus:
           PyVarObject ob_base;
        so that the "ob_size" field is located insize the "ob_base" field, and
        the "ob_type" is most easily accessed by casting back to a (PyObject*).
        '''
        if self.is_null():
            raise NullPyObjectPtr(self)

        if name == 'ob_type':
            pyo_ptr = self._gdbval.cast(PyObjectPtr.get_gdb_type())
            return pyo_ptr.dereference()[name]

        if name == 'ob_size':
            pyo_ptr = self._gdbval.cast(PyVarObjectPtr.get_gdb_type())
            return pyo_ptr.dereference()[name]

        # General case: look it up inside the object:
        return self._gdbval.dereference()[name]

    def pyop_field(self, name):
        '''
        Get a PyObjectPtr for the given PyObject* field within this PyObject.
        '''
        return PyObjectPtr.from_pyobject_ptr(self.field(name))

    def write_field_repr(self, name, out, visited):
        '''
        Extract the PyObject* field named "name", and write its representation
        to file-like object "out"
        '''
        field_obj = self.pyop_field(name)
        field_obj.write_repr(out, visited)

    def get_truncated_repr(self, maxlen):
        '''
        Get a repr-like string for the data, but truncate it at "maxlen" bytes
        (ending the object graph traversal as soon as you do)
        '''
        out = TruncatedStringIO(maxlen)
        try:
            self.write_repr(out, set())
        except StringTruncated:
            # Truncation occurred:
            return out.getvalue() + '...(truncated)'

        # No truncation occurred:
        return out.getvalue()

    def type(self):
        return PyTypeObjectPtr(self.field('ob_type'))

    def is_null(self):
        return 0 == int(self._gdbval)

    def is_optimized_out(self):
        '''
        Is the value of the underlying PyObject* visible to the debugger?

        This can vary with the precise version of the compiler used to build
        Python, and the precise version of gdb.

        See e.g. https://bugzilla.redhat.com/show_bug.cgi?id=556975 with
        PyEval_EvalFrameEx's "f"
        '''
        return self._gdbval.is_optimized_out

    def safe_tp_name(self):
        try:
            ob_type = self.type()
            tp_name = ob_type.field('tp_name')
            return tp_name.string()
        # NullPyObjectPtr: NULL tp_name?
        # RuntimeError: Can't even read the object at all?
        # UnicodeDecodeError: Failed to decode tp_name bytestring
        except (NullPyObjectPtr, RuntimeError, UnicodeDecodeError):
            return 'unknown'

    def proxyval(self, visited):
        '''
        Scrape a value from the inferior process, and try to represent it
        within the gdb process, whilst (hopefully) avoiding crashes when
        the remote data is corrupt.

        Derived classes will override this.

        For example, a PyIntObject* with ob_ival 42 in the inferior process
        should result in an int(42) in this process.

        visited: a set of all gdb.Value pyobject pointers already visited
        whilst generating this value (to guard against infinite recursion when
        visiting object graphs with loops).  Analogous to Py_ReprEnter and
        Py_ReprLeave
        '''

        class FakeRepr(object):
            """
            Class representing a non-descript PyObject* value in the inferior
            process for when we don't have a custom scraper, intended to have
            a sane repr().
            """

            def __init__(self, tp_name, address):
                self.tp_name = tp_name
                self.address = address

            def __repr__(self):
                # For the NULL pointer, we have no way of knowing a type, so
                # special-case it as per
                # http://bugs.python.org/issue8032#msg100882
                if self.address == 0:
                    return '0x0'
                return '<%s at remote 0x%x>' % (self.tp_name, self.address)

        return FakeRepr(self.safe_tp_name(),
                        int(self._gdbval))

    def write_repr(self, out, visited):
        '''
        Write a string representation of the value scraped from the inferior
        process to "out", a file-like object.
        '''
        # Default implementation: generate a proxy value and write its repr
        # However, this could involve a lot of work for complicated objects,
        # so for derived classes we specialize this
        return out.write(repr(self.proxyval(visited)))

    @classmethod
    def subclass_from_type(cls, t):
        '''
        Given a PyTypeObjectPtr instance wrapping a gdb.Value that's a
        (PyTypeObject*), determine the corresponding subclass of PyObjectPtr
        to use

        Ideally, we would look up the symbols for the global types, but that
        isn't working yet:
          (gdb) python print gdb.lookup_symbol('PyList_Type')[0].value
          Traceback (most recent call last):
            File "<string>", line 1, in <module>
          NotImplementedError: Symbol type not yet supported in Python scripts.
          Error while executing Python code.

        For now, we use tp_flags, after doing some string comparisons on the
        tp_name for some special-cases that don't seem to be visible through
        flags
        '''
        try:
            tp_name = t.field('tp_name').string()
            tp_flags = int(t.field('tp_flags'))
        # RuntimeError: NULL pointers
        # UnicodeDecodeError: string() fails to decode the bytestring
        except (RuntimeError, UnicodeDecodeError):
            # Handle any kind of error e.g. NULL ptrs by simply using the base
            # class
            return cls

        #print('tp_flags = 0x%08x' % tp_flags)
        #print('tp_name = %r' % tp_name)

        name_map = {'bool': PyBoolObjectPtr,
                    'classobj': PyClassObjectPtr,
                    'NoneType': PyNoneStructPtr,
                    'frame': PyFrameObjectPtr,
                    'set' : PySetObjectPtr,
                    'frozenset' : PySetObjectPtr,
                    'builtin_function_or_method' : PyCFunctionObjectPtr,
                    'method-wrapper': wrapperobject,
                    }
        if tp_name in name_map:
            return name_map[tp_name]

        if tp_flags & Py_TPFLAGS_HEAPTYPE:
            return HeapTypeObjectPtr

        if tp_flags & Py_TPFLAGS_LONG_SUBCLASS:
            return PyLongObjectPtr
        if tp_flags & Py_TPFLAGS_LIST_SUBCLASS:
            return PyListObjectPtr
        if tp_flags & Py_TPFLAGS_TUPLE_SUBCLASS:
            return PyTupleObjectPtr
        if tp_flags & Py_TPFLAGS_BYTES_SUBCLASS:
            return PyBytesObjectPtr
        if tp_flags & Py_TPFLAGS_UNICODE_SUBCLASS:
            return PyUnicodeObjectPtr
        if tp_flags & Py_TPFLAGS_DICT_SUBCLASS:
            return PyDictObjectPtr
        if tp_flags & Py_TPFLAGS_BASE_EXC_SUBCLASS:
            return PyBaseExceptionObjectPtr
        #if tp_flags & Py_TPFLAGS_TYPE_SUBCLASS:
        #    return PyTypeObjectPtr

        # Use the base class:
        return cls

    @classmethod
    def from_pyobject_ptr(cls, gdbval):
        '''
        Try to locate the appropriate derived class dynamically, and cast
        the pointer accordingly.
        '''
        try:
            p = PyObjectPtr(gdbval)
            cls = cls.subclass_from_type(p.type())
            return cls(gdbval, cast_to=cls.get_gdb_type())
        except RuntimeError:
            # Handle any kind of error e.g. NULL ptrs by simply using the base
            # class
            pass
        return cls(gdbval)

    @classmethod
    def get_gdb_type(cls):
        return gdb.lookup_type(cls._typename).pointer()

    def as_address(self):
        return int(self._gdbval)

class PyVarObjectPtr(PyObjectPtr):
    _typename = 'PyVarObject'

class ProxyAlreadyVisited(object):
    '''
    Placeholder proxy to use when protecting against infinite recursion due to
    loops in the object graph.

    Analogous to the values emitted by the users of Py_ReprEnter and Py_ReprLeave
    '''
    def __init__(self, rep):
        self._rep = rep

    def __repr__(self):
        return self._rep


def _write_instance_repr(out, visited, name, pyop_attrdict, address):
    '''Shared code for use by all classes:
    write a representation to file-like object "out"'''
    out.write('<')
    out.write(name)

    # Write dictionary of instance attributes:
    if isinstance(pyop_attrdict, (PyKeysValuesPair, PyDictObjectPtr)):
        out.write('(')
        first = True
        items = pyop_attrdict.iteritems()
        for pyop_arg, pyop_val in items:
            if not first:
                out.write(', ')
            first = False
            out.write(pyop_arg.proxyval(visited))
            out.write('=')
            pyop_val.write_repr(out, visited)
        out.write(')')
    out.write(' at remote 0x%x>' % address)


class InstanceProxy(object):

    def __init__(self, cl_name, attrdict, address):
        self.cl_name = cl_name
        self.attrdict = attrdict
        self.address = address

    def __repr__(self):
        if isinstance(self.attrdict, dict):
            kwargs = ', '.join(["%s=%r" % (arg, val)
                                for arg, val in self.attrdict.items()])
            return '<%s(%s) at remote 0x%x>' % (self.cl_name,
                                                kwargs, self.address)
        else:
            return '<%s at remote 0x%x>' % (self.cl_name,
                                            self.address)

def _PyObject_VAR_SIZE(typeobj, nitems):
    if _PyObject_VAR_SIZE._type_size_t is None:
        _PyObject_VAR_SIZE._type_size_t = gdb.lookup_type('size_t')

    return ( ( typeobj.field('tp_basicsize') +
               nitems * typeobj.field('tp_itemsize') +
               (_sizeof_void_p() - 1)
             ) & ~(_sizeof_void_p() - 1)
           ).cast(_PyObject_VAR_SIZE._type_size_t)
_PyObject_VAR_SIZE._type_size_t = None

class HeapTypeObjectPtr(PyObjectPtr):
    _typename = 'PyObject'

    def get_attr_dict(self):
        '''
        Get the PyDictObject ptr representing the attribute dictionary
        (or None if there's a problem)
        '''
        try:
            typeobj = self.type()
            dictoffset = int_from_int(typeobj.field('tp_dictoffset'))
            if dictoffset != 0:
                if dictoffset < 0:
                    if int_from_int(typeobj.field('tp_flags')) & Py_TPFLAGS_MANAGED_DICT:
                        assert dictoffset == -1
                        dictoffset = -3 * _sizeof_void_p()
                    else:
                        type_PyVarObject_ptr = gdb.lookup_type('PyVarObject').pointer()
                        tsize = int_from_int(self._gdbval.cast(type_PyVarObject_ptr)['ob_size'])
                        if tsize < 0:
                            tsize = -tsize
                        size = _PyObject_VAR_SIZE(typeobj, tsize)
                        dictoffset += size
                        assert dictoffset % _sizeof_void_p() == 0

                dictptr = self._gdbval.cast(_type_char_ptr()) + dictoffset
                PyObjectPtrPtr = PyObjectPtr.get_gdb_type().pointer()
                dictptr = dictptr.cast(PyObjectPtrPtr)
                if int(dictptr.dereference()) & 1:
                    return None
                return PyObjectPtr.from_pyobject_ptr(dictptr.dereference())
        except RuntimeError:
            # Corrupt data somewhere; fail safe
            pass

        # Not found, or some kind of error:
        return None

    def get_keys_values(self):
        typeobj = self.type()
        has_values =  int_from_int(typeobj.field('tp_flags')) & Py_TPFLAGS_MANAGED_DICT
        if not has_values:
            return None
        charptrptr_t = _type_char_ptr().pointer()
        ptr = self._gdbval.cast(charptrptr_t) - 3
        char_ptr = ptr.dereference()
        if (int(char_ptr) & 1) == 0:
            return None
        char_ptr += 1
        values_ptr = char_ptr.cast(gdb.lookup_type("PyDictValues").pointer())
        values = values_ptr['values']
        return PyKeysValuesPair(self.get_cached_keys(), values)

    def get_cached_keys(self):
        typeobj = self.type()
        HeapTypePtr = gdb.lookup_type("PyHeapTypeObject").pointer()
        return typeobj._gdbval.cast(HeapTypePtr)['ht_cached_keys']

    def proxyval(self, visited):
        '''
        Support for classes.

        Currently we just locate the dictionary using a transliteration to
        python of _PyObject_GetDictPtr, ignoring descriptors
        '''
        # Guard against infinite loops:
        if self.as_address() in visited:
            return ProxyAlreadyVisited('<...>')
        visited.add(self.as_address())

        keys_values = self.get_keys_values()
        if keys_values:
            attr_dict = keys_values.proxyval(visited)
        else:
            pyop_attr_dict = self.get_attr_dict()
            if pyop_attr_dict:
                attr_dict = pyop_attr_dict.proxyval(visited)
            else:
                attr_dict = {}
        tp_name = self.safe_tp_name()

        # Class:
        return InstanceProxy(tp_name, attr_dict, int(self._gdbval))

    def write_repr(self, out, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            out.write('<...>')
            return
        visited.add(self.as_address())

        pyop_attrs = self.get_keys_values()
        if not pyop_attrs:
            pyop_attrs = self.get_attr_dict()
        _write_instance_repr(out, visited,
                             self.safe_tp_name(), pyop_attrs, self.as_address())

class ProxyException(Exception):
    def __init__(self, tp_name, args):
        self.tp_name = tp_name
        self.args = args

    def __repr__(self):
        return '%s%r' % (self.tp_name, self.args)

class PyBaseExceptionObjectPtr(PyObjectPtr):
    """
    Class wrapping a gdb.Value that's a PyBaseExceptionObject* i.e. an exception
    within the process being debugged.
    """
    _typename = 'PyBaseExceptionObject'

    def proxyval(self, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            return ProxyAlreadyVisited('(...)')
        visited.add(self.as_address())
        arg_proxy = self.pyop_field('args').proxyval(visited)
        return ProxyException(self.safe_tp_name(),
                              arg_proxy)

    def write_repr(self, out, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            out.write('(...)')
            return
        visited.add(self.as_address())

        out.write(self.safe_tp_name())
        self.write_field_repr('args', out, visited)

class PyClassObjectPtr(PyObjectPtr):
    """
    Class wrapping a gdb.Value that's a PyClassObject* i.e. a <classobj>
    instance within the process being debugged.
    """
    _typename = 'PyClassObject'


class BuiltInFunctionProxy(object):
    def __init__(self, ml_name):
        self.ml_name = ml_name

    def __repr__(self):
        return "<built-in function %s>" % self.ml_name

class BuiltInMethodProxy(object):
    def __init__(self, ml_name, pyop_m_self):
        self.ml_name = ml_name
        self.pyop_m_self = pyop_m_self

    def __repr__(self):
        return ('<built-in method %s of %s object at remote 0x%x>'
                % (self.ml_name,
                   self.pyop_m_self.safe_tp_name(),
                   self.pyop_m_self.as_address())
                )

class PyCFunctionObjectPtr(PyObjectPtr):
    """
    Class wrapping a gdb.Value that's a PyCFunctionObject*
    (see Include/methodobject.h and Objects/methodobject.c)
    """
    _typename = 'PyCFunctionObject'

    def proxyval(self, visited):
        m_ml = self.field('m_ml') # m_ml is a (PyMethodDef*)
        try:
            ml_name = m_ml['ml_name'].string()
        except UnicodeDecodeError:
            ml_name = '<ml_name:UnicodeDecodeError>'

        pyop_m_self = self.pyop_field('m_self')
        if pyop_m_self.is_null():
            return BuiltInFunctionProxy(ml_name)
        else:
            return BuiltInMethodProxy(ml_name, pyop_m_self)

# Python implementation of location table parsing algorithm
def read(it):
    return ord(next(it))

def read_varint(it):
    b = read(it)
    val = b & 63;
    shift = 0;
    while b & 64:
        b = read(it)
        shift += 6
        val |= (b&63) << shift
    return val

def read_signed_varint(it):
    uval = read_varint(it)
    if uval & 1:
        return -(uval >> 1)
    else:
        return uval >> 1

def parse_location_table(firstlineno, linetable):
    line = firstlineno
    addr = 0
    it = iter(linetable)
    while True:
        try:
            first_byte = read(it)
        except StopIteration:
            return
        code = (first_byte >> 3) & 15
        length = (first_byte & 7) + 1
        end_addr = addr + length
        if code == 15:
            yield addr, end_addr, None
            addr = end_addr
            continue
        elif code == 14: # Long form
            line_delta = read_signed_varint(it)
            line += line_delta
            end_line = line + read_varint(it)
            col = read_varint(it)
            end_col = read_varint(it)
        elif code == 13: # No column
            line_delta = read_signed_varint(it)
            line += line_delta
        elif code in (10, 11, 12): # new line
            line_delta = code - 10
            line += line_delta
            column = read(it)
            end_column = read(it)
        else:
            assert (0 <= code < 10)
            second_byte = read(it)
            column = code << 3 | (second_byte >> 4)
        yield addr, end_addr, line
        addr = end_addr

class PyCodeObjectPtr(PyObjectPtr):
    """
    Class wrapping a gdb.Value that's a PyCodeObject* i.e. a <code> instance
    within the process being debugged.
    """
    _typename = 'PyCodeObject'

    def addr2line(self, addrq):
        '''
        Get the line number for a given bytecode offset

        Analogous to PyCode_Addr2Line; translated from pseudocode in
        Objects/lnotab_notes.txt
        '''
        co_linetable = self.pyop_field('co_linetable').proxyval(set())

        # Initialize lineno to co_firstlineno as per PyCode_Addr2Line
        # not 0, as lnotab_notes.txt has it:
        lineno = int_from_int(self.field('co_firstlineno'))

        if addrq < 0:
            return lineno
        addr = 0
        for addr, end_addr, line in parse_location_table(lineno, co_linetable):
            if addr <= addrq and end_addr > addrq:
                return line
        assert False, "Unreachable"


def items_from_keys_and_values(keys, values):
    entries, nentries = PyDictObjectPtr._get_entries(keys)
    for i in safe_range(nentries):
        ep = entries[i]
        pyop_value = PyObjectPtr.from_pyobject_ptr(values[i])
        if not pyop_value.is_null():
            pyop_key = PyObjectPtr.from_pyobject_ptr(ep['me_key'])
            yield (pyop_key, pyop_value)

class PyKeysValuesPair:

    def __init__(self, keys, values):
        self.keys = keys
        self.values = values

    def iteritems(self):
        return items_from_keys_and_values(self.keys, self.values)

    def proxyval(self, visited):
        result = {}
        for pyop_key, pyop_value in self.iteritems():
            proxy_key = pyop_key.proxyval(visited)
            proxy_value = pyop_value.proxyval(visited)
            result[proxy_key] = proxy_value
        return result

class PyDictObjectPtr(PyObjectPtr):
    """
    Class wrapping a gdb.Value that's a PyDictObject* i.e. a dict instance
    within the process being debugged.
    """
    _typename = 'PyDictObject'

    def iteritems(self):
        '''
        Yields a sequence of (PyObjectPtr key, PyObjectPtr value) pairs,
        analogous to dict.iteritems()
        '''
        keys = self.field('ma_keys')
        values = self.field('ma_values')
        has_values = int(values)
        if has_values:
            values = values['values']
        if has_values:
            for item in items_from_keys_and_values(keys, values):
                yield item
            return
        entries, nentries = self._get_entries(keys)
        for i in safe_range(nentries):
            ep = entries[i]
            pyop_value = PyObjectPtr.from_pyobject_ptr(ep['me_value'])
            if not pyop_value.is_null():
                pyop_key = PyObjectPtr.from_pyobject_ptr(ep['me_key'])
                yield (pyop_key, pyop_value)

    def proxyval(self, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            return ProxyAlreadyVisited('{...}')
        visited.add(self.as_address())

        result = {}
        for pyop_key, pyop_value in self.iteritems():
            proxy_key = pyop_key.proxyval(visited)
            proxy_value = pyop_value.proxyval(visited)
            result[proxy_key] = proxy_value
        return result

    def write_repr(self, out, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            out.write('{...}')
            return
        visited.add(self.as_address())

        out.write('{')
        first = True
        for pyop_key, pyop_value in self.iteritems():
            if not first:
                out.write(', ')
            first = False
            pyop_key.write_repr(out, visited)
            out.write(': ')
            pyop_value.write_repr(out, visited)
        out.write('}')

    @staticmethod
    def _get_entries(keys):
        dk_nentries = int(keys['dk_nentries'])
        dk_size = 1<<int(keys['dk_log2_size'])

        if dk_size <= 0xFF:
            offset = dk_size
        elif dk_size <= 0xFFFF:
            offset = 2 * dk_size
        elif dk_size <= 0xFFFFFFFF:
            offset = 4 * dk_size
        else:
            offset = 8 * dk_size

        ent_addr = keys['dk_indices'].address
        ent_addr = ent_addr.cast(_type_unsigned_char_ptr()) + offset
        if int(keys['dk_kind']) == 0:  # DICT_KEYS_GENERAL
            ent_ptr_t = gdb.lookup_type('PyDictKeyEntry').pointer()
        else:
            ent_ptr_t = gdb.lookup_type('PyDictUnicodeEntry').pointer()
        ent_addr = ent_addr.cast(ent_ptr_t)

        return ent_addr, dk_nentries


class PyListObjectPtr(PyObjectPtr):
    _typename = 'PyListObject'

    def __getitem__(self, i):
        # Get the gdb.Value for the (PyObject*) with the given index:
        field_ob_item = self.field('ob_item')
        return field_ob_item[i]

    def proxyval(self, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            return ProxyAlreadyVisited('[...]')
        visited.add(self.as_address())

        result = [PyObjectPtr.from_pyobject_ptr(self[i]).proxyval(visited)
                  for i in safe_range(int_from_int(self.field('ob_size')))]
        return result

    def write_repr(self, out, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            out.write('[...]')
            return
        visited.add(self.as_address())

        out.write('[')
        for i in safe_range(int_from_int(self.field('ob_size'))):
            if i > 0:
                out.write(', ')
            element = PyObjectPtr.from_pyobject_ptr(self[i])
            element.write_repr(out, visited)
        out.write(']')

class PyLongObjectPtr(PyObjectPtr):
    _typename = 'PyLongObject'

    def proxyval(self, visited):
        '''
        Python's Include/longobjrep.h has this declaration:

            typedef struct _PyLongValue {
                uintptr_t lv_tag; /* Number of digits, sign and flags */
                digit ob_digit[1];
            } _PyLongValue;

            struct _longobject {
                PyObject_HEAD
               _PyLongValue long_value;
            };

        with this description:
            The absolute value of a number is equal to
                 SUM(for i=0 through abs(ob_size)-1) ob_digit[i] * 2**(SHIFT*i)
            Negative numbers are represented with ob_size < 0;
            zero is represented by ob_size == 0.

        where SHIFT can be either:
            #define PyLong_SHIFT        30
            #define PyLong_SHIFT        15
        '''
        long_value = self.field('long_value')
        lv_tag = int(long_value['lv_tag'])
        size = lv_tag >> 3
        if size == 0:
            return 0

        ob_digit = long_value['ob_digit']

        if gdb.lookup_type('digit').sizeof == 2:
            SHIFT = 15
        else:
            SHIFT = 30

        digits = [int(ob_digit[i]) * 2**(SHIFT*i)
                  for i in safe_range(size)]
        result = sum(digits)
        if (lv_tag & 3) == 2:
            result = -result
        return result

    def write_repr(self, out, visited):
        # Write this out as a Python int literal
        proxy = self.proxyval(visited)
        out.write("%s" % proxy)


class PyBoolObjectPtr(PyLongObjectPtr):
    """
    Class wrapping a gdb.Value that's a PyBoolObject* i.e. one of the two
    <bool> instances (Py_True/Py_False) within the process being debugged.
    """
    def proxyval(self, visited):
        if PyLongObjectPtr.proxyval(self, visited):
            return True
        else:
            return False

class PyNoneStructPtr(PyObjectPtr):
    """
    Class wrapping a gdb.Value that's a PyObject* pointing to the
    singleton (we hope) _Py_NoneStruct with ob_type PyNone_Type
    """
    _typename = 'PyObject'

    def proxyval(self, visited):
        return None

class PyFrameObjectPtr(PyObjectPtr):
    _typename = 'PyFrameObject'

    def __init__(self, gdbval, cast_to=None):
        PyObjectPtr.__init__(self, gdbval, cast_to)

        if not self.is_optimized_out():
            self._frame = PyFramePtr(self.field('f_frame'))

    def iter_locals(self):
        '''
        Yield a sequence of (name,value) pairs of PyObjectPtr instances, for
        the local variables of this frame
        '''
        if self.is_optimized_out():
            return
        return self._frame.iter_locals()

    def iter_globals(self):
        '''
        Yield a sequence of (name,value) pairs of PyObjectPtr instances, for
        the global variables of this frame
        '''
        if self.is_optimized_out():
            return ()
        return self._frame.iter_globals()

    def iter_builtins(self):
        '''
        Yield a sequence of (name,value) pairs of PyObjectPtr instances, for
        the builtin variables
        '''
        if self.is_optimized_out():
            return ()
        return self._frame.iter_builtins()

    def get_var_by_name(self, name):

        if self.is_optimized_out():
            return None, None
        return self._frame.get_var_by_name(name)

    def filename(self):
        '''Get the path of the current Python source file, as a string'''
        if self.is_optimized_out():
            return FRAME_INFO_OPTIMIZED_OUT
        return self._frame.filename()

    def current_line_num(self):
        '''Get current line number as an integer (1-based)

        Translated from PyFrame_GetLineNumber and PyCode_Addr2Line

        See Objects/lnotab_notes.txt
        '''
        if self.is_optimized_out():
            return None
        return self._frame.current_line_num()

    def current_line(self):
        '''Get the text of the current source line as a string, with a trailing
        newline character'''
        if self.is_optimized_out():
            return FRAME_INFO_OPTIMIZED_OUT
        return self._frame.current_line()

    def write_repr(self, out, visited):
        if self.is_optimized_out():
            out.write(FRAME_INFO_OPTIMIZED_OUT)
            return
        return self._frame.write_repr(out, visited)

    def print_traceback(self):
        if self.is_optimized_out():
            sys.stdout.write('  %s\n' % FRAME_INFO_OPTIMIZED_OUT)
            return
        return self._frame.print_traceback()

class PyFramePtr:

    def __init__(self, gdbval):
        self._gdbval = gdbval

        if not self.is_optimized_out():
            try:
                self.co = self._f_code()
                self.co_name = self.co.pyop_field('co_name')
                self.co_filename = self.co.pyop_field('co_filename')

                self.f_lasti = self._f_lasti()
                self.co_nlocals = int_from_int(self.co.field('co_nlocals'))
                pnames = self.co.field('co_localsplusnames')
                self.co_localsplusnames = PyTupleObjectPtr.from_pyobject_ptr(pnames)
                self._is_code = True
            except:
                self._is_code = False

    def is_optimized_out(self):
        return self._gdbval.is_optimized_out

    def iter_locals(self):
        '''
        Yield a sequence of (name,value) pairs of PyObjectPtr instances, for
        the local variables of this frame
        '''
        if self.is_optimized_out():
            return


        obj_ptr_ptr = gdb.lookup_type("PyObject").pointer().pointer()

        localsplus = self._gdbval["localsplus"].cast(obj_ptr_ptr)

        for i in safe_range(self.co_nlocals):
            pyop_value = PyObjectPtr.from_pyobject_ptr(localsplus[i])
            if pyop_value.is_null():
                continue
            pyop_name = PyObjectPtr.from_pyobject_ptr(self.co_localsplusnames[i])
            yield (pyop_name, pyop_value)

    def _f_special(self, name, convert=PyObjectPtr.from_pyobject_ptr):
        return convert(self._gdbval[name])

    def _f_globals(self):
        return self._f_special("f_globals")

    def _f_builtins(self):
        return self._f_special("f_builtins")

    def _f_code(self):
        return self._f_special("f_executable", PyCodeObjectPtr.from_pyobject_ptr)

    def _f_executable(self):
        return self._f_special("f_executable")

    def _f_nlocalsplus(self):
        return self._f_special("nlocalsplus", int_from_int)

    def _f_lasti(self):
        codeunit_p = gdb.lookup_type("_Py_CODEUNIT").pointer()
        instr_ptr = self._gdbval["instr_ptr"]
        first_instr = self._f_code().field("co_code_adaptive").cast(codeunit_p)
        return int(instr_ptr - first_instr)

    def is_shim(self):
        return self._f_special("owner", int) == FRAME_OWNED_BY_CSTACK

    def previous(self):
        return self._f_special("previous", PyFramePtr)

    def iter_globals(self):
        '''
        Yield a sequence of (name,value) pairs of PyObjectPtr instances, for
        the global variables of this frame
        '''
        if self.is_optimized_out():
            return ()

        pyop_globals = self._f_globals()
        return pyop_globals.iteritems()

    def iter_builtins(self):
        '''
        Yield a sequence of (name,value) pairs of PyObjectPtr instances, for
        the builtin variables
        '''
        if self.is_optimized_out():
            return ()

        pyop_builtins = self._f_builtins()
        return pyop_builtins.iteritems()

    def get_var_by_name(self, name):
        '''
        Look for the named local variable, returning a (PyObjectPtr, scope) pair
        where scope is a string 'local', 'global', 'builtin'

        If not found, return (None, None)
        '''
        for pyop_name, pyop_value in self.iter_locals():
            if name == pyop_name.proxyval(set()):
                return pyop_value, 'local'
        for pyop_name, pyop_value in self.iter_globals():
            if name == pyop_name.proxyval(set()):
                return pyop_value, 'global'
        for pyop_name, pyop_value in self.iter_builtins():
            if name == pyop_name.proxyval(set()):
                return pyop_value, 'builtin'
        return None, None

    def filename(self):
        '''Get the path of the current Python source file, as a string'''
        if self.is_optimized_out():
            return FRAME_INFO_OPTIMIZED_OUT
        return self.co_filename.proxyval(set())

    def current_line_num(self):
        '''Get current line number as an integer (1-based)

        Translated from PyFrame_GetLineNumber and PyCode_Addr2Line

        See Objects/lnotab_notes.txt
        '''
        if self.is_optimized_out():
            return None
        try:
            return self.co.addr2line(self.f_lasti)
        except Exception as ex:
            # bpo-34989: addr2line() is a complex function, it can fail in many
            # ways. For example, it fails with a TypeError on "FakeRepr" if
            # gdb fails to load debug symbols. Use a catch-all "except
            # Exception" to make the whole function safe. The caller has to
            # handle None anyway for optimized Python.
            return None

    def current_line(self):
        '''Get the text of the current source line as a string, with a trailing
        newline character'''
        if self.is_optimized_out():
            return FRAME_INFO_OPTIMIZED_OUT

        lineno = self.current_line_num()
        if lineno is None:
            return '(failed to get frame line number)'

        filename = self.filename()
        try:
            with open(os.fsencode(filename), 'r', encoding="utf-8") as fp:
                lines = fp.readlines()
        except IOError:
            return None

        try:
            # Convert from 1-based current_line_num to 0-based list offset
            return lines[lineno - 1]
        except IndexError:
            return None

    def write_repr(self, out, visited):
        if self.is_optimized_out():
            out.write(FRAME_INFO_OPTIMIZED_OUT)
            return
        lineno = self.current_line_num()
        lineno = str(lineno) if lineno is not None else "?"
        out.write('Frame 0x%x, for file %s, line %s, in %s ('
                  % (self.as_address(),
                     self.co_filename.proxyval(visited),
                     lineno,
                     self.co_name.proxyval(visited)))
        first = True
        for pyop_name, pyop_value in self.iter_locals():
            if not first:
                out.write(', ')
            first = False

            out.write(pyop_name.proxyval(visited))
            out.write('=')
            pyop_value.write_repr(out, visited)

        out.write(')')

    def as_address(self):
        return int(self._gdbval)

    def print_traceback(self):
        if self.is_optimized_out():
            sys.stdout.write('  %s\n' % FRAME_INFO_OPTIMIZED_OUT)
            return
        visited = set()
        lineno = self.current_line_num()
        lineno = str(lineno) if lineno is not None else "?"
        sys.stdout.write('  File "%s", line %s, in %s\n'
                  % (self.co_filename.proxyval(visited),
                     lineno,
                     self.co_name.proxyval(visited)))

    def get_truncated_repr(self, maxlen):
        '''
        Get a repr-like string for the data, but truncate it at "maxlen" bytes
        (ending the object graph traversal as soon as you do)
        '''
        out = TruncatedStringIO(maxlen)
        try:
            self.write_repr(out, set())
        except StringTruncated:
            # Truncation occurred:
            return out.getvalue() + '...(truncated)'

        # No truncation occurred:
        return out.getvalue()

class PySetObjectPtr(PyObjectPtr):
    _typename = 'PySetObject'

    @classmethod
    def _dummy_key(self):
        return gdb.lookup_global_symbol('_PySet_Dummy').value()

    def __iter__(self):
        dummy_ptr = self._dummy_key()
        table = self.field('table')
        for i in safe_range(self.field('mask') + 1):
            setentry = table[i]
            key = setentry['key']
            if key != 0 and key != dummy_ptr:
                yield PyObjectPtr.from_pyobject_ptr(key)

    def proxyval(self, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            return ProxyAlreadyVisited('%s(...)' % self.safe_tp_name())
        visited.add(self.as_address())

        members = (key.proxyval(visited) for key in self)
        if self.safe_tp_name() == 'frozenset':
            return frozenset(members)
        else:
            return set(members)

    def write_repr(self, out, visited):
        # Emulate Python's set_repr
        tp_name = self.safe_tp_name()

        # Guard against infinite loops:
        if self.as_address() in visited:
            out.write('(...)')
            return
        visited.add(self.as_address())

        # Python's set_repr special-cases the empty set:
        if not self.field('used'):
            out.write(tp_name)
            out.write('()')
            return

        # Python uses {} for set literals:
        if tp_name != 'set':
            out.write(tp_name)
            out.write('(')

        out.write('{')
        first = True
        for key in self:
            if not first:
                out.write(', ')
            first = False
            key.write_repr(out, visited)
        out.write('}')

        if tp_name != 'set':
            out.write(')')


class PyBytesObjectPtr(PyObjectPtr):
    _typename = 'PyBytesObject'

    def __str__(self):
        field_ob_size = self.field('ob_size')
        field_ob_sval = self.field('ob_sval')
        char_ptr = field_ob_sval.address.cast(_type_unsigned_char_ptr())
        return ''.join([chr(char_ptr[i]) for i in safe_range(field_ob_size)])

    def proxyval(self, visited):
        return str(self)

    def write_repr(self, out, visited):
        # Write this out as a Python bytes literal, i.e. with a "b" prefix

        # Get a PyStringObject* within the Python gdb process:
        proxy = self.proxyval(visited)

        # Transliteration of Python's Objects/bytesobject.c:PyBytes_Repr
        # to Python code:
        quote = "'"
        if "'" in proxy and not '"' in proxy:
            quote = '"'
        out.write('b')
        out.write(quote)
        for byte in proxy:
            if byte == quote or byte == '\\':
                out.write('\\')
                out.write(byte)
            elif byte == '\t':
                out.write('\\t')
            elif byte == '\n':
                out.write('\\n')
            elif byte == '\r':
                out.write('\\r')
            elif byte < ' ' or ord(byte) >= 0x7f:
                out.write('\\x')
                out.write(hexdigits[(ord(byte) & 0xf0) >> 4])
                out.write(hexdigits[ord(byte) & 0xf])
            else:
                out.write(byte)
        out.write(quote)

class PyTupleObjectPtr(PyObjectPtr):
    _typename = 'PyTupleObject'

    def __getitem__(self, i):
        # Get the gdb.Value for the (PyObject*) with the given index:
        field_ob_item = self.field('ob_item')
        return field_ob_item[i]

    def proxyval(self, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            return ProxyAlreadyVisited('(...)')
        visited.add(self.as_address())

        result = tuple(PyObjectPtr.from_pyobject_ptr(self[i]).proxyval(visited)
                       for i in safe_range(int_from_int(self.field('ob_size'))))
        return result

    def write_repr(self, out, visited):
        # Guard against infinite loops:
        if self.as_address() in visited:
            out.write('(...)')
            return
        visited.add(self.as_address())

        out.write('(')
        for i in safe_range(int_from_int(self.field('ob_size'))):
            if i > 0:
                out.write(', ')
            element = PyObjectPtr.from_pyobject_ptr(self[i])
            element.write_repr(out, visited)
        if self.field('ob_size') == 1:
            out.write(',)')
        else:
            out.write(')')

class PyTypeObjectPtr(PyObjectPtr):
    _typename = 'PyTypeObject'


def _unichr_is_printable(char):
    # Logic adapted from Python's Tools/unicode/makeunicodedata.py
    if char == u" ":
        return True
    import unicodedata
    return unicodedata.category(char) not in ("C", "Z")


class PyUnicodeObjectPtr(PyObjectPtr):
    _typename = 'PyUnicodeObject'

    def proxyval(self, visited):
        compact = self.field('_base')
        ascii = compact['_base']
        state = ascii['state']
        is_compact_ascii = (int(state['ascii']) and int(state['compact']))
        field_length = int(ascii['length'])
        if is_compact_ascii:
            field_str = ascii.address + 1
        elif int(state['compact']):
            field_str = compact.address + 1
        else:
            field_str = self.field('data')['any']
        repr_kind = int(state['kind'])
        if repr_kind == 1:
            field_str = field_str.cast(_type_unsigned_char_ptr())
        elif repr_kind == 2:
            field_str = field_str.cast(_type_unsigned_short_ptr())
        elif repr_kind == 4:
            field_str = field_str.cast(_type_unsigned_int_ptr())

        # Gather a list of ints from the code point array; these are either
        # UCS-1, UCS-2 or UCS-4 code points:
        code_points = [int(field_str[i]) for i in safe_range(field_length)]

        # Convert the int code points to unicode characters, and generate a
        # local unicode instance.
        result = ''.join(map(chr, code_points))
        return result

    def write_repr(self, out, visited):
        # Write this out as a Python str literal

        # Get a PyUnicodeObject* within the Python gdb process:
        proxy = self.proxyval(visited)

        # Transliteration of Python's Object/unicodeobject.c:unicode_repr
        # to Python:
        if "'" in proxy and '"' not in proxy:
            quote = '"'
        else:
            quote = "'"
        out.write(quote)

        i = 0
        while i < len(proxy):
            ch = proxy[i]
            i += 1

            # Escape quotes and backslashes
            if ch == quote or ch == '\\':
                out.write('\\')
                out.write(ch)

            #  Map special whitespace to '\t', \n', '\r'
            elif ch == '\t':
                out.write('\\t')
            elif ch == '\n':
                out.write('\\n')
            elif ch == '\r':
                out.write('\\r')

            # Map non-printable US ASCII to '\xhh' */
            elif ch < ' ' or ord(ch) == 0x7F:
                out.write('\\x')
                out.write(hexdigits[(ord(ch) >> 4) & 0x000F])
                out.write(hexdigits[ord(ch) & 0x000F])

            # Copy ASCII characters as-is
            elif ord(ch) < 0x7F:
                out.write(ch)

            # Non-ASCII characters
            else:
                ucs = ch
                ch2 = None

                printable = ucs.isprintable()
                if printable:
                    try:
                        ucs.encode(ENCODING)
                    except UnicodeEncodeError:
                        printable = False

                # Map Unicode whitespace and control characters
                # (categories Z* and C* except ASCII space)
                if not printable:
                    if ch2 is not None:
                        # Match Python's representation of non-printable
                        # wide characters.
                        code = (ord(ch) & 0x03FF) << 10
                        code |= ord(ch2) & 0x03FF
                        code += 0x00010000
                    else:
                        code = ord(ucs)

                    # Map 8-bit characters to '\\xhh'
                    if code <= 0xff:
                        out.write('\\x')
                        out.write(hexdigits[(code >> 4) & 0x000F])
                        out.write(hexdigits[code & 0x000F])
                    # Map 21-bit characters to '\U00xxxxxx'
                    elif code >= 0x10000:
                        out.write('\\U')
                        out.write(hexdigits[(code >> 28) & 0x0000000F])
                        out.write(hexdigits[(code >> 24) & 0x0000000F])
                        out.write(hexdigits[(code >> 20) & 0x0000000F])
                        out.write(hexdigits[(code >> 16) & 0x0000000F])
                        out.write(hexdigits[(code >> 12) & 0x0000000F])
                        out.write(hexdigits[(code >> 8) & 0x0000000F])
                        out.write(hexdigits[(code >> 4) & 0x0000000F])
                        out.write(hexdigits[code & 0x0000000F])
                    # Map 16-bit characters to '\uxxxx'
                    else:
                        out.write('\\u')
                        out.write(hexdigits[(code >> 12) & 0x000F])
                        out.write(hexdigits[(code >> 8) & 0x000F])
                        out.write(hexdigits[(code >> 4) & 0x000F])
                        out.write(hexdigits[code & 0x000F])
                else:
                    # Copy characters as-is
                    out.write(ch)
                    if ch2 is not None:
                        out.write(ch2)

        out.write(quote)


class wrapperobject(PyObjectPtr):
    _typename = 'wrapperobject'

    def safe_name(self):
        try:
            name = self.field('descr')['d_base']['name'].string()
            return repr(name)
        except (NullPyObjectPtr, RuntimeError, UnicodeDecodeError):
            return '<unknown name>'

    def safe_tp_name(self):
        try:
            return self.field('self')['ob_type']['tp_name'].string()
        except (NullPyObjectPtr, RuntimeError, UnicodeDecodeError):
            return '<unknown tp_name>'

    def safe_self_addresss(self):
        try:
            address = int(self.field('self'))
            return '%#x' % address
        except (NullPyObjectPtr, RuntimeError):
            return '<failed to get self address>'

    def proxyval(self, visited):
        name = self.safe_name()
        tp_name = self.safe_tp_name()
        self_address = self.safe_self_addresss()
        return ("<method-wrapper %s of %s object at %s>"
                % (name, tp_name, self_address))

    def write_repr(self, out, visited):
        proxy = self.proxyval(visited)
        out.write(proxy)


def int_from_int(gdbval):
    return int(gdbval)


def stringify(val):
    # TODO: repr() puts everything on one line; pformat can be nicer, but
    # can lead to v.long results; this function isolates the choice
    if True:
        return repr(val)
    else:
        from pprint import pformat
        return pformat(val)


class PyObjectPtrPrinter:
    "Prints a (PyObject*)"

    def __init__ (self, gdbval):
        self.gdbval = gdbval

    def to_string (self):
        pyop = PyObjectPtr.from_pyobject_ptr(self.gdbval)
        if True:
            return pyop.get_truncated_repr(MAX_OUTPUT_LEN)
        else:
            # Generate full proxy value then stringify it.
            # Doing so could be expensive
            proxyval = pyop.proxyval(set())
            return stringify(proxyval)

def pretty_printer_lookup(gdbval):
    type = gdbval.type.unqualified()
    if type.code != gdb.TYPE_CODE_PTR:
        return None

    type = type.target().unqualified()
    t = str(type)
    if t in ("PyObject", "PyFrameObject", "PyUnicodeObject", "wrapperobject"):
        return PyObjectPtrPrinter(gdbval)

"""
During development, I've been manually invoking the code in this way:
(gdb) python

import sys
sys.path.append('/home/david/coding/python-gdb')
import libpython
end

then reloading it after each edit like this:
(gdb) python reload(libpython)

The following code should ensure that the prettyprinter is registered
if the code is autoloaded by gdb when visiting libpython.so, provided
that this python file is installed to the same path as the library (or its
.debug file) plus a "-gdb.py" suffix, e.g:
  /usr/lib/libpython3.12.so.1.0-gdb.py
  /usr/lib/debug/usr/lib/libpython3.12.so.1.0.debug-gdb.py
"""
def register (obj):
    if obj is None:
        obj = gdb

    # Wire up the pretty-printer
    obj.pretty_printers.append(pretty_printer_lookup)

register (gdb.current_objfile ())



# Unfortunately, the exact API exposed by the gdb module varies somewhat
# from build to build
# See http://bugs.python.org/issue8279?#msg102276

class Frame(object):
    '''
    Wrapper for gdb.Frame, adding various methods
    '''
    def __init__(self, gdbframe):
        self._gdbframe = gdbframe

    def older(self):
        older = self._gdbframe.older()
        if older:
            return Frame(older)
        else:
            return None

    def newer(self):
        newer = self._gdbframe.newer()
        if newer:
            return Frame(newer)
        else:
            return None

    def select(self):
        '''If supported, select this frame and return True; return False if unsupported

        Not all builds have a gdb.Frame.select method; seems to be present on Fedora 12
        onwards, but absent on Ubuntu buildbot'''
        if not hasattr(self._gdbframe, 'select'):
            print ('Unable to select frame: '
                   'this build of gdb does not expose a gdb.Frame.select method')
            return False
        self._gdbframe.select()
        return True

    def get_index(self):
        '''Calculate index of frame, starting at 0 for the newest frame within
        this thread'''
        index = 0
        # Go down until you reach the newest frame:
        iter_frame = self
        while iter_frame.newer():
            index += 1
            iter_frame = iter_frame.newer()
        return index

    # We divide frames into:
    #   - "python frames":
    #       - "bytecode frames" i.e. PyEval_EvalFrameEx
    #       - "other python frames": things that are of interest from a python
    #         POV, but aren't bytecode (e.g. GC, GIL)
    #   - everything else

    def is_python_frame(self):
        '''Is this a _PyEval_EvalFrameDefault frame, or some other important
        frame? (see is_other_python_frame for what "important" means in this
        context)'''
        if self.is_evalframe():
            return True
        if self.is_other_python_frame():
            return True
        return False

    def is_evalframe(self):
        '''Is this a _PyEval_EvalFrameDefault frame?'''
        if self._gdbframe.name() == EVALFRAME:
            '''
            I believe we also need to filter on the inline
            struct frame_id.inline_depth, only regarding frames with
            an inline depth of 0 as actually being this function

            So we reject those with type gdb.INLINE_FRAME
            '''
            if self._gdbframe.type() == gdb.NORMAL_FRAME:
                # We have a _PyEval_EvalFrameDefault frame:
                return True

        return False

    def is_other_python_frame(self):
        '''Is this frame worth displaying in python backtraces?
        Examples:
          - waiting on the GIL
          - garbage-collecting
          - within a CFunction
         If it is, return a descriptive string
         For other frames, return False
         '''
        if self.is_waiting_for_gil():
            return 'Waiting for the GIL'

        if self.is_gc_collect():
            return 'Garbage-collecting'

        # Detect invocations of PyCFunction instances:
        frame = self._gdbframe
        caller = frame.name()
        if not caller:
            return False

        if (caller.startswith('cfunction_vectorcall_') or
            caller == 'cfunction_call'):
            arg_name = 'func'
            # Within that frame:
            #   "func" is the local containing the PyObject* of the
            # PyCFunctionObject instance
            #   "f" is the same value, but cast to (PyCFunctionObject*)
            #   "self" is the (PyObject*) of the 'self'
            try:
                # Use the prettyprinter for the func:
                func = frame.read_var(arg_name)
                return str(func)
            except ValueError:
                return ('PyCFunction invocation (unable to read %s: '
                        'missing debuginfos?)' % arg_name)
            except RuntimeError:
                return 'PyCFunction invocation (unable to read %s)' % arg_name

        if caller == 'wrapper_call':
            arg_name = 'wp'
            try:
                func = frame.read_var(arg_name)
                return str(func)
            except ValueError:
                return ('<wrapper_call invocation (unable to read %s: '
                        'missing debuginfos?)>' % arg_name)
            except RuntimeError:
                return '<wrapper_call invocation (unable to read %s)>' % arg_name

        # This frame isn't worth reporting:
        return False

    def is_waiting_for_gil(self):
        '''Is this frame waiting on the GIL?'''
        # This assumes the _POSIX_THREADS version of Python/ceval_gil.c:
        name = self._gdbframe.name()
        if name:
            return (name == 'take_gil')

    def is_gc_collect(self):
        '''Is this frame gc_collect_main() within the garbage-collector?'''
        return self._gdbframe.name() in ('collect', 'gc_collect_main')

    def get_pyop(self):
        try:
            frame = self._gdbframe.read_var('frame')
            frame = PyFramePtr(frame)
            if not frame.is_optimized_out():
                return frame
            cframe = self._gdbframe.read_var('cframe')
            if cframe is None:
                return None
            frame = PyFramePtr(cframe["current_frame"])
            if frame and not frame.is_optimized_out():
                return frame
            return None
        except ValueError:
            return None

    @classmethod
    def get_selected_frame(cls):
        _gdbframe = gdb.selected_frame()
        if _gdbframe:
            return Frame(_gdbframe)
        return None

    @classmethod
    def get_selected_python_frame(cls):
        '''Try to obtain the Frame for the python-related code in the selected
        frame, or None'''
        try:
            frame = cls.get_selected_frame()
        except gdb.error:
            # No frame: Python didn't start yet
            return None

        while frame:
            if frame.is_python_frame():
                return frame
            frame = frame.older()

        # Not found:
        return None

    @classmethod
    def get_selected_bytecode_frame(cls):
        '''Try to obtain the Frame for the python bytecode interpreter in the
        selected GDB frame, or None'''
        frame = cls.get_selected_frame()

        while frame:
            if frame.is_evalframe():
                return frame
            frame = frame.older()

        # Not found:
        return None

    def print_summary(self):
        if self.is_evalframe():
            interp_frame = self.get_pyop()
            while True:
                if interp_frame:
                    if interp_frame.is_shim():
                        break
                    line = interp_frame.get_truncated_repr(MAX_OUTPUT_LEN)
                    sys.stdout.write('#%i %s\n' % (self.get_index(), line))
                    if not interp_frame.is_optimized_out():
                        line = interp_frame.current_line()
                        if line is not None:
                            sys.stdout.write('    %s\n' % line.strip())
                else:
                    sys.stdout.write('#%i (unable to read python frame information)\n' % self.get_index())
                    break
                interp_frame = interp_frame.previous()
        else:
            info = self.is_other_python_frame()
            if info:
                sys.stdout.write('#%i %s\n' % (self.get_index(), info))
            else:
                sys.stdout.write('#%i\n' % self.get_index())

    def print_traceback(self):
        if self.is_evalframe():
            interp_frame = self.get_pyop()
            while True:
                if interp_frame:
                    if interp_frame.is_shim():
                        break
                    interp_frame.print_traceback()
                    if not interp_frame.is_optimized_out():
                        line = interp_frame.current_line()
                        if line is not None:
                            sys.stdout.write('    %s\n' % line.strip())
                else:
                    sys.stdout.write('  (unable to read python frame information)\n')
                    break
                interp_frame = interp_frame.previous()
        else:
            info = self.is_other_python_frame()
            if info:
                sys.stdout.write('  %s\n' % info)
            else:
                sys.stdout.write('  (not a python frame)\n')

class PyList(gdb.Command):
    '''List the current Python source code, if any

    Use
       py-list START
    to list at a different line number within the python source.

    Use
       py-list START, END
    to list a specific range of lines within the python source.
    '''

    def __init__(self):
        gdb.Command.__init__ (self,
                              "py-list",
                              gdb.COMMAND_FILES,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        import re

        start = None
        end = None

        m = re.match(r'\s*(\d+)\s*', args)
        if m:
            start = int(m.group(0))
            end = start + 10

        m = re.match(r'\s*(\d+)\s*,\s*(\d+)\s*', args)
        if m:
            start, end = map(int, m.groups())

        # py-list requires an actual PyEval_EvalFrameEx frame:
        frame = Frame.get_selected_bytecode_frame()
        if not frame:
            print('Unable to locate gdb frame for python bytecode interpreter')
            return

        pyop = frame.get_pyop()
        if not pyop or pyop.is_optimized_out():
            print(UNABLE_READ_INFO_PYTHON_FRAME)
            return

        filename = pyop.filename()
        lineno = pyop.current_line_num()
        if lineno is None:
            print('Unable to read python frame line number')
            return

        if start is None:
            start = lineno - 5
            end = lineno + 5

        if start<1:
            start = 1

        try:
            f = open(os.fsencode(filename), 'r', encoding="utf-8")
        except IOError as err:
            sys.stdout.write('Unable to open %s: %s\n'
                             % (filename, err))
            return
        with f:
            all_lines = f.readlines()
            # start and end are 1-based, all_lines is 0-based;
            # so [start-1:end] as a python slice gives us [start, end] as a
            # closed interval
            for i, line in enumerate(all_lines[start-1:end]):
                linestr = str(i+start)
                # Highlight current line:
                if i + start == lineno:
                    linestr = '>' + linestr
                sys.stdout.write('%4s    %s' % (linestr, line))


# ...and register the command:
PyList()

def move_in_stack(move_up):
    '''Move up or down the stack (for the py-up/py-down command)'''
    # Important:
    # The amount of frames that are printed out depends on how many frames are inlined
    # in the same evaluation loop. As this command links directly the C stack with the
    # Python stack, the results are sensitive to the number of inlined frames and this
    # is likely to change between versions and optimizations.
    frame = Frame.get_selected_python_frame()
    if not frame:
        print('Unable to locate python frame')
        return
    while frame:
        if move_up:
            iter_frame = frame.older()
        else:
            iter_frame = frame.newer()

        if not iter_frame:
            break

        if iter_frame.is_python_frame():
            # Result:
            if iter_frame.select():
                iter_frame.print_summary()
            return

        frame = iter_frame

    if move_up:
        print('Unable to find an older python frame')
    else:
        print('Unable to find a newer python frame')


class PyUp(gdb.Command):
    'Select and print all python stack frame in the same eval loop starting from the one that called this one (if any)'
    def __init__(self):
        gdb.Command.__init__ (self,
                              "py-up",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        move_in_stack(move_up=True)

class PyDown(gdb.Command):
    'Select and print all python stack frame in the same eval loop starting from the one called this one (if any)'
    def __init__(self):
        gdb.Command.__init__ (self,
                              "py-down",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        move_in_stack(move_up=False)

# Not all builds of gdb have gdb.Frame.select
if hasattr(gdb.Frame, 'select'):
    PyUp()
    PyDown()

class PyBacktraceFull(gdb.Command):
    'Display the current python frame and all the frames within its call stack (if any)'
    def __init__(self):
        gdb.Command.__init__ (self,
                              "py-bt-full",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        frame = Frame.get_selected_python_frame()
        if not frame:
            print('Unable to locate python frame')
            return

        while frame:
            if frame.is_python_frame():
                frame.print_summary()
            frame = frame.older()

PyBacktraceFull()

class PyBacktrace(gdb.Command):
    'Display the current python frame and all the frames within its call stack (if any)'
    def __init__(self):
        gdb.Command.__init__ (self,
                              "py-bt",
                              gdb.COMMAND_STACK,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        frame = Frame.get_selected_python_frame()
        if not frame:
            print('Unable to locate python frame')
            return

        sys.stdout.write('Traceback (most recent call first):\n')
        while frame:
            if frame.is_python_frame():
                frame.print_traceback()
            frame = frame.older()

PyBacktrace()

class PyPrint(gdb.Command):
    'Look up the given python variable name, and print it'
    def __init__(self):
        gdb.Command.__init__ (self,
                              "py-print",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        name = str(args)

        frame = Frame.get_selected_python_frame()
        if not frame:
            print('Unable to locate python frame')
            return

        pyop_frame = frame.get_pyop()
        if not pyop_frame:
            print(UNABLE_READ_INFO_PYTHON_FRAME)
            return

        pyop_var, scope = pyop_frame.get_var_by_name(name)

        if pyop_var:
            print('%s %r = %s'
                   % (scope,
                      name,
                      pyop_var.get_truncated_repr(MAX_OUTPUT_LEN)))
        else:
            print('%r not found' % name)

PyPrint()

class PyLocals(gdb.Command):
    'Look up the given python variable name, and print it'
    def __init__(self):
        gdb.Command.__init__ (self,
                              "py-locals",
                              gdb.COMMAND_DATA,
                              gdb.COMPLETE_NONE)


    def invoke(self, args, from_tty):
        name = str(args)

        frame = Frame.get_selected_python_frame()
        if not frame:
            print('Unable to locate python frame')
            return

        pyop_frame = frame.get_pyop()
        while True:
            if not pyop_frame:
                print(UNABLE_READ_INFO_PYTHON_FRAME)
                break
            if pyop_frame.is_shim():
                break

            sys.stdout.write('Locals for %s\n' % (pyop_frame.co_name.proxyval(set())))

            for pyop_name, pyop_value in pyop_frame.iter_locals():
                print('%s = %s'
                    % (pyop_name.proxyval(set()),
                        pyop_value.get_truncated_repr(MAX_OUTPUT_LEN)))


            pyop_frame = pyop_frame.previous()

PyLocals()
