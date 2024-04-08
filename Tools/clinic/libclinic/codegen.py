from __future__ import annotations
import dataclasses as dc
import io
import os
from typing import Final, TYPE_CHECKING

import libclinic
from libclinic import fail
from libclinic.crenderdata import Include
from libclinic.language import Language
from libclinic.block_parser import Block
if TYPE_CHECKING:
    from libclinic.app import Clinic


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
        core_includes: bool = False,
        limited_capi: bool,
        header_includes: dict[str, Include],
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
        if core_includes and header_includes:
            # Emit optional "#include" directives for C headers
            output += '\n'

            current_condition: str | None = None
            includes = sorted(header_includes.values(), key=Include.sort_key)
            for include in includes:
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
