#!/usr/bin/env python
from test import test_support
from test.test_support import TESTFN, _4G, bigmemtest, import_module, findfile

import unittest
from cStringIO import StringIO
import os
import subprocess
import sys

try:
    import threading
except ImportError:
    threading = None

bz2 = import_module('bz2')
from bz2 import BZ2File, BZ2Compressor, BZ2Decompressor

has_cmdline_bunzip2 = sys.platform not in ("win32", "os2emx", "riscos")

class BaseTest(unittest.TestCase):
    "Base for other testcases."
    TEXT = 'root:x:0:0:root:/root:/bin/bash\nbin:x:1:1:bin:/bin:\ndaemon:x:2:2:daemon:/sbin:\nadm:x:3:4:adm:/var/adm:\nlp:x:4:7:lp:/var/spool/lpd:\nsync:x:5:0:sync:/sbin:/bin/sync\nshutdown:x:6:0:shutdown:/sbin:/sbin/shutdown\nhalt:x:7:0:halt:/sbin:/sbin/halt\nmail:x:8:12:mail:/var/spool/mail:\nnews:x:9:13:news:/var/spool/news:\nuucp:x:10:14:uucp:/var/spool/uucp:\noperator:x:11:0:operator:/root:\ngames:x:12:100:games:/usr/games:\ngopher:x:13:30:gopher:/usr/lib/gopher-data:\nftp:x:14:50:FTP User:/var/ftp:/bin/bash\nnobody:x:65534:65534:Nobody:/home:\npostfix:x:100:101:postfix:/var/spool/postfix:\nniemeyer:x:500:500::/home/niemeyer:/bin/bash\npostgres:x:101:102:PostgreSQL Server:/var/lib/pgsql:/bin/bash\nmysql:x:102:103:MySQL server:/var/lib/mysql:/bin/bash\nwww:x:103:104::/var/www:/bin/false\n'
    DATA = 'BZh91AY&SY.\xc8N\x18\x00\x01>_\x80\x00\x10@\x02\xff\xf0\x01\x07n\x00?\xe7\xff\xe00\x01\x99\xaa\x00\xc0\x03F\x86\x8c#&\x83F\x9a\x03\x06\xa6\xd0\xa6\x93M\x0fQ\xa7\xa8\x06\x804hh\x12$\x11\xa4i4\xf14S\xd2<Q\xb5\x0fH\xd3\xd4\xdd\xd5\x87\xbb\xf8\x94\r\x8f\xafI\x12\xe1\xc9\xf8/E\x00pu\x89\x12]\xc9\xbbDL\nQ\x0e\t1\x12\xdf\xa0\xc0\x97\xac2O9\x89\x13\x94\x0e\x1c7\x0ed\x95I\x0c\xaaJ\xa4\x18L\x10\x05#\x9c\xaf\xba\xbc/\x97\x8a#C\xc8\xe1\x8cW\xf9\xe2\xd0\xd6M\xa7\x8bXa<e\x84t\xcbL\xb3\xa7\xd9\xcd\xd1\xcb\x84.\xaf\xb3\xab\xab\xad`n}\xa0lh\tE,\x8eZ\x15\x17VH>\x88\xe5\xcd9gd6\x0b\n\xe9\x9b\xd5\x8a\x99\xf7\x08.K\x8ev\xfb\xf7xw\xbb\xdf\xa1\x92\xf1\xdd|/";\xa2\xba\x9f\xd5\xb1#A\xb6\xf6\xb3o\xc9\xc5y\\\xebO\xe7\x85\x9a\xbc\xb6f8\x952\xd5\xd7"%\x89>V,\xf7\xa6z\xe2\x9f\xa3\xdf\x11\x11"\xd6E)I\xa9\x13^\xca\xf3r\xd0\x03U\x922\xf26\xec\xb6\xed\x8b\xc3U\x13\x9d\xc5\x170\xa4\xfa^\x92\xacDF\x8a\x97\xd6\x19\xfe\xdd\xb8\xbd\x1a\x9a\x19\xa3\x80ankR\x8b\xe5\xd83]\xa9\xc6\x08\x82f\xf6\xb9"6l$\xb8j@\xc0\x8a\xb0l1..\xbak\x83ls\x15\xbc\xf4\xc1\x13\xbe\xf8E\xb8\x9d\r\xa8\x9dk\x84\xd3n\xfa\xacQ\x07\xb1%y\xaav\xb4\x08\xe0z\x1b\x16\xf5\x04\xe9\xcc\xb9\x08z\x1en7.G\xfc]\xc9\x14\xe1B@\xbb!8`'
    DATA_CRLF = 'BZh91AY&SY\xaez\xbbN\x00\x01H\xdf\x80\x00\x12@\x02\xff\xf0\x01\x07n\x00?\xe7\xff\xe0@\x01\xbc\xc6`\x86*\x8d=M\xa9\x9a\x86\xd0L@\x0fI\xa6!\xa1\x13\xc8\x88jdi\x8d@\x03@\x1a\x1a\x0c\x0c\x83 \x00\xc4h2\x19\x01\x82D\x84e\t\xe8\x99\x89\x19\x1ah\x00\r\x1a\x11\xaf\x9b\x0fG\xf5(\x1b\x1f?\t\x12\xcf\xb5\xfc\x95E\x00ps\x89\x12^\xa4\xdd\xa2&\x05(\x87\x04\x98\x89u\xe40%\xb6\x19\'\x8c\xc4\x89\xca\x07\x0e\x1b!\x91UIFU%C\x994!DI\xd2\xfa\xf0\xf1N8W\xde\x13A\xf5\x9cr%?\x9f3;I45A\xd1\x8bT\xb1<l\xba\xcb_\xc00xY\x17r\x17\x88\x08\x08@\xa0\ry@\x10\x04$)`\xf2\xce\x89z\xb0s\xec\x9b.iW\x9d\x81\xb5-+t\x9f\x1a\'\x97dB\xf5x\xb5\xbe.[.\xd7\x0e\x81\xe7\x08\x1cN`\x88\x10\xca\x87\xc3!"\x80\x92R\xa1/\xd1\xc0\xe6mf\xac\xbd\x99\xcca\xb3\x8780>\xa4\xc7\x8d\x1a\\"\xad\xa1\xabyBg\x15\xb9l\x88\x88\x91k"\x94\xa4\xd4\x89\xae*\xa6\x0b\x10\x0c\xd6\xd4m\xe86\xec\xb5j\x8a\x86j\';\xca.\x01I\xf2\xaaJ\xe8\x88\x8cU+t3\xfb\x0c\n\xa33\x13r2\r\x16\xe0\xb3(\xbf\x1d\x83r\xe7M\xf0D\x1365\xd8\x88\xd3\xa4\x92\xcb2\x06\x04\\\xc1\xb0\xea//\xbek&\xd8\xe6+t\xe5\xa1\x13\xada\x16\xder5"w]\xa2i\xb7[\x97R \xe2IT\xcd;Z\x04dk4\xad\x8a\t\xd3\x81z\x10\xf1:^`\xab\x1f\xc5\xdc\x91N\x14$+\x9e\xae\xd3\x80'
    EMPTY_DATA = 'BZh9\x17rE8P\x90\x00\x00\x00\x00'

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
        with open(self.filename, "wb") as f:
            if crlf:
                data = self.DATA_CRLF
            else:
                data = self.DATA
            f.write(data)

    def testRead(self):
        # "Test BZ2File.read()"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.read, None)
            self.assertEqual(bz2f.read(), self.TEXT)

    def testRead0(self):
        # Test BBZ2File.read(0)"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.read, None)
            self.assertEqual(bz2f.read(0), "")

    def testReadChunk10(self):
        # "Test BZ2File.read() in chunks of 10 bytes"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            text = ''
            while 1:
                str = bz2f.read(10)
                if not str:
                    break
                text += str
            self.assertEqual(text, self.TEXT)

    def testRead100(self):
        # "Test BZ2File.read(100)"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertEqual(bz2f.read(100), self.TEXT[:100])

    def testReadLine(self):
        # "Test BZ2File.readline()"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.readline, None)
            sio = StringIO(self.TEXT)
            for line in sio.readlines():
                self.assertEqual(bz2f.readline(), line)

    def testReadLines(self):
        # "Test BZ2File.readlines()"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.readlines, None)
            sio = StringIO(self.TEXT)
            self.assertEqual(bz2f.readlines(), sio.readlines())

    def testIterator(self):
        # "Test iter(BZ2File)"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            sio = StringIO(self.TEXT)
            self.assertEqual(list(iter(bz2f)), sio.readlines())

    def testClosedIteratorDeadlock(self):
        # "Test that iteration on a closed bz2file releases the lock."
        # http://bugs.python.org/issue3309
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        bz2f.close()
        self.assertRaises(ValueError, bz2f.next)
        # This call will deadlock of the above .next call failed to
        # release the lock.
        self.assertRaises(ValueError, bz2f.readlines)

    def testXReadLines(self):
        # "Test BZ2File.xreadlines()"
        self.createTempFile()
        bz2f = BZ2File(self.filename)
        sio = StringIO(self.TEXT)
        self.assertEqual(list(bz2f.xreadlines()), sio.readlines())
        bz2f.close()

    def testUniversalNewlinesLF(self):
        # "Test BZ2File.read() with universal newlines (\\n)"
        self.createTempFile()
        bz2f = BZ2File(self.filename, "rU")
        self.assertEqual(bz2f.read(), self.TEXT)
        self.assertEqual(bz2f.newlines, "\n")
        bz2f.close()

    def testUniversalNewlinesCRLF(self):
        # "Test BZ2File.read() with universal newlines (\\r\\n)"
        self.createTempFile(crlf=1)
        bz2f = BZ2File(self.filename, "rU")
        self.assertEqual(bz2f.read(), self.TEXT)
        self.assertEqual(bz2f.newlines, "\r\n")
        bz2f.close()

    def testWrite(self):
        # "Test BZ2File.write()"
        with BZ2File(self.filename, "w") as bz2f:
            self.assertRaises(TypeError, bz2f.write)
            bz2f.write(self.TEXT)
        with open(self.filename, 'rb') as f:
            self.assertEqual(self.decompress(f.read()), self.TEXT)

    def testWriteChunks10(self):
        # "Test BZ2File.write() with chunks of 10 bytes"
        with BZ2File(self.filename, "w") as bz2f:
            n = 0
            while 1:
                str = self.TEXT[n*10:(n+1)*10]
                if not str:
                    break
                bz2f.write(str)
                n += 1
        with open(self.filename, 'rb') as f:
            self.assertEqual(self.decompress(f.read()), self.TEXT)

    def testWriteLines(self):
        # "Test BZ2File.writelines()"
        with BZ2File(self.filename, "w") as bz2f:
            self.assertRaises(TypeError, bz2f.writelines)
            sio = StringIO(self.TEXT)
            bz2f.writelines(sio.readlines())
        # patch #1535500
        self.assertRaises(ValueError, bz2f.writelines, ["a"])
        with open(self.filename, 'rb') as f:
            self.assertEqual(self.decompress(f.read()), self.TEXT)

    def testWriteMethodsOnReadOnlyFile(self):
        with BZ2File(self.filename, "w") as bz2f:
            bz2f.write("abc")

        with BZ2File(self.filename, "r") as bz2f:
            self.assertRaises(IOError, bz2f.write, "a")
            self.assertRaises(IOError, bz2f.writelines, ["a"])

    def testSeekForward(self):
        # "Test BZ2File.seek(150, 0)"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            self.assertRaises(TypeError, bz2f.seek)
            bz2f.seek(150)
            self.assertEqual(bz2f.read(), self.TEXT[150:])

    def testSeekBackwards(self):
        # "Test BZ2File.seek(-150, 1)"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.read(500)
            bz2f.seek(-150, 1)
            self.assertEqual(bz2f.read(), self.TEXT[500-150:])

    def testSeekBackwardsFromEnd(self):
        # "Test BZ2File.seek(-150, 2)"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(-150, 2)
            self.assertEqual(bz2f.read(), self.TEXT[len(self.TEXT)-150:])

    def testSeekPostEnd(self):
        # "Test BZ2File.seek(150000)"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(150000)
            self.assertEqual(bz2f.tell(), len(self.TEXT))
            self.assertEqual(bz2f.read(), "")

    def testSeekPostEndTwice(self):
        # "Test BZ2File.seek(150000) twice"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(150000)
            bz2f.seek(150000)
            self.assertEqual(bz2f.tell(), len(self.TEXT))
            self.assertEqual(bz2f.read(), "")

    def testSeekPreStart(self):
        # "Test BZ2File.seek(-150, 0)"
        self.createTempFile()
        with BZ2File(self.filename) as bz2f:
            bz2f.seek(-150)
            self.assertEqual(bz2f.tell(), 0)
            self.assertEqual(bz2f.read(), self.TEXT)

    def testOpenDel(self):
        # "Test opening and deleting a file many times"
        self.createTempFile()
        for i in xrange(10000):
            o = BZ2File(self.filename)
            del o

    def testOpenNonexistent(self):
        # "Test opening a nonexistent file"
        self.assertRaises(IOError, BZ2File, "/non/existent")

    def testModeU(self):
        # Bug #1194181: bz2.BZ2File opened for write with mode "U"
        self.createTempFile()
        bz2f = BZ2File(self.filename, "U")
        bz2f.close()
        f = file(self.filename)
        f.seek(0, 2)
        self.assertEqual(f.tell(), len(self.DATA))
        f.close()

    def testBug1191043(self):
        # readlines() for files containing no newline
        data = 'BZh91AY&SY\xd9b\x89]\x00\x00\x00\x03\x80\x04\x00\x02\x00\x0c\x00 \x00!\x9ah3M\x13<]\xc9\x14\xe1BCe\x8a%t'
        with open(self.filename, "wb") as f:
            f.write(data)
        with BZ2File(self.filename) as bz2f:
            lines = bz2f.readlines()
        self.assertEqual(lines, ['Test'])
        with BZ2File(self.filename) as bz2f:
            xlines = list(bz2f.readlines())
        self.assertEqual(xlines, ['Test'])

    def testContextProtocol(self):
        # BZ2File supports the context management protocol
        f = None
        with BZ2File(self.filename, "wb") as f:
            f.write(b"xxx")
        f = BZ2File(self.filename, "rb")
        f.close()
        try:
            with f:
                pass
        except ValueError:
            pass
        else:
            self.fail("__enter__ on a closed file didn't raise an exception")
        try:
            with BZ2File(self.filename, "wb") as f:
                1 // 0
        except ZeroDivisionError:
            pass
        else:
            self.fail("1 // 0 didn't raise an exception")

    @unittest.skipUnless(threading, 'Threading required for this test.')
    def testThreading(self):
        # Using a BZ2File from several threads doesn't deadlock (issue #7205).
        data = "1" * 2**20
        nthreads = 10
        with bz2.BZ2File(self.filename, 'wb') as f:
            def comp():
                for i in range(5):
                    f.write(data)
            threads = [threading.Thread(target=comp) for i in range(nthreads)]
            for t in threads:
                t.start()
            for t in threads:
                t.join()

    def testMixedIterationReads(self):
        # Issue #8397: mixed iteration and reads should be forbidden.
        with bz2.BZ2File(self.filename, 'wb') as f:
            # The internal buffer size is hard-wired to 8192 bytes, we must
            # write out more than that for the test to stop half through
            # the buffer.
            f.write(self.TEXT * 100)
        with bz2.BZ2File(self.filename, 'rb') as f:
            next(f)
            self.assertRaises(ValueError, f.read)
            self.assertRaises(ValueError, f.readline)
            self.assertRaises(ValueError, f.readlines)

    @unittest.skipIf(sys.platform == 'win32',
                     'test depends on being able to delete a still-open file,'
                     ' which is not possible on Windows')
    def testInitNonExistentFile(self):
        # Issue #19878: Should not segfault when __init__ with non-existent
        # file for the second time.
        self.createTempFile()
        # Test close():
        with BZ2File(self.filename, "wb") as f:
            self.assertRaises(IOError, f.__init__, "non-existent-file")
        # Test object deallocation without call to close():
        f = bz2.BZ2File(self.filename)
        self.assertRaises(IOError, f.__init__, "non-existent-file")
        del f

