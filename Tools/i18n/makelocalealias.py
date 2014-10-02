#!/usr/bin/env python3
"""
    Convert the X11 locale.alias file into a mapping dictionary suitable
    for locale.py.

    Written by Marc-Andre Lemburg <mal@genix.com>, 2004-12-10.

"""
import locale
import sys
_locale = locale

# Location of the X11 alias file.
LOCALE_ALIAS = '/usr/share/X11/locale/locale.alias'
# Location of the glibc SUPPORTED locales file.
SUPPORTED = '/usr/share/i18n/SUPPORTED'

def parse(filename):

    with open(filename, encoding='latin1') as f:
        lines = list(f)
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
        data[locale] = alias
    return data

def parse_glibc_supported(filename):

    with open(filename, encoding='latin1') as f:
        lines = list(f)
    data = {}
    for line in lines:
        line = line.strip()
        if not line:
            continue
        if line[:1] == '#':
            continue
        line = line.replace('/', ' ').strip()
        line = line.rstrip('\\').rstrip()
        words = line.split()
        if len(words) != 2:
            continue
        alias, alias_encoding = words
        # Lower-case locale
        locale = alias.lower()
        # Normalize encoding, if given
        if '.' in locale:
            lang, encoding = locale.split('.')[:2]
            encoding = encoding.replace('-', '')
            encoding = encoding.replace('_', '')
            locale = lang + '.' + encoding
        # Add an encoding to alias
        alias, _, modifier = alias.partition('@')
        alias = _locale._replace_encoding(alias, alias_encoding)
        if modifier and not (modifier == 'euro' and alias_encoding == 'ISO-8859-15'):
            alias += '@' + modifier
        data[locale] = alias
    return data

def pprint(data):
    items = sorted(data.items())
    for k, v in items:
        print('    %-40s%a,' % ('%a:' % k, v))

def print_differences(data, olddata):
    items = sorted(olddata.items())
    for k, v in items:
        if k not in data:
            print('#    removed %a' % k)
        elif olddata[k] != data[k]:
            print('#    updated %a -> %a to %a' % \
                  (k, olddata[k], data[k]))
        # Additions are not mentioned

def optimize(data):
    locale_alias = locale.locale_alias
    locale.locale_alias = data.copy()
    for k, v in data.items():
        del locale.locale_alias[k]
        if locale.normalize(k) != v:
            locale.locale_alias[k] = v
    newdata = locale.locale_alias
    errors = check(data)
    locale.locale_alias = locale_alias
    if errors:
        sys.exit(1)
    return newdata

def check(data):
    # Check that all alias definitions from the X11 file
    # are actually mapped to the correct alias locales.
    errors = 0
    for k, v in data.items():
        if locale.normalize(k) != v:
            print('ERROR: %a -> %a != %a' % (k, locale.normalize(k), v),
                  file=sys.stderr)
            errors += 1
    return errors

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--locale-alias', default=LOCALE_ALIAS,
                        help='location of the X11 alias file '
                             '(default: %a)' % LOCALE_ALIAS)
    parser.add_argument('--glibc-supported', default=SUPPORTED,
                        help='location of the glibc SUPPORTED locales file '
                             '(default: %a)' % SUPPORTED)
    args = parser.parse_args()

    data = locale.locale_alias.copy()
    data.update(parse_glibc_supported(args.glibc_supported))
    data.update(parse(args.locale_alias))
    while True:
        # Repeat optimization while the size is decreased.
        n = len(data)
        data = optimize(data)
        if len(data) == n:
            break
    print_differences(data, locale.locale_alias)
    print()
    print('locale_alias = {')
    pprint(data)
    print('}')
