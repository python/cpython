from .parser import parse as _parse
from .preprocessor import get_preprocessor as _get_preprocessor


def parse_file(filename, *,
               match_kind=None,
               get_file_preprocessor=None,
               ):
    if get_file_preprocessor is None:
        get_file_preprocessor = _get_preprocessor()
    yield from _parse_file(filename, match_kind, get_file_preprocessor)


def parse_files(filenames, *,
                match_kind=None,
                get_file_preprocessor=None,
                ):
    if get_file_preprocessor is None:
        get_file_preprocessor = _get_preprocessor()
    for filename in filenames:
        yield from _parse_file(filename, match_kind, get_file_preprocessor)


def _parse_file(filename, match_kind, get_file_preprocessor):
    # Preprocess the file.
    preprocess = get_file_preprocessor(filename)
    preprocessed = preprocess()
    if preprocessed is None:
        return

    # Parse the lines.
    srclines = ((l.file, l.data) for l in preprocessed if l.kind == 'source')
    for item in _parse(srclines):
        if match_kind is not None and not match_kind(item.kind):
            continue
        if not item.filename:
            raise NotImplementedError(repr(item))
        yield item


def parse_signature(text):
    raise NotImplementedError


# aliases
from .info import resolve_parsed
