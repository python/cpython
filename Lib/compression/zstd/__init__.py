"""Python bindings to Zstandard (zstd) compression library, the API style is
similar to Python's bz2/lzma/zlib modules.
"""

__all__ = (
    # compression.zstd
    "compressionLevel_values",
    "compress",
    "CParameter",
    "decompress",
    "DParameter",
    "finalize_dict",
    "get_frame_info",
    "Strategy",
    "train_dict",
    "zstd_support_multithread",

    # compression.zstd.zstdfile
    "open",
    "ZstdFile",

    # _zstd
    "get_frame_size",
    "zstd_version",
    "zstd_version_info",
    "ZstdCompressor",
    "ZstdDecompressor",
    "ZstdDict",
    "ZstdError",
)

import enum
import functools
import dataclasses

from compression.zstd.zstdfile import ZstdFile, open
from _zstd import *

import _zstd


_ZSTD_CStreamSizes = _zstd._ZSTD_CStreamSizes
_ZSTD_DStreamSizes = _zstd._ZSTD_DStreamSizes
_train_dict = _zstd._train_dict
_finalize_dict = _zstd._finalize_dict


class _CLValues:
    __slots__ = 'default', 'min', 'max'

    def __init__(self, default, min, max):
        super().__setattr__('default', default)
        super().__setattr__('min', min)
        super().__setattr__('max', max)

    def __repr__(self):
        return (f'compression_level_values(default={self.default}, '
                f'min={self.min}, max={self.max})')

    def __setattr__(self, name, _):
        raise AttributeError(f"can't set attribute {name!r}")

compressionLevel_values = _CLValues(*_zstd._compressionLevel_values)

class FrameInfo:
    """Information about a Zstandard frame."""
    __slots__ = 'decompressed_size', 'dictionary_id'

    def __init__(self, decompressed_size, dictionary_id):
        super().__setattr__('decompressed_size', decompressed_size)
        super().__setattr__('dictionary_id', dictionary_id)

    def __repr__(self):
        return (f'FrameInfo(decompressed_size={self.decompressed_size}, '
                f'dictionary_id={self.dictionary_id})')

    def __setattr__(self, name, _):
        raise AttributeError(f"can't set attribute {name!r}")


def get_frame_info(frame_buffer):
    """Get zstd frame information from a frame header.

    *frame_buffer* is a bytes-like object. It should starts from the beginning of
    a frame, and needs to include at least the frame header (6 to 18 bytes).

    Return a FrameInfo dataclass, which currently has two attributes

    'decompressed_size' is the size in bytes of the data in the frame when
    decompressed, or None when the decompressed size is unknown.

    'dictionary_id' is a 32-bit unsigned integer value. 0 means dictionary ID was
    not recorded in the frame header, the frame may or may not need a dictionary
    to be decoded, and the ID of such a dictionary is not specified.
    """
    return FrameInfo(*_zstd._get_frame_info(frame_buffer))


def _nbytes(dat):
    if isinstance(dat, (bytes, bytearray)):
        return len(dat)
    with memoryview(dat) as mv:
        return mv.nbytes


def train_dict(samples, dict_size):
    """Train a zstd dictionary, return a ZstdDict object.

    *samples* is an iterable of samples, where a sample is a bytes-like
    object representing a file.
    *dict_size* is the dictionary's maximum size, in bytes.
    """
    if not isinstance(dict_size, int):
        ds_cls = type(dict_size).__qualname__
        raise TypeError('dict_size must be an int object, not {ds_cls!r}.')

    samples = tuple(samples)
    chunks = b''.join(samples)
    chunk_sizes = tuple(map(_nbytes, samples))
    if not chunks:
        raise ValueError("samples contained no data; can't train dictionary.")
    dict_content = _train_dict(chunks, chunk_sizes, dict_size)

    return ZstdDict(dict_content)


def finalize_dict(zstd_dict, samples, dict_size, level):
    """Finalize a zstd dictionary, return a ZstdDict object.

    Given a custom content as a basis for dictionary, and a set of samples,
    finalize dictionary by adding headers and statistics according to the zstd
    dictionary format.

    You may compose an effective dictionary content by hand, which is used as
    basis dictionary, and use some samples to finalize a dictionary. The basis
    dictionary can be a "raw content" dictionary, see is_raw parameter in
    ZstdDict.__init__ method.

    Parameters
    zstd_dict: A ZstdDict object, basis dictionary.
    samples:   An iterable of samples, a sample is a bytes-like object
               represents a file.
    dict_size: The dictionary's maximum size, in bytes.
    level:     The compression level expected to use in production. The
               statistics for each compression level differ, so tuning the
               dictionary for the compression level can help quite a bit.
    """

    # Check arguments' type
    if not isinstance(zstd_dict, ZstdDict):
        raise TypeError('zstd_dict argument should be a ZstdDict object.')
    if not isinstance(dict_size, int):
        raise TypeError('dict_size argument should be an int object.')
    if not isinstance(level, int):
        raise TypeError('level argument should be an int object.')

    # Prepare data
    chunks = []
    chunk_sizes = []
    for chunk in samples:
        chunks.append(chunk)
        chunk_sizes.append(_nbytes(chunk))

    chunks = b''.join(chunks)
    if not chunks:
        raise ValueError("The samples are empty content, can't finalize dictionary.")

    # custom_dict_bytes: existing dictionary.
    # samples_bytes: samples be stored concatenated in a single flat buffer.
    # samples_size_list: a list of each sample's size.
    # dict_size: maximal size of the dictionary, in bytes.
    # compression_level: compression level expected to use in production.
    dict_content = _finalize_dict(zstd_dict.dict_content,
                                  chunks, chunk_sizes,
                                  dict_size, level)
    return ZstdDict(dict_content)

