import errno
import os
import select
import selectors
import sys
import time
import unittest
from test import support


@unittest.skipUnless(hasattr(os, 'timerfd_create'), 'requires os.timerfd_create')
@unittest.skipIf(sys.platform == "android", "gh-124873: Test is flaky on Android")
@support.requires_linux_version(2, 6, 30)
class TimerfdTests(unittest.TestCase):
    # gh-126112: Use 10 ms to tolerate slow buildbots
    CLOCK_RES_PLACES = 2  # 10 ms
    CLOCK_RES = 10 ** -CLOCK_RES_PLACES
    CLOCK_RES_NS = 10 ** (9 - CLOCK_RES_PLACES)

    def timerfd_create(self, *args, **kwargs):
        fd = os.timerfd_create(*args, **kwargs)
        self.assertGreaterEqual(fd, 0)
        self.assertFalse(os.get_inheritable(fd))
        self.addCleanup(os.close, fd)
        return fd

    def read_count_signaled(self, fd):
        # read 8 bytes
        data = os.read(fd, 8)
        return int.from_bytes(data, byteorder=sys.byteorder)

    def test_timerfd_initval(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        initial_expiration = 0.25
        interval = 0.125

        # 1st call
        next_expiration, interval2 = os.timerfd_settime(fd, initial=initial_expiration, interval=interval)
        self.assertAlmostEqual(interval2, 0.0, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, 0.0, places=self.CLOCK_RES_PLACES)

        # 2nd call
        next_expiration, interval2 = os.timerfd_settime(fd, initial=initial_expiration, interval=interval)
        self.assertAlmostEqual(interval2, interval, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, initial_expiration, places=self.CLOCK_RES_PLACES)

        # timerfd_gettime
        next_expiration, interval2 = os.timerfd_gettime(fd)
        self.assertAlmostEqual(interval2, interval, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, initial_expiration, places=self.CLOCK_RES_PLACES)

    def test_timerfd_non_blocking(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME, flags=os.TFD_NONBLOCK)

        # 0.1 second later
        initial_expiration = 0.1
        os.timerfd_settime(fd, initial=initial_expiration, interval=0)

        # read() raises OSError with errno is EAGAIN for non-blocking timer.
        with self.assertRaises(OSError) as ctx:
            self.read_count_signaled(fd)
        self.assertEqual(ctx.exception.errno, errno.EAGAIN)

        # Wait more than 0.1 seconds
        time.sleep(initial_expiration + 0.1)

        # confirm if timerfd is readable and read() returns 1 as bytes.
        self.assertEqual(self.read_count_signaled(fd), 1)

    @unittest.skipIf(sys.platform.startswith('netbsd'),
                     "gh-131263: Skip on NetBSD due to system freeze "
                     "with negative timer values")
    def test_timerfd_negative(self):
        one_sec_in_nsec = 10**9
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        test_flags = [0, os.TFD_TIMER_ABSTIME]
        if hasattr(os, 'TFD_TIMER_CANCEL_ON_SET'):
            test_flags.append(os.TFD_TIMER_ABSTIME | os.TFD_TIMER_CANCEL_ON_SET)

        # Any of 'initial' and 'interval' is negative value.
        for initial, interval in ( (-1, 0), (1, -1), (-1, -1),  (-0.1, 0), (1, -0.1), (-0.1, -0.1)):
            for flags in test_flags:
                with self.subTest(flags=flags, initial=initial, interval=interval):
                    with self.assertRaises(OSError) as context:
                        os.timerfd_settime(fd, flags=flags, initial=initial, interval=interval)
                    self.assertEqual(context.exception.errno, errno.EINVAL)

                    with self.assertRaises(OSError) as context:
                        initial_ns = int( one_sec_in_nsec * initial )
                        interval_ns = int( one_sec_in_nsec * interval )
                        os.timerfd_settime_ns(fd, flags=flags, initial=initial_ns, interval=interval_ns)
                    self.assertEqual(context.exception.errno, errno.EINVAL)

    def test_timerfd_interval(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        # 1 second
        initial_expiration = 1
        # 0.5 second
        interval = 0.5

        os.timerfd_settime(fd, initial=initial_expiration, interval=interval)

        # timerfd_gettime
        next_expiration, interval2 = os.timerfd_gettime(fd)
        self.assertAlmostEqual(interval2, interval, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, initial_expiration, places=self.CLOCK_RES_PLACES)

        count = 3
        t = time.perf_counter()
        for _ in range(count):
            self.assertEqual(self.read_count_signaled(fd), 1)
        t = time.perf_counter() - t

        total_time = initial_expiration + interval * (count - 1)
        self.assertGreater(t, total_time - self.CLOCK_RES)

        # wait 3.5 time of interval
        time.sleep( (count+0.5) * interval)
        self.assertEqual(self.read_count_signaled(fd), count)

    def test_timerfd_TFD_TIMER_ABSTIME(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        now = time.clock_gettime(time.CLOCK_REALTIME)

        # 1 second later from now.
        offset = 1
        initial_expiration = now + offset
        # not interval timer
        interval = 0

        os.timerfd_settime(fd, flags=os.TFD_TIMER_ABSTIME, initial=initial_expiration, interval=interval)

        # timerfd_gettime
        # Note: timerfd_gettime returns relative values even if TFD_TIMER_ABSTIME is specified.
        next_expiration, interval2 = os.timerfd_gettime(fd)
        self.assertAlmostEqual(interval2, interval, places=self.CLOCK_RES_PLACES)
        self.assertAlmostEqual(next_expiration, offset, places=self.CLOCK_RES_PLACES)

        t = time.perf_counter()
        count_signaled = self.read_count_signaled(fd)
        t = time.perf_counter() - t
        self.assertEqual(count_signaled, 1)

        self.assertGreater(t, offset - self.CLOCK_RES)

    def test_timerfd_select(self):
        fd = self.timerfd_create(time.CLOCK_REALTIME, flags=os.TFD_NONBLOCK)

        rfd, wfd, xfd = select.select([fd], [fd], [fd], 0)
        self.assertEqual((rfd, wfd, xfd), ([], [], []))

        # 0.25 second
        initial_expiration = 0.25
        # every 0.125 second
        interval = 0.125

        os.timerfd_settime(fd, initial=initial_expiration, interval=interval)

        count = 3
        t = time.perf_counter()
        for _ in range(count):
            rfd, wfd, xfd = select.select([fd], [fd], [fd], initial_expiration + interval)
            self.assertEqual((rfd, wfd, xfd), ([fd], [], []))
            self.assertEqual(self.read_count_signaled(fd), 1)
        t = time.perf_counter() - t

        total_time = initial_expiration + interval * (count - 1)
        self.assertGreater(t, total_time - self.CLOCK_RES)

    def check_timerfd_poll(self, nanoseconds):
        fd = self.timerfd_create(time.CLOCK_REALTIME, flags=os.TFD_NONBLOCK)

        selector = selectors.DefaultSelector()
        selector.register(fd, selectors.EVENT_READ)
        self.addCleanup(selector.close)

        sec_to_nsec = 10 ** 9
        # 0.25 second
        initial_expiration_ns = sec_to_nsec // 4
        # every 0.125 second
        interval_ns = sec_to_nsec // 8

        if nanoseconds:
            os.timerfd_settime_ns(fd,
                                  initial=initial_expiration_ns,
                                  interval=interval_ns)
        else:
            os.timerfd_settime(fd,
                               initial=initial_expiration_ns / sec_to_nsec,
                               interval=interval_ns / sec_to_nsec)

        count = 3
        if nanoseconds:
            t = time.perf_counter_ns()
        else:
            t = time.perf_counter()
        for i in range(count):
            timeout_margin_ns = interval_ns
            if i == 0:
                timeout_ns = initial_expiration_ns + interval_ns + timeout_margin_ns
            else:
                timeout_ns = interval_ns + timeout_margin_ns

            ready = selector.select(timeout_ns / sec_to_nsec)
            self.assertEqual(len(ready), 1, ready)
            event = ready[0][1]
            self.assertEqual(event, selectors.EVENT_READ)

            self.assertEqual(self.read_count_signaled(fd), 1)

        total_time = initial_expiration_ns + interval_ns * (count - 1)
        if nanoseconds:
            dt = time.perf_counter_ns() - t
            self.assertGreater(dt, total_time - self.CLOCK_RES_NS)
        else:
            dt = time.perf_counter() - t
            self.assertGreater(dt, total_time / sec_to_nsec - self.CLOCK_RES)
        selector.unregister(fd)

    def test_timerfd_poll(self):
        self.check_timerfd_poll(False)

    def test_timerfd_ns_poll(self):
        self.check_timerfd_poll(True)

    def test_timerfd_ns_initval(self):
        one_sec_in_nsec = 10**9
        limit_error = one_sec_in_nsec // 10**3
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        # 1st call
        initial_expiration_ns = 0
        interval_ns = one_sec_in_nsec // 1000
        next_expiration_ns, interval_ns2  = os.timerfd_settime_ns(fd, initial=initial_expiration_ns, interval=interval_ns)
        self.assertEqual(interval_ns2, 0)
        self.assertEqual(next_expiration_ns, 0)

        # 2nd call
        next_expiration_ns, interval_ns2 = os.timerfd_settime_ns(fd, initial=initial_expiration_ns, interval=interval_ns)
        self.assertEqual(interval_ns2, interval_ns)
        self.assertEqual(next_expiration_ns, initial_expiration_ns)

        # timerfd_gettime
        next_expiration_ns, interval_ns2 = os.timerfd_gettime_ns(fd)
        self.assertEqual(interval_ns2, interval_ns)
        self.assertLessEqual(next_expiration_ns, initial_expiration_ns)

        self.assertAlmostEqual(next_expiration_ns, initial_expiration_ns, delta=limit_error)

    def test_timerfd_ns_interval(self):
        one_sec_in_nsec = 10**9
        limit_error = one_sec_in_nsec // 10**3
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        # 1 second
        initial_expiration_ns = one_sec_in_nsec
        # every 0.5 second
        interval_ns = one_sec_in_nsec // 2

        os.timerfd_settime_ns(fd, initial=initial_expiration_ns, interval=interval_ns)

        # timerfd_gettime
        next_expiration_ns, interval_ns2 = os.timerfd_gettime_ns(fd)
        self.assertEqual(interval_ns2, interval_ns)
        self.assertLessEqual(next_expiration_ns, initial_expiration_ns)

        count = 3
        t = time.perf_counter_ns()
        for _ in range(count):
            self.assertEqual(self.read_count_signaled(fd), 1)
        t = time.perf_counter_ns() - t

        total_time_ns = initial_expiration_ns + interval_ns * (count - 1)
        self.assertGreater(t, total_time_ns - self.CLOCK_RES_NS)

        # wait 3.5 time of interval
        time.sleep( (count+0.5) * interval_ns / one_sec_in_nsec)
        self.assertEqual(self.read_count_signaled(fd), count)


    def test_timerfd_ns_TFD_TIMER_ABSTIME(self):
        one_sec_in_nsec = 10**9
        limit_error = one_sec_in_nsec // 10**3
        fd = self.timerfd_create(time.CLOCK_REALTIME)

        now_ns = time.clock_gettime_ns(time.CLOCK_REALTIME)

        # 1 second later from now.
        offset_ns = one_sec_in_nsec
        initial_expiration_ns = now_ns + offset_ns
        # not interval timer
        interval_ns = 0

        os.timerfd_settime_ns(fd, flags=os.TFD_TIMER_ABSTIME, initial=initial_expiration_ns, interval=interval_ns)

        # timerfd_gettime
        # Note: timerfd_gettime returns relative values even if TFD_TIMER_ABSTIME is specified.
        next_expiration_ns, interval_ns2 = os.timerfd_gettime_ns(fd)
        self.assertLess(abs(interval_ns2 - interval_ns),  limit_error)
        self.assertLess(abs(next_expiration_ns - offset_ns),  limit_error)

        t = time.perf_counter_ns()
        count_signaled = self.read_count_signaled(fd)
        t = time.perf_counter_ns() - t
        self.assertEqual(count_signaled, 1)

        self.assertGreater(t, offset_ns - self.CLOCK_RES_NS)

    def test_timerfd_ns_select(self):
        one_sec_in_nsec = 10**9

        fd = self.timerfd_create(time.CLOCK_REALTIME, flags=os.TFD_NONBLOCK)

        rfd, wfd, xfd = select.select([fd], [fd], [fd], 0)
        self.assertEqual((rfd, wfd, xfd), ([], [], []))

        # 0.25 second
        initial_expiration_ns = one_sec_in_nsec // 4
        # every 0.125 second
        interval_ns = one_sec_in_nsec // 8

        os.timerfd_settime_ns(fd, initial=initial_expiration_ns, interval=interval_ns)

        count = 3
        t = time.perf_counter_ns()
        for _ in range(count):
            rfd, wfd, xfd = select.select([fd], [fd], [fd], (initial_expiration_ns + interval_ns) / 1e9 )
            self.assertEqual((rfd, wfd, xfd), ([fd], [], []))
            self.assertEqual(self.read_count_signaled(fd), 1)
        t = time.perf_counter_ns() - t

        total_time_ns = initial_expiration_ns + interval_ns * (count - 1)
        self.assertGreater(t, total_time_ns - self.CLOCK_RES_NS)


if __name__ == "__main__":
    unittest.main()
