#
# Copyright (c) 2008-2012 Stefan Krah. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#


# Generate PEP-3101 format strings.


import os, sys, locale, random
import platform, subprocess
from test.support import import_fresh_module
from distutils.spawn import find_executable

C = import_fresh_module('decimal', fresh=['_decimal'])
P = import_fresh_module('decimal', blocked=['_decimal'])


windows_lang_strings = [
  "chinese", "chinese-simplified", "chinese-traditional", "czech", "danish",
  "dutch", "belgian", "english", "australian", "canadian", "english-nz",
  "english-uk", "english-us", "finnish", "french", "french-belgian",
  "french-canadian", "french-swiss", "german", "german-austrian",
  "german-swiss", "greek", "hungarian", "icelandic", "italian", "italian-swiss",
  "japanese", "korean", "norwegian", "norwegian-bokmal", "norwegian-nynorsk",
  "polish", "portuguese", "portuguese-brazil", "russian", "slovak", "spanish",
  "spanish-mexican", "spanish-modern", "swedish", "turkish",
]

preferred_encoding = {
  'cs_CZ': 'ISO8859-2',
  'cs_CZ.iso88592': 'ISO8859-2',
  'czech': 'ISO8859-2',
  'eesti': 'ISO8859-1',
  'estonian': 'ISO8859-1',
  'et_EE': 'ISO8859-15',
  'et_EE.ISO-8859-15': 'ISO8859-15',
  'et_EE.iso885915': 'ISO8859-15',
  'et_EE.iso88591': 'ISO8859-1',
  'fi_FI.iso88591': 'ISO8859-1',
  'fi_FI': 'ISO8859-15',
  'fi_FI@euro': 'ISO8859-15',
  'fi_FI.iso885915@euro': 'ISO8859-15',
  'finnish': 'ISO8859-1',
  'lv_LV': 'ISO8859-13',
  'lv_LV.iso885913': 'ISO8859-13',
  'nb_NO': 'ISO8859-1',
  'nb_NO.iso88591': 'ISO8859-1',
  'bokmal': 'ISO8859-1',
  'nn_NO': 'ISO8859-1',
  'nn_NO.iso88591': 'ISO8859-1',
  'no_NO': 'ISO8859-1',
  'norwegian': 'ISO8859-1',
  'nynorsk': 'ISO8859-1',
  'ru_RU': 'ISO8859-5',
  'ru_RU.iso88595': 'ISO8859-5',
  'russian': 'ISO8859-5',
  'ru_RU.KOI8-R': 'KOI8-R',
  'ru_RU.koi8r': 'KOI8-R',
  'ru_RU.CP1251': 'CP1251',
  'ru_RU.cp1251': 'CP1251',
  'sk_SK': 'ISO8859-2',
  'sk_SK.iso88592': 'ISO8859-2',
  'slovak': 'ISO8859-2',
  'sv_FI': 'ISO8859-1',
  'sv_FI.iso88591': 'ISO8859-1',
  'sv_FI@euro': 'ISO8859-15',
  'sv_FI.iso885915@euro': 'ISO8859-15',
  'uk_UA': 'KOI8-U',
  'uk_UA.koi8u': 'KOI8-U'
}

integers = [
  "",
  "1",
  "12",
  "123",
  "1234",
  "12345",
  "123456",
  "1234567",
  "12345678",
  "123456789",
  "1234567890",
  "12345678901",
  "123456789012",
  "1234567890123",
  "12345678901234",
  "123456789012345",
  "1234567890123456",
  "12345678901234567",
  "123456789012345678",
  "1234567890123456789",
  "12345678901234567890",
  "123456789012345678901",
  "1234567890123456789012",
]

numbers = [
  "0", "-0", "+0",
  "0.0", "-0.0", "+0.0",
  "0e0", "-0e0", "+0e0",
  ".0", "-.0",
  ".1", "-.1",
  "1.1", "-1.1",
  "1e1", "-1e1"
]

# Get the list of available locales.
if platform.system() == 'Windows':
    locale_list = windows_lang_strings
else:
    locale_list = ['C']
    if os.path.isfile("/var/lib/locales/supported.d/local"):
        # On Ubuntu, `locale -a` gives the wrong case for some locales,
        # so we get the correct names directly:
        with open("/var/lib/locales/supported.d/local") as f:
            locale_list = [loc.split()[0] for loc in f.readlines() \
                           if not loc.startswith('#')]
    elif find_executable('locale'):
        locale_list = subprocess.Popen(["locale", "-a"],
                          stdout=subprocess.PIPE).communicate()[0]
        try:
            locale_list = locale_list.decode()
        except UnicodeDecodeError:
            # Some distributions insist on using latin-1 characters
            # in their locale names.
            locale_list = locale_list.decode('latin-1')
        locale_list = locale_list.split('\n')
try:
    locale_list.remove('')
except ValueError:
    pass

# Debian
if os.path.isfile("/etc/locale.alias"):
    with open("/etc/locale.alias") as f:
        while 1:
            try:
                line = f.readline()
            except UnicodeDecodeError:
                continue
            if line == "":
                break
            if line.startswith('#'):
                continue
            x = line.split()
            if len(x) == 2:
                if x[0] in locale_list:
                    locale_list.remove(x[0])

# FreeBSD
if platform.system() == 'FreeBSD':
    # http://www.freebsd.org/cgi/query-pr.cgi?pr=142173
    # en_GB.US-ASCII has 163 as the currency symbol.
    for loc in ['it_CH.ISO8859-1', 'it_CH.ISO8859-15', 'it_CH.UTF-8',
                'it_IT.ISO8859-1', 'it_IT.ISO8859-15', 'it_IT.UTF-8',
                'sl_SI.ISO8859-2', 'sl_SI.UTF-8',
                'en_GB.US-ASCII']:
        try:
            locale_list.remove(loc)
        except ValueError:
            pass

