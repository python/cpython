import contextlib
import re
import typing
from collections.abc import Iterator

from parsing import StackEffect, Family

UNUSED = "unused"


class Formatter:
    """Wraps an output stream with the ability to indent etc."""

    stream: typing.TextIO
    prefix: str
    emit_line_directives: bool = False
    lineno: int  # Next line number, 1-based
    filename: str  # Slightly improved stream.filename
    nominal_lineno: int
    nominal_filename: str

    def __init__(
        self,
        stream: typing.TextIO,
        indent: int,
        emit_line_directives: bool = False,
        comment: str = "//",
    ) -> None:
        self.stream = stream
        self.prefix = " " * indent
        self.emit_line_directives = emit_line_directives
        self.comment = comment
        self.lineno = 1
        self.filename = prettify_filename(self.stream.name)
        self.nominal_lineno = 1
        self.nominal_filename = self.filename

    def write_raw(self, s: str) -> None:
        self.stream.write(s)
        newlines = s.count("\n")
        self.lineno += newlines
        self.nominal_lineno += newlines

    def emit(self, arg: str) -> None:
        if arg:
            self.write_raw(f"{self.prefix}{arg}\n")
        else:
            self.write_raw("\n")

    def set_lineno(self, lineno: int, filename: str) -> None:
        if self.emit_line_directives:
            if lineno != self.nominal_lineno or filename != self.nominal_filename:
                self.emit(f'#line {lineno} "{filename}"')
                self.nominal_lineno = lineno
                self.nominal_filename = filename

    def reset_lineno(self) -> None:
        if self.lineno != self.nominal_lineno or self.filename != self.nominal_filename:
            self.set_lineno(self.lineno + 1, self.filename)

    @contextlib.contextmanager
    def indent(self) -> Iterator[None]:
        self.prefix += "    "
        yield
        self.prefix = self.prefix[:-4]

    @contextlib.contextmanager
    def block(self, head: str, tail: str = "") -> Iterator[None]:
        if head:
            self.emit(head + " {")
        else:
            self.emit("{")
        with self.indent():
            yield
        self.emit("}" + tail)

    def stack_adjust(
        self,
        input_effects: list[StackEffect],
        output_effects: list[StackEffect],
    ) -> None:
        shrink, isym = list_effect_size(input_effects)
        grow, osym = list_effect_size(output_effects)
        diff = grow - shrink
        if isym and isym != osym:
            self.emit(f"STACK_SHRINK({isym});")
        if diff < 0:
            self.emit(f"STACK_SHRINK({-diff});")
        if diff > 0:
            self.emit(f"STACK_GROW({diff});")
        if osym and osym != isym:
            self.emit(f"STACK_GROW({osym});")

    def declare(self, dst: StackEffect, src: StackEffect | None) -> None:
        if dst.name == UNUSED or dst.cond == "0":
            return
        typ = f"{dst.type}" if dst.type else "PyObject *"
        if src:
            cast = self.cast(dst, src)
            initexpr = f"{cast}{src.name}"
            if src.cond and src.cond != "1":
                initexpr = f"{parenthesize_cond(src.cond)} ? {initexpr} : NULL"
            init = f" = {initexpr}"
        elif dst.cond and dst.cond != "1":
            init = " = NULL"
        else:
            init = ""
        sepa = "" if typ.endswith("*") else " "
        self.emit(f"{typ}{sepa}{dst.name}{init};")

    def assign(self, dst: StackEffect, src: StackEffect) -> None:
        if src.name == UNUSED or dst.name == UNUSED:
            return
        cast = self.cast(dst, src)
        if re.match(r"^REG\(oparg(\d+)\)$", dst.name):
            self.emit(f"Py_XSETREF({dst.name}, {cast}{src.name});")
        else:
            stmt = f"{dst.name} = {cast}{src.name};"
            if src.cond and src.cond != "1":
                if src.cond == "0":
                    # It will not be executed
                    return
                stmt = f"if ({src.cond}) {{ {stmt} }}"
            self.emit(stmt)

    def cast(self, dst: StackEffect, src: StackEffect) -> str:
        return f"({dst.type or 'PyObject *'})" if src.type != dst.type else ""

    def static_assert_family_size(
        self, name: str, family: Family | None, cache_offset: int
    ) -> None:
        """Emit a static_assert for the size of a family, if known.

        This will fail at compile time if the cache size computed from
        the instruction definition does not match the size of the struct
        used by specialize.c.
        """
        if family and name == family.name:
            cache_size = family.size
            if cache_size:
                self.emit(
                    f"static_assert({cache_size} == {cache_offset}, "
                    f'"incorrect cache size");'
                )


def prettify_filename(filename: str) -> str:
    # Make filename more user-friendly and less platform-specific,
    # it is only used for error reporting at this point.
    filename = filename.replace("\\", "/")
    if filename.startswith("./"):
        filename = filename[2:]
    if filename.endswith(".new"):
        filename = filename[:-4]
    return filename


def list_effect_size(effects: list[StackEffect]) -> tuple[int, str]:
    numeric = 0
    symbolic: list[str] = []
    for effect in effects:
        diff, sym = effect_size(effect)
        numeric += diff
        if sym:
            symbolic.append(maybe_parenthesize(sym))
    return numeric, " + ".join(symbolic)


def effect_size(effect: StackEffect) -> tuple[int, str]:
    """Return the 'size' impact of a stack effect.

    Returns a tuple (numeric, symbolic) where:

    - numeric is an int giving the statically analyzable size of the effect
    - symbolic is a string representing a variable effect (e.g. 'oparg*2')

    At most one of these will be non-zero / non-empty.
    """
    if effect.size:
        assert not effect.cond, "Array effects cannot have a condition"
        return 0, effect.size
    elif effect.cond:
        if effect.cond in ("0", "1"):
            return int(effect.cond), ""
        return 0, f"{maybe_parenthesize(effect.cond)} ? 1 : 0"
    else:
        return 1, ""


def maybe_parenthesize(sym: str) -> str:
    """Add parentheses around a string if it contains an operator.

    An exception is made for '*' which is common and harmless
    in the context where the symbolic size is used.
    """
    if re.match(r"^[\s\w*]+$", sym):
        return sym
    else:
        return f"({sym})"


def parenthesize_cond(cond: str) -> str:
    """Parenthesize a condition, but only if it contains ?: itself."""
    if "?" in cond:
        cond = f"({cond})"
    return cond