def compress(data, level=None, options=None, zstd_dict=None):
    """Return Zstandard compressed *data* as bytes.

    Refer to ZstdCompressor's docstring for a description of the
    optional arguments *level*, *options*, and *zstd_dict*.

    For incremental compression, use an ZstdCompressor instead.
    """
    comp = ZstdCompressor(level=level, options=options, zstd_dict=zstd_dict)
    return comp.compress(data, ZstdCompressor.FLUSH_FRAME)

def decompress(data, zstd_dict=None, options=None):
    """Decompress one or more frames of data.

    Refer to ZstdDecompressor's docstring for a description of the
    optional arguments *zstd_dict*, *options*.

    For incremental decompression, use an ZstdDecompressor instead.
    """
    results = []
    while True:
        decomp = ZstdDecompressor(options=options, zstd_dict=zstd_dict)
        try:
            res = decomp.decompress(data)
        except ZstdError:
            if results:
                break  # Leftover data is not a valid LZMA/XZ stream; ignore it.
            else:
                raise  # Error on the first iteration; bail out.
        results.append(res)
        if not decomp.eof:
            raise ZstdError("Compressed data ended before the "
                            "end-of-stream marker was reached")
        data = decomp.unused_data
        if not data:
            break
    return b"".join(results)

class _UnsupportedCParameter:
    def __set_name__(self, _, name):
        self.name = name

    def __get__(self, *_, **__):
        msg = ("%s CParameter not available, zstd version is %s.") % (
            self.name,
            zstd_version,
        )
        raise NotImplementedError(msg)


class CParameter(enum.IntEnum):
    """Compression parameters"""

    compressionLevel = _zstd._ZSTD_c_compressionLevel
    windowLog = _zstd._ZSTD_c_windowLog
    hashLog = _zstd._ZSTD_c_hashLog
    chainLog = _zstd._ZSTD_c_chainLog
    searchLog = _zstd._ZSTD_c_searchLog
    minMatch = _zstd._ZSTD_c_minMatch
    targetLength = _zstd._ZSTD_c_targetLength
    strategy = _zstd._ZSTD_c_strategy

    targetCBlockSize = _UnsupportedCParameter()

    enableLongDistanceMatching = _zstd._ZSTD_c_enableLongDistanceMatching
    ldmHashLog = _zstd._ZSTD_c_ldmHashLog
    ldmMinMatch = _zstd._ZSTD_c_ldmMinMatch
    ldmBucketSizeLog = _zstd._ZSTD_c_ldmBucketSizeLog
    ldmHashRateLog = _zstd._ZSTD_c_ldmHashRateLog

    contentSizeFlag = _zstd._ZSTD_c_contentSizeFlag
    checksumFlag = _zstd._ZSTD_c_checksumFlag
    dictIDFlag = _zstd._ZSTD_c_dictIDFlag

    nbWorkers = _zstd._ZSTD_c_nbWorkers
    jobSize = _zstd._ZSTD_c_jobSize
    overlapLog = _zstd._ZSTD_c_overlapLog

    @functools.lru_cache(maxsize=None)
    def bounds(self):
        """Return lower and upper bounds of a compression parameter, both inclusive."""
        # 1 means compression parameter
        return _zstd._get_param_bounds(1, self.value)


class DParameter(enum.IntEnum):
    """Decompression parameters"""

    windowLogMax = _zstd._ZSTD_d_windowLogMax

    @functools.lru_cache(maxsize=None)
    def bounds(self):
        """Return lower and upper bounds of a decompression parameter, both inclusive."""
        # 0 means decompression parameter
        return _zstd._get_param_bounds(0, self.value)


class Strategy(enum.IntEnum):
    """Compression strategies, listed from fastest to strongest.

    Note : new strategies _might_ be added in the future, only the order
    (from fast to strong) is guaranteed.
    """

    fast = _zstd._ZSTD_fast
    dfast = _zstd._ZSTD_dfast
    greedy = _zstd._ZSTD_greedy
    lazy = _zstd._ZSTD_lazy
    lazy2 = _zstd._ZSTD_lazy2
    btlazy2 = _zstd._ZSTD_btlazy2
    btopt = _zstd._ZSTD_btopt
    btultra = _zstd._ZSTD_btultra
    btultra2 = _zstd._ZSTD_btultra2


# Set CParameter/DParameter types for validity check
_zstd._set_parameter_types(CParameter, DParameter)

zstd_support_multithread = CParameter.nbWorkers.bounds() != (0, 0)
