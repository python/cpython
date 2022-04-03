:mod:`tkinter.async` --- AsyncIO Support for Tkinter
=====================================================

.. module:: tkinter.async
   :platform: Tk
   :synopsis: A tkinter.Tk subclass for use with asyncio

**Source code:** :source:`Lib/tkinter/async/tk.py`

--------------

The :mod:`tkinter.async.tk` module provides a subclass of `tkinter.Tk`
for use with an `asyncio` event loop.

.. class:: AsyncTk(screenName=None, baseName=None, className='Tk', useTk=True, sync=False, use=None)

   Create a Toplevel Tk widget for use with `asyncio`.

.. method:: tk_loop()

   Process `Tk` events until told to stop.

.. method:: stop()

   Set :data:`running` to `False` so running loops in the runner
   functions can exit.

.. method:: run()

   Run the async functions in :data:`self.runners`.

To add async functions to be run, append them to `self.runners` before
calling :func:`run()`. If they are to run as long as the event loop
runs, use the value of :data:`running` to break out of the loop. The
:func:`counter()` function demonstrates this in the example below.

Example::

    import asyncio
    import random
    from tkinter import Button
    from tkinter.async import AsyncTk

    class App(AsyncTk):
        "User's app"
        def __init__(self):
            super().__init__()
            self.create_interface()
            self.runners.append(self.counter())

        def create_interface(self):
            b1 = Button(master=self, text='Random Float',
                        command=lambda: print("your wish, as they say...",
                                              random.random()))
            b1.pack()
            b2 = Button(master=self, text='Quit', command=self.stop)
            b2.pack()

        async def counter(self):
            "sample async worker"
            i = 1
            while self.running:
                print("and a", i)
                await asyncio.sleep(1)
                i += 1

    async def main():
        app = App()
        await app.run()

    if __name__ == '__main__':
        asyncio.run(main())
