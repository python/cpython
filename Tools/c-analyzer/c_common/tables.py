from collections import namedtuple
import csv
import re
import textwrap

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
        if unknown:
            row = (UNKNOWN if val in unknown else val for val in row)
    elif not unknown:
        row = (EMPTY if val in empty else val for val in row)
    else:
        row = (EMPTY if val in empty else (UNKNOWN if val in unknown else val)
               for val in row)
    return tuple(row)


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


##################################
# stdout tables

WIDTH = 20


def resolve_columns(specs):
    if isinstance(specs, str):
        specs = specs.replace(',', ' ').strip().split()
    resolved = []
    for raw in specs:
        column = ColumnSpec.from_raw(raw)
        resolved.append(column)
    return resolved


def build_table(specs, *, sep=' ', defaultwidth=None):
    columns = resolve_columns(specs)
    return _build_table(columns, sep=sep, defaultwidth=defaultwidth)


class ColumnSpec(namedtuple('ColumnSpec', 'field label fmt')):

    REGEX = re.compile(textwrap.dedent(r'''
        ^
        (?:
            \[
            (
                (?: [^\s\]] [^\]]* )?
                [^\s\]]
            )  # <label>
            ]
        )?
        ( [-\w]+ )  # <field>
        (?:
            (?:
                :
                ( [<^>] )  # <align>
                ( \d+ )?  # <width1>
            )
            |
            (?:
                (?:
                    :
                    ( \d+ )  # <width2>
                )?
                (?:
                    :
                    ( .*? )  # <fmt>
                )?
            )
        )?
        $
    '''), re.VERBOSE)

    @classmethod
    def from_raw(cls, raw):
        if not raw:
            raise ValueError('missing column spec')
        elif isinstance(raw, cls):
            return raw

        if isinstance(raw, str):
            *values, _ = cls._parse(raw)
        else:
            *values, _ = cls._normalize(raw)
        if values is None:
            raise ValueError(f'unsupported column spec {raw!r}')
        return cls(*values)

    @classmethod
    def parse(cls, specstr):
        parsed = cls._parse(specstr)
        if not parsed:
            return None
        *values, _ = parsed
        return cls(*values)

    @classmethod
    def _parse(cls, specstr):
        m = cls.REGEX.match(specstr)
        if not m:
            return None
        (label, field,
         align, width1,
         width2, fmt,
         ) = m.groups()
        if not label:
            label = field
        if fmt:
            assert not align and not width1, (specstr,)
            _parsed = _parse_fmt(fmt)
            if not _parsed:
                raise NotImplementedError
            elif width2:
                width, _ = _parsed
                if width != int(width2):
                    raise NotImplementedError(specstr)
        elif width2:
            fmt = width2
            width = int(width2)
        else:
            assert not fmt, (fmt, specstr)
            if align:
                width = int(width1) if width1 else len(label)
                fmt = f'{align}{width}'
            else:
                width = None
        return field, label, fmt, width

    @classmethod
    def _normalize(cls, spec):
        if len(spec) == 1:
            raw, = spec
            raise NotImplementedError
            return _resolve_column(raw)

        if len(spec) == 4:
            label, field, width, fmt = spec
            if width:
                if not fmt:
                    fmt = str(width)
                elif _parse_fmt(fmt)[0] != width:
                    raise ValueError(f'width mismatch in {spec}')
        elif len(raw) == 3:
            label, field, fmt = spec
            if not field:
                label, field = None, label
            elif not isinstance(field, str) or not field.isidentifier():
                # XXX This doesn't seem right...
                fmt = f'{field}:{fmt}' if fmt else field
                label, field = None, label
        elif len(raw) == 2:
            label = None
            field, fmt = raw
            if not field:
                field, fmt = fmt, None
            elif not field.isidentifier() or fmt.isidentifier():
                label, field = field, fmt
        else:
            raise NotImplementedError

        fmt = f':{fmt}' if fmt else ''
        if label:
            return cls._parse(f'[{label}]{field}{fmt}')
        else:
            return cls._parse(f'{field}{fmt}')

    @property
    def width(self):
        if not self.fmt:
            return None
        parsed = _parse_fmt(self.fmt)
        if not parsed:
            return None
        width, _ = parsed
        return width

    def resolve_width(self, default=None):
        return _resolve_width(self.width, self.fmt, self.label, default)


def _parse_fmt(fmt):
    if fmt.startswith(tuple('<^>')):
        align = fmt[0]
        width = fmt[1:]
        if width.isdigit():
            return int(width), align
    elif fmt.isdigit():
        return int(fmt), '<'
    return None


def _resolve_width(width, fmt, label, default):
    if width:
        if not isinstance(width, int):
            raise NotImplementedError
        return width
    elif fmt:
        parsed = _parse_fmt(fmt)
        if parsed:
            width, _ = parsed
            if width:
                return width

    if not default:
        return WIDTH
    elif hasattr(default, 'get'):
        defaults = default
        default = defaults.get(None) or WIDTH
        return defaults.get(label) or default
    else:
        return default or WIDTH


def _build_table(columns, *, sep=' ', defaultwidth=None):
    header = []
    div = []
    rowfmt = []
    for spec in columns:
        width = spec.resolve_width(defaultwidth)
        colfmt = spec.fmt
        colfmt = f':{spec.fmt}' if spec.fmt else f':{width}'

        header.append(f' {{:^{width}}} '.format(spec.label))
        div.append('-' * (width + 2))
        rowfmt.append(f' {{{spec.field}{colfmt}}} ')
    return (
        sep.join(header),
        sep.join(div),
        sep.join(rowfmt),
    )
