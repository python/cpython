import os
import os.path
import shutil
import subprocess


TESTS_DIR = os.path.dirname(__file__)
TOOL_ROOT = os.path.dirname(TESTS_DIR)
SRCDIR = os.path.dirname(os.path.dirname(TOOL_ROOT))

CONFIGURE = os.path.join(SRCDIR, 'configure')
MAKE = shutil.which('make')
FREEZE = os.path.join(TOOL_ROOT, 'freeze.py')
OUTDIR = os.path.join(TESTS_DIR, 'outdir')


class UnsupportedError(Exception):
    """The operation isn't supported."""


def _run_cmd(cmd, cwd, verbose=True):
    proc = subprocess.run(
        cmd,
        cwd=cwd,
        capture_output=not verbose,
        text=True,
    )
    if proc.returncode != 0:
        print(proc.stderr, file=sys.stderr)
        proc.check_returncode()
    return proc.stdout


##################################
# building Python

def configure_python(outdir, prefix=None, cachefile=None, *, verbose=True):
    if not prefix:
        prefix = os.path.join(outdir, 'python-installation')
    builddir = os.path.join(outdir, 'python-build')
    print(f'configuring python in {builddir}...')
    os.makedirs(builddir, exist_ok=True)
    cmd = [CONFIGURE, f'--prefix={prefix}']
    if cachefile:
        cmd.extend(['--cache-file', cachefile])
    _run_cmd(cmd, builddir, verbose)
    return builddir


def get_prefix(build=None):
    if build:
        if os.path.isfile(build):
            return _run_cmd(
                [build, '-c' 'import sys; print(sys.prefix)'],
                cwd=os.path.dirname(build),
            )

        builddir = build
    else:
        builddir = os.path.abspath('.')
    # We have to read it out of Makefile.
    makefile = os.path.join(builddir, 'Makefile')
    try:
        infile = open(makefile)
    except FileNotFoundError:
        return None
    with infile:
        for line in infile:
            if line.startswith('prefix='):
                return line.partition('=')[-1].strip()
    return None


def build_python(builddir=None, *, verbose=True):
    if not MAKE:
        raise UnsupportedError('make')

    if builddir:
        builddir = os.path.abspath(builddir)
    else:
        builddir = SRCDIR
    if builddir != SRCDIR:
        _run_cmd([MAKE, 'clean'], SRCDIR, verbose=False)

    print(f'building python in {builddir}...')
    _run_cmd([MAKE, '-j'], builddir, verbose)

    return os.path.join(builddir, 'python')


def install_python(builddir=None, *, verbose=True):
    if not MAKE:
        raise UnsupportedError('make')

    if not builddir:
        builddir = '.'
    prefix = get_prefix(builddir)

    print(f'installing python into {prefix}...')
    _run_cmd([MAKE, '-j', 'install'], builddir, verbose)

    if not prefix:
        return None
    return os.path.join(prefix, 'bin', 'python3')


##################################
# freezing

def pre_freeze(outdir=None, *, verbose=True):
    if not outdir:
        outdir = OUTDIR
    cachefile = os.path.join(outdir, 'python-config.cache')
    builddir = configure_python(outdir, cachefile=cachefile, verbose=verbose)
    build_python(builddir, verbose=verbose)
    return install_python(builddir, verbose=verbose)


def freeze(python, scriptfile, outdir=None, *, verbose=True):
    if not MAKE:
        raise UnsupportedError('make')

    if not outdir:
        outdir = OUTDIR

    print(f'freezing {scriptfile}...')
    os.makedirs(outdir, exist_ok=True)
    subprocess.run(
        [python, FREEZE, '-o', outdir, scriptfile],
        cwd=outdir,
        capture_output=not verbose,
        check=True,
        )

    subprocess.run(
        [MAKE],
        cwd=os.path.dirname(scriptfile),
        capture_output=not verbose,
        check=True,
        )

    name = os.path.basename(scriptfile).rpartition('.')[0]
    executable = os.path.join(outdir, name)
    return executable


def run(executable):
    proc = subprocess.run(
        [executable],
        capture_output=True,
        text=True,
        check=True,
    )
    return proc.stdout.strip()
