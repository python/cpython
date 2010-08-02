#!/usr/bin/env python3

# Takes an optional filename, defaulting to this file itself.
# Reads the file and compresses the content using level 1 and level 9
# compression, printing a summary of the results.

import zlib, sys

def main():
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    else:
        filename = sys.argv[0]
    print('Reading', filename)

    with open(filename, 'rb') as f:           # Get the data to compress
        s = f.read()

    # First, we'll compress the string in one step
    comptext = zlib.compress(s, 1)
    decomp = zlib.decompress(comptext)

    print('1-step compression: (level 1)')
    print('    Original:', len(s), 'Compressed:', len(comptext), end=' ')
    print('Uncompressed:', len(decomp))

    # Now, let's compress the string in stages; set chunk to work in smaller steps

    chunk = 256
    compressor = zlib.compressobj(9)
    decompressor = zlib.decompressobj()
    comptext = decomp = b''
    for i in range(0, len(s), chunk):
        comptext += compressor.compress(s[i:i+chunk])
    # Don't forget to call flush()!!
    comptext += compressor.flush()

    for i in range(0, len(comptext), chunk):
        decomp += decompressor.decompress(comptext[i:i+chunk])
    decomp += decompressor.flush()

    print('Progressive compression (level 9):')
    print('    Original:', len(s), 'Compressed:', len(comptext), end=' ')
    print('Uncompressed:', len(decomp))

if __name__ == '__main__':
    main()
