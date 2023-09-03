import os
import subprocess
import sys
from typing import TextIO

from test import support
from test.support import os_helper

from .setup import setup_tests, setup_test_dir
from .single import run_single_test, RunTests, TestResult


USE_PROCESS_GROUP = (hasattr(os, "setsid") and hasattr(os, "killpg"))


def create_worker_process(runtests: RunTests,
                          output_file: TextIO,
                          tmp_dir: str | None = None) -> subprocess.Popen:
    worker_json = runtests.as_json()

    python = runtests.python_executable
    if python is not None:
        executable = python
    else:
        executable = [sys.executable]
    cmd = [*executable, *support.args_from_interpreter_flags(),
           '-u',    # Unbuffered stdout and stderr
           '-m', 'test.regrtest',
           '--worker-json', worker_json]

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
        cwd=os_helper.SAVEDCWD,
    )
    if USE_PROCESS_GROUP:
        kw['start_new_session'] = True
    return subprocess.Popen(cmd, **kw)


def worker_process(worker_json: str) -> None:
    runtests = RunTests.from_json(worker_json)
    test_name = runtests.tests[0]

    setup_test_dir(runtests.test_dir)
    setup_tests(runtests)

    match_tests = runtests.match_tests
    if runtests.rerun:
        if match_tests:
            matching = "matching: " + ", ".join(match_tests)
            print(f"Re-running {test_name} in verbose mode ({matching})", flush=True)
        else:
            print(f"Re-running {test_name} in verbose mode", flush=True)

    result: TestResult = run_single_test(test_name, runtests)
    print()   # Force a newline (just in case)

    # Serialize TestResult as JSON into stdout
    result.as_json_into(sys.stdout)
    sys.stdout.flush()
    sys.exit(0)
