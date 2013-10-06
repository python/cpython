.. _extending-index:

##################################################
  Extending and Embedding the Python Interpreter
##################################################

This document describes how to write modules in C or C++ to extend the Python
interpreter with new modules.  Those modules can not only define new functions
but also new object types and their methods.  The document also describes how
to embed the Python interpreter in another application, for use as an extension
language.  Finally, it shows how to compile and link extension modules so that
they can be loaded dynamically (at run time) into the interpreter, if the
underlying operating system supports this feature.

This document assumes basic knowledge about Python.  For an informal
introduction to the language, see :ref:`tutorial-index`.  :ref:`reference-index`
gives a more formal definition of the language.  :ref:`library-index` documents
the existing object types, functions and modules (both built-in and written in
Python) that give the language its wide application range.

For a detailed description of the whole Python/C API, see the separate
:ref:`c-api-index`.

.. toctree::
   :maxdepth: 2
   :numbered:

   extending.rst
   newtypes.rst
   building.rst
   windows.rst
   embedding.rst
