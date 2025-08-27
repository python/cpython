.. _argparse-tutorial:

*****************
Argparse Tutorial
*****************

:author: Tshepang Mbambo

.. currentmodule:: argparse

This tutorial is intended to be a gentle introduction to :mod:`argparse`, the
recommended command-line parsing module in the Python standard library.

.. note::

   The standard library includes two other libraries directly related
   to command-line parameter processing: the lower level :mod:`optparse`
   module (which may require more code to configure for a given application,
   but also allows an application to request behaviors that ``argparse``
   doesn't support), and the very low level :mod:`getopt` (which specifically
   serves as an equivalent to the :c:func:`!getopt` family of functions
   available to C programmers).
   While neither of those modules is covered directly in this guide, many of
   the core concepts in ``argparse`` first originated in ``optparse``, so
   some aspects of this tutorial will also be relevant to ``optparse`` users.


Concepts
========

Let's show the sort of functionality that we are going to explore in this
introductory tutorial by making use of the :command:`ls` command:

.. code-block:: shell-session

   $ ls
   cpython  devguide  prog.py  pypy  rm-unused-function.patch
   $ ls pypy
   ctypes_configure  demo  dotviewer  include  lib_pypy  lib-python ...
   $ ls -l
   total 20
   drwxr-xr-x 19 wena wena 4096 Feb 18 18:51 cpython
   drwxr-xr-x  4 wena wena 4096 Feb  8 12:04 devguide
   -rwxr-xr-x  1 wena wena  535 Feb 19 00:05 prog.py
   drwxr-xr-x 14 wena wena 4096 Feb  7 00:59 pypy
   -rw-r--r--  1 wena wena  741 Feb 18 01:01 rm-unused-function.patch
   $ ls --help
   Usage: ls [OPTION]... [FILE]...
   List information about the FILEs (the current directory by default).
   Sort entries alphabetically if none of -cftuvSUX nor --sort is specified.
   ...

A few concepts we can learn from the four commands:

* The :command:`ls` command is useful when run without any options at all. It defaults
  to displaying the contents of the current directory.

* If we want beyond what it provides by default, we tell it a bit more. In
  this case, we want it to display a different directory, ``pypy``.
  What we did is specify what is known as a positional argument. It's named so
  because the program should know what to do with the value, solely based on
  where it appears on the command line. This concept is more relevant
  to a command like :command:`cp`, whose most basic usage is ``cp SRC DEST``.
  The first position is *what you want copied,* and the second
  position is *where you want it copied to*.

* Now, say we want to change behaviour of the program. In our example,
  we display more info for each file instead of just showing the file names.
  The ``-l`` in that case is known as an optional argument.

* That's a snippet of the help text. It's very useful in that you can
  come across a program you have never used before, and can figure out
  how it works simply by reading its help text.


The basics
==========

Let us start with a very simple example which does (almost) nothing::

   import argparse
   parser = argparse.ArgumentParser()
   parser.parse_args()

Following is a result of running the code:

.. code-block:: shell-session

   $ python prog.py
   $ python prog.py --help
   usage: prog.py [-h]

   options:
     -h, --help  show this help message and exit
   $ python prog.py --verbose
   usage: prog.py [-h]
   prog.py: error: unrecognized arguments: --verbose
   $ python prog.py foo
   usage: prog.py [-h]
   prog.py: error: unrecognized arguments: foo

Here is what is happening:

* Running the script without any options results in nothing displayed to
  stdout. Not so useful.

* The second one starts to display the usefulness of the :mod:`argparse`
  module. We have done almost nothing, but already we get a nice help message.

* The ``--help`` option, which can also be shortened to ``-h``, is the only
  option we get for free (i.e. no need to specify it). Specifying anything
  else results in an error. But even then, we do get a useful usage message,
  also for free.


Introducing Positional arguments
================================

An example::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("echo")
   args = parser.parse_args()
   print(args.echo)

And running the code:

.. code-block:: shell-session

   $ python prog.py
   usage: prog.py [-h] echo
   prog.py: error: the following arguments are required: echo
   $ python prog.py --help
   usage: prog.py [-h] echo

   positional arguments:
     echo

   options:
     -h, --help  show this help message and exit
   $ python prog.py foo
   foo

