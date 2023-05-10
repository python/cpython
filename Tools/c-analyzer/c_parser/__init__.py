from c_common.fsutil import match_glob as _match_glob
from .parser import parse as _parse
from .preprocessor import get_preprocessor as _get_preprocessor


def parse_file(filename, *,
               match_kind=None,
               get_file_preprocessor=None,
               file_maxsizes=None,
               ):
    if get_file_preprocessor is None:
        get_file_preprocessor = _get_preprocessor()
    yield from _parse_file(
            filename, match_kind, get_file_preprocessor, file_maxsizes)


def parse_files(filenames, *,
                match_kind=None,
                get_file_preprocessor=None,
                file_maxsizes=None,
                ):
    if get_file_preprocessor is None:
        get_file_preprocessor = _get_preprocessor()
    for filename in filenames:
        try:
            yield from _parse_file(
                    filename, match_kind, get_file_preprocessor, file_maxsizes)
        except Exception:
            print(f'# requested file: <{filename}>')
            raise  # re-raise


def _parse_file(filename, match_kind, get_file_preprocessor, maxsizes):
    srckwargs = {}
    maxsize = _resolve_max_size(filename, maxsizes)
    if maxsize:
        srckwargs['maxtext'], srckwargs['maxlines'] = maxsize

    # Preprocess the file.
    preprocess = get_file_preprocessor(filename)
    preprocessed = preprocess()
    if preprocessed is None:
        return

    # Parse the lines.
    srclines = ((l.file, l.data) for l in preprocessed if l.kind == 'source')
    for item in _parse(srclines, **srckwargs):
        if match_kind is not None and not match_kind(item.kind):
            continue
        if not item.filename:
            raise NotImplementedError(repr(item))
        yield item


def _resolve_max_size(filename, maxsizes):
    for pattern, maxsize in (maxsizes.items() if maxsizes else ()):
        if _match_glob(filename, pattern):
            break
    else:
        return None
    if not maxsize:
        return None, None
    maxtext, maxlines = maxsize
    if maxtext is not None:
        maxtext = int(maxtext)
    if maxlines is not None:
        maxlines = int(maxlines)
    return maxtext, maxlines


def parse_signature(text):
    raise NotImplementedError


# aliases
from .info import resolve_parsed
