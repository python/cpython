"""Test suite for 2to3's parser and grammar files.

This is the place to add tests for changes to 2to3's grammar, such as those
merging the grammars for Python 2 and 3. In addition to specific tests for
parts of the grammar we've changed, we also make sure we can parse the
test_grammar.py files from both Python 2 and Python 3.
"""

# Testing imports
from . import support
from .support import driver, test_dir

# Python imports
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

# Local imports
from lib2to3.pgen2 import driver as pgen2_driver
from lib2to3.pgen2 import tokenize
from ..pgen2.parse import ParseError
from lib2to3.pygram import python_symbols as syms


class TestDriver(support.TestCase):

    def test_formfeed(self):
        s = """print 1\n\x0Cprint 2\n"""
        t = driver.parse_string(s)
        self.assertEqual(t.children[0].children[0].type, syms.print_stmt)
        self.assertEqual(t.children[1].children[0].type, syms.print_stmt)


class TestPgen2Caching(support.TestCase):
    def test_load_grammar_from_txt_file(self):
        pgen2_driver.load_grammar(support.grammar_path, save=False, force=True)

    def test_load_grammar_from_pickle(self):
        # Make a copy of the grammar file in a temp directory we are
        # guaranteed to be able to write to.
        tmpdir = tempfile.mkdtemp()
        try:
            grammar_copy = os.path.join(
                    tmpdir, os.path.basename(support.grammar_path))
            shutil.copy(support.grammar_path, grammar_copy)
            pickle_name = pgen2_driver._generate_pickle_name(grammar_copy)

            pgen2_driver.load_grammar(grammar_copy, save=True, force=True)
            self.assertTrue(os.path.exists(pickle_name))

            os.unlink(grammar_copy)  # Only the pickle remains...
            pgen2_driver.load_grammar(grammar_copy, save=False, force=False)
        finally:
            shutil.rmtree(tmpdir)

    @unittest.skipIf(sys.executable is None, 'sys.executable required')
    def test_load_grammar_from_subprocess(self):
        tmpdir = tempfile.mkdtemp()
        tmpsubdir = os.path.join(tmpdir, 'subdir')
        try:
            os.mkdir(tmpsubdir)
            grammar_base = os.path.basename(support.grammar_path)
            grammar_copy = os.path.join(tmpdir, grammar_base)
            grammar_sub_copy = os.path.join(tmpsubdir, grammar_base)
            shutil.copy(support.grammar_path, grammar_copy)
            shutil.copy(support.grammar_path, grammar_sub_copy)
            pickle_name = pgen2_driver._generate_pickle_name(grammar_copy)
            pickle_sub_name = pgen2_driver._generate_pickle_name(
                     grammar_sub_copy)
            self.assertNotEqual(pickle_name, pickle_sub_name)

            # Generate a pickle file from this process.
            pgen2_driver.load_grammar(grammar_copy, save=True, force=True)
            self.assertTrue(os.path.exists(pickle_name))

            # Generate a new pickle file in a subprocess with a most likely
            # different hash randomization seed.
            sub_env = dict(os.environ)
            sub_env['PYTHONHASHSEED'] = 'random'
            subprocess.check_call(
                    [sys.executable, '-c', """
from lib2to3.pgen2 import driver as pgen2_driver
pgen2_driver.load_grammar(%r, save=True, force=True)
                    """ % (grammar_sub_copy,)],
                    env=sub_env)
            self.assertTrue(os.path.exists(pickle_sub_name))

            with open(pickle_name, 'rb') as pickle_f_1, \
                    open(pickle_sub_name, 'rb') as pickle_f_2:
                self.assertEqual(
                    pickle_f_1.read(), pickle_f_2.read(),
                    msg='Grammar caches generated using different hash seeds'
                    ' were not identical.')
        finally:
            shutil.rmtree(tmpdir)



class GrammarTest(support.TestCase):
    def validate(self, code):
        support.parse_string(code)

    def invalid_syntax(self, code):
        try:
            self.validate(code)
        except ParseError:
            pass
        else:
            raise AssertionError("Syntax shouldn't have been valid")


class TestMatrixMultiplication(GrammarTest):
    def test_matrix_multiplication_operator(self):
        self.validate("a @ b")
        self.validate("a @= b")


class TestYieldFrom(GrammarTest):
    def test_matrix_multiplication_operator(self):
        self.validate("yield from x")
        self.validate("(yield from x) + y")
        self.invalid_syntax("yield from")


class TestRaiseChanges(GrammarTest):
    def test_2x_style_1(self):
        self.validate("raise")

    def test_2x_style_2(self):
        self.validate("raise E, V")

    def test_2x_style_3(self):
        self.validate("raise E, V, T")

    def test_2x_style_invalid_1(self):
        self.invalid_syntax("raise E, V, T, Z")

    def test_3x_style(self):
        self.validate("raise E1 from E2")

    def test_3x_style_invalid_1(self):
        self.invalid_syntax("raise E, V from E1")

    def test_3x_style_invalid_2(self):
        self.invalid_syntax("raise E from E1, E2")

    def test_3x_style_invalid_3(self):
        self.invalid_syntax("raise from E1, E2")

    def test_3x_style_invalid_4(self):
        self.invalid_syntax("raise E from")