Here is what's happening:

* We've added the :meth:`~ArgumentParser.add_argument` method, which is what we use to specify
  which command-line options the program is willing to accept. In this case,
  I've named it ``echo`` so that it's in line with its function.

* Calling our program now requires us to specify an option.

* The :meth:`~ArgumentParser.parse_args` method actually returns some data from the
  options specified, in this case, ``echo``.

* The variable is some form of 'magic' that :mod:`argparse` performs for free
  (i.e. no need to specify which variable that value is stored in).
  You will also notice that its name matches the string argument given
  to the method, ``echo``.

Note however that, although the help display looks nice and all, it currently
is not as helpful as it can be. For example we see that we got ``echo`` as a
positional argument, but we don't know what it does, other than by guessing or
by reading the source code. So, let's make it a bit more useful::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("echo", help="echo the string you use here")
   args = parser.parse_args()
   print(args.echo)

And we get:

.. code-block:: shell-session

   $ python prog.py -h
   usage: prog.py [-h] echo

   positional arguments:
     echo        echo the string you use here

   options:
     -h, --help  show this help message and exit

Now, how about doing something even more useful::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("square", help="display a square of a given number")
   args = parser.parse_args()
   print(args.square**2)

Following is a result of running the code:

.. code-block:: shell-session

   $ python prog.py 4
   Traceback (most recent call last):
     File "prog.py", line 5, in <module>
       print(args.square**2)
   TypeError: unsupported operand type(s) for ** or pow(): 'str' and 'int'

That didn't go so well. That's because :mod:`argparse` treats the options we
give it as strings, unless we tell it otherwise. So, let's tell
:mod:`argparse` to treat that input as an integer::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("square", help="display a square of a given number",
                       type=int)
   args = parser.parse_args()
   print(args.square**2)

Following is a result of running the code:

.. code-block:: shell-session

   $ python prog.py 4
   16
   $ python prog.py four
   usage: prog.py [-h] square
   prog.py: error: argument square: invalid int value: 'four'

That went well. The program now even helpfully quits on bad illegal input
before proceeding.


Introducing Optional arguments
==============================

So far we have been playing with positional arguments. Let us
have a look on how to add optional ones::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("--verbosity", help="increase output verbosity")
   args = parser.parse_args()
   if args.verbosity:
       print("verbosity turned on")

And the output:

.. code-block:: shell-session

   $ python prog.py --verbosity 1
   verbosity turned on
   $ python prog.py
   $ python prog.py --help
   usage: prog.py [-h] [--verbosity VERBOSITY]

   options:
     -h, --help            show this help message and exit
     --verbosity VERBOSITY
                           increase output verbosity
   $ python prog.py --verbosity
   usage: prog.py [-h] [--verbosity VERBOSITY]
   prog.py: error: argument --verbosity: expected one argument

Here is what is happening:

* The program is written so as to display something when ``--verbosity`` is
  specified and display nothing when not.

* To show that the option is actually optional, there is no error when running
  the program without it. Note that by default, if an optional argument isn't
  used, the relevant variable, in this case ``args.verbosity``, is
  given ``None`` as a value, which is the reason it fails the truth
  test of the :keyword:`if` statement.

* The help message is a bit different.

* When using the ``--verbosity`` option, one must also specify some value,
  any value.

The above example accepts arbitrary integer values for ``--verbosity``, but for
our simple program, only two values are actually useful, ``True`` or ``False``.
Let's modify the code accordingly::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("--verbose", help="increase output verbosity",
                       action="store_true")
   args = parser.parse_args()
   if args.verbose:
       print("verbosity turned on")

And the output:

.. code-block:: shell-session

   $ python prog.py --verbose
   verbosity turned on
   $ python prog.py --verbose 1
   usage: prog.py [-h] [--verbose]
   prog.py: error: unrecognized arguments: 1
   $ python prog.py --help
   usage: prog.py [-h] [--verbose]

   options:
     -h, --help  show this help message and exit
     --verbose   increase output verbosity

Here is what is happening:

