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
    # frozenid
    # <frozenid.**.*>
    # (frozenid, pyfile)

    # importlib
    'importlib._bootstrap',
    'importlib._bootstrap_external',
    'zipimport',
    # test
    ('hello', os.path.join(TOOLS_DIR, 'freeze', 'flag.py')),
]
FROZEN_GROUPS = {
    # group: [frozenid]
    # group: [<frozenid.**.*>]
    'importlib': [
        'importlib._bootstrap',
        'importlib._bootstrap_external',
        'zipimport',
    ],
    'test': [
        'hello',
    ],
}
# These are the modules defined in frozen.c, in order.
MODULES = [
    # frozenid
    # <frozenid>
    # (module, frozenid)
    # (<package>, frozenid)

    # importlib
    '# [importlib]',
    ('_frozen_importlib', 'importlib._bootstrap'),
    ('_frozen_importlib_external', 'importlib._bootstrap_external'),
    'zipimport',

    # test
    '# [Test module]',
    ('__hello__', 'hello'),
    '# Test package (negative size indicates package-ness)',
    ('<__phello__>', 'hello'),
    ('__phello__.spam', 'hello'),
]


def expand_frozen(destdir=MODULES_DIR):
    # First, expand FROZEN.
    _frozen = []
    packages = {}
    for row in FROZEN:
        frozenid, pyfile = (row, None) if isinstance(row, str) else row
        resolved = iter(resolve_modules(frozenid, pyfile))
        modname, _pyfile, ispkg = next(resolved)
        if not pyfile:
            pyfile = _pyfile
        _frozen.append((modname, pyfile, ispkg))
        if ispkg:
            modids = packages[frozenid] = [modname]
            for subname, subpyfile, ispkg in resolved:
                frozen[subname] = (subpyfile, ispkg)
                _frozen.append((subname, subpyfile, ispkg))
                modids.append(subname)
        else:
            assert not list(resolved)
    frozen = {}
    for frozenid, pyfile, ispkg in _frozen:
        if not pyfile:
            pyfile = _resolve_module(frozenid, ispkg=ispkg)
        frozenfile = _resolve_frozen(frozenid, destdir)
        frozen[frozenid] = (pyfile, frozenfile, ispkg)

    # Then, expand FROZEN_GROUPS.
    groups = {}
    for groupname, frozenids in FROZEN_GROUPS.items():
        group = groups[groupname] = []
        for frozenid in frozenids:
            if frozenid in packages:
                group.extend(packages[frozenid])
            else:
                assert frozenid in frozen, (frozenid, frozen)
                group.append(frozenid)

    # Finally, expand MODULES.
    rows = []
    for row in MODULES:
        if isinstance(row, str) and not row.startswith('#'):
            if row in packages:
                for modname in packages[row]:
                    rows.append((modname, modname))
            else:
                row = (row, row)
        rows.append(row)
    headers = []
    specs = []
    for row in rows:
        if isinstance(row, str):
            assert row.startswith('# ')
            comment = row[2:]
            if comment.startswith('['):
                line = f'/* {comment[1:-1]} */'
                headers.append(line)
                specs.extend(['', line])
            else:
                specs.append(f'/* {comment} */')
            continue
        modname, frozenid = row
        _, frozenfile, ispkg = frozen[frozenid]

        assert frozenfile.startswith(destdir), (frozenfile, destdir)
        header = os.path.relpath(frozenfile, destdir)
        if header not in headers:
            headers.append(header)

        if modname.startswith('<'):
            modname = modname[1:-1]
            assert check_modname(modname), modname
            ispkg = True
        spec = (modname, frozenid, ispkg)
        specs.append(spec)

    return groups, frozen, headers, specs


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

def regen_frozen(headers, specs):
    headerlines = []
    for row in headers:
        assert isinstance(row, str), row
        if not row.startswith('/*'):
            assert row.endswith('.h')
            assert os.path.basename(row) == row, row
            row = f'#include "{row}"'
        headerlines.append(row)

    speclines = []
    sentinel = '{0, 0, 0} /* sentinel */'
    indent = '    '
    for spec in specs:
        if isinstance(spec, str):
            speclines.append(spec)
            continue
        modname, frozenid, ispkg = spec
        # This matches what we do in Programs/_freeze_module.c:
        symbol = '_Py_M__' + frozenid.replace('.', '_')
        pkg = '-' if ispkg else ''
        line = '{"%s", %s, %s(int)sizeof(%s)},' % (modname, symbol, pkg, symbol)
        if len(line) < 80:
            speclines.append(line)
        else:
            line1, _, line2 = line.rpartition(' ')
            speclines.append(line1)
            speclines.append(indent + line2)
    if not speclines[0]:
        del speclines[0]
    speclines.extend(['', sentinel])
    for i, line in enumerate(speclines):
        if line:
            speclines[i] = indent + line

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
        for line in speclines:
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
    KINDS = ['all', *sorted(FROZEN_GROUPS)]
    import argparse
    parser = argparse.ArgumentParser(prog=prog)
    parser.add_argument('--no-regen', dest='regen', action='store_false')
    parser.add_argument('kinds', nargs='*', choices=KINDS, default='all')

    args = parser.parse_args(argv)
    ns = vars(args)

    if not args.kinds or 'all' in args.kinds:
        args.kinds = None

    return ns


def main(kinds=None, *, regen=True):
    groups, frozen, headers, specs = expand_frozen(MODULES_DIR)

    # First, freeze the modules.
    # (We use a consistent order:)
    ordered = ['importlib', 'test']
    assert (set(ordered) == set(groups))
    for kind in ordered:
        if not kinds or kind in kinds:
            for frozenid in groups[kind]:
                #freeze_module(frozenid, frozen)
                pyfile, frozenfile, _ = frozen[frozenid]
                _freeze_module(frozenid, pyfile, frozenfile)

    if regen:
        # Then regen frozen.c and Makefile.pre.in.
        regen_frozen(headers, specs)
        regen_makefile(headers, MODULES_DIR)


if __name__ == '__main__':
    kwargs = parse_args()
    main(**kwargs)
