"""This module includes tests for syntax errors that occur when a name
declared as `nonlocal` is used in ways that violate the language
specification, such as after assignment, usage, or annotation. The tests
verify that syntax errors are correctly raised for improper `nonlocal`
statements following variable use or assignment within functions.
Additionally, it tests various name-binding scenarios for nonlocal
variables to ensure correct behavior.

See `test_scope.py` for additional related behavioral tests covering
variable scoping and usage in different contexts.
"""

import contextlib
from test.support import check_syntax_error
from test.support.warnings_helper import check_warnings
from types import SimpleNamespace
import unittest
import warnings


class NonlocalTests(unittest.TestCase):

    def setUp(self):
        self.enterContext(check_warnings())
        warnings.filterwarnings("error", module="<test string>")

    ######################################################
    ### Syntax error cases as covered in Python/symtable.c
    ######################################################

    def test_name_param(self):
        prog_text = """\
def fn(name_param):
    nonlocal name_param
"""
        check_syntax_error(self, prog_text, lineno=2, offset=5)

    def test_name_after_assign(self):
        prog_text = """\
def fn():
    name_assign = 1
    nonlocal name_assign
"""
        check_syntax_error(self, prog_text, lineno=3, offset=5)

    def test_name_after_use(self):
        prog_text = """\
def fn():
    print(name_use)
    nonlocal name_use
"""
        check_syntax_error(self, prog_text, lineno=3, offset=5)

    def test_name_annot(self):
        prog_text = """\
def fn():
    name_annot: int
    nonlocal name_annot
"""
        check_syntax_error(self, prog_text, lineno=3, offset=5)

    ###############################################################
    ### Tests for nonlocal variables across all name binding cases,
    ### as described in executionmodel.rst
    ###############################################################

    def test_assignment_statement(self):
        name_assignment_statement = None
        value = object()

        def inner():
            nonlocal name_assignment_statement
            name_assignment_statement = value

        inner()
        self.assertIs(name_assignment_statement, value)

    def test_unpacking_assignment(self):
        name_unpacking_assignment = None
        value = object()

        def inner():
            nonlocal name_unpacking_assignment
            _, name_unpacking_assignment = [None, value]

        inner()
        self.assertIs(name_unpacking_assignment, value)

    def test_assignment_expression(self):
        name_assignment_expression = None
        value = object()

        def inner():
            nonlocal name_assignment_expression
            if name_assignment_expression := value:
                pass

        inner()
        self.assertIs(name_assignment_expression, value)

    def test_iteration_variable(self):
        name_iteration_variable = None
        value = object()

        def inner():
            nonlocal name_iteration_variable
            for name_iteration_variable in [value]:
                pass

        inner()
        self.assertIs(name_iteration_variable, value)

    def test_func_def(self):
        name_func_def = None

        def inner():
            nonlocal name_func_def

            def name_func_def():
                pass

        inner()
        self.assertIs(name_func_def, name_func_def)

    def test_class_def(self):
        name_class_def = None

        def inner():
            nonlocal name_class_def

            class name_class_def:
                pass

        inner()
        self.assertIs(name_class_def, name_class_def)

    def test_type_alias(self):
        name_type_alias = None

        def inner():
            nonlocal name_type_alias
            type name_type_alias = tuple[int, int]

        inner()
        self.assertIs(name_type_alias, name_type_alias)

    def test_caught_exception(self):
        name_caught_exc = None

        def inner():
            nonlocal self, inner, name_caught_exc
            idx = inner.__code__.co_freevars.index("name_caught_exc")
            try:
                1 / 0
            except ZeroDivisionError as name_caught_exc:
                # `name_caught_exc` is cleared automatically after the except block
                self.assertIs(inner.__closure__[idx].cell_contents, name_caught_exc)

        inner()

    def test_caught_exception_group(self):
        name_caught_exc_group = None

        def inner():
            nonlocal self, inner, name_caught_exc_group
            idx = inner.__code__.co_freevars.index("name_caught_exc_group")

            try:
                try:
                    1 / 0
                except ZeroDivisionError as exc:
                    raise ExceptionGroup("eg", [exc])
            except* ZeroDivisionError as name_caught_exc_group:
                # `name_caught_exc_group` is cleared automatically after the except block
                self.assertIs(
                    inner.__closure__[idx].cell_contents, name_caught_exc_group
                )

        inner()

    def test_enter_result(self):
        name_enter_result = None
        value = object()

        def inner():
            nonlocal name_enter_result
            with contextlib.nullcontext(value) as name_enter_result:
                pass

        inner()
        self.assertIs(name_enter_result, value)

    def test_import_result(self):
        name_import_result = None
        value = contextlib

        def inner():
            nonlocal name_import_result
            import contextlib as name_import_result

        inner()
        self.assertIs(name_import_result, value)

    def test_match(self):
        name_match = None
        value = object()

        def inner():
            nonlocal name_match
            match value:
                case name_match:
                    pass

        inner()
        self.assertIs(name_match, value)

    def test_match_as(self):
        name_match_as = None
        value = object()

        def inner():
            nonlocal name_match_as
            match value:
                case _ as name_match_as:
                    pass

        inner()
        self.assertIs(name_match_as, value)

    def test_match_seq(self):
        name_match_seq = None
        value = object()

        def inner():
            nonlocal name_match_seq
            match (None, value):
                case (_, name_match_seq):
                    pass

        inner()
        self.assertIs(name_match_seq, value)

    def test_match_map(self):
        name_match_map = None
        value = object()

        def inner():
            nonlocal name_match_map
            match {"key": value}:
                case {"key": name_match_map}:
                    pass

        inner()
        self.assertIs(name_match_map, value)

    def test_match_attr(self):
        name_match_attr = None
        value = object()

        def inner():
            nonlocal name_match_attr
            match SimpleNamespace(key=value):
                case SimpleNamespace(key=name_match_attr):
                    pass

        inner()
        self.assertIs(name_match_attr, value)


def setUpModule():
    unittest.enterModuleContext(warnings.catch_warnings())
    warnings.filterwarnings("error", module="<test string>")


if __name__ == "__main__":
    unittest.main()
