:mod:`argparse` --- Parser for command-line options, arguments and sub-commands
===============================================================================

.. module:: argparse
   :synopsis: Command-line option and argument parsing library.
.. moduleauthor:: Steven Bethard <steven.bethard@gmail.com>
.. sectionauthor:: Steven Bethard <steven.bethard@gmail.com>

**Source code:** :source:`Lib/argparse.py`

.. versionadded:: 3.2

--------------

The :mod:`argparse` module makes it easy to write user friendly command line
interfaces. The program defines what arguments it requires, and :mod:`argparse`
will figure out how to parse those out of :data:`sys.argv`.  The :mod:`argparse`
module also automatically generates help and usage messages and issues errors
when users give the program invalid arguments.


Example
-------

The following code is a Python program that takes a list of integers and
produces either the sum or the max::

   import argparse

   parser = argparse.ArgumentParser(description='Process some integers.')
   parser.add_argument('integers', metavar='N', type=int, nargs='+',
                      help='an integer for the accumulator')
   parser.add_argument('--sum', dest='accumulate', action='store_const',
                      const=sum, default=max,
                      help='sum the integers (default: find the max)')

   args = parser.parse_args()
   print(args.accumulate(args.integers))

Assuming the Python code above is saved into a file called ``prog.py``, it can
be run at the command line and provides useful help messages::

   $ prog.py -h
   usage: prog.py [-h] [--sum] N [N ...]

   Process some integers.

   positional arguments:
    N           an integer for the accumulator

   optional arguments:
    -h, --help  show this help message and exit
    --sum       sum the integers (default: find the max)

When run with the appropriate arguments, it prints either the sum or the max of
the command-line integers::

   $ prog.py 1 2 3 4
   4

   $ prog.py 1 2 3 4 --sum
   10

If invalid arguments are passed in, it will issue an error::

   $ prog.py a b c
   usage: prog.py [-h] [--sum] N [N ...]
   prog.py: error: argument N: invalid int value: 'a'

The following sections walk you through this example.


Creating a parser
^^^^^^^^^^^^^^^^^

The first step in using the :mod:`argparse` is creating an
:class:`ArgumentParser` object::

   >>> parser = argparse.ArgumentParser(description='Process some integers.')

The :class:`ArgumentParser` object will hold all the information necessary to
parse the command line into python data types.


Adding arguments
^^^^^^^^^^^^^^^^

Filling an :class:`ArgumentParser` with information about program arguments is
done by making calls to the :meth:`~ArgumentParser.add_argument` method.
Generally, these calls tell the :class:`ArgumentParser` how to take the strings
on the command line and turn them into objects.  This information is stored and
used when :meth:`~ArgumentParser.parse_args` is called. For example::

   >>> parser.add_argument('integers', metavar='N', type=int, nargs='+',
   ...                     help='an integer for the accumulator')
   >>> parser.add_argument('--sum', dest='accumulate', action='store_const',
   ...                     const=sum, default=max,
   ...                     help='sum the integers (default: find the max)')

Later, calling :meth:`parse_args` will return an object with
two attributes, ``integers`` and ``accumulate``.  The ``integers`` attribute
will be a list of one or more ints, and the ``accumulate`` attribute will be
either the :func:`sum` function, if ``--sum`` was specified at the command line,
or the :func:`max` function if it was not.


Parsing arguments
^^^^^^^^^^^^^^^^^

:class:`ArgumentParser` parses args through the
:meth:`~ArgumentParser.parse_args` method.  This will inspect the command line,
convert each arg to the appropriate type and then invoke the appropriate action.
In most cases, this means a simple namespace object will be built up from
attributes parsed out of the command line::

   >>> parser.parse_args(['--sum', '7', '-1', '42'])
   Namespace(accumulate=<built-in function sum>, integers=[7, -1, 42])

In a script, :meth:`~ArgumentParser.parse_args` will typically be called with no
arguments, and the :class:`ArgumentParser` will automatically determine the
command-line args from :data:`sys.argv`.


ArgumentParser objects
----------------------

.. class:: ArgumentParser([description], [epilog], [prog], [usage], [add_help], \
                          [argument_default], [parents], [prefix_chars], \
                          [conflict_handler], [formatter_class])

   Create a new :class:`ArgumentParser` object.  Each parameter has its own more
   detailed description below, but in short they are:

   * description_ - Text to display before the argument help.

   * epilog_ - Text to display after the argument help.

   * add_help_ - Add a -h/--help option to the parser. (default: ``True``)

   * argument_default_ - Set the global default value for arguments.
     (default: ``None``)

   * parents_ - A list of :class:`ArgumentParser` objects whose arguments should
     also be included.

   * prefix_chars_ - The set of characters that prefix optional arguments.
     (default: '-')

   * fromfile_prefix_chars_ - The set of characters that prefix files from
     which additional arguments should be read. (default: ``None``)

   * formatter_class_ - A class for customizing the help output.

   * conflict_handler_ - Usually unnecessary, defines strategy for resolving
     conflicting optionals.

   * prog_ - The name of the program (default:
     :data:`sys.argv[0]`)

   * usage_ - The string describing the program usage (default: generated)

The following sections describe how each of these are used.


description
^^^^^^^^^^^

Most calls to the :class:`ArgumentParser` constructor will use the
``description=`` keyword argument.  This argument gives a brief description of
what the program does and how it works.  In help messages, the description is
displayed between the command-line usage string and the help messages for the
various arguments::

   >>> parser = argparse.ArgumentParser(description='A foo that bars')
   >>> parser.print_help()
   usage: argparse.py [-h]

   A foo that bars

   optional arguments:
    -h, --help  show this help message and exit

By default, the description will be line-wrapped so that it fits within the
given space.  To change this behavior, see the formatter_class_ argument.


epilog
^^^^^^

Some programs like to display additional description of the program after the
description of the arguments.  Such text can be specified using the ``epilog=``
argument to :class:`ArgumentParser`::

   >>> parser = argparse.ArgumentParser(
   ...     description='A foo that bars',
   ...     epilog="And that's how you'd foo a bar")
   >>> parser.print_help()
   usage: argparse.py [-h]

   A foo that bars

   optional arguments:
    -h, --help  show this help message and exit

   And that's how you'd foo a bar

As with the description_ argument, the ``epilog=`` text is by default
line-wrapped, but this behavior can be adjusted with the formatter_class_
argument to :class:`ArgumentParser`.


add_help
^^^^^^^^

By default, ArgumentParser objects add an option which simply displays
the parser's help message. For example, consider a file named
``myprogram.py`` containing the following code::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument('--foo', help='foo help')
   args = parser.parse_args()

If ``-h`` or ``--help`` is supplied at the command line, the ArgumentParser
help will be printed::

   $ python myprogram.py --help
   usage: myprogram.py [-h] [--foo FOO]

   optional arguments:
    -h, --help  show this help message and exit
    --foo FOO   foo help

Occasionally, it may be useful to disable the addition of this help option.
This can be achieved by passing ``False`` as the ``add_help=`` argument to
:class:`ArgumentParser`::

   >>> parser = argparse.ArgumentParser(prog='PROG', add_help=False)
   >>> parser.add_argument('--foo', help='foo help')
   >>> parser.print_help()
   usage: PROG [--foo FOO]

   optional arguments:
    --foo FOO  foo help

The help option is typically ``-h/--help``. The exception to this is
if the ``prefix_chars=`` is specified and does not include ``'-'``, in
which case ``-h`` and ``--help`` are not valid options.  In
this case, the first character in ``prefix_chars`` is used to prefix
the help options::

   >>> parser = argparse.ArgumentParser(prog='PROG', prefix_chars='+/')
   >>> parser.print_help()
   usage: PROG [+h]

   optional arguments:
     +h, ++help  show this help message and exit


