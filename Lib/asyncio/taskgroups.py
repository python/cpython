# Adapted with permission from the EdgeDB project.


__all__ = ["TaskGroup"]

import weakref

from . import events
from . import exceptions
from . import tasks


class TaskGroup:

    def __init__(self):
        self._entered = False
        self._exiting = False
        self._aborting = False
        self._loop = None
        self._parent_task = None
        self._parent_cancel_requested = False
        self._tasks = weakref.WeakSet()
        self._unfinished_tasks = 0
        self._errors = []
        self._base_error = None
        self._on_completed_fut = None

    def __repr__(self):
        info = ['']
        if self._tasks:
            info.append(f'tasks={len(self._tasks)}')
        if self._unfinished_tasks:
            info.append(f'unfinished={self._unfinished_tasks}')
        if self._errors:
            info.append(f'errors={len(self._errors)}')
        if self._aborting:
            info.append('cancelling')
        elif self._entered:
            info.append('entered')

        info_str = ' '.join(info)
        return f'<TaskGroup{info_str}>'

    async def __aenter__(self):
        if self._entered:
            raise RuntimeError(
                f"TaskGroup {self!r} has been already entered")
        self._entered = True

        if self._loop is None:
            self._loop = events.get_running_loop()

        self._parent_task = tasks.current_task(self._loop)
        if self._parent_task is None:
            raise RuntimeError(
                f'TaskGroup {self!r} cannot determine the parent task')

        return self

    async def __aexit__(self, et, exc, tb):
        self._exiting = True
        propagate_cancellation_error = None

        if (exc is not None and
                self._is_base_error(exc) and
                self._base_error is None):
            self._base_error = exc

        if et is exceptions.CancelledError:
            if self._parent_cancel_requested:
                # Only if we did request task to cancel ourselves
                # we mark it as no longer cancelled.
                self._parent_task.uncancel()
            else:
                propagate_cancellation_error = et

        if et is not None and not self._aborting:
            # Our parent task is being cancelled:
            #
            #    async with TaskGroup() as g:
            #        g.create_task(...)
            #        await ...  # <- CancelledError
            #
            if et is exceptions.CancelledError:
                propagate_cancellation_error = et

            # or there's an exception in "async with":
            #
            #    async with TaskGroup() as g:
            #        g.create_task(...)
            #        1 / 0
            #
            self._abort()

        # We use while-loop here because "self._on_completed_fut"
        # can be cancelled multiple times if our parent task
        # is being cancelled repeatedly (or even once, when
        # our own cancellation is already in progress)
        while self._unfinished_tasks:
            if self._on_completed_fut is None:
                self._on_completed_fut = self._loop.create_future()

            try:
                await self._on_completed_fut
            except exceptions.CancelledError as ex:
                if not self._aborting:
                    # Our parent task is being cancelled:
                    #
                    #    async def wrapper():
                    #        async with TaskGroup() as g:
                    #            g.create_task(foo)
                    #
                    # "wrapper" is being cancelled while "foo" is
                    # still running.
                    propagate_cancellation_error = ex
                    self._abort()

            self._on_completed_fut = None

        assert self._unfinished_tasks == 0
        self._on_completed_fut = None  # no longer needed

        if self._base_error is not None:
            raise self._base_error

        if propagate_cancellation_error is not None:
            # The wrapping task was cancelled; since we're done with
            # closing all child tasks, just propagate the cancellation
            # request now.
            raise propagate_cancellation_error

        if et is not None and et is not exceptions.CancelledError:
            self._errors.append(exc)

        if self._errors:
            # Exceptions are heavy objects that can have object
            # cycles (bad for GC); let's not keep a reference to
            # a bunch of them.
            errors = self._errors
            self._errors = None

            me = BaseExceptionGroup('unhandled errors in a TaskGroup', errors)
            raise me from None

    def create_task(self, coro, *, name=None):
        if not self._entered:
            raise RuntimeError(f"TaskGroup {self!r} has not been entered")
        if self._exiting and self._unfinished_tasks == 0:
            raise RuntimeError(f"TaskGroup {self!r} is finished")
        task = self._loop.create_task(coro)
        tasks._set_task_name(task, name)
        task.add_done_callback(self._on_task_done)
        self._unfinished_tasks += 1
        self._tasks.add(task)
        return task

    # Since Python 3.8 Tasks propagate all exceptions correctly,
    # except for KeyboardInterrupt and SystemExit which are
    # still considered special.

    def _is_base_error(self, exc: BaseException) -> bool:
        assert isinstance(exc, BaseException)
        return isinstance(exc, (SystemExit, KeyboardInterrupt))

    def _abort(self):
        self._aborting = True

        for t in self._tasks:
            if not t.done():
                t.cancel()

    def _on_task_done(self, task):
        self._unfinished_tasks -= 1
        assert self._unfinished_tasks >= 0

        if self._on_completed_fut is not None and not self._unfinished_tasks:
            if not self._on_completed_fut.done():
                self._on_completed_fut.set_result(True)

        if task.cancelled():
            return

        exc = task.exception()
        if exc is None:
            return

        self._errors.append(exc)
        if self._is_base_error(exc) and self._base_error is None:
            self._base_error = exc

        if self._parent_task.done():
            # Not sure if this case is possible, but we want to handle
            # it anyways.
            self._loop.call_exception_handler({
                'message': f'Task {task!r} has errored out but its parent '
                           f'task {self._parent_task} is already completed',
                'exception': exc,
                'task': task,
            })
            return

        self._abort()
        if not self._parent_task.cancelling():
            # If parent task *is not* being cancelled, it means that we want
            # to manually cancel it to abort whatever is being run right now
            # in the TaskGroup.  But we want to mark parent task as
            # "not cancelled" later in __aexit__.  Example situation that
            # we need to handle:
            #
            #    async def foo():
            #        try:
            #            async with TaskGroup() as g:
            #                g.create_task(crash_soon())
            #                await something  # <- this needs to be canceled
            #                                 #    by the TaskGroup, e.g.
            #                                 #    foo() needs to be cancelled
            #        except Exception:
            #            # Ignore any exceptions raised in the TaskGroup
            #            pass
            #        await something_else     # this line has to be called
            #                                 # after TaskGroup is finished.
            self._parent_cancel_requested = True
            self._parent_task.cancel()
