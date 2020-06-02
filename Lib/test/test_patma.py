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

    def test_patma_ast_00(self) -> None:
        match_cases = [MatchCase("0", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_01(self) -> None:
        match_cases = [MatchCase("False", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_02(self) -> None:
        match_cases = [MatchCase("1", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_patma_ast_03(self) -> None:
        match_cases = [MatchCase("None", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_patma_ast_04(self) -> None:
        match_cases = [MatchCase("0", "y = 0"), MatchCase("0", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_05(self) -> None:
        match_cases = [MatchCase("1", "y = 0"), MatchCase("1", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_patma_ast_06(self) -> None:
        match_cases = [MatchCase("'x'", "y = 0"), MatchCase("'y'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_07(self) -> None:
        match_cases = [MatchCase("'y'", "y = 0"), MatchCase("'x'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 1)

    def test_patma_ast_08(self) -> None:
        match_cases = [MatchCase("'x'", "y = 0"), MatchCase("'y'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_09(self) -> None:
        match_cases = [MatchCase("b'y'", "y = 0"), MatchCase("b'x'", "y = 1")]
        namespace = self.execute_match("x = b'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"x")
        self.assertEqual(namespace.get("y"), 1)

    def test_patma_ast_10(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "False"), MatchCase("0", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 1)

    def test_patma_ast_11(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "0"), MatchCase("0", "y = 1", "0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_patma_ast_12(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_13(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_14(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "y = 2")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 2)

    def test_patma_ast_15(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "0"), MatchCase("0", "y = 1", "1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "y = 2")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 2)

    def test_patma_ast_16(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "not (x := 1)"), MatchCase("1", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertNotIn("y", namespace)

    def test_patma_ast_17(self) -> None:
        match_cases = [
            MatchCase("0", "y = 0", "not (x := 1)"),
            MatchCase("(z := 0)", "y = 1"),
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 1)
        self.assertEqual(namespace.get("z"), 0)

    def test_patma_ast_18(self) -> None:
        match_cases = [
            MatchCase("(z := 1)", "y = 0", "not (x := 1)"),
            MatchCase("0", "y = 1"),
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 1)
        self.assertNotIn("z", namespace)

    def test_patma_ast_19(self) -> None:
        match_cases = [MatchCase("(z := 0)", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_patma_ast_20(self) -> None:
        match_cases = [MatchCase("(z := 1)", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)
        self.assertNotIn("z", namespace)

    def test_patma_ast_21(self) -> None:
        match_cases = [MatchCase("(z := 0)", "y = 0", "(w := 0)")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("w"), 0)
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)
        self.assertEqual(namespace.get("z"), 0)

    def test_patma_ast_22(self) -> None:
        match_cases = [MatchCase("(z := (w := 0))", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("w"), 0)
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_patma_ast_23(self) -> None:
        match_cases = [MatchCase("0 or 1 or 2", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_24(self) -> None:
        match_cases = [MatchCase("0 or 1 or 2", "y = 0")]
        namespace = self.execute_match("x = 1", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_25(self) -> None:
        match_cases = [MatchCase("0 or 1 or 2", "y = 0")]
        namespace = self.execute_match("x = 2", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 2)
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_26(self) -> None:
        match_cases = [MatchCase("0 or 1 or 2", "y = 0")]
        namespace = self.execute_match("x = 3", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 3)
        self.assertNotIn("y", namespace)

    def test_patma_ast_27(self) -> None:
        match_cases = [
            MatchCase("(z := 0) or (z := 1) or (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_patma_ast_28(self) -> None:
        match_cases = [
            MatchCase("(z := 0) or (z := 1) or (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 1", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 1)

    def test_patma_ast_29(self) -> None:
        match_cases = [
            MatchCase("(z := 0) or (z := 1) or (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 2", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 2)
        self.assertNotIn("y", namespace)
        self.assertEqual(namespace.get("z"), 2)

    def test_patma_ast_30(self) -> None:
        match_cases = [
            MatchCase("(z := 0) or (z := 1) or (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 3", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 3)
        self.assertNotIn("y", namespace)
        self.assertNotIn("z", namespace)

    def test_patma_ast_31(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = ()", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), ())
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_32(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = ()", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), ())
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_33(self) -> None:
        match_cases = [MatchCase("[0]", "y = 0")]
        namespace = self.execute_match("x = (0,)", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), (0,))
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_34(self) -> None:
        match_cases = [MatchCase("[[]]", "y = 0")]
        namespace = self.execute_match("x = ((),)", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), ((),))
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_35(self) -> None:
        match_cases = [MatchCase("[0, 1] or [1, 0]", "y = 0")]
        namespace = self.execute_match("x = [0, 1]", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [0, 1])
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_36(self) -> None:
        match_cases = [MatchCase("[0, 1] or [1, 0]", "y = 0")]
        namespace = self.execute_match("x = [1, 0]", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [1, 0])
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_37(self) -> None:
        match_cases = [MatchCase("[0, 1] or [1, 0]", "y = 0")]
        namespace = self.execute_match("x = [0, 0]", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [0, 0])
        self.assertNotIn("y", namespace)

    def test_patma_ast_38(self) -> None:
        match_cases = [
            MatchCase("[w := 0,]", "y = 0"),
            MatchCase("[z := 0] or [1, z := (0 or 1)] or [z := 0]", "y = 1"),
        ]
        namespace = self.execute_match("x = [1, 0]", "x", match_cases, "")
        self.assertNotIn("w", namespace)
        self.assertEqual(namespace.get("x"), [1, 0])
        self.assertEqual(namespace.get("y"), 1)
        self.assertEqual(namespace.get("z"), 0)

    def test_patma_ast_39(self) -> None:
        match_cases = [
            MatchCase("[0,]", "y = 0"),
            MatchCase("[1, 0]", "y = 1", "(x := x[:0])"),
            MatchCase("[1, 0]", "y = 2"),
        ]
        namespace = self.execute_match("x = [1, 0]", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [])
        self.assertEqual(namespace.get("y"), 2)

    def test_patma_ast_40(self) -> None:
        match_cases = [MatchCase("[0]", "y = 0")]
        namespace = self.execute_match("x = {0}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0})
        self.assertNotIn("y", namespace)

    def test_patma_ast_41(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = set()", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), set())
        self.assertNotIn("y", namespace)

    def test_patma_ast_42(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = iter([1,2,3])", "x", match_cases, "")
        self.assertEqual(list(namespace.get("x")), [1, 2, 3])
        self.assertNotIn("y", namespace)

    def test_patma_ast_43(self) -> None:
        match_cases = [MatchCase("[]", "y = 0")]
        namespace = self.execute_match("x = {}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {})
        self.assertNotIn("y", namespace)

    def test_patma_ast_44(self) -> None:
        match_cases = [MatchCase("[0, 1]", "y = 0")]
        namespace = self.execute_match("x = {0: False, 1: True}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: False, 1: True})
        self.assertNotIn("y", namespace)

    def test_patma_ast_45(self) -> None:
        match_cases = [MatchCase("['x']", "y = 0"), MatchCase("'x'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 1)

    def test_patma_ast_46(self) -> None:
        match_cases = [
            MatchCase("[b'x']", "y = 0"),
            MatchCase("['x']", "y = 1"),
            MatchCase("[120]", "y = 2"),
            MatchCase("b'x'", "y = 4"),
        ]
        namespace = self.execute_match("x = b'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"x")
        self.assertEqual(namespace.get("y"), 4)

    def test_patma_ast_47(self) -> None:
        match_cases = [MatchCase("[120]", "y = 0"), MatchCase("120", "y = 1")]
        namespace = self.execute_match("x = bytearray(b'x')", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"x")
        self.assertNotIn("y", namespace)

    def test_patma_ast_48(self) -> None:
        match_cases = [
            MatchCase("[]", "y = 0"),
            MatchCase("['']", "y = 1"),
            MatchCase("''", "y = 2"),
        ]
        namespace = self.execute_match("x = ''", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "")
        self.assertEqual(namespace.get("y"), 2)

    def test_patma_ast_49(self) -> None:
        match_cases = [
            MatchCase("['x', 'x', 'x']", "y = 0"),
            MatchCase("['xxx']", "y = 1"),
            MatchCase("'xxx'", "y = 2"),
        ]
        namespace = self.execute_match("x = 'xxx'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "xxx")
        self.assertEqual(namespace.get("y"), 2)

    def test_patma_ast_50(self) -> None:
        match_cases = [
            MatchCase("[120, 120, 120]", "y = 0"),
            MatchCase("[b'xxx']", "y = 1"),
            MatchCase("b'xxx'", "y = 2"),
        ]
        namespace = self.execute_match("x = b'xxx'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"xxx")
        self.assertEqual(namespace.get("y"), 2)

    def test_patma_ast_51(self) -> None:
        match_cases = [MatchCase("{}", "y = 0")]
        namespace = self.execute_match("x = {}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {})
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_52(self) -> None:
        match_cases = [MatchCase("{}", "y = 0")]
        namespace = self.execute_match("x = {0: 0}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 0})
        self.assertEqual(namespace.get("y"), 0)

    def test_patma_ast_53(self) -> None:
        match_cases = [MatchCase("{0: 0}", "y = 0")]
        namespace = self.execute_match("x = {}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {})
        self.assertNotIn("y", namespace)

    def test_patma_ast_54(self) -> None:
        match_cases = [MatchCase("{0: (z := (0 or 1 or 2))}", "y = 0")]
        namespace = self.execute_match("x = {0: 0}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 0})
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_patma_ast_55(self) -> None:
        match_cases = [MatchCase("{0: (z := (0 or 1 or 2))}", "y = 0")]
        namespace = self.execute_match("x = {0: 1}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 1})
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 1)

    def test_patma_ast_56(self) -> None:
        match_cases = [MatchCase("{0: (z := (0 or 1 or 2))}", "y = 0")]
        namespace = self.execute_match("x = {0: 2}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 2})
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 2)

    def test_patma_ast_57(self) -> None:
        match_cases = [MatchCase("{0: (z := (0 or 1 or 2))}", "y = 0")]
        namespace = self.execute_match("x = {0: 3}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {0: 3})
        self.assertNotIn("y", namespace)
        self.assertNotIn("z", namespace)

    def test_patma_ast_58(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2, {}]}", "y = 0"),
            MatchCase("{0: [1, 2, {}], 1: [[]]}", "y = 1"),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match("x = {}", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), {})
        self.assertNotIn("y", namespace)

    def test_patma_ast_59(self) -> None:
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

    def test_patma_ast_60(self) -> None:
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

    def test_patma_ast_61(self) -> None:
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

    def test_patma_ast_62(self) -> None:
        match_cases = [
            MatchCase("{0: [1, 2, {}]}", "y = 0"),
            MatchCase("{0: [1, 2, {}], 1: [[]]}", "y = 1"),
            MatchCase("[]", "y = 2"),
        ]
        namespace = self.execute_match("x = []", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), [])
        self.assertEqual(namespace.get("y"), 2)

    def test_patma_ast_63(self) -> None:
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

    def test_patma_ast_64(self) -> None:
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

    def test_patma_000(self) -> None:
        match 0:
            case 0:
                x = True
        self.assertEqual(x, True)

    def test_patma_001(self) -> None:
        match 0:
            case 0 if False:
                x = False
            case 0 if True:
                x = True
        self.assertEqual(x, True)

    def test_patma_002(self) -> None:
        match 0:
            case 0:
                x = True
            case 0:
                x = False
        self.assertEqual(x, True)

    def test_patma_003(self) -> None:
        x = False
        match 0:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_004(self) -> None:
        x = False
        match 1:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_005(self) -> None:
        x = False
        match 2:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_006(self) -> None:
        x = False
        match 3:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, True)

    def test_patma_007(self) -> None:
        x = False
        match 4:
            case 0 | 1 | 2 | 3:
                x = True
        self.assertEqual(x, False)

    def test_patma_008(self) -> None:
        x = 0
        y = 1
        z = None
        match x:
            case z := .y:
                pass
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, None)

    def test_patma_009(self) -> None:
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

    def test_patma_010(self) -> None:
        match ():
            case []:
                x = 0
        self.assertEqual(x, 0)

    def test_patma_011(self) -> None:
        match (0, 1, 2):
            case [*x]:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_012(self) -> None:
        match (0, 1, 2):
            case [0, *x]:
                y = 0
        self.assertEqual(x, [1, 2])
        self.assertEqual(y, 0)

    def test_patma_013(self) -> None:
        match (0, 1, 2):
            case [0, 1, *x,]:
                y = 0
        self.assertEqual(x, [2])
        self.assertEqual(y, 0)

    def test_patma_014(self) -> None:
        match (0, 1, 2):
            case [0, 1, 2, *x]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_015(self) -> None:
        match (0, 1, 2):
            case [*x, 2,]:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_patma_016(self) -> None:
        match (0, 1, 2):
            case [*x, 1, 2]:
                y = 0
        self.assertEqual(x, [0])
        self.assertEqual(y, 0)

    def test_patma_017(self) -> None:
        match (0, 1, 2):
            case [*x, 0, 1, 2,]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_018(self) -> None:
        match (0, 1, 2):
            case [0, *x, 2]:
                y = 0
        self.assertEqual(x, [1])
        self.assertEqual(y, 0)

    def test_patma_019(self) -> None:
        match (0, 1, 2):
            case [0, 1, *x, 2,]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_020(self) -> None:
        match (0, 1, 2):
            case [0, *x, 1, 2]:
                y = 0
        self.assertEqual(x, [])
        self.assertEqual(y, 0)

    def test_patma_021(self) -> None:
        match (0, 1, 2):
            case [*x,]:
                y = 0
        self.assertEqual(x, [0, 1, 2])
        self.assertEqual(y, 0)

    def test_patma_022(self) -> None:
        x = {}
        match x:
            case {}:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, 0)

    def test_patma_023(self) -> None:
        x = {0: 0}
        match x:
            case {}:
                y = 0
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 0)

    def test_patma_024(self) -> None:
        x = {}
        y = None
        match x:
            case {0: 0}:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, None)

    def test_patma_025(self) -> None:
        x = {0: 0}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_026(self) -> None:
        x = {0: 1}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 1})
        self.assertEqual(y, 0)
        self.assertEqual(z, 1)

    def test_patma_027(self) -> None:
        x = {0: 2}
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 2})
        self.assertEqual(y, 0)
        self.assertEqual(z, 2)

    def test_patma_028(self) -> None:
        x = {0: 3}
        y = None
        z = None
        match x:
            case {0: (z := 0 | 1 | 2)}:
                y = 0
        self.assertEqual(x, {0: 3})
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_029(self) -> None:
        x = {}
        y = None
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {})
        self.assertEqual(y, None)

    def test_patma_030(self) -> None:
        x = {False: (True, 2.0, {})}
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {False: (True, 2.0, {})})
        self.assertEqual(y, 0)

    def test_patma_031(self) -> None:
        x = {False: (True, 2.0, {}), 1: [[]], 2: 0}
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {False: (True, 2.0, {}), 1: [[]], 2: 0})
        self.assertEqual(y, 0)

    def test_patma_032(self) -> None:
        x = {False: (True, 2.0, {}), 1: [[]], 2: 0}
        match x:
            case {0: [1, 2]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {False: (True, 2.0, {}), 1: [[]], 2: 0})
        self.assertEqual(y, 1)

    def test_patma_033(self) -> None:
        x = []
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}], 1: [[]]}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, [])
        self.assertEqual(y, 2)

    def test_patma_034(self) -> None:
        x = {0: 0}
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: ([1, 2, {}] | False)} | {1: [[]]} | {0: [1, 2, {}]} | [] | 'X' | {}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 1)

    def test_patma_035(self) -> None:
        x = {0: 0}
        match x:
            case {0: [1, 2, {}]}:
                y = 0
            case {0: [1, 2, {}] | True} | {1: [[]]} | {0: [1, 2, {}]} | [] | 'X' | {}:
                y = 1
            case []:
                y = 2
        self.assertEqual(x, {0: 0})
        self.assertEqual(y, 1)

    def test_patma_036(self) -> None:
        x = 0
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_037(self) -> None:
        x = 1
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_038(self) -> None:
        x = 2
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_039(self) -> None:
        x = 3
        y = None
        match x:
            case 0 | 1 | 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_040(self) -> None:
        x = 0
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_041(self) -> None:
        x = 1
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)
        self.assertEqual(z, 1)

    def test_patma_042(self) -> None:
        x = 2
        y = None
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, None)
        self.assertEqual(z, 2)

    def test_patma_043(self) -> None:
        x = 3
        y = None
        z = None
        match x:
            case (z := 0) | (z := 1) | (z := 2) if z == x % 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_044(self) -> None:
        x = ()
        match x:
            case []:
                y = 0
        self.assertEqual(x, ())
        self.assertEqual(y, 0)

    def test_patma_045(self) -> None:
        x = ()
        match x:
            case []:
                y = 0
        self.assertEqual(x, ())
        self.assertEqual(y, 0)

    def test_patma_046(self) -> None:
        x = (0,)
        match x:
            case [0]:
                y = 0
        self.assertEqual(x, (0,))
        self.assertEqual(y, 0)

    def test_patma_047(self) -> None:
        x = ((),)
        match x:
            case [[]]:
                y = 0
        self.assertEqual(x, ((),))
        self.assertEqual(y, 0)

    def test_patma_048(self) -> None:
        x = [0, 1]
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [0, 1])
        self.assertEqual(y, 0)

    def test_patma_049(self) -> None:
        x = [1, 0]
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [1, 0])
        self.assertEqual(y, 0)

    def test_patma_050(self) -> None:
        x = [0, 0]
        y = None
        match x:
            case [0, 1] | [1, 0]:
                y = 0
        self.assertEqual(x, [0, 0])
        self.assertEqual(y, None)

    def test_patma_051(self) -> None:
        w = None
        x = [1, 0]
        match x:
            case [(w := 0)]:
                y = 0
            case [z] | [1, (z := 0 | 1)] | [z]:
                y = 1
        self.assertEqual(w, None)
        self.assertEqual(x, [1, 0])
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)

    def test_patma_052(self) -> None:
        x = [1, 0]
        match x:
            case [0]:
                y = 0
            case [1, 0] if (x := x[:0]):
                y = 1
            case [1, 0]:
                y = 2
        self.assertEqual(x, [])
        self.assertEqual(y, 2)

    def test_patma_053(self) -> None:
        x = {0}
        y = None
        match x:
            case [0]:
                y = 0
        self.assertEqual(x, {0})
        self.assertEqual(y, None)

    def test_patma_054(self) -> None:
        x = set()
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual(x, set())
        self.assertEqual(y, None)

    def test_patma_055(self) -> None:
        x = iter([1, 2, 3])
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual([*x], [1, 2, 3])
        self.assertEqual(y, None)

    def test_patma_056(self) -> None:
        x = {}
        y = None
        match x:
            case []:
                y = 0
        self.assertEqual(x, {})
        self.assertEqual(y, None)

    def test_patma_057(self) -> None:
        x = {0: False, 1: True}
        y = None
        match x:
            case [0, 1]:
                y = 0
        self.assertEqual(x, {0: False, 1: True})
        self.assertEqual(y, None)

    def test_patma_058(self) -> None:
        x = 0
        match x:
            case 0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_059(self) -> None:
        x = 0
        match x:
            case False:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_060(self) -> None:
        x = 0
        y = None
        match x:
            case 1:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_061(self) -> None:
        x = 0
        y = None
        match x:
            case None:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_062(self) -> None:
        x = 0
        match x:
            case 0:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_063(self) -> None:
        x = 0
        y = None
        match x:
            case 1:
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_064(self) -> None:
        x = 'x'
        match x:
            case 'x':
                y = 0
            case 'y':
                y = 1
        self.assertEqual(x, 'x')
        self.assertEqual(y, 0)

    def test_patma_065(self) -> None:
        x = 'x'
        match x:
            case 'y':
                y = 0
            case 'x':
                y = 1
        self.assertEqual(x, 'x')
        self.assertEqual(y, 1)

    def test_patma_066(self) -> None:
        x = 'x'
        match x:
            case 'x':
                y = 0
            case 'y':
                y = 1
        self.assertEqual(x, 'x')
        self.assertEqual(y, 0)

    def test_patma_067(self) -> None:
        x = b'x'
        match x:
            case b'y':
                y = 0
            case b'x':
                y = 1
        self.assertEqual(x, b'x')
        self.assertEqual(y, 1)

    def test_patma_068(self) -> None:
        x = 0
        match x:
            case 0 if False:
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)

    def test_patma_069(self) -> None:
        x = 0
        y = None
        match x:
            case 0 if 0:
                y = 0
            case 0 if 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, None)

    def test_patma_070(self) -> None:
        x = 0
        match x:
            case 0 if True:
                y = 0
            case 0 if True:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_071(self) -> None:
        x = 0
        match x:
            case 0 if True:
                y = 0
            case 0 if True:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_072(self) -> None:
        x = 0
        match x:
            case 0 if True:
                y = 0
            case 0 if True:
                y = 1
        y = 2
        self.assertEqual(x, 0)
        self.assertEqual(y, 2)

    def test_patma_073(self) -> None:
        x = 0
        match x:
            case 0 if 0:
                y = 0
            case 0 if 1:
                y = 1
        y = 2
        self.assertEqual(x, 0)
        self.assertEqual(y, 2)

    def test_patma_074(self) -> None:
        x = 0
        y = None
        match x:
            case 0 if not (x := 1):
                y = 0
            case 1:
                y = 1
        self.assertEqual(x, 1)
        self.assertEqual(y, None)

    def test_patma_075(self) -> None:
        x = 'x'
        match x:
            case ['x']:
                y = 0
            case 'x':
                y = 1
        self.assertEqual(x, 'x')
        self.assertEqual(y, 1)

    def test_patma_076(self) -> None:
        x = b'x'
        match x:
            case [b'x']:
                y = 0
            case ['x']:
                y = 1
            case [120]:
                y = 2
            case b'x':
                y = 4
        self.assertEqual(x, b'x')
        self.assertEqual(y, 4)

    def test_patma_077(self) -> None:
        x = bytearray(b'x')
        y = None
        match x:
            case [120]:
                y = 0
            case 120:
                y = 1
        self.assertEqual(x, b'x')
        self.assertEqual(y, None)

    def test_patma_078(self) -> None:
        x = ''
        match x:
            case []:
                y = 0
            case ['']:
                y = 1
            case '':
                y = 2
        self.assertEqual(x, '')
        self.assertEqual(y, 2)

    def test_patma_079(self) -> None:
        x = 'xxx'
        match x:
            case ['x', 'x', 'x']:
                y = 0
            case ['xxx']:
                y = 1
            case 'xxx':
                y = 2
        self.assertEqual(x, 'xxx')
        self.assertEqual(y, 2)

    def test_patma_080(self) -> None:
        x = b'xxx'
        match x:
            case [120, 120, 120]:
                y = 0
            case [b'xxx']:
                y = 1
            case b'xxx':
                y = 2
        self.assertEqual(x, b'xxx')
        self.assertEqual(y, 2)

    def test_patma_081(self) -> None:
        x = 0
        match x:
            case 0 if not (x := 1):
                y = 0
            case (z := 0):
                y = 1
        self.assertEqual(x, 1)
        self.assertEqual(y, 1)
        self.assertEqual(z, 0)

    def test_patma_082(self) -> None:
        x = 0
        z = None
        match x:
            case (z := 1) if not (x := 1):
                y = 0
            case 0:
                y = 1
        self.assertEqual(x, 0)
        self.assertEqual(y, 1)
        self.assertEqual(z, None)

    def test_patma_083(self) -> None:
        x = 0
        match x:
            case (z := 0):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_084(self) -> None:
        x = 0
        y = None
        z = None
        match x:
            case (z := 1):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, None)
        self.assertEqual(z, None)

    def test_patma_085(self) -> None:
        x = 0
        y = None
        match x:
            case (z := 0) if (w := 0):
                y = 0
        self.assertEqual(w, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, None)
        self.assertEqual(z, 0)

    def test_patma_086(self) -> None:
        x = 0
        match x:
            case (z := (w := 0)):
                y = 0
        self.assertEqual(w, 0)
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)
        self.assertEqual(z, 0)

    def test_patma_087(self) -> None:
        x = 0
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_088(self) -> None:
        x = 1
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_089(self) -> None:
        x = 2
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_090(self) -> None:
        x = 3
        y = None
        match x:
            case (0 | 1) | 2:
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_091(self) -> None:
        x = 0
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_092(self) -> None:
        x = 1
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 1)
        self.assertEqual(y, 0)

    def test_patma_093(self) -> None:
        x = 2
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 2)
        self.assertEqual(y, 0)

    def test_patma_094(self) -> None:
        x = 3
        y = None
        match x:
            case 0 | (1 | 2):
                y = 0
        self.assertEqual(x, 3)
        self.assertEqual(y, None)

    def test_patma_095(self) -> None:
        x = 0
        match x:
            case -0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_096(self) -> None:
        x = 0
        match x:
            case -0.0:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_097(self) -> None:
        x = 0
        match x:
            case -0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_098(self) -> None:
        x = 0
        match x:
            case -0.0j:
                y = 0
        self.assertEqual(x, 0)
        self.assertEqual(y, 0)

    def test_patma_099(self) -> None:
        x = -1
        match x:
            case -1:
                y = 0
        self.assertEqual(x, -1)
        self.assertEqual(y, 0)

    def test_patma_100(self) -> None:
        x = -1.5
        match x:
            case -1.5:
                y = 0
        self.assertEqual(x, -1.5)
        self.assertEqual(y, 0)

    def test_patma_101(self) -> None:
        x = -1j
        match x:
            case -1j:
                y = 0
        self.assertEqual(x, -1j)
        self.assertEqual(y, 0)

    def test_patma_102(self) -> None:
        x = -1.5j
        match x:
            case -1.5j:
                y = 0
        self.assertEqual(x, -1.5j)
        self.assertEqual(y, 0)



if __name__ == "__main__":  # XXX: For quick test debugging...
    import dis

    match_cases = [
        MatchCase("([0, 1]) or {'XXX': (1 or (z := 2))} or (0, q, [[[{}]]])", "pass")
    ]
    dis.dis(TestAST.compile_match("", "x", match_cases, ""))
