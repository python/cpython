"""distutils.fancy_getopt

Wrapper around the standard getopt module that provides the following
additional features:
  * short and long options are tied together
  * options have help strings, so fancy_getopt could potentially
    create a complete usage summary
  * options set attributes of a passed-in object
"""

# created 1999/03/03, Greg Ward

__rcsid__ = "$Id$"

import sys, string, re
from types import *
import getopt
from distutils.errors import *

# Much like command_re in distutils.core, this is close to but not quite
# the same as a Python NAME -- except, in the spirit of most GNU
# utilities, we use '-' in place of '_'.  (The spirit of LISP lives on!)
# The similarities to NAME are again not a coincidence...
longopt_pat = r'[a-zA-Z](?:[a-zA-Z0-9-]*)'
longopt_re = re.compile (r'^%s$' % longopt_pat)

# For recognizing "negative alias" options, eg. "quiet=!verbose"
neg_alias_re = re.compile ("^(%s)=!(%s)$" % (longopt_pat, longopt_pat))


# This is used to translate long options to legitimate Python identifiers
# (for use as attributes of some object).
longopt_xlate = string.maketrans ('-', '_')


def fancy_getopt (options, negative_opt, object, args):

    # The 'options' table is a list of 3-tuples:
    #   (long_option, short_option, help_string)
    # if an option takes an argument, its long_option should have '='
    # appended; short_option should just be a single character, no ':' in
    # any case.  If a long_option doesn't have a corresponding
    # short_option, short_option should be None.  All option tuples must
    # have long options.

    # Build the short_opts string and long_opts list, remembering how
    # the two are tied together

    short_opts = []                     # we'll join 'em when done
    long_opts = []
    short2long = {}
    attr_name = {}
    takes_arg = {}

    for option in options:
        try:
            (long, short, help) = option
        except ValueError:
            raise DistutilsGetoptError, \
                  "invalid option tuple " + str (option)

        # Type-check the option names
        if type (long) is not StringType or len (long) < 2:
            raise DistutilsGetoptError, \
                  "long option '%s' must be a string of length >= 2" % \
                  long

        if (not ((short is None) or
                 (type (short) is StringType and len (short) == 1))):
            raise DistutilsGetoptError, \
                  "short option '%s' must be None or string of length 1" % \
                  short

        long_opts.append (long)

        if long[-1] == '=':             # option takes an argument?
            if short: short = short + ':'
            long = long[0:-1]
            takes_arg[long] = 1
        else:

            # Is option is a "negative alias" for some other option (eg.
            # "quiet" == "!verbose")?
            alias_to = negative_opt.get(long)
            if alias_to is not None:
                if not takes_arg.has_key(alias_to) or takes_arg[alias_to]:
                    raise DistutilsGetoptError, \
                          ("option '%s' is a negative alias for '%s', " +
                           "which either hasn't been defined yet " +
                           "or takes an argument") % (long, alias_to)

                long_opts[-1] = long
                takes_arg[long] = 0

            else:
                takes_arg[long] = 0
                

        # Now enforce some bondage on the long option name, so we can later
        # translate it to an attribute name in 'object'.  Have to do this a
        # bit late to make sure we've removed any trailing '='.
        if not longopt_re.match (long):
            raise DistutilsGetoptError, \
                  ("invalid long option name '%s' " +
                   "(must be letters, numbers, hyphens only") % long

        attr_name[long] = string.translate (long, longopt_xlate)
        if short:
            short_opts.append (short)
            short2long[short[0]] = long

    # end loop over 'options'

    short_opts = string.join (short_opts)
    try:
        (opts, args) = getopt.getopt (args, short_opts, long_opts)
    except getopt.error, msg:
        raise DistutilsArgError, msg

    for (opt, val) in opts:
        if len (opt) == 2 and opt[0] == '-': # it's a short option
            opt = short2long[opt[1]]

        elif len (opt) > 2 and opt[0:2] == '--':
            opt = opt[2:]

        else:
            raise RuntimeError, "getopt lies! (bad option string '%s')" % \
                  opt

        attr = attr_name[opt]
        if takes_arg[opt]:
            setattr (object, attr, val)
        else:
            if val == '':
                alias = negative_opt.get (opt)
                if alias:
                    setattr (object, attr_name[alias], 0)
                else:
                    setattr (object, attr, 1)
            else:
                raise RuntimeError, "getopt lies! (bad value '%s')" % value

    # end loop over options found in 'args'

    return args

# fancy_getopt()


WS_TRANS = string.maketrans (string.whitespace, ' ' * len (string.whitespace))

