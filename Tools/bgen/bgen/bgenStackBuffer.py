"""Buffers allocated on the stack."""


from bgenBuffer import FixedInputBufferType, FixedOutputBufferType


class StackOutputBufferType(FixedOutputBufferType):

    """Fixed output buffer allocated on the stack -- passed as (buffer, size).

    Instantiate with the buffer size as parameter.
    """

    def passOutput(self, name):
        return "%s__out__, %s" % (name, self.size)


class VarStackOutputBufferType(StackOutputBufferType):

    """Output buffer allocated on the stack -- passed as (buffer, &size).

    Instantiate with the buffer size as parameter.
    """

    def declareSize(self, name):
        Output("int %s__len__ = %s;", name, self.size)

    def passOutput(self, name):
        return "%s__out__, &%s__len__" % (name, name)

    def mkvalueArgs(self, name):
        return "%s__out__, (int)%s__len__" % (name, name)


class VarVarStackOutputBufferType(VarStackOutputBufferType):

    """Output buffer allocated on the stack -- passed as (buffer, size, &size).

    Instantiate with the buffer size as parameter.
    """

    def passOutput(self, name):
        return "%s__out__, %s__len__, &%s__len__" % (name, name, name)


class ReturnVarStackOutputBufferType(VarStackOutputBufferType):

    """Output buffer allocated on the stack -- passed as (buffer, size) -> size.

    Instantiate with the buffer size as parameter.
    The function's return value is the size.
    (XXX Should have a way to suppress returning it separately, too.)
    """

    def passOutput(self, name):
        return "%s__out__, %s__len__" % (name, name)

    def mkvalueArgs(self, name):
        return "%s__out__, (int)_rv" % name
