"""distutils.dist

Provides the Distribution class, which represents the module distribution
being built/installed/distributed."""

# created 2000/04/03, Greg Ward
# (extricated from core.py; actually dates back to the beginning)

__revision__ = "$Id$"

import sys, string, re
from types import *
from copy import copy
from distutils.errors import *
from distutils.fancy_getopt import FancyGetopt, longopt_xlate


# Regex to define acceptable Distutils command names.  This is not *quite*
# the same as a Python NAME -- I don't allow leading underscores.  The fact
# that they're very similar is no coincidence; the default naming scheme is
# to look for a Python module named after the command.
command_re = re.compile (r'^[a-zA-Z]([a-zA-Z0-9_]*)$')


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
    # supplied to the setup script prior to any actual commands.
    # Eg. "./setup.py -n" or "./setup.py --quiet" both take advantage of
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
                      ('help', 'h',
                       "show this help message, plus help for any commands " +
                       "given on the command-line"),
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
         "print the name of the maintainer if present, "
         "else author"),
        ('contact-email', None,
         "print the email of the maintainer if present, "
         "else author"),
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
        ]
    display_option_names = map(lambda x: string.translate(x[0], longopt_xlate),
                           display_options)

    # negative options are options that exclude other options
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
        self.help = 0
        for attr in self.display_option_names:
            setattr(self, attr, 0)

        # Store the distribution meta-data (name, version, author, and so
        # forth) in a separate object -- we're getting to have enough
        # information here (and enough command-line options) that it's
        # worth it.  Also delegate 'get_XXX()' methods to the 'metadata'
        # object in a sneaky and underhanded (but efficient!) way.
        self.metadata = DistributionMetadata ()
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

        # These options are really the business of various commands, rather
        # than of the Distribution itself.  We provide aliases for them in
        # Distribution as a convenience to the developer.
        self.packages = None
        self.package_dir = None
        self.py_modules = None
        self.libraries = None
        self.ext_modules = None
        self.ext_package = None
        self.include_dirs = None
        self.extra_path = None

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
        # the setup script) to possibly override any or all of these
        # distribution options.

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
                if hasattr (self.metadata, key):
                    setattr (self.metadata, key, val)
                elif hasattr (self, key):
                    setattr (self, key, val)
                else:
                    raise DistutilsSetupError, \
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

        # late import because of mutual dependence between these modules
        from distutils.cmd import Command
        from distutils.core import usage

        # We have to parse the command line a bit at a time -- global
        # options, then the first command, then its options, and so on --
        # because each command will be handled by a different class, and
        # the options that are valid for a particular class aren't
        # known until we instantiate the command class, which doesn't
        # happen until we know what the command is.

        self.commands = []
        parser = FancyGetopt (self.global_options + self.display_options)
        parser.set_negative_aliases (self.negative_opt)
        parser.set_aliases ({'license': 'licence'})
        args = parser.getopt (object=self)
        option_order = parser.get_option_order()

        # for display options we return immediately
        if self.handle_display_options(option_order):
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
            # want to be sure that the basic "command" interface is
            # implemented.
            if not isinstance (cmd_obj, Command):
                raise DistutilsClassError, \
                      "command class %s must subclass Command" % \
                      cmd_obj.__class__

            # Also make sure that the command object provides a list of its
            # known options
            if not (hasattr (cmd_obj, 'user_options') and
                    type (cmd_obj.user_options) is ListType):
                raise DistutilsClassError, \
                      ("command class %s must provide " +
                       "'user_options' attribute (a list of tuples)") % \
                      cmd_obj.__class__

            # Poof! like magic, all commands support the global
            # options too, just by adding in 'global_options'.
            negative_opt = self.negative_opt
            if hasattr (cmd_obj, 'negative_opt'):
                negative_opt = copy (negative_opt)
                negative_opt.update (cmd_obj.negative_opt)

            parser.set_option_table (self.global_options +
                                     cmd_obj.user_options)
            parser.set_negative_aliases (negative_opt)
            args = parser.getopt (args[1:], cmd_obj)
            if cmd_obj.help:
                parser.set_option_table (self.global_options)
                parser.print_help ("Global options:")
                print

                parser.set_option_table (cmd_obj.user_options)
                parser.print_help ("Options for '%s' command:" % command)
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
            parser.set_option_table (self.global_options)
            parser.print_help (
                "Global options (apply to all commands, " +
                "or can be used per command):")
            print

            if not self.commands:
                parser.set_option_table (self.display_options)
                parser.print_help (
                    "Information display options (just display " +
                    "information, ignore any commands)")
                print

            for command in self.commands:
                klass = self.find_command_class (command)
                parser.set_option_table (klass.user_options)
                parser.print_help ("Options for '%s' command:" % command)
                print

            print usage
            return

        # Oops, no commands found -- an end-user error
        if not self.commands:
            raise DistutilsArgError, "no commands supplied"

        # All is well: return true
        return 1

    # parse_command_line()

    def handle_display_options (self, option_order):
        """If there were any non-global "display-only" options
         (--help-commands or the metadata display options) on the command
         line, display the requested info and return true; else return
         false."""

        from distutils.core import usage

        # User just wants a list of commands -- we'll print it out and stop
        # processing now (ie. if they ran "setup --help-commands foo bar",
        # we ignore "foo bar").
        if self.help_commands:
            self.print_commands ()
            print
            print usage
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
                opt = string.translate (opt, longopt_xlate)
                print getattr(self.metadata, "get_"+opt)()
                any_display_options = 1

        return any_display_options

    # handle_display_options()

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
        klass_name = command

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
        """Run each command that was seen on the setup script command line.
           Uses the list of commands found and cache of command objects
           created by 'create_command_obj()'."""

        for cmd in self.commands:
            self.run_command (cmd)


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


    # -- Distribution query methods ------------------------------------

    def has_pure_modules (self):
        return len (self.packages or self.py_modules or []) > 0

    def has_ext_modules (self):
        return self.ext_modules and len (self.ext_modules) > 0

    def has_c_libraries (self):
        return self.libraries and len (self.libraries) > 0

    def has_modules (self):
        return self.has_pure_modules() or self.has_ext_modules()

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
    author, and so forth."""

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

# class DistributionMetadata

if __name__ == "__main__":
    dist = Distribution ()
    print "ok"
