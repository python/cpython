"""Capture and report missing cross-references in the documentation."""

import os.path
from collections import Counter


def _rpr(obj, /):
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

    typ = _rpr(node['reftype'])
    target = _rpr(node['reftarget'])
    domain = _rpr(node.get('refdomain', 'std'))
    source = os.path.relpath(get_node_source(node), app.srcdir)
    with open(warnfile, 'a', encoding='utf-8') as f:
        f.write(f"{domain},{typ},{target},{source}\n")
    return False


def summarise_warnings(app, exception):
    warnfile = os.path.join(app.outdir, 'refwarn.csv')
    warnfile_count = os.path.join(app.outdir, 'refwarn_count.csv')

    with open(warnfile, 'r', encoding='utf-8') as f:
        lines = [tuple(line.strip().split(',')[:3]) for line in f]
    prevalence = Counter(lines).most_common()
    with open(warnfile_count, 'w', encoding='utf-8') as f:
        f.write(f"Count,Domain,Type,Target\n")
        for (tpl, count) in prevalence:
            f.write(f"{count},{','.join(tpl)}\n")


def setup(app):
    warnfile = os.path.join(app.outdir, 'refwarn.csv')

    with open(warnfile, 'w', encoding='utf-8') as f:
        f.write(f"Domain,Type,Target,Source\n")
    app.connect('warn-missing-reference', warn_missing_reference)
    app.connect('build-finished', summarise_warnings)
