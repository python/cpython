
:mod:`__main__` --- Top-level script environment
================================================

.. module:: __main__
   :synopsis: The environment where the top-level script is run.


This module represents the (otherwise anonymous) scope in which the
interpreter's main program executes --- commands read either from standard
input, from a script file, or from an interactive prompt.  It is this
environment in which the idiomatic "conditional script" stanza causes a script
to run::

   if __name__ == "__main__":
       main()

