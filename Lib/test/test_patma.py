import ast
import typing
import unittest


FILENAME = "<patma tests>"


class MatchCase(typing.NamedTuple):
    pattern: str
    body: str
    guard: typing.Optional[str] = None


class TestAST(unittest.TestCase):
    """Tests that predate parser support, and just execute ASTs instead."""

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
    def execute_match(
        cls, pre: str, target: str, match_cases: typing.Iterable[MatchCase], post: str
    ) -> typing.Dict[str, typing.Any]:
        cases = [cls.parse_match_case(case) for case in match_cases]
        match = ast.Match(target=cls.parse_expr(target), cases=cases)
        body = [*cls.parse_stmts(pre), match, *cls.parse_stmts(post)]
        tree = ast.fix_missing_locations(ast.Module(body=body, type_ignores=[]))
        namespace = {}
        exec(compile(tree, FILENAME, "exec"), None, namespace)
        return namespace

    def test_steroid_switch_0(self) -> None:
        match_cases = [MatchCase("0", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_1(self) -> None:
        match_cases = [MatchCase("False", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_2(self) -> None:
        match_cases = [MatchCase("1", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_steroid_switch_3(self) -> None:
        match_cases = [MatchCase("None", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_steroid_switch_4(self) -> None:
        match_cases = [MatchCase("0", "y = 0"), MatchCase("0", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_5(self) -> None:
        match_cases = [MatchCase("1", "y = 0"), MatchCase("1", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_steroid_switch_6(self) -> None:
        match_cases = [MatchCase("'x'", "y = 0"), MatchCase("'y'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_7(self) -> None:
        match_cases = [MatchCase("'y'", "y = 0"), MatchCase("'x'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 1)

    def test_steroid_switch_8(self) -> None:
        match_cases = [MatchCase("'x'", "y = 0"), MatchCase("'y'", "y = 1")]
        namespace = self.execute_match("x = 'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), "x")
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_9(self) -> None:
        match_cases = [MatchCase("b'y'", "y = 0"), MatchCase("b'x'", "y = 1")]
        namespace = self.execute_match("x = b'x'", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), b"x")
        self.assertEqual(namespace.get("y"), 1)

    def test_steroid_switch_guard_0(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "False"), MatchCase("0", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 1)

    def test_steroid_switch_guard_1(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "0"), MatchCase("0", "y = 1", "0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)

    def test_steroid_switch_guard_2(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_guard_3(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_steroid_switch_guard_4(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "True"), MatchCase("0", "y = 1", "True")]
        namespace = self.execute_match("x = 0", "x", match_cases, "y = 2")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 2)

    def test_steroid_switch_guard_5(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "0"), MatchCase("0", "y = 1", "1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "y = 2")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 2)

    def test_steroid_switch_guard_6(self) -> None:
        match_cases = [MatchCase("0", "y = 0", "not (x := 1)"), MatchCase("1", "y = 1")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertNotIn("y", namespace)

    def test_walrus_0(self) -> None:
        match_cases = [
            MatchCase("0", "y = 0", "not (x := 1)"),
            MatchCase("(z := 0)", "y = 1"),
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 1)
        self.assertEqual(namespace.get("z"), 0)

    def test_walrus_1(self) -> None:
        match_cases = [
            MatchCase("(z := 1)", "y = 0", "not (x := 1)"),
            MatchCase("0", "y = 1"),
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 1)
        self.assertNotIn("z", namespace)

    def test_walrus_2(self) -> None:
        match_cases = [MatchCase("(z := 0)", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_walrus_3(self) -> None:
        match_cases = [MatchCase("(z := 1)", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)
        self.assertNotIn("z", namespace)

    def test_walrus_4(self) -> None:
        match_cases = [MatchCase("(z := 0)", "y = 0", "(w := 0)")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("w"), 0)
        self.assertEqual(namespace.get("x"), 0)
        self.assertNotIn("y", namespace)
        self.assertEqual(namespace.get("z"), 0)

    def test_walrus_5(self) -> None:
        match_cases = [MatchCase("(z := (w := 0))", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("w"), 0)
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_pipe_0(self) -> None:
        match_cases = [MatchCase("0 | 1 | 2", "y = 0")]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)

    def test_pipe_1(self) -> None:
        match_cases = [MatchCase("0 | 1 | 2", "y = 0")]
        namespace = self.execute_match("x = 1", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 0)

    def test_pipe_2(self) -> None:
        match_cases = [MatchCase("0 | 1 | 2", "y = 0")]
        namespace = self.execute_match("x = 2", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 2)
        self.assertEqual(namespace.get("y"), 0)

    def test_pipe_3(self) -> None:
        match_cases = [MatchCase("0 | 1 | 2", "y = 0")]
        namespace = self.execute_match("x = 3", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 3)
        self.assertNotIn("y", namespace)

    def test_pipe_4(self) -> None:
        match_cases = [
            MatchCase("(z := 0) | (z := 1) | (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 0", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 0)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 0)

    def test_pipe_5(self) -> None:
        match_cases = [
            MatchCase("(z := 0) | (z := 1) | (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 1", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 1)
        self.assertEqual(namespace.get("y"), 0)
        self.assertEqual(namespace.get("z"), 1)

    def test_pipe_6(self) -> None:
        match_cases = [
            MatchCase("(z := 0) | (z := 1) | (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 2", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 2)
        self.assertNotIn("y", namespace)
        self.assertEqual(namespace.get("z"), 2)

    def test_pipe_7(self) -> None:
        match_cases = [
            MatchCase("(z := 0) | (z := 1) | (z := 2)", "y = 0", "z == x % 2")
        ]
        namespace = self.execute_match("x = 3", "x", match_cases, "")
        self.assertEqual(namespace.get("x"), 3)
        self.assertNotIn("y", namespace)
        self.assertNotIn("z", namespace)
