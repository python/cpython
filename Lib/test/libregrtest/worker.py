import subprocess
import sys
import os
from _colorize import can_colorize  # type: ignore[import-not-found]
from typing import Any, NoReturn

from test.support import os_helper, Py_DEBUG

from .setup import setup_process, setup_test_dir
from .runtests import WorkerRunTests, JsonFile, JsonFileType
from .single import run_single_test
from .utils import (
    StrPath, StrJSON, TestFilter,
    get_temp_dir, get_work_dir, exit_timeout)


USE_PROCESS_GROUP = (hasattr(os, "setsid") and hasattr(os, "killpg"))
NEED_TTY = {
    'test_ioctl',
}


def create_worker_process(runtests: WorkerRunTests, output_fd: int,
                          tmp_dir: StrPath | None = None) -> subprocess.Popen[str]:
    worker_json = runtests.as_json()

    cmd = runtests.create_python_cmd()
    cmd.extend(['-m', 'test.libregrtest.worker', worker_json])

    env = dict(os.environ)
    if tmp_dir is not None:
        env['TMPDIR'] = tmp_dir
        env['TEMP'] = tmp_dir
        env['TMP'] = tmp_dir

    # The subcommand is run with a temporary output which means it is not a TTY
    # and won't auto-color. The test results are printed to stdout so if we can
    # color that have the subprocess use color.
    if can_colorize(file=sys.stdout):
        env['FORCE_COLOR'] = '1'

    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    #
    # Emscripten and WASI Python must start in the Python source code directory
    # to get 'python.js' or 'python.wasm' file. Then worker_process() changes
    # to a temporary directory created to run tests.
    work_dir = os_helper.SAVEDCWD

    kwargs: dict[str, Any] = dict(
        env=env,
        stdout=output_fd,
        # bpo-45410: Write stderr into stdout to keep messages order
        stderr=output_fd,
        text=True,
        close_fds=True,
        cwd=work_dir,
    )

    # Don't use setsid() in tests using TTY
    test_name = runtests.tests[0]
    if USE_PROCESS_GROUP and test_name not in NEED_TTY:
        kwargs['start_new_session'] = True

    # Include the test name in the TSAN log file name
    if 'TSAN_OPTIONS' in env:
        parts = env['TSAN_OPTIONS'].split(' ')
        for i, part in enumerate(parts):
            if part.startswith('log_path='):
                parts[i] = f'{part}.{test_name}'
                break
        env['TSAN_OPTIONS'] = ' '.join(parts)

    # Pass json_file to the worker process
    json_file = runtests.json_file
    json_file.configure_subprocess(kwargs)

    with json_file.inherit_subprocess():
        return subprocess.Popen(cmd, **kwargs)


def worker_process(worker_json: StrJSON) -> NoReturn:
    runtests = WorkerRunTests.from_json(worker_json)
    test_name = runtests.tests[0]
    match_tests: TestFilter = runtests.match_tests
    json_file: JsonFile = runtests.json_file

    setup_test_dir(runtests.test_dir)
    setup_process()

    if runtests.rerun:
        if match_tests:
            matching = "matching: " + ", ".join(pattern for pattern, result in match_tests if result)
            print(f"Re-running {test_name} in verbose mode ({matching})", flush=True)
        else:
            print(f"Re-running {test_name} in verbose mode", flush=True)

    result = run_single_test(test_name, runtests)
    if runtests.coverage:
        if "test.cov" in sys.modules:  # imported by -Xpresite=
            result.covered_lines = list(sys.modules["test.cov"].coverage)
        elif not Py_DEBUG:
            print(
                "Gathering coverage in worker processes requires --with-pydebug",
                flush=True,
            )
        else:
            raise LookupError(
                "`test.cov` not found in sys.modules but coverage wanted"
            )

    if json_file.file_type == JsonFileType.STDOUT:
        print()
        result.write_json_into(sys.stdout)
    else:
        with json_file.open('w', encoding='utf-8') as json_fp:
            result.write_json_into(json_fp)

    sys.exit(0)


def main() -> NoReturn:
    if len(sys.argv) != 2:
        print("usage: python -m test.libregrtest.worker JSON")
        sys.exit(1)
    worker_json = sys.argv[1]

    tmp_dir = get_temp_dir()
    work_dir = get_work_dir(tmp_dir, worker=True)

    with exit_timeout():
        with os_helper.temp_cwd(work_dir, quiet=True):
            worker_process(worker_json)


if __name__ == "__main__":
    main()
