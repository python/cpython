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
from distutils.errors import *
from distutils.fancy_getopt import fancy_getopt
from distutils import util

# This is not *quite* the same as a Python NAME; I don't allow leading
# underscores.  The fact that they're very similar is no coincidence...
command_re = re.compile (r'^[a-zA-Z]([a-zA-Z0-9_]*)$')

# Defining this as a global is probably inadequate -- what about
# listing the available options (or even commands, which can vary
# quite late as well)
usage = '%s [global_opts] cmd1 [cmd1_opts] [cmd2 [cmd2_opts] ...]' % sys.argv[0]



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

    # Get it to parse the command line; any command-line errors are
    # the end-users fault, so turn them into SystemExit to suppress
    # tracebacks.
    try:
        dist.parse_command_line (sys.argv[1:])
    except DistutilsArgError, msg:
        raise SystemExit, msg

    # And finally, run all the commands found on the command line.
    dist.run_commands ()

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


    # 'global_options' describes the command-line options that may
    # be supplied to the client (setup.py) prior to any actual
    # commands.  Eg. "./setup.py -nv" or "./setup.py --verbose"
    # both take advantage of these global options.
    global_options = [('verbose', 'v', "run verbosely"),
                      ('dry-run', 'n', "don't actually do anything"),
                     ]


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
        self.verbose = 0
        self.dry_run = 0

        # And for all other attributes (stuff that might be passed in
        # from setup.py, rather than from the end-user)
        self.name = None
        self.version = None
        self.author = None
        self.licence = None
        self.description = None

        # 'cmdclass' maps command names to class objects, so we
        # can 1) quickly figure out which class to instantiate when
        # we need to create a new command object, and 2) have a way
        # for the client to override command classes
        self.cmdclass = {}

        # The rest of these are really the business of various commands,
        # rather than of the Distribution itself.  However, they have
        # to be here as a conduit to the relevant command class.        
        self.py_modules = None
        self.ext_modules = None
        self.package = None

        # Now we'll use the attrs dictionary to possibly override
        # any or all of these distribution options
        if attrs:
            for k in attrs.keys():
                setattr (self, k, attrs[k])

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

    # __init__ ()


    def parse_command_line (self, args):
        """Parse the client's command line: set any Distribution
           attributes tied to command-line options, create all command
           objects, and set their options from the command-line.  'args'
           must be a list of command-line arguments, most likely
           'sys.argv[1:]' (see the 'setup()' function).  This list is
           first processed for "global options" -- options that set
           attributes of the Distribution instance.  Then, it is
           alternately scanned for Distutils command and options for
           that command.  Each new command terminates the options for
           the previous command.  The allowed options for a command are
           determined by the 'options' attribute of the command object
           -- thus, we instantiate (and cache) every command object
           here, in order to access its 'options' attribute.  Any error
           in that 'options' attribute raises DistutilsGetoptError; any
           error on the command-line raises DistutilsArgError.  If no
           Distutils commands were found on the command line, raises
           DistutilsArgError."""

        # We have to parse the command line a bit at a time -- global
        # options, then the first command, then its options, and so on --
        # because each command will be handled by a different class, and
        # the options that are valid for a particular class aren't
        # known until we instantiate the command class, which doesn't
        # happen until we know what the command is.

        self.commands = []
        args = fancy_getopt (self.global_options, self, sys.argv[1:])

        while args:
            # Pull the current command from the head of the command line
            command = args[0]
            if not command_re.match (command):
                raise SystemExit, "invalid command name '%s'" % command
            self.commands.append (command)

            # Have to instantiate the command class now, so we have a
            # way to get its valid options and somewhere to put the
            # results of parsing its share of the command-line
            cmd_obj = self.create_command_obj (command)

            # Require that the command class be derived from Command --
            # that way, we can be sure that we at least have the 'run'
            # and 'get_option' methods.
            if not isinstance (cmd_obj, Command):
                raise DistutilsClassError, \
                      "command class %s must subclass Command" % \
                      cmd_obj.__class__

            # XXX this assumes that cmd_obj provides an 'options'
            # attribute, but we're not enforcing that anywhere!
            args = fancy_getopt (cmd_obj.options, cmd_obj, args[1:])
            self.command_obj[command] = cmd_obj
            self.have_run[command] = 0

        # while args

        # Oops, no commands found -- an end-user error
        if not self.commands:
            sys.stderr.write (usage + "\n")
            raise DistutilsArgError, "no commands supplied"

    # parse_command_line()


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
                  "invalid command '%s' (no module named %s)" % \
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
           it; otherwise, return None."""

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
        cmd_obj.run ()
        self.have_run[command] = 1


    def get_command_option (self, command, option):
        """Create a command object for 'command' if necessary, finalize
           its option values by invoking its 'set_final_options()'
           method, and return the value of its 'option' option.  Raise
           DistutilsOptionError if 'option' is not known for
           that 'command'."""

        cmd_obj = self.find_command_obj (command)
        cmd_obj.set_final_options ()
        return cmd_obj.get_option (option)
        try:
            return getattr (cmd_obj, option)
        except AttributeError:
            raise DistutilsOptionError, \
                  "command %s: no such option %s" % (command, option)


    def get_command_options (self, command, *options):
        """Create a command object for 'command' if necessary, finalize
           its option values by invoking its 'set_final_options()'
           method, and return the values of all the options listed in
           'options' for that command.  Raise DistutilsOptionError if
           'option' is not known for that 'command'."""

        cmd_obj = self.find_command_obj (command)
        cmd_obj.set_final_options ()
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
       "options".  The options are "declared" in 'set_initial_options()'
       and "initialized" (given their real values) in
       'set_final_options()', both of which must be defined by every
       command class.  The distinction between the two is necessary
       because option values might come from the outside world (command
       line, option file, ...), and any options dependent on other
       options must be computed *after* these outside influences have
       been processed -- hence 'set_final_values()'.  The "body" of the
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

    # end __init__ ()

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
        if self.distribution.verbose >= level:
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
                  (self.command_name(), option)


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
                  (self.command_name(), name)
            
        return tuple (values)
    

    def set_option (self, option, value):
        """Set the value of a single option for this command.  Raise
           DistutilsOptionError if 'option' is not known."""

        if not hasattr (self, option):
            raise DistutilsOptionError, \
                  "command %s: no such option %s" % \
                  (self.command_name(), option)
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
           value in another command.  Creates a command object for
           'command' if necessary, sets 'option' to 'value', and invokes
           'set_final_options()' on that command object.  This will only
           have the desired effect if the command object for 'command'
           has not previously been created.  Generally this is used to
           ensure that the options in 'command' dependent on 'option'
           are computed, hopefully (but not necessarily) deriving from
           'value'.  It might be more accurate to call this method
           'influence_dependent_peer_options()'."""        

        cmd_obj = self.distribution.find_command_obj (command)
        cmd_obj.set_option (option, value)
        cmd_obj.set_final_options ()


    def run_peer (self, command):
        """Run some other command: uses the 'run_command()' method of
           Distribution, which creates the command object if necessary
           and then invokes its 'run()' method."""

        self.distribution.run_command (command)


    # -- External world manipulation -----------------------------------

    def execute (self, func, args, msg=None, level=1):
        """Perform some action that affects the outside world (eg.
           by writing to the filesystem).  Such actions are special because
           they should be disabled by the "dry run" flag (carried around by
           the Command's Distribution), and should announce themselves if
           the current verbosity level is high enough.  This method takes
           care of all that bureaucracy for you; all you have to do is
           supply the funtion to call and an argument tuple for it (to
           embody the "external action" being performed), a message to
           print if the verbosity level is high enough, and an optional
           verbosity threshold."""


        # Generate a message if we weren't passed one
        if msg is None:
            msg = "%s %s" % (func.__name__, `args`)
            if msg[-2:] == ',)':        # correct for singleton tuple 
                msg = msg[0:-2] + ')'

        # Print it if verbosity level is high enough
        self.announce (msg, level)

        # And do it, as long as we're not in dry-run mode
        if not self.distribution.dry_run:
            apply (func, args)

    # execute()


    def mkpath (self, name, mode=0777):
        util.mkpath (name, mode,
                     self.distribution.verbose, self.distribution.dry_run)


    def copy_file (self, infile, outfile,
                   preserve_mode=1, preserve_times=1, update=1, level=1):
        """Copy a file respecting verbose and dry-run flags."""

        return util.copy_file (infile, outfile,
                               preserve_mode, preserve_times,
                               update, self.distribution.verbose >= level,
                               self.distribution.dry_run)


    def copy_tree (self, infile, outfile,
                   preserve_mode=1, preserve_times=1, preserve_symlinks=0,
                   update=1, level=1):
        """Copy an entire directory tree respecting verbose and dry-run
           flags."""

        return util.copy_tree (infile, outfile, 
                               preserve_mode,preserve_times,preserve_symlinks,
                               update, self.distribution.verbose >= level,
                               self.distribution.dry_run)


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

        # XXX this stuff should probably be moved off to a function
        # in 'distutils.util'
        from stat import *

        if os.path.exists (outfile):
            out_mtime = os.stat (outfile)[ST_MTIME]

            # Loop over all infiles.  If any infile is newer than outfile,
            # then we'll have to regenerate outfile
            for f in infiles:
                in_mtime = os.stat (f)[ST_MTIME]
                if in_mtime > out_mtime:
                    runit = 1
                    break
            else:
                runit = 0

        else:
            runit = 1

        # If we determined that 'outfile' must be regenerated, then
        # perform the action that presumably regenerates it
        if runit:
            self.execute (func, args, exec_msg, level)

        # Otherwise, print the "skip" message
        else:
            self.announce (skip_msg, level)

    # make_file ()


#     def make_files (self, infiles, outfiles, func, args,
#                     exec_msg=None, skip_msg=None, level=1):

#         """Special case of 'execute()' for operations that process one or
#            more input files and generate one or more output files.  Works
#            just like 'execute()', except the operation is skipped and a
#            different message printed if all files listed in 'outfiles'
#            already exist and are newer than all files listed in
#            'infiles'."""

#         pass
    
    

# end class Command
