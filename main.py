import asyncio
from asyncio import subprocess


async def whoami():
    print('Finding out who am I...')
    proc = await subprocess.create_subprocess_exec(
        'whoami',
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    print('Waiting for the process to finish...', proc)
    stdout, _ = await proc.communicate()
    print(f'I am {stdout}')


async def main():
    t2 = asyncio.create_task(whoami())
    await asyncio.sleep(0)
    # await asyncio.sleep(0) Uncomment to fix hang

if __name__ == '__main__':
    asyncio.run(main())
