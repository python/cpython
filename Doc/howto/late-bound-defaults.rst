.. _late-bound-defaults-howto:

Experimental late-bound argument defaults
========================================

.. warning::

   This branch implements draft :pep:`671` for experimentation. The PEP has
   not been accepted, and this syntax is not available in released Python.

A default written with ``=`` is evaluated when a function is defined. A default
written with ``=>`` is evaluated in the function's local scope when the argument
is omitted::

    def add_item(item, target=>[]):
        target.append(item)
        return target

    assert add_item(1) == [1]
    assert add_item(2) == [2]

An explicitly supplied argument, including ``None`` or ``Ellipsis``, suppresses
evaluation of its default. Late-bound defaults are evaluated from left to right
and can refer to earlier arguments::

    def window(items, start=0, stop=>len(items)):
        return items[start:stop]

Positional-only and keyword-only parameters, annotated parameters, methods,
generic functions, and lambdas support the same syntax. All supplied arguments
and early-bound defaults are bound before late-bound defaults are evaluated.
An unbound reference raises :exc:`UnboundLocalError`::

    def invalid(value=>value):
        return value

    # invalid() raises UnboundLocalError; invalid(3) returns 3.

Coroutine and generator prologues run when execution starts, as does their
function body. An argument error is reported before any default is evaluated.

Introspection
-------------

``function.__defaults__`` and ``function.__kwdefaults__`` contain ``Ellipsis``
placeholders for late-bound parameters. ``function.__defaults_extra__`` holds a
parallel tuple of normalized expression strings (or ``None`` for early-bound
entries), and ``function.__kwdefaults_extra__`` holds expression strings keyed
by keyword-only parameter name. Both metadata attributes are ``None`` when no
corresponding late-bound defaults exist. These attributes can be replaced or
deleted using the same tuple/dict restrictions as the ordinary defaults.

``inspect.signature()`` displays the ``=>`` expression. Its parameters expose
the source string through ``Parameter.late_default``. ``getfullargspec()`` keeps
its existing seven-field result; its defaults contain the same placeholders as
the function attributes. Signature binding does not execute expressions.

``ast.arg.late_default`` is an optional string containing the normalized source
expression. The actual expression remains in ``arguments.defaults`` or
``arguments.kw_defaults``. This preserves the existing defaults fields and
allows :func:`ast.unparse` to reconstruct the new syntax.

Implementation notes
--------------------

The call binder leaves omitted late-bound arguments unbound. Function prologues
use ``QUERY_FAST`` and ``QUERY_DEREF`` to determine which expressions to evaluate.
``QUERY_DEREF`` supports parameters captured by closures. Expression descriptions
are captured before AST constant folding. Code objects using late-bound defaults
carry the experimental ``CO_LATE_DEFAULTS`` flag.

Existing call specializations are disabled for these functions because they
assume all arguments are bound before entering a frame. The compiler also treats
all parameters as potentially unbound during local-variable analysis. More
precise analysis and specialized call paths are future performance work.

Two metadata pointers are added to each function object. This prototype changes
the internal function layout and bytecode magic number; extensions must be
rebuilt for it. It does not add public C API entry points.
