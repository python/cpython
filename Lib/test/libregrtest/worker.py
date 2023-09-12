import subprocess
import sys
import os
from typing import NoReturn

from test import support
from test.support import os_helper

from .setup import setup_process, setup_test_dir
from .runtests import RunTests, JsonFileType, JSON_FILE_USE_FILENAME
from .single import run_single_test
from .utils import (
    StrPath, StrJSON, FilterTuple, MS_WINDOWS,
    get_temp_dir, get_work_dir, exit_timeout)


USE_PROCESS_GROUP = (hasattr(os, "setsid") and hasattr(os, "killpg"))


def create_worker_process(runtests: RunTests,
                          output_fd: int, json_file: JsonFileType,
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

    # Emscripten and WASI Python must start in the Python source code directory
    # to get 'python.js' or 'python.wasm' file. Then worker_process() changes
    # to a temporary directory created to run tests.
    work_dir = os_helper.SAVEDCWD

    # Running the child from the same working directory as regrtest's original
    # invocation ensures that TEMPDIR for the child is the same when
    # sysconfig.is_python_build() is true. See issue 15300.
    kwargs = dict(
        env=env,
        stdout=output_fd,
        # bpo-45410: Write stderr into stdout to keep messages order
        stderr=output_fd,
        text=True,
        close_fds=True,
        cwd=work_dir,
    )
    if JSON_FILE_USE_FILENAME:
        # Nothing to do to pass the JSON filename to the worker process
        pass
    elif MS_WINDOWS:
        # Pass the JSON handle to the worker process
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.lpAttributeList = {"handle_list": [json_file]}
        kwargs['startupinfo'] = startupinfo
    else:
        # Pass the JSON file descriptor to the worker process
        kwargs['pass_fds'] = [json_file]
    if USE_PROCESS_GROUP:
        kwargs['start_new_session'] = True

    if MS_WINDOWS:
        os.set_handle_inheritable(json_file, True)
    try:
        return subprocess.Popen(cmd, **kwargs)
    finally:
        if MS_WINDOWS:
            os.set_handle_inheritable(json_file, False)


def worker_process(worker_json: StrJSON) -> NoReturn:
    runtests = RunTests.from_json(worker_json)
    test_name = runtests.tests[0]
    match_tests: FilterTuple | None = runtests.match_tests
    # On Unix, it's a file descriptor.
    # On Windows, it's a handle.
    # On Emscripten/WASI, it's a filename.
    json_file: JsonFileType = runtests.json_file

    if MS_WINDOWS:
        import msvcrt
        # Create a file descriptor from the handle
        json_file = msvcrt.open_osfhandle(json_file, os.O_WRONLY)


    setup_test_dir(runtests.test_dir)
    setup_process()

    if runtests.rerun:
        if match_tests:
            matching = "matching: " + ", ".join(match_tests)
            print(f"Re-running {test_name} in verbose mode ({matching})", flush=True)
        else:
            print(f"Re-running {test_name} in verbose mode", flush=True)

    result = run_single_test(test_name, runtests)

    with open(json_file, 'w', encoding='utf-8') as fp:
        result.write_json_into(fp)

    sys.exit(0)


def main():
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
