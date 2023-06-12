import re
import sys
from collections.abc import Callable
from typing import NoReturn


TokenAndCondition = tuple[str, str]
TokenStack = list[TokenAndCondition]

def negate(condition: str) -> str:
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

    is_a_simple_defined: Callable[[str], re.Match[str] | None]
    is_a_simple_defined = re.compile(r'^defined\s*\(\s*[A-Za-z0-9_]+\s*\)$').match

    def __init__(self, filename: str | None = None, *, verbose: bool = False) -> None:
        self.stack: TokenStack = []
        self.in_comment = False
        self.continuation: str | None = None
        self.line_number = 0
        self.filename = filename
        self.verbose = verbose

    def __repr__(self) -> str:
        return ''.join((
            '<Monitor ',
            str(id(self)),
            " line=", str(self.line_number),
            " condition=", repr(self.condition()),
            ">"))

    def status(self) -> str:
        return str(self.line_number).rjust(4) + ": " + self.condition()

    def condition(self) -> str:
        """
        Returns the current preprocessor state, as a single #if condition.
        """
        return " && ".join(condition for token, condition in self.stack)

    def fail(self, *a: object) -> NoReturn:
        if self.filename:
            filename = " " + self.filename
        else:
            filename = ''
        print("Error at" + filename, "line", self.line_number, ":")
        print("   ", ' '.join(str(x) for x in a))
        sys.exit(-1)

    def close(self) -> None:
        if self.stack:
            self.fail("Ended file while still in a preprocessor conditional block!")

    def write(self, s: str) -> None:
        for line in s.split("\n"):
            self.writeline(line)

    def writeline(self, line: str) -> None:
        self.line_number += 1
        line = line.strip()

        def pop_stack() -> TokenAndCondition:
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

        if token in {'if', 'ifdef', 'ifndef', 'elif'}:
            if not condition:
                self.fail("Invalid format for #" + token + " line: no argument!")
            if token in {'if', 'elif'}:
                if not self.is_a_simple_defined(condition):
                    condition = "(" + condition + ")"
                if token == 'elif':
                    previous_token, previous_condition = pop_stack()
                    self.stack.append((previous_token, negate(previous_condition)))
            else:
                fields = condition.split()
                if len(fields) != 1:
                    self.fail("Invalid format for #" + token + " line: should be exactly one argument!")
                symbol = fields[0]
                condition = 'defined(' + symbol + ')'
                if token == 'ifndef':
                    condition = '!' + condition
                token = 'if'

            self.stack.append((token, condition))

        elif token == 'else':
            previous_token, previous_condition = pop_stack()
            self.stack.append((previous_token, negate(previous_condition)))

        elif token == 'endif':
            while pop_stack()[0] != 'if':
                pass

        else:
            return

        if self.verbose:
            print(self.status())

if __name__ == '__main__':
    for filename in sys.argv[1:]:
        with open(filename) as f:
            cpp = Monitor(filename, verbose=True)
            print()
            print(filename)
            for line_number, line in enumerate(f.read().split('\n'), 1):
                cpp.writeline(line)
