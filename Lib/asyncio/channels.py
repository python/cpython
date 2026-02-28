"""Memory object channels for asyncio."""

__all__ = ('open_channel', 'SendChannel', 'ReceiveChannel',
           'ChannelStatistics')

import collections
import dataclasses
import math
from types import GenericAlias

from . import exceptions
from . import mixins


@dataclasses.dataclass(frozen=True)
class ChannelStatistics:
    """Statistics for a memory channel pair."""
    current_buffer_used: int
    max_buffer_size: int | float
    open_send_channels: int
    open_receive_channels: int
    tasks_waiting_send: int
    tasks_waiting_receive: int


class _ChannelState:
    """Shared internal state between SendChannel and ReceiveChannel."""

    __slots__ = ('max_buffer_size', 'buffer', 'open_send_channels',
                 'open_receive_channels', 'waiting_senders',
                 'waiting_receivers')

    def __init__(self, max_buffer_size):
        self.max_buffer_size = max_buffer_size
        self.buffer = collections.deque()
        self.open_send_channels = 0
        self.open_receive_channels = 0
        # OrderedDict preserves insertion order for FIFO wakeup.
        # waiting_senders: Future -> item
        self.waiting_senders = collections.OrderedDict()
        # waiting_receivers: Future -> None
        self.waiting_receivers = collections.OrderedDict()

    def statistics(self):
        return ChannelStatistics(
            current_buffer_used=len(self.buffer),
            max_buffer_size=self.max_buffer_size,
            open_send_channels=self.open_send_channels,
            open_receive_channels=self.open_receive_channels,
            tasks_waiting_send=len(self.waiting_senders),
            tasks_waiting_receive=len(self.waiting_receivers),
        )


class SendChannel(mixins._LoopBoundMixin):
    """The sending end of a memory object channel.

    Items sent through this channel will be received by the corresponding
    ReceiveChannel. Multiple clones can exist for fan-in patterns.
    """

    __slots__ = ('_state', '_closed')

    def __init__(self, state):
        self._state = state
        self._closed = False
        state.open_send_channels += 1

    def send_nowait(self, item):
        """Send an item without blocking.

        Raises ClosedResourceError if the channel is closed.
        Raises BrokenResourceError if all receivers are closed.
        Raises WouldBlock if the buffer is full and no receiver is waiting.
        """
        if self._closed:
            raise exceptions.ClosedResourceError(
                "this send channel is closed")
        state = self._state
        if state.open_receive_channels == 0:
            raise exceptions.BrokenResourceError(
                "all receive channels are closed")
        # Try to deliver directly to a waiting receiver.
        while state.waiting_receivers:
            fut, _ = state.waiting_receivers.popitem(last=False)
            if not fut.done():
                fut.set_result(item)
                return
        # Try to buffer the item.
        if len(state.buffer) < state.max_buffer_size:
            state.buffer.append(item)
            return
        raise exceptions.WouldBlock

    async def send(self, item):
        """Send an item, blocking if the buffer is full.

        Raises ClosedResourceError if the channel is closed.
        Raises BrokenResourceError if all receivers are closed.
        """
        try:
            self.send_nowait(item)
            return
        except exceptions.WouldBlock:
            pass
        loop = self._get_loop()
        fut = loop.create_future()
        state = self._state
        state.waiting_senders[fut] = item
        try:
            await fut
        except BaseException:
            state.waiting_senders.pop(fut, None)
            raise

    def clone(self):
        """Create a clone of this send channel sharing the same state.

        Raises ClosedResourceError if this channel is closed.
        """
        if self._closed:
            raise exceptions.ClosedResourceError(
                "this send channel is closed")
        return SendChannel(self._state)

    def close(self):
        """Close this send channel.

        When the last send channel clone is closed, all waiting receivers
        will receive EndOfChannel.
        """
        if self._closed:
            return
        self._closed = True
        state = self._state
        state.open_send_channels -= 1
        if state.open_send_channels == 0:
            # Last sender closed — wake all waiting receivers.
            while state.waiting_receivers:
                fut, _ = state.waiting_receivers.popitem(last=False)
                if not fut.done():
                    fut.set_exception(exceptions.EndOfChannel())
            # Don't clear buffer — receivers may still drain it.

    async def aclose(self):
        """Async close (for async with support)."""
        self.close()

    def statistics(self):
        """Return channel statistics."""
        return self._state.statistics()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()

    async def __aenter__(self):
        return self

    async def __aexit__(self, *exc_info):
        self.close()

    def __repr__(self):
        state = self._state
        info = []
        if self._closed:
            info.append('closed')
        info.append(f'max_buffer_size={state.max_buffer_size!r}')
        info.append(f'current_buffer_used={len(state.buffer)}')
        return f'<{type(self).__name__} {" ".join(info)}>'

    __class_getitem__ = classmethod(GenericAlias)


