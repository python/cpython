from __future__ import annotations

import os
import re

from collections.abc import Sequence
from typing import (
    Any,
    Callable,
    TYPE_CHECKING,
)

from . import utils
from .utils import (
    fail, warn,
    _TextAccumulator)
from .function import (
    Function, Module, Class, ModuleDict, ClassDict)
from .language import Language, PythonLanguage
from .clanguage import CLanguage
from .block_parser import Block, BlockParser
from .block_printer import BlockPrinter, Include, Destination, DestinationDict
from .parser import parsers
if TYPE_CHECKING:
    from .parser import Parser


# TODO:
#
# soon:
#
# * allow mixing any two of {positional-only, positional-or-keyword,
#   keyword-only}
#       * dict constructor uses positional-only and keyword-only
#       * max and min use positional only with an optional group
#         and keyword-only
#

# match '#define Py_LIMITED_API'
LIMITED_CAPI_REGEX = re.compile(r'#define +Py_LIMITED_API')


# "extensions" maps the file extension ("c", "py") to Language classes.
LangDict = dict[str, Callable[[str], Language]]
extensions: LangDict = { name: CLanguage for name in "c cc cpp cxx h hh hpp hxx".split() }
extensions['py'] = PythonLanguage


def write_file(filename: str, new_contents: str) -> None:
    try:
        with open(filename, encoding="utf-8") as fp:
            old_contents = fp.read()

        if old_contents == new_contents:
            # no change: avoid modifying the file modification time
            return
    except FileNotFoundError:
        pass
    # Atomic write using a temporary file and os.replace()
    filename_new = f"{filename}.new"
    with open(filename_new, "w", encoding="utf-8") as fp:
        fp.write(new_contents)
    try:
        os.replace(filename_new, filename)
    except:
        os.unlink(filename_new)
        raise


