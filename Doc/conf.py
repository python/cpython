# -*- coding: utf-8 -*-
#
# Python documentation build configuration file
#
# This file is execfile()d with the current directory set to its containing dir.
#
# The contents of this file are pickled, so don't put values in the namespace
# that aren't pickleable (module imports are okay, they're removed automatically).

import sys, os, time
sys.path.append('tools/sphinxext')

# General configuration
# ---------------------

extensions = ['sphinx.ext.refcounting', 'sphinx.ext.coverage']

# General substitutions.
project = 'Python'
copyright = '1990-%s, Python Software Foundation' % time.strftime('%Y')

# The default replacements for |version| and |release|.
#
# The short X.Y version.
# version = '2.6'
# The full version, including alpha/beta/rc tags.
# release = '2.6a0'

# We look for the Include/patchlevel.h file in the current Python source tree
# and replace the values accordingly.
import patchlevel
version, release = patchlevel.get_version_info()

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
today = ''
# Else, today_fmt is used as the format for a strftime call.
today_fmt = '%B %d, %Y'

# List of files that shouldn't be included in the build.
unused_docs = [
    'whatsnew/2.0',
    'whatsnew/2.1',
    'whatsnew/2.2',
    'whatsnew/2.3',
    'whatsnew/2.4',
    'whatsnew/2.5',
    'whatsnew/2.6',
    'maclib/scrap',
    'library/xmllib',
    'library/xml.etree',
]

# Relative filename of the reference count data file.
refcount_file = 'data/refcounts.dat'

# If true, '()' will be appended to :func: etc. cross-reference text.
add_function_parentheses = True

# If true, the current module name will be prepended to all description
# unit titles (such as .. function::).
add_module_names = True


# Options for HTML output
# -----------------------

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
html_last_updated_fmt = '%b %d, %Y'

# If true, SmartyPants will be used to convert quotes and dashes to
# typographically correct entities.
html_use_smartypants = True

# Content template for the index page, filename relative to this file.
html_index = 'tools/sphinxext/indexcontent.html'

# Custom sidebar templates, filenames relative to this file.
html_sidebars = {
    'index': 'tools/sphinxext/indexsidebar.html',
}

# Additional templates that should be rendered to pages.
html_additional_pages = {
    'download': 'tools/sphinxext/download.html',
}

# Output file base name for HTML help builder.
htmlhelp_basename = 'python' + release.replace('.', '')


# Options for LaTeX output
# ------------------------

# The paper size ('letter' or 'a4').
latex_paper_size = 'a4'

# The font size ('10pt', '11pt' or '12pt').
latex_font_size = '10pt'

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, document class [howto/manual]).
_stdauthor = r'Guido van Rossum\\Fred L. Drake, Jr., editor'
latex_documents = [
    ('c-api/index', 'c-api.tex',
     'The Python/C API', _stdauthor, 'manual'),
    ('distutils/index', 'distutils.tex',
     'Distributing Python Modules', _stdauthor, 'manual'),
    ('documenting/index', 'documenting.tex',
     'Documenting Python', 'Georg Brandl', 'manual'),
    ('extending/index', 'extending.tex',
     'Extending and Embedding Python', _stdauthor, 'manual'),
    ('install/index', 'install.tex',
     'Installing Python Modules', _stdauthor, 'manual'),
    ('library/index', 'library.tex',
     'The Python Library Reference', _stdauthor, 'manual'),
    ('reference/index', 'reference.tex',
     'The Python Language Reference', _stdauthor, 'manual'),
    ('tutorial/index', 'tutorial.tex',
     'Python Tutorial', _stdauthor, 'manual'),
    ('using/index', 'using.tex',
     'Using Python', _stdauthor, 'manual'),
    ('whatsnew/' + version, 'whatsnew.tex',
     'What\'s New in Python', 'A. M. Kuchling', 'howto'),
]
# Collect all HOWTOs individually
latex_documents.extend(('howto/' + fn[:-4], 'howto-' + fn[:-4] + '.tex',
                        'HOWTO', _stdauthor, 'howto')
                       for fn in os.listdir('howto')
                       if fn.endswith('.rst') and fn != 'index.rst')

# Additional stuff for the LaTeX preamble.
latex_preamble = r'''
\authoraddress{
  \strong{Python Software Foundation}\\
  Email: \email{docs@python.org}
}
'''

# Documents to append as an appendix to all manuals.
latex_appendices = ['glossary', 'about', 'license', 'copyright']

# Options for the coverage checker
# --------------------------------

# The coverage checker will ignore all modules/functions/classes whose names
# match any of the following regexes (using re.match).
coverage_ignore_modules = [
    r'[T|t][k|K]',
    r'Tix',
    r'distutils.*',
]

coverage_ignore_functions = [
    'test($|_)',
]

coverage_ignore_classes = [
]

# Glob patterns for C source files for C API coverage, relative to this directory.
coverage_c_path = [
    '../Include/*.h',
]

# Regexes to find C items in the source files.
coverage_c_regexes = {
    'cfunction': (r'^PyAPI_FUNC\(.*\)\s+([^_][\w_]+)'),
    'data': (r'^PyAPI_DATA\(.*\)\s+([^_][\w_]+)'),
    'macro': (r'^#define ([^_][\w_]+)\(.*\)[\s|\\]'),
}

# The coverage checker will ignore all C items whose names match these regexes
# (using re.match) -- the keys must be the same as in coverage_c_regexes.
coverage_ignore_c_items = {
#    'cfunction': [...]
}
