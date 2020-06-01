import ast
import types
import typing
import unittest


FILENAME = "<patma tests>"


class MatchCase(typing.NamedTuple):
    pattern: str
    body: str
    guard: typing.Optional[str] = None


class TestAST(unittest.TestCase):
    """Tests that predate parser support, and just execute ASTs instead.

    No tests for name loads/stores, since these need a patma parser to
    disambiguate.

    Note that we use "or" for "|" here, since the parser gives us a BoolOp-Or,
    not a BinOp-BitOr.
    """

    @staticmethod
    def parse_stmts(stmts: str) -> typing.List[ast.stmt]:
        return ast.parse(stmts, FILENAME, "exec").body

    @staticmethod
    def parse_expr(expr: str) -> ast.expr:
        return ast.parse(expr, FILENAME, "eval").body

    @classmethod
    def parse_match_case(cls, match_case: MatchCase) -> ast.expr:
        pattern = cls.parse_expr(match_case.pattern)
        guard = None if match_case.guard is None else cls.parse_expr(match_case.guard)
        body = cls.parse_stmts(match_case.body)
        return ast.match_case(pattern=pattern, guard=guard, body=body)

    @classmethod
    def compile_match(
        cls, pre: str, target: str, match_cases: typing.Iterable[MatchCase], post: str
    ) -> types.CodeType:
        cases = [cls.parse_match_case(case) for case in match_cases]
        match = ast.Match(target=cls.parse_expr(target), cases=cases)
        body = [*cls.parse_stmts(pre), match, *cls.parse_stmts(post)]
        tree = ast.fix_missing_locations(ast.Module(body=body, type_ignores=[]))
        return compile(tree, FILENAME, "exec")

    @classmethod
    def execute_match(
        cls, pre: str, target: str, match_cases: typing.Iterable[MatchCase], post: str
    ) -> typing.Dict[str, typing.Any]:
        namespace = {}
        exec(cls.compile_match(pre, target, match_cases, post), None, namespace)
        return namespace

    def test_steroid_switch_00(self) -> None:
        match_cases = [MatchCase("0", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_01(self) -> None:
        match_cases = [MatchCase("False", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_02(self) -> None:
        match_cases = [MatchCase("1", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_steroid_switch_03(self) -> None:
        match_cases = [MatchCase("None", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_steroid_switch_04(self) -> None:
        match_cases = [MatchCase("0", "y = 0"), MatchCase("0", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_05(self) -> None:
        match_cases = [MatchCase("1", "y = 0"), MatchCase("1", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_steroid_switch_06(self) -> None:
        match_cases = [MatchCase("'x'", "y = 0"), MatchCase("'y'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_07(self) -> None:
        match_cases = [MatchCase("'y'", "y = 0"), MatchCase("'x'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 1)

    def test_steroid_switch_08(self) -> None:
        match_cases = [MatchCase("'x'", "y = 0"), MatchCase("'y'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_09(self) -> None:
        match_cases = [MatchCase("b'y'", "y = 0"), MatchCase("b'x'", "y = 1")]
        namespace = self.execute_match("x = b'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"x")
        self.assertEqual(namespace.get("y"), 1)

    def test_steroid_switch_guard_00(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "False"), MatchCase("0", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 1)

    def test_steroid_switch_guard_01(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "0"), MatchCase("0", "y = 1", "0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_steroid_switch_guard_02(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_guard_03(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_guard_04(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "y = 2")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 2)

    def test_steroid_switch_guard_05(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "0"), MatchCase("0", "y = 1", "1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "y = 2")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 2)

    def test_steroid_switch_guard_06(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "not (x := 1)"), MatchCase("1", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertNotIn("y", namespace)

    def test_walrus_00(self) -> None:
        match_cases = [
            MatchCase("0", "y = 0", "not (x := 1)"),
            MatchCase("(z := 0)", "y = 1"),
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 1)
        self.assertEqual(namespace.get("z"), 0)

    def test_walrus_01(self) -> None:
        match_cases = [
            MatchCase("(z := 1)", "y = 0", "not (x := 1)"),
            MatchCase("0", "y = 1"),
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 1)
        self.assertNotIn("z", namespace)

    def test_walrus_02(self) -> None:
        match_cases = [MatchCase("(z := 0)", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_walrus_03(self) -> None:
        match_cases = [MatchCase("(z := 1)", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)
        self.assertNotIn("z", namespace)

    def test_walrus_04(self) -> None:
        match_cases = [MatchCase("(z := 0)", "y = 0", "(w := 0)")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("w"), 0)
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)
        self.assertEqual(namespace.get("z"), 0)

    def test_walrus_05(self) -> None:
        match_cases = [MatchCase("(z := (w := 0))", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("w"), 0)
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_pipe_00(self) -> None:
        match_cases = [MatchCase("0 or 1 or 2", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_pipe_01(self) -> None:
        match_cases = [MatchCase("0 or 1 or 2", "y = 0")]
        namespace = self.execute_match("x = 1", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 0)

    def test_pipe_02(self) -> None:
        match_cases = [MatchCase("0 or 1 or 2", "y = 0")]
        namespace = self.execute_match("x = 2", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 2)
        self.assertEqual(namespace.get("y"), 0)

    def test_pipe_03(self) -> None:
        match_cases = [MatchCase("0 or 1 or 2", "y = 0")]
        namespace = self.execute_match("x = 3", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 3)
        self.assertNotIn("y", namespace)

    def test_pipe_04(self) -> None:
        match_cases = [
            MatchCase("(z := 0) or (z := 1) or (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_pipe_05(self) -> None:
        match_cases = [
            MatchCase("(z := 0) or (z := 1) or (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 1", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 1)

    def test_pipe_06(self) -> None:
        match_cases = [
            MatchCase("(z := 0) or (z := 1) or (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 2", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 2)
        self.assertNotIn("y", namespace)
        self.assertEqual(namespace.get("z"), 2)

    def test_pipe_07(self) -> None:
        match_cases = [
            MatchCase("(z := 0) or (z := 1) or (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 3", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 3)
        self.assertNotIn("y", namespace)
        self.assertNotIn("z", namespace)

    def test_sequence_00(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = ()", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), ())
        self.assertEqual(namespace.get("y"), 0)

    def test_sequence_01(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = ()", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), ())
        self.assertEqual(namespace.get("y"), 0)

    def test_sequence_02(self) -> None:
        match_cases = [MatchCase("[0]", "y = 0")]
        namespace = self.execute_match("x = (0,)", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), (0,))
        self.assertEqual(namespace.get("y"), 0)

    def test_sequence_03(self) -> None:
        match_cases = [MatchCase("[[]]", "y = 0")]
        namespace = self.execute_match("x = ((),)", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), ((),))
        self.assertEqual(namespace.get("y"), 0)

    def test_sequence_04(self) -> None:
        match_cases = [MatchCase("[0, 1] or [1, 0]", "y = 0")]
        namespace = self.execute_match("x = [0, 1]", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [0, 1])
        self.assertEqual(namespace.get("y"), 0)

    def test_sequence_05(self) -> None:
        match_cases = [MatchCase("[0, 1] or [1, 0]", "y = 0")]
        namespace = self.execute_match("x = [1, 0]", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [1, 0])
        self.assertEqual(namespace.get("y"), 0)

    def test_sequence_06(self) -> None:
        match_cases = [MatchCase("[0, 1] or [1, 0]", "y = 0")]
        namespace = self.execute_match("x = [0, 0]", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [0, 0])
        self.assertNotIn("y", namespace)

    def test_sequence_07(self) -> None:
        match_cases = [
            MatchCase("[w := 0,]", "y = 0"),
            MatchCase("[] or [1, z := (0 or 1)] or []", "y = 1"),
        ]
        namespace = self.execute_match("x = [1, 0]", "x", match_cases, "")
        self.assertNotIn("w", namespace)
        self.assertEqual(namespace.get("x"), [1, 0])
        self.assertEqual(namespace.get("y"), 1)
        self.assertEqual(namespace.get("z"), 0)

    def test_sequence_08(self) -> None:
        match_cases = [
            MatchCase("[0,]", "y = 0"),
            MatchCase("[1, 0]", "y = 1", "(x := x[:0])"),
            MatchCase("[1, 0]", "y = 2"),
        ]
        namespace = self.execute_match("x = [1, 0]", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [])
        self.assertEqual(namespace.get("y"), 2)

    def test_sequence_09(self) -> None:
        match_cases = [MatchCase("[0]", "y = 0")]
        namespace = self.execute_match("x = {0}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0})
        self.assertNotIn("y", namespace)

    def test_sequence_10(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = set()", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), set())
        self.assertNotIn("y", namespace)

    def test_sequence_11(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = iter([1,2,3])", "x", match_cases, "")
        self.assertEqual(list(namespace.get("x")), [1, 2, 3])
        self.assertNotIn("y", namespace)

    def test_sequence_12(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = {}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {})
        self.assertNotIn("y", namespace)

    def test_sequence_13(self) -> None:
        match_cases = [MatchCase("[0, 1]", "y = 0")]
        namespace = self.execute_match("x = {0: False, 1: True}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: False, 1: True})
        self.assertNotIn("y", namespace)

    def test_string_00(self) -> None:
        match_cases = [MatchCase("['x']", "y = 0"), MatchCase("'x'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 1)

    def test_string_01(self) -> None:
        match_cases = [
            MatchCase("[b'x']", "y = 0"),
            MatchCase("['x']", "y = 1"),
            MatchCase("[120]", "y = 2"),
            MatchCase("b'x'", "y = 4"),
        ]
        namespace = self.execute_match("x = b'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"x")
        self.assertEqual(namespace.get("y"), 4)

    def test_string_02(self) -> None:
        match_cases = [MatchCase("[120]", "y = 0"), MatchCase("120", "y = 1")]
        namespace = self.execute_match("x = bytearray(b'x')", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"x")
        self.assertNotIn("y", namespace)

    def test_string_03(self) -> None:
        match_cases = [
            MatchCase("[]", "y = 0"),
            MatchCase("['']", "y = 1"),
            MatchCase("''", "y = 2"),
        ]
        namespace = self.execute_match("x = ''", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "")
        self.assertEqual(namespace.get("y"), 2)

    def test_string_04(self) -> None:
        match_cases = [
            MatchCase("['x', 'x', 'x']", "y = 0"),
            MatchCase("['xxx']", "y = 1"),
            MatchCase("'xxx'", "y = 2"),
        ]
        namespace = self.execute_match("x = 'xxx'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "xxx")
        self.assertEqual(namespace.get("y"), 2)

    def test_string_05(self) -> None:
        match_cases = [
            MatchCase("[120, 120, 120]", "y = 0"),
            MatchCase("[b'xxx']", "y = 1"),
            MatchCase("b'xxx'", "y = 2"),
        ]
        namespace = self.execute_match("x = b'xxx'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"xxx")
        self.assertEqual(namespace.get("y"), 2)

    def test_mapping_00(self) -> None:
        match_cases = [MatchCase("{}", "y = 0")]
        namespace = self.execute_match("x = {}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {})
        self.assertEqual(namespace.get("y"), 0)

    def test_mapping_01(self) -> None:
        match_cases = [MatchCase("{}", "y = 0")]
        namespace = self.execute_match("x = {0: 0}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 0})
        self.assertEqual(namespace.get("y"), 0)

    def test_mapping_02(self) -> None:
        match_cases = [MatchCase("{0: 0}", "y = 0")]
        namespace = self.execute_match("x = {}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {})
        self.assertNotIn("y", namespace)

    def test_mapping_03(self) -> None:
        match_cases = [MatchCase("{0: (z := (0 or 1 or 2))}", "y = 0")]
        namespace = self.execute_match("x = {0: 0}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 0})
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_mapping_04(self) -> None:
        match_cases = [MatchCase("{0: (z := (0 or 1 or 2))}", "y = 0")]
        namespace = self.execute_match("x = {0: 1}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 1})
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 1)

    def test_mapping_05(self) -> None:
        match_cases = [MatchCase("{0: (z := (0 or 1 or 2))}", "y = 0")]
        namespace = self.execute_match("x = {0: 2}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 2})
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 2)

    def test_mapping_06(self) -> None:
        match_cases = [MatchCase("{0: (z := (0 or 1 or 2))}", "y = 0")]
        namespace = self.execute_match("x = {0: 3}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 3})
        self.assertNotIn("y", namespace)
        self.assertNotIn("z", namespace)

    def test_mapping_07(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2, {}]}", "y = 0"),
            MatchCase("{0: [1, 2, {}], 1: [[]]}", "y = 1"),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match("x = {}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {})
        self.assertNotIn("y", namespace)

    def test_mapping_08(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2, {}]}", "y = 0"),
            MatchCase("{0: [1, 2, {}], 1: [[]]}", "y = 1"),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match(
            "x = {False: (True, 2.0, {})}", "x", match_cases, ""
        )
        self.assertEqual(namespace.get("x"), {False: (True, 2.0, {})})
        self.assertEqual(namespace.get("y"), 0)

    def test_mapping_09(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2, {}]}", "y = 0"),
            MatchCase("{0: [1, 2, {}], 1: [[]]}", "y = 1"),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match(
            "x = {False: (True, 2.0, {}), 1: [[]], 2: 0}", "x", match_cases, ""
        )
        self.assertEqual(namespace.get("x"), {False: (True, 2.0, {}), 1: [[]], 2: 0})
        self.assertEqual(namespace.get("y"), 0)

    def test_mapping_10(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2]}", "y = 0"),
            MatchCase("{0: [1, 2, {}], 1: [[]]}", "y = 1"),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match(
            "x = {False: (True, 2.0, {}), 1: [[]], 2: 0}", "x", match_cases, ""
        )
        self.assertEqual(namespace.get("x"), {False: (True, 2.0, {}), 1: [[]], 2: 0})
        self.assertEqual(namespace.get("y"), 1)

    def test_mapping_11(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2, {}]}", "y = 0"),
            MatchCase("{0: [1, 2, {}], 1: [[]]}", "y = 1"),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match("x = []", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [])
        self.assertEqual(namespace.get("y"), 2)

    def test_mapping_12(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2, {}]}", "y = 0"),
            MatchCase(
                "{0: [1, 2, {}] or False} or {1: [[]]} or {0: [1, 2, {}]} or [] or 'X' or {}",
                "y = 1",
            ),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match("x = {0: 0}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 0})
        self.assertEqual(namespace.get("y"), 1)

    def test_mapping_13(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2, {}]}", "y = 0"),
            MatchCase(
                "{0: [1, 2, {}] or True} or {1: [[]]} or {0: [1, 2, {}]} or [] or 'X' or {}",
                "y = 1",
            ),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match("x = {0: 0}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 0})
        self.assertEqual(namespace.get("y"), 1)


class TestMatch(unittest.TestCase):

    def test_grammar_00(self) -> None:
        match 0:
            case 0:
                x = True
        self.assertEqual(x, True)

    def test_grammar_01(self) -> None:
        match 0:
            case 0 if False:
                x = False
            case 0 if True:
                x = True
        self.assertEqual(x, True)

    def test_grammar_02(self) -> None:
        match 0:
            case 0:
                x = True
            case 0:
                x = False
        self.assertEqual(x, True)

    def test_grammar_03(self) -> None:
        x = False
        match 0:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_grammar_04(self) -> None:
        x = False
        match 1:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_grammar_05(self) -> None:
        x = False
        match 2:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_grammar_06(self) -> None:
        x = False
        match 3:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_grammar_07(self) -> None:
        x = False
        match 4:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, False)

    def test_grammar_08(self) -> None:
        x = 0
        y = 1
        match x:
            case z := .y:
                pass
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        with self.assertRaises(NameError):
            z

    def test_grammar_09(self) -> None:
        class A:
            B = 0
        match 0:
            case x if x:
                z = 0
            case y := .x if y:
                z = 1
            case A.B:
                z = 2
        self.assertEqual(A.B, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 2)

    def test_grammar_10(self) -> None:
        match ():
            case []:
                x = 0
        self.assertEqual(x, 0)

    def test_grammar_11(self) -> None:
        match (0, 1, 2):
            case [*x]:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_grammar_12(self) -> None:
        match (0, 1, 2):
            case [0, *x]:
                y = 0
        self.assertEqual(x, [1, 2])
        self.assertEqual(y, 0)

    def test_grammar_13(self) -> None:
        match (0, 1, 2):
            case [0, 1, *x,]:
                y = 0
        self.assertEqual(x, [2])
        self.assertEqual(y, 0)

    def test_grammar_14(self) -> None:
        match (0, 1, 2):
            case [0, 1, 2, *x]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_grammar_15(self) -> None:
        match (0, 1, 2):
            case [*x, 2,]:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_grammar_16(self) -> None:
        match (0, 1, 2):
            case [*x, 1, 2]:
                y = 0
        self.assertEqual(x, [0])
        self.assertEqual(y, 0)

    def test_grammar_17(self) -> None:
        match (0, 1, 2):
            case [*x, 0, 1, 2,]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_grammar_18(self) -> None:
        match (0, 1, 2):
            case [0, *x, 2]:
                y = 0
        self.assertEqual(x, [1])
        self.assertEqual(y, 0)

    def test_grammar_19(self) -> None:
        match (0, 1, 2):
            case [0, 1, *x, 2,]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_grammar_20(self) -> None:
        match (0, 1, 2):
            case [0, *x, 1, 2]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_grammar_21(self) -> None:
        match (0, 1, 2):
            case [*x,]:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)


if __name__ == "__main__":  # XXX: For quick test debugging...
    import dis

    match_cases = [
        MatchCase("([0, 1]) or {'XXX': (1 or (z := 2))} or (0, q, [[[{}]]])", "pass")
    ]
    dis.dis(TestAST.compile_match("", "x", match_cases, ""))
