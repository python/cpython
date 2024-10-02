"""Introspection utils for tasks call stacks."""

import sys
import types
import typing

from . import events
from . import futures
from . import tasks

__all__ = (
    'capture_call_graph',
    'print_call_graph',
    'FrameCallGraphEntry',
    'CoroutineCallGraphEntry',
    'FutureCallGraph',
)

# Sadly, we can't re-use the traceback's module datastructures as those
# are tailored for error reporting, whereas we need to represent an
# async call stack.
#
# Going with pretty verbose names as we'd like to export them to the
# top level asyncio namespace, and want to avoid future name clashes.


class FrameCallGraphEntry(typing.NamedTuple):
    frame: types.FrameType


class CoroutineCallGraphEntry(typing.NamedTuple):
    coroutine: types.CoroutineType


class FutureCallGraph(typing.NamedTuple):
    future: futures.Future
    call_graph: list[FrameCallGraphEntry | CoroutineCallGraphEntry]
    awaited_by: list[FutureCallGraph]


def _build_stack_for_future(future: any) -> FutureCallGraph:
    if not isinstance(future, futures.Future):
        raise TypeError(
            f"{future!r} object does not appear to be compatible "
            f"with asyncio.Future"
        )

    try:
        get_coro = future.get_coro
    except AttributeError:
        coro = None
    else:
        coro = get_coro()

    st: list[CoroutineCallGraphEntry] = []
    awaited_by: list[FutureCallGraph] = []

    while coro is not None:
        if hasattr(coro, 'cr_await'):
            # A native coroutine or duck-type compatible iterator
            st.append(CoroutineCallGraphEntry(coro))
            coro = coro.cr_await
        elif hasattr(coro, 'ag_await'):
            # A native async generator or duck-type compatible iterator
            st.append(CoroutineCallGraphEntry(coro))
            coro = coro.ag_await
        else:
            break

    if fut_waiters := getattr(future, '_asyncio_awaited_by', None):
        for parent in fut_waiters:
            awaited_by.append(_build_stack_for_future(parent))

    st.reverse()
    return FutureCallGraph(future, st, awaited_by)


def capture_call_graph(*, future: any = None) -> FutureCallGraph | None:
    """Capture async call stack for the current task or the provided Future.

    The stack is represented with three data structures:

    * FutureCallGraph(future, call_graph, awaited_by)

      Where 'future' is a reference to an asyncio.Future or asyncio.Task
      (or their subclasses.)

      'call_graph' is a list of FrameCallGraphEntry and CoroutineCallGraphEntry
      objects (more on them below.)

      'awaited_by' is a list of FutureCallGraph objects.

    * FrameCallGraphEntry(frame)

      Where 'frame' is a frame object of a regular Python function
      in the call stack.

    * CoroutineCallGraphEntry(coroutine)

      Where 'coroutine' is a coroutine object of an awaiting coroutine
      or asyncronous generator.

    Receives an optional keyword-only "future" argument. If not passed,
    the current task will be used. If there's no current task, the function
    returns None.
    """

    loop = events._get_running_loop()

    if future is not None:
        # Check if we're in a context of a running event loop;
        # if yes - check if the passed future is the currently
        # running task or not.
        if loop is None or future is not tasks.current_task(loop=loop):
            return _build_stack_for_future(future)
        # else: future is the current task, move on.
    else:
        if loop is None:
            raise RuntimeError(
                'capture_call_graph() is called outside of a running '
                'event loop and no *future* to introspect was provided')
        future = tasks.current_task(loop=loop)

    if future is None:
        # This isn't a generic call stack introspection utility. If we
        # can't determine the current task and none was provided, we
        # just return.
        return None

    if not isinstance(future, futures.Future):
        raise TypeError(
            f"{future!r} object does not appear to be compatible "
            f"with asyncio.Future"
        )

    call_graph: list[FrameCallGraphEntry | CoroutineCallGraphEntry] = []

    f = sys._getframe(1)
    try:
        while f is not None:
            is_async = f.f_generator is not None

            if is_async:
                call_graph.append(CoroutineCallGraphEntry(f.f_generator))
                if f.f_back is not None and f.f_back.f_generator is None:
                    # We've reached the bottom of the coroutine stack, which
                    # must be the Task that runs it.
                    break
            else:
                call_graph.append(FrameCallGraphEntry(f))

            f = f.f_back
    finally:
        del f

    awaited_by = []
    if fut_waiters := getattr(future, '_asyncio_awaited_by', None):
        for parent in fut_waiters:
            awaited_by.append(_build_stack_for_future(parent))

    return FutureCallGraph(future, call_graph, awaited_by)


def print_call_graph(*, future: any = None, file=None) -> None:
    """Print async call stack for the current task or the provided Future."""

    def render_level(st: FutureCallGraph, buf: list[str], level: int):
        def add_line(line: str):
            buf.append(level * '    ' + line)

        if isinstance(st.future, tasks.Task):
            add_line(
                f'* Task(name={st.future.get_name()!r}, id=0x{id(st.future):x})'
            )
        else:
            add_line(
                f'* Future(id=0x{id(st.future):x})'
            )

        if st.call_graph:
            add_line(
                f'  + Call stack:'
            )
            for ste in st.call_graph:
                if isinstance(ste, FrameCallGraphEntry):
                    f = ste.frame
                    add_line(
                        f'  | * {f.f_code.co_qualname}()'
                    )
                    add_line(
                        f'  |   {f.f_code.co_filename}:{f.f_lineno}'
                    )
                else:
                    assert isinstance(ste, CoroutineCallGraphEntry)
                    c = ste.coroutine

                    try:
                        f = c.cr_frame
                        code = c.cr_code
                        tag = 'async'
                    except AttributeError:
                        try:
                            f = c.ag_frame
                            code = c.ag_code
                            tag = 'async generator'
                        except AttributeError:
                            f = c.gi_frame
                            code = c.gi_code
                            tag = 'generator'

                    add_line(
                        f'  | * {tag} {code.co_qualname}()'
                    )
                    add_line(
                        f'  |   {f.f_code.co_filename}:{f.f_lineno}'
                    )

        if st.awaited_by:
            add_line(
                f'  + Awaited by:'
            )
            for fut in st.awaited_by:
                render_level(fut, buf, level + 1)

    stack = capture_call_graph(future=future)
    if stack is None:
        return

    try:
        buf = []
        render_level(stack, buf, 0)
        rendered = '\n'.join(buf)
        print(rendered, file=file)
    finally:
        # 'stack' has references to frames so we should
        # make sure it's GC'ed as soon as we don't need it.
        del stack
