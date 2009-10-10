#! /usr/bin/env python

# Emulate some Perl command line options.
# Usage: pp [-a] [-c] [-d] [-e scriptline] [-F fieldsep] [-n] [-p] [file] ...
# Where the options mean the following:
#   -a            : together with -n or -p, splits each line into list F
#   -c            : check syntax only, do not execute any code
#   -d            : run the script under the debugger, pdb
#   -e scriptline : gives one line of the Python script; may be repeated
#   -F fieldsep   : sets the field separator for the -a option [not in Perl]
#   -n            : runs the script for each line of input
#   -p            : prints the line after the script has run
# When no script lines have been passed, the first file argument
# contains the script.  With -n or -p, the remaining arguments are
# read as input to the script, line by line.  If a file is '-'
# or missing, standard input is read.

# XXX To do:
# - add -i extension option (change files in place)
# - make a single loop over the files and lines (changes effect of 'break')?
# - add an option to specify the record separator
# - except for -n/-p, run directly from the file if at all possible

import sys
import getopt

FS = ''
SCRIPT = []
AFLAG = 0
CFLAG = 0
DFLAG = 0
NFLAG = 0
PFLAG = 0

try:
    optlist, ARGS = getopt.getopt(sys.argv[1:], 'acde:F:np')
except getopt.error, msg:
    sys.stderr.write('%s: %s\n' % (sys.argv[0], msg))
    sys.exit(2)

for option, optarg in optlist:
    if option == '-a':
        AFLAG = 1
    elif option == '-c':
        CFLAG = 1
    elif option == '-d':
        DFLAG = 1
    elif option == '-e':
        for line in optarg.split('\n'):
            SCRIPT.append(line)
    elif option == '-F':
        FS = optarg
    elif option == '-n':
        NFLAG = 1
        PFLAG = 0
    elif option == '-p':
        NFLAG = 1
        PFLAG = 1
    else:
        print option, 'not recognized???'

if not ARGS: ARGS.append('-')

if not SCRIPT:
    if ARGS[0] == '-':
        fp = sys.stdin
    else:
        fp = open(ARGS[0], 'r')
    while 1:
        line = fp.readline()
        if not line: break
        SCRIPT.append(line[:-1])
    del fp
    del ARGS[0]
    if not ARGS: ARGS.append('-')

if CFLAG:
    prologue = ['if 0:']
    epilogue = []
elif NFLAG:
    # Note that it is on purpose that AFLAG and PFLAG are
    # tested dynamically each time through the loop
    prologue = [
            'LINECOUNT = 0',
            'for FILE in ARGS:',
            '   \tif FILE == \'-\':',
            '   \t   \tFP = sys.stdin',
            '   \telse:',
            '   \t   \tFP = open(FILE, \'r\')',
            '   \tLINENO = 0',
            '   \twhile 1:',
            '   \t   \tLINE = FP.readline()',
            '   \t   \tif not LINE: break',
            '   \t   \tLINENO = LINENO + 1',
            '   \t   \tLINECOUNT = LINECOUNT + 1',
            '   \t   \tL = LINE[:-1]',
            '   \t   \taflag = AFLAG',
            '   \t   \tif aflag:',
            '   \t   \t   \tif FS: F = L.split(FS)',
            '   \t   \t   \telse: F = L.split()'
            ]
    epilogue = [
            '   \t   \tif not PFLAG: continue',
            '   \t   \tif aflag:',
            '   \t   \t   \tif FS: print FS.join(F)',
            '   \t   \t   \telse: print \' \'.join(F)',
            '   \t   \telse: print L',
            ]
else:
    prologue = ['if 1:']
    epilogue = []

# Note that we indent using tabs only, so that any indentation style
# used in 'command' will come out right after re-indentation.

program = '\n'.join(prologue) + '\n'
for line in SCRIPT:
    program += '   \t   \t' + line + '\n'
program += '\n'.join(epilogue) + '\n'

import tempfile
fp = tempfile.NamedTemporaryFile()
fp.write(program)
fp.flush()
if DFLAG:
    import pdb
    pdb.run('execfile(%r)' % (fp.name,))
else:
    execfile(fp.name)
