"""Freeze modules and regen related files (e.g. Python/frozen.c).

See the notes at the top of Python/frozen.c for more info.
"""

import os
import os.path
import subprocess
import sys
import textwrap

from update_file import updating_file_with_tmpfile


SCRIPTS_DIR = os.path.abspath(os.path.dirname(__file__))
TOOLS_DIR = os.path.dirname(SCRIPTS_DIR)
ROOT_DIR = os.path.dirname(TOOLS_DIR)

STDLIB_DIR = os.path.join(ROOT_DIR, 'Lib')
# XXX It may make more sense to put frozen modules in Modules/ or Lib/...
MODULES_DIR = os.path.join(ROOT_DIR, 'Python')
TOOL = os.path.join(ROOT_DIR, 'Programs', '_freeze_module')

FROZEN_FILE = os.path.join(ROOT_DIR, 'Python', 'frozen.c')
MAKEFILE = os.path.join(ROOT_DIR, 'Makefile.pre.in')

# These are modules that get frozen.
FROZEN = [
    # See parse_frozen_spec() for the format.
    # The comments are added to Python/frozen.c at the relative position.
    # In cases where the frozenid is duplicated, the first one is re-used.

    '# [importlib]',
    'importlib._bootstrap : _frozen_importlib',
    'importlib._bootstrap_external : _frozen_importlib_external',
    'zipimport',

    '# [Test module]',
    'hello : __hello__ = ' + os.path.join(TOOLS_DIR, 'freeze', 'flag.py'),
    '# Test package (negative size indicates package-ness)',
    'hello : <__phello__>',
    'hello : __phello__.spam',
]


def parse_frozen_spec(spec, known=None):
    """Yield (frozenid, pyfile, modname, ispkg) for the corresponding modules.

    Supported formats:

      frozenid
      frozenid : modname
      frozenid : modname = pyfile

    "frozenid" and "modname" must be valid module names (dot-separated
    identifiers).  If "modname" is not provided then "frozenid" is used.
    If "pyfile" is not provided then the filename of the module
    corresponding to "frozenid" is used.

    Angle brackets around a frozenid (e.g. '<encodings>") indicate
    it is a package.  This also means it must be an actual module
    (i.e. "pyfile" cannot have been provided).  Such values can have
    patterns to expand submodules:

      <encodings.*>    - also freeze all direct submodules
      <encodings.**.*> - also freeze the full submodule tree

    As with "frozenid", angle brackets around "modname" indicate
    it is a package.  However, in this case "pyfile" should not
    have been provided and patterns in "modname" are not supported.
    Also, if "modname" has brackets then "frozenid" should not,
    and "pyfile" should have been provided..
    """
    frozenid, _, remainder = spec.partition(':')
    modname, _, pyfile = remainder.partition('=')
    frozenid = frozenid.strip()
    modname = modname.strip()
    pyfile = pyfile.strip()

    if modname.startswith('<') and modname.endswith('>'):
        assert check_modname(frozenid), spec
        modname = modname[1:-1]
        assert check_modname(modname), spec
        if known and frozenid in known:
            assert not pyfile, spec
        elif pyfile:
            assert not os.path.isdir(pyfile), spec
        else:
            pyfile = _resolve_module(frozenid, ispkg=False)
        yield frozenid, pyfile or None, modname, True
    elif pyfile:
        assert check_modname(frozenid), spec
        assert not known or frozenid not in known, spec
        assert check_modname(modname), spec
        assert not os.path.isdir(pyfile), spec
        yield frozenid, pyfile, modname, False
    elif known and frozenid in known:
        assert check_modname(frozenid), spec
        assert check_modname(modname), spec
        yield frozenid, None, modname, False
    else:
        assert not modname or check_modname(modname), spec
        resolved = iter(resolve_modules(frozenid))
        frozenid, pyfile, ispkg = next(resolved)
        yield frozenid, pyfile, modname or frozenid, ispkg
        if ispkg:
            pkgid = frozenid
            pkgname = modname
            for frozenid, pyfile, ispkg in resolved:
                assert not known or frozenid not in known, (frozenid, spec)
                if pkgname:
                    modname = frozenid.replace(pkgid, pkgname, 1)
                else:
                    modname = frozenid
                yield frozenid, pyfile, modname, ispkg


def expand_frozen(destdir=MODULES_DIR):
    frozen = {}
    frozenids = []
    headers = []
    definitions = []
    for spec in FROZEN:
        if spec.startswith('#'):
            comment = spec[2:]
            if comment.startswith('['):
                line = f'/* {comment[1:-1]} */'
                headers.append(line)
                definitions.extend(['', line])
            else:
                definitions.append(f'/* {comment} */')
        else:
            for frozenid, pyfile, modname, ispkg in parse_frozen_spec(spec, frozen):
                if frozenid not in frozen:
                    assert pyfile, spec
                    frozenfile = _resolve_frozen(frozenid, destdir)
