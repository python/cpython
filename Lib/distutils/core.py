"""distutils.core

The only module that needs to be imported to use the Distutils; provides
the 'setup' function (which is to be called from the setup script).  Also
indirectly provides the Distribution and Command classes, although they are
really defined in distutils.dist and distutils.cmd."""

# created 1999/03/01, Greg Ward

__revision__ = "$Id$"

import sys, os
from types import *
from distutils.errors import *
from distutils.dist import Distribution
from distutils.cmd import Command

# This is a barebones help message generated displayed when the user
# runs the setup script with no arguments at all.  More useful help
# is generated with various --help options: global help, list commands,
# and per-command help.
usage = """\
usage: %s [global_opts] cmd1 [cmd1_opts] [cmd2 [cmd2_opts] ...]
   or: %s --help [cmd1 cmd2 ...]
   or: %s --help-commands
   or: %s cmd --help
""" % ((os.path.basename(sys.argv[0]),) * 4)


# If DISTUTILS_DEBUG is anything other than the empty string, we run in
# debug mode.
DEBUG = os.environ.get('DISTUTILS_DEBUG')


def setup (**attrs):
    """The gateway to the Distutils: do everything your setup script
       needs to do, in a highly flexible and user-driven way.  Briefly:
       create a Distribution instance; parse the command-line, creating
       and customizing instances of the command class for each command
       found on the command-line; run each of those commands.

       The Distribution instance might be an instance of a class
       supplied via the 'distclass' keyword argument to 'setup'; if no
       such class is supplied, then the 'Distribution' class (also in
       this module) is instantiated.  All other arguments to 'setup'
       (except for 'cmdclass') are used to set attributes of the
       Distribution instance.

       The 'cmdclass' argument, if supplied, is a dictionary mapping
       command names to command classes.  Each command encountered on
       the command line will be turned into a command class, which is in
       turn instantiated; any class found in 'cmdclass' is used in place
       of the default, which is (for command 'foo_bar') class 'foo_bar'
       in module 'distutils.command.foo_bar'.  The command class must
       provide a 'user_options' attribute which is a list of option
       specifiers for 'distutils.fancy_getopt'.  Any command-line
       options between the current and the next command are used to set
       attributes of the current command object.

       When the entire command-line has been successfully parsed, calls
       the 'run()' method on each command object in turn.  This method
       will be driven entirely by the Distribution object (which each
       command object has a reference to, thanks to its constructor),
       and the command-specific options that became attributes of each
       command object."""

    from pprint import pprint               # for debugging output

    # Determine the distribution class -- either caller-supplied or
    # our Distribution (see below).
    klass = attrs.get ('distclass')
    if klass:
        del attrs['distclass']
    else:
        klass = Distribution

    # Create the Distribution instance, using the remaining arguments
    # (ie. everything except distclass) to initialize it
    dist = klass (attrs)

    # Find and parse the config file(s): they will override options from
    # the setup script, but be overridden by the command line.
    dist.parse_config_files()
    
    print "options (after parsing config files):"
    pprint (dist.command_options)

    # Parse the command line; any command-line errors are the end user's
    # fault, so turn them into SystemExit to suppress tracebacks.
    try:
        ok = dist.parse_command_line (sys.argv[1:])
    except DistutilsArgError, msg:
        sys.stderr.write (usage + "\n")
        raise SystemExit, "error: %s" % msg

    print "options (after parsing command line):"
    pprint (dist.command_options)

    # And finally, run all the commands found on the command line.
    if ok:
        try:
            dist.run_commands ()
        except KeyboardInterrupt:
            raise SystemExit, "interrupted"
        except (IOError, os.error), exc:
            # check for Python 1.5.2-style {IO,OS}Error exception objects
            if hasattr (exc, 'filename') and hasattr (exc, 'strerror'):
                if exc.filename:
                    error = "error: %s: %s" % (exc.filename, exc.strerror)
                else:
                    # two-argument functions in posix module don't
                    # include the filename in the exception object!
                    error = "error: %s" % exc.strerror
            else:
                error = "error: " + str(exc[-1])

            if DEBUG:
                sys.stderr.write(error + "\n")
                raise
            else:
                raise SystemExit, error
            
        except (DistutilsExecError,
                DistutilsFileError,
                DistutilsOptionError,
                CCompilerError), msg:
            if DEBUG:
                raise
            else:
                raise SystemExit, "error: " + str(msg)

# setup ()
