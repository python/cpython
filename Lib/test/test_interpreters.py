import interpreters
import _interpreters
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
    for id in _interpreters.list_all():
        if id == 0:  # main
            continue
        try:
            _interpreters.destroy(id)
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

class CreateTests(TestBase):

    def test_create(self):
        interp = interpreters.create()
        lst = interpreters.list_all()
        self.assertEqual(interp.id, lst[1].id)

class GetCurrentTests(TestBase):

    def test_get_current(self):
        main_interp_id = _interpreters.get_main()
        cur_interp_id =  interpreters.get_current().id
        self.assertEqual(cur_interp_id, main_interp_id)

class ListAllTests(TestBase):

    def test_initial(self):
        interps = interpreters.list_all()
        self.assertEqual(1, len(interps))

class TestInterpreter(TestBase):

    def test_id_fields(self):
        interp = interpreters.Interpreter(1)
        self.assertEqual(1, interp.id)

    def test_is_running(self):
        interp_de = interpreters.create()
        self.assertEqual(False, interp_de.is_running())

    def test_destroy(self):
        interp = interpreters.create()
        interp2 = interpreters.create()
        interp.destroy()
        interps = interpreters.list_all()
        self.assertEqual(2, len(interps))

    def test_run(self):
        interp = interpreters.create()
        interp.run("3")
        self.assertEqual(False, interp.is_running())


