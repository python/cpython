#!/usr/bin/python
from test import test_support
from test.test_support import TESTFN

import unittest
from io import BytesIO
import os
import subprocess
import sys

import bz2
from bz2 import BZ2File, BZ2Compressor, BZ2Decompressor

has_cmdline_bunzip2 = sys.platform not in ("win32", "os2emx", "riscos")

class BaseTest(unittest.TestCase):
    "Base for other testcases."
    TEXT = b'root:x:0:0:root:/root:/bin/bash\nbin:x:1:1:bin:/bin:\ndaemon:x:2:2:daemon:/sbin:\nadm:x:3:4:adm:/var/adm:\nlp:x:4:7:lp:/var/spool/lpd:\nsync:x:5:0:sync:/sbin:/bin/sync\nshutdown:x:6:0:shutdown:/sbin:/sbin/shutdown\nhalt:x:7:0:halt:/sbin:/sbin/halt\nmail:x:8:12:mail:/var/spool/mail:\nnews:x:9:13:news:/var/spool/news:\nuucp:x:10:14:uucp:/var/spool/uucp:\noperator:x:11:0:operator:/root:\ngames:x:12:100:games:/usr/games:\ngopher:x:13:30:gopher:/usr/lib/gopher-data:\nftp:x:14:50:FTP User:/var/ftp:/bin/bash\nnobody:x:65534:65534:Nobody:/home:\npostfix:x:100:101:postfix:/var/spool/postfix:\nniemeyer:x:500:500::/home/niemeyer:/bin/bash\npostgres:x:101:102:PostgreSQL Server:/var/lib/pgsql:/bin/bash\nmysql:x:102:103:MySQL server:/var/lib/mysql:/bin/bash\nwww:x:103:104::/var/www:/bin/false\n'
    DATA = b'BZh91AY&SY.\xc8N\x18\x00\x01>_\x80\x00\x10@\x02\xff\xf0\x01\x07n\x00?\xe7\xff\xe00\x01\x99\xaa\x00\xc0\x03F\x86\x8c#&\x83F\x9a\x03\x06\xa6\xd0\xa6\x93M\x0fQ\xa7\xa8\x06\x804hh\x12$\x11\xa4i4\xf14S\xd2<Q\xb5\x0fH\xd3\xd4\xdd\xd5\x87\xbb\xf8\x94\r\x8f\xafI\x12\xe1\xc9\xf8/E\x00pu\x89\x12]\xc9\xbbDL\nQ\x0e\t1\x12\xdf\xa0\xc0\x97\xac2O9\x89\x13\x94\x0e\x1c7\x0ed\x95I\x0c\xaaJ\xa4\x18L\x10\x05#\x9c\xaf\xba\xbc/\x97\x8a#C\xc8\xe1\x8cW\xf9\xe2\xd0\xd6M\xa7\x8bXa<e\x84t\xcbL\xb3\xa7\xd9\xcd\xd1\xcb\x84.\xaf\xb3\xab\xab\xad`n}\xa0lh\tE,\x8eZ\x15\x17VH>\x88\xe5\xcd9gd6\x0b\n\xe9\x9b\xd5\x8a\x99\xf7\x08.K\x8ev\xfb\xf7xw\xbb\xdf\xa1\x92\xf1\xdd|/";\xa2\xba\x9f\xd5\xb1#A\xb6\xf6\xb3o\xc9\xc5y\\\xebO\xe7\x85\x9a\xbc\xb6f8\x952\xd5\xd7"%\x89>V,\xf7\xa6z\xe2\x9f\xa3\xdf\x11\x11"\xd6E)I\xa9\x13^\xca\xf3r\xd0\x03U\x922\xf26\xec\xb6\xed\x8b\xc3U\x13\x9d\xc5\x170\xa4\xfa^\x92\xacDF\x8a\x97\xd6\x19\xfe\xdd\xb8\xbd\x1a\x9a\x19\xa3\x80ankR\x8b\xe5\xd83]\xa9\xc6\x08\x82f\xf6\xb9"6l$\xb8j@\xc0\x8a\xb0l1..\xbak\x83ls\x15\xbc\xf4\xc1\x13\xbe\xf8E\xb8\x9d\r\xa8\x9dk\x84\xd3n\xfa\xacQ\x07\xb1%y\xaav\xb4\x08\xe0z\x1b\x16\xf5\x04\xe9\xcc\xb9\x08z\x1en7.G\xfc]\xc9\x14\xe1B@\xbb!8`'
    DATA_CRLF = b'BZh91AY&SY\xaez\xbbN\x00\x01H\xdf\x80\x00\x12@\x02\xff\xf0\x01\x07n\x00?\xe7\xff\xe0@\x01\xbc\xc6`\x86*\x8d=M\xa9\x9a\x86\xd0L@\x0fI\xa6!\xa1\x13\xc8\x88jdi\x8d@\x03@\x1a\x1a\x0c\x0c\x83 \x00\xc4h2\x19\x01\x82D\x84e\t\xe8\x99\x89\x19\x1ah\x00\r\x1a\x11\xaf\x9b\x0fG\xf5(\x1b\x1f?\t\x12\xcf\xb5\xfc\x95E\x00ps\x89\x12^\xa4\xdd\xa2&\x05(\x87\x04\x98\x89u\xe40%\xb6\x19\'\x8c\xc4\x89\xca\x07\x0e\x1b!\x91UIFU%C\x994!DI\xd2\xfa\xf0\xf1N8W\xde\x13A\xf5\x9cr%?\x9f3;I45A\xd1\x8bT\xb1<l\xba\xcb_\xc00xY\x17r\x17\x88\x08\x08@\xa0\ry@\x10\x04$)`\xf2\xce\x89z\xb0s\xec\x9b.iW\x9d\x81\xb5-+t\x9f\x1a\'\x97dB\xf5x\xb5\xbe.[.\xd7\x0e\x81\xe7\x08\x1cN`\x88\x10\xca\x87\xc3!"\x80\x92R\xa1/\xd1\xc0\xe6mf\xac\xbd\x99\xcca\xb3\x8780>\xa4\xc7\x8d\x1a\\"\xad\xa1\xabyBg\x15\xb9l\x88\x88\x91k"\x94\xa4\xd4\x89\xae*\xa6\x0b\x10\x0c\xd6\xd4m\xe86\xec\xb5j\x8a\x86j\';\xca.\x01I\xf2\xaaJ\xe8\x88\x8cU+t3\xfb\x0c\n\xa33\x13r2\r\x16\xe0\xb3(\xbf\x1d\x83r\xe7M\xf0D\x1365\xd8\x88\xd3\xa4\x92\xcb2\x06\x04\\\xc1\xb0\xea//\xbek&\xd8\xe6+t\xe5\xa1\x13\xada\x16\xder5"w]\xa2i\xb7[\x97R \xe2IT\xcd;Z\x04dk4\xad\x8a\t\xd3\x81z\x10\xf1:^`\xab\x1f\xc5\xdc\x91N\x14$+\x9e\xae\xd3\x80'

    if has_cmdline_bunzip2:
        def decompress(self, data):
            pop = subprocess.Popen("bunzip2", shell=True,
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT)
            pop.stdin.write(data)
            pop.stdin.close()
            ret = pop.stdout.read()
            pop.stdout.close()
            if pop.wait() != 0:
                ret = bz2.decompress(data)
            return ret

    else:
        # bunzip2 isn't available to run on Windows.
        def decompress(self, data):
            return bz2.decompress(data)

