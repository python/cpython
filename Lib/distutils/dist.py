"""distutils.dist

Provides the Distribution class, which represents the module distribution
being built/installed/distributed.
"""

# created 2000/04/03, Greg Ward
# (extricated from core.py; actually dates back to the beginning)

__revision__ = "$Id$"

import sys, os, string, re
from types import *
from copy import copy
from distutils.errors import *
from distutils import sysconfig
from distutils.fancy_getopt import FancyGetopt, translate_longopt
from distutils.util import check_environ, strtobool, rfc822_escape


# Regex to define acceptable Distutils command names.  This is not *quite*
# the same as a Python NAME -- I don't allow leading underscores.  The fact
# that they're very similar is no coincidence; the default naming scheme is
# to look for a Python module named after the command.
command_re = re.compile (r'^[a-zA-Z]([a-zA-Z0-9_]*)$')


class Distribution:
    """The core of the Distutils.  Most of the work hiding behind 'setup'
    is really done within a Distribution instance, which farms the work out
    to the Distutils commands specified on the command line.

    Setup scripts will almost never instantiate Distribution directly,
    unless the 'setup()' function is totally inadequate to their needs.
    However, it is conceivable that a setup script might wish to subclass
    Distribution for some specialized purpose, and then pass the subclass
    to 'setup()' as the 'distclass' keyword argument.  If so, it is
    necessary to respect the expectations that 'setup' has of Distribution.
    See the code for 'setup()', in core.py, for details.
    """


    # 'global_options' describes the command-line options that may be
    # supplied to the setup script prior to any actual commands.
    # Eg. "./setup.py -n" or "./setup.py --quiet" both take advantage of
    # these global options.  This list should be kept to a bare minimum,
    # since every global option is also valid as a command option -- and we
    # don't want to pollute the commands with too many options that they
    # have minimal control over.
    global_options = [('verbose', 'v', "run verbosely (default)"),
                      ('quiet', 'q', "run quietly (turns verbosity off)"),
                      ('dry-run', 'n', "don't actually do anything"),
                      ('help', 'h', "show detailed help message"),
                     ]

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
        ('licence', None,
         "print the licence of the package"),
        ('license', None,
         "alias for --licence"),
        ('description', None,
         "print the package description"),
        ('long-description', None,
         "print the long package description"),
        ('platforms', None,
         "print the list of platforms"),
        ('keywords', None,
         "print the list of keywords"),
        ]
    display_option_names = map(lambda x: translate_longopt(x[0]),
                               display_options)

    # negative options are options that exclude other options
    negative_opt = {'quiet': 'verbose'}


    # -- Creation/initialization methods -------------------------------
    
    def __init__ (self, attrs=None):
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
        self.verbose = 1
        self.dry_run = 0
        self.help = 0
        for attr in self.display_option_names:
            setattr(self, attr, 0)

        # Store the distribution meta-data (name, version, author, and so
        # forth) in a separate object -- we're getting to have enough
        # information here (and enough command-line options) that it's
        # worth it.  Also delegate 'get_XXX()' methods to the 'metadata'
        # object in a sneaky and underhanded (but efficient!) way.
        self.metadata = DistributionMetadata()
        method_basenames = dir(self.metadata) + \
                           ['fullname', 'contact', 'contact_email']
        for basename in method_basenames:
            method_name = "get_" + basename
            setattr(self, method_name, getattr(self.metadata, method_name))

        # 'cmdclass' maps command names to class objects, so we
        # can 1) quickly figure out which class to instantiate when
        # we need to create a new command object, and 2) have a way
        # for the setup script to override command classes
        self.cmdclass = {}

        # 'script_name' and 'script_args' are usually set to sys.argv[0]
        # and sys.argv[1:], but they can be overridden when the caller is
        # not necessarily a setup script run from the command-line.
        self.script_name = None
        self.script_args = None

        # 'command_options' is where we store command options between
        # parsing them (from config files, the command-line, etc.) and when
        # they are actually needed -- ie. when the command in question is
        # instantiated.  It is a dictionary of dictionaries of 2-tuples:
        #   command_options = { command_name : { option : (source, value) } }
        self.command_options = {}

        # These options are really the business of various commands, rather
        # than of the Distribution itself.  We provide aliases for them in
        # Distribution as a convenience to the developer.
        self.packages = None
        self.package_dir = None
        self.py_modules = None
        self.libraries = None
        self.headers = None
        self.ext_modules = None
        self.ext_package = None
        self.include_dirs = None
        self.extra_path = None
        self.scripts = None
        self.data_files = None

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

        if attrs:

            # Pull out the set of command options and work on them
            # specifically.  Note that this order guarantees that aliased
            # command options will override any supplied redundantly
            # through the general options dictionary.
            options = attrs.get('options')
            if options:
                del attrs['options']
                for (command, cmd_options) in options.items():
                    opt_dict = self.get_option_dict(command)
                    for (opt, val) in cmd_options.items():
                        opt_dict[opt] = ("setup script", val)

            # Now work on the rest of the attributes.  Any attribute that's
            # not already defined is invalid!
            for (key,val) in attrs.items():
                if hasattr(self.metadata, key):
                    setattr(self.metadata, key, val)
                elif hasattr(self, key):
                    setattr(self, key, val)
                else:
                    raise DistutilsSetupError, \
                          "invalid distribution option '%s'" % key

        self.finalize_options()
        
    # __init__ ()


    def get_option_dict (self, command):
        """Get the option dictionary for a given command.  If that
        command's option dictionary hasn't been created yet, then create it
        and return the new dictionary; otherwise, return the existing
        option dictionary.
        """

        dict = self.command_options.get(command)
        if dict is None:
            dict = self.command_options[command] = {}
        return dict


    def dump_option_dicts (self, header=None, commands=None, indent=""):
        from pprint import pformat

        if commands is None:             # dump all command option dicts
            commands = self.command_options.keys()
            commands.sort()

        if header is not None:
            print indent + header
            indent = indent + "  "

        if not commands:
            print indent + "no commands known yet"
            return

        for cmd_name in commands:
            opt_dict = self.command_options.get(cmd_name)
            if opt_dict is None:
                print indent + "no option dict for '%s' command" % cmd_name
            else:
                print indent + "option dict for '%s' command:" % cmd_name
                out = pformat(opt_dict)
                for line in string.split(out, "\n"):
                    print indent + "  " + line

    # dump_option_dicts ()
            


    # -- Config file finding/parsing methods ---------------------------

    def find_config_files (self):
        """Find as many configuration files as should be processed for this
        platform, and return a list of filenames in the order in which they
        should be parsed.  The filenames returned are guaranteed to exist
        (modulo nasty race conditions).

        On Unix, there are three possible config files: pydistutils.cfg in
        the Distutils installation directory (ie. where the top-level
        Distutils __inst__.py file lives), .pydistutils.cfg in the user's
        home directory, and setup.cfg in the current directory.

        On Windows and Mac OS, there are two possible config files:
        pydistutils.cfg in the Python installation directory (sys.prefix)
        and setup.cfg in the current directory.
        """
        files = []
        check_environ()

        # Where to look for the system-wide Distutils config file
        sys_dir = os.path.dirname(sys.modules['distutils'].__file__)

        # Look for the system config file
        sys_file = os.path.join(sys_dir, "distutils.cfg")
        if os.path.isfile(sys_file):
            files.append(sys_file)

        # What to call the per-user config file
        if os.name == 'posix':
            user_filename = ".pydistutils.cfg"
        else:
            user_filename = "pydistutils.cfg"

        # And look for the user config file
        if os.environ.has_key('HOME'):
            user_file = os.path.join(os.environ.get('HOME'), user_filename)
            if os.path.isfile(user_file):
                files.append(user_file)

        # All platforms support local setup.cfg
        local_file = "setup.cfg"
        if os.path.isfile(local_file):
            files.append(local_file)

        return files

    # find_config_files ()


    def parse_config_files (self, filenames=None):

        from ConfigParser import ConfigParser
        from distutils.core import DEBUG

        if filenames is None:
            filenames = self.find_config_files()

        if DEBUG: print "Distribution.parse_config_files():"

        parser = ConfigParser()
        for filename in filenames:
            if DEBUG: print "  reading", filename
            parser.read(filename)
            for section in parser.sections():
                options = parser.options(section)
                opt_dict = self.get_option_dict(section)

                for opt in options:
                    if opt != '__name__':
                        val = parser.get(section,opt)
                        opt = string.replace(opt, '-', '_')
                        opt_dict[opt] = (filename, val)

            # Make the ConfigParser forget everything (so we retain
            # the original filenames that options come from) -- gag,
            # retch, puke -- another good reason for a distutils-
            # specific config parser (sigh...)
            parser.__init__()

        # If there was a "global" section in the config file, use it
        # to set Distribution options.

        if self.command_options.has_key('global'):
            for (opt, (src, val)) in self.command_options['global'].items():
                alias = self.negative_opt.get(opt)
                try:
                    if alias:
                        setattr(self, alias, not strtobool(val))
                    elif opt in ('verbose', 'dry_run'): # ugh!
                        setattr(self, opt, strtobool(val))
                except ValueError, msg:
                    raise DistutilsOptionError, msg

    # parse_config_files ()


    # -- Command-line parsing methods ----------------------------------

    def parse_command_line (self):
        """Parse the setup script's command line, taken from the
        'script_args' instance attribute (which defaults to 'sys.argv[1:]'
        -- see 'setup()' in core.py).  This list is first processed for
        "global options" -- options that set attributes of the Distribution
        instance.  Then, it is alternately scanned for Distutils commands
        and options for that command.  Each new command terminates the
        options for the previous command.  The allowed options for a
        command are determined by the 'user_options' attribute of the
        command class -- thus, we have to be able to load command classes
        in order to parse the command line.  Any error in that 'options'
        attribute raises DistutilsGetoptError; any error on the
        command-line raises DistutilsArgError.  If no Distutils commands
        were found on the command line, raises DistutilsArgError.  Return
        true if command-line was successfully parsed and we should carry
        on with executing commands; false if no errors but we shouldn't
        execute commands (currently, this only happens if user asks for
        help).
        """
        #
        # We now have enough information to show the Macintosh dialog that allows
        # the user to interactively specify the "command line".
        #
        if sys.platform == 'mac':
            import EasyDialogs
            cmdlist = self.get_command_list()
            self.script_args = EasyDialogs.GetArgv(
                self.global_options + self.display_options, cmdlist)
 
        # We have to parse the command line a bit at a time -- global
        # options, then the first command, then its options, and so on --
        # because each command will be handled by a different class, and
        # the options that are valid for a particular class aren't known
        # until we have loaded the command class, which doesn't happen
        # until we know what the command is.

        self.commands = []
        parser = FancyGetopt(self.global_options + self.display_options)
        parser.set_negative_aliases(self.negative_opt)
        parser.set_aliases({'license': 'licence'})
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
        # former, we show global options (--verbose, --dry-run, etc.)
        # and display-only options (--name, --version, etc.); for the
        # latter, we omit the display-only options and show help for
        # each command listed on the command line.
        if self.help:
            self._show_help(parser,
                            display_options=len(self.commands) == 0,
                            commands=self.commands)
            return

        # Oops, no commands found -- an end-user error
        if not self.commands:
            raise DistutilsArgError, "no commands supplied"

        # All is well: return true
        return 1

    # parse_command_line()

    def _parse_command_opts (self, parser, args):
        """Parse the command-line options for a single command.
        'parser' must be a FancyGetopt instance; 'args' must be the list
        of arguments, starting with the current command (whose options
        we are about to parse).  Returns a new version of 'args' with
        the next command at the front of the list; will be the empty
        list if there are no more commands on the command line.  Returns
        None if the user asked for help on this command.
        """
        # late import because of mutual dependence between these modules
        from distutils.cmd import Command

        # Pull the current command from the head of the command line
        command = args[0]
        if not command_re.match(command):
            raise SystemExit, "invalid command name '%s'" % command
        self.commands.append(command)

        # Dig up the command class that implements this command, so we
        # 1) know that it's a valid command, and 2) know which options
        # it takes.
        try:
            cmd_class = self.get_command_class(command)
        except DistutilsModuleError, msg:
            raise DistutilsArgError, msg

        # Require that the command class be derived from Command -- want
        # to be sure that the basic "command" interface is implemented.
        if not issubclass(cmd_class, Command):
            raise DistutilsClassError, \
                  "command class %s must subclass Command" % cmd_class

        # Also make sure that the command object provides a list of its
        # known options.
        if not (hasattr(cmd_class, 'user_options') and
                type(cmd_class.user_options) is ListType):
            raise DistutilsClassError, \
                  ("command class %s must provide " +
                   "'user_options' attribute (a list of tuples)") % \
                  cmd_class

        # If the command class has a list of negative alias options,
        # merge it in with the global negative aliases.
        negative_opt = self.negative_opt
        if hasattr(cmd_class, 'negative_opt'):
            negative_opt = copy(negative_opt)
            negative_opt.update(cmd_class.negative_opt)

        # Check for help_options in command class.  They have a different
        # format (tuple of four) so we need to preprocess them here.
        if (hasattr(cmd_class, 'help_options') and
            type(cmd_class.help_options) is ListType):
            help_options = fix_help_options(cmd_class.help_options)
        else:
            help_options = []


        # All commands support the global options too, just by adding
        # in 'global_options'.
        parser.set_option_table(self.global_options +
                                cmd_class.user_options +
                                help_options)
        parser.set_negative_aliases(negative_opt)
        (args, opts) = parser.getopt(args[1:])
        if hasattr(opts, 'help') and opts.help:
            self._show_help(parser, display_options=0, commands=[cmd_class])
            return

        if (hasattr(cmd_class, 'help_options') and
            type(cmd_class.help_options) is ListType):
            help_option_found=0
            for (help_option, short, desc, func) in cmd_class.help_options:
                if hasattr(opts, parser.get_attr_name(help_option)):
                    help_option_found=1
                    #print "showing help for option %s of command %s" % \
                    #      (help_option[0],cmd_class)

                    if callable(func):
                        func()
                    else:
                        raise DistutilsClassError, \
                            ("invalid help function %s for help option '%s': "
                             "must be a callable object (function, etc.)") % \
                            (`func`, help_option)

            if help_option_found: 
                return

        # Put the options from the command-line into their official
        # holding pen, the 'command_options' dictionary.
        opt_dict = self.get_option_dict(command)
        for (name, value) in vars(opts).items():
            opt_dict[name] = ("command line", value)

        return args

    # _parse_command_opts ()


    def finalize_options (self):
        """Set final values for all the options on the Distribution
        instance, analogous to the .finalize_options() method of Command
        objects.
        """

        if self.metadata.version is None:
            raise DistutilsSetupError, \
                  "No version number specified for distribution"

        keywords = self.metadata.keywords
        if keywords is not None:
            if type(keywords) is StringType:
                keywordlist = string.split(keywords, ',')
                self.metadata.keywords = map(string.strip, keywordlist)

        platforms = self.metadata.platforms
        if platforms is not None:
            if type(platforms) is StringType:
                platformlist = string.split(platforms, ',')
                self.metadata.platforms = map(string.strip, platformlist)

    def _show_help (self,
                    parser,
                    global_options=1,
                    display_options=1,
                    commands=[]):
        """Show help for the setup script command-line in the form of
        several lists of command-line options.  'parser' should be a
        FancyGetopt instance; do not expect it to be returned in the
        same state, as its option table will be reset to make it
        generate the correct help text.

        If 'global_options' is true, lists the global options:
        --verbose, --dry-run, etc.  If 'display_options' is true, lists
        the "display-only" options: --name, --version, etc.  Finally,
        lists per-command help for every command name or command class
        in 'commands'.
        """
        # late import because of mutual dependence between these modules
        from distutils.core import gen_usage
        from distutils.cmd import Command

        if global_options:
            parser.set_option_table(self.global_options)
            parser.print_help("Global options:")
            print

        if display_options:
            parser.set_option_table(self.display_options)
            parser.print_help(
                "Information display options (just display " +
                "information, ignore any commands)")
            print

        for command in self.commands:
            if type(command) is ClassType and issubclass(klass, Command):
                klass = command
            else:
                klass = self.get_command_class(command)
            if (hasattr(klass, 'help_options') and
                type(klass.help_options) is ListType):
                parser.set_option_table(klass.user_options +
                                        fix_help_options(klass.help_options))
            else:
                parser.set_option_table(klass.user_options)
            parser.print_help("Options for '%s' command:" % klass.__name__)
            print

        print gen_usage(self.script_name)
        return

    # _show_help ()


    def handle_display_options (self, option_order):
        """If there were any non-global "display-only" options
        (--help-commands or the metadata display options) on the command
        line, display the requested info and return true; else return
        false.
        """
        from distutils.core import gen_usage

        # User just wants a list of commands -- we'll print it out and stop
        # processing now (ie. if they ran "setup --help-commands foo bar",
        # we ignore "foo bar").
        if self.help_commands:
            self.print_commands()
            print
            print gen_usage(self.script_name)
            return 1

        # If user supplied any of the "display metadata" options, then
        # display that metadata in the order in which the user supplied the
        # metadata options.
        any_display_options = 0
        is_display_option = {}
        for option in self.display_options:
            is_display_option[option[0]] = 1

        for (opt, val) in option_order:
            if val and is_display_option.get(opt):
                opt = translate_longopt(opt)
                value = getattr(self.metadata, "get_"+opt)()
                if opt in ['keywords', 'platforms']:
                    print string.join(value, ',')
                else:
                    print value
                any_display_options = 1

        return any_display_options

    # handle_display_options()

    def print_command_list (self, commands, header, max_length):
        """Print a subset of the list of all commands -- used by
        'print_commands()'.
        """

        print header + ":"

        for cmd in commands:
            klass = self.cmdclass.get(cmd)
            if not klass:
                klass = self.get_command_class(cmd)
            try:
                description = klass.description
            except AttributeError:
                description = "(no description available)"

            print "  %-*s  %s" % (max_length, cmd, description)

    # print_command_list ()


    def print_commands (self):
        """Print out a help message listing all available commands with a
        description of each.  The list is divided into "standard commands"
        (listed in distutils.command.__all__) and "extra commands"
        (mentioned in self.cmdclass, but not a standard command).  The
        descriptions come from the command class attribute
        'description'.
        """

        import distutils.command
        std_commands = distutils.command.__all__
        is_std = {}
        for cmd in std_commands:
            is_std[cmd] = 1

        extra_commands = []
        for cmd in self.cmdclass.keys():
            if not is_std.get(cmd):
                extra_commands.append(cmd)

        max_length = 0
        for cmd in (std_commands + extra_commands):
            if len(cmd) > max_length:
                max_length = len(cmd)

        self.print_command_list(std_commands,
                                "Standard commands",
                                max_length)
        if extra_commands:
            print
            self.print_command_list(extra_commands,
                                    "Extra commands",
                                    max_length)

    # print_commands ()

    def get_command_list (self):
        """Get a list of (command, description) tuples.
        The list is divided into "standard commands" (listed in
        distutils.command.__all__) and "extra commands" (mentioned in
        self.cmdclass, but not a standard command).  The descriptions come
        from the command class attribute 'description'.
        """
        # Currently this is only used on Mac OS, for the Mac-only GUI
        # Distutils interface (by Jack Jansen)

        import distutils.command
        std_commands = distutils.command.__all__
        is_std = {}
        for cmd in std_commands:
            is_std[cmd] = 1

        extra_commands = []
        for cmd in self.cmdclass.keys():
            if not is_std.get(cmd):
                extra_commands.append(cmd)

        rv = []
        for cmd in (std_commands + extra_commands):
            klass = self.cmdclass.get(cmd)
            if not klass:
                klass = self.get_command_class(cmd)
            try:
                description = klass.description
            except AttributeError:
                description = "(no description available)"
            rv.append((cmd, description))
        return rv

    # -- Command class/object methods ----------------------------------

    def get_command_class (self, command):
        """Return the class that implements the Distutils command named by
        'command'.  First we check the 'cmdclass' dictionary; if the
        command is mentioned there, we fetch the class object from the
        dictionary and return it.  Otherwise we load the command module
        ("distutils.command." + command) and fetch the command class from
        the module.  The loaded class is also stored in 'cmdclass'
        to speed future calls to 'get_command_class()'.

        Raises DistutilsModuleError if the expected module could not be
        found, or if that module does not define the expected class.
        """
        klass = self.cmdclass.get(command)
        if klass:
            return klass

        module_name = 'distutils.command.' + command
        klass_name = command

        try:
            __import__ (module_name)
            module = sys.modules[module_name]
        except ImportError:
            raise DistutilsModuleError, \
                  "invalid command '%s' (no module named '%s')" % \
                  (command, module_name)

        try:
            klass = getattr(module, klass_name)
        except AttributeError:
            raise DistutilsModuleError, \
                  "invalid command '%s' (no class '%s' in module '%s')" \
                  % (command, klass_name, module_name)

        self.cmdclass[command] = klass
        return klass

    # get_command_class ()

    def get_command_obj (self, command, create=1):
        """Return the command object for 'command'.  Normally this object
        is cached on a previous call to 'get_command_obj()'; if no command
        object for 'command' is in the cache, then we either create and
        return it (if 'create' is true) or return None.
        """
        from distutils.core import DEBUG
        cmd_obj = self.command_obj.get(command)
        if not cmd_obj and create:
            if DEBUG:
                print "Distribution.get_command_obj(): " \
                      "creating '%s' command object" % command

            klass = self.get_command_class(command)
            cmd_obj = self.command_obj[command] = klass(self)
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

    def _set_command_options (self, command_obj, option_dict=None):
        """Set the options for 'command_obj' from 'option_dict'.  Basically
        this means copying elements of a dictionary ('option_dict') to
        attributes of an instance ('command').

        'command_obj' must be a Command instance.  If 'option_dict' is not
        supplied, uses the standard option dictionary for this command
        (from 'self.command_options').
        """
        from distutils.core import DEBUG
        
        command_name = command_obj.get_command_name()
        if option_dict is None:
            option_dict = self.get_option_dict(command_name)

        if DEBUG: print "  setting options for '%s' command:" % command_name
        for (option, (source, value)) in option_dict.items():
            if DEBUG: print "    %s = %s (from %s)" % (option, value, source)
            try:
                bool_opts = map(translate_longopt, command_obj.boolean_options)
            except AttributeError:
                bool_opts = []
            try:
                neg_opt = command_obj.negative_opt
            except AttributeError:
                neg_opt = {}

            try:
                is_string = type(value) is StringType
                if neg_opt.has_key(option) and is_string:
                    setattr(command_obj, neg_opt[option], not strtobool(value))
                elif option in bool_opts and is_string:
                    setattr(command_obj, option, strtobool(value))
                elif hasattr(command_obj, option):
                    setattr(command_obj, option, value)
                else:
                    raise DistutilsOptionError, \
                          ("error in %s: command '%s' has no such option '%s'"
                           % (source, command_name, option))
            except ValueError, msg:
                raise DistutilsOptionError, msg

    def reinitialize_command (self, command, reinit_subcommands=0):
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
        it has one).  See the "install" command for an example.  Only
        reinitializes the sub-commands that actually matter, ie. those
        whose test predicates return true.

        Returns the reinitialized command object.
        """
        from distutils.cmd import Command
        if not isinstance(command, Command):
            command_name = command
            command = self.get_command_obj(command_name)
        else:
            command_name = command.get_command_name()

        if not command.finalized:
            return command
        command.initialize_options()
        command.finalized = 0
        self.have_run[command_name] = 0
        self._set_command_options(command)

        if reinit_subcommands:
            for sub in command.get_sub_commands():
                self.reinitialize_command(sub, reinit_subcommands)            

        return command

        
    # -- Methods that operate on the Distribution ----------------------

    def announce (self, msg, level=1):
        """Print 'msg' if 'level' is greater than or equal to the verbosity
        level recorded in the 'verbose' attribute (which, currently, can be
        only 0 or 1).
        """
        if self.verbose >= level:
            print msg


    def run_commands (self):
        """Run each command that was seen on the setup script command line.
        Uses the list of commands found and cache of command objects
        created by 'get_command_obj()'.
        """
        for cmd in self.commands:
            self.run_command(cmd)


    # -- Methods that operate on its Commands --------------------------

    def run_command (self, command):
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

        self.announce("running " + command)
        cmd_obj = self.get_command_obj(command)
        cmd_obj.ensure_finalized()
        cmd_obj.run()
        self.have_run[command] = 1


    # -- Distribution query methods ------------------------------------

    def has_pure_modules (self):
        return len(self.packages or self.py_modules or []) > 0

    def has_ext_modules (self):
        return self.ext_modules and len(self.ext_modules) > 0

    def has_c_libraries (self):
        return self.libraries and len(self.libraries) > 0

    def has_modules (self):
        return self.has_pure_modules() or self.has_ext_modules()

    def has_headers (self):
        return self.headers and len(self.headers) > 0

    def has_scripts (self):
        return self.scripts and len(self.scripts) > 0

    def has_data_files (self):
        return self.data_files and len(self.data_files) > 0

    def is_pure (self):
        return (self.has_pure_modules() and
                not self.has_ext_modules() and
                not self.has_c_libraries())

    # -- Metadata query methods ----------------------------------------

    # If you're looking for 'get_name()', 'get_version()', and so forth,
    # they are defined in a sneaky way: the constructor binds self.get_XXX
    # to self.metadata.get_XXX.  The actual code is in the
    # DistributionMetadata class, below.

# class Distribution


class DistributionMetadata:
    """Dummy class to hold the distribution meta-data: name, version,
    author, and so forth.
    """

    def __init__ (self):
        self.name = None
        self.version = None
        self.author = None
        self.author_email = None
        self.maintainer = None
        self.maintainer_email = None
        self.url = None
        self.licence = None
        self.description = None
        self.long_description = None
        self.keywords = None
        self.platforms = None
        
    def write_pkg_info (self, base_dir):
        """Write the PKG-INFO file into the release tree.
        """

        pkg_info = open( os.path.join(base_dir, 'PKG-INFO'), 'w')

        pkg_info.write('Metadata-Version: 1.0\n')
        pkg_info.write('Name: %s\n' % self.get_name() )
        pkg_info.write('Version: %s\n' % self.get_version() )
        pkg_info.write('Summary: %s\n' % self.get_description() )
        pkg_info.write('Home-page: %s\n' % self.get_url() )
        pkg_info.write('Author: %s\n' % self.get_contact() )
        pkg_info.write('Author-email: %s\n' % self.get_contact_email() )
        pkg_info.write('License: %s\n' % self.get_licence() )

        long_desc = rfc822_escape( self.get_long_description() )
        pkg_info.write('Description: %s\n' % long_desc)

        keywords = string.join( self.get_keywords(), ',')
        if keywords:
            pkg_info.write('Keywords: %s\n' % keywords )

        for platform in self.get_platforms():
            pkg_info.write('Platform: %s\n' % platform )

        pkg_info.close()
        
    # write_pkg_info ()
    
    # -- Metadata query methods ----------------------------------------

    def get_name (self):
        return self.name or "UNKNOWN"

    def get_version(self):
        return self.version or "???"

    def get_fullname (self):
        return "%s-%s" % (self.get_name(), self.get_version())

    def get_author(self):
        return self.author or "UNKNOWN"

    def get_author_email(self):
        return self.author_email or "UNKNOWN"

    def get_maintainer(self):
        return self.maintainer or "UNKNOWN"

    def get_maintainer_email(self):
        return self.maintainer_email or "UNKNOWN"

    def get_contact(self):
        return (self.maintainer or
                self.author or
                "UNKNOWN")

    def get_contact_email(self):
        return (self.maintainer_email or
                self.author_email or
                "UNKNOWN")

    def get_url(self):
        return self.url or "UNKNOWN"

    def get_licence(self):
        return self.licence or "UNKNOWN"

    def get_description(self):
        return self.description or "UNKNOWN"

    def get_long_description(self):
        return self.long_description or "UNKNOWN"

    def get_keywords(self):
        return self.keywords or []

    def get_platforms(self):
        return self.platforms or ["UNKNOWN"]

# class DistributionMetadata


def fix_help_options (options):
    """Convert a 4-tuple 'help_options' list as found in various command
    classes to the 3-tuple form required by FancyGetopt.
    """
    new_options = []
    for help_tuple in options:
        new_options.append(help_tuple[0:3])
    return new_options


if __name__ == "__main__":
    dist = Distribution()
    print "ok"
