import sys
import unittest
import textwrap

class TestInvalidExceptStar(unittest.TestCase):
    def test_mixed_except_and_except_star_is_syntax_error(self):
        errors = [
            "try: pass\nexcept ValueError: pass\nexcept* TypeError: pass\n",
            "try: pass\nexcept* ValueError: pass\nexcept TypeError: pass\n",
            "try: pass\nexcept ValueError as e: pass\nexcept* TypeError: pass\n",
            "try: pass\nexcept* ValueError as e: pass\nexcept TypeError: pass\n",
            "try: pass\nexcept ValueError: pass\nexcept* TypeError as e: pass\n",
            "try: pass\nexcept* ValueError: pass\nexcept TypeError as e: pass\n",
            "try: pass\nexcept ValueError: pass\nexcept*: pass\n",
            "try: pass\nexcept* ValueError: pass\nexcept: pass\n",
        ]

        for err in errors:
            with self.assertRaises(SyntaxError):
                compile(err, "<string>", "exec")

    def test_except_star_ExceptionGroup_is_runtime_error_single(self):
        with self.assertRaises(TypeError):
            try:
                raise OSError("blah")
            except* ExceptionGroup as e:
                pass

    def test_except_star_ExceptionGroup_is_runtime_error_tuple(self):
        with self.assertRaises(TypeError):
            try:
                raise ExceptionGroup("eg", [ValueError(42)])
            except* (TypeError, ExceptionGroup):
                pass

    def test_except_star_invalid_exception_type(self):
        with self.assertRaises(TypeError):
            try:
                raise ValueError
            except* 42:
                pass

        with self.assertRaises(TypeError):
            try:
                raise ValueError
            except* (ValueError, 42):
                pass


class TestBreakContinueReturnInExceptStarBlock(unittest.TestCase):
    MSG = (r"'break', 'continue' and 'return'"
           r" cannot appear in an except\* block")

    def check_invalid(self, src):
        with self.assertRaisesRegex(SyntaxError, self.MSG):
            compile(textwrap.dedent(src), "<string>", "exec")

    def test_break_in_except_star(self):
        self.check_invalid(
            """
            try:
                raise ValueError
            except* Exception as e:
                break
            """)

        self.check_invalid(
            """
            for i in range(5):
                try:
                    pass
                except* Exception as e:
                    if i == 2:
                        break
            """)

        self.check_invalid(
            """
            for i in range(5):
                try:
                    pass
                except* Exception as e:
                    if i == 2:
                        break
                finally:
                    return 0
            """)


    def test_continue_in_except_star_block_invalid(self):
        self.check_invalid(
            """
            for i in range(5):
                try:
                    raise ValueError
                except* Exception as e:
                    continue
            """)

        self.check_invalid(
            """
            for i in range(5):
                try:
                    pass
                except* Exception as e:
                    if i == 2:
                        continue
            """)

        self.check_invalid(
            """
            for i in range(5):
                try:
                    pass
                except* Exception as e:
                    if i == 2:
                        continue
                finally:
                    return 0
            """)

    def test_return_in_except_star_block_invalid(self):
        self.check_invalid(
            """
            def f():
                try:
                    raise ValueError
                except* Exception as e:
                    return 42
            """)

        self.check_invalid(
            """
            def f():
                try:
                    pass
                except* Exception as e:
                    return 42
                finally:
                    finished = True
            """)

    def test_break_continue_in_except_star_block_valid(self):
        try:
            raise ValueError(42)
        except* Exception as e:
            count = 0
            for i in range(5):
                if i == 0:
                    continue
                if i == 4:
                    break
                count += 1

            self.assertEqual(count, 3)
            self.assertEqual(i, 4)
            exc = e
        self.assertIsInstance(exc, ExceptionGroup)

    def test_return_in_except_star_block_valid(self):
        try:
            raise ValueError(42)
        except* Exception as e:
            def f(x):
                return 2*x
            r = f(3)
            exc = e
        self.assertEqual(r, 6)
        self.assertIsInstance(exc, ExceptionGroup)


