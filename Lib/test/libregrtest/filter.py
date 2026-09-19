import itertools
import operator
import re
import sys


# By default, don't filter tests
_test_matchers = ()
_test_patterns = ()
_match_labels = ()

# Sentinel returned by _get_label() when the test has no such label.
_no_label = object()


def match_test(test):
    # Function used by support.run_unittest() and regrtest --list-cases
    return match_test_id(test) and match_test_label(test)

def match_test_id(test):
    result = False
    for matcher, result in reversed(_test_matchers):
        if matcher(test.id()):
            return result
    return not result

def match_test_label(test):
    result = False
    for name, value, result in reversed(_match_labels):
        actual = _get_label(test, name)
        if actual is _no_label:
            continue
        # value is None for a plain "--label name" (match any value).
        if value is None or value == str(actual):
            return result
    return not result

def _get_label(test, label):
    attrname = f'_label_{label}'
    value = getattr(test, attrname, _no_label)
    if value is not _no_label:
        return value
    testMethod = getattr(test, test._testMethodName)
    while testMethod is not None:
        value = getattr(testMethod, attrname, _no_label)
        if value is not _no_label:
            return value
        testMethod = getattr(testMethod, '__wrapped__', None)
    try:
        module = sys.modules[test.__class__.__module__]
    except KeyError:
        pass
    else:
        value = getattr(module, attrname, _no_label)
        if value is not _no_label:
            return value
    return _no_label


def _is_full_match_test(pattern):
    # If a pattern contains at least one dot, it's considered
    # as a full test identifier.
    # Example: 'test.test_os.FileTests.test_access'.
    #
    # ignore patterns which contain fnmatch patterns: '*', '?', '[...]'
    # or '[!...]'. For example, ignore 'test_access*'.
    return ('.' in pattern) and (not re.search(r'[?*\[\]]', pattern))


def get_match_tests():
    global _test_patterns
    return _test_patterns


def set_match_tests(patterns=None, match_labels=None):
    global _test_matchers, _test_patterns, _match_labels

    if not patterns:
        _test_matchers = ()
        _test_patterns = ()
    else:
        itemgetter = operator.itemgetter
        patterns = tuple(patterns)
        if patterns != _test_patterns:
            _test_matchers = [
                (_compile_match_function(map(itemgetter(0), it)), result)
                for result, it in itertools.groupby(patterns, itemgetter(1))
            ]
            _test_patterns = patterns

    if not match_labels:
        _match_labels = ()
    else:
        # "name" matches a label with any value, "name=value" matches only
        # the specified value.
        _match_labels = tuple(
            (name, value if sep else None, result)
            for label, result in match_labels
            for name, sep, value in [label.partition('=')]
        )


def _compile_match_function(patterns):
    patterns = list(patterns)

    if all(map(_is_full_match_test, patterns)):
        # Simple case: all patterns are full test identifier.
        # The test.bisect_cmd utility only uses such full test identifiers.
        return set(patterns).__contains__
    else:
        import fnmatch
        regex = '|'.join(map(fnmatch.translate, patterns))
        # The search *is* case sensitive on purpose:
        # don't use flags=re.IGNORECASE
        regex_match = re.compile(regex).match

        def match_test_regex(test_id, regex_match=regex_match):
            if regex_match(test_id):
                # The regex matches the whole identifier, for example
                # 'test.test_os.FileTests.test_access'.
                return True
            else:
                # Try to match parts of the test identifier.
                # For example, split 'test.test_os.FileTests.test_access'
                # into: 'test', 'test_os', 'FileTests' and 'test_access'.
                return any(map(regex_match, test_id.split(".")))

        return match_test_regex
