import contextlib
import os.path
import test.test_tools
from test.support import load_package_tests


@contextlib.contextmanager
def tool_imports_for_tests():
    test.test_tools.skip_if_missing('c-analyzer')
    with test.test_tools.imports_under_tool('c-analyzer'):
        yield


def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)
