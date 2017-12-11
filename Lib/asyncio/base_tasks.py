import linecache
import traceback

from . import base_futures
from . import coroutines
from .events import get_running_loop


def _task_repr_info(task):
    info = base_futures._future_repr_info(task)

    if task._must_cancel:
        # replace status
        info[0] = 'cancelling'

    coro = coroutines._format_coroutine(task._coro)
    info.insert(1, f'coro=<{coro}>')

    if task._fut_waiter is not None:
        info.insert(2, f'wait_for={task._fut_waiter!r}')
    return info


def _task_get_stack(task, limit):
    frames = []
    try:
        # 'async def' coroutines
        f = task._coro.cr_frame
    except AttributeError:
        f = task._coro.gi_frame
    if f is not None:
        while f is not None:
            if limit is not None:
                if limit <= 0:
                    break
                limit -= 1
            frames.append(f)
            f = f.f_back
        frames.reverse()
    elif task._exception is not None:
        tb = task._exception.__traceback__
        while tb is not None:
            if limit is not None:
                if limit <= 0:
                    break
                limit -= 1
            frames.append(tb.tb_frame)
            tb = tb.tb_next
    return frames


def _task_print_stack(task, limit, file):
    extracted_list = []
    checked = set()
    for f in task.get_stack(limit=limit):
        lineno = f.f_lineno
        co = f.f_code
        filename = co.co_filename
        name = co.co_name
        if filename not in checked:
            checked.add(filename)
            linecache.checkcache(filename)
        line = linecache.getline(filename, lineno, f.f_globals)
        extracted_list.append((filename, lineno, name, line))

    exc = task._exception
    if not extracted_list:
        print(f'No stack for {task!r}', file=file)
    elif exc is not None:
        print(f'Traceback for {task!r} (most recent call last):', file=file)
    else:
        print(f'Stack for {task!r} (most recent call last):', file=file)

    traceback.print_list(extracted_list, file=file)
    if exc is not None:
        for line in traceback.format_exception_only(exc.__class__, exc):
            print(line, file=file, end='')


# dict of {EventLoop: Set[tasks]} containing all tasks alive.
_all_tasks = {}

# Dictionary containing tasks that are currently active in
# all running event loops.  {EventLoop: Task}
_current_tasks = {}


def current_task(loop=None):
    if loop is None:
        loop = get_running_loop()
    task = _current_tasks.get(loop)
    if task is None:
        raise RuntimeError("no current task")
    return task


def all_tasks(loop=None):
    if loop is None:
        loop = get_running_loop()
    task = _current_tasks.get(loop)
    if task is None:
        raise RuntimeError("no current task")
    return task


def _register_task(loop, task):
    tasks = _all_tasks.get(loop)
    if tasks is None:
        tasks = _all_tasks[loop] = set()
    tasks.add(task)


def _enter_task(loop, task):
    current_task = _current_tasks.get(loop)
    if current_task is not None:
        raise RuntimeError(f"Entering into task {task!r} "
                           f"when other task {current_task!r} is executed.")
    _current_tasks[loop] = task


def _leave_task(loop, task):
    current_task = _current_tasks.get(loop)
    if current_task is not task:
        raise RuntimeError(f"Leaving task {task!r} "
                           f"is not current {current_task!r}.")
    del _current_tasks[loop]


def _unregister_task(loop, task):
    tasks = _all_tasks.get(loop)
    if tasks is None:
        # loop is not registered
        # raise RuntimeWarning?
        return
    tasks.remove(task)
    if not tasks:
        del _all_tasks[loop]
