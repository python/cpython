import collections.abc
import traceback
import types
import unittest


class TestExceptionGroupTypeHierarchy(unittest.TestCase):
    def test_exception_group_types(self):
        self.assertTrue(issubclass(ExceptionGroup, Exception))
        self.assertTrue(issubclass(ExceptionGroup, BaseExceptionGroup))
        self.assertTrue(issubclass(BaseExceptionGroup, BaseException))


class BadConstructorArgs(unittest.TestCase):
    def test_bad_EG_construction__too_many_args(self):
        MSG = 'function takes exactly 2 arguments'
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup('no errors')
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup([ValueError('no msg')])
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup('eg', [ValueError('too')], [TypeError('many')])

    def test_bad_EG_construction__bad_message(self):
        MSG = 'Expected a message'
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup(ValueError(12), SyntaxError('bad syntax'))
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup(None, [ValueError(12)])

    def test_bad_EG_construction__bad_excs_sequence(self):
        MSG = 'Expected a non-empty sequence'
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup('errors not sequence', {ValueError(42)})
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup("eg", [])
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup("eg", None)

    def test_bad_EG_construction__nested_non_exceptions(self):
        MSG = 'Nested exceptions must derive from BaseException'
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup('bad error', ["not an exception"])
        with self.assertRaisesRegex(TypeError, MSG):
            _ = ExceptionGroup('eg1',
                               [ExceptionGroup('eg2', ["not an exception"])])


class InstanceCreation(unittest.TestCase):
    def test_EG_wraps_Exceptions__creates_EG(self):
        excs = [ValueError(1), TypeError(2)]
        self.assertIs(type(ExceptionGroup("eg", excs)), ExceptionGroup)

    def test_BEG_wraps_Exceptions__creates_EG(self):
        excs = [ValueError(1), TypeError(2)]
        self.assertIs(type(BaseExceptionGroup("beg", excs)), ExceptionGroup)

    def test_EG_wraps_BaseException__raises_TypeError(self):
        MSG= "Cannot nest BaseExceptions in an ExceptionGroup"
        with self.assertRaisesRegex(TypeError, MSG):
            eg = ExceptionGroup("eg", [ValueError(1), KeyboardInterrupt(2)])

    def test_BEG_wraps_BaseException__creates_BEG(self):
        beg = BaseExceptionGroup("beg", [ValueError(1), KeyboardInterrupt(2)])
        self.assertIs(type(beg), BaseExceptionGroup)

    def test_EG_subclass_wraps_anything(self):
        class MyEG(ExceptionGroup):
            pass
        self.assertIs(
            type(MyEG("eg", [ValueError(12), TypeError(42)])), MyEG)
        self.assertIs(
            type(MyEG("eg", [ValueError(12), KeyboardInterrupt(42)])), MyEG)

    def test_BEG_subclass_wraps_anything(self):
        class MyBEG(BaseExceptionGroup):
            pass
        self.assertIs(
            type(MyBEG("eg", [ValueError(12), TypeError(42)])), MyBEG)
        self.assertIs(
            type(MyBEG("eg", [ValueError(12), KeyboardInterrupt(42)])), MyBEG)


def leaf_generator(exc, tbs=None):
    if tbs is None:
        tbs = []
    tbs.append(exc.__traceback__)
    if isinstance(exc, BaseExceptionGroup):
        for e in exc.exceptions:
            yield from leaf_generator(e, tbs)
    else:
        # exc is a leaf exception and its traceback
        # is the concatenation of the traceback
        # segments in tbs
        yield exc, tbs
    tbs.pop()


def tb_funcnames(tbs):
    def names_list(tb):
        if tb is None:
            return None
        else:
            return [f.name for f in traceback.extract_tb(tb)]

    if isinstance(tbs, types.TracebackType):
        return names_list(tbs)
    else:
        return [names_list(tb) for tb in tbs]


def tbs_for_leaf(leaf, eg):
    for e, tbs in leaf_generator(eg):
        if e == leaf:
            return tbs


class ExceptionGroupTestBase(unittest.TestCase):
    def assertMatchesTemplate(self, exc, exc_type, template):
        """ Assert that the exception matches the template """
        if exc_type is not None:
            self.assertIs(type(exc), exc_type)

        if isinstance(exc, BaseExceptionGroup):
            self.assertIsInstance(template, collections.abc.Sequence)
            self.assertEqual(len(exc.exceptions), len(template))
            for e, t in zip(exc.exceptions, template):
                self.assertMatchesTemplate(e, None, t)
        else:
            self.assertIsInstance(template, BaseException)
            self.assertEqual(type(exc), type(template))
            self.assertEqual(exc.args, template.args)


