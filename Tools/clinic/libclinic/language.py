from __future__ import annotations
import abc
import typing
from collections.abc import (
    Iterable,
)

import libclinic
from libclinic import fail
from libclinic.function import (
    Module, Class, Function)

if typing.TYPE_CHECKING:
    from libclinic.app import Clinic


class Language(metaclass=abc.ABCMeta):

    start_line = ""
    body_prefix = ""
    stop_line = ""
    checksum_line = ""

    def __init__(self, filename: str) -> None:
        self.filename = filename

    @abc.abstractmethod
    def render(
            self,
            clinic: Clinic,
            signatures: Iterable[Module | Class | Function]
    ) -> str:
        ...

    def parse_line(self, line: str) -> None:
        ...

    def validate(self) -> None:
        def assert_only_one(
                attr: str,
                *additional_fields: str
        ) -> None:
            """
            Ensures that the string found at getattr(self, attr)
            contains exactly one formatter replacement string for
            each valid field.  The list of valid fields is
            ['dsl_name'] extended by additional_fields.

            e.g.
                self.fmt = "{dsl_name} {a} {b}"

                # this passes
                self.assert_only_one('fmt', 'a', 'b')

                # this fails, the format string has a {b} in it
                self.assert_only_one('fmt', 'a')

                # this fails, the format string doesn't have a {c} in it
                self.assert_only_one('fmt', 'a', 'b', 'c')

                # this fails, the format string has two {a}s in it,
                # it must contain exactly one
                self.fmt2 = '{dsl_name} {a} {a}'
                self.assert_only_one('fmt2', 'a')

            """
            fields = ['dsl_name']
            fields.extend(additional_fields)
            line: str = getattr(self, attr)
            fcf = libclinic.FormatCounterFormatter()
            fcf.format(line)
            def local_fail(should_be_there_but_isnt: bool) -> None:
                if should_be_there_but_isnt:
                    fail("{} {} must contain {{{}}} exactly once!".format(
                        self.__class__.__name__, attr, name))
                else:
                    fail("{} {} must not contain {{{}}}!".format(
                        self.__class__.__name__, attr, name))

            for name, count in fcf.counts.items():
                if name in fields:
                    if count > 1:
                        local_fail(True)
                else:
                    local_fail(False)
            for name in fields:
                if fcf.counts.get(name) != 1:
                    local_fail(True)

        assert_only_one('start_line')
        assert_only_one('stop_line')

        field = "arguments" if "{arguments}" in self.checksum_line else "checksum"
        assert_only_one('checksum_line', field)


class PythonLanguage(Language):

    language      = 'Python'
    start_line    = "#/*[{dsl_name} input]"
    body_prefix   = "#"
    stop_line     = "#[{dsl_name} start generated code]*/"
    checksum_line = "#/*[{dsl_name} end generated code: {arguments}]*/"
