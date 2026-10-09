# Script to generate Modules/getbuildinfo.h

import argparse
import locale
import os
import re
import shlex
import subprocess
import sys
import sysconfig
import time

MS_WINDOWS = (sys.platform == 'win32')
SRC_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
SCRIPT_NAME = os.path.basename(__file__)
SCRIPT_FULLNAME = f'Tools/build/{SCRIPT_NAME}'

# Get PY_VERSION from Include/patchlevel.h
PY_VERSION_REGEX = re.compile(r'^#define PY_VERSION +"(.*)"$', re.MULTILINE)

def create_makefile_regex(name):
    return re.compile(fr'^{name}[ \t]*=[ \t]*(.*)$', re.MULTILINE)

# Get HOSTRUNNER and CC from Makefile
HOSTRUNNER_REGEX = create_makefile_regex('HOSTRUNNER')
CC_REGEX = create_makefile_regex('CC')

# Parse Clang version (truncated examples using '...'):
# 'Clang 21.0.0 (clang-2100.1.1.101)'
# 'clang version 18.0.4'
# 'Apple clang version 21.0.0 (clang-2100.1.1.101)'
# 'Clang 18.0.4 (https://android.googlesource.com/toolchain/llvm-project d80...62)'
CLANG_VERSION_REGEX = re.compile(
    r'(?:([A-Za-z_-][A-Za-z_ -]+) .*)?'  # Vendor name: 'Android' or 'Apple'
    r'(?:Clang|clang version) '          # 'Clang' or 'clang version'
    r'([0-9]+\.[0-9a-z.+]+)'             # version
)

# Parse 'Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36260 for ARM64'
CL_VERSION_REGEX = re.compile(r'Compiler Version ([0-9]+).([0-9]+).[0-9]+ for (.*)')
CL_VERSION_SUFFIX = {
    'x86': "32 bit (Intel)",
    'x64': "64 bit (AMD64)",
    'ARM': "32 bit (ARM)",
    'ARM64': "64 bit (ARM64)",
}


def log(msg):
    print(f"{SCRIPT_NAME}: {msg}")


def exit_error(msg):
    log(msg)
    sys.exit(1)


def get_py_version():
    filename = os.path.join(SRC_DIR, 'Include', 'patchlevel.h')
    with open(filename, encoding='utf8') as fp:
        code = fp.read()
    match = PY_VERSION_REGEX.search(code)
    if not match:
        exit_error(f"ERROR: Unable to locate PY_VERSION in {filename}")
    return match.group(1)


def parse_file(variable_name, filename, regex):
    with open(filename, encoding='utf8') as fp:
        code = fp.read()
    match = regex.search(code)
    if not match:
        exit_error(f"ERROR: Unable to locate {variable_name} in {filename}")
    return match.group(1)


def get_hostrunner():
    # Look in the current working directory
    makefile = 'Makefile'
    HOSTRUNNER = parse_file('HOSTRUNNER', makefile, HOSTRUNNER_REGEX)
    return HOSTRUNNER.rstrip()


def get_makefile_cc():
    # Look in the current working directory
    makefile = 'Makefile'
    CC = parse_file('CC', makefile, CC_REGEX)
    return CC.rstrip()


def get_gil_disabled():
    # Look in the current working directory
    pyconfig_h = 'pyconfig.h'
    with open(pyconfig_h, encoding="utf-8") as fp:
        config_vars = sysconfig.parse_config_h(fp)
    return bool(config_vars['Py_GIL_DISABLED'])


def get_date_time():
    if os.environ.get('SOURCE_DATE_EPOCH'):
        ts = int(os.environ['SOURCE_DATE_EPOCH'])
        time_tuple = time.gmtime(ts)
    else:
        ts = time.time()
        time_tuple = time.localtime(ts)

    day = time.strftime("%d", time_tuple)
    if day.startswith("0"):
        day = " " + day[1:]
    build_date = time.strftime(f"%b {day} %Y", time_tuple)
    build_time = time.strftime("%H:%M:%S", time_tuple)
    return (build_date, build_time)


def get_build_info(git_tag, git_branch, git_version):
    build_date, build_time = get_date_time()

    if git_tag and git_tag != "undefined":
        git_id = git_tag
    else:
        git_id = git_branch
    if not git_id:
        git_id = "main"

    sep = ":" if git_version else ""
    build_info = f"{git_id}{sep}{git_version}, {build_date}, {build_time}"
    return (build_info, git_id)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--output', type=str)
    parser.add_argument('--platform', type=str, required=True)
    parser.add_argument('--git-program', type=str)
    parser.add_argument('--git-version', type=str)
    parser.add_argument('--git-tag', type=str)
    parser.add_argument('--git-branch', type=str)
    parser.add_argument('--getcompiler-program', type=str)
    parser.add_argument('--c-compiler', type=str)
    parser.add_argument('--compiler', type=str)
    parser.add_argument('--free-threading', type=int)
    return parser.parse_args()


def run_command(cmd, *, check=True, stderr=False):
    cmd_str = shlex.join(cmd)
    log(f"+ {cmd_str}")
    kwargs = {}
    if stderr:
        kwargs['stderr'] = subprocess.STDOUT
    try:
        proc = subprocess.run(cmd,
                              stdout=subprocess.PIPE,
                              text=True,
                              **kwargs)
    except OSError as exc:
        error = f'with: {exc!r}'
    else:
        exitcode = proc.returncode
        if exitcode:
            error = f'with exit code {exitcode}'
        else:
            error = None

    if error:
        msg = f"Command {cmd_str} failed {error}"
        if check:
            exit_error(msg)
        else:
            log(msg)
        return None

    return proc.stdout.rstrip()


