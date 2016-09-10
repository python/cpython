import io
import types
import textwrap
import unittest
import email.policy
import email.parser
import email.generator
import email.message
from email import headerregistry

def make_defaults(base_defaults, differences):
    defaults = base_defaults.copy()
    defaults.update(differences)
    return defaults

class PolicyAPITests(unittest.TestCase):

    longMessage = True

    # Base default values.
    compat32_defaults = {
        'max_line_length':          78,
        'linesep':                  '\n',
        'cte_type':                 '8bit',
        'raise_on_defect':          False,
        'mangle_from_':             True,
        'message_factory':          None,
        }
    # These default values are the ones set on email.policy.default.
    # If any of these defaults change, the docs must be updated.
    policy_defaults = compat32_defaults.copy()
    policy_defaults.update({
        'utf8':                     False,
        'raise_on_defect':          False,
        'header_factory':           email.policy.EmailPolicy.header_factory,
        'refold_source':            'long',
        'content_manager':          email.policy.EmailPolicy.content_manager,
        'mangle_from_':             False,
        'message_factory':          email.message.EmailMessage,
        })

    # For each policy under test, we give here what we expect the defaults to
    # be for that policy.  The second argument to make defaults is the
    # difference between the base defaults and that for the particular policy.
    new_policy = email.policy.EmailPolicy()
    policies = {
        email.policy.compat32: make_defaults(compat32_defaults, {}),
        email.policy.default: make_defaults(policy_defaults, {}),
        email.policy.SMTP: make_defaults(policy_defaults,
                                         {'linesep': '\r\n'}),
        email.policy.SMTPUTF8: make_defaults(policy_defaults,
                                             {'linesep': '\r\n',
                                              'utf8': True}),
        email.policy.HTTP: make_defaults(policy_defaults,
                                         {'linesep': '\r\n',
                                          'max_line_length': None}),
        email.policy.strict: make_defaults(policy_defaults,
                                           {'raise_on_defect': True}),
        new_policy: make_defaults(policy_defaults, {}),
        }
    # Creating a new policy creates a new header factory.  There is a test
    # later that proves this.
    policies[new_policy]['header_factory'] = new_policy.header_factory

    def test_defaults(self):
        for policy, expected in self.policies.items():
            for attr, value in expected.items():
                with self.subTest(policy=policy, attr=attr):
                    self.assertEqual(getattr(policy, attr), value,
                                    ("change {} docs/docstrings if defaults have "
                                    "changed").format(policy))

    def test_all_attributes_covered(self):
        for policy, expected in self.policies.items():
            for attr in dir(policy):
                with self.subTest(policy=policy, attr=attr):
                    if (attr.startswith('_') or
                            isinstance(getattr(email.policy.EmailPolicy, attr),
                                  types.FunctionType)):
                        continue
                    else:
                        self.assertIn(attr, expected,
                                      "{} is not fully tested".format(attr))

    def test_abc(self):
        with self.assertRaises(TypeError) as cm:
            email.policy.Policy()
        msg = str(cm.exception)
        abstract_methods = ('fold',
                            'fold_binary',
                            'header_fetch_parse',
                            'header_source_parse',
                            'header_store_parse')
        for method in abstract_methods:
            self.assertIn(method, msg)

    def test_policy_is_immutable(self):
        for policy, defaults in self.policies.items():
            for attr in defaults:
                with self.assertRaisesRegex(AttributeError, attr+".*read-only"):
                    setattr(policy, attr, None)
            with self.assertRaisesRegex(AttributeError, 'no attribute.*foo'):
                policy.foo = None

    def test_set_policy_attrs_when_cloned(self):
        # None of the attributes has a default value of None, so we set them
        # all to None in the clone call and check that it worked.
        for policyclass, defaults in self.policies.items():
            testattrdict = {attr: None for attr in defaults}
            policy = policyclass.clone(**testattrdict)
            for attr in defaults:
                self.assertIsNone(getattr(policy, attr))

    def test_reject_non_policy_keyword_when_called(self):
        for policyclass in self.policies:
            with self.assertRaises(TypeError):
                policyclass(this_keyword_should_not_be_valid=None)
            with self.assertRaises(TypeError):
                policyclass(newtline=None)

    def test_policy_addition(self):
        expected = self.policy_defaults.copy()
        p1 = email.policy.default.clone(max_line_length=100)
        p2 = email.policy.default.clone(max_line_length=50)
        added = p1 + p2
        expected.update(max_line_length=50)
        for attr, value in expected.items():
            self.assertEqual(getattr(added, attr), value)
        added = p2 + p1
        expected.update(max_line_length=100)
        for attr, value in expected.items():
            self.assertEqual(getattr(added, attr), value)
        added = added + email.policy.default
        for attr, value in expected.items():
            self.assertEqual(getattr(added, attr), value)

    def test_register_defect(self):
        class Dummy:
            def __init__(self):
                self.defects = []
        obj = Dummy()
        defect = object()
        policy = email.policy.EmailPolicy()
        policy.register_defect(obj, defect)
        self.assertEqual(obj.defects, [defect])
        defect2 = object()
        policy.register_defect(obj, defect2)
        self.assertEqual(obj.defects, [defect, defect2])

    class MyObj:
        def __init__(self):
            self.defects = []

    class MyDefect(Exception):
        pass

    def test_handle_defect_raises_on_strict(self):
        foo = self.MyObj()
        defect = self.MyDefect("the telly is broken")
        with self.assertRaisesRegex(self.MyDefect, "the telly is broken"):
            email.policy.strict.handle_defect(foo, defect)

    def test_handle_defect_registers_defect(self):
        foo = self.MyObj()
        defect1 = self.MyDefect("one")
        email.policy.default.handle_defect(foo, defect1)
        self.assertEqual(foo.defects, [defect1])
        defect2 = self.MyDefect("two")
        email.policy.default.handle_defect(foo, defect2)
        self.assertEqual(foo.defects, [defect1, defect2])

    class MyPolicy(email.policy.EmailPolicy):
        defects = None
        def __init__(self, *args, **kw):
            super().__init__(*args, defects=[], **kw)
        def register_defect(self, obj, defect):
            self.defects.append(defect)

    def test_overridden_register_defect_still_raises(self):
        foo = self.MyObj()
        defect = self.MyDefect("the telly is broken")
        with self.assertRaisesRegex(self.MyDefect, "the telly is broken"):
            self.MyPolicy(raise_on_defect=True).handle_defect(foo, defect)

    def test_overridden_register_defect_works(self):
        foo = self.MyObj()
        defect1 = self.MyDefect("one")
        my_policy = self.MyPolicy()
        my_policy.handle_defect(foo, defect1)
        self.assertEqual(my_policy.defects, [defect1])
        self.assertEqual(foo.defects, [])
        defect2 = self.MyDefect("two")
        my_policy.handle_defect(foo, defect2)
        self.assertEqual(my_policy.defects, [defect1, defect2])
        self.assertEqual(foo.defects, [])

    def test_default_header_factory(self):
        h = email.policy.default.header_factory('Test', 'test')
        self.assertEqual(h.name, 'Test')
        self.assertIsInstance(h, headerregistry.UnstructuredHeader)
        self.assertIsInstance(h, headerregistry.BaseHeader)

    class Foo:
        parse = headerregistry.UnstructuredHeader.parse

    def test_each_Policy_gets_unique_factory(self):
        policy1 = email.policy.EmailPolicy()
        policy2 = email.policy.EmailPolicy()
        policy1.header_factory.map_to_type('foo', self.Foo)
        h = policy1.header_factory('foo', 'test')
        self.assertIsInstance(h, self.Foo)
        self.assertNotIsInstance(h, headerregistry.UnstructuredHeader)
        h = policy2.header_factory('foo', 'test')
        self.assertNotIsInstance(h, self.Foo)
        self.assertIsInstance(h, headerregistry.UnstructuredHeader)

    def test_clone_copies_factory(self):
        policy1 = email.policy.EmailPolicy()
        policy2 = policy1.clone()
        policy1.header_factory.map_to_type('foo', self.Foo)
        h = policy1.header_factory('foo', 'test')
        self.assertIsInstance(h, self.Foo)
        h = policy2.header_factory('foo', 'test')
        self.assertIsInstance(h, self.Foo)

    def test_new_factory_overrides_default(self):
        mypolicy = email.policy.EmailPolicy()
        myfactory = mypolicy.header_factory
        newpolicy = mypolicy + email.policy.strict
        self.assertEqual(newpolicy.header_factory, myfactory)
        newpolicy = email.policy.strict + mypolicy
        self.assertEqual(newpolicy.header_factory, myfactory)

    def test_adding_default_policies_preserves_default_factory(self):
        newpolicy = email.policy.default + email.policy.strict
        self.assertEqual(newpolicy.header_factory,
                         email.policy.EmailPolicy.header_factory)
        self.assertEqual(newpolicy.__dict__, {'raise_on_defect': True})

    # XXX: Need subclassing tests.
    # For adding subclassed objects, make sure the usual rules apply (subclass
    # wins), but that the order still works (right overrides left).


