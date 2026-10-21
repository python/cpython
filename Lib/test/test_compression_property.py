"""Property-based tests for the incremental compression APIs.

The compression modules wrap external libraries, but the incremental
``decompress(data, max_length)`` protocol (``eof``, ``needs_input``,
``unused_data``), the ``zlib.decompressobj`` variant, and the file layer built
on ``compression._common._streams.DecompressReader`` are implemented in
CPython and share one contract across bz2, gzip, lzma, zlib and zstd.  The
tests here exercise that contract with random data, random chunking and
random ``max_length`` values, using the module-level one-shot functions as
oracles.

Each API is covered twice.  A ``RuleBasedStateMachine`` declares the calls
a user may make (feed a chunk, seek, read a line, ...) with their argument
ranges and what must hold after each one; Hypothesis then generates and
shrinks whole call sequences against it.  State machines have no
``@example`` equivalent, so each one is paired with a ``@given`` companion
test that expresses one call sequence as data (chunk sizes, ``max_length``
values, read sizes).  The companions carry the pinned examples, including
the shrunk reproducers for the known bugs, and are what the stub replays
when Hypothesis is not installed; the machines are skipped then.
"""
import dataclasses
import functools
import io
import itertools
import unittest
from collections.abc import Callable

from test import support
from test.support.hypothesis_helper import hypothesis

try:
    import bz2
except ImportError:
    bz2 = None
try:
    import gzip
except ImportError:
    gzip = None
try:
    import lzma
except ImportError:
    lzma = None
try:
    import zlib
except ImportError:
    zlib = None
try:
    from compression import zstd
except ImportError:
    zstd = None

st = hypothesis.strategies
stateful = hypothesis.stateful


def _repeat(pair):
    chunk, count = pair
    return chunk * count


# Small inputs keep each example cheap; the repeat count makes some of the
# data compressible so that not every stream is a stored block.
data_strategy = st.tuples(st.binary(max_size=512),
                          st.integers(1, 4)).map(_repeat)
trailer_strategy = st.binary(max_size=16)
chunk_sizes_strategy = st.lists(st.integers(0, 64), max_size=16)
max_length_strategy = st.integers(-1, 64)
max_lengths_strategy = st.lists(max_length_strategy, max_size=16)


def _chunks(data, sizes):
    """Split *data* at the given sizes; whatever is left is the last chunk.

    Unlike ``itertools.batched`` the pieces have different sizes, so the
    boundaries between calls land at arbitrary positions in the stream.
    """
    pos = 0
    for size in sizes:
        yield data[pos:pos + size]
        pos += size
    yield data[pos:]


def _chunks_with(data, sizes, values, default):
    """Pair each chunk with the next of *values*, cycling through them."""
    return zip(_chunks(data, sizes), itertools.cycle(values or [default]))


@dataclasses.dataclass(frozen=True)
class _Backend:
    name: str
    new_decompressor: Callable[[], object]
    new_compressor: Callable[[], object]
    compress: Callable[[bytes], bytes]
    decompress: Callable[[bytes], bytes]
    # Raised by the incremental decompressor on invalid input.
    error: type[Exception]
    # Everything the module-level decompress may raise on invalid input.
    decompress_errors: tuple[type[Exception], ...]
    # Opens a binary file object for reading, if the module has one.
    open_file: Callable[[io.BytesIO], io.BufferedIOBase] | None = None


# Each factory returns None when its module is missing, so that the test
# classes below can be defined unconditionally and skipped by requires_*().

def _bz2_backend():
    if bz2 is None:
        return None
    return _Backend(
        name="bz2",
        new_decompressor=bz2.BZ2Decompressor,
        new_compressor=functools.partial(bz2.BZ2Compressor, 1),
        compress=functools.partial(bz2.compress, compresslevel=1),
        decompress=bz2.decompress,
        error=OSError,
        decompress_errors=(OSError, ValueError),
        open_file=bz2.BZ2File,
    )


def _open_gzip(fp):
    return gzip.GzipFile(fileobj=fp)


def _gzip_backend():
    if gzip is None or zlib is None:
        return None
    return _Backend(
        name="gzip",
        new_decompressor=functools.partial(zlib._ZlibDecompressor, 31),
        new_compressor=functools.partial(zlib.compressobj, 1, wbits=31),
        compress=functools.partial(gzip.compress, compresslevel=1),
        decompress=gzip.decompress,
        error=zlib.error,
        decompress_errors=(gzip.BadGzipFile, EOFError, zlib.error),
        open_file=_open_gzip,
    )


def _lzma_backend():
    if lzma is None:
        return None
    return _Backend(
        name="lzma",
        new_decompressor=lzma.LZMADecompressor,
        new_compressor=functools.partial(lzma.LZMACompressor, preset=0),
        compress=functools.partial(lzma.compress, preset=0),
        decompress=lzma.decompress,
        error=lzma.LZMAError,
        decompress_errors=(lzma.LZMAError,),
        open_file=lzma.LZMAFile,
    )


def _zlib_backend(wbits):
    if zlib is None:
        return None
    return _Backend(
        name=f"zlib(wbits={wbits})",
        new_decompressor=functools.partial(zlib._ZlibDecompressor, wbits),
        new_compressor=functools.partial(zlib.compressobj, 1, wbits=wbits),
        compress=functools.partial(zlib.compress, level=1, wbits=wbits),
        decompress=functools.partial(zlib.decompress, wbits=wbits),
        error=zlib.error,
        decompress_errors=(zlib.error,),
    )


def _zstd_backend():
    if zstd is None:
        return None
    return _Backend(
        name="zstd",
        new_decompressor=zstd.ZstdDecompressor,
        new_compressor=functools.partial(zstd.ZstdCompressor, 1),
        compress=functools.partial(zstd.compress, level=1),
        decompress=zstd.decompress,
        error=zstd.ZstdError,
        decompress_errors=(zstd.ZstdError,),
        open_file=zstd.ZstdFile,
    )


