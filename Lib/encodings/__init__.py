""" Standard "encodings" Package

    Standard Python encoding modules are stored in this package
    directory.

    Codec modules must have names corresponding to normalized encoding
    names as defined in the normalize_encoding() function below, e.g.
    'utf-8' must be implemented by the module 'utf_8.py'.

    Each codec module must export the following interface:

    * getregentry() -> (encoder, decoder, stream_reader, stream_writer)
    The getregentry() API must return callable objects which adhere to
    the Python Codec Interface Standard.

    In addition, a module may optionally also define the following
    APIs which are then used by the package's codec search function:

    * getaliases() -> sequence of encoding name strings to use as aliases

    Alias names returned by getaliases() must be normalized encoding
    names as defined by normalize_encoding().

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"

import codecs, exceptions, types

_cache = {}
_unknown = '--unknown--'
_import_tail = ['*']
_norm_encoding_map = ('                                              . '
                      '0123456789       ABCDEFGHIJKLMNOPQRSTUVWXYZ     '
                      ' abcdefghijklmnopqrstuvwxyz                     '
                      '                                                '
                      '                                                '
                      '                ')

class CodecRegistryError(exceptions.LookupError,
                         exceptions.SystemError):
    pass

def normalize_encoding(encoding):

    """ Normalize an encoding name.

        Normalization works as follows: all non-alphanumeric
        characters except the dot used for Python package names are
        collapsed and replaced with a single underscore, e.g. '  -;#'
        becomes '_'. Leading and trailing underscores are removed.

        Note that encoding names should be ASCII only; if they do use
        non-ASCII characters, these must be Latin-1 compatible.

    """
    # Make sure we have an 8-bit string, because .translate() works
    # differently for Unicode strings.
    if type(encoding) is types.UnicodeType:
        # Note that .encode('latin-1') does *not* use the codec
        # registry, so this call doesn't recurse. (See unicodeobject.c
        # PyUnicode_AsEncodedString() for details)
        encoding = encoding.encode('latin-1')
    return '_'.join(encoding.translate(_norm_encoding_map).split())

def search_function(encoding):

    # Cache lookup
    entry = _cache.get(encoding, _unknown)
    if entry is not _unknown:
        return entry

    # Import the module:
    #
    # First look in the encodings package, then try to lookup the
    # encoding in the aliases mapping and retry the import using the
    # default import module lookup scheme with the alias name.
    #
    modname = normalize_encoding(encoding)
    try:
        mod = __import__('encodings.' + modname,
                         globals(), locals(), _import_tail)
    except ImportError:
        import aliases
        modname = (aliases.aliases.get(modname) or
                   aliases.aliases.get(modname.replace('.', '_')) or
                   modname)
        try:
            mod = __import__(modname, globals(), locals(), _import_tail)
        except ImportError:
            mod = None

    try:
        getregentry = mod.getregentry
    except AttributeError:
        # Not a codec module
        mod = None

    if mod is None:
        # Cache misses
        _cache[encoding] = None
        return None

    # Now ask the module for the registry entry
    entry = tuple(getregentry())
    if len(entry) != 4:
        raise CodecRegistryError,\
              'module "%s" (%s) failed to register' % \
              (mod.__name__, mod.__file__)
    for obj in entry:
        if not callable(obj):
            raise CodecRegistryError,\
                  'incompatible codecs in module "%s" (%s)' % \
                  (mod.__name__, mod.__file__)

    # Cache the codec registry entry
    _cache[encoding] = entry

    # Register its aliases (without overwriting previously registered
    # aliases)
    try:
        codecaliases = mod.getaliases()
    except AttributeError:
        pass
    else:
        import aliases
        for alias in codecaliases:
            if not aliases.aliases.has_key(alias):
                aliases.aliases[alias] = modname

    # Return the registry entry
    return entry

# Register the search_function in the Python codec registry
codecs.register(search_function)
