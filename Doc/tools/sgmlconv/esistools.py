"""Miscellaneous utility functions useful for dealing with ESIS streams."""
__version__ = '$Revision$'

import re
import string
import sys


_data_rx = re.compile(r"[^\\][^\\]*")

def decode(s):
    r = ''
    while s:
        m = _data_rx.match(s)
        if m:
            r = r + m.group()
            s = s[len(m.group()):]
        elif s[1] == "\\":
            r = r + "\\"
            s = s[2:]
        elif s[1] == "n":
            r = r + "\n"
            s = s[2:]
        else:
            raise ValueError, "can't handle " + `s`
    return r


_charmap = {}
for c in map(chr, range(256)):
    _charmap[c] = c
_charmap["\n"] = r"\n"
_charmap["\\"] = r"\\"
del c

def encode(s):
    return string.join(map(_charmap.get, s), '')


import xml.dom.esis_builder


class ExtendedEsisBuilder(xml.dom.esis_builder.EsisBuilder):
    def __init__(self, *args, **kw):
        self.__empties = {}
        self.__is_empty = 0
        apply(xml.dom.esis_builder.EsisBuilder.__init__, (self,) + args, kw)

    def feed(self, data):
        for line in string.split(data, '\n'):
            if not line: 
                break
            event = line[0]
            text = line[1:]
            if event == '(':
                element = self.document.createElement(text, self.attr_store)
                self.attr_store = {}
                self.push(element)
                if self.__is_empty:
                    self.__empties[text] = text
                    self.__is_empty = 0
            elif event == ')':
                self.pop()
            elif event == 'A':
                l = re.split(' ', text, 2)
                name = l[0]
                value = decode(l[2])
                self.attr_store[name] = value
            elif event == '-':
                text = self.document.createText(decode(text))
                self.push(text)
            elif event == 'C':
                return
            elif event == 'e':
                self.__is_empty = 1
            else:
                sys.stderr.write('Unknown event: %s\n' % line)

    def get_empties(self):
        return self.__empties.keys()
