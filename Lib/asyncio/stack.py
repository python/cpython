"""Introspection utils for tasks call stacks."""

import sys
import types
import typing

from . import events
from . import futures
from . import tasks

__all__ = (
    'capture_call_stack',
    'print_call_stack',
    'FrameCallStackEntry',
    'CoroutineCallStackEntry',
    'FutureCallStack',
)

# Sadly, we can't re-use the traceback's module datastructures as those
# are tailored for error reporting, whereas we need to represent an
# async call stack.
#
# Going with pretty verbose names as we'd like to export them to the
# top level asyncio namespace, and want to avoid future name clashes.


class FrameCallStackEntry(typing.NamedTuple):
    frame: types.FrameType


class CoroutineCallStackEntry(typing.NamedTuple):
    coroutine: types.CoroutineType


class FutureCallStack(typing.NamedTuple):
    future: futures.Future
    call_stack: list[FrameCallStackEntry | CoroutineCallStackEntry]
    awaited_by: list[FutureCallStack]


def _build_stack_for_future(future: any) -> FutureCallStack:
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

    st: list[CoroutineCallStackEntry] = []
    awaited_by: list[FutureCallStack] = []

    while coro is not None:
        if hasattr(coro, 'cr_await'):
            # A native coroutine or duck-type compatible iterator
            st.append(CoroutineCallStackEntry(coro))
            coro = coro.cr_await
        elif hasattr(coro, 'ag_await'):
            # A native async generator or duck-type compatible iterator
            st.append(CoroutineCallStackEntry(coro))
            coro = coro.ag_await
        else:
            break

    if fut_waiters := getattr(future, '_asyncio_awaited_by', None):
        for parent in fut_waiters:
            awaited_by.append(_build_stack_for_future(parent))

    st.reverse()
    return FutureCallStack(future, st, awaited_by)


def capture_call_stack(*, future: any = None) -> FutureCallStack | None:
    """Capture async call stack for the current task or the provided Future.

    The stack is represented with three data structures:

    * FutureCallStack(future, call_stack, awaited_by)

      Where 'future' is a reference to an asyncio.Future or asyncio.Task
      (or their subclasses.)

      'call_stack' is a list of FrameCallStackEntry and CoroutineCallStackEntry
      objects (more on them below.)

      'awaited_by' is a list of FutureCallStack objects.

    * FrameCallStackEntry(frame)

      Where 'frame' is a frame object of a regular Python function
      in the call stack.

    * CoroutineCallStackEntry(coroutine)

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
                'capture_call_stack() is called outside of a running '
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

    call_stack: list[FrameCallStackEntry | CoroutineCallStackEntry] = []

    f = sys._getframe(1)
    try:
        while f is not None:
            is_async = f.f_generator is not None

            if is_async:
                call_stack.append(CoroutineCallStackEntry(f.f_generator))
                if f.f_back is not None and f.f_back.f_generator is None:
                    # We've reached the bottom of the coroutine stack, which
                    # must be the Task that runs it.
                    break
            else:
                call_stack.append(FrameCallStackEntry(f))

            f = f.f_back
    finally:
        del f

    awaited_by = []
    if fut_waiters := getattr(future, '_asyncio_awaited_by', None):
        for parent in fut_waiters:
            awaited_by.append(_build_stack_for_future(parent))

    return FutureCallStack(future, call_stack, awaited_by)


def print_call_stack(*, future: any = None, file=None) -> None:
    """Print async call stack for the current task or the provided Future."""

    def render_level(st: FutureCallStack, buf: list[str], level: int):
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

        if st.call_stack:
            add_line(
                f'  + Call stack:'
            )
            for ste in st.call_stack:
                if isinstance(ste, FrameCallStackEntry):
                    f = ste.frame
                    add_line(
                        f'  | * {f.f_code.co_qualname}()'
                    )
                    add_line(
                        f'  |   {f.f_code.co_filename}:{f.f_lineno}'
                    )
                else:
                    assert isinstance(ste, CoroutineCallStackEntry)
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

    stack = capture_call_stack(future=future)
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