* The option is now more of a flag than something that requires a value.
  We even changed the name of the option to match that idea.
  Note that we now specify a new keyword, ``action``, and give it the value
  ``"store_true"``. This means that, if the option is specified,
  assign the value ``True`` to ``args.verbose``.
  Not specifying it implies ``False``.

* It complains when you specify a value, in true spirit of what flags
  actually are.

* Notice the different help text.


Short options
-------------

If you are familiar with command line usage,
you will notice that I haven't yet touched on the topic of short
versions of the options. It's quite simple::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("-v", "--verbose", help="increase output verbosity",
                       action="store_true")
   args = parser.parse_args()
   if args.verbose:
       print("verbosity turned on")

And here goes:

.. code-block:: shell-session

   $ python prog.py -v
   verbosity turned on
   $ python prog.py --help
   usage: prog.py [-h] [-v]

   options:
     -h, --help     show this help message and exit
     -v, --verbose  increase output verbosity

Note that the new ability is also reflected in the help text.


Combining Positional and Optional arguments
===========================================

Our program keeps growing in complexity::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("square", type=int,
                       help="display a square of a given number")
   parser.add_argument("-v", "--verbose", action="store_true",
                       help="increase output verbosity")
   args = parser.parse_args()
   answer = args.square**2
   if args.verbose:
       print(f"the square of {args.square} equals {answer}")
   else:
       print(answer)

And now the output:

.. code-block:: shell-session

   $ python prog.py
   usage: prog.py [-h] [-v] square
   prog.py: error: the following arguments are required: square
   $ python prog.py 4
   16
   $ python prog.py 4 --verbose
   the square of 4 equals 16
   $ python prog.py --verbose 4
   the square of 4 equals 16

* We've brought back a positional argument, hence the complaint.

* Note that the order does not matter.

How about we give this program of ours back the ability to have
multiple verbosity values, and actually get to use them::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("square", type=int,
                       help="display a square of a given number")
   parser.add_argument("-v", "--verbosity", type=int,
                       help="increase output verbosity")
   args = parser.parse_args()
   answer = args.square**2
   if args.verbosity == 2:
       print(f"the square of {args.square} equals {answer}")
   elif args.verbosity == 1:
       print(f"{args.square}^2 == {answer}")
   else:
       print(answer)

And the output:

.. code-block:: shell-session

   $ python prog.py 4
   16
   $ python prog.py 4 -v
   usage: prog.py [-h] [-v VERBOSITY] square
   prog.py: error: argument -v/--verbosity: expected one argument
   $ python prog.py 4 -v 1
   4^2 == 16
   $ python prog.py 4 -v 2
   the square of 4 equals 16
   $ python prog.py 4 -v 3
   16

These all look good except the last one, which exposes a bug in our program.
Let's fix it by restricting the values the ``--verbosity`` option can accept::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("square", type=int,
                       help="display a square of a given number")
   parser.add_argument("-v", "--verbosity", type=int, choices=[0, 1, 2],
                       help="increase output verbosity")
   args = parser.parse_args()
   answer = args.square**2
   if args.verbosity == 2:
       print(f"the square of {args.square} equals {answer}")
   elif args.verbosity == 1:
       print(f"{args.square}^2 == {answer}")
   else:
       print(answer)

And the output:

.. code-block:: shell-session

   $ python prog.py 4 -v 3
   usage: prog.py [-h] [-v {0,1,2}] square
   prog.py: error: argument -v/--verbosity: invalid choice: 3 (choose from 0, 1, 2)
   $ python prog.py 4 -h
   usage: prog.py [-h] [-v {0,1,2}] square

   positional arguments:
     square                display a square of a given number

   options:
     -h, --help            show this help message and exit
     -v, --verbosity {0,1,2}
                           increase output verbosity

Note that the change also reflects both in the error message as well as the
help string.

Now, let's use a different approach of playing with verbosity, which is pretty
common. It also matches the way the CPython executable handles its own
verbosity argument (check the output of ``python --help``)::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("square", type=int,
                       help="display the square of a given number")
   parser.add_argument("-v", "--verbosity", action="count",
                       help="increase output verbosity")
   args = parser.parse_args()
   answer = args.square**2
   if args.verbosity == 2:
       print(f"the square of {args.square} equals {answer}")
   elif args.verbosity == 1:
       print(f"{args.square}^2 == {answer}")
   else:
       print(answer)

