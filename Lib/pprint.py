#  pprint.py
#
#  Author:	Fred L. Drake, Jr.
#		fdrake@cnri.reston.va.us, fdrake@intr.net
#
#  This is a simple little module I wrote to make life easier.  I didn't
#  see anything quite like it in the library, though I may have overlooked
#  something.  I wrote this when I was trying to read some heavily nested
#  tuples with fairly non-descriptive content.  This is modelled very much
#  after Lisp/Scheme - style pretty-printing of lists.  If you find it
#  useful, thank small children who sleep at night.

"""Support to pretty-print lists, tuples, & dictionaries recursively.

Very simple, but useful, especially in debugging data structures.

Functions
---------

pformat()
    Format a Python object into a pretty-printed representation.

pprint()
    Pretty-print a list, tuple or dictionary.



Constants
---------

INDENT_PER_LEVEL
    Amount of indentation to use for each new recursive level.  The
    default is 1.  This must be a non-negative integer, and may be set
    by the caller before calling pprint().

MAX_WIDTH
    Maximum width of the display.  This is only used if the
    representation *can* be kept less than MAX_WIDTH characters wide.
    May be set by the user before calling pprint() if needed.

"""

INDENT_PER_LEVEL = 1

MAX_WIDTH = 80

from types import DictType, ListType, TupleType


def pformat(seq):
    """Format a Python object into a pretty-printed representation.

    The representation is returned with no trailing newline.

    """
    import StringIO
    sio = StringIO.StringIO()
    pprint(seq, stream=sio)
    str = sio.getvalue()
    if str and str[-1] == '\n':
	str = str[:-1]
    return str


def pprint(seq, stream=None, indent=0, allowance=0):
    """Pretty-print a list, tuple, or dictionary.

    seq
	List, tuple, or dictionary object to be pretty-printed.  Other
	object types are permitted by are not specially interpreted.

    stream
	Output stream.  If not provided, `sys.stdout' is used.  This
	parameter must support the `write()' method with a single
	parameter, which will always be a string.  It may be a
	`StringIO.StringIO' object if the result is needed as a
	string.

    Indentation is done according to `INDENT_PER_LEVEL', which may be
    set to any non-negative integer before calling this function.  The
    output written on the stream is a perfectly valid representation
    of the Python object passed in, with indentation to assist
    human-readable interpretation.  The output can be used as input
    without error, given readable representations of all elements are
    available via `repr()'.  Output is restricted to `MAX_WIDTH'
    columns where possible.

    """
    if stream is None:
	import sys
	stream = sys.stdout

    rep = `seq`
    typ = type(seq)
    sepLines = len(rep) > (MAX_WIDTH - 1 - indent - allowance)

    if sepLines and (typ is ListType or typ is TupleType):
	#  Pretty-print the sequence.
	stream.write(((typ is ListType) and '[') or '(')

	length = len(seq)
	if length:
	    indent = indent + INDENT_PER_LEVEL
	    pprint(seq[0], stream, indent, allowance + 1)

	    if len(seq) > 1:
		for ent in seq[1:]:
		    stream.write(',\n' + ' '*indent)
		    pprint(ent, stream, indent, allowance + 1)

	    indent = indent - INDENT_PER_LEVEL

	stream.write(((typ is ListType) and ']') or ')')

    elif typ is DictType and sepLines:
	stream.write('{')

	length = len(seq)
	if length:
	    indent = indent + INDENT_PER_LEVEL
	    items  = seq.items()
	    items.sort()
	    key, ent = items[0]
	    rep = `key` + ': '
	    stream.write(rep)
	    pprint(ent, stream, indent + len(rep), allowance + 1)

	    if len(items) > 1:
		for key, ent in items[1:]:
		    rep = `key` + ': '
		    stream.write(',\n' + ' '*indent + rep)
		    pprint(ent, stream, indent + len(rep), allowance + 1)

	    indent = indent - INDENT_PER_LEVEL

	stream.write('}')

    else:
	stream.write(rep)

    #  Terminate the 'print' if we're not a recursive invocation.
    if not indent:
	stream.write('\n')