class BZ2CompressorTest(BaseTest):
    def testCompress(self):
        # "Test BZ2Compressor.compress()/flush()"
        bz2c = BZ2Compressor()
        self.assertRaises(TypeError, bz2c.compress)
        data = bz2c.compress(self.TEXT)
        data += bz2c.flush()
        self.assertEqual(self.decompress(data), self.TEXT)

    def testCompressEmptyString(self):
        # "Test BZ2Compressor.compress()/flush() of empty string"
        bz2c = BZ2Compressor()
        data = bz2c.compress('')
        data += bz2c.flush()
        self.assertEqual(data, self.EMPTY_DATA)

    def testCompressChunks10(self):
        # "Test BZ2Compressor.compress()/flush() with chunks of 10 bytes"
        bz2c = BZ2Compressor()
        n = 0
        data = ''
        while 1:
            str = self.TEXT[n*10:(n+1)*10]
            if not str:
                break
            data += bz2c.compress(str)
            n += 1
        data += bz2c.flush()
        self.assertEqual(self.decompress(data), self.TEXT)

    @bigmemtest(_4G, memuse=1.25)
    def testBigmem(self, size):
        text = "a" * size
        bz2c = bz2.BZ2Compressor()
        data = bz2c.compress(text) + bz2c.flush()
        del text
        text = self.decompress(data)
        self.assertEqual(len(text), size)
        self.assertEqual(text.strip("a"), "")


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
        text = ''
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
        unused_data = "this is unused data"
        text = bz2d.decompress(self.DATA+unused_data)
        self.assertEqual(text, self.TEXT)
        self.assertEqual(bz2d.unused_data, unused_data)

    def testEOFError(self):
        # "Calling BZ2Decompressor.decompress() after EOS must raise EOFError"
        bz2d = BZ2Decompressor()
        text = bz2d.decompress(self.DATA)
        self.assertRaises(EOFError, bz2d.decompress, "anything")
        self.assertRaises(EOFError, bz2d.decompress, "")

    @bigmemtest(_4G, memuse=1.25)
    def testBigmem(self, size):
        # Issue #14398: decompression fails when output data is >=2GB.
        if size < _4G:
            self.skipTest("Test needs 5GB of memory to run.")
        compressed = bz2.compress("a" * _4G)
        text = bz2.BZ2Decompressor().decompress(compressed)
        self.assertEqual(len(text), _4G)
        self.assertEqual(text.strip("a"), "")