class BZ2FileTest(BaseTest):
    "Test BZ2File type miscellaneous methods."

    def setUp(self):
        self.filename = TESTFN

    def tearDown(self):
        if os.path.isfile(self.filename):
            os.unlink(self.filename)

    def createTempFile(self, crlf=0):
        f = open(self.filename, "wb")
        if crlf:
            data = self.DATA_CRLF
        else:
            data = self.DATA
        f.write(data)
        f.close()

    def testRead(self):
        # "Test BZ2File.read()"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        self.assertRaises(TypeError, bz2f.read, None)
        self.assertEqual(bz2f.read(), self.TEXT)
        bz2f.close()

    def testRead0(self):
        # Test BBZ2File.read(0)"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        self.assertRaises(TypeError, bz2f.read, None)
        self.assertEqual(bz2f.read(0), b"")
        bz2f.close()

    def testReadChunk10(self):
        # "Test BZ2File.read() in chunks of 10 bytes"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        text = b''
        while 1:
            str = bz2f.read(10)
            if not str:
                break
            text += str
        self.assertEqual(text, text)
        bz2f.close()

    def testRead100(self):
        # "Test BZ2File.read(100)"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        self.assertEqual(bz2f.read(100), self.TEXT[:100])
        bz2f.close()

    def testReadLine(self):
        # "Test BZ2File.readline()"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        self.assertRaises(TypeError, bz2f.readline, None)
        sio = BytesIO(self.TEXT)
        for line in sio.readlines():
            self.assertEqual(bz2f.readline(), line)
        bz2f.close()

    def testReadLines(self):
        # "Test BZ2File.readlines()"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        self.assertRaises(TypeError, bz2f.readlines, None)
        sio = BytesIO(self.TEXT)
        self.assertEqual(bz2f.readlines(), sio.readlines())
        bz2f.close()

    def testIterator(self):
        # "Test iter(BZ2File)"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        sio = BytesIO(self.TEXT)
        self.assertEqual(list(iter(bz2f)), sio.readlines())
        bz2f.close()

    def testWrite(self):
        # "Test BZ2File.write()"
        bz2f = BZ2File(self.filename, "w")
        self.assertRaises(TypeError, bz2f.write)
        bz2f.write(self.TEXT)
        bz2f.close()
        f = open(self.filename, 'rb')
        self.assertEqual(self.decompress(f.read()), self.TEXT)
        f.close()

    def testWriteChunks10(self):
        # "Test BZ2File.write() with chunks of 10 bytes"
        bz2f = BZ2File(self.filename, "w")
        n = 0
        while 1:
            str = self.TEXT[n*10:(n+1)*10]
            if not str:
                break
            bz2f.write(str)
            n += 1
        bz2f.close()
        f = open(self.filename, 'rb')
        self.assertEqual(self.decompress(f.read()), self.TEXT)
        f.close()

    def testWriteLines(self):
        # "Test BZ2File.writelines()"
        bz2f = BZ2File(self.filename, "w")
        self.assertRaises(TypeError, bz2f.writelines)
        sio = BytesIO(self.TEXT)
        bz2f.writelines(sio.readlines())
        bz2f.close()
        # patch #1535500
        self.assertRaises(ValueError, bz2f.writelines, ["a"])
        f = open(self.filename, 'rb')
        self.assertEqual(self.decompress(f.read()), self.TEXT)
        f.close()

    def testWriteMethodsOnReadOnlyFile(self):
        bz2f = BZ2File(self.filename, "w")
        bz2f.write("abc")
        bz2f.close()

        bz2f = BZ2File(self.filename, "r")
        self.assertRaises(IOError, bz2f.write, "a")
        self.assertRaises(IOError, bz2f.writelines, ["a"])

    def testSeekForward(self):
        # "Test BZ2File.seek(150, 0)"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        self.assertRaises(TypeError, bz2f.seek)
        bz2f.seek(150)
        self.assertEqual(bz2f.read(), self.TEXT[150:])
        bz2f.close()

    def testSeekBackwards(self):
        # "Test BZ2File.seek(-150, 1)"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        bz2f.read(500)
        bz2f.seek(-150, 1)
        self.assertEqual(bz2f.read(), self.TEXT[500-150:])
        bz2f.close()

    def testSeekBackwardsFromEnd(self):
        # "Test BZ2File.seek(-150, 2)"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        bz2f.seek(-150, 2)
        self.assertEqual(bz2f.read(), self.TEXT[len(self.TEXT)-150:])
        bz2f.close()

    def testSeekPostEnd(self):
        # "Test BZ2File.seek(150000)"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        bz2f.seek(150000)
        self.assertEqual(bz2f.tell(), len(self.TEXT))
        self.assertEqual(bz2f.read(), b"")
        bz2f.close()

    def testSeekPostEndTwice(self):
        # "Test BZ2File.seek(150000) twice"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        bz2f.seek(150000)
        bz2f.seek(150000)
        self.assertEqual(bz2f.tell(), len(self.TEXT))
        self.assertEqual(bz2f.read(), b"")
        bz2f.close()

    def testSeekPreStart(self):
        # "Test BZ2File.seek(-150, 0)"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        bz2f.seek(-150)
        self.assertEqual(bz2f.tell(), 0)
        self.assertEqual(bz2f.read(), self.TEXT)
        bz2f.close()

    def testOpenDel(self):
        # "Test opening and deleting a file many times"
        self.createTempFile()
        for i in range(10000):
            o = BZ2File(self.filename)
            del o

    def testOpenNonexistent(self):
        # "Test opening a nonexistent file"
        self.assertRaises(IOError, BZ2File, "/non/existent")

    def testBug1191043(self):
        # readlines() for files containing no newline
        data = b'BZh91AY&SY\xd9b\x89]\x00\x00\x00\x03\x80\x04\x00\x02\x00\x0c\x00 \x00!\x9ah3M\x13<]\xc9\x14\xe1BCe\x8a%t'
        f = open(self.filename, "wb")
        f.write(data)
        f.close()
        bz2f = BZ2File(self.filename)
        lines = bz2f.readlines()
        bz2f.close()
        self.assertEqual(lines, [b'Test'])
        bz2f = BZ2File(self.filename)
        xlines = list(bz2f.readlines())
        bz2f.close()
        self.assertEqual(xlines, [b'Test'])