# Modelled after Lib/test/test_grammar.py:TokenTests.test_funcdef issue2292
# and Lib/test/text_parser.py test_list_displays, test_set_displays,
# test_dict_displays, test_argument_unpacking, ... changes.
class TestUnpackingGeneralizations(GrammarTest):
    def test_mid_positional_star(self):
        self.validate("""func(1, *(2, 3), 4)""")

    def test_double_star_dict_literal(self):
        self.validate("""func(**{'eggs':'scrambled', 'spam':'fried'})""")

    def test_double_star_dict_literal_after_keywords(self):
        self.validate("""func(spam='fried', **{'eggs':'scrambled'})""")

    def test_list_display(self):
        self.validate("""[*{2}, 3, *[4]]""")

    def test_set_display(self):
        self.validate("""{*{2}, 3, *[4]}""")

    def test_dict_display_1(self):
        self.validate("""{**{}}""")

    def test_dict_display_2(self):
        self.validate("""{**{}, 3:4, **{5:6, 7:8}}""")

    def test_argument_unpacking_1(self):
        self.validate("""f(a, *b, *c, d)""")

    def test_argument_unpacking_2(self):
        self.validate("""f(**a, **b)""")

    def test_argument_unpacking_3(self):
        self.validate("""f(2, *a, *b, **b, **c, **d)""")


# Adaptated from Python 3's Lib/test/test_grammar.py:GrammarTests.testFuncdef
class TestFunctionAnnotations(GrammarTest):
    def test_1(self):
        self.validate("""def f(x) -> list: pass""")

    def test_2(self):
        self.validate("""def f(x:int): pass""")

    def test_3(self):
        self.validate("""def f(*x:str): pass""")

    def test_4(self):
        self.validate("""def f(**x:float): pass""")

    def test_5(self):
        self.validate("""def f(x, y:1+2): pass""")

    def test_6(self):
        self.validate("""def f(a, (b:1, c:2, d)): pass""")

    def test_7(self):
        self.validate("""def f(a, (b:1, c:2, d), e:3=4, f=5, *g:6): pass""")

    def test_8(self):
        s = """def f(a, (b:1, c:2, d), e:3=4, f=5,
                        *g:6, h:7, i=8, j:9=10, **k:11) -> 12: pass"""
        self.validate(s)


class TestExcept(GrammarTest):
    def test_new(self):
        s = """
            try:
                x
            except E as N:
                y"""
        self.validate(s)

    def test_old(self):
        s = """
            try:
                x
            except E, N:
                y"""
        self.validate(s)


# Adapted from Python 3's Lib/test/test_grammar.py:GrammarTests.testAtoms
class TestSetLiteral(GrammarTest):
    def test_1(self):
        self.validate("""x = {'one'}""")

    def test_2(self):
        self.validate("""x = {'one', 1,}""")

    def test_3(self):
        self.validate("""x = {'one', 'two', 'three'}""")

    def test_4(self):
        self.validate("""x = {2, 3, 4,}""")


class TestNumericLiterals(GrammarTest):
    def test_new_octal_notation(self):
        self.validate("""0o7777777777777""")
        self.invalid_syntax("""0o7324528887""")

    def test_new_binary_notation(self):
        self.validate("""0b101010""")
        self.invalid_syntax("""0b0101021""")


class TestClassDef(GrammarTest):
    def test_new_syntax(self):
        self.validate("class B(t=7): pass")
        self.validate("class B(t, *args): pass")
        self.validate("class B(t, **kwargs): pass")
        self.validate("class B(t, *args, **kwargs): pass")
        self.validate("class B(t, y=9, *args, **kwargs): pass")


class TestParserIdempotency(support.TestCase):

    """A cut-down version of pytree_idempotency.py."""

    def test_all_project_files(self):
        if sys.platform.startswith("win"):
            # XXX something with newlines goes wrong on Windows.
            return
        for filepath in support.all_project_files():
            with open(filepath, "rb") as fp:
                encoding = tokenize.detect_encoding(fp.readline)[0]
            self.assertIsNotNone(encoding,
                                 "can't detect encoding for %s" % filepath)
            with open(filepath, "r") as fp:
                source = fp.read()
                source = source.decode(encoding)
            tree = driver.parse_string(source)
            new = unicode(tree)
            if diff(filepath, new, encoding):
                self.fail("Idempotency failed: %s" % filepath)

    def test_extended_unpacking(self):
        driver.parse_string("a, *b, c = x\n")
        driver.parse_string("[*a, b] = x\n")
        driver.parse_string("(z, *y, w) = m\n")
        driver.parse_string("for *z, m in d: pass\n")

class TestLiterals(GrammarTest):

    def validate(self, s):
        driver.parse_string(support.dedent(s) + "\n\n")

    def test_multiline_bytes_literals(self):
        s = """
            md5test(b"\xaa" * 80,
                    (b"Test Using Larger Than Block-Size Key "
                     b"and Larger Than One Block-Size Data"),
                    "6f630fad67cda0ee1fb1f562db3aa53e")
            """
        self.validate(s)

    def test_multiline_bytes_tripquote_literals(self):
        s = '''
            b"""
            <?xml version="1.0" encoding="UTF-8"?>
            <!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN">
            """
            '''
        self.validate(s)

    def test_multiline_str_literals(self):
        s = """
            md5test("\xaa" * 80,
                    ("Test Using Larger Than Block-Size Key "
                     "and Larger Than One Block-Size Data"),
                    "6f630fad67cda0ee1fb1f562db3aa53e")
            """
        self.validate(s)


def diff(fn, result, encoding):
    f = open("@", "w")
    try:
        f.write(result.encode(encoding))
    finally:
        f.close()
    try:
        fn = fn.replace('"', '\\"')
        return os.system('diff -u "%s" @' % fn)
    finally:
        os.remove("@")
