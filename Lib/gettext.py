"""Internationalization and localization support.

This module provides internationalization (I18N) and localization (L10N)
support for your Python programs by providing an interface to the GNU gettext
message catalog library.

I18N refers to the operation by which a program is made aware of multiple
languages.  L10N refers to the adaptation of your program, once
internationalized, to the local language and cultural habits.

"""

# This module represents the integration of work, contributions, feedback, and
# suggestions from the following people:
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
#
# Francois Pinard and Marc-Andre Lemburg also contributed valuably to this
# module.
#
# TODO:
# - Lazy loading of .mo files.  Currently the entire catalog is loaded into
#   memory, but that's probably bad for large translated programs.  Instead,
#   the lexical sort of original strings in GNU .mo files should be exploited
#   to do binary searches and lazy initializations.  Or you might want to use
#   the undocumented double-hash algorithm for .mo files with hash tables, but
#   you'll need to study the GNU gettext code to do this.
#
# - Support Solaris .mo file formats.  Unfortunately, we've been unable to
#   find this format documented anywhere.

import os
import sys
import struct
from errno import ENOENT

_default_localedir = os.path.join(sys.prefix, 'share', 'locale')



def _expand_lang(locale):
    from locale import normalize
    locale = normalize(locale)
    COMPONENT_CODESET   = 1 << 0
    COMPONENT_TERRITORY = 1 << 1
    COMPONENT_MODIFIER  = 1 << 2
    # split up the locale into its base components
    mask = 0
    pos = locale.find('@')
    if pos >= 0:
        modifier = locale[pos:]
        locale = locale[:pos]
        mask |= COMPONENT_MODIFIER
    else:
        modifier = ''
    pos = locale.find('.')
    if pos >= 0:
        codeset = locale[pos:]
        locale = locale[:pos]
        mask |= COMPONENT_CODESET
    else:
        codeset = ''
    pos = locale.find('_')
    if pos >= 0:
        territory = locale[pos:]
        locale = locale[:pos]
        mask |= COMPONENT_TERRITORY
    else:
        territory = ''
    language = locale
    ret = []
    for i in range(mask+1):
        if not (i & ~mask):  # if all components for this combo exist ...
            val = language
            if i & COMPONENT_TERRITORY: val += territory
            if i & COMPONENT_CODESET:   val += codeset
            if i & COMPONENT_MODIFIER:  val += modifier
            ret.append(val)
    ret.reverse()
    return ret



class NullTranslations:
    def __init__(self, fp=None):
        self._info = {}
        self._charset = None
        if fp:
            self._parse(fp)

    def _parse(self, fp):
        pass

    def gettext(self, message):
        return message

    def ugettext(self, message):
        return unicode(message)

    def info(self):
        return self._info

    def charset(self):
        return self._charset

    def install(self, unicode=0):
        import __builtin__
        __builtin__.__dict__['_'] = unicode and self.ugettext or self.gettext