clinic: Clinic | None = None
class Clinic:

    presets_text = """
preset block
everything block
methoddef_ifndef buffer 1
docstring_prototype suppress
parser_prototype suppress
cpp_if suppress
cpp_endif suppress

preset original
everything block
methoddef_ifndef buffer 1
docstring_prototype suppress
parser_prototype suppress
cpp_if suppress
cpp_endif suppress

preset file
everything file
methoddef_ifndef file 1
docstring_prototype suppress
parser_prototype suppress
impl_definition block

preset buffer
everything buffer
methoddef_ifndef buffer 1
impl_definition block
docstring_prototype suppress
impl_prototype suppress
parser_prototype suppress

preset partial-buffer
everything buffer
methoddef_ifndef buffer 1
docstring_prototype block
impl_prototype suppress
methoddef_define block
parser_prototype block
impl_definition block

"""

    def __init__(
            self,
            language: CLanguage,
            printer: BlockPrinter | None = None,
            *,
            filename: str,
            limited_capi: bool,
            verify: bool = True,
    ) -> None:
        # maps strings to Parser objects.
        # (instantiated from the "parsers" global.)
        self.parsers: dict[str, Parser] = {}
        self.language: CLanguage = language
        if printer:
            fail("Custom printers are broken right now")
        self.printer = printer or BlockPrinter(language)
        self.verify = verify
        self.limited_capi = limited_capi
        self.filename = filename
        self.modules: ModuleDict = {}
        self.classes: ClassDict = {}
        self.functions: list[Function] = []
        # dict: include name => Include instance
        self.includes: dict[str, Include] = {}

        self.line_prefix = self.line_suffix = ''

        self.destinations: DestinationDict = {}
        self.add_destination("block", "buffer")
        self.add_destination("suppress", "suppress")
        self.add_destination("buffer", "buffer")
        if filename:
            self.add_destination("file", "file", "{dirname}/clinic/{basename}.h")

        d = self.get_destination_buffer
        self.destination_buffers = {
            'cpp_if': d('file'),
            'docstring_prototype': d('suppress'),
            'docstring_definition': d('file'),
            'methoddef_define': d('file'),
            'impl_prototype': d('file'),
            'parser_prototype': d('suppress'),
            'parser_definition': d('file'),
            'cpp_endif': d('file'),
            'methoddef_ifndef': d('file', 1),
            'impl_definition': d('block'),
        }

        DestBufferType = dict[str, _TextAccumulator]
        DestBufferList = list[DestBufferType]

        self.destination_buffers_stack: DestBufferList = []
        self.ifndef_symbols: set[str] = set()

        self.presets: dict[str, dict[Any, Any]] = {}
        preset = None
        for line in self.presets_text.strip().split('\n'):
            line = line.strip()
            if not line:
                continue
            name, value, *options = line.split()
            if name == 'preset':
                self.presets[value] = preset = {}
                continue

            if len(options):
                index = int(options[0])
            else:
                index = 0
            buffer = self.get_destination_buffer(value, index)

            if name == 'everything':
                for name in self.destination_buffers:
                    preset[name] = buffer
                continue

            assert name in self.destination_buffers
            preset[name] = buffer

        global clinic
        clinic = self
        # inject clinic in the utils globals
        utils.clinic = self

    def add_include(self, name: str, reason: str,
                    *, condition: str | None = None) -> None:
        try:
            existing = self.includes[name]
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

        self.includes[name] = Include(name, reason, condition)

    def add_destination(
            self,
            name: str,
            type: str,
            *args: str
    ) -> None:
        if name in self.destinations:
            fail(f"Destination already exists: {name!r}")
        self.destinations[name] = Destination(name, type, self, args)

    def get_destination(self, name: str) -> Destination:
        d = self.destinations.get(name)
        if not d:
            fail(f"Destination does not exist: {name!r}")
        return d

    def get_destination_buffer(
            self,
            name: str,
            item: int = 0
    ) -> _TextAccumulator:
        d = self.get_destination(name)
        return d.buffers[item]

    def parse(self, input: str) -> str:
        printer = self.printer
        self.block_parser = BlockParser(input, self.language, verify=self.verify)
        for block in self.block_parser:
            dsl_name = block.dsl_name
            if dsl_name:
                if dsl_name not in self.parsers:
                    assert dsl_name in parsers, f"No parser to handle {dsl_name!r} block."
                    self.parsers[dsl_name] = parsers[dsl_name](self)
                parser = self.parsers[dsl_name]
                parser.parse(block)
            printer.print_block(block,
                                limited_capi=self.limited_capi,
                                header_includes=self.includes)

        # these are destinations not buffers
        for name, destination in self.destinations.items():
            if destination.type == 'suppress':
                continue
            output = destination.dump()

            if output:
                block = Block("", dsl_name="clinic", output=output)

                if destination.type == 'buffer':
                    block.input = "dump " + name + "\n"
                    warn("Destination buffer " + repr(name) + " not empty at end of file, emptying.")
                    printer.write("\n")
                    printer.print_block(block,
                                        limited_capi=self.limited_capi,
                                        header_includes=self.includes)
                    continue

                if destination.type == 'file':
                    try:
                        dirname = os.path.dirname(destination.filename)
                        try:
                            os.makedirs(dirname)
                        except FileExistsError:
                            if not os.path.isdir(dirname):
                                fail(f"Can't write to destination "
                                     f"{destination.filename!r}; "
                                     f"can't make directory {dirname!r}!")
                        if self.verify:
                            with open(destination.filename) as f:
                                parser_2 = BlockParser(f.read(), language=self.language)
                                blocks = list(parser_2)
                                if (len(blocks) != 1) or (blocks[0].input != 'preserve\n'):
                                    fail(f"Modified destination file "
                                         f"{destination.filename!r}; not overwriting!")
                    except FileNotFoundError:
                        pass

                    block.input = 'preserve\n'
                    printer_2 = BlockPrinter(self.language)
                    printer_2.print_block(block,
                                          core_includes=True,
                                          limited_capi=self.limited_capi,
                                          header_includes=self.includes)
                    write_file(destination.filename, printer_2.f.getvalue())
                    continue

        return printer.f.getvalue()

    def _module_and_class(
        self, fields: Sequence[str]
    ) -> tuple[Module | Clinic, Class | None]:
        """
        fields should be an iterable of field names.
        returns a tuple of (module, class).
        the module object could actually be self (a clinic object).
        this function is only ever used to find the parent of where
        a new class/module should go.
        """
        parent: Clinic | Module | Class = self
        module: Clinic | Module = self
        cls: Class | None = None

        for idx, field in enumerate(fields):
            if not isinstance(parent, Class):
                if field in parent.modules:
                    parent = module = parent.modules[field]
                    continue
            if field in parent.classes:
                parent = cls = parent.classes[field]
            else:
                fullname = ".".join(fields[idx:])
                fail(f"Parent class or module {fullname!r} does not exist.")

        return module, cls

    def __repr__(self) -> str:
        return "<clinic.Clinic object>"


def parse_file(
        filename: str,
        *,
        limited_capi: bool,
        output: str | None = None,
        verify: bool = True,
) -> None:
    if not output:
        output = filename

    extension = os.path.splitext(filename)[1][1:]
    if not extension:
        fail(f"Can't extract file type for file {filename!r}")

    try:
        language = extensions[extension](filename)
    except KeyError:
        fail(f"Can't identify file type for file {filename!r}")

    with open(filename, encoding="utf-8") as f:
        raw = f.read()

    # exit quickly if there are no clinic markers in the file
    find_start_re = BlockParser("", language).find_start_re
    if not find_start_re.search(raw):
        return

    if LIMITED_CAPI_REGEX.search(raw):
        limited_capi = True

    assert isinstance(language, CLanguage)
    clinic = Clinic(language,
                    verify=verify,
                    filename=filename,
                    limited_capi=limited_capi)
    cooked = clinic.parse(raw)

    write_file(output, cooked)
