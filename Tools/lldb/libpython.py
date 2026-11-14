"""CPython object summaries and py-* commands for LLDB.

Load with ``command script import /path/to/python_lldb.py``.  This adapter
implements the GDB API subset used by Tools/gdb/libpython.py, so both
debuggers use the same object layouts, output limits, and commands.

Based on the original LLDB extension for CPython,
https://github.com/malor/cpython-lldb.
"""

import contextlib
import contextvars
import functools
import importlib.util
import io
import os
import sys
import types

import lldb


class Type:
    def __init__(self, sbtype):
        if not sbtype.IsValid():
            raise RuntimeError('Type information is unavailable')
        self.sbtype = sbtype

    @property
    def name(self):
        return self.sbtype.GetName()

    @property
    def sizeof(self):
        return self.sbtype.GetByteSize()

    def pointer(self):
        return Type(self.sbtype.GetPointerType())

    def fields(self):
        return self.sbtype.fields


@functools.total_ordering
class Value:
    def __init__(self, backend, sbvalue):
        if not sbvalue.IsValid():
            raise RuntimeError(str(sbvalue.GetError()))
        self.backend = backend
        self.sbvalue = sbvalue.GetNonSyntheticValue()

    @property
    def type(self):
        return Type(self.sbvalue.GetType())

    @property
    def address(self):
        return Value(self.backend, self.sbvalue.AddressOf())

    @property
    def is_optimized_out(self):
        return self.sbvalue.GetError().Fail()

    def cast(self, type):
        # GDB decays an array to a pointer when it is cast.
        if self.sbvalue.GetType().IsArrayType():
            return self.address.cast(type)
        return Value(self.backend, self.sbvalue.Cast(type.sbtype))

    def dereference(self):
        return Value(self.backend, self.sbvalue.Dereference())

    def __getitem__(self, key):
        if isinstance(key, str):
            value = self.sbvalue.GetChildMemberWithName(key)
        else:
            # CPython uses flexible arrays and arrays declared with one item.
            # Compute their addresses without LLDB's declared array bounds.
            type = self.sbvalue.GetType()
            if type.IsPointerType():
                element = type.GetPointeeType()
                address = int(self)
            elif type.IsArrayType():
                element = type.GetArrayElementType()
                address = self.sbvalue.GetLoadAddress()
            else:
                raise TypeError(f'Cannot index {type.GetName()}')
            address += int(key) * element.GetByteSize()
            target = self.sbvalue.GetTarget()
            value = target.CreateValueFromAddress(
                'item', lldb.SBAddress(address, target), element
            )
        return Value(self.backend, value)

    def __int__(self):
        error = lldb.SBError()
        if self.sbvalue.GetType().GetTypeFlags() & lldb.eTypeIsSigned:
            value = self.sbvalue.GetValueAsSigned(error)
        else:
            value = self.sbvalue.GetValueAsUnsigned(error)
        if error.Fail():
            raise RuntimeError(str(error))
        return value

    __index__ = __int__

    def __bool__(self):
        return bool(int(self))

    def __eq__(self, other):
        return int(self) == int(other)

    def __lt__(self, other):
        return int(self) < int(other)

    def __add__(self, other):
        if self.sbvalue.GetType().IsPointerType():
            size = self.sbvalue.GetType().GetPointeeType().GetByteSize()
            return self.backend.Value(int(self) + int(other) * size).cast(
                self.type
            )
        return self.backend.Value(int(self) + int(other))

    __radd__ = __add__

    def __sub__(self, other):
        # The GDB helpers only subtract pointers.
        size = self.sbvalue.GetType().GetPointeeType().GetByteSize()
        return (int(self) - int(other)) // size

    def __mul__(self, other):
        return self.backend.Value(int(self) * int(other))

    __rmul__ = __mul__

    def __and__(self, other):
        return self.backend.Value(int(self) & int(other))

    def __rshift__(self, other):
        return self.backend.Value(int(self) >> int(other))

    def __mod__(self, other):
        return self.backend.Value(int(self) % int(other))

    def string(self):
        error = lldb.SBError()
        text = self.sbvalue.GetProcess().ReadCStringFromMemory(
            int(self), 1024, error
        )
        if error.Fail():
            raise RuntimeError(str(error))
        return text

    def __str__(self):
        return self.backend.libpython.PyObjectPtrPrinter(self).to_string()


class Frame:
    def __init__(self, backend, sbframe, index=None):
        self.backend = backend
        self.sbframe = sbframe
        self.index = sbframe.GetFrameID() if index is None else index

    def _relative(self, offset):
        thread = self.sbframe.GetThread()
        # Artificial tail-call frames can share an SBFrame with an earlier
        # frame. Keep the traversal index: GetFrameID() can go backwards.
        index = self.index + offset
        if 0 <= index < thread.GetNumFrames():
            return Frame(self.backend, thread.GetFrameAtIndex(index), index)
        return None

    def older(self):
        return self._relative(1)

    def newer(self):
        return self._relative(-1)

    def select(self):
        self.sbframe.GetThread().SetSelectedFrame(self.index)

    def name(self):
        return self.sbframe.GetFunctionName()

    def type(self):
        return int(self.sbframe.IsInlined())

    def read_var(self, name):
        value = self.sbframe.FindVariable(name)
        if not value.IsValid():
            raise ValueError(f'Variable {name!r} is unavailable')
        return Value(self.backend, value)


