import importlib
from ..builtin import test_loader


class LoaderTests(test_loader.LoaderTests):

    name = '__phello__'
    load_module = staticmethod(lambda name:
                                importlib.FrozenImporter().load_module(name))
    verification = {'__name__': '__phello__', '__file__': '<frozen>',
                    '__package__': None, '__path__': ['__phello__']}


class SubmoduleLoaderTests(LoaderTests):

    name = '__phello__.spam'
    verification = {'__name__': '__phello__.spam', '__file__': '<frozen>',
                    '__package__': None}


def test_main():
    from test.support import run_unittest
    run_unittest(LoaderTests, SubmoduleLoaderTests)


if __name__ == '__main__':
    test_main()
