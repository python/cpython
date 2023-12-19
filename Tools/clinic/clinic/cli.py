import argparse
import os
import inspect
import sys

from . import clinic
from .utils import ClinicError
from .converter import legacy_converters
from .clinic import parse_file
from typing import (
    NoReturn,
)


def create_cli() -> argparse.ArgumentParser:
    cmdline = argparse.ArgumentParser(
        prog="clinic.py",
        description="""Preprocessor for CPython C files.

The purpose of the Argument Clinic is automating all the boilerplate involved
with writing argument parsing code for builtins and providing introspection
signatures ("docstrings") for CPython builtins.

For more information see https://docs.python.org/3/howto/clinic.html""")
    cmdline.add_argument("-f", "--force", action='store_true',
                         help="force output regeneration")
    cmdline.add_argument("-o", "--output", type=str,
                         help="redirect file output to OUTPUT")
    cmdline.add_argument("-v", "--verbose", action='store_true',
                         help="enable verbose mode")
    cmdline.add_argument("--converters", action='store_true',
                         help=("print a list of all supported converters "
                               "and return converters"))
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


def run_clinic(parser: argparse.ArgumentParser, ns: argparse.Namespace) -> None:
    if ns.converters:
        if ns.filename:
            parser.error(
                "can't specify --converters and a filename at the same time"
            )
        converters: list[tuple[str, str]] = []
        return_converters: list[tuple[str, str]] = []
        ignored = set("""
            add_c_converter
            add_c_return_converter
            add_default_legacy_c_converter
            add_legacy_c_converter
            """.strip().split())
        from . import converters as converters_mod
        from . import return_converter as return_converter_mod
        module = {
            **vars(converters_mod),
            **vars(return_converter_mod),
        }
        for name in module:
            for suffix, ids in (
                ("_return_converter", return_converters),
                ("_converter", converters),
            ):
                if name in ignored:
                    continue
                if name.endswith(suffix):
                    ids.append((name, name.removesuffix(suffix)))
                    break
        print()

        print("Legacy converters:")
        legacy = sorted(legacy_converters)
        print('    ' + ' '.join(c for c in legacy if c[0].isupper()))
        print('    ' + ' '.join(c for c in legacy if c[0].islower()))
        print()

        for title, attribute, ids in (
            ("Converters", 'converter_init', converters),
            ("Return converters", 'return_converter_init', return_converters),
        ):
            print(title + ":")
            longest = -1
            for name, short_name in ids:
                longest = max(longest, len(short_name))
            for name, short_name in sorted(ids, key=lambda x: x[1].lower()):
                cls = module[name]
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
                print('    {}({})'.format(short_name, ', '.join(parameters)))
            print()
        print("All converters also accept (c_default=None, py_default=None, annotation=None).")
        print("All return converters also accept (py_default=None).")
        return

    if ns.make:
        if ns.output or ns.filename:
            parser.error("can't use -o or filenames with --make")
        if not ns.srcdir:
            parser.error("--srcdir must not be empty with --make")
        if ns.exclude:
            excludes = [os.path.join(ns.srcdir, f) for f in ns.exclude]
            excludes = [os.path.normpath(f) for f in excludes]
        else:
            excludes = []
        for root, dirs, files in os.walk(ns.srcdir):
            for rcs_dir in ('.svn', '.git', '.hg', 'build', 'externals'):
                if rcs_dir in dirs:
                    dirs.remove(rcs_dir)
            for filename in files:
                # handle .c, .cpp and .h files
                if not filename.endswith(('.c', '.cpp', '.h')):
                    continue
                path = os.path.join(root, filename)
                path = os.path.normpath(path)
                if path in excludes:
                    continue
                if ns.verbose:
                    print(path)
                parse_file(path,
                           verify=not ns.force, limited_capi=ns.limited_capi)
        return

    if not ns.filename:
        parser.error("no input files")

    if ns.output and len(ns.filename) > 1:
        parser.error("can't use -o with multiple filenames")

    for filename in ns.filename:
        if ns.verbose:
            print(filename)
        parse_file(filename, output=ns.output,
                   verify=not ns.force, limited_capi=ns.limited_capi)


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
