import argparse
import py_compile
import re
import sys
import shutil
import stat
import os
import tempfile

from itertools import chain
from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED
import subprocess

TKTCL_RE = re.compile(r'^(_?tk|tcl).+\.(pyd|dll)', re.IGNORECASE)
DEBUG_RE = re.compile(r'_d\.(pyd|dll|exe|pdb|lib)$', re.IGNORECASE)
PYTHON_DLL_RE = re.compile(r'python\d\d?\.dll$', re.IGNORECASE)

DEBUG_FILES = {
    '_ctypes_test',
    '_testbuffer',
    '_testcapi',
    '_testconsole',
    '_testimportmultiple',
    '_testmultiphase',
    'xxlimited',
    'python3_dstub',
}

EXCLUDE_FROM_LIBRARY = {
    '__pycache__',
    'idlelib',
    'pydoc_data',
    'site-packages',
    'tkinter',
    'turtledemo',
}

EXCLUDE_FROM_EMBEDDABLE_LIBRARY = {
    'ensurepip',
    'venv',
}

EXCLUDE_FILE_FROM_LIBRARY = {
    'bdist_wininst.py',
}

EXCLUDE_FILE_FROM_LIBS = {
    'liblzma',
    'ssleay',
    'libeay',
    'python3stub',
}

EXCLUDED_FILES = {
    'pyshellext',
}

def is_not_debug(p):
    if DEBUG_RE.search(p.name):
        return False

    if TKTCL_RE.search(p.name):
        return False

    return p.stem.lower() not in DEBUG_FILES and p.stem.lower() not in EXCLUDED_FILES

def is_not_debug_or_python(p):
    return is_not_debug(p) and not PYTHON_DLL_RE.search(p.name)

def include_in_lib(p):
    name = p.name.lower()
    if p.is_dir():
        if name in EXCLUDE_FROM_LIBRARY:
            return False
        if name == 'test' and p.parts[-2].lower() == 'lib':
            return False
        if name in {'test', 'tests'} and p.parts[-3].lower() == 'lib':
            return False
        return True

    if name in EXCLUDE_FILE_FROM_LIBRARY:
        return False

    suffix = p.suffix.lower()
    return suffix not in {'.pyc', '.pyo', '.exe'}

def include_in_embeddable_lib(p):
    if p.is_dir() and p.name.lower() in EXCLUDE_FROM_EMBEDDABLE_LIBRARY:
        return False

    return include_in_lib(p)

def include_in_libs(p):
    if not is_not_debug(p):
        return False

    return p.stem.lower() not in EXCLUDE_FILE_FROM_LIBS

def include_in_tools(p):
    if p.is_dir() and p.name.lower() in {'scripts', 'i18n', 'pynche', 'demo', 'parser'}:
        return True

    return p.suffix.lower() in {'.py', '.pyw', '.txt'}

BASE_NAME = 'python{0.major}{0.minor}'.format(sys.version_info)

FULL_LAYOUT = [
    ('/', '$build', 'python.exe', is_not_debug),
    ('/', '$build', 'pythonw.exe', is_not_debug),
    ('/', '$build', 'python{}.dll'.format(sys.version_info.major), is_not_debug),
    ('/', '$build', '{}.dll'.format(BASE_NAME), is_not_debug),
    ('DLLs/', '$build', '*.pyd', is_not_debug),
    ('DLLs/', '$build', '*.dll', is_not_debug_or_python),
    ('include/', 'include', '*.h', None),
    ('include/', 'PC', 'pyconfig.h', None),
    ('Lib/', 'Lib', '**/*', include_in_lib),
    ('libs/', '$build', '*.lib', include_in_libs),
    ('Tools/', 'Tools', '**/*', include_in_tools),
]

EMBED_LAYOUT = [
    ('/', '$build', 'python*.exe', is_not_debug),
    ('/', '$build', '*.pyd', is_not_debug),
    ('/', '$build', '*.dll', is_not_debug),
    ('{}.zip'.format(BASE_NAME), 'Lib', '**/*', include_in_embeddable_lib),
]

if os.getenv('DOC_FILENAME'):
    FULL_LAYOUT.append(('Doc/', 'Doc/build/htmlhelp', os.getenv('DOC_FILENAME'), None))
