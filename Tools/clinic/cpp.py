import re
import sys

def negate(condition):
    """
    Returns a CPP conditional that is the opposite of the conditional passed in.
    """
    if condition.startswith('!'):
        return condition[1:]
    return "!" + condition

class Monitor:
    """
    A simple C preprocessor that scans C source and computes, line by line,
    what the current C preprocessor #if state is.

    Doesn't handle everything--for example, if you have /* inside a C string,
    without a matching */ (also inside a C string), or with a */ inside a C
    string but on another line and with preprocessor macros in between...
    the parser will get lost.

    Anyway this implementation seems to work well enough for the CPython sources.
    """

    is_a_simple_defined = re.compile(r'^defined\s*\(\s*[A-Za-z0-9_]+\s*\)$').match

    def __init__(self, filename=None, *, verbose=False):
        self.stack = []
        self.in_comment = False
        self.continuation = None
        self.line_number = 0
        self.filename = filename
        self.verbose = verbose

    def __repr__(self):
        return ''.join((
            '<Monitor ',
            str(id(self)),
            " line=", str(self.line_number),
            " condition=", repr(self.condition()),
            ">"))

    def status(self):
        return str(self.line_number).rjust(4) + ": " + self.condition()

    def condition(self):
        """
        Returns the current preprocessor state, as a single #if condition.
        """
        return " && ".join(condition for token, condition in self.stack)

    def fail(self, *a):
        if self.filename:
            filename = " " + self.filename
        else:
            filename = ''
        print("Error at" + filename, "line", self.line_number, ":")
        print("   ", ' '.join(str(x) for x in a))
        sys.exit(-1)

    def close(self):
        if self.stack:
            self.fail("Ended file while still in a preprocessor conditional block!")

    def write(self, s):
        for line in s.split("\n"):
            self.writeline(line)

    def writeline(self, line):
        self.line_number += 1
        line = line.strip()

        def pop_stack():
            if not self.stack:
                self.fail("#" + token + " without matching #if / #ifdef / #ifndef!")
            return self.stack.pop()

        if self.continuation:
            line = self.continuation + line
            self.continuation = None

        if not line:
            return

        if line.endswith('\\'):
            self.continuation = line[:-1].rstrip() + " "
            return

        # we have to ignore preprocessor commands inside comments
        #
        # we also have to handle this:
        #     /* start
        #     ...
        #     */   /*    <-- tricky!
        #     ...
        #     */
        # and this:
        #     /* start
        #     ...
        #     */   /* also tricky! */
        if self.in_comment:
            if '*/' in line:
                # snip out the comment and continue
                #
                # GCC allows
                #    /* comment
                #    */ #include <stdio.h>
                # maybe other compilers too?
                _, _, line = line.partition('*/')
                self.in_comment = False

        while True:
            if '/*' in line:
                if self.in_comment:
                    self.fail("Nested block comment!")

                before, _, remainder = line.partition('/*')
                comment, comment_ends, after = remainder.partition('*/')
                if comment_ends:
                    # snip out the comment
                    line = before.rstrip() + ' ' + after.lstrip()
                    continue
                # comment continues to eol
                self.in_comment = True
                line = before.rstrip()
            break

        # we actually have some // comments
        # (but block comments take precedence)
        before, line_comment, comment = line.partition('//')
        if line_comment:
            line = before.rstrip()

        if not line.startswith('#'):
            return

        line = line[1:].lstrip()
        assert line

        fields = line.split()
        token = fields[0].lower()
        condition = ' '.join(fields[1:]).strip()

        if_tokens = {'if', 'ifdef', 'ifndef'}
        all_tokens = if_tokens | {'elif', 'else', 'endif'}

        if token not in all_tokens:
            return

        # cheat a little here, to reuse the implementation of if
        if token == 'elif':
            pop_stack()
            token = 'if'

        if token in if_tokens:
            if not condition:
                self.fail("Invalid format for #" + token + " line: no argument!")
            if token == 'if':
                if not self.is_a_simple_defined(condition):
                    condition = "(" + condition + ")"
            else:
                fields = condition.split()
                if len(fields) != 1:
                    self.fail("Invalid format for #" + token + " line: should be exactly one argument!")
                symbol = fields[0]
                condition = 'defined(' + symbol + ')'
                if token == 'ifndef':
                    condition = '!' + condition

            self.stack.append(("if", condition))
            if self.verbose:
                print(self.status())
            return

        previous_token, previous_condition = pop_stack()

        if token == 'else':
            self.stack.append(('else', negate(previous_condition)))
        elif token == 'endif':
            pass
        if self.verbose:
            print(self.status())

if __name__ == '__main__':
    for filename in sys.argv[1:]:
        with open(filename, "rt") as f:
            cpp = Monitor(filename, verbose=True)
            print()
            print(filename)
            for line_number, line in enumerate(f.read().split('\n'), 1):
                cpp.writeline(line)
