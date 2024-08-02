# Test docstring extraction
"""
multiline
module docstring
"""


def test(x):
    """foo"""
    return 2*x


def test2(x):


    """docstring with some blank lines in front"""
    return 2*x


def test3(x):
    """multiline
    docstring with some more content
    """


def test4(x):
    """outer docstring"""
    def inner(y):
        """nested docstring"""


def test5(x):
    """some docstring"""
    """another string"""
    """and one more"""


def test6(x):
    """
    A very long docstring which should be correctly wrapped into multiple lines
    in the output po file according to the maximum line width setting.
    """


def test7(x):
    r"""Raw docstrings are ok"""


def test8(x):
    u"""Unicode docstrings are ok"""


def test9(x):
    b"""bytes should not be picked up"""


def test10(x):
    f"""f-strings should not be picked up"""


def test11(x):
    """Hello, {}!""".format("docstring")


def test12(x):
    """"Some non-ascii dosctring: ěščř αβγδ"""


def test13(x):
    """"""


class Foo:
    """foo"""


class Outer:
    """outer class docstring"""
    class Inner:
        "inner class docstring"

        def inner_method(self):
            """method docstring"""

    def outer_method(self):
        """method docstring"""


async def async_test(x):
    """Async function docstring"""
    async def async_inner(y):
        """async function nested docstring"""
