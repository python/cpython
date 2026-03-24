:mod:`!string.templatelib` --- Support for template string literals
===================================================================

.. module:: string.templatelib
   :synopsis: Support for template string literals.

**Source code:** :source:`Lib/string/templatelib.py`

--------------

.. seealso::

   * :ref:`Format strings <f-strings>`
   * :ref:`Template string literal (t-string) syntax <t-strings>`
   * :pep:`750`

.. _template-strings:

Template strings
----------------

.. versionadded:: 3.14

Template strings are a mechanism for custom string processing.
They have the full flexibility of Python's :ref:`f-strings`,
but return a :class:`Template` instance that gives access
to the static and interpolated (in curly brackets) parts of a string
*before* they are combined.

To write a t-string, use a ``'t'`` prefix instead of an ``'f'``, like so:

.. code-block:: pycon

   >>> pi = 3.14
   >>> t't-strings are new in Python {pi!s}!'
   Template(
      strings=('t-strings are new in Python ', '!'),
      interpolations=(Interpolation(3.14, 'pi', 's', ''),)
   )

Types
-----

.. class:: Template

   The :class:`!Template` class describes the contents of a template string.
   It is immutable, meaning that attributes of a template cannot be reassigned.

   The most common way to create a :class:`!Template` instance is to use the
   :ref:`template string literal syntax <t-strings>`.
   This syntax is identical to that of :ref:`f-strings <f-strings>`,
   except that it uses a ``t`` prefix in place of an ``f``:

   >>> cheese = 'Red Leicester'
   >>> template = t"We're fresh out of {cheese}, sir."
   >>> type(template)
   <class 'string.templatelib.Template'>

   Templates are stored as sequences of literal :attr:`~Template.strings`
   and dynamic :attr:`~Template.interpolations`.
   A :attr:`~Template.values` attribute holds the values of the interpolations:

   >>> cheese = 'Camembert'
   >>> template = t'Ah! We do have {cheese}.'
   >>> template.strings
   ('Ah! We do have ', '.')
   >>> template.interpolations
   (Interpolation('Camembert', ...),)
   >>> template.values
   ('Camembert',)

   The :attr:`!strings` tuple has one more element than :attr:`!interpolations`
   and :attr:`!values`; the interpolations “belong” between the strings.
   This may be easier to understand when tuples are aligned

   .. code-block:: python

      template.strings:  ('Ah! We do have ',              '.')
      template.values:   (                   'Camembert',    )

   .. rubric:: Attributes

   .. attribute:: strings
      :type: tuple[str, ...]

      A :class:`tuple` of the static strings in the template.

      >>> cheese = 'Camembert'
      >>> template = t'Ah! We do have {cheese}.'
      >>> template.strings
      ('Ah! We do have ', '.')

      Empty strings *are* included in the tuple:

      >>> response = 'We do have '
      >>> cheese = 'Camembert'
      >>> template = t'Ah! {response}{cheese}.'
      >>> template.strings
      ('Ah! ', '', '.')

      The ``strings`` tuple is never empty, and always contains one more
      string than the ``interpolations`` and ``values`` tuples:

      >>> t''.strings
      ('',)
      >>> t''.values
      ()
      >>> t'{'cheese'}'.strings
      ('', '')
      >>> t'{'cheese'}'.values
      ('cheese',)

   .. attribute:: interpolations
      :type: tuple[Interpolation, ...]

      A :class:`tuple` of the interpolations in the template.

      >>> cheese = 'Camembert'
      >>> template = t'Ah! We do have {cheese}.'
      >>> template.interpolations
      (Interpolation('Camembert', 'cheese', None, ''),)

      The ``interpolations`` tuple may be empty and always contains one fewer
      values than the ``strings`` tuple:

      >>> t'Red Leicester'.interpolations
      ()

   .. attribute:: values
      :type: tuple[object, ...]

      A tuple of all interpolated values in the template.

      >>> cheese = 'Camembert'
      >>> template = t'Ah! We do have {cheese}.'
      >>> template.values
      ('Camembert',)

      The ``values`` tuple always has the same length as the
      ``interpolations`` tuple. It is always equivalent to
      ``tuple(i.value for i in template.interpolations)``.

   .. rubric:: Methods

   .. method:: __new__(*args: str | Interpolation)

      While literal syntax is the most common way to create a :class:`!Template`,
      it is also possible to create them directly using the constructor:

      >>> from string.templatelib import Interpolation, Template
      >>> cheese = 'Camembert'
      >>> template = Template(
      ...     'Ah! We do have ', Interpolation(cheese, 'cheese'), '.'
      ... )
      >>> list(template)
      ['Ah! We do have ', Interpolation('Camembert', 'cheese', None, ''), '.']

      If multiple strings are passed consecutively, they will be concatenated
      into a single value in the :attr:`~Template.strings` attribute. For example,
      the following code creates a :class:`Template` with a single final string:

      >>> from string.templatelib import Template
      >>> template = Template('Ah! We do have ', 'Camembert', '.')
      >>> template.strings
      ('Ah! We do have Camembert.',)

      If multiple interpolations are passed consecutively, they will be treated
      as separate interpolations and an empty string will be inserted between them.
      For example, the following code creates a template with empty placeholders
      in the :attr:`~Template.strings` attribute:

      >>> from string.templatelib import Interpolation, Template
      >>> template = Template(
      ...     Interpolation('Camembert', 'cheese'),
      ...     Interpolation('.', 'punctuation'),
      ... )
      >>> template.strings
      ('', '', '')

   .. describe:: iter(template)

      Iterate over the template, yielding each non-empty string and
      :class:`Interpolation` in the correct order:

      >>> cheese = 'Camembert'
      >>> list(t'Ah! We do have {cheese}.')
      ['Ah! We do have ', Interpolation('Camembert', 'cheese', None, ''), '.']

      .. caution::

         Empty strings are **not** included in the iteration:

         >>> response = 'We do have '
         >>> cheese = 'Camembert'
         >>> list(t'Ah! {response}{cheese}.')  # doctest: +NORMALIZE_WHITESPACE
         ['Ah! ',
          Interpolation('We do have ', 'response', None, ''),
          Interpolation('Camembert', 'cheese', None, ''),
          '.']

   .. describe:: template + other
                 template += other

      Concatenate this template with another, returning a new
      :class:`!Template` instance:

      >>> cheese = 'Camembert'
      >>> list(t'Ah! ' + t'We do have {cheese}.')
      ['Ah! We do have ', Interpolation('Camembert', 'cheese', None, ''), '.']

      Concatenating a :class:`!Template` and a ``str`` is **not** supported.
      This is because it is unclear whether the string should be treated as
      a static string or an interpolation.
      If you want to concatenate a :class:`!Template` with a string,
      you should either wrap the string directly in a :class:`!Template`
      (to treat it as a static string)
      or use an :class:`!Interpolation` (to treat it as dynamic):

      >>> from string.templatelib import Interpolation, Template
      >>> template = t'Ah! '
      >>> # Treat 'We do have ' as a static string
      >>> template += Template('We do have ')
      >>> # Treat cheese as an interpolation
      >>> cheese = 'Camembert'
      >>> template += Template(Interpolation(cheese, 'cheese'))
      >>> list(template)
      ['Ah! We do have ', Interpolation('Camembert', 'cheese', None, '')]


