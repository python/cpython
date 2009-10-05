from distutils.core import Extension as _Extension
from distutils.core import Distribution as _Distribution

def _get_unpatched(cls):
    """Protect against re-patching the distutils if reloaded

    Also ensures that no other distutils extension monkeypatched the distutils
    first.
    """
    while cls.__module__.startswith('setuptools'):
        cls, = cls.__bases__
    if not cls.__module__.startswith('distutils'):
        raise AssertionError(
            "distutils has already been patched by %r" % cls
        )
    return cls

_Distribution = _get_unpatched(_Distribution)
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
