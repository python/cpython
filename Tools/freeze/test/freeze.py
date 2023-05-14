import os
import os.path
import re
import shlex
import shutil
import subprocess


TESTS_DIR = os.path.dirname(__file__)
TOOL_ROOT = os.path.dirname(TESTS_DIR)
SRCDIR = os.path.dirname(os.path.dirname(TOOL_ROOT))

MAKE = shutil.which('make')
FREEZE = os.path.join(TOOL_ROOT, 'freeze.py')
OUTDIR = os.path.join(TESTS_DIR, 'outdir')


class UnsupportedError(Exception):
    """The operation isn't supported."""


def _run_quiet(cmd, cwd=None):
    #print(f'# {" ".join(shlex.quote(a) for a in cmd)}')
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


def _run_stdout(cmd, cwd=None):
    proc = _run_quiet(cmd, cwd)
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
    print(f'copying the source tree into {newroot}...')
    if os.path.exists(newroot):
        if newroot == SRCDIR:
            raise Exception('this probably isn\'t what you wanted')
        shutil.rmtree(newroot)

    def ignore_non_src(src, names):
        """Turns what could be a 1000M copy into a 100M copy."""
        # Don't copy the ~600M+ of needless git repo metadata.
        # source only, ignore cached .pyc files.
        subdirs_to_skip = {'.git', '__pycache__'}
        if os.path.basename(src) == 'Doc':
            # Another potential ~250M+ of non test related data.
            subdirs_to_skip.add('build')
            subdirs_to_skip.add('venv')
        return subdirs_to_skip

    shutil.copytree(oldroot, newroot, ignore=ignore_non_src)
    if os.path.exists(os.path.join(newroot, 'Makefile')):
        _run_quiet([MAKE, 'clean'], newroot)


def get_makefile_var(builddir, name):
    regex = re.compile(rf'^{name} *=\s*(.*?)\s*$')
    filename = os.path.join(builddir, 'Makefile')
    try:
        infile = open(filename, encoding='utf-8')
    except FileNotFoundError:
        return None
    with infile:
        for line in infile:
            m = regex.match(line)
            if m:
                value, = m.groups()
                return value or ''
    return None


def get_config_var(builddir, name):
    python = os.path.join(builddir, 'python')
    if os.path.isfile(python):
        cmd = [python, '-c',
               f'import sysconfig; print(sysconfig.get_config_var("{name}"))']
        try:
            return _run_stdout(cmd)
        except subprocess.CalledProcessError:
            pass
    return get_makefile_var(builddir, name)


##################################
# freezing

def prepare(script=None, outdir=None):
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
    cmd = [
        os.path.join(srcdir, 'configure'),
        *shlex.split(get_config_var(srcdir, 'CONFIG_ARGS') or ''),
    ]
    ensure_opt(cmd, 'cache-file', os.path.join(outdir, 'python-config.cache'))
    prefix = os.path.join(outdir, 'python-installation')
    ensure_opt(cmd, 'prefix', prefix)
    _run_quiet(cmd, builddir)

    if not MAKE:
        raise UnsupportedError('make')

    cores = os.cpu_count()
    if cores and cores >= 3:
        # this test is most often run as part of the whole suite with a lot
        # of other tests running in parallel, from 1-2 vCPU systems up to
        # people's NNN core beasts. Don't attempt to use it all.
        parallel = f'-j{cores*2//3}'
    else:
        parallel = '-j2'

    # Build python.
    print(f'building python {parallel=} in {builddir}...')
    if os.path.exists(os.path.join(srcdir, 'Makefile')):
        # Out-of-tree builds require a clean srcdir.
        _run_quiet([MAKE, '-C', srcdir, 'clean'])
    _run_quiet([MAKE, '-C', builddir, parallel])

    # Install the build.
    print(f'installing python into {prefix}...')
    _run_quiet([MAKE, '-C', builddir, 'install'])
    python = os.path.join(prefix, 'bin', 'python3')

    return outdir, scriptfile, python


def freeze(python, scriptfile, outdir):
    if not MAKE:
        raise UnsupportedError('make')

    print(f'freezing {scriptfile}...')
    os.makedirs(outdir, exist_ok=True)
    # Use -E to ignore PYTHONSAFEPATH
    _run_quiet([python, '-E', FREEZE, '-o', outdir, scriptfile], outdir)
    _run_quiet([MAKE, '-C', os.path.dirname(scriptfile)])

    name = os.path.basename(scriptfile).rpartition('.')[0]
    executable = os.path.join(outdir, name)
    return executable


def run(executable):
    return _run_stdout([executable])
