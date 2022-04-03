"An asyncio-friendly Tk class"

import asyncio
from tkinter import Tk

class AsyncTk(Tk):
    "Basic Tk with an asyncio-compatible event loop"
    def __init__(self, screenName=None, baseName=None, className='Tk',
                 useTk=True, sync=False, use=None):
        super().__init__(screenName=screenName, baseName=baseName,
                         className=className, useTk=useTk, sync=sync,
                         use=use)
        self.running = True
        self.runners = [self.tk_loop()]

    async def tk_loop(self):
        "asyncio 'compatible' tk event loop?"
        # Is there a better way to trigger loop exit than using a state vrbl?
        while self.running:
            self.update()
            await asyncio.sleep(0.05) # obviously, sleep time could be parameterized

    def stop(self):
        self.running = False

    async def run(self):
        await asyncio.gather(*self.runners)
