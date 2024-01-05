"""Check the stable ABI manifest or generate files from it

By default, the tool only checks existing files/libraries.
Pass --generate to recreate auto-generated files instead.

For actions that take a FILENAME, the filename can be left out to use a default
(relative to the manifest file, as they appear in the CPython codebase).
"""

from functools import partial
from pathlib import Path
import dataclasses
import subprocess
import sysconfig
import argparse
import textwrap
import tomllib
import difflib
import pprint
import sys
import os
import os.path
import io
import re
import csv

SCRIPT_NAME = 'Tools/build/stable_abi.py'
MISSING = object()

EXCLUDED_HEADERS = {
    "bytes_methods.h",
    "cellobject.h",
    "classobject.h",
    "code.h",
    "compile.h",
    "datetime.h",
    "dtoa.h",
    "frameobject.h",
    "genobject.h",
    "longintrepr.h",
    "parsetok.h",
    "pyatomic.h",
    "token.h",
    "ucnhash.h",
}
MACOS = (sys.platform == "darwin")
UNIXY = MACOS or (sys.platform == "linux")  # XXX should this be "not Windows"?


# The stable ABI manifest (Misc/stable_abi.toml) exists only to fill the
# following dataclasses.
# Feel free to change its syntax (and the `parse_manifest` function)
# to better serve that purpose (while keeping it human-readable).

class Manifest:
    """Collection of `ABIItem`s forming the stable ABI/limited API."""
    def __init__(self):
        self.contents = dict()

    def add(self, item):
        if item.name in self.contents:
            # We assume that stable ABI items do not share names,
            # even if they're different kinds (e.g. function vs. macro).
            raise ValueError(f'duplicate ABI item {item.name}')
        self.contents[item.name] = item

    def select(self, kinds, *, include_abi_only=True, ifdef=None):
        """Yield selected items of the manifest

        kinds: set of requested kinds, e.g. {'function', 'macro'}
        include_abi_only: if True (default), include all items of the
            stable ABI.
            If False, include only items from the limited API
            (i.e. items people should use today)
        ifdef: set of feature macros (e.g. {'HAVE_FORK', 'MS_WINDOWS'}).
            If None (default), items are not filtered by this. (This is
            different from the empty set, which filters out all such
            conditional items.)
        """
        for name, item in sorted(self.contents.items()):
            if item.kind not in kinds:
                continue
            if item.abi_only and not include_abi_only:
                continue
            if (ifdef is not None
                    and item.ifdef is not None
                    and item.ifdef not in ifdef):
                continue
            yield item

    def dump(self):
        """Yield lines to recreate the manifest file (sans comments/newlines)"""
        for item in self.contents.values():
            fields = dataclasses.fields(item)
            yield f"[{item.kind}.{item.name}]"
            for field in fields:
                if field.name in {'name', 'value', 'kind'}:
                    continue
                value = getattr(item, field.name)
                if value == field.default:
                    pass
                elif value is True:
                    yield f"    {field.name} = true"
                elif value:
                    yield f"    {field.name} = {value!r}"


itemclasses = {}
def itemclass(kind):
    """Register the decorated class in `itemclasses`"""
    def decorator(cls):
        itemclasses[kind] = cls
        return cls
    return decorator

@itemclass('function')
@itemclass('macro')
@itemclass('data')
@itemclass('const')
@itemclass('typedef')
@dataclasses.dataclass
class ABIItem:
    """Information on one item (function, macro, struct, etc.)"""

    name: str
    kind: str
    added: str = None
    abi_only: bool = False
    ifdef: str = None

@itemclass('feature_macro')
@dataclasses.dataclass(kw_only=True)
class FeatureMacro(ABIItem):
    name: str
    doc: str
    windows: bool = False
    abi_only: bool = True

@itemclass('struct')
@dataclasses.dataclass(kw_only=True)
class Struct(ABIItem):
    struct_abi_kind: str
    members: list = None


def parse_manifest(file):
    """Parse the given file (iterable of lines) to a Manifest"""

    manifest = Manifest()

    data = tomllib.load(file)

    for kind, itemclass in itemclasses.items():
        for name, item_data in data[kind].items():
            try:
                item = itemclass(name=name, kind=kind, **item_data)
                manifest.add(item)
            except BaseException as exc:
                exc.add_note(f'in {kind} {name}')
                raise

    return manifest

# The tool can run individual "actions".
# Most actions are "generators", which generate a single file from the
# manifest. (Checking works by generating a temp file & comparing.)
# Other actions, like "--unixy-check", don't work on a single file.

