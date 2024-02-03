#
# genmap_korean.py: Korean Codecs Map Generator
#
# Original Author:  Hye-Shik Chang <perky@FreeBSD.org>
# Modified Author:  Donghee Na <donghee.na92@gmail.com>
#
import os

from genmap_support import *


KSX1001_C1 = (0x21, 0x7e)
KSX1001_C2 = (0x21, 0x7e)
UHCL1_C1 = (0x81, 0xa0)
UHCL1_C2 = (0x41, 0xfe)
UHCL2_C1 = (0xa1, 0xfe)
UHCL2_C2 = (0x41, 0xa0)
MAPPINGS_CP949 = 'http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP949.TXT'


def main():
    mapfile = open_mapping_file('python-mappings/CP949.TXT', MAPPINGS_CP949)
    print("Loading Mapping File...")
    decmap = loadmap(mapfile)
    uhcdecmap, ksx1001decmap, cp949encmap = {}, {}, {}
    for c1, c2map in decmap.items():
        for c2, code in c2map.items():
            if c1 >= 0xa1 and c2 >= 0xa1:
                ksx1001decmap.setdefault(c1 & 0x7f, {})
                ksx1001decmap[c1 & 0x7f][c2 & 0x7f] = c2map[c2]
                cp949encmap.setdefault(code >> 8, {})
                cp949encmap[code >> 8][code & 0xFF] = (c1 << 8 | c2) & 0x7f7f
            else:
                # uhc
                uhcdecmap.setdefault(c1, {})
                uhcdecmap[c1][c2] = c2map[c2]
                cp949encmap.setdefault(code >> 8, {})  # MSB set
                cp949encmap[code >> 8][code & 0xFF] = (c1 << 8 | c2)

    with open('mappings_kr.h', 'w') as fp:
        print_autogen(fp, os.path.basename(__file__))

        print("Generating KS X 1001 decode map...")
        writer = DecodeMapWriter(fp, "ksx1001", ksx1001decmap)
        writer.update_decode_map(KSX1001_C1, KSX1001_C2)
        writer.generate()

        print("Generating UHC decode map...")
        writer = DecodeMapWriter(fp, "cp949ext", uhcdecmap)
        writer.update_decode_map(UHCL1_C1, UHCL1_C2)
        writer.update_decode_map(UHCL2_C1, UHCL2_C2)
        writer.generate()

        print("Generating CP949 (includes KS X 1001) encode map...")
        writer = EncodeMapWriter(fp, "cp949", cp949encmap)
        writer.generate()

    print("Done!")


if __name__ == '__main__':
    main()
