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
from docutils.parsers.rst import Directive
from docutils.statemachine import StringList
from sphinx.locale import _ as sphinx_gettext
import csv

from sphinx import addnodes
from sphinx.domains.c import CObject


REST_ROLE_MAP = {
    'function': 'func',
    'macro': 'macro',
    'member': 'member',
    'type': 'type',
    'var': 'data',
}


class RCEntry:
    def __init__(self, name):
        self.name = name
        self.args = []
        self.result_type = ''
        self.result_refs = None


class Annotations:
    def __init__(self, refcount_filename, stable_abi_file):
        self.refcount_data = {}
        with open(refcount_filename, encoding='utf8') as fp:
            for line in fp:
                line = line.strip()
                if line[:1] in ("", "#"):
                    # blank lines and comments
                    continue
                parts = line.split(":", 4)
                if len(parts) != 5:
                    raise ValueError(f"Wrong field count in {line!r}")
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
        with open(stable_abi_file, encoding='utf8') as fp:
            for record in csv.DictReader(fp):
                name = record['name']
                self.stable_abi_data[name] = record

    def add_annotations(self, app, doctree):
        for node in doctree.findall(addnodes.desc_content):
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
            # For structs, there's some more info in the message:
            #   Part of the [Limited API](link) (as an opaque struct).
            #   Part of the [Stable ABI](link) (including all members).
            #   Part of the [Limited API](link) (Only some members are part
            #       of the stable ABI.).
            # ... all of which can have "since version X.Y" appended.
            record = self.stable_abi_data.get(name)
            if record:
                if record['role'] != objtype:
                    raise ValueError(
                        f"Object type mismatch in limited API annotation "
                        f"for {name}: {record['role']!r} != {objtype!r}")
                stable_added = record['added']
                message = sphinx_gettext('Part of the')
                message = message.center(len(message) + 2)
                emph_node = nodes.emphasis(message, message,
                                           classes=['stableabi'])
                ref_node = addnodes.pending_xref(
                    'Stable ABI', refdomain="std", reftarget='stable',
                    reftype='ref', refexplicit="False")
                struct_abi_kind = record['struct_abi_kind']
                if struct_abi_kind in {'opaque', 'members'}:
                    ref_node += nodes.Text(sphinx_gettext('Limited API'))
                else:
                    ref_node += nodes.Text(sphinx_gettext('Stable ABI'))
                emph_node += ref_node
                if struct_abi_kind == 'opaque':
                    emph_node += nodes.Text(' ' + sphinx_gettext('(as an opaque struct)'))
                elif struct_abi_kind == 'full-abi':
                    emph_node += nodes.Text(' ' + sphinx_gettext('(including all members)'))
                if record['ifdef_note']:
                    emph_node += nodes.Text(' ' + record['ifdef_note'])
                if stable_added == '3.2':
                    # Stable ABI was introduced in 3.2.
                    pass
                else:
                    emph_node += nodes.Text(' ' + sphinx_gettext('since version %s') % stable_added)
                emph_node += nodes.Text('.')
                if struct_abi_kind == 'members':
                    emph_node += nodes.Text(
                        ' ' + sphinx_gettext('(Only some members are part of the stable ABI.)'))
                node.insert(0, emph_node)

            # Unstable API annotation.
            if name.startswith('PyUnstable'):
                warn_node = nodes.admonition(
                    classes=['unstable-c-api', 'warning'])
                message = sphinx_gettext('This is') + ' '
                emph_node = nodes.emphasis(message, message)
                ref_node = addnodes.pending_xref(
                    'Unstable API', refdomain="std",
                    reftarget='unstable-c-api',
                    reftype='ref', refexplicit="False")
                ref_node += nodes.Text(sphinx_gettext('Unstable API'))
                emph_node += ref_node
                emph_node += nodes.Text(sphinx_gettext('. It may change without warning in minor releases.'))
                warn_node += emph_node
                node.insert(0, warn_node)

            # Return value annotation
            if objtype != 'function':
                continue
            entry = self.refcount_data.get(name)
            if not entry:
                continue
            elif not entry.result_type.endswith("Object*"):
                continue
            classes = ['refcount']
            if entry.result_refs is None:
                rc = sphinx_gettext('Return value: Always NULL.')
                classes.append('return_null')
            elif entry.result_refs:
                rc = sphinx_gettext('Return value: New reference.')
                classes.append('return_new_ref')
            else:
                rc = sphinx_gettext('Return value: Borrowed reference.')
                classes.append('return_borrowed_ref')
            node.insert(0, nodes.emphasis(rc, rc, classes=classes))


def init_annotations(app):
    annotations = Annotations(
        path.join(app.srcdir, app.config.refcount_file),
        path.join(app.srcdir, app.config.stable_abi_file),
    )
    app.connect('doctree-read', annotations.add_annotations)

    class LimitedAPIList(Directive):

        has_content = False
        required_arguments = 0
        optional_arguments = 0
        final_argument_whitespace = True

        def run(self):
            content = []
            for record in annotations.stable_abi_data.values():
                role = REST_ROLE_MAP[record['role']]
                name = record['name']
                content.append(f'* :c:{role}:`{name}`')

            pnode = nodes.paragraph()
            self.state.nested_parse(StringList(content), 0, pnode)
            return [pnode]

    app.add_directive('limited-api-list', LimitedAPIList)


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
