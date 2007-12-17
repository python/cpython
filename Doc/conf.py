# -*- coding: utf-8 -*-
#
# Python documentation build configuration file
#
# The contents of this file are pickled, so don't put values in the namespace
# that aren't pickleable (module imports are okay, they're removed automatically).

# General configuration
# ---------------------

# The default replacements for |version| and |release|.
# If 'auto', Sphinx looks for the Include/patchlevel.h file in the current Python
# source tree and replaces the values accordingly.
#
# The short X.Y version.
# version = '2.6'
version = 'auto'
# The full version, including alpha/beta/rc tags.
# release = '2.6a0'
release = 'auto'

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
today = ''
# Else, today_fmt is used as the format for a strftime call.
today_fmt = '%B %d, %Y'

# List of files that shouldn't be included in the build.
unused_files = [
    'whatsnew/2.0.rst',
    'whatsnew/2.1.rst',
    'whatsnew/2.2.rst',
    'whatsnew/2.3.rst',
    'whatsnew/2.4.rst',
    'whatsnew/2.5.rst',
    'whatsnew/2.6.rst',
    'maclib/scrap.rst',
    'library/xmllib.rst',
    'library/xml.etree.rst',
]

# If true, '()' will be appended to :func: etc. cross-reference text.
add_function_parentheses = True

# If true, the current module name will be prepended to all description
# unit titles (such as .. function::).
add_module_names = True


# Options for HTML output
# -----------------------

# The base URL for download links.
html_download_base_url = 'http://docs.python.org/ftp/python/doc/'

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
html_last_updated_fmt = '%b %d, %Y'

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
html_use_smartypants = True


# Options for LaTeX output
# ------------------------

# The paper size ("letter" or "a4").
latex_paper_size = "a4"

# The font size ("10pt", "11pt" or "12pt").
latex_font_size = "10pt"
