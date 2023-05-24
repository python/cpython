import asyncio
import gc
async def foo():pass

async def main():
    await asyncio.create_task(foo())
    gc.collect(0)
    print(asyncio.all_tasks())


asyncio.run(main())
