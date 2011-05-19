"""Base class for commands."""

import os
import re
from shutil import copyfile, move, make_archive
from packaging import util
from packaging import logger
from packaging.errors import PackagingOptionError


class Command:
    """Abstract base class for defining command classes, the "worker bees"
    of the Packaging.  A useful analogy for command classes is to think of
    them as subroutines with local variables called "options".  The options
    are "declared" in 'initialize_options()' and "defined" (given their
    final values, aka "finalized") in 'finalize_options()', both of which
    must be defined by every command class.  The distinction between the
    two is necessary because option values might come from the outside
    world (command line, config file, ...), and any options dependent on
    other options must be computed *after* these outside influences have
    been processed -- hence 'finalize_options()'.  The "body" of the
    subroutine, where it does all its work based on the values of its
    options, is the 'run()' method, which must also be implemented by every
    command class.
    """

    # 'sub_commands' formalizes the notion of a "family" of commands,
    # eg. "install_dist" as the parent with sub-commands "install_lib",
    # "install_headers", etc.  The parent of a family of commands
    # defines 'sub_commands' as a class attribute; it's a list of
    #    (command_name : string, predicate : unbound_method | string | None)
    # tuples, where 'predicate' is a method of the parent command that
    # determines whether the corresponding command is applicable in the
    # current situation.  (Eg. we "install_headers" is only applicable if
    # we have any C header files to install.)  If 'predicate' is None,
    # that command is always applicable.
    #
    # 'sub_commands' is usually defined at the *end* of a class, because
    # predicates can be unbound methods, so they must already have been
    # defined.  The canonical example is the "install_dist" command.
    sub_commands = []

    # Pre and post command hooks are run just before or just after the command
    # itself. They are simple functions that receive the command instance. They
    # are specified as callable objects or dotted strings (for lazy loading).
    pre_hook = None
    post_hook = None

    # -- Creation/initialization methods -------------------------------

    def __init__(self, dist):
        """Create and initialize a new Command object.  Most importantly,
        invokes the 'initialize_options()' method, which is the real
        initializer and depends on the actual command being instantiated.
        """
        # late import because of mutual dependence between these classes
        from packaging.dist import Distribution

        if not isinstance(dist, Distribution):
            raise TypeError("dist must be a Distribution instance")
        if self.__class__ is Command:
            raise RuntimeError("Command is an abstract class")

        self.distribution = dist
        self.initialize_options()

        # Per-command versions of the global flags, so that the user can
        # customize Packaging' behaviour command-by-command and let some
        # commands fall back on the Distribution's behaviour.  None means
        # "not defined, check self.distribution's copy", while 0 or 1 mean
        # false and true (duh).  Note that this means figuring out the real
        # value of each flag is a touch complicated -- hence "self._dry_run"
        # will be handled by a property, below.
        # XXX This needs to be fixed. [I changed it to a property--does that
        #     "fix" it?]
        self._dry_run = None

        # Some commands define a 'self.force' option to ignore file
        # timestamps, but methods defined *here* assume that
        # 'self.force' exists for all commands.  So define it here
        # just to be safe.
        self.force = None

        # The 'help' flag is just used for command line parsing, so
        # none of that complicated bureaucracy is needed.
        self.help = False

        # 'finalized' records whether or not 'finalize_options()' has been
        # called.  'finalize_options()' itself should not pay attention to
        # this flag: it is the business of 'ensure_finalized()', which
        # always calls 'finalize_options()', to respect/update it.
        self.finalized = False

    # XXX A more explicit way to customize dry_run would be better.
    @property
    def dry_run(self):
        if self._dry_run is None:
            return getattr(self.distribution, 'dry_run')
        else:
            return self._dry_run

    def ensure_finalized(self):
        if not self.finalized:
            self.finalize_options()
        self.finalized = True

    # Subclasses must define:
    #   initialize_options()
    #     provide default values for all options; may be customized by
    #     setup script, by options from config file(s), or by command-line
    #     options
    #   finalize_options()
    #     decide on the final values for all options; this is called
    #     after all possible intervention from the outside world
    #     (command line, option file, etc.) has been processed
    #   run()
    #     run the command: do whatever it is we're here to do,
    #     controlled by the command's various option values

    def initialize_options(self):
        """Set default values for all the options that this command
        supports.  Note that these defaults may be overridden by other
        commands, by the setup script, by config files, or by the
        command line.  Thus, this is not the place to code dependencies
        between options; generally, 'initialize_options()' implementations
        are just a bunch of "self.foo = None" assignments.

        This method must be implemented by all command classes.
        """
        raise RuntimeError(
            "abstract method -- subclass %s must override" % self.__class__)

    def finalize_options(self):
        """Set final values for all the options that this command supports.
        This is always called as late as possible, ie.  after any option
        assignments from the command line or from other commands have been
        done.  Thus, this is the place to code option dependencies: if
        'foo' depends on 'bar', then it is safe to set 'foo' from 'bar' as
        long as 'foo' still has the same value it was assigned in
        'initialize_options()'.

        This method must be implemented by all command classes.
        """
        raise RuntimeError(
            "abstract method -- subclass %s must override" % self.__class__)

    def dump_options(self, header=None, indent=""):
        if header is None:
            header = "command options for '%s':" % self.get_command_name()
        logger.info(indent + header)
        indent = indent + "  "
        negative_opt = getattr(self, 'negative_opt', ())
        for option, _, _ in self.user_options:
            if option in negative_opt:
                continue
            option = option.replace('-', '_')
            if option[-1] == "=":
                option = option[:-1]
            value = getattr(self, option)
            logger.info(indent + "%s = %s", option, value)

    def run(self):
        """A command's raison d'etre: carry out the action it exists to
        perform, controlled by the options initialized in
        'initialize_options()', customized by other commands, the setup
        script, the command line and config files, and finalized in
        'finalize_options()'.  All terminal output and filesystem
        interaction should be done by 'run()'.

        This method must be implemented by all command classes.
        """
        raise RuntimeError(
            "abstract method -- subclass %s must override" % self.__class__)

    # -- External interface --------------------------------------------
    # (called by outsiders)

    def get_source_files(self):
        """Return the list of files that are used as inputs to this command,
        i.e. the files used to generate the output files.  The result is used
        by the `sdist` command in determining the set of default files.

        Command classes should implement this method if they operate on files
        from the source tree.
        """
        return []

    def get_outputs(self):
        """Return the list of files that would be produced if this command
        were actually run.  Not affected by the "dry-run" flag or whether
        any other commands have been run.

        Command classes should implement this method if they produce any
        output files that get consumed by another command.  e.g., `build_ext`
        returns the list of built extension modules, but not any temporary
        files used in the compilation process.
        """
        return []

    # -- Option validation methods -------------------------------------
    # (these are very handy in writing the 'finalize_options()' method)
    #
    # NB. the general philosophy here is to ensure that a particular option
    # value meets certain type and value constraints.  If not, we try to
    # force it into conformance (eg. if we expect a list but have a string,
    # split the string on comma and/or whitespace).  If we can't force the
    # option into conformance, raise PackagingOptionError.  Thus, command
    # classes need do nothing more than (eg.)
    #   self.ensure_string_list('foo')
    # and they can be guaranteed that thereafter, self.foo will be
    # a list of strings.

    def _ensure_stringlike(self, option, what, default=None):
        val = getattr(self, option)
        if val is None:
            setattr(self, option, default)
            return default
        elif not isinstance(val, str):
            raise PackagingOptionError("'%s' must be a %s (got `%s`)" %
                                       (option, what, val))
        return val

    def ensure_string(self, option, default=None):
        """Ensure that 'option' is a string; if not defined, set it to
        'default'.
        """
        self._ensure_stringlike(option, "string", default)

    def ensure_string_list(self, option):
        r"""Ensure that 'option' is a list of strings.  If 'option' is
        currently a string, we split it either on /,\s*/ or /\s+/, so
        "foo bar baz", "foo,bar,baz", and "foo,   bar baz" all become
        ["foo", "bar", "baz"].
        """
        val = getattr(self, option)
        if val is None:
            return
        elif isinstance(val, str):
            setattr(self, option, re.split(r',\s*|\s+', val))
        else:
            if isinstance(val, list):
                # checks if all elements are str
                ok = True
                for element in val:
                    if not isinstance(element, str):
                        ok = False
                        break
            else:
                ok = False

            if not ok:
                raise PackagingOptionError(
                    "'%s' must be a list of strings (got %r)" % (option, val))

    def _ensure_tested_string(self, option, tester,
                              what, error_fmt, default=None):
        val = self._ensure_stringlike(option, what, default)
        if val is not None and not tester(val):
            raise PackagingOptionError(
                ("error in '%s' option: " + error_fmt) % (option, val))

    def ensure_filename(self, option):
        """Ensure that 'option' is the name of an existing file."""
        self._ensure_tested_string(option, os.path.isfile,
                                   "filename",
                                   "'%s' does not exist or is not a file")

    def ensure_dirname(self, option):
        self._ensure_tested_string(option, os.path.isdir,
                                   "directory name",
                                   "'%s' does not exist or is not a directory")

    # -- Convenience methods for commands ------------------------------

    @classmethod
    def get_command_name(cls):
        if hasattr(cls, 'command_name'):
            return cls.command_name
        else:
            return cls.__name__

    def set_undefined_options(self, src_cmd, *options):
        """Set values of undefined options from another command.

        Undefined options are options set to None, which is the convention
        used to indicate that an option has not been changed between
        'initialize_options()' and 'finalize_options()'.  This method is
        usually called from 'finalize_options()' for options that depend on
        some other command rather than another option of the same command,
        typically subcommands.

        The 'src_cmd' argument is the other command from which option values
        will be taken (a command object will be created for it if necessary);
        the remaining positional arguments are strings that give the name of
        the option to set. If the name is different on the source and target
        command, you can pass a tuple with '(name_on_source, name_on_dest)' so
        that 'self.name_on_dest' will be set from 'src_cmd.name_on_source'.
        """
        src_cmd_obj = self.distribution.get_command_obj(src_cmd)
        src_cmd_obj.ensure_finalized()
        for obj in options:
            if isinstance(obj, tuple):
                src_option, dst_option = obj
            else:
                src_option, dst_option = obj, obj
            if getattr(self, dst_option) is None:
                setattr(self, dst_option,
                        getattr(src_cmd_obj, src_option))

    def get_finalized_command(self, command, create=True):
        """Wrapper around Distribution's 'get_command_obj()' method: find
        (create if necessary and 'create' is true) the command object for
        'command', call its 'ensure_finalized()' method, and return the
        finalized command object.
        """
        cmd_obj = self.distribution.get_command_obj(command, create)
        cmd_obj.ensure_finalized()
        return cmd_obj

    def get_reinitialized_command(self, command, reinit_subcommands=False):
        return self.distribution.get_reinitialized_command(
            command, reinit_subcommands)

    def run_command(self, command):
        """Run some other command: uses the 'run_command()' method of
        Distribution, which creates and finalizes the command object if
        necessary and then invokes its 'run()' method.
        """
        self.distribution.run_command(command)

    def get_sub_commands(self):
        """Determine the sub-commands that are relevant in the current
        distribution (ie., that need to be run).  This is based on the
        'sub_commands' class attribute: each tuple in that list may include
        a method that we call to determine if the subcommand needs to be
        run for the current distribution.  Return a list of command names.
        """
        commands = []
        for sub_command in self.sub_commands:
            if len(sub_command) == 2:
                cmd_name, method = sub_command
                if method is None or method(self):
                    commands.append(cmd_name)
            else:
                commands.append(sub_command)
        return commands

    # -- External world manipulation -----------------------------------

    def execute(self, func, args, msg=None, level=1):
        util.execute(func, args, msg, dry_run=self.dry_run)

    def mkpath(self, name, mode=0o777, dry_run=None, verbose=0):
        if dry_run is None:
            dry_run = self.dry_run
        name = os.path.normpath(name)
        if os.path.isdir(name) or name == '':
            return
        if dry_run:
            head = ''
            for part in name.split(os.sep):
                logger.info("created directory %s%s", head, part)
                head += part + os.sep
            return
        os.makedirs(name, mode)

    def copy_file(self, infile, outfile,
                  preserve_mode=True, preserve_times=True, link=None, level=1):
        """Copy a file respecting verbose, dry-run and force flags.  (The
        former two default to whatever is in the Distribution object, and
        the latter defaults to false for commands that don't define it.)"""
        if self.dry_run:
            # XXX add a comment
            return
        if os.path.isdir(outfile):
            outfile = os.path.join(outfile, os.path.split(infile)[-1])
        copyfile(infile, outfile)
        return outfile, None  # XXX

    def copy_tree(self, infile, outfile, preserve_mode=True,
                  preserve_times=True, preserve_symlinks=False, level=1):
        """Copy an entire directory tree respecting verbose, dry-run,
        and force flags.
        """
        if self.dry_run:
            return  # see if we want to display something


        return util.copy_tree(infile, outfile, preserve_mode, preserve_times,
            preserve_symlinks, not self.force, dry_run=self.dry_run)

    def move_file(self, src, dst, level=1):
        """Move a file respecting the dry-run flag."""
        if self.dry_run:
            return  # XXX log ?
        return move(src, dst)

    def spawn(self, cmd, search_path=True, level=1):
        """Spawn an external command respecting dry-run flag."""
        from packaging.util import spawn
        spawn(cmd, search_path, dry_run=self.dry_run)

    def make_archive(self, base_name, format, root_dir=None, base_dir=None,
                     owner=None, group=None):
        return make_archive(base_name, format, root_dir,
                            base_dir, dry_run=self.dry_run,
                            owner=owner, group=group)

    def make_file(self, infiles, outfile, func, args,
                  exec_msg=None, skip_msg=None, level=1):
        """Special case of 'execute()' for operations that process one or
        more input files and generate one output file.  Works just like
        'execute()', except the operation is skipped and a different
        message printed if 'outfile' already exists and is newer than all
        files listed in 'infiles'.  If the command defined 'self.force',
        and it is true, then the command is unconditionally run -- does no
        timestamp checks.
        """
        if skip_msg is None:
            skip_msg = "skipping %s (inputs unchanged)" % outfile

        # Allow 'infiles' to be a single string
        if isinstance(infiles, str):
            infiles = (infiles,)
        elif not isinstance(infiles, (list, tuple)):
            raise TypeError(
                "'infiles' must be a string, or a list or tuple of strings")

        if exec_msg is None:
            exec_msg = "generating %s from %s" % (outfile, ', '.join(infiles))

        # If 'outfile' must be regenerated (either because it doesn't
        # exist, is out-of-date, or the 'force' flag is true) then
        # perform the action that presumably regenerates it
        if self.force or util.newer_group(infiles, outfile):
            self.execute(func, args, exec_msg, level)

        # Otherwise, print the "skip" message
        else:
            logger.debug(skip_msg)
