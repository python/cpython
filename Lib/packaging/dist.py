"""Class representing the distribution being built/installed/etc."""

import os
import re

from packaging.errors import (PackagingOptionError, PackagingArgError,
                              PackagingModuleError, PackagingClassError)
from packaging.fancy_getopt import FancyGetopt
from packaging.util import strtobool, resolve_name
from packaging import logger
from packaging.metadata import Metadata
from packaging.config import Config
from packaging.command import get_command_class, STANDARD_COMMANDS

# Regex to define acceptable Packaging command names.  This is not *quite*
# the same as a Python NAME -- I don't allow leading underscores.  The fact
# that they're very similar is no coincidence; the default naming scheme is
# to look for a Python module named after the command.
command_re = re.compile(r'^[a-zA-Z]([a-zA-Z0-9_]*)$')

USAGE = """\
usage: %(script)s [global_opts] cmd1 [cmd1_opts] [cmd2 [cmd2_opts] ...]
   or: %(script)s --help [cmd1 cmd2 ...]
   or: %(script)s --help-commands
   or: %(script)s cmd --help
"""


def gen_usage(script_name):
    script = os.path.basename(script_name)
    return USAGE % {'script': script}


class Distribution:
    """The core of the Packaging.  Most of the work hiding behind 'setup'
    is really done within a Distribution instance, which farms the work out
    to the Packaging commands specified on the command line.

    Setup scripts will almost never instantiate Distribution directly,
    unless the 'setup()' function is totally inadequate to their needs.
    However, it is conceivable that a setup script might wish to subclass
    Distribution for some specialized purpose, and then pass the subclass
    to 'setup()' as the 'distclass' keyword argument.  If so, it is
    necessary to respect the expectations that 'setup' has of Distribution.
    See the code for 'setup()', in run.py, for details.
    """

    # 'global_options' describes the command-line options that may be
    # supplied to the setup script prior to any actual commands.
    # Eg. "./setup.py -n" or "./setup.py --dry-run" both take advantage of
    # these global options.  This list should be kept to a bare minimum,
    # since every global option is also valid as a command option -- and we
    # don't want to pollute the commands with too many options that they
    # have minimal control over.
    global_options = [
        ('dry-run', 'n', "don't actually do anything"),
        ('help', 'h', "show detailed help message"),
        ('no-user-cfg', None, 'ignore pydistutils.cfg in your home directory'),
    ]

    # 'common_usage' is a short (2-3 line) string describing the common
    # usage of the setup script.
    common_usage = """\
Common commands: (see '--help-commands' for more)

  setup.py build      will build the package underneath 'build/'
  setup.py install    will install the package
"""

    # options that are not propagated to the commands
    display_options = [
        ('help-commands', None,
         "list all available commands"),
        ('name', None,
         "print package name"),
        ('version', 'V',
         "print package version"),
        ('fullname', None,
         "print <package name>-<version>"),
        ('author', None,
         "print the author's name"),
        ('author-email', None,
         "print the author's email address"),
        ('maintainer', None,
         "print the maintainer's name"),
        ('maintainer-email', None,
         "print the maintainer's email address"),
        ('contact', None,
         "print the maintainer's name if known, else the author's"),
        ('contact-email', None,
         "print the maintainer's email address if known, else the author's"),
        ('url', None,
         "print the URL for this package"),
        ('license', None,
         "print the license of the package"),
        ('licence', None,
         "alias for --license"),
        ('description', None,
         "print the package description"),
        ('long-description', None,
         "print the long package description"),
        ('platforms', None,
         "print the list of platforms"),
        ('classifier', None,
         "print the list of classifiers"),
        ('keywords', None,
         "print the list of keywords"),
        ('provides', None,
         "print the list of packages/modules provided"),
        ('requires', None,
         "print the list of packages/modules required"),
        ('obsoletes', None,
         "print the list of packages/modules made obsolete"),
        ('use-2to3', None,
         "use 2to3 to make source python 3.x compatible"),
        ('convert-2to3-doctests', None,
         "use 2to3 to convert doctests in seperate text files"),
        ]
    display_option_names = [x[0].replace('-', '_') for x in display_options]

    # negative options are options that exclude other options
    negative_opt = {}

    # -- Creation/initialization methods -------------------------------
    def __init__(self, attrs=None):
        """Construct a new Distribution instance: initialize all the
        attributes of a Distribution, and then use 'attrs' (a dictionary
        mapping attribute names to values) to assign some of those
        attributes their "real" values.  (Any attributes not mentioned in
        'attrs' will be assigned to some null value: 0, None, an empty list
        or dictionary, etc.)  Most importantly, initialize the
        'command_obj' attribute to the empty dictionary; this will be
        filled in with real command objects by 'parse_command_line()'.
        """

        # Default values for our command-line options
        self.dry_run = False
        self.help = False
        for attr in self.display_option_names:
            setattr(self, attr, False)

        # Store the configuration
        self.config = Config(self)

        # Store the distribution metadata (name, version, author, and so
        # forth) in a separate object -- we're getting to have enough
        # information here (and enough command-line options) that it's
        # worth it.
        self.metadata = Metadata()

        # 'cmdclass' maps command names to class objects, so we
        # can 1) quickly figure out which class to instantiate when
        # we need to create a new command object, and 2) have a way
        # for the setup script to override command classes
        self.cmdclass = {}

        # 'script_name' and 'script_args' are usually set to sys.argv[0]
        # and sys.argv[1:], but they can be overridden when the caller is
        # not necessarily a setup script run from the command line.
        self.script_name = None
        self.script_args = None

        # 'command_options' is where we store command options between
        # parsing them (from config files, the command line, etc.) and when
        # they are actually needed -- ie. when the command in question is
        # instantiated.  It is a dictionary of dictionaries of 2-tuples:
        #   command_options = { command_name : { option : (source, value) } }
        self.command_options = {}

        # 'dist_files' is the list of (command, pyversion, file) that
        # have been created by any dist commands run so far. This is
        # filled regardless of whether the run is dry or not. pyversion
        # gives sysconfig.get_python_version() if the dist file is
        # specific to a Python version, 'any' if it is good for all
        # Python versions on the target platform, and '' for a source
        # file. pyversion should not be used to specify minimum or
        # maximum required Python versions; use the metainfo for that
        # instead.
        self.dist_files = []

        # These options are really the business of various commands, rather
        # than of the Distribution itself.  We provide aliases for them in
        # Distribution as a convenience to the developer.
        self.packages = []
        self.package_data = {}
        self.package_dir = None
        self.py_modules = []
        self.libraries = []
        self.headers = []
        self.ext_modules = []
        self.ext_package = None
        self.include_dirs = []
        self.extra_path = None
        self.scripts = []
        self.data_files = {}
        self.password = ''
        self.use_2to3 = False
        self.convert_2to3_doctests = []
        self.extra_files = []

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
        # the command is successfully run.  Thus it's probably best to use
        # '.get()' rather than a straight lookup.
        self.have_run = {}

        # Now we'll use the attrs dictionary (ultimately, keyword args from
        # the setup script) to possibly override any or all of these
        # distribution options.

        if attrs is not None:
            # Pull out the set of command options and work on them
            # specifically.  Note that this order guarantees that aliased
            # command options will override any supplied redundantly
            # through the general options dictionary.
            options = attrs.get('options')
            if options is not None:
                del attrs['options']
                for command, cmd_options in options.items():
                    opt_dict = self.get_option_dict(command)
                    for opt, val in cmd_options.items():
                        opt_dict[opt] = ("setup script", val)

            # Now work on the rest of the attributes.  Any attribute that's
            # not already defined is invalid!
            for key, val in attrs.items():
                if self.metadata.is_metadata_field(key):
                    self.metadata[key] = val
                elif hasattr(self, key):
                    setattr(self, key, val)
                else:
                    logger.warning(
                        'unknown argument given to Distribution: %r', key)

        # no-user-cfg is handled before other command line args
        # because other args override the config files, and this
        # one is needed before we can load the config files.
        # If attrs['script_args'] wasn't passed, assume false.
        #
        # This also make sure we just look at the global options
        self.want_user_cfg = True

        if self.script_args is not None:
            for arg in self.script_args:
                if not arg.startswith('-'):
                    break
                if arg == '--no-user-cfg':
                    self.want_user_cfg = False
                    break

        self.finalize_options()

    def get_option_dict(self, command):
        """Get the option dictionary for a given command.  If that
        command's option dictionary hasn't been created yet, then create it
        and return the new dictionary; otherwise, return the existing
        option dictionary.
        """
        d = self.command_options.get(command)
        if d is None:
            d = self.command_options[command] = {}
        return d

    def get_fullname(self):
        return self.metadata.get_fullname()

    def dump_option_dicts(self, header=None, commands=None, indent=""):
        from pprint import pformat

        if commands is None:             # dump all command option dicts
            commands = sorted(self.command_options)

        if header is not None:
            logger.info(indent + header)
            indent = indent + "  "

        if not commands:
            logger.info(indent + "no commands known yet")
            return

        for cmd_name in commands:
            opt_dict = self.command_options.get(cmd_name)
            if opt_dict is None:
                logger.info(indent + "no option dict for %r command",
                            cmd_name)
            else:
                logger.info(indent + "option dict for %r command:", cmd_name)
                out = pformat(opt_dict)
                for line in out.split('\n'):
                    logger.info(indent + "  " + line)

    # -- Config file finding/parsing methods ---------------------------
    # XXX to be removed
    def parse_config_files(self, filenames=None):
        return self.config.parse_config_files(filenames)

    def find_config_files(self):
        return self.config.find_config_files()

    # -- Command-line parsing methods ----------------------------------

    def parse_command_line(self):
        """Parse the setup script's command line, taken from the
        'script_args' instance attribute (which defaults to 'sys.argv[1:]'
        -- see 'setup()' in run.py).  This list is first processed for
        "global options" -- options that set attributes of the Distribution
        instance.  Then, it is alternately scanned for Packaging commands
        and options for that command.  Each new command terminates the
        options for the previous command.  The allowed options for a
        command are determined by the 'user_options' attribute of the
        command class -- thus, we have to be able to load command classes
        in order to parse the command line.  Any error in that 'options'
        attribute raises PackagingGetoptError; any error on the
        command line raises PackagingArgError.  If no Packaging commands
        were found on the command line, raises PackagingArgError.  Return
        true if command line was successfully parsed and we should carry
        on with executing commands; false if no errors but we shouldn't
        execute commands (currently, this only happens if user asks for
        help).
        """
        #
        # We now have enough information to show the Macintosh dialog
        # that allows the user to interactively specify the "command line".
        #
        toplevel_options = self._get_toplevel_options()

        # We have to parse the command line a bit at a time -- global
        # options, then the first command, then its options, and so on --
        # because each command will be handled by a different class, and
        # the options that are valid for a particular class aren't known
        # until we have loaded the command class, which doesn't happen
        # until we know what the command is.

        self.commands = []
        parser = FancyGetopt(toplevel_options + self.display_options)
        parser.set_negative_aliases(self.negative_opt)
        parser.set_aliases({'licence': 'license'})
        args = parser.getopt(args=self.script_args, object=self)
        option_order = parser.get_option_order()

        # for display options we return immediately
        if self.handle_display_options(option_order):
            return

        while args:
            args = self._parse_command_opts(parser, args)
            if args is None:            # user asked for help (and got it)
                return

        # Handle the cases of --help as a "global" option, ie.
        # "setup.py --help" and "setup.py --help command ...".  For the
        # former, we show global options (--dry-run, etc.)
        # and display-only options (--name, --version, etc.); for the
        # latter, we omit the display-only options and show help for
        # each command listed on the command line.
        if self.help:
            self._show_help(parser,
                            display_options=len(self.commands) == 0,
                            commands=self.commands)
            return

        return 1

    def _get_toplevel_options(self):
        """Return the non-display options recognized at the top level.

        This includes options that are recognized *only* at the top
        level as well as options recognized for commands.
        """
        return self.global_options

    def _parse_command_opts(self, parser, args):
        """Parse the command-line options for a single command.
        'parser' must be a FancyGetopt instance; 'args' must be the list
        of arguments, starting with the current command (whose options
        we are about to parse).  Returns a new version of 'args' with
        the next command at the front of the list; will be the empty
        list if there are no more commands on the command line.  Returns
        None if the user asked for help on this command.
        """
        # Pull the current command from the head of the command line
        command = args[0]
        if not command_re.match(command):
            raise SystemExit("invalid command name %r" % command)
        self.commands.append(command)

        # Dig up the command class that implements this command, so we
        # 1) know that it's a valid command, and 2) know which options
        # it takes.
        try:
            cmd_class = get_command_class(command)
        except PackagingModuleError as msg:
            raise PackagingArgError(msg)

        # XXX We want to push this in packaging.command
        #
        # Require that the command class be derived from Command -- want
        # to be sure that the basic "command" interface is implemented.
        for meth in ('initialize_options', 'finalize_options', 'run'):
            if hasattr(cmd_class, meth):
                continue
            raise PackagingClassError(
                'command %r must implement %r' % (cmd_class, meth))

        # Also make sure that the command object provides a list of its
        # known options.
        if not (hasattr(cmd_class, 'user_options') and
                isinstance(cmd_class.user_options, list)):
            raise PackagingClassError(
                "command class %s must provide "
                "'user_options' attribute (a list of tuples)" % cmd_class)

        # If the command class has a list of negative alias options,
        # merge it in with the global negative aliases.
        negative_opt = self.negative_opt
        if hasattr(cmd_class, 'negative_opt'):
            negative_opt = negative_opt.copy()
            negative_opt.update(cmd_class.negative_opt)

        # Check for help_options in command class.  They have a different
        # format (tuple of four) so we need to preprocess them here.
        if (hasattr(cmd_class, 'help_options') and
            isinstance(cmd_class.help_options, list)):
            help_options = cmd_class.help_options[:]
        else:
            help_options = []

        # All commands support the global options too, just by adding
        # in 'global_options'.
        parser.set_option_table(self.global_options +
                                cmd_class.user_options +
                                help_options)
        parser.set_negative_aliases(negative_opt)
        args, opts = parser.getopt(args[1:])
        if hasattr(opts, 'help') and opts.help:
            self._show_help(parser, display_options=False,
                            commands=[cmd_class])
            return

        if (hasattr(cmd_class, 'help_options') and
            isinstance(cmd_class.help_options, list)):
            help_option_found = False
            for help_option, short, desc, func in cmd_class.help_options:
                if hasattr(opts, help_option.replace('-', '_')):
                    help_option_found = True
                    if hasattr(func, '__call__'):
                        func()
                    else:
                        raise PackagingClassError(
                            "invalid help function %r for help option %r: "
                            "must be a callable object (function, etc.)"
                            % (func, help_option))

            if help_option_found:
                return

        # Put the options from the command line into their official
        # holding pen, the 'command_options' dictionary.
        opt_dict = self.get_option_dict(command)
        for name, value in vars(opts).items():
            opt_dict[name] = ("command line", value)

        return args

    def finalize_options(self):
        """Set final values for all the options on the Distribution
        instance, analogous to the .finalize_options() method of Command
        objects.
        """
        if getattr(self, 'convert_2to3_doctests', None):
            self.convert_2to3_doctests = [os.path.join(p)
                                for p in self.convert_2to3_doctests]
        else:
            self.convert_2to3_doctests = []

    def _show_help(self, parser, global_options=True, display_options=True,
                   commands=[]):
        """Show help for the setup script command line in the form of
        several lists of command-line options.  'parser' should be a
        FancyGetopt instance; do not expect it to be returned in the
        same state, as its option table will be reset to make it
        generate the correct help text.

        If 'global_options' is true, lists the global options:
        --dry-run, etc.  If 'display_options' is true, lists
        the "display-only" options: --name, --version, etc.  Finally,
        lists per-command help for every command name or command class
        in 'commands'.
        """
        # late import because of mutual dependence between these modules
        from packaging.command.cmd import Command

        if global_options:
            if display_options:
                options = self._get_toplevel_options()
            else:
                options = self.global_options
            parser.set_option_table(options)
            parser.print_help(self.common_usage + "\nGlobal options:")
            print('')

        if display_options:
            parser.set_option_table(self.display_options)
            parser.print_help(
                "Information display options (just display " +
                "information, ignore any commands)")
            print('')

        for command in self.commands:
            if isinstance(command, type) and issubclass(command, Command):
                cls = command
            else:
                cls = get_command_class(command)
            if (hasattr(cls, 'help_options') and
                isinstance(cls.help_options, list)):
                parser.set_option_table(cls.user_options + cls.help_options)
            else:
                parser.set_option_table(cls.user_options)
            parser.print_help("Options for %r command:" % cls.__name__)
            print('')

        print(gen_usage(self.script_name))

    def handle_display_options(self, option_order):
        """If there were any non-global "display-only" options
        (--help-commands or the metadata display options) on the command
        line, display the requested info and return true; else return
        false.
        """
        # User just wants a list of commands -- we'll print it out and stop
        # processing now (ie. if they ran "setup --help-commands foo bar",
        # we ignore "foo bar").
        if self.help_commands:
            self.print_commands()
            print('')
            print(gen_usage(self.script_name))
            return 1

        # If user supplied any of the "display metadata" options, then
        # display that metadata in the order in which the user supplied the
        # metadata options.
        any_display_options = False
        is_display_option = set()
        for option in self.display_options:
            is_display_option.add(option[0])

        for opt, val in option_order:
            if val and opt in is_display_option:
                opt = opt.replace('-', '_')
                value = self.metadata[opt]
                if opt in ('keywords', 'platform'):
                    print(','.join(value))
                elif opt in ('classifier', 'provides', 'requires',
                             'obsoletes'):
                    print('\n'.join(value))
                else:
                    print(value)
                any_display_options = True

        return any_display_options

    def print_command_list(self, commands, header, max_length):
        """Print a subset of the list of all commands -- used by
        'print_commands()'.
        """
        print(header + ":")

        for cmd in commands:
            cls = self.cmdclass.get(cmd) or get_command_class(cmd)
            description = getattr(cls, 'description',
                                  '(no description available)')

            print("  %-*s  %s" % (max_length, cmd, description))

    def _get_command_groups(self):
        """Helper function to retrieve all the command class names divided
        into standard commands (listed in
        packaging2.command.STANDARD_COMMANDS) and extra commands (given in
        self.cmdclass and not standard commands).
        """
        extra_commands = [cmd for cmd in self.cmdclass
                          if cmd not in STANDARD_COMMANDS]
        return STANDARD_COMMANDS, extra_commands

    def print_commands(self):
        """Print out a help message listing all available commands with a
        description of each.  The list is divided into standard commands
        (listed in packaging2.command.STANDARD_COMMANDS) and extra commands
        (given in self.cmdclass and not standard commands).  The
        descriptions come from the command class attribute
        'description'.
        """
        std_commands, extra_commands = self._get_command_groups()
        max_length = 0
        for cmd in (std_commands + extra_commands):
            if len(cmd) > max_length:
                max_length = len(cmd)

        self.print_command_list(std_commands,
                                "Standard commands",
                                max_length)
        if extra_commands:
            print()
            self.print_command_list(extra_commands,
                                    "Extra commands",
                                    max_length)

    # -- Command class/object methods ----------------------------------

    def get_command_obj(self, command, create=True):
        """Return the command object for 'command'.  Normally this object
        is cached on a previous call to 'get_command_obj()'; if no command
        object for 'command' is in the cache, then we either create and
        return it (if 'create' is true) or return None.
        """
        cmd_obj = self.command_obj.get(command)
        if not cmd_obj and create:
            logger.debug("Distribution.get_command_obj(): " \
                         "creating %r command object", command)

            cls = get_command_class(command)
            cmd_obj = self.command_obj[command] = cls(self)
            self.have_run[command] = 0

            # Set any options that were supplied in config files
            # or on the command line.  (NB. support for error
            # reporting is lame here: any errors aren't reported
            # until 'finalize_options()' is called, which means
            # we won't report the source of the error.)
            options = self.command_options.get(command)
            if options:
                self._set_command_options(cmd_obj, options)

        return cmd_obj

    def _set_command_options(self, command_obj, option_dict=None):
        """Set the options for 'command_obj' from 'option_dict'.  Basically
        this means copying elements of a dictionary ('option_dict') to
        attributes of an instance ('command').

        'command_obj' must be a Command instance.  If 'option_dict' is not
        supplied, uses the standard option dictionary for this command
        (from 'self.command_options').
        """
        command_name = command_obj.get_command_name()
        if option_dict is None:
            option_dict = self.get_option_dict(command_name)

        logger.debug("  setting options for %r command:", command_name)

        for option, (source, value) in option_dict.items():
            logger.debug("    %s = %s (from %s)", option, value, source)
            try:
                bool_opts = [x.replace('-', '_')
                             for x in command_obj.boolean_options]
            except AttributeError:
                bool_opts = []
            try:
                neg_opt = command_obj.negative_opt
            except AttributeError:
                neg_opt = {}

            try:
                is_string = isinstance(value, str)
                if option in neg_opt and is_string:
                    setattr(command_obj, neg_opt[option], not strtobool(value))
                elif option in bool_opts and is_string:
                    setattr(command_obj, option, strtobool(value))
                elif hasattr(command_obj, option):
                    setattr(command_obj, option, value)
                else:
                    raise PackagingOptionError(
                        "error in %s: command %r has no such option %r" %
                        (source, command_name, option))
            except ValueError as msg:
                raise PackagingOptionError(msg)

    def get_reinitialized_command(self, command, reinit_subcommands=False):
        """Reinitializes a command to the state it was in when first
        returned by 'get_command_obj()': ie., initialized but not yet
        finalized.  This provides the opportunity to sneak option
        values in programmatically, overriding or supplementing
        user-supplied values from the config files and command line.
        You'll have to re-finalize the command object (by calling
        'finalize_options()' or 'ensure_finalized()') before using it for
        real.

        'command' should be a command name (string) or command object.  If
        'reinit_subcommands' is true, also reinitializes the command's
        sub-commands, as declared by the 'sub_commands' class attribute (if
        it has one).  See the "install_dist" command for an example.  Only
        reinitializes the sub-commands that actually matter, ie. those
        whose test predicates return true.

        Returns the reinitialized command object.
        """
        from packaging.command.cmd import Command
        if not isinstance(command, Command):
            command_name = command
            command = self.get_command_obj(command_name)
        else:
            command_name = command.get_command_name()

        if not command.finalized:
            return command
        command.initialize_options()
        self.have_run[command_name] = 0
        command.finalized = False
        self._set_command_options(command)

        if reinit_subcommands:
            for sub in command.get_sub_commands():
                self.get_reinitialized_command(sub, reinit_subcommands)

        return command

    # -- Methods that operate on the Distribution ----------------------

    def run_commands(self):
        """Run each command that was seen on the setup script command line.
        Uses the list of commands found and cache of command objects
        created by 'get_command_obj()'.
        """
        for cmd in self.commands:
            self.run_command(cmd)

    # -- Methods that operate on its Commands --------------------------

    def run_command(self, command, options=None):
        """Do whatever it takes to run a command (including nothing at all,
        if the command has already been run).  Specifically: if we have
        already created and run the command named by 'command', return
        silently without doing anything.  If the command named by 'command'
        doesn't even have a command object yet, create one.  Then invoke
        'run()' on that command object (or an existing one).
        """
        # Already been here, done that? then return silently.
        if self.have_run.get(command):
            return

        if options is not None:
            self.command_options[command] = options

        cmd_obj = self.get_command_obj(command)
        cmd_obj.ensure_finalized()
        self.run_command_hooks(cmd_obj, 'pre_hook')
        logger.info("running %s", command)
        cmd_obj.run()
        self.run_command_hooks(cmd_obj, 'post_hook')
        self.have_run[command] = 1

    def run_command_hooks(self, cmd_obj, hook_kind):
        """Run hooks registered for that command and phase.

        *cmd_obj* is a finalized command object; *hook_kind* is either
        'pre_hook' or 'post_hook'.
        """
        if hook_kind not in ('pre_hook', 'post_hook'):
            raise ValueError('invalid hook kind: %r' % hook_kind)

        hooks = getattr(cmd_obj, hook_kind, None)

        if hooks is None:
            return

        for hook in hooks.values():
            if isinstance(hook, str):
                try:
                    hook_obj = resolve_name(hook)
                except ImportError as e:
                    raise PackagingModuleError(e)
            else:
                hook_obj = hook

            if not hasattr(hook_obj, '__call__'):
                raise PackagingOptionError('hook %r is not callable' % hook)

            logger.info('running %s %s for command %s',
                        hook_kind, hook, cmd_obj.get_command_name())
            hook_obj(cmd_obj)

    # -- Distribution query methods ------------------------------------
    def has_pure_modules(self):
        return len(self.packages or self.py_modules or []) > 0

    def has_ext_modules(self):
        return self.ext_modules and len(self.ext_modules) > 0

    def has_c_libraries(self):
        return self.libraries and len(self.libraries) > 0

    def has_modules(self):
        return self.has_pure_modules() or self.has_ext_modules()

    def has_headers(self):
        return self.headers and len(self.headers) > 0

    def has_scripts(self):
        return self.scripts and len(self.scripts) > 0

    def has_data_files(self):
        return self.data_files and len(self.data_files) > 0

    def is_pure(self):
        return (self.has_pure_modules() and
                not self.has_ext_modules() and
                not self.has_c_libraries())
