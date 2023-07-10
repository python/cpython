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

extensions = [
    'asdl_highlight',
    'c_annotations',
    'escape4chm',
    'glossary_search',
    'peg_highlight',
    'pyspecific',
    'sphinx.ext.coverage',
    'sphinx.ext.doctest',
]

# Skip if downstream redistributors haven't installed it
try:
    import sphinxext.opengraph
except ImportError:
    pass
else:
    extensions.append('sphinxext.opengraph')


doctest_global_setup = '''
try:
    import _tkinter
except ImportError:
    _tkinter = None
# Treat warnings as errors, done here to prevent warnings in Sphinx code from
# causing spurious test failures.
import warnings
warnings.simplefilter('error')
del warnings
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
needs_sphinx = '3.2'

# Ignore any .rst files in the includes/ directory;
# they're embedded in pages but not rendered individually.
# Ignore any .rst files in the venv/ directory.
exclude_patterns = ['includes/*.rst', 'venv/*', 'README.rst']
venvdir = os.getenv('VENVDIR')
if venvdir is not None:
    exclude_patterns.append(venvdir + '/*')

nitpick_ignore = [
    # Do not error nit-picky mode builds when _SubParsersAction.add_parser cannot
    # be resolved, as the method is currently undocumented. For context, see
    # https://github.com/python/cpython/pull/103289.
    ('py:meth', '_SubParsersAction.add_parser'),
]

# Disable Docutils smartquotes for several translations
smartquotes_excludes = {
    'languages': ['ja', 'fr', 'zh_TW', 'zh_CN'], 'builders': ['man', 'text'],
}

# Avoid a warning with Sphinx >= 2.0
master_doc = 'contents'

# Allow translation of index directives
gettext_additional_targets = [
    'index',
]

# Options for HTML output
# -----------------------

# Use our custom theme.
html_theme = 'python_docs_theme'
html_theme_path = ['tools']
html_theme_options = {
    'collapsiblesidebar': True,
    'issues_url': '/bugs.html',
    'license_url': '/license.html',
    'root_include_title': False   # We use the version switcher instead.
}

# Override stylesheet fingerprinting for Windows CHM htmlhelp to fix GH-91207
# https://github.com/python/cpython/issues/91207
if any('htmlhelp' in arg for arg in sys.argv):
    html_style = 'pydoctheme.css'
    print("\nWARNING: Windows CHM Help is no longer supported.")
    print("It may be removed in the future\n")

# Short title used e.g. for <title> HTML tags.
html_short_title = '%s Documentation' % release

# Deployment preview information
# (See .readthedocs.yml and https://docs.readthedocs.io/en/stable/reference/environment-variables.html)
repository_url = os.getenv("READTHEDOCS_GIT_CLONE_URL")
html_context = {
    "is_deployment_preview": os.getenv("READTHEDOCS_VERSION_TYPE") == "external",
    "repository_url": repository_url.removesuffix(".git") if repository_url else None,
    "pr_id": os.getenv("READTHEDOCS_VERSION")
}

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
html_static_path = ['_static', 'tools/static']

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
_stdauthor = 'Guido van Rossum and the Python development team'
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

linkcheck_allowed_redirects = {
    # bpo-NNNN -> BPO -> GH Issues
    r'https://bugs.python.org/issue\?@action=redirect&bpo=\d+': r'https://github.com/python/cpython/issues/\d+',
    # GH-NNNN used to refer to pull requests
    r'https://github.com/python/cpython/issues/\d+': r'https://github.com/python/cpython/pull/\d+',
    # :source:`something` linking files in the repository
    r'https://github.com/python/cpython/tree/.*': 'https://github.com/python/cpython/blob/.*',
    # Intentional HTTP use at Misc/NEWS.d/3.5.0a1.rst
    r'http://www.python.org/$': 'https://www.python.org/$',
    # Used in license page, keep as is
    r'https://www.zope.org/': r'https://www.zope.dev/',
    # Microsoft's redirects to learn.microsoft.com
    r'https://msdn.microsoft.com/.*': 'https://learn.microsoft.com/.*',
    r'https://docs.microsoft.com/.*': 'https://learn.microsoft.com/.*',
    r'https://go.microsoft.com/fwlink/\?LinkID=\d+': 'https://learn.microsoft.com/.*',
    # Language redirects
    r'https://toml.io': 'https://toml.io/en/',
    r'https://www.redhat.com': 'https://www.redhat.com/en',
    # Other redirects
    r'https://www.boost.org/libs/.+': r'https://www.boost.org/doc/libs/\d_\d+_\d/.+',
    r'https://support.microsoft.com/en-us/help/\d+': 'https://support.microsoft.com/en-us/topic/.+',
    r'https://perf.wiki.kernel.org$': 'https://perf.wiki.kernel.org/index.php/Main_Page',
    r'https://www.sqlite.org': 'https://www.sqlite.org/index.html',
    r'https://mitpress.mit.edu/sicp$': 'https://mitpress.mit.edu/9780262510875/structure-and-interpretation-of-computer-programs/',
    r'https://www.python.org/psf/': 'https://www.python.org/psf-landing/',
}

linkcheck_anchors_ignore = [
    # ignore anchors that start with a '/', e.g. Wikipedia media files:
    # https://en.wikipedia.org/wiki/Walrus#/media/File:Pacific_Walrus_-_Bull_(8247646168).jpg
    r'\/.*',
]

linkcheck_ignore = [
    # The crawler gets "Anchor not found"
    r'https://developer.apple.com/documentation/.+?#.*',
    r'https://devguide.python.org.+?/#.*',
    r'https://github.com.+?#.*',
    # Robot crawlers not allowed: "403 Client Error: Forbidden"
    r'https://support.enthought.com/hc/.*',
    # SSLError CertificateError, even though it is valid
    r'https://unix.org/version2/whatsnew/lp64_wp.html',
]


# Options for extensions
# ----------------------

# Relative filename of the data files
refcount_file = 'data/refcounts.dat'
stable_abi_file = 'data/stable_abi.dat'

# sphinxext-opengraph config
ogp_site_url = 'https://docs.python.org/3/'
ogp_site_name = 'Python documentation'
ogp_image = '_static/og-image.png'
ogp_custom_meta_tags = [
    '<meta property="og:image:width" content="200" />',
    '<meta property="og:image:height" content="200" />',
    '<meta name="theme-color" content="#3776ab" />',
]
