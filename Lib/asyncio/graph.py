"""Introspection utils for tasks call graphs."""

import dataclasses
import io
import sys
import types

from . import events
from . import futures
from . import tasks

__all__ = (
    'capture_call_graph',
    'format_call_graph',
    'print_call_graph',
    'FrameCallGraphEntry',
    'FutureCallGraph',
)

# Sadly, we can't re-use the traceback module's datastructures as those
# are tailored for error reporting, whereas we need to represent an
# async call graph.
#
# Going with pretty verbose names as we'd like to export them to the
# top level asyncio namespace, and want to avoid future name clashes.


@dataclasses.dataclass(frozen=True, slots=True)
class FrameCallGraphEntry:
    frame: types.FrameType


@dataclasses.dataclass(frozen=True, slots=True)
class FutureCallGraph:
    future: futures.Future
    call_stack: tuple["FrameCallGraphEntry", ...]
    awaited_by: tuple["FutureCallGraph", ...]


def _build_graph_for_future(
    future: futures.Future,
    *,
    limit: int | None = None,
) -> FutureCallGraph:
    if not isinstance(future, futures.Future):
        raise TypeError(
            f"{future!r} object does not appear to be compatible "
            f"with asyncio.Future"
        )

    coro = None
    if get_coro := getattr(future, 'get_coro', None):
        coro = get_coro() if limit != 0 else None

    st: list[FrameCallGraphEntry] = []
    awaited_by: list[FutureCallGraph] = []

    while coro is not None:
        if hasattr(coro, 'cr_await'):
            # A native coroutine or duck-type compatible iterator
            st.append(FrameCallGraphEntry(coro.cr_frame))
            coro = coro.cr_await
        elif hasattr(coro, 'ag_await'):
            # A native async generator or duck-type compatible iterator
            st.append(FrameCallGraphEntry(coro.cr_frame))
            coro = coro.ag_await
        else:
            break

    if future._asyncio_awaited_by:
        for parent in future._asyncio_awaited_by:
            awaited_by.append(_build_graph_for_future(parent, limit=limit))

    if limit is not None:
        if limit > 0:
            st = st[:limit]
        elif limit < 0:
            st = st[limit:]
    st.reverse()
    return FutureCallGraph(future, tuple(st), tuple(awaited_by))


def capture_call_graph(
    future: futures.Future | None = None,
    /,
    *,
    depth: int = 1,
    limit: int | None = None,
) -> FutureCallGraph | None:
    """Capture the async call graph for the current task or the provided Future.

    The graph is represented with three data structures:

    * FutureCallGraph(future, call_stack, awaited_by)

      Where 'future' is an instance of asyncio.Future or asyncio.Task.

      'call_stack' is a tuple of FrameGraphEntry objects.

      'awaited_by' is a tuple of FutureCallGraph objects.

    * FrameCallGraphEntry(frame)

      Where 'frame' is a frame object of a regular Python function
      in the call stack.

    Receives an optional 'future' argument. If not passed,
    the current task will be used. If there's no current task, the function
    returns None.

    If "capture_call_graph()" is introspecting *the current task*, the
    optional keyword-only 'depth' argument can be used to skip the specified
    number of frames from top of the stack.

    If the optional keyword-only 'limit' argument is provided, each call stack
    in the resulting graph is truncated to include at most ``abs(limit)``
    entries. If 'limit' is positive, the entries left are the closest to
    the invocation point. If 'limit' is negative, the topmost entries are
    left. If 'limit' is omitted or None, all entries are present.
    If 'limit' is 0, the call stack is not captured at all, only
    "awaited by" information is present.
    """

    loop = events._get_running_loop()

    if future is not None:
        # Check if we're in a context of a running event loop;
        # if yes - check if the passed future is the currently
        # running task or not.
        if loop is None or future is not tasks.current_task(loop=loop):
            return _build_graph_for_future(future, limit=limit)
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

    call_stack: list[FrameCallGraphEntry] = []

    f = sys._getframe(depth) if limit != 0 else None
    try:
        while f is not None:
            is_async = f.f_generator is not None
            call_stack.append(FrameCallGraphEntry(f))

            if is_async:
                if f.f_back is not None and f.f_back.f_generator is None:
                    # We've reached the bottom of the coroutine stack, which
                    # must be the Task that runs it.
                    break

            f = f.f_back
    finally:
        del f

    awaited_by = []
    if future._asyncio_awaited_by:
        for parent in future._asyncio_awaited_by:
            awaited_by.append(_build_graph_for_future(parent, limit=limit))

    if limit is not None:
        limit *= -1
        if limit > 0:
            call_stack = call_stack[:limit]
        elif limit < 0:
            call_stack = call_stack[limit:]

    return FutureCallGraph(future, tuple(call_stack), tuple(awaited_by))


def format_call_graph(
    future: futures.Future | None = None,
    /,
    *,
    depth: int = 1,
    limit: int | None = None,
) -> str:
    """Return the async call graph as a string for `future`.

    If `future` is not provided, format the call graph for the current task.
    """

    def render_level(st: FutureCallGraph, buf: list[str], level: int) -> None:
        def add_line(line: str) -> None:
            buf.append(level * '    ' + line)

        if isinstance(st.future, tasks.Task):
            add_line(
                f'* Task(name={st.future.get_name()!r}, id={id(st.future):#x})'
            )
        else:
            add_line(
                f'* Future(id={id(st.future):#x})'
            )

        if st.call_stack:
            add_line(
                f'  + Call stack:'
            )
            for ste in st.call_stack:
                f = ste.frame

                if f.f_generator is None:
                    f = ste.frame
                    add_line(
                        f'  |   File {f.f_code.co_filename!r},'
                        f' line {f.f_lineno}, in'
                        f' {f.f_code.co_qualname}()'
                    )
                else:
                    c = f.f_generator

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
                        f'  |   File {f.f_code.co_filename!r},'
                        f' line {f.f_lineno}, in'
                        f' {tag} {code.co_qualname}()'
                    )

        if st.awaited_by:
            add_line(
                f'  + Awaited by:'
            )
            for fut in st.awaited_by:
                render_level(fut, buf, level + 1)

    graph = capture_call_graph(future, depth=depth + 1, limit=limit)
    if graph is None:
        return ""

    buf: list[str] = []
    try:
        render_level(graph, buf, 0)
    finally:
        # 'graph' has references to frames so we should
        # make sure it's GC'ed as soon as we don't need it.
        del graph
    return '\n'.join(buf)

def print_call_graph(
    future: futures.Future | None = None,
    /,
    *,
    file: io.Writer[str] | None = None,
    depth: int = 1,
    limit: int | None = None,
) -> None:
    """Print the async call graph for the current task or the provided Future."""
    print(format_call_graph(future, depth=depth, limit=limit), file=file)