if os.getenv('VCREDIST_PATH'):
    FULL_LAYOUT.append(('/', os.getenv('VCREDIST_PATH'), 'vcruntime*.dll', None))
    EMBED_LAYOUT.append(('/', os.getenv('VCREDIST_PATH'), 'vcruntime*.dll', None))

def copy_to_layout(target, rel_sources):
    count = 0

    if target.suffix.lower() == '.zip':
        if target.exists():
            target.unlink()

        with ZipFile(str(target), 'w', ZIP_DEFLATED) as f:
            with tempfile.TemporaryDirectory() as tmpdir:
                for s, rel in rel_sources:
                    if rel.suffix.lower() == '.py':
                        pyc = Path(tmpdir) / rel.with_suffix('.pyc').name
                        try:
                            py_compile.compile(str(s), str(pyc), str(rel), doraise=True, optimize=2)
                        except py_compile.PyCompileError:
                            f.write(str(s), str(rel))
                        else:
                            f.write(str(pyc), str(rel.with_suffix('.pyc')))
                    else:
                        f.write(str(s), str(rel))
                    count += 1

    else:
        for s, rel in rel_sources:
            dest = target / rel
            try:
                dest.parent.mkdir(parents=True)
            except FileExistsError:
                pass
            if dest.is_file():
                dest.chmod(stat.S_IWRITE)
            shutil.copy(str(s), str(dest))
            if dest.is_file():
                dest.chmod(stat.S_IWRITE)
            count += 1

    return count

def rglob(root, pattern, condition):
    dirs = [root]
    recurse = pattern[:3] in {'**/', '**\\'}
    while dirs:
        d = dirs.pop(0)
        for f in d.glob(pattern[3:] if recurse else pattern):
            if recurse and f.is_dir() and (not condition or condition(f)):
                dirs.append(f)
            elif f.is_file() and (not condition or condition(f)):
                yield f, f.relative_to(root)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--source', metavar='dir', help='The directory containing the repository root', type=Path)
    parser.add_argument('-o', '--out', metavar='file', help='The name of the output archive', type=Path, default=None)
    parser.add_argument('-t', '--temp', metavar='dir', help='A directory to temporarily extract files into', type=Path, default=None)
    parser.add_argument('-e', '--embed', help='Create an embedding layout', action='store_true', default=False)
    parser.add_argument('-b', '--build', help='Specify the build directory', type=Path, default=None)
    ns = parser.parse_args()

    source = ns.source or (Path(__file__).resolve().parent.parent.parent)
    out = ns.out
    build = ns.build or Path(sys.exec_prefix)
    assert isinstance(source, Path)
    assert not out or isinstance(out, Path)
    assert isinstance(build, Path)

    if ns.temp:
        temp = ns.temp
        delete_temp = False
    else:
        temp = Path(tempfile.mkdtemp())
        delete_temp = True

    if out:
        try:
            out.parent.mkdir(parents=True)
        except FileExistsError:
            pass
    try:
        temp.mkdir(parents=True)
    except FileExistsError:
        pass

    layout = EMBED_LAYOUT if ns.embed else FULL_LAYOUT

    try:
        for t, s, p, c in layout:
            if s == '$build':
                fs = build
            else:
                fs = source / s
            files = rglob(fs, p, c)
            extra_files = []
            if s == 'Lib' and p == '**/*':
                extra_files.append((
                    source / 'tools' / 'msi' / 'distutils.command.bdist_wininst.py',
                    Path('distutils') / 'command' / 'bdist_wininst.py'
                ))
            copied = copy_to_layout(temp / t.rstrip('/'), chain(files, extra_files))
            print('Copied {} files'.format(copied))

        if ns.embed:
            with open(str(temp / (BASE_NAME + '._pth')), 'w') as f:
                print(BASE_NAME + '.zip', file=f)
                print('.', file=f)
                print('', file=f)
                print('# Uncomment to run site.main() automatically', file=f)
                print('#import site', file=f)

        if out:
            total = copy_to_layout(out, rglob(temp, '**/*', None))
            print('Wrote {} files to {}'.format(total, out))
    finally:
        if delete_temp:
            shutil.rmtree(temp, True)


if __name__ == "__main__":
    sys.exit(int(main() or 0))
