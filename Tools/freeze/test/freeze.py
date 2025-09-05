import os
import os.path
import shlex
import shutil
import subprocess
import sysconfig
from test import support


def get_python_source_dir():
    src_dir = sysconfig.get_config_var('abs_srcdir')
    if not src_dir:
        src_dir = sysconfig.get_config_var('srcdir')
    return os.path.abspath(src_dir)


TESTS_DIR = os.path.dirname(__file__)
TOOL_ROOT = os.path.dirname(TESTS_DIR)
SRCDIR = get_python_source_dir()

MAKE = shutil.which('make')
FREEZE = os.path.join(TOOL_ROOT, 'freeze.py')
OUTDIR = os.path.join(TESTS_DIR, 'outdir')


class UnsupportedError(Exception):
    """The operation isn't supported."""


def _run_quiet(cmd, *, cwd=None):
    if cwd:
        print('+', 'cd', cwd, flush=True)
    print('+', shlex.join(cmd), flush=True)
    try:
        return subprocess.run(
            cmd,
            cwd=cwd,
            capture_output=True,
            text=True,
            check=True,
        )
    except subprocess.CalledProcessError as err:
        # Don't be quiet if things fail
        print(f"{err.__class__.__name__}: {err}")
        print("--- STDOUT ---")
        print(err.stdout)
        print("--- STDERR ---")
        print(err.stderr)
        print("---- END ----")
        raise


def _run_stdout(cmd):
    proc = _run_quiet(cmd)
    return proc.stdout.strip()


def find_opt(args, name):
    opt = f'--{name}'
    optstart = f'{opt}='
    for i, arg in enumerate(args):
        if arg == opt or arg.startswith(optstart):
            return i
    return -1


def ensure_opt(args, name, value):
    opt = f'--{name}'
    pos = find_opt(args, name)
    if value is None:
        if pos < 0:
            args.append(opt)
        else:
            args[pos] = opt
    elif pos < 0:
        args.extend([opt, value])
    else:
        arg = args[pos]
        if arg == opt:
            if pos == len(args) - 1:
                raise NotImplementedError((args, opt))
            args[pos + 1] = value
        else:
            args[pos] = f'{opt}={value}'


def copy_source_tree(newroot, oldroot):
    print(f'copying the source tree from {oldroot} to {newroot}...')
    if os.path.exists(newroot):
        if newroot == SRCDIR:
            raise Exception('this probably isn\'t what you wanted')
        shutil.rmtree(newroot)

    shutil.copytree(oldroot, newroot, ignore=support.copy_python_src_ignore)
    if os.path.exists(os.path.join(newroot, 'Makefile')):
        # Out-of-tree builds require a clean srcdir. "make clean" keeps
        # the "python" program, so use "make distclean" instead.
        _run_quiet([MAKE, 'distclean'], cwd=newroot)


##################################
# freezing

def prepare(script=None, outdir=None):
    print()
    print("cwd:", os.getcwd())

    if not outdir:
        outdir = OUTDIR
    os.makedirs(outdir, exist_ok=True)

    # Write the script to disk.
    if script:
        scriptfile = os.path.join(outdir, 'app.py')
        print(f'creating the script to be frozen at {scriptfile}')
        with open(scriptfile, 'w', encoding='utf-8') as outfile:
            outfile.write(script)

    # Make a copy of the repo to avoid affecting the current build
    # (e.g. changing PREFIX).
    srcdir = os.path.join(outdir, 'cpython')
    copy_source_tree(srcdir, SRCDIR)

    # We use an out-of-tree build (instead of srcdir).
    builddir = os.path.join(outdir, 'python-build')
    os.makedirs(builddir, exist_ok=True)

    # Run configure.
    print(f'configuring python in {builddir}...')
    config_args = shlex.split(sysconfig.get_config_var('CONFIG_ARGS') or '')
    cmd = [os.path.join(srcdir, 'configure'), *config_args]
    ensure_opt(cmd, 'cache-file', os.path.join(outdir, 'python-config.cache'))
    prefix = os.path.join(outdir, 'python-installation')
    ensure_opt(cmd, 'prefix', prefix)
    _run_quiet(cmd, cwd=builddir)

    if not MAKE:
        raise UnsupportedError('make')

    cores = os.process_cpu_count()
    if cores and cores >= 3:
        # this test is most often run as part of the whole suite with a lot
        # of other tests running in parallel, from 1-2 vCPU systems up to
        # people's NNN core beasts. Don't attempt to use it all.
        jobs = cores * 2 // 3
        parallel = f'-j{jobs}'
    else:
        parallel = '-j2'

    # Build python.
    print(f'building python {parallel=} in {builddir}...')
    _run_quiet([MAKE, parallel], cwd=builddir)

    # Install the build.
    print(f'installing python into {prefix}...')
    _run_quiet([MAKE, 'install'], cwd=builddir)
    python = os.path.join(prefix, 'bin', 'python3')

    return outdir, scriptfile, python


def freeze(python, scriptfile, outdir):
    if not MAKE:
        raise UnsupportedError('make')

    print(f'freezing {scriptfile}...')
    os.makedirs(outdir, exist_ok=True)
    # Use -E to ignore PYTHONSAFEPATH
    _run_quiet([python, '-E', FREEZE, '-o', outdir, scriptfile], cwd=outdir)
    _run_quiet([MAKE], cwd=os.path.dirname(scriptfile))

    name = os.path.basename(scriptfile).rpartition('.')[0]
    executable = os.path.join(outdir, name)
    return executable


def run(executable):
    return _run_stdout([executable])
