
.. _compiler:

***********************
Python compiler package
***********************

.. sectionauthor:: Jeremy Hylton <jeremy@zope.com>


The Python compiler package is a tool for analyzing Python source code and
generating Python bytecode.  The compiler contains libraries to generate an
abstract syntax tree from Python source code and to generate Python
:term:`bytecode` from the tree.

The :mod:`compiler` package is a Python source to bytecode translator written in
Python.  It uses the built-in parser and standard :mod:`parser` module to
generated a concrete syntax tree.  This tree is used to generate an abstract
syntax tree (AST) and then Python bytecode.

The full functionality of the package duplicates the builtin compiler provided
with the Python interpreter.  It is intended to match its behavior almost
exactly.  Why implement another compiler that does the same thing?  The package
is useful for a variety of purposes.  It can be modified more easily than the
builtin compiler.  The AST it generates is useful for analyzing Python source
code.

This chapter explains how the various components of the :mod:`compiler` package
work.  It blends reference material with a tutorial.

The following modules are part of the :mod:`compiler` package:

.. toctree::

   _ast.rst


The basic interface
===================

.. module:: compiler
   :synopsis: Python code compiler written in Python.


The top-level of the package defines four functions.  If you import
:mod:`compiler`, you will get these functions and a collection of modules
contained in the package.


.. function:: parse(buf)

   Returns an abstract syntax tree for the Python source code in *buf*. The
   function raises :exc:`SyntaxError` if there is an error in the source code.  The
   return value is a :class:`compiler.ast.Module` instance that contains the tree.


.. function:: parseFile(path)

   Return an abstract syntax tree for the Python source code in the file specified
   by *path*.  It is equivalent to ``parse(open(path).read())``.


.. function:: walk(ast, visitor[, verbose])

   Do a pre-order walk over the abstract syntax tree *ast*.  Call the appropriate
   method on the *visitor* instance for each node encountered.


.. function:: compile(source, filename, mode, flags=None,  dont_inherit=None)

   Compile the string *source*, a Python module, statement or expression, into a
   code object that can be executed by the exec statement or :func:`eval`. This
   function is a replacement for the built-in :func:`compile` function.

   The *filename* will be used for run-time error messages.

   The *mode* must be 'exec' to compile a module, 'single' to compile a single
   (interactive) statement, or 'eval' to compile an expression.

   The *flags* and *dont_inherit* arguments affect future-related statements, but
   are not supported yet.


.. function:: compileFile(source)

   Compiles the file *source* and generates a .pyc file.

The :mod:`compiler` package contains the following modules: :mod:`ast`,
:mod:`consts`, :mod:`future`, :mod:`misc`, :mod:`pyassem`, :mod:`pycodegen`,
:mod:`symbols`, :mod:`transformer`, and :mod:`visitor`.


Limitations
===========

There are some problems with the error checking of the compiler package.  The
interpreter detects syntax errors in two distinct phases.  One set of errors is
detected by the interpreter's parser, the other set by the compiler.  The
compiler package relies on the interpreter's parser, so it get the first phases
of error checking for free.  It implements the second phase itself, and that
implementation is incomplete.  For example, the compiler package does not raise
an error if a name appears more than once in an argument list:  ``def f(x, x):
...``

A future version of the compiler should fix these problems.


Python Abstract Syntax
======================

The :mod:`compiler.ast` module defines an abstract syntax for Python.  In the
abstract syntax tree, each node represents a syntactic construct.  The root of
the tree is :class:`Module` object.

The abstract syntax offers a higher level interface to parsed Python source
code.  The :mod:`parser` module and the compiler written in C for the Python
interpreter use a concrete syntax tree.  The concrete syntax is tied closely to
the grammar description used for the Python parser.  Instead of a single node
for a construct, there are often several levels of nested nodes that are
introduced by Python's precedence rules.

The abstract syntax tree is created by the :mod:`compiler.transformer` module.
The transformer relies on the builtin Python parser to generate a concrete
syntax tree.  It generates an abstract syntax tree from the concrete tree.

.. index::
   single: Stein, Greg
   single: Tutt, Bill

The :mod:`transformer` module was created by Greg Stein and Bill Tutt for an
experimental Python-to-C compiler.  The current version contains a number of
modifications and improvements, but the basic form of the abstract syntax and of
the transformer are due to Stein and Tutt.


AST Nodes
---------

.. module:: compiler.ast


The :mod:`compiler.ast` module is generated from a text file that describes each
node type and its elements.  Each node type is represented as a class that
inherits from the abstract base class :class:`compiler.ast.Node` and defines a
set of named attributes for child nodes.


