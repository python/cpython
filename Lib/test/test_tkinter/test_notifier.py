import os
import select
import time
import unittest

from tkinter import _tkinter


class TkinterEventHandler:
    def __init__(self, proc, clientdata):
        self.proc = proc
        self.clientdata = clientdata
        self.ready_events = 0
        self.dispatched = False

    def process_event(self, flags):
        if not (flags & _tkinter.FILE_EVENTS):
            return False

        events = self.ready_events
        self.ready_events = 0

        if events:
            self.dispatched = True

        return True

    def __call__(self, events):
        if self.ready_events == 0:
            _tkinter.queue_event(self.process_event)

        self.ready_events = events


class EventSource:
    def __init__(self):
        self.handlers = {}
        self.timer_abstime = 0

    def set_timer(self, t):
        if t is None:
            self.timer_abstime = 0
        else:
            sec, usec = t
            self.timer_abstime = (
                time.perf_counter_ns() // 1000
                + sec * 1_000_000
                + usec
            )

        _tkinter.set_service_mode(_tkinter.SERVICE_ALL)

    def wait_for_event(self, timeout):
        return 0

    def create_file_handler(self, fd, mask, proc, clientdata):
        if mask & _tkinter.READABLE:
            self.handlers[fd] = TkinterEventHandler(
                proc,
                clientdata,
            )

    def delete_file_handler(self, fd):
        self.handlers.pop(fd, None)

    def init_notifier(self):
        return None

    def finalize_notifier(self, clientdata):
        return None

    def alert_notifier(self, clientdata):
        return None

    def service_mode_hook(self, mode):
        return None

    def setup(self):
        if self.timer_abstime:
            now = time.perf_counter_ns() // 1000
            timeout_usec = self.timer_abstime - now
            timeout = max(
                0.0,
                timeout_usec / 1_000_000,
            )
        else:
            timeout = None

        return timeout, list(self.handlers)

    def check(self, ready_events):
        if self.timer_abstime:
            now = time.perf_counter_ns() // 1000
            if self.timer_abstime <= now:
                self.timer_abstime = 0

        ready_rlist, _, _ = ready_events

        for fd in ready_rlist:
            handler = self.handlers.get(fd)
            if handler is not None:
                handler(_tkinter.READABLE)

        _tkinter.service_all()


source = EventSource()

_tkinter.set_notifier(
    source.set_timer,
    source.wait_for_event,
    source.create_file_handler,
    source.delete_file_handler,
    source.init_notifier,
    source.finalize_notifier,
    source.alert_notifier,
    source.service_mode_hook,
)

INTERP = _tkinter.create(
    None,
    "",
    "Tk",
    False,
    1,
    False,
    False,
    None,
)


class TclNotifierTests(unittest.TestCase):

    def test_set_notifier_is_one_shot(self):
        with self.assertRaises(RuntimeError):
            _tkinter.set_notifier(
                None,
                None,
                None,
                None,
                None,
                None,
                None,
                None,
            )

    def test_queue_event_is_serviced(self):
        _tkinter.set_service_mode(_tkinter.SERVICE_ALL)

        served = []

        def callback(flags):
            served.append(flags)
            return True

        _tkinter.queue_event(callback)
        _tkinter.service_all()

        self.assertEqual(len(served), 1)

    def test_file_handler_registration_via_external_loop(self):
        rfd, wfd = os.pipe()

        self.addCleanup(os.close, rfd)
        self.addCleanup(os.close, wfd)

        def callback(fd, mask):
            os.read(fd, 64)

        INTERP.createfilehandler(
            rfd,
            _tkinter.READABLE,
            callback,
        )

        self.addCleanup(
            INTERP.deletefilehandler,
            rfd,
        )

        self.assertIn(rfd, source.handlers)

    def test_timer_via_external_loop(self):
        called = []

        def callback():
            called.append(True)

        INTERP.createtimerhandler(
            1,
            callback,
        )

        deadline = time.monotonic() + 1.0

        while time.monotonic() < deadline and not called:
            timeout, fds = source.setup()

            remaining = max(
                0.0,
                deadline - time.monotonic(),
            )

            if timeout is None:
                timeout = remaining
            else:
                timeout = min(timeout, remaining)

            ready = select.select(
                fds,
                [],
                [],
                timeout,
            )

            source.check(ready)

        self.assertTrue(called)

    def test_external_loop_dispatches_ready_event(self):
        rfd, wfd = os.pipe()

        self.addCleanup(os.close, rfd)
        self.addCleanup(os.close, wfd)

        INTERP.createfilehandler(
            rfd,
            _tkinter.READABLE,
            lambda fd, mask: None,
        )

        self.addCleanup(
            INTERP.deletefilehandler,
            rfd,
        )

        os.write(wfd, b"ready")

        timeout, fds = source.setup()

        self.assertIn(rfd, fds)

        ready = select.select(
            fds,
            [],
            [],
            timeout,
        )

        source.check(ready)

        handler = source.handlers[rfd]

        self.assertTrue(
            handler.dispatched,
            "file-handler event was not queued and serviced",
        )


if __name__ == "__main__":
    unittest.main()
