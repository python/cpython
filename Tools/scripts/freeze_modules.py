import os
import os.path
import subprocess
import sys
import textwrap


SCRIPTS_DIR = os.path.abspath(os.path.dirname(__file__))
TOOLS_DIR = os.path.dirname(SCRIPTS_DIR)
ROOT_DIR = os.path.dirname(TOOLS_DIR)

STDLIB_DIR = os.path.join(ROOT_DIR, 'Lib')
MODULES_DIR = os.path.join(ROOT_DIR, 'Python')
#MODULES_DIR = os.path.join(ROOT_DIR, 'Modules')
TOOL = os.path.join(ROOT_DIR, 'Programs', '_freeze_module')

FROZEN_FILE = os.path.join(ROOT_DIR, 'Python', 'frozen.c')
MAKEFILE = os.path.join(ROOT_DIR, 'Makefile.pre.in')

# These are modules that get frozen.
FROZEN = {
    # frozen_id: (pyfile, frozenfile)
    # frozen_id: pyfile

    # importlib
    'importlib._bootstrap': (None, 'importlib.h'),
    'importlib._bootstrap_external': (None, 'importlib_external.h'),
    'zipimport': (None, 'importlib_zipimport.h'),
    # stdlib
    # ...
    # test
    'hello': os.path.join(TOOLS_DIR, 'freeze', 'flag.py'),
}
FROZEN_GROUPS = {
    'importlib': [
        'importlib._bootstrap',
        'importlib._bootstrap_external',
        'zipimport',
    ],
    'stdlib': [
        # without site (python -S):

        # with site:

    ],
    'test': [
        'hello',
    ],
}
# These are the modules defined in frozen.c, in order.
MODULES = [
    # frozen_id
    # (module, frozen_id)
    # (<package>, frozen_id)

    # importlib
    '# [importlib]',
    ('_frozen_importlib', 'importlib._bootstrap'),
    ('_frozen_importlib_external', 'importlib._bootstrap_external'),
    'zipimport',

    # stdlib
    '# [stdlib]',
    '# without site (python -S)',
    # ...
    '# with site',
    # ...

    # test
    '# [Test module]',
    ('__hello__', 'hello'),
    '# Test package (negative size indicates package-ness)',
    ('<__phello__>', 'hello'),
    ('__phello__.spam', 'hello'),
]


def expand_frozen(destdir=MODULES_DIR):
    # First, expand FROZEN.
    frozen = {}
    packages = {}
    for frozenid, info in FROZEN.items():
        if frozenid.startswith('<'):
            assert info is None, (frozenid, info)
            modids = packages[frozenid] = []
            pkgname = frozenid[1:-1]
            _pkgname, sep, match = pkgname.rpartition('.')
            if not sep or match.isidentifier():
                frozen[pkgname] = (None, None, True)
                modids.append(pkgname)
            else:
                pkgname = _pkgname
                frozen[pkgname] = (None, None, True)
                modids.append(okgname)
                for subname, ispkg in _iter_submodules(pkgname, match=match):
                    frozen[subname] = (None, None, ispkg)
                    modids.append(subname)
        else:
            if info is None or isinstance(info, str):
                info = (info, None, False)
            else:
                pyfile, frozenfile = info
                info = (pyfile, frozenfile, False)
            frozen[frozenid] = info
    for frozenid, info in frozen.items():
        pyfile, frozenfile, ispkg = info

        if not pyfile:
            pyfile = _resolve_module(frozenid, ispkg)
            info = (pyfile, frozenfile, ispkg)

        if not frozenfile:
            frozenfile = _resolve_frozen(frozenid, destdir)
#            frozenfile = f'frozen_{frozenid}'
#            frozenfile = os.path.join(destdir, frozenfile)
            info = (pyfile, frozenfile, ispkg)
        elif not os.path.isabs(frozenfile):
            frozenfile = os.path.join(destdir, frozenfile)
            info = (pyfile, frozenfile, ispkg)

        frozen[frozenid] = info

    # Then, expand FROZEN_GROUPS.
    groups = {}
    for groupname, frozenids in FROZEN_GROUPS.items():
        group = groups[groupname] = []
        for frozenid in frozenids:
            if frozenid in packages:
                group.extend(packages[frozenid])
            else:
                group.append(frozenid)

    # Finally, expand MODULES.
    rows = []
    for row in MODULES:
        if isinstance(row, str) and not row.startswith('#'):
            if row in packages:
                for modname in packages[row]:
                    rows.append((modname, modname))
                continue
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
            ispkg = True
        spec = (modname, frozenid, ispkg)
        specs.append(spec)

    return groups, frozen, headers, specs


