"""Internationalization and localization support.

This module provides internationalization (I18N) and localization (L10N)
support for your Python programs by providing an interface to the GNU gettext
message catalog library.

I18N refers to the operation by which a program is made aware of multiple
languages.  L10N refers to the adaptation of your program, once
internationalized, to the local language and cultural habits.  In order to
provide multilingual messages for your Python programs, you need to take the
following steps:

    - prepare your program by specially marking translatable strings
    - run a suite of tools over your marked program files to generate raw
      messages catalogs
    - create language specific translations of the message catalogs
    - use this module so that message strings are properly translated

In order to prepare your program for I18N, you need to look at all the strings
in your program.  Any string that needs to be translated should be marked by
wrapping it in _('...') -- i.e. a call to the function `_'.  For example:

    filename = 'mylog.txt'
    message = _('writing a log message')
    fp = open(filename, 'w')
    fp.write(message)
    fp.close()

In this example, the string `writing a log message' is marked as a candidate
for translation, while the strings `mylog.txt' and `w' are not.

The GNU gettext package provides a tool, called xgettext, that scans C and C++
source code looking for these specially marked strings.  xgettext generates
what are called `.pot' files, essentially structured human readable files
which contain every marked string in the source code.  These .pot files are
copied and handed over to translators who write language-specific versions for
every supported language.

For I18N Python programs however, xgettext won't work; it doesn't understand
the myriad of string types support by Python.  The standard Python
distribution provides a tool called pygettext that does though (found in the
Tools/i18n directory).  This is a command line script that supports a similar
interface as xgettext; see its documentation for details.  Once you've used
pygettext to create your .pot files, you can use the standard GNU gettext
tools to generate your machine-readable .mo files, which are what's used by
this module.

In the simple case, to use this module then, you need only add the following
bit of code to the main driver file of your application:

    import gettext
    gettext.install()

This sets everything up so that your _('...') function calls Just Work.  In
other words, it installs `_' in the builtins namespace for convenience.  You
can skip this step and do it manually by the equivalent code:

    import gettext
    import __builtin__
    __builtin__['_'] = gettext.gettext

Once you've done this, you probably want to call bindtextdomain() and
textdomain() to get the domain set up properly.  Again, for convenience, you
can pass the domain and localedir to install to set everything up in one fell
swoop:

    import gettext
    gettext.install('mydomain', '/my/locale/dir')

If your program needs to support many languages at the same time, you will
want to create Translation objects explicitly, like so:

    import gettext
    gettext.install()

    lang1 = gettext.Translations(open('/path/to/my/lang1/messages.mo'))
    lang2 = gettext.Translations(open('/path/to/my/lang2/messages.mo'))
    lang3 = gettext.Translations(open('/path/to/my/lang3/messages.mo'))

    gettext.set(lang1)
    # all _() will now translate to language 1
    gettext.set(lang2)
    # all _() will now translate to language 2

Currently, only GNU gettext format binary .mo files are supported.

"""

# This module represents the integration of work from the following authors:
#
# Martin von Loewis, who wrote the initial implementation of the underlying
# C-based libintlmodule (later renamed _gettext), along with a skeletal
# gettext.py implementation.
#
# Peter Funk, who wrote fintl.py, a fairly complete wrapper around intlmodule,
# which also included a pure-Python implementation to read .mo files if
# intlmodule wasn't available.
#
# James Henstridge, who also wrote a gettext.py module, which has some
# interesting, but currently unsupported experimental features: the notion of
# a Catalog class and instances, and the ability to add to a catalog file via
# a Python API.
#
# Barry Warsaw integrated these modules, wrote the .install() API and code,
# and conformed all C and Python code to Python's coding standards.

import os
import sys
import struct
from UserDict import UserDict



# globals
_translations = {}
_current_translation = None
_current_domain = 'messages'

# Domain to directory mapping, for use by bindtextdomain()
_localedirs = {}