class _StateMachine(stateful.RuleBasedStateMachine):
    """A state machine that checks with the unittest assertion methods.

    The methods are borrowed from a throwaway ``TestCase`` so that failures
    inside rules get the same diagnostics as the rest of the test suite.
    """

    def __init__(self):
        super().__init__()
        self._case = unittest.TestCase()

    # Hypothesis runs a machine from the ``runTest`` of the ``TestCase`` it
    # generates, creating a fresh instance with ``Machine()`` for every
    # example.  The machine is not that test case and has none of its
    # assertion state, so the methods are forwarded to a throwaway one.
    def __getattr__(self, name):
        if name.startswith("assert"):
            return getattr(self._case, name)
        raise AttributeError(name)


def _machine_test(name, machine_base, backend_factory, requires):
    """Build the unittest TestCase that runs *machine_base* on one backend.

    The machine subclass only adds the backend, and ``Machine.TestCase`` is
    generated by Hypothesis anyway, so neither needs a class statement.
    """
    machine = type(name, (machine_base,), {"backend": backend_factory()})
    return requires(machine.TestCase)


# ---------------------------------------------------------------------------
# Incremental decompressor protocol
# ---------------------------------------------------------------------------

class _DecompressorMachine(_StateMachine):
    """Feed one compressed stream through a decompressor in random pieces.

    After every call the output must be a prefix of the original data and no
    longer than ``max_length``; ``eof`` is set exactly when all of the data
    is out, at which point ``unused_data`` holds whatever followed the
    stream; and any call after ``eof`` raises ``EOFError``.

    Subclasses set ``backend``.
    """

    backend: _Backend

    @stateful.initialize(data=data_strategy, trailer=trailer_strategy)
    def start(self, data, trailer):
        self.data = data
        self.trailer = trailer
        self.stream = self.backend.compress(data) + trailer
        self.pos = 0
        self.out = b""
        self.decomp = self.backend.new_decompressor()

    def _feed(self, chunk, max_length):
        if self.decomp.eof:
            with self.assertRaises(EOFError):
                self.decomp.decompress(chunk, max_length)
            return
        ret = self.decomp.decompress(chunk, max_length)
        self.assertIsInstance(ret, bytes)
        if max_length >= 0:
            self.assertLessEqual(len(ret), max_length)
        self.out += ret
        self.assertStartsWith(self.data, self.out)
        if self.decomp.eof:
            self.assertEqual(self.out, self.data)
            rest = self.stream[self.pos:]
            self.assertEqual(self.decomp.unused_data + rest, self.trailer)
        else:
            self.assertEqual(self.decomp.unused_data, b"")

    def _feed_next(self, n, max_length):
        chunk = self.stream[self.pos:self.pos + n]
        self.pos += len(chunk)
        self._feed(chunk, max_length)

    @stateful.rule(n=st.integers(0, 64), max_length=max_length_strategy)
    def feed(self, n, max_length):
        self._feed_next(n, max_length)

    @stateful.rule()
    def finish(self):
        if self.decomp.eof:
            self._feed(b"", -1)
            return
        self._feed_next(len(self.stream), -1)
        # The input is exhausted; unlimited calls must drain any output that
        # is still buffered and reach the end of the stream.
        for _ in range(3):
            if self.decomp.eof:
                break
            self._feed(b"", -1)
        self.assertTrue(self.decomp.eof)
        self.assertEqual(self.out, self.data)

    @stateful.invariant()
    def output_is_a_prefix(self):
        self.assertStartsWith(self.data, self.out)
        self.assertIsInstance(self.decomp.needs_input, bool)
        if self.decomp.eof:
            self.assertEqual(self.out, self.data)


