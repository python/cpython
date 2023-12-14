import threading
from textwrap import dedent
import unittest
import time

from test.support import import_helper
# Raise SkipTest if subinterpreters not supported.
_channels = import_helper.import_module('_xxinterpchannels')
from test.support import interpreters
from test.support.interpreters import channels
from .utils import _run_output, TestBase


class TestChannels(TestBase):

    def test_create(self):
        r, s = channels.create()
        self.assertIsInstance(r, channels.RecvChannel)
        self.assertIsInstance(s, channels.SendChannel)

    def test_list_all(self):
        self.assertEqual(channels.list_all(), [])
        created = set()
        for _ in range(3):
            ch = channels.create()
            created.add(ch)
        after = set(channels.list_all())
        self.assertEqual(after, created)

    def test_shareable(self):
        rch, sch = channels.create()

        self.assertTrue(
            interpreters.is_shareable(rch))
        self.assertTrue(
            interpreters.is_shareable(sch))

        sch.send_nowait(rch)
        sch.send_nowait(sch)
        rch2 = rch.recv()
        sch2 = rch.recv()

        self.assertEqual(rch2, rch)
        self.assertEqual(sch2, sch)

    def test_is_closed(self):
        rch, sch = channels.create()
        rbefore = rch.is_closed
        sbefore = sch.is_closed
        rch.close()
        rafter = rch.is_closed
        safter = sch.is_closed

        self.assertFalse(rbefore)
        self.assertFalse(sbefore)
        self.assertTrue(rafter)
        self.assertTrue(safter)


class TestRecvChannelAttrs(TestBase):

    def test_id_type(self):
        rch, _ = channels.create()
        self.assertIsInstance(rch.id, _channels.ChannelID)

    def test_custom_id(self):
        rch = channels.RecvChannel(1)
        self.assertEqual(rch.id, 1)

        with self.assertRaises(TypeError):
            channels.RecvChannel('1')

    def test_id_readonly(self):
        rch = channels.RecvChannel(1)
        with self.assertRaises(AttributeError):
            rch.id = 2

    def test_equality(self):
        ch1, _ = channels.create()
        ch2, _ = channels.create()
        self.assertEqual(ch1, ch1)
        self.assertNotEqual(ch1, ch2)


class TestSendChannelAttrs(TestBase):

    def test_id_type(self):
        _, sch = channels.create()
        self.assertIsInstance(sch.id, _channels.ChannelID)

    def test_custom_id(self):
        sch = channels.SendChannel(1)
        self.assertEqual(sch.id, 1)

        with self.assertRaises(TypeError):
            channels.SendChannel('1')

    def test_id_readonly(self):
        sch = channels.SendChannel(1)
        with self.assertRaises(AttributeError):
            sch.id = 2

    def test_equality(self):
        _, ch1 = channels.create()
        _, ch2 = channels.create()
        self.assertEqual(ch1, ch1)
        self.assertNotEqual(ch1, ch2)