def wrap_text (text, width):

    if text is None:
        return []
    if len (text) <= width:
        return [text]

    text = string.expandtabs (text)
    text = string.translate (text, WS_TRANS)
    chunks = re.split (r'( +|-+)', text)
    chunks = filter (None, chunks)      # ' - ' results in empty strings
    lines = []

    while chunks:

        cur_line = []                   # list of chunks (to-be-joined)
        cur_len = 0                     # length of current line

        while chunks:
            l = len (chunks[0])
            if cur_len + l <= width:    # can squeeze (at least) this chunk in
                cur_line.append (chunks[0])
                del chunks[0]
                cur_len = cur_len + l
            else:                       # this line is full
                # drop last chunk if all space
                if cur_line and cur_line[-1][0] == ' ':
                    del cur_line[-1]
                break

        if chunks:                      # any chunks left to process?

            # if the current line is still empty, then we had a single
            # chunk that's too big too fit on a line -- so we break
            # down and break it up at the line width
            if cur_len == 0:
                cur_line.append (chunks[0][0:width])
                chunks[0] = chunks[0][width:]

            # all-whitespace chunks at the end of a line can be discarded
            # (and we know from the re.split above that if a chunk has
            # *any* whitespace, it is *all* whitespace)
            if chunks[0][0] == ' ':
                del chunks[0]

        # and store this line in the list-of-all-lines -- as a single
        # string, of course!
        lines.append (string.join (cur_line, ''))

    # while chunks

    return lines

# wrap_text ()
        

def generate_help (options, header=None):
    """Generate help text (a list of strings, one per suggested line of
       output) from an option table."""

    # Blithely assume the option table is good: probably wouldn't call
    # 'generate_help()' unless you've already called 'fancy_getopt()'.

    # First pass: determine maximum length of long option names
    max_opt = 0
    for option in options:
        long = option[0]
        short = option[1]
        l = len (long)
        if long[-1] == '=':
            l = l - 1
        if short is not None:
            l = l + 5                   # " (-x)" where short == 'x'
        if l > max_opt:
            max_opt = l
            
    opt_width = max_opt + 2 + 2 + 2     # room for indent + dashes + gutter

    # Typical help block looks like this:
    #   --foo       controls foonabulation
    # Help block for longest option looks like this:
    #   --flimflam  set the flim-flam level
    # and with wrapped text:
    #   --flimflam  set the flim-flam level (must be between
    #               0 and 100, except on Tuesdays)
    # Options with short names will have the short name shown (but
    # it doesn't contribute to max_opt):
    #   --foo (-f)  controls foonabulation
    # If adding the short option would make the left column too wide,
    # we push the explanation off to the next line
    #   --flimflam (-l)
    #               set the flim-flam level
    # Important parameters:
    #   - 2 spaces before option block start lines
    #   - 2 dashes for each long option name
    #   - min. 2 spaces between option and explanation (gutter)
    #   - 5 characters (incl. space) for short option name

    # Now generate lines of help text.
    line_width = 78                     # if 80 columns were good enough for
    text_width = line_width - opt_width # Jesus, then 78 are good enough for me
    big_indent = ' ' * opt_width
    if header:
        lines = [header]
    else:
        lines = ['Option summary:']

    for (long,short,help) in options:
       
        text = wrap_text (help, text_width)
        if long[-1] == '=':
            long = long[0:-1]

        # Case 1: no short option at all (makes life easy)
        if short is None:
            if text:
                lines.append ("  --%-*s  %s" % (max_opt, long, text[0]))
            else:
                lines.append ("  --%-*s  " % (max_opt, long))

            for l in text[1:]:
                lines.append (big_indent + l)

        # Case 2: we have a short option, so we have to include it
        # just after the long option
        else:
            opt_names = "%s (-%s)" % (long, short)
            if text:
                lines.append ("  --%-*s  %s" %
                              (max_opt, opt_names, text[0]))
            else:
                lines.append ("  --%-*s" % opt_names)

    # for loop over options

    return lines

# generate_help ()


def print_help (options, file=None, header=None):
    if file is None:
        file = sys.stdout
    for line in generate_help (options, header):
        file.write (line + "\n")
# print_help ()
    

if __name__ == "__main__":
    text = """\
Tra-la-la, supercalifragilisticexpialidocious.
How *do* you spell that odd word, anyways?
(Someone ask Mary -- she'll know [or she'll
say, "How should I know?"].)"""

    for w in (10, 20, 30, 40):
        print "width: %d" % w
        print string.join (wrap_text (text, w), "\n")
        print
