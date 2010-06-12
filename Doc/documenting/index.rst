.. _documenting-index:

######################
  Documenting Python
######################


The Python language has a substantial body of documentation, much of it
contributed by various authors. The markup used for the Python documentation is
`reStructuredText`_, developed by the `docutils`_ project, amended by custom
directives and using a toolset named `Sphinx`_ to postprocess the HTML output.

This document describes the style guide for our documentation as well as the
custom reStructuredText markup introduced by Sphinx to support Python
documentation and how it should be used.

.. _reStructuredText: http://docutils.sf.net/rst.html
.. _docutils: http://docutils.sf.net/
.. _Sphinx: http://sphinx.pocoo.org/

.. note::

   If you're interested in contributing to Python's documentation, there's no
   need to write reStructuredText if you're not so inclined; plain text
   contributions are more than welcome as well.  Send an e-mail to
   docs@python.org or open an issue on the :ref:`tracker <reporting-bugs>`.


.. toctree::
   :numbered:
   :maxdepth: 1

   intro.rst
   style.rst
   rest.rst
   markup.rst
   fromlatex.rst
   building.rst