class ExceptionGroupBasicsTests(ExceptionGroupTestBase):
    def setUp(self):
        # simple ExceptionGroup (without nesting)

        def do_raise(type, arg, cause=None):
            if cause is None:
                raise type(arg)
            else:
                raise type(arg) from cause

        def getTypeError(v):
            try:
                try:
                    do_raise(OSError, "context for TypeError")
                except:
                    do_raise(TypeError, v)
            except Exception as e:
                return e

        def getValueError(v):
            try:
                try:
                    do_raise(MemoryError, "context and cause for ValueError")
                except MemoryError as e:
                    do_raise(ValueError, v, cause=e)
            except Exception as e:
                return e

        def simple_group():
            try:
                raise ExceptionGroup(
                    'simple EG',
                    [getValueError(1), getTypeError(int), getValueError(2)])
            except ExceptionGroup as e:
                return e

        try:
            raise simple_group()
        except Exception as e:
            self.eg = e

        self.eg_template = [ValueError(1), TypeError(int), ValueError(2)]

        self.root_tb_fnames = ['setUp', 'simple_group']
        self.value_error_tb_fnames = ['getValueError', 'do_raise']
        self.type_error_tb_fnames = ['getTypeError', 'do_raise']

    def test_basics_ExceptionGroup_fields(self):
        eg = self.eg

        self.assertMatchesTemplate(eg, ExceptionGroup, self.eg_template)

        # check msg
        self.assertEqual(eg.message, 'simple EG')
        self.assertEqual(eg.args[0], 'simple EG')

        # check cause and context
        self.assertIsInstance(eg.exceptions[0], ValueError)
        self.assertIsInstance(eg.exceptions[0].__cause__, MemoryError)
        self.assertIsInstance(eg.exceptions[0].__context__, MemoryError)
        self.assertIsInstance(eg.exceptions[1], TypeError)
        self.assertIsNone(eg.exceptions[1].__cause__)
        self.assertIsInstance(eg.exceptions[1].__context__, OSError)
        self.assertIsInstance(eg.exceptions[2], ValueError)
        self.assertIsInstance(eg.exceptions[2].__cause__, MemoryError)
        self.assertIsInstance(eg.exceptions[2].__context__, MemoryError)

        # check tracebacks
        self.assertSequenceEqual(
            tb_funcnames(eg.__traceback__), self.root_tb_fnames)

        self.assertSequenceEqual(
            tb_funcnames(eg.exceptions[0].__traceback__),
            self.value_error_tb_fnames)
        self.assertSequenceEqual(
            tb_funcnames(eg.exceptions[1].__traceback__),
            self.type_error_tb_fnames)
        self.assertSequenceEqual(
            tb_funcnames(eg.exceptions[2].__traceback__),
            self.value_error_tb_fnames)


    def test_basics_ExceptionGroup_fields_are_read_only(self):
        eg = self.eg
        self.assertEqual(type(eg.exceptions), tuple)

        _ = eg.message
        with self.assertRaises(AttributeError):
            eg.message = "new msg"
        _ = eg.exceptions
        with self.assertRaises(AttributeError):
            eg.exceptions = [OSError('xyz')]

    def test_basics_leaf_generator(self):
        eg = self.eg
        self.assertSequenceEqual(
            [e for e, _ in leaf_generator(eg)], eg.exceptions)

        self.assertSequenceEqual(
            [tb_funcnames(tbs) for _, tbs in leaf_generator(eg)],
            [[self.root_tb_fnames, self.value_error_tb_fnames],
             [self.root_tb_fnames, self.type_error_tb_fnames],
             [self.root_tb_fnames, self.value_error_tb_fnames]])

    def test_basics_tbs_for_leaf(self):
        eg = self.eg
        for e, tbs in leaf_generator(eg):
            self.assertSequenceEqual(tbs, tbs_for_leaf(e, eg))

    def test_basics_subgroup_split__bad_arg_type(self):
        bad_args = ["bad arg",
                    OSError('instance not type'),
                    [OSError('instance not type')],]
        for arg in bad_args:
            with self.assertRaises(TypeError):
                self.eg.subgroup(arg)
            with self.assertRaises(TypeError):
                self.eg.split(arg)

    def test_basics_subgroup_by_type__passthrough(self):
        eg = self.eg
        self.assertIs(eg, eg.subgroup(BaseException))
        self.assertIs(eg, eg.subgroup(Exception))
        self.assertIs(eg, eg.subgroup(BaseExceptionGroup))
        self.assertIs(eg, eg.subgroup(ExceptionGroup))

    def test_basics_subgroup_by_type__no_match(self):
        self.assertIsNone(self.eg.subgroup(OSError))

    def test_basics_subgroup_by_type__match(self):
        eg = self.eg
        testcases = [
            # (match_type, result_template)
            (ValueError, [ValueError(1), ValueError(2)]),
            (TypeError, [TypeError(int)]),
            ((ValueError, TypeError), self.eg_template)]

        for match_type, template in testcases:
            subeg = eg.subgroup(match_type)
            self.assertEqual(subeg.message, eg.message)
            self.assertMatchesTemplate(subeg, ExceptionGroup, template)

    def test_basics_subgroup_by_predicate__passthrough(self):
        self.assertIs(self.eg, self.eg.subgroup(lambda e: True))

    def test_basics_subgroup_by_predicate__no_match(self):
        self.assertIsNone(self.eg.subgroup(lambda e: False))

    def test_basics_subgroup_by_predicate__match(self):
        eg = self.eg
        testcases = [
            # (match_type, result_template)
            (ValueError, [ValueError(1), ValueError(2)]),
            (TypeError, [TypeError(int)]),
            ((ValueError, TypeError), self.eg_template)]

        for match_type, template in testcases:
            subeg = eg.subgroup(lambda e: isinstance(e, match_type))
            self.assertEqual(subeg.message, eg.message)
            self.assertMatchesTemplate(subeg, ExceptionGroup, template)

    def test_basics_split_by_type__passthrough(self):
        for E in [BaseException, Exception,
                  BaseExceptionGroup, ExceptionGroup]:
            match, rest = self.eg.split(E)
            self.assertMatchesTemplate(match, ExceptionGroup, self.eg_template)
            self.assertIsNone(rest)

    def test_basics_split_by_type__no_match(self):
        match, rest = self.eg.split(OSError)
        self.assertIsNone(match)
        self.assertMatchesTemplate(rest, ExceptionGroup, self.eg_template)

    def test_basics_split_by_type__match(self):
        eg = self.eg
        testcases = [
            # (matcher, match_template, rest_template)
            (ValueError, [ValueError(1), ValueError(2)], [TypeError(int)]),
            (TypeError, [TypeError(int)], [ValueError(1), ValueError(2)]),
            ((ValueError, TypeError), self.eg_template, None)]
        for match_type, match_template, rest_template in testcases:
            match, rest = eg.split(match_type)
            self.assertEqual(match.message, eg.message)
            self.assertMatchesTemplate(
                match, ExceptionGroup, match_template)
            if rest_template is not None:
                self.assertEqual(rest.message, eg.message)
                self.assertMatchesTemplate(
                    rest, ExceptionGroup, rest_template)

    def test_basics_split_by_predicate__passthrough(self):
        match, rest = self.eg.split(lambda e: True)
        self.assertMatchesTemplate(match, ExceptionGroup, self.eg_template)
        self.assertIsNone(rest)

    def test_basics_split_by_predicate__no_match(self):
        match, rest = self.eg.split(lambda e: False)
        self.assertIsNone(match)
        self.assertMatchesTemplate(rest, ExceptionGroup, self.eg_template)

    def test_basics_split_by_predicate__match(self):
        eg = self.eg
        testcases = [
            # (matcher, match_template, rest_template)
            (ValueError, [ValueError(1), ValueError(2)], [TypeError(int)]),
            (TypeError, [TypeError(int)], [ValueError(1), ValueError(2)]),
            ((ValueError, TypeError), self.eg_template, None)]
        for match_type, match_template, rest_template in testcases:
            match, rest = eg.split(lambda e: isinstance(e, match_type))
            self.assertEqual(match.message, eg.message)
            self.assertMatchesTemplate(
                match, ExceptionGroup, match_template)
            if rest_template is not None:
                self.assertEqual(rest.message, eg.message)
                self.assertMatchesTemplate(
                    rest, ExceptionGroup, rest_template)


