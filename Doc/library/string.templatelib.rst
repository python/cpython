:mod:`!string.templatelib` --- Templates and Interpolations for t-strings
=========================================================================

.. module:: string.templatelib
   :synopsis: Support for t-string literals.

**Source code:** :source:`Lib/string/templatelib.py`

--------------


.. seealso::

   :ref:`Format strings <f-strings>`



.. _template-strings:

Template strings
----------------

Documentation forthcoming for PEP 750 template strings, also known as t-strings.

.. versionadded:: 3.14


.. _templatelib-template:

Template
--------

The :class:`!Template` class describes the contents of a template string.

The most common way to create a new :class:`!Template` instance is to use the t-string literal syntax. This syntax is identical to that of :ref:`f-strings`, except that the string is prefixed with a ``t`` instead of an ``f``. For example, the following code creates a :class:`Template` that can be used to format strings:

   >>> name = "World"
   >>> greeting = t"Hello {name}!"
   >>> type(greeting)
   <class 'string.templatelib.Template'>
   >>> print(list(greeting))
   ['Hello ', Interpolation('World', 'name', None, ''), '!']

It is also possible to create a :class:`!Template` directly, using its constructor. This takes an arbitrary collection of strings and :class:`Interpolation` instances:

   >>> from string.templatelib import Interpolation, Template
   >>> name = "World"
   >>> greeting = Template("Hello, ", Interpolation(name, "name"), "!")
   >>> print(list(greeting))
   ['Hello, ', Interpolation('World', 'name', None, ''), '!']

.. class:: Template(*args)

   Create a new :class:`!Template` object.

   :param args: A mix of strings and :class:`Interpolation` instances in any order.
   :type args: str | Interpolation

   If two or more consecutive strings are passed, they will be concatenated into a single value in the :attr:`~Template.strings` attribute. For example, the following code creates a :class:`Template` with a single final string:

   >>> from string.templatelib import Template
   >>> greeting = Template("Hello ", "World", "!")
   >>> print(greeting.strings)
   ('Hello World!',)

   If two or more consecutive interpolations are passed, they will be treated as separate interpolations and an empty string will be inserted between them. For example, the following code creates a template with a single value in the :attr:`~Template.strings` attribute:

   >>> from string.templatelib import Interpolation, Template
   >>> greeting = Template(Interpolation("World", "name"), Interpolation("!", "punctuation"))
   >>> print(greeting.strings)
   ('', '', '')

   .. attribute:: strings
       :type: tuple[str, ...]

       A :ref:`tuple <tut-tuples>` of the static strings in the template.

       >>> name = "World"
       >>> print(t"Hello {name}!".strings)
       ('Hello ', '!')

       Empty strings *are* included in the tuple:

       >>> name = "World"
       >>> print(t"Hello {name}{name}!".strings)
       ('Hello ', '', '!')

   .. attribute:: interpolations
       :type: tuple[Interpolation, ...]

       A tuple of the interpolations in the template.

       >>> name = "World"
       >>> print(t"Hello {name}!".interpolations)
       (Interpolation('World', 'name', None, ''),)


   .. attribute:: values
       :type: tuple[Any, ...]

       A tuple of all interpolated values in the template.

       >>> name = "World"
       >>> print(t"Hello {name}!".values)
       ('World',)

   .. method:: __iter__()

       Iterate over the template, yielding each string and :class:`Interpolation` in order.

       >>> name = "World"
       >>> print(list(t"Hello {name}!"))
       ['Hello ', Interpolation('World', 'name', None, ''), '!']

       Empty strings are *not* included in the iteration:

       >>> name = "World"
       >>> print(list(t"Hello {name}{name}"))
       ['Hello ', Interpolation('World', 'name', None, ''), Interpolation('World', 'name', None, '')]

       :returns: An iterable of all the parts in the template.
       :rtype: typing.Iterator[str | Interpolation]

.. class:: Interpolation(*args)

   Create a new :class:`!Interpolation` object.

   :param value: The evaluated, in-scope result of the interpolation.
   :type value: object

   :param expression: The original *text* of the interpolation's Python :ref:`expressions <expressions>`.
   :type expression: str

   :param conversion: The optional :ref:`conversion <formatstrings>` to be used, one of r, s, and a,.
   :type value: Literal["a", "r", "s"] | None

   :param format_spec: An optional, arbitrary string used as the :ref:`format specification <formatspec>` to present the value.

   The :class:`!Interpolation` type represents an expression inside a template string. It is shallow immutable -- its attributes cannot be reassigned.

   >>> name = "World"
   >>> template = t"Hello {name}"
   >>> template.interpolations[0].value
   'World'
   >>> template.interpolations[0].value = "Galaxy"
   Traceback (most recent call last):
     File "<input>", line 1, in <module>
   AttributeError: readonly attribute

   While f-strings and t-strings are largely similar in syntax and expectations, the :attr:`~Interpolation.conversion` and :attr:`~Interpolation.format_spec` behave differently. With f-strings, these are applied to the resulting value automatically. For example, in this ``format_spec``:

   >>> value = 42
   >>> f"Value: {value:.2f}"
   'Value: 42.00'

   With a t-string :class:`!Interpolation`, the template function is expected to apply this to the value:

   >>> value = 42
   >>> template = t"Value: {value:.2f}"
   >>> template.interpolations[0].value
   42

   .. property:: __match_args__

       :returns: A tuple of the attributes to use for structural pattern matching.
       :rtype: (Literal["value"], Literal["expression"], Literal["conversion"], Literal["format_spec"])


   .. property:: value

       :returns: The evaluated value of the interpolation.
       :rtype: object

   .. property:: expression

       :returns: The original text of the interpolation's Python expression if the interpolation was created from a t-string literal
       :rtype: str

       The :attr:`~Interpolation.expression` is the original text of the interpolation's Python expression, if the interpolation was created from a t-string literal. Developers creating
       interpolations manually should either set this to an empty
       string or choose a suitable valid python expression.

   .. property:: conversion

      :returns: The conversion to apply to the value, one of "a", "r", or "s", or None.
      :rtype: Literal["a", "r", "s"] | None

      The :attr:`~Interpolation.conversion` is the optional conversion to apply to the value. This is one of "a", "r", or "s", or None if no conversion is specified.

   .. property:: format_spec

      :returns: The format specification to apply to the value.
      :rtype: str

      The :attr:`~Interpolation.format_spec` is an optional, arbitrary string used as the format specification to present the value. This is similar to the format specification used in :ref:`format strings <formatstrings>`, but it is not limited to a specific set of formats.
