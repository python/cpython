"""
fancycompleter: colorful TAB completion for Python prompt
"""
from __future__ import with_statement
from __future__ import print_function

import rlcompleter
import sys
import types
import os.path
from itertools import count

PY3K = sys.version_info[0] >= 3

# python3 compatibility
# ---------------------
try:
    from itertools import izip
except ImportError:
    izip = zip

try:
    from types import ClassType
except ImportError:
    ClassType = type

try:
    unicode
except NameError:
    unicode = str

# ----------------------


class LazyVersion(object):

    def __init__(self, pkg):
        self.pkg = pkg
        self.__version = None

    @property
    def version(self):
        if self.__version is None:
            self.__version = self._load_version()
        return self.__version

    def _load_version(self):
        try:
            from pkg_resources import get_distribution, DistributionNotFound
        except ImportError:
            return 'N/A'
        #
        try:
            return get_distribution(self.pkg).version
        except DistributionNotFound:
            # package is not installed
            return 'N/A'

    def __repr__(self):
        return self.version

    def __eq__(self, other):
        return self.version == other

    def __ne__(self, other):
        return not self == other


__version__ = LazyVersion(__name__)

# ----------------------


class Color:
    black = '30'
    darkred = '31'
    darkgreen = '32'
    brown = '33'
    darkblue = '34'
    purple = '35'
    teal = '36'
    lightgray = '37'
    darkgray = '30;01'
    red = '31;01'
    green = '32;01'
    yellow = '33;01'
    blue = '34;01'
    fuchsia = '35;01'
    turquoise = '36;01'
    white = '37;01'

    @classmethod
    def set(cls, color, string):
        try:
            color = getattr(cls, color)
        except AttributeError:
            pass
        return '\x1b[%sm%s\x1b[00m' % (color, string)


class DefaultConfig:

    consider_getitems = True
    prefer_pyrepl = True
    use_colors = 'auto'
    readline = None  # set by setup()
    using_pyrepl = False  # overwritten by find_pyrepl

    color_by_type = {
        types.BuiltinMethodType: Color.turquoise,
        types.MethodType: Color.turquoise,
        type((42).__add__): Color.turquoise,
        type(int.__add__): Color.turquoise,
        type(str.replace): Color.turquoise,

        types.FunctionType: Color.blue,
        types.BuiltinFunctionType: Color.blue,

        ClassType: Color.fuchsia,
        type: Color.fuchsia,

        types.ModuleType: Color.teal,
        type(None): Color.lightgray,
        str: Color.green,
        unicode: Color.green,
        int: Color.yellow,
        float: Color.yellow,
        complex: Color.yellow,
        bool: Color.yellow,
    }
    # Fallback to look up colors by `isinstance` when not matched
    # via color_by_type.
    color_by_baseclass = [
        ((BaseException,), Color.red),
    ]

    def find_pyrepl(self):
        try:
            import pyrepl.readline
            import pyrepl.completing_reader
        except ImportError:
            return None
        self.using_pyrepl = True
        if hasattr(pyrepl.completing_reader, 'stripcolor'):
            # modern version of pyrepl
            return pyrepl.readline, True
        else:
            return pyrepl.readline, False

    def find_pyreadline(self):
        try:
            import readline
            import pyreadline  # noqa: F401  # XXX: needed really?
            from pyreadline.modes import basemode
        except ImportError:
            return None
        if hasattr(basemode, 'stripcolor'):
            # modern version of pyreadline; see:
            # https://github.com/pyreadline/pyreadline/pull/48
            return readline, True
        else:
            return readline, False

    def find_best_readline(self):
        if self.prefer_pyrepl:
            result = self.find_pyrepl()
            if result:
                return result
        if sys.platform == 'win32':
            result = self.find_pyreadline()
            if result:
                return result
        import readline
        return readline, False  # by default readline does not support colors

    def setup(self):
        self.readline, supports_color = self.find_best_readline()
        if self.use_colors == 'auto':
            self.use_colors = supports_color


def my_execfile(filename, mydict):
    with open(filename) as f:
        code = compile(f.read(), filename, 'exec')
        exec(code, mydict)


