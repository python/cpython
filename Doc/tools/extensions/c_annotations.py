# -*- coding: utf-8 -*-
"""
    c_annotations.py
    ~~~~~~~~~~~~~~~~

    Supports annotations for C API elements:

    * reference count annotations for C API functions.  Based on
      refcount.py and anno-api.py in the old Python documentation tools.

    * stable API annotations

    Usage: Set the `refcount_file` config value to the path to the reference
    count data file.

    :copyright: Copyright 2007-2014 by Georg Brandl.
    :license: Python license.
"""

from os import path
from docutils import nodes
from docutils.parsers.rst import directives

from sphinx import addnodes
from sphinx.domains.c import CObject


class RCEntry:
    def __init__(self, name):
        self.name = name
        self.args = []
        self.result_type = ''
        self.result_refs = None


class Annotations(dict):
    @classmethod
    def fromfile(cls, filename):
        d = cls()
        fp = open(filename, 'r')
        try:
            for line in fp:
                line = line.strip()
                if line[:1] in ("", "#"):
                    # blank lines and comments
                    continue
                parts = line.split(":", 4)
                if len(parts) != 5:
                    raise ValueError("Wrong field count in %r" % line)
                function, type, arg, refcount, comment = parts
                # Get the entry, creating it if needed:
                try:
                    entry = d[function]
                except KeyError:
                    entry = d[function] = RCEntry(function)
                if not refcount or refcount == "null":
                    refcount = None
                else:
                    refcount = int(refcount)
                # Update the entry with the new parameter or the result
                # information.
                if arg:
                    entry.args.append((arg, type, refcount))
                else:
                    entry.result_type = type
                    entry.result_refs = refcount
        finally:
            fp.close()
        return d

    def add_annotations(self, app, doctree):
        for node in doctree.traverse(addnodes.desc_content):
            par = node.parent
            if par['domain'] != 'c':
                continue
            if par['stableabi']:
                node.insert(0, nodes.emphasis(' Part of the stable ABI.',
                                              ' Part of the stable ABI.',
                                              classes=['stableabi']))
            if par['objtype'] != 'function':
                continue
            if not par[0].has_key('names') or not par[0]['names']:
                continue
            name = par[0]['names'][0]
            if name.startswith("c."):
                name = name[2:]
            entry = self.get(name)
            if not entry:
                continue
            elif not entry.result_type.endswith("Object*"):
                continue
            if entry.result_refs is None:
                rc = 'Return value: Always NULL.'
            elif entry.result_refs:
                rc = 'Return value: New reference.'
            else:
                rc = 'Return value: Borrowed reference.'
            node.insert(0, nodes.emphasis(rc, rc, classes=['refcount']))


def init_annotations(app):
    refcounts = Annotations.fromfile(
        path.join(app.srcdir, app.config.refcount_file))
    app.connect('doctree-read', refcounts.add_annotations)


def setup(app):
    app.add_config_value('refcount_file', '', True)
    app.connect('builder-inited', init_annotations)

    # monkey-patch C object...
    CObject.option_spec = {
        'noindex': directives.flag,
        'stableabi': directives.flag,
    }
    old_handle_signature = CObject.handle_signature
    def new_handle_signature(self, sig, signode):
        signode.parent['stableabi'] = 'stableabi' in self.options
        return old_handle_signature(self, sig, signode)
    CObject.handle_signature = new_handle_signature
    return {'version': '1.0', 'parallel_read_safe': True}
