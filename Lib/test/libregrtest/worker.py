import subprocess
import sys
import os
from typing import TextIO, NoReturn

from test import support
from test.support import os_helper

from .setup import setup_process, setup_test_dir
from .runtests import RunTests
from .single import run_single_test
from .utils import (
    StrPath, StrJSON, FilterTuple,
    get_work_dir, exit_timeout)


USE_PROCESS_GROUP = (hasattr(os, "setsid") and hasattr(os, "killpg"))


def create_worker_process(runtests: RunTests,
                          output_file: TextIO,
                          tmp_dir: StrPath | None = None) -> subprocess.Popen:
    python_cmd = runtests.python_cmd
    worker_json = runtests.as_json()

    if python_cmd is not None:
        executable = python_cmd
    else:
        executable = [sys.executable]
    cmd = [*executable, *support.args_from_interpreter_flags(),
           '-u',    # Unbuffered stdout and stderr
           '-m', 'test.libregrtest.worker',
           worker_json]

    env = dict(os.environ)
    if tmp_dir is not None:
        env['TMPDIR'] = tmp_dir
        env['TEMP'] = tmp_dir
        env['TMP'] = tmp_dir

    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    kw = dict(
        env=env,
        stdout=output_file,
        # bpo-45410: Write stderr into stdout to keep messages order
        stderr=output_file,
        text=True,
        close_fds=(os.name != 'nt'),
    )
    if USE_PROCESS_GROUP:
        kw['start_new_session'] = True
    return subprocess.Popen(cmd, **kw)


def worker_process(worker_json: StrJSON) -> NoReturn:
    runtests = RunTests.from_json(worker_json)
    test_name = runtests.tests[0]
    match_tests: FilterTuple | None = runtests.match_tests

    setup_test_dir(runtests.test_dir)
    setup_process()

    if runtests.rerun:
        if match_tests:
            matching = "matching: " + ", ".join(match_tests)
            print(f"Re-running {test_name} in verbose mode ({matching})", flush=True)
        else:
            print(f"Re-running {test_name} in verbose mode", flush=True)

    result = run_single_test(test_name, runtests)
    print()   # Force a newline (just in case)

    # Serialize TestResult as dict in JSON
    result.write_json(sys.stdout)
    sys.stdout.flush()
    sys.exit(0)


def main():
    if len(sys.argv) != 2:
        print("usage: python -m test.libregrtest.worker JSON")
        sys.exit(1)
    worker_json = sys.argv[1]

    work_dir = get_work_dir(worker=True)

    with exit_timeout():
        with os_helper.temp_cwd(work_dir, quiet=True):
            worker_process(worker_json)


if __name__ == "__main__":
    main()