class ConfigurableClass:
    DefaultConfig = None
    config_filename = None

    def get_config(self, Config):
        if Config is not None:
            return Config()
        # try to load config from the ~/filename file
        filename = '~/' + self.config_filename
        rcfile = os.path.normpath(os.path.expanduser(filename))
        if not os.path.exists(rcfile):
            return self.DefaultConfig()

        mydict = {}
        try:
            my_execfile(rcfile, mydict)
        except Exception as exc:
            import traceback

            sys.stderr.write("** error when importing %s: %r **\n" % (filename, exc))
            traceback.print_tb(sys.exc_info()[2])
            return self.DefaultConfig()

        try:
            Config = mydict["Config"]
        except KeyError:
            return self.DefaultConfig()

        try:
            return Config()
        except Exception as exc:
            err = "error when setting up Config from %s: %s" % (filename, exc)
            tb = sys.exc_info()[2]
            if tb and tb.tb_next:
                tb = tb.tb_next
                err_fname = tb.tb_frame.f_code.co_filename
                err_lnum = tb.tb_lineno
                err += " (%s:%d)" % (err_fname, err_lnum,)
            sys.stderr.write("** %s **\n" % err)
        return self.DefaultConfig()


class Completer(rlcompleter.Completer, ConfigurableClass):
    """
    When doing someting like a.b.<TAB>, display only the attributes of
    b instead of the full a.b.attr string.

    Optionally, display the various completions in different colors
    depending on the type.
    """

    DefaultConfig = DefaultConfig
    config_filename = '.fancycompleterrc.py'

    def __init__(self, namespace=None, Config=None):
        rlcompleter.Completer.__init__(self, namespace)
        self.config = self.get_config(Config)
        self.config.setup()
        readline = self.config.readline
        if hasattr(readline, '_setup'):
            # this is needed to offer pyrepl a better chance to patch
            # raw_input. Usually, it does at import time, but is we are under
            # pytest with output captured, at import time we don't have a
            # terminal and thus the raw_input hook is not installed
            readline._setup()
        if self.config.use_colors:
            readline.parse_and_bind('set dont-escape-ctrl-chars on')
        if self.config.consider_getitems:
            delims = readline.get_completer_delims()
            delims = delims.replace('[', '')
            delims = delims.replace(']', '')
            readline.set_completer_delims(delims)

    def complete(self, text, state):
        """
        stolen from:
        http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/496812
        """
        if text == "":
            return ['\t', None][state]
        else:
            return rlcompleter.Completer.complete(self, text, state)

    def _callable_postfix(self, val, word):
        # disable automatic insertion of '(' for global callables:
        # this method exists only in Python 2.6+
        return word

    def global_matches(self, text):
        import keyword
        names = rlcompleter.Completer.global_matches(self, text)
        prefix = commonprefix(names)
        if prefix and prefix != text:
            return [prefix]

        names.sort()
        values = []
        for name in names:
            clean_name = name.rstrip(': ')
            if clean_name in keyword.kwlist:
                values.append(None)
            else:
                try:
                    values.append(eval(name, self.namespace))
                except Exception as exc:
                    values.append(exc)
        if self.config.use_colors and names:
            return self.color_matches(names, values)
        return names

    def attr_matches(self, text):
        expr, attr = text.rsplit('.', 1)
        if '(' in expr or ')' in expr:  # don't call functions
            return []
        try:
            thisobject = eval(expr, self.namespace)
        except Exception:
            return []

        # get the content of the object, except __builtins__
        words = set(dir(thisobject))
        words.discard("__builtins__")

        if hasattr(thisobject, '__class__'):
            words.add('__class__')
            words.update(rlcompleter.get_class_members(thisobject.__class__))
        names = []
        values = []
        n = len(attr)
        if attr == '':
            noprefix = '_'
        elif attr == '_':
            noprefix = '__'
        else:
            noprefix = None
        words = sorted(words)
        while True:
            for word in words:
                if (word[:n] == attr and
                        not (noprefix and word[:n+1] == noprefix)):
                    try:
                        val = getattr(thisobject, word)
                    except Exception:
                        val = None  # Include even if attribute not set

                    if not PY3K and isinstance(word, unicode):
                        # this is needed because pyrepl doesn't like unicode
                        # completions: as soon as it finds something which is not str,
                        # it stops.
                        word = word.encode('utf-8')

                    names.append(word)
                    values.append(val)
            if names or not noprefix:
                break
            if noprefix == '_':
                noprefix = '__'
            else:
                noprefix = None

        if not names:
            return []

        if len(names) == 1:
            return ['%s.%s' % (expr, names[0])]  # only option, no coloring.

        prefix = commonprefix(names)
        if prefix and prefix != attr:
            return ['%s.%s' % (expr, prefix)]  # autocomplete prefix

        if self.config.use_colors:
            return self.color_matches(names, values)

        if prefix:
            names += [' ']
        return names

    def color_matches(self, names, values):
        matches = [self.color_for_obj(i, name, obj)
                   for i, name, obj
                   in izip(count(), names, values)]
        # We add a space at the end to prevent the automatic completion of the
        # common prefix, which is the ANSI ESCAPE sequence.
        return matches + [' ']

    def color_for_obj(self, i, name, value):
        t = type(value)
        color = self.config.color_by_type.get(t, None)
        if color is None:
            for x, _color in self.config.color_by_baseclass:
                if isinstance(value, x):
                    color = _color
                    break
            else:
                color = '00'
        # hack: prepend an (increasing) fake escape sequence,
        # so that readline can sort the matches correctly.
        return '\x1b[%03d;00m' % i + Color.set(color, name)


