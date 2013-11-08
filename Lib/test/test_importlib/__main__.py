"""Run importlib's test suite.

Specifying the ``--builtin`` flag will run tests, where applicable, with
builtins.__import__ instead of importlib.__import__.

"""
if __name__ == '__main__':
    from . import test_main
    test_main()
