"""Run Python's test suite in a fast, rigorous way.

The defaults are meant to be reasonably thorough, while skipping certain
tests that can be time-consuming or resource-intensive (e.g. largefile),
or distracting (e.g. audio and gui). These defaults can be overridden by
simply passing a -u option to this script.

"""

import os
import shlex
import sys
import sysconfig
import test.support


def is_multiprocess_flag(arg):
    return arg.startswith('-j') or arg.startswith('--multiprocess')


def is_resource_use_flag(arg):
    return arg.startswith('-u') or arg.startswith('--use')

def is_python_flag(arg):
    return arg.startswith('-p') or arg.startswith('--python')


def main(regrtest_args):
    args = [sys.executable,
            '-u',                 # Unbuffered stdout and stderr
            '-W', 'default',      # Warnings set to 'default'
            '-bb',                # Warnings about bytes/bytearray
            ]

    cross_compile = '_PYTHON_HOST_PLATFORM' in os.environ
    if (hostrunner := os.environ.get("_PYTHON_HOSTRUNNER")) is None:
        hostrunner = sysconfig.get_config_var("HOSTRUNNER")
    if cross_compile:
        # emulate -E, but keep PYTHONPATH + cross compile env vars, so
        # test executable can load correct sysconfigdata file.
        keep = {
            '_PYTHON_PROJECT_BASE',
            '_PYTHON_HOST_PLATFORM',
            '_PYTHON_SYSCONFIGDATA_NAME',
            'PYTHONPATH'
        }
        environ = {
            name: value for name, value in os.environ.items()
            if not name.startswith(('PYTHON', '_PYTHON')) or name in keep
        }
    else:
        environ = os.environ.copy()
        args.append("-E")

    # Allow user-specified interpreter options to override our defaults.
    args.extend(test.support.args_from_interpreter_flags())

    args.extend(['-m', 'test',    # Run the test suite
                 '-r',            # Randomize test order
                 '-w',            # Re-run failed tests in verbose mode
                 ])
    if sys.platform == 'win32':
        args.append('-n')         # Silence alerts under Windows
    if not any(is_multiprocess_flag(arg) for arg in regrtest_args):
        if cross_compile and hostrunner:
            # For now use only two cores for cross-compiled builds;
            # hostrunner can be expensive.
            args.extend(['-j', '2'])
        else:
            args.extend(['-j', '0'])  # Use all CPU cores
    if not any(is_resource_use_flag(arg) for arg in regrtest_args):
        args.extend(['-u', 'all,-largefile,-audio,-gui'])

    if cross_compile and hostrunner:
        # If HOSTRUNNER is set and -p/--python option is not given, then
        # use hostrunner to execute python binary for tests.
        if not any(is_python_flag(arg) for arg in regrtest_args):
            buildpython = sysconfig.get_config_var("BUILDPYTHON")
            args.extend(["--python", f"{hostrunner} {buildpython}"])

    args.extend(regrtest_args)

    print(shlex.join(args))
    if sys.platform == 'win32':
        from subprocess import call
        sys.exit(call(args))
    else:
        os.execve(sys.executable, args, environ)


if __name__ == '__main__':
    main(sys.argv[1:])
