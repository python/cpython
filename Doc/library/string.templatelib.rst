:mod:`!string.templatelib` --- Support for template string literals
===================================================================

.. module:: string.templatelib
   :synopsis: Support for template string literals.

**Source code:** :source:`Lib/string/templatelib.py`

--------------

.. seealso::

   * :ref:`Format strings <f-strings>`
   * :ref:`T-string literal syntax <t-strings>`


.. _template-strings:

Template strings
----------------

.. versionadded:: 3.14

Template strings are a formatting mechanism that allows for deep control over
how strings are processed. You can create templates using
:ref:`t-string literal syntax <t-strings>`, which is identical to
:ref:`f-string syntax <f-strings>` but uses a ``t`` instead of an ``f``.
While f-strings evaluate to ``str``, t-strings create a :class:`Template`
instance that gives you access to the static and interpolated (in curly braces)
parts of a string *before* they are combined.


.. _templatelib-template:

Template
--------

The :class:`!Template` class describes the contents of a template string.

:class:`!Template` instances are immutable: their attributes cannot be
reassigned.

.. class:: Template(*args)

   Create a new :class:`!Template` object.

   :param args: A mix of strings and :class:`Interpolation` instances in any order.
   :type args: str | Interpolation

   The most common way to create a :class:`!Template` instance is to use the
   :ref:`t-string literal syntax <t-strings>`. This syntax is identical to that of
   :ref:`f-strings <f-strings>` except that it uses a ``t`` instead of an ``f``:

   >>> name = "World"
   >>> template = t"Hello {name}!"
   >>> type(template)
   <class 'string.templatelib.Template'>

   Templates ars stored as sequences of literal :attr:`~Template.strings`
   and dynamic :attr:`~Template.interpolations`.
   A :attr:`~Template.values` attribute holds the interpolation values:

   >>> template.strings
   ('Hello ', '!')
   >>> template.interpolations
   (Interpolation('World', ...),)
   >>> template.values
   ('World',)

   The :attr:`!strings` tuple has one more element than :attr:`!interpolations`
   and :attr:`!values`; the interpolations “belong” between the strings.
   This may be easier to understand when tuples are aligned::

      template.strings:  ('Hello ',          '!')
      template.values:   (          'World',    )

   While literal syntax is the most common way to create :class:`!Template`
   instances, it is also possible to create them directly using the constructor:

   >>> from string.templatelib import Interpolation, Template
   >>> name = "World"
   >>> template = Template("Hello, ", Interpolation(name, "name"), "!")
   >>> list(template)
   ['Hello, ', Interpolation('World', 'name', None, ''), '!']

   If two or more consecutive strings are passed, they will be concatenated
   into a single value in the :attr:`~Template.strings` attribute. For example,
   the following code creates a :class:`Template` with a single final string:

   >>> from string.templatelib import Template
   >>> template = Template("Hello ", "World", "!")
   >>> template.strings
   ('Hello World!',)

   If two or more consecutive interpolations are passed, they will be treated
   as separate interpolations and an empty string will be inserted between them.
   For example, the following code creates a template with empty placeholders
   in the :attr:`~Template.strings` attribute:

   >>> from string.templatelib import Interpolation, Template
   >>> template = Template(Interpolation("World", "name"), Interpolation("!", "punctuation"))
   >>> template.strings
   ('', '', '')

   .. attribute:: strings
       :type: tuple[str, ...]

       A :ref:`tuple <tut-tuples>` of the static strings in the template.

       >>> name = "World"
       >>> t"Hello {name}!".strings
       ('Hello ', '!')

       Empty strings *are* included in the tuple:

       >>> name = "World"
       >>> t"Hello {name}{name}!".strings
       ('Hello ', '', '!')

       The ``strings`` tuple is never empty, and always contains one more
       string than the ``interpolations`` and ``values`` tuples:

       >>> t"".strings
       ('',)
       >>> t"".values
       ()
       >>> t"{'cheese'}".strings
       ('', '')
       >>> t"{'cheese'}".values
       ('cheese',)

   .. attribute:: interpolations
       :type: tuple[Interpolation, ...]

       A tuple of the interpolations in the template.

       >>> name = "World"
       >>> t"Hello {name}!".interpolations
       (Interpolation('World', 'name', None, ''),)

       The ``interpolations`` tuple may be empty and always contains one fewer
       values than the ``strings`` tuple:

       >>> t"Hello!".interpolations
       ()

   .. attribute:: values
       :type: tuple[Any, ...]

       A tuple of all interpolated values in the template.

       >>> name = "World"
       >>> t"Hello {name}!".values
       ('World',)

       The ``values`` tuple always has the same length as the
       ``interpolations`` tuple. It is equivalent to
       ``tuple(i.value for i in template.interpolations)``.

   .. describe:: iter(template)

       Iterate over the template, yielding each string and
       :class:`Interpolation` in order.

       >>> name = "World"
       >>> list(t"Hello {name}!")
       ['Hello ', Interpolation('World', 'name', None, ''), '!']

       Empty strings are *not* included in the iteration:

       >>> name = "World"
       >>> list(t"Hello {name}{name}")
       ['Hello ', Interpolation('World', 'name', None, ''), Interpolation('World', 'name', None, '')]

   .. describe:: template + other
                 template += other

       Concatenate this template with another, returning a new
       :class:`!Template` instance:

       >>> name = "World"
       >>> list(t"Hello " + t"there {name}!")
       ['Hello there ', Interpolation('World', 'name', None, ''), '!']

       Concatenation between a :class:`!Template` and a ``str`` is *not* supported.
       This is because it is ambiguous whether the string should be treated as
       a static string or an interpolation. If you want to concatenate a
       :class:`!Template` with a string, you should either wrap the string
       directly in a :class:`!Template` (to treat it as a static string) or use
       an :class:`!Interpolation` (to treat it as dynamic):

       >>> from string.templatelib import Template, Interpolation
       >>> template = t"Hello "
       >>> # Treat "there " as a static string
       >>> template += Template("there ")
       >>> # Treat name as an interpolation
       >>> name = "World"
       >>> template += Template(Interpolation(name, "name"))
       >>> list(template)
       ['Hello there ', Interpolation('World', 'name', None, '')]


