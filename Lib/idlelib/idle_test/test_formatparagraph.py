# Test the functions and main class method of FormatParagraph.py
import unittest
from idlelib import FormatParagraph as fp
from idlelib.EditorWindow import EditorWindow
from Tkinter import Tk, Text
from test.test_support import requires


class Is_Get_Test(unittest.TestCase):
    """Test the is_ and get_ functions"""
    test_comment = '# This is a comment'
    test_nocomment = 'This is not a comment'
    trailingws_comment = '# This is a comment   '
    leadingws_comment = '    # This is a comment'
    leadingws_nocomment = '    This is not a comment'

    def test_is_all_white(self):
        self.assertTrue(fp.is_all_white(''))
        self.assertTrue(fp.is_all_white('\t\n\r\f\v'))
        self.assertFalse(fp.is_all_white(self.test_comment))

    def test_get_indent(self):
        Equal = self.assertEqual
        Equal(fp.get_indent(self.test_comment), '')
        Equal(fp.get_indent(self.trailingws_comment), '')
        Equal(fp.get_indent(self.leadingws_comment), '    ')
        Equal(fp.get_indent(self.leadingws_nocomment), '    ')

    def test_get_comment_header(self):
        Equal = self.assertEqual
        # Test comment strings
        Equal(fp.get_comment_header(self.test_comment), '#')
        Equal(fp.get_comment_header(self.trailingws_comment), '#')
        Equal(fp.get_comment_header(self.leadingws_comment), '    #')
        # Test non-comment strings
        Equal(fp.get_comment_header(self.leadingws_nocomment), '    ')
        Equal(fp.get_comment_header(self.test_nocomment), '')