generators = []
def generator(var_name, default_path):
    """Decorates a file generator: function that writes to a file"""
    def _decorator(func):
        func.var_name = var_name
        func.arg_name = '--' + var_name.replace('_', '-')
        func.default_path = default_path
        generators.append(func)
        return func
    return _decorator


@generator("python3dll", 'PC/python3dll.c')
def gen_python3dll(manifest, args, outfile):
    """Generate/check the source for the Windows stable ABI library"""
    write = partial(print, file=outfile)
    content = f"""\
        /* Re-export stable Python ABI */

        /* Generated by {SCRIPT_NAME} */
    """
    content += r"""
        #ifdef _M_IX86
        #define DECORATE "_"
        #else
        #define DECORATE
        #endif

        #define EXPORT_FUNC(name) \
            __pragma(comment(linker, "/EXPORT:" DECORATE #name "=" PYTHON_DLL_NAME "." #name))
        #define EXPORT_DATA(name) \
            __pragma(comment(linker, "/EXPORT:" DECORATE #name "=" PYTHON_DLL_NAME "." #name ",DATA"))
    """
    write(textwrap.dedent(content))

    def sort_key(item):
        return item.name.lower()

    windows_feature_macros = {
        item.name for item in manifest.select({'feature_macro'}) if item.windows
    }
    for item in sorted(
            manifest.select(
                {'function'},
                include_abi_only=True,
                ifdef=windows_feature_macros),
            key=sort_key):
        write(f'EXPORT_FUNC({item.name})')

    write()

    for item in sorted(
            manifest.select(
                {'data'},
                include_abi_only=True,
                ifdef=windows_feature_macros),
            key=sort_key):
        write(f'EXPORT_DATA({item.name})')

REST_ROLES = {
    'function': 'function',
    'data': 'var',
    'struct': 'type',
    'macro': 'macro',
    # 'const': 'const',  # all undocumented
    'typedef': 'type',
}

@generator("doc_list", 'Doc/data/stable_abi.dat')
def gen_doc_annotations(manifest, args, outfile):
    """Generate/check the stable ABI list for documentation annotations"""
    writer = csv.DictWriter(
        outfile,
        ['role', 'name', 'added', 'ifdef_note', 'struct_abi_kind'],
        lineterminator='\n')
    writer.writeheader()
    for item in manifest.select(REST_ROLES.keys(), include_abi_only=False):
        if item.ifdef:
            ifdef_note = manifest.contents[item.ifdef].doc
        else:
            ifdef_note = None
        row = {
            'role': REST_ROLES[item.kind],
            'name': item.name,
            'added': item.added,
            'ifdef_note': ifdef_note}
        rows = [row]
        if item.kind == 'struct':
            row['struct_abi_kind'] = item.struct_abi_kind
            for member_name in item.members or ():
                rows.append({
                    'role': 'member',
                    'name': f'{item.name}.{member_name}',
                    'added': item.added})
        writer.writerows(rows)

@generator("ctypes_test", 'Lib/test/test_stable_abi_ctypes.py')
def gen_ctypes_test(manifest, args, outfile):
    """Generate/check the ctypes-based test for exported symbols"""
    write = partial(print, file=outfile)
    write(textwrap.dedent(f'''\
        # Generated by {SCRIPT_NAME}

        """Test that all symbols of the Stable ABI are accessible using ctypes
        """

        import sys
        import unittest
        from test.support.import_helper import import_module
        from _testcapi import get_feature_macros

        feature_macros = get_feature_macros()
        ctypes_test = import_module('ctypes')

        class TestStableABIAvailability(unittest.TestCase):
            def test_available_symbols(self):

                for symbol_name in SYMBOL_NAMES:
                    with self.subTest(symbol_name):
                        ctypes_test.pythonapi[symbol_name]

            def test_feature_macros(self):
                self.assertEqual(
                    set(get_feature_macros()), EXPECTED_FEATURE_MACROS)

            # The feature macros for Windows are used in creating the DLL
            # definition, so they must be known on all platforms.
            # If we are on Windows, we check that the hardcoded data matches
            # the reality.
            @unittest.skipIf(sys.platform != "win32", "Windows specific test")
            def test_windows_feature_macros(self):
                for name, value in WINDOWS_FEATURE_MACROS.items():
                    if value != 'maybe':
                        with self.subTest(name):
                            self.assertEqual(feature_macros[name], value)

        SYMBOL_NAMES = (
    '''))
    items = manifest.select(
        {'function', 'data'},
        include_abi_only=True,
    )
    optional_items = {}
    for item in items:
        if item.name in (
                # Some symbols aren't exported on all platforms.
                # This is a bug: https://bugs.python.org/issue44133
                'PyModule_Create2', 'PyModule_FromDefAndSpec2',
            ):
            continue
        if item.ifdef:
            optional_items.setdefault(item.ifdef, []).append(item.name)
        else:
            write(f'    "{item.name}",')
    write(")")
    for ifdef, names in optional_items.items():
        write(f"if feature_macros[{ifdef!r}]:")
        write(f"    SYMBOL_NAMES += (")
        for name in names:
            write(f"        {name!r},")
        write("    )")
    write("")
    feature_macros = list(manifest.select({'feature_macro'}))
    feature_names = sorted(m.name for m in feature_macros)
    write(f"EXPECTED_FEATURE_MACROS = set({pprint.pformat(feature_names)})")

    windows_feature_macros = {m.name: m.windows for m in feature_macros}
    write(f"WINDOWS_FEATURE_MACROS = {pprint.pformat(windows_feature_macros)}")


