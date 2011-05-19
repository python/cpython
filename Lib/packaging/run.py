"""Main command line parser.  Implements the pysetup script."""

import os
import re
import sys
import getopt
import logging

from packaging import logger
from packaging.dist import Distribution
from packaging.util import _is_archive_file
from packaging.command import get_command_class, STANDARD_COMMANDS
from packaging.install import install, install_local_project, remove
from packaging.database import get_distribution, get_distributions
from packaging.depgraph import generate_graph
from packaging.fancy_getopt import FancyGetopt
from packaging.errors import (PackagingArgError, PackagingError,
                              PackagingModuleError, PackagingClassError,
                              CCompilerError)


command_re = re.compile(r'^[a-zA-Z]([a-zA-Z0-9_]*)$')

common_usage = """\
Actions:
%(actions)s

To get more help on an action, use:

    pysetup action --help
"""

create_usage = """\
Usage: pysetup create
   or: pysetup create --help

Create a new Python package.
"""

graph_usage = """\
Usage: pysetup graph dist
   or: pysetup graph --help

Print dependency graph for the distribution.

positional arguments:
   dist  installed distribution name
"""

install_usage = """\
Usage: pysetup install [dist]
   or: pysetup install [archive]
   or: pysetup install [src_dir]
   or: pysetup install --help

Install a Python distribution from the indexes, source directory, or sdist.

positional arguments:
   archive  path to source distribution (zip, tar.gz)
   dist     distribution name to install from the indexes
   scr_dir  path to source directory

"""

metadata_usage = """\
Usage: pysetup metadata [dist] [-f field ...]
   or: pysetup metadata [dist] [--all]
   or: pysetup metadata --help

Print metadata for the distribution.

positional arguments:
   dist  installed distribution name

optional arguments:
   -f     metadata field to print
   --all  print all metadata fields
"""

remove_usage = """\
Usage: pysetup remove dist [-y]
   or: pysetup remove --help

Uninstall a Python distribution.

positional arguments:
   dist  installed distribution name

optional arguments:
   -y  auto confirm package removal
"""

run_usage = """\
Usage: pysetup run [global_opts] cmd1 [cmd1_opts] [cmd2 [cmd2_opts] ...]
   or: pysetup run --help
   or: pysetup run --list-commands
   or: pysetup run cmd --help
"""

list_usage = """\
Usage: pysetup list dist [dist ...]
   or: pysetup list --help
   or: pysetup list --all

Print name, version and location for the matching installed distributions.

positional arguments:
   dist  installed distribution name

optional arguments:
   --all  list all installed distributions
"""

search_usage = """\
Usage: pysetup search [project] [--simple [url]] [--xmlrpc [url] [--fieldname value ...] --operator or|and]
   or: pysetup search --help

Search the indexes for the matching projects.

positional arguments:
    project     the project pattern to search for

optional arguments:
    --xmlrpc [url]      wether to use the xmlrpc index or not. If an url is
                        specified, it will be used rather than the default one.

    --simple [url]      wether to use the simple index or not. If an url is
                        specified, it will be used rather than the default one.

    --fieldname value   Make a search on this field. Can only be used if
                        --xmlrpc has been selected or is the default index.

    --operator or|and   Defines what is the operator to use when doing xmlrpc
                        searchs with multiple fieldnames. Can only be used if
                        --xmlrpc has been selected or is the default index.
"""

global_options = [
    # The fourth entry for verbose means that it can be repeated.
    ('verbose', 'v', "run verbosely (default)", True),
    ('quiet', 'q', "run quietly (turns verbosity off)"),
    ('dry-run', 'n', "don't actually do anything"),
    ('help', 'h', "show detailed help message"),
    ('no-user-cfg', None, 'ignore pydistutils.cfg in your home directory'),
    ('version', None, 'Display the version'),
]

negative_opt = {'quiet': 'verbose'}

display_options = [
    ('help-commands', None, "list all available commands"),
]

display_option_names = [x[0].replace('-', '_') for x in display_options]


def _parse_args(args, options, long_options):
    """Transform sys.argv input into a dict.

    :param args: the args to parse (i.e sys.argv)
    :param options: the list of options to pass to getopt
    :param long_options: the list of string with the names of the long options
                         to be passed to getopt.

    The function returns a dict with options/long_options as keys and matching
    values as values.
    """
    optlist, args = getopt.gnu_getopt(args, options, long_options)
    optdict = {}
    optdict['args'] = args
    for k, v in optlist:
        k = k.lstrip('-')
        if k not in optdict:
            optdict[k] = []
            if v:
                optdict[k].append(v)
        else:
            optdict[k].append(v)
    return optdict


class action_help:
    """Prints a help message when the standard help flags: -h and --help
    are used on the commandline.
    """

    def __init__(self, help_msg):
        self.help_msg = help_msg

    def __call__(self, f):
        def wrapper(*args, **kwargs):
            f_args = args[1]
            if '--help' in f_args or '-h' in f_args:
                print(self.help_msg)
                return
            return f(*args, **kwargs)
        return wrapper


