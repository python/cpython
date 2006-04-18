from distutils.core import Extension as _Extension
from dist import _get_unpatched
_Extension = _get_unpatched(_Extension)

try:
    from Pyrex.Distutils.build_ext import build_ext
except ImportError:
    have_pyrex = False
else:
    have_pyrex = True


class Extension(_Extension):
    """Extension that uses '.c' files in place of '.pyx' files"""

    if not have_pyrex:
        # convert .pyx extensions to .c 
        def __init__(self,*args,**kw):
            _Extension.__init__(self,*args,**kw)
            sources = []
            for s in self.sources:
                if s.endswith('.pyx'):
                    sources.append(s[:-3]+'c')
                else:
                    sources.append(s)
            self.sources = sources

class Library(Extension):
    """Just like a regular Extension, but built as a library instead"""

import sys, distutils.core, distutils.extension
distutils.core.Extension = Extension
distutils.extension.Extension = Extension
if 'distutils.command.build_ext' in sys.modules:
    sys.modules['distutils.command.build_ext'].Extension = Extension

