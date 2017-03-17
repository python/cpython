A new version of typing.py provides several new classes and
features: @overload outside stubs, Reversible, DefaultDict, Text,
ContextManager, Type[], NewType(), TYPE_CHECKING, and numerous bug
fixes (note that some of the new features are not yet implemented in
mypy or other static analyzers).  Also classes for PEP 492
(Awaitable, AsyncIterable, AsyncIterator) have been added (in fact
they made it into 3.5.1 but were never mentioned).