#def freeze_stdlib_module(modname, destdir=MODULES_DIR):
#    if modname.startswith('<'):
#        pkgname = modname[1:-1]
#        pkgname, sep, match = pkgname.rpartition('.')
#        if not sep or match.isidentifier():
#            pyfile = _resolve_module(pkgname, ispkg=True)
#            frozenfile = _resolve_frozen(pkgname, destdir)
#            _freeze_module(pkgname, pyfile, frozenfile)
#        else:
#            pkgdir = os.path.dirname(pyfile)
#            for subname, ispkg in _iter_submodules(pkgname, pkgdir, match):
#                pyfile = _resolve_module(subname, ispkg)
#                frozenfile = _resolve_frozen(subname, destdir)
#                _freeze_module(subname, pyfile, frozenfile)
#    else:
#        pyfile = _resolve_module(modname, ispkg=False)
#        frozenfile = _resolve_frozen(modname, destdir)
#        _freeze_module(modname, pyfile, frozenfile)


def regen_frozen(headers, specs):
    headerlines = []
    for row in headers:
        assert isinstance(row, str), row
        if row == '/* Test module */':
            comment = textwrap.dedent('''
                /* In order to test the support for frozen modules, by default we
                   define a single frozen module, __hello__.  Loading it will print
                   some famous words... */

                /* Run "make regen-frozen" to regen the file below (e.g. after a bytecode
                 * format change).  The include file defines _Py_M__hello as an array of bytes.
                 */
                 '''.rstrip())
            headerlines.extend(comment.splitlines())
            continue
        elif not row.startswith('/*'):
            assert row.endswith('.h')
            assert os.path.basename(row) == row, row
            row = f'#include "{row}"'
        headerlines.append(row)

    speclines = []
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
    for i, line in enumerate(speclines):
        if line:
            speclines[i] = indent + line

    text = textwrap.dedent('''\
        /* Auto-generated by Tools/scripts/freeze_modules.py */";

        /* Frozen modules initializer */

        #include "Python.h"
        %s

        static const struct _frozen _PyImport_FrozenModules[] = {
        %s

            {0, 0, 0} /* sentinel */
        };

        /* Embedding apps may change this pointer to point to their favorite
           collection of frozen modules: */

        const struct _frozen *PyImport_FrozenModules = _PyImport_FrozenModules;
        '''
    ) % (
        os.linesep.join(headerlines),
        os.linesep.join(speclines),
    )
    with open(FROZEN_FILE, 'w') as outfile:
        outfile.write(text)


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

    tmpfile = MAKEFILE + '.tmp'
    try:
        with open(tmpfile, 'w') as outfile:
            with open(MAKEFILE) as infile:
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
        os.rename(tmpfile, MAKEFILE)
    finally:
        if os.path.exists(tmpfile):
            os.remove(tmpfile)


def _freeze_module(frozenid, pyfile, frozenfile):
    tmpfile = frozenfile + '.new'

    argv = [TOOL, frozenid, pyfile, tmpfile]
    print('#', ' '.join(argv))
    subprocess.run(argv, check=True)

    os.replace(tmpfile, frozenfile)


def _resolve_module(modname, ispkg=False):
    if ispkg:
        return os.path.join(STDLIB_DIR, *modname.split('.'), '__init__.py')
    return os.path.join(STDLIB_DIR, *modname.split('.')) + '.py'


def _resolve_frozen(modname, destdir):
    return os.path.join(destdir, 'frozen_' + modname.replace('.', '_')) + '.h'


def _iter_submodules(pkgname, pkgdir=None, match='*'):
    if not pkgdir:
        pkgdir = os.path.join(STDLIB_DIR, *pkgname.split('.'))
    if not match:
        match = '*'
    if match != '*':
        raise NotImplementedError(match)

    for entry in os.scandir(pkgdir):
        modname = f'{pkgname}.{entry.name}'
        if modname.endswith('.py'):
            yield modname[:-3], False
        elif entry.is_dir():
            if os.path.exists(os.path.join(entry.path, '__init__.py')):
                yield modname, True
                yield from _iter_submodules(modname, entry.path)


#######################################
# the script

def parse_args(argv=sys.argv[1:], prog=sys.argv[0]):
    KINDS = ['all', *sorted(FROZEN_GROUPS)]
    usage = f'usage: {prog} [-h] [{"|".join(KINDS)}] ...'
    if '-h' in argv or '--help' in argv:
        print(usage)
        sys.exit(0)
    else:
        kinds = set()
        for arg in argv:
            if arg not in KINDS:
                print(usage, file=sys.stderr)
                sys.exit(f'ERROR: unsupported kind {arg!r}')
            kinds.add(arg)
    return {'kinds': kinds if kinds and 'all' not in kinds else None}


def main(kinds=None):
    groups, frozen, headers, specs = expand_frozen(MODULES_DIR)

    # First, freeze the modules.
    # (We use a consistent order:)
    ordered = ['importlib', 'stdlib', 'test']
    assert (set(ordered) == set(groups))
    for kind in ordered:
        if not kinds or kind in kinds:
            for frozenid in groups[kind]:
                #freeze_module(frozenid, frozen)
                pyfile, frozenfile, _ = frozen[frozenid]
                _freeze_module(frozenid, pyfile, frozenfile)

    # Then regen frozen.c and Makefile.pre.in.
    regen_frozen(headers, specs)
    regen_makefile(headers, MODULES_DIR)


if __name__ == '__main__':
    kwargs = parse_args()
    main(**kwargs)