class ExceptStarTest(unittest.TestCase):
    def assertExceptionIsLike(self, exc, template):
        if exc is None and template is None:
            return

        if template is None:
            self.fail(f"unexpected exception: {exc}")

        if exc is None:
            self.fail(f"expected an exception like {template!r}, got None")

        if not isinstance(exc, ExceptionGroup):
            self.assertEqual(exc.__class__, template.__class__)
            self.assertEqual(exc.args[0], template.args[0])
        else:
            self.assertEqual(exc.message, template.message)
            self.assertEqual(len(exc.exceptions), len(template.exceptions))
            for e, t in zip(exc.exceptions, template.exceptions):
                self.assertExceptionIsLike(e, t)

    def assertMetadataEqual(self, e1, e2):
        if e1 is None or e2 is None:
            self.assertTrue(e1 is None and e2 is None)
        else:
            self.assertEqual(e1.__context__, e2.__context__)
            self.assertEqual(e1.__cause__, e2.__cause__)
            self.assertEqual(e1.__traceback__, e2.__traceback__)

    def assertMetadataNotEqual(self, e1, e2):
        if e1 is None or e2 is None:
            self.assertNotEqual(e1, e2)
        else:
            return not (e1.__context__ == e2.__context__
                        and e1.__cause__ == e2.__cause__
                        and e1.__traceback__ == e2.__traceback__)


