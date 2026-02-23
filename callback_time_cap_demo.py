import asyncio

import cProfile


def callback(final_fut, remaining, fut):
    if remaining > 0:
        next_fut = asyncio.get_running_loop().create_future()
        next_fut.add_done_callback(lambda x: callback(final_fut, remaining - 1, x))
        next_fut.set_result(None)
    else:
        final_fut.set_result(None)


async def execute_test(num_steps):
    loop = asyncio.get_running_loop()
    final_f = loop.create_future()
    f = loop.create_future()
    f.add_done_callback(lambda x: callback(final_f, num_steps, x))

    t0 = loop.time()
    f.set_result(None)
    await final_f
    return loop.time() - t0


async def main():
    loop = asyncio.get_running_loop()

    # Warm up cache
    loop.set_callback_total_time_cap(None)
    await execute_test(100000)

    loop.set_callback_total_time_cap(100)
    opt_dt = await execute_test(10000)

    loop.set_callback_total_time_cap(None)
    orig_dt = await execute_test(10000)

    await asyncio.sleep(0.1)

    print(f"{opt_dt=}")
    print(f"{orig_dt=}")

asyncio.run(main())

# cProfile.run('asyncio.run(main())', sort='cumulative')

