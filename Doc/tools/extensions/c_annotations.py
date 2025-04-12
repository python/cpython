"""Support annotations for C API elements.

* Reference count annotations for C API functions.
* Stable ABI annotations
* Limited API annotations

Configuration:
* Set ``refcount_file`` to the path to the reference count data file.
* Set ``stable_abi_file`` to the path to stable ABI list.
"""

from __future__ import annotations

import csv
import dataclasses
from pathlib import Path
from typing import TYPE_CHECKING

from docutils import nodes
from docutils.statemachine import StringList
from sphinx import addnodes
from sphinx.locale import _ as sphinx_gettext
from sphinx.util.docutils import SphinxDirective

if TYPE_CHECKING:
    from sphinx.application import Sphinx
    from sphinx.util.typing import ExtensionMetadata

ROLE_TO_OBJECT_TYPE = {
    "func": "function",
    "macro": "macro",
    "member": "member",
    "type": "type",
    "data": "var",
}


@dataclasses.dataclass(slots=True)
class RefCountEntry:
    # Name of the function.
    name: str
    # List of (argument name, type, refcount effect) tuples.
    # (Currently not used. If it was, a dataclass might work better.)
    args: list = dataclasses.field(default_factory=list)
    # Return type of the function.
    result_type: str = ""
    # Reference count effect for the return value.
    result_refs: int | None = None


@dataclasses.dataclass(frozen=True, slots=True)
class StableABIEntry:
    # Role of the object.
    # Source: Each [item_kind] in stable_abi.toml is mapped to a C Domain role.
    role: str
    # Name of the object.
    # Source: [<item_kind>.*] in stable_abi.toml.
    name: str
    # Version when the object was added to the stable ABI.
    # (Source: [<item_kind>.*.added] in stable_abi.toml.
    added: str
    # An explananatory blurb for the ifdef.
    # Source: ``feature_macro.*.doc`` in stable_abi.toml.
    ifdef_note: str
    # Defines how much of the struct is exposed. Only relevant for structs.
    # Source: [<item_kind>.*.struct_abi_kind] in stable_abi.toml.
    struct_abi_kind: str


def read_refcount_data(refcount_filename: Path) -> dict[str, RefCountEntry]:
    refcount_data = {}
    refcounts = refcount_filename.read_text(encoding="utf8")
    for line in refcounts.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            # blank lines and comments
            continue

        # Each line is of the form
        # function ':' type ':' [param name] ':' [refcount effect] ':' [comment]
        parts = line.split(":", 4)
        if len(parts) != 5:
            raise ValueError(f"Wrong field count in {line!r}")
        function, type, arg, refcount, _comment = parts

        # Get the entry, creating it if needed:
        try:
            entry = refcount_data[function]
        except KeyError:
            entry = refcount_data[function] = RefCountEntry(function)
        if not refcount or refcount == "null":
            refcount = None
        else:
            refcount = int(refcount)
        # Update the entry with the new parameter
        # or the result information.
        if arg:
            entry.args.append((arg, type, refcount))
        else:
            entry.result_type = type
            entry.result_refs = refcount

    return refcount_data


def read_stable_abi_data(stable_abi_file: Path) -> dict[str, StableABIEntry]:
    stable_abi_data = {}
    with open(stable_abi_file, encoding="utf8") as fp:
        for record in csv.DictReader(fp):
            name = record["name"]
            stable_abi_data[name] = StableABIEntry(**record)

    return stable_abi_data


def add_annotations(app: Sphinx, doctree: nodes.document) -> None:
    state = app.env.domaindata["c_annotations"]
    refcount_data = state["refcount_data"]
    stable_abi_data = state["stable_abi_data"]
    for node in doctree.findall(addnodes.desc_content):
        par = node.parent
        if par["domain"] != "c":
            continue
        if not par[0].get("ids", None):
            continue
        name = par[0]["ids"][0].removeprefix("c.")
        objtype = par["objtype"]

        # Stable ABI annotation.
        if record := stable_abi_data.get(name):
            if ROLE_TO_OBJECT_TYPE[record.role] != objtype:
                msg = (
                    f"Object type mismatch in limited API annotation for {name}: "
                    f"{ROLE_TO_OBJECT_TYPE[record.role]!r} != {objtype!r}"
                )
                raise ValueError(msg)
            annotation = _stable_abi_annotation(record)
            node.insert(0, annotation)

        # Unstable API annotation.
        if name.startswith("PyUnstable"):
            annotation = _unstable_api_annotation()
            node.insert(0, annotation)

        # Return value annotation
        if objtype != "function":
            continue
        if name not in refcount_data:
            continue
        entry = refcount_data[name]
        if not entry.result_type.endswith("Object*"):
            continue
        annotation = _return_value_annotation(entry.result_refs)
        node.insert(0, annotation)


