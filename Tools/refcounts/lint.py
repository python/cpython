"""Lint Doc/data/refcounts.dat."""

from __future__ import annotations

import enum
import itertools
import re
import tomllib
from argparse import ArgumentParser, RawDescriptionHelpFormatter
from dataclasses import dataclass, field
from pathlib import Path
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Callable, Iterable, Mapping
    from typing import Final, LiteralString

ROOT = Path(__file__).parent.parent.parent.resolve()
DEFAULT_REFCOUNT_DAT_PATH: Final[str] = str(ROOT / "Doc/data/refcounts.dat")
DEFAULT_STABLE_ABI_TOML_PATH: Final[str] = str(ROOT / "Misc/stable_abi.toml")

C_ELLIPSIS: LiteralString = "..."

MATCH_TODO: Callable[[str], re.Match[str] | None]
MATCH_TODO = re.compile(r"^#\s*TODO:\s*(\w+)$").match

OBJECT_TYPES: frozenset[str] = frozenset()

for qualifier, object_type, suffix in itertools.product(
    ("const ", ""),
    (
        "PyObject", "PyLongObject", "PyTypeObject",
        "PyCodeObject", "PyFrameObject", "PyModuleObject",
        "PyVarObject",
    ),
    (
        "*",
        "**", "* *",
        "*const*", "*const *", "* const*", "* const *",
    ),
):
    OBJECT_TYPES |= {
        f"{qualifier}{object_type}{suffix}",
        f"{qualifier}{object_type} {suffix}",
    }
del suffix, object_type, qualifier

IGNORE_LIST: frozenset[str] = frozenset((
    # part of the stable ABI but should not be used at all
    "PyUnicode_GetSize",
    # part of the stable ABI but completely removed
    "_PyState_AddModule",
))


def flno_(lineno: int) -> str:
    # Format the line so that users can C/C from the terminal
    # the line number and jump with their editor using Ctrl+G.
    return f"{lineno:>5} "


def is_c_parameter_name(name: str) -> bool:
    """Check if *name* is a valid C parameter name."""
    # C accepts the same identifiers as in Python
    return name == C_ELLIPSIS or name.isidentifier()


class RefEffect(enum.Enum):
    UNKNOWN = enum.auto()
    UNUSED = enum.auto()
    DECREF = enum.auto()
    INCREF = enum.auto()
    BORROW = enum.auto()
    STEAL = enum.auto()
    NULL = enum.auto()  # for return values only


@dataclass(slots=True, frozen=True, kw_only=True)
class LineInfo:
    func: str
    ctype: str | None
    name: str | None
    effect: RefEffect
    comment: str

    raw_func: str
    raw_ctype: str
    raw_name: str
    raw_effect: str

    strip_func: bool
    strip_ctype: bool
    strip_name: bool
    strip_effect: bool


@dataclass(slots=True, frozen=True)
class Return:
    ctype: str | None
    effect: RefEffect
    comment: str
    lineno: int = field(kw_only=True)


@dataclass(slots=True, frozen=True)
class Param:
    name: str

    ctype: str | None
    effect: RefEffect
    comment: str

    lineno: int = field(kw_only=True)


@dataclass(slots=True, frozen=True)
class Signature:
    name: str
    rparam: Return
    params: dict[str, Param] = field(default_factory=dict)
    lineno: int = field(kw_only=True)


@dataclass(slots=True, frozen=True)
class FileView:
    signatures: Mapping[str, Signature]
    incomplete: frozenset[str]


def parse_line(line: str) -> LineInfo | None:
    parts = line.split(":", maxsplit=4)
    if len(parts) != 5:
        return None

    raw_func, raw_ctype, raw_name, raw_effect, comment = parts

    func = raw_func.strip()
    strip_func = func != raw_func
    if not func:
        return None

    clean_ctype = raw_ctype.strip()
    ctype = clean_ctype or None
    strip_ctype = clean_ctype != raw_ctype

    clean_name = raw_name.strip()
    name = clean_name or None
    strip_name = clean_name != raw_name

    clean_effect = raw_effect.strip()
    strip_effect = clean_effect != raw_effect

    match clean_effect:
        case "-1":
            effect = RefEffect.DECREF
        case "+1":
            effect = RefEffect.INCREF
        case "0":
            effect = RefEffect.BORROW
        case "$":
            effect = RefEffect.STEAL
        case "null":
            effect = RefEffect.NULL
        case "":
            effect = RefEffect.UNUSED
        case _:
            effect = RefEffect.UNKNOWN

    comment = comment.strip()
    return LineInfo(
        func=func, ctype=ctype, name=name, effect=effect, comment=comment,
        raw_func=raw_func, raw_ctype=raw_ctype,
        raw_name=raw_name, raw_effect=raw_effect,
        strip_func=strip_func, strip_ctype=strip_ctype,
        strip_name=strip_name, strip_effect=strip_effect,
    )


@dataclass(slots=True)
class ParserReporter:
    count: int = 0

    def info(self, lineno: int, message: str) -> None:
        self.count += 1
        print(f"{flno_(lineno)} {message}")

    def warn(self, lineno: int, message: str) -> None:
        # same as info() but semantically different
        self.info(lineno, message)

    def error(self, lineno: int, message: str) -> None:
        # same as info() but semantically different
        self.info(lineno, message)


