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

import string, re
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


def fancy_getopt (options, object, args):

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
    neg_alias = {}

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
            # "quiet=!verbose")?
            match = neg_alias_re.match (long)
            if match:
                (alias_from, alias_to) = match.group (1,2)
                if not takes_arg.has_key(alias_to) or takes_arg[alias_to]:
                    raise DistutilsGetoptError, \
                          ("option '%s' is a negative alias for '%s', " +
                           "which either hasn't been defined yet " +
                           "or takes an argument") % (alias_from, alias_to)

                long = alias_from
                neg_alias[long] = alias_to
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
                alias = neg_alias.get (opt)
                if alias:
                    setattr (object, attr_name[alias], 0)
                else:
                    setattr (object, attr, 1)
            else:
                raise RuntimeError, "getopt lies! (bad value '%s')" % value

    # end loop over options found in 'args'

    return args

# end fancy_getopt()
