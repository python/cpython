import asyncio
from utils import new_messages, send_message

from prompt_toolkit.eventloop.defaults import use_asyncio_event_loop
from prompt_toolkit.patch_stdout import patch_stdout
from prompt_toolkit.shortcuts import PromptSession


async def main():
    use_asyncio_event_loop()

    reader, writer = await asyncio.open_connection('localhost', '9011')
    await send_message(writer, dict(action='connect', username='Eric'))
    await send_message(writer, dict(action='joinroom', room='nonsense'))

    asyncio.create_task(enter_message(writer))

    async for msg in new_messages(reader):
        print(f"{msg['from']}: {msg['message']}")


async def enter_message(writer):
    session = PromptSession('Send message: ', erase_when_done=True)
    while True:
        try:
            msg = await session.prompt(async_=True)
            if not msg:
                continue
            await send_message(writer, msg)
        except (EOFError, asyncio.CancelledError):
            return


if __name__ == '__main__':
    with patch_stdout():
        asyncio.run(main())
