from .. import util

machinery = util.import_importlib('importlib.machinery')

import codecs
import importlib.util
import re
import types
# Because sys.path gets essentially blanked, need to have unicodedata already
# imported for the parser to use.
import unicodedata
import unittest
import warnings


CODING_RE = re.compile(r'^[ \t\f]*#.*?coding[:=][ \t]*([-\w.]+)', re.ASCII)


class EncodingTest:

    """PEP 3120 makes UTF-8 the default encoding for source code
    [default encoding].

    PEP 263 specifies how that can change on a per-file basis. Either the first
    or second line can contain the encoding line [encoding first line]
    encoding second line]. If the file has the BOM marker it is considered UTF-8
    implicitly [BOM]. If any encoding is specified it must be UTF-8, else it is
    an error [BOM and utf-8][BOM conflict].

    """

    variable = '\u00fc'
    character = '\u00c9'
    source_line = "{0} = '{1}'\n".format(variable, character)
    module_name = '_temp'

    def run_test(self, source):
        with util.create_modules(self.module_name) as mapping:
            with open(mapping[self.module_name], 'wb') as file:
                file.write(source)
            loader = self.machinery.SourceFileLoader(self.module_name,
                                                  mapping[self.module_name])
            return self.load(loader)

    def create_source(self, encoding):
        encoding_line = "# coding={0}".format(encoding)
        assert CODING_RE.match(encoding_line)
        source_lines = [encoding_line.encode('utf-8')]
        source_lines.append(self.source_line.encode(encoding))
        return b'\n'.join(source_lines)

    def test_non_obvious_encoding(self):
        # Make sure that an encoding that has never been a standard one for
        # Python works.
        encoding_line = "# coding=koi8-r"
        assert CODING_RE.match(encoding_line)
        source = "{0}\na=42\n".format(encoding_line).encode("koi8-r")
        self.run_test(source)

    # [default encoding]
    def test_default_encoding(self):
        self.run_test(self.source_line.encode('utf-8'))

    # [encoding first line]
    def test_encoding_on_first_line(self):
        encoding = 'Latin-1'
        source = self.create_source(encoding)
        self.run_test(source)

    # [encoding second line]
    def test_encoding_on_second_line(self):
        source = b"#/usr/bin/python\n" + self.create_source('Latin-1')
        self.run_test(source)

    # [BOM]
    def test_bom(self):
        self.run_test(codecs.BOM_UTF8 + self.source_line.encode('utf-8'))

    # [BOM and utf-8]
    def test_bom_and_utf_8(self):
        source = codecs.BOM_UTF8 + self.create_source('utf-8')
        self.run_test(source)

    # [BOM conflict]
    def test_bom_conflict(self):
        source = codecs.BOM_UTF8 + self.create_source('latin-1')
        with self.assertRaises(SyntaxError):
            self.run_test(source)


class EncodingTestPEP451(EncodingTest):

    def load(self, loader):
        module = types.ModuleType(self.module_name)
        module.__spec__ = importlib.util.spec_from_loader(self.module_name, loader)
        loader.exec_module(module)
        return module


(Frozen_EncodingTestPEP451,
 Source_EncodingTestPEP451
 ) = util.test_both(EncodingTestPEP451, machinery=machinery)


class EncodingTestPEP302(EncodingTest):

    def load(self, loader):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            return loader.load_module(self.module_name)


(Frozen_EncodingTestPEP302,
 Source_EncodingTestPEP302
 ) = util.test_both(EncodingTestPEP302, machinery=machinery)


class LineEndingTest:

    r"""Source written with the three types of line endings (\n, \r\n, \r)
    need to be readable [cr][crlf][lf]."""

    def run_test(self, line_ending):
        module_name = '_temp'
        source_lines = [b"a = 42", b"b = -13", b'']
        source = line_ending.join(source_lines)
        with util.create_modules(module_name) as mapping:
            with open(mapping[module_name], 'wb') as file:
                file.write(source)
            loader = self.machinery.SourceFileLoader(module_name,
                                                     mapping[module_name])
            return self.load(loader, module_name)

    # [cr]
    def test_cr(self):
        self.run_test(b'\r')

    # [crlf]
    def test_crlf(self):
        self.run_test(b'\r\n')

    # [lf]
    def test_lf(self):
        self.run_test(b'\n')


class LineEndingTestPEP451(LineEndingTest):

    def load(self, loader, module_name):
        module = types.ModuleType(module_name)
        module.__spec__ = importlib.util.spec_from_loader(module_name, loader)
        loader.exec_module(module)
        return module


(Frozen_LineEndingTestPEP451,
 Source_LineEndingTestPEP451
 ) = util.test_both(LineEndingTestPEP451, machinery=machinery)


class LineEndingTestPEP302(LineEndingTest):

    def load(self, loader, module_name):
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            return loader.load_module(module_name)


(Frozen_LineEndingTestPEP302,
 Source_LineEndingTestPEP302
 ) = util.test_both(LineEndingTestPEP302, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
