import interpreters
import os
from textwrap import dedent
import unittest

def _captured_script(script):
    r, w = os.pipe()
    indented = script.replace('\n', '\n                ')
    wrapped = dedent(f"""
        import contextlib
        with open({w}, 'w') as spipe:
            with contextlib.redirect_stdout(spipe):
                {indented}
        """)
    return wrapped, open(r)

def clean_up_interpreters():
    for id in interpreters.list_all():
        if id == 0:  # main
            continue
        try:
            interpreters.destroy(id)
        except RuntimeError:
            pass  # already destroyed

def _run_output(interp, request, shared=None):
    script, rpipe = _captured_script(request)
    with rpipe:
        interpreters.run_string(interp, script, shared)
        return rpipe.read()

class TestBase(unittest.TestCase):

    def tearDown(self):
        clean_up_interpreters()

class TestInterpreter(TestBase):

    def test_is_running(self):
        interp = interpreters.Interpreter(1)
        self.assertEqual(True, interp.is_running())

    def test_destroy(self):
        interp = interpreters.Interpreter(1)
        interp2 = interpreters.Interpreter(2)
        interp.destroy()
        ids = interpreters.list_all()
        self.assertEqual(ids, [interp2.id])

    def test_run(self):
        interp = interpreters.Interpreter(1)
        interp.run(dedent(f"""
            import _interpreters
            _interpreters.channel_send({cid}, b'spam')
            """))
        out = _run_output(id2, dedent(f"""
            import _interpreters
            obj = _interpreters.channel_recv({cid})
            _interpreters.channel_release({cid})
            print(repr(obj))
            """))
        self.assertEqual(out.strip(), "b'spam'")

class RecvChannelTest(TestBase):

    def test_release(self):
        import _interpreters as interpreters

        chanl = interpreters.RecvChannel(1)
        interpreters.channel_send(chanl.id, b'spam')
        interpreters.channel_recv(cid)
        chanl.release(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_close(self):
        chanl = interpreters.RecvChannel(1)
        chanl.close()
        with self.assertRaises(interpreters.ChannelClosedError):
                interpreters.channel_recv(fix.cid)

class SendChannelTest(TestBase):

    def test_release(self):
        import _interpreters as interpreters

        chanl = interpreters.SendChannel(1)
        interpreters.channel_send(chanl.id, b'spam')
        interpreters.channel_recv(cid)
        chanl.release(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_close(self):
        chanl = interpreters.RecvChannel(1)
        chanl.close()
        with self.assertRaises(interpreters.ChannelClosedError):
                interpreters.channel_recv(fix.cid)


class ListAllTests(TestBase):

    def test_initial(self):
        main = interpreters.get_main()
        ids = interpreters.list_all()
        self.assertEqual(ids, [main])

class GetCurrentTests(TestBase):

    def test_get_current(self):
        main = interpreters.get_main()
        cur = interpreters.get_current()
        self.assertEqual(cur, main)

class CreateTests(TestBase):

    def test_create(self):
        interp = interpreters.create()
        self.assertIn(interp, interpreters.list_all())

class ExceptionTests(TestBase):

    def test_does_not_exist(self):
        cid = interpreters.create_channel()
        recvCha = interpreters.RecvChannel(cid)
        with self.assertRaises(interpreters.ChannelNotFoundError):
            interpreters._channel_id(int(cid) + 1)

    def test_recv_empty(self):
        cid = interpreters.create_channel()
        recvCha = interpreters.RecvChannel(cid)
        with self.assertRaises(interpreters.ChannelEmptyError):
            recvCha.recv()

    def test_channel_not_empty(self):
        cid = interpreters.create_channel()
        sendCha = interpreters.SendChannel(cid)
        sendCha.send(b'spam')
        sendCha.send(b'ham')

        with self.assertRaises(interpreters.ChannelNotEmptyError):
            sendCha.close()

    def test_channel_closed(self):
        cid = interpreters.channel_create()
        sendCha = interpreters.SendChannel(cid)
        sendCha.send(b'spam')
        sendCha.send(b'ham')
        sendCha.close()

        with self.assertRaises(interpreters.ChannelClosedError):
            sendCha.send(b'spam')


if __name__ == '__main__':
    unittest.main()
