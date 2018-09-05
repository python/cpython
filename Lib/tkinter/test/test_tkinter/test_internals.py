import unittest, test.support
import _tkinter
import re

class InternalsTest(unittest.TestCase):
    def assert_raise_on_new(self, tkinter_type):
        # Users are intentionally prevented from creating new instances of
        # _tkinter.TkappType, _tkinter.Tcl_Obj and _tkinter.TkttType
        tkinter_type_name = f'_tkinter.{tkinter_type.__name__}'

        cant_create_message = re.escape(f"cannot create '{tkinter_type_name}' instances")
        object_message = re.escape(
            f'object.__new__({tkinter_type_name}) is not safe, '
            f'use {tkinter_type_name}.__new__()'
        )

        with self.assertRaisesRegex(TypeError, cant_create_message):
            tkinter_type()
        with self.assertRaisesRegex(TypeError, cant_create_message):
            tkinter_type.__new__(tkinter_type)
        with self.assertRaisesRegex(TypeError, object_message):
            object.__new__(tkinter_type)

    def test_tkapp_type_no_instantiation(self):
        self.assert_raise_on_new(_tkinter.TkappType)

    def test_tck_obj_no_instantiation(self):
        self.assert_raise_on_new(_tkinter.Tcl_Obj)

    def test_tktt_type_info_no_instantiation(self):
        self.assert_raise_on_new(_tkinter.TkttType)


def test_main():
    test.support.run_unittest(InternalsTest,)

if __name__ == "__main__":
    test_main()