def commonprefix(names, base=''):
    """ return the common prefix of all 'names' starting with 'base'
    """
    if base:
        names = [x for x in names if x.startswith(base)]
    if not names:
        return ''
    s1 = min(names)
    s2 = max(names)
    for i, c in enumerate(s1):
        if c != s2[i]:
            return s1[:i]
    return s1


def has_leopard_libedit(config):
    # Detect if we are using Leopard's libedit.
    # Adapted from IPython's rlineimpl.py.
    if config.using_pyrepl or sys.platform != 'darwin':
        return False

    # Official Python docs state that 'libedit' is in the docstring for
    # libedit readline.
    return config.readline.__doc__ and 'libedit' in config.readline.__doc__


def setup():
    """
    Install fancycompleter as the default completer for readline.
    """
    completer = Completer()
    readline = completer.config.readline
    if has_leopard_libedit(completer.config):
        readline.parse_and_bind("bind ^I rl_complete")
    else:
        readline.parse_and_bind('tab: complete')
    readline.set_completer(completer.complete)
    return completer


def interact_pyrepl():
    import sys
    from pyrepl import readline
    from pyrepl.simple_interact import run_multiline_interactive_console
    sys.modules['readline'] = readline
    run_multiline_interactive_console()


def setup_history(completer, persist_history):
    import atexit
    readline = completer.config.readline
    #
    if isinstance(persist_history, (str, unicode)):
        filename = persist_history
    else:
        filename = '~/.history.py'
    filename = os.path.expanduser(filename)
    if os.path.isfile(filename):
        readline.read_history_file(filename)

    def save_history():
        readline.write_history_file(filename)
    atexit.register(save_history)


def interact(persist_history=None):
    """
    Main entry point for fancycompleter: run an interactive Python session
    after installing fancycompleter.

    This function is supposed to be called at the end of PYTHONSTARTUP:

      - if we are using pyrepl: install fancycompleter, run pyrepl multiline
        prompt, and sys.exit().  The standard python prompt will never be
        reached

      - if we are not using pyrepl: install fancycompleter and return.  The
        execution will continue as normal, and the standard python prompt will
        be displayed.

    This is necessary because there is no way to tell the standard python
    prompt to use the readline provided by pyrepl instead of the builtin one.

    By default, pyrepl is preferred and automatically used if found.
    """
    import sys
    completer = setup()
    if persist_history:
        setup_history(completer, persist_history)
    if completer.config.using_pyrepl and '__pypy__' not in sys.builtin_module_names:
        # if we are on PyPy, we don't need to run a "fake" interpeter, as the
        # standard one is fake enough :-)
        interact_pyrepl()
        sys.exit()


class Installer(object):
    """
    Helper to install fancycompleter in PYTHONSTARTUP
    """

    def __init__(self, basepath, force):
        fname = os.path.join(basepath, 'python_startup.py')
        self.filename = os.path.expanduser(fname)
        self.force = force

    def check(self):
        PYTHONSTARTUP = os.environ.get('PYTHONSTARTUP')
        if PYTHONSTARTUP:
            return 'PYTHONSTARTUP already defined: %s' % PYTHONSTARTUP
        if os.path.exists(self.filename):
            return '%s already exists' % self.filename

    def install(self):
        import textwrap
        error = self.check()
        if error and not self.force:
            print(error)
            print('Use --force to overwrite.')
            return False
        with open(self.filename, 'w') as f:
            f.write(textwrap.dedent("""
                import fancycompleter
                fancycompleter.interact(persist_history=True)
            """))
        self.set_env_var()
        return True

    def set_env_var(self):
        if sys.platform == 'win32':
            os.system('SETX PYTHONSTARTUP "%s"' % self.filename)
            print('%PYTHONSTARTUP% set to', self.filename)
        else:
            print('startup file written to', self.filename)
            print('Append this line to your ~/.bashrc:')
            print('    export PYTHONSTARTUP=%s' % self.filename)


if __name__ == '__main__':
    def usage():
        print('Usage: python -m fancycompleter install [-f|--force]')
        sys.exit(1)

    cmd = None
    force = False
    for item in sys.argv[1:]:
        if item in ('install',):
            cmd = item
        elif item in ('-f', '--force'):
            force = True
        else:
            usage()
    #
    if cmd == 'install':
        installer = Installer('~', force)
        installer.install()
    else:
        usage()
