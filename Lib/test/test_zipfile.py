import zipfile, os
from test_support import TestFailed

srcname = "junk9630.tmp"
zipname = "junk9708.tmp"

try:
    fp = open(srcname, "w")               # Make a source file with some lines
    for i in range(0, 1000):
        fp.write("Test of zipfile line %d.\n" % i)
    fp.close()

    zip = zipfile.ZipFile(zipname, "w")   # Create the ZIP archive
    zip.write(srcname, srcname)
    zip.write(srcname, "another.name")
    zip.close()

    zip = zipfile.ZipFile(zipname, "r")   # Read the ZIP archive
    zip.read("another.name")
    zip.read(srcname)
    zip.close()
finally:
    if os.path.isfile(srcname):           # Remove temporary files
        os.unlink(srcname)
    if os.path.isfile(zipname):
        os.unlink(zipname)

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
