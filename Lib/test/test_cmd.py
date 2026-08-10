"""
Test script for the 'cmd' module
Original by Michael Schneider
"""


import cmd
import sys
import doctest
import unittest
import io
import textwrap
from test import support
from test.support.import_helper import ensure_lazy_imports, import_module
from test.support.pty_helper import run_pty

class LazyImportTest(unittest.TestCase):
    @support.cpython_only
    def test_lazy_import(self):
        ensure_lazy_imports("cmd", {"inspect", "string"})


class samplecmdclass(cmd.Cmd):
    """
    Instance the sampleclass:
    >>> mycmd = samplecmdclass()

    Test for the function parseline():
    >>> mycmd.parseline("")
    (None, None, '')
    >>> mycmd.parseline("?")
    ('help', '', 'help ')
    >>> mycmd.parseline("?help")
    ('help', 'help', 'help help')
    >>> mycmd.parseline("!")
    ('shell', '', 'shell ')
    >>> mycmd.parseline("!command")
    ('shell', 'command', 'shell command')
    >>> mycmd.parseline("func")
    ('func', '', 'func')
    >>> mycmd.parseline("func arg1")
    ('func', 'arg1', 'func arg1')


    Test for the function onecmd():
    >>> mycmd.onecmd("")
    >>> mycmd.onecmd("add 4 5")
    9
    >>> mycmd.onecmd("")
    9
    >>> mycmd.onecmd("test")
    *** Unknown syntax: test

    Test for the function emptyline():
    >>> mycmd.emptyline()
    *** Unknown syntax: test

    Test for the function default():
    >>> mycmd.default("default")
    *** Unknown syntax: default

    Test for the function completedefault():
    >>> mycmd.completedefault()
    This is the completedefault method
    >>> mycmd.completenames("a")
    ['add']

    Test for the function completenames():
    >>> mycmd.completenames("12")
    []
    >>> mycmd.completenames("help")
    ['help']

    Test for the function complete_help():
    >>> mycmd.complete_help("a")
    ['add']
    >>> mycmd.complete_help("he")
    ['help']
    >>> mycmd.complete_help("12")
    []
    >>> sorted(mycmd.complete_help(""))
    ['add', 'exit', 'help', 'life', 'meaning', 'shell']

    Test for the function do_help():
    >>> mycmd.do_help("testet")
    *** No help on testet
    >>> mycmd.do_help("add")
    help text for add
    >>> mycmd.onecmd("help add")
    help text for add
    >>> mycmd.onecmd("help meaning")  # doctest: +NORMALIZE_WHITESPACE
    Try and be nice to people, avoid eating fat, read a good book every
    now and then, get some walking in, and try to live together in peace
    and harmony with people of all creeds and nations.
    >>> mycmd.do_help("")
    <BLANKLINE>
    Documented commands (type help <topic>):
    ========================================
    add  help
    <BLANKLINE>
    Miscellaneous help topics:
    ==========================
    life  meaning
    <BLANKLINE>
    Undocumented commands:
    ======================
    exit  shell
    <BLANKLINE>

    Test for the function print_topics():
    >>> mycmd.print_topics("header", ["command1", "command2"], 2 ,10)
    header
    ======
    command1
    command2
    <BLANKLINE>

    Test for the function columnize():
    >>> mycmd.columnize([str(i) for i in range(20)])
    0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19
    >>> mycmd.columnize([str(i) for i in range(20)], 10)
    0  7   14
    1  8   15
    2  9   16
    3  10  17
    4  11  18
    5  12  19
    6  13

    This is an interactive test, put some commands in the cmdqueue attribute
    and let it execute
    This test includes the preloop(), postloop(), default(), emptyline(),
    parseline(), do_help() functions
    >>> mycmd.use_rawinput=0

    >>> mycmd.cmdqueue=["add", "add 4 5", "", "help", "help add", "exit"]
    >>> mycmd.cmdloop()  # doctest: +REPORT_NDIFF
    Hello from preloop
    *** invalid number of arguments
    9
    9
    <BLANKLINE>
    Documented commands (type help <topic>):
    ========================================
    add  help
    <BLANKLINE>
    Miscellaneous help topics:
    ==========================
    life  meaning
    <BLANKLINE>
    Undocumented commands:
    ======================
    exit  shell
    <BLANKLINE>
    help text for add
    Hello from postloop
    """

    def preloop(self):
        print("Hello from preloop")

    def postloop(self):
        print("Hello from postloop")

    def completedefault(self, *ignored):
        print("This is the completedefault method")

    def complete_command(self):
        print("complete command")

    def do_shell(self, s):
        pass

    def do_add(self, s):
        l = s.split()
        if len(l) != 2:
            print("*** invalid number of arguments")
            return
        try:
            l = [int(i) for i in l]
        except ValueError:
            print("*** arguments should be numbers")
            return
        print(l[0]+l[1])

    def help_add(self):
        print("help text for add")
        return

    def help_meaning(self):
        print("Try and be nice to people, avoid eating fat, read a "
              "good book every now and then, get some walking in, "
              "and try to live together in peace and harmony with "
              "people of all creeds and nations.")
        return

    def help_life(self):
        print("Always look on the bright side of life")
        return

    def do_exit(self, arg):
        return True


