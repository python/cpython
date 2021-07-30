#
# Python documentation build configuration file
#
# This file is execfile()d with the current directory set to its containing dir.
#
# The contents of this file are pickled, so don't put values in the namespace
# that aren't pickleable (module imports are okay, they're removed automatically).

import sys, os, time
sys.path.append(os.path.abspath('tools/extensions'))
sys.path.append(os.path.abspath('includes'))

# General configuration
# ---------------------

extensions = ['sphinx.ext.coverage', 'sphinx.ext.doctest',
              'pyspecific', 'c_annotations', 'escape4chm',
              'asdl_highlight', 'peg_highlight']


doctest_global_setup = '''
try:
    import _tkinter
except ImportError:
    _tkinter = None
'''

manpages_url = 'https://manpages.debian.org/{path}'

# General substitutions.
project = 'Python'
copyright = '2001-%s, Python Software Foundation' % time.strftime('%Y')

# We look for the Include/patchlevel.h file in the current Python source tree
# and replace the values accordingly.
import patchlevel
version, release = patchlevel.get_version_info()

# There are two options for replacing |today|: either, you set today to some
# non-false value, then it is used:
today = ''
# Else, today_fmt is used as the format for a strftime call.
today_fmt = '%B %d, %Y'

# By default, highlight as Python 3.
highlight_language = 'python3'

# Minimum version of sphinx required
needs_sphinx = '1.8'

# Ignore any .rst files in the venv/ directory.
exclude_patterns = ['venv/*', 'README.rst']
venvdir = os.getenv('VENVDIR')
if venvdir is not None:
    exclude_patterns.append(venvdir + '/*')

# Disable Docutils smartquotes for several translations
smartquotes_excludes = {
    'languages': ['ja', 'fr', 'zh_TW', 'zh_CN'], 'builders': ['man', 'text'],
}

# Avoid a warning with Sphinx >= 2.0
master_doc = 'contents'

# Options for HTML output
# -----------------------

# Use our custom theme.
html_theme = 'python_docs_theme'
html_theme_path = ['tools']
html_theme_options = {
    'collapsiblesidebar': True,
    'issues_url': 'https://docs.python.org/3/bugs.html',
    'root_include_title': False   # We use the version switcher instead.
}

# Short title used e.g. for <title> HTML tags.
html_short_title = '%s Documentation' % release

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
html_last_updated_fmt = '%b %d, %Y'

# Path to find HTML templates.
templates_path = ['tools/templates']

# Custom sidebar templates, filenames relative to this file.
html_sidebars = {
    # Defaults taken from https://www.sphinx-doc.org/en/master/usage/configuration.html#confval-html_sidebars
    # Removes the quick search block
    '**': ['localtoc.html', 'relations.html', 'customsourcelink.html'],
    'index': ['indexsidebar.html'],
}

# Additional templates that should be rendered to pages.
html_additional_pages = {
    'download': 'download.html',
    'index': 'indexcontent.html',
}

# Output an OpenSearch description file.
html_use_opensearch = 'https://docs.python.org/' + version

# Additional static files.
html_static_path = ['tools/static']

# Output file base name for HTML help builder.
htmlhelp_basename = 'python' + release.replace('.', '')

# Split the index
html_split_index = True


# Options for LaTeX output
# ------------------------

latex_engine = 'xelatex'

# Get LaTeX to handle Unicode correctly
latex_elements = {
}

# Additional stuff for the LaTeX preamble.
latex_elements['preamble'] = r'''
\authoraddress{
  \sphinxstrong{Python Software Foundation}\\
  Email: \sphinxemail{docs@python.org}
}
\let\Verbatim=\OriginalVerbatim
\let\endVerbatim=\endOriginalVerbatim
\setcounter{tocdepth}{2}
'''

# The paper size ('letter' or 'a4').
latex_elements['papersize'] = 'a4'

# The font size ('10pt', '11pt' or '12pt').
latex_elements['pointsize'] = '10pt'

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, document class [howto/manual]).
_stdauthor = r'Guido van Rossum\\and the Python development team'
latex_documents = [
    ('c-api/index', 'c-api.tex',
     'The Python/C API', _stdauthor, 'manual'),
    ('distributing/index', 'distributing.tex',
     'Distributing Python Modules', _stdauthor, 'manual'),
    ('extending/index', 'extending.tex',
     'Extending and Embedding Python', _stdauthor, 'manual'),
    ('installing/index', 'installing.tex',
     'Installing Python Modules', _stdauthor, 'manual'),
    ('library/index', 'library.tex',
     'The Python Library Reference', _stdauthor, 'manual'),
    ('reference/index', 'reference.tex',
     'The Python Language Reference', _stdauthor, 'manual'),
    ('tutorial/index', 'tutorial.tex',
     'Python Tutorial', _stdauthor, 'manual'),
    ('using/index', 'using.tex',
     'Python Setup and Usage', _stdauthor, 'manual'),
    ('faq/index', 'faq.tex',
     'Python Frequently Asked Questions', _stdauthor, 'manual'),
    ('whatsnew/' + version, 'whatsnew.tex',
     'What\'s New in Python', 'A. M. Kuchling', 'howto'),
]
# Collect all HOWTOs individually
latex_documents.extend(('howto/' + fn[:-4], 'howto-' + fn[:-4] + '.tex',
                        '', _stdauthor, 'howto')
                       for fn in os.listdir('howto')
                       if fn.endswith('.rst') and fn != 'index.rst')

# Documents to append as an appendix to all manuals.
latex_appendices = ['glossary', 'about', 'license', 'copyright']

# Options for Epub output
# -----------------------

epub_author = 'Python Documentation Authors'
epub_publisher = 'Python Software Foundation'

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


# Options for the link checker
# ----------------------------

# Ignore certain URLs.
linkcheck_ignore = [r'https://bugs.python.org/(issue)?\d+',
                    # Ignore PEPs for now, they all have permanent redirects.
                    r'http://www.python.org/dev/peps/pep-\d+']


# Options for extensions
# ----------------------

# Relative filename of the reference count data file.
refcount_file = 'data/refcounts.dat'

# Sphinx 2 and Sphinx 3 compatibility
# -----------------------------------

# bpo-40204: Allow Sphinx 2 syntax in the C domain
c_allow_pre_v3 = True

# bpo-40204: Disable warnings on Sphinx 2 syntax of the C domain since the
# documentation is built with -W (warnings treated as errors).
c_warn_on_allowed_pre_v3 = False