.. class:: Interpolation(value, expression="", conversion=None, format_spec="")

   Create a new :class:`!Interpolation` object.

   :param value: The evaluated, in-scope result of the interpolation.
   :type value: object

   :param expression: The text of a valid Python expression, or an empty string.
   :type expression: str

   :param conversion: The optional :ref:`conversion <formatstrings>` to be used, one of r, s, and a.
   :type conversion: ``Literal["a", "r", "s"] | None``

   :param format_spec: An optional, arbitrary string used as the :ref:`format specification <formatspec>` to present the value.
   :type format_spec: str

   The :class:`!Interpolation` type represents an expression inside a template string.

   :class:`!Interpolation` instances are immutable: their attributes cannot be
   reassigned.

   .. attribute:: value

       :returns: The evaluated value of the interpolation.
       :type: object

       >>> t"{1 + 2}".interpolations[0].value
       3

   .. attribute:: expression

       :returns: The text of a valid Python expression, or an empty string.
       :type: str

       The :attr:`~Interpolation.expression` is the original text of the
       interpolation's Python expression, if the interpolation was created
       from a t-string literal. Developers creating interpolations manually
       should either set this to an empty string or choose a suitable valid
       Python expression.

       >>> t"{1 + 2}".interpolations[0].expression
       '1 + 2'

   .. attribute:: conversion

       :returns: The conversion to apply to the value, or ``None``.
       :type: ``Literal["a", "r", "s"] | None``

       The :attr:`!Interpolation.conversion` is the optional conversion to apply
       to the value:

       >>> t"{1 + 2!a}".interpolations[0].conversion
       'a'

       .. note::

         Unlike f-strings, where conversions are applied automatically,
         the expected behavior with t-strings is that code that *processes* the
         :class:`!Template` will decide how to interpret and whether to apply
         the :attr:`!Interpolation.conversion`.

   .. attribute:: format_spec

       :returns: The format specification to apply to the value.
       :type: str

       The :attr:`!Interpolation.format_spec` is an optional, arbitrary string
       used as the format specification to present the value:

       >>> t"{1 + 2:.2f}".interpolations[0].format_spec
       '.2f'

       .. note::

         Unlike f-strings, where format specifications are applied automatically
         via the :func:`format` protocol, the expected behavior with
         t-strings is that code that *processes* the :class:`!Template` will
         decide how to interpret and whether to apply the format specification.
         As a result, :attr:`!Interpolation.format_spec` values in
         :class:`!Template` instances can be arbitrary strings, even those that
         do not necessarily conform to the rules of Python's :func:`format`
         protocol.

   Interpolations support pattern matching, allowing you to match against
   their attributes with the :ref:`match statement <match>`:

   >>> from string.templatelib import Interpolation
   >>> interpolation = Interpolation(3.0, "1 + 2", None, ".2f")
   >>> match interpolation:
   ...     case Interpolation(value, expression, conversion, format_spec):
   ...         print(value, expression, conversion, format_spec)
   ...
   3.0 1 + 2 None .2f


Helper functions
----------------

.. function:: convert(obj, /, conversion)

   Applies formatted string literal :ref:`conversion <formatstrings-conversion>`
   semantics to the given object *obj*.
   This is frequently useful for custom template string processing logic.

   Three conversion flags are currently supported:

   * ``'s'`` which calls :func:`str` on the value,
   * ``'r'`` which calls :func:`repr`, and
   * ``'a'`` which calls :func:`ascii`.

   If the conversion flag is ``None``, *obj* is returned unchanged.
