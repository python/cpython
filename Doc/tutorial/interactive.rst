.. _tut-interacting:

**************************************************
Interactive Input Editing and History Substitution
**************************************************

Some versions of the Python interpreter support editing of the current input
line and history substitution, similar to facilities found in the Korn shell and
the GNU Bash shell.  This is implemented using the `GNU Readline`_ library,
which supports various styles of editing.  This library has its own
documentation which we won't duplicate here.


.. _tut-keybindings:

Tab Completion and History Editing
==================================

Completion of variable and module names is
:ref:`automatically enabled <rlcompleter-config>` at interpreter startup so
that the :kbd:`Tab` key invokes the completion function; it looks at
Python statement names, the current local variables, and the available
module names.  For dotted expressions such as ``string.a``, it will evaluate
the expression up to the final ``'.'`` and then suggest completions from
the attributes of the resulting object.  Note that this may execute
application-defined code if an object with a :meth:`~object.__getattr__` method
is part of the expression.  The default configuration also saves your
history into a file named :file:`.python_history` in your user directory.
The history will be available again during the next interactive interpreter
session.


.. _tut-commentary:

Alternatives to the Interactive Interpreter
===========================================

This facility is an enormous step forward compared to earlier versions of the
interpreter; however, some wishes are left: It would be nice if the proper
indentation were suggested on continuation lines (the parser knows if an indent
token is required next).  The completion mechanism might use the interpreter's
symbol table.  A command to check (or even suggest) matching parentheses,
quotes, etc., would also be useful.

One alternative enhanced interactive interpreter that has been around for quite
some time is IPython_, which features tab completion, object exploration and
advanced history management.  It can also be thoroughly customized and embedded
into other applications.  Another similar enhanced interactive environment is
bpython_.


.. _GNU Readline: https://tiswww.case.edu/php/chet/readline/rltop.html
.. _IPython: https://ipython.org/
.. _bpython: https://www.bpython-interpreter.org/