class _DecompressorTests:
    """Deterministic companions to the state machine, replayable by the stub.

    Subclasses set ``backend``.
    """

    backend: _Backend

    def _walk(self, data, sizes, max_lengths, trailer=b""):
        """Feed the stream in pieces, yielding the decompressor after each.

        ``max_lengths`` is cycled over the chunks; an empty list means every
        call is unlimited.  Once the pieces run out the stream is drained
        and the final state checked.
        """
        stream = self.backend.compress(data) + trailer
        decomp = self.backend.new_decompressor()
        out = b""
        fed = 0
        for chunk, max_length in _chunks_with(stream, sizes, max_lengths, -1):
            if decomp.eof:
                with self.assertRaises(EOFError):
                    decomp.decompress(chunk, max_length)
                break
            ret = decomp.decompress(chunk, max_length)
            fed += len(chunk)
            if max_length >= 0:
                self.assertLessEqual(len(ret), max_length)
            out += ret
            self.assertStartsWith(data, out)
            yield decomp
        while not decomp.eof:
            out += decomp.decompress(b"", -1)
        self.assertEqual(out, data)
        self.assertEqual(decomp.unused_data + stream[fed:], trailer)

    def _check_chunked_decompress(self, data, trailer, sizes, max_lengths):
        for _ in self._walk(data, sizes, max_lengths, trailer):
            pass

    @hypothesis.given(
        data=data_strategy,
        trailer=trailer_strategy,
        sizes=chunk_sizes_strategy,
        max_lengths=max_lengths_strategy,
    )
    @hypothesis.example(data=b"", trailer=b"", sizes=[], max_lengths=[])
    @hypothesis.example(data=b"", trailer=b"tail", sizes=[1, 1, 1],
                        max_lengths=[0])
    @hypothesis.example(data=b"ab" * 40, trailer=b"", sizes=[5] * 12,
                        max_lengths=[1, 0, 3])
    @hypothesis.example(data=bytes(range(256)), trailer=b"x",
                        sizes=[0, 7, 0, 300], max_lengths=[2, -1])
    def test_chunked_decompress(self, data, trailer, sizes, max_lengths):
        self._check_chunked_decompress(data, trailer, sizes, max_lengths)

    def _check_needs_input_means_drained(self, data, sizes, max_lengths):
        for decomp in self._walk(data, sizes, max_lengths):
            if decomp.needs_input and not decomp.eof:
                # A decompressor that asks for input has no output to give.
                self.assertEqual(decomp.decompress(b"", -1), b"")

    @hypothesis.given(
        data=data_strategy,
        sizes=chunk_sizes_strategy,
        max_lengths=max_lengths_strategy,
    )
    @hypothesis.example(data=b"", sizes=[], max_lengths=[])
    @hypothesis.example(data=b"abc" * 50, sizes=[10] * 10,
                        max_lengths=[1, 5, 0])
    def test_needs_input_means_drained(self, data, sizes, max_lengths):
        self._check_needs_input_means_drained(data, sizes, max_lengths)

    def _check_repeated_max_length_zero(self, data, count):
        # Asking for no output, repeatedly, must not break the decompressor.
        decomp = self.backend.new_decompressor()
        for _ in range(count):
            self.assertEqual(decomp.decompress(b"", 0), b"")
            self.assertFalse(decomp.eof)
        stream = self.backend.compress(data)
        self.assertEqual(decomp.decompress(stream, -1), data)
        self.assertTrue(decomp.eof)

    @hypothesis.given(data=data_strategy, count=st.integers(1, 4))
    @hypothesis.example(data=b"", count=1)
    @hypothesis.example(data=b"", count=2)
    @hypothesis.example(data=b"spam" * 10, count=3)
    def test_repeated_max_length_zero(self, data, count):
        self._check_repeated_max_length_zero(data, count)

    def _check_max_length_zero_with_pending_output(self, data, count):
        # The same, but with a whole stream buffered and its output pending.
        decomp = self.backend.new_decompressor()
        self.assertEqual(decomp.decompress(self.backend.compress(data), 0),
                         b"")
        for _ in range(count):
            if decomp.eof:
                break
            self.assertEqual(decomp.decompress(b"", 0), b"")
        if not decomp.eof:
            self.assertEqual(decomp.decompress(b"", -1), data)
        self.assertTrue(decomp.eof)

    @hypothesis.given(data=data_strategy, count=st.integers(1, 20))
    @hypothesis.example(data=b"", count=1)
    @hypothesis.example(data=b"\x00", count=16)
    @hypothesis.example(data=b"spam" * 10, count=20)
    def test_max_length_zero_with_pending_output(self, data, count):
        self._check_max_length_zero_with_pending_output(data, count)

    @hypothesis.given(data=data_strategy, cut=st.integers(0, 10**6))
    @hypothesis.example(data=b"", cut=0)
    @hypothesis.example(data=b"", cut=10**6)
    @hypothesis.example(data=b"hello" * 20, cut=3)
    def test_truncated_stream_does_not_raise(self, data, cut):
        stream = self.backend.compress(data)
        cut %= len(stream) + 1
        decomp = self.backend.new_decompressor()
        out = decomp.decompress(stream[:cut], -1)
        self.assertStartsWith(data, out)
        self.assertEqual(decomp.eof, cut == len(stream))
        if not decomp.eof:
            self.assertTrue(decomp.needs_input)
            self.assertEqual(decomp.unused_data, b"")

    @hypothesis.given(data=data_strategy, sizes=chunk_sizes_strategy)
    @hypothesis.example(data=b"", sizes=[])
    @hypothesis.example(data=b"", sizes=[0, 0])
    @hypothesis.example(data=b"xyzzy" * 30, sizes=[1, 0, 2, 100])
    def test_chunked_compress_round_trip(self, data, sizes):
        comp = self.backend.new_compressor()
        stream = b"".join(comp.compress(chunk)
                          for chunk in _chunks(data, sizes))
        stream += comp.flush()
        self.assertEqual(self.backend.decompress(stream), data)
        decomp = self.backend.new_decompressor()
        self.assertEqual(decomp.decompress(stream), data)
        self.assertTrue(decomp.eof)
        self.assertEqual(decomp.unused_data, b"")

    # Invalid input: random bytes, or a valid stream with one byte replaced
    # or with its end cut off.  The only acceptable outcomes are success or
    # the backend's own error type.
    @hypothesis.given(
        garbage=st.binary(max_size=256),
        data=data_strategy,
        index=st.integers(0, 10**6),
        byte=st.integers(0, 255),
        sizes=chunk_sizes_strategy,
        max_lengths=max_lengths_strategy,
    )
    @hypothesis.example(garbage=b"", data=b"", index=0, byte=0, sizes=[],
                        max_lengths=[])
    @hypothesis.example(garbage=b"\xff" * 20, data=b"", index=0, byte=0,
                        sizes=[1], max_lengths=[0])
    @hypothesis.example(garbage=b"", data=b"abc" * 30, index=10**6, byte=0,
                        sizes=[3], max_lengths=[1, -1])
    @hypothesis.example(garbage=b"", data=b"abc" * 30, index=5, byte=255,
                        sizes=[], max_lengths=[])
    def test_invalid_input(self, garbage, data, index, byte, sizes,
                           max_lengths):
        self._check_invalid_input(garbage, data, index, byte, sizes,
                                  max_lengths)

    def _skip_invalid_blob(self, blob):
        # Hook for a known bug: a mixin may rule out inputs that crash.
        return False

    def _check_invalid_input(self, garbage, data, index, byte, sizes,
                             max_lengths):
        stream = self.backend.compress(data)
        index %= len(stream) + 1
        if index == len(stream):
            corrupted = stream[:byte % (len(stream) + 1)]
        else:
            corrupted = stream[:index] + bytes([byte]) + stream[index + 1:]
        for blob in (garbage, corrupted):
            if self._skip_invalid_blob(blob):
                continue
            with self.subTest(blob=blob):
                decomp = self.backend.new_decompressor()
                try:
                    for chunk, max_length in _chunks_with(blob, sizes,
                                                          max_lengths, -1):
                        decomp.decompress(chunk, max_length)
                except (self.backend.error, EOFError):
                    pass
                try:
                    self.backend.decompress(blob)
                except self.backend.decompress_errors:
                    pass