class TestExceptStarSplitSemantics(ExceptStarTest):
    def doSplitTestNamed(self, exc, T, match_template, rest_template):
        initial_exc_info = sys.exc_info()
        exc_info = match = rest = None
        try:
            try:
                raise exc
            except* T as e:
                exc_info = sys.exc_info()
                match = e
        except BaseException as e:
            rest = e

        if match_template:
            self.assertEqual(exc_info[1], match)
        else:
            self.assertIsNone(exc_info)
        self.assertExceptionIsLike(match, match_template)
        self.assertExceptionIsLike(rest, rest_template)
        self.assertEqual(sys.exc_info(), initial_exc_info)

    def doSplitTestUnnamed(self, exc, T, match_template, rest_template):
        initial_exc_info = sys.exc_info()
        exc_info = match = rest = None
        try:
            try:
                raise exc
            except* T:
                exc_info = sys.exc_info()
                match = sys.exc_info()[1]
            else:
                if rest_template:
                    self.fail("Exception not raised")
        except BaseException as e:
            rest = e
        self.assertExceptionIsLike(match, match_template)
        if match_template:
            self.assertEqual(exc_info[0], type(match_template))
        self.assertExceptionIsLike(rest, rest_template)
        self.assertEqual(sys.exc_info(), initial_exc_info)

    def doSplitTestInExceptHandler(self, exc, T, match_template, rest_template):
        try:
            raise ExceptionGroup('eg', [TypeError(1), ValueError(2)])
        except Exception:
            self.doSplitTestNamed(exc, T, match_template, rest_template)
            self.doSplitTestUnnamed(exc, T, match_template, rest_template)

    def doSplitTestInExceptStarHandler(self, exc, T, match_template, rest_template):
        try:
            raise ExceptionGroup('eg', [TypeError(1), ValueError(2)])
        except* Exception:
            self.doSplitTestNamed(exc, T, match_template, rest_template)
            self.doSplitTestUnnamed(exc, T, match_template, rest_template)

    def doSplitTest(self, exc, T, match_template, rest_template):
        self.doSplitTestNamed(exc, T, match_template, rest_template)
        self.doSplitTestUnnamed(exc, T, match_template, rest_template)
        self.doSplitTestInExceptHandler(exc, T, match_template, rest_template)
        self.doSplitTestInExceptStarHandler(exc, T, match_template, rest_template)

    def test_no_match_single_type(self):
        self.doSplitTest(
            ExceptionGroup("test1", [ValueError("V"), TypeError("T")]),
            SyntaxError,
            None,
            ExceptionGroup("test1", [ValueError("V"), TypeError("T")]))

    def test_match_single_type(self):
        self.doSplitTest(
            ExceptionGroup("test2", [ValueError("V1"), ValueError("V2")]),
            ValueError,
            ExceptionGroup("test2", [ValueError("V1"), ValueError("V2")]),
            None)

    def test_match_single_type_partial_match(self):
        self.doSplitTest(
            ExceptionGroup(
                "test3",
                [ValueError("V1"), OSError("OS"), ValueError("V2")]),
            ValueError,
            ExceptionGroup("test3", [ValueError("V1"), ValueError("V2")]),
            ExceptionGroup("test3", [OSError("OS")]))

    def test_match_single_type_nested(self):
        self.doSplitTest(
            ExceptionGroup(
                "g1", [
                ValueError("V1"),
                OSError("OS1"),
                ExceptionGroup(
                    "g2", [
                    OSError("OS2"),
                    ValueError("V2"),
                    TypeError("T")])]),
            ValueError,
            ExceptionGroup(
                "g1", [
                ValueError("V1"),
                ExceptionGroup("g2", [ValueError("V2")])]),
            ExceptionGroup("g1", [
                OSError("OS1"),
                ExceptionGroup("g2", [
                    OSError("OS2"), TypeError("T")])]))

    def test_match_type_tuple_nested(self):
        self.doSplitTest(
            ExceptionGroup(
                "h1", [
                ValueError("V1"),
                OSError("OS1"),
                ExceptionGroup(
                    "h2", [OSError("OS2"), ValueError("V2"), TypeError("T")])]),
            (ValueError, TypeError),
            ExceptionGroup(
                "h1", [
                ValueError("V1"),
                ExceptionGroup("h2", [ValueError("V2"), TypeError("T")])]),
            ExceptionGroup(
                "h1", [
                OSError("OS1"),
                ExceptionGroup("h2", [OSError("OS2")])]))

    def test_empty_groups_removed(self):
        self.doSplitTest(
            ExceptionGroup(
                "eg", [
                ExceptionGroup("i1", [ValueError("V1")]),
                ExceptionGroup("i2", [ValueError("V2"), TypeError("T1")]),
                ExceptionGroup("i3", [TypeError("T2")])]),
            TypeError,
            ExceptionGroup("eg", [
                ExceptionGroup("i2", [TypeError("T1")]),
                ExceptionGroup("i3", [TypeError("T2")])]),
            ExceptionGroup("eg", [
                    ExceptionGroup("i1", [ValueError("V1")]),
                    ExceptionGroup("i2", [ValueError("V2")])]))

    def test_singleton_groups_are_kept(self):
        self.doSplitTest(
            ExceptionGroup("j1", [
                ExceptionGroup("j2", [
                    ExceptionGroup("j3", [ValueError("V1")]),
                    ExceptionGroup("j4", [TypeError("T")])])]),
            TypeError,
            ExceptionGroup(
                "j1",
                [ExceptionGroup("j2", [ExceptionGroup("j4", [TypeError("T")])])]),
            ExceptionGroup(
                "j1",
                [ExceptionGroup("j2", [ExceptionGroup("j3", [ValueError("V1")])])]))

    def test_naked_exception_matched_wrapped1(self):
        self.doSplitTest(
            ValueError("V"),
            ValueError,
            ExceptionGroup("", [ValueError("V")]),
            None)

    def test_naked_exception_matched_wrapped2(self):
        self.doSplitTest(
            ValueError("V"),
            Exception,
            ExceptionGroup("", [ValueError("V")]),
            None)

    def test_exception_group_except_star_Exception_not_wrapped(self):
        self.doSplitTest(
            ExceptionGroup("eg", [ValueError("V")]),
            Exception,
            ExceptionGroup("eg", [ValueError("V")]),
            None)

    def test_plain_exception_not_matched(self):
        self.doSplitTest(
            ValueError("V"),
            TypeError,
            None,
            ValueError("V"))

    def test_match__supertype(self):
        self.doSplitTest(
            ExceptionGroup("st", [BlockingIOError("io"), TypeError("T")]),
            OSError,
            ExceptionGroup("st", [BlockingIOError("io")]),
            ExceptionGroup("st", [TypeError("T")]))

    def test_multiple_matches_named(self):
        try:
            raise ExceptionGroup("mmn", [OSError("os"), BlockingIOError("io")])
        except* BlockingIOError as e:
            self.assertExceptionIsLike(e,
                ExceptionGroup("mmn", [BlockingIOError("io")]))
        except* OSError as e:
            self.assertExceptionIsLike(e,
                ExceptionGroup("mmn", [OSError("os")]))
        else:
            self.fail("Exception not raised")

    def test_multiple_matches_unnamed(self):
        try:
            raise ExceptionGroup("mmu", [OSError("os"), BlockingIOError("io")])
        except* BlockingIOError:
            e = sys.exc_info()[1]
            self.assertExceptionIsLike(e,
                ExceptionGroup("mmu", [BlockingIOError("io")]))
        except* OSError:
            e = sys.exc_info()[1]
            self.assertExceptionIsLike(e,
                ExceptionGroup("mmu", [OSError("os")]))
        else:
            self.fail("Exception not raised")

    def test_first_match_wins_named(self):
        try:
            raise ExceptionGroup("fst", [BlockingIOError("io")])
        except* OSError as e:
            self.assertExceptionIsLike(e,
                ExceptionGroup("fst", [BlockingIOError("io")]))
        except* BlockingIOError:
            self.fail("Should have been matched as OSError")
        else:
            self.fail("Exception not raised")

    def test_first_match_wins_unnamed(self):
        try:
            raise ExceptionGroup("fstu", [BlockingIOError("io")])
        except* OSError:
            e = sys.exc_info()[1]
            self.assertExceptionIsLike(e,
                ExceptionGroup("fstu", [BlockingIOError("io")]))
        except* BlockingIOError:
            pass
        else:
            self.fail("Exception not raised")

    def test_nested_except_stars(self):
        try:
            raise ExceptionGroup("n", [BlockingIOError("io")])
        except* BlockingIOError:
            try:
                raise ExceptionGroup("n", [ValueError("io")])
            except* ValueError:
                pass
            else:
                self.fail("Exception not raised")
            e = sys.exc_info()[1]
            self.assertExceptionIsLike(e,
                 ExceptionGroup("n", [BlockingIOError("io")]))
        else:
            self.fail("Exception not raised")

    def test_nested_in_loop(self):
        for _ in range(2):
            try:
                raise ExceptionGroup("nl", [BlockingIOError("io")])
            except* BlockingIOError:
                pass
            else:
                self.fail("Exception not raised")


