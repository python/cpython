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

import sys

sys.path[0:0] = [""]

from test import unittest

from pymongo.saslprep import saslprep


class TestSASLprep(unittest.TestCase):
    def test_saslprep(self):
        try:
            import stringprep  # noqa
        except ImportError:
            self.assertRaises(TypeError, saslprep, "anything...")
            # Bytes strings are ignored.
            self.assertEqual(saslprep(b"user"), b"user")
        else:
            # Examples from RFC4013, Section 3.
            self.assertEqual(saslprep("I\u00ADX"), "IX")
            self.assertEqual(saslprep("user"), "user")
            self.assertEqual(saslprep("USER"), "USER")
            self.assertEqual(saslprep("\u00AA"), "a")
            self.assertEqual(saslprep("\u2168"), "IX")
            self.assertRaises(ValueError, saslprep, "\u0007")
            self.assertRaises(ValueError, saslprep, "\u0627\u0031")

            # Bytes strings are ignored.
            self.assertEqual(saslprep(b"user"), b"user")
