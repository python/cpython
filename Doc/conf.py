#
# Python documentation build configuration file
#
# This file is execfile()d with the current directory set to its containing dir.
#
# The contents of this file are pickled, so don't put values in the namespace
# that aren't pickleable (module imports are okay, they're removed automatically).

import os
import sys
from importlib import import_module
from importlib.util import find_spec

# Make our custom extensions available to Sphinx
sys.path.append(os.path.abspath('tools/extensions'))
sys.path.append(os.path.abspath('includes'))

# Python specific content from Doc/Tools/extensions/pyspecific.py
from pyspecific import SOURCE_URI

# General configuration
# ---------------------

# Our custom Sphinx extensions are found in Doc/Tools/extensions/
extensions = [
    'audit_events',
    'availability',
    'c_annotations',
    'changes',
    'glossary_search',
    'grammar_snippet',
    'implementation_detail',
    'issue_role',
    'lexers',
    'misc_news',
    'pydoc_topics',
    'pyspecific',
    'sphinx.ext.coverage',
    'sphinx.ext.doctest',
    'sphinx.ext.extlinks',
]

# Skip if downstream redistributors haven't installed them
_OPTIONAL_EXTENSIONS = (
    'notfound.extension',
    'sphinxext.opengraph',
)
for optional_ext in _OPTIONAL_EXTENSIONS:
    try:
        if find_spec(optional_ext) is not None:
            extensions.append(optional_ext)
    except (ImportError, ValueError):
        pass
del _OPTIONAL_EXTENSIONS

doctest_global_setup = '''
try:
    import _tkinter
except ImportError:
    _tkinter = None
# Treat warnings as errors, done here to prevent warnings in Sphinx code from
# causing spurious CPython test failures.
import warnings
warnings.simplefilter('error')
del warnings
'''

manpages_url = 'https://manpages.debian.org/{path}'

# General substitutions.
project = 'Python'
copyright = "2001 Python Software Foundation"

# We look for the Include/patchlevel.h file in the current Python source tree
# and replace the values accordingly.
# See Doc/tools/extensions/patchlevel.py
version, release = import_module('patchlevel').get_version_info()

rst_epilog = f"""
.. |python_version_literal| replace:: ``Python {version}``
.. |python_x_dot_y_literal| replace:: ``python{version}``
.. |python_x_dot_y_t_literal| replace:: ``python{version}t``
.. |python_x_dot_y_t_literal_config| replace:: ``python{version}t-config``
.. |x_dot_y_b2_literal| replace:: ``{version}.0b2``
.. |applications_python_version_literal| replace:: ``/Applications/Python {version}/``
.. |usr_local_bin_python_x_dot_y_literal| replace:: ``/usr/local/bin/python{version}``

.. Apparently this how you hack together a formatted link:
   (https://www.docutils.org/docs/ref/rst/directives.html#replacement-text)
.. |FORCE_COLOR| replace:: ``FORCE_COLOR``
.. _FORCE_COLOR: https://force-color.org/
.. |NO_COLOR| replace:: ``NO_COLOR``
.. _NO_COLOR: https://no-color.org/
"""

# There are two options for replacing |today|. Either, you set today to some
# non-false value and use it.
today = ''
# Or else, today_fmt is used as the format for a strftime call.
today_fmt = '%B %d, %Y'

# By default, highlight as Python 3.
highlight_language = 'python3'

# Minimum version of sphinx required
# Keep this version in sync with ``Doc/requirements.txt``.
needs_sphinx = '8.2.0'

# Create table of contents entries for domain objects (e.g. functions, classes,
# attributes, etc.). Default is True.
toc_object_entries = False

# Ignore any .rst files in the includes/ directory;
# they're embedded in pages but not rendered as individual pages.
# Ignore any .rst files in the venv/ directory.
exclude_patterns = ['includes/*.rst', 'venv/*', 'README.rst']
venvdir = os.getenv('VENVDIR')
if venvdir is not None:
    exclude_patterns.append(venvdir + '/*')