@generator("testcapi_feature_macros", 'Modules/_testcapi_feature_macros.inc')
def gen_testcapi_feature_macros(manifest, args, outfile):
    """Generate/check the stable ABI list for documentation annotations"""
    write = partial(print, file=outfile)
    write(f'// Generated by {SCRIPT_NAME}')
    write()
    write('// Add an entry in dict `result` for each Stable ABI feature macro.')
    write()
    for macro in manifest.select({'feature_macro'}):
        name = macro.name
        write(f'#ifdef {name}')
        write(f'    res = PyDict_SetItemString(result, "{name}", Py_True);')
        write('#else')
        write(f'    res = PyDict_SetItemString(result, "{name}", Py_False);')
        write('#endif')
        write('if (res) {')
        write('    Py_DECREF(result); return NULL;')
        write('}')
        write()


def generate_or_check(manifest, args, path, func):
    """Generate/check a file with a single generator

    Return True if successful; False if a comparison failed.
    """

    outfile = io.StringIO()
    func(manifest, args, outfile)
    generated = outfile.getvalue()
    existing = path.read_text()

    if generated != existing:
        if args.generate:
            path.write_text(generated)
        else:
            print(f'File {path} differs from expected!')
            diff = difflib.unified_diff(
                generated.splitlines(), existing.splitlines(),
                str(path), '<expected>',
                lineterm='',
            )
            for line in diff:
                print(line)
            return False
    return True


def do_unixy_check(manifest, args):
    """Check headers & library using "Unixy" tools (GCC/clang, binutils)"""
    okay = True

    # Get all macros first: we'll need feature macros like HAVE_FORK and
    # MS_WINDOWS for everything else
    present_macros = gcc_get_limited_api_macros(['Include/Python.h'])
    feature_macros = set(m.name for m in manifest.select({'feature_macro'}))
    feature_macros &= present_macros

    # Check that we have all needed macros
    expected_macros = set(
        item.name for item in manifest.select({'macro'})
    )
    missing_macros = expected_macros - present_macros
    okay &= _report_unexpected_items(
        missing_macros,
        'Some macros from are not defined from "Include/Python.h"'
        + 'with Py_LIMITED_API:')

    expected_symbols = set(item.name for item in manifest.select(
        {'function', 'data'}, include_abi_only=True, ifdef=feature_macros,
    ))

    # Check the static library (*.a)
    LIBRARY = sysconfig.get_config_var("LIBRARY")
    if not LIBRARY:
        raise Exception("failed to get LIBRARY variable from sysconfig")
    if os.path.exists(LIBRARY):
        okay &= binutils_check_library(
            manifest, LIBRARY, expected_symbols, dynamic=False)

    # Check the dynamic library (*.so)
    LDLIBRARY = sysconfig.get_config_var("LDLIBRARY")
    if not LDLIBRARY:
        raise Exception("failed to get LDLIBRARY variable from sysconfig")
    okay &= binutils_check_library(
            manifest, LDLIBRARY, expected_symbols, dynamic=False)

    # Check definitions in the header files
    expected_defs = set(item.name for item in manifest.select(
        {'function', 'data'}, include_abi_only=False, ifdef=feature_macros,
    ))
    found_defs = gcc_get_limited_api_definitions(['Include/Python.h'])
    missing_defs = expected_defs - found_defs
    okay &= _report_unexpected_items(
        missing_defs,
        'Some expected declarations were not declared in '
        + '"Include/Python.h" with Py_LIMITED_API:')

    # Some Limited API macros are defined in terms of private symbols.
    # These are not part of Limited API (even though they're defined with
    # Py_LIMITED_API). They must be part of the Stable ABI, though.
    private_symbols = {n for n in expected_symbols if n.startswith('_')}
    extra_defs = found_defs - expected_defs - private_symbols
    okay &= _report_unexpected_items(
        extra_defs,
        'Some extra declarations were found in "Include/Python.h" '
        + 'with Py_LIMITED_API:')

    return okay


