"""distutils.cmd

Provides the Command class, the base class for the command classes
in the distutils.command package."""

# created 2000/04/03, Greg Ward
# (extricated from core.py; actually dates back to the beginning)

__revision__ = "$Id$"

import sys, os, string
from types import *
from distutils.errors import *
from distutils import util


class Command:
    """Abstract base class for defining command classes, the "worker bees"
       of the Distutils.  A useful analogy for command classes is to
       think of them as subroutines with local variables called
       "options".  The options are "declared" in 'initialize_options()'
       and "defined" (given their final values, aka "finalized") in
       'finalize_options()', both of which must be defined by every
       command class.  The distinction between the two is necessary
       because option values might come from the outside world (command
       line, option file, ...), and any options dependent on other
       options must be computed *after* these outside influences have
       been processed -- hence 'finalize_options()'.  The "body" of the
       subroutine, where it does all its work based on the values of its
       options, is the 'run()' method, which must also be implemented by
       every command class."""

    # -- Creation/initialization methods -------------------------------

    def __init__ (self, dist):
        """Create and initialize a new Command object.  Most importantly,
           invokes the 'initialize_options()' method, which is the
           real initializer and depends on the actual command being
           instantiated."""

        # late import because of mutual dependence between these classes
        from distutils.dist import Distribution

        if not isinstance (dist, Distribution):
            raise TypeError, "dist must be a Distribution instance"
        if self.__class__ is Command:
            raise RuntimeError, "Command is an abstract class"

        self.distribution = dist
        self.initialize_options ()

        # Per-command versions of the global flags, so that the user can
        # customize Distutils' behaviour command-by-command and let some
        # commands fallback on the Distribution's behaviour.  None means
        # "not defined, check self.distribution's copy", while 0 or 1 mean
        # false and true (duh).  Note that this means figuring out the real
        # value of each flag is a touch complicatd -- hence "self.verbose"
        # (etc.) will be handled by __getattr__, below.
        self._verbose = None
        self._dry_run = None

        # Some commands define a 'self.force' option to ignore file
        # timestamps, but methods defined *here* assume that
        # 'self.force' exists for all commands.  So define it here
        # just to be safe.
        self.force = None

        # The 'help' flag is just used for command-line parsing, so
        # none of that complicated bureaucracy is needed.
        self.help = 0

        # 'ready' records whether or not 'finalize_options()' has been
        # called.  'finalize_options()' itself should not pay attention to
        # this flag: it is the business of 'ensure_ready()', which always
        # calls 'finalize_options()', to respect/update it.
        self.ready = 0

    # __init__ ()


    def __getattr__ (self, attr):
        if attr in ('verbose', 'dry_run'):
            myval = getattr (self, "_" + attr)
            if myval is None:
                return getattr (self.distribution, attr)
            else:
                return myval
        else:
            raise AttributeError, attr


    def ensure_ready (self):
        if not self.ready:
            self.finalize_options ()
        self.ready = 1
        

    # Subclasses must define:
    #   initialize_options()
    #     provide default values for all options; may be overridden
    #     by Distutils client, by command-line options, or by options
    #     from option file
    #   finalize_options()
    #     decide on the final values for all options; this is called
    #     after all possible intervention from the outside world
    #     (command-line, option file, etc.) has been processed
    #   run()
    #     run the command: do whatever it is we're here to do,
    #     controlled by the command's various option values

    def initialize_options (self):
        """Set default values for all the options that this command
           supports.  Note that these defaults may be overridden
           by the command-line supplied by the user; thus, this is
           not the place to code dependencies between options; generally,
           'initialize_options()' implementations are just a bunch
           of "self.foo = None" assignments.
           
           This method must be implemented by all command classes."""
           
        raise RuntimeError, \
              "abstract method -- subclass %s must override" % self.__class__
        
    def finalize_options (self):
        """Set final values for all the options that this command
           supports.  This is always called as late as possible, ie.
           after any option assignments from the command-line or from
           other commands have been done.  Thus, this is the place to to
           code option dependencies: if 'foo' depends on 'bar', then it
           is safe to set 'foo' from 'bar' as long as 'foo' still has
           the same value it was assigned in 'initialize_options()'.

           This method must be implemented by all command classes."""
           
        raise RuntimeError, \
              "abstract method -- subclass %s must override" % self.__class__

    def run (self):
        """A command's raison d'etre: carry out the action it exists
           to perform, controlled by the options initialized in
           'initialize_options()', customized by the user and other
           commands, and finalized in 'finalize_options()'.  All
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


    # -- Convenience methods for commands ------------------------------

    def get_command_name (self):
        if hasattr (self, 'command_name'):
            return self.command_name
        else:
            return self.__class__.__name__


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

        src_cmd_obj = self.distribution.get_command_obj (src_cmd)
        src_cmd_obj.ensure_ready ()
        for (src_option, dst_option) in option_pairs:
            if getattr (self, dst_option) is None:
                setattr (self, dst_option,
                         getattr (src_cmd_obj, src_option))


    def find_peer (self, command, create=1):
        """Wrapper around Distribution's 'get_command_obj()' method:
           find (create if necessary and 'create' is true) the command
           object for 'command'.."""

        cmd_obj = self.distribution.get_command_obj (command, create)
        cmd_obj.ensure_ready ()
        return cmd_obj


    def get_peer_option (self, command, option):
        """Find or create the command object for 'command', and return
           its 'option' option."""

        cmd_obj = self.find_peer (command)
        return getattr(cmd_obj, option)


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
                   preserve_mode=1, preserve_times=1, link=None, level=1):
        """Copy a file respecting verbose, dry-run and force flags.  (The
        former two default to whatever is in the Distribution object, and
        the latter defaults to false for commands that don't define it.)"""

        return util.copy_file (infile, outfile,
                               preserve_mode, preserve_times,
                               not self.force,
                               link,
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


    def make_archive (self, base_name, format,
                      root_dir=None, base_dir=None):
        util.make_archive (base_name, format, root_dir, base_dir,
                           self.verbose, self.dry_run)


    def make_file (self, infiles, outfile, func, args,
                   exec_msg=None, skip_msg=None, level=1):

        """Special case of 'execute()' for operations that process one or
        more input files and generate one output file.  Works just like
        'execute()', except the operation is skipped and a different
        message printed if 'outfile' already exists and is newer than all
        files listed in 'infiles'.  If the command defined 'self.force',
        and it is true, then the command is unconditionally run -- does no
        timestamp checks."""


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

# class Command


class install_misc (Command):
    """Common base class for installing some files in a subdirectory.
    Currently used by install_data and install_scripts.
    """
    
    user_options = [('install-dir=', 'd', "directory to install the files to")]

    def initialize_options (self):
        self.install_dir = None
        self.outfiles = []

    def _install_dir_from (self, dirname):
        self.set_undefined_options('install', (dirname, 'install_dir'))

    def _copy_files (self, filelist):
        self.outfiles = []
        if not filelist:
            return
        self.mkpath(self.install_dir)
        for f in filelist:
            self.copy_file(f, self.install_dir)
            self.outfiles.append(os.path.join(self.install_dir, f))

    def get_outputs (self):
        return self.outfiles


if __name__ == "__main__":
    print "ok"
