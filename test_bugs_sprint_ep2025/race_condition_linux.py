# N.B.: We apply the monkeypatch before subprocess is imported because subprocess will
# hold strong references to os.waitpid.
from __future__ import annotations

import os
import sys
import textwrap
import traceback
from functools import wraps

orig_waitpid = os.waitpid
orig_kill = os.kill
freed_pids = set[int]()


@wraps(orig_waitpid)
def waitpid(pid: int, options: int, /) -> tuple[int, int]:
    print(f"--DBG: start  waitpid({pid!r}, {options!r}) @")
    print(
        textwrap.indent(
            "".join(traceback.extract_stack(sys._getframe(1), limit=2).format()),
            prefix=" " * (-2 + len("--DBG: ")),
        ),
        end="",
    )
    try:
        res = orig_waitpid(pid, options)
    except BaseException as exc:
        print(f"--DBG: finish waitpid({pid!r}, {options!r}) -> {exc!r}")
        raise
    else:
        res_pid, status = res
        if res_pid != 0:
            freed_pids.add(res_pid)
        print(f"--DBG: finish waitpid({pid!r}, {options!r}) = {res!r}")
        return res


@wraps(orig_kill)
def kill(pid: int, sig: int, /) -> None:
    print(f"--DBG: kill({pid}, {sig})")
    if pid in freed_pids:
        raise ValueError(
            "caller is trying to signal an already-freed PID! did a site call waitpid without telling the sites with references to that PID about it?"
        )
    return orig_kill(pid, sig)


os.waitpid = waitpid
os.kill = kill

assert "subprocess" not in sys.modules

import asyncio
import subprocess
from signal import Signals as Signal
from typing import Literal
from typing import assert_never


async def main() -> None:
    _watcher_case: Literal["_PidfdChildWatcher", "_ThreadedChildWatcher"]
    if sys.version_info >= (3, 14):
        _watcher = asyncio.get_running_loop()._watcher  # type: ignore[attr-defined]
        if isinstance(_watcher, asyncio.unix_events._PidfdChildWatcher):  # type: ignore[attr-defined]
            _watcher_case = "_PidfdChildWatcher"
        elif isinstance(_watcher, asyncio.unix_events._ThreadedChildWatcher):  # type: ignore[attr-defined]
            _watcher_case = "_ThreadedChildWatcher"
        else:
            raise NotImplementedError()
    else:
        _watcher = asyncio.get_child_watcher()
        if isinstance(_watcher, asyncio.PidfdChildWatcher):
            _watcher_case = "_PidfdChildWatcher"
        elif isinstance(_watcher, asyncio.ThreadedChildWatcher):
            _watcher_case = "_ThreadedChildWatcher"
        else:
            raise NotImplementedError()
    print(f"{_watcher_case = !r}")

    process = await asyncio.create_subprocess_exec(
        "python",
        "-c",
        "import time; time.sleep(1)",
        stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    print(f"{process.pid = !r}")

    process.send_signal(Signal.SIGKILL)

    # This snippet is contrived, in order to make this snippet hit the race condition
    # consistently for reproduction & testing purposes.
    if _watcher_case == "_PidfdChildWatcher":
        os.waitid(os.P_PID, process.pid, os.WEXITED | os.WNOWAIT)
        # Or alternatively, time.sleep(0.1).

        # On the next loop cycle asyncio will select on the pidfd and append the reader
        # callback:
        await asyncio.sleep(0)
        # On the next loop cycle the reader callback will run, calling (a) waitpid
        # (freeing the PID) and (b) call_soon_threadsafe(transport._process_exited):
        await asyncio.sleep(0)

        # The _PidfdChildWatcher has now freed the PID but hasn't yet told the
        # asyncio.subprocess.Process or the subprocess.Popen about this
        # (call_soon_threadsafe).
    elif _watcher_case == "_ThreadedChildWatcher":
        if (thread := _watcher._threads.get(process.pid)) is not None:  # type: ignore[attr-defined]
            thread.join()
        # Or alternatively, time.sleep(0.1).

        # The _ThreadedChildWatcher has now freed the PID but hasn't yet told the
        # asyncio.subprocess.Process or the subprocess.Popen about this
        # (call_soon_threadsafe).
    else:
        assert_never(_watcher_case)

    # The watcher has now freed the PID but hasn't yet told the
    # asyncio.subprocess.Process or the subprocess.Popen that the PID they hold a
    # reference to has been freed externally!
    #
    # I think these two things need to happen atomically.

    try:
        process.send_signal(Signal.SIGKILL)
    except ProcessLookupError:
        pass


# Pretend we don't have pidfd support
# if sys.version_info >= (3, 14):
#     asyncio.unix_events.can_use_pidfd = lambda: False  # type: ignore[attr-defined]
# else:
#     asyncio.set_child_watcher(asyncio.ThreadedChildWatcher())

asyncio.run(main())