# Adapted with permission from the EdgeDB project;
# license: PSFL.


__all__ = "TaskGroup",

from . import exceptions
from . import taskscopes


class TaskGroup(taskscopes.TaskScope):
    """Asynchronous context manager for managing groups of tasks.

    Example use:

        async with asyncio.TaskGroup() as group:
            task1 = group.create_task(some_coroutine(...))
            task2 = group.create_task(other_coroutine(...))
        print("Both tasks have completed now.")

    All tasks are awaited when the context manager exits.

    Any exceptions other than `asyncio.CancelledError` raised within
    a task will cancel all remaining tasks and wait for them to exit.
    The exceptions are then combined and raised as an `ExceptionGroup`.
    """
    def __init__(self):
        super().__init__(delegate_errors=None)
        self._errors = []

    def __repr__(self):
        info = ['']
        if self._tasks:
            info.append(f'tasks={len(self._tasks)}')
        if self._errors:
            info.append(f'errors={len(self._errors)}')
        if self._aborting:
            info.append('cancelling')
        elif self._entered:
            info.append('entered')

        info_str = ' '.join(info)
        return f'<TaskGroup {info_str}>'

    def create_task(self, coro, *, name=None, context=None):
        """Create a new task in this group and return it.

        Similar to `asyncio.create_task`.
        """
        task = super().create_task(coro, name=name, context=context)
        if not task.done():
            task.add_done_callback(self._handle_completion_as_group)
        return task

    async def __aexit__(self, et, exc, tb):
        await super().__aexit__(et, exc, tb)

        if et is not None and et is not exceptions.CancelledError:
            self._errors.append(exc)

        if self._errors:
            # Exceptions are heavy objects that can have object
            # cycles (bad for GC); let's not keep a reference to
            # a bunch of them.
            try:
                me = BaseExceptionGroup('unhandled errors in a TaskGroup', self._errors)
                raise me from None
            finally:
                self._errors = None

    def _handle_completion_as_group(self, task):
        if task.cancelled():
            return
        if (exc := task.exception()) is not None:
            self._errors.append(exc)
            if not self._aborting and not self._parent_cancel_requested:
                self._abort()
                self._parent_cancel_requested = True
                self._parent_task.cancel()
