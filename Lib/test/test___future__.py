#! /usr/bin/env python
import unittest
from test import test_support
import __future__

GOOD_SERIALS = ("alpha", "beta", "candidate", "final")

features = __future__.all_feature_names

class FutureTest(unittest.TestCase):

    def test_names(self):
        # Verify that all_feature_names appears correct.
        given_feature_names = features[:]
        for name in dir(__future__):
            obj = getattr(__future__, name, None)
            if obj is not None and isinstance(obj, __future__._Feature):
                self.assert_(
                    name in given_feature_names,
                    "%r should have been in all_feature_names" % name
                )
                given_feature_names.remove(name)
        self.assertEqual(len(given_feature_names), 0,
               "all_feature_names has too much: %r" % given_feature_names)

    def test_attributes(self):
        for feature in features:
            value = getattr(__future__, feature)

            optional = value.getOptionalRelease()
            mandatory = value.getMandatoryRelease()

            a = self.assert_
            e = self.assertEqual
            def check(t, name):
                a(isinstance(t, tuple), "%s isn't tuple" % name)
                e(len(t), 5, "%s isn't 5-tuple" % name)
                (major, minor, micro, level, serial) = t
                a(isinstance(major, int), "%s major isn't int"  % name)
                a(isinstance(minor, int), "%s minor isn't int" % name)
                a(isinstance(micro, int), "%s micro isn't int" % name)
                a(isinstance(level, str),
                    "%s level isn't string" % name)
                a(level in GOOD_SERIALS,
                       "%s level string has unknown value" % name)
                a(isinstance(serial, int), "%s serial isn't int" % name)

            check(optional, "optional")
            if mandatory is not None:
                check(mandatory, "mandatory")
                a(optional < mandatory,
                       "optional not less than mandatory, and mandatory not None")

            a(hasattr(value, "compiler_flag"),
                   "feature is missing a .compiler_flag attr")
            a(isinstance(getattr(value, "compiler_flag"), int),
                   ".compiler_flag isn't int")

def test_main():
    test_support.run_unittest(FutureTest)

if __name__ == "__main__":
    test_main()