def _stable_abi_annotation(record: StableABIEntry) -> nodes.emphasis:
    """Create the Stable ABI annotation.

    These have two forms:
      Part of the `Stable ABI <link>`_.
      Part of the `Stable ABI <link>`_ since version X.Y.
    For structs, there's some more info in the message:
      Part of the `Limited API <link>`_ (as an opaque struct).
      Part of the `Stable ABI <link>`_ (including all members).
      Part of the `Limited API <link>`_ (Only some members are part
          of the stable ABI.).
    ... all of which can have "since version X.Y" appended.
    """
    stable_added = record.added
    message = sphinx_gettext("Part of the")
    message = message.center(len(message) + 2)
    emph_node = nodes.emphasis(message, message, classes=["stableabi"])
    ref_node = addnodes.pending_xref(
        "Stable ABI",
        refdomain="std",
        reftarget="stable",
        reftype="ref",
        refexplicit="False",
    )
    struct_abi_kind = record.struct_abi_kind
    if struct_abi_kind in {"opaque", "members"}:
        ref_node += nodes.Text(sphinx_gettext("Limited API"))
    else:
        ref_node += nodes.Text(sphinx_gettext("Stable ABI"))
    emph_node += ref_node
    if struct_abi_kind == "opaque":
        emph_node += nodes.Text(" " + sphinx_gettext("(as an opaque struct)"))
    elif struct_abi_kind == "full-abi":
        emph_node += nodes.Text(
            " " + sphinx_gettext("(including all members)")
        )
    if record.ifdef_note:
        emph_node += nodes.Text(f" {record.ifdef_note}")
    if stable_added == "3.2":
        # Stable ABI was introduced in 3.2.
        pass
    else:
        emph_node += nodes.Text(
            " " + sphinx_gettext("since version %s") % stable_added
        )
    emph_node += nodes.Text(".")
    if struct_abi_kind == "members":
        msg = " " + sphinx_gettext(
            "(Only some members are part of the stable ABI.)"
        )
        emph_node += nodes.Text(msg)
    return emph_node


def _unstable_api_annotation() -> nodes.admonition:
    ref_node = addnodes.pending_xref(
        "Unstable API",
        nodes.Text(sphinx_gettext("Unstable API")),
        refdomain="std",
        reftarget="unstable-c-api",
        reftype="ref",
        refexplicit="False",
    )
    emph_node = nodes.emphasis(
        "This is ",
        sphinx_gettext("This is") + " ",
        ref_node,
        nodes.Text(
            sphinx_gettext(
                ". It may change without warning in minor releases."
            )
        ),
    )
    return nodes.admonition(
        "",
        emph_node,
        classes=["unstable-c-api", "warning"],
    )


def _return_value_annotation(result_refs: int | None) -> nodes.emphasis:
    classes = ["refcount"]
    if result_refs is None:
        rc = sphinx_gettext("Return value: Always NULL.")
        classes.append("return_null")
    elif result_refs:
        rc = sphinx_gettext("Return value: New reference.")
        classes.append("return_new_ref")
    else:
        rc = sphinx_gettext("Return value: Borrowed reference.")
        classes.append("return_borrowed_ref")
    return nodes.emphasis(rc, rc, classes=classes)


class LimitedAPIList(SphinxDirective):
    has_content = False
    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = True

    def run(self) -> list[nodes.Node]:
        state = self.env.domaindata["c_annotations"]
        content = [
            f"* :c:{record.role}:`{record.name}`"
            for record in state["stable_abi_data"].values()
        ]
        node = nodes.paragraph()
        self.state.nested_parse(StringList(content), 0, node)
        return [node]


def init_annotations(app: Sphinx) -> None:
    # Using domaindata is a bit hack-ish,
    # but allows storing state without a global variable or closure.
    app.env.domaindata["c_annotations"] = state = {}
    state["refcount_data"] = read_refcount_data(
        Path(app.srcdir, app.config.refcount_file)
    )
    state["stable_abi_data"] = read_stable_abi_data(
        Path(app.srcdir, app.config.stable_abi_file)
    )


def setup(app: Sphinx) -> ExtensionMetadata:
    app.add_config_value("refcount_file", "", "env", types={str})
    app.add_config_value("stable_abi_file", "", "env", types={str})
    app.add_directive("limited-api-list", LimitedAPIList)
    app.connect("builder-inited", init_annotations)
    app.connect("doctree-read", add_annotations)

    return {
        "version": "1.0",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