def _report_unexpected_items(items, msg):
    """If there are any `items`, report them using "msg" and return false"""
    if items:
        print(msg, file=sys.stderr)
        for item in sorted(items):
            print(' -', item, file=sys.stderr)
        return False
    return True


def binutils_get_exported_symbols(library, dynamic=False):
    """Retrieve exported symbols using the nm(1) tool from binutils"""
    # Only look at dynamic symbols
    args = ["nm", "--no-sort"]
    if dynamic:
        args.append("--dynamic")
    args.append(library)
    proc = subprocess.run(args, stdout=subprocess.PIPE, universal_newlines=True)
    if proc.returncode:
        sys.stdout.write(proc.stdout)
        sys.exit(proc.returncode)

    stdout = proc.stdout.rstrip()
    if not stdout:
        raise Exception("command output is empty")

    for line in stdout.splitlines():
        # Split line '0000000000001b80 D PyTextIOWrapper_Type'
        if not line:
            continue

        parts = line.split(maxsplit=2)
        if len(parts) < 3:
            continue

        symbol = parts[-1]
        if MACOS and symbol.startswith("_"):
            yield symbol[1:]
        else:
            yield symbol


def binutils_check_library(manifest, library, expected_symbols, dynamic):
    """Check that library exports all expected_symbols"""
    available_symbols = set(binutils_get_exported_symbols(library, dynamic))
    missing_symbols = expected_symbols - available_symbols
    if missing_symbols:
        print(textwrap.dedent(f"""\
            Some symbols from the limited API are missing from {library}:
                {', '.join(missing_symbols)}

            This error means that there are some missing symbols among the
            ones exported in the library.
            This normally means that some symbol, function implementation or
            a prototype belonging to a symbol in the limited API has been
            deleted or is missing.
        """), file=sys.stderr)
        return False
    return True


def gcc_get_limited_api_macros(headers):
    """Get all limited API macros from headers.

    Runs the preprocessor over all the header files in "Include" setting
    "-DPy_LIMITED_API" to the correct value for the running version of the
    interpreter and extracting all macro definitions (via adding -dM to the
    compiler arguments).

    Requires Python built with a GCC-compatible compiler. (clang might work)
    """

    api_hexversion = sys.version_info.major << 24 | sys.version_info.minor << 16

    preprocessor_output_with_macros = subprocess.check_output(
        sysconfig.get_config_var("CC").split()
        + [
            # Prevent the expansion of the exported macros so we can
            # capture them later
            "-DSIZEOF_WCHAR_T=4",  # The actual value is not important
            f"-DPy_LIMITED_API={api_hexversion}",
            "-I.",
            "-I./Include",
            "-dM",
            "-E",
        ]
        + [str(file) for file in headers],
        text=True,
    )

    return {
        target
        for target in re.findall(
            r"#define (\w+)", preprocessor_output_with_macros
        )
    }


def gcc_get_limited_api_definitions(headers):
    """Get all limited API definitions from headers.

    Run the preprocessor over all the header files in "Include" setting
    "-DPy_LIMITED_API" to the correct value for the running version of the
    interpreter.

    The limited API symbols will be extracted from the output of this command
    as it includes the prototypes and definitions of all the exported symbols
    that are in the limited api.

    This function does *NOT* extract the macros defined on the limited API

    Requires Python built with a GCC-compatible compiler. (clang might work)
    """
    api_hexversion = sys.version_info.major << 24 | sys.version_info.minor << 16
    preprocessor_output = subprocess.check_output(
        sysconfig.get_config_var("CC").split()
        + [
            # Prevent the expansion of the exported macros so we can capture
            # them later
            "-DPyAPI_FUNC=__PyAPI_FUNC",
            "-DPyAPI_DATA=__PyAPI_DATA",
            "-DEXPORT_DATA=__EXPORT_DATA",
            "-D_Py_NO_RETURN=",
            "-DSIZEOF_WCHAR_T=4",  # The actual value is not important
            f"-DPy_LIMITED_API={api_hexversion}",
            "-I.",
            "-I./Include",
            "-E",
        ]
        + [str(file) for file in headers],
        text=True,
        stderr=subprocess.DEVNULL,
    )
    stable_functions = set(
        re.findall(r"__PyAPI_FUNC\(.*?\)\s*(.*?)\s*\(", preprocessor_output)
    )
    stable_exported_data = set(
        re.findall(r"__EXPORT_DATA\((.*?)\)", preprocessor_output)
    )
    stable_data = set(
        re.findall(r"__PyAPI_DATA\(.*?\)[\s\*\(]*([^);]*)\)?.*;", preprocessor_output)
    )
    return stable_data | stable_exported_data | stable_functions

