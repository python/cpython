from collections import namedtuple
import contextlib
import sys
from textwrap import dedent
import threading
import time
import unittest

from test.support import import_helper

from test.test__xxsubinterpreters import (
    interpreters,
    _run_output,
    clean_up_interpreters,
)


channels = import_helper.import_module('_xxinterpchannels')


##################################
# helpers

def recv_wait(cid):
    while True:
        try:
            return channels.recv(cid)
        except channels.ChannelEmptyError:
            time.sleep(0.1)

#@contextmanager
#def run_threaded(id, source, **shared):
#    def run():
#        run_interp(id, source, **shared)
#    t = threading.Thread(target=run)
#    t.start()
#    yield
#    t.join()


def run_interp(id, source, **shared):
    _run_interp(id, source, shared)


def _run_interp(id, source, shared, _mainns={}):
    source = dedent(source)
    main = interpreters.get_main()
    if main == id:
        if interpreters.get_current() != main:
            raise RuntimeError
        # XXX Run a func?
        exec(source, _mainns)
    else:
        interpreters.run_string(id, source, shared)


class Interpreter(namedtuple('Interpreter', 'name id')):

    @classmethod
    def from_raw(cls, raw):
        if isinstance(raw, cls):
            return raw
        elif isinstance(raw, str):
            return cls(raw)
        else:
            raise NotImplementedError

    def __new__(cls, name=None, id=None):
        main = interpreters.get_main()
        if id == main:
            if not name:
                name = 'main'
            elif name != 'main':
                raise ValueError(
                    'name mismatch (expected "main", got "{}")'.format(name))
            id = main
        elif id is not None:
            if not name:
                name = 'interp'
            elif name == 'main':
                raise ValueError('name mismatch (unexpected "main")')
            assert isinstance(id, int), repr(id)
        elif not name or name == 'main':
            name = 'main'
            id = main
        else:
            id = interpreters.create()
        self = super().__new__(cls, name, id)
        return self


# XXX expect_channel_closed() is unnecessary once we improve exc propagation.

@contextlib.contextmanager
def expect_channel_closed():
    try:
        yield
    except channels.ChannelClosedError:
        pass
    else:
        assert False, 'channel not closed'


class ChannelAction(namedtuple('ChannelAction', 'action end interp')):

    def __new__(cls, action, end=None, interp=None):
        if not end:
            end = 'both'
        if not interp:
            interp = 'main'
        self = super().__new__(cls, action, end, interp)
        return self

    def __init__(self, *args, **kwargs):
        if self.action == 'use':
            if self.end not in ('same', 'opposite', 'send', 'recv'):
                raise ValueError(self.end)
        elif self.action in ('close', 'force-close'):
            if self.end not in ('both', 'same', 'opposite', 'send', 'recv'):
                raise ValueError(self.end)
        else:
            raise ValueError(self.action)
        if self.interp not in ('main', 'same', 'other', 'extra'):
            raise ValueError(self.interp)

    def resolve_end(self, end):
        if self.end == 'same':
            return end
        elif self.end == 'opposite':
            return 'recv' if end == 'send' else 'send'
        else:
            return self.end

    def resolve_interp(self, interp, other, extra):
        if self.interp == 'same':
            return interp
        elif self.interp == 'other':
            if other is None:
                raise RuntimeError
            return other
        elif self.interp == 'extra':
            if extra is None:
                raise RuntimeError
            return extra
        elif self.interp == 'main':
            if interp.name == 'main':
                return interp
            elif other and other.name == 'main':
                return other
            else:
                raise RuntimeError
        # Per __init__(), there aren't any others.


class ChannelState(namedtuple('ChannelState', 'pending closed')):

    def __new__(cls, pending=0, *, closed=False):
        self = super().__new__(cls, pending, closed)
        return self

    def incr(self):
        return type(self)(self.pending + 1, closed=self.closed)

    def decr(self):
        return type(self)(self.pending - 1, closed=self.closed)

    def close(self, *, force=True):
        if self.closed:
            if not force or self.pending == 0:
                return self
        return type(self)(0 if force else self.pending, closed=True)


def run_action(cid, action, end, state, *, hideclosed=True):
    if state.closed:
        if action == 'use' and end == 'recv' and state.pending:
            expectfail = False
        else:
            expectfail = True
    else:
        expectfail = False

    try:
        result = _run_action(cid, action, end, state)
    except channels.ChannelClosedError:
        if not hideclosed and not expectfail:
            raise
        result = state.close()
    else:
        if expectfail:
            raise ...  # XXX
    return result


def _run_action(cid, action, end, state):
    if action == 'use':
        if end == 'send':
            channels.send(cid, b'spam', blocking=False)
            return state.incr()
        elif end == 'recv':
            if not state.pending:
                try:
                    channels.recv(cid)
                except channels.ChannelEmptyError:
                    return state
                else:
                    raise Exception('expected ChannelEmptyError')
            else:
                channels.recv(cid)
                return state.decr()
        else:
            raise ValueError(end)
    elif action == 'close':
        kwargs = {}
        if end in ('recv', 'send'):
            kwargs[end] = True
        channels.close(cid, **kwargs)
        return state.close()
    elif action == 'force-close':
        kwargs = {
            'force': True,
            }
        if end in ('recv', 'send'):
            kwargs[end] = True
        channels.close(cid, **kwargs)
        return state.close(force=True)
    else:
        raise ValueError(action)


def clean_up_channels():
    for cid in channels.list_all():
        try:
            channels.destroy(cid)
        except channels.ChannelNotFoundError:
            pass  # already destroyed


class TestBase(unittest.TestCase):

    def tearDown(self):
        clean_up_channels()
        clean_up_interpreters()


##################################
# channel tests

