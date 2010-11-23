:mod:`textwrap` --- Text wrapping and filling
=============================================

.. module:: textwrap
   :synopsis: Text wrapping and filling
.. moduleauthor:: Greg Ward <gward@python.net>
.. sectionauthor:: Greg Ward <gward@python.net>


The :mod:`textwrap` module provides two convenience functions, :func:`wrap` and
:func:`fill`, as well as :class:`TextWrapper`, the class that does all the work,
and a utility function  :func:`dedent`.  If you're just wrapping or filling one
or two  text strings, the convenience functions should be good enough;
otherwise,  you should use an instance of :class:`TextWrapper` for efficiency.


.. function:: wrap(text, width=70, **kwargs)

   Wraps the single paragraph in *text* (a string) so every line is at most
   *width* characters long.  Returns a list of output lines, without final
   newlines.

   Optional keyword arguments correspond to the instance attributes of
   :class:`TextWrapper`, documented below.  *width* defaults to ``70``.


.. function:: fill(text, width=70, **kwargs)

   Wraps the single paragraph in *text*, and returns a single string containing the
   wrapped paragraph.  :func:`fill` is shorthand for  ::

      "\n".join(wrap(text, ...))

   In particular, :func:`fill` accepts exactly the same keyword arguments as
   :func:`wrap`.

Both :func:`wrap` and :func:`fill` work by creating a :class:`TextWrapper`
instance and calling a single method on it.  That instance is not reused, so for
applications that wrap/fill many text strings, it will be more efficient for you
to create your own :class:`TextWrapper` object.

Text is preferably wrapped on whitespaces and right after the hyphens in
hyphenated words; only then will long words be broken if necessary, unless
:attr:`TextWrapper.break_long_words` is set to false.

An additional utility function, :func:`dedent`, is provided to remove
indentation from strings that have unwanted whitespace to the left of the text.


.. function:: dedent(text)

   Remove any common leading whitespace from every line in *text*.

   This can be used to make triple-quoted strings line up with the left edge of the
   display, while still presenting them in the source code in indented form.

   Note that tabs and spaces are both treated as whitespace, but they are not
   equal: the lines ``"  hello"`` and ``"\thello"`` are considered to have no
   common leading whitespace.

   For example::

      def test():
          # end first line with \ to avoid the empty line!
          s = '''\
          hello
            world
          '''
          print(repr(s))          # prints '    hello\n      world\n    '
          print(repr(dedent(s)))  # prints 'hello\n  world\n'


.. class:: TextWrapper(**kwargs)

   The :class:`TextWrapper` constructor accepts a number of optional keyword
   arguments.  Each keyword argument corresponds to an instance attribute, so
   for example ::

      wrapper = TextWrapper(initial_indent="* ")

   is the same as  ::

      wrapper = TextWrapper()
      wrapper.initial_indent = "* "

   You can re-use the same :class:`TextWrapper` object many times, and you can
   change any of its options through direct assignment to instance attributes
   between uses.

   The :class:`TextWrapper` instance attributes (and keyword arguments to the
   constructor) are as follows:


   .. attribute:: width

      (default: ``70``) The maximum length of wrapped lines.  As long as there
      are no individual words in the input text longer than :attr:`width`,
      :class:`TextWrapper` guarantees that no output line will be longer than
      :attr:`width` characters.


   .. attribute:: expand_tabs

      (default: ``True``) If true, then all tab characters in *text* will be
      expanded to spaces using the :meth:`expandtabs` method of *text*.


   .. attribute:: replace_whitespace

      (default: ``True``) If true, each whitespace character (as defined by
      ``string.whitespace``) remaining after tab expansion will be replaced by a
      single space.

      .. note::

         If :attr:`expand_tabs` is false and :attr:`replace_whitespace` is true,
         each tab character will be replaced by a single space, which is *not*
         the same as tab expansion.

      .. note::

         If :attr:`replace_whitespace` is false, newlines may appear in the
         middle of a line and cause strange output. For this reason, text should
         be split into paragraphs (using :meth:`str.splitlines` or similar)
         which are wrapped separately.


   .. attribute:: drop_whitespace

      (default: ``True``) If true, whitespace that, after wrapping, happens to
      end up at the beginning or end of a line is dropped (leading whitespace in
      the first line is always preserved, though).


   .. attribute:: initial_indent

      (default: ``''``) String that will be prepended to the first line of
      wrapped output.  Counts towards the length of the first line.


   .. attribute:: subsequent_indent

      (default: ``''``) String that will be prepended to all lines of wrapped
      output except the first.  Counts towards the length of each line except
      the first.


   .. attribute:: fix_sentence_endings

      (default: ``False``) If true, :class:`TextWrapper` attempts to detect
      sentence endings and ensure that sentences are always separated by exactly
      two spaces.  This is generally desired for text in a monospaced font.
      However, the sentence detection algorithm is imperfect: it assumes that a
      sentence ending consists of a lowercase letter followed by one of ``'.'``,
      ``'!'``, or ``'?'``, possibly followed by one of ``'"'`` or ``"'"``,
      followed by a space.  One problem with this is algorithm is that it is
      unable to detect the difference between "Dr." in ::

         [...] Dr. Frankenstein's monster [...]

      and "Spot." in ::

         [...] See Spot. See Spot run [...]

      :attr:`fix_sentence_endings` is false by default.

      Since the sentence detection algorithm relies on ``string.lowercase`` for
      the definition of "lowercase letter," and a convention of using two spaces
      after a period to separate sentences on the same line, it is specific to
      English-language texts.


   .. attribute:: break_long_words

      (default: ``True``) If true, then words longer than :attr:`width` will be
      broken in order to ensure that no lines are longer than :attr:`width`.  If
      it is false, long words will not be broken, and some lines may be longer
      than :attr:`width`.  (Long words will be put on a line by themselves, in
      order to minimize the amount by which :attr:`width` is exceeded.)


   .. attribute:: break_on_hyphens

      (default: ``True``) If true, wrapping will occur preferably on whitespaces
      and right after hyphens in compound words, as it is customary in English.
      If false, only whitespaces will be considered as potentially good places
      for line breaks, but you need to set :attr:`break_long_words` to false if
      you want truly insecable words.  Default behaviour in previous versions
      was to always allow breaking hyphenated words.


   :class:`TextWrapper` also provides two public methods, analogous to the
   module-level convenience functions:

   .. method:: wrap(text)

      Wraps the single paragraph in *text* (a string) so every line is at most
      :attr:`width` characters long.  All wrapping options are taken from
      instance attributes of the :class:`TextWrapper` instance. Returns a list
      of output lines, without final newlines.


   .. method:: fill(text)

      Wraps the single paragraph in *text*, and returns a single string
      containing the wrapped paragraph.