class TestException(Exception):
    pass

class TestPolicyPropagation(unittest.TestCase):

    # The abstract methods are used by the parser but not by the wrapper
    # functions that call it, so if the exception gets raised we know that the
    # policy was actually propagated all the way to feedparser.
    class MyPolicy(email.policy.Policy):
        def badmethod(self, *args, **kw):
            raise TestException("test")
        fold = fold_binary = header_fetch_parser = badmethod
        header_source_parse = header_store_parse = badmethod

    def test_message_from_string(self):
        with self.assertRaisesRegex(TestException, "^test$"):
            email.message_from_string("Subject: test\n\n",
                                      policy=self.MyPolicy)

    def test_message_from_bytes(self):
        with self.assertRaisesRegex(TestException, "^test$"):
            email.message_from_bytes(b"Subject: test\n\n",
                                     policy=self.MyPolicy)

    def test_message_from_file(self):
        f = io.StringIO('Subject: test\n\n')
        with self.assertRaisesRegex(TestException, "^test$"):
            email.message_from_file(f, policy=self.MyPolicy)

    def test_message_from_binary_file(self):
        f = io.BytesIO(b'Subject: test\n\n')
        with self.assertRaisesRegex(TestException, "^test$"):
            email.message_from_binary_file(f, policy=self.MyPolicy)

    # These are redundant, but we need them for black-box completeness.

    def test_parser(self):
        p = email.parser.Parser(policy=self.MyPolicy)
        with self.assertRaisesRegex(TestException, "^test$"):
            p.parsestr('Subject: test\n\n')

    def test_bytes_parser(self):
        p = email.parser.BytesParser(policy=self.MyPolicy)
        with self.assertRaisesRegex(TestException, "^test$"):
            p.parsebytes(b'Subject: test\n\n')

    # Now that we've established that all the parse methods get the
    # policy in to feedparser, we can use message_from_string for
    # the rest of the propagation tests.

    def _make_msg(self, source='Subject: test\n\n', policy=None):
        self.policy = email.policy.default.clone() if policy is None else policy
        return email.message_from_string(source, policy=self.policy)

    def test_parser_propagates_policy_to_message(self):
        msg = self._make_msg()
        self.assertIs(msg.policy, self.policy)

    def test_parser_propagates_policy_to_sub_messages(self):
        msg = self._make_msg(textwrap.dedent("""\
            Subject: mime test
            MIME-Version: 1.0
            Content-Type: multipart/mixed, boundary="XXX"

            --XXX
            Content-Type: text/plain

            test
            --XXX
            Content-Type: text/plain

            test2
            --XXX--
            """))
        for part in msg.walk():
            self.assertIs(part.policy, self.policy)

    def test_message_policy_propagates_to_generator(self):
        msg = self._make_msg("Subject: test\nTo: foo\n\n",
                             policy=email.policy.default.clone(linesep='X'))
        s = io.StringIO()
        g = email.generator.Generator(s)
        g.flatten(msg)
        self.assertEqual(s.getvalue(), "Subject: testXTo: fooXX")

    def test_message_policy_used_by_as_string(self):
        msg = self._make_msg("Subject: test\nTo: foo\n\n",
                             policy=email.policy.default.clone(linesep='X'))
        self.assertEqual(msg.as_string(), "Subject: testXTo: fooXX")


class TestConcretePolicies(unittest.TestCase):

    def test_header_store_parse_rejects_newlines(self):
        instance = email.policy.EmailPolicy()
        self.assertRaises(ValueError,
                          instance.header_store_parse,
                          'From', 'spam\negg@foo.py')


if __name__ == '__main__':
    unittest.main()