class ChannelIDTests(TestBase):

    def test_default_kwargs(self):
        cid = channels._channel_id(10, force=True)

        self.assertEqual(int(cid), 10)
        self.assertEqual(cid.end, 'both')

    def test_with_kwargs(self):
        cid = channels._channel_id(10, send=True, force=True)
        self.assertEqual(cid.end, 'send')

        cid = channels._channel_id(10, send=True, recv=False, force=True)
        self.assertEqual(cid.end, 'send')

        cid = channels._channel_id(10, recv=True, force=True)
        self.assertEqual(cid.end, 'recv')

        cid = channels._channel_id(10, recv=True, send=False, force=True)
        self.assertEqual(cid.end, 'recv')

        cid = channels._channel_id(10, send=True, recv=True, force=True)
        self.assertEqual(cid.end, 'both')

    def test_coerce_id(self):
        class Int(str):
            def __index__(self):
                return 10

        cid = channels._channel_id(Int(), force=True)
        self.assertEqual(int(cid), 10)

    def test_bad_id(self):
        self.assertRaises(TypeError, channels._channel_id, object())
        self.assertRaises(TypeError, channels._channel_id, 10.0)
        self.assertRaises(TypeError, channels._channel_id, '10')
        self.assertRaises(TypeError, channels._channel_id, b'10')
        self.assertRaises(ValueError, channels._channel_id, -1)
        self.assertRaises(OverflowError, channels._channel_id, 2**64)

    def test_bad_kwargs(self):
        with self.assertRaises(ValueError):
            channels._channel_id(10, send=False, recv=False)

    def test_does_not_exist(self):
        cid = channels.create()
        with self.assertRaises(channels.ChannelNotFoundError):
            channels._channel_id(int(cid) + 1)  # unforced

    def test_str(self):
        cid = channels._channel_id(10, force=True)
        self.assertEqual(str(cid), '10')

    def test_repr(self):
        cid = channels._channel_id(10, force=True)
        self.assertEqual(repr(cid), 'ChannelID(10)')

        cid = channels._channel_id(10, send=True, force=True)
        self.assertEqual(repr(cid), 'ChannelID(10, send=True)')

        cid = channels._channel_id(10, recv=True, force=True)
        self.assertEqual(repr(cid), 'ChannelID(10, recv=True)')

        cid = channels._channel_id(10, send=True, recv=True, force=True)
        self.assertEqual(repr(cid), 'ChannelID(10)')

    def test_equality(self):
        cid1 = channels.create()
        cid2 = channels._channel_id(int(cid1))
        cid3 = channels.create()

        self.assertTrue(cid1 == cid1)
        self.assertTrue(cid1 == cid2)
        self.assertTrue(cid1 == int(cid1))
        self.assertTrue(int(cid1) == cid1)
        self.assertTrue(cid1 == float(int(cid1)))
        self.assertTrue(float(int(cid1)) == cid1)
        self.assertFalse(cid1 == float(int(cid1)) + 0.1)
        self.assertFalse(cid1 == str(int(cid1)))
        self.assertFalse(cid1 == 2**1000)
        self.assertFalse(cid1 == float('inf'))
        self.assertFalse(cid1 == 'spam')
        self.assertFalse(cid1 == cid3)

        self.assertFalse(cid1 != cid1)
        self.assertFalse(cid1 != cid2)
        self.assertTrue(cid1 != cid3)

    def test_shareable(self):
        chan = channels.create()

        obj = channels.create()
        channels.send(chan, obj, blocking=False)
        got = channels.recv(chan)

        self.assertEqual(got, obj)
        self.assertIs(type(got), type(obj))
        # XXX Check the following in the channel tests?
        #self.assertIsNot(got, obj)


