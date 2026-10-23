from __future__ import annotations

import argparse
import difflib
import inspect
import os
import re
import sys
from collections.abc import Callable, Iterable, Iterator, Mapping
from typing import NoReturn


# Local imports.
import libclinic
import libclinic.cpp
from libclinic import ClinicError
from libclinic.language import Language, PythonLanguage
from libclinic.block_parser import BlockParser
from libclinic.converter import (
    ConverterType, converters, legacy_converters)
from libclinic.return_converters import (
    return_converters, ReturnConverterType)
from libclinic.clanguage import CLanguage
from libclinic.app import Clinic
from libclinic.dsl_parser import render_text_signature
from libclinic.function import (
    Class, Definition, Module, GETTER, SETTER, walk_definitions)


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


# Match '#define Py_LIMITED_API'.
# Match '#  define Py_LIMITED_API 0x030d0000' (without the version).
LIMITED_CAPI_REGEX = re.compile(r'# *define +Py_LIMITED_API')


# "extensions" maps the file extension ("c", "py") to Language classes.
LangDict = dict[str, Callable[[str], Language]]
extensions: LangDict = { name: CLanguage for name in "c cc cpp cxx h hh hpp hxx".split() }
extensions['py'] = PythonLanguage


def parse_file(
        filename: str,
        *,
        limited_capi: bool,
        output: str | None = None,
        verify: bool = True,
        writer: libclinic.FileWriter | None = None,
) -> Clinic | None:
    if not output:
        output = filename
    if writer is None:
        writer = libclinic.FileWriter()

    extension = os.path.splitext(filename)[1][1:]
    if not extension:
        raise ClinicError(f"Can't extract file type for file {filename!r}")

    try:
        language = extensions[extension](filename)
    except KeyError:
        raise ClinicError(f"Can't identify file type for file {filename!r}")

    if os.path.isdir(filename):
        raise ClinicError(f"Can't read file {filename!r}: it is a directory")

    with open(filename, encoding="utf-8") as f:
        raw = f.read()

    # exit quickly if there are no clinic markers in the file
    find_start_re = BlockParser("", language).find_start_re
    if not find_start_re.search(raw):
        return None

    if LIMITED_CAPI_REGEX.search(raw):
        limited_capi = True

    assert isinstance(language, CLanguage)
    clinic = Clinic(language,
                    verify=verify,
                    filename=filename,
                    limited_capi=limited_capi,
                    writer=writer)
    index = len(writer.files)
    cooked = clinic.parse(raw)
    writer.write(output, cooked)

    files = writer.files[index:]
    writer.update_times(output,
                        [fn for fn, _ in files if fn != output],
                        any(changed for _, changed in files))
    return clinic


def format_definition(depth: int, name: str, definition: Definition) -> str:
    indent = "  " * (depth + 1)
    if isinstance(definition, Module):
        return f"{indent}module {name}"
    if isinstance(definition, Class):
        return f"{indent}class {name}"
    if definition.kind is GETTER:
        return f"{indent}getter {name}"
    if definition.kind is SETTER:
        return f"{indent}setter {name}"
    signature = render_text_signature(definition, definition.render_parameters,
                                      name=name, line_width=None)
    return indent + signature


def print_definitions(clinic: Clinic) -> None:
    """Print the modules, classes and functions defined in the parsed file."""
    lines = [format_definition(depth, name, definition)
             for depth, name, definition in walk_definitions(clinic)]
    if lines:
        print(clinic.filename)
        print("\n".join(lines))


def create_cli() -> argparse.ArgumentParser:
    cmdline = argparse.ArgumentParser(
        prog="clinic.py",
        description="""Preprocessor for CPython C files.

The purpose of the Argument Clinic is automating all the boilerplate involved
with writing argument parsing code for builtins and providing introspection
signatures ("docstrings") for CPython builtins.

For more information see https://devguide.python.org/development-tools/clinic/""")
    cmdline.add_argument("-f", "--force", action='store_true',
                         help="force output regeneration")
    cmdline.add_argument("-o", "--output", type=str,
                         help="redirect file output to OUTPUT")
    cmdline.add_argument("-v", "--verbose", action='store_true',
                         help="enable verbose mode")
    cmdline.add_argument("--dry-run", action='store_true',
                         help=("don't write any file, only list the files "
                               "which would be changed"))
    cmdline.add_argument("--diff", action='store_true',
                         help=("don't write any file, write a unified diff "
                               "of the changes to the standard output"))
    cmdline.add_argument("--converters", action='store_true',
                         help=("print a list of all supported converters "
                               "and return converters; if files are "
                               "specified, print only the converters "
                               "which they define"))
    cmdline.add_argument("--list", action='store_true',
                         help=("don't write any file, only list the modules, "
                               "classes and functions which the specified "
                               "files define, with their signatures"))
    cmdline.add_argument("--make", action='store_true',
                         help="walk --srcdir to run over all relevant files")
    cmdline.add_argument("--srcdir", type=str, default=os.curdir,
                         help="the directory tree to walk in --make mode")
    cmdline.add_argument("--exclude", type=str, action="append",
                         help=("a file to exclude in --make mode; "
                               "can be given multiple times"))
    cmdline.add_argument("--limited", dest="limited_capi", action='store_true',
                         help="use the Limited C API")
    cmdline.add_argument("filename", metavar="FILE", type=str, nargs="*",
                         help="the list of files to process")
    return cmdline


def print_diff(change: libclinic.FileChange) -> None:
    if change.old_contents is None:
        fromfile = "/dev/null"
        old_lines: list[str] = []
    else:
        fromfile = change.filename
        old_lines = change.old_contents.splitlines(keepends=True)
    sys.stdout.writelines(difflib.unified_diff(
        old_lines,
        change.new_contents.splitlines(keepends=True),
        fromfile=fromfile,
        tofile=change.filename,
    ))