nitpick_ignore = [
    # Standard C functions
    ('c:func', 'calloc'),
    ('c:func', 'ctime'),
    ('c:func', 'dlopen'),
    ('c:func', 'exec'),
    ('c:func', 'fcntl'),
    ('c:func', 'flock'),
    ('c:func', 'fork'),
    ('c:func', 'free'),
    ('c:func', 'gettimeofday'),
    ('c:func', 'gmtime'),
    ('c:func', 'grantpt'),
    ('c:func', 'ioctl'),
    ('c:func', 'localeconv'),
    ('c:func', 'localtime'),
    ('c:func', 'main'),
    ('c:func', 'malloc'),
    ('c:func', 'mktime'),
    ('c:func', 'posix_openpt'),
    ('c:func', 'printf'),
    ('c:func', 'ptsname'),
    ('c:func', 'ptsname_r'),
    ('c:func', 'realloc'),
    ('c:func', 'snprintf'),
    ('c:func', 'sprintf'),
    ('c:func', 'stat'),
    ('c:func', 'strftime'),
    ('c:func', 'system'),
    ('c:func', 'time'),
    ('c:func', 'unlockpt'),
    ('c:func', 'vsnprintf'),
    # Standard C types
    ('c:type', 'FILE'),
    ('c:type', 'int8_t'),
    ('c:type', 'int16_t'),
    ('c:type', 'int32_t'),
    ('c:type', 'int64_t'),
    ('c:type', 'intmax_t'),
    ('c:type', 'off_t'),
    ('c:type', 'ptrdiff_t'),
    ('c:type', 'siginfo_t'),
    ('c:type', 'size_t'),
    ('c:type', 'ssize_t'),
    ('c:type', 'time_t'),
    ('c:type', 'uint8_t'),
    ('c:type', 'uint16_t'),
    ('c:type', 'uint32_t'),
    ('c:type', 'uint64_t'),
    ('c:type', 'uintmax_t'),
    ('c:type', 'uintptr_t'),
    ('c:type', 'va_list'),
    ('c:type', 'wchar_t'),
    ('c:type', '__int64'),
    ('c:type', 'unsigned __int64'),
    ('c:type', 'double'),
    # Standard C structures
    ('c:struct', 'in6_addr'),
    ('c:struct', 'in_addr'),
    ('c:struct', 'stat'),
    ('c:struct', 'statvfs'),
    ('c:struct', 'timeval'),
    ('c:struct', 'timespec'),
    # Standard C macros
    ('c:macro', 'LLONG_MAX'),
    ('c:macro', 'LLONG_MIN'),
    ('c:macro', 'LONG_MAX'),
    ('c:macro', 'LONG_MIN'),
    # Standard C variables
    ('c:data', 'errno'),
    # Standard environment variables
    ('envvar', 'BROWSER'),
    ('envvar', 'COLUMNS'),
    ('envvar', 'COMSPEC'),
    ('envvar', 'DISPLAY'),
    ('envvar', 'HOME'),
    ('envvar', 'HOMEDRIVE'),
    ('envvar', 'HOMEPATH'),
    ('envvar', 'IDLESTARTUP'),
    ('envvar', 'LANG'),
    ('envvar', 'LANGUAGE'),
    ('envvar', 'LC_ALL'),
    ('envvar', 'LC_CTYPE'),
    ('envvar', 'LC_COLLATE'),
    ('envvar', 'LC_MESSAGES'),
    ('envvar', 'LC_MONETARY'),
    ('envvar', 'LC_NUMERIC'),
    ('envvar', 'LC_TIME'),
    ('envvar', 'LINES'),
    ('envvar', 'LOGNAME'),
    ('envvar', 'MANPAGER'),
    ('envvar', 'PAGER'),
    ('envvar', 'PATH'),
    ('envvar', 'PATHEXT'),
    ('envvar', 'SOURCE_DATE_EPOCH'),
    ('envvar', 'TEMP'),
    ('envvar', 'TERM'),
    ('envvar', 'TMP'),
    ('envvar', 'TMPDIR'),
    ('envvar', 'TZ'),
    ('envvar', 'USER'),
    ('envvar', 'USERNAME'),
    ('envvar', 'USERPROFILE'),
]

# Temporary undocumented names.
# In future this list must be empty.
nitpick_ignore += [
    # Do not error nit-picky mode builds when _SubParsersAction.add_parser cannot
    # be resolved, as the method is currently undocumented. For context, see
    # https://github.com/python/cpython/pull/103289.
    ('py:meth', '_SubParsersAction.add_parser'),
    # Attributes/methods/etc. that definitely should be documented better,
    # but are deferred for now:
    ('py:attr', '__wrapped__'),
]

# gh-106948: Copy standard C types declared in the "c:type" domain and C
# structures declared in the "c:struct" domain to the "c:identifier" domain,
# since "c:function" markup looks for types in the "c:identifier" domain. Use
# list() to not iterate on items which are being added
for role, name in list(nitpick_ignore):
    if role in ('c:type', 'c:struct'):
        nitpick_ignore.append(('c:identifier', name))
