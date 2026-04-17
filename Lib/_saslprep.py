# Copyright 2016-present MongoDB, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Implementation of RFC 4013 SASLprep."""

import stringprep
import unicodedata
from collections.abc import Callable

# RFC 4013 section 2.3 prohibited output.
# A strict reading of RFC 4013 requires table C.1.2 here, but characters
# from it are mapped to SPACE in the Map step, so they won't appear here.
_PROHIBITED: tuple[Callable[[str], bool], ...] = (
    stringprep.in_table_c12,
    stringprep.in_table_c21_c22,
    stringprep.in_table_c3,
    stringprep.in_table_c4,
    stringprep.in_table_c5,
    stringprep.in_table_c6,
    stringprep.in_table_c7,
    stringprep.in_table_c8,
    stringprep.in_table_c9,
)


def saslprep(data: bytes | str, *, allow_unassigned_code_points: bool) -> bytes | str:
    """Prepare a string per RFC 4013 SASLprep.

    :Parameters:
      - `data`: The string to SASLprep. Unicode strings (:class:`str`) are
        normalized; byte strings (:class:`bytes`) are returned unchanged.
      - `allow_unassigned_code_points`: Per RFC 3454 and RFC 4013, pass
        ``False`` for stored strings (unassigned code points are prohibited)
        and ``True`` for queries (unassigned code points are permitted).
        Always pass this explicitly; there is no default.

    :Returns:
      The SASLprep'd version of `data`.

    :Raises ValueError:
      If `data` contains a prohibited character or fails the bidirectional
      string check.
    """
    if not isinstance(data, str):
        return data

    prohibited: tuple[Callable[[str], bool], ...]
    if allow_unassigned_code_points:
        prohibited = _PROHIBITED
    else:
        prohibited = _PROHIBITED + (stringprep.in_table_a1,)

    # RFC 3454 section 2, step 1 - Map
    # RFC 4013 section 2.1: map non-ASCII space characters to SPACE (U+0020),
    # and map commonly-mapped-to-nothing characters to nothing.
    in_table_c12 = stringprep.in_table_c12
    in_table_b1 = stringprep.in_table_b1
    data = "".join(
        "\u0020" if in_table_c12(c) else c
        for c in data
        if not in_table_b1(c)
    )

    # RFC 3454 section 2, step 2 - Normalize
    # RFC 4013 section 2.2: use Unicode NFKC normalization.
    data = unicodedata.ucd_3_2_0.normalize("NFKC", data)

    if not data:
        return data

    in_table_d1 = stringprep.in_table_d1
    if in_table_d1(data[0]):
        if not in_table_d1(data[-1]):
            # RFC 3454 section 6, rule 3: if a string contains any RandALCat
            # character, the first and last characters MUST be RandALCat.
            raise ValueError("SASLprep: failed bidirectional check")
        # RFC 3454 section 6, rule 2: if a string contains any RandALCat
        # character, it MUST NOT contain any LCat character.
        prohibited = prohibited + (stringprep.in_table_d2,)
    else:
        # Rule 3 (converse): if the first character is not RandALCat,
        # no other character may be RandALCat.
        prohibited = prohibited + (in_table_d1,)

    # RFC 3454 section 2, steps 3 and 4 - Prohibit and check bidi.
    for char in data:
        if any(in_table(char) for in_table in prohibited):
            raise ValueError("SASLprep: failed prohibited character check")

    return data
