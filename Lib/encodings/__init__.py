""" Standard "encodings" Package

    Standard Python encoding modules are stored in this package
    directory.

    Codec modules must have names corresponding to standard lower-case
    encoding names with hyphens mapped to underscores, e.g. 'utf-8' is
    implemented by the module 'utf_8.py'.

    Each codec module must export the following interface:

    * getregentry() -> (encoder, decoder, stream_reader, stream_writer)
    The getregentry() API must return callable objects which adhere to
    the Python Codec Interface Standard.

    In addition, a module may optionally also define the following
    APIs which are then used by the package's codec search function:

    * getaliases() -> sequence of encoding name strings to use as aliases

    Alias names returned by getaliases() must be standard encoding
    names as defined above (lower-case, hyphens converted to
    underscores).

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"

import codecs,aliases,exceptions

_cache = {}
_unknown = '--unknown--'

class CodecRegistryError(exceptions.LookupError,
                         exceptions.SystemError):
    pass

def search_function(encoding):
    
    # Cache lookup
    entry = _cache.get(encoding,_unknown)
    if entry is not _unknown:
        return entry

    # Import the module
    modname = encoding.replace('-', '_')
    modname = aliases.aliases.get(modname,modname)
    try:
        mod = __import__(modname,globals(),locals(),'*')
    except ImportError,why:
        # cache misses
        _cache[encoding] = None
        return None

    try:
        getregentry = mod.getregentry
    except AttributeError:
        # Not a codec module
        _cache[encoding] = None
        return None
    
    # Now ask the module for the registry entry
    try:
        entry = tuple(getregentry())
    except AttributeError:
        entry = ()
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
        for alias in codecaliases:
            if not aliases.aliases.has_key(alias):
                aliases.aliases[alias] = modname

    # Return the registry entry
    return entry

# Register the search_function in the Python codec registry
codecs.register(search_function)
