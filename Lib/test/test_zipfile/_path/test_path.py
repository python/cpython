import io
import itertools
import contextlib
import pathlib
import pickle
import stat
import sys
import time
import unittest
import zipfile
import zipfile._path

from test.support.os_helper import temp_dir, FakePath

from ._functools import compose
from ._itertools import Counter

from ._test_params import parameterize, Invoked


class jaraco:
    class itertools:
        Counter = Counter


def _make_link(info: zipfile.ZipInfo):  # type: ignore[name-defined]
    info.external_attr |= stat.S_IFLNK << 16


def build_alpharep_fixture():
    """
    Create a zip file with this structure:

    .
    ├── a.txt
    ├── n.txt (-> a.txt)
    ├── b
    │   ├── c.txt
    │   ├── d
    │   │   └── e.txt
    │   └── f.txt
    ├── g
    │   └── h
    │       └── i.txt
    └── j
        ├── k.bin
        ├── l.baz
        └── m.bar

    This fixture has the following key characteristics:

    - a file at the root (a)
    - a file two levels deep (b/d/e)
    - multiple files in a directory (b/c, b/f)
    - a directory containing only a directory (g/h)
    - a directory with files of different extensions (j/klm)
    - a symlink (n) pointing to (a)

    "alpha" because it uses alphabet
    "rep" because it's a representative example
    """
    data = io.BytesIO()
    zf = zipfile.ZipFile(data, "w")
    zf.writestr("a.txt", b"content of a")
    zf.writestr("b/c.txt", b"content of c")
    zf.writestr("b/d/e.txt", b"content of e")
    zf.writestr("b/f.txt", b"content of f")
    zf.writestr("g/h/i.txt", b"content of i")
    zf.writestr("j/k.bin", b"content of k")
    zf.writestr("j/l.baz", b"content of l")
    zf.writestr("j/m.bar", b"content of m")
    zf.writestr("n.txt", b"a.txt")
    _make_link(zf.infolist()[-1])

    zf.filename = "alpharep.zip"
    return zf


alpharep_generators = [
    Invoked.wrap(build_alpharep_fixture),
    Invoked.wrap(compose(zipfile._path.CompleteDirs.inject, build_alpharep_fixture)),
]

pass_alpharep = parameterize(['alpharep'], alpharep_generators)


