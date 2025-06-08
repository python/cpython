:mod:`!string.templatelib` --- Templates and Interpolations for t-strings
=========================================================================

.. module:: string.templatelib
   :synopsis: Support for t-string literals.

**Source code:** :source:`Lib/string/templatelib.py`

--------------


.. seealso::

   :ref:`f-strings` -- Format strings (f-strings)


.. _templatelib-template:

Template
--------

The :class:`Template` class describes the contents of a template string.

The most common way to create a new :class:`Template` instance is to use the t-string literal syntax. This syntax is identical to that of :ref:`f-strings`, except that the string is prefixed with a ``t`` instead of an ``f``. For example, the following code creates a :class:`Template` that can be used to format strings:

   >>> name = "World"
   >>> greeting = t"Hello {name}!"
   >>> type(greeting)
   <class 'string.templatelib.Template'>
   >>> print(list(greeting))
   ['Hello ', Interpolation('World'), '!']

It is also possible to create a :class:`Template` directly, using its constructor. This takes an arbitrary collection of strings and :class:`Interpolation` instances:

   >>> from string.templatelib import Interpolation, Template
   >>> name = "World"
   >>> greeting = Template("Hello, ", Interpolation(name, "name"), "!")
   >>> print(list(greeting))
   ['Hello, ', Interpolation('World'), '!']

.. class:: Template(*args)

   Create a new :class:`Template` object.

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

       A :ref:`tuple <tut-tuples>` of the static strings in the template.

       >>> name = "World"
       >>> print(t"Hello {name}!".strings)
       ('Hello ', '!')

       Empty strings *are* included in the tuple:

       >>> name = "World"
       >>> print(t"Hello {name}{name}!".strings)
       ('Hello ', '', '!')

   .. attribute:: interpolations: tuple[Interpolation, ...]

       A tuple of the interpolations in the template.

       >>> name = "World"
       >>> print(t"Hello {name}!".interpolations)
       (Interpolation('World'),)


   .. attribute:: values: tuple[Any, ...]

       A tuple of all interpolated values in the template.

       >>> name = "World"
       >>> print(t"Hello {name}!".values)
       ('World',)

   .. method:: __iter__() -> typing.Iterator[str | Interpolation]

       Iterate over the template, yielding each string and :class:`Interpolation` in order.

       >>> name = "World"
       >>> print(list(t"Hello {name}!"))
       ['Hello ', Interpolation('World'), '!']

       Empty strings are *not* included in the iteration:

       >>> name = "World"
       >>> print(list(t"Hello {name}{name}"))
       ['Hello ', Interpolation('World'), Interpolation('World')]