# Known bugs, as mixins that override the affected test for one backend.
# They go first in the bases so that they win over _DecompressorTests.

class _BZ2NeedsInputBug:
    # bz2 reports needs_input=True as soon as its input buffer is empty, even
    # when max_length cut the output short and more of it is pending
    # (gh-159999).
    @unittest.expectedFailure
    @hypothesis.given(
        data=data_strategy,
        sizes=chunk_sizes_strategy,
        max_lengths=max_lengths_strategy,
    )
    @hypothesis.example(data=b"\x00\x10 0@" * 2, sizes=[40], max_lengths=[0])
    def test_needs_input_means_drained(self, data, sizes, max_lengths):
        self._check_needs_input_means_drained(data, sizes, max_lengths)


class _ZlibNeedsInputBug:
    # zlib._ZlibDecompressor has the same needs_input problem as bz2
    # (gh-159999).  The examples depend on where the compressor puts block
    # boundaries, so there are several: at least one must fail for each
    # wbits variant (zlib wrapper, raw deflate, gzip wrapper) with both zlib
    # and zlib-ng.
    @unittest.expectedFailure
    @hypothesis.given(
        data=data_strategy,
        sizes=chunk_sizes_strategy,
        max_lengths=max_lengths_strategy,
    )
    @hypothesis.example(data=b"\x00", sizes=[4], max_lengths=[0])
    @hypothesis.example(data=b"\x00" * 21, sizes=[], max_lengths=[2])
    @hypothesis.example(
        data=b"\x00\x00\x00\x01\x00\x00\x02\x00\x00\x03\x00\x00"
             b"\x90\x00\x90\x90\x90\x91\x90" * 2,
        sizes=[34],
        max_lengths=[20],
    )
    @hypothesis.example(
        data=b"\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x03" * 2,
        sizes=[25],
        max_lengths=[12],
    )
    def test_needs_input_means_drained(self, data, sizes, max_lengths):
        self._check_needs_input_means_drained(data, sizes, max_lengths)


def _zlib_fdict_header(blob):
    # A valid zlib header whose FDICT flag is set.
    return (len(blob) >= 2 and blob[0] & 0x0f == 8 and blob[0] >> 4 <= 7
            and int.from_bytes(blob[:2]) % 31 == 0 and bool(blob[1] & 0x20))


class _ZlibNeedsDictBug:
    # zlib._ZlibDecompressor returns NULL without an exception when inflate
    # asks for a preset dictionary (gh-160001).  Any zlib-wrapped input whose
    # header has FDICT set gets there, so random invalid input would hit it,
    # and in a debug build the missing exception is a fatal C assertion.
    # test_invalid_input leaves those inputs out; test_fdict_header pins the
    # bug and can only run in a release build.
    def _skip_invalid_blob(self, blob):
        return _zlib_fdict_header(blob)

    @unittest.skipIf(support.Py_DEBUG, "gh-160001 aborts debug builds")
    @unittest.expectedFailure
    @hypothesis.given(data=st.binary(max_size=64))
    @hypothesis.example(data=b"")
    @hypothesis.example(data=b"\x00\x00\x00\x01\x03\x00")
    def test_fdict_header(self, data):
        # zlib.decompressobj raises zlib.error for the same input.
        stream = b"\x78\xbb" + data
        with self.assertRaises(zlib.error):
            zlib.decompressobj().decompress(stream)
        with self.assertRaises(zlib.error):
            zlib._ZlibDecompressor().decompress(stream)


# liblzma reports an error when two consecutive calls can make no progress,
# which lzma.LZMADecompressor lets through as LZMAError("Insufficient buffer
# space") for two max_length=0 calls in a row with nothing buffered
# (gh-160000), and on the fourth such call when output is pending.  The
# broad lzma tests avoid max_length=0 so that they keep checking everything
# else; test_repeated_max_length_zero and
# test_max_length_zero_with_pending_output pin the bug.
lzma_max_length_strategy = st.integers(1, 64) | st.just(-1)
lzma_max_lengths_strategy = st.lists(lzma_max_length_strategy, max_size=16)


