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


class LowLevelStub:
    # set these as appropriate in tests
    errors = ()
    return_create = ()
    ...
    def __init__(self):
        self._calls = []
    def _add_call(self, name, args=(), kwargs=None):
        self.calls.append(
            (name, args, kwargs or {}))
    def _maybe_error(self):
        if not self.errors:
            return
        err = self.errors.pop(0)
        if err is not None:
            raise err
    def check_calls(self, test, expected):
        test.assertEqual(self._calls, expected)
        for returns in [self.errors, self.return_create, ...]:
            test.assertEqual(tuple(returns), ())  # make sure all were used
        
    # the stubbed methods
    def create(self):
        self._add_call('create')
        self._maybe_error()
        return self.return_create.pop(0)
    def list_all(self):
        ...
    def get_current(self):
        ...
    def get_main(self):
        ...
    def destroy(self, id):
        ...
    def run_string(self, id, text, ...):
        ...


class TestBase(unittest.TestCase):

    def tearDown(self):
        clean_up_interpreters()


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


class GetMainTests(TestBase):

    def test_get_main(self):
        [expected] = interpreters.list_all()
        main = interpreters.get_main()
        self.assertEqual(main, expected)


class CreateTests(TestBase):

    def test_create(self):
        interp = interpreters.create()
        self.assertIn(interp, interpreters.list_all())


class DestroyTests(TestBase):

    def test_destroy(self):
        id1 = interpreters.create()
        id2 = interpreters.create()
        id3 = interpreters.create()
        self.assertIn(id2, interpreters.list_all())
        interpreters.destroy(id2)
        self.assertNotIn(id2, interpreters.list_all())


class RunStringTests(TestBase):

    def test_run_string(self):
        script, file = _captured_script('print("it worked!", end="")')
        id = interpreters.create()
        with file:
            interpreters.run_string(id, script, None)
            out = file.read()

        self.assertEqual(out, 'it worked!')


if __name__ == '__main__':
    unittest.main()
