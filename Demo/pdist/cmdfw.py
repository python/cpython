"Framework for command line interfaces like CVS.  See class CmdFrameWork."


class CommandFrameWork:

    """Framework class for command line interfaces like CVS.

    The general command line structure is

            command [flags] subcommand [subflags] [argument] ...

    There's a class variable GlobalFlags which specifies the
    global flags options.  Subcommands are defined by defining
    methods named do_<subcommand>.  Flags for the subcommand are
    defined by defining class or instance variables named
    flags_<subcommand>.  If there's no command, method default()
    is called.  The __doc__ strings for the do_ methods are used
    for the usage message, printed after the general usage message
    which is the class variable UsageMessage.  The class variable
    PostUsageMessage is printed after all the do_ methods' __doc__
    strings.  The method's return value can be a suggested exit
    status.  [XXX Need to rewrite this to clarify it.]

    Common usage is to derive a class, instantiate it, and then call its
    run() method; by default this takes its arguments from sys.argv[1:].
    """

    UsageMessage = \
      "usage: (name)s [flags] subcommand [subflags] [argument] ..."

    PostUsageMessage = None

    GlobalFlags = ''

    def __init__(self):
        """Constructor, present for completeness."""
        pass

    def run(self, args = None):
        """Process flags, subcommand and options, then run it."""
        import getopt, sys
        if args is None: args = sys.argv[1:]
        try:
            opts, args = getopt.getopt(args, self.GlobalFlags)
        except getopt.error, msg:
            return self.usage(msg)
        self.options(opts)
        if not args:
            self.ready()
            return self.default()
        else:
            cmd = args[0]
            mname = 'do_' + cmd
            fname = 'flags_' + cmd
            try:
                method = getattr(self, mname)
            except AttributeError:
                return self.usage("command %r unknown" % (cmd,))
            try:
                flags = getattr(self, fname)
            except AttributeError:
                flags = ''
            try:
                opts, args = getopt.getopt(args[1:], flags)
            except getopt.error, msg:
                return self.usage(
                        "subcommand %s: " % cmd + str(msg))
            self.ready()
            return method(opts, args)

    def options(self, opts):
        """Process the options retrieved by getopt.
        Override this if you have any options."""
        if opts:
            print "-"*40
            print "Options:"
            for o, a in opts:
                print 'option', o, 'value', repr(a)
            print "-"*40

    def ready(self):
        """Called just before calling the subcommand."""
        pass

    def usage(self, msg = None):
        """Print usage message.  Return suitable exit code (2)."""
        if msg: print msg
        print self.UsageMessage % {'name': self.__class__.__name__}
        docstrings = {}
        c = self.__class__
        while 1:
            for name in dir(c):
                if name[:3] == 'do_':
                    if docstrings.has_key(name):
                        continue
                    try:
                        doc = getattr(c, name).__doc__
                    except:
                        doc = None
                    if doc:
                        docstrings[name] = doc
            if not c.__bases__:
                break
            c = c.__bases__[0]
        if docstrings:
            print "where subcommand can be:"
            names = docstrings.keys()
            names.sort()
            for name in names:
                print docstrings[name]
        if self.PostUsageMessage:
            print self.PostUsageMessage
        return 2

    def default(self):
        """Default method, called when no subcommand is given.
        You should always override this."""
        print "Nobody expects the Spanish Inquisition!"


def test():
    """Test script -- called when this module is run as a script."""
    import sys
    class Hello(CommandFrameWork):
        def do_hello(self, opts, args):
            "hello -- print 'hello world', needs no arguments"
            print "Hello, world"
    x = Hello()
    tests = [
            [],
            ['hello'],
            ['spam'],
            ['-x'],
            ['hello', '-x'],
            None,
            ]
    for t in tests:
        print '-'*10, t, '-'*10
        sts = x.run(t)
        print "Exit status:", repr(sts)


if __name__ == '__main__':
    test()