class FindTest(unittest.TestCase):
    """Test the find_paragraph function in FormatParagraph.

    Using the runcase() function, find_paragraph() is called with 'mark' set at
    multiple indexes before and inside the test paragraph.

    It appears that code with the same indentation as a quoted string is grouped
    as part of the same paragraph, which is probably incorrect behavior.
    """

    @classmethod
    def setUpClass(cls):
        from idlelib.idle_test.mock_tk import Text
        cls.text = Text()

    def runcase(self, inserttext, stopline, expected):
        # Check that find_paragraph returns the expected paragraph when
        # the mark index is set to beginning, middle, end of each line
        # up to but not including the stop line
        text = self.text
        text.insert('1.0', inserttext)
        for line in range(1, stopline):
            linelength = int(text.index("%d.end" % line).split('.')[1])
            for col in (0, linelength//2, linelength):
                tempindex = "%d.%d" % (line, col)
                self.assertEqual(fp.find_paragraph(text, tempindex), expected)
        text.delete('1.0', 'end')

    def test_find_comment(self):
        comment = (
            "# Comment block with no blank lines before\n"
            "# Comment line\n"
            "\n")
        self.runcase(comment, 3, ('1.0', '3.0', '#', comment[0:58]))

        comment = (
            "\n"
            "# Comment block with whitespace line before and after\n"
            "# Comment line\n"
            "\n")
        self.runcase(comment, 4, ('2.0', '4.0', '#', comment[1:70]))

        comment = (
            "\n"
            "    # Indented comment block with whitespace before and after\n"
            "    # Comment line\n"
            "\n")
        self.runcase(comment, 4, ('2.0', '4.0', '    #', comment[1:82]))

        comment = (
            "\n"
            "# Single line comment\n"
            "\n")
        self.runcase(comment, 3, ('2.0', '3.0', '#', comment[1:23]))

        comment = (
            "\n"
            "    # Single line comment with leading whitespace\n"
            "\n")
        self.runcase(comment, 3, ('2.0', '3.0', '    #', comment[1:51]))

        comment = (
            "\n"
            "# Comment immediately followed by code\n"
            "x = 42\n"
            "\n")
        self.runcase(comment, 3, ('2.0', '3.0', '#', comment[1:40]))

        comment = (
            "\n"
            "    # Indented comment immediately followed by code\n"
            "x = 42\n"
            "\n")
        self.runcase(comment, 3, ('2.0', '3.0', '    #', comment[1:53]))

        comment = (
            "\n"
            "# Comment immediately followed by indented code\n"
            "    x = 42\n"
            "\n")
        self.runcase(comment, 3, ('2.0', '3.0', '#', comment[1:49]))

    def test_find_paragraph(self):
        teststring = (
            '"""String with no blank lines before\n'
            'String line\n'
            '"""\n'
            '\n')
        self.runcase(teststring, 4, ('1.0', '4.0', '', teststring[0:53]))

        teststring = (
            "\n"
            '"""String with whitespace line before and after\n'
            'String line.\n'
            '"""\n'
            '\n')
        self.runcase(teststring, 5, ('2.0', '5.0', '', teststring[1:66]))

        teststring = (
            '\n'
            '    """Indented string with whitespace before and after\n'
            '    Comment string.\n'
            '    """\n'
            '\n')
        self.runcase(teststring, 5, ('2.0', '5.0', '    ', teststring[1:85]))

        teststring = (
            '\n'
            '"""Single line string."""\n'
            '\n')
        self.runcase(teststring, 3, ('2.0', '3.0', '', teststring[1:27]))

        teststring = (
            '\n'
            '    """Single line string with leading whitespace."""\n'
            '\n')
        self.runcase(teststring, 3, ('2.0', '3.0', '    ', teststring[1:55]))


class ReformatFunctionTest(unittest.TestCase):
    """Test the reformat_paragraph function without the editor window."""

    def test_reformat_paragraph(self):
        Equal = self.assertEqual
        reform = fp.reformat_paragraph
        hw = "O hello world"
        Equal(reform(' ', 1), ' ')
        Equal(reform("Hello    world", 20), "Hello  world")

        # Test without leading newline
        Equal(reform(hw, 1), "O\nhello\nworld")
        Equal(reform(hw, 6), "O\nhello\nworld")
        Equal(reform(hw, 7), "O hello\nworld")
        Equal(reform(hw, 12), "O hello\nworld")
        Equal(reform(hw, 13), "O hello world")

        # Test with leading newline
        hw = "\nO hello world"
        Equal(reform(hw, 1), "\nO\nhello\nworld")
        Equal(reform(hw, 6), "\nO\nhello\nworld")
        Equal(reform(hw, 7), "\nO hello\nworld")
        Equal(reform(hw, 12), "\nO hello\nworld")
        Equal(reform(hw, 13), "\nO hello world")


class ReformatCommentTest(unittest.TestCase):
    """Test the reformat_comment function without the editor window."""

    def test_reformat_comment(self):
        Equal = self.assertEqual

        # reformat_comment formats to a minimum of 20 characters
        test_string = (
            "    \"\"\"this is a test of a reformat for a triple quoted string"
            " will it reformat to less than 70 characters for me?\"\"\"")
        result = fp.reformat_comment(test_string, 70, "    ")
        expected = (
            "    \"\"\"this is a test of a reformat for a triple quoted string will it\n"
            "    reformat to less than 70 characters for me?\"\"\"")
        Equal(result, expected)

        test_comment = (
            "# this is a test of a reformat for a triple quoted string will "
            "it reformat to less than 70 characters for me?")
        result = fp.reformat_comment(test_comment, 70, "#")
        expected = (
            "# this is a test of a reformat for a triple quoted string will it\n"
            "# reformat to less than 70 characters for me?")
        Equal(result, expected)


class FormatClassTest(unittest.TestCase):
    def test_init_close(self):
        instance = fp.FormatParagraph('editor')
        self.assertEqual(instance.editwin, 'editor')
        instance.close()
        self.assertEqual(instance.editwin, None)


# For testing format_paragraph_event, Initialize FormatParagraph with
# a mock Editor with .text and  .get_selection_indices.  The text must
# be a Text wrapper that adds two methods

# A real EditorWindow creates unneeded, time-consuming baggage and
# sometimes emits shutdown warnings like this:
# "warning: callback failed in WindowList <class '_tkinter.TclError'>
# : invalid command name ".55131368.windows".
# Calling EditorWindow._close in tearDownClass prevents this but causes
# other problems (windows left open).

class TextWrapper:
    def __init__(self, master):
        self.text = Text(master=master)
    def __getattr__(self, name):
        return getattr(self.text, name)
    def undo_block_start(self): pass
    def undo_block_stop(self): pass

class Editor:
    def __init__(self, root):
        self.text = TextWrapper(root)
    get_selection_indices = EditorWindow. get_selection_indices.im_func

class FormatEventTest(unittest.TestCase):
    """Test the formatting of text inside a Text widget.

    This is done with FormatParagraph.format.paragraph_event,
    which calls functions in the module as appropriate.
    """
    test_string = (
        "    '''this is a test of a reformat for a triple "
        "quoted string will it reformat to less than 70 "
        "characters for me?'''\n")
    multiline_test_string = (
        "    '''The first line is under the max width.\n"
        "    The second line's length is way over the max width. It goes "
        "on and on until it is over 100 characters long.\n"
        "    Same thing with the third line. It is also way over the max "
        "width, but FormatParagraph will fix it.\n"
        "    '''\n")
    multiline_test_comment = (
        "# The first line is under the max width.\n"
        "# The second line's length is way over the max width. It goes on "
        "and on until it is over 100 characters long.\n"
        "# Same thing with the third line. It is also way over the max "
        "width, but FormatParagraph will fix it.\n"
        "# The fourth line is short like the first line.")

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        editor = Editor(root=cls.root)
        cls.text = editor.text.text  # Test code does not need the wrapper.
        cls.formatter = fp.FormatParagraph(editor).format_paragraph_event
        # Sets the insert mark just after the re-wrapped and inserted  text.

    @classmethod
    def tearDownClass(cls):
        del cls.text, cls.formatter
        cls.root.destroy()
        del cls.root

    def test_short_line(self):
        self.text.insert('1.0', "Short line\n")
        self.formatter("Dummy")
        self.assertEqual(self.text.get('1.0', 'insert'), "Short line\n" )
        self.text.delete('1.0', 'end')

    def test_long_line(self):
        text = self.text

        # Set cursor ('insert' mark) to '1.0', within text.
        text.insert('1.0', self.test_string)
        text.mark_set('insert', '1.0')
        self.formatter('ParameterDoesNothing', limit=70)
        result = text.get('1.0', 'insert')
        # find function includes \n
        expected = (
"    '''this is a test of a reformat for a triple quoted string will it\n"
"    reformat to less than 70 characters for me?'''\n")  # yes
        self.assertEqual(result, expected)
        text.delete('1.0', 'end')

        # Select from 1.11 to line end.
        text.insert('1.0', self.test_string)
        text.tag_add('sel', '1.11', '1.end')
        self.formatter('ParameterDoesNothing', limit=70)
        result = text.get('1.0', 'insert')
        # selection excludes \n
        expected = (
"    '''this is a test of a reformat for a triple quoted string will it reformat\n"
" to less than 70 characters for me?'''")  # no
        self.assertEqual(result, expected)
        text.delete('1.0', 'end')

    def test_multiple_lines(self):
        text = self.text
        #  Select 2 long lines.
        text.insert('1.0', self.multiline_test_string)
        text.tag_add('sel', '2.0', '4.0')
        self.formatter('ParameterDoesNothing', limit=70)
        result = text.get('2.0', 'insert')
        expected = (
"    The second line's length is way over the max width. It goes on and\n"
"    on until it is over 100 characters long. Same thing with the third\n"
"    line. It is also way over the max width, but FormatParagraph will\n"
"    fix it.\n")
        self.assertEqual(result, expected)
        text.delete('1.0', 'end')

    def test_comment_block(self):
        text = self.text

        # Set cursor ('insert') to '1.0', within block.
        text.insert('1.0', self.multiline_test_comment)
        self.formatter('ParameterDoesNothing', limit=70)
        result = text.get('1.0', 'insert')
        expected = (
"# The first line is under the max width. The second line's length is\n"
"# way over the max width. It goes on and on until it is over 100\n"
"# characters long. Same thing with the third line. It is also way over\n"
"# the max width, but FormatParagraph will fix it. The fourth line is\n"
"# short like the first line.\n")
        self.assertEqual(result, expected)
        text.delete('1.0', 'end')

        # Select line 2, verify line 1 unaffected.
        text.insert('1.0', self.multiline_test_comment)
        text.tag_add('sel', '2.0', '3.0')
        self.formatter('ParameterDoesNothing', limit=70)
        result = text.get('1.0', 'insert')
        expected = (
"# The first line is under the max width.\n"
"# The second line's length is way over the max width. It goes on and\n"
"# on until it is over 100 characters long.\n")
        self.assertEqual(result, expected)
        text.delete('1.0', 'end')

# The following block worked with EditorWindow but fails with the mock.
# Lines 2 and 3 get pasted together even though the previous block left
# the previous line alone. More investigation is needed.
##        # Select lines 3 and 4
##        text.insert('1.0', self.multiline_test_comment)
##        text.tag_add('sel', '3.0', '5.0')
##        self.formatter('ParameterDoesNothing')
##        result = text.get('3.0', 'insert')
##        expected = (
##"# Same thing with the third line. It is also way over the max width,\n"
##"# but FormatParagraph will fix it. The fourth line is short like the\n"
##"# first line.\n")
##        self.assertEqual(result, expected)
##        text.delete('1.0', 'end')


if __name__ == '__main__':
    unittest.main(verbosity=2, exit=2)