We have introduced another action, "count",
to count the number of occurrences of specific options.


.. code-block:: shell-session

   $ python prog.py 4
   16
   $ python prog.py 4 -v
   4^2 == 16
   $ python prog.py 4 -vv
   the square of 4 equals 16
   $ python prog.py 4 --verbosity --verbosity
   the square of 4 equals 16
   $ python prog.py 4 -v 1
   usage: prog.py [-h] [-v] square
   prog.py: error: unrecognized arguments: 1
   $ python prog.py 4 -h
   usage: prog.py [-h] [-v] square

   positional arguments:
     square           display a square of a given number

   options:
     -h, --help       show this help message and exit
     -v, --verbosity  increase output verbosity
   $ python prog.py 4 -vvv
   16

* Yes, it's now more of a flag (similar to ``action="store_true"``) in the
  previous version of our script. That should explain the complaint.

* It also behaves similar to "store_true" action.

* Now here's a demonstration of what the "count" action gives. You've probably
  seen this sort of usage before.

* And if you don't specify the ``-v`` flag, that flag is considered to have
  ``None`` value.

* As should be expected, specifying the long form of the flag, we should get
  the same output.

* Sadly, our help output isn't very informative on the new ability our script
  has acquired, but that can always be fixed by improving the documentation for
  our script (e.g. via the ``help`` keyword argument).

* That last output exposes a bug in our program.


Let's fix::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("square", type=int,
                       help="display a square of a given number")
   parser.add_argument("-v", "--verbosity", action="count",
                       help="increase output verbosity")
   args = parser.parse_args()
   answer = args.square**2

   # bugfix: replace == with >=
   if args.verbosity >= 2:
       print(f"the square of {args.square} equals {answer}")
   elif args.verbosity >= 1:
       print(f"{args.square}^2 == {answer}")
   else:
       print(answer)

And this is what it gives:

.. code-block:: shell-session

   $ python prog.py 4 -vvv
   the square of 4 equals 16
   $ python prog.py 4 -vvvv
   the square of 4 equals 16
   $ python prog.py 4
   Traceback (most recent call last):
     File "prog.py", line 11, in <module>
       if args.verbosity >= 2:
   TypeError: '>=' not supported between instances of 'NoneType' and 'int'


* First output went well, and fixes the bug we had before.
  That is, we want any value >= 2 to be as verbose as possible.

* Third output not so good.

Let's fix that bug::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("square", type=int,
                       help="display a square of a given number")
   parser.add_argument("-v", "--verbosity", action="count", default=0,
                       help="increase output verbosity")
   args = parser.parse_args()
   answer = args.square**2
   if args.verbosity >= 2:
       print(f"the square of {args.square} equals {answer}")
   elif args.verbosity >= 1:
       print(f"{args.square}^2 == {answer}")
   else:
       print(answer)

We've just introduced yet another keyword, ``default``.
We've set it to ``0`` in order to make it comparable to the other int values.
Remember that by default,
if an optional argument isn't specified,
it gets the ``None`` value, and that cannot be compared to an int value
(hence the :exc:`TypeError` exception).

And:

.. code-block:: shell-session

   $ python prog.py 4
   16

You can go quite far just with what we've learned so far,
and we have only scratched the surface.
The :mod:`argparse` module is very powerful,
and we'll explore a bit more of it before we end this tutorial.


Getting a little more advanced
==============================

What if we wanted to expand our tiny program to perform other powers,
not just squares::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("x", type=int, help="the base")
   parser.add_argument("y", type=int, help="the exponent")
   parser.add_argument("-v", "--verbosity", action="count", default=0)
   args = parser.parse_args()
   answer = args.x**args.y
   if args.verbosity >= 2:
       print(f"{args.x} to the power {args.y} equals {answer}")
   elif args.verbosity >= 1:
       print(f"{args.x}^{args.y} == {answer}")
   else:
       print(answer)

Output:

