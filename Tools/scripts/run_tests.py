"""Run Python's test suite in a fast, rigorous way.

The defaults are meant to be reasonably thorough, while skipping certain
tests that can be time-consuming or resource-intensive (e.g. largefile),
or distracting (e.g. audio and gui). These defaults can be overridden by
simply passing a -u option to this script.

"""

import os
import sys
import test.support
try:
    import threading
except ImportError:
    threading = None


def is_multiprocess_flag(arg):
    return arg.startswith('-j') or arg.startswith('--multiprocess')


def is_resource_use_flag(arg):
    return arg.startswith('-u') or arg.startswith('--use')


def main(regrtest_args):
    args = [sys.executable,
            '-u',                 # Unbuffered stdout and stderr
            '-W', 'default',      # Warnings set to 'default'
            '-bb',                # Warnings about bytes/bytearray
            '-E',                 # Ignore environment variables
            ]
    # Allow user-specified interpreter options to override our defaults.
    args.extend(test.support.args_from_interpreter_flags())

    # Workaround for issue #20361
    args.extend(['-W', 'error::BytesWarning'])

    args.extend(['-m', 'test',    # Run the test suite
                 '-r',            # Randomize test order
                 '-w',            # Re-run failed tests in verbose mode
                 ])
    if sys.platform == 'win32':
        args.append('-n')         # Silence alerts under Windows
    if threading and not any(is_multiprocess_flag(arg) for arg in regrtest_args):
        args.extend(['-j', '0'])  # Use all CPU cores
    if not any(is_resource_use_flag(arg) for arg in regrtest_args):
        args.extend(['-u', 'all,-largefile,-audio,-gui'])
    args.extend(regrtest_args)
    print(' '.join(args))
    if sys.platform == 'win32':
        from subprocess import call
        sys.exit(call(args))
    else:
        os.execv(sys.executable, args)


if __name__ == '__main__':
    main(sys.argv[1:])
