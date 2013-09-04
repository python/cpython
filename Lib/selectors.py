"""Selectors module.

This module allows high-level and efficient I/O multiplexing, built upon the
`select` module primitives.
"""


from abc import ABCMeta, abstractmethod
from collections import namedtuple
import functools
import select
import sys


# generic events, that must be mapped to implementation-specific ones
EVENT_READ = (1 << 0)
EVENT_WRITE = (1 << 1)


def _fileobj_to_fd(fileobj):
    """Return a file descriptor from a file object.

    Parameters:
    fileobj -- file object or file descriptor

    Returns:
    corresponding file descriptor
    """
    if isinstance(fileobj, int):
        fd = fileobj
    else:
        try:
            fd = int(fileobj.fileno())
        except (AttributeError, TypeError, ValueError):
            raise ValueError("Invalid file object: "
                             "{!r}".format(fileobj)) from None
    if fd < 0:
        raise ValueError("Invalid file descriptor: {}".format(fd))
    return fd


SelectorKey = namedtuple('SelectorKey', ['fileobj', 'fd', 'events', 'data'])
"""Object used to associate a file object to its backing file descriptor,
selected event mask and attached data."""


class BaseSelector(metaclass=ABCMeta):
    """Base selector class.

    A selector supports registering file objects to be monitored for specific
    I/O events.

    A file object is a file descriptor or any object with a `fileno()` method.
    An arbitrary object can be attached to the file object, which can be used
    for example to store context information, a callback, etc.

    A selector can use various implementations (select(), poll(), epoll()...)
    depending on the platform. The default `Selector` class uses the most
    performant implementation on the current platform.
    """

    def __init__(self):
        # this maps file descriptors to keys
        self._fd_to_key = {}

    def register(self, fileobj, events, data=None):
        """Register a file object.

        Parameters:
        fileobj -- file object or file descriptor
        events  -- events to monitor (bitwise mask of EVENT_READ|EVENT_WRITE)
        data    -- attached data

        Returns:
        SelectorKey instance
        """
        if (not events) or (events & ~(EVENT_READ | EVENT_WRITE)):
            raise ValueError("Invalid events: {!r}".format(events))

        key = SelectorKey(fileobj, _fileobj_to_fd(fileobj), events, data)

        if key.fd in self._fd_to_key:
            raise KeyError("{!r} (FD {}) is already "
                           "registered".format(fileobj, key.fd))

        self._fd_to_key[key.fd] = key
        return key

    def unregister(self, fileobj):
        """Unregister a file object.

        Parameters:
        fileobj -- file object or file descriptor

        Returns:
        SelectorKey instance
        """
        try:
            key = self._fd_to_key.pop(_fileobj_to_fd(fileobj))
        except KeyError:
            raise KeyError("{!r} is not registered".format(fileobj)) from None
        return key

    def modify(self, fileobj, events, data=None):
        """Change a registered file object monitored events or attached data.

        Parameters:
        fileobj -- file object or file descriptor
        events  -- events to monitor (bitwise mask of EVENT_READ|EVENT_WRITE)
        data    -- attached data

        Returns:
        SelectorKey instance
        """
        # TODO: Subclasses can probably optimize this even further.
        try:
            key = self._fd_to_key[_fileobj_to_fd(fileobj)]
        except KeyError:
            raise KeyError("{!r} is not registered".format(fileobj)) from None
        if events != key.events or data != key.data:
            # TODO: If only the data changed, use a shortcut that only
            # updates the data.
            self.unregister(fileobj)
            return self.register(fileobj, events, data)
        else:
            return key

    @abstractmethod
    def select(self, timeout=None):
        """Perform the actual selection, until some monitored file objects are
        ready or a timeout expires.

        Parameters:
        timeout -- if timeout > 0, this specifies the maximum wait time, in
                   seconds
                   if timeout <= 0, the select() call won't block, and will
                   report the currently ready file objects
                   if timeout is None, select() will block until a monitored
                   file object becomes ready

        Returns:
        list of (key, events) for ready file objects
        `events` is a bitwise mask of EVENT_READ|EVENT_WRITE
        """
        raise NotImplementedError()

    def close(self):
        """Close the selector.

        This must be called to make sure that any underlying resource is freed.
        """
        self._fd_to_key.clear()

    def get_key(self, fileobj):
        """Return the key associated to a registered file object.

        Returns:
        SelectorKey for this file object
        """
        try:
            return self._fd_to_key[_fileobj_to_fd(fileobj)]
        except KeyError:
            raise KeyError("{!r} is not registered".format(fileobj)) from None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()

    def _key_from_fd(self, fd):
        """Return the key associated to a given file descriptor.

        Parameters:
        fd -- file descriptor

        Returns:
        corresponding key, or None if not found
        """
        try:
            return self._fd_to_key[fd]
        except KeyError:
            return None


class SelectSelector(BaseSelector):
    """Select-based selector."""

    def __init__(self):
        super().__init__()
        self._readers = set()
        self._writers = set()

    def register(self, fileobj, events, data=None):
        key = super().register(fileobj, events, data)
        if events & EVENT_READ:
            self._readers.add(key.fd)
        if events & EVENT_WRITE:
            self._writers.add(key.fd)
        return key

    def unregister(self, fileobj):
        key = super().unregister(fileobj)
        self._readers.discard(key.fd)
        self._writers.discard(key.fd)
        return key

    if sys.platform == 'win32':
        def _select(self, r, w, _, timeout=None):
            r, w, x = select.select(r, w, w, timeout)
            return r, w + x, []
    else:
        _select = select.select

    def select(self, timeout=None):
        timeout = None if timeout is None else max(timeout, 0)
        ready = []
        try:
            r, w, _ = self._select(self._readers, self._writers, [], timeout)
        except InterruptedError:
            return ready
        r = set(r)
        w = set(w)
        for fd in r | w:
            events = 0
            if fd in r:
                events |= EVENT_READ
            if fd in w:
                events |= EVENT_WRITE

            key = self._key_from_fd(fd)
            if key:
                ready.append((key, events & key.events))
        return ready


