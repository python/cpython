import asyncio
from time import monotonic

t = monotonic()
async def test():
    process = await asyncio.create_subprocess_shell(
        "sleep 20 && echo done",
        stdout=asyncio.subprocess.PIPE,
    )
    await asyncio.sleep(1)
    process.kill()
    await process.wait()
    # process._transport.close()

asyncio.run(test())
print(monotonic()-t)