prefix_chars
^^^^^^^^^^^^

Most command-line options will use ``'-'`` as the prefix, e.g. ``-f/--foo``.
Parsers that need to support different or additional prefix
characters, e.g. for options
like ``+f`` or ``/foo``, may specify them using the ``prefix_chars=`` argument
to the ArgumentParser constructor::

   >>> parser = argparse.ArgumentParser(prog='PROG', prefix_chars='-+')
   >>> parser.add_argument('+f')
   >>> parser.add_argument('++bar')
   >>> parser.parse_args('+f X ++bar Y'.split())
   Namespace(bar='Y', f='X')

The ``prefix_chars=`` argument defaults to ``'-'``. Supplying a set of
characters that does not include ``'-'`` will cause ``-f/--foo`` options to be
disallowed.


fromfile_prefix_chars
^^^^^^^^^^^^^^^^^^^^^

Sometimes, for example when dealing with a particularly long argument lists, it
may make sense to keep the list of arguments in a file rather than typing it out
at the command line.  If the ``fromfile_prefix_chars=`` argument is given to the
:class:`ArgumentParser` constructor, then arguments that start with any of the
specified characters will be treated as files, and will be replaced by the
arguments they contain.  For example::

   >>> with open('args.txt', 'w') as fp:
   ...    fp.write('-f\nbar')
   >>> parser = argparse.ArgumentParser(fromfile_prefix_chars='@')
   >>> parser.add_argument('-f')
   >>> parser.parse_args(['-f', 'foo', '@args.txt'])
   Namespace(f='bar')

Arguments read from a file must by default be one per line (but see also
:meth:`convert_arg_line_to_args`) and are treated as if they were in the same
place as the original file referencing argument on the command line.  So in the
example above, the expression ``['-f', 'foo', '@args.txt']`` is considered
equivalent to the expression ``['-f', 'foo', '-f', 'bar']``.

The ``fromfile_prefix_chars=`` argument defaults to ``None``, meaning that
arguments will never be treated as file references.


argument_default
^^^^^^^^^^^^^^^^

Generally, argument defaults are specified either by passing a default to
:meth:`add_argument` or by calling the :meth:`set_defaults` methods with a
specific set of name-value pairs.  Sometimes however, it may be useful to
specify a single parser-wide default for arguments.  This can be accomplished by
passing the ``argument_default=`` keyword argument to :class:`ArgumentParser`.
For example, to globally suppress attribute creation on :meth:`parse_args`
calls, we supply ``argument_default=SUPPRESS``::

   >>> parser = argparse.ArgumentParser(argument_default=argparse.SUPPRESS)
   >>> parser.add_argument('--foo')
   >>> parser.add_argument('bar', nargs='?')
   >>> parser.parse_args(['--foo', '1', 'BAR'])
   Namespace(bar='BAR', foo='1')
   >>> parser.parse_args([])
   Namespace()


parents
^^^^^^^

Sometimes, several parsers share a common set of arguments. Rather than
repeating the definitions of these arguments, a single parser with all the
shared arguments and passed to ``parents=`` argument to :class:`ArgumentParser`
can be used.  The ``parents=`` argument takes a list of :class:`ArgumentParser`
objects, collects all the positional and optional actions from them, and adds
these actions to the :class:`ArgumentParser` object being constructed::

   >>> parent_parser = argparse.ArgumentParser(add_help=False)
   >>> parent_parser.add_argument('--parent', type=int)

   >>> foo_parser = argparse.ArgumentParser(parents=[parent_parser])
   >>> foo_parser.add_argument('foo')
   >>> foo_parser.parse_args(['--parent', '2', 'XXX'])
   Namespace(foo='XXX', parent=2)

   >>> bar_parser = argparse.ArgumentParser(parents=[parent_parser])
   >>> bar_parser.add_argument('--bar')
   >>> bar_parser.parse_args(['--bar', 'YYY'])
   Namespace(bar='YYY', parent=None)

Note that most parent parsers will specify ``add_help=False``.  Otherwise, the
:class:`ArgumentParser` will see two ``-h/--help`` options (one in the parent
and one in the child) and raise an error.

.. note::
   You must fully initialize the parsers before passing them via ``parents=``.
   If you change the parent parsers after the child parser, those changes will
   not be reflected in the child.


formatter_class
^^^^^^^^^^^^^^^

:class:`ArgumentParser` objects allow the help formatting to be customized by
specifying an alternate formatting class.  Currently, there are three such
classes: :class:`argparse.RawDescriptionHelpFormatter`,
:class:`argparse.RawTextHelpFormatter` and
:class:`argparse.ArgumentDefaultsHelpFormatter`.  The first two allow more
control over how textual descriptions are displayed, while the last
automatically adds information about argument default values.

By default, :class:`ArgumentParser` objects line-wrap the description_ and
epilog_ texts in command-line help messages::

   >>> parser = argparse.ArgumentParser(
   ...     prog='PROG',
   ...     description='''this description
   ...         was indented weird
   ...             but that is okay''',
   ...     epilog='''
   ...             likewise for this epilog whose whitespace will
   ...         be cleaned up and whose words will be wrapped
   ...         across a couple lines''')
   >>> parser.print_help()
   usage: PROG [-h]

   this description was indented weird but that is okay

   optional arguments:
    -h, --help  show this help message and exit

   likewise for this epilog whose whitespace will be cleaned up and whose words
   will be wrapped across a couple lines

Passing :class:`argparse.RawDescriptionHelpFormatter` as ``formatter_class=``
indicates that description_ and epilog_ are already correctly formatted and
should not be line-wrapped::

   >>> parser = argparse.ArgumentParser(
   ...     prog='PROG',
   ...     formatter_class=argparse.RawDescriptionHelpFormatter,
   ...     description=textwrap.dedent('''\
   ...         Please do not mess up this text!
   ...         --------------------------------
   ...             I have indented it
   ...             exactly the way
   ...             I want it
   ...         '''))
   >>> parser.print_help()
   usage: PROG [-h]

   Please do not mess up this text!
   --------------------------------
      I have indented it
      exactly the way
      I want it

   optional arguments:
    -h, --help  show this help message and exit

:class:`RawTextHelpFormatter` maintains whitespace for all sorts of help text
including argument descriptions.