if hasattr(select, 'poll'):

    class PollSelector(BaseSelector):
        """Poll-based selector."""

        def __init__(self):
            super().__init__()
            self._poll = select.poll()

        def register(self, fileobj, events, data=None):
            key = super().register(fileobj, events, data)
            poll_events = 0
            if events & EVENT_READ:
                poll_events |= select.POLLIN
            if events & EVENT_WRITE:
                poll_events |= select.POLLOUT
            self._poll.register(key.fd, poll_events)
            return key

        def unregister(self, fileobj):
            key = super().unregister(fileobj)
            self._poll.unregister(key.fd)
            return key

        def select(self, timeout=None):
            timeout = None if timeout is None else max(int(1000 * timeout), 0)
            ready = []
            try:
                fd_event_list = self._poll.poll(timeout)
            except InterruptedError:
                return ready
            for fd, event in fd_event_list:
                events = 0
                if event & ~select.POLLIN:
                    events |= EVENT_WRITE
                if event & ~select.POLLOUT:
                    events |= EVENT_READ

                key = self._key_from_fd(fd)
                if key:
                    ready.append((key, events & key.events))
            return ready


if hasattr(select, 'epoll'):

    class EpollSelector(BaseSelector):
        """Epoll-based selector."""

        def __init__(self):
            super().__init__()
            self._epoll = select.epoll()

        def fileno(self):
            return self._epoll.fileno()

        def register(self, fileobj, events, data=None):
            key = super().register(fileobj, events, data)
            epoll_events = 0
            if events & EVENT_READ:
                epoll_events |= select.EPOLLIN
            if events & EVENT_WRITE:
                epoll_events |= select.EPOLLOUT
            self._epoll.register(key.fd, epoll_events)
            return key

        def unregister(self, fileobj):
            key = super().unregister(fileobj)
            self._epoll.unregister(key.fd)
            return key

        def select(self, timeout=None):
            timeout = -1 if timeout is None else max(timeout, 0)
            max_ev = len(self._fd_to_key)
            ready = []
            try:
                fd_event_list = self._epoll.poll(timeout, max_ev)
            except InterruptedError:
                return ready
            for fd, event in fd_event_list:
                events = 0
                if event & ~select.EPOLLIN:
                    events |= EVENT_WRITE
                if event & ~select.EPOLLOUT:
                    events |= EVENT_READ

                key = self._key_from_fd(fd)
                if key:
                    ready.append((key, events & key.events))
            return ready

        def close(self):
            super().close()
            self._epoll.close()


if hasattr(select, 'kqueue'):

    class KqueueSelector(BaseSelector):
        """Kqueue-based selector."""

        def __init__(self):
            super().__init__()
            self._kqueue = select.kqueue()

        def fileno(self):
            return self._kqueue.fileno()

        def register(self, fileobj, events, data=None):
            key = super().register(fileobj, events, data)
            if events & EVENT_READ:
                kev = select.kevent(key.fd, select.KQ_FILTER_READ,
                                    select.KQ_EV_ADD)
                self._kqueue.control([kev], 0, 0)
            if events & EVENT_WRITE:
                kev = select.kevent(key.fd, select.KQ_FILTER_WRITE,
                                    select.KQ_EV_ADD)
                self._kqueue.control([kev], 0, 0)
            return key

        def unregister(self, fileobj):
            key = super().unregister(fileobj)
            if key.events & EVENT_READ:
                kev = select.kevent(key.fd, select.KQ_FILTER_READ,
                                    select.KQ_EV_DELETE)
                self._kqueue.control([kev], 0, 0)
            if key.events & EVENT_WRITE:
                kev = select.kevent(key.fd, select.KQ_FILTER_WRITE,
                                    select.KQ_EV_DELETE)
                self._kqueue.control([kev], 0, 0)
            return key

        def select(self, timeout=None):
            timeout = None if timeout is None else max(timeout, 0)
            max_ev = len(self._fd_to_key)
            ready = []
            try:
                kev_list = self._kqueue.control(None, max_ev, timeout)
            except InterruptedError:
                return ready
            for kev in kev_list:
                fd = kev.ident
                flag = kev.filter
                events = 0
                if flag == select.KQ_FILTER_READ:
                    events |= EVENT_READ
                if flag == select.KQ_FILTER_WRITE:
                    events |= EVENT_WRITE

                key = self._key_from_fd(fd)
                if key:
                    ready.append((key, events & key.events))
            return ready

        def close(self):
            super().close()
            self._kqueue.close()


# Choose the best implementation: roughly, epoll|kqueue > poll > select.
# select() also can't accept a FD > FD_SETSIZE (usually around 1024)
if 'KqueueSelector' in globals():
    DefaultSelector = KqueueSelector
elif 'EpollSelector' in globals():
    DefaultSelector = EpollSelector
elif 'PollSelector' in globals():
    DefaultSelector = PollSelector
else:
    DefaultSelector = SelectSelector
