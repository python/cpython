
KINDS = [
    'section-major',
    'section-minor',
    'section-group',
    'row',
]


def iter_clean_lines(lines):
    lines = iter(lines)
    for rawline in lines:
        line = rawline.strip()
        if line.startswith('#') and not rawline.startswith('##'):
            continue
        yield line, rawline


def parse_table_lines(lines):
    lines = iter_clean_lines(lines)

    group = None
    prev = ''
    for line, rawline in lines:
        if line.startswith('## '):
            assert not rawline.startswith(' '), (line, rawline)
            if group:
                assert prev, (line, rawline)
                kind, after, _ = group
                assert kind and kind != 'section-group', (group, line, rawline)
                assert after is not None, (group, line, rawline)
            else:
                assert not prev, (prev, line, rawline)
                kind, after = group = ('section-group', None)
            title = line[3:].lstrip()
            assert title, (line, rawline)
            if after is not None:
                try:
                    line, rawline = next(lines)
                except StopIteration:
                    line = None
                if line != after:
                    raise NotImplementedError((group, line, rawline))
            yield kind, title
            group = None
        elif group:
            raise NotImplementedError((group, line, rawline))
        elif line.startswith('##---'):
            assert line.rstrip('-') == '##', (line, rawline)
            group = ('section-minor', '', line)
        elif line.startswith('#####'):
            assert not line.strip('#'), (line, rawline)
            group = ('section-major', '', line)
        elif line:
            yield 'row', line
        prev = line


def iter_sections(lines):
    header = None
    section = []
    for kind, value in parse_table_lines(lines):
        if kind == 'row':
            if not section:
                if header is None:
                    header = value
                    continue
                raise NotImplementedError(repr(value))
            yield tuple(section), value
        else:
            if header is None:
                header = False
            start = KINDS.index(kind)
            section[start:] = [value]


def collect_sections(lines):
    sections = {}
    for section, row in iter_sections(lines):
        if section not in sections:
            sections[section] = [row]
        else:
            sections[section].append(row)
    return sections


def collate_sections(lines):
    collated = {}
    for section, rows in collect_sections(lines).items():
        parent = collated
        current = ()
        for name in section:
            current += (name,)
            try:
                child, secrows, totalrows = parent[name]
            except KeyError:
                child = {}
                secrows = []
                totalrows = []
                parent[name] = (child, secrows, totalrows)
            parent = child
            if current == section:
                secrows.extend(rows)
            totalrows.extend(rows)
    return collated


#############################
# the commands

def cmd_count_by_section(lines):
    div = ' ' + '-' * 50
    total = 0
    def render_tree(root, depth=0):
        nonlocal total
        indent = '    ' * depth
        for name, data in root.items():
            subroot, rows, totalrows = data
            sectotal = f'({len(totalrows)})' if totalrows != rows else ''
            count = len(rows) if rows else ''
            if depth == 0:
                yield div
            yield f'{sectotal:>7} {count:>4}  {indent}{name}'
            yield from render_tree(subroot, depth+1)
            total += len(rows)
    sections = collate_sections(lines)
    yield from render_tree(sections)
    yield div
    yield f'(total: {total})'


#############################
# the script

def parse_args(argv=None, prog=None):
    import argparse
    parser = argparse.ArgumentParser(prog=prog)
    parser.add_argument('filename')

    args = parser.parse_args(argv)
    ns = vars(args)

    return ns


def main(filename):
    with open(filename) as infile:
        for line in cmd_count_by_section(infile):
            print(line)


if __name__ == '__main__':
    kwargs = parse_args()
    main(**kwargs)
