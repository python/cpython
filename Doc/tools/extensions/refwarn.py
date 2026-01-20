"""Capture and report missing cross-references in the documentation."""

import csv
import os.path
from collections import Counter


def _repr(obj, /):
    """Return the repr without the enclosing quotation marks."""
    return repr(obj)[1:-1]


def get_node_source(node):
    """Return the source file for 'node'."""
    while node:
        if node.source:
            return node.source
        node = node.parent
    return "<unknown>"


def warn_missing_reference(app, domain, node):
    warnfile = os.path.join(app.outdir, 'refwarn.csv')

    type_ = _repr(node['reftype'])
    target = _repr(node['reftarget'])
    domain = _repr(node.get('refdomain', 'std'))
    source = os.path.relpath(get_node_source(node), app.srcdir)
    with open(warnfile, 'a', encoding='utf-8') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow((domain, type_, target, source))
    return False


def summarise_warnings(app, exception):
    warnfile = os.path.join(app.outdir, 'refwarn.csv')
    warnfile_count = os.path.join(app.outdir, 'refwarn_count.csv')

    with open(warnfile, 'r', encoding='utf-8') as f:
        r = csv.reader(f, lineterminator='\n')
        next(r, None)  # Skip the header
        lines = [tuple(line[:3]) for line in r]
    prevalence = Counter(lines).most_common()
    with open(warnfile_count, 'w', encoding='utf-8') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(('Count', 'Domain', 'Type', 'Target'))
        w.writerows(((count,) + tpl for tpl, count in prevalence))


def setup(app):
    warnfile = os.path.join(app.outdir, 'refwarn.csv')
    os.makedirs(app.outdir, exist_ok=True)

    with open(warnfile, 'w', encoding='utf-8') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(('Domain', 'Type', 'Target', 'Source'))
    app.connect('warn-missing-reference', warn_missing_reference)
    app.connect('build-finished', summarise_warnings)
    return {'version': '1', 'parallel_read_safe': True, 'parallel_write_safe': True}