.. class:: Node()

   The :class:`Node` instances are created automatically by the parser generator.
   The recommended interface for specific :class:`Node` instances is to use the
   public attributes to access child nodes.  A public attribute may be bound to a
   single node or to a sequence of nodes, depending on the :class:`Node` type.  For
   example, the :attr:`bases` attribute of the :class:`Class` node, is bound to a
   list of base class nodes, and the :attr:`doc` attribute is bound to a single
   node.

   Each :class:`Node` instance has a :attr:`lineno` attribute which may be
   ``None``.  XXX Not sure what the rules are for which nodes will have a useful
   lineno.

All :class:`Node` objects offer the following methods:


.. method:: Node.getChildren()

   Returns a flattened list of the child nodes and objects in the order they occur.
   Specifically, the order of the nodes is the order in which they appear in the
   Python grammar.  Not all of the children are :class:`Node` instances.  The names
   of functions and classes, for example, are plain strings.


.. method:: Node.getChildNodes()

   Returns a flattened list of the child nodes in the order they occur.  This
   method is like :meth:`getChildren`, except that it only returns those children
   that are :class:`Node` instances.

Two examples illustrate the general structure of :class:`Node` classes.  The
:keyword:`while` statement is defined by the following grammar production::

   while_stmt:     "while" expression ":" suite
                  ["else" ":" suite]

The :class:`While` node has three attributes: :attr:`test`, :attr:`body`, and
:attr:`else_`.  (If the natural name for an attribute is also a Python reserved
word, it can't be used as an attribute name.  An underscore is appended to the
word to make it a legal identifier, hence :attr:`else_` instead of
:keyword:`else`.)

The :keyword:`if` statement is more complicated because it can include several
tests.   ::

   if_stmt: 'if' test ':' suite ('elif' test ':' suite)* ['else' ':' suite]

The :class:`If` node only defines two attributes: :attr:`tests` and
:attr:`else_`.  The :attr:`tests` attribute is a sequence of test expression,
consequent body pairs.  There is one pair for each :keyword:`if`/:keyword:`elif`
clause.  The first element of the pair is the test expression.  The second
elements is a :class:`Stmt` node that contains the code to execute if the test
is true.

The :meth:`getChildren` method of :class:`If` returns a flat list of child
nodes.  If there are three :keyword:`if`/:keyword:`elif` clauses and no
:keyword:`else` clause, then :meth:`getChildren` will return a list of six
elements: the first test expression, the first :class:`Stmt`, the second text
expression, etc.

The following table lists each of the :class:`Node` subclasses defined in
:mod:`compiler.ast` and each of the public attributes available on their
instances.  The values of most of the attributes are themselves :class:`Node`
instances or sequences of instances.  When the value is something other than an
instance, the type is noted in the comment.  The attributes are listed in the
order in which they are returned by :meth:`getChildren` and
:meth:`getChildNodes`.

