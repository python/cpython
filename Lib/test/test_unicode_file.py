# Test some Unicode file name semantics
# We dont test many operations on files other than
# that their names can be used with Unicode characters.
import os

from test_support import verify, TestSkipped, TESTFN_UNICODE
try:
    from test_support import TESTFN_ENCODING
    oldlocale = None
except ImportError:
    import locale
    # try to run the test in an UTF-8 locale. If this locale is not
    # available, avoid running the test since the locale's encoding
    # might not support TESTFN_UNICODE. Likewise, if the system does
    # not support locale.CODESET, Unicode file semantics is not
    # available, either.
    oldlocale = locale.setlocale(locale.LC_CTYPE)
    try:
        locale.setlocale(locale.LC_CTYPE,"en_US.UTF-8")
        TESTFN_ENCODING = locale.nl_langinfo(locale.CODESET)
    except (locale.Error, AttributeError):
        raise TestSkipped("No Unicode filesystem semantics on this platform.")

TESTFN_ENCODED = TESTFN_UNICODE.encode(TESTFN_ENCODING)

# Check with creation as Unicode string.
f = open(TESTFN_UNICODE, 'wb')
if not os.path.isfile(TESTFN_UNICODE):
    print "File doesn't exist after creating it"

if not os.path.isfile(TESTFN_ENCODED):
    print "File doesn't exist (encoded string) after creating it"

f.close()

# Test stat and chmod
if os.stat(TESTFN_ENCODED) != os.stat(TESTFN_UNICODE):
    print "os.stat() did not agree on the 2 filenames"
os.chmod(TESTFN_ENCODED, 0777)
os.chmod(TESTFN_UNICODE, 0777)

# Test rename
os.rename(TESTFN_ENCODED, TESTFN_ENCODED + ".new")
os.rename(TESTFN_UNICODE+".new", TESTFN_ENCODED)

os.unlink(TESTFN_ENCODED)
if os.path.isfile(TESTFN_ENCODED) or \
   os.path.isfile(TESTFN_UNICODE):
    print "File exists after deleting it"

# Check with creation as encoded string.
f = open(TESTFN_ENCODED, 'wb')
if not os.path.isfile(TESTFN_UNICODE) or \
   not os.path.isfile(TESTFN_ENCODED):
    print "File doesn't exist after creating it"

path, base = os.path.split(os.path.abspath(TESTFN_ENCODED))
if base not in os.listdir(path):
    print "Filename did not appear in os.listdir()"

f.close()
os.unlink(TESTFN_UNICODE)
if os.path.isfile(TESTFN_ENCODED) or \
   os.path.isfile(TESTFN_UNICODE):
    print "File exists after deleting it"

# test os.open
f = os.open(TESTFN_ENCODED, os.O_CREAT)
if not os.path.isfile(TESTFN_UNICODE) or \
   not os.path.isfile(TESTFN_ENCODED):
    print "File doesn't exist after creating it"
os.close(f)
os.unlink(TESTFN_UNICODE)

# Test directories etc
cwd = os.getcwd()
abs_encoded = os.path.abspath(TESTFN_ENCODED) + ".dir"
abs_unicode = os.path.abspath(TESTFN_UNICODE) + ".dir"
os.mkdir(abs_encoded)
try:
    os.chdir(abs_encoded)
    os.chdir(abs_unicode)
finally:
    os.chdir(cwd)
    os.rmdir(abs_unicode)
os.mkdir(abs_unicode)
try:
    os.chdir(abs_encoded)
    os.chdir(abs_unicode)
finally:
    os.chdir(cwd)
    os.rmdir(abs_encoded)
print "All the Unicode tests appeared to work"
if oldlocale:
    locale.setlocale(locale.LC_CTYPE, oldlocale)
