"""Simple code to extract class & function docstrings from a module.


"""

import symbol
import token
import types


def get_docs(fileName):
    """Retrieve information from the parse tree of a source file.

    fileName
	Name of the file to read Python source code from.
    """
    source = open(fileName).read()
    import os
    basename = os.path.basename(os.path.splitext(fileName)[0])
    import parser
    ast = parser.suite(source)
    tup = parser.ast2tuple(ast)
    return ModuleInfo(tup, basename)


class DefnInfo:
    _docstring = ''
    _name = ''

    def __init__(self, tree):
	self._name = tree[2][1]

    def get_docstring(self):
	return self._docstring

    def get_name(self):
	return self._name

class SuiteInfoBase(DefnInfo):
    def __init__(self):
	self._class_info = {}
	self._function_info = {}

    def get_class_names(self):
	return self._class_info.keys()

    def get_class_info(self, name):
	return self._class_info[name]

    def _extract_info(self, tree):
	if len(tree) >= 4:
	    found, vars = match(DOCSTRING_STMT_PATTERN, tree[3])
	    if found:
		self._docstring = eval(vars['docstring'])
	for node in tree[1:]:
	    if (node[0] == symbol.stmt
		and node[1][0] == symbol.compound_stmt):
		if node[1][1][0] == symbol.funcdef:
		    name = node[1][1][2][1]
		    self._function_info[name] = \
					      FunctionInfo(node[1][1])
		elif node[1][1][0] == symbol.classdef:
		    name = node[1][1][2][1]
		    self._class_info[name] = ClassInfo(node[1][1])


class SuiteInfo(SuiteInfoBase):
    def __init__(self, tree):
	SuiteInfoBase.__init__(self)
	self._extract_info(tree)

    def get_function_names(self):
	return self._function_info.keys()

    def get_function_info(self, name):
	return self._function_info[name]


class FunctionInfo(SuiteInfo):
    def __init__(self, tree):
	DefnInfo.__init__(self, tree)
	suite = tree[-1]
	if len(suite) >= 4:
	    found, vars = match(DOCSTRING_STMT_PATTERN, suite[3])
	    if found:
		self._docstring = eval(vars['docstring'])
	SuiteInfoBase.__init__(self)
	self._extract_info(suite)


class ClassInfo(SuiteInfoBase):
    def __init__(self, tree):
	SuiteInfoBase.__init__(self)
	DefnInfo.__init__(self, tree)
	self._extract_info(tree[-1])

    def get_method_names(self):
	return self._function_info.keys()

    def get_method_info(self, name):
	return self._function_info[name]


class ModuleInfo(SuiteInfo):
    def __init__(self, tree, name="<string>"):
	self._name = name
	SuiteInfo.__init__(self, tree)
	found, vars = match(DOCSTRING_STMT_PATTERN, tree[1])
	if found:
	    self._docstring = vars["docstring"]


from types import ListType, TupleType

def match(pattern, data, vars=None):
    """
    """
    if vars is None:
	vars = {}
    if type(pattern) is ListType:	# 'variables' are ['varname']
	vars[pattern[0]] = data
	return 1, vars
    if type(pattern) is not TupleType:
	return (pattern == data), vars
    if len(data) != len(pattern):
	return 0, vars
    for pattern, data in map(None, pattern, data):
	same, vars = match(pattern, data, vars)
	if not same:
	    break
    return same, vars


#  This pattern will match a 'stmt' node which *might* represent a docstring;
#  docstrings require that the statement which provides the docstring be the
#  first statement in the class or function, which this pattern does not check.
#
DOCSTRING_STMT_PATTERN = (
    symbol.stmt,
    (symbol.simple_stmt,
     (symbol.small_stmt,
      (symbol.expr_stmt,
       (symbol.testlist,
	(symbol.test,
	 (symbol.and_test,
	  (symbol.not_test,
	   (symbol.comparison,
	    (symbol.expr,
	     (symbol.xor_expr,
	      (symbol.and_expr,
	       (symbol.shift_expr,
		(symbol.arith_expr,
		 (symbol.term,
		  (symbol.factor,
		   (symbol.power,
		    (symbol.atom,
		     (token.STRING, ['docstring'])
		     )))))))))))))))),
     (token.NEWLINE, '')
     ))

#
#  end of file
