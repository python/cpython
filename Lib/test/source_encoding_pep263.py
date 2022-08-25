# -*- coding: koi8-r -*-

assert "Питон".encode("utf-8") == b'\xd0\x9f\xd0\xb8\xd1\x82\xd0\xbe\xd0\xbd'
assert "\П".encode("utf-8") == b'\\\xd0\x9f'