def check_private_names(manifest):
    """Ensure limited API doesn't contain private names

    Names prefixed by an underscore are private by definition.
    """
    for name, item in manifest.contents.items():
        if name.startswith('_') and not item.abi_only:
            raise ValueError(
                f'`{name}` is private (underscore-prefixed) and should be '
                + 'removed from the stable ABI list or or marked `abi_only`')

def check_dump(manifest, filename):
    """Check that manifest.dump() corresponds to the data.

    Mainly useful when debugging this script.
    """
    dumped = tomllib.loads('\n'.join(manifest.dump()))
    with filename.open('rb') as file:
        from_file = tomllib.load(file)
    if dumped != from_file:
        print(f'Dump differs from loaded data!', file=sys.stderr)
        diff = difflib.unified_diff(
            pprint.pformat(dumped).splitlines(),
            pprint.pformat(from_file).splitlines(),
            '<dumped>', str(filename),
            lineterm='',
        )
        for line in diff:
            print(line, file=sys.stderr)
        return False
    else:
        return True

def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "file", type=Path, metavar='FILE',
        help="file with the stable abi manifest",
    )
    parser.add_argument(
        "--generate", action='store_true',
        help="generate file(s), rather than just checking them",
    )
    parser.add_argument(
        "--generate-all", action='store_true',
        help="as --generate, but generate all file(s) using default filenames."
            + " (unlike --all, does not run any extra checks)",
    )
    parser.add_argument(
        "-a", "--all", action='store_true',
        help="run all available checks using default filenames",
    )
    parser.add_argument(
        "-l", "--list", action='store_true',
        help="list available generators and their default filenames; then exit",
    )
    parser.add_argument(
        "--dump", action='store_true',
        help="dump the manifest contents (used for debugging the parser)",
    )

    actions_group = parser.add_argument_group('actions')
    for gen in generators:
        actions_group.add_argument(
            gen.arg_name, dest=gen.var_name,
            type=str, nargs="?", default=MISSING,
            metavar='FILENAME',
            help=gen.__doc__,
        )
    actions_group.add_argument(
        '--unixy-check', action='store_true',
        help=do_unixy_check.__doc__,
    )
    args = parser.parse_args()

    base_path = args.file.parent.parent

    if args.list:
        for gen in generators:
            print(f'{gen.arg_name}: {base_path / gen.default_path}')
        sys.exit(0)

    run_all_generators = args.generate_all

    if args.generate_all:
        args.generate = True

    if args.all:
        run_all_generators = True
        if UNIXY:
            args.unixy_check = True

    try:
        file = args.file.open('rb')
    except FileNotFoundError as err:
        if args.file.suffix == '.txt':
            # Provide a better error message
            suggestion = args.file.with_suffix('.toml')
            raise FileNotFoundError(
                f'{args.file} not found. Did you mean {suggestion} ?') from err
        raise
    with file:
        manifest = parse_manifest(file)

    check_private_names(manifest)

    # Remember results of all actions (as booleans).
    # At the end we'll check that at least one action was run,
    # and also fail if any are false.
    results = {}

    if args.dump:
        for line in manifest.dump():
            print(line)
        results['dump'] = check_dump(manifest, args.file)

    for gen in generators:
        filename = getattr(args, gen.var_name)
        if filename is None or (run_all_generators and filename is MISSING):
            filename = base_path / gen.default_path
        elif filename is MISSING:
            continue

        results[gen.var_name] = generate_or_check(manifest, args, filename, gen)

    if args.unixy_check:
        results['unixy_check'] = do_unixy_check(manifest, args)

    if not results:
        if args.generate:
            parser.error('No file specified. Use --help for usage.')
        parser.error('No check specified. Use --help for usage.')

    failed_results = [name for name, result in results.items() if not result]

    if failed_results:
        raise Exception(f"""
        These checks related to the stable ABI did not succeed:
            {', '.join(failed_results)}

        If you see diffs in the output, files derived from the stable
        ABI manifest the were not regenerated.
        Run `make regen-limited-abi` to fix this.

        Otherwise, see the error(s) above.

        The stable ABI manifest is at: {args.file}
        Note that there is a process to follow when modifying it.

        You can read more about the limited API and its contracts at:

        https://docs.python.org/3/c-api/stable.html

        And in PEP 384:

        https://peps.python.org/pep-0384/
        """)


if __name__ == "__main__":
    main()
