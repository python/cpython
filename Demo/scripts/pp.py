#! /usr/local/python

# Wrapper around Python to emulate the Perl -ae options:
# (1) first argument is a Python command
# (2) rest of arguments are input to the command in an implied loop
# (3) each line is put into the string L with trailing '\n' stripped
# (4) the fields of the line are put in the list F
# (5) also: FILE: full filename; LINE: full line; FP: open file object
# The command line option "-f FS" sets the field separator;
# this is available to the program as FS.

import sys
import string
import getopt

FS = ''

optlist, args = getopt.getopt(sys.argv[1:], 'f:')
for option, optarg in optlist:
	if option == '-f': FS = optarg

command = args[0]

if not args[1:]: args.append('-')

prologue = [ \
	'for FILE in args[1:]:', \
	'\tif FILE == \'-\':', \
	'\t\tFP = sys.stdin', \
	'\telse:', \
	'\t\tFP = open(FILE, \'r\')', \
	'\twhile 1:', \
	'\t\tLINE = FP.readline()', \
	'\t\tif not LINE: break', \
	'\t\tL = LINE[:-1]', \
	'\t\tif FS: F = string.splitfields(L, FS)', \
	'\t\telse: F = string.split(L)' \
	]

# Note that we indent using tabs only, so that any indentation style
# used in 'command' will come out right after re-indentation.

program = string.joinfields(prologue, '\n')
for line in string.splitfields(command, '\n'):
	program = program + ('\n\t\t' + line)
program = program + '\n'

exec(program)