class _LZMAMaxLengthZeroBug:
    @hypothesis.given(
        data=data_strategy,
        trailer=trailer_strategy,
        sizes=chunk_sizes_strategy,
        max_lengths=lzma_max_lengths_strategy,
    )
    @hypothesis.example(data=b"", trailer=b"", sizes=[], max_lengths=[])
    @hypothesis.example(data=b"", trailer=b"tail", sizes=[1, 1, 1],
                        max_lengths=[1])
    @hypothesis.example(data=b"ab" * 40, trailer=b"", sizes=[5] * 12,
                        max_lengths=[1, 2, 3])
    @hypothesis.example(data=bytes(range(256)), trailer=b"x",
                        sizes=[0, 7, 0, 300], max_lengths=[2, -1])
    def test_chunked_decompress(self, data, trailer, sizes, max_lengths):
        self._check_chunked_decompress(data, trailer, sizes, max_lengths)

    @hypothesis.given(
        data=data_strategy,
        sizes=chunk_sizes_strategy,
        max_lengths=lzma_max_lengths_strategy,
    )
    @hypothesis.example(data=b"", sizes=[], max_lengths=[])
    @hypothesis.example(data=b"abc" * 50, sizes=[10] * 10,
                        max_lengths=[1, 5, 2])
    def test_needs_input_means_drained(self, data, sizes, max_lengths):
        self._check_needs_input_means_drained(data, sizes, max_lengths)

    @unittest.expectedFailure
    @hypothesis.given(data=data_strategy, count=st.integers(1, 4))
    @hypothesis.example(data=b"", count=2)
    def test_repeated_max_length_zero(self, data, count):
        self._check_repeated_max_length_zero(data, count)

    @unittest.expectedFailure
    @hypothesis.given(data=data_strategy, count=st.integers(1, 20))
    @hypothesis.example(data=b"\x00", count=16)
    def test_max_length_zero_with_pending_output(self, data, count):
        self._check_max_length_zero_with_pending_output(data, count)


class _ZstdMaxLengthZeroBug:
    # libzstd refuses the 16th consecutive call that makes no progress, and
    # ZstdDecompressor calls it even for max_length=0, so sixteen such calls
    # with output pending raise ZstdError.
    @unittest.expectedFailure
    @hypothesis.given(data=data_strategy, count=st.integers(1, 20))
    @hypothesis.example(data=b"\x00", count=16)
    def test_max_length_zero_with_pending_output(self, data, count):
        self._check_max_length_zero_with_pending_output(data, count)


class LZMADecompressorMachine(_DecompressorMachine):
    # See lzma_max_length_strategy above.
    @stateful.rule(n=st.integers(0, 64), max_length=lzma_max_length_strategy)
    def feed(self, n, max_length):
        self._feed_next(n, max_length)


@support.requires_bz2()
class BZ2DecompressorTests(_BZ2NeedsInputBug, _DecompressorTests,
                           unittest.TestCase):
    backend = _bz2_backend()


@support.requires_gzip()
class GzipDecompressorTests(_ZlibNeedsInputBug, _DecompressorTests,
                            unittest.TestCase):
    backend = _gzip_backend()


@support.requires_lzma()
class LZMADecompressorTests(_LZMAMaxLengthZeroBug, _DecompressorTests,
                            unittest.TestCase):
    backend = _lzma_backend()


@support.requires_zlib()
class ZlibDecompressorTests(_ZlibNeedsInputBug, _ZlibNeedsDictBug,
                            _DecompressorTests, unittest.TestCase):
    backend = _zlib_backend(15)


@support.requires_zlib()
class RawDeflateDecompressorTests(_ZlibNeedsInputBug, _DecompressorTests,
                                  unittest.TestCase):
    backend = _zlib_backend(-15)


@support.requires_zstd()
class ZstdDecompressorTests(_ZstdMaxLengthZeroBug, _DecompressorTests,
                            unittest.TestCase):
    backend = _zstd_backend()


BZ2DecompressorMachineTest = _machine_test(
    "BZ2DecompressorMachine", _DecompressorMachine, _bz2_backend,
    support.requires_bz2())
GzipDecompressorMachineTest = _machine_test(
    "GzipDecompressorMachine", _DecompressorMachine, _gzip_backend,
    support.requires_gzip())
LZMADecompressorMachineTest = _machine_test(
    "LZMADecompressorMachine", LZMADecompressorMachine, _lzma_backend,
    support.requires_lzma())
ZlibDecompressorMachineTest = _machine_test(
    "ZlibDecompressorMachine", _DecompressorMachine,
    functools.partial(_zlib_backend, 15), support.requires_zlib())
RawDeflateDecompressorMachineTest = _machine_test(
    "RawDeflateDecompressorMachine", _DecompressorMachine,
    functools.partial(_zlib_backend, -15), support.requires_zlib())
ZstdDecompressorMachineTest = _machine_test(
    "ZstdDecompressorMachine", _DecompressorMachine, _zstd_backend,
    support.requires_zstd())


# ---------------------------------------------------------------------------
# Compressor flush modes
# ---------------------------------------------------------------------------

@support.requires_zlib()
class ZlibFlushModeTests(unittest.TestCase):
    @hypothesis.given(
        data=data_strategy,
        sizes=chunk_sizes_strategy,
        modes=st.lists(st.sampled_from(["Z_NO_FLUSH", "Z_PARTIAL_FLUSH",
                                        "Z_SYNC_FLUSH", "Z_FULL_FLUSH",
                                        "Z_BLOCK"]), max_size=16),
        level=st.integers(0, 9),
        wbits=st.sampled_from([15, -15, 31]),
    )
    @hypothesis.example(data=b"", sizes=[], modes=[], level=0, wbits=15)
    @hypothesis.example(data=b"", sizes=[0, 0], modes=["Z_SYNC_FLUSH"],
                        level=9, wbits=-15)
    @hypothesis.example(data=b"abc" * 100, sizes=[1, 2, 3],
                        modes=["Z_FULL_FLUSH", "Z_BLOCK", "Z_NO_FLUSH",
                               "Z_PARTIAL_FLUSH"],
                        level=6, wbits=31)
    def test_flush_modes(self, data, sizes, modes, level, wbits):
        # Any sequence of intermediate flushes still produces one valid
        # stream that decompresses to the input.
        comp = zlib.compressobj(level, wbits=wbits)
        stream = b""
        for chunk, mode in _chunks_with(data, sizes, modes, None):
            stream += comp.compress(chunk)
            if mode is not None:
                stream += comp.flush(getattr(zlib, mode))
        stream += comp.flush()
        self.assertEqual(zlib.decompress(stream, wbits=wbits), data)
        decomp = zlib._ZlibDecompressor(wbits)
        self.assertEqual(decomp.decompress(stream), data)
        self.assertTrue(decomp.eof)
        self.assertEqual(decomp.unused_data, b"")