class TestAlternateInput(unittest.TestCase):

    class simplecmd(cmd.Cmd):

        def do_print(self, args):
            print(args, file=self.stdout)

        def do_EOF(self, args):
            return True


    class simplecmd2(simplecmd):

        def do_EOF(self, args):
            print('*** Unknown syntax: EOF', file=self.stdout)
            return True


    def test_file_with_missing_final_nl(self):
        input = io.StringIO("print test\nprint test2")
        output = io.StringIO()
        cmd = self.simplecmd(stdin=input, stdout=output)
        cmd.use_rawinput = False
        cmd.cmdloop()
        self.assertMultiLineEqual(output.getvalue(),
            ("(Cmd) test\n"
             "(Cmd) test2\n"
             "(Cmd) "))


    def test_input_reset_at_EOF(self):
        input = io.StringIO("print test\nprint test2")
        output = io.StringIO()
        cmd = self.simplecmd2(stdin=input, stdout=output)
        cmd.use_rawinput = False
        cmd.cmdloop()
        self.assertMultiLineEqual(output.getvalue(),
            ("(Cmd) test\n"
             "(Cmd) test2\n"
             "(Cmd) *** Unknown syntax: EOF\n"))
        input = io.StringIO("print \n\n")
        output = io.StringIO()
        cmd.stdin = input
        cmd.stdout = output
        cmd.cmdloop()
        self.assertMultiLineEqual(output.getvalue(),
            ("(Cmd) \n"
             "(Cmd) \n"
             "(Cmd) *** Unknown syntax: EOF\n"))


class CmdPrintExceptionClass(cmd.Cmd):
    """
    GH-80731
    cmd.Cmd should print the correct exception in default()
    >>> mycmd = CmdPrintExceptionClass()
    >>> try:
    ...     raise ValueError("test")
    ... except ValueError:
    ...     mycmd.onecmd("not important")
    (<class 'ValueError'>, ValueError('test'))
    """

    def default(self, line):
        print(sys.exc_info()[:2])


@support.requires_subprocess()
class CmdTestReadline(unittest.TestCase):
    def setUpClass():
        # Ensure that the readline module is loaded
        # If this fails, the test is skipped because SkipTest will be raised
        readline = import_module('readline')

    def test_basic_completion(self):
        script = textwrap.dedent("""
            import cmd
            class simplecmd(cmd.Cmd):
                def do_tab_completion_test(self, args):
                    print('tab completion success')
                    return True

            simplecmd().cmdloop()
        """)

        # 't' and complete 'ab_completion_test' to 'tab_completion_test'
        input = b"t\t\n"

        output = run_pty(script, input)

        self.assertIn(b'ab_completion_test', output)
        self.assertIn(b'tab completion success', output)

    def test_bang_completion_without_do_shell(self):
        script = textwrap.dedent("""
            import cmd
            class simplecmd(cmd.Cmd):
                def completedefault(self, text, line, begidx, endidx):
                    return ["hello"]

                def default(self, line):
                    if line.replace(" ", "") == "!hello":
                        print('tab completion success')
                    else:
                        print('tab completion failure')
                    return True

            simplecmd().cmdloop()
        """)

        # '! h' or '!h' and complete 'ello' to 'hello'
        for input in [b"! h\t\n", b"!h\t\n"]:
            with self.subTest(input=input):
                output = run_pty(script, input)
                self.assertIn(b'hello', output)
                self.assertIn(b'tab completion success', output)

def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    if "-i" in sys.argv:
        samplecmdclass().cmdloop()
    else:
        unittest.main()
