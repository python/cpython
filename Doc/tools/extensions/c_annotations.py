# -*- coding: utf-8 -*-
"""
    c_annotations.py
    ~~~~~~~~~~~~~~~~

    Supports annotations for C API elements:

    * reference count annotations for C API functions.  Based on
      refcount.py and anno-api.py in the old Python documentation tools.

    * stable API annotations

    Usage:
    * Set the `refcount_file` config value to the path to the reference
    count data file.
    * Set the `stable_abi_file` config value to the path to stable ABI list.

    :copyright: Copyright 2007-2014 by Georg Brandl.
    :license: Python license.
"""

from os import path
from docutils import nodes
from docutils.parsers.rst import directives
import csv

from sphinx import addnodes
from sphinx.domains.c import CObject


class RCEntry:
    def __init__(self, name):
        self.name = name
        self.args = []
        self.result_type = ''
        self.result_refs = None


class Annotations:
    def __init__(self, refcount_filename, stable_abi_file):
        self.refcount_data = {}
        with open(refcount_filename, 'r') as fp:
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
                    entry = self.refcount_data[function]
                except KeyError:
                    entry = self.refcount_data[function] = RCEntry(function)
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

        self.stable_abi_data = {}
        with open(stable_abi_file, 'r') as fp:
            for record in csv.DictReader(fp):
                role = record['role']
                name = record['name']
                self.stable_abi_data.setdefault(role, {})[name] = record

    def add_annotations(self, app, doctree):
        for node in doctree.traverse(addnodes.desc_content):
            par = node.parent
            if par['domain'] != 'c':
                continue
            if not par[0].has_key('ids') or not par[0]['ids']:
                continue
            name = par[0]['ids'][0]
            if name.startswith("c."):
                name = name[2:]

            objtype = par['objtype']

            # Stable ABI annotation. These have two forms:
            #   Part of the [Stable ABI](link).
            #   Part of the [Stable ABI](link) since version X.Y.
            record = self.stable_abi_data.get(objtype, {}).get(name)
            if record:
                stable_added = record['added']
                message = ' Part of the '
                emph_node = nodes.emphasis(message, message,
                                           classes=['stableabi'])
                ref_node = addnodes.pending_xref(
                    'Stable ABI', refdomain="std", reftarget='stable',
                    reftype='ref', refexplicit="False")
                ref_node += nodes.Text('Stable ABI')
                emph_node += ref_node
                if record['ifdef_note']:
                    emph_node += nodes.Text(' ' + record['ifdef_note'])
                if stable_added == '3.2':
                    # Stable ABI was introduced in 3.2.
                    emph_node += nodes.Text('.')
                else:
                    emph_node += nodes.Text(f' since version {stable_added}.')
                node.insert(0, emph_node)

            # Return value annotation
            if objtype != 'function':
                continue
            entry = self.refcount_data.get(name)
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
    refcounts = Annotations(
        path.join(app.srcdir, app.config.refcount_file),
        path.join(app.srcdir, app.config.stable_abi_file),
    )
    app.connect('doctree-read', refcounts.add_annotations)


def setup(app):
    app.add_config_value('refcount_file', '', True)
    app.add_config_value('stable_abi_file', '', True)
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
