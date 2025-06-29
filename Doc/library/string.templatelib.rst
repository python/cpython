:mod:`!string.templatelib` --- Templates and Interpolations for t-strings
=========================================================================

.. module:: string.templatelib
   :synopsis: Support for t-string literals.

**Source code:** :source:`Lib/string/templatelib.py`

--------------


.. seealso::

   :ref:`T-strings tutorial <tut-t-strings>`
   :ref:`Format strings <f-strings>`
   :ref:`T-string literal syntax <t-strings>`


.. _template-strings:

Template strings
----------------

.. versionadded:: 3.14

Template strings are an extension of :ref:`f-strings <f-strings>`
that allow for greater control of formatting behavior. The :class:`Template`
class gives you access to the static and interpolated (in curly braces)
parts of a string *before* they are combined into a final string.

See the :ref:`t-strings tutorial <tut-t-strings>` for an introduction.


.. _templatelib-template:

Template
--------

The :class:`!Template` class describes the contents of a template string.

:class:`!Template` instances are shallow immutable: their attributes cannot be
reassigned.

.. class:: Template(*args)

   Create a new :class:`!Template` object.

   :param args: A mix of strings and :class:`Interpolation` instances in any order.
   :type args: str | Interpolation

   The most common way to create a :class:`!Template` instance is to use the
   :ref:`t-string literal syntax <t-strings>`. This syntax is identical to that of
   :ref:`f-strings` except that it uses a ``t`` instead of an ``f``:

   >>> name = "World"
   >>> template = t"Hello {name}!"
   >>> type(template)
   <class 'string.templatelib.Template'>
   >>> template.strings
   ('Hello ', '!')
   >>> template.values
   ('World',)

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
   For example, the following code creates a template with a single value in
   the :attr:`~Template.strings` attribute:

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

       The ``values`` tuple is always the same length as the
       ``interpolations`` tuple. It is equivalent to
       ``tuple(i.value for i in template.interpolations)``.

   .. method:: __iter__()

       Iterate over the template, yielding each string and
       :class:`Interpolation` in order.

       >>> name = "World"
       >>> list(t"Hello {name}!")
       ['Hello ', Interpolation('World', 'name', None, ''), '!']

       Empty strings are *not* included in the iteration:

       >>> name = "World"
       >>> list(t"Hello {name}{name}")
       ['Hello ', Interpolation('World', 'name', None, ''), Interpolation('World', 'name', None, '')]

   .. method:: __add__(other)

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

   :param expression: The text of a valid Python expression, or an empty string
   :type expression: str

   :param conversion: The optional :ref:`conversion <formatstrings>` to be used, one of r, s, and a,.
   :type conversion: Literal["a", "r", "s"] | None

   :param format_spec: An optional, arbitrary string used as the :ref:`format specification <formatspec>` to present the value.
   :type format_spec: str

   The :class:`!Interpolation` type represents an expression inside a template string.

   :class:`!Interpolation` instances are shallow immutable: their attributes cannot be
   reassigned.

   .. property:: value

       :returns: The evaluated value of the interpolation.
       :rtype: object

       >>> t"{1 + 2}".interpolations[0].value
       3

   .. property:: expression

       :returns: The text of a valid Python expression, or an empty string.
       :rtype: str

       The :attr:`~Interpolation.expression` is the original text of the
       interpolation's Python expression, if the interpolation was created
       from a t-string literal. Developers creating interpolations manually
       should either set this to an empty string or choose a suitable valid
       Python expression.

       >>> t"{1 + 2}".interpolations[0].expression
       '1 + 2'

   .. property:: conversion

       :returns: The conversion to apply to the value, or ``None``
       :rtype: Literal["a", "r", "s"] | None

       The :attr:`!Interpolation.conversion` is the optional conversion to apply
       to the value:

       >>> t"{1 + 2!a}".interpolations[0].conversion
       'a'

       .. note::

         Unlike f-strings, where conversions are applied automatically,
         the expected behavior with t-strings is that code that *processes* the
         :class:`!Template` will decide how to interpret and whether to apply
         the :attr:`!Interpolation.conversion`.

   .. property:: format_spec

       :returns: The format specification to apply to the value.
       :rtype: str

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

   .. property:: __match_args__

       :returns: A tuple of the attributes to use for structural pattern matching.

       The tuple returned is ``('value', 'expression', 'conversion', 'format_spec')``.
       This allows for :ref:`pattern matching <match>` on :class:`!Interpolation` instances.

