# -*- coding: utf-8 -*-
"""
    Sphinx - Python documentation toolchain
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    :copyright: 2007 by Georg Brandl.
    :license: Python license.
"""

import sys

if __name__ == '__main__':

    if sys.version_info[:3] < (2, 4, 0):
        print("""\
Error: Sphinx needs to be executed with Python 2.4 or newer (not 3.0 though).
(If you run this from the Makefile, you can set the PYTHON variable
to the path of an alternative interpreter executable, e.g.,
``make html PYTHON=python2.5``).
""", file=sys.stderr)
        sys.exit(1)

    from sphinx import main
    sys.exit(main(sys.argv))
