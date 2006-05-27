# To fully test this module, we would need a copy of the stringprep tables.
# Since we don't have them, this test checks only a few codepoints.

from test.test_support import verify, vereq

import stringprep
from stringprep import *

verify(in_table_a1(u"\u0221"))
verify(not in_table_a1(u"\u0222"))

verify(in_table_b1(u"\u00ad"))
verify(not in_table_b1(u"\u00ae"))

verify(map_table_b2(u"\u0041"), u"\u0061")
verify(map_table_b2(u"\u0061"), u"\u0061")

verify(map_table_b3(u"\u0041"), u"\u0061")
verify(map_table_b3(u"\u0061"), u"\u0061")

verify(in_table_c11(u"\u0020"))
verify(not in_table_c11(u"\u0021"))

verify(in_table_c12(u"\u00a0"))
verify(not in_table_c12(u"\u00a1"))

verify(in_table_c12(u"\u00a0"))
verify(not in_table_c12(u"\u00a1"))

verify(in_table_c11_c12(u"\u00a0"))
verify(not in_table_c11_c12(u"\u00a1"))

verify(in_table_c21(u"\u001f"))
verify(not in_table_c21(u"\u0020"))

verify(in_table_c22(u"\u009f"))
verify(not in_table_c22(u"\u00a0"))

verify(in_table_c21_c22(u"\u009f"))
verify(not in_table_c21_c22(u"\u00a0"))

verify(in_table_c3(u"\ue000"))
verify(not in_table_c3(u"\uf900"))

verify(in_table_c4(u"\uffff"))
verify(not in_table_c4(u"\u0000"))

verify(in_table_c5(u"\ud800"))
verify(not in_table_c5(u"\ud7ff"))

verify(in_table_c6(u"\ufff9"))
verify(not in_table_c6(u"\ufffe"))

verify(in_table_c7(u"\u2ff0"))
verify(not in_table_c7(u"\u2ffc"))

verify(in_table_c8(u"\u0340"))
verify(not in_table_c8(u"\u0342"))

# C.9 is not in the bmp
# verify(in_table_c9(u"\U000E0001"))
# verify(not in_table_c8(u"\U000E0002"))

verify(in_table_d1(u"\u05be"))
verify(not in_table_d1(u"\u05bf"))

verify(in_table_d2(u"\u0041"))
verify(not in_table_d2(u"\u0040"))

# This would generate a hash of all predicates. However, running
# it is quite expensive, and only serves to detect changes in the
# unicode database. Instead, stringprep.py asserts the version of
# the database.

# import hashlib
# predicates = [k for k in dir(stringprep) if k.startswith("in_table")]
# predicates.sort()
# for p in predicates:
#     f = getattr(stringprep, p)
#     # Collect all BMP code points
#     data = ["0"] * 0x10000
#     for i in range(0x10000):
#         if f(unichr(i)):
#             data[i] = "1"
#     data = "".join(data)
#     h = hashlib.sha1()
#     h.update(data)
#     print p, h.hexdigest()
