import zlib # implied prerequisite
import zipfile, os, StringIO, tempfile
from test_support import TestFailed

srcname = "junk9630"+os.extsep+"tmp"
zipname = "junk9708"+os.extsep+"tmp"


def zipTest(f, compression, srccontents):
    zip = zipfile.ZipFile(f, "w", compression)   # Create the ZIP archive
    zip.write(srcname, "another"+os.extsep+"name")
    zip.write(srcname, srcname)
    zip.close()

    zip = zipfile.ZipFile(f, "r", compression)   # Read the ZIP archive
    readData2 = zip.read(srcname)
    readData1 = zip.read("another"+os.extsep+"name")
    zip.close()

    if readData1 != srccontents or readData2 != srccontents:
        raise TestFailed, "Written data doesn't equal read data."


try:
    fp = open(srcname, "wb")               # Make a source file with some lines
    for i in range(0, 1000):
        fp.write("Test of zipfile line %d.\n" % i)
    fp.close()

    fp = open(srcname, "rb")
    writtenData = fp.read()
    fp.close()

    for file in (zipname, tempfile.TemporaryFile(), StringIO.StringIO()):
        zipTest(file, zipfile.ZIP_STORED, writtenData)

    for file in (zipname, tempfile.TemporaryFile(), StringIO.StringIO()):
        zipTest(file, zipfile.ZIP_DEFLATED, writtenData)

finally:
    if os.path.isfile(srcname):           # Remove temporary files
        os.unlink(srcname)
    if os.path.isfile(zipname):
        os.unlink(zipname)


# This test checks that the ZipFile constructor closes the file object
# it opens if there's an error in the file.  If it doesn't, the traceback
# holds a reference to the ZipFile object and, indirectly, the file object.
# On Windows, this causes the os.unlink() call to fail because the
# underlying file is still open.  This is SF bug #412214.
#
fp = open(srcname, "w")
fp.write("this is not a legal zip file\n")
fp.close()
try:
    zf = zipfile.ZipFile(srcname)
except zipfile.BadZipfile:
    os.unlink(srcname)


# make sure we don't raise an AttributeError when a partially-constructed
# ZipFile instance is finalized; this tests for regression on SF tracker
# bug #403871.
try:
    zipfile.ZipFile(srcname)
except IOError:
    # The bug we're testing for caused an AttributeError to be raised
    # when a ZipFile instance was created for a file that did not
    # exist; the .fp member was not initialized but was needed by the
    # __del__() method.  Since the AttributeError is in the __del__(),
    # it is ignored, but the user should be sufficiently annoyed by
    # the message on the output that regression will be noticed
    # quickly.
    pass
else:
    raise TestFailed("expected creation of readable ZipFile without\n"
                     "  a file to raise an IOError.")
