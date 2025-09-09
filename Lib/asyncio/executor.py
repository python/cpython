import time
from collections.abc import AsyncIterable, Awaitable, Iterable
from dataclasses import dataclass
from typing import Any, Protocol

from . import timeouts
from .exceptions import CancelledError
from .futures import Future
from .locks import Event
from .queues import Queue, QueueShutDown
from .tasks import FIRST_COMPLETED, Task, create_task, gather, wait

__all__ = (
    "Executor",
)


class _WorkFunction[**P, R](Protocol):
    def __call__(self, *args: P.args, **kwargs: P.kwargs) -> Awaitable[R]:
        ...


@dataclass(frozen=True, slots=True)
class _WorkItem[**P, R]:
    fn: _WorkFunction[P, R]
    args: tuple[Any, ...]
    kwargs: dict[Any, Any]
    future: Future[R]


async def _azip(*iterables: Iterable | AsyncIterable) -> AsyncIterable[tuple]:
    def _as_async_iterable[T](
        iterable: Iterable[T] | AsyncIterable[T],
    ) -> AsyncIterable[T]:
        async def _to_async_iterable(
            iterable: Iterable[T],
        ) -> AsyncIterable[T]:
            for item in iterable:
                yield item

        if isinstance(iterable, AsyncIterable):
            return iterable
        return _to_async_iterable(iterable)

    async_iterables = [_as_async_iterable(iterable) for iterable in iterables]
    iterators = [aiter(async_iterable) for async_iterable in async_iterables]
    while True:
        try:
            items = [await anext(iterator) for iterator in iterators]
            yield tuple(items)
        except StopAsyncIteration:
            break


async def _consume_cancelled_future(future):
    try:
        await future
    except CancelledError:
        pass


class Executor[**P, R]:
    _input_queue: Queue[_WorkItem[P, R]]
    _workers: list[Task]
    _feeders: set[Task]
    _shutdown: bool = False

    def __init__(self, max_workers: int) -> None:
        if max_workers <= 0:
            raise ValueError("max_workers must be greater than 0")

        self._input_queue = Queue(max_workers)
        self._workers = [
            create_task(self._worker())
            for _ in range(max_workers)
        ]
        self._feeders = set()

    async def submit(
        self,
        fn: _WorkFunction[P, R],
        /,
        *args: P.args,
        **kwargs: P.kwargs,
    ) -> Future[R]:
        if self._shutdown:
            raise RuntimeError("Cannot schedule new tasks after shutdown")

        future = Future()
        work_item = _WorkItem(fn, args, kwargs, future)
        await self._input_queue.put(work_item)

        return future

    async def map(
        self,
        fn: _WorkFunction[P, R],
        *iterables: Iterable | AsyncIterable,
        timeout: float | None = None,
    ) -> AsyncIterable[R]:
        if self._shutdown:
            raise RuntimeError("Cannot schedule new tasks after shutdown")

        end_time = None if timeout is None else time.monotonic() + timeout

        inputs_stream = _azip(*iterables)
        submitted_tasks = Queue[Future[R]]()
        tasks_in_flight_limit = len(self._workers) + self._input_queue.maxsize
        resume_feeding = Event()

        feeder_task = create_task(self._feeder(
            inputs_stream,
            fn,
            submitted_tasks,
            tasks_in_flight_limit,
            resume_feeding,
        ))
        self._feeders.add(feeder_task)
        feeder_task.add_done_callback(self._feeders.remove)

        try:
            while True:
                task = await submitted_tasks.get()

                remaining_time = (
                    None if end_time is None else end_time - time.monotonic()
                )
                if remaining_time is not None and remaining_time <= 0:
                    raise TimeoutError()

                async with timeouts.timeout(remaining_time):
                    result = await task
                yield result
                resume_feeding.set()
        except QueueShutDown:
            # The executor was shut down while map was running.
            pass
        finally:
            feeder_task.cancel()
            await _consume_cancelled_future(feeder_task)

            finalization_tasks = []
            while submitted_tasks.qsize() > 0:
                task = submitted_tasks.get_nowait()
                task.cancel()
                finalization_tasks.append(task)
            for task in finalization_tasks:
                await _consume_cancelled_future(task)

    async def shutdown(self, wait=True, *, cancel_futures=False) -> None:
        if self._shutdown:
            return
        self._shutdown = True

        if cancel_futures:
            finalization_tasks = []
            while not self._input_queue.empty():
                work_item = self._input_queue.get_nowait()
                work_item.future.cancel()
                finalization_tasks.append(work_item.future)
            for task in finalization_tasks:
                await _consume_cancelled_future(task)

        self._input_queue.shutdown()

        if wait:
            await gather(*self._workers)

    async def _worker(self) -> None:
        while True:
            try:
                work_item = await self._input_queue.get()
                item_future = work_item.future

                try:
                    if item_future.cancelled():
                        continue

                    task = create_task(work_item.fn(
                        *work_item.args,
                        **work_item.kwargs,
                    ))
                    await wait([task, item_future], return_when=FIRST_COMPLETED)
                    if not item_future.cancelled():
                        item_future.set_result(task.result())
                    else:
                        task.cancel()
                except BaseException as exception:
                    if not item_future.cancelled():
                        item_future.set_exception(exception)
                finally:
                    self._input_queue.task_done()
            except QueueShutDown:  # The executor has been shut down.
                break

    async def _feeder[I](
        self,
        inputs_stream: AsyncIterable[I],
        fn: _WorkFunction[P, R],
        submitted_tasks: Queue[Future[R]],
        tasks_in_flight_limit: int,
        resume_feeding: Event,
    ) -> None:
        try:
            async for args in inputs_stream:
                if self._shutdown:
                    break
                future = await self.submit(fn, *args)  # type: ignore
                await submitted_tasks.put(future)

                if submitted_tasks.qsize() >= tasks_in_flight_limit:
                    await resume_feeding.wait()
                resume_feeding.clear()
        except QueueShutDown:
            # The executor was shut down while feeder waited to submit a
            # task.
            pass
        finally:
            submitted_tasks.shutdown()

    async def __aenter__(self) -> "Executor":
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb) -> bool:
        await self.shutdown(wait=True)
        return False