class TestPath(unittest.TestCase):
    def setUp(self):
        self.fixtures = contextlib.ExitStack()
        self.addCleanup(self.fixtures.close)

    def zipfile_ondisk(self, alpharep):
        tmpdir = pathlib.Path(self.fixtures.enter_context(temp_dir()))
        buffer = alpharep.fp
        alpharep.close()
        path = tmpdir / alpharep.filename
        with path.open("wb") as strm:
            strm.write(buffer.getvalue())
        return path

    @pass_alpharep
    def test_iterdir_and_types(self, alpharep):
        root = zipfile.Path(alpharep)
        assert root.is_dir()
        a, n, b, g, j = root.iterdir()
        assert a.is_file()
        assert b.is_dir()
        assert g.is_dir()
        c, f, d = b.iterdir()
        assert c.is_file() and f.is_file()
        (e,) = d.iterdir()
        assert e.is_file()
        (h,) = g.iterdir()
        (i,) = h.iterdir()
        assert i.is_file()

    @pass_alpharep
    def test_is_file_missing(self, alpharep):
        root = zipfile.Path(alpharep)
        assert not root.joinpath('missing.txt').is_file()

    @pass_alpharep
    def test_iterdir_on_file(self, alpharep):
        root = zipfile.Path(alpharep)
        a, n, b, g, j = root.iterdir()
        with self.assertRaises(ValueError):
            a.iterdir()

    @pass_alpharep
    def test_subdir_is_dir(self, alpharep):
        root = zipfile.Path(alpharep)
        assert (root / 'b').is_dir()
        assert (root / 'b/').is_dir()
        assert (root / 'g').is_dir()
        assert (root / 'g/').is_dir()

    @pass_alpharep
    def test_open(self, alpharep):
        root = zipfile.Path(alpharep)
        a, n, b, g, j = root.iterdir()
        with a.open(encoding="utf-8") as strm:
            data = strm.read()
        self.assertEqual(data, "content of a")
        with a.open('r', "utf-8") as strm:  # not a kw, no gh-101144 TypeError
            data = strm.read()
        self.assertEqual(data, "content of a")

    def test_open_encoding_utf16(self):
        in_memory_file = io.BytesIO()
        zf = zipfile.ZipFile(in_memory_file, "w")
        zf.writestr("path/16.txt", "This was utf-16".encode("utf-16"))
        zf.filename = "test_open_utf16.zip"
        root = zipfile.Path(zf)
        (path,) = root.iterdir()
        u16 = path.joinpath("16.txt")
        with u16.open('r', "utf-16") as strm:
            data = strm.read()
        assert data == "This was utf-16"
        with u16.open(encoding="utf-16") as strm:
            data = strm.read()
        assert data == "This was utf-16"

    def test_open_encoding_errors(self):
        in_memory_file = io.BytesIO()
        zf = zipfile.ZipFile(in_memory_file, "w")
        zf.writestr("path/bad-utf8.bin", b"invalid utf-8: \xff\xff.")
        zf.filename = "test_read_text_encoding_errors.zip"
        root = zipfile.Path(zf)
        (path,) = root.iterdir()
        u16 = path.joinpath("bad-utf8.bin")

        # encoding= as a positional argument for gh-101144.
        data = u16.read_text("utf-8", errors="ignore")
        assert data == "invalid utf-8: ."
        with u16.open("r", "utf-8", errors="surrogateescape") as f:
            assert f.read() == "invalid utf-8: \udcff\udcff."

        # encoding= both positional and keyword is an error; gh-101144.
        with self.assertRaisesRegex(TypeError, "encoding"):
            data = u16.read_text("utf-8", encoding="utf-8")

        # both keyword arguments work.
        with u16.open("r", encoding="utf-8", errors="strict") as f:
            # error during decoding with wrong codec.
            with self.assertRaises(UnicodeDecodeError):
                f.read()

    @unittest.skipIf(
        not getattr(sys.flags, 'warn_default_encoding', 0),
        "Requires warn_default_encoding",
    )
    @pass_alpharep
    def test_encoding_warnings(self, alpharep):
        """EncodingWarning must blame the read_text and open calls."""
        assert sys.flags.warn_default_encoding
        root = zipfile.Path(alpharep)
        with self.assertWarns(EncodingWarning) as wc:
            root.joinpath("a.txt").read_text()
        assert __file__ == wc.filename
        with self.assertWarns(EncodingWarning) as wc:
            root.joinpath("a.txt").open("r").close()
        assert __file__ == wc.filename

    def test_open_write(self):
        """
        If the zipfile is open for write, it should be possible to
        write bytes or text to it.
        """
        zf = zipfile.Path(zipfile.ZipFile(io.BytesIO(), mode='w'))
        with zf.joinpath('file.bin').open('wb') as strm:
            strm.write(b'binary contents')
        with zf.joinpath('file.txt').open('w', encoding="utf-8") as strm:
            strm.write('text file')

    @pass_alpharep
    def test_open_extant_directory(self, alpharep):
        """
        Attempting to open a directory raises IsADirectoryError.
        """
        zf = zipfile.Path(alpharep)
        with self.assertRaises(IsADirectoryError):
            zf.joinpath('b').open()

    @pass_alpharep
    def test_open_binary_invalid_args(self, alpharep):
        root = zipfile.Path(alpharep)
        with self.assertRaises(ValueError):
            root.joinpath('a.txt').open('rb', encoding='utf-8')
        with self.assertRaises(ValueError):
            root.joinpath('a.txt').open('rb', 'utf-8')

    @pass_alpharep
    def test_open_missing_directory(self, alpharep):
        """
        Attempting to open a missing directory raises FileNotFoundError.
        """
        zf = zipfile.Path(alpharep)
        with self.assertRaises(FileNotFoundError):
            zf.joinpath('z').open()

    @pass_alpharep
    def test_read(self, alpharep):
        root = zipfile.Path(alpharep)
        a, n, b, g, j = root.iterdir()
        assert a.read_text(encoding="utf-8") == "content of a"
        # Also check positional encoding arg (gh-101144).
        assert a.read_text("utf-8") == "content of a"
        assert a.read_bytes() == b"content of a"

    @pass_alpharep
    def test_joinpath(self, alpharep):
        root = zipfile.Path(alpharep)
        a = root.joinpath("a.txt")
        assert a.is_file()
        e = root.joinpath("b").joinpath("d").joinpath("e.txt")
        assert e.read_text(encoding="utf-8") == "content of e"

    @pass_alpharep
    def test_joinpath_multiple(self, alpharep):
        root = zipfile.Path(alpharep)
        e = root.joinpath("b", "d", "e.txt")
        assert e.read_text(encoding="utf-8") == "content of e"

    @pass_alpharep
    def test_traverse_truediv(self, alpharep):
        root = zipfile.Path(alpharep)
        a = root / "a.txt"
        assert a.is_file()
        e = root / "b" / "d" / "e.txt"
        assert e.read_text(encoding="utf-8") == "content of e"

    @pass_alpharep
    def test_pathlike_construction(self, alpharep):
        """
        zipfile.Path should be constructable from a path-like object
        """
        zipfile_ondisk = self.zipfile_ondisk(alpharep)
        pathlike = FakePath(str(zipfile_ondisk))
        zipfile.Path(pathlike)

    @pass_alpharep
    def test_traverse_pathlike(self, alpharep):
        root = zipfile.Path(alpharep)
        root / FakePath("a")

    @pass_alpharep
    def test_parent(self, alpharep):
        root = zipfile.Path(alpharep)
        assert (root / 'a').parent.at == ''
        assert (root / 'a' / 'b').parent.at == 'a/'

    @pass_alpharep
    def test_dir_parent(self, alpharep):
        root = zipfile.Path(alpharep)
        assert (root / 'b').parent.at == ''
        assert (root / 'b/').parent.at == ''

    @pass_alpharep
    def test_missing_dir_parent(self, alpharep):
        root = zipfile.Path(alpharep)
        assert (root / 'missing dir/').parent.at == ''

    @pass_alpharep
    def test_mutability(self, alpharep):
        """
        If the underlying zipfile is changed, the Path object should
        reflect that change.
        """
        root = zipfile.Path(alpharep)
        a, n, b, g, j = root.iterdir()
        alpharep.writestr('foo.txt', 'foo')
        alpharep.writestr('bar/baz.txt', 'baz')
        assert any(child.name == 'foo.txt' for child in root.iterdir())
        assert (root / 'foo.txt').read_text(encoding="utf-8") == 'foo'
        (baz,) = (root / 'bar').iterdir()
        assert baz.read_text(encoding="utf-8") == 'baz'

    HUGE_ZIPFILE_NUM_ENTRIES = 2**13

    def huge_zipfile(self):
        """Create a read-only zipfile with a huge number of entries entries."""
        strm = io.BytesIO()
        zf = zipfile.ZipFile(strm, "w")
        for entry in map(str, range(self.HUGE_ZIPFILE_NUM_ENTRIES)):
            zf.writestr(entry, entry)
        zf.mode = 'r'
        return zf

    def test_joinpath_constant_time(self):
        """
        Ensure joinpath on items in zipfile is linear time.
        """
        root = zipfile.Path(self.huge_zipfile())
        entries = jaraco.itertools.Counter(root.iterdir())
        for entry in entries:
            entry.joinpath('suffix')
        # Check the file iterated all items
        assert entries.count == self.HUGE_ZIPFILE_NUM_ENTRIES

    @pass_alpharep
    def test_read_does_not_close(self, alpharep):
        alpharep = self.zipfile_ondisk(alpharep)
        with zipfile.ZipFile(alpharep) as file:
            for rep in range(2):
                zipfile.Path(file, 'a.txt').read_text(encoding="utf-8")

    @pass_alpharep
    def test_subclass(self, alpharep):
        class Subclass(zipfile.Path):
            pass

        root = Subclass(alpharep)
        assert isinstance(root / 'b', Subclass)

    @pass_alpharep
    def test_filename(self, alpharep):
        root = zipfile.Path(alpharep)
        assert root.filename == pathlib.Path('alpharep.zip')

    @pass_alpharep
    def test_root_name(self, alpharep):
        """
        The name of the root should be the name of the zipfile
        """
        root = zipfile.Path(alpharep)
        assert root.name == 'alpharep.zip' == root.filename.name

    @pass_alpharep
    def test_suffix(self, alpharep):
        """
        The suffix of the root should be the suffix of the zipfile.
        The suffix of each nested file is the final component's last suffix, if any.
        Includes the leading period, just like pathlib.Path.
        """
        root = zipfile.Path(alpharep)
        assert root.suffix == '.zip' == root.filename.suffix

        b = root / "b.txt"
        assert b.suffix == ".txt"

        c = root / "c" / "filename.tar.gz"
        assert c.suffix == ".gz"

        d = root / "d"
        assert d.suffix == ""

    @pass_alpharep
    def test_suffixes(self, alpharep):
        """
        The suffix of the root should be the suffix of the zipfile.
        The suffix of each nested file is the final component's last suffix, if any.
        Includes the leading period, just like pathlib.Path.
        """
        root = zipfile.Path(alpharep)
        assert root.suffixes == ['.zip'] == root.filename.suffixes

        b = root / 'b.txt'
        assert b.suffixes == ['.txt']

        c = root / 'c' / 'filename.tar.gz'
        assert c.suffixes == ['.tar', '.gz']

        d = root / 'd'
        assert d.suffixes == []

        e = root / '.hgrc'
        assert e.suffixes == []

    @pass_alpharep
    def test_suffix_no_filename(self, alpharep):
        alpharep.filename = None
        root = zipfile.Path(alpharep)
        assert root.joinpath('example').suffix == ""
        assert root.joinpath('example').suffixes == []

    @pass_alpharep
    def test_stem(self, alpharep):
        """
        The final path component, without its suffix
        """
        root = zipfile.Path(alpharep)
        assert root.stem == 'alpharep' == root.filename.stem

        b = root / "b.txt"
        assert b.stem == "b"

        c = root / "c" / "filename.tar.gz"
        assert c.stem == "filename.tar"

        d = root / "d"
        assert d.stem == "d"

        assert (root / ".gitignore").stem == ".gitignore"

    @pass_alpharep
    def test_root_parent(self, alpharep):
        root = zipfile.Path(alpharep)
        assert root.parent == pathlib.Path('.')
        root.root.filename = 'foo/bar.zip'
        assert root.parent == pathlib.Path('foo')

    @pass_alpharep
    def test_root_unnamed(self, alpharep):
        """
        It is an error to attempt to get the name
        or parent of an unnamed zipfile.
        """
        alpharep.filename = None
        root = zipfile.Path(alpharep)
        with self.assertRaises(TypeError):
            root.name
        with self.assertRaises(TypeError):
            root.parent

        # .name and .parent should still work on subs
        sub = root / "b"
        assert sub.name == "b"
        assert sub.parent

    @pass_alpharep
    def test_match_and_glob(self, alpharep):
        root = zipfile.Path(alpharep)
        assert not root.match("*.txt")

        assert list(root.glob("b/c.*")) == [zipfile.Path(alpharep, "b/c.txt")]
        assert list(root.glob("b/*.txt")) == [
            zipfile.Path(alpharep, "b/c.txt"),
            zipfile.Path(alpharep, "b/f.txt"),
        ]

    @pass_alpharep
    def test_glob_recursive(self, alpharep):
        root = zipfile.Path(alpharep)
        files = root.glob("**/*.txt")
        assert all(each.match("*.txt") for each in files)

        assert list(root.glob("**/*.txt")) == list(root.rglob("*.txt"))

    @pass_alpharep
    def test_glob_dirs(self, alpharep):
        root = zipfile.Path(alpharep)
        assert list(root.glob('b')) == [zipfile.Path(alpharep, "b/")]
        assert list(root.glob('b*')) == [zipfile.Path(alpharep, "b/")]

    @pass_alpharep
    def test_glob_subdir(self, alpharep):
        root = zipfile.Path(alpharep)
        assert list(root.glob('g/h')) == [zipfile.Path(alpharep, "g/h/")]
        assert list(root.glob('g*/h*')) == [zipfile.Path(alpharep, "g/h/")]

    @pass_alpharep
    def test_glob_subdirs(self, alpharep):
        root = zipfile.Path(alpharep)

        assert list(root.glob("*/i.txt")) == []
        assert list(root.rglob("*/i.txt")) == [zipfile.Path(alpharep, "g/h/i.txt")]

    @pass_alpharep
    def test_glob_does_not_overmatch_dot(self, alpharep):
        root = zipfile.Path(alpharep)

        assert list(root.glob("*.xt")) == []

    @pass_alpharep
    def test_glob_single_char(self, alpharep):
        root = zipfile.Path(alpharep)

        assert list(root.glob("a?txt")) == [zipfile.Path(alpharep, "a.txt")]
        assert list(root.glob("a[.]txt")) == [zipfile.Path(alpharep, "a.txt")]
        assert list(root.glob("a[?]txt")) == []

    @pass_alpharep
    def test_glob_chars(self, alpharep):
        root = zipfile.Path(alpharep)

        assert list(root.glob("j/?.b[ai][nz]")) == [
            zipfile.Path(alpharep, "j/k.bin"),
            zipfile.Path(alpharep, "j/l.baz"),
        ]

    def test_glob_empty(self):
        root = zipfile.Path(zipfile.ZipFile(io.BytesIO(), 'w'))
        with self.assertRaises(ValueError):
            root.glob('')

    @pass_alpharep
    def test_eq_hash(self, alpharep):
        root = zipfile.Path(alpharep)
        assert root == zipfile.Path(alpharep)

        assert root != (root / "a.txt")
        assert (root / "a.txt") == (root / "a.txt")

        root = zipfile.Path(alpharep)
        assert root in {root}

    @pass_alpharep
    def test_is_symlink(self, alpharep):
        root = zipfile.Path(alpharep)
        assert not root.joinpath('a.txt').is_symlink()
        assert root.joinpath('n.txt').is_symlink()

    @pass_alpharep
    def test_relative_to(self, alpharep):
        root = zipfile.Path(alpharep)
        relative = root.joinpath("b", "c.txt").relative_to(root / "b")
        assert str(relative) == "c.txt"

        relative = root.joinpath("b", "d", "e.txt").relative_to(root / "b")
        assert str(relative) == "d/e.txt"

    @pass_alpharep
    def test_inheritance(self, alpharep):
        cls = type('PathChild', (zipfile.Path,), {})
        file = cls(alpharep).joinpath('some dir').parent
        assert isinstance(file, cls)

    @parameterize(
        ['alpharep', 'path_type', 'subpath'],
        itertools.product(
            alpharep_generators,
            [str, FakePath],
            ['', 'b/'],
        ),
    )
    def test_pickle(self, alpharep, path_type, subpath):
        zipfile_ondisk = path_type(str(self.zipfile_ondisk(alpharep)))

        saved_1 = pickle.dumps(zipfile.Path(zipfile_ondisk, at=subpath))
        restored_1 = pickle.loads(saved_1)
        first, *rest = restored_1.iterdir()
        assert first.read_text(encoding='utf-8').startswith('content of ')

    @pass_alpharep
    def test_extract_orig_with_implied_dirs(self, alpharep):
        """
        A zip file wrapped in a Path should extract even with implied dirs.
        """
        source_path = self.zipfile_ondisk(alpharep)
        zf = zipfile.ZipFile(source_path)
        # wrap the zipfile for its side effect
        zipfile.Path(zf)
        zf.extractall(source_path.parent)

    @pass_alpharep
    def test_getinfo_missing(self, alpharep):
        """
        Validate behavior of getinfo on original zipfile after wrapping.
        """
        zipfile.Path(alpharep)
        with self.assertRaises(KeyError):
            alpharep.getinfo('does-not-exist')

    def test_malformed_paths(self):
        """
        Path should handle malformed paths gracefully.

        Paths with leading slashes are not visible.

        Paths with dots are treated like regular files.
        """
        data = io.BytesIO()
        zf = zipfile.ZipFile(data, "w")
        zf.writestr("/one-slash.txt", b"content")
        zf.writestr("//two-slash.txt", b"content")
        zf.writestr("../parent.txt", b"content")
        zf.filename = ''
        root = zipfile.Path(zf)
        assert list(map(str, root.iterdir())) == ['../']
        assert root.joinpath('..').joinpath('parent.txt').read_bytes() == b'content'

    def test_unsupported_names(self):
        """
        Path segments with special characters are readable.

        On some platforms or file systems, characters like
        ``:`` and ``?`` are not allowed, but they are valid
        in the zip file.
        """
        data = io.BytesIO()
        zf = zipfile.ZipFile(data, "w")
        zf.writestr("path?", b"content")
        zf.writestr("V: NMS.flac", b"fLaC...")
        zf.filename = ''
        root = zipfile.Path(zf)
        contents = root.iterdir()
        assert next(contents).name == 'path?'
        assert next(contents).name == 'V: NMS.flac'
        assert root.joinpath('V: NMS.flac').read_bytes() == b"fLaC..."

    def test_backslash_not_separator(self):
        """
        In a zip file, backslashes are not separators.
        """
        data = io.BytesIO()
        zf = zipfile.ZipFile(data, "w")
        zf.writestr(DirtyZipInfo.for_name("foo\\bar", zf), b"content")
        zf.filename = ''
        root = zipfile.Path(zf)
        (first,) = root.iterdir()
        assert not first.is_dir()
        assert first.name == 'foo\\bar'

    @pass_alpharep
    def test_interface(self, alpharep):
        from importlib.resources.abc import Traversable

        zf = zipfile.Path(alpharep)
        assert isinstance(zf, Traversable)


class DirtyZipInfo(zipfile.ZipInfo):
    """
    Bypass name sanitization.
    """

    def __init__(self, filename, *args, **kwargs):
        super().__init__(filename, *args, **kwargs)
        self.filename = filename

    @classmethod
    def for_name(cls, name, archive):
        """
        Construct the same way that ZipFile.writestr does.

        TODO: extract this functionality and re-use
        """
        self = cls(filename=name, date_time=time.localtime(time.time())[:6])
        self.compress_type = archive.compression
        self.compress_level = archive.compresslevel
        if self.filename.endswith('/'):  # pragma: no cover
            self.external_attr = 0o40775 << 16  # drwxrwxr-x
            self.external_attr |= 0x10  # MS-DOS directory flag
        else:
            self.external_attr = 0o600 << 16  # ?rw-------
        return self