.. code-block:: shell-session

   $ python prog.py
   usage: prog.py [-h] [-v] x y
   prog.py: error: the following arguments are required: x, y
   $ python prog.py -h
   usage: prog.py [-h] [-v] x y

   positional arguments:
     x                the base
     y                the exponent

   options:
     -h, --help       show this help message and exit
     -v, --verbosity
   $ python prog.py 4 2 -v
   4^2 == 16


Notice that so far we've been using verbosity level to *change* the text
that gets displayed. The following example instead uses verbosity level
to display *more* text instead::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument("x", type=int, help="the base")
   parser.add_argument("y", type=int, help="the exponent")
   parser.add_argument("-v", "--verbosity", action="count", default=0)
   args = parser.parse_args()
   answer = args.x**args.y
   if args.verbosity >= 2:
       print(f"Running '{__file__}'")
   if args.verbosity >= 1:
       print(f"{args.x}^{args.y} == ", end="")
   print(answer)

Output:

.. code-block:: shell-session

   $ python prog.py 4 2
   16
   $ python prog.py 4 2 -v
   4^2 == 16
   $ python prog.py 4 2 -vv
   Running 'prog.py'
   4^2 == 16


.. _specifying-ambiguous-arguments:

Specifying ambiguous arguments
------------------------------

When there is ambiguity in deciding whether an argument is positional or for an
argument, ``--`` can be used to tell :meth:`~ArgumentParser.parse_args` that
everything after that is a positional argument::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-n', nargs='+')
   >>> parser.add_argument('args', nargs='*')

   >>> # ambiguous, so parse_args assumes it's an option
   >>> parser.parse_args(['-f'])
   usage: PROG [-h] [-n N [N ...]] [args ...]
   PROG: error: unrecognized arguments: -f

   >>> parser.parse_args(['--', '-f'])
   Namespace(args=['-f'], n=None)

   >>> # ambiguous, so the -n option greedily accepts arguments
   >>> parser.parse_args(['-n', '1', '2', '3'])
   Namespace(args=[], n=['1', '2', '3'])

   >>> parser.parse_args(['-n', '1', '--', '2', '3'])
   Namespace(args=['2', '3'], n=['1'])


Conflicting options
-------------------

So far, we have been working with two methods of an
:class:`argparse.ArgumentParser` instance. Let's introduce a third one,
:meth:`~ArgumentParser.add_mutually_exclusive_group`. It allows for us to specify options that
conflict with each other. Let's also change the rest of the program so that
the new functionality makes more sense:
we'll introduce the ``--quiet`` option,
which will be the opposite of the ``--verbose`` one::

   import argparse

   parser = argparse.ArgumentParser()
   group = parser.add_mutually_exclusive_group()
   group.add_argument("-v", "--verbose", action="store_true")
   group.add_argument("-q", "--quiet", action="store_true")
   parser.add_argument("x", type=int, help="the base")
   parser.add_argument("y", type=int, help="the exponent")
   args = parser.parse_args()
   answer = args.x**args.y

   if args.quiet:
       print(answer)
   elif args.verbose:
       print(f"{args.x} to the power {args.y} equals {answer}")
   else:
       print(f"{args.x}^{args.y} == {answer}")

Our program is now simpler, and we've lost some functionality for the sake of
demonstration. Anyways, here's the output:

.. code-block:: shell-session

   $ python prog.py 4 2
   4^2 == 16
   $ python prog.py 4 2 -q
   16
   $ python prog.py 4 2 -v
   4 to the power 2 equals 16
   $ python prog.py 4 2 -vq
   usage: prog.py [-h] [-v | -q] x y
   prog.py: error: argument -q/--quiet: not allowed with argument -v/--verbose
   $ python prog.py 4 2 -v --quiet
   usage: prog.py [-h] [-v | -q] x y
   prog.py: error: argument -q/--quiet: not allowed with argument -v/--verbose

That should be easy to follow. I've added that last output so you can see the
sort of flexibility you get, i.e. mixing long form options with short form
ones.