@action_help(create_usage)
def _create(distpatcher, args, **kw):
    from packaging.create import main
    return main()


@action_help(graph_usage)
def _graph(dispatcher, args, **kw):
    name = args[1]
    dist = get_distribution(name, use_egg_info=True)
    if dist is None:
        print('Distribution not found.')
    else:
        dists = get_distributions(use_egg_info=True)
        graph = generate_graph(dists)
        print(graph.repr_node(dist))


@action_help(install_usage)
def _install(dispatcher, args, **kw):
    # first check if we are in a source directory
    if len(args) < 2:
        # are we inside a project dir?
        listing = os.listdir(os.getcwd())
        if 'setup.py' in listing or 'setup.cfg' in listing:
            args.insert(1, os.getcwd())
        else:
            logger.warning('no project to install')
            return

    # installing from a source dir or archive file?
    if os.path.isdir(args[1]) or _is_archive_file(args[1]):
        install_local_project(args[1])
    else:
        # download from PyPI
        install(args[1])


@action_help(metadata_usage)
def _metadata(dispatcher, args, **kw):
    opts = _parse_args(args[1:], 'f:', ['all'])
    if opts['args']:
        name = opts['args'][0]
        dist = get_distribution(name, use_egg_info=True)
        if dist is None:
            logger.warning('%s not installed', name)
            return
    else:
        logger.info('searching local dir for metadata')
        dist = Distribution()
        dist.parse_config_files()

    metadata = dist.metadata

    if 'all' in opts:
        keys = metadata.keys()
    else:
        if 'f' in opts:
            keys = (k for k in opts['f'] if k in metadata)
        else:
            keys = ()

    for key in keys:
        if key in metadata:
            print(metadata._convert_name(key) + ':')
            value = metadata[key]
            if isinstance(value, list):
                for v in value:
                    print('    ' + v)
            else:
                print('    ' + value.replace('\n', '\n    '))


@action_help(remove_usage)
def _remove(distpatcher, args, **kw):
    opts = _parse_args(args[1:], 'y', [])
    if 'y' in opts:
        auto_confirm = True
    else:
        auto_confirm = False

    for dist in set(opts['args']):
        try:
            remove(dist, auto_confirm=auto_confirm)
        except PackagingError:
            logger.warning('%s not installed', dist)


@action_help(run_usage)
def _run(dispatcher, args, **kw):
    parser = dispatcher.parser
    args = args[1:]

    commands = STANDARD_COMMANDS  # + extra commands

    if args == ['--list-commands']:
        print('List of available commands:')
        cmds = sorted(commands)

        for cmd in cmds:
            cls = dispatcher.cmdclass.get(cmd) or get_command_class(cmd)
            desc = getattr(cls, 'description',
                            '(no description available)')
            print('  %s: %s' % (cmd, desc))
        return

    while args:
        args = dispatcher._parse_command_opts(parser, args)
        if args is None:
            return

    # create the Distribution class
    # need to feed setup.cfg here !
    dist = Distribution()

    # Find and parse the config file(s): they will override options from
    # the setup script, but be overridden by the command line.

    # XXX still need to be extracted from Distribution
    dist.parse_config_files()

    try:
        for cmd in dispatcher.commands:
            dist.run_command(cmd, dispatcher.command_options[cmd])

    except KeyboardInterrupt:
        raise SystemExit("interrupted")
    except (IOError, os.error, PackagingError, CCompilerError) as msg:
        raise SystemExit("error: " + str(msg))

    # XXX this is crappy
    return dist


@action_help(list_usage)
def _list(dispatcher, args, **kw):
    opts = _parse_args(args[1:], '', ['all'])
    dists = get_distributions(use_egg_info=True)
    if 'all' in opts:
        results = dists
    else:
        results = [d for d in dists if d.name.lower() in opts['args']]

    for dist in results:
        print('%s %s at %s' % (dist.name, dist.metadata['version'], dist.path))


@action_help(search_usage)
def _search(dispatcher, args, **kw):
    """The search action.

    It is able to search for a specific index (specified with --index), using
    the simple or xmlrpc index types (with --type xmlrpc / --type simple)
    """
    opts = _parse_args(args[1:], '', ['simple', 'xmlrpc'])
    # 1. what kind of index is requested ? (xmlrpc / simple)


actions = [
    ('run', 'Run one or several commands', _run),
    ('metadata', 'Display the metadata of a project', _metadata),
    ('install', 'Install a project', _install),
    ('remove', 'Remove a project', _remove),
    ('search', 'Search for a project in the indexes', _search),
    ('list', 'Search for local projects', _list),
    ('graph', 'Display a graph', _graph),
    ('create', 'Create a Project', _create),
]


