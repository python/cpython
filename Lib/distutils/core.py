"""distutils.core

The only module that needs to be imported to use the Distutils; provides
the 'setup' function (which must be called); the 'Distribution' class
(which may be subclassed if additional functionality is desired), and
the 'Command' class (which is used both internally by Distutils, and
may be subclassed by clients for still more flexibility)."""

# created 1999/03/01, Greg Ward

__rcsid__ = "$Id$"

import sys, os
import string, re
from types import *
from copy import copy
from distutils.errors import *
from distutils.fancy_getopt import fancy_getopt, print_help
from distutils import util

# Regex to define acceptable Distutils command names.  This is not *quite*
# the same as a Python NAME -- I don't allow leading underscores.  The fact
# that they're very similar is no coincidence; the default naming scheme is
# to look for a Python module named after the command.
command_re = re.compile (r'^[a-zA-Z]([a-zA-Z0-9_]*)$')

# Defining this as a global is probably inadequate -- what about
# listing the available options (or even commands, which can vary
# quite late as well)
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
       command names to command classes.  Each command encountered on the
       command line will be turned into a command class, which is in turn
       instantiated; any class found in 'cmdclass' is used in place of the
       default, which is (for command 'foo_bar') class 'FooBar' in module
       'distutils.command.foo_bar'.  The command object must provide an
       'options' attribute which is a list of option specifiers for
       'distutils.fancy_getopt'.  Any command-line options between the
       current and the next command are used to set attributes in the
       current command object.

       When the entire command-line has been successfully parsed, calls the
       'run' method on each command object in turn.  This method will be
       driven entirely by the Distribution object (which each command
       object has a reference to, thanks to its constructor), and the
       command-specific options that became attributes of each command
       object."""

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
        except IOError, exc:
            # is this 1.5.2-specific? 1.5-specific?
            raise SystemExit, "error: %s: %s" % (exc.filename, exc.strerror)

# setup ()


class Distribution:
    """The core of the Distutils.  Most of the work hiding behind
       'setup' is really done within a Distribution instance, which
       farms the work out to the Distutils commands specified on the
       command line.

       Clients will almost never instantiate Distribution directly,
       unless the 'setup' function is totally inadequate to their needs.
       However, it is conceivable that a client might wish to subclass
       Distribution for some specialized purpose, and then pass the
       subclass to 'setup' as the 'distclass' keyword argument.  If so,
       it is necessary to respect the expectations that 'setup' has of
       Distribution: it must have a constructor and methods
       'parse_command_line()' and 'run_commands()' with signatures like
       those described below."""


    # 'global_options' describes the command-line options that may be
    # supplied to the client (setup.py) prior to any actual commands.
    # Eg. "./setup.py -nv" or "./setup.py --verbose" both take advantage of
    # these global options.  This list should be kept to a bare minimum,
    # since every global option is also valid as a command option -- and we
    # don't want to pollute the commands with too many options that they
    # have minimal control over.
    global_options = [('verbose', 'v',
                       "run verbosely (default)"),
                      ('quiet', 'q',
                       "run quietly (turns verbosity off)"),
                      ('dry-run', 'n',
                       "don't actually do anything"),
                      ('force', 'f',
                       "skip dependency checking between files"),
                      ('help', 'h',
                       "show this help message"),
                     ]
    negative_opt = {'quiet': 'verbose'}


    # -- Creation/initialization methods -------------------------------
    
    def __init__ (self, attrs=None):
        """Construct a new Distribution instance: initialize all the
           attributes of a Distribution, and then uses 'attrs' (a
           dictionary mapping attribute names to values) to assign
           some of those attributes their "real" values.  (Any attributes
           not mentioned in 'attrs' will be assigned to some null
           value: 0, None, an empty list or dictionary, etc.)  Most
           importantly, initialize the 'command_obj' attribute
           to the empty dictionary; this will be filled in with real
           command objects by 'parse_command_line()'."""

        # Default values for our command-line options
        self.verbose = 1
        self.dry_run = 0
        self.force = 0
        self.help = 0
        self.help_commands = 0

        # And the "distribution meta-data" options -- these can only
        # come from setup.py (the caller), not the command line
        # (or a hypothetical config file).
        self.name = None
        self.version = None
        self.author = None
        self.author_email = None
        self.maintainer = None
        self.maintainer_email = None
        self.url = None
        self.licence = None
        self.description = None

        # 'cmdclass' maps command names to class objects, so we
        # can 1) quickly figure out which class to instantiate when
        # we need to create a new command object, and 2) have a way
        # for the client to override command classes
        self.cmdclass = {}

        # These options are really the business of various commands, rather
        # than of the Distribution itself.  We provide aliases for them in
        # Distribution as a convenience to the developer.
        # dictionary.        
        self.packages = None
        self.package_dir = None
        self.py_modules = None
        self.libraries = None
        self.ext_modules = None
        self.ext_package = None
        self.include_dirs = None
        self.install_path = None

        # And now initialize bookkeeping stuff that can't be supplied by
        # the caller at all.  'command_obj' maps command names to
        # Command instances -- that's how we enforce that every command
        # class is a singleton.
        self.command_obj = {}

        # 'have_run' maps command names to boolean values; it keeps track
        # of whether we have actually run a particular command, to make it
        # cheap to "run" a command whenever we think we might need to -- if
        # it's already been done, no need for expensive filesystem
        # operations, we just check the 'have_run' dictionary and carry on.
        # It's only safe to query 'have_run' for a command class that has
        # been instantiated -- a false value will be inserted when the
        # command object is created, and replaced with a true value when
        # the command is succesfully run.  Thus it's probably best to use
        # '.get()' rather than a straight lookup.
        self.have_run = {}

        # Now we'll use the attrs dictionary (ultimately, keyword args from
        # the client) to possibly override any or all of these distribution
        # options.        
        if attrs:

            # Pull out the set of command options and work on them
            # specifically.  Note that this order guarantees that aliased
            # command options will override any supplied redundantly
            # through the general options dictionary.
            options = attrs.get ('options')
            if options:
                del attrs['options']
                for (command, cmd_options) in options.items():
                    cmd_obj = self.find_command_obj (command)
                    for (key, val) in cmd_options.items():
                        cmd_obj.set_option (key, val)
                # loop over commands
            # if any command options                        

            # Now work on the rest of the attributes.  Any attribute that's
            # not already defined is invalid!
            for (key,val) in attrs.items():
                if hasattr (self, key):
                    setattr (self, key, val)
                else:
                    raise DistutilsOptionError, \
                          "invalid distribution option '%s'" % key

    # __init__ ()


    def parse_command_line (self, args):
        """Parse the setup script's command line: set any Distribution
           attributes tied to command-line options, create all command
           objects, and set their options from the command-line.  'args'
           must be a list of command-line arguments, most likely
           'sys.argv[1:]' (see the 'setup()' function).  This list is first
           processed for "global options" -- options that set attributes of
           the Distribution instance.  Then, it is alternately scanned for
           Distutils command and options for that command.  Each new
           command terminates the options for the previous command.  The
           allowed options for a command are determined by the 'options'
           attribute of the command object -- thus, we instantiate (and
           cache) every command object here, in order to access its
           'options' attribute.  Any error in that 'options' attribute
           raises DistutilsGetoptError; any error on the command-line
           raises DistutilsArgError.  If no Distutils commands were found
           on the command line, raises DistutilsArgError.  Return true if
           command-line successfully parsed and we should carry on with
           executing commands; false if no errors but we shouldn't execute
           commands (currently, this only happens if user asks for
           help)."""

        # We have to parse the command line a bit at a time -- global
        # options, then the first command, then its options, and so on --
        # because each command will be handled by a different class, and
        # the options that are valid for a particular class aren't
        # known until we instantiate the command class, which doesn't
        # happen until we know what the command is.

        self.commands = []
        options = self.global_options + \
                  [('help-commands', None,
                    "list all available commands")]
        args = fancy_getopt (options, self.negative_opt,
                             self, sys.argv[1:])

        # User just wants a list of commands -- we'll print it out and stop
        # processing now (ie. if they ran "setup --help-commands foo bar",
        # we ignore "foo bar").
        if self.help_commands:
            self.print_commands ()
            print
            print usage
            return
            
        while args:
            # Pull the current command from the head of the command line
            command = args[0]
            if not command_re.match (command):
                raise SystemExit, "invalid command name '%s'" % command
            self.commands.append (command)

            # Make sure we have a command object to put the options into
            # (this either pulls it out of a cache of command objects,
            # or finds and instantiates the command class).
            try:
                cmd_obj = self.find_command_obj (command)
            except DistutilsModuleError, msg:
                raise DistutilsArgError, msg

            # Require that the command class be derived from Command --
            # that way, we can be sure that we at least have the 'run'
            # and 'get_option' methods.
            if not isinstance (cmd_obj, Command):
                raise DistutilsClassError, \
                      "command class %s must subclass Command" % \
                      cmd_obj.__class__

            # Also make sure that the command object provides a list of its
            # known options
            if not (hasattr (cmd_obj, 'options') and
                    type (cmd_obj.options) is ListType):
                raise DistutilsClassError, \
                      ("command class %s must provide an 'options' attribute "+
                       "(a list of tuples)") % \
                      cmd_obj.__class__

            # Poof! like magic, all commands support the global
            # options too, just by adding in 'global_options'.
            negative_opt = self.negative_opt
            if hasattr (cmd_obj, 'negative_opt'):
                negative_opt = copy (negative_opt)
                negative_opt.update (cmd_obj.negative_opt)

            options = self.global_options + cmd_obj.options
            args = fancy_getopt (options, negative_opt,
                                 cmd_obj, args[1:])
            if cmd_obj.help:
                print_help (self.global_options,
                            header="Global options:")
                print
                print_help (cmd_obj.options,
                            header="Options for '%s' command:" % command)
                print
                print usage
                return
                
            self.command_obj[command] = cmd_obj
            self.have_run[command] = 0

        # while args

        # If the user wants help -- ie. they gave the "--help" option --
        # give it to 'em.  We do this *after* processing the commands in
        # case they want help on any particular command, eg.
        # "setup.py --help foo".  (This isn't the documented way to
        # get help on a command, but I support it because that's how
        # CVS does it -- might as well be consistent.)
        if self.help:
            print_help (self.global_options, header="Global options:")
            print

            for command in self.commands:
                klass = self.find_command_class (command)
                print_help (klass.options,
                            header="Options for '%s' command:" % command)
                print

            print usage
            return

        # Oops, no commands found -- an end-user error
        if not self.commands:
            raise DistutilsArgError, "no commands supplied"

        # All is well: return true
        return 1

    # parse_command_line()


    def print_command_list (self, commands, header, max_length):
        """Print a subset of the list of all commands -- used by
           'print_commands()'."""

        print header + ":"

        for cmd in commands:
            klass = self.cmdclass.get (cmd)
            if not klass:
                klass = self.find_command_class (cmd)
            try:
                description = klass.description
            except AttributeError:
                description = "(no description available)"

            print "  %-*s  %s" % (max_length, cmd, description)

    # print_command_list ()


    def print_commands (self):
        """Print out a help message listing all available commands with
           a description of each.  The list is divided into "standard
           commands" (listed in distutils.command.__all__) and "extra
           commands" (mentioned in self.cmdclass, but not a standard
           command).  The descriptions come from the command class
           attribute 'description'."""

        import distutils.command
        std_commands = distutils.command.__all__
        is_std = {}
        for cmd in std_commands:
            is_std[cmd] = 1

        extra_commands = []
        for cmd in self.cmdclass.keys():
            if not is_std.get(cmd):
                extra_commands.append (cmd)

        max_length = 0
        for cmd in (std_commands + extra_commands):
            if len (cmd) > max_length:
                max_length = len (cmd)

        self.print_command_list (std_commands,
                                 "Standard commands",
                                 max_length)
        if extra_commands:
            print
            self.print_command_list (extra_commands,
                                     "Extra commands",
                                     max_length)

    # print_commands ()
        


    # -- Command class/object methods ----------------------------------

    # This is a method just so it can be overridden if desired; it doesn't
    # actually use or change any attributes of the Distribution instance.
    def find_command_class (self, command):
        """Given a command, derives the names of the module and class
           expected to implement the command: eg. 'foo_bar' becomes
           'distutils.command.foo_bar' (the module) and 'FooBar' (the
           class within that module).  Loads the module, extracts the
           class from it, and returns the class object.

           Raises DistutilsModuleError with a semi-user-targeted error
           message if the expected module could not be loaded, or the
           expected class was not found in it."""

        module_name = 'distutils.command.' + command
        klass_name = string.join \
            (map (string.capitalize, string.split (command, '_')), '')

        try:
            __import__ (module_name)
            module = sys.modules[module_name]
        except ImportError:
            raise DistutilsModuleError, \
                  "invalid command '%s' (no module named '%s')" % \
                  (command, module_name)

        try:
            klass = vars(module)[klass_name]
        except KeyError:
            raise DistutilsModuleError, \
                  "invalid command '%s' (no class '%s' in module '%s')" \
                  % (command, klass_name, module_name)

        return klass

    # find_command_class ()


    def create_command_obj (self, command):
        """Figure out the class that should implement a command,
           instantiate it, cache and return the new "command object".
           The "command class" is determined either by looking it up in
           the 'cmdclass' attribute (this is the mechanism whereby
           clients may override default Distutils commands or add their
           own), or by calling the 'find_command_class()' method (if the
           command name is not in 'cmdclass'."""

        # Determine the command class -- either it's in the command_class
        # dictionary, or we have to divine the module and class name
        klass = self.cmdclass.get(command)
        if not klass:
            klass = self.find_command_class (command)
            self.cmdclass[command] = klass

        # Found the class OK -- instantiate it 
        cmd_obj = klass (self)
        return cmd_obj
    

    def find_command_obj (self, command, create=1):
        """Look up and return a command object in the cache maintained by
           'create_command_obj()'.  If none found, the action taken
           depends on 'create': if true (the default), create a new
           command object by calling 'create_command_obj()' and return
           it; otherwise, return None.  If 'command' is an invalid
           command name, then DistutilsModuleError will be raised."""

        cmd_obj = self.command_obj.get (command)
        if not cmd_obj and create:
            cmd_obj = self.create_command_obj (command)
            self.command_obj[command] = cmd_obj

        return cmd_obj

        
    # -- Methods that operate on the Distribution ----------------------

    def announce (self, msg, level=1):
        """Print 'msg' if 'level' is greater than or equal to the verbosity
           level recorded in the 'verbose' attribute (which, currently,
           can be only 0 or 1)."""

        if self.verbose >= level:
            print msg


    def run_commands (self):
        """Run each command that was seen on the client command line.
           Uses the list of commands found and cache of command objects
           created by 'create_command_obj()'."""

        for cmd in self.commands:
            self.run_command (cmd)


    def get_option (self, option):
        """Return the value of a distribution option.  Raise
           DistutilsOptionError if 'option' is not known."""

        try:
            return getattr (self, opt)
        except AttributeError:
            raise DistutilsOptionError, \
                  "unknown distribution option %s" % option


    def get_options (self, *options):
        """Return (as a tuple) the values of several distribution
           options.  Raise DistutilsOptionError if any element of
           'options' is not known."""
        
        values = []
        try:
            for opt in options:
                values.append (getattr (self, opt))
        except AttributeError, name:
            raise DistutilsOptionError, \
                  "unknown distribution option %s" % name

        return tuple (values)


    # -- Methods that operate on its Commands --------------------------

    def run_command (self, command):

        """Do whatever it takes to run a command (including nothing at all,
           if the command has already been run).  Specifically: if we have
           already created and run the command named by 'command', return
           silently without doing anything.  If the command named by
           'command' doesn't even have a command object yet, create one.
           Then invoke 'run()' on that command object (or an existing
           one)."""

        # Already been here, done that? then return silently.
        if self.have_run.get (command):
            return

        self.announce ("running " + command)
        cmd_obj = self.find_command_obj (command)
        cmd_obj.ensure_ready ()
        cmd_obj.run ()
        self.have_run[command] = 1


    def get_command_option (self, command, option):
        """Create a command object for 'command' if necessary, ensure that
           its option values are all set to their final values, and return
           the value of its 'option' option.  Raise DistutilsOptionError if
           'option' is not known for that 'command'."""

        cmd_obj = self.find_command_obj (command)
        cmd_obj.ensure_ready ()
        return cmd_obj.get_option (option)
        try:
            return getattr (cmd_obj, option)
        except AttributeError:
            raise DistutilsOptionError, \
                  "command %s: no such option %s" % (command, option)


    def get_command_options (self, command, *options):
        """Create a command object for 'command' if necessary, ensure that
           its option values are all set to their final values, and return
           a tuple containing the values of all the options listed in
           'options' for that command.  Raise DistutilsOptionError if any
           invalid option is supplied in 'options'."""

        cmd_obj = self.find_command_obj (command)
        cmd_obj.ensure_ready ()
        values = []
        try:
            for opt in options:
                values.append (getattr (cmd_obj, option))
        except AttributeError, name:
            raise DistutilsOptionError, \
                  "command %s: no such option %s" % (command, name)

        return tuple (values)

# end class Distribution


class Command:
    """Abstract base class for defining command classes, the "worker bees"
       of the Distutils.  A useful analogy for command classes is to
       think of them as subroutines with local variables called
       "options".  The options are "declared" in 'set_default_options()'
       and "initialized" (given their real values) in
       'set_final_options()', both of which must be defined by every
       command class.  The distinction between the two is necessary
       because option values might come from the outside world (command
       line, option file, ...), and any options dependent on other
       options must be computed *after* these outside influences have
       been processed -- hence 'set_final_options()'.  The "body" of the
       subroutine, where it does all its work based on the values of its
       options, is the 'run()' method, which must also be implemented by
       every command class."""

    # -- Creation/initialization methods -------------------------------

    def __init__ (self, dist):
        """Create and initialize a new Command object.  Most importantly,
           invokes the 'set_default_options()' method, which is the
           real initializer and depends on the actual command being
           instantiated."""

        if not isinstance (dist, Distribution):
            raise TypeError, "dist must be a Distribution instance"
        if self.__class__ is Command:
            raise RuntimeError, "Command is an abstract class"

        self.distribution = dist
        self.set_default_options ()

        # Per-command versions of the global flags, so that the user can
        # customize Distutils' behaviour command-by-command and let some
        # commands fallback on the Distribution's behaviour.  None means
        # "not defined, check self.distribution's copy", while 0 or 1 mean
        # false and true (duh).  Note that this means figuring out the real
        # value of each flag is a touch complicatd -- hence "self.verbose"
        # (etc.) will be handled by __getattr__, below.
        self._verbose = None
        self._dry_run = None
        self._force = None

        # The 'help' flag is just used for command-line parsing, so
        # none of that complicated bureaucracy is needed.
        self.help = 0

        # 'ready' records whether or not 'set_final_options()' has been
        # called.  'set_final_options()' itself should not pay attention to
        # this flag: it is the business of 'ensure_ready()', which always
        # calls 'set_final_options()', to respect/update it.
        self.ready = 0

    # end __init__ ()


    def __getattr__ (self, attr):
        if attr in ('verbose', 'dry_run', 'force'):
            myval = getattr (self, "_" + attr)
            if myval is None:
                return getattr (self.distribution, attr)
            else:
                return myval
        else:
            raise AttributeError, attr


    def ensure_ready (self):
        if not self.ready:
            self.set_final_options ()
        self.ready = 1
        

    # Subclasses must define:
    #   set_default_options()
    #     provide default values for all options; may be overridden
    #     by Distutils client, by command-line options, or by options
    #     from option file
    #   set_final_options()
    #     decide on the final values for all options; this is called
    #     after all possible intervention from the outside world
    #     (command-line, option file, etc.) has been processed
    #   run()
    #     run the command: do whatever it is we're here to do,
    #     controlled by the command's various option values

    def set_default_options (self):
        """Set default values for all the options that this command
           supports.  Note that these defaults may be overridden
           by the command-line supplied by the user; thus, this is
           not the place to code dependencies between options; generally,
           'set_default_options()' implementations are just a bunch
           of "self.foo = None" assignments.
           
           This method must be implemented by all command classes."""
           
        raise RuntimeError, \
              "abstract method -- subclass %s must override" % self.__class__
        
    def set_final_options (self):
        """Set final values for all the options that this command
           supports.  This is always called as late as possible, ie.
           after any option assignments from the command-line or from
           other commands have been done.  Thus, this is the place to to
           code option dependencies: if 'foo' depends on 'bar', then it
           is safe to set 'foo' from 'bar' as long as 'foo' still has
           the same value it was assigned in 'set_default_options()'.

           This method must be implemented by all command classes."""
           
        raise RuntimeError, \
              "abstract method -- subclass %s must override" % self.__class__

    def run (self):
        """A command's raison d'etre: carry out the action it exists
           to perform, controlled by the options initialized in
           'set_initial_options()', customized by the user and other
           commands, and finalized in 'set_final_options()'.  All
           terminal output and filesystem interaction should be done by
           'run()'.

           This method must be implemented by all command classes."""

        raise RuntimeError, \
              "abstract method -- subclass %s must override" % self.__class__

    def announce (self, msg, level=1):
        """If the Distribution instance to which this command belongs
           has a verbosity level of greater than or equal to 'level'
           print 'msg' to stdout."""
    
        if self.verbose >= level:
            print msg


    # -- Option query/set methods --------------------------------------

    def get_option (self, option):
        """Return the value of a single option for this command.  Raise
           DistutilsOptionError if 'option' is not known."""
        try:
            return getattr (self, option)
        except AttributeError:
            raise DistutilsOptionError, \
                  "command %s: no such option %s" % \
                  (self.get_command_name(), option)


    def get_options (self, *options):
        """Return (as a tuple) the values of several options for this
           command.  Raise DistutilsOptionError if any of the options in
           'options' are not known."""

        values = []
        try:
            for opt in options:
                values.append (getattr (self, opt))
        except AttributeError, name:
            raise DistutilsOptionError, \
                  "command %s: no such option %s" % \
                  (self.get_command_name(), name)
            
        return tuple (values)
    

    def set_option (self, option, value):
        """Set the value of a single option for this command.  Raise
           DistutilsOptionError if 'option' is not known."""

        if not hasattr (self, option):
            raise DistutilsOptionError, \
                  "command '%s': no such option '%s'" % \
                  (self.get_command_name(), option)
        if value is not None:
            setattr (self, option, value)

    def set_options (self, **optval):
        """Set the values of several options for this command.  Raise
           DistutilsOptionError if any of the options specified as
           keyword arguments are not known."""

        for k in optval.keys():
            if optval[k] is not None:
                self.set_option (k, optval[k])


    # -- Convenience methods for commands ------------------------------

    def get_command_name (self):
        if hasattr (self, 'command_name'):
            return self.command_name
        else:
            class_name = self.__class__.__name__

            # The re.split here returs empty strings delimited by the
            # words we're actually interested in -- e.g.  "FooBarBaz"
            # splits to ['', 'Foo', '', 'Bar', '', 'Baz', ''].  Hence
            # the 'filter' to strip out the empties.            
            words = filter (None, re.split (r'([A-Z][a-z]+)', class_name))
            self.command_name = string.join (map (string.lower, words), "_")
            return self.command_name


    def set_undefined_options (self, src_cmd, *option_pairs):
        """Set the values of any "undefined" options from corresponding
           option values in some other command object.  "Undefined" here
           means "is None", which is the convention used to indicate
           that an option has not been changed between
           'set_initial_values()' and 'set_final_values()'.  Usually
           called from 'set_final_values()' for options that depend on
           some other command rather than another option of the same
           command.  'src_cmd' is the other command from which option
           values will be taken (a command object will be created for it
           if necessary); the remaining arguments are
           '(src_option,dst_option)' tuples which mean "take the value
           of 'src_option' in the 'src_cmd' command object, and copy it
           to 'dst_option' in the current command object"."""

        # Option_pairs: list of (src_option, dst_option) tuples

        src_cmd_obj = self.distribution.find_command_obj (src_cmd)
        src_cmd_obj.set_final_options ()
        try:
            for (src_option, dst_option) in option_pairs:
                if getattr (self, dst_option) is None:
                    self.set_option (dst_option,
                                     src_cmd_obj.get_option (src_option))
        except AttributeError, name:
            # duh, which command?
            raise DistutilsOptionError, "unknown option %s" % name


    def set_peer_option (self, command, option, value):
        """Attempt to simulate a command-line override of some option
           value in another command.  Finds the command object for
           'command', sets its 'option' to 'value', and unconditionally
           calls 'set_final_options()' on it: this means that some command
           objects may have 'set_final_options()' invoked more than once.
           Even so, this is not entirely reliable: the other command may
           already be initialized to its satisfaction, in which case the
           second 'set_final_options()' invocation will have little or no
           effect."""

        cmd_obj = self.distribution.find_command_obj (command)
        cmd_obj.set_option (option, value)
        cmd_obj.set_final_options ()


    def find_peer (self, command, create=1):
        """Wrapper around Distribution's 'find_command_obj()' method:
           find (create if necessary and 'create' is true) the command
           object for 'command'.."""

        return self.distribution.find_command_obj (command, create)


    def get_peer_option (self, command, option):
        """Find or create the command object for 'command', and return
           its 'option' option."""

        cmd_obj = self.distribution.find_command_obj (command)
        return cmd_obj.get_option (option)


    def run_peer (self, command):
        """Run some other command: uses the 'run_command()' method of
           Distribution, which creates the command object if necessary
           and then invokes its 'run()' method."""

        self.distribution.run_command (command)


    # -- External world manipulation -----------------------------------

    def warn (self, msg):
        sys.stderr.write ("warning: %s: %s\n" %
                          (self.get_command_name(), msg))


    def execute (self, func, args, msg=None, level=1):
        """Perform some action that affects the outside world (eg.
           by writing to the filesystem).  Such actions are special because
           they should be disabled by the "dry run" flag, and should
           announce themselves if the current verbosity level is high
           enough.  This method takes care of all that bureaucracy for you;
           all you have to do is supply the funtion to call and an argument
           tuple for it (to embody the "external action" being performed),
           a message to print if the verbosity level is high enough, and an
           optional verbosity threshold."""

        # Generate a message if we weren't passed one
        if msg is None:
            msg = "%s %s" % (func.__name__, `args`)
            if msg[-2:] == ',)':        # correct for singleton tuple 
                msg = msg[0:-2] + ')'

        # Print it if verbosity level is high enough
        self.announce (msg, level)

        # And do it, as long as we're not in dry-run mode
        if not self.dry_run:
            apply (func, args)

    # execute()


    def mkpath (self, name, mode=0777):
        util.mkpath (name, mode,
                     self.verbose, self.dry_run)


    def copy_file (self, infile, outfile,
                   preserve_mode=1, preserve_times=1, level=1):
        """Copy a file respecting verbose, dry-run and force flags."""

        return util.copy_file (infile, outfile,
                               preserve_mode, preserve_times,
                               not self.force,
                               self.verbose >= level,
                               self.dry_run)


    def copy_tree (self, infile, outfile,
                   preserve_mode=1, preserve_times=1, preserve_symlinks=0,
                   level=1):
        """Copy an entire directory tree respecting verbose, dry-run,
           and force flags."""

        return util.copy_tree (infile, outfile, 
                               preserve_mode,preserve_times,preserve_symlinks,
                               not self.force,
                               self.verbose >= level,
                               self.dry_run)


    def move_file (self, src, dst, level=1):
        """Move a file respecting verbose and dry-run flags."""
        return util.move_file (src, dst,
                               self.verbose >= level,
                               self.dry_run)


    def spawn (self, cmd, search_path=1, level=1):
        from distutils.spawn import spawn
        spawn (cmd, search_path,
               self.verbose >= level,
               self.dry_run)


    def make_file (self, infiles, outfile, func, args,
                    exec_msg=None, skip_msg=None, level=1):

        """Special case of 'execute()' for operations that process one or
           more input files and generate one output file.  Works just like
           'execute()', except the operation is skipped and a different
           message printed if 'outfile' already exists and is newer than
           all files listed in 'infiles'."""


        if exec_msg is None:
            exec_msg = "generating %s from %s" % \
                       (outfile, string.join (infiles, ', '))
        if skip_msg is None:
            skip_msg = "skipping %s (inputs unchanged)" % outfile
        

        # Allow 'infiles' to be a single string
        if type (infiles) is StringType:
            infiles = (infiles,)
        elif type (infiles) not in (ListType, TupleType):
            raise TypeError, \
                  "'infiles' must be a string, or a list or tuple of strings"

        # If 'outfile' must be regenerated (either because it doesn't
        # exist, is out-of-date, or the 'force' flag is true) then
        # perform the action that presumably regenerates it
        if self.force or util.newer_group (infiles, outfile):
            self.execute (func, args, exec_msg, level)

        # Otherwise, print the "skip" message
        else:
            self.announce (skip_msg, level)

    # make_file ()

# end class Command