Before we conclude, you probably want to tell your users the main purpose of
your program, just in case they don't know::

   import argparse

   parser = argparse.ArgumentParser(description="calculate X to the power of Y")
   group = parser.add_mutually_exclusive_group()
   group.add_argument("-v", "--verbose", action="store_true")
   group.add_argument("-q", "--quiet", action="store_true")
   parser.add_argument("x", type=int, help="the base")
   parser.add_argument("y", type=int, help="the exponent")
   args = parser.parse_args()
   answer = args.x**args.y

   if args.quiet:
       print(answer)
   elif args.verbose:
       print(f"{args.x} to the power {args.y} equals {answer}")
   else:
       print(f"{args.x}^{args.y} == {answer}")

Note that slight difference in the usage text. Note the ``[-v | -q]``,
which tells us that we can either use ``-v`` or ``-q``,
but not both at the same time:

.. code-block:: shell-session

   $ python prog.py --help
   usage: prog.py [-h] [-v | -q] x y

   calculate X to the power of Y

   positional arguments:
     x              the base
     y              the exponent

   options:
     -h, --help     show this help message and exit
     -v, --verbose
     -q, --quiet


How to translate the argparse output
====================================

The output of the :mod:`argparse` module such as its help text and error
messages are all made translatable using the :mod:`gettext` module. This
allows applications to easily localize messages produced by
:mod:`argparse`. See also :ref:`i18n-howto`.

For instance, in this :mod:`argparse` output:

.. code-block:: shell-session

   $ python prog.py --help
   usage: prog.py [-h] [-v | -q] x y

   calculate X to the power of Y

   positional arguments:
     x              the base
     y              the exponent

   options:
     -h, --help     show this help message and exit
     -v, --verbose
     -q, --quiet

The strings ``usage:``, ``positional arguments:``, ``options:`` and
``show this help message and exit`` are all translatable.

In order to translate these strings, they must first be extracted
into a ``.po`` file. For example, using `Babel <https://babel.pocoo.org/>`__,
run this command:

.. code-block:: shell-session

  $ pybabel extract -o messages.po /usr/lib/python3.12/argparse.py

This command will extract all translatable strings from the :mod:`argparse`
module and output them into a file named ``messages.po``. This command assumes
that your Python installation is in ``/usr/lib``.

You can find out the location of the :mod:`argparse` module on your system
using this script::

   import argparse
   print(argparse.__file__)

Once the messages in the ``.po`` file are translated and the translations are
installed using :mod:`gettext`, :mod:`argparse` will be able to display the
translated messages.

To translate your own strings in the :mod:`argparse` output, use :mod:`gettext`.

Custom type converters
======================

The :mod:`argparse` module allows you to specify custom type converters for
your command-line arguments. This allows you to modify user input before it's
stored in the :class:`argparse.Namespace`. This can be useful when you need to
pre-process the input before it is used in your program.

When using a custom type converter, you can use any callable that takes a
single string argument (the argument value) and returns the converted value.
However, if you need to handle more complex scenarios, you can use a custom
action class with the **action** parameter instead.

For example, let's say you want to handle arguments with different prefixes and
process them accordingly::

   import argparse

   parser = argparse.ArgumentParser(prefix_chars='-+')

   parser.add_argument('-a', metavar='<value>', action='append',
                       type=lambda x: ('-', x))
   parser.add_argument('+a', metavar='<value>', action='append',
                       type=lambda x: ('+', x))

   args = parser.parse_args()
   print(args)

Output:

.. code-block:: shell-session

   $ python prog.py -a value1 +a value2
   Namespace(a=[('-', 'value1'), ('+', 'value2')])

In this example, we:

* Created a parser with custom prefix characters using the ``prefix_chars``
  parameter.

* Defined two arguments, ``-a`` and ``+a``, which used the ``type`` parameter to
  create custom type converters to store the value in a tuple with the prefix.

Without the custom type converters, the arguments would have treated the ``-a``
and ``+a`` as the same argument, which would have been undesirable. By using custom
type converters, we were able to differentiate between the two arguments.

Conclusion
==========

The :mod:`argparse` module offers a lot more than shown here.
Its docs are quite detailed and thorough, and full of examples.
Having gone through this tutorial, you should easily digest them
without feeling overwhelmed.