class FuncTest(BaseTest):
    "Test module functions"

    def testCompress(self):
        # "Test compress() function"
        data = bz2.compress(self.TEXT)
        self.assertEqual(self.decompress(data), self.TEXT)

    def testCompressEmptyString(self):
        # "Test compress() of empty string"
        text = bz2.compress('')
        self.assertEqual(text, self.EMPTY_DATA)

    def testDecompress(self):
        # "Test decompress() function"
        text = bz2.decompress(self.DATA)
        self.assertEqual(text, self.TEXT)

    def testDecompressEmpty(self):
        # "Test decompress() function with empty string"
        text = bz2.decompress("")
        self.assertEqual(text, "")

    def testDecompressToEmptyString(self):
        # "Test decompress() of minimal bz2 data to empty string"
        text = bz2.decompress(self.EMPTY_DATA)
        self.assertEqual(text, '')

    def testDecompressIncomplete(self):
        # "Test decompress() function with incomplete data"
        self.assertRaises(ValueError, bz2.decompress, self.DATA[:-10])

    @bigmemtest(_4G, memuse=1.25)
    def testCompressBigmem(self, size):
        text = "a" * size
        data = bz2.compress(text)
        del text
        text = self.decompress(data)
        self.assertEqual(len(text), size)
        self.assertEqual(text.strip("a"), "")

    @bigmemtest(_4G, memuse=1.25)
    def testDecompressBigmem(self, size):
        # Issue #14398: decompression fails when output data is >=2GB.
        if size < _4G:
            self.skipTest("Test needs 5GB of memory to run.")
        compressed = bz2.compress("a" * _4G)
        text = bz2.decompress(compressed)
        self.assertEqual(len(text), _4G)
        self.assertEqual(text.strip("a"), "")

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