.. class:: Interpolation

   The :class:`!Interpolation` type represents an expression inside a template string.
   It is immutable, meaning that attributes of an interpolation cannot be reassigned.

   Interpolations support pattern matching, allowing you to match against
   their attributes with the :ref:`match statement <match>`:

   >>> from string.templatelib import Interpolation
   >>> interpolation = t'{1. + 2.:.2f}'.interpolations[0]
   >>> interpolation
   Interpolation(3.0, '1. + 2.', None, '.2f')
   >>> match interpolation:
   ...     case Interpolation(value, expression, conversion, format_spec):
   ...         print(value, expression, conversion, format_spec, sep=' | ')
   ...
   3.0 | 1. + 2. | None | .2f

   .. rubric:: Attributes

   .. attribute:: value
      :type: object

      The evaluated value of the interpolation.

      >>> t'{1 + 2}'.interpolations[0].value
      3

   .. attribute:: expression
      :type: str

      For interpolations created by t-string literals, :attr:`!expression`
      is the expression text found inside the curly brackets (``{`` & ``}``),
      including any whitespace, excluding the curly brackets themselves,
      and ending before the first ``!``, ``:``, or ``=`` if any is present.
      For manually created interpolations, :attr:`!expression` is the arbitrary
      string provided when constructing the interpolation instance.

      We recommend using valid Python expressions or the empty string for the
      ``expression`` field of manually created :class:`!Interpolation`
      instances, although this is not enforced at runtime.

      >>> t'{1 + 2}'.interpolations[0].expression
      '1 + 2'

   .. attribute:: conversion
      :type: typing.Literal['a', 'r', 's'] | None

      The conversion to apply to the value, or ``None``.

      The :attr:`!conversion` is the optional conversion to apply
      to the value:

      >>> t'{1 + 2!a}'.interpolations[0].conversion
      'a'

      .. note::

         Unlike f-strings, where conversions are applied automatically,
         the expected behavior with t-strings is that code that *processes* the
         :class:`!Template` will decide how to interpret and whether to apply
         the :attr:`!conversion`.
         For convenience, the :func:`convert` function can be used to mimic
         f-string conversion semantics.

   .. attribute:: format_spec
      :type: str

      The format specification to apply to the value.

      The :attr:`!format_spec` is an optional, arbitrary string
      used as the format specification to present the value:

      >>> t'{1 + 2:.2f}'.interpolations[0].format_spec
      '.2f'

      .. note::

         Unlike f-strings, where format specifications are applied automatically
         via the :func:`format` protocol, the expected behavior with
         t-strings is that code that *processes* the interpolation will
         decide how to interpret and whether to apply the format specification.
         As a result, :attr:`!format_spec` values in interpolations
         can be arbitrary strings,
         including those that do not conform to the :func:`format` protocol.

   .. rubric:: Methods

   .. method:: __new__(value: object, \
                       expression: str, \
                       conversion: typing.Literal['a', 'r', 's'] | None = None, \
                       format_spec: str = '')

      Create a new :class:`!Interpolation` object from component parts.

      :param value: The evaluated, in-scope result of the interpolation.
      :param expression: The text of a valid Python expression,
           or an empty string.
      :param conversion: The :ref:`conversion <formatstrings>` to be used,
           one of ``None``, ``'a'``, ``'r'``, or ``'s'``.
      :param format_spec: An optional, arbitrary string used as the
           :ref:`format specification <formatspec>` to present the value.


Helper functions
----------------

.. function:: convert(obj, /, conversion)

   Applies formatted string literal :ref:`conversion <formatstrings-conversion>`
   semantics to the given object *obj*.
   This is frequently useful for custom template string processing logic.

   Three conversion flags are currently supported:

   * ``'s'`` which calls :func:`str` on the value (like ``!s``),
   * ``'r'`` which calls :func:`repr` (like ``!r``), and
   * ``'a'`` which calls :func:`ascii` (like ``!a``).

   If the conversion flag is ``None``, *obj* is returned unchanged.
