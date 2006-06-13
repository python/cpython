""" CommandLine - Get and parse command line options

    NOTE: This still is very much work in progress !!!

    Different version are likely to be incompatible.

    TODO:

    * Incorporate the changes made by (see Inbox)
    * Add number range option using srange()

"""

__copyright__ = """\
Copyright (c), 1997-2006, Marc-Andre Lemburg (mal@lemburg.com)
Copyright (c), 2000-2006, eGenix.com Software GmbH (info@egenix.com)
See the documentation for further information on copyrights,
or contact the author. All Rights Reserved.
"""

__version__ = '1.2'

import sys, getopt, string, glob, os, re, exceptions, traceback

### Helpers

def _getopt_flags(options):

    """ Convert the option list to a getopt flag string and long opt
        list

    """
    s = []
    l = []
    for o in options:
        if o.prefix == '-':
            # short option
            s.append(o.name)
            if o.takes_argument:
                s.append(':')
        else:
            # long option
            if o.takes_argument:
                l.append(o.name+'=')
            else:
                l.append(o.name)
    return string.join(s,''),l

def invisible_input(prompt='>>> '):

    """ Get raw input from a terminal without echoing the characters to
        the terminal, e.g. for password queries.

    """
    import getpass
    entry = getpass.getpass(prompt)
    if entry is None:
        raise KeyboardInterrupt
    return entry

def fileopen(name, mode='wb', encoding=None):

    """ Open a file using mode.

        Default mode is 'wb' meaning to open the file for writing in
        binary mode. If encoding is given, I/O to and from the file is
        transparently encoded using the given encoding.

        Files opened for writing are chmod()ed to 0600.

    """
    if name == 'stdout':
        return sys.stdout
    elif name == 'stderr':
        return sys.stderr
    elif name == 'stdin':
        return sys.stdin
    else:
        if encoding is not None:
            import codecs
            f = codecs.open(name, mode, encoding)
        else:
            f = open(name, mode)
        if 'w' in mode:
            os.chmod(name, 0600)
        return f

def option_dict(options):

    """ Return a dictionary mapping option names to Option instances.
    """
    d = {}
    for option in options:
        d[option.name] = option
    return d

# Alias
getpasswd = invisible_input

_integerRE = re.compile('\s*(-?\d+)\s*$')
_integerRangeRE = re.compile('\s*(-?\d+)\s*-\s*(-?\d+)\s*$')

def srange(s,

           split=string.split,integer=_integerRE,
           integerRange=_integerRangeRE):

    """ Converts a textual representation of integer numbers and ranges
        to a Python list.

        Supported formats: 2,3,4,2-10,-1 - -3, 5 - -2

        Values are appended to the created list in the order specified
        in the string.

    """
    l = []
    append = l.append
    for entry in split(s,','):
        m = integer.match(entry)
        if m:
            append(int(m.groups()[0]))
            continue
        m = integerRange.match(entry)
        if m:
            start,end = map(int,m.groups())
            l[len(l):] = range(start,end+1)
    return l

def abspath(path,

            expandvars=os.path.expandvars,expanduser=os.path.expanduser,
            join=os.path.join,getcwd=os.getcwd):

    """ Return the corresponding absolute path for path.

        path is expanded in the usual shell ways before
        joining it with the current working directory.

    """
    try:
        path = expandvars(path)
    except AttributeError:
        pass
    try:
        path = expanduser(path)
    except AttributeError:
        pass
    return join(getcwd(), path)

### Option classes

class Option:

    """ Option base class. Takes no argument.

    """
    default = None
    helptext = ''
    prefix = '-'
    takes_argument = 0
    has_default = 0
    tab = 15

    def __init__(self,name,help=None):

        if not name[:1] == '-':
            raise TypeError,'option names must start with "-"'
        if name[1:2] == '-':
            self.prefix = '--'
            self.name = name[2:]
        else:
            self.name = name[1:]
        if help:
            self.help = help

    def __str__(self):

        o = self
        name = o.prefix + o.name
        if o.takes_argument:
            name = name + ' arg'
        if len(name) > self.tab:
            name = name + '\n' + ' ' * (self.tab + 1 + len(o.prefix))
        else:
            name = '%-*s ' % (self.tab, name)
        description = o.help
        if o.has_default:
            description = description + ' (%s)' % o.default
        return '%s %s' % (name, description)

class ArgumentOption(Option):

    """ Option that takes an argument.

        An optional default argument can be given.

    """
    def __init__(self,name,help=None,default=None):

        # Basemethod
        Option.__init__(self,name,help)

        if default is not None:
            self.default = default
            self.has_default = 1
        self.takes_argument = 1