+-----------------------+--------------------+---------------------------------+
| Node type             | Attribute          | Value                           |
+=======================+====================+=================================+
| :class:`Add`          | :attr:`left`       | left operand                    |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      | right operand                   |
+-----------------------+--------------------+---------------------------------+
| :class:`And`          | :attr:`nodes`      | list of operands                |
+-----------------------+--------------------+---------------------------------+
| :class:`AssAttr`      |                    | *attribute as target of         |
|                       |                    | assignment*                     |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`expr`       | expression on the left-hand     |
|                       |                    | side of the dot                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`attrname`   | the attribute name, a string    |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`flags`      | XXX                             |
+-----------------------+--------------------+---------------------------------+
| :class:`AssList`      | :attr:`nodes`      | list of list elements being     |
|                       |                    | assigned to                     |
+-----------------------+--------------------+---------------------------------+
| :class:`AssName`      | :attr:`name`       | name being assigned to          |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`flags`      | XXX                             |
+-----------------------+--------------------+---------------------------------+
| :class:`AssTuple`     | :attr:`nodes`      | list of tuple elements being    |
|                       |                    | assigned to                     |
+-----------------------+--------------------+---------------------------------+
| :class:`Assert`       | :attr:`test`       | the expression to be tested     |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`fail`       | the value of the                |
|                       |                    | :exc:`AssertionError`           |
+-----------------------+--------------------+---------------------------------+
| :class:`Assign`       | :attr:`nodes`      | a list of assignment targets,   |
|                       |                    | one per equal sign              |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`expr`       | the value being assigned        |
+-----------------------+--------------------+---------------------------------+
| :class:`AugAssign`    | :attr:`node`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`op`         |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Backquote`    | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Bitand`       | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Bitor`        | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Bitxor`       | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Break`        |                    |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`CallFunc`     | :attr:`node`       | expression for the callee       |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`args`       | a list of arguments             |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`star_args`  | the extended \*-arg value       |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`dstar_args` | the extended \*\*-arg value     |
+-----------------------+--------------------+---------------------------------+
| :class:`Class`        | :attr:`name`       | the name of the class, a string |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`bases`      | a list of base classes          |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`doc`        | doc string, a string or         |
|                       |                    | ``None``                        |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`code`       | the body of the class statement |
+-----------------------+--------------------+---------------------------------+
| :class:`Compare`      | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`ops`        |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Const`        | :attr:`value`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Continue`     |                    |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Decorators`   | :attr:`nodes`      | List of function decorator      |
|                       |                    | expressions                     |
+-----------------------+--------------------+---------------------------------+
| :class:`Dict`         | :attr:`items`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Discard`      | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Div`          | :attr:`left`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Ellipsis`     |                    |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Expression`   | :attr:`node`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Exec`         | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`locals`     |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`globals`    |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`FloorDiv`     | :attr:`left`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`For`          | :attr:`assign`     |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`list`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`body`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`else_`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`From`         | :attr:`modname`    |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`names`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Function`     | :attr:`decorators` | :class:`Decorators` or ``None`` |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`name`       | name used in def, a string      |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`argnames`   | list of argument names, as      |
|                       |                    | strings                         |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`defaults`   | list of default values          |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`flags`      | xxx                             |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`doc`        | doc string, a string or         |
|                       |                    | ``None``                        |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`code`       | the body of the function        |
+-----------------------+--------------------+---------------------------------+
| :class:`GenExpr`      | :attr:`code`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`GenExprFor`   | :attr:`assign`     |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`iter`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`ifs`        |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`GenExprIf`    | :attr:`test`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`GenExprInner` | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`quals`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Getattr`      | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`attrname`   |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Global`       | :attr:`names`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`If`           | :attr:`tests`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`else_`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Import`       | :attr:`names`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Invert`       | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Keyword`      | :attr:`name`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Lambda`       | :attr:`argnames`   |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`defaults`   |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`flags`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`code`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`LeftShift`    | :attr:`left`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`List`         | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`ListComp`     | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`quals`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`ListCompFor`  | :attr:`assign`     |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`list`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`ifs`        |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`ListCompIf`   | :attr:`test`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Mod`          | :attr:`left`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Module`       | :attr:`doc`        | doc string, a string or         |
|                       |                    | ``None``                        |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`node`       | body of the module, a           |
|                       |                    | :class:`Stmt`                   |
+-----------------------+--------------------+---------------------------------+
| :class:`Mul`          | :attr:`left`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Name`         | :attr:`name`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Not`          | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Or`           | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Pass`         |                    |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Power`        | :attr:`left`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Print`        | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`dest`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Printnl`      | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`dest`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Raise`        | :attr:`expr1`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`expr2`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`expr3`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Return`       | :attr:`value`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`RightShift`   | :attr:`left`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Slice`        | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`flags`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`lower`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`upper`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Sliceobj`     | :attr:`nodes`      | list of statements              |
+-----------------------+--------------------+---------------------------------+
| :class:`Stmt`         | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Sub`          | :attr:`left`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`right`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Subscript`    | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`flags`      |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`subs`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`TryExcept`    | :attr:`body`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`handlers`   |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`else_`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`TryFinally`   | :attr:`body`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`final`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Tuple`        | :attr:`nodes`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`UnaryAdd`     | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`UnarySub`     | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`While`        | :attr:`test`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`body`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`else_`      |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`With`         | :attr:`expr`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`vars`       |                                 |
+-----------------------+--------------------+---------------------------------+
|                       | :attr:`body`       |                                 |
+-----------------------+--------------------+---------------------------------+
| :class:`Yield`        | :attr:`value`      |                                 |
+-----------------------+--------------------+---------------------------------+


Assignment nodes
----------------

There is a collection of nodes used to represent assignments.  Each assignment
statement in the source code becomes a single :class:`Assign` node in the AST.
The :attr:`nodes` attribute is a list that contains a node for each assignment
target.  This is necessary because assignment can be chained, e.g. ``a = b =
2``. Each :class:`Node` in the list will be one of the following classes:
:class:`AssAttr`, :class:`AssList`, :class:`AssName`, or :class:`AssTuple`.

