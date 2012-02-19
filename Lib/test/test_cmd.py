#!/usr/bin/env python
"""
Test script for the 'cmd' module
Original by Michael Schneider
"""


import cmd
import sys
from test import test_support
import re
import unittest
import StringIO

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
    This is the completedefault methode
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
    ['add', 'exit', 'help', 'shell']

    Test for the function do_help():
    >>> mycmd.do_help("testet")
    *** No help on testet
    >>> mycmd.do_help("add")
    help text for add
    >>> mycmd.onecmd("help add")
    help text for add
    >>> mycmd.do_help("")
    <BLANKLINE>
    Documented commands (type help <topic>):
    ========================================
    add
    <BLANKLINE>
    Undocumented commands:
    ======================
    exit  help  shell
    <BLANKLINE>

    Test for the function print_topics():
    >>> mycmd.print_topics("header", ["command1", "command2"], 2 ,10)
    header
    ======
    command1
    command2
    <BLANKLINE>

    Test for the function columnize():
    >>> mycmd.columnize([str(i) for i in xrange(20)])
    0  1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19
    >>> mycmd.columnize([str(i) for i in xrange(20)], 10)
    0  7   14
    1  8   15
    2  9   16
    3  10  17
    4  11  18
    5  12  19
    6  13

    This is a interactive test, put some commands in the cmdqueue attribute
    and let it execute
    This test includes the preloop(), postloop(), default(), emptyline(),
    parseline(), do_help() functions
    >>> mycmd.use_rawinput=0
    >>> mycmd.cmdqueue=["", "add", "add 4 5", "help", "help add","exit"]
    >>> mycmd.cmdloop()
    Hello from preloop
    help text for add
    *** invalid number of arguments
    9
    <BLANKLINE>
    Documented commands (type help <topic>):
    ========================================
    add
    <BLANKLINE>
    Undocumented commands:
    ======================
    exit  help  shell
    <BLANKLINE>
    help text for add
    Hello from postloop
    """

    def preloop(self):
        print "Hello from preloop"

    def postloop(self):
        print "Hello from postloop"

    def completedefault(self, *ignored):
        print "This is the completedefault methode"
        return

    def complete_command(self):
        print "complete command"
        return

    def do_shell(self, s):
        pass

    def do_add(self, s):
        l = s.split()
        if len(l) != 2:
            print "*** invalid number of arguments"
            return
        try:
            l = [int(i) for i in l]
        except ValueError:
            print "*** arguments should be numbers"
            return
        print l[0]+l[1]

    def help_add(self):
        print "help text for add"
        return

    def do_exit(self, arg):
        return True


class TestAlternateInput(unittest.TestCase):

    class simplecmd(cmd.Cmd):

        def do_print(self, args):
            print >>self.stdout, args

        def do_EOF(self, args):
            return True


    class simplecmd2(simplecmd):

        def do_EOF(self, args):
            print >>self.stdout, '*** Unknown syntax: EOF'
            return True


    def test_file_with_missing_final_nl(self):
        input = StringIO.StringIO("print test\nprint test2")
        output = StringIO.StringIO()
        cmd = self.simplecmd(stdin=input, stdout=output)
        cmd.use_rawinput = False
        cmd.cmdloop()
        self.assertMultiLineEqual(output.getvalue(),
            ("(Cmd) test\n"
             "(Cmd) test2\n"
             "(Cmd) "))


    def test_input_reset_at_EOF(self):
        input = StringIO.StringIO("print test\nprint test2")
        output = StringIO.StringIO()
        cmd = self.simplecmd2(stdin=input, stdout=output)
        cmd.use_rawinput = False
        cmd.cmdloop()
        self.assertMultiLineEqual(output.getvalue(),
            ("(Cmd) test\n"
             "(Cmd) test2\n"
             "(Cmd) *** Unknown syntax: EOF\n"))
        input = StringIO.StringIO("print \n\n")
        output = StringIO.StringIO()
        cmd.stdin = input
        cmd.stdout = output
        cmd.cmdloop()
        self.assertMultiLineEqual(output.getvalue(),
            ("(Cmd) \n"
             "(Cmd) \n"
             "(Cmd) *** Unknown syntax: EOF\n"))


def test_main(verbose=None):
    from test import test_cmd
    test_support.run_doctest(test_cmd, verbose)
    test_support.run_unittest(TestAlternateInput)

def test_coverage(coverdir):
    trace = test_support.import_module('trace')
    tracer=trace.Trace(ignoredirs=[sys.prefix, sys.exec_prefix,],
                        trace=0, count=1)
    tracer.run('reload(cmd);test_main()')
    r=tracer.results()
    print "Writing coverage results..."
    r.write_results(show_missing=True, summary=True, coverdir=coverdir)

if __name__ == "__main__":
    if "-c" in sys.argv:
        test_coverage('/tmp/cmd.cover')
    elif "-i" in sys.argv:
        samplecmdclass().cmdloop()
    else:
        test_main()
