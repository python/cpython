from functools import partial, Placeholder as _

import asyncio

print(asyncio.Future)  # Will show if it's the C or Python version
print(asyncio.Task)

def callback_no_exc(fut, idx):
    print(f"callback_no_throw: {idx}")


def callback_with_exc(fut):
    print("callback_with_exception")
    raise RuntimeError("TEST EXCEPTION")


async def func(fut):
    try:
        print("func: entered")
        await fut
        print("func: done")
    except Exception as exc:
        print(f"func: EXCEPTION: {exc}")


async def main():
    loop = asyncio.get_running_loop()
    loop.set_task_factory(asyncio.eager_task_factory)
    fut = asyncio.Future(loop=loop)
    fut.add_done_callback(partial(callback_no_exc, _, 1))
    fut.add_done_callback(callback_with_exc)
    fut.add_done_callback(partial(callback_no_exc, _, 2))
    loop.create_task(func(fut))
    await asyncio.sleep(1)
    print("main: before set_result")
    fut.eager_set_result(None)
    print("main: set_result done")
    await asyncio.sleep(1)
    print("main: done")


asyncio.run(main())