def get_c_compiler_version_windows(c_compiler):
    cmd = [c_compiler]
    output = run_command(cmd, check=False, stderr=True)
    if not output:
        return None

    # Get the first line
    line = output.splitlines()[0]
    match = CL_VERSION_REGEX.search(line)
    if not match:
        exit_error(f"Unable to parse cl.exe version: {output!r}")

    major = match.group(1)
    minor = match.group(2)
    platform = match.group(3)
    suffix = CL_VERSION_SUFFIX.get(platform)
    if not suffix:
        exit_error(f"Unknown cl.exe platform: {platform!r}")

    return f"MSC v.{major}{minor} {suffix}"


def get_c_compiler_version_unix(c_compiler=None):
    CC = get_makefile_cc()
    if not CC:
        exit_error(f"ERROR: Unable to locate CC in Makefile")

    cmd = shlex.split(CC)
    cmd = [*cmd, '--version']

    output = run_command(cmd, check=False)
    if output:
        # Get the first line
        return output.splitlines()[0]

    return None


def _get_compiler(getcompiler=None, c_compiler=None):
    # Run _getcompiler program
    if not getcompiler:
        getcompiler = os.path.join('Programs', '_getcompiler')
    if not MS_WINDOWS:
        HOSTRUNNER = get_hostrunner()
    else:
        HOSTRUNNER = None
    if HOSTRUNNER:
        # Cross-compilation
        runner = shlex.split(HOSTRUNNER)[0]
        compiler = run_command([runner, getcompiler], check=False)
    else:
        compiler = run_command([getcompiler], check=False)
    if compiler:
        return compiler

    # Running _getcompiler failed, get the C compiler version
    if MS_WINDOWS:
        if c_compiler:
            return get_c_compiler_version_windows(c_compiler)
        return None
    else:
        return get_c_compiler_version_unix()


def compact_compiler(compiler):
    if '64 bit (AMD64)' in compiler:
        # On Windows, the '64 bit (AMD64)' pattern is expected by multiple
        # tools such as sysconfig.get_platform() and ctypes.util.find_msvcrt().
        # Example: 'MSC v.1951 64 bit (AMD64)'.
        return compiler
    if ' with MSC v' in compiler:
        # Do not truncate "Clang with MSC" version:
        # ''Clang 22.1.3 64 bit (AMD64) with MSC v.1951 CRT'
        return compiler

    # Truncate long Clang version (ex: strip long URL)
    match = CLANG_VERSION_REGEX.search(compiler)
    if match:
        vendor = match.group(1)
        version = match.group(2)
        if vendor:
            compiler = f'{vendor} Clang {version}'
        else:
            compiler = f'Clang {version}'

    return compiler


def get_compiler(compiler, getcompiler=None, c_compiler=None):
    if not compiler:
        compiler = _get_compiler(getcompiler, c_compiler)
    if not compiler:
        # Default compiler name when everything else failed
        # (see Programs/_getcompiler.c)
        compiler = 'C'

    old_compiler = compiler
    compiler = compact_compiler(old_compiler)
    if compiler != old_compiler:
        log(f"Truncate compiler string {old_compiler!r} to {compiler!r}")

    return f'[{compiler}]'


def main():
    # Force the C locale to format date as English
    locale.setlocale(locale.LC_ALL, 'C')

    args = parse_args()
    output_filename = args.output
    platform = args.platform
    git_program = args.git_program
    git_version = args.git_version
    git_tag = args.git_tag
    git_branch = args.git_branch
    free_threading = args.free_threading

    if git_program:
        git_dir = os.path.join(SRC_DIR, '.git')
        git_program = [git_program, '--git-dir', git_dir]
    if git_version is None:
        cmd = [*git_program, 'rev-parse', '--short', 'HEAD']
        git_version = run_command(cmd, check=False) or ''
    if git_branch is None:
        cmd = [*git_program, 'describe', '--all', '--always', '--dirty']
        git_branch = run_command(cmd, check=False) or ''
    if git_tag is None:
        cmd = [*git_program, 'name-rev', '--name-only', 'HEAD']
        git_tag = run_command(cmd, check=False) or ''

    if not output_filename:
        # Write to the current directory
        output_filename = os.path.join('Modules', 'getbuildinfo.h')
    if not platform:
        platform = "unknown"

    compiler = get_compiler(args.compiler, args.getcompiler_program, args.c_compiler)

    build_info, git_id = get_build_info(git_tag, git_branch, git_version)

    # Get PY_VERSION macro from Include/patchlevel.h
    PY_VERSION = get_py_version()

    if free_threading is None:
        # Get Py_GIL_DISABLED macro from pyconfig.h
        free_threading = get_gil_disabled()

    if free_threading:
        version = f"{PY_VERSION} free-threading build ({build_info}) {compiler}"
    else:
        version = f"{PY_VERSION} ({build_info}) {compiler}"

    new_filename = output_filename + ".new"
    with open(new_filename, "w", encoding="utf8") as fp:
        def write_macro(name, value):
            # Escape quote
            value = value.replace('"', '\\"')
            if '\0' in value:
                raise ValueError(f'embedded null character: {value!r}')
            print(f'#define {name} "{value}"', file=fp)

        print(f'// Header file auto-generated by {SCRIPT_FULLNAME}', file=fp)
        print(file=fp)
        write_macro('PLATFORM', platform)
        write_macro('COMPILER', compiler)
        write_macro('GIT_VERSION', git_version)
        write_macro('GIT_IDENTIFIER', git_id)
        write_macro('BUILD_INFO', build_info)
        write_macro('VERSION', version)

    os.replace(new_filename, output_filename)
    log(f"{output_filename} updated")


if __name__ == "__main__":
    main()
