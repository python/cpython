import itertools
import operator
import re


# By default, don't filter tests
_test_matchers = ()
_test_patterns = ()


def match_test(test):
    # Function used by support.run_unittest() and regrtest --list-cases
    result = False
    for matcher, result in reversed(_test_matchers):
        if matcher(test.id()):
            return result
    return not result


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


def set_match_tests(patterns):
    global _test_matchers, _test_patterns

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