class BZ2CompressorTest(BaseTest):
    def testCompress(self):
        # "Test BZ2Compressor.compress()/flush()"
        bz2c = BZ2Compressor()
        self.assertRaises(TypeError, bz2c.compress)
        data = bz2c.compress(self.TEXT)
        data += bz2c.flush()
        self.assertEqual(self.decompress(data), self.TEXT)

    def testCompressChunks10(self):
        # "Test BZ2Compressor.compress()/flush() with chunks of 10 bytes"
        bz2c = BZ2Compressor()
        n = 0
        data = b''
        while 1:
            str = self.TEXT[n*10:(n+1)*10]
            if not str:
                break
            data += bz2c.compress(str)
            n += 1
        data += bz2c.flush()
        self.assertEqual(self.decompress(data), self.TEXT)

class BZ2DecompressorTest(BaseTest):
    def test_Constructor(self):
        self.assertRaises(TypeError, BZ2Decompressor, 42)

    def testDecompress(self):
        # "Test BZ2Decompressor.decompress()"
        bz2d = BZ2Decompressor()
        self.assertRaises(TypeError, bz2d.decompress)
        text = bz2d.decompress(self.DATA)
        self.assertEqual(text, self.TEXT)

    def testDecompressChunks10(self):
        # "Test BZ2Decompressor.decompress() with chunks of 10 bytes"
        bz2d = BZ2Decompressor()
        text = b''
        n = 0
        while 1:
            str = self.DATA[n*10:(n+1)*10]
            if not str:
                break
            text += bz2d.decompress(str)
            n += 1
        self.assertEqual(text, self.TEXT)

    def testDecompressUnusedData(self):
        # "Test BZ2Decompressor.decompress() with unused data"
        bz2d = BZ2Decompressor()
        unused_data = b"this is unused data"
        text = bz2d.decompress(self.DATA+unused_data)
        self.assertEqual(text, self.TEXT)
        self.assertEqual(bz2d.unused_data, unused_data)

    def testEOFError(self):
        # "Calling BZ2Decompressor.decompress() after EOS must raise EOFError"
        bz2d = BZ2Decompressor()
        text = bz2d.decompress(self.DATA)
        self.assertRaises(EOFError, bz2d.decompress, "anything")


class FuncTest(BaseTest):
    "Test module functions"

    def testCompress(self):
        # "Test compress() function"
        data = bz2.compress(self.TEXT)
        self.assertEqual(self.decompress(data), self.TEXT)

    def testDecompress(self):
        # "Test decompress() function"
        text = bz2.decompress(self.DATA)
        self.assertEqual(text, self.TEXT)

    def testDecompressEmpty(self):
        # "Test decompress() function with empty string"
        text = bz2.decompress(b"")
        self.assertEqual(text, b"")

    def testDecompressIncomplete(self):
        # "Test decompress() function with incomplete data"
        self.assertRaises(ValueError, bz2.decompress, self.DATA[:-10])

def test_main():
    test_support.run_unittest(
        BZ2FileTest,
        BZ2CompressorTest,
        BZ2DecompressorTest,
        FuncTest
    )
    test_support.reap_children()

if __name__ == '__main__':
    test_main()

# vim:ts=4:sw=4
