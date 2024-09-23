"""Support for running coroutines in parallel with staggered start times."""

__all__ = 'staggered_race',

import contextlib

from . import events
from . import exceptions as exceptions_mod
from . import tasks
from . import taskgroups


async def staggered_race(coro_fns, delay, *, loop=None):
    """Run coroutines with staggered start times and take the first to finish.

    This method takes an iterable of coroutine functions. The first one is
    started immediately. From then on, whenever the immediately preceding one
    fails (raises an exception), or when *delay* seconds has passed, the next
    coroutine is started. This continues until one of the coroutines complete
    successfully, in which case all others are cancelled, or until all
    coroutines fail.

    The coroutines provided should be well-behaved in the following way:

    * They should only ``return`` if completed successfully.

    * They should always raise an exception if they did not complete
      successfully. In particular, if they handle cancellation, they should
      probably reraise, like this::

        try:
            # do work
        except asyncio.CancelledError:
            # undo partially completed work
            raise

    Args:
        coro_fns: an iterable of coroutine functions, i.e. callables that
            return a coroutine object when called. Use ``functools.partial`` or
            lambdas to pass arguments.

        delay: amount of time, in seconds, between starting coroutines. If
            ``None``, the coroutines will run sequentially.

        loop: the event loop to use.

    Returns:
        tuple *(winner_result, winner_index, exceptions)* where

        - *winner_result*: the result of the winning coroutine, or ``None``
          if no coroutines won.

        - *winner_index*: the index of the winning coroutine in
          ``coro_fns``, or ``None`` if no coroutines won. If the winning
          coroutine may return None on success, *winner_index* can be used
          to definitively determine whether any coroutine won.

        - *exceptions*: list of exceptions returned by the coroutines.
          ``len(exceptions)`` is equal to the number of coroutines actually
          started, and the order is the same as in ``coro_fns``. The winning
          coroutine's entry is ``None``.

    """
    # TODO: allow async iterables in coro_fns
    loop = loop or events.get_running_loop()
    winner_result = None
    winner_index = None
    exceptions = []

    def future_callback(index, future, task_group):
        assert future.done()

        try:
            error = future.exception()
            exceptions[index] = error
        except exceptions_mod.CancelledError as cancelled_error:
            # If another task finishes first and cancels this task, it
            # is propagated here.
            exceptions[index] = cancelled_error
            return
        else:
            if error is not None:
                return

        nonlocal winner_result, winner_index
        # If this is in an eager task factory, it's possible
        # for multiple tasks to get here. In that case, we want
        # only the first one to win and the rest to no-op before
        # cancellation.
        if winner_result is None and not task_group._aborting:
            winner_result = future.result()
            winner_index = index

            # Cancel all other tasks, we win!
            task_group._abort()

    async with taskgroups.TaskGroup() as task_group:
        for index, coro in enumerate(coro_fns):
            if task_group._aborting:
                break

            exceptions.append(None)
            task = loop.create_task(coro())

            # We don't want the task group to propagate the error. Instead,
            # we want to put it in our special exceptions list, so we manually
            # create the task.
            task.add_done_callback(task_group._on_task_done_without_propagation)
            task_group._tasks.add(task)

            # We need this extra wrapper here to stop the closure from having
            # an incorrect index.
            def wrapper(idx):
                return lambda future: future_callback(idx, future, task_group)

            task.add_done_callback(wrapper(index))

            if delay is not None:
                await tasks.sleep(delay or 0)
            else:
                # We don't care about exceptions here, the callback will
                # deal with it.
                with contextlib.suppress(BaseException):
                    # If there's no delay, we just wait until completion.
                    await task

    return winner_result, winner_index, exceptions
