from __future__ import annotations
import dataclasses as dc
import io
import os
from typing import Final, TYPE_CHECKING

import libclinic
from libclinic import fail
from libclinic.language import Language
from libclinic.block_parser import Block
if TYPE_CHECKING:
    from libclinic.app import Clinic


TemplateDict = dict[str, str]


class CRenderData:
    def __init__(self) -> None:

        # The C statements to declare variables.
        # Should be full lines with \n eol characters.
        self.declarations: list[str] = []

        # The C statements required to initialize the variables before the parse call.
        # Should be full lines with \n eol characters.
        self.initializers: list[str] = []

        # The C statements needed to dynamically modify the values
        # parsed by the parse call, before calling the impl.
        self.modifications: list[str] = []

        # The entries for the "keywords" array for PyArg_ParseTuple.
        # Should be individual strings representing the names.
        self.keywords: list[str] = []

        # The "format units" for PyArg_ParseTuple.
        # Should be individual strings that will get
        self.format_units: list[str] = []

        # The varargs arguments for PyArg_ParseTuple.
        self.parse_arguments: list[str] = []

        # The parameter declarations for the impl function.
        self.impl_parameters: list[str] = []

        # The arguments to the impl function at the time it's called.
        self.impl_arguments: list[str] = []

        # For return converters: the name of the variable that
        # should receive the value returned by the impl.
        self.return_value = "return_value"

        # For return converters: the code to convert the return
        # value from the parse function.  This is also where
        # you should check the _return_value for errors, and
        # "goto exit" if there are any.
        self.return_conversion: list[str] = []
        self.converter_retval = "_return_value"

        # The C statements required to do some operations
        # after the end of parsing but before cleaning up.
        # These operations may be, for example, memory deallocations which
        # can only be done without any error happening during argument parsing.
        self.post_parsing: list[str] = []

        # The C statements required to clean up after the impl call.
        self.cleanup: list[str] = []

        # The C statements to generate critical sections (per-object locking).
        self.lock: list[str] = []
        self.unlock: list[str] = []


@dc.dataclass(slots=True, frozen=True)
class Include:
    """
    An include like: #include "pycore_long.h"   // _Py_ID()
    """
    # Example: "pycore_long.h".
    filename: str

    # Example: "_Py_ID()".
    reason: str

    # None means unconditional include.
    # Example: "#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)".
    condition: str | None

    def sort_key(self) -> tuple[str, str]:
        # order: '#if' comes before 'NO_CONDITION'
        return (self.condition or 'NO_CONDITION', self.filename)


@dc.dataclass(slots=True)
class BlockPrinter:
    language: Language
    f: io.StringIO = dc.field(default_factory=io.StringIO)

    # '#include "header.h"   // reason': column of '//' comment
    INCLUDE_COMMENT_COLUMN: Final[int] = 35

    def print_block(
        self,
        block: Block,
        *,
        header_includes: list[Include] | None = None,
    ) -> None:
        input = block.input
        output = block.output
        dsl_name = block.dsl_name
        write = self.f.write

        assert not ((dsl_name is None) ^ (output is None)), "you must specify dsl_name and output together, dsl_name " + repr(dsl_name)

        if not dsl_name:
            write(input)
            return

        write(self.language.start_line.format(dsl_name=dsl_name))
        write("\n")

        body_prefix = self.language.body_prefix.format(dsl_name=dsl_name)
        if not body_prefix:
            write(input)
        else:
            for line in input.split('\n'):
                write(body_prefix)
                write(line)
                write("\n")

        write(self.language.stop_line.format(dsl_name=dsl_name))
        write("\n")

        output = ''
        if header_includes:
            # Emit optional "#include" directives for C headers
            output += '\n'

            current_condition: str | None = None
            for include in header_includes:
                if include.condition != current_condition:
                    if current_condition:
                        output += '#endif\n'
                    current_condition = include.condition
                    if include.condition:
                        output += f'{include.condition}\n'

                if current_condition:
                    line = f'#  include "{include.filename}"'
                else:
                    line = f'#include "{include.filename}"'
                if include.reason:
                    comment = f'// {include.reason}\n'
                    line = line.ljust(self.INCLUDE_COMMENT_COLUMN - 1) + comment
                output += line

            if current_condition:
                output += '#endif\n'

        input = ''.join(block.input)
        output += ''.join(block.output)
        if output:
            if not output.endswith('\n'):
                output += '\n'
            write(output)

        arguments = "output={output} input={input}".format(
            output=libclinic.compute_checksum(output, 16),
            input=libclinic.compute_checksum(input, 16)
        )
        write(self.language.checksum_line.format(dsl_name=dsl_name, arguments=arguments))
        write("\n")

    def write(self, text: str) -> None:
        self.f.write(text)