# Print a testcase in the format of the IBM tests (for runtest.c):
def get_preferred_encoding():
    loc = locale.setlocale(locale.LC_CTYPE)
    if loc in preferred_encoding:
        return preferred_encoding[loc]
    else:
        return locale.getpreferredencoding()

def printit(testno, s, fmt, encoding=None):
    if not encoding:
        encoding = get_preferred_encoding()
    try:
        result = format(P.Decimal(s), fmt)
        fmt = str(fmt.encode(encoding))[2:-1]
        result = str(result.encode(encoding))[2:-1]
        if "'" in result:
            sys.stdout.write("xfmt%d  format  %s  '%s'  ->  \"%s\"\n"
                             % (testno, s, fmt, result))
        else:
            sys.stdout.write("xfmt%d  format  %s  '%s'  ->  '%s'\n"
                             % (testno, s, fmt, result))
    except Exception as err:
        sys.stderr.write("%s  %s  %s\n" % (err, s, fmt))


# Check if an integer can be converted to a valid fill character.
def check_fillchar(i):
    try:
        c = chr(i)
        c.encode('utf-8').decode()
        format(P.Decimal(0), c + '<19g')
        return c
    except:
        return None

# Generate all unicode characters that are accepted as
# fill characters by decimal.py.
def all_fillchars():
    for i in range(0, 0x110002):
        c = check_fillchar(i)
        if c: yield c

# Return random fill character.
def rand_fillchar():
    while 1:
        i = random.randrange(0, 0x110002)
        c = check_fillchar(i)
        if c: return c

# Generate random format strings
# [[fill]align][sign][#][0][width][.precision][type]
def rand_format(fill, typespec='EeGgFfn%'):
    active = sorted(random.sample(range(7), random.randrange(8)))
    have_align = 0
    s = ''
    for elem in active:
        if elem == 0: # fill+align
            s += fill
            s += random.choice('<>=^')
            have_align = 1
        elif elem == 1: # sign
            s += random.choice('+- ')
        elif elem == 2 and not have_align: # zeropad
            s += '0'
        elif elem == 3: # width
            s += str(random.randrange(1, 100))
        elif elem == 4: # thousands separator
            s += ','
        elif elem == 5: # prec
            s += '.'
            s += str(random.randrange(100))
        elif elem == 6:
            if 4 in active: c = typespec.replace('n', '')
            else: c = typespec
            s += random.choice(c)
    return s

# Partially brute force all possible format strings containing a thousands
# separator. Fall back to random where the runtime would become excessive.
# [[fill]align][sign][#][0][width][,][.precision][type]
def all_format_sep():
    for align in ('', '<', '>', '=', '^'):
        for fill in ('', 'x'):
            if align == '': fill = ''
            for sign in ('', '+', '-', ' '):
                for zeropad in ('', '0'):
                    if align != '': zeropad = ''
                    for width in ['']+[str(y) for y in range(1, 15)]+['101']:
                        for prec in ['']+['.'+str(y) for y in range(15)]:
                            # for type in ('', 'E', 'e', 'G', 'g', 'F', 'f', '%'):
                            type = random.choice(('', 'E', 'e', 'G', 'g', 'F', 'f', '%'))
                            yield ''.join((fill, align, sign, zeropad, width, ',', prec, type))

# Partially brute force all possible format strings with an 'n' specifier.
# [[fill]align][sign][#][0][width][,][.precision][type]
def all_format_loc():
    for align in ('', '<', '>', '=', '^'):
        for fill in ('', 'x'):
            if align == '': fill = ''
            for sign in ('', '+', '-', ' '):
                for zeropad in ('', '0'):
                    if align != '': zeropad = ''
                    for width in ['']+[str(y) for y in range(1, 20)]+['101']:
                        for prec in ['']+['.'+str(y) for y in range(1, 20)]:
                            yield ''.join((fill, align, sign, zeropad, width, prec, 'n'))

# Generate random format strings with a unicode fill character
# [[fill]align][sign][#][0][width][,][.precision][type]
def randfill(fill):
    active = sorted(random.sample(range(5), random.randrange(6)))
    s = ''
    s += str(fill)
    s += random.choice('<>=^')
    for elem in active:
        if elem == 0: # sign
            s += random.choice('+- ')
        elif elem == 1: # width
            s += str(random.randrange(1, 100))
        elif elem == 2: # thousands separator
            s += ','
        elif elem == 3: # prec
            s += '.'
            s += str(random.randrange(100))
        elif elem == 4:
            if 2 in active: c = 'EeGgFf%'
            else: c = 'EeGgFfn%'
            s += random.choice(c)
    return s

# Generate random format strings with random locale setting
# [[fill]align][sign][#][0][width][,][.precision][type]
def rand_locale():
    try:
        loc = random.choice(locale_list)
        locale.setlocale(locale.LC_ALL, loc)
    except locale.Error as err:
        pass
    active = sorted(random.sample(range(5), random.randrange(6)))
    s = ''
    have_align = 0
    for elem in active:
        if elem == 0: # fill+align
            s += chr(random.randrange(32, 128))
            s += random.choice('<>=^')
            have_align = 1
        elif elem == 1: # sign
            s += random.choice('+- ')
        elif elem == 2 and not have_align: # zeropad
            s += '0'
        elif elem == 3: # width
            s += str(random.randrange(1, 100))
        elif elem == 4: # prec
            s += '.'
            s += str(random.randrange(100))
    s += 'n'
    return s
