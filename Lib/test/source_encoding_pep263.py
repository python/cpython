# -*- coding: koi8-r -*-

def test_pep263(self):
    self.assertEqual(
        "Питон".encode("utf-8"),
        b'\xd0\x9f\xd0\xb8\xd1\x82\xd0\xbe\xd0\xbd'
    )
    self.assertEqual(
        "\П".encode("utf-8"),
        b'\\\xd0\x9f'
    )