class GNUTranslations(UserDict):
    # Magic number of .mo files
    MAGIC = 0x950412de

    def __init__(self, fp):
        if fp is None:
            d = {}
        else:
            d = self._parse(fp)
        UserDict.__init__(self, d)

    def _parse(self, fp):
        """Override this method to support alternative .mo formats."""
        unpack = struct.unpack
        filename = getattr(fp, 'name', '')
        # Parse the .mo file header, which consists of 5 little endian 32
        # bit words.
        catalog = {}
        buf = fp.read()
        magic, version, msgcount, masteridx, transidx = unpack(
            '<5i', buf[:20])
        if magic <> self.MAGIC:
            raise IOError(0, 'Bad magic number', filename)
        #
        # Now put all messages from the .mo file buffer into the catalog
        # dictionary.
        for i in xrange(0, msgcount):
            mstart = unpack('<i', buf[masteridx+4:masteridx+8])[0]
            mend = mstart + unpack('<i', buf[masteridx:masteridx+4])[0]
            tstart = unpack('<i', buf[transidx+4:transidx+8])[0]
            tend = tstart + unpack('<i', buf[transidx:transidx+4])[0]
            if mend < len(buf) and tend < len(buf):
                catalog[buf[mstart:mend]] = buf[tstart:tend]
            else:
                raise IOError(0, 'File is corrupt', filename)
            #
            # advance to next entry in the seek tables
            masteridx = masteridx + 8
            transidx = transidx + 8
        return catalog



# By default, use GNU gettext format .mo files
Translations = GNUTranslations

# Locate a .mo file using the gettext strategy
def _find(localedir=None, languages=None, category=None, domain=None):
    global _current_domain
    global _localedirs

    # Get some reasonable defaults for arguments that were not supplied
    if domain is None:
        domain = _current_domain
    if category is None:
        category = 'LC_MESSAGES'
    if localedir is None:
        localedir = _localedirs.get(
            domain,
            # TBD: The default localedir is actually system dependent.  I
            # don't know of a good platform-consistent and portable way to
            # default it, so instead, we'll just use sys.prefix.  Most
            # programs should be calling bindtextdomain() or such explicitly
            # anyway.
            os.path.join(sys.prefix, 'share', 'locale'))
    if languages is None:
        languages = []
        for envar in ('LANGUAGE', 'LC_ALL', 'LC_MESSAGES', 'LANG'):
            val = os.environ.get(envar)
            if val:
                languages = val.split(':')
                break
        if 'C' not in languages:
            languages.append('C')
    # select a language
    for lang in languages:
        if lang == 'C':
            break
        mofile = os.path.join(localedir, lang, category, '%s.mo' % domain)
        # see if it's in the cache
        mo = _translations.get(mofile)
        if mo:
            return mo
        fp = None
        try:
            try:
                fp = open(mofile, 'rb')
                t = Translations(fp)
                _translations[mofile] = t
                return t
            except IOError:
                pass
        finally:
            if fp:
                fp.close()
    return {}



def bindtextdomain(domain=None, localedir=None):
    """Bind domain to a file in the specified directory."""
    global _localedirs
    if domain is None:
        return None
    if localedir is None:
        return _localedirs.get(domain, _localedirs.get('C'))
    _localedirs[domain] = localedir
    return localedir


def textdomain(domain=None):
    """Change or query the current global domain."""
    global _current_domain
    if domain is None:
        return _current_domain
    else:
        _current_domain = domain
        return domain


def gettext(message):
    """Return localized version of a message."""
    return _find().get(message, message)


def dgettext(domain, message):
    """Like gettext(), but look up message in specified domain."""
    return _find(domain=domain).get(message, message)


def dcgettext(domain, message, category):
    try:
        from locale import LC_CTYPE, LC_TIME, LC_COLLATE
        from locale import LC_MONETARY, LC_MESSAGES, LC_NUMERIC
    except ImportError:
        return message
    categories = {
        LC_CTYPE    : 'LC_CTYPE',
        LC_TIME     : 'LC_TIME',
        LC_COLLATE  : 'LC_COLLATE',
        LC_MONETARY : 'LC_MONETARY',
        LC_MESSAGES : 'LC_MESSAGES',
        LC_NUMERIC  : 'LC_NUMERIC'
        }
    return _find(domain=domain, category=category).get(message, message)



# A higher level API
def set(translation):
    global _current_translation
    _current_translation = translation


def get():
    global _current_translation
    return _current_translation


def install(domain=None, localedir=None):
    import __builtin__
    __builtin__.__dict__['_'] = gettext
    if domain is not None:
        bindtextdomain(domain, localedir)
        textdomain(domain)
