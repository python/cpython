#! /usr/bin/env python
from test_support import verbose, verify
from types import TupleType, StringType, IntType
import __future__

GOOD_SERIALS = ("alpha", "beta", "candidate", "final")

features = __future__.all_feature_names

# Verify that all_feature_names appears correct.
given_feature_names = features[:]
for name in dir(__future__):
    obj = getattr(__future__, name, None)
    if obj is not None and isinstance(obj, __future__._Feature):
        verify(name in given_feature_names,
               "%r should have been in all_feature_names" % name)
        given_feature_names.remove(name)
verify(len(given_feature_names) == 0,
       "all_feature_names has too much: %r" % given_feature_names)
del given_feature_names

for feature in features:
    value = getattr(__future__, feature)
    if verbose:
        print "Checking __future__ ", feature, "value", value

    optional = value.getOptionalRelease()
    mandatory = value.getMandatoryRelease()

    verify(type(optional) is TupleType, "optional isn't tuple")
    verify(len(optional) == 5, "optional isn't 5-tuple")
    major, minor, micro, level, serial = optional
    verify(type(major) is IntType, "optional major isn't int")
    verify(type(minor) is IntType, "optional minor isn't int")
    verify(type(micro) is IntType, "optional micro isn't int")
    verify(type(level) is StringType, "optional level isn't string")
    verify(level in GOOD_SERIALS,
           "optional level string has unknown value")
    verify(type(serial) is IntType, "optional serial isn't int")

    verify(type(mandatory) is TupleType or
           mandatory is None, "mandatory isn't tuple or None")
    if mandatory is not None:
        verify(len(mandatory) == 5, "mandatory isn't 5-tuple")
        major, minor, micro, level, serial = mandatory
        verify(type(major) is IntType, "mandatory major isn't int")
        verify(type(minor) is IntType, "mandatory minor isn't int")
        verify(type(micro) is IntType, "mandatory micro isn't int")
        verify(type(level) is StringType, "mandatory level isn't string")
        verify(level in GOOD_SERIALS,
               "mandatory serial string has unknown value")
        verify(type(serial) is IntType, "mandatory serial isn't int")
        verify(optional < mandatory,
               "optional not less than mandatory, and mandatory not None")

    verify(hasattr(value, "compiler_flag"),
           "feature is missing a .compiler_flag attr")
    verify(type(getattr(value, "compiler_flag")) is IntType,
           ".compiler_flag isn't int")
