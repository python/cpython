#! /usr/bin/env python

# This program reads all *.py and test/*.py in "libDir", and
# copies those files with illegal DOS names to libDir/dos-8x3.
# Names are illegal if they are longer than 8x3 chars or if they
# contain uppercase chars.  It also tests for name collisions.
# You must first create the directory libDir/dos-8x3 yourself.
# You should remove all files in dos-8x3 if you run it again.

# CHANGE libDir TO THE CORRECT DIRECTORY.  RM dos-8x3/* FIRST.

import sys, os, regex, string

libDir = "./Lib"	# Location of Python Lib

def make8x3():
  reg_uppercase = regex.compile("[A-Z]")
  collisions = {}	# See if all names are unique in first 8 chars.
  destDir = os.path.join(libDir, "dos-8x3")
  if not os.path.isdir(destDir):
    print "Please create the directory", destDir, "first."
    err()
  while 1:
    ans = raw_input("Ok to copy to " + destDir + " [yn]? ")
    if not ans:
      continue
    elif ans[0] == "n":
      err()
    elif ans[0] == "y":
      break
  for dirname in libDir, os.path.join(libDir, "test"):
    for filename in os.listdir(dirname):
      if filename[-3:] == ".py":
        name = filename[0:-3]
        if len(name) > 8 or reg_uppercase.search(name) >= 0:
          shortName = string.lower(name[0:8])
          if collisions.has_key(shortName):
            print "Name not unique in first 8 chars:", collisions[shortName], name
          else:
            collisions[shortName] = name
            fin = open(os.path.join(dirname, filename), "r")
            dest = os.path.join(destDir, shortName + ".py")
            fout = open(dest, "w")
            fout.write(fin.read())
            fin.close()
            fout.close()
            os.chmod(dest, 0644)
      elif filename == "." or filename == "..":
        continue
      elif filename[-4:] == ".pyc":
        continue
      elif filename == "Makefile":
        continue
      else:
        parts = string.splitfields(filename, ".")
        if len(parts) > 2 or \
           len(parts[0]) > 8 or \
           reg_uppercase.search(filename) >= 0 or \
           (len(parts) > 1 and len(parts[1]) > 3):
                 print "Illegal DOS name", os.path.join(dirname, filename)
  sys.exit(0)
def err():
  print "No files copied."
  sys.exit(1)
            

if __name__ == "__main__":
  make8x3()
