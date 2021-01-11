# -*- coding: utf-8 -*-
#
# Python documentation build configuration file
#
# This file is execfile()d with the current directory set to its containing dir.
#
# The contents of this file are pickled, so don't put values in the namespace
# that aren't pickleable (module imports are okay, they're removed automatically).

import sys, os, time
sys.path.append(os.path.abspath('tools/extensions'))

# General configuration
# ---------------------

extensions = ['sphinx.ext.coverage', 'sphinx.ext.doctest',
              'pyspecific', 'c_annotations']

# General substitutions.
project = 'Python'
copyright = '1990-%s, Python Software Foundation' % time.strftime('%Y')

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
exclude_patterns = [
    'maclib/scrap.rst',
    'library/xmllib.rst',
    'library/xml.etree.rst',
]

# Require Sphinx 1.2 for build.
needs_sphinx = '1.2'

# Avoid a warning with Sphinx >= 2.0
master_doc = 'contents'

# Options for HTML output
# -----------------------

html_theme = 'default'
html_theme_options = {'collapsiblesidebar': True}

# If not '', a 'Last updated on:' timestamp is inserted at every page bottom,
# using the given strftime format.
html_last_updated_fmt = '%b %d, %Y'

# Path to find HTML templates.
templates_path = ['tools/templates']

# Custom sidebar templates, filenames relative to this file.
html_sidebars = {
    'index': ['indexsidebar.html'],
}

# Additional templates that should be rendered to pages.
html_additional_pages = {
    'download': 'download.html',
    'index': 'indexcontent.html',
}

# Output an OpenSearch description file.
html_use_opensearch = 'https://docs.python.org/'

# Additional static files.
html_static_path = ['tools/static']

# Output file base name for HTML help builder.
htmlhelp_basename = 'python' + release.replace('.', '')

# Split the index
html_split_index = True

html_context = {
    'outdated': True,
    'moved_files': {
        'library/fpformat': None,
        'library/mutex': None,
        'library/new': None,
        'library/statvfs': None,
        'library/dircache': None,
        'library/macpath': None,
        'library/dbhash': None,
        'library/bsddb': None,
        # 'library/someos': 'library/index',
        'library/someos': None,
        # 'library/popen2': 'library/subprocess',
        'library/popen2': None,
        # 'library/mhlib': 'library/mailbox',
        'library/mhlib': None,
        # 'library/mimetools': 'library/email',
        'library/mimetools': None,
        # 'library/mimewriter': 'library/email',
        'library/mimewriter': None,
        # 'library/mimify': 'library/email',
        'library/mimify': None,
        # 'library/multifile': 'library/email',
        'library/multifile': None,
        'library/sgmllib': None,
        'library/imageop': None,
        # 'library/hotshot': 'library/profile',
        'library/hotshot': None,
        'library/future_builtins': None,
        'library/user': None,
        'library/fpectl': None,
        'library/restricted': None,
        'library/rexec': None,
        'library/bastion': None,
        'library/imputil': None,
        'library/compiler': None,
        # 'library/dl': 'library/ctypes',
        'library/dl': None,
        'library/posixfile': None,
        # 'library/commands': 'library/subprocess',
        'library/commands': None,
        'library/mac': None,
        'library/ic': None,
        'library/macos': None,
        'library/macostools': None,
        'library/easydialogs': None,
        'library/framework': None,
        'library/autogil': None,
        'library/carbon': None,
        'library/colorpicker': None,
        'library/macosa': None,
        'library/gensuitemodule': None,
        'library/aetools': None,
        'library/aepack': None,
        'library/aetypes': None,
        'library/miniaeframe': None,
        'library/sgi': None,
        'library/al': None,
        'library/cd': None,
        'library/fl': None,
        'library/fm': None,
        'library/gl': None,
        'library/imgfile': None,
        'library/jpeg': None,
        'library/sun': None,
        'library/sunaudio': None,
        # 'c-api/int': 'c-api/long',
        'c-api/int': None,
        # 'c-api/string': 'c-api/bytes',
        'c-api/string': None,
        'c-api/class': None,
        # 'c-api/cobject': 'c-api/capsule',
        'c-api/cobject': None,
        'howto/doanddont': None,
        'howto/webservers': None,
        'library/strings': 'library/text',
        'library/stringio': ('library/io', 'io.StringIO'),
        # 'library/sets': ('library/stdtypes', 'set'),
        # 'library/sets': ('library/stdtypes', 'set-types-set-frozenset'),
        'library/sets': None,
        'library/userdict': ('library/collections', 'userdict-objects'),
        'library/repr': 'library/reprlib',
        'library/copy_reg': 'library/copyreg',
        'library/anydbm': 'library/dbm',
        'library/whichdb': ('library/dbm', 'dbm.whichdb'),
        'library/dumbdbm': ('library/dbm', 'module-dbm.dumb'),
        'library/dbm': ('library/dbm', 'module-dbm.ndbm'),
        'library/gdbm': ('library/dbm', 'module-dbm.gnu'),
        'library/robotparser': 'library/urllib.robotparser',
        # 'library/md5': 'library/hashlib',
        'library/md5': None,
        # 'library/sha': 'library/hashlib',
        'library/sha': None,
        'library/thread': 'library/_thread',
        # Renamed to _dummy_thread but has since been removed completely
        # 'library/dummy_thread': 'library/_dummy_thread',
        'library/dummy_thread': None,
        'library/email-examples': 'library/email.examples',
        # 'library/rfc822': 'library/email',
        'library/rfc822': None,
        'library/htmlparser': 'library/html.parser',
        'library/htmllib': None,
        # 'library/urllib2': 'library/urllib',
        'library/urllib2': 'library/urllib.request',
        'library/httplib': 'library/http.client',
        'library/urlparse': 'library/urllib.parse',
        'library/basehttpserver': 'library/http.server',
        'library/simplehttpserver': ('library/http.server', 'http.server.SimpleHTTPRequestHandler'),
        'library/cgihttpserver': ('library/http.server', 'http.server.CGIHTTPRequestHandler'),
        'library/cookielib': 'library/http.cookiejar',
        'library/cookie': 'library/http.cookies',
        'library/xmlrpclib': 'library/xmlrpc.client',
        'library/simplexmlrpcserver': 'library/xmlrpc.server',
        'library/docxmlrpcserver': ('library/xmlrpc.server', 'documenting-xmlrpc-server'),
        'library/ttk': 'library/tkinter.ttk',
        'library/tix': 'library/tkinter.tix',
        'library/scrolledtext': 'library/tkinter.scrolledtext',
        'library/__builtin__': 'library/builtins',
        'library/_winreg': 'library/winreg',
    },
}


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