@support.requires_zstd()
class ZstdCompressModeTests(unittest.TestCase):
    @hypothesis.given(
        data=data_strategy,
        sizes=chunk_sizes_strategy,
        modes=st.lists(st.sampled_from(["CONTINUE", "FLUSH_BLOCK",
                                        "FLUSH_FRAME"]), max_size=16),
    )
    @hypothesis.example(data=b"", sizes=[], modes=[])
    @hypothesis.example(data=b"", sizes=[0, 0], modes=["FLUSH_FRAME"])
    @hypothesis.example(data=b"abc" * 100, sizes=[1, 2, 3, 50],
                        modes=["FLUSH_BLOCK", "CONTINUE", "FLUSH_FRAME",
                               "FLUSH_BLOCK"])
    def test_compress_modes(self, data, sizes, modes):
        # FLUSH_FRAME ends a frame and the next call starts a new one, so the
        # output is a sequence of frames.  The module-level decompress() must
        # handle all of them and the incremental decompressor must stop at
        # each frame boundary, leaving the rest in unused_data.
        comp = zstd.ZstdCompressor(1)
        self.assertEqual(comp.last_mode, zstd.ZstdCompressor.FLUSH_FRAME)
        stream = b""
        for chunk, mode in _chunks_with(data, sizes, modes, "CONTINUE"):
            mode = getattr(zstd.ZstdCompressor, mode)
            stream += comp.compress(chunk, mode)
            self.assertEqual(comp.last_mode, mode)
        stream += comp.flush()
        self.assertEqual(comp.last_mode, zstd.ZstdCompressor.FLUSH_FRAME)
        self.assertEqual(zstd.decompress(stream), data)

        out = b""
        rest = stream
        while rest:
            decomp = zstd.ZstdDecompressor()
            out += decomp.decompress(rest)
            self.assertTrue(decomp.eof)
            self.assertEqual(zstd.get_frame_size(rest),
                             len(rest) - len(decomp.unused_data))
            rest = decomp.unused_data
        self.assertEqual(out, data)


# ---------------------------------------------------------------------------
# zlib.decompressobj: the older protocol with unconsumed_tail
# ---------------------------------------------------------------------------

@support.requires_zlib()
class DecompressObjTests(unittest.TestCase):
    @hypothesis.given(
        data=data_strategy,
        trailer=trailer_strategy,
        sizes=chunk_sizes_strategy,
        max_lengths=st.lists(st.integers(0, 64), max_size=16),
        wbits=st.sampled_from([15, -15, 31]),
    )
    @hypothesis.example(data=b"", trailer=b"", sizes=[], max_lengths=[],
                        wbits=15)
    @hypothesis.example(data=b"", trailer=b"tail", sizes=[1, 1],
                        max_lengths=[0], wbits=-15)
    @hypothesis.example(data=b"ab" * 40, trailer=b"x", sizes=[5] * 12,
                        max_lengths=[1, 0, 3], wbits=31)
    def test_max_length_and_unconsumed_tail(self, data, trailer, sizes,
                                            max_lengths, wbits):
        # Unlike the newer decompressors, decompressobj() keeps no input
        # buffer: whatever max_length prevented it from consuming comes back
        # in unconsumed_tail and must be passed in again.  Data after the
        # end of the stream accumulates in unused_data instead.
        stream = zlib.compress(data, 1, wbits=wbits) + trailer
        decomp = zlib.decompressobj(wbits)
        out = b""
        fed = 0
        for chunk, max_length in _chunks_with(stream, sizes, max_lengths, 0):
            ret = decomp.decompress(decomp.unconsumed_tail + chunk,
                                    max_length)
            fed += len(chunk)
            if max_length:
                self.assertLessEqual(len(ret), max_length)
            out += ret
            self.assertStartsWith(data, out)
            if decomp.eof:
                self.assertEqual(out, data)
                self.assertEqual(decomp.unconsumed_tail, b"")
                self.assertEqual(decomp.unused_data + stream[fed:], trailer)
            else:
                self.assertEqual(decomp.unused_data, b"")
        while not decomp.eof:
            out += decomp.decompress(decomp.unconsumed_tail)
        self.assertEqual(out, data)
        self.assertEqual(decomp.unused_data + stream[fed:], trailer)

    @hypothesis.given(data=data_strategy, cut=st.integers(0, 10**6))
    @hypothesis.example(data=b"", cut=0)
    @hypothesis.example(data=b"", cut=10**6)
    @hypothesis.example(data=b"hello" * 20, cut=7)
    def test_flush(self, data, cut):
        stream = zlib.compress(data, 1)
        cut %= len(stream) + 1
        decomp = zlib.decompressobj()
        out = decomp.decompress(stream[:cut])
        self.assertStartsWith(data, out)
        self.assertEqual(decomp.eof, cut == len(stream))
        # flush() on a truncated stream returns what it can without raising;
        # on a complete stream it is a no-op and may be repeated.
        out += decomp.flush()
        self.assertStartsWith(data, out)
        if cut == len(stream):
            self.assertEqual(out, data)
            self.assertTrue(decomp.eof)
            self.assertEqual(decomp.flush(), b"")

    @hypothesis.given(
        data=data_strategy,
        zdict=st.binary(min_size=1, max_size=256),
        level=st.integers(0, 9),
    )
    @hypothesis.example(data=b"", zdict=b"\x00", level=0)
    @hypothesis.example(data=b"hello world" * 10, zdict=b"hello world",
                        level=9)
    def test_zdict_round_trip(self, data, zdict, level):
        for wbits in (15, -15):
            with self.subTest(wbits=wbits):
                comp = zlib.compressobj(level, wbits=wbits, zdict=zdict)
                stream = comp.compress(data) + comp.flush()
                decomp = zlib.decompressobj(wbits, zdict=zdict)
                self.assertEqual(decomp.decompress(stream), data)
                self.assertTrue(decomp.eof)