class GNUTranslations(NullTranslations):
    # Magic number of .mo files
    LE_MAGIC = 0x950412de
    BE_MAGIC = 0xde120495

    def _parse(self, fp):
        """Override this method to support alternative .mo formats."""
        # We need to & all 32 bit unsigned integers with 0xffffffff for
        # portability to 64 bit machines.
        MASK = 0xffffffff
        unpack = struct.unpack
        filename = getattr(fp, 'name', '')
        # Parse the .mo file header, which consists of 5 little endian 32
        # bit words.
        self._catalog = catalog = {}
        buf = fp.read()
        buflen = len(buf)
        # Are we big endian or little endian?
        magic = unpack('<i', buf[:4])[0] & MASK
        if magic == self.LE_MAGIC:
            version, msgcount, masteridx, transidx = unpack('<4i', buf[4:20])
            ii = '<ii'
        elif magic == self.BE_MAGIC:
            version, msgcount, masteridx, transidx = unpack('>4i', buf[4:20])
            ii = '>ii'
        else:
            raise IOError(0, 'Bad magic number', filename)
        # more unsigned ints
        msgcount &= MASK
        masteridx &= MASK
        transidx &= MASK
        # Now put all messages from the .mo file buffer into the catalog
        # dictionary.
        for i in xrange(0, msgcount):
            mlen, moff = unpack(ii, buf[masteridx:masteridx+8])
            moff &= MASK
            mend = moff + (mlen & MASK)
            tlen, toff = unpack(ii, buf[transidx:transidx+8])
            toff &= MASK
            tend = toff + (tlen & MASK)
            if mend < buflen and tend < buflen:
                tmsg = buf[toff:tend]
                catalog[buf[moff:mend]] = tmsg
            else:
                raise IOError(0, 'File is corrupt', filename)
            # See if we're looking at GNU .mo conventions for metadata
            if mlen == 0 and tmsg.lower().startswith('project-id-version:'):
                # Catalog description
                for item in tmsg.split('\n'):
                    item = item.strip()
                    if not item:
                        continue
                    k, v = item.split(':', 1)
                    k = k.strip().lower()
                    v = v.strip()
                    self._info[k] = v
                    if k == 'content-type':
                        self._charset = v.split('charset=')[1]
            # advance to next entry in the seek tables
            masteridx += 8
            transidx += 8

    def gettext(self, message):
        return self._catalog.get(message, message)

    def ugettext(self, message):
        tmsg = self._catalog.get(message, message)
        return unicode(tmsg, self._charset)



# Locate a .mo file using the gettext strategy
def find(domain, localedir=None, languages=None):
    # Get some reasonable defaults for arguments that were not supplied
    if localedir is None:
        localedir = _default_localedir
    if languages is None:
        languages = []
        for envar in ('LANGUAGE', 'LC_ALL', 'LC_MESSAGES', 'LANG'):
            val = os.environ.get(envar)
            if val:
                languages = val.split(':')
                break
        if 'C' not in languages:
            languages.append('C')
    # now normalize and expand the languages
    nelangs = []
    for lang in languages:
        for nelang in _expand_lang(lang):
            if nelang not in nelangs:
                nelangs.append(nelang)
    # select a language
    for lang in nelangs:
        if lang == 'C':
            break
        mofile = os.path.join(localedir, lang, 'LC_MESSAGES', '%s.mo' % domain)
        if os.path.exists(mofile):
            return mofile
    return None



# a mapping between absolute .mo file path and Translation object
_translations = {}

def translation(domain, localedir=None, languages=None, class_=None):
    if class_ is None:
        class_ = GNUTranslations
    mofile = find(domain, localedir, languages)
    if mofile is None:
        raise IOError(ENOENT, 'No translation file found for domain', domain)
    key = os.path.abspath(mofile)
    # TBD: do we need to worry about the file pointer getting collected?
    # Avoid opening, reading, and parsing the .mo file after it's been done
    # once.
    t = _translations.get(key)
    if t is None:
        t = _translations.setdefault(key, class_(open(mofile, 'rb')))
    return t



def install(domain, localedir=None, unicode=0):
    translation(domain, localedir).install(unicode)



# a mapping b/w domains and locale directories
_localedirs = {}
# current global domain, `messages' used for compatibility w/ GNU gettext
_current_domain = 'messages'


def textdomain(domain=None):
    global _current_domain
    if domain is not None:
        _current_domain = domain
    return _current_domain


def bindtextdomain(domain, localedir=None):
    global _localedirs
    if localedir is not None:
        _localedirs[domain] = localedir
    return _localedirs.get(domain, _default_localedir)


def dgettext(domain, message):
    try:
        t = translation(domain, _localedirs.get(domain, None))
    except IOError:
        return message
    return t.gettext(message)
    

def gettext(message):
    return dgettext(_current_domain, message)


# dcgettext() has been deemed unnecessary and is not implemented.

# James Henstridge's Catalog constructor from GNOME gettext.  Documented usage
# was:
#
#    import gettext
#    cat = gettext.Catalog(PACKAGE, localedir=LOCALEDIR)
#    _ = cat.gettext
#    print _('Hello World')

# The resulting catalog object currently don't support access through a
# dictionary API, which was supported (but apparently unused) in GNOME
# gettext.

Catalog = translation