The other formatter class available, :class:`ArgumentDefaultsHelpFormatter`,
will add information about the default value of each of the arguments::

   >>> parser = argparse.ArgumentParser(
   ...     prog='PROG',
   ...     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
   >>> parser.add_argument('--foo', type=int, default=42, help='FOO!')
   >>> parser.add_argument('bar', nargs='*', default=[1, 2, 3], help='BAR!')
   >>> parser.print_help()
   usage: PROG [-h] [--foo FOO] [bar [bar ...]]

   positional arguments:
    bar         BAR! (default: [1, 2, 3])

   optional arguments:
    -h, --help  show this help message and exit
    --foo FOO   FOO! (default: 42)


conflict_handler
^^^^^^^^^^^^^^^^

:class:`ArgumentParser` objects do not allow two actions with the same option
string.  By default, :class:`ArgumentParser` objects raises an exception if an
attempt is made to create an argument with an option string that is already in
use::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-f', '--foo', help='old foo help')
   >>> parser.add_argument('--foo', help='new foo help')
   Traceback (most recent call last):
    ..
   ArgumentError: argument --foo: conflicting option string(s): --foo

Sometimes (e.g. when using parents_) it may be useful to simply override any
older arguments with the same option string.  To get this behavior, the value
``'resolve'`` can be supplied to the ``conflict_handler=`` argument of
:class:`ArgumentParser`::

   >>> parser = argparse.ArgumentParser(prog='PROG', conflict_handler='resolve')
   >>> parser.add_argument('-f', '--foo', help='old foo help')
   >>> parser.add_argument('--foo', help='new foo help')
   >>> parser.print_help()
   usage: PROG [-h] [-f FOO] [--foo FOO]

   optional arguments:
    -h, --help  show this help message and exit
    -f FOO      old foo help
    --foo FOO   new foo help

Note that :class:`ArgumentParser` objects only remove an action if all of its
option strings are overridden.  So, in the example above, the old ``-f/--foo``
action is retained as the ``-f`` action, because only the ``--foo`` option
string was overridden.


prog
^^^^

By default, :class:`ArgumentParser` objects uses ``sys.argv[0]`` to determine
how to display the name of the program in help messages.  This default is almost
always desirable because it will make the help messages match how the program was
invoked on the command line.  For example, consider a file named
``myprogram.py`` with the following code::

   import argparse
   parser = argparse.ArgumentParser()
   parser.add_argument('--foo', help='foo help')
   args = parser.parse_args()

The help for this program will display ``myprogram.py`` as the program name
(regardless of where the program was invoked from)::

   $ python myprogram.py --help
   usage: myprogram.py [-h] [--foo FOO]

   optional arguments:
    -h, --help  show this help message and exit
    --foo FOO   foo help
   $ cd ..
   $ python subdir\myprogram.py --help
   usage: myprogram.py [-h] [--foo FOO]

   optional arguments:
    -h, --help  show this help message and exit
    --foo FOO   foo help

To change this default behavior, another value can be supplied using the
``prog=`` argument to :class:`ArgumentParser`::

   >>> parser = argparse.ArgumentParser(prog='myprogram')
   >>> parser.print_help()
   usage: myprogram [-h]

   optional arguments:
    -h, --help  show this help message and exit

Note that the program name, whether determined from ``sys.argv[0]`` or from the
``prog=`` argument, is available to help messages using the ``%(prog)s`` format
specifier.

::

   >>> parser = argparse.ArgumentParser(prog='myprogram')
   >>> parser.add_argument('--foo', help='foo of the %(prog)s program')
   >>> parser.print_help()
   usage: myprogram [-h] [--foo FOO]

   optional arguments:
    -h, --help  show this help message and exit
    --foo FOO   foo of the myprogram program


usage
^^^^^

By default, :class:`ArgumentParser` calculates the usage message from the
arguments it contains::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('--foo', nargs='?', help='foo help')
   >>> parser.add_argument('bar', nargs='+', help='bar help')
   >>> parser.print_help()
   usage: PROG [-h] [--foo [FOO]] bar [bar ...]

   positional arguments:
    bar          bar help

   optional arguments:
    -h, --help   show this help message and exit
    --foo [FOO]  foo help

The default message can be overridden with the ``usage=`` keyword argument::

   >>> parser = argparse.ArgumentParser(prog='PROG', usage='%(prog)s [options]')
   >>> parser.add_argument('--foo', nargs='?', help='foo help')
   >>> parser.add_argument('bar', nargs='+', help='bar help')
   >>> parser.print_help()
   usage: PROG [options]

   positional arguments:
    bar          bar help

   optional arguments:
    -h, --help   show this help message and exit
    --foo [FOO]  foo help

The ``%(prog)s`` format specifier is available to fill in the program name in
your usage messages.


The add_argument() method
-------------------------

.. method:: ArgumentParser.add_argument(name or flags..., [action], [nargs], \
                           [const], [default], [type], [choices], [required], \
                           [help], [metavar], [dest])

   Define how a single command-line argument should be parsed.  Each parameter
   has its own more detailed description below, but in short they are:

   * `name or flags`_ - Either a name or a list of option strings, e.g. ``foo``
     or ``-f, --foo``

   * action_ - The basic type of action to be taken when this argument is
     encountered at the command line.

   * nargs_ - The number of command-line arguments that should be consumed.

   * const_ - A constant value required by some action_ and nargs_ selections.

   * default_ - The value produced if the argument is absent from the
     command line.

   * type_ - The type to which the command-line arg should be converted.

   * choices_ - A container of the allowable values for the argument.

   * required_ - Whether or not the command-line option may be omitted
     (optionals only).

   * help_ - A brief description of what the argument does.

   * metavar_ - A name for the argument in usage messages.

   * dest_ - The name of the attribute to be added to the object returned by
     :meth:`parse_args`.

The following sections describe how each of these are used.


name or flags
^^^^^^^^^^^^^

The :meth:`add_argument` method must know whether an optional argument, like
``-f`` or ``--foo``, or a positional argument, like a list of filenames, is
expected.  The first arguments passed to :meth:`add_argument` must therefore be
either a series of flags, or a simple argument name.  For example, an optional
argument could be created like::

   >>> parser.add_argument('-f', '--foo')

while a positional argument could be created like::

   >>> parser.add_argument('bar')

When :meth:`parse_args` is called, optional arguments will be identified by the
``-`` prefix, and the remaining arguments will be assumed to be positional::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-f', '--foo')
   >>> parser.add_argument('bar')
   >>> parser.parse_args(['BAR'])
   Namespace(bar='BAR', foo=None)
   >>> parser.parse_args(['BAR', '--foo', 'FOO'])
   Namespace(bar='BAR', foo='FOO')
   >>> parser.parse_args(['--foo', 'FOO'])
   usage: PROG [-h] [-f FOO] bar
   PROG: error: too few arguments


action
^^^^^^

:class:`ArgumentParser` objects associate command-line args with actions.  These
actions can do just about anything with the command-line args associated with
them, though most actions simply add an attribute to the object returned by
:meth:`parse_args`.  The ``action`` keyword argument specifies how the
command-line args should be handled. The supported actions are:

* ``'store'`` - This just stores the argument's value.  This is the default
   action. For example::

    >>> parser = argparse.ArgumentParser()
    >>> parser.add_argument('--foo')
    >>> parser.parse_args('--foo 1'.split())
    Namespace(foo='1')

* ``'store_const'`` - This stores the value specified by the const_ keyword
   argument.  (Note that the const_ keyword argument defaults to the rather
   unhelpful ``None``.)  The ``'store_const'`` action is most commonly used with
   optional arguments that specify some sort of flag.  For example::

    >>> parser = argparse.ArgumentParser()
    >>> parser.add_argument('--foo', action='store_const', const=42)
    >>> parser.parse_args('--foo'.split())
    Namespace(foo=42)

* ``'store_true'`` and ``'store_false'`` - These store the values ``True`` and
  ``False`` respectively.  These are special cases of ``'store_const'``.  For
  example::

    >>> parser = argparse.ArgumentParser()
    >>> parser.add_argument('--foo', action='store_true')
    >>> parser.add_argument('--bar', action='store_false')
    >>> parser.parse_args('--foo --bar'.split())
    Namespace(bar=False, foo=True)

* ``'append'`` - This stores a list, and appends each argument value to the
  list.  This is useful to allow an option to be specified multiple times.
  Example usage::

    >>> parser = argparse.ArgumentParser()
    >>> parser.add_argument('--foo', action='append')
    >>> parser.parse_args('--foo 1 --foo 2'.split())
    Namespace(foo=['1', '2'])

* ``'append_const'`` - This stores a list, and appends the value specified by
  the const_ keyword argument to the list.  (Note that the const_ keyword
  argument defaults to ``None``.)  The ``'append_const'`` action is typically
  useful when multiple arguments need to store constants to the same list. For
  example::

    >>> parser = argparse.ArgumentParser()
    >>> parser.add_argument('--str', dest='types', action='append_const', const=str)
    >>> parser.add_argument('--int', dest='types', action='append_const', const=int)
    >>> parser.parse_args('--str --int'.split())
    Namespace(types=[<type 'str'>, <type 'int'>])

* ``'version'`` - This expects a ``version=`` keyword argument in the
  :meth:`add_argument` call, and prints version information and exits when
  invoked.

    >>> import argparse
    >>> parser = argparse.ArgumentParser(prog='PROG')
    >>> parser.add_argument('--version', action='version', version='%(prog)s 2.0')
    >>> parser.parse_args(['--version'])
    PROG 2.0

You can also specify an arbitrary action by passing an object that implements
the Action API.  The easiest way to do this is to extend
:class:`argparse.Action`, supplying an appropriate ``__call__`` method.  The
``__call__`` method should accept four parameters:

* ``parser`` - The ArgumentParser object which contains this action.

* ``namespace`` - The namespace object that will be returned by
  :meth:`parse_args`.  Most actions add an attribute to this object.

* ``values`` - The associated command-line args, with any type-conversions
  applied.  (Type-conversions are specified with the type_ keyword argument to
  :meth:`add_argument`.

* ``option_string`` - The option string that was used to invoke this action.
  The ``option_string`` argument is optional, and will be absent if the action
  is associated with a positional argument.

An example of a custom action::

   >>> class FooAction(argparse.Action):
   ...     def __call__(self, parser, namespace, values, option_string=None):
   ...         print('%r %r %r' % (namespace, values, option_string))
   ...         setattr(namespace, self.dest, values)
   ...
   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo', action=FooAction)
   >>> parser.add_argument('bar', action=FooAction)
   >>> args = parser.parse_args('1 --foo 2'.split())
   Namespace(bar=None, foo=None) '1' None
   Namespace(bar='1', foo=None) '2' '--foo'
   >>> args
   Namespace(bar='1', foo='2')


nargs
^^^^^

ArgumentParser objects usually associate a single command-line argument with a
single action to be taken.  The ``nargs`` keyword argument associates a
different number of command-line arguments with a single action..  The supported
values are:

* N (an integer).  N args from the command line will be gathered together into a
  list.  For example::

     >>> parser = argparse.ArgumentParser()
     >>> parser.add_argument('--foo', nargs=2)
     >>> parser.add_argument('bar', nargs=1)
     >>> parser.parse_args('c --foo a b'.split())
     Namespace(bar=['c'], foo=['a', 'b'])

  Note that ``nargs=1`` produces a list of one item.  This is different from
  the default, in which the item is produced by itself.

* ``'?'``. One arg will be consumed from the command line if possible, and
  produced as a single item.  If no command-line arg is present, the value from
  default_ will be produced.  Note that for optional arguments, there is an
  additional case - the option string is present but not followed by a
  command-line arg.  In this case the value from const_ will be produced.  Some
  examples to illustrate this::

     >>> parser = argparse.ArgumentParser()
     >>> parser.add_argument('--foo', nargs='?', const='c', default='d')
     >>> parser.add_argument('bar', nargs='?', default='d')
     >>> parser.parse_args('XX --foo YY'.split())
     Namespace(bar='XX', foo='YY')
     >>> parser.parse_args('XX --foo'.split())
     Namespace(bar='XX', foo='c')
     >>> parser.parse_args(''.split())
     Namespace(bar='d', foo='d')

  One of the more common uses of ``nargs='?'`` is to allow optional input and
  output files::

     >>> parser = argparse.ArgumentParser()
     >>> parser.add_argument('infile', nargs='?', type=argparse.FileType('r'),
     ...                     default=sys.stdin)
     >>> parser.add_argument('outfile', nargs='?', type=argparse.FileType('w'),
     ...                     default=sys.stdout)
     >>> parser.parse_args(['input.txt', 'output.txt'])
     Namespace(infile=<_io.TextIOWrapper name='input.txt' encoding='UTF-8'>,
               outfile=<_io.TextIOWrapper name='output.txt' encoding='UTF-8'>)
     >>> parser.parse_args([])
     Namespace(infile=<_io.TextIOWrapper name='<stdin>' encoding='UTF-8'>,
               outfile=<_io.TextIOWrapper name='<stdout>' encoding='UTF-8'>)

* ``'*'``.  All command-line args present are gathered into a list.  Note that
  it generally doesn't make much sense to have more than one positional argument
  with ``nargs='*'``, but multiple optional arguments with ``nargs='*'`` is
  possible.  For example::

     >>> parser = argparse.ArgumentParser()
     >>> parser.add_argument('--foo', nargs='*')
     >>> parser.add_argument('--bar', nargs='*')
     >>> parser.add_argument('baz', nargs='*')
     >>> parser.parse_args('a b --foo x y --bar 1 2'.split())
     Namespace(bar=['1', '2'], baz=['a', 'b'], foo=['x', 'y'])

* ``'+'``. Just like ``'*'``, all command-line args present are gathered into a
  list.  Additionally, an error message will be generated if there wasn't at
  least one command-line arg present.  For example::

     >>> parser = argparse.ArgumentParser(prog='PROG')
     >>> parser.add_argument('foo', nargs='+')
     >>> parser.parse_args('a b'.split())
     Namespace(foo=['a', 'b'])
     >>> parser.parse_args(''.split())
     usage: PROG [-h] foo [foo ...]
     PROG: error: too few arguments

If the ``nargs`` keyword argument is not provided, the number of args consumed
is determined by the action_.  Generally this means a single command-line arg
will be consumed and a single item (not a list) will be produced.


const
^^^^^

The ``const`` argument of :meth:`add_argument` is used to hold constant values
that are not read from the command line but are required for the various
ArgumentParser actions.  The two most common uses of it are:

* When :meth:`add_argument` is called with ``action='store_const'`` or
  ``action='append_const'``.  These actions add the ``const`` value to one of
  the attributes of the object returned by :meth:`parse_args`.  See the action_
  description for examples.

* When :meth:`add_argument` is called with option strings (like ``-f`` or
  ``--foo``) and ``nargs='?'``.  This creates an optional argument that can be
  followed by zero or one command-line args.  When parsing the command line, if
  the option string is encountered with no command-line arg following it, the
  value of ``const`` will be assumed instead. See the nargs_ description for
  examples.

The ``const`` keyword argument defaults to ``None``.


default
^^^^^^^

All optional arguments and some positional arguments may be omitted at the
command line.  The ``default`` keyword argument of :meth:`add_argument`, whose
value defaults to ``None``, specifies what value should be used if the
command-line arg is not present.  For optional arguments, the ``default`` value
is used when the option string was not present at the command line::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo', default=42)
   >>> parser.parse_args('--foo 2'.split())
   Namespace(foo='2')
   >>> parser.parse_args(''.split())
   Namespace(foo=42)

For positional arguments with nargs_ ``='?'`` or ``'*'``, the ``default`` value
is used when no command-line arg was present::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('foo', nargs='?', default=42)
   >>> parser.parse_args('a'.split())
   Namespace(foo='a')
   >>> parser.parse_args(''.split())
   Namespace(foo=42)


Providing ``default=argparse.SUPPRESS`` causes no attribute to be added if the
command-line argument was not present.::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo', default=argparse.SUPPRESS)
   >>> parser.parse_args([])
   Namespace()
   >>> parser.parse_args(['--foo', '1'])
   Namespace(foo='1')


type
^^^^

By default, ArgumentParser objects read command-line args in as simple strings.
However, quite often the command-line string should instead be interpreted as
another type, like a :class:`float` or :class:`int`.  The ``type`` keyword
argument of :meth:`add_argument` allows any necessary type-checking and
type-conversions to be performed.  Common built-in types and functions can be
used directly as the value of the ``type`` argument::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('foo', type=int)
   >>> parser.add_argument('bar', type=open)
   >>> parser.parse_args('2 temp.txt'.split())
   Namespace(bar=<_io.TextIOWrapper name='temp.txt' encoding='UTF-8'>, foo=2)

To ease the use of various types of files, the argparse module provides the
factory FileType which takes the ``mode=`` and ``bufsize=`` arguments of the
:func:`open` function.  For example, ``FileType('w')`` can be used to create a
writable file::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('bar', type=argparse.FileType('w'))
   >>> parser.parse_args(['out.txt'])
   Namespace(bar=<_io.TextIOWrapper name='out.txt' encoding='UTF-8'>)

``type=`` can take any callable that takes a single string argument and returns
the type-converted value::

   >>> def perfect_square(string):
   ...     value = int(string)
   ...     sqrt = math.sqrt(value)
   ...     if sqrt != int(sqrt):
   ...         msg = "%r is not a perfect square" % string
   ...         raise argparse.ArgumentTypeError(msg)
   ...     return value
   ...
   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('foo', type=perfect_square)
   >>> parser.parse_args('9'.split())
   Namespace(foo=9)
   >>> parser.parse_args('7'.split())
   usage: PROG [-h] foo
   PROG: error: argument foo: '7' is not a perfect square

The choices_ keyword argument may be more convenient for type checkers that
simply check against a range of values::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('foo', type=int, choices=range(5, 10))
   >>> parser.parse_args('7'.split())
   Namespace(foo=7)
   >>> parser.parse_args('11'.split())
   usage: PROG [-h] {5,6,7,8,9}
   PROG: error: argument foo: invalid choice: 11 (choose from 5, 6, 7, 8, 9)

See the choices_ section for more details.


choices
^^^^^^^

Some command-line args should be selected from a restricted set of values.
These can be handled by passing a container object as the ``choices`` keyword
argument to :meth:`add_argument`.  When the command line is parsed, arg values
will be checked, and an error message will be displayed if the arg was not one
of the acceptable values::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('foo', choices='abc')
   >>> parser.parse_args('c'.split())
   Namespace(foo='c')
   >>> parser.parse_args('X'.split())
   usage: PROG [-h] {a,b,c}
   PROG: error: argument foo: invalid choice: 'X' (choose from 'a', 'b', 'c')

Note that inclusion in the ``choices`` container is checked after any type_
conversions have been performed, so the type of the objects in the ``choices``
container should match the type_ specified::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('foo', type=complex, choices=[1, 1j])
   >>> parser.parse_args('1j'.split())
   Namespace(foo=1j)
   >>> parser.parse_args('-- -4'.split())
   usage: PROG [-h] {1,1j}
   PROG: error: argument foo: invalid choice: (-4+0j) (choose from 1, 1j)

Any object that supports the ``in`` operator can be passed as the ``choices``
value, so :class:`dict` objects, :class:`set` objects, custom containers,
etc. are all supported.


required
^^^^^^^^

In general, the argparse module assumes that flags like ``-f`` and ``--bar``
indicate *optional* arguments, which can always be omitted at the command line.
To make an option *required*, ``True`` can be specified for the ``required=``
keyword argument to :meth:`add_argument`::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo', required=True)
   >>> parser.parse_args(['--foo', 'BAR'])
   Namespace(foo='BAR')
   >>> parser.parse_args([])
   usage: argparse.py [-h] [--foo FOO]
   argparse.py: error: option --foo is required

As the example shows, if an option is marked as ``required``, :meth:`parse_args`
will report an error if that option is not present at the command line.

.. note::

    Required options are generally considered bad form because users expect
    *options* to be *optional*, and thus they should be avoided when possible.


help
^^^^

The ``help`` value is a string containing a brief description of the argument.
When a user requests help (usually by using ``-h`` or ``--help`` at the
command line), these ``help`` descriptions will be displayed with each
argument::

   >>> parser = argparse.ArgumentParser(prog='frobble')
   >>> parser.add_argument('--foo', action='store_true',
   ...         help='foo the bars before frobbling')
   >>> parser.add_argument('bar', nargs='+',
   ...         help='one of the bars to be frobbled')
   >>> parser.parse_args('-h'.split())
   usage: frobble [-h] [--foo] bar [bar ...]

   positional arguments:
    bar     one of the bars to be frobbled

   optional arguments:
    -h, --help  show this help message and exit
    --foo   foo the bars before frobbling

The ``help`` strings can include various format specifiers to avoid repetition
of things like the program name or the argument default_.  The available
specifiers include the program name, ``%(prog)s`` and most keyword arguments to
:meth:`add_argument`, e.g. ``%(default)s``, ``%(type)s``, etc.::

   >>> parser = argparse.ArgumentParser(prog='frobble')
   >>> parser.add_argument('bar', nargs='?', type=int, default=42,
   ...         help='the bar to %(prog)s (default: %(default)s)')
   >>> parser.print_help()
   usage: frobble [-h] [bar]

   positional arguments:
    bar     the bar to frobble (default: 42)

   optional arguments:
    -h, --help  show this help message and exit


metavar
^^^^^^^

When :class:`ArgumentParser` generates help messages, it need some way to refer
to each expected argument.  By default, ArgumentParser objects use the dest_
value as the "name" of each object.  By default, for positional argument
actions, the dest_ value is used directly, and for optional argument actions,
the dest_ value is uppercased.  So, a single positional argument with
``dest='bar'`` will that argument will be referred to as ``bar``. A single
optional argument ``--foo`` that should be followed by a single command-line arg
will be referred to as ``FOO``.  An example::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo')
   >>> parser.add_argument('bar')
   >>> parser.parse_args('X --foo Y'.split())
   Namespace(bar='X', foo='Y')
   >>> parser.print_help()
   usage:  [-h] [--foo FOO] bar

   positional arguments:
    bar

   optional arguments:
    -h, --help  show this help message and exit
    --foo FOO

An alternative name can be specified with ``metavar``::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo', metavar='YYY')
   >>> parser.add_argument('bar', metavar='XXX')
   >>> parser.parse_args('X --foo Y'.split())
   Namespace(bar='X', foo='Y')
   >>> parser.print_help()
   usage:  [-h] [--foo YYY] XXX

   positional arguments:
    XXX

   optional arguments:
    -h, --help  show this help message and exit
    --foo YYY

Note that ``metavar`` only changes the *displayed* name - the name of the
attribute on the :meth:`parse_args` object is still determined by the dest_
value.

Different values of ``nargs`` may cause the metavar to be used multiple times.
Providing a tuple to ``metavar`` specifies a different display for each of the
arguments::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-x', nargs=2)
   >>> parser.add_argument('--foo', nargs=2, metavar=('bar', 'baz'))
   >>> parser.print_help()
   usage: PROG [-h] [-x X X] [--foo bar baz]

   optional arguments:
    -h, --help     show this help message and exit
    -x X X
    --foo bar baz


dest
^^^^

Most :class:`ArgumentParser` actions add some value as an attribute of the
object returned by :meth:`parse_args`.  The name of this attribute is determined
by the ``dest`` keyword argument of :meth:`add_argument`.  For positional
argument actions, ``dest`` is normally supplied as the first argument to
:meth:`add_argument`::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('bar')
   >>> parser.parse_args('XXX'.split())
   Namespace(bar='XXX')

For optional argument actions, the value of ``dest`` is normally inferred from
the option strings.  :class:`ArgumentParser` generates the value of ``dest`` by
taking the first long option string and stripping away the initial ``'--'``
string.  If no long option strings were supplied, ``dest`` will be derived from
the first short option string by stripping the initial ``'-'`` character.  Any
internal ``'-'`` characters will be converted to ``'_'`` characters to make sure
the string is a valid attribute name.  The examples below illustrate this
behavior::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('-f', '--foo-bar', '--foo')
   >>> parser.add_argument('-x', '-y')
   >>> parser.parse_args('-f 1 -x 2'.split())
   Namespace(foo_bar='1', x='2')
   >>> parser.parse_args('--foo 1 -y 2'.split())
   Namespace(foo_bar='1', x='2')

``dest`` allows a custom attribute name to be provided::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo', dest='bar')
   >>> parser.parse_args('--foo XXX'.split())
   Namespace(bar='XXX')


The parse_args() method
-----------------------

.. method:: ArgumentParser.parse_args(args=None, namespace=None)

   Convert argument strings to objects and assign them as attributes of the
   namespace.  Return the populated namespace.

   Previous calls to :meth:`add_argument` determine exactly what objects are
   created and how they are assigned. See the documentation for
   :meth:`add_argument` for details.

   By default, the arg strings are taken from :data:`sys.argv`, and a new empty
   :class:`Namespace` object is created for the attributes.


Option value syntax
^^^^^^^^^^^^^^^^^^^

The :meth:`parse_args` method supports several ways of specifying the value of
an option (if it takes one).  In the simplest case, the option and its value are
passed as two separate arguments::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-x')
   >>> parser.add_argument('--foo')
   >>> parser.parse_args('-x X'.split())
   Namespace(foo=None, x='X')
   >>> parser.parse_args('--foo FOO'.split())
   Namespace(foo='FOO', x=None)

For long options (options with names longer than a single character), the option
and value can also be passed as a single command-line argument, using ``=`` to
separate them::

   >>> parser.parse_args('--foo=FOO'.split())
   Namespace(foo='FOO', x=None)

For short options (options only one character long), the option and its value
can be concatenated::

   >>> parser.parse_args('-xX'.split())
   Namespace(foo=None, x='X')

Several short options can be joined together, using only a single ``-`` prefix,
as long as only the last option (or none of them) requires a value::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-x', action='store_true')
   >>> parser.add_argument('-y', action='store_true')
   >>> parser.add_argument('-z')
   >>> parser.parse_args('-xyzZ'.split())
   Namespace(x=True, y=True, z='Z')


Invalid arguments
^^^^^^^^^^^^^^^^^

While parsing the command line, ``parse_args`` checks for a variety of errors,
including ambiguous options, invalid types, invalid options, wrong number of
positional arguments, etc.  When it encounters such an error, it exits and
prints the error along with a usage message::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('--foo', type=int)
   >>> parser.add_argument('bar', nargs='?')

   >>> # invalid type
   >>> parser.parse_args(['--foo', 'spam'])
   usage: PROG [-h] [--foo FOO] [bar]
   PROG: error: argument --foo: invalid int value: 'spam'

   >>> # invalid option
   >>> parser.parse_args(['--bar'])
   usage: PROG [-h] [--foo FOO] [bar]
   PROG: error: no such option: --bar

   >>> # wrong number of arguments
   >>> parser.parse_args(['spam', 'badger'])
   usage: PROG [-h] [--foo FOO] [bar]
   PROG: error: extra arguments found: badger


Arguments containing ``"-"``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``parse_args`` method attempts to give errors whenever the user has clearly
made a mistake, but some situations are inherently ambiguous.  For example, the
command-line arg ``'-1'`` could either be an attempt to specify an option or an
attempt to provide a positional argument.  The ``parse_args`` method is cautious
here: positional arguments may only begin with ``'-'`` if they look like
negative numbers and there are no options in the parser that look like negative
numbers::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-x')
   >>> parser.add_argument('foo', nargs='?')

   >>> # no negative number options, so -1 is a positional argument
   >>> parser.parse_args(['-x', '-1'])
   Namespace(foo=None, x='-1')

   >>> # no negative number options, so -1 and -5 are positional arguments
   >>> parser.parse_args(['-x', '-1', '-5'])
   Namespace(foo='-5', x='-1')

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-1', dest='one')
   >>> parser.add_argument('foo', nargs='?')

   >>> # negative number options present, so -1 is an option
   >>> parser.parse_args(['-1', 'X'])
   Namespace(foo=None, one='X')

   >>> # negative number options present, so -2 is an option
   >>> parser.parse_args(['-2'])
   usage: PROG [-h] [-1 ONE] [foo]
   PROG: error: no such option: -2

   >>> # negative number options present, so both -1s are options
   >>> parser.parse_args(['-1', '-1'])
   usage: PROG [-h] [-1 ONE] [foo]
   PROG: error: argument -1: expected one argument

If you have positional arguments that must begin with ``'-'`` and don't look
like negative numbers, you can insert the pseudo-argument ``'--'`` which tells
``parse_args`` that everything after that is a positional argument::

   >>> parser.parse_args(['--', '-f'])
   Namespace(foo='-f', one=None)


Argument abbreviations
^^^^^^^^^^^^^^^^^^^^^^

The :meth:`parse_args` method allows long options to be abbreviated if the
abbreviation is unambiguous::

   >>> parser = argparse.ArgumentParser(prog='PROG')
   >>> parser.add_argument('-bacon')
   >>> parser.add_argument('-badger')
   >>> parser.parse_args('-bac MMM'.split())
   Namespace(bacon='MMM', badger=None)
   >>> parser.parse_args('-bad WOOD'.split())
   Namespace(bacon=None, badger='WOOD')
   >>> parser.parse_args('-ba BA'.split())
   usage: PROG [-h] [-bacon BACON] [-badger BADGER]
   PROG: error: ambiguous option: -ba could match -badger, -bacon

An error is produced for arguments that could produce more than one options.


Beyond ``sys.argv``
^^^^^^^^^^^^^^^^^^^

Sometimes it may be useful to have an ArgumentParser parse args other than those
of :data:`sys.argv`.  This can be accomplished by passing a list of strings to
``parse_args``.  This is useful for testing at the interactive prompt::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument(
   ...     'integers', metavar='int', type=int, choices=range(10),
   ...  nargs='+', help='an integer in the range 0..9')
   >>> parser.add_argument(
   ...     '--sum', dest='accumulate', action='store_const', const=sum,
   ...   default=max, help='sum the integers (default: find the max)')
   >>> parser.parse_args(['1', '2', '3', '4'])
   Namespace(accumulate=<built-in function max>, integers=[1, 2, 3, 4])
   >>> parser.parse_args('1 2 3 4 --sum'.split())
   Namespace(accumulate=<built-in function sum>, integers=[1, 2, 3, 4])


The Namespace object
^^^^^^^^^^^^^^^^^^^^

By default, :meth:`parse_args` will return a new object of type :class:`Namespace`
where the necessary attributes have been set. This class is deliberately simple,
just an :class:`object` subclass with a readable string representation. If you
prefer to have dict-like view of the attributes, you can use the standard Python
idiom via :func:`vars`::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo')
   >>> args = parser.parse_args(['--foo', 'BAR'])
   >>> vars(args)
   {'foo': 'BAR'}

It may also be useful to have an :class:`ArgumentParser` assign attributes to an
already existing object, rather than a new :class:`Namespace` object.  This can
be achieved by specifying the ``namespace=`` keyword argument::

   >>> class C:
   ...     pass
   ...
   >>> c = C()
   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo')
   >>> parser.parse_args(args=['--foo', 'BAR'], namespace=c)
   >>> c.foo
   'BAR'


Other utilities
---------------

Sub-commands
^^^^^^^^^^^^

.. method:: ArgumentParser.add_subparsers()

   Many programs split up their functionality into a number of sub-commands,
   for example, the ``svn`` program can invoke sub-commands like ``svn
   checkout``, ``svn update``, and ``svn commit``.  Splitting up functionality
   this way can be a particularly good idea when a program performs several
   different functions which require different kinds of command-line arguments.
   :class:`ArgumentParser` supports the creation of such sub-commands with the
   :meth:`add_subparsers` method.  The :meth:`add_subparsers` method is normally
   called with no arguments and returns an special action object.  This object
   has a single method, ``add_parser``, which takes a command name and any
   :class:`ArgumentParser` constructor arguments, and returns an
   :class:`ArgumentParser` object that can be modified as usual.

   Some example usage::

     >>> # create the top-level parser
     >>> parser = argparse.ArgumentParser(prog='PROG')
     >>> parser.add_argument('--foo', action='store_true', help='foo help')
     >>> subparsers = parser.add_subparsers(help='sub-command help')
     >>>
     >>> # create the parser for the "a" command
     >>> parser_a = subparsers.add_parser('a', help='a help')
     >>> parser_a.add_argument('bar', type=int, help='bar help')
     >>>
     >>> # create the parser for the "b" command
     >>> parser_b = subparsers.add_parser('b', help='b help')
     >>> parser_b.add_argument('--baz', choices='XYZ', help='baz help')
     >>>
     >>> # parse some arg lists
     >>> parser.parse_args(['a', '12'])
     Namespace(bar=12, foo=False)
     >>> parser.parse_args(['--foo', 'b', '--baz', 'Z'])
     Namespace(baz='Z', foo=True)

   Note that the object returned by :meth:`parse_args` will only contain
   attributes for the main parser and the subparser that was selected by the
   command line (and not any other subparsers).  So in the example above, when
   the ``"a"`` command is specified, only the ``foo`` and ``bar`` attributes are
   present, and when the ``"b"`` command is specified, only the ``foo`` and
   ``baz`` attributes are present.

   Similarly, when a help message is requested from a subparser, only the help
   for that particular parser will be printed.  The help message will not
   include parent parser or sibling parser messages.  (A help message for each
   subparser command, however, can be given by supplying the ``help=`` argument
   to ``add_parser`` as above.)

   ::

     >>> parser.parse_args(['--help'])
     usage: PROG [-h] [--foo] {a,b} ...

     positional arguments:
       {a,b}   sub-command help
     a     a help
     b     b help

     optional arguments:
       -h, --help  show this help message and exit
       --foo   foo help

     >>> parser.parse_args(['a', '--help'])
     usage: PROG a [-h] bar

     positional arguments:
       bar     bar help

     optional arguments:
       -h, --help  show this help message and exit

     >>> parser.parse_args(['b', '--help'])
     usage: PROG b [-h] [--baz {X,Y,Z}]

     optional arguments:
       -h, --help     show this help message and exit
       --baz {X,Y,Z}  baz help

   The :meth:`add_subparsers` method also supports ``title`` and ``description``
   keyword arguments.  When either is present, the subparser's commands will
   appear in their own group in the help output.  For example::

     >>> parser = argparse.ArgumentParser()
     >>> subparsers = parser.add_subparsers(title='subcommands',
     ...                                    description='valid subcommands',
     ...                                    help='additional help')
     >>> subparsers.add_parser('foo')
     >>> subparsers.add_parser('bar')
     >>> parser.parse_args(['-h'])
     usage:  [-h] {foo,bar} ...

     optional arguments:
       -h, --help  show this help message and exit

     subcommands:
       valid subcommands

       {foo,bar}   additional help

   Furthermore, ``add_parser`` supports an additional ``aliases`` argument,
   which allows multiple strings to refer to the same subparser. This example,
   like ``svn``, aliases ``co`` as a shorthand for ``checkout``::

     >>> parser = argparse.ArgumentParser()
     >>> subparsers = parser.add_subparsers()
     >>> checkout = subparsers.add_parser('checkout', aliases=['co'])
     >>> checkout.add_argument('foo')
     >>> parser.parse_args(['co', 'bar'])
     Namespace(foo='bar')

   One particularly effective way of handling sub-commands is to combine the use
   of the :meth:`add_subparsers` method with calls to :meth:`set_defaults` so
   that each subparser knows which Python function it should execute.  For
   example::

     >>> # sub-command functions
     >>> def foo(args):
     ...     print(args.x * args.y)
     ...
     >>> def bar(args):
     ...     print('((%s))' % args.z)
     ...
     >>> # create the top-level parser
     >>> parser = argparse.ArgumentParser()
     >>> subparsers = parser.add_subparsers()
     >>>
     >>> # create the parser for the "foo" command
     >>> parser_foo = subparsers.add_parser('foo')
     >>> parser_foo.add_argument('-x', type=int, default=1)
     >>> parser_foo.add_argument('y', type=float)
     >>> parser_foo.set_defaults(func=foo)
     >>>
     >>> # create the parser for the "bar" command
     >>> parser_bar = subparsers.add_parser('bar')
     >>> parser_bar.add_argument('z')
     >>> parser_bar.set_defaults(func=bar)
     >>>
     >>> # parse the args and call whatever function was selected
     >>> args = parser.parse_args('foo 1 -x 2'.split())
     >>> args.func(args)
     2.0
     >>>
     >>> # parse the args and call whatever function was selected
     >>> args = parser.parse_args('bar XYZYX'.split())
     >>> args.func(args)
     ((XYZYX))

   This way, you can let :meth:`parse_args` do the job of calling the
   appropriate function after argument parsing is complete.  Associating
   functions with actions like this is typically the easiest way to handle the
   different actions for each of your subparsers.  However, if it is necessary
   to check the name of the subparser that was invoked, the ``dest`` keyword
   argument to the :meth:`add_subparsers` call will work::

     >>> parser = argparse.ArgumentParser()
     >>> subparsers = parser.add_subparsers(dest='subparser_name')
     >>> subparser1 = subparsers.add_parser('1')
     >>> subparser1.add_argument('-x')
     >>> subparser2 = subparsers.add_parser('2')
     >>> subparser2.add_argument('y')
     >>> parser.parse_args(['2', 'frobble'])
     Namespace(subparser_name='2', y='frobble')


FileType objects
^^^^^^^^^^^^^^^^

.. class:: FileType(mode='r', bufsize=None)

   The :class:`FileType` factory creates objects that can be passed to the type
   argument of :meth:`ArgumentParser.add_argument`.  Arguments that have
   :class:`FileType` objects as their type will open command-line args as files
   with the requested modes and buffer sizes:

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--output', type=argparse.FileType('wb', 0))
   >>> parser.parse_args(['--output', 'out'])
   Namespace(output=<_io.BufferedWriter name='out'>)

   FileType objects understand the pseudo-argument ``'-'`` and automatically
   convert this into ``sys.stdin`` for readable :class:`FileType` objects and
   ``sys.stdout`` for writable :class:`FileType` objects:

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('infile', type=argparse.FileType('r'))
   >>> parser.parse_args(['-'])
   Namespace(infile=<_io.TextIOWrapper name='<stdin>' encoding='UTF-8'>)


Argument groups
^^^^^^^^^^^^^^^

.. method:: ArgumentParser.add_argument_group(title=None, description=None)

   By default, :class:`ArgumentParser` groups command-line arguments into
   "positional arguments" and "optional arguments" when displaying help
   messages. When there is a better conceptual grouping of arguments than this
   default one, appropriate groups can be created using the
   :meth:`add_argument_group` method::

     >>> parser = argparse.ArgumentParser(prog='PROG', add_help=False)
     >>> group = parser.add_argument_group('group')
     >>> group.add_argument('--foo', help='foo help')
     >>> group.add_argument('bar', help='bar help')
     >>> parser.print_help()
     usage: PROG [--foo FOO] bar

     group:
       bar    bar help
       --foo FOO  foo help

   The :meth:`add_argument_group` method returns an argument group object which
   has an :meth:`~ArgumentParser.add_argument` method just like a regular
   :class:`ArgumentParser`.  When an argument is added to the group, the parser
   treats it just like a normal argument, but displays the argument in a
   separate group for help messages.  The :meth:`add_argument_group` method
   accepts *title* and *description* arguments which can be used to
   customize this display::

     >>> parser = argparse.ArgumentParser(prog='PROG', add_help=False)
     >>> group1 = parser.add_argument_group('group1', 'group1 description')
     >>> group1.add_argument('foo', help='foo help')
     >>> group2 = parser.add_argument_group('group2', 'group2 description')
     >>> group2.add_argument('--bar', help='bar help')
     >>> parser.print_help()
     usage: PROG [--bar BAR] foo

     group1:
       group1 description

       foo    foo help

     group2:
       group2 description

       --bar BAR  bar help

   Note that any arguments not your user defined groups will end up back in the
   usual "positional arguments" and "optional arguments" sections.


Mutual exclusion
^^^^^^^^^^^^^^^^

.. method:: add_mutually_exclusive_group(required=False)

   Create a mutually exclusive group. argparse will make sure that only one of
   the arguments in the mutually exclusive group was present on the command
   line::

     >>> parser = argparse.ArgumentParser(prog='PROG')
     >>> group = parser.add_mutually_exclusive_group()
     >>> group.add_argument('--foo', action='store_true')
     >>> group.add_argument('--bar', action='store_false')
     >>> parser.parse_args(['--foo'])
     Namespace(bar=True, foo=True)
     >>> parser.parse_args(['--bar'])
     Namespace(bar=False, foo=False)
     >>> parser.parse_args(['--foo', '--bar'])
     usage: PROG [-h] [--foo | --bar]
     PROG: error: argument --bar: not allowed with argument --foo

   The :meth:`add_mutually_exclusive_group` method also accepts a *required*
   argument, to indicate that at least one of the mutually exclusive arguments
   is required::

     >>> parser = argparse.ArgumentParser(prog='PROG')
     >>> group = parser.add_mutually_exclusive_group(required=True)
     >>> group.add_argument('--foo', action='store_true')
     >>> group.add_argument('--bar', action='store_false')
     >>> parser.parse_args([])
     usage: PROG [-h] (--foo | --bar)
     PROG: error: one of the arguments --foo --bar is required

   Note that currently mutually exclusive argument groups do not support the
   *title* and *description* arguments of :meth:`add_argument_group`.


Parser defaults
^^^^^^^^^^^^^^^

.. method:: ArgumentParser.set_defaults(**kwargs)

   Most of the time, the attributes of the object returned by :meth:`parse_args`
   will be fully determined by inspecting the command-line args and the argument
   actions.  :meth:`ArgumentParser.set_defaults` allows some additional
   attributes that are determined without any inspection of the command line to
   be added::

     >>> parser = argparse.ArgumentParser()
     >>> parser.add_argument('foo', type=int)
     >>> parser.set_defaults(bar=42, baz='badger')
     >>> parser.parse_args(['736'])
     Namespace(bar=42, baz='badger', foo=736)

   Note that parser-level defaults always override argument-level defaults::

     >>> parser = argparse.ArgumentParser()
     >>> parser.add_argument('--foo', default='bar')
     >>> parser.set_defaults(foo='spam')
     >>> parser.parse_args([])
     Namespace(foo='spam')

   Parser-level defaults can be particularly useful when working with multiple
   parsers.  See the :meth:`~ArgumentParser.add_subparsers` method for an
   example of this type.

.. method:: ArgumentParser.get_default(dest)

   Get the default value for a namespace attribute, as set by either
   :meth:`~ArgumentParser.add_argument` or by
   :meth:`~ArgumentParser.set_defaults`::

     >>> parser = argparse.ArgumentParser()
     >>> parser.add_argument('--foo', default='badger')
     >>> parser.get_default('foo')
     'badger'


Printing help
^^^^^^^^^^^^^

In most typical applications, :meth:`parse_args` will take care of formatting
and printing any usage or error messages.  However, several formatting methods
are available:

.. method:: ArgumentParser.print_usage(file=None)

   Print a brief description of how the :class:`ArgumentParser` should be
   invoked on the command line.  If *file* is ``None``, :data:`sys.stdout` is
   assumed.

.. method:: ArgumentParser.print_help(file=None)

   Print a help message, including the program usage and information about the
   arguments registered with the :class:`ArgumentParser`.  If *file* is
   ``None``, :data:`sys.stdout` is assumed.

There are also variants of these methods that simply return a string instead of
printing it:

.. method:: ArgumentParser.format_usage()

   Return a string containing a brief description of how the
   :class:`ArgumentParser` should be invoked on the command line.

.. method:: ArgumentParser.format_help()

   Return a string containing a help message, including the program usage and
   information about the arguments registered with the :class:`ArgumentParser`.


Partial parsing
^^^^^^^^^^^^^^^

.. method:: ArgumentParser.parse_known_args(args=None, namespace=None)

Sometimes a script may only parse a few of the command-line arguments, passing
the remaining arguments on to another script or program. In these cases, the
:meth:`parse_known_args` method can be useful.  It works much like
:meth:`~ArgumentParser.parse_args` except that it does not produce an error when
extra arguments are present.  Instead, it returns a two item tuple containing
the populated namespace and the list of remaining argument strings.

::

   >>> parser = argparse.ArgumentParser()
   >>> parser.add_argument('--foo', action='store_true')
   >>> parser.add_argument('bar')
   >>> parser.parse_known_args(['--foo', '--badger', 'BAR', 'spam'])
   (Namespace(bar='BAR', foo=True), ['--badger', 'spam'])


Customizing file parsing
^^^^^^^^^^^^^^^^^^^^^^^^

.. method:: ArgumentParser.convert_arg_line_to_args(arg_line)

   Arguments that are read from a file (see the *fromfile_prefix_chars*
   keyword argument to the :class:`ArgumentParser` constructor) are read one
   argument per line. :meth:`convert_arg_line_to_args` can be overriden for
   fancier reading.

   This method takes a single argument *arg_line* which is a string read from
   the argument file.  It returns a list of arguments parsed from this string.
   The method is called once per line read from the argument file, in order.

   A useful override of this method is one that treats each space-separated word
   as an argument::

    def convert_arg_line_to_args(self, arg_line):
        for arg in arg_line.split():
            if not arg.strip():
                continue
            yield arg


Exiting methods
^^^^^^^^^^^^^^^

.. method:: ArgumentParser.exit(status=0, message=None)

   This method terminates the program, exiting with the specified *status*
   and, if given, it prints a *message* before that.

.. method:: ArgumentParser.error(message)

   This method prints a usage message including the *message* to the
   standard output and terminates the program with a status code of 2.

.. _upgrading-optparse-code:

Upgrading optparse code
-----------------------

Originally, the argparse module had attempted to maintain compatibility with
optparse.  However, optparse was difficult to extend transparently, particularly
with the changes required to support the new ``nargs=`` specifiers and better
usage messages.  When most everything in optparse had either been copy-pasted
over or monkey-patched, it no longer seemed practical to try to maintain the
backwards compatibility.

A partial upgrade path from optparse to argparse:

* Replace all ``add_option()`` calls with :meth:`ArgumentParser.add_argument`
  calls.

* Replace ``options, args = parser.parse_args()`` with ``args =
  parser.parse_args()`` and add additional :meth:`ArgumentParser.add_argument`
  calls for the positional arguments.

* Replace callback actions and the ``callback_*`` keyword arguments with
  ``type`` or ``action`` arguments.

* Replace string names for ``type`` keyword arguments with the corresponding
  type objects (e.g. int, float, complex, etc).

* Replace :class:`optparse.Values` with :class:`Namespace` and
  :exc:`optparse.OptionError` and :exc:`optparse.OptionValueError` with
  :exc:`ArgumentError`.

* Replace strings with implicit arguments such as ``%default`` or ``%prog`` with
  the standard python syntax to use dictionaries to format strings, that is,
  ``%(default)s`` and ``%(prog)s``.

* Replace the OptionParser constructor ``version`` argument with a call to
  ``parser.add_argument('--version', action='version', version='<the version>')``
