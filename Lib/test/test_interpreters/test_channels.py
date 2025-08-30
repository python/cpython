import importlib
import pickle
import threading
from textwrap import dedent
import unittest
import time

from test.support import import_helper
# Raise SkipTest if subinterpreters not supported.
_channels = import_helper.import_module('_interpchannels')
from concurrent import interpreters
from test.support import channels
from .utils import _run_output, TestBase


class LowLevelTests(TestBase):

    # The behaviors in the low-level module is important in as much
    # as it is exercised by the high-level module.  Therefore the
    # most # important testing happens in the high-level tests.
    # These low-level tests cover corner cases that are not
    # encountered by the high-level module, thus they
    # mostly shouldn't matter as much.

    # Additional tests are found in Lib/test/test__interpchannels.py.
    # XXX Those should be either moved to LowLevelTests or eliminated
    # in favor of high-level tests in this file.

    def test_highlevel_reloaded(self):
        # See gh-115490 (https://github.com/python/cpython/issues/115490).
        importlib.reload(channels)


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
        interp = interpreters.create()
        rch, sch = channels.create()

        self.assertTrue(
            interpreters.is_shareable(rch))
        self.assertTrue(
            interpreters.is_shareable(sch))

        sch.send_nowait(rch)
        sch.send_nowait(sch)
        rch2 = rch.recv()
        sch2 = rch.recv()

        interp.prepare_main(rch=rch, sch=sch)
        sch.send_nowait(rch)
        sch.send_nowait(sch)
        interp.exec(dedent("""
            rch2 = rch.recv()
            sch2 = rch.recv()
            assert rch2 == rch
            assert sch2 == sch

            sch.send_nowait(rch2)
            sch.send_nowait(sch2)
            """))
        rch3 = rch.recv()
        sch3 = rch.recv()

        self.assertEqual(rch2, rch)
        self.assertEqual(sch2, sch)
        self.assertEqual(rch3, rch)
        self.assertEqual(sch3, sch)

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

    def test_pickle(self):
        ch, _ = channels.create()
        for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(protocol=protocol):
                data = pickle.dumps(ch, protocol)
                unpickled = pickle.loads(data)
                self.assertEqual(unpickled, ch)


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

    def test_pickle(self):
        _, ch = channels.create()
        for protocol in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(protocol=protocol):
                data = pickle.dumps(ch, protocol)
                unpickled = pickle.loads(data)
                self.assertEqual(unpickled, ch)


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
        interp.exec(dedent("""
            from test.support import channels
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
        interp.exec(dedent("""
            from test.support import channels
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

    def test_send_cleared_with_subinterpreter(self):
        def common(rch, sch, unbound=None, presize=0):
            if not unbound:
                extraargs = ''
            elif unbound is channels.UNBOUND:
                extraargs = ', unbounditems=channels.UNBOUND'
            elif unbound is channels.UNBOUND_ERROR:
                extraargs = ', unbounditems=channels.UNBOUND_ERROR'
            elif unbound is channels.UNBOUND_REMOVE:
                extraargs = ', unbounditems=channels.UNBOUND_REMOVE'
            else:
                raise NotImplementedError(repr(unbound))
            interp = interpreters.create()

            _run_output(interp, dedent(f"""
                from test.support import channels
                sch = channels.SendChannel({sch.id})
                obj1 = b'spam'
                obj2 = b'eggs'
                sch.send_nowait(obj1{extraargs})
                sch.send_nowait(obj2{extraargs})
                """))
            self.assertEqual(
                _channels.get_count(rch.id),
                presize + 2,
            )

            if presize == 0:
                obj1 = rch.recv()
                self.assertEqual(obj1, b'spam')
                self.assertEqual(
                    _channels.get_count(rch.id),
                    presize + 1,
                )

            return interp

        with self.subTest('default'):  # UNBOUND
            rch, sch = channels.create()
            interp = common(rch, sch)
            del interp
            self.assertEqual(_channels.get_count(rch.id), 1)
            obj1 = rch.recv()
            self.assertEqual(_channels.get_count(rch.id), 0)
            self.assertIs(obj1, channels.UNBOUND)
            self.assertEqual(_channels.get_count(rch.id), 0)
            with self.assertRaises(channels.ChannelEmptyError):
                rch.recv_nowait()

        with self.subTest('UNBOUND'):
            rch, sch = channels.create()
            interp = common(rch, sch, channels.UNBOUND)
            del interp
            self.assertEqual(_channels.get_count(rch.id), 1)
            obj1 = rch.recv()
            self.assertIs(obj1, channels.UNBOUND)
            self.assertEqual(_channels.get_count(rch.id), 0)
            with self.assertRaises(channels.ChannelEmptyError):
                rch.recv_nowait()

        with self.subTest('UNBOUND_ERROR'):
            rch, sch = channels.create()
            interp = common(rch, sch, channels.UNBOUND_ERROR)

            del interp
            self.assertEqual(_channels.get_count(rch.id), 1)
            with self.assertRaises(channels.ItemInterpreterDestroyed):
                rch.recv()

            self.assertEqual(_channels.get_count(rch.id), 0)
            with self.assertRaises(channels.ChannelEmptyError):
                rch.recv_nowait()

        with self.subTest('UNBOUND_REMOVE'):
            rch, sch = channels.create()

            interp = common(rch, sch, channels.UNBOUND_REMOVE)
            del interp
            self.assertEqual(_channels.get_count(rch.id), 0)
            with self.assertRaises(channels.ChannelEmptyError):
                rch.recv_nowait()

            sch.send_nowait(b'ham', unbounditems=channels.UNBOUND_REMOVE)
            self.assertEqual(_channels.get_count(rch.id), 1)
            interp = common(rch, sch, channels.UNBOUND_REMOVE, 1)
            self.assertEqual(_channels.get_count(rch.id), 3)
            sch.send_nowait(42, unbounditems=channels.UNBOUND_REMOVE)
            self.assertEqual(_channels.get_count(rch.id), 4)
            del interp
            self.assertEqual(_channels.get_count(rch.id), 2)
            obj1 = rch.recv()
            obj2 = rch.recv()
            self.assertEqual(obj1, b'ham')
            self.assertEqual(obj2, 42)
            self.assertEqual(_channels.get_count(rch.id), 0)
            with self.assertRaises(channels.ChannelEmptyError):
                rch.recv_nowait()

    def test_send_cleared_with_subinterpreter_mixed(self):
        rch, sch = channels.create()
        interp = interpreters.create()

        # If we don't associate the main interpreter with the channel
        # then the channel will be automatically closed when interp
        # is destroyed.
        sch.send_nowait(None)
        rch.recv()
        self.assertEqual(_channels.get_count(rch.id), 0)

        _run_output(interp, dedent(f"""
            from test.support import channels
            sch = channels.SendChannel({sch.id})
            sch.send_nowait(1, unbounditems=channels.UNBOUND)
            sch.send_nowait(2, unbounditems=channels.UNBOUND_ERROR)
            sch.send_nowait(3)
            sch.send_nowait(4, unbounditems=channels.UNBOUND_REMOVE)
            sch.send_nowait(5, unbounditems=channels.UNBOUND)
            """))
        self.assertEqual(_channels.get_count(rch.id), 5)

        del interp
        self.assertEqual(_channels.get_count(rch.id), 4)

        obj1 = rch.recv()
        self.assertIs(obj1, channels.UNBOUND)
        self.assertEqual(_channels.get_count(rch.id), 3)

        with self.assertRaises(channels.ItemInterpreterDestroyed):
            rch.recv()
        self.assertEqual(_channels.get_count(rch.id), 2)

        obj2 = rch.recv()
        self.assertIs(obj2, channels.UNBOUND)
        self.assertEqual(_channels.get_count(rch.id), 1)

        obj3 = rch.recv()
        self.assertIs(obj3, channels.UNBOUND)
        self.assertEqual(_channels.get_count(rch.id), 0)

    def test_send_cleared_with_subinterpreter_multiple(self):
        rch, sch = channels.create()
        interp1 = interpreters.create()
        interp2 = interpreters.create()

        sch.send_nowait(1)
        _run_output(interp1, dedent(f"""
            from test.support import channels
            rch = channels.RecvChannel({rch.id})
            sch = channels.SendChannel({sch.id})
            obj1 = rch.recv()
            sch.send_nowait(2, unbounditems=channels.UNBOUND)
            sch.send_nowait(obj1, unbounditems=channels.UNBOUND_REMOVE)
            """))
        _run_output(interp2, dedent(f"""
            from test.support import channels
            rch = channels.RecvChannel({rch.id})
            sch = channels.SendChannel({sch.id})
            obj2 = rch.recv()
            obj1 = rch.recv()
            """))
        self.assertEqual(_channels.get_count(rch.id), 0)
        sch.send_nowait(3)
        _run_output(interp1, dedent("""
            sch.send_nowait(4, unbounditems=channels.UNBOUND)
            # interp closed here
            sch.send_nowait(5, unbounditems=channels.UNBOUND_REMOVE)
            sch.send_nowait(6, unbounditems=channels.UNBOUND)
            """))
        _run_output(interp2, dedent("""
            sch.send_nowait(7, unbounditems=channels.UNBOUND_ERROR)
            # interp closed here
            sch.send_nowait(obj1, unbounditems=channels.UNBOUND_ERROR)
            sch.send_nowait(obj2, unbounditems=channels.UNBOUND_REMOVE)
            sch.send_nowait(8, unbounditems=channels.UNBOUND)
            """))
        _run_output(interp1, dedent("""
            sch.send_nowait(9, unbounditems=channels.UNBOUND_REMOVE)
            sch.send_nowait(10, unbounditems=channels.UNBOUND)
            """))
        self.assertEqual(_channels.get_count(rch.id), 10)

        obj3 = rch.recv()
        self.assertEqual(obj3, 3)
        self.assertEqual(_channels.get_count(rch.id), 9)

        obj4 = rch.recv()
        self.assertEqual(obj4, 4)
        self.assertEqual(_channels.get_count(rch.id), 8)

        del interp1
        self.assertEqual(_channels.get_count(rch.id), 6)

        # obj5 was removed

        obj6 = rch.recv()
        self.assertIs(obj6, channels.UNBOUND)
        self.assertEqual(_channels.get_count(rch.id), 5)

        obj7 = rch.recv()
        self.assertEqual(obj7, 7)
        self.assertEqual(_channels.get_count(rch.id), 4)

        del interp2
        self.assertEqual(_channels.get_count(rch.id), 3)

        # obj1
        with self.assertRaises(channels.ItemInterpreterDestroyed):
            rch.recv()
        self.assertEqual(_channels.get_count(rch.id), 2)

        # obj2 was removed

        obj8 = rch.recv()
        self.assertIs(obj8, channels.UNBOUND)
        self.assertEqual(_channels.get_count(rch.id), 1)

        # obj9 was removed

        obj10 = rch.recv()
        self.assertIs(obj10, channels.UNBOUND)
        self.assertEqual(_channels.get_count(rch.id), 0)


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
