#! /usr/bin/env python
import sys
import string
import perfect_hash

# This is a user of perfect_hash.py
# that takes as input the UnicodeData.txt file available from:
# ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt

# It generates a hash table from Unicode Character Name ->
# unicode code space value.

# These variables determine which hash function is tried first.
# Yields a multiple of 1.7875 for UnicodeData.txt on 2000/06/24/
f1Seed = 1694245428
f2Seed = -1917331657

# Maximum allowed multipler, if this isn't None then instead of continually
# increasing C, it resets it back to initC to keep searching for
# a solution.
minC = 1.7875
# Initial multiplier for trying to find a perfect hash function.
initC = 1.7875

moduleName = "ucnhash"
dataArrayName = "aucn"
dataArrayType = "_Py_UnicodeCharacterName"
headerFileName = "ucnhash.h"
cFileName      = "ucnhash.c"
structName     = "_Py_UCNHashAPI"

keys = []
hashData = {}

def generateOutputFiles(perfHash, hashData):
  header = perfHash.generate_header(structName)
  header = header + """
typedef struct 
{
    const char *pszUCN;
    Py_UCS4 value;
} _Py_UnicodeCharacterName;

"""
  
  code = perfHash.generate_code(moduleName,
                                dataArrayName,
                                dataArrayType,
                                structName)
  out = open(headerFileName, "w")
  out.write(header)
  out = open(cFileName, "w")
  out.write("#include \"%s\"\n" % headerFileName)
  out.write(code)
  perfHash.generate_graph(out)
  out.write("""
  
static const _Py_UnicodeCharacterName aucn[] = 
{
""")
  for i in xrange(len(keys)):
    v = hashData[keys[i][0]]
    out.write('  { "' + keys[i][0] + '", ' + hex(v) + " }," + "\n")
  out.write("};\n\n")
  sys.stderr.write('\nGenerated output files: \n')
  sys.stderr.write('%s\n%s\n' % (headerFileName, cFileName))

def main():
  # Suck in UnicodeData.txt and spit out the generated files.
  input = open(sys.argv[1], 'r')
  i = 0
  while 1:
    line = input.readline()
    if line == "": break
    fields = string.split(line, ';')
    if len(fields) < 2:
      sys.stderr.write('Ill-formated line!\n')
      sys.stderr.write('line #: %d\n' % (i + 1))
      sys.exit()
    data, key = fields[:2]
    key = string.strip( key )
    # Any name starting with '<' is a control, or start/end character,
    # so skip it...
    if key[0] == "<":
      continue
    hashcode = i
    i = i + 1
    # force the name to uppercase
    keys.append( (string.upper(key),hashcode) )
    data = string.atoi(data, 16)
    hashData[key] = data

  input.close()
  sys.stderr.write('%i key/hash pairs read\n' % len(keys) )
  perfHash = perfect_hash.generate_hash(keys, 1,
                                        minC, initC,
                                        f1Seed, f2Seed,
                                        # increment, tries
                                        0.0025, 50)
  generateOutputFiles(perfHash, hashData)

if __name__ == '__main__':
  if len(sys.argv) == 1:
    sys.stdout = sys.stderr
    print 'Usage: %s <input filename>' % sys.argv[0]
    print '  The input file needs to be UnicodeData.txt'
    sys.exit()
  main()
  
