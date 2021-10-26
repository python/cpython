import os
import os.path
import re
import shlex
import shutil
import subprocess
import sys


TESTS_DIR = os.path.dirname(__file__)
TOOL_ROOT = os.path.dirname(TESTS_DIR)
SRCDIR = os.path.dirname(os.path.dirname(TOOL_ROOT))

CONFIGURE = os.path.join(SRCDIR, 'configure')
MAKE = shutil.which('make')
GIT = shutil.which('git')
FREEZE = os.path.join(TOOL_ROOT, 'freeze.py')
OUTDIR = os.path.join(TESTS_DIR, 'outdir')


class UnsupportedError(Exception):
    """The operation isn't supported."""


def _run_cmd(cmd, cwd=None, verbose=True, showcmd=True):
    if showcmd:
        print(f'# {" ".join(shlex.quote(a) for a in cmd)}')
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


def find_opt(args, name):
    opt = f'--{name}'
    optstart = f'{opt}='
    for i, arg in enumerate(args):
        if arg == opt or arg.startswith(optstart):
            return i
    return -1


def git_copy_repo(newroot, remote=None, *, verbose=True):
    if not GIT:
        raise UnsupportedError('git')
    if not remote:
        remote = SRCDIR
    if os.path.exists(newroot):
        print(f'updating copied repo {newroot}...')
        _run_cmd([GIT, '-C', newroot, 'reset'], verbose=verbose)
        _run_cmd([GIT, '-C', newroot, 'checkout', '.'], verbose=verbose)
        _run_cmd([GIT, '-C', newroot, 'pull', '-f', remote], verbose=verbose)
    else:
        print(f'copying repo into {newroot}...')
        _run_cmd([GIT, 'clone', remote, newroot], verbose=verbose)
    if os.path.exists(remote):
        # Copy over any uncommited files.
        reporoot = remote
        text = _run_cmd([GIT, '-C', reporoot, 'status', '-s'], verbose=verbose)
        for line in text.splitlines():
            _, _, relfile = line.strip().partition(' ')
            relfile = relfile.strip()
            srcfile = os.path.join(reporoot, relfile)
            dstfile = os.path.join(newroot, relfile)
            os.makedirs(os.path.dirname(dstfile), exist_ok=True)
            shutil.copy2(srcfile, dstfile)


##################################
# build queries

def get_makefile_var(builddir, name, *, fail=True):
    regex = re.compile(rf'^{name} *=\s*(.*?)\s*$')
    filename = os.path.join(builddir, 'Makefile')
    try:
        infile = open(filename)
    except FileNotFoundError:
        if fail:
            raise  # re-raise
        return None
    with infile:
        for line in infile:
            m = regex.match(line)
            if m:
                value, = m.groups()
                return value or ''
    if fail:
        raise KeyError(f'{name!r} not in Makefile', name=name)
    return None


def get_config_var(build, name, *, fail=True):
    if os.path.isfile(build):
        python = build
    else:
        builddir = build
        python = os.path.join(builddir, 'python')
        if not os.path.isfile(python):
            return get_makefile_var(builddir, 'CONFIG_ARGS', fail=fail)

    text = _run_cmd(
        [python, '-c',
         'import sysconfig', 'print(sysconfig.get_config_var("CONFIG_ARGS"))'],
        showcmd=False,
    )
    return text


def get_configure_args(build, *, fail=True):
    text = get_config_var(build, 'CONFIG_ARGS', fail=fail)
    if not text:
        return None
    return shlex.split(text)


def get_prefix(build=None):
    if build and os.path.isfile(build):
        return _run_cmd(
            [build, '-c' 'import sys; print(sys.prefix)'],
            cwd=os.path.dirname(build),
            showcmd=False,
        )
    else:
        return get_makefile_var(build or '.', 'prefix', fail=False)


##################################
# building Python

def configure_python(builddir=None, prefix=None, cachefile=None, args=None, *,
                     srcdir=None,
                     inherit=False,
                     verbose=True,
                     ):
    if not builddir:
        builddir = srcdir or SRCDIR
    if not srcdir:
        configure = os.path.join(builddir, 'configure')
        if not os.path.isfile(configure):
            srcdir = SRCDIR
            configure = CONFIGURE
    else:
        configure = os.path.join(srcdir, 'configure')

    cmd = [configure]
    if inherit:
        oldargs = get_configure_args(builddir)
        if oldargs:
            cmd.extend(oldargs)
    if cachefile:
        if args and find_opt(args, 'cache-file') >= 0:
            raise ValueError('unexpected --cache-file')
        cmd.extend(['--cache-file', os.path.abspath(cachefile)])
    if prefix:
        if args and find_opt(args, 'prefix') >= 0:
            raise ValueError('unexpected --prefix')
        cmd.extend(['--prefix', os.path.abspath(prefix)])
    if args:
        cmd.extend(args)

    print(f'configuring python in {builddir}...')
    os.makedirs(builddir, exist_ok=True)
    _run_cmd(cmd, builddir, verbose)
    return builddir


def build_python(builddir, *, verbose=True):
    if not MAKE:
        raise UnsupportedError('make')

    if not builddir:
        builddir = '.'

    srcdir = get_config_var(builddir, 'srcdir', fail=False) or SRCDIR
    if os.path.abspath(builddir) != srcdir:
        _run_cmd([MAKE, '-C', srcdir, 'clean'], verbose=False)

    _run_cmd([MAKE, '-C', builddir, '-j'], verbose)

    return os.path.join(builddir, 'python')


def install_python(builddir, *, verbose=True):
    if not MAKE:
        raise UnsupportedError('make')

    if not builddir:
        builddir = '.'
    prefix = get_prefix(builddir)

    print(f'installing python into {prefix}...')
    _run_cmd([MAKE, '-C', builddir, '-j', 'install'], verbose)

    if not prefix:
        return None
    return os.path.join(prefix, 'bin', 'python3')


def ensure_python_installed(outdir, srcdir=None, *,
                            outoftree=True,
                            verbose=True,
                            ):
    cachefile = os.path.join(outdir, 'python-config.cache')
    prefix = os.path.join(outdir, 'python-installation')
    if outoftree:
        builddir = os.path.join(outdir, 'python-build')
    else:
        builddir = srcdir or SRCDIR
    configure_python(builddir, prefix, cachefile,
                     srcdir=srcdir, inherit=True, verbose=verbose)
    build_python(builddir, verbose=verbose)
    return install_python(builddir, verbose=verbose)


##################################
# freezing

def prepare(script=None, outdir=None, *,
            outoftree=True,
            copy=False,
            verbose=True,
            ):
    if not outdir:
        outdir = OUTDIR
    os.makedirs(outdir, exist_ok=True)
    if script:
        if script.splitlines()[0] == script and os.path.isfile(script):
            scriptfile = script
        else:
            scriptfile = os.path.join(outdir, 'app.py')
            with open(scriptfile, 'w') as outfile:
                outfile.write(script)
    else:
        scriptfile = None
    if copy:
        srcdir = os.path.join(outdir, 'cpython')
        git_copy_repo(srcdir, SRCDIR, verbose=verbose)
    else:
        srcdir = SRCDIR
    python = ensure_python_installed(outdir, srcdir,
                                     outoftree=outoftree, verbose=verbose)
    return outdir, scriptfile, python


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