class ReceiveChannel(mixins._LoopBoundMixin):
    """The receiving end of a memory object channel.

    Items can be received one at a time or via async iteration.
    Multiple clones can exist for fan-out patterns.
    """

    __slots__ = ('_state', '_closed')

    def __init__(self, state):
        self._state = state
        self._closed = False
        state.open_receive_channels += 1

    def receive_nowait(self):
        """Receive an item without blocking.

        Raises ClosedResourceError if the channel is closed.
        Raises EndOfChannel if all senders are closed and the buffer is empty.
        Raises WouldBlock if no item is available.
        """
        if self._closed:
            raise exceptions.ClosedResourceError(
                "this receive channel is closed")
        state = self._state
        # Try to accept an item from a waiting sender to refill the buffer.
        while state.waiting_senders:
            fut, item = state.waiting_senders.popitem(last=False)
            if not fut.done():
                state.buffer.append(item)
                fut.set_result(None)
                break
        # Try to return from the buffer.
        if state.buffer:
            return state.buffer.popleft()
        if state.open_send_channels == 0:
            raise exceptions.EndOfChannel
        raise exceptions.WouldBlock

    async def receive(self):
        """Receive an item, blocking if the buffer is empty.

        Raises ClosedResourceError if the channel is closed.
        Raises EndOfChannel if all senders are closed and the buffer is empty.
        """
        try:
            return self.receive_nowait()
        except exceptions.WouldBlock:
            pass
        loop = self._get_loop()
        fut = loop.create_future()
        state = self._state
        state.waiting_receivers[fut] = None
        try:
            return await fut
        except BaseException:
            state.waiting_receivers.pop(fut, None)
            raise

    def clone(self):
        """Create a clone of this receive channel sharing the same state.

        Raises ClosedResourceError if this channel is closed.
        """
        if self._closed:
            raise exceptions.ClosedResourceError(
                "this receive channel is closed")
        return ReceiveChannel(self._state)

    def close(self):
        """Close this receive channel.

        When the last receive channel clone is closed, all waiting senders
        will receive BrokenResourceError and the buffer will be cleared.
        """
        if self._closed:
            return
        self._closed = True
        state = self._state
        state.open_receive_channels -= 1
        if state.open_receive_channels == 0:
            # Last receiver closed — wake all waiting senders.
            while state.waiting_senders:
                fut, _ = state.waiting_senders.popitem(last=False)
                if not fut.done():
                    fut.set_exception(exceptions.BrokenResourceError(
                        "all receive channels are closed"))
            state.buffer.clear()

    async def aclose(self):
        """Async close (for async with support)."""
        self.close()

    def statistics(self):
        """Return channel statistics."""
        return self._state.statistics()

    def __enter__(self):
        return self

    def __exit__(self, *exc_info):
        self.close()

    async def __aenter__(self):
        return self

    async def __aexit__(self, *exc_info):
        self.close()

    def __aiter__(self):
        return self

    async def __anext__(self):
        try:
            return await self.receive()
        except exceptions.EndOfChannel:
            raise StopAsyncIteration from None

    def __repr__(self):
        state = self._state
        info = []
        if self._closed:
            info.append('closed')
        info.append(f'max_buffer_size={state.max_buffer_size!r}')
        info.append(f'current_buffer_used={len(state.buffer)}')
        return f'<{type(self).__name__} {" ".join(info)}>'

    __class_getitem__ = classmethod(GenericAlias)


def open_channel(max_buffer_size):
    """Create a new memory object channel pair.

    Returns a (SendChannel, ReceiveChannel) tuple.

    max_buffer_size is the maximum number of items that can be buffered.
    Use 0 for an unbuffered (rendezvous) channel. Use math.inf for an
    unbounded buffer.

    Raises ValueError if max_buffer_size is negative or not a valid number.
    """
    if not isinstance(max_buffer_size, (int, float)):
        raise TypeError(
            f"max_buffer_size must be int or float, "
            f"got {type(max_buffer_size).__name__}")
    if max_buffer_size < 0:
        raise ValueError("max_buffer_size must be >= 0")
    if isinstance(max_buffer_size, float) and max_buffer_size != math.inf:
        raise ValueError(
            "float max_buffer_size only accepts math.inf")

    state = _ChannelState(max_buffer_size)
    send_channel = SendChannel(state)
    receive_channel = ReceiveChannel(state)
    return send_channel, receive_channel
