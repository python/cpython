# XXX this is ridiculous! if commands need to pass options around,
# they can just pass them via the 'run' method... what we REALLY need
# is a way for commands to get at each other, via the Distribution!

class Options:
    """Used by Distribution and Command to encapsulate distribution
       and command options -- parsing them from command-line arguments,
       passing them between the distribution and command objects, etc."""

    # -- Creation/initialization methods -------------------------------

    def __init__ (self, owner):

        # 'owner' is the object (presumably either a Distribution
        # or Command instance) to which this set of options applies.
        self.owner = owner

        # The option table: maps option names to dictionaries, which
        # look something like:
        #    { 'longopt': long command-line option string (optional)
        #      'shortopt': short option (1 char) (optional)
        #      'type': 'string', 'boolean', or 'list'
        #      'description': text description (eg. for help strings)
        #      'default': default value for the option
        #      'send': list of (cmd,option) tuples: send option down the line
        #      'receive': (cmd,option) tuple: pull option from upstream
        #    }
        self.table = {}


    def set_basic_options (self, *options):
        """Add very basic options: no separate longopt, no fancy typing, no
           send targets or receive destination.  The arguments should just
           be {1..4}-tuples of
              (name [, shortopt [, description [, default]]])
           If name ends with '=', the option takes a string argument;
           otherwise it's boolean."""

        for opt in options:
            if not (type (opt) is TupleType and 1 <= len (opt) <= 4):
                raise ValueError, \
                      ("invalid basic option record '%s': " + \
                       "must be tuple of length 1 .. 4") % opt

            elements = ('name', 'shortopt', 'description', 'default')
            name = opt[0]
            self.table[name] = {}
            for i in range (1,4):
                if len (opt) >= i:
                    self.table[name][elements[i]] = opt[i]
                else:
                    break

    # set_basic_options ()


    def add_option (self, name, **args):

        # XXX should probably sanity-check the keys of args
        self.table[name] = args


    # ------------------------------------------------------------------

    # These are in the order that they will execute in to ensure proper
    # prioritizing of option sources -- the default value is the most
    # basic; it can be overridden by "client options" (the keyword args
    # passed from setup.py to the 'setup' function); they in turn lose to
    # options passed in "from above" (ie. from the Distribution, or from
    # higher-level Commands); these in turn may be overridden by
    # command-line arguments (which come from the end-user, the runner of
    # setup.py).  Only when all this is done can we pass options down to
    # other Commands.

    # Hmmm, it also matters in which order Commands are processed: should a
    # command-line option to 'make_blib' take precedence over the
    # corresponding value passed down from its boss, 'build'?

    def set_defaults (self):
        pass

    def set_client_options (self, options):
        # 'self' should be a Distribution instance for this one --
        # this is to process the kw args passed to 'setup'
        pass

    def receive_option (self, option, value):
        # do we need to know the identity of the sender? don't
        # think we should -- too much B&D

        # oh, 'self' should be anything *but* a Distribution (ie.
        # a Command instance) -- only Commands take orders from above!
        # (ironically enough)
        pass

    def parse_command_line (self, args):
        # here, 'self' can usefully be either a Distribution (for parsing
        # "global" command-line options) or a Command (for "command-specific"
        # options)
        pass

           
    def send_option (self, option, dest):
        # perhaps this should not take a dest, but send the option
        # to all possible receivers?
        pass


    # ------------------------------------------------------------------

# class Options