class Dispatcher:
    """Reads the command-line options
    """
    def __init__(self, args=None):
        self.verbose = 1
        self.dry_run = False
        self.help = False
        self.script_name = 'pysetup'
        self.cmdclass = {}
        self.commands = []
        self.command_options = {}

        for attr in display_option_names:
            setattr(self, attr, False)

        self.parser = FancyGetopt(global_options + display_options)
        self.parser.set_negative_aliases(negative_opt)
        # FIXME this parses everything, including command options (e.g. "run
        # build -i" errors with "option -i not recognized")
        args = self.parser.getopt(args=args, object=self)

        # if first arg is "run", we have some commands
        if len(args) == 0:
            self.action = None
        else:
            self.action = args[0]

        allowed = [action[0] for action in actions] + [None]
        if self.action not in allowed:
            msg = 'Unrecognized action "%s"' % self.action
            raise PackagingArgError(msg)

        # setting up the logging level from the command-line options
        # -q gets warning, error and critical
        if self.verbose == 0:
            level = logging.WARNING
        # default level or -v gets info too
        # XXX there's a bug somewhere: the help text says that -v is default
        # (and verbose is set to 1 above), but when the user explicitly gives
        # -v on the command line, self.verbose is incremented to 2!  Here we
        # compensate for that (I tested manually).  On a related note, I think
        # it's a good thing to use -q/nothing/-v/-vv on the command line
        # instead of logging constants; it will be easy to add support for
        # logging configuration in setup.cfg for advanced users. --merwok
        elif self.verbose in (1, 2):
            level = logging.INFO
        else:  # -vv and more for debug
            level = logging.DEBUG

        # for display options we return immediately
        option_order = self.parser.get_option_order()

        self.args = args

        if self.help or self.action is None:
            self._show_help(self.parser, display_options_=False)

    def _parse_command_opts(self, parser, args):
        # Pull the current command from the head of the command line
        command = args[0]
        if not command_re.match(command):
            raise SystemExit("invalid command name %r" % (command,))
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
        _negative_opt = negative_opt.copy()

        if hasattr(cmd_class, 'negative_opt'):
            _negative_opt.update(cmd_class.negative_opt)

        # Check for help_options in command class.  They have a different
        # format (tuple of four) so we need to preprocess them here.
        if (hasattr(cmd_class, 'help_options') and
            isinstance(cmd_class.help_options, list)):
            help_options = cmd_class.help_options[:]
        else:
            help_options = []

        # All commands support the global options too, just by adding
        # in 'global_options'.
        parser.set_option_table(global_options +
                                cmd_class.user_options +
                                help_options)
        parser.set_negative_aliases(_negative_opt)
        args, opts = parser.getopt(args[1:])

        if hasattr(opts, 'help') and opts.help:
            self._show_command_help(cmd_class)
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

    def show_help(self):
        self._show_help(self.parser)

    def print_usage(self, parser):
        parser.set_option_table(global_options)

        actions_ = ['    %s: %s' % (name, desc) for name, desc, __ in actions]
        usage = common_usage % {'actions': '\n'.join(actions_)}

        parser.print_help(usage + "\nGlobal options:")

    def _show_help(self, parser, global_options_=True, display_options_=True,
                   commands=[]):
        # late import because of mutual dependence between these modules
        from packaging.command.cmd import Command

        print('Usage: pysetup [options] action [action_options]')
        print('')
        if global_options_:
            self.print_usage(self.parser)
            print('')

        if display_options_:
            parser.set_option_table(display_options)
            parser.print_help(
                "Information display options (just display " +
                "information, ignore any commands)")
            print('')

        for command in commands:
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

    def _show_command_help(self, command):
        if isinstance(command, str):
            command = get_command_class(command)

        name = command.get_command_name()

        desc = getattr(command, 'description', '(no description available)')
        print('Description: %s' % desc)
        print('')

        if (hasattr(command, 'help_options') and
            isinstance(command.help_options, list)):
            self.parser.set_option_table(command.user_options +
                                         command.help_options)
        else:
            self.parser.set_option_table(command.user_options)

        self.parser.print_help("Options:")
        print('')

    def _get_command_groups(self):
        """Helper function to retrieve all the command class names divided
        into standard commands (listed in
        packaging.command.STANDARD_COMMANDS) and extra commands (given in
        self.cmdclass and not standard commands).
        """
        extra_commands = [cmd for cmd in self.cmdclass
                          if cmd not in STANDARD_COMMANDS]
        return STANDARD_COMMANDS, extra_commands

    def print_commands(self):
        """Print out a help message listing all available commands with a
        description of each.  The list is divided into standard commands
        (listed in packaging.command.STANDARD_COMMANDS) and extra commands
        (given in self.cmdclass and not standard commands).  The
        descriptions come from the command class attribute
        'description'.
        """
        std_commands, extra_commands = self._get_command_groups()
        max_length = max(len(command)
                         for commands in (std_commands, extra_commands)
                         for command in commands)

        self.print_command_list(std_commands, "Standard commands", max_length)
        if extra_commands:
            print()
            self.print_command_list(extra_commands, "Extra commands",
                                    max_length)

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

    def __call__(self):
        if self.action is None:
            return
        for action, desc, func in actions:
            if action == self.action:
                return func(self, self.args)
        return -1


def main(args=None):
    dispatcher = Dispatcher(args)
    if dispatcher.action is None:
        return

    return dispatcher()

if __name__ == '__main__':
    sys.exit(main())