@support.requires_zlib()
class ZlibDecompressorZdictTests(unittest.TestCase):
    def _check_round_trip(self, data, zdict, level, wbits):
        comp = zlib.compressobj(level, wbits=wbits, zdict=zdict)
        stream = comp.compress(data) + comp.flush()
        decomp = zlib._ZlibDecompressor(wbits, zdict=zdict)
        self.assertEqual(decomp.decompress(stream), data)
        self.assertTrue(decomp.eof)

    @hypothesis.given(
        data=data_strategy,
        zdict=st.binary(min_size=1, max_size=256),
        level=st.integers(0, 9),
    )
    @hypothesis.example(data=b"", zdict=b"\x00", level=0)
    @hypothesis.example(data=b"hello world" * 10, zdict=b"hello world",
                        level=9)
    def test_raw_deflate(self, data, zdict, level):
        self._check_round_trip(data, zdict, level, -15)

    # _ZlibDecompressor only applies zdict up front for raw deflate; for a
    # zlib-wrapped stream it hits Z_NEED_DICT and returns NULL without
    # setting an exception (gh-160001).  That surfaces as SystemError in a
    # release build and as a fatal C assertion in a debug build, so the test
    # can only run in the former.
    @unittest.skipIf(support.Py_DEBUG, "gh-160001 aborts debug builds")
    @unittest.expectedFailure
    @hypothesis.given(
        data=data_strategy,
        zdict=st.binary(min_size=1, max_size=256),
        level=st.integers(0, 9),
    )
    @hypothesis.example(data=b"", zdict=b"\x00", level=0)
    @hypothesis.example(data=b"hello world" * 10, zdict=b"hello world",
                        level=9)
    def test_zlib_wrapper(self, data, zdict, level):
        self._check_round_trip(data, zdict, level, 15)


# ---------------------------------------------------------------------------
# File layer: BZ2File, GzipFile, LZMAFile and ZstdFile in read mode
# ---------------------------------------------------------------------------

members_strategy = st.lists(data_strategy, min_size=1, max_size=3)
read_size_strategy = st.integers(0, 300) | st.just(-1)


class _StreamReaderMachine(_StateMachine):
    """Compare a compressed file object against io.BytesIO on the same data.

    The compressed input is several concatenated streams (members, frames),
    which the file layer must present as one contiguous file.

    Subclasses set ``backend``.
    """

    backend: _Backend

    @stateful.initialize(members=members_strategy)
    def start(self, members):
        self.plain = b"".join(members)
        stream = b"".join(self.backend.compress(member) for member in members)
        self.file = self.backend.open_file(io.BytesIO(stream))
        self.oracle = io.BytesIO(self.plain)

    def teardown(self):
        self.file.close()

    def _remaining(self):
        return self.plain[self.oracle.tell():]

    @stateful.rule(size=read_size_strategy)
    def read(self, size):
        self.assertEqual(self.file.read(size), self.oracle.read(size))

    @stateful.rule(size=read_size_strategy)
    def read1(self, size):
        ret = self.file.read1(size)
        remaining = self._remaining()
        self.assertStartsWith(remaining, ret)
        if size != 0 and remaining:
            self.assertTrue(ret, "read1() returned nothing before EOF")
        if size >= 0:
            self.assertLessEqual(len(ret), size)
        self.oracle.seek(len(ret), io.SEEK_CUR)

    @stateful.rule(size=st.integers(0, 300))
    def peek(self, size):
        pos = self.file.tell()
        ret = self.file.peek(size)
        remaining = self._remaining()
        self.assertStartsWith(remaining, ret)
        if remaining:
            self.assertTrue(ret, "peek() returned nothing before EOF")
        self.assertEqual(self.file.tell(), pos)

    @stateful.rule(size=st.integers(0, 300))
    def readinto(self, size):
        buf = bytearray(size)
        n = self.file.readinto(buf)
        expected = self.oracle.read(size)
        self.assertEqual(n, len(expected))
        self.assertEqual(buf[:n], expected)

    @stateful.rule()
    def readline(self):
        self.assertEqual(self.file.readline(), self.oracle.readline())

    @stateful.rule(offset=st.integers(-10**6, 10**6),
                   whence=st.sampled_from([io.SEEK_SET, io.SEEK_CUR,
                                           io.SEEK_END]))
    def seek(self, offset, whence):
        # DecompressReader clamps to the file, unlike BytesIO.
        if whence == io.SEEK_SET:
            target = offset
        elif whence == io.SEEK_CUR:
            target = self.oracle.tell() + offset
        else:
            target = len(self.plain) + offset
        target = min(max(target, 0), len(self.plain))
        self.assertEqual(self.file.seek(offset, whence), target)
        self.oracle.seek(target)

    @stateful.invariant()
    def positions_agree(self):
        self.assertEqual(self.file.tell(), self.oracle.tell())