del role, name

# Disable Docutils smartquotes for several translations
smartquotes_excludes = {
    'languages': ['ja', 'fr', 'zh_TW', 'zh_CN'],
    'builders': ['man', 'text'],
}

# Avoid a warning with Sphinx >= 4.0
root_doc = 'contents'

# Allow translation of index directives
gettext_additional_targets = [
    'index',
    'literal-block',
]

# Options for HTML output
# -----------------------

# Use our custom theme: https://github.com/python/python-docs-theme
html_theme = 'python_docs_theme'
# Location of overrides for theme templates and static files
html_theme_path = ['tools']
html_theme_options = {
    'collapsiblesidebar': True,
    'issues_url': '/bugs.html',
    'license_url': '/license.html',
    'root_include_title': False,  # We use the version switcher instead.
}

if os.getenv("READTHEDOCS"):
    html_theme_options["hosted_on"] = (
        '<a href="https://about.readthedocs.com/">Read the Docs</a>'
    )

# Override stylesheet fingerprinting for Windows CHM htmlhelp to fix GH-91207
# https://github.com/python/cpython/issues/91207
if any('htmlhelp' in arg for arg in sys.argv):
    html_style = 'pydoctheme.css'
    print("\nWARNING: Windows CHM Help is no longer supported.")
    print("It may be removed in the future\n")

# Short title used e.g. for <title> HTML tags.
html_short_title = f'{release} Documentation'

# Deployment preview information
# (See .readthedocs.yml and https://docs.readthedocs.io/en/stable/reference/environment-variables.html)
is_deployment_preview = os.getenv("READTHEDOCS_VERSION_TYPE") == "external"
repository_url = os.getenv("READTHEDOCS_GIT_CLONE_URL", "")
repository_url = repository_url.removesuffix(".git")
html_context = {
    "is_deployment_preview": is_deployment_preview,
    "repository_url": repository_url or None,
    "pr_id": os.getenv("READTHEDOCS_VERSION"),
    "enable_analytics": os.getenv("PYTHON_DOCS_ENABLE_ANALYTICS"),
}

# This 'Last updated on:' timestamp is inserted at the bottom of every page.
html_last_updated_fmt = '%b %d, %Y (%H:%M UTC)'
html_last_updated_use_utc = True

# Path to find HTML templates to override theme
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

# Split pot files one per reST file
gettext_compact = False

# Options for LaTeX output
# ------------------------

latex_engine = 'xelatex'

latex_elements = {
    # For the LaTeX preamble.
    'preamble': r'''
\authoraddress{
  \sphinxstrong{Python Software Foundation}\\
  Email: \sphinxemail{docs@python.org}
}
\let\Verbatim=\OriginalVerbatim
\let\endVerbatim=\endOriginalVerbatim
\setcounter{tocdepth}{2}
''',
    # The paper size ('letterpaper' or 'a4paper').
    'papersize': 'a4paper',
    # The font size ('10pt', '11pt' or '12pt').
    'pointsize': '10pt',
    'maxlistdepth': '8',  # See https://github.com/python/cpython/issues/139588
}

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, document class [howto/manual]).
_stdauthor = 'The Python development team'
latex_documents = [
    ('c-api/index', 'c-api.tex', 'The Python/C API', _stdauthor, 'manual'),
    (
        'extending/index',
        'extending.tex',
        'Extending and Embedding Python',
        _stdauthor,
        'manual',
    ),
    (
        'installing/index',
        'installing.tex',
        'Installing Python Modules',
        _stdauthor,
        'manual',
    ),
    (
        'library/index',
        'library.tex',
        'The Python Library Reference',
        _stdauthor,
        'manual',
    ),
    (
        'reference/index',
        'reference.tex',
        'The Python Language Reference',
        _stdauthor,
        'manual',
    ),
    (
        'tutorial/index',
        'tutorial.tex',
        'Python Tutorial',
        _stdauthor,
        'manual',
    ),
    (
        'using/index',
        'using.tex',
        'Python Setup and Usage',
        _stdauthor,
        'manual',
    ),
    (
        'faq/index',
        'faq.tex',
        'Python Frequently Asked Questions',
        _stdauthor,
        'manual',
    ),
    (
        'whatsnew/' + version,
        'whatsnew.tex',
        'What\'s New in Python',
        'A. M. Kuchling',
        'howto',
    ),
]
# Collect all HOWTOs individually
latex_documents.extend(
    ('howto/' + fn[:-4], 'howto-' + fn[:-4] + '.tex', '', _stdauthor, 'howto')
    for fn in os.listdir('howto')
    if fn.endswith('.rst') and fn != 'index.rst'
)