def report_changes(writer: libclinic.FileWriter, *, diff: bool) -> None:
    for change in sorted(writer.changes, key=lambda change: change.filename):
        if diff:
            print_diff(change)
        else:
            action = "create" if change.old_contents is None else "update"
            print(f"would {action} {change.filename}")


AnyConverterType = ConverterType | ReturnConverterType


def defined_in_files(
    registry: Mapping[str, AnyConverterType],
    builtin: Mapping[str, AnyConverterType],
) -> dict[str, AnyConverterType]:
    """Return the converters which the parsed files define or redefine."""
    return {name: cls for name, cls in registry.items()
            if builtin.get(name) is not cls}


def print_converter_list(
    title: str,
    attribute: str,
    registry: Mapping[str, AnyConverterType],
) -> None:
    print(title + ":")
    for name, cls in sorted(registry.items(), key=lambda item: item[0].lower()):
        callable = getattr(cls, attribute, None)
        if not callable:
            continue
        signature = inspect.signature(callable)
        parameters = []
        for parameter_name, parameter in signature.parameters.items():
            if parameter.kind == inspect.Parameter.KEYWORD_ONLY:
                if parameter.default != inspect.Parameter.empty:
                    s = f'{parameter_name}={parameter.default!r}'
                else:
                    s = parameter_name
                parameters.append(s)
        print('    {}({})'.format(name, ', '.join(parameters)))
    print()


def print_converters(
    converters: Mapping[str, AnyConverterType],
    legacy_converters: Mapping[str, AnyConverterType],
    return_converters: Mapping[str, AnyConverterType],
) -> None:
    if not (converters or legacy_converters or return_converters):
        return
    print()
    if legacy_converters:
        print("Legacy converters:")
        legacy = sorted(legacy_converters)
        # A converter defined in a file can use any string, even a C
        # expression, as its format unit, not only a letter.
        groups = ([c for c in legacy if c[0].isupper()],
                  [c for c in legacy if c[0].islower()],
                  [c for c in legacy if not c[0].isalpha()])
        for group in groups:
            if group:
                print('    ' + ' '.join(group))
        print()
    if converters:
        print_converter_list("Converters", 'converter_init', converters)
    if return_converters:
        print_converter_list("Return converters", 'return_converter_init',
                             return_converters)
    print("All converters also accept (c_default=None, py_default=None, annotation=None).")
    print("All return converters also accept (py_default=None).")


def walk_srcdir(srcdir: str, exclude: list[str] | None) -> Iterator[str]:
    """Yield the C files in the source directory tree."""
    if exclude:
        excludes = [os.path.normpath(os.path.join(srcdir, f)) for f in exclude]
    else:
        excludes = []
    for root, dirs, files in os.walk(srcdir):
        for rcs_dir in ('.svn', '.git', '.hg', 'build', 'externals'):
            if rcs_dir in dirs:
                dirs.remove(rcs_dir)
        for filename in files:
            # handle .c, .cpp and .h files
            if not filename.endswith(('.c', '.cpp', '.h')):
                continue
            path = os.path.normpath(os.path.join(root, filename))
            if path in excludes:
                continue
            yield path


def run_clinic(parser: argparse.ArgumentParser, ns: argparse.Namespace) -> None:
    dry_run = ns.dry_run or ns.diff
    # The report is written to the standard output, so the progress
    # is written to the standard error stream to not mix them.
    verbose_file = sys.stderr if dry_run or ns.list else sys.stdout

    filenames: Iterable[str]
    if ns.make:
        if ns.output or ns.filename:
            parser.error("can't use -o or filenames with --make")
        if not ns.srcdir:
            parser.error("--srcdir must not be empty with --make")
        filenames = walk_srcdir(ns.srcdir, ns.exclude)
    else:
        if not ns.filename and not ns.converters:
            parser.error("no input files")
        if ns.output and len(ns.filename) > 1:
            parser.error("can't use -o with multiple filenames")
        filenames = ns.filename

    if ns.list:
        if dry_run:
            parser.error("can't use --dry-run or --diff with --list")
        if ns.converters:
            parser.error("can't use --converters with --list")

    if ns.converters:
        if dry_run:
            parser.error("can't use --dry-run or --diff with --converters")
        if not ns.make and not ns.filename:
            print_converters(converters, legacy_converters, return_converters)
            return
        # Converters defined in a file are added to the same registries
        # as the built-in ones, so remember the latter to tell them apart.
        builtin_converters = dict(converters)
        builtin_legacy_converters = dict(legacy_converters)
        builtin_return_converters = dict(return_converters)

    writer = libclinic.FileWriter(dry_run=dry_run or ns.converters or ns.list)
    for filename in filenames:
        if ns.verbose:
            print(filename, file=verbose_file)
        clinic = parse_file(filename, output=ns.output,
                            verify=not ns.force, limited_capi=ns.limited_capi,
                            writer=writer)
        if ns.list and clinic is not None:
            print_definitions(clinic)

    if ns.converters:
        print_converters(
            defined_in_files(converters, builtin_converters),
            defined_in_files(legacy_converters, builtin_legacy_converters),
            defined_in_files(return_converters, builtin_return_converters))
    elif not ns.list:
        report_changes(writer, diff=ns.diff)


def main(argv: list[str] | None = None) -> NoReturn:
    parser = create_cli()
    args = parser.parse_args(argv)
    try:
        run_clinic(parser, args)
    except ClinicError as exc:
        sys.stderr.write(exc.report())
        sys.exit(1)
    else:
        sys.exit(0)