class _StreamReaderTests:
    """Deterministic companion to the state machine.

    Subclasses set ``backend``.
    """

    backend: _Backend

    @hypothesis.given(members=members_strategy,
                      sizes=st.lists(read_size_strategy, max_size=16))
    @hypothesis.example(members=[b""], sizes=[])
    @hypothesis.example(members=[b"", b"a", b""], sizes=[0, 1, -1, 1])
    @hypothesis.example(members=[b"abc\n" * 30, b"def" * 20],
                        sizes=[5, 0, 300, 7])
    def test_read_sizes(self, members, sizes):
        plain = b"".join(members)
        stream = b"".join(self.backend.compress(member) for member in members)
        with self.backend.open_file(io.BytesIO(stream)) as f:
            out = b""
            for size in sizes:
                ret = f.read(size)
                expected = plain[len(out):]
                if size >= 0:
                    expected = expected[:size]
                self.assertEqual(ret, expected)
                out += ret
                self.assertEqual(f.tell(), len(out))
            self.assertEqual(out + f.read(), plain)
            self.assertEqual(f.seek(0), 0)
            self.assertEqual(f.read(), plain)


@support.requires_bz2()
class BZ2StreamReaderTests(_StreamReaderTests, unittest.TestCase):
    backend = _bz2_backend()


@support.requires_gzip()
class GzipStreamReaderTests(_StreamReaderTests, unittest.TestCase):
    backend = _gzip_backend()


@support.requires_lzma()
class LZMAStreamReaderTests(_StreamReaderTests, unittest.TestCase):
    backend = _lzma_backend()


@support.requires_zstd()
class ZstdStreamReaderTests(_StreamReaderTests, unittest.TestCase):
    backend = _zstd_backend()


BZ2StreamReaderMachineTest = _machine_test(
    "BZ2StreamReaderMachine", _StreamReaderMachine, _bz2_backend,
    support.requires_bz2())
GzipStreamReaderMachineTest = _machine_test(
    "GzipStreamReaderMachine", _StreamReaderMachine, _gzip_backend,
    support.requires_gzip())
LZMAStreamReaderMachineTest = _machine_test(
    "LZMAStreamReaderMachine", _StreamReaderMachine, _lzma_backend,
    support.requires_lzma())
ZstdStreamReaderMachineTest = _machine_test(
    "ZstdStreamReaderMachine", _StreamReaderMachine, _zstd_backend,
    support.requires_zstd())


# ---------------------------------------------------------------------------
# zstd: frames, dictionaries and pledged sizes
# ---------------------------------------------------------------------------

@support.requires_zstd()
class ZstdFrameTests(unittest.TestCase):
    @hypothesis.given(data=data_strategy, level=st.integers(1, 5),
                      trailer=trailer_strategy)
    @hypothesis.example(data=b"", level=1, trailer=b"")
    @hypothesis.example(data=b"x" * 100, level=3, trailer=b"junk")
    def test_frame_size_and_info(self, data, level, trailer):
        frame = zstd.compress(data, level)
        self.assertEqual(zstd.get_frame_size(frame + trailer), len(frame))
        info = zstd.get_frame_info(frame)
        self.assertEqual(info.decompressed_size, len(data))
        self.assertEqual(info.dictionary_id, 0)

    @hypothesis.given(members=members_strategy)
    @hypothesis.example(members=[b""])
    @hypothesis.example(members=[b"", b"abc" * 20, b""])
    def test_multiple_frames(self, members):
        stream = b"".join(zstd.compress(member, 1) for member in members)
        self.assertEqual(zstd.decompress(stream), b"".join(members))
        rest = stream
        for member in members:
            decomp = zstd.ZstdDecompressor()
            self.assertEqual(decomp.decompress(rest), member)
            self.assertTrue(decomp.eof)
            rest = decomp.unused_data
        self.assertEqual(rest, b"")

    @hypothesis.given(
        data=data_strategy,
        dict_content=st.binary(min_size=8, max_size=256),
        level=st.integers(1, 5),
    )
    @hypothesis.example(data=b"", dict_content=b"\x00" * 8, level=1)
    @hypothesis.example(data=b"hello world" * 10, dict_content=b"hello world!",
                        level=3)
    def test_raw_dict_round_trip(self, data, dict_content, level):
        # A raw content dictionary has no ID and can be used either as a
        # dictionary or as a prefix; the frame is the same either way and
        # decompresses with any of the loading modes.
        zd = zstd.ZstdDict(dict_content, is_raw=True)
        self.assertEqual(zd.dict_id, 0)
        self.assertEqual(zd.dict_content, dict_content)
        modes = [zd, zd.as_digested_dict, zd.as_undigested_dict, zd.as_prefix]
        for comp_mode in modes:
            frame = zstd.compress(data, level, zstd_dict=comp_mode)
            self.assertEqual(zstd.get_frame_info(frame).dictionary_id, 0)
            for decomp_mode in modes:
                self.assertEqual(
                    zstd.decompress(frame, zstd_dict=decomp_mode), data)

    @hypothesis.given(data=data_strategy, extra=st.integers(1, 100))
    @hypothesis.example(data=b"", extra=1)
    @hypothesis.example(data=b"abc" * 30, extra=7)
    def test_pledged_input_size(self, data, extra):
        comp = zstd.ZstdCompressor(1)
        comp.set_pledged_input_size(len(data))
        frame = comp.compress(data) + comp.flush()
        self.assertEqual(zstd.get_frame_info(frame).decompressed_size,
                         len(data))
        self.assertEqual(zstd.decompress(frame), data)

        # A wrong pledge is detected when the frame is finished, and the
        # compressor is reset so it can be reused.  (A frame compressed in a
        # single FLUSH_FRAME call is exempt: libzstd then knows the real
        # size and records that instead.)
        comp.set_pledged_input_size(len(data) + extra)
        with self.assertRaises(zstd.ZstdError):
            comp.compress(data)
            comp.flush()
        self.assertEqual(comp.last_mode, zstd.ZstdCompressor.FLUSH_FRAME)
        frame = comp.compress(data, zstd.ZstdCompressor.FLUSH_FRAME)
        self.assertEqual(zstd.decompress(frame), data)


if __name__ == "__main__":
    unittest.main()