#                    assert frozenfile.startswith(destdir), (frozenfile, destdir)
                    frozen[frozenid] = (pyfile, frozenfile)
                    frozenids.append(frozenid)
                else:
                    assert not pyfile, spec

                header = os.path.relpath(frozenfile, destdir)
                if header not in headers:
                    headers.append(header)

                definition = (modname, frozenid, ispkg)
                definitions.append(definition)
    return frozen, frozenids, headers, definitions


def resolve_modules(modname, pyfile=None):
    if modname.startswith('<') and modname.endswith('>'):
        if pyfile:
            assert os.path.isdir(pyfile) or os.path.basename(pyfile) == '__init__.py', pyfile
        ispkg = True
        modname = modname[1:-1]
        rawname = modname
        # For now, we only expect match patterns at the end of the name.
        _modname, sep, match = pkgname.rpartition('.')
        if sep:
            if _modname.endswith('.**'):
                modname = _modname[:-3]
                match = f'**.{match}'
            elif match and not match.isidentifier():
                modname = _modname
            # Otherwise it's a plain name so we leave it alone.
        else:
            match = None
    else:
        ispkg = False
        rawname = modname
        match = None

    if not check_modname(modname):
        raise ValueError(f'not a valid module name ({rawname})')

    if not pyfile:
        pyfile = _resolve_module(modname, ispkg=ispkg)
    elif os.path.isdir(pyfile):
        pyfile = _resolve_module(modname, pyfile, ispkg)
    yield modname, pyfile, ispkg

    if match:
        pkgdir = os.path.dirname(pyfile)
        yield from iter_submodules(modname, pkgdir, match)


def check_modname(modname):
    return all(n.isidentifier() for n in modname.split('.'))


def iter_submodules(pkgname, pkgdir=None, match='*'):
    if not pkgdir:
        pkgdir = os.path.join(STDLIB_DIR, *pkgname.split('.'))
    if not match:
        match = '**.*'
    match_modname = _resolve_modname_matcher(match, pkgdir)

    def _iter_submodules(pkgname, pkgdir):
        for entry in os.scandir(pkgdir):
            matched, recursive = match_modname(entry.name)
            if not matched:
                continue
            modname = f'{pkgname}.{entry.name}'
            if modname.endswith('.py'):
                yield modname[:-3], entry.path, False
            elif entry.is_dir():
                pyfile = os.path.join(entry.path, '__init__.py')
                # We ignore namespace packages.
                if os.path.exists(pyfile):
                    yield modname, pyfile, True
                    if recursive:
                        yield from _iter_submodules(modname, entry.path)

    return _iter_submodules(pkgname, pkgdir)


def _resolve_modname_matcher(match, rootdir=None):
    if isinstance(match, str):
        if match.startswith('**.'):
            recursive = True
            pat = match[3:]
            assert match
        else:
            recursive = False
            pat = match

        if pat == '*':
            def match_modname(modname):
                return True, recursive
        else:
            raise NotImplementedError(match)
    elif callable(match):
        match_modname = match(rootdir)
    else:
        raise ValueError(f'unsupported matcher {match!r}')
    return match_modname


def _resolve_module(modname, pathentry=STDLIB_DIR, ispkg=False):
    assert pathentry, pathentry
    pathentry = os.path.normpath(pathentry)
    assert os.path.isabs(pathentry)
    if ispkg:
        return os.path.join(pathentry, *modname.split('.'), '__init__.py')
    return os.path.join(pathentry, *modname.split('.')) + '.py'


def _resolve_frozen(modname, destdir):
    # We use a consistent naming convention for frozen modules.
    return os.path.join(destdir, 'frozen_' + modname.replace('.', '_')) + '.h'


#######################################
# freezing modules

def freeze_module(modname, pyfile=None, destdir=MODULES_DIR):
    """Generate the frozen module .h file for the given module."""
    for modname, pyfile, ispkg in resolve_modules(modname, pyfile):
        frozenfile = _resolve_frozen(modname, destdir)
        _freeze_module(modname, pyfile, frozenfile)


def _freeze_module(frozenid, pyfile, frozenfile):
    tmpfile = frozenfile + '.new'

    argv = [TOOL, frozenid, pyfile, tmpfile]
    print('#', ' '.join(argv))
    subprocess.run(argv, check=True)

    os.replace(tmpfile, frozenfile)


#######################################
# regenerating dependent files