class TestExceptStarReraise(ExceptStarTest):
    def test_reraise_all_named(self):
        try:
            try:
                raise ExceptionGroup(
                    "eg", [TypeError(1), ValueError(2), OSError(3)])
            except* TypeError as e:
                raise
            except* ValueError as e:
                raise
            # OSError not handled
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc,
            ExceptionGroup("eg", [TypeError(1), ValueError(2), OSError(3)]))

    def test_reraise_all_unnamed(self):
        try:
            try:
                raise ExceptionGroup(
                    "eg", [TypeError(1), ValueError(2), OSError(3)])
            except* TypeError:
                raise
            except* ValueError:
                raise
            # OSError not handled
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc,
            ExceptionGroup("eg", [TypeError(1), ValueError(2), OSError(3)]))

    def test_reraise_some_handle_all_named(self):
        try:
            try:
                raise ExceptionGroup(
                    "eg", [TypeError(1), ValueError(2), OSError(3)])
            except* TypeError as e:
                raise
            except* ValueError as e:
                pass
            # OSError not handled
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("eg", [TypeError(1), OSError(3)]))

    def test_reraise_partial_handle_all_unnamed(self):
        try:
            try:
                raise ExceptionGroup(
                    "eg", [TypeError(1), ValueError(2)])
            except* TypeError:
                raise
            except* ValueError:
                pass
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("eg", [TypeError(1)]))

    def test_reraise_partial_handle_some_named(self):
        try:
            try:
                raise ExceptionGroup(
                    "eg", [TypeError(1), ValueError(2), OSError(3)])
            except* TypeError as e:
                raise
            except* ValueError as e:
                pass
            # OSError not handled
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("eg", [TypeError(1), OSError(3)]))

    def test_reraise_partial_handle_some_unnamed(self):
        try:
            try:
                raise ExceptionGroup(
                    "eg", [TypeError(1), ValueError(2), OSError(3)])
            except* TypeError:
                raise
            except* ValueError:
                pass
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("eg", [TypeError(1), OSError(3)]))

    def test_reraise_plain_exception_named(self):
        try:
            try:
                raise ValueError(42)
            except* ValueError as e:
                raise
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [ValueError(42)]))

    def test_reraise_plain_exception_unnamed(self):
        try:
            try:
                raise ValueError(42)
            except* ValueError:
                raise
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [ValueError(42)]))


