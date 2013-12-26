#!/usr/bin/env python
"""
    Convert the X11 locale.alias file into a mapping dictionary suitable
    for locale.py.

    Written by Marc-Andre Lemburg <mal@genix.com>, 2004-12-10.

"""
import locale

# Location of the alias file
LOCALE_ALIAS = '/usr/share/X11/locale/locale.alias'

def parse(filename):

    f = open(filename)
    lines = f.read().splitlines()
    data = {}
    for line in lines:
        line = line.strip()
        if not line:
            continue
        if line[:1] == '#':
            continue
        locale, alias = line.split()
        # Fix non-standard locale names, e.g. ks_IN@devanagari.UTF-8
        if '@' in alias:
            alias_lang, _, alias_mod = alias.partition('@')
            if '.' in alias_mod:
                alias_mod, _, alias_enc = alias_mod.partition('.')
                alias = alias_lang + '.' + alias_enc + '@' + alias_mod
        # Strip ':'
        if locale[-1] == ':':
            locale = locale[:-1]
        # Lower-case locale
        locale = locale.lower()
        # Ignore one letter locale mappings (except for 'c')
        if len(locale) == 1 and locale != 'c':
            continue
        # Normalize encoding, if given
        if '.' in locale:
            lang, encoding = locale.split('.')[:2]
            encoding = encoding.replace('-', '')
            encoding = encoding.replace('_', '')
            locale = lang + '.' + encoding
            if encoding.lower() == 'utf8':
                # Ignore UTF-8 mappings - this encoding should be
                # available for all locales
                continue
        data[locale] = alias
    return data

def pprint(data):

    items = data.items()
    items.sort()
    for k,v in items:
        print '    %-40s%r,' % ('%r:' % k, v)

def print_differences(data, olddata):

    items = olddata.items()
    items.sort()
    for k, v in items:
        if not data.has_key(k):
            print '#    removed %r' % k
        elif olddata[k] != data[k]:
            print '#    updated %r -> %r to %r' % \
                  (k, olddata[k], data[k])
        # Additions are not mentioned

if __name__ == '__main__':
    data = locale.locale_alias.copy()
    data.update(parse(LOCALE_ALIAS))
    print_differences(data, locale.locale_alias)
    print
    print 'locale_alias = {'
    pprint(data)
    print '}'