class NestedExceptionGroupBasicsTest(ExceptionGroupTestBase):
    def setUp(self):
        def raiseVE(v):
            raise ValueError(v)

        def raiseTE(t):
            raise TypeError(t)

        def nested_group():
            try:
                try:
                    raiseTE(bytes)
                except TypeError as e:
                    raise ExceptionGroup("nested", [e])
            except ExceptionGroup as e:
                excs = [e]
                try:
                    raiseVE(1)
                except ValueError as e:
                    excs.append(e)
                raise ExceptionGroup("root", excs)

        try:
            nested_group()
        except ExceptionGroup as eg:
            self.eg = eg

    def test_nested_group_matches_template(self):
        self.assertMatchesTemplate(
            self.eg, ExceptionGroup, [[TypeError(bytes)], ValueError(1)])

    def test_nested_group_tracebacks(self):
        eg = self.eg
        def check(exc, expected):
            self.assertEqual(tb_funcnames(exc.__traceback__), expected)

        check(eg, ['setUp', 'nested_group'])
        check(eg.exceptions[0], ['nested_group'])
        check(eg.exceptions[1], ['nested_group', 'raiseVE'])
        check(eg.exceptions[0].exceptions[0], ['nested_group', 'raiseTE'])

    def test_iteration_full_tracebacks(self):
        eg = self.eg
        # check that iteration over leaves
        # produces the expected tracebacks
        self.assertEqual(len(list(leaf_generator(eg))), 2)

        expected_tbs = [
            [['setUp', 'nested_group'],
             ['nested_group'],
             ['nested_group', 'raiseTE']],
            [['setUp', 'nested_group'],
             ['nested_group', 'raiseVE']]]

        for (i, (e, tbs)) in enumerate(leaf_generator(eg)):
            self.assertSequenceEqual(
                expected_tbs[i], [tb_funcnames(tb) for tb in tbs])