class TestSendRecv(TestBase):

    def test_send_recv_main(self):
        r, s = channels.create()
        orig = b'spam'
        s.send_nowait(orig)
        obj = r.recv()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_same_interpreter(self):
        interp = interpreters.create()
        interp.exec_sync(dedent("""
            from test.support.interpreters import channels
            r, s = channels.create()
            orig = b'spam'
            s.send_nowait(orig)
            obj = r.recv()
            assert obj == orig, 'expected: obj == orig'
            assert obj is not orig, 'expected: obj is not orig'
            """))

    @unittest.skip('broken (see BPO-...)')
    def test_send_recv_different_interpreters(self):
        r1, s1 = channels.create()
        r2, s2 = channels.create()
        orig1 = b'spam'
        s1.send_nowait(orig1)
        out = _run_output(
            interpreters.create(),
            dedent(f"""
                obj1 = r.recv()
                assert obj1 == b'spam', 'expected: obj1 == orig1'
                # When going to another interpreter we get a copy.
                assert id(obj1) != {id(orig1)}, 'expected: obj1 is not orig1'
                orig2 = b'eggs'
                print(id(orig2))
                s.send_nowait(orig2)
                """),
            channels=dict(r=r1, s=s2),
            )
        obj2 = r2.recv()

        self.assertEqual(obj2, b'eggs')
        self.assertNotEqual(id(obj2), int(out))

    def test_send_recv_different_threads(self):
        r, s = channels.create()

        def f():
            while True:
                try:
                    obj = r.recv()
                    break
                except channels.ChannelEmptyError:
                    time.sleep(0.1)
            s.send(obj)
        t = threading.Thread(target=f)
        t.start()

        orig = b'spam'
        s.send(orig)
        obj = r.recv()
        t.join()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_nowait_main(self):
        r, s = channels.create()
        orig = b'spam'
        s.send_nowait(orig)
        obj = r.recv_nowait()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_nowait_main_with_default(self):
        r, _ = channels.create()
        obj = r.recv_nowait(None)

        self.assertIsNone(obj)

    def test_send_recv_nowait_same_interpreter(self):
        interp = interpreters.create()
        interp.exec_sync(dedent("""
            from test.support.interpreters import channels
            r, s = channels.create()
            orig = b'spam'
            s.send_nowait(orig)
            obj = r.recv_nowait()
            assert obj == orig, 'expected: obj == orig'
            # When going back to the same interpreter we get the same object.
            assert obj is not orig, 'expected: obj is not orig'
            """))

    @unittest.skip('broken (see BPO-...)')
    def test_send_recv_nowait_different_interpreters(self):
        r1, s1 = channels.create()
        r2, s2 = channels.create()
        orig1 = b'spam'
        s1.send_nowait(orig1)
        out = _run_output(
            interpreters.create(),
            dedent(f"""
                obj1 = r.recv_nowait()
                assert obj1 == b'spam', 'expected: obj1 == orig1'
                # When going to another interpreter we get a copy.
                assert id(obj1) != {id(orig1)}, 'expected: obj1 is not orig1'
                orig2 = b'eggs'
                print(id(orig2))
                s.send_nowait(orig2)
                """),
            channels=dict(r=r1, s=s2),
            )
        obj2 = r2.recv_nowait()

        self.assertEqual(obj2, b'eggs')
        self.assertNotEqual(id(obj2), int(out))

    def test_recv_timeout(self):
        r, _ = channels.create()
        with self.assertRaises(TimeoutError):
            r.recv(timeout=1)

    def test_recv_channel_does_not_exist(self):
        ch = channels.RecvChannel(1_000_000)
        with self.assertRaises(channels.ChannelNotFoundError):
            ch.recv()

    def test_send_channel_does_not_exist(self):
        ch = channels.SendChannel(1_000_000)
        with self.assertRaises(channels.ChannelNotFoundError):
            ch.send(b'spam')

    def test_recv_nowait_channel_does_not_exist(self):
        ch = channels.RecvChannel(1_000_000)
        with self.assertRaises(channels.ChannelNotFoundError):
            ch.recv_nowait()

    def test_send_nowait_channel_does_not_exist(self):
        ch = channels.SendChannel(1_000_000)
        with self.assertRaises(channels.ChannelNotFoundError):
            ch.send_nowait(b'spam')

    def test_recv_nowait_empty(self):
        ch, _ = channels.create()
        with self.assertRaises(channels.ChannelEmptyError):
            ch.recv_nowait()

    def test_recv_nowait_default(self):
        default = object()
        rch, sch = channels.create()
        obj1 = rch.recv_nowait(default)
        sch.send_nowait(None)
        sch.send_nowait(1)
        sch.send_nowait(b'spam')
        sch.send_nowait(b'eggs')
        obj2 = rch.recv_nowait(default)
        obj3 = rch.recv_nowait(default)
        obj4 = rch.recv_nowait()
        obj5 = rch.recv_nowait(default)
        obj6 = rch.recv_nowait(default)

        self.assertIs(obj1, default)
        self.assertIs(obj2, None)
        self.assertEqual(obj3, 1)
        self.assertEqual(obj4, b'spam')
        self.assertEqual(obj5, b'eggs')
        self.assertIs(obj6, default)

    def test_send_buffer(self):
        buf = bytearray(b'spamspamspam')
        obj = None
        rch, sch = channels.create()

        def f():
            nonlocal obj
            while True:
                try:
                    obj = rch.recv()
                    break
                except channels.ChannelEmptyError:
                    time.sleep(0.1)
        t = threading.Thread(target=f)
        t.start()

        sch.send_buffer(buf)
        t.join()

        self.assertIsNot(obj, buf)
        self.assertIsInstance(obj, memoryview)
        self.assertEqual(obj, buf)

        buf[4:8] = b'eggs'
        self.assertEqual(obj, buf)
        obj[4:8] = b'ham.'
        self.assertEqual(obj, buf)

    def test_send_buffer_nowait(self):
        buf = bytearray(b'spamspamspam')
        rch, sch = channels.create()
        sch.send_buffer_nowait(buf)
        obj = rch.recv()

        self.assertIsNot(obj, buf)
        self.assertIsInstance(obj, memoryview)
        self.assertEqual(obj, buf)

        buf[4:8] = b'eggs'
        self.assertEqual(obj, buf)
        obj[4:8] = b'ham.'
        self.assertEqual(obj, buf)


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
