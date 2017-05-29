"""Protocol server controller."""

__all__ = ['Controller']


import asyncio
import os
import socket
import threading


class Controller:
    def __init__(self, hostname, port, ready_timeout=1.0, loop=None):
        self.hostname = hostname
        self.port = port
        self.loop = asyncio.new_event_loop() if loop is None else loop
        self.server = None
        self._thread = None
        self._thread_exception = None
        envar = os.getenv('PYTHONASYNCIOCONTROLLERTIMEOUT')
        self.ready_timeout = ready_timeout if envar is None else float(envar)
        # For exiting the loop.
        self._rsock, self._wsock = socket.socketpair()
        self.loop.add_reader(self._rsock, self._reader)

    def _reader(self):
        self.loop.remove_reader(self._rsock)
        self.loop.stop()
        for task in asyncio.Task.all_tasks(self.loop):
            task.cancel()
        self._rsock.close()
        self._wsock.close()

    def factory(self):
        """Allow subclasses to customize the handler/server creation."""
        raise NotImplementedError

    def _run(self, ready_event):
        asyncio.set_event_loop(self.loop)
        try:
            self.server = self.loop.run_until_complete(
                self.loop.create_server(
                    self.factory, host=self.hostname, port=self.port))
        except Exception as error:
            self._thread_exception = error
            return
        self.loop.call_soon(ready_event.set)
        self.loop.run_forever()
        self.server.close()
        self.loop.run_until_complete(self.server.wait_closed())
        self.loop.close()
        self.server = None

    def start(self):
        if self._thread is not None:
            # Already started.
            return
        ready_event = threading.Event()
        self._thread = threading.Thread(target=self._run, args=(ready_event,))
        self._thread.daemon = True
        self._thread.start()
        # Wait a while until the server is responding.
        ready_event.wait(self.ready_timeout)
        if self._thread_exception is not None:
            raise self._thread_exception

    def stop(self):
        if self._thread is None:
            # Already stopped.
            return
        self._wsock.send(b'x')
        self._thread.join()
        self._thread = None
