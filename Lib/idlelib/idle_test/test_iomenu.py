import unittest
import io

from idlelib.run import PseudoInputFile, PseudoOutputFile


class S(str):
    def __str__(self):
        return '%s:str' % type(self).__name__
    def __unicode__(self):
        return '%s:unicode' % type(self).__name__
    def __len__(self):
        return 3
    def __iter__(self):
        return iter('abc')
    def __getitem__(self, *args):
        return '%s:item' % type(self).__name__
    def __getslice__(self, *args):
        return '%s:slice' % type(self).__name__

class MockShell:
    def __init__(self):
        self.reset()

    def write(self, *args):
        self.written.append(args)

    def readline(self):
        return self.lines.pop()

    def close(self):
        pass

    def reset(self):
        self.written = []

    def push(self, lines):
        self.lines = list(lines)[::-1]


class PseudeOutputFilesTest(unittest.TestCase):
    def test_misc(self):
        shell = MockShell()
        f = PseudoOutputFile(shell, 'stdout', 'utf-8')
        self.assertIsInstance(f, io.TextIOBase)
        self.assertEqual(f.encoding, 'utf-8')
        self.assertIsNone(f.errors)
        self.assertIsNone(f.newlines)
        self.assertEqual(f.name, '<stdout>')
        self.assertFalse(f.closed)
        self.assertTrue(f.isatty())
        self.assertFalse(f.readable())
        self.assertTrue(f.writable())
        self.assertFalse(f.seekable())

    def test_unsupported(self):
        shell = MockShell()
        f = PseudoOutputFile(shell, 'stdout', 'utf-8')
        self.assertRaises(OSError, f.fileno)
        self.assertRaises(OSError, f.tell)
        self.assertRaises(OSError, f.seek, 0)
        self.assertRaises(OSError, f.read, 0)
        self.assertRaises(OSError, f.readline, 0)

    def test_write(self):
        shell = MockShell()
        f = PseudoOutputFile(shell, 'stdout', 'utf-8')
        f.write('test')
        self.assertEqual(shell.written, [('test', 'stdout')])
        shell.reset()
        f.write('t\xe8st')
        self.assertEqual(shell.written, [('t\xe8st', 'stdout')])
        shell.reset()

        f.write(S('t\xe8st'))
        self.assertEqual(shell.written, [('t\xe8st', 'stdout')])
        self.assertEqual(type(shell.written[0][0]), str)
        shell.reset()

        self.assertRaises(TypeError, f.write)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.write, b'test')
        self.assertRaises(TypeError, f.write, 123)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.write, 'test', 'spam')
        self.assertEqual(shell.written, [])

    def test_writelines(self):
        shell = MockShell()
        f = PseudoOutputFile(shell, 'stdout', 'utf-8')
        f.writelines([])
        self.assertEqual(shell.written, [])
        shell.reset()
        f.writelines(['one\n', 'two'])
        self.assertEqual(shell.written,
                         [('one\n', 'stdout'), ('two', 'stdout')])
        shell.reset()
        f.writelines(['on\xe8\n', 'tw\xf2'])
        self.assertEqual(shell.written,
                         [('on\xe8\n', 'stdout'), ('tw\xf2', 'stdout')])
        shell.reset()

        f.writelines([S('t\xe8st')])
        self.assertEqual(shell.written, [('t\xe8st', 'stdout')])
        self.assertEqual(type(shell.written[0][0]), str)
        shell.reset()

        self.assertRaises(TypeError, f.writelines)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.writelines, 123)
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.writelines, [b'test'])
        self.assertRaises(TypeError, f.writelines, [123])
        self.assertEqual(shell.written, [])
        self.assertRaises(TypeError, f.writelines, [], [])
        self.assertEqual(shell.written, [])

    def test_close(self):
        shell = MockShell()
        f = PseudoOutputFile(shell, 'stdout', 'utf-8')
        self.assertFalse(f.closed)
        f.write('test')
        f.close()
        self.assertTrue(f.closed)
        self.assertRaises(ValueError, f.write, 'x')
        self.assertEqual(shell.written, [('test', 'stdout')])
        f.close()
        self.assertRaises(TypeError, f.close, 1)


