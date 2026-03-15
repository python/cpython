#!/usr/bin/env python3
"""Minimal demo: asyncio running as a guest inside Tkinter's mainloop.

A progress bar counts from 0 to MAX_COUNT using ``asyncio.sleep()``.
The Tk GUI stays fully responsive throughout.  Closing the window or
pressing the Cancel button cancels the async task cleanly.

Usage::

    python asyncio_guest_tkinter.py
"""

import asyncio
import collections
import tkinter as tk
import tkinter.ttk as ttk
import traceback


# -- Host adapter for Tkinter ------------------------------------------

class TkHost:
    """Bridge between asyncio guest mode and the Tk event loop."""

    def __init__(self, root):
        self.root = root
        self._tk_func_name = root.register(self._dispatch)
        self._q = collections.deque()

    def _dispatch(self):
        self._q.popleft()()

    def run_sync_soon_threadsafe(self, fn):
        """Schedule *fn* on the Tk thread.

        ``Tkapp_ThreadSend`` (the C layer behind ``root.call`` from a
        non-Tcl thread) posts the command to the Tcl event queue, making
        this safe to call from any thread.
        """
        self._q.append(fn)
        self.root.call('after', 'idle', self._tk_func_name)

    def done_callback(self, task):
        """Called when the async task finishes."""
        if task.cancelled():
            print("Task was cancelled.")
        elif task.exception() is not None:
            exc = task.exception()
            traceback.print_exception(type(exc), exc, exc.__traceback__)
        else:
            print(f"Task returned: {task.result()}")
        self.root.destroy()


# -- Async workload ----------------------------------------------------

MAX_COUNT = 20
PERIOD = 0.5  # seconds between increments


async def count(progress, root):
    """Increment a progress bar, updating the Tk GUI each step."""
    root.wm_title(f"Counting every {PERIOD}s ...")
    progress.configure(maximum=MAX_COUNT)

    task = asyncio.current_task()
    loop = asyncio.get_event_loop()

    # Wire the Cancel button and window close to task.cancel().
    # Use call_soon_threadsafe so the I/O thread's selector is woken.
    def request_cancel():
        loop.call_soon_threadsafe(task.cancel)

    cancel_btn = root.nametowidget('cancel')
    cancel_btn.configure(command=request_cancel)
    root.protocol("WM_DELETE_WINDOW", request_cancel)

    for i in range(1, MAX_COUNT + 1):
        await asyncio.sleep(PERIOD)
        progress.step(1)
        root.wm_title(f"Count: {i}/{MAX_COUNT}")

    return i


# -- Main ---------------------------------------------------------------

def main():
    root = tk.Tk()
    root.wm_title("asyncio guest + Tkinter")

    progress = ttk.Progressbar(root, length='6i')
    progress.pack(fill=tk.BOTH, expand=True, padx=8, pady=(8, 4))

    cancel_btn = tk.Button(root, text='Cancel', name='cancel')
    cancel_btn.pack(pady=(0, 8))

    host = TkHost(root)

    asyncio.start_guest_run(
        count, progress, root,
        run_sync_soon_threadsafe=host.run_sync_soon_threadsafe,
        done_callback=host.done_callback,
    )

    root.mainloop()


if __name__ == '__main__':
    main()
