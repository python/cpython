#!/usr/bin/env python3
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
    items = sorted(data.items())
    for k, v in items:
        print('    %-40s%r,' % ('%r:' % k, v))

def print_differences(data, olddata):
    items = sorted(olddata.items())
    for k, v in items:
        if k not in data:
            print('#    removed %r' % k)
        elif olddata[k] != data[k]:
            print('#    updated %r -> %r to %r' % \
                  (k, olddata[k], data[k]))
        # Additions are not mentioned

if __name__ == '__main__':
    data = locale.locale_alias.copy()
    data.update(parse(LOCALE_ALIAS))
    print_differences(data, locale.locale_alias)
    print()
    print('locale_alias = {')
    pprint(data)
    print('}')
