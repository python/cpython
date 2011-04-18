import types
import unittest
import email.policy

class PolicyAPITests(unittest.TestCase):

    longMessage = True

    # These default values are the ones set on email.policy.default.
    # If any of these defaults change, the docs must be updated.
    policy_defaults = {
        'max_line_length':          78,
        'linesep':                  '\n',
        'must_be_7bit':             False,
        'raise_on_defect':          False,
        }

    # For each policy under test, we give here the values of the attributes
    # that are different from the defaults for that policy.
    policies = {
        email.policy.Policy(): {},
        email.policy.default: {},
        email.policy.SMTP: {'linesep': '\r\n'},
        email.policy.HTTP: {'linesep': '\r\n', 'max_line_length': None},
        email.policy.strict: {'raise_on_defect': True},
        }

    def test_defaults(self):
        for policy, changed_defaults in self.policies.items():
            expected = self.policy_defaults.copy()
            expected.update(changed_defaults)
            for attr, value in expected.items():
                self.assertEqual(getattr(policy, attr), value,
                                ("change {} docs/docstrings if defaults have "
                                "changed").format(policy))

    def test_all_attributes_covered(self):
        for attr in dir(email.policy.default):
            if (attr.startswith('_') or
               isinstance(getattr(email.policy.Policy, attr),
                          types.FunctionType)):
                continue
            else:
                self.assertIn(attr, self.policy_defaults,
                              "{} is not fully tested".format(attr))

    def test_policy_is_immutable(self):
        for policy in self.policies:
            for attr in self.policy_defaults:
                with self.assertRaisesRegex(AttributeError, attr+".*read-only"):
                    setattr(policy, attr, None)
            with self.assertRaisesRegex(AttributeError, 'no attribute.*foo'):
                policy.foo = None

    def test_set_policy_attrs_when_calledl(self):
        testattrdict = { attr: None for attr in self.policy_defaults }
        for policyclass in self.policies:
            policy = policyclass.clone(**testattrdict)
            for attr in self.policy_defaults:
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
        policy = email.policy.Policy()
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

    class MyPolicy(email.policy.Policy):
        defects = []
        def register_defect(self, obj, defect):
            self.defects.append(defect)

    def test_overridden_register_defect_still_raises(self):
        foo = self.MyObj()
        defect = self.MyDefect("the telly is broken")
        with self.assertRaisesRegex(self.MyDefect, "the telly is broken"):
            self.MyPolicy(raise_on_defect=True).handle_defect(foo, defect)

    def test_overriden_register_defect_works(self):
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

    # XXX: Need subclassing tests.
    # For adding subclassed objects, make sure the usual rules apply (subclass
    # wins), but that the order still works (right overrides left).

if __name__ == '__main__':
    unittest.main()
