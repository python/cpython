"""Python bindings to the Zstandard (zstd) compression library (RFC-8878)."""

__all__ = (
    # compression.zstd
    'COMPRESSION_LEVEL_DEFAULT',
    'compress',
    'CompressionParameter',
    'decompress',
    'DecompressionParameter',
    'finalize_dict',
    'get_frame_info',
    'Strategy',
    'train_dict',

    # compression.zstd._zstdfile
    'open',
    'ZstdFile',

    # _zstd
    'get_frame_size',
    'zstd_version',
    'zstd_version_info',
    'ZstdCompressor',
    'ZstdDecompressor',
    'ZstdDict',
    'ZstdError',
)

import _zstd
import enum
from _zstd import (ZstdCompressor, ZstdDecompressor, ZstdDict, ZstdError,
                   get_frame_size, zstd_version)
from compression.zstd._zstdfile import ZstdFile, open, _nbytes

# zstd_version_number is (MAJOR * 100 * 100 + MINOR * 100 + RELEASE)
zstd_version_info = (*divmod(_zstd.zstd_version_number // 100, 100),
                     _zstd.zstd_version_number % 100)
"""Version number of the runtime zstd library as a tuple of integers."""

COMPRESSION_LEVEL_DEFAULT = _zstd.ZSTD_CLEVEL_DEFAULT
"""The default compression level for Zstandard, currently '3'."""


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
    """Get Zstandard frame information from a frame header.

    *frame_buffer* is a bytes-like object. It should start from the beginning
    of a frame, and needs to include at least the frame header (6 to 18 bytes).

    The returned FrameInfo object has two attributes.
    'decompressed_size' is the size in bytes of the data in the frame when
    decompressed, or None when the decompressed size is unknown.
    'dictionary_id' is an int in the range (0, 2**32). The special value 0
    means that the dictionary ID was not recorded in the frame header,
    the frame may or may not need a dictionary to be decoded,
    and the ID of such a dictionary is not specified.
    """
    return FrameInfo(*_zstd.get_frame_info(frame_buffer))


def train_dict(samples, dict_size):
    """Return a ZstdDict representing a trained Zstandard dictionary.

    *samples* is an iterable of samples, where a sample is a bytes-like
    object representing a file.

    *dict_size* is the dictionary's maximum size, in bytes.
    """
    if not isinstance(dict_size, int):
        ds_cls = type(dict_size).__qualname__
        raise TypeError(f'dict_size must be an int object, not {ds_cls!r}.')

    samples = tuple(samples)
    chunks = b''.join(samples)
    chunk_sizes = tuple(_nbytes(sample) for sample in samples)
    if not chunks:
        raise ValueError("samples contained no data; can't train dictionary.")
    dict_content = _zstd.train_dict(chunks, chunk_sizes, dict_size)
    return ZstdDict(dict_content)


def finalize_dict(zstd_dict, /, samples, dict_size, level):
    """Return a ZstdDict representing a finalized Zstandard dictionary.

    Given a custom content as a basis for dictionary, and a set of samples,
    finalize *zstd_dict* by adding headers and statistics according to the
    Zstandard dictionary format.

    You may compose an effective dictionary content by hand, which is used as
    basis dictionary, and use some samples to finalize a dictionary. The basis
    dictionary may be a "raw content" dictionary. See *is_raw* in ZstdDict.

    *samples* is an iterable of samples, where a sample is a bytes-like object
    representing a file.
    *dict_size* is the dictionary's maximum size, in bytes.
    *level* is the expected compression level. The statistics for each
    compression level differ, so tuning the dictionary to the compression level
    can provide improvements.
    """

    if not isinstance(zstd_dict, ZstdDict):
        raise TypeError('zstd_dict argument should be a ZstdDict object.')
    if not isinstance(dict_size, int):
        raise TypeError('dict_size argument should be an int object.')
    if not isinstance(level, int):
        raise TypeError('level argument should be an int object.')

    samples = tuple(samples)
    chunks = b''.join(samples)
    chunk_sizes = tuple(_nbytes(sample) for sample in samples)
    if not chunks:
        raise ValueError("The samples are empty content, can't finalize the "
                         "dictionary.")
    dict_content = _zstd.finalize_dict(zstd_dict.dict_content, chunks,
                                       chunk_sizes, dict_size, level)
    return ZstdDict(dict_content)


def compress(data, level=None, options=None, zstd_dict=None):
    """Return Zstandard compressed *data* as bytes.

    *level* is an int specifying the compression level to use, defaulting to
    COMPRESSION_LEVEL_DEFAULT ('3').
    *options* is a dict object that contains advanced compression
    parameters. See CompressionParameter for more on options.
    *zstd_dict* is a ZstdDict object, a pre-trained Zstandard dictionary. See
    the function train_dict for how to train a ZstdDict on sample data.

    For incremental compression, use a ZstdCompressor instead.
    """
    comp = ZstdCompressor(level=level, options=options, zstd_dict=zstd_dict)
    return comp.compress(data, mode=ZstdCompressor.FLUSH_FRAME)


def decompress(data, zstd_dict=None, options=None):
    """Decompress one or more frames of Zstandard compressed *data*.

    *zstd_dict* is a ZstdDict object, a pre-trained Zstandard dictionary. See
    the function train_dict for how to train a ZstdDict on sample data.
    *options* is a dict object that contains advanced compression
    parameters. See DecompressionParameter for more on options.

    For incremental decompression, use a ZstdDecompressor instead.
    """
    results = []
    while True:
        decomp = ZstdDecompressor(options=options, zstd_dict=zstd_dict)
        results.append(decomp.decompress(data))
        if not decomp.eof:
            raise ZstdError('Compressed data ended before the '
                            'end-of-stream marker was reached')
        data = decomp.unused_data
        if not data:
            break
    return b''.join(results)


class CompressionParameter(enum.IntEnum):
    """Compression parameters."""

    compression_level = _zstd.ZSTD_c_compressionLevel
    window_log = _zstd.ZSTD_c_windowLog
    hash_log = _zstd.ZSTD_c_hashLog
    chain_log = _zstd.ZSTD_c_chainLog
    search_log = _zstd.ZSTD_c_searchLog
    min_match = _zstd.ZSTD_c_minMatch
    target_length = _zstd.ZSTD_c_targetLength
    strategy = _zstd.ZSTD_c_strategy

    enable_long_distance_matching = _zstd.ZSTD_c_enableLongDistanceMatching
    ldm_hash_log = _zstd.ZSTD_c_ldmHashLog
    ldm_min_match = _zstd.ZSTD_c_ldmMinMatch
    ldm_bucket_size_log = _zstd.ZSTD_c_ldmBucketSizeLog
    ldm_hash_rate_log = _zstd.ZSTD_c_ldmHashRateLog

    content_size_flag = _zstd.ZSTD_c_contentSizeFlag
    checksum_flag = _zstd.ZSTD_c_checksumFlag
    dict_id_flag = _zstd.ZSTD_c_dictIDFlag

    nb_workers = _zstd.ZSTD_c_nbWorkers
    job_size = _zstd.ZSTD_c_jobSize
    overlap_log = _zstd.ZSTD_c_overlapLog

    def bounds(self):
        """Return the (lower, upper) int bounds of a compression parameter.

        Both the lower and upper bounds are inclusive.
        """
        return _zstd.get_param_bounds(self.value, is_compress=True)


class DecompressionParameter(enum.IntEnum):
    """Decompression parameters."""

    window_log_max = _zstd.ZSTD_d_windowLogMax

    def bounds(self):
        """Return the (lower, upper) int bounds of a decompression parameter.

        Both the lower and upper bounds are inclusive.
        """
        return _zstd.get_param_bounds(self.value, is_compress=False)


class Strategy(enum.IntEnum):
    """Compression strategies, listed from fastest to strongest.

    Note that new strategies might be added in the future.
    Only the order (from fast to strong) is guaranteed,
    the numeric value might change.
    """

    fast = _zstd.ZSTD_fast
    dfast = _zstd.ZSTD_dfast
    greedy = _zstd.ZSTD_greedy
    lazy = _zstd.ZSTD_lazy
    lazy2 = _zstd.ZSTD_lazy2
    btlazy2 = _zstd.ZSTD_btlazy2
    btopt = _zstd.ZSTD_btopt
    btultra = _zstd.ZSTD_btultra
    btultra2 = _zstd.ZSTD_btultra2


# Check validity of the CompressionParameter & DecompressionParameter types
_zstd.set_parameter_types(CompressionParameter, DecompressionParameter)