class ExceptionGroupSplitTests(ExceptionGroupTestBase):
    def _split_exception_group(self, eg, types):
        """ Split an EG and do some sanity checks on the result """
        self.assertIsInstance(eg, BaseExceptionGroup)

        match, rest = eg.split(types)
        sg = eg.subgroup(types)

        if match is not None:
            self.assertIsInstance(match, BaseExceptionGroup)
            for e,_ in leaf_generator(match):
                self.assertIsInstance(e, types)

            self.assertIsNotNone(sg)
            self.assertIsInstance(sg, BaseExceptionGroup)
            for e,_ in leaf_generator(sg):
                self.assertIsInstance(e, types)

        if rest is not None:
            self.assertIsInstance(rest, BaseExceptionGroup)

        def leaves(exc):
            return [] if exc is None else [e for e,_ in leaf_generator(exc)]

        # match and subgroup have the same leaves
        self.assertSequenceEqual(leaves(match), leaves(sg))

        match_leaves = leaves(match)
        rest_leaves = leaves(rest)
        # each leaf exception of eg and exactly one of match and rest
        self.assertEqual(len(leaves(eg)), len(leaves(match)) + len(leaves(rest)))
        for e in leaves(eg):
            self.assertNotEqual(
                match and e in match_leaves,
                rest and e in rest_leaves)

        # message, cause and context equal to eg
        for part in [match, rest, sg]:
            if part is not None:
                self.assertEqual(eg.message, part.message)
                self.assertIs(eg.__cause__, part.__cause__)
                self.assertIs(eg.__context__, part.__context__)
                self.assertIs(eg.__traceback__, part.__traceback__)

        # full tracebacks match
        for part in [match, rest, sg]:
            for e in leaves(part):
                self.assertSequenceEqual(
                    tb_funcnames(tbs_for_leaf(e, eg)),
                    tb_funcnames(tbs_for_leaf(e, part)))

        return match, rest

    def test_split_by_type(self):
        class MyExceptionGroup(ExceptionGroup):
            pass

        def raiseVE(v):
            raise ValueError(v)

        def raiseTE(t):
            raise TypeError(t)

        def nested_group():
            def level1(i):
                excs = []
                for f, arg in [(raiseVE, i), (raiseTE, int), (raiseVE, i+1)]:
                    try:
                        f(arg)
                    except Exception as e:
                        excs.append(e)
                raise ExceptionGroup('msg1', excs)

            def level2(i):
                excs = []
                for f, arg in [(level1, i), (level1, i+1), (raiseVE, i+2)]:
                    try:
                        f(arg)
                    except Exception as e:
                        excs.append(e)
                raise MyExceptionGroup('msg2', excs)

            def level3(i):
                excs = []
                for f, arg in [(level2, i+1), (raiseVE, i+2)]:
                    try:
                        f(arg)
                    except Exception as e:
                        excs.append(e)
                raise ExceptionGroup('msg3', excs)

            level3(5)

        try:
            nested_group()
        except ExceptionGroup as e:
            eg = e

        eg_template = [
            [
                [ValueError(6), TypeError(int), ValueError(7)],
                [ValueError(7), TypeError(int), ValueError(8)],
                ValueError(8),
            ],
            ValueError(7)]

        valueErrors_template = [
            [
                [ValueError(6), ValueError(7)],
                [ValueError(7), ValueError(8)],
                ValueError(8),
            ],
            ValueError(7)]

        typeErrors_template = [[[TypeError(int)], [TypeError(int)]]]

        self.assertMatchesTemplate(eg, ExceptionGroup, eg_template)

        # Match Nothing
        match, rest = self._split_exception_group(eg, SyntaxError)
        self.assertIsNone(match)
        self.assertMatchesTemplate(rest, ExceptionGroup, eg_template)

        # Match Everything
        match, rest = self._split_exception_group(eg, BaseException)
        self.assertMatchesTemplate(match, ExceptionGroup, eg_template)
        self.assertIsNone(rest)
        match, rest = self._split_exception_group(eg, (ValueError, TypeError))
        self.assertMatchesTemplate(match, ExceptionGroup, eg_template)
        self.assertIsNone(rest)

        # Match ValueErrors
        match, rest = self._split_exception_group(eg, ValueError)
        self.assertMatchesTemplate(match, ExceptionGroup, valueErrors_template)
        self.assertMatchesTemplate(rest, ExceptionGroup, typeErrors_template)

        # Match TypeErrors
        match, rest = self._split_exception_group(eg, (TypeError, SyntaxError))
        self.assertMatchesTemplate(match, ExceptionGroup, typeErrors_template)
        self.assertMatchesTemplate(rest, ExceptionGroup, valueErrors_template)

        # Match ExceptionGroup
        match, rest = eg.split(ExceptionGroup)
        self.assertIs(match, eg)
        self.assertIsNone(rest)

        # Match MyExceptionGroup (ExceptionGroup subclass)
        match, rest = eg.split(MyExceptionGroup)
        self.assertMatchesTemplate(match, ExceptionGroup, [eg_template[0]])
        self.assertMatchesTemplate(rest, ExceptionGroup, [eg_template[1]])

    def test_split_BaseExceptionGroup(self):
        def exc(ex):
            try:
                raise ex
            except BaseException as e:
                return e

        try:
            raise BaseExceptionGroup(
                "beg", [exc(ValueError(1)), exc(KeyboardInterrupt(2))])
        except BaseExceptionGroup as e:
            beg = e

        # Match Nothing
        match, rest = self._split_exception_group(beg, TypeError)
        self.assertIsNone(match)
        self.assertMatchesTemplate(
            rest, BaseExceptionGroup, [ValueError(1), KeyboardInterrupt(2)])

        # Match Everything
        match, rest = self._split_exception_group(beg, (ValueError, KeyboardInterrupt))
        self.assertMatchesTemplate(
            match, BaseExceptionGroup, [ValueError(1), KeyboardInterrupt(2)])
        self.assertIsNone(rest)

        # Match ValueErrors
        match, rest = self._split_exception_group(beg, ValueError)
        self.assertMatchesTemplate(match, ExceptionGroup, [ValueError(1)])
        self.assertMatchesTemplate(rest, BaseExceptionGroup, [KeyboardInterrupt(2)])

        # Match KeyboardInterrupts
        match, rest = self._split_exception_group(beg, KeyboardInterrupt)
        self.assertMatchesTemplate(match, BaseExceptionGroup, [KeyboardInterrupt(2)])
        self.assertMatchesTemplate(rest, ExceptionGroup, [ValueError(1)])

    def test_split_ExceptionGroup_subclass_no_derive_no_new_override(self):
        class EG(ExceptionGroup):
            pass

        try:
            try:
                try:
                    raise TypeError(2)
                except TypeError as te:
                    raise EG("nested", [te])
            except EG as nested:
                try:
                    raise ValueError(1)
                except ValueError as ve:
                    raise EG("eg", [ve, nested])
        except EG as e:
            eg = e

        self.assertMatchesTemplate(eg, EG, [ValueError(1), [TypeError(2)]])

        # Match Nothing
        match, rest = self._split_exception_group(eg, OSError)
        self.assertIsNone(match)
        self.assertMatchesTemplate(
            rest, ExceptionGroup, [ValueError(1), [TypeError(2)]])

        # Match Everything
        match, rest = self._split_exception_group(eg, (ValueError, TypeError))
        self.assertMatchesTemplate(
            match, ExceptionGroup, [ValueError(1), [TypeError(2)]])
        self.assertIsNone(rest)

        # Match ValueErrors
        match, rest = self._split_exception_group(eg, ValueError)
        self.assertMatchesTemplate(match, ExceptionGroup, [ValueError(1)])
        self.assertMatchesTemplate(rest, ExceptionGroup, [[TypeError(2)]])

        # Match TypeErrors
        match, rest = self._split_exception_group(eg, TypeError)
        self.assertMatchesTemplate(match, ExceptionGroup, [[TypeError(2)]])
        self.assertMatchesTemplate(rest, ExceptionGroup, [ValueError(1)])

    def test_split_BaseExceptionGroup_subclass_no_derive_new_override(self):
        class EG(BaseExceptionGroup):
            def __new__(cls, message, excs, unused):
                # The "unused" arg is here to show that split() doesn't call
                # the actual class constructor from the default derive()
                # implementation (it would fail on unused arg if so because
                # it assumes the BaseExceptionGroup.__new__ signature).
                return super().__new__(cls, message, excs)

        try:
            raise EG("eg", [ValueError(1), KeyboardInterrupt(2)], "unused")
        except EG as e:
            eg = e

        self.assertMatchesTemplate(
            eg, EG, [ValueError(1), KeyboardInterrupt(2)])

        # Match Nothing
        match, rest = self._split_exception_group(eg, OSError)
        self.assertIsNone(match)
        self.assertMatchesTemplate(
            rest, BaseExceptionGroup, [ValueError(1), KeyboardInterrupt(2)])

        # Match Everything
        match, rest = self._split_exception_group(eg, (ValueError, KeyboardInterrupt))
        self.assertMatchesTemplate(
            match, BaseExceptionGroup, [ValueError(1), KeyboardInterrupt(2)])
        self.assertIsNone(rest)

        # Match ValueErrors
        match, rest = self._split_exception_group(eg, ValueError)
        self.assertMatchesTemplate(match, ExceptionGroup, [ValueError(1)])
        self.assertMatchesTemplate(
            rest, BaseExceptionGroup, [KeyboardInterrupt(2)])

        # Match KeyboardInterrupt
        match, rest = self._split_exception_group(eg, KeyboardInterrupt)
        self.assertMatchesTemplate(
            match, BaseExceptionGroup, [KeyboardInterrupt(2)])
        self.assertMatchesTemplate(rest, ExceptionGroup, [ValueError(1)])

    def test_split_ExceptionGroup_subclass_derive_and_new_overrides(self):
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
                    raise TypeError(2)
                except TypeError as te:
                    raise EG("nested", [te], 101)
            except EG as nested:
                try:
                    raise ValueError(1)
                except ValueError as ve:
                    raise EG("eg", [ve, nested], 42)
        except EG as e:
            eg = e

        self.assertMatchesTemplate(eg, EG, [ValueError(1), [TypeError(2)]])

        # Match Nothing
        match, rest = self._split_exception_group(eg, OSError)
        self.assertIsNone(match)
        self.assertMatchesTemplate(rest, EG, [ValueError(1), [TypeError(2)]])
        self.assertEqual(rest.code, 42)
        self.assertEqual(rest.exceptions[1].code, 101)

        # Match Everything
        match, rest = self._split_exception_group(eg, (ValueError, TypeError))
        self.assertMatchesTemplate(match, EG, [ValueError(1), [TypeError(2)]])
        self.assertEqual(match.code, 42)
        self.assertEqual(match.exceptions[1].code, 101)
        self.assertIsNone(rest)

        # Match ValueErrors
        match, rest = self._split_exception_group(eg, ValueError)
        self.assertMatchesTemplate(match, EG, [ValueError(1)])
        self.assertEqual(match.code, 42)
        self.assertMatchesTemplate(rest, EG, [[TypeError(2)]])
        self.assertEqual(rest.code, 42)
        self.assertEqual(rest.exceptions[0].code, 101)

        # Match TypeErrors
        match, rest = self._split_exception_group(eg, TypeError)
        self.assertMatchesTemplate(match, EG, [[TypeError(2)]])
        self.assertEqual(match.code, 42)
        self.assertEqual(match.exceptions[0].code, 101)
        self.assertMatchesTemplate(rest, EG, [ValueError(1)])
        self.assertEqual(rest.code, 42)


if __name__ == '__main__':
    unittest.main()