class SwitchOption(Option):

    """ Options that can be on or off. Has an optional default value.

    """
    def __init__(self,name,help=None,default=None):

        # Basemethod
        Option.__init__(self,name,help)

        if default is not None:
            self.default = default
            self.has_default = 1

### Application baseclass

class Application:

    """ Command line application interface with builtin argument
        parsing.

    """
    # Options the program accepts (Option instances)
    options = []

    # Standard settings; these are appended to options in __init__
    preset_options = [SwitchOption('-v',
                                   'generate verbose output'),
                      SwitchOption('-h',
                                   'show this help text'),
                      SwitchOption('--help',
                                   'show this help text'),
                      SwitchOption('--debug',
                                   'enable debugging'),
                      SwitchOption('--copyright',
                                   'show copyright'),
                      SwitchOption('--examples',
                                   'show examples of usage')]

    # The help layout looks like this:
    # [header]   - defaults to ''
    #
    # [synopsis] - formatted as '<self.name> %s' % self.synopsis
    #
    # options:
    # [options]  - formatted from self.options
    #
    # [version]  - formatted as 'Version:\n %s' % self.version, if given
    #
    # [about]    - defaults to ''
    #
    # Note: all fields that do not behave as template are formatted
    #       using the instances dictionary as substitution namespace,
    #       e.g. %(name)s will be replaced by the applications name.
    #

    # Header (default to program name)
    header = ''

    # Name (defaults to program name)
    name = ''

    # Synopsis (%(name)s is replaced by the program name)
    synopsis = '%(name)s [option] files...'

    # Version (optional)
    version = ''

    # General information printed after the possible options (optional)
    about = ''

    # Examples of usage to show when the --examples option is given (optional)
    examples = ''

    # Copyright to show
    copyright = __copyright__

    # Apply file globbing ?
    globbing = 1

    # Generate debug output ?
    debug = 0

    # Generate verbose output ?
    verbose = 0

    # Internal errors to catch
    InternalError = exceptions.Exception

    # Instance variables:
    values = None       # Dictionary of passed options (or default values)
                        # indexed by the options name, e.g. '-h'
    files = None        # List of passed filenames
    optionlist = None   # List of passed options

    def __init__(self,argv=None):

        # Setup application specs
        if argv is None:
            argv = sys.argv
        self.filename = os.path.split(argv[0])[1]
        if not self.name:
            self.name = os.path.split(self.filename)[1]
        else:
            self.name = self.name
        if not self.header:
            self.header = self.name
        else:
            self.header = self.header

        # Init .arguments list
        self.arguments = argv[1:]

        # Setup Option mapping
        self.option_map = option_dict(self.options)

        # Append preset options
        for option in self.preset_options:
            if not self.option_map.has_key(option.name):
                self.add_option(option)

        # Init .files list
        self.files = []

        # Start Application
        try:
            # Process startup
            rc = self.startup()
            if rc is not None:
                raise SystemExit,rc

            # Parse command line
            rc = self.parse()
            if rc is not None:
                raise SystemExit,rc

            # Start application
            rc = self.main()
            if rc is None:
                rc = 0

        except SystemExit,rc:
            pass

        except KeyboardInterrupt:
            print
            print '* User Break'
            print
            rc = 1

        except self.InternalError:
            print
            print '* Internal Error (use --debug to display the traceback)'
            if self.debug:
                print
                traceback.print_exc(20, sys.stdout)
            elif self.verbose:
                print '  %s: %s' % sys.exc_info()[:2]
            print
            rc = 1

        raise SystemExit,rc

    def add_option(self, option):

        """ Add a new Option instance to the Application dynamically.

            Note that this has to be done *before* .parse() is being
            executed.

        """
        self.options.append(option)
        self.option_map[option.name] = option

    def startup(self):

        """ Set user defined instance variables.

            If this method returns anything other than None, the
            process is terminated with the return value as exit code.

        """
        return None

    def exit(self, rc=0):

        """ Exit the program.

            rc is used as exit code and passed back to the calling
            program. It defaults to 0 which usually means: OK.

        """
        raise SystemExit, rc

    def parse(self):

        """ Parse the command line and fill in self.values and self.files.

            After having parsed the options, the remaining command line
            arguments are interpreted as files and passed to .handle_files()
            for processing.

            As final step the option handlers are called in the order
            of the options given on the command line.

        """
        # Parse arguments
        self.values = values = {}
        for o in self.options:
            if o.has_default:
                values[o.prefix+o.name] = o.default
            else:
                values[o.prefix+o.name] = 0
        flags,lflags = _getopt_flags(self.options)
        try:
            optlist,files = getopt.getopt(self.arguments,flags,lflags)
            if self.globbing:
                l = []
                for f in files:
                    gf = glob.glob(f)
                    if not gf:
                        l.append(f)
                    else:
                        l[len(l):] = gf
                files = l
            self.optionlist = optlist
            self.files = files + self.files
        except getopt.error,why:
            self.help(why)
            sys.exit(1)

        # Call file handler
        rc = self.handle_files(self.files)
        if rc is not None:
            sys.exit(rc)

        # Call option handlers
        for optionname, value in optlist:

            # Try to convert value to integer
            try:
                value = string.atoi(value)
            except ValueError:
                pass

            # Find handler and call it (or count the number of option
            # instances on the command line)
            handlername = 'handle' + string.replace(optionname, '-', '_')
            try:
                handler = getattr(self, handlername)
            except AttributeError:
                if value == '':
                    # count the number of occurances
                    if values.has_key(optionname):
                        values[optionname] = values[optionname] + 1
                    else:
                        values[optionname] = 1
                else:
                    values[optionname] = value
            else:
                rc = handler(value)
                if rc is not None:
                    raise SystemExit, rc

        # Apply final file check (for backward compatibility)
        rc = self.check_files(self.files)
        if rc is not None:
            sys.exit(rc)

    def check_files(self,filelist):

        """ Apply some user defined checks on the files given in filelist.

            This may modify filelist in place. A typical application
            is checking that at least n files are given.

            If this method returns anything other than None, the
            process is terminated with the return value as exit code.

        """
        return None

    def help(self,note=''):

        self.print_header()
        if self.synopsis:
            print 'Synopsis:'
            # To remain backward compatible:
            try:
                synopsis = self.synopsis % self.name
            except (NameError, KeyError, TypeError):
                synopsis = self.synopsis % self.__dict__
            print ' ' + synopsis
        print
        self.print_options()
        if self.version:
            print 'Version:'
            print ' %s' % self.version
            print
        if self.about:
            print string.strip(self.about % self.__dict__)
            print
        if note:
            print '-'*72
            print 'Note:',note
            print

    def notice(self,note):

        print '-'*72
        print 'Note:',note
        print '-'*72
        print

    def print_header(self):

        print '-'*72
        print self.header % self.__dict__
        print '-'*72
        print

    def print_options(self):

        options = self.options
        print 'Options and default settings:'
        if not options:
            print '  None'
            return
        long = filter(lambda x: x.prefix == '--', options)
        short = filter(lambda x: x.prefix == '-', options)
        items = short + long
        for o in options:
            print ' ',o
        print

    #
    # Example handlers:
    #
    # If a handler returns anything other than None, processing stops
    # and the return value is passed to sys.exit() as argument.
    #

    # File handler
    def handle_files(self,files):

        """ This may process the files list in place.
        """
        return None

    # Short option handler
    def handle_h(self,arg):

        self.help()
        return 0

    def handle_v(self, value):

        """ Turn on verbose output.
        """
        self.verbose = 1

    # Handlers for long options have two underscores in their name
    def handle__help(self,arg):

        self.help()
        return 0

    def handle__debug(self,arg):

        self.debug = 1
        # We don't want to catch internal errors:
        self.InternalError = None

    def handle__copyright(self,arg):

        self.print_header()
        print string.strip(self.copyright % self.__dict__)
        print
        return 0

    def handle__examples(self,arg):

        self.print_header()
        if self.examples:
            print 'Examples:'
            print
            print string.strip(self.examples % self.__dict__)
            print
        else:
            print 'No examples available.'
            print
        return 0

    def main(self):

        """ Override this method as program entry point.

            The return value is passed to sys.exit() as argument.  If
            it is None, 0 is assumed (meaning OK). Unhandled
            exceptions are reported with exit status code 1 (see
            __init__ for further details).

        """
        return None

# Alias
CommandLine = Application

def _test():

    class MyApplication(Application):
        header = 'Test Application'
        version = __version__
        options = [Option('-v','verbose')]

        def handle_v(self,arg):
            print 'VERBOSE, Yeah !'

    cmd = MyApplication()
    if not cmd.values['-h']:
        cmd.help()
    print 'files:',cmd.files
    print 'Bye...'

if __name__ == '__main__':
    _test()