# Documents to append as an appendix to all manuals.
latex_appendices = ['glossary', 'about', 'license', 'copyright']

# Options for Epub output
# -----------------------

epub_author = 'Python Documentation Authors'
epub_publisher = 'Python Software Foundation'
epub_exclude_files = ('index.xhtml', 'download.xhtml')

# index pages are not valid xhtml
# https://github.com/sphinx-doc/sphinx/issues/12359
epub_use_index = False

# translation tag
# ---------------

language_code = None
for arg in sys.argv:
    if arg.startswith('language='):
        language_code = arg.split('=', 1)[1]

if language_code:
    tags.add('translation')  # noqa: F821

    rst_epilog += f"""\
.. _TRANSLATION_REPO: https://github.com/python/python-docs-{language_code.replace("_", "-").lower()}
"""  # noqa: F821
else:
    rst_epilog += """\
.. _TRANSLATION_REPO: https://github.com/python
"""

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

coverage_ignore_classes = []

# Glob patterns for C source files for C API coverage, relative to this directory.
coverage_c_path = [
    '../Include/*.h',
]

# Regexes to find C items in the source files.
coverage_c_regexes = {
    'cfunction': r'^PyAPI_FUNC\(.*\)\s+([^_][\w_]+)',
    'data': r'^PyAPI_DATA\(.*\)\s+([^_][\w_]+)',
    'macro': r'^#define ([^_][\w_]+)\(.*\)[\s|\\]',
}

# The coverage checker will ignore all C items whose names match these regexes
# (using re.match) -- the keys must be the same as in coverage_c_regexes.
coverage_ignore_c_items = {
    # 'cfunction': [...]
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
    # Microsoft's redirects to learn.microsoft.com
    r'https://msdn.microsoft.com/.*': 'https://learn.microsoft.com/.*',
    r'https://docs.microsoft.com/.*': 'https://learn.microsoft.com/.*',
    r'https://go.microsoft.com/fwlink/\?LinkID=\d+': 'https://learn.microsoft.com/.*',
    # Debian's man page redirects to its current stable version
    r'https://manpages.debian.org/\w+\(\d(\w+)?\)': r'https://manpages.debian.org/\w+/[\w/\-\.]*\.\d(\w+)?\.en\.html',
    # Language redirects
    r'https://toml.io': 'https://toml.io/en/',
    r'https://www.redhat.com': 'https://www.redhat.com/en',
    # pypi.org project name normalization (upper to lowercase, underscore to hyphen)
    r'https://pypi.org/project/[A-Za-z\d_\-\.]+/': r'https://pypi.org/project/[a-z\d\-\.]+/',
    # Discourse title name expansion (text changes when title is edited)
    r'https://discuss\.python\.org/t/\d+': r'https://discuss\.python\.org/t/.*/\d+',
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

# Options for sphinx.ext.extlinks
# -------------------------------

# This config is a dictionary of external sites,
# mapping unique short aliases to a base URL and a prefix.
# https://www.sphinx-doc.org/en/master/usage/extensions/extlinks.html
extlinks = {
    "pypi": ("https://pypi.org/project/%s/", "%s"),
    "source": (SOURCE_URI, "%s"),
}
extlinks_detect_hardcoded_links = True

# Options for c_annotations extension
# -----------------------------------

# Relative filename of the data files
refcount_file = 'data/refcounts.dat'
stable_abi_file = 'data/stable_abi.dat'

# Options for sphinxext-opengraph
# -------------------------------

ogp_canonical_url = 'https://docs.python.org/3/'
ogp_site_name = 'Python documentation'
ogp_social_cards = {  # Used when matplotlib is installed
    'image': '_static/og-image.png',
    'line_color': '#3776ab',
}
ogp_custom_meta_tags = ('<meta name="theme-color" content="#3776ab">',)
if 'create-social-cards' not in tags:  # noqa: F821
    # Define a static preview image when not creating social cards
    ogp_image = '_static/og-image.png'
    ogp_custom_meta_tags += (
        '<meta property="og:image:width" content="200">',
        '<meta property="og:image:height" content="200">',
    )
