"""Python part of the warnings subsystem."""

import sys, re, types

__all__ = ["warn", "showwarning", "formatwarning", "filterwarnings",
           "resetwarnings"]

defaultaction = "default"
filters = []
onceregistry = {}

def warn(message, category=None, stacklevel=1):
    """Issue a warning, or maybe ignore it or raise an exception."""
    # Check category argument
    if category is None:
        category = UserWarning
    assert issubclass(category, Warning)
    # Get context information
    try:
        caller = sys._getframe(stacklevel)
    except ValueError:
        globals = sys.__dict__
        lineno = 1
    else:
        globals = caller.f_globals
        lineno = caller.f_lineno
    if globals.has_key('__name__'):
        module = globals['__name__']
    else:
        module = "<string>"
    filename = globals.get('__file__')
    if filename:
        fnl = filename.lower()
        if fnl.endswith(".pyc") or fnl.endswith(".pyo"):
            filename = filename[:-1]
    else:
        if module == "__main__":
            filename = sys.argv[0]
        if not filename:
            filename = module
    registry = globals.setdefault("__warningregistry__", {})
    warn_explicit(message, category, filename, lineno, module, registry)

def warn_explicit(message, category, filename, lineno,
                  module=None, registry=None):
    if module is None:
        module = filename
        if module[-3:].lower() == ".py":
            module = module[:-3] # XXX What about leading pathname?
    if registry is None:
        registry = {}
    key = (message, category, lineno)
    # Quick test for common case
    if registry.get(key):
        return
    # Search the filters
    for item in filters:
        action, msg, cat, mod, ln = item
        if (msg.match(message) and
            issubclass(category, cat) and
            mod.match(module) and
            (ln == 0 or lineno == ln)):
            break
    else:
        action = defaultaction
    # Early exit actions
    if action == "ignore":
        registry[key] = 1
        return
    if action == "error":
        raise category(message)
    # Other actions
    if action == "once":
        registry[key] = 1
        oncekey = (message, category)
        if onceregistry.get(oncekey):
            return
        onceregistry[oncekey] = 1
    elif action == "always":
        pass
    elif action == "module":
        registry[key] = 1
        altkey = (message, category, 0)
        if registry.get(altkey):
            return
        registry[altkey] = 1
    elif action == "default":
        registry[key] = 1
    else:
        # Unrecognized actions are errors
        raise RuntimeError(
              "Unrecognized action (%s) in warnings.filters:\n %s" %
              (`action`, str(item)))
    # Print message and context
    showwarning(message, category, filename, lineno)

def showwarning(message, category, filename, lineno, file=None):
    """Hook to write a warning to a file; replace if you like."""
    if file is None:
        file = sys.stderr
    try:
        file.write(formatwarning(message, category, filename, lineno))
    except IOError:
        pass # the file (probably stderr) is invalid - this warning gets lost.

def formatwarning(message, category, filename, lineno):
    """Function to format a warning the standard way."""
    import linecache
    s =  "%s:%s: %s: %s\n" % (filename, lineno, category.__name__, message)
    line = linecache.getline(filename, lineno).strip()
    if line:
        s = s + "  " + line + "\n"
    return s

def filterwarnings(action, message="", category=Warning, module="", lineno=0,
                   append=0):
    """Insert an entry into the list of warnings filters (at the front).

    Use assertions to check that all arguments have the right type."""
    assert action in ("error", "ignore", "always", "default", "module",
                      "once"), "invalid action: %s" % `action`
    assert isinstance(message, types.StringType), "message must be a string"
    assert isinstance(category, types.ClassType), "category must be a class"
    assert issubclass(category, Warning), "category must be a Warning subclass"
    assert type(module) is types.StringType, "module must be a string"
    assert type(lineno) is types.IntType and lineno >= 0, \
           "lineno must be an int >= 0"
    item = (action, re.compile(message, re.I), category,
            re.compile(module), lineno)
    if append:
        filters.append(item)
    else:
        filters.insert(0, item)

def resetwarnings():
    """Clear the list of warning filters, so that no filters are active."""
    filters[:] = []

class _OptionError(Exception):
    """Exception used by option processing helpers."""
    pass

# Helper to process -W options passed via sys.warnoptions
def _processoptions(args):
    for arg in args:
        try:
            _setoption(arg)
        except _OptionError, msg:
            print >>sys.stderr, "Invalid -W option ignored:", msg

# Helper for _processoptions()
def _setoption(arg):
    parts = arg.split(':')
    if len(parts) > 5:
        raise _OptionError("too many fields (max 5): %s" % `arg`)
    while len(parts) < 5:
        parts.append('')
    action, message, category, module, lineno = [s.strip()
                                                 for s in parts]
    action = _getaction(action)
    message = re.escape(message)
    category = _getcategory(category)
    module = re.escape(module)
    if module:
        module = module + '$'
    if lineno:
        try:
            lineno = int(lineno)
            if lineno < 0:
                raise ValueError
        except (ValueError, OverflowError):
            raise _OptionError("invalid lineno %s" % `lineno`)
    else:
        lineno = 0
    filterwarnings(action, message, category, module, lineno)

# Helper for _setoption()
def _getaction(action):
    if not action:
        return "default"
    if action == "all": return "always" # Alias
    for a in ['default', 'always', 'ignore', 'module', 'once', 'error']:
        if a.startswith(action):
            return a
    raise _OptionError("invalid action: %s" % `action`)

# Helper for _setoption()
def _getcategory(category):
    if not category:
        return Warning
    if re.match("^[a-zA-Z0-9_]+$", category):
        try:
            cat = eval(category)
        except NameError:
            raise _OptionError("unknown warning category: %s" % `category`)
    else:
        i = category.rfind(".")
        module = category[:i]
        klass = category[i+1:]
        try:
            m = __import__(module, None, None, [klass])
        except ImportError:
            raise _OptionError("invalid module name: %s" % `module`)
        try:
            cat = getattr(m, klass)
        except AttributeError:
            raise _OptionError("unknown warning category: %s" % `category`)
    if (not isinstance(cat, types.ClassType) or
        not issubclass(cat, Warning)):
        raise _OptionError("invalid warning category: %s" % `category`)
    return cat

# Self-test
def _test():
    import getopt
    testoptions = []
    try:
        opts, args = getopt.getopt(sys.argv[1:], "W:")
    except getopt.error, msg:
        print >>sys.stderr, msg
        return
    for o, a in opts:
        testoptions.append(a)
    try:
        _processoptions(testoptions)
    except _OptionError, msg:
        print >>sys.stderr, msg
        return
    for item in filters: print item
    hello = "hello world"
    warn(hello); warn(hello); warn(hello); warn(hello)
    warn(hello, UserWarning)
    warn(hello, DeprecationWarning)
    for i in range(3):
        warn(hello)
    filterwarnings("error", "", Warning, "", 0)
    try:
        warn(hello)
    except Exception, msg:
        print "Caught", msg.__class__.__name__ + ":", msg
    else:
        print "No exception"
    resetwarnings()
    try:
        filterwarnings("booh", "", Warning, "", 0)
    except Exception, msg:
        print "Caught", msg.__class__.__name__ + ":", msg
    else:
        print "No exception"

# Module initialization
if __name__ == "__main__":
    import __main__
    sys.modules['warnings'] = __main__
    _test()
else:
    _processoptions(sys.warnoptions)
    filterwarnings("ignore", category=OverflowWarning, append=1)