class BufferSeries:
    """
    Behaves like a "defaultlist".
    When you ask for an index that doesn't exist yet,
    the object grows the list until that item exists.
    So o[n] will always work.

    Supports negative indices for actual items.
    e.g. o[-1] is an element immediately preceding o[0].
    """

    def __init__(self) -> None:
        self._start = 0
        self._array: list[list[str]] = []

    def __getitem__(self, i: int) -> list[str]:
        i -= self._start
        if i < 0:
            self._start += i
            prefix: list[list[str]] = [[] for x in range(-i)]
            self._array = prefix + self._array
            i = 0
        while i >= len(self._array):
            self._array.append([])
        return self._array[i]

    def clear(self) -> None:
        for ta in self._array:
            ta.clear()

    def dump(self) -> str:
        texts = ["".join(ta) for ta in self._array]
        self.clear()
        return "".join(texts)


@dc.dataclass(slots=True, repr=False)
class Destination:
    name: str
    type: str
    clinic: Clinic
    buffers: BufferSeries = dc.field(init=False, default_factory=BufferSeries)
    filename: str = dc.field(init=False)  # set in __post_init__

    args: dc.InitVar[tuple[str, ...]] = ()

    def __post_init__(self, args: tuple[str, ...]) -> None:
        valid_types = ('buffer', 'file', 'suppress')
        if self.type not in valid_types:
            fail(
                f"Invalid destination type {self.type!r} for {self.name}, "
                f"must be {', '.join(valid_types)}"
            )
        extra_arguments = 1 if self.type == "file" else 0
        if len(args) < extra_arguments:
            fail(f"Not enough arguments for destination "
                 f"{self.name!r} new {self.type!r}")
        if len(args) > extra_arguments:
            fail(f"Too many arguments for destination {self.name!r} new {self.type!r}")
        if self.type =='file':
            d = {}
            filename = self.clinic.filename
            d['path'] = filename
            dirname, basename = os.path.split(filename)
            if not dirname:
                dirname = '.'
            d['dirname'] = dirname
            d['basename'] = basename
            d['basename_root'], d['basename_extension'] = os.path.splitext(filename)
            self.filename = args[0].format_map(d)

    def __repr__(self) -> str:
        if self.type == 'file':
            type_repr = f"type='file' file={self.filename!r}"
        else:
            type_repr = f"type={self.type!r}"
        return f"<clinic.Destination {self.name!r} {type_repr}>"

    def clear(self) -> None:
        if self.type != 'buffer':
            fail(f"Can't clear destination {self.name!r}: it's not of type 'buffer'")
        self.buffers.clear()

    def dump(self) -> str:
        return self.buffers.dump()


DestinationDict = dict[str, Destination]


class CodeGen:
    def __init__(self, limited_capi: bool) -> None:
        self.limited_capi = limited_capi
        self._ifndef_symbols: set[str] = set()
        # dict: include name => Include instance
        self._includes: dict[str, Include] = {}

    def add_ifndef_symbol(self, name: str) -> bool:
        if name in self._ifndef_symbols:
            return False
        self._ifndef_symbols.add(name)
        return True

    def add_include(self, name: str, reason: str,
                    *, condition: str | None = None) -> None:
        try:
            existing = self._includes[name]
        except KeyError:
            pass
        else:
            if existing.condition and not condition:
                # If the previous include has a condition and the new one is
                # unconditional, override the include.
                pass
            else:
                # Already included, do nothing. Only mention a single reason,
                # no need to list all of them.
                return

        self._includes[name] = Include(name, reason, condition)

    def get_includes(self) -> list[Include]:
        return sorted(self._includes.values(),
                      key=Include.sort_key)
