import zipfile, os

srcname = "junk9630.tmp"
zipname = "junk9708.tmp"

try:
  fp = open(srcname, "w")		# Make a source file with some lines
  for i in range(0, 1000):
    fp.write("Test of zipfile line %d.\n" % i)
  fp.close()

  zip = zipfile.ZipFile(zipname, "w")	# Create the ZIP archive
  zip.write(srcname, srcname)
  zip.write(srcname, "another.name")
  zip.close()

  zip = zipfile.ZipFile(zipname, "r")	# Read the ZIP archive
  zip.read("another.name")
  zip.read(srcname)
  zip.close()
finally:
  if os.path.isfile(srcname):		# Remove temporary files
    os.unlink(srcname)
  if os.path.isfile(zipname):
    os.unlink(zipname)

