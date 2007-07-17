# Coroutine example:  general coroutine transfers
#
# The program is a variation of a Simula 67 program due to Dahl & Hoare,
# (Dahl/Dijkstra/Hoare, Structured Programming; Academic Press, 1972)
# who in turn credit the original example to Conway.
#
# We have a number of input lines, terminated by a 0 byte.  The problem
# is to squash them together into output lines containing 72 characters
# each.  A semicolon must be added between input lines.  Runs of blanks
# and tabs in input lines must be squashed into single blanks.
# Occurrences of "**" in input lines must be replaced by "^".
#
# Here's a test case:

test = """\
   d    =   sqrt(b**2  -  4*a*c)
twoa    =   2*a
   L    =   -b/twoa
   R    =   d/twoa
  A1    =   L + R
  A2    =   L - R\0
"""

# The program should print:

# d = sqrt(b^2 - 4*a*c);twoa = 2*a; L = -b/twoa; R = d/twoa; A1 = L + R;
#A2 = L - R
#done

# getline: delivers the next input line to its invoker
# disassembler: grabs input lines from getline, and delivers them one
#    character at a time to squasher, also inserting a semicolon into
#    the stream between lines
# squasher:  grabs characters from disassembler and passes them on to
#    assembler, first replacing "**" with "^" and squashing runs of
#    whitespace
# assembler: grabs characters from squasher and packs them into lines
#    with 72 character each, delivering each such line to putline;
#    when it sees a null byte, passes the last line to putline and
#    then kills all the coroutines
# putline: grabs lines from assembler, and just prints them

from Coroutine import *

def getline(text):
    for line in string.splitfields(text, '\n'):
        co.tran(codisassembler, line)

def disassembler():
    while 1:
        card = co.tran(cogetline)
        for i in range(len(card)):
            co.tran(cosquasher, card[i])
        co.tran(cosquasher, ';')

def squasher():
    while 1:
        ch = co.tran(codisassembler)
        if ch == '*':
            ch2 = co.tran(codisassembler)
            if ch2 == '*':
                ch = '^'
            else:
                co.tran(coassembler, ch)
                ch = ch2
        if ch in ' \t':
            while 1:
                ch2 = co.tran(codisassembler)
                if ch2 not in ' \t':
                    break
            co.tran(coassembler, ' ')
            ch = ch2
        co.tran(coassembler, ch)

def assembler():
    line = ''
    while 1:
        ch = co.tran(cosquasher)
        if ch == '\0':
            break
        if len(line) == 72:
            co.tran(coputline, line)
            line = ''
        line = line + ch
    line = line + ' ' * (72 - len(line))
    co.tran(coputline, line)
    co.kill()

def putline():
    while 1:
        line = co.tran(coassembler)
        print(line)

import string
co = Coroutine()
cogetline = co.create(getline, test)
coputline = co.create(putline)
coassembler = co.create(assembler)
codisassembler = co.create(disassembler)
cosquasher = co.create(squasher)

co.tran(coputline)
print('done')

# end of example
