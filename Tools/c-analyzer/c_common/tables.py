import csv

from . import NOT_SET, strutil, fsutil


EMPTY = '-'
UNKNOWN = '???'


def parse_markers(markers, default=None):
    if markers is NOT_SET:
        return default
    if not markers:
        return None
    if type(markers) is not str:
        return markers
    if markers == markers[0] * len(markers):
        return [markers]
    return list(markers)


def fix_row(row, **markers):
    if isinstance(row, str):
        raise NotImplementedError(row)
    empty = parse_markers(markers.pop('empty', ('-',)))
    unknown = parse_markers(markers.pop('unknown', ('???',)))
    row = (val if val else None for val in row)
    if not empty:
        if not unknown:
            return row
        return (UNKNOWN if val in unknown else val for val in row)
    elif not unknown:
        return (EMPTY if val in empty else val for val in row)
    return (EMPTY if val in empty else (UNKNOWN if val in unknown else val)
            for val in row)


def _fix_read_default(row):
    for value in row:
        yield value.strip()


def _fix_write_default(row, empty=''):
    for value in row:
        yield empty if value is None else str(value)


def _normalize_fix_read(fix):
    if fix is None:
        fix = ''
    if callable(fix):
        def fix_row(row):
            values = fix(row)
            return _fix_read_default(values)
    elif isinstance(fix, str):
        def fix_row(row):
            values = _fix_read_default(row)
            return (None if v == fix else v
                    for v in values)
    else:
        raise NotImplementedError(fix)
    return fix_row


def _normalize_fix_write(fix, empty=''):
    if fix is None:
        fix = empty
    if callable(fix):
        def fix_row(row):
            values = fix(row)
            return _fix_write_default(values, empty)
    elif isinstance(fix, str):
        def fix_row(row):
            return _fix_write_default(row, fix)
    else:
        raise NotImplementedError(fix)
    return fix_row


def read_table(infile, header, *,
               sep='\t',
               fix=None,
               _open=open,
               _get_reader=csv.reader,
               ):
    """Yield each row of the given ???-separated (e.g. tab) file."""
    if isinstance(infile, str):
        with _open(infile, newline='') as infile:
            yield from read_table(
                infile,
                header,
                sep=sep,
                fix=fix,
                _open=_open,
                _get_reader=_get_reader,
            )
            return
    lines = strutil._iter_significant_lines(infile)

    # Validate the header.
    if not isinstance(header, str):
        header = sep.join(header)
    try:
        actualheader = next(lines).strip()
    except StopIteration:
        actualheader = ''
    if actualheader != header:
        raise ValueError(f'bad header {actualheader!r}')

    fix_row = _normalize_fix_read(fix)
    for row in _get_reader(lines, delimiter=sep or '\t'):
        yield tuple(fix_row(row))


def write_table(outfile, header, rows, *,
                sep='\t',
                fix=None,
                backup=True,
                _open=open,
                _get_writer=csv.writer,
                ):
    """Write each of the rows to the given ???-separated (e.g. tab) file."""
    if backup:
        fsutil.create_backup(outfile, backup)
    if isinstance(outfile, str):
        with _open(outfile, 'w', newline='') as outfile:
            return write_table(
                outfile,
                header,
                rows,
                sep=sep,
                fix=fix,
                backup=backup,
                _open=_open,
                _get_writer=_get_writer,
            )

    if isinstance(header, str):
        header = header.split(sep or '\t')
    fix_row = _normalize_fix_write(fix)
    writer = _get_writer(outfile, delimiter=sep or '\t')
    writer.writerow(header)
    for row in rows:
        writer.writerow(
            tuple(fix_row(row))
        )


def parse_table(entries, sep, header=None, rawsep=None, *,
                default=NOT_SET,
                strict=True,
                ):
    header, sep = _normalize_table_file_props(header, sep)
    if not sep:
        raise ValueError('missing "sep"')

    ncols = None
    if header:
        if strict:
            ncols = len(header.split(sep))
        cur_file = None
    for line, filename in strutil.parse_entries(entries, ignoresep=sep):
        _sep = sep
        if filename:
            if header and cur_file != filename:
                cur_file = filename
                # Skip the first line if it's the header.
                if line.strip() == header:
                    continue
                else:
                    # We expected the header.
                    raise NotImplementedError((header, line))
        elif rawsep and sep not in line:
            _sep = rawsep

        row = _parse_row(line, _sep, ncols, default)
        if strict and not ncols:
            ncols = len(row)
        yield row, filename


def parse_row(line, sep, *, ncols=None, default=NOT_SET):
    if not sep:
        raise ValueError('missing "sep"')
    return _parse_row(line, sep, ncols, default)


def _parse_row(line, sep, ncols, default):
    row = tuple(v.strip() for v in line.split(sep))
    if (ncols or 0) > 0:
        diff = ncols - len(row)
        if diff:
            if default is NOT_SET or diff < 0:
                raise Exception(f'bad row (expected {ncols} columns, got {row!r})')
            row += (default,) * diff
    return row


def _normalize_table_file_props(header, sep):
    if not header:
        return None, sep

    if not isinstance(header, str):
        if not sep:
            raise NotImplementedError(header)
        header = sep.join(header)
    elif not sep:
        for sep in ('\t', ',', ' '):
            if sep in header:
                break
        else:
            sep = None
    return header, sep
