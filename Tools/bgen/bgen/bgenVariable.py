"""Variables, arguments and argument transfer modes etc."""


# Values to represent argument transfer modes
InMode    = 1 # input-only argument
OutMode   = 2 # output-only argument
InOutMode = 3 # input-output argument
ModeMask  = 3 # bits to keep for mode


# Special cases for mode/flags argument
# XXX This is still a mess!
SelfMode   =  4+InMode  # this is 'self' -- don't declare it
ReturnMode =  8+OutMode # this is the function return value
ErrorMode  = 16+OutMode # this is an error status -- turn it into an exception


class Variable:

    """A Variable holds a type, a name, a transfer mode and flags.

    Most of its methods call the correponding type method with the
    variable name.
    """

    def __init__(self, type, name = None, flags = InMode):
        """Call with a type, a name and flags.

        If name is None, it muse be set later.
        flags defaults to InMode.
        """
        self.type = type
        self.name = name
        self.flags = flags
        self.mode = flags & ModeMask

    def declare(self):
        """Declare the variable if necessary.

        If it is "self", it is not declared.
        """
        if self.flags != SelfMode:
            self.type.declare(self.name)

    def getargsFormat(self):
        """Call the type's getargsFormatmethod."""
        return self.type.getargsFormat()

    def getargsArgs(self):
        """Call the type's getargsArgsmethod."""
        return self.type.getargsArgs(self.name)

    def getargsCheck(self):
        return self.type.getargsCheck(self.name)

    def passArgument(self):
        """Return the string required to pass the variable as argument.

        For "in" arguments, return the variable name.
        For "out" and "in out" arguments,
        return its name prefixed with "&".
        """
        if self.mode == InMode:
            return self.type.passInput(self.name)
        if self.mode in (OutMode, InOutMode):
            return self.type.passOutput(self.name)
        # XXX Shouldn't get here
        return "/*mode?*/" + self.type.passInput(self.name)

    def errorCheck(self):
        """Check for an error if necessary.

        This only generates code if the variable's mode is ErrorMode.
        """
        if self.flags == ErrorMode:
            self.type.errorCheck(self.name)

    def mkvalueFormat (self):
        """Call the type's mkvalueFormat method."""
        return self.type.mkvalueFormat()

    def mkvalueArgs(self):
        """Call the type's mkvalueArgs method."""
        return self.type.mkvalueArgs(self.name)

    def cleanup(self):
        """Call the type's cleanup method."""
        return self.type.cleanup(self.name)