class TestExceptStarRaise(ExceptStarTest):
    def test_raise_named(self):
        orig = ExceptionGroup("eg", [ValueError(1), OSError(2)])
        try:
            try:
                raise orig
            except* OSError as e:
                raise TypeError(3)
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc,
            ExceptionGroup(
                "", [TypeError(3), ExceptionGroup("eg", [ValueError(1)])]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [OSError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)

    def test_raise_unnamed(self):
        orig = ExceptionGroup("eg", [ValueError(1), OSError(2)])
        try:
            try:
                raise orig
            except* OSError:
                raise TypeError(3)
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc,
            ExceptionGroup(
                "", [TypeError(3), ExceptionGroup("eg", [ValueError(1)])]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [OSError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)

    def test_raise_handle_all_raise_one_named(self):
        orig = ExceptionGroup("eg", [TypeError(1), ValueError(2)])
        try:
            try:
                raise orig
            except* (TypeError, ValueError) as e:
                raise SyntaxError(3)
        except BaseException as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [SyntaxError(3)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [TypeError(1), ValueError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)

    def test_raise_handle_all_raise_one_unnamed(self):
        orig = ExceptionGroup("eg", [TypeError(1), ValueError(2)])
        try:
            try:
                raise orig
            except* (TypeError, ValueError) as e:
                raise SyntaxError(3)
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [SyntaxError(3)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [TypeError(1), ValueError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)

    def test_raise_handle_all_raise_two_named(self):
        orig = ExceptionGroup("eg", [TypeError(1), ValueError(2)])
        try:
            try:
                raise orig
            except* TypeError as e:
                raise SyntaxError(3)
            except* ValueError as e:
                raise SyntaxError(4)
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [SyntaxError(3), SyntaxError(4)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [TypeError(1)]))

        self.assertExceptionIsLike(
            exc.exceptions[1].__context__,
            ExceptionGroup("eg", [ValueError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[1].__context__)

    def test_raise_handle_all_raise_two_unnamed(self):
        orig = ExceptionGroup("eg", [TypeError(1), ValueError(2)])
        try:
            try:
                raise orig
            except* TypeError:
                raise SyntaxError(3)
            except* ValueError:
                raise SyntaxError(4)
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [SyntaxError(3), SyntaxError(4)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [TypeError(1)]))

        self.assertExceptionIsLike(
            exc.exceptions[1].__context__,
            ExceptionGroup("eg", [ValueError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[1].__context__)


class TestExceptStarRaiseFrom(ExceptStarTest):
    def test_raise_named(self):
        orig = ExceptionGroup("eg", [ValueError(1), OSError(2)])
        try:
            try:
                raise orig
            except* OSError as e:
                raise TypeError(3) from e
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc,
            ExceptionGroup(
                "", [TypeError(3), ExceptionGroup("eg", [ValueError(1)])]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [OSError(2)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__cause__,
            ExceptionGroup("eg", [OSError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[0].__cause__)
        self.assertMetadataNotEqual(orig, exc.exceptions[1].__context__)
        self.assertMetadataNotEqual(orig, exc.exceptions[1].__cause__)

    def test_raise_unnamed(self):
        orig = ExceptionGroup("eg", [ValueError(1), OSError(2)])
        try:
            try:
                raise orig
            except* OSError:
                e = sys.exc_info()[1]
                raise TypeError(3) from e
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc,
            ExceptionGroup(
                "", [TypeError(3), ExceptionGroup("eg", [ValueError(1)])]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [OSError(2)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__cause__,
            ExceptionGroup("eg", [OSError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[0].__cause__)
        self.assertMetadataNotEqual(orig, exc.exceptions[1].__context__)
        self.assertMetadataNotEqual(orig, exc.exceptions[1].__cause__)

    def test_raise_handle_all_raise_one_named(self):
        orig = ExceptionGroup("eg", [TypeError(1), ValueError(2)])
        try:
            try:
                raise orig
            except* (TypeError, ValueError) as e:
                raise SyntaxError(3) from e
        except BaseException as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [SyntaxError(3)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [TypeError(1), ValueError(2)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__cause__,
            ExceptionGroup("eg", [TypeError(1), ValueError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[0].__cause__)

    def test_raise_handle_all_raise_one_unnamed(self):
        orig = ExceptionGroup("eg", [TypeError(1), ValueError(2)])
        try:
            try:
                raise orig
            except* (TypeError, ValueError) as e:
                e = sys.exc_info()[1]
                raise SyntaxError(3) from e
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [SyntaxError(3)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [TypeError(1), ValueError(2)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__cause__,
            ExceptionGroup("eg", [TypeError(1), ValueError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[0].__cause__)

    def test_raise_handle_all_raise_two_named(self):
        orig = ExceptionGroup("eg", [TypeError(1), ValueError(2)])
        try:
            try:
                raise orig
            except* TypeError as e:
                raise SyntaxError(3) from e
            except* ValueError as e:
                raise SyntaxError(4) from e
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [SyntaxError(3), SyntaxError(4)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [TypeError(1)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__cause__,
            ExceptionGroup("eg", [TypeError(1)]))

        self.assertExceptionIsLike(
            exc.exceptions[1].__context__,
            ExceptionGroup("eg", [ValueError(2)]))

        self.assertExceptionIsLike(
            exc.exceptions[1].__cause__,
            ExceptionGroup("eg", [ValueError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[0].__cause__)

    def test_raise_handle_all_raise_two_unnamed(self):
        orig = ExceptionGroup("eg", [TypeError(1), ValueError(2)])
        try:
            try:
                raise orig
            except* TypeError:
                e = sys.exc_info()[1]
                raise SyntaxError(3) from e
            except* ValueError:
                e = sys.exc_info()[1]
                raise SyntaxError(4) from e
        except ExceptionGroup as e:
            exc = e

        self.assertExceptionIsLike(
            exc, ExceptionGroup("", [SyntaxError(3), SyntaxError(4)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__context__,
            ExceptionGroup("eg", [TypeError(1)]))

        self.assertExceptionIsLike(
            exc.exceptions[0].__cause__,
            ExceptionGroup("eg", [TypeError(1)]))

        self.assertExceptionIsLike(
            exc.exceptions[1].__context__,
            ExceptionGroup("eg", [ValueError(2)]))

        self.assertExceptionIsLike(
            exc.exceptions[1].__cause__,
            ExceptionGroup("eg", [ValueError(2)]))

        self.assertMetadataNotEqual(orig, exc)
        self.assertMetadataEqual(orig, exc.exceptions[0].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[0].__cause__)
        self.assertMetadataEqual(orig, exc.exceptions[1].__context__)
        self.assertMetadataEqual(orig, exc.exceptions[1].__cause__)


class TestExceptStarExceptionGroupSubclass(ExceptStarTest):
    def test_except_star_EG_subclass(self):
        class EG(ExceptionGroup):
            def __new__(cls, message, excs, code):
                obj = super().__new__(cls, message, excs)
                obj.code = code
                return obj

            def derive(self, excs):
                return EG(self.message, excs, self.code)

        try:
            try:
                try:
                    try:
                        raise TypeError(2)
                    except TypeError as te:
                        raise EG("nested", [te], 101) from None
                except EG as nested:
                    try:
                        raise ValueError(1)
                    except ValueError as ve:
                        raise EG("eg", [ve, nested], 42)
            except* ValueError as eg:
                veg = eg
        except EG as eg:
            teg = eg

        self.assertIsInstance(veg, EG)
        self.assertIsInstance(teg, EG)
        self.assertIsInstance(teg.exceptions[0], EG)
        self.assertMetadataEqual(veg, teg)
        self.assertEqual(veg.code, 42)
        self.assertEqual(teg.code, 42)
        self.assertEqual(teg.exceptions[0].code, 101)


class TestExceptStarCleanup(ExceptStarTest):
    def test_exc_info_restored(self):
        try:
            try:
                raise ValueError(42)
            except:
                try:
                    raise TypeError(int)
                except* Exception:
                    pass
                1/0
        except Exception as e:
            exc = e

        self.assertExceptionIsLike(exc, ZeroDivisionError('division by zero'))
        self.assertExceptionIsLike(exc.__context__, ValueError(42))
        self.assertEqual(sys.exc_info(), (None, None, None))


if __name__ == '__main__':
    unittest.main()