Each target assignment node will describe the kind of object being assigned to:
:class:`AssName` for a simple name, e.g. ``a = 1``. :class:`AssAttr` for an
attribute assigned, e.g. ``a.x = 1``. :class:`AssList` and :class:`AssTuple` for
list and tuple expansion respectively, e.g. ``a, b, c = a_tuple``.

The target assignment nodes also have a :attr:`flags` attribute that indicates
whether the node is being used for assignment or in a delete statement.  The
:class:`AssName` is also used to represent a delete statement, e.g. :class:`del
x`.

When an expression contains several attribute references, an assignment or
delete statement will contain only one :class:`AssAttr` node -- for the final
attribute reference.  The other attribute references will be represented as
:class:`Getattr` nodes in the :attr:`expr` attribute of the :class:`AssAttr`
instance.


Examples
--------

This section shows several simple examples of ASTs for Python source code.  The
examples demonstrate how to use the :func:`parse` function, what the repr of an
AST looks like, and how to access attributes of an AST node.

The first module defines a single function.  Assume it is stored in
:file:`/tmp/doublelib.py`.  ::

   """This is an example module.

   This is the docstring.
   """

   def double(x):
       "Return twice the argument"
       return x * 2

In the interactive interpreter session below, I have reformatted the long AST
reprs for readability.  The AST reprs use unqualified class names.  If you want
to create an instance from a repr, you must import the class names from the
:mod:`compiler.ast` module. ::

   >>> import compiler
   >>> mod = compiler.parseFile("/tmp/doublelib.py")
   >>> mod
   Module('This is an example module.\n\nThis is the docstring.\n', 
          Stmt([Function(None, 'double', ['x'], [], 0,
                         'Return twice the argument', 
                         Stmt([Return(Mul((Name('x'), Const(2))))]))]))
   >>> from compiler.ast import *
   >>> Module('This is an example module.\n\nThis is the docstring.\n', 
   ...    Stmt([Function(None, 'double', ['x'], [], 0,
   ...                   'Return twice the argument', 
   ...                   Stmt([Return(Mul((Name('x'), Const(2))))]))]))
   Module('This is an example module.\n\nThis is the docstring.\n', 
          Stmt([Function(None, 'double', ['x'], [], 0,
                         'Return twice the argument', 
                         Stmt([Return(Mul((Name('x'), Const(2))))]))]))
   >>> mod.doc
   'This is an example module.\n\nThis is the docstring.\n'
   >>> for node in mod.node.nodes:
   ...     print node
   ... 
   Function(None, 'double', ['x'], [], 0, 'Return twice the argument',
            Stmt([Return(Mul((Name('x'), Const(2))))]))
   >>> func = mod.node.nodes[0]
   >>> func.code
   Stmt([Return(Mul((Name('x'), Const(2))))])


Using Visitors to Walk ASTs
===========================

.. module:: compiler.visitor


The visitor pattern is ...  The :mod:`compiler` package uses a variant on the
visitor pattern that takes advantage of Python's introspection features to
eliminate the need for much of the visitor's infrastructure.

The classes being visited do not need to be programmed to accept visitors.  The
visitor need only define visit methods for classes it is specifically interested
in; a default visit method can handle the rest.

XXX The magic :meth:`visit` method for visitors.


.. function:: walk(tree, visitor[, verbose])


.. class:: ASTVisitor()

   The :class:`ASTVisitor` is responsible for walking over the tree in the correct
   order.  A walk begins with a call to :meth:`preorder`.  For each node, it checks
   the *visitor* argument to :meth:`preorder` for a method named 'visitNodeType,'
   where NodeType is the name of the node's class, e.g. for a :class:`While` node a
   :meth:`visitWhile` would be called.  If the method exists, it is called with the
   node as its first argument.

   The visitor method for a particular node type can control how child nodes are
   visited during the walk.  The :class:`ASTVisitor` modifies the visitor argument
   by adding a visit method to the visitor; this method can be used to visit a
   particular child node.  If no visitor is found for a particular node type, the
   :meth:`default` method is called.

:class:`ASTVisitor` objects have the following methods:

XXX describe extra arguments


.. method:: ASTVisitor.default(node[, ...])


.. method:: ASTVisitor.dispatch(node[, ...])


.. method:: ASTVisitor.preorder(tree, visitor)


Bytecode Generation
===================

The code generator is a visitor that emits bytecodes.  Each visit method can
call the :meth:`emit` method to emit a new bytecode.  The basic code generator
is specialized for modules, classes, and functions.  An assembler converts that
emitted instructions to the low-level bytecode format.  It handles things like
generator of constant lists of code objects and calculation of jump offsets.

