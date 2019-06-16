import asyncio

from prompt_toolkit.eventloop.defaults import use_asyncio_event_loop
from prompt_toolkit.patch_stdout import patch_stdout
from prompt_toolkit.shortcuts import PromptSession


async def blah():
    while True:
        print('.')
        await asyncio.sleep(10.0)


async def prompt():
    session = PromptSession('Message: ', erase_when_done=True)
    while True:
        try:
            msg = await session.prompt(async_=True)
            print(msg)
        except (EOFError, asyncio.CancelledError):
            return


async def main():
    use_asyncio_event_loop()
    await asyncio.gather(blah(), prompt())
    # await asyncio.gather(blah())


if __name__ == '__main__':
    with patch_stdout():
        asyncio.run(main())
