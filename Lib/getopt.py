"""Parser for command line options.

This module helps scripts to parse the command line arguments in
sys.argv.  It supports the same conventions as the Unix getopt()
function (including the special meanings of arguments of the form `-'
and `--').  Long options similar to those supported by GNU software
may be used as well via an optional third argument.  This module
provides a single function and an exception:

getopt() -- Parse command line options
GetoptError -- exception (class) raised with 'opt' attribute, which is the
option involved with the exception.
"""

# Long option support added by Lars Wirzenius <liw@iki.fi>.

# Gerrit Holl <gerrit@nl.linux.org> moved the string-based exceptions
# to class-based exceptions.

class GetoptError(Exception):
    opt = ''
    msg = ''
    def __init__(self, *args):
        self.args = args
        if len(args) == 1:
            self.msg = args[0]
        elif len(args) == 2:
            self.msg = args[0]
            self.opt = args[1]

    def __str__(self):
        return self.msg

error = GetoptError # backward compatibility

def getopt(args, shortopts, longopts = []):
    """getopt(args, options[, long_options]) -> opts, args

    Parses command line options and parameter list.  args is the
    argument list to be parsed, without the leading reference to the
    running program.  Typically, this means "sys.argv[1:]".  shortopts
    is the string of option letters that the script wants to
    recognize, with options that require an argument followed by a
    colon (i.e., the same format that Unix getopt() uses).  If
    specified, longopts is a list of strings with the names of the
    long options which should be supported.  The leading '--'
    characters should not be included in the option name.  Options
    which require an argument should be followed by an equal sign
    ('=').

    The return value consists of two elements: the first is a list of
    (option, value) pairs; the second is the list of program arguments
    left after the option list was stripped (this is a trailing slice
    of the first argument).  Each option-and-value pair returned has
    the option as its first element, prefixed with a hyphen (e.g.,
    '-x'), and the option argument as its second element, or an empty
    string if the option has no argument.  The options occur in the
    list in the same order in which they were found, thus allowing
    multiple occurrences.  Long and short options may be mixed.

    """

    opts = []
    if type(longopts) == type(""):
        longopts = [longopts]
    else:
        longopts = list(longopts)
    longopts.sort()
    while args and args[0][:1] == '-' and args[0] != '-':
        if args[0] == '--':
            args = args[1:]
            break
        if args[0][:2] == '--':
            opts, args = do_longs(opts, args[0][2:], longopts, args[1:])
        else:
            opts, args = do_shorts(opts, args[0][1:], shortopts, args[1:])

    return opts, args

def do_longs(opts, opt, longopts, args):
    try:
        i = opt.index('=')
        opt, optarg = opt[:i], opt[i+1:]
    except ValueError:
        optarg = None

    has_arg, opt = long_has_args(opt, longopts)
    if has_arg:
        if optarg is None:
            if not args:
                raise GetoptError('option --%s requires argument' % opt, opt)
            optarg, args = args[0], args[1:]
    elif optarg:
        raise GetoptError('option --%s must not have an argument' % opt, opt)
    opts.append(('--' + opt, optarg or ''))
    return opts, args

# Return:
#   has_arg?
#   full option name
def long_has_args(opt, longopts):
    optlen = len(opt)
    for i in range(len(longopts)):
        x, y = longopts[i][:optlen], longopts[i][optlen:]
        if opt != x:
            continue
        if y != '' and y != '=' and i+1 < len(longopts):
            if opt == longopts[i+1][:optlen]:
                raise GetoptError('option --%s not a unique prefix' % opt, opt)
        if longopts[i][-1:] in ('=', ):
            return 1, longopts[i][:-1]
        return 0, longopts[i]
    raise GetoptError('option --%s not recognized' % opt, opt)

def do_shorts(opts, optstring, shortopts, args):
    while optstring != '':
        opt, optstring = optstring[0], optstring[1:]
        if short_has_arg(opt, shortopts):
            if optstring == '':
                if not args:
                    raise GetoptError('option -%s requires argument' % opt, opt)
                optstring, args = args[0], args[1:]
            optarg, optstring = optstring, ''
        else:
            optarg = ''
        opts.append(('-' + opt, optarg))
    return opts, args

def short_has_arg(opt, shortopts):
    for i in range(len(shortopts)):
        if opt == shortopts[i] != ':':
            return shortopts[i+1:i+2] == ':'
    raise GetoptError('option -%s not recognized' % opt, opt)

if __name__ == '__main__':
    import sys
    print getopt(sys.argv[1:], "a:b", ["alpha=", "beta"])