def regen_frozen(headers, definitions):
    headerlines = []
    for row in headers:
        assert isinstance(row, str), row
        if not row.startswith('/*'):
            assert row.endswith('.h')
            assert os.path.basename(row) == row, row
            row = f'#include "{row}"'
        headerlines.append(row)

    deflines = []
    sentinel = '{0, 0, 0} /* sentinel */'
    indent = '    '
    for definition in definitions:
        if isinstance(definition, str):
            deflines.append(definition)
            continue
        modname, frozenid, ispkg = definition
        # This matches what we do in Programs/_freeze_module.c:
        symbol = '_Py_M__' + frozenid.replace('.', '_')
        pkg = '-' if ispkg else ''
        line = '{"%s", %s, %s(int)sizeof(%s)},' % (modname, symbol, pkg, symbol)
        if len(line) < 80:
            deflines.append(line)
        else:
            line1, _, line2 = line.rpartition(' ')
            deflines.append(line1)
            deflines.append(indent + line2)
    if not deflines[0]:
        del deflines[0]
    deflines.extend(['', sentinel])
    for i, line in enumerate(deflines):
        if line:
            deflines[i] = indent + line

    with updating_file_with_tmpfile(FROZEN_FILE) as (infile, outfile):
        lines = iter(infile)
        # Get to the frozen includes.
        for line in lines:
            outfile.write(line)
            if line.rstrip() == '/* Includes for frozen modules: */':
                break
        else:
            raise Exception("did you delete a line you shouldn't have?")
        # Pop off the existing values.
        blank_before = next(lines)
        assert blank_before.strip() == ''
        for line in lines:
            if not line.strip():
                blank_after = line
                break
        else:
            raise Exception("did you delete a line you shouldn't have?")
        # Add the frozen includes.
        outfile.write(blank_before)
        for line in headerlines:
            outfile.write(line + os.linesep)
        outfile.write(blank_after)
        # Get to the modules array.
        for line in lines:
            outfile.write(line)
            if line.rstrip() == 'static const struct _frozen _PyImport_FrozenModules[] = {':
                break
        else:
            raise Exception("did you delete a line you shouldn't have?")
        # Pop off the existing values.
        for line in lines:
            if line.strip() == sentinel:
                break
        else:
            raise Exception("did you delete a line you shouldn't have?")
        # Add the modules.
        for line in deflines:
            outfile.write(line + os.linesep)
        # Keep the rest of the file.
        for line in lines:
            outfile.write(line)


def regen_makefile(headers, destdir=MODULES_DIR):
    reldir = os.path.relpath(destdir, ROOT_DIR)
    assert destdir.endswith(reldir), destdir
    frozenfiles = []
    for row in headers:
        if not row.startswith('/*'):
            header = row
            assert header.endswith('.h'), header
            assert os.path.basename(header) == header, header
            relfile = f'{reldir}/{header}'.replace('\\', '/')
            frozenfiles.append(f'$(srcdir)/{relfile}')

    with updating_file_with_tmpfile(MAKEFILE) as (infile, outfile):
        lines = iter(infile)
        # Get to $FROZEN_FILES.
        for line in lines:
            outfile.write(line)
            if line.rstrip() == 'FROZEN_FILES = \\':
                break
        else:
            raise Exception("did you delete a line you shouldn't have?")
        # Pop off the existing values.
        for line in lines:
            if line.rstrip() == 'Python/frozen.o: $(FROZEN_FILES)':
                break
        else:
            raise Exception("did you delete a line you shouldn't have?")
        # Add the regen'ed values.
        for header in frozenfiles[:-1]:
            outfile.write(f'\t\t{header} \\{os.linesep}')
        outfile.write(f'\t\t{frozenfiles[-1]}{os.linesep}')
        # Keep the rest of the file.
        outfile.write(os.linesep)
        outfile.write('Python/frozen.o: $(FROZEN_FILES)' + os.linesep)
        for line in lines:
            outfile.write(line)


#######################################
# the script

def parse_args(argv=sys.argv[1:], prog=sys.argv[0]):
    import argparse
    parser = argparse.ArgumentParser(prog=prog)
    parser.add_argument('--no-regen', dest='regen', action='store_false')

    args = parser.parse_args(argv)
    ns = vars(args)

    return ns


def main(*, regen=True):
    frozen, frozenids, headers, definitions = expand_frozen(MODULES_DIR)

    # First, freeze the modules.
    # (We use a consistent order: that of FROZEN above.)
    for frozenid in frozenids:
        pyfile, frozenfile = frozen[frozenid]
        _freeze_module(frozenid, pyfile, frozenfile)

    if regen:
        # Then regen frozen.c and Makefile.pre.in.
        regen_frozen(headers, definitions)
        regen_makefile(headers, MODULES_DIR)


if __name__ == '__main__':
    kwargs = parse_args()
    main(**kwargs)
