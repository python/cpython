"""Adjust set and frozenset methods in search index and general index."""

from sphinx import addnodes
from sphinx.domains.python import ObjectEntry

COMMON_METHODS = {
    "isdisjoint",
    "issubset",
    "issuperset",
    "union",
    "intersection",
    "difference",
    "symmetric_difference",
    "copy",
}

SET_ONLY_METHODS = {
    "update",
    "intersection_update",
    "difference_update",
    "symmetric_difference_update",
    "add",
    "remove",
    "discard",
    "pop",
    "clear",
}


def adjust_set_frozenset_method_anchors(app, doctree):
    """
    Change all frozenset.method references to be set.method instead
    (old anchors are kept around so as not to break links)
    """
    if app.env.path2doc(doctree['source']) != 'library/stdtypes':
        return

    # adjust anchors in the text, adding set-based anchors
    for desc in doctree.traverse(addnodes.desc):
        for sig in desc.traverse(addnodes.desc_signature):
            if not sig.get('fullname', '').startswith('frozenset.'):
                continue
            name = sig['fullname'].rsplit('.', 1)[-1]
            if name not in COMMON_METHODS and name not in SET_ONLY_METHODS:
                continue

            newname = f"set.{name}"
            for target in ('names', 'ids'):
                sig.get(target, []).insert(0, newname)
            sig['fullname'] = newname
            sig['class'] = "set"


def index_set_frozenset_methods(app, env):
    """
    Adjust search results for the built-in `set` and `frozenset` types so that
    mutation-oriented methods show up only under `set`, and other methods show
    up under both `set` and `frozenset`.

    Also adjust the entries in the general index.
    """
    domain = env.domains.get("py")

    # add set methods to search index and remove the frozenset-based search
    # results for those methods that frozenset does not support
    for name in COMMON_METHODS | SET_ONLY_METHODS:
        oldname = f"frozenset.{name}"
        if oldname in domain.objects:
            newname = f"set.{name}"
            getter = (
                domain.objects.pop
                if name in SET_ONLY_METHODS
                else domain.objects.__getitem__
            )
            docname, objname, objtype, alias = getter(oldname)
            domain.objects[newname] = ObjectEntry(
                docname, newname, objtype, False
            )

    # add set methods to general index and remove the frozenset-based entries
    # for those methods that frozenset does not support
    stddata = env.domaindata.get('index', {}).get('entries', None)
    if stddata is not None:
        for docname, entries in list(stddata.items()):
            new_entries = []
            for entry in entries:
                entrytype, entryname, target, ignored, key = entry
                if (
                    target.startswith('frozenset.')
                    and '(frozenset method)' in entryname
                ):
                    method = target.removeprefix('frozenset.')
                    new_entries.append((
                        entrytype,
                        entryname.replace(
                            '(frozenset method)', '(set method)'
                        ),
                        f"set.{method}",
                        ignored,
                        key,
                    ))
                    if method not in SET_ONLY_METHODS:
                        new_entries.append(entry)
                else:
                    new_entries.append(entry)

            stddata[docname] = new_entries


def setup(app):
    app.connect("doctree-read", adjust_set_frozenset_method_anchors)
    app.connect("env-updated", index_set_frozenset_methods)
    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
