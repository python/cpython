import re
import os

# The regular expression for freeze directives. These are comments with the
# word macfreeze immedeately followed by a colon, followed by a directive,
# followed by argument(s)
#
# The directives supported are
# include - Include a module or file
# exclude - Exclude a module
# optional - Include a module if it is found, but don't complain if it isn't
# path - Add sys.path entries. Relative paths are relative to the source file.
#
# See the macfreeze.py main program for a real live example.
#
DIRECTIVE_RE=r'^\s*#\s*macfreeze:\s*(\S*)\s*(.*)\s*$'
REPROG=re.compile(DIRECTIVE_RE)

def findfreezedirectives(program):
    extra_modules = []
    exclude_modules = []
    optional_modules = []
    extra_path = []
    progdir, filename = os.path.split(program)
    fp = open(program)
    for line in fp.readlines():
        match = REPROG.match(line)
        if match:
            directive = match.group(1)
            argument = match.group(2)
            if directive == 'include':
                extra_modules.append(argument)
            elif directive == 'exclude':
                exclude_modules.append(argument)
            elif directive == 'optional':
                optional_modules.append(argument)
            elif directive == 'path':
                argument = os.path.join(progdir, argument)
                extra_path.append(argument)
            else:
                print '** Unknown directive', line
    return extra_modules, exclude_modules, optional_modules, extra_path
