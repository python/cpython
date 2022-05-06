Added ``object.__getstate__`` which provides the default implementation of
the ``__getstate__()`` method.

Copying and pickling instances of subclasses of builtin types bytearray,
set, frozenset, collections.OrderedDict, collections.deque, weakref.WeakSet,
and datetime.tzinfo now copies and pickles instance attributes implemented as
slots.