class ChannelTests(TestBase):

    def test_create_cid(self):
        cid = channels.create()
        self.assertIsInstance(cid, channels.ChannelID)

    def test_sequential_ids(self):
        before = channels.list_all()
        id1 = channels.create()
        id2 = channels.create()
        id3 = channels.create()
        after = channels.list_all()

        self.assertEqual(id2, int(id1) + 1)
        self.assertEqual(id3, int(id2) + 1)
        self.assertEqual(set(after) - set(before), {id1, id2, id3})

    def test_ids_global(self):
        id1 = interpreters.create()
        out = _run_output(id1, dedent("""
            import _xxinterpchannels as _channels
            cid = _channels.create()
            print(cid)
            """))
        cid1 = int(out.strip())

        id2 = interpreters.create()
        out = _run_output(id2, dedent("""
            import _xxinterpchannels as _channels
            cid = _channels.create()
            print(cid)
            """))
        cid2 = int(out.strip())

        self.assertEqual(cid2, int(cid1) + 1)

    def test_channel_list_interpreters_none(self):
        """Test listing interpreters for a channel with no associations."""
        # Test for channel with no associated interpreters.
        cid = channels.create()
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(send_interps, [])
        self.assertEqual(recv_interps, [])

    def test_channel_list_interpreters_basic(self):
        """Test basic listing channel interpreters."""
        interp0 = interpreters.get_main()
        cid = channels.create()
        channels.send(cid, "send", blocking=False)
        # Test for a channel that has one end associated to an interpreter.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(send_interps, [interp0])
        self.assertEqual(recv_interps, [])

        interp1 = interpreters.create()
        _run_output(interp1, dedent(f"""
            import _xxinterpchannels as _channels
            obj = _channels.recv({cid})
            """))
        # Test for channel that has both ends associated to an interpreter.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(send_interps, [interp0])
        self.assertEqual(recv_interps, [interp1])

    def test_channel_list_interpreters_multiple(self):
        """Test listing interpreters for a channel with many associations."""
        interp0 = interpreters.get_main()
        interp1 = interpreters.create()
        interp2 = interpreters.create()
        interp3 = interpreters.create()
        cid = channels.create()

        channels.send(cid, "send", blocking=False)
        _run_output(interp1, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.send({cid}, "send", blocking=False)
            """))
        _run_output(interp2, dedent(f"""
            import _xxinterpchannels as _channels
            obj = _channels.recv({cid})
            """))
        _run_output(interp3, dedent(f"""
            import _xxinterpchannels as _channels
            obj = _channels.recv({cid})
            """))
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(set(send_interps), {interp0, interp1})
        self.assertEqual(set(recv_interps), {interp2, interp3})

    def test_channel_list_interpreters_destroyed(self):
        """Test listing channel interpreters with a destroyed interpreter."""
        interp0 = interpreters.get_main()
        interp1 = interpreters.create()
        cid = channels.create()
        channels.send(cid, "send", blocking=False)
        _run_output(interp1, dedent(f"""
            import _xxinterpchannels as _channels
            obj = _channels.recv({cid})
            """))
        # Should be one interpreter associated with each end.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(send_interps, [interp0])
        self.assertEqual(recv_interps, [interp1])

        interpreters.destroy(interp1)
        # Destroyed interpreter should not be listed.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(send_interps, [interp0])
        self.assertEqual(recv_interps, [])

    def test_channel_list_interpreters_released(self):
        """Test listing channel interpreters with a released channel."""
        # Set up one channel with main interpreter on the send end and two
        # subinterpreters on the receive end.
        interp0 = interpreters.get_main()
        interp1 = interpreters.create()
        interp2 = interpreters.create()
        cid = channels.create()
        channels.send(cid, "data", blocking=False)
        _run_output(interp1, dedent(f"""
            import _xxinterpchannels as _channels
            obj = _channels.recv({cid})
            """))
        channels.send(cid, "data", blocking=False)
        _run_output(interp2, dedent(f"""
            import _xxinterpchannels as _channels
            obj = _channels.recv({cid})
            """))
        # Check the setup.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(len(send_interps), 1)
        self.assertEqual(len(recv_interps), 2)

        # Release the main interpreter from the send end.
        channels.release(cid, send=True)
        # Send end should have no associated interpreters.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(len(send_interps), 0)
        self.assertEqual(len(recv_interps), 2)

        # Release one of the subinterpreters from the receive end.
        _run_output(interp2, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.release({cid})
            """))
        # Receive end should have the released interpreter removed.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(len(send_interps), 0)
        self.assertEqual(recv_interps, [interp1])

    def test_channel_list_interpreters_closed(self):
        """Test listing channel interpreters with a closed channel."""
        interp0 = interpreters.get_main()
        interp1 = interpreters.create()
        cid = channels.create()
        # Put something in the channel so that it's not empty.
        channels.send(cid, "send", blocking=False)

        # Check initial state.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(len(send_interps), 1)
        self.assertEqual(len(recv_interps), 0)

        # Force close the channel.
        channels.close(cid, force=True)
        # Both ends should raise an error.
        with self.assertRaises(channels.ChannelClosedError):
            channels.list_interpreters(cid, send=True)
        with self.assertRaises(channels.ChannelClosedError):
            channels.list_interpreters(cid, send=False)

    def test_channel_list_interpreters_closed_send_end(self):
        """Test listing channel interpreters with a channel's send end closed."""
        interp0 = interpreters.get_main()
        interp1 = interpreters.create()
        cid = channels.create()
        # Put something in the channel so that it's not empty.
        channels.send(cid, "send", blocking=False)

        # Check initial state.
        send_interps = channels.list_interpreters(cid, send=True)
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(len(send_interps), 1)
        self.assertEqual(len(recv_interps), 0)

        # Close the send end of the channel.
        channels.close(cid, send=True)
        # Send end should raise an error.
        with self.assertRaises(channels.ChannelClosedError):
            channels.list_interpreters(cid, send=True)
        # Receive end should not be closed (since channel is not empty).
        recv_interps = channels.list_interpreters(cid, send=False)
        self.assertEqual(len(recv_interps), 0)

        # Close the receive end of the channel from a subinterpreter.
        _run_output(interp1, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.close({cid}, force=True)
            """))
        return
        # Both ends should raise an error.
        with self.assertRaises(channels.ChannelClosedError):
            channels.list_interpreters(cid, send=True)
        with self.assertRaises(channels.ChannelClosedError):
            channels.list_interpreters(cid, send=False)

    def test_allowed_types(self):
        cid = channels.create()
        objects = [
            None,
            'spam',
            b'spam',
            42,
        ]
        for obj in objects:
            with self.subTest(obj):
                channels.send(cid, obj, blocking=False)
                got = channels.recv(cid)

                self.assertEqual(got, obj)
                self.assertIs(type(got), type(obj))
                # XXX Check the following?
                #self.assertIsNot(got, obj)
                # XXX What about between interpreters?

    def test_run_string_arg_unresolved(self):
        cid = channels.create()
        interp = interpreters.create()

        interpreters.set___main___attrs(interp, dict(cid=cid.send))
        out = _run_output(interp, dedent("""
            import _xxinterpchannels as _channels
            print(cid.end)
            _channels.send(cid, b'spam', blocking=False)
            """))
        obj = channels.recv(cid)

        self.assertEqual(obj, b'spam')
        self.assertEqual(out.strip(), 'send')

    # XXX For now there is no high-level channel into which the
    # sent channel ID can be converted...
    # Note: this test caused crashes on some buildbots (bpo-33615).
    @unittest.skip('disabled until high-level channels exist')
    def test_run_string_arg_resolved(self):
        cid = channels.create()
        cid = channels._channel_id(cid, _resolve=True)
        interp = interpreters.create()

        out = _run_output(interp, dedent("""
            import _xxinterpchannels as _channels
            print(chan.id.end)
            _channels.send(chan.id, b'spam', blocking=False)
            """),
            dict(chan=cid.send))
        obj = channels.recv(cid)

        self.assertEqual(obj, b'spam')
        self.assertEqual(out.strip(), 'send')

    #-------------------
    # send/recv

    def test_send_recv_main(self):
        cid = channels.create()
        orig = b'spam'
        channels.send(cid, orig, blocking=False)
        obj = channels.recv(cid)

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_same_interpreter(self):
        id1 = interpreters.create()
        out = _run_output(id1, dedent("""
            import _xxinterpchannels as _channels
            cid = _channels.create()
            orig = b'spam'
            _channels.send(cid, orig, blocking=False)
            obj = _channels.recv(cid)
            assert obj is not orig
            assert obj == orig
            """))

    def test_send_recv_different_interpreters(self):
        cid = channels.create()
        id1 = interpreters.create()
        out = _run_output(id1, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.send({cid}, b'spam', blocking=False)
            """))
        obj = channels.recv(cid)

        self.assertEqual(obj, b'spam')

    def test_send_recv_different_threads(self):
        cid = channels.create()

        def f():
            obj = recv_wait(cid)
            channels.send(cid, obj)
        t = threading.Thread(target=f)
        t.start()

        channels.send(cid, b'spam')
        obj = recv_wait(cid)
        t.join()

        self.assertEqual(obj, b'spam')

    def test_send_recv_different_interpreters_and_threads(self):
        cid = channels.create()
        id1 = interpreters.create()
        out = None

        def f():
            nonlocal out
            out = _run_output(id1, dedent(f"""
                import time
                import _xxinterpchannels as _channels
                while True:
                    try:
                        obj = _channels.recv({cid})
                        break
                    except _channels.ChannelEmptyError:
                        time.sleep(0.1)
                assert(obj == b'spam')
                _channels.send({cid}, b'eggs')
                """))
        t = threading.Thread(target=f)
        t.start()

        channels.send(cid, b'spam')
        obj = recv_wait(cid)
        t.join()

        self.assertEqual(obj, b'eggs')

    def test_send_not_found(self):
        with self.assertRaises(channels.ChannelNotFoundError):
            channels.send(10, b'spam')

    def test_recv_not_found(self):
        with self.assertRaises(channels.ChannelNotFoundError):
            channels.recv(10)

    def test_recv_empty(self):
        cid = channels.create()
        with self.assertRaises(channels.ChannelEmptyError):
            channels.recv(cid)

    def test_recv_default(self):
        default = object()
        cid = channels.create()
        obj1 = channels.recv(cid, default)
        channels.send(cid, None, blocking=False)
        channels.send(cid, 1, blocking=False)
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'eggs', blocking=False)
        obj2 = channels.recv(cid, default)
        obj3 = channels.recv(cid, default)
        obj4 = channels.recv(cid)
        obj5 = channels.recv(cid, default)
        obj6 = channels.recv(cid, default)

        self.assertIs(obj1, default)
        self.assertIs(obj2, None)
        self.assertEqual(obj3, 1)
        self.assertEqual(obj4, b'spam')
        self.assertEqual(obj5, b'eggs')
        self.assertIs(obj6, default)

    def test_recv_sending_interp_destroyed(self):
        with self.subTest('closed'):
            cid1 = channels.create()
            interp = interpreters.create()
            interpreters.run_string(interp, dedent(f"""
                import _xxinterpchannels as _channels
                _channels.send({cid1}, b'spam', blocking=False)
                """))
            interpreters.destroy(interp)

            with self.assertRaisesRegex(RuntimeError,
                                        f'channel {cid1} is closed'):
                channels.recv(cid1)
            del cid1
        with self.subTest('still open'):
            cid2 = channels.create()
            interp = interpreters.create()
            interpreters.run_string(interp, dedent(f"""
                import _xxinterpchannels as _channels
                _channels.send({cid2}, b'spam', blocking=False)
                """))
            channels.send(cid2, b'eggs', blocking=False)
            interpreters.destroy(interp)

            channels.recv(cid2)
            with self.assertRaisesRegex(RuntimeError,
                                        f'channel {cid2} is empty'):
                channels.recv(cid2)
            del cid2

    #-------------------
    # send_buffer

    def test_send_buffer(self):
        buf = bytearray(b'spamspamspam')
        cid = channels.create()
        channels.send_buffer(cid, buf, blocking=False)
        obj = channels.recv(cid)

        self.assertIsNot(obj, buf)
        self.assertIsInstance(obj, memoryview)
        self.assertEqual(obj, buf)

        buf[4:8] = b'eggs'
        self.assertEqual(obj, buf)
        obj[4:8] = b'ham.'
        self.assertEqual(obj, buf)

    #-------------------
    # send with waiting

    def build_send_waiter(self, obj, *, buffer=False):
        # We want a long enough sleep that send() actually has to wait.

        if buffer:
            send = channels.send_buffer
        else:
            send = channels.send

        cid = channels.create()
        try:
            started = time.monotonic()
            send(cid, obj, blocking=False)
            stopped = time.monotonic()
            channels.recv(cid)
        finally:
            channels.destroy(cid)
        delay = stopped - started  # seconds
        delay *= 3

        def wait():
            time.sleep(delay)
        return wait

    def test_send_blocking_waiting(self):
        received = None
        obj = b'spam'
        wait = self.build_send_waiter(obj)
        cid = channels.create()
        def f():
            nonlocal received
            wait()
            received = recv_wait(cid)
        t = threading.Thread(target=f)
        t.start()
        channels.send(cid, obj, blocking=True)
        t.join()

        self.assertEqual(received, obj)

    def test_send_buffer_blocking_waiting(self):
        received = None
        obj = bytearray(b'spam')
        wait = self.build_send_waiter(obj, buffer=True)
        cid = channels.create()
        def f():
            nonlocal received
            wait()
            received = recv_wait(cid)
        t = threading.Thread(target=f)
        t.start()
        channels.send_buffer(cid, obj, blocking=True)
        t.join()

        self.assertEqual(received, obj)

    def test_send_blocking_no_wait(self):
        received = None
        obj = b'spam'
        cid = channels.create()
        def f():
            nonlocal received
            received = recv_wait(cid)
        t = threading.Thread(target=f)
        t.start()
        channels.send(cid, obj, blocking=True)
        t.join()

        self.assertEqual(received, obj)

    def test_send_buffer_blocking_no_wait(self):
        received = None
        obj = bytearray(b'spam')
        cid = channels.create()
        def f():
            nonlocal received
            received = recv_wait(cid)
        t = threading.Thread(target=f)
        t.start()
        channels.send_buffer(cid, obj, blocking=True)
        t.join()

        self.assertEqual(received, obj)

    def test_send_timeout(self):
        obj = b'spam'

        with self.subTest('non-blocking with timeout'):
            cid = channels.create()
            with self.assertRaises(ValueError):
                channels.send(cid, obj, blocking=False, timeout=0.1)

        with self.subTest('timeout hit'):
            cid = channels.create()
            with self.assertRaises(TimeoutError):
                channels.send(cid, obj, blocking=True, timeout=0.1)
            with self.assertRaises(channels.ChannelEmptyError):
                received = channels.recv(cid)
                print(repr(received))

        with self.subTest('timeout not hit'):
            cid = channels.create()
            def f():
                recv_wait(cid)
            t = threading.Thread(target=f)
            t.start()
            channels.send(cid, obj, blocking=True, timeout=10)
            t.join()

    def test_send_buffer_timeout(self):
        try:
            self._has_run_once_timeout
        except AttributeError:
            # At the moment, this test leaks a few references.
            # It looks like the leak originates with the addition
            # of _channels.send_buffer() (gh-110246), whereas the
            # tests were added afterward.  We want this test even
            # if the refleak isn't fixed yet, so we skip here.
            raise unittest.SkipTest('temporarily skipped due to refleaks')
        else:
            self._has_run_once_timeout = True

        obj = bytearray(b'spam')

        with self.subTest('non-blocking with timeout'):
            cid = channels.create()
            with self.assertRaises(ValueError):
                channels.send_buffer(cid, obj, blocking=False, timeout=0.1)

        with self.subTest('timeout hit'):
            cid = channels.create()
            with self.assertRaises(TimeoutError):
                channels.send_buffer(cid, obj, blocking=True, timeout=0.1)
            with self.assertRaises(channels.ChannelEmptyError):
                received = channels.recv(cid)
                print(repr(received))

        with self.subTest('timeout not hit'):
            cid = channels.create()
            def f():
                recv_wait(cid)
            t = threading.Thread(target=f)
            t.start()
            channels.send_buffer(cid, obj, blocking=True, timeout=10)
            t.join()

    def test_send_closed_while_waiting(self):
        obj = b'spam'
        wait = self.build_send_waiter(obj)

        with self.subTest('without timeout'):
            cid = channels.create()
            def f():
                wait()
                channels.close(cid, force=True)
            t = threading.Thread(target=f)
            t.start()
            with self.assertRaises(channels.ChannelClosedError):
                channels.send(cid, obj, blocking=True)
            t.join()

        with self.subTest('with timeout'):
            cid = channels.create()
            def f():
                wait()
                channels.close(cid, force=True)
            t = threading.Thread(target=f)
            t.start()
            with self.assertRaises(channels.ChannelClosedError):
                channels.send(cid, obj, blocking=True, timeout=30)
            t.join()

    def test_send_buffer_closed_while_waiting(self):
        try:
            self._has_run_once_closed
        except AttributeError:
            # At the moment, this test leaks a few references.
            # It looks like the leak originates with the addition
            # of _channels.send_buffer() (gh-110246), whereas the
            # tests were added afterward.  We want this test even
            # if the refleak isn't fixed yet, so we skip here.
            raise unittest.SkipTest('temporarily skipped due to refleaks')
        else:
            self._has_run_once_closed = True

        obj = bytearray(b'spam')
        wait = self.build_send_waiter(obj, buffer=True)

        with self.subTest('without timeout'):
            cid = channels.create()
            def f():
                wait()
                channels.close(cid, force=True)
            t = threading.Thread(target=f)
            t.start()
            with self.assertRaises(channels.ChannelClosedError):
                channels.send_buffer(cid, obj, blocking=True)
            t.join()

        with self.subTest('with timeout'):
            cid = channels.create()
            def f():
                wait()
                channels.close(cid, force=True)
            t = threading.Thread(target=f)
            t.start()
            with self.assertRaises(channels.ChannelClosedError):
                channels.send_buffer(cid, obj, blocking=True, timeout=30)
            t.join()

    #-------------------
    # close

    def test_close_single_user(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.recv(cid)
        channels.close(cid)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_close_multiple_users(self):
        cid = channels.create()
        id1 = interpreters.create()
        id2 = interpreters.create()
        interpreters.run_string(id1, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.send({cid}, b'spam', blocking=False)
            """))
        interpreters.run_string(id2, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.recv({cid})
            """))
        channels.close(cid)

        excsnap = interpreters.run_string(id1, dedent(f"""
                _channels.send({cid}, b'spam')
                """))
        self.assertEqual(excsnap.type.__name__, 'ChannelClosedError')

        excsnap = interpreters.run_string(id2, dedent(f"""
                _channels.send({cid}, b'spam')
                """))
        self.assertEqual(excsnap.type.__name__, 'ChannelClosedError')

    def test_close_multiple_times(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.recv(cid)
        channels.close(cid)

        with self.assertRaises(channels.ChannelClosedError):
            channels.close(cid)

    def test_close_empty(self):
        tests = [
            (False, False),
            (True, False),
            (False, True),
            (True, True),
            ]
        for send, recv in tests:
            with self.subTest((send, recv)):
                cid = channels.create()
                channels.send(cid, b'spam', blocking=False)
                channels.recv(cid)
                channels.close(cid, send=send, recv=recv)

                with self.assertRaises(channels.ChannelClosedError):
                    channels.send(cid, b'eggs')
                with self.assertRaises(channels.ChannelClosedError):
                    channels.recv(cid)

    def test_close_defaults_with_unused_items(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'ham', blocking=False)

        with self.assertRaises(channels.ChannelNotEmptyError):
            channels.close(cid)
        channels.recv(cid)
        channels.send(cid, b'eggs', blocking=False)

    def test_close_recv_with_unused_items_unforced(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'ham', blocking=False)

        with self.assertRaises(channels.ChannelNotEmptyError):
            channels.close(cid, recv=True)
        channels.recv(cid)
        channels.send(cid, b'eggs', blocking=False)
        channels.recv(cid)
        channels.recv(cid)
        channels.close(cid, recv=True)

    def test_close_send_with_unused_items_unforced(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'ham', blocking=False)
        channels.close(cid, send=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        channels.recv(cid)
        channels.recv(cid)
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_close_both_with_unused_items_unforced(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'ham', blocking=False)

        with self.assertRaises(channels.ChannelNotEmptyError):
            channels.close(cid, recv=True, send=True)
        channels.recv(cid)
        channels.send(cid, b'eggs', blocking=False)
        channels.recv(cid)
        channels.recv(cid)
        channels.close(cid, recv=True)

    def test_close_recv_with_unused_items_forced(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'ham', blocking=False)
        channels.close(cid, recv=True, force=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_close_send_with_unused_items_forced(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'ham', blocking=False)
        channels.close(cid, send=True, force=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_close_both_with_unused_items_forced(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'ham', blocking=False)
        channels.close(cid, send=True, recv=True, force=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_close_never_used(self):
        cid = channels.create()
        channels.close(cid)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'spam')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_close_by_unassociated_interp(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        interp = interpreters.create()
        interpreters.run_string(interp, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.close({cid}, force=True)
            """))
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)
        with self.assertRaises(channels.ChannelClosedError):
            channels.close(cid)

    def test_close_used_multiple_times_by_single_user(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'spam', blocking=False)
        channels.recv(cid)
        channels.close(cid, force=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_channel_list_interpreters_invalid_channel(self):
        cid = channels.create()
        # Test for invalid channel ID.
        with self.assertRaises(channels.ChannelNotFoundError):
            channels.list_interpreters(1000, send=True)

        channels.close(cid)
        # Test for a channel that has been closed.
        with self.assertRaises(channels.ChannelClosedError):
            channels.list_interpreters(cid, send=True)

    def test_channel_list_interpreters_invalid_args(self):
        # Tests for invalid arguments passed to the API.
        cid = channels.create()
        with self.assertRaises(TypeError):
            channels.list_interpreters(cid)


class ChannelReleaseTests(TestBase):

    # XXX Add more test coverage a la the tests for close().

    """
    - main / interp / other
    - run in: current thread / new thread / other thread / different threads
    - end / opposite
    - force / no force
    - used / not used  (associated / not associated)
    - empty / emptied / never emptied / partly emptied
    - closed / not closed
    - released / not released
    - creator (interp) / other
    - associated interpreter not running
    - associated interpreter destroyed
    """

    """
    use
    pre-release
    release
    after
    check
    """

    """
    release in:         main, interp1
    creator:            same, other (incl. interp2)

    use:                None,send,recv,send/recv in None,same,other(incl. interp2),same+other(incl. interp2),all
    pre-release:        None,send,recv,both in None,same,other(incl. interp2),same+other(incl. interp2),all
    pre-release forced: None,send,recv,both in None,same,other(incl. interp2),same+other(incl. interp2),all

    release:            same
    release forced:     same

    use after:          None,send,recv,send/recv in None,same,other(incl. interp2),same+other(incl. interp2),all
    release after:      None,send,recv,send/recv in None,same,other(incl. interp2),same+other(incl. interp2),all
    check released:     send/recv for same/other(incl. interp2)
    check closed:       send/recv for same/other(incl. interp2)
    """

    def test_single_user(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.recv(cid)
        channels.release(cid, send=True, recv=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_multiple_users(self):
        cid = channels.create()
        id1 = interpreters.create()
        id2 = interpreters.create()
        interpreters.run_string(id1, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.send({cid}, b'spam', blocking=False)
            """))
        out = _run_output(id2, dedent(f"""
            import _xxinterpchannels as _channels
            obj = _channels.recv({cid})
            _channels.release({cid})
            print(repr(obj))
            """))
        interpreters.run_string(id1, dedent(f"""
            _channels.release({cid})
            """))

        self.assertEqual(out.strip(), "b'spam'")

    def test_no_kwargs(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.recv(cid)
        channels.release(cid)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_multiple_times(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.recv(cid)
        channels.release(cid, send=True, recv=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.release(cid, send=True, recv=True)

    def test_with_unused_items(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'ham', blocking=False)
        channels.release(cid, send=True, recv=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_never_used(self):
        cid = channels.create()
        channels.release(cid)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'spam')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_by_unassociated_interp(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        interp = interpreters.create()
        interpreters.run_string(interp, dedent(f"""
            import _xxinterpchannels as _channels
            _channels.release({cid})
            """))
        obj = channels.recv(cid)
        channels.release(cid)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        self.assertEqual(obj, b'spam')

    def test_close_if_unassociated(self):
        # XXX Something's not right with this test...
        cid = channels.create()
        interp = interpreters.create()
        interpreters.run_string(interp, dedent(f"""
            import _xxinterpchannels as _channels
            obj = _channels.send({cid}, b'spam', blocking=False)
            _channels.release({cid})
            """))

        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)

    def test_partially(self):
        # XXX Is partial close too weird/confusing?
        cid = channels.create()
        channels.send(cid, None, blocking=False)
        channels.recv(cid)
        channels.send(cid, b'spam', blocking=False)
        channels.release(cid, send=True)
        obj = channels.recv(cid)

        self.assertEqual(obj, b'spam')

    def test_used_multiple_times_by_single_user(self):
        cid = channels.create()
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'spam', blocking=False)
        channels.send(cid, b'spam', blocking=False)
        channels.recv(cid)
        channels.release(cid, send=True, recv=True)

        with self.assertRaises(channels.ChannelClosedError):
            channels.send(cid, b'eggs')
        with self.assertRaises(channels.ChannelClosedError):
            channels.recv(cid)


class ChannelCloseFixture(namedtuple('ChannelCloseFixture',
                                     'end interp other extra creator')):

    # Set this to True to avoid creating interpreters, e.g. when
    # scanning through test permutations without running them.
    QUICK = False

    def __new__(cls, end, interp, other, extra, creator):
        assert end in ('send', 'recv')
        if cls.QUICK:
            known = {}
        else:
            interp = Interpreter.from_raw(interp)
            other = Interpreter.from_raw(other)
            extra = Interpreter.from_raw(extra)
            known = {
                interp.name: interp,
                other.name: other,
                extra.name: extra,
                }
        if not creator:
            creator = 'same'
        self = super().__new__(cls, end, interp, other, extra, creator)
        self._prepped = set()
        self._state = ChannelState()
        self._known = known
        return self

    @property
    def state(self):
        return self._state

    @property
    def cid(self):
        try:
            return self._cid
        except AttributeError:
            creator = self._get_interpreter(self.creator)
            self._cid = self._new_channel(creator)
            return self._cid

    def get_interpreter(self, interp):
        interp = self._get_interpreter(interp)
        self._prep_interpreter(interp)
        return interp

    def expect_closed_error(self, end=None):
        if end is None:
            end = self.end
        if end == 'recv' and self.state.closed == 'send':
            return False
        return bool(self.state.closed)

    def prep_interpreter(self, interp):
        self._prep_interpreter(interp)

    def record_action(self, action, result):
        self._state = result

    def clean_up(self):
        clean_up_interpreters()
        clean_up_channels()

    # internal methods

    def _new_channel(self, creator):
        if creator.name == 'main':
            return channels.create()
        else:
            ch = channels.create()
            run_interp(creator.id, f"""
                import _xxsubinterpreters
                cid = _xxsubchannels.create()
                # We purposefully send back an int to avoid tying the
                # channel to the other interpreter.
                _xxsubchannels.send({ch}, int(cid), blocking=False)
                del _xxsubinterpreters
                """)
            self._cid = channels.recv(ch)
        return self._cid

    def _get_interpreter(self, interp):
        if interp in ('same', 'interp'):
            return self.interp
        elif interp == 'other':
            return self.other
        elif interp == 'extra':
            return self.extra
        else:
            name = interp
            try:
                interp = self._known[name]
            except KeyError:
                interp = self._known[name] = Interpreter(name)
            return interp

    def _prep_interpreter(self, interp):
        if interp.id in self._prepped:
            return
        self._prepped.add(interp.id)
        if interp.name == 'main':
            return
        run_interp(interp.id, f"""
            import _xxinterpchannels as channels
            import test.test__xxinterpchannels as helpers
            ChannelState = helpers.ChannelState
            try:
                cid
            except NameError:
                cid = channels._channel_id({self.cid})
            """)


@unittest.skip('these tests take several hours to run')
class ExhaustiveChannelTests(TestBase):

    """
    - main / interp / other
    - run in: current thread / new thread / other thread / different threads
    - end / opposite
    - force / no force
    - used / not used  (associated / not associated)
    - empty / emptied / never emptied / partly emptied
    - closed / not closed
    - released / not released
    - creator (interp) / other
    - associated interpreter not running
    - associated interpreter destroyed

    - close after unbound
    """

    """
    use
    pre-close
    close
    after
    check
    """

    """
    close in:         main, interp1
    creator:          same, other, extra

    use:              None,send,recv,send/recv in None,same,other,same+other,all
    pre-close:        None,send,recv in None,same,other,same+other,all
    pre-close forced: None,send,recv in None,same,other,same+other,all

    close:            same
    close forced:     same

    use after:        None,send,recv,send/recv in None,same,other,extra,same+other,all
    close after:      None,send,recv,send/recv in None,same,other,extra,same+other,all
    check closed:     send/recv for same/other(incl. interp2)
    """

    def iter_action_sets(self):
        # - used / not used  (associated / not associated)
        # - empty / emptied / never emptied / partly emptied
        # - closed / not closed
        # - released / not released

        # never used
        yield []

        # only pre-closed (and possible used after)
        for closeactions in self._iter_close_action_sets('same', 'other'):
            yield closeactions
            for postactions in self._iter_post_close_action_sets():
                yield closeactions + postactions
        for closeactions in self._iter_close_action_sets('other', 'extra'):
            yield closeactions
            for postactions in self._iter_post_close_action_sets():
                yield closeactions + postactions

        # used
        for useactions in self._iter_use_action_sets('same', 'other'):
            yield useactions
            for closeactions in self._iter_close_action_sets('same', 'other'):
                actions = useactions + closeactions
                yield actions
                for postactions in self._iter_post_close_action_sets():
                    yield actions + postactions
            for closeactions in self._iter_close_action_sets('other', 'extra'):
                actions = useactions + closeactions
                yield actions
                for postactions in self._iter_post_close_action_sets():
                    yield actions + postactions
        for useactions in self._iter_use_action_sets('other', 'extra'):
            yield useactions
            for closeactions in self._iter_close_action_sets('same', 'other'):
                actions = useactions + closeactions
                yield actions
                for postactions in self._iter_post_close_action_sets():
                    yield actions + postactions
            for closeactions in self._iter_close_action_sets('other', 'extra'):
                actions = useactions + closeactions
                yield actions
                for postactions in self._iter_post_close_action_sets():
                    yield actions + postactions

    def _iter_use_action_sets(self, interp1, interp2):
        interps = (interp1, interp2)

        # only recv end used
        yield [
            ChannelAction('use', 'recv', interp1),
            ]
        yield [
            ChannelAction('use', 'recv', interp2),
            ]
        yield [
            ChannelAction('use', 'recv', interp1),
            ChannelAction('use', 'recv', interp2),
            ]

        # never emptied
        yield [
            ChannelAction('use', 'send', interp1),
            ]
        yield [
            ChannelAction('use', 'send', interp2),
            ]
        yield [
            ChannelAction('use', 'send', interp1),
            ChannelAction('use', 'send', interp2),
            ]

        # partially emptied
        for interp1 in interps:
            for interp2 in interps:
                for interp3 in interps:
                    yield [
                        ChannelAction('use', 'send', interp1),
                        ChannelAction('use', 'send', interp2),
                        ChannelAction('use', 'recv', interp3),
                        ]

        # fully emptied
        for interp1 in interps:
            for interp2 in interps:
                for interp3 in interps:
                    for interp4 in interps:
                        yield [
                            ChannelAction('use', 'send', interp1),
                            ChannelAction('use', 'send', interp2),
                            ChannelAction('use', 'recv', interp3),
                            ChannelAction('use', 'recv', interp4),
                            ]

    def _iter_close_action_sets(self, interp1, interp2):
        ends = ('recv', 'send')
        interps = (interp1, interp2)
        for force in (True, False):
            op = 'force-close' if force else 'close'
            for interp in interps:
                for end in ends:
                    yield [
                        ChannelAction(op, end, interp),
                        ]
        for recvop in ('close', 'force-close'):
            for sendop in ('close', 'force-close'):
                for recv in interps:
                    for send in interps:
                        yield [
                            ChannelAction(recvop, 'recv', recv),
                            ChannelAction(sendop, 'send', send),
                            ]

    def _iter_post_close_action_sets(self):
        for interp in ('same', 'extra', 'other'):
            yield [
                ChannelAction('use', 'recv', interp),
                ]
            yield [
                ChannelAction('use', 'send', interp),
                ]

    def run_actions(self, fix, actions):
        for action in actions:
            self.run_action(fix, action)

    def run_action(self, fix, action, *, hideclosed=True):
        end = action.resolve_end(fix.end)
        interp = action.resolve_interp(fix.interp, fix.other, fix.extra)
        fix.prep_interpreter(interp)
        if interp.name == 'main':
            result = run_action(
                fix.cid,
                action.action,
                end,
                fix.state,
                hideclosed=hideclosed,
                )
            fix.record_action(action, result)
        else:
            _cid = channels.create()
            run_interp(interp.id, f"""
                result = helpers.run_action(
                    {fix.cid},
                    {repr(action.action)},
                    {repr(end)},
                    {repr(fix.state)},
                    hideclosed={hideclosed},
                    )
                channels.send({_cid}, result.pending.to_bytes(1, 'little'), blocking=False)
                channels.send({_cid}, b'X' if result.closed else b'', blocking=False)
                """)
            result = ChannelState(
                pending=int.from_bytes(channels.recv(_cid), 'little'),
                closed=bool(channels.recv(_cid)),
                )
            fix.record_action(action, result)

    def iter_fixtures(self):
        # XXX threads?
        interpreters = [
            ('main', 'interp', 'extra'),
            ('interp', 'main', 'extra'),
            ('interp1', 'interp2', 'extra'),
            ('interp1', 'interp2', 'main'),
        ]
        for interp, other, extra in interpreters:
            for creator in ('same', 'other', 'creator'):
                for end in ('send', 'recv'):
                    yield ChannelCloseFixture(end, interp, other, extra, creator)

    def _close(self, fix, *, force):
        op = 'force-close' if force else 'close'
        close = ChannelAction(op, fix.end, 'same')
        if not fix.expect_closed_error():
            self.run_action(fix, close, hideclosed=False)
        else:
            with self.assertRaises(channels.ChannelClosedError):
                self.run_action(fix, close, hideclosed=False)

    def _assert_closed_in_interp(self, fix, interp=None):
        if interp is None or interp.name == 'main':
            with self.assertRaises(channels.ChannelClosedError):
                channels.recv(fix.cid)
            with self.assertRaises(channels.ChannelClosedError):
                channels.send(fix.cid, b'spam')
            with self.assertRaises(channels.ChannelClosedError):
                channels.close(fix.cid)
            with self.assertRaises(channels.ChannelClosedError):
                channels.close(fix.cid, force=True)
        else:
            run_interp(interp.id, """
                with helpers.expect_channel_closed():
                    channels.recv(cid)
                """)
            run_interp(interp.id, """
                with helpers.expect_channel_closed():
                    channels.send(cid, b'spam', blocking=False)
                """)
            run_interp(interp.id, """
                with helpers.expect_channel_closed():
                    channels.close(cid)
                """)
            run_interp(interp.id, """
                with helpers.expect_channel_closed():
                    channels.close(cid, force=True)
                """)

    def _assert_closed(self, fix):
        self.assertTrue(fix.state.closed)

        for _ in range(fix.state.pending):
            channels.recv(fix.cid)
        self._assert_closed_in_interp(fix)

        for interp in ('same', 'other'):
            interp = fix.get_interpreter(interp)
            if interp.name == 'main':
                continue
            self._assert_closed_in_interp(fix, interp)

        interp = fix.get_interpreter('fresh')
        self._assert_closed_in_interp(fix, interp)

    def _iter_close_tests(self, verbose=False):
        i = 0
        for actions in self.iter_action_sets():
            print()
            for fix in self.iter_fixtures():
                i += 1
                if i > 1000:
                    return
                if verbose:
                    if (i - 1) % 6 == 0:
                        print()
                    print(i, fix, '({} actions)'.format(len(actions)))
                else:
                    if (i - 1) % 6 == 0:
                        print(' ', end='')
                    print('.', end=''); sys.stdout.flush()
                yield i, fix, actions
            if verbose:
                print('---')
        print()

    # This is useful for scanning through the possible tests.
    def _skim_close_tests(self):
        ChannelCloseFixture.QUICK = True
        for i, fix, actions in self._iter_close_tests():
            pass

    def test_close(self):
        for i, fix, actions in self._iter_close_tests():
            with self.subTest('{} {}  {}'.format(i, fix, actions)):
                fix.prep_interpreter(fix.interp)
                self.run_actions(fix, actions)

                self._close(fix, force=False)

                self._assert_closed(fix)
            # XXX Things slow down if we have too many interpreters.
            fix.clean_up()

    def test_force_close(self):
        for i, fix, actions in self._iter_close_tests():
            with self.subTest('{} {}  {}'.format(i, fix, actions)):
                fix.prep_interpreter(fix.interp)
                self.run_actions(fix, actions)

                self._close(fix, force=True)

                self._assert_closed(fix)
            # XXX Things slow down if we have too many interpreters.
            fix.clean_up()


if __name__ == '__main__':
    unittest.main()
