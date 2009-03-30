.. _documenting-index:

######################
  Documenting Python
######################


The Python language has a substantial body of documentation, much of it
contributed by various authors. The markup used for the Python documentation is
`reStructuredText`_, developed by the `docutils`_ project, amended by custom
directives and using a toolset named `Sphinx`_ to postprocess the HTML output.

This document describes the style guide for our documentation, the custom
reStructuredText markup introduced to support Python documentation and how it
should be used, as well as the Sphinx build system.

.. _reStructuredText: http://docutils.sf.net/rst.html
.. _docutils: http://docutils.sf.net/
.. _Sphinx: http://sphinx.pocoo.org/

If you're interested in contributing to Python's documentation, there's no need
to write reStructuredText if you're not so inclined; plain text contributions
are more than welcome as well.

.. toctree::
   :numbered:

   intro.rst
   style.rst
   rest.rst
   markup.rst
   fromlatex.rst