class Backend:
    """The debugger operations used by the shared CPython helpers."""

    error = RuntimeError
    NORMAL_FRAME = 0
    COMMAND_FILES = COMMAND_STACK = COMMAND_DATA = COMPLETE_NONE = 0
    Frame = Frame

    def __init__(self, path, target=None):
        self.target = target
        self._thread = contextvars.ContextVar('lldb_thread', default=None)
        self.pretty_printers = []
        self.commands = {}
        commands = self.commands

        class Command:
            def __init__(self, name, *unused):
                commands[name] = self

        self.Command = Command
        spec = importlib.util.spec_from_file_location(
            '_cpython_debugger', path
        )
        self.libpython = importlib.util.module_from_spec(spec)
        # Each target gets its own copy of the GDB helpers, with this object
        # as their gdb module.
        sys.modules['gdb'] = self
        try:
            spec.loader.exec_module(self.libpython)
        finally:
            del sys.modules['gdb']

    @contextlib.contextmanager
    def context(self, thread):
        # LLDB can request summaries on different host threads. Keep each
        # request's selected thread separate, including nested callbacks.
        token = self._thread.set(thread)
        try:
            yield
        finally:
            self._thread.reset(token)

    def current_objfile(self):
        return self

    def host_charset(self):
        return 'utf-8'

    def lookup_type(self, name):
        return Type(self.target.FindFirstType(name))

    def Value(self, integer):
        data = lldb.SBData.CreateDataFromUInt64Array(
            self.target.GetByteOrder(),
            self.target.GetAddressByteSize(),
            [int(integer) & ((1 << 64) - 1)],
        )
        type = self.target.GetBasicType(lldb.eBasicTypeUnsignedLongLong)
        return Value(
            self, self.target.CreateValueFromData('integer', data, type)
        )

    def lookup_global_symbol(self, name):
        value = Value(self, self.target.FindFirstGlobalVariable(name))
        return types.SimpleNamespace(value=lambda: value)

    def selected_frame(self):
        thread = self._thread.get()
        if thread is None or not thread.IsValid():
            return None
        frame = thread.GetSelectedFrame()
        return Frame(self, frame) if frame.IsValid() else None

    def parse_and_eval(self, expression):
        frame = self.selected_frame()
        if frame is None:
            raise RuntimeError('No frame is selected')
        options = lldb.SBExpressionOptions()
        # Helpers must also work on core files. Never call into the inferior.
        options.SetAllowJIT(False)
        options.SetFetchDynamicValue(lldb.eNoDynamicValues)
        options.SetSuppressPersistentResult(True)
        value = frame.sbframe.EvaluateExpression(expression, options)
        if value.GetError().Fail():
            raise RuntimeError(str(value.GetError()))
        return Value(self, value)


_backends = {}


def _get_backend(target):
    if not target.IsValid():
        return _backend
    # Type caches belong to one process. A debugger can have several targets,
    # including normal and free-threaded Python builds, or restart a target.
    key = target.GetDebugger().GetID(), target.GetProcess().GetUniqueID()
    if key not in _backends:
        _backends[key] = Backend(_backend.libpython.__file__, target)
    return _backends[key]


def pretty_printer(value, internal_dict):
    backend = _get_backend(value.GetTarget())
    with backend.context(value.GetThread()):
        if (
            not value.GetType().IsPointerType()
            and value.GetTypeName() != '_PyStackRef'
        ):
            value = value.AddressOf()
        try:
            return backend.libpython.PyObjectPtrPrinter(
                Value(backend, value)
            ).to_string()
        except (RuntimeError, ValueError, TypeError, OverflowError):
            return '<unable to read Python object>'


def _command_callback(name, command):
    def callback(debugger, arguments, exe_ctx, result, internal_dict):
        output = io.StringIO()
        backend = _get_backend(exe_ctx.GetTarget())
        with backend.context(exe_ctx.GetThread()):
            try:
                with contextlib.redirect_stdout(output):
                    backend.commands[name].invoke(arguments, False)
            except Exception as error:
                result.SetError(str(error))
            result.PutCString(output.getvalue().rstrip('\n'))

    callback.__doc__ = command.__doc__
    return callback


def __lldb_init_module(debugger, internal_dict):
    global _backend
    directory = os.path.dirname(os.path.abspath(__file__))
    # The build puts both scripts beside the executable. Also support a direct
    # import from Tools/lldb in the source tree.
    for name in ('python-gdb.py', 'python.exe-gdb.py', '../gdb/libpython.py'):
        path = os.path.join(directory, name)
        if os.path.isfile(path):
            break
    _backend = Backend(path)

    for name, command in _backend.commands.items():
        function = name.replace('-', '_')
        globals()[function] = _command_callback(name, command)
        debugger.HandleCommand(
            f'command script add -o -f {__name__}.{function} {name}'
        )

    category = debugger.GetCategory('CPython')
    if not category.IsValid():
        category = debugger.CreateCategory('CPython')
    summary = lldb.SBTypeSummary.CreateWithFunctionName(
        f'{__name__}.pretty_printer', lldb.eTypeOptionSkipPointers
    )
    # Summarize the PyObject struct tag and every concrete object type.
    names = {'_object'}
    for cls in vars(_backend.libpython).values():
        if isinstance(cls, type) and issubclass(
            cls, _backend.libpython.PyObjectPtr
        ):
            names.add(cls._typename)
    for name in names:
        category.AddTypeSummary(lldb.SBTypeNameSpecifier(name), summary)
        category.AddTypeSummary(lldb.SBTypeNameSpecifier(name + ' *'), summary)
    category.AddTypeSummary(lldb.SBTypeNameSpecifier('_PyStackRef'), summary)
    category.SetEnabled(True)
