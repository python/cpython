"""distutils.core

The only module that needs to be imported to use the Distutils; provides
the 'setup' function (which is to be called from the setup script).  Also
indirectly provides the Distribution and Command classes, although they are
really defined in distutils.dist and distutils.cmd."""

# created 1999/03/01, Greg Ward

__revision__ = "$Id$"

import sys
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
   or: %s --help
   or: %s --help-commands
   or: %s cmd --help
""" % ((sys.argv[0],) * 4)


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

    # If we had a config file, this is where we would parse it: override
    # the client-supplied command options, but be overridden by the
    # command line.

    # Parse the command line; any command-line errors are the end-users
    # fault, so turn them into SystemExit to suppress tracebacks.
    try:
        ok = dist.parse_command_line (sys.argv[1:])
    except DistutilsArgError, msg:
        sys.stderr.write (usage + "\n")
        raise SystemExit, "error: %s" % msg

    # And finally, run all the commands found on the command line.
    if ok:
        try:
            dist.run_commands ()
        except KeyboardInterrupt:
            raise SystemExit, "interrupted"
        except (OSError, IOError), exc:
            # arg, try to work with Python pre-1.5.2
            if hasattr (exc, 'filename') and hasattr (exc, 'strerror'):
                raise SystemExit, \
                      "error: %s: %s" % (exc.filename, exc.strerror)
            else:
                raise SystemExit, str (exc)
        except (DistutilsExecError, DistutilsFileError), msg:
            raise SystemExit, "error: " + str (msg)

# setup ()
