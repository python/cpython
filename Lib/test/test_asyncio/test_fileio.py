"""Tests for asyncio.fileio (async file I/O)."""

import asyncio
import os
import pathlib
import tempfile
import unittest


class TestOpenFile(unittest.IsolatedAsyncioTestCase):

    async def test_open_file_text_read(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            tmp.write("hello world\n")
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'r') as f:
                content = await f.read()
            self.assertEqual(content, "hello world\n")
        finally:
            os.unlink(path)

    async def test_open_file_text_write(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'w') as f:
                await f.write("async write\n")
            with open(path, 'r') as f:
                self.assertEqual(f.read(), "async write\n")
        finally:
            os.unlink(path)

    async def test_open_file_binary(self):
        with tempfile.NamedTemporaryFile(mode='wb', suffix='.bin',
                                         delete=False) as tmp:
            tmp.write(b'\x00\x01\x02\x03')
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'rb') as f:
                data = await f.read()
            self.assertEqual(data, b'\x00\x01\x02\x03')
        finally:
            os.unlink(path)

    async def test_async_context_manager(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            tmp.write("ctx test\n")
            path = tmp.name
        try:
            f = await asyncio.open_file(path, 'r')
            async with f:
                content = await f.read()
            self.assertEqual(content, "ctx test\n")
            self.assertTrue(f.closed)
        finally:
            os.unlink(path)

    async def test_async_iteration(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            tmp.write("line1\nline2\nline3\n")
            path = tmp.name
        try:
            lines = []
            async with await asyncio.open_file(path, 'r') as f:
                async for line in f:
                    lines.append(line)
            self.assertEqual(lines, ["line1\n", "line2\n", "line3\n"])
        finally:
            os.unlink(path)


class TestWrapFile(unittest.IsolatedAsyncioTestCase):

    async def test_wrap_file(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            tmp.write("wrap test\n")
            path = tmp.name
        try:
            raw = open(path, 'r')
            f = asyncio.wrap_file(raw)
            content = await f.read()
            self.assertEqual(content, "wrap test\n")
            await f.aclose()
            self.assertTrue(raw.closed)
        finally:
            os.unlink(path)

    async def test_wrap_file_invalid_no_close(self):
        with self.assertRaises(TypeError):
            asyncio.wrap_file(42)

    async def test_wrap_file_invalid_no_read_write(self):
        class FakeFile:
            def close(self): pass
        with self.assertRaises(TypeError):
            asyncio.wrap_file(FakeFile())


class TestAsyncFile(unittest.IsolatedAsyncioTestCase):

    async def test_read_write_seek_tell(self):
        with tempfile.NamedTemporaryFile(mode='w+b', suffix='.bin',
                                         delete=False) as tmp:
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'w+b') as f:
                await f.write(b'hello')
                pos = await f.tell()
                self.assertEqual(pos, 5)
                await f.seek(0)
                data = await f.read()
                self.assertEqual(data, b'hello')
        finally:
            os.unlink(path)

    async def test_readline_readlines(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            tmp.write("aaa\nbbb\nccc\n")
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'r') as f:
                line1 = await f.readline()
                self.assertEqual(line1, "aaa\n")
                rest = await f.readlines()
                self.assertEqual(rest, ["bbb\n", "ccc\n"])
        finally:
            os.unlink(path)

    async def test_flush_truncate(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'w+') as f:
                await f.write("hello world")
                await f.flush()
                await f.seek(5)
                await f.truncate()
                await f.seek(0)
                data = await f.read()
                self.assertEqual(data, "hello")
        finally:
            os.unlink(path)

    async def test_sync_attributes(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'r') as f:
                self.assertEqual(f.name, path)
                self.assertEqual(f.mode, 'r')
                self.assertFalse(f.closed)
            self.assertTrue(f.closed)
        finally:
            os.unlink(path)

    async def test_wrapped_property(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            path = tmp.name
        try:
            raw = open(path, 'r')
            f = asyncio.wrap_file(raw)
            self.assertIs(f.wrapped, raw)
            await f.aclose()
        finally:
            os.unlink(path)

    async def test_aclose(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            path = tmp.name
        try:
            f = await asyncio.open_file(path, 'r')
            self.assertFalse(f.closed)
            await f.aclose()
            self.assertTrue(f.closed)
        finally:
            os.unlink(path)

    async def test_repr(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'r') as f:
                r = repr(f)
                self.assertIn('AsyncFile', r)
                self.assertIn('wrapped=', r)
        finally:
            os.unlink(path)

    async def test_read1_buffered(self):
        with tempfile.NamedTemporaryFile(mode='wb', suffix='.bin',
                                         delete=False) as tmp:
            tmp.write(b'data')
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'rb') as f:
                data = await f.read1()
                self.assertIsInstance(data, bytes)
                self.assertIn(b'data', data)
        finally:
            os.unlink(path)

    async def test_read1_text_raises(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            tmp.write('data')
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'r') as f:
                with self.assertRaises(AttributeError):
                    await f.read1()
        finally:
            os.unlink(path)

    async def test_peek_buffered(self):
        with tempfile.NamedTemporaryFile(mode='wb', suffix='.bin',
                                         delete=False) as tmp:
            tmp.write(b'peek data')
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'rb') as f:
                data = await f.peek(4)
                self.assertIsInstance(data, bytes)
                # peek doesn't advance position
                full = await f.read()
                self.assertEqual(full, b'peek data')
        finally:
            os.unlink(path)

    async def test_writelines(self):
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False) as tmp:
            path = tmp.name
        try:
            async with await asyncio.open_file(path, 'w') as f:
                await f.writelines(["a\n", "b\n", "c\n"])
            with open(path, 'r') as f:
                self.assertEqual(f.read(), "a\nb\nc\n")
        finally:
            os.unlink(path)


class TestPath(unittest.IsolatedAsyncioTestCase):

    async def test_path_construction(self):
        p = asyncio.Path("/tmp")
        self.assertEqual(str(p), "/tmp")

    async def test_path_from_pathlib(self):
        pp = pathlib.Path("/tmp")
        p = asyncio.Path(pp)
        self.assertEqual(str(p), "/tmp")

    async def test_sync_properties(self):
        p = asyncio.Path("/tmp/foo/bar.txt")
        self.assertEqual(p.name, "bar.txt")
        self.assertEqual(p.stem, "bar")
        self.assertEqual(p.suffix, ".txt")
        self.assertEqual(p.parent, asyncio.Path("/tmp/foo"))
        self.assertEqual(p.parts, ("/", "tmp", "foo", "bar.txt"))

    async def test_truediv(self):
        p = asyncio.Path("/tmp") / "foo"
        self.assertIsInstance(p, asyncio.Path)
        self.assertEqual(str(p), "/tmp/foo")

    async def test_rtruediv(self):
        p = "/tmp" / asyncio.Path("foo")
        self.assertIsInstance(p, asyncio.Path)
        self.assertEqual(str(p), "/tmp/foo")

    async def test_fspath(self):
        p = asyncio.Path("/tmp/foo")
        self.assertEqual(os.fspath(p), "/tmp/foo")

    async def test_exists(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            p = asyncio.Path(tmpdir)
            self.assertTrue(await p.exists())
        p_gone = asyncio.Path(tmpdir)
        self.assertFalse(await p_gone.exists())

    async def test_stat(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            p = asyncio.Path(tmpdir)
            st = await p.stat()
            self.assertTrue(hasattr(st, 'st_mode'))

    async def test_mkdir_rmdir(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            d = asyncio.Path(tmpdir) / "newdir"
            await d.mkdir()
            self.assertTrue(await d.is_dir())
            await d.rmdir()
            self.assertFalse(await d.exists())

    async def test_touch_unlink(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f = asyncio.Path(tmpdir) / "newfile.txt"
            await f.touch()
            self.assertTrue(await f.is_file())
            await f.unlink()
            self.assertFalse(await f.exists())

    async def test_read_write_text(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f = asyncio.Path(tmpdir) / "test.txt"
            await f.write_text("hello async")
            content = await f.read_text()
            self.assertEqual(content, "hello async")

    async def test_read_write_bytes(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f = asyncio.Path(tmpdir) / "test.bin"
            await f.write_bytes(b'\xde\xad\xbe\xef')
            data = await f.read_bytes()
            self.assertEqual(data, b'\xde\xad\xbe\xef')

    async def test_open_returns_async_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f = asyncio.Path(tmpdir) / "test.txt"
            await f.write_text("open test")
            async with await f.open('r') as af:
                self.assertIsInstance(af, asyncio.AsyncFile)
                content = await af.read()
                self.assertEqual(content, "open test")

    async def test_rename(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src = asyncio.Path(tmpdir) / "src.txt"
            dst_path = os.path.join(tmpdir, "dst.txt")
            await src.write_text("rename me")
            result = await src.rename(dst_path)
            self.assertIsInstance(result, asyncio.Path)
            self.assertFalse(await src.exists())
            dst = asyncio.Path(dst_path)
            self.assertEqual(await dst.read_text(), "rename me")

    async def test_replace(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src = asyncio.Path(tmpdir) / "src.txt"
            dst = asyncio.Path(tmpdir) / "dst.txt"
            await src.write_text("source")
            await dst.write_text("target")
            result = await src.replace(str(dst))
            self.assertIsInstance(result, asyncio.Path)
            content = await dst.read_text()
            self.assertEqual(content, "source")

    async def test_resolve(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            p = asyncio.Path(tmpdir)
            resolved = await p.resolve()
            self.assertIsInstance(resolved, asyncio.Path)
            self.assertTrue(resolved.is_absolute())

    async def test_absolute(self):
        p = asyncio.Path("relative")
        result = await p.absolute()
        self.assertIsInstance(result, asyncio.Path)
        self.assertTrue(result.is_absolute())

    async def test_iterdir(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            for name in ["a.txt", "b.txt", "c.txt"]:
                pathlib.Path(tmpdir, name).touch()
            p = asyncio.Path(tmpdir)
            names = []
            async for child in p.iterdir():
                self.assertIsInstance(child, asyncio.Path)
                names.append(child.name)
            self.assertEqual(sorted(names), ["a.txt", "b.txt", "c.txt"])

    async def test_glob(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            for name in ["a.txt", "b.py", "c.txt"]:
                pathlib.Path(tmpdir, name).touch()
            p = asyncio.Path(tmpdir)
            names = []
            async for child in p.glob("*.txt"):
                self.assertIsInstance(child, asyncio.Path)
                names.append(child.name)
            self.assertEqual(sorted(names), ["a.txt", "c.txt"])

    async def test_rglob(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            sub = pathlib.Path(tmpdir, "sub")
            sub.mkdir()
            pathlib.Path(tmpdir, "a.txt").touch()
            pathlib.Path(sub, "b.txt").touch()
            p = asyncio.Path(tmpdir)
            names = []
            async for child in p.rglob("*.txt"):
                names.append(child.name)
            self.assertEqual(sorted(names), ["a.txt", "b.txt"])

    async def test_walk(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            sub = pathlib.Path(tmpdir, "sub")
            sub.mkdir()
            pathlib.Path(tmpdir, "a.txt").touch()
            pathlib.Path(sub, "b.txt").touch()
            p = asyncio.Path(tmpdir)
            entries = []
            async for dirpath, dirnames, filenames in p.walk():
                self.assertIsInstance(dirpath, asyncio.Path)
                entries.append((str(dirpath), sorted(dirnames),
                                sorted(filenames)))
            # Should have root and sub directory
            self.assertEqual(len(entries), 2)

    async def test_cwd(self):
        p = await asyncio.Path.cwd()
        self.assertIsInstance(p, asyncio.Path)
        self.assertTrue(p.is_absolute())

    async def test_home(self):
        p = await asyncio.Path.home()
        self.assertIsInstance(p, asyncio.Path)
        self.assertTrue(p.is_absolute())

    @unittest.skipUnless(hasattr(os, 'symlink'), 'requires symlink support')
    async def test_symlink(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            target = asyncio.Path(tmpdir) / "target.txt"
            await target.write_text("symlink target")
            link = asyncio.Path(tmpdir) / "link.txt"
            await link.symlink_to(str(target))
            self.assertTrue(await link.is_symlink())
            resolved = await link.readlink()
            self.assertIsInstance(resolved, asyncio.Path)
            content = await link.read_text()
            self.assertEqual(content, "symlink target")

    async def test_repr(self):
        p = asyncio.Path("/tmp/foo")
        self.assertEqual(repr(p), "asyncio.Path('/tmp/foo')")

    async def test_comparison(self):
        a = asyncio.Path("/a")
        b = asyncio.Path("/b")
        a2 = asyncio.Path("/a")
        self.assertEqual(a, a2)
        self.assertNotEqual(a, b)
        self.assertLess(a, b)
        self.assertLessEqual(a, a2)
        self.assertGreater(b, a)
        self.assertGreaterEqual(b, a)

    async def test_hash(self):
        a = asyncio.Path("/tmp/foo")
        b = asyncio.Path("/tmp/foo")
        self.assertEqual(hash(a), hash(b))
        s = {a, b}
        self.assertEqual(len(s), 1)

    async def test_joinpath(self):
        p = asyncio.Path("/tmp")
        joined = p.joinpath("foo", "bar")
        self.assertIsInstance(joined, asyncio.Path)
        self.assertEqual(str(joined), "/tmp/foo/bar")

    async def test_with_name_stem_suffix(self):
        p = asyncio.Path("/tmp/foo.txt")
        self.assertEqual(str(p.with_name("bar.txt")), "/tmp/bar.txt")
        self.assertEqual(str(p.with_stem("bar")), "/tmp/bar.txt")
        self.assertEqual(str(p.with_suffix(".py")), "/tmp/foo.py")

    async def test_is_absolute(self):
        self.assertTrue(asyncio.Path("/tmp").is_absolute())
        self.assertFalse(asyncio.Path("relative").is_absolute())

    @unittest.skipUnless(hasattr(pathlib.Path, 'copy'),
                         'requires pathlib.Path.copy (3.14+)')
    async def test_copy(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src = asyncio.Path(tmpdir) / "src.txt"
            dst = asyncio.Path(tmpdir) / "dst.txt"
            await src.write_text("copy me")
            result = await src.copy(str(dst))
            self.assertIsInstance(result, asyncio.Path)
            self.assertEqual(await dst.read_text(), "copy me")

    @unittest.skipUnless(hasattr(pathlib.Path, 'move'),
                         'requires pathlib.Path.move (3.14+)')
    async def test_move(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src = asyncio.Path(tmpdir) / "src.txt"
            dst_path = os.path.join(tmpdir, "moved.txt")
            await src.write_text("move me")
            result = await src.move(dst_path)
            self.assertIsInstance(result, asyncio.Path)
            self.assertFalse(await src.exists())
            dst = asyncio.Path(dst_path)
            self.assertEqual(await dst.read_text(), "move me")

    async def test_chmod(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f = asyncio.Path(tmpdir) / "test.txt"
            await f.touch()
            await f.chmod(0o644)
            st = await f.stat()
            self.assertEqual(st.st_mode & 0o777, 0o644)

    async def test_expanduser(self):
        p = asyncio.Path("~")
        expanded = await p.expanduser()
        self.assertIsInstance(expanded, asyncio.Path)
        self.assertTrue(expanded.is_absolute())

    async def test_parents_property(self):
        p = asyncio.Path("/a/b/c")
        parents = p.parents
        self.assertIsInstance(parents, tuple)
        self.assertTrue(all(isinstance(x, asyncio.Path) for x in parents))
        self.assertEqual(str(parents[0]), "/a/b")

    async def test_samefile(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f = asyncio.Path(tmpdir) / "test.txt"
            await f.touch()
            self.assertTrue(await f.samefile(str(f)))


if __name__ == '__main__':
    unittest.main()