def parse(lines: Iterable[str]) -> FileView:
    signatures: dict[str, Signature] = {}
    incomplete: set[str] = set()

    w = ParserReporter()

    for lineno, line in enumerate(map(str.strip, lines), 1):
        if not line:
            continue
        if line.startswith("#"):
            if match := MATCH_TODO(line):
                incomplete.add(match.group(1))
            continue

        e = parse_line(line)
        if e is None:
            w.error(lineno, f"cannot parse: {line!r}")
            continue

        if e.strip_func:
            w.warn(lineno, f"[func] whitespaces around {e.raw_func!r}")
        if e.strip_ctype:
            w.warn(lineno, f"[type] whitespaces around {e.raw_ctype!r}")
        if e.strip_name:
            w.warn(lineno, f"[name] whitespaces around {e.raw_name!r}")
        if e.strip_effect:
            w.warn(lineno, f"[ref] whitespaces around {e.raw_effect!r}")

        func, name = e.func, e.name
        ctype, effect = e.ctype, e.effect
        comment = e.comment

        if func not in signatures:
            # process return value
            if name is not None:
                w.warn(lineno, f"named return value in {line!r}")
            ret_param = Return(ctype, effect, comment, lineno=lineno)
            signatures[func] = Signature(func, ret_param, lineno=lineno)
        else:
            # process parameter
            if name is None:
                w.error(lineno, f"missing parameter name in {line!r}")
                continue
            sig: Signature = signatures[func]
            params = sig.params
            if name in params:
                w.error(lineno, f"duplicated parameter name in {line!r}")
                continue
            params[name] = Param(name, ctype, effect, comment, lineno=lineno)

    if w.count:
        print()
        print(f"Found {w.count} issue(s)")

    return FileView(signatures, frozenset(incomplete))


@dataclass(slots=True)
class CheckerWarnings:
    count: int = 0

    def block(self, sig: Signature, message: str) -> None:
        self.count += 1
        print(f"{flno_(sig.lineno)} {sig.name:50} {message}")

    def param(self, sig: Signature, param: Param, message: str) -> None:
        self.count += 1
        fullname = f"{sig.name}[{param.name}]"
        print(f"{flno_(param.lineno)} {fullname:50} {message}")


def check(view: FileView) -> None:
    w = CheckerWarnings()

    for sig in view.signatures.values():  # type: Signature
        # check the return value
        rparam = sig.rparam
        if not rparam.ctype:
            w.block(sig, "missing return value type")
        if rparam.effect is RefEffect.UNKNOWN:
            w.block(sig, "unknown return value type")
        # check the parameters
        for param in sig.params.values():  # type: Param
            ctype, effect = param.ctype, param.effect
            if ctype in OBJECT_TYPES and effect is RefEffect.UNUSED:
                w.param(sig, param, "missing reference count management")
            if ctype not in OBJECT_TYPES and effect is not RefEffect.UNUSED:
                w.param(sig, param, "unused reference count management")
            if not is_c_parameter_name(param.name):
                w.param(sig, param, "invalid parameter name")

    if w.count:
        print()
        print(f"Found {w.count} issue(s)")
    names = view.signatures.keys()
    if sorted(names) != list(names):
        print("Entries are not sorted")


def check_structure(view: FileView, stable_abi_file: str) -> None:
    print(f"Stable ABI file: {stable_abi_file}")
    print()
    stable_abi_str = Path(stable_abi_file).read_text(encoding="utf-8")
    stable_abi = tomllib.loads(stable_abi_str)
    expect = stable_abi["function"].keys()
    # check if there are missing entries (those marked as "TODO" are ignored)
    actual = IGNORE_LIST | view.incomplete | view.signatures.keys()
    if missing := (expect - actual):
        print(f"Missing {len(missing)} stable ABI entries:")
        for name in sorted(missing):
            print(name)


_STABLE_ABI_PATH_SENTINEL: Final = object()


def _create_parser() -> ArgumentParser:
    parser = ArgumentParser(
        prog="lint.py", formatter_class=RawDescriptionHelpFormatter,
        description=(
            "Lint the refcounts.dat file.\n\n"
            "Use --abi or --abi=FILE to check against the stable ABI."
        ),
    )
    _ = parser.add_argument
    _("file", nargs="?", default=DEFAULT_REFCOUNT_DAT_PATH,
      help="the refcounts.dat file to check (default: %(default)s)")
    _("--abi", nargs="?", default=_STABLE_ABI_PATH_SENTINEL,
      help=(f"check against the given stable_abi.toml file "
            f"(default: {DEFAULT_STABLE_ABI_TOML_PATH})"))
    return parser


def main() -> None:
    parser = _create_parser()
    args = parser.parse_args()
    lines = Path(args.file).read_text(encoding="utf-8").splitlines()
    print(" PARSING ".center(80, "-"))
    view = parse(lines)
    print(" CHECKING ".center(80, "-"))
    check(view)
    if args.abi is not _STABLE_ABI_PATH_SENTINEL:
        abi = args.abi or DEFAULT_STABLE_ABI_TOML_PATH
        print(" CHECKING STABLE ABI ".center(80, "-"))
        check_structure(view, abi)


if __name__ == "__main__":
    main()
