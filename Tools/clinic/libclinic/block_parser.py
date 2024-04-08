from __future__ import annotations
import collections
import dataclasses as dc
import re
import shlex
from typing import Any

import libclinic
from libclinic import fail, ClinicError
from libclinic.language import Language
from libclinic.function import (
    Module, Class, Function)


@dc.dataclass(slots=True, repr=False)
class Block:
    r"""
    Represents a single block of text embedded in
    another file.  If dsl_name is None, the block represents
    verbatim text, raw original text from the file, in
    which case "input" will be the only non-false member.
    If dsl_name is not None, the block represents a Clinic
    block.

    input is always str, with embedded \n characters.
    input represents the original text from the file;
    if it's a Clinic block, it is the original text with
    the body_prefix and redundant leading whitespace removed.

    dsl_name is either str or None.  If str, it's the text
    found on the start line of the block between the square
    brackets.

    signatures is a list.
    It may only contain clinic.Module, clinic.Class, and
    clinic.Function objects.  At the moment it should
    contain at most one of each.

    output is either str or None.  If str, it's the output
    from this block, with embedded '\n' characters.

    indent is a str.  It's the leading whitespace
    that was found on every line of input.  (If body_prefix is
    not empty, this is the indent *after* removing the
    body_prefix.)

    "indent" is different from the concept of "preindent"
    (which is not stored as state on Block objects).
    "preindent" is the whitespace that
    was found in front of every line of input *before* the
    "body_prefix" (see the Language object).  If body_prefix
    is empty, preindent must always be empty too.

    To illustrate the difference between "indent" and "preindent":

    Assume that '_' represents whitespace.
    If the block processed was in a Python file, and looked like this:
      ____#/*[python]
      ____#__for a in range(20):
      ____#____print(a)
      ____#[python]*/
    "preindent" would be "____" and "indent" would be "__".

    """
    input: str
    dsl_name: str | None = None
    signatures: list[Module | Class | Function] = dc.field(default_factory=list)
    output: Any = None  # TODO: Very dynamic; probably untypeable in its current form?
    indent: str = ''

    def __repr__(self) -> str:
        dsl_name = self.dsl_name or "text"
        def summarize(s: object) -> str:
            s = repr(s)
            if len(s) > 30:
                return s[:26] + "..." + s[0]
            return s
        parts = (
            repr(dsl_name),
            f"input={summarize(self.input)}",
            f"output={summarize(self.output)}"
        )
        return f"<clinic.Block {' '.join(parts)}>"


class BlockParser:
    """
    Block-oriented parser for Argument Clinic.
    Iterator, yields Block objects.
    """

    def __init__(
            self,
            input: str,
            language: Language,
            *,
            verify: bool = True
    ) -> None:
        """
        "input" should be a str object
        with embedded \n characters.

        "language" should be a Language object.
        """
        language.validate()

        self.input = collections.deque(reversed(input.splitlines(keepends=True)))
        self.block_start_line_number = self.line_number = 0

        self.language = language
        before, _, after = language.start_line.partition('{dsl_name}')
        assert _ == '{dsl_name}'
        self.find_start_re = libclinic.create_regex(before, after,
                                                    whole_line=False)
        self.start_re = libclinic.create_regex(before, after)
        self.verify = verify
        self.last_checksum_re: re.Pattern[str] | None = None
        self.last_dsl_name: str | None = None
        self.dsl_name: str | None = None
        self.first_block = True

    def __iter__(self) -> BlockParser:
        return self

    def __next__(self) -> Block:
        while True:
            if not self.input:
                raise StopIteration

            if self.dsl_name:
                try:
                    return_value = self.parse_clinic_block(self.dsl_name)
                except ClinicError as exc:
                    exc.filename = self.language.filename
                    exc.lineno = self.line_number
                    raise
                self.dsl_name = None
                self.first_block = False
                return return_value
            block = self.parse_verbatim_block()
            if self.first_block and not block.input:
                continue
            self.first_block = False
            return block


    def is_start_line(self, line: str) -> str | None:
        match = self.start_re.match(line.lstrip())
        return match.group(1) if match else None

    def _line(self, lookahead: bool = False) -> str:
        self.line_number += 1
        line = self.input.pop()
        if not lookahead:
            self.language.parse_line(line)
        return line

    def parse_verbatim_block(self) -> Block:
        lines = []
        self.block_start_line_number = self.line_number

        while self.input:
            line = self._line()
            dsl_name = self.is_start_line(line)
            if dsl_name:
                self.dsl_name = dsl_name
                break
            lines.append(line)

        return Block("".join(lines))

    def parse_clinic_block(self, dsl_name: str) -> Block:
        in_lines = []
        self.block_start_line_number = self.line_number + 1
        stop_line = self.language.stop_line.format(dsl_name=dsl_name)
        body_prefix = self.language.body_prefix.format(dsl_name=dsl_name)

        def is_stop_line(line: str) -> bool:
            # make sure to recognize stop line even if it
            # doesn't end with EOL (it could be the very end of the file)
            if line.startswith(stop_line):
                remainder = line.removeprefix(stop_line)
                if remainder and not remainder.isspace():
                    fail(f"Garbage after stop line: {remainder!r}")
                return True
            else:
                # gh-92256: don't allow incorrectly formatted stop lines
                if line.lstrip().startswith(stop_line):
                    fail(f"Whitespace is not allowed before the stop line: {line!r}")
                return False

        # consume body of program
        while self.input:
            line = self._line()
            if is_stop_line(line) or self.is_start_line(line):
                break
            if body_prefix:
                line = line.lstrip()
                assert line.startswith(body_prefix)
                line = line.removeprefix(body_prefix)
            in_lines.append(line)

        # consume output and checksum line, if present.
        if self.last_dsl_name == dsl_name:
            checksum_re = self.last_checksum_re
        else:
            before, _, after = self.language.checksum_line.format(dsl_name=dsl_name, arguments='{arguments}').partition('{arguments}')
            assert _ == '{arguments}'
            checksum_re = libclinic.create_regex(before, after, word=False)
            self.last_dsl_name = dsl_name
            self.last_checksum_re = checksum_re
        assert checksum_re is not None

        # scan forward for checksum line
        out_lines = []
        arguments = None
        while self.input:
            line = self._line(lookahead=True)
            match = checksum_re.match(line.lstrip())
            arguments = match.group(1) if match else None
            if arguments:
                break
            out_lines.append(line)
            if self.is_start_line(line):
                break

        output: str | None
        output = "".join(out_lines)
        if arguments:
            d = {}
            for field in shlex.split(arguments):
                name, equals, value = field.partition('=')
                if not equals:
                    fail(f"Mangled Argument Clinic marker line: {line!r}")
                d[name.strip()] = value.strip()

            if self.verify:
                if 'input' in d:
                    checksum = d['output']
                else:
                    checksum = d['checksum']

                computed = libclinic.compute_checksum(output, len(checksum))
                if checksum != computed:
                    fail("Checksum mismatch! "
                         f"Expected {checksum!r}, computed {computed!r}. "
                         "Suggested fix: remove all generated code including "
                         "the end marker, or use the '-f' option.")
        else:
            # put back output
            output_lines = output.splitlines(keepends=True)
            self.line_number -= len(output_lines)
            self.input.extend(reversed(output_lines))
            output = None

        return Block("".join(in_lines), dsl_name, output=output)