class PseudeInputFilesTest(unittest.TestCase):
    def test_misc(self):
        shell = MockShell()
        f = PseudoInputFile(shell, 'stdin', 'utf-8')
        self.assertIsInstance(f, io.TextIOBase)
        self.assertEqual(f.encoding, 'utf-8')
        self.assertIsNone(f.errors)
        self.assertIsNone(f.newlines)
        self.assertEqual(f.name, '<stdin>')
        self.assertFalse(f.closed)
        self.assertTrue(f.isatty())
        self.assertTrue(f.readable())
        self.assertFalse(f.writable())
        self.assertFalse(f.seekable())

    def test_unsupported(self):
        shell = MockShell()
        f = PseudoInputFile(shell, 'stdin', 'utf-8')
        self.assertRaises(OSError, f.fileno)
        self.assertRaises(OSError, f.tell)
        self.assertRaises(OSError, f.seek, 0)
        self.assertRaises(OSError, f.write, 'x')
        self.assertRaises(OSError, f.writelines, ['x'])

    def test_read(self):
        shell = MockShell()
        f = PseudoInputFile(shell, 'stdin', 'utf-8')
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.read(), 'one\ntwo\n')
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.read(-1), 'one\ntwo\n')
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.read(None), 'one\ntwo\n')
        shell.push(['one\n', 'two\n', 'three\n', ''])
        self.assertEqual(f.read(2), 'on')
        self.assertEqual(f.read(3), 'e\nt')
        self.assertEqual(f.read(10), 'wo\nthree\n')

        shell.push(['one\n', 'two\n'])
        self.assertEqual(f.read(0), '')
        self.assertRaises(TypeError, f.read, 1.5)
        self.assertRaises(TypeError, f.read, '1')
        self.assertRaises(TypeError, f.read, 1, 1)

    def test_readline(self):
        shell = MockShell()
        f = PseudoInputFile(shell, 'stdin', 'utf-8')
        shell.push(['one\n', 'two\n', 'three\n', 'four\n'])
        self.assertEqual(f.readline(), 'one\n')
        self.assertEqual(f.readline(-1), 'two\n')
        self.assertEqual(f.readline(None), 'three\n')
        shell.push(['one\ntwo\n'])
        self.assertEqual(f.readline(), 'one\n')
        self.assertEqual(f.readline(), 'two\n')
        shell.push(['one', 'two', 'three'])
        self.assertEqual(f.readline(), 'one')
        self.assertEqual(f.readline(), 'two')
        shell.push(['one\n', 'two\n', 'three\n'])
        self.assertEqual(f.readline(2), 'on')
        self.assertEqual(f.readline(1), 'e')
        self.assertEqual(f.readline(1), '\n')
        self.assertEqual(f.readline(10), 'two\n')

        shell.push(['one\n', 'two\n'])
        self.assertEqual(f.readline(0), '')
        self.assertRaises(TypeError, f.readlines, 1.5)
        self.assertRaises(TypeError, f.readlines, '1')
        self.assertRaises(TypeError, f.readlines, 1, 1)

    def test_readlines(self):
        shell = MockShell()
        f = PseudoInputFile(shell, 'stdin', 'utf-8')
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(), ['one\n', 'two\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(-1), ['one\n', 'two\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(None), ['one\n', 'two\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(0), ['one\n', 'two\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(3), ['one\n'])
        shell.push(['one\n', 'two\n', ''])
        self.assertEqual(f.readlines(4), ['one\n', 'two\n'])

        shell.push(['one\n', 'two\n', ''])
        self.assertRaises(TypeError, f.readlines, 1.5)
        self.assertRaises(TypeError, f.readlines, '1')
        self.assertRaises(TypeError, f.readlines, 1, 1)

    def test_close(self):
        shell = MockShell()
        f = PseudoInputFile(shell, 'stdin', 'utf-8')
        shell.push(['one\n', 'two\n', ''])
        self.assertFalse(f.closed)
        self.assertEqual(f.readline(), 'one\n')
        f.close()
        self.assertFalse(f.closed)
        self.assertEqual(f.readline(), 'two\n')
        self.assertRaises(TypeError, f.close, 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
