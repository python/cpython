"""Object-oriented interface to the parser module.

This module exports four classes which together provide an interface
to the parser module.  Together, the three classes represent two ways
to create parsed representations of Python source and the two starting
data types (source text and tuple representations).  Each class
provides interfaces which are identical other than the constructors.
The constructors are described in detail in the documentation for each
class and the remaining, shared portion of the interface is documented
below.  Briefly, the classes provided are:

AST
    Defines the primary interface to the AST objects and supports creation
    from the tuple representation of the parse tree.

ExpressionAST
    Supports creation of expression constructs from source text.

SuiteAST
    Supports creation of statement suites from source text.

FileSuiteAST
    Convenience subclass of the `SuiteAST' class; loads source text of the
    suite from an external file.

Common Methods
--------------

Aside from the constructors, several methods are provided to allow
access to the various interpretations of the parse tree and to check
conditions of the construct represented by the parse tree.

ast()
    Returns the corresponding `parser.ASTType' object.

code()
    Returns the compiled code object.

filename()
    Returns the name of the associated source file, if known.

isExpression()
    Returns true value if parse tree represents an expression, or a false
    value otherwise.

isSuite()
    Returns true value if parse tree represents a suite of statements, or
    a false value otherwise.

text()
    Returns the source text, or None if not available.

tuple()
    Returns the tuple representing the parse tree.
"""

__version__ = '$Revision$'
__copyright__ = """Copyright (c) 1995, 1996, 1997 by Fred L. Drake, Jr.

This software may be used and distributed freely for any purpose provided
that this notice is included unchanged on any and all copies.  The author
does not warrant or guarantee this software in any way.
"""

import parser


class AST:
    """Base class for Abstract Syntax Tree objects.

    Creates an Abstract Syntax Tree based on the tuple representation
    of the parse tree.  The parse tree can represent either an
    expression or a suite; which will be recognized automatically.
    This base class provides all of the query methods for subclass
    objects defined in this module.
    """
    _text = None
    _code = None
    _ast  = None
    _type = 'unknown'
    _tupl = None

    def __init__(self, tuple):
	"""Create an `AST' instance from a tuple-tree representation.

	tuple
	    The tuple tree to convert.

	The tuple-tree may represent either an expression or a suite; the
	type will be determined automatically.  Line number information may
	optionally be present for any subset of the terminal tokens.
	"""
	if type(tuple) is not type(()):
	    raise TypeError, 'Base AST class requires tuple parameter.'

	self._tupl = tuple
	self._ast  = parser.tuple2ast(tuple)
	self._type = (parser.isexpr(self._ast) and 'expression') or 'suite'

    def list(self, line_info = 0):
	"""Returns a fresh list representing the parse tree.

	line_info
	    If true, includes line number information for terminal tokens in
	    the output data structure,
	"""
	return parser.ast2list(self._ast, line_info)

    def tuple(self, line_info = 0):
	"""Returns the tuple representing the parse tree.

	line_info
	    If true, includes line number information for terminal tokens in
	    the output data structure,
	"""
	if self._tupl is None:
	    self._tupl = parser.ast2tuple(self._ast, line_info)
	return self._tupl

    def code(self):
	"""Returns the compiled code object.

	The code object returned by this method may be passed to the
	exec statement if `AST.isSuite()' is true or to the eval()
	function if `AST.isExpression()' is true.  All the usual rules
	regarding execution of code objects apply.
	"""
	if not self._code:
	    self._code = parser.compileast(self._ast)
	return self._code

    def ast(self):
	"""Returns the corresponding `parser.ASTType' object.
	"""
	return self._ast

    def filename(self):
	"""Returns the name of the source file if known, or None.
	"""
	return None

    def text(self):
	"""Returns the source text, or None if not available.

	If the instance is of class `AST', None is returned since no
	source text is available.  If of class `ExpressionAST' or
	`SuiteAST', the source text passed to the constructor is
	returned.
	"""
	return self._text

    def isSuite(self):
	"""Determine if `AST' instance represents a suite of statements.
	"""
	return self._type == 'suite'

    def isExpression(self):
	"""Determine if `AST' instance represents an expression.
	"""
	return self._type == 'expression'



class SuiteAST(AST):
    """Statement suite parse tree representation.

    This subclass of the `AST' base class represents statement suites
    parsed from the source text of a Python suite.  If the source text
    does not represent a parsable suite of statements, the appropriate
    exception is raised by the parser.
    """
    _type = 'suite'

    def __init__(self, text):
	"""Initialize a `SuiteAST' from source text.

	text
	    Source text to parse.
	"""
	if type(text) is not type(''):
	    raise TypeError, 'SuiteAST requires source text parameter.'
	self._text = text
	self._ast  = parser.suite(text)

    def isSuite(self):
	return 1

    def isExpression(self):
	return 0


class FileSuiteAST(SuiteAST):
    """Representation of a python source file syntax tree.

    This provides a convenience wrapper around the `SuiteAST' class to
    load the source text from an external file.
    """
    def __init__(self, fileName):
	"""Initialize a `SuiteAST' from a source file.

	fileName
	    Name of the external source file.
	"""
	self._fileName = fileName
	SuiteAST.__init__(self, open(fileName).read())

    def filename(self):
	return self._fileName



class ExpressionAST(AST):
    """Expression parse tree representation.

    This subclass of the `AST' base class represents expression
    constructs parsed from the source text of a Python expression.  If
    the source text does not represent a parsable expression, the
    appropriate exception is raised by the Python parser.
    """
    _type = 'expression'

    def __init__(self, text):
	"""Initialize an expression AST from source text.

	text
	    Source text to parse.
	"""
	if type(text) is not type(''):
	    raise TypeError, 'ExpressionAST requires source text parameter.'
	self._text = text
	self._ast  = parser.expr(text)

    def isSuite(self):
	return 0

    def isExpression(self):
	return 1


#
#  end of file
