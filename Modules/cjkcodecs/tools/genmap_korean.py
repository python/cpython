#
# genmap_korean.py: Korean Codecs Map Generator
#
# Original Author:  Hye-Shik Chang <perky@FreeBSD.org>
# Modified Author:  Dong-hee Na <donghee.na92@gmail.com>
#
import os

from genmap_support import *

KSX1001_C1  = (0x21, 0x7e)
KSX1001_C2  = (0x21, 0x7e)
UHCL1_C1    = (0x81, 0xa0)
UHCL1_C2    = (0x41, 0xfe)
UHCL2_C1    = (0xa1, 0xfe)
UHCL2_C2    = (0x41, 0xa0)

mapfile = open_mapping_file('data/CP949.TXT', 'http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP949.TXT')

print("Loading Mapping File...")
decmap = loadmap(mapfile)
uhcdecmap, ksx1001decmap = {}, {}
cp949encmap = {}
for c1, c2map in decmap.items():
    for c2, code in c2map.items():
        if c1 >= 0xa1 and c2 >= 0xa1:
            ksx1001decmap.setdefault(c1&0x7f, {})
            ksx1001decmap[c1&0x7f][c2&0x7f] = c2map[c2]
            cp949encmap.setdefault(code >> 8, {})
            cp949encmap[code >> 8][code & 0xFF] = (c1<<8 | c2) & 0x7f7f
        else: # uhc
            uhcdecmap.setdefault(c1, {})
            uhcdecmap[c1][c2] = c2map[c2]
            cp949encmap.setdefault(code >> 8, {}) # MSB set
            cp949encmap[code >> 8][code & 0xFF] = (c1<<8 | c2)

with open('mappings_kr.h', 'w') as omap:
    print_autogen(omap, os.path.basename(__file__))

    print("Generating KS X 1001 decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "ksx1001", KSX1001_C1, KSX1001_C2, ksx1001decmap)
    print_decmap(omap, filler, "ksx1001", ksx1001decmap)

    print("Generating UHC decode map...")
    filler = BufferedFiller()
    genmap_decode(filler, "cp949ext", UHCL1_C1, UHCL1_C2, uhcdecmap)
    genmap_decode(filler, "cp949ext", UHCL2_C1, UHCL2_C2, uhcdecmap)
    print_decmap(omap, filler, "cp949ext", uhcdecmap)

    print("Generating CP949 (includes KS X 1001) encode map...")
    filler = BufferedFiller()
    genmap_encode(filler, "cp949", cp949encmap)
    print_encmap(omap, filler, "cp949", cp949encmap)

print("Done!")
