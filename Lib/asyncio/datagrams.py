__all__ = ("CallbackDatagramProtocol", "start_datagram_endpoint")

from . import coroutines
from . import events
from . import protocols


class CallbackDatagramProtocol(protocols.DatagramProtocol):
    def __init__(self, datagram_received_cb, error_received_cb, loop):
        super().__init__()
        self.transport = None
        self.datagram_received_cb = datagram_received_cb
        self.error_received_cb = error_received_cb
        self._loop = loop

    def connection_made(self, transport):
        self.transport = transport

    def connection_lost(self, exc):
        self.transport = None

    def datagram_received(self, data, addr):
        res = self.datagram_received_cb(addr, data, self.transport)
        if coroutines.iscoroutine(res):

            def callback(task):
                if task.cancelled():
                    self.transport.close()
                    return
                exc = task.exception()
                if exc is not None:
                    self._loop.call_exception_handler(
                        {
                            "message": "Unhandled exception in datagram_received_cb",
                            "exception": exc,
                            "transport": self.transport,
                        }
                    )
                    self.transport.close()
                    return

            self._task = self._loop.create_task(res)
            self._task.add_done_callback(callback)

    def error_received(self, exc):
        res = self.error_received_cb(exc, self.transport)
        if coroutines.iscoroutine(res):

            def callback(task):
                if task.cancelled():
                    self.transport.close()
                    return
                exc = task.exception()
                if exc is not None:
                    self._loop.call_exception_handler(
                        {
                            "message": "Unhandled exception in error_received_cb",
                            "exception": exc,
                            "transport": self.transport,
                        }
                    )
                    self.transport.close()
                    return

            self._task = self._loop.create_task(res)
            self._task.add_done_callback(callback)


async def start_datagram_endpoint(datagram_received_cb, error_received_cb, host=None, port=None, **kwds):
    loop = events.get_running_loop()

    def factory():
        protocol = CallbackDatagramProtocol(datagram_received_cb, error_received_cb, loop=loop)
        return protocol

    return await loop.create_datagram_endpoint(factory, local_addr=(host, port), **kwds)
