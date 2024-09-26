import re
from analyzer import StackItem, StackEffect, Instruction, Uop, PseudoInstruction
from dataclasses import dataclass
from cwriter import CWriter
from typing import Iterator

UNUSED = {"unused"}


def maybe_parenthesize(sym: str) -> str:
    """Add parentheses around a string if it contains an operator
       and is not already parenthesized.

    An exception is made for '*' which is common and harmless
    in the context where the symbolic size is used.
    """
    if sym.startswith("(") and sym.endswith(")"):
        return sym
    if re.match(r"^[\s\w*]+$", sym):
        return sym
    else:
        return f"({sym})"


def var_size(var: StackItem) -> str:
    if var.condition:
        # Special case simplifications
        if var.condition == "0":
            return "0"
        elif var.condition == "1":
            return var.get_size()
        elif var.condition == "oparg & 1" and not var.size:
            return f"({var.condition})"
        else:
            return f"(({var.condition}) ? {var.get_size()} : 0)"
    elif var.size:
        return var.size
    else:
        return "1"


@dataclass
class Local:
    item: StackItem
    cached: bool
    in_memory: bool
    defined: bool

    @staticmethod
    def unused(defn: StackItem) -> "Local":
        return Local(defn, False, defn.is_array(), False)

    @staticmethod
    def local(defn: StackItem) -> "Local":
        array = defn.is_array()
        return Local(defn, not array, array, True)

    @staticmethod
    def redefinition(var: StackItem, prev: "Local") -> "Local":
        assert var.is_array() == prev.is_array()
        return Local(var, prev.cached, prev.in_memory, True)

    @property
    def size(self) -> str:
        return self.item.size

    @property
    def name(self) -> str:
        return self.item.name

    @property
    def condition(self) -> str | None:
        return self.item.condition

    def is_array(self) -> bool:
        return self.item.is_array()


@dataclass
class StackOffset:
    "The stack offset of the virtual base of the stack from the physical stack pointer"

    popped: list[str]
    pushed: list[str]

    @staticmethod
    def empty() -> "StackOffset":
        return StackOffset([], [])

    def copy(self) -> "StackOffset":
        return StackOffset(self.popped[:], self.pushed[:])

    def pop(self, item: StackItem) -> None:
        self.popped.append(var_size(item))

    def push(self, item: StackItem) -> None:
        self.pushed.append(var_size(item))

    def __sub__(self, other: "StackOffset") -> "StackOffset":
        return StackOffset(self.popped + other.pushed, self.pushed + other.popped)

    def __neg__(self) -> "StackOffset":
        return StackOffset(self.pushed, self.popped)

    def simplify(self) -> None:
        "Remove matching values from both the popped and pushed list"
        if not self.popped:
            self.pushed.sort()
            return
        if not self.pushed:
            self.popped.sort()
            return
        # Sort the list so the lexically largest element is last.
        popped = sorted(self.popped)
        pushed = sorted(self.pushed)
        self.popped = []
        self.pushed = []
        while popped and pushed:
            pop = popped.pop()
            push = pushed.pop()
            if pop == push:
                pass
            elif pop > push:
                # if pop > push, there can be no element in pushed matching pop.
                self.popped.append(pop)
                pushed.append(push)
            else:
                self.pushed.append(push)
                popped.append(pop)
        self.popped.extend(popped)
        self.pushed.extend(pushed)
        self.pushed.sort()
        self.popped.sort()

    def to_c(self) -> str:
        self.simplify()
        int_offset = 0
        symbol_offset = ""
        for item in self.popped:
            try:
                int_offset -= int(item)
            except ValueError:
                symbol_offset += f" - {maybe_parenthesize(item)}"
        for item in self.pushed:
            try:
                int_offset += int(item)
            except ValueError:
                symbol_offset += f" + {maybe_parenthesize(item)}"
        if symbol_offset and not int_offset:
            res = symbol_offset
        else:
            res = f"{int_offset}{symbol_offset}"
        if res.startswith(" + "):
            res = res[3:]
        if res.startswith(" - "):
            res = "-" + res[3:]
        return res

    def clear(self) -> None:
        self.popped = []
        self.pushed = []


class StackError(Exception):
    pass

def array_or_scalar(var: StackItem | Local) -> str:
    return "array" if var.is_array() else "scalar"

class Stack:
    def __init__(self) -> None:
        self.top_offset = StackOffset.empty()
        self.base_offset = StackOffset.empty()
        self.variables: list[Local] = []
        self.defined: set[str] = set()

    def pop(self, var: StackItem, extract_bits: bool = False) -> tuple[str, Local]:
        self.top_offset.pop(var)
        indirect = "&" if var.is_array() else ""
        if self.variables:
            popped = self.variables.pop()
            if var.is_array() ^ popped.is_array():
                raise StackError(
                    f"Array mismatch when popping '{popped.name}' from stack to assign to '{var.name}'. "
                    f"Expected {array_or_scalar(var)} got {array_or_scalar(popped)}"
                )
            if popped.size != var.size:
                raise StackError(
                    f"Size mismatch when popping '{popped.name}' from stack to assign to '{var.name}'. "
                    f"Expected {var_size(var)} got {var_size(popped.item)}"
                )
            if var.name in UNUSED:
                if popped.name not in UNUSED and popped.name in self.defined:
                    raise StackError(
                        f"Value is declared unused, but is already cached by prior operation"
                    )
                return "", popped
            if not var.used:
                return "", popped
            self.defined.add(var.name)
            if popped.defined:
                if popped.name == var.name:
                    return "", popped
                else:
                    defn = f"{var.name} = {popped.name};\n"
            else:
                if var.is_array():
                    defn = f"{var.name} = &stack_pointer[{self.top_offset.to_c()}];\n"
                else:
                    defn = f"{var.name} = stack_pointer[{self.top_offset.to_c()}];\n"
            return defn, Local.redefinition(var, popped)

        self.base_offset.pop(var)
        if var.name in UNUSED or not var.used:
            return "", Local.unused(var)
        self.defined.add(var.name)
        cast = f"({var.type})" if (not indirect and var.type) else ""
        bits = ".bits" if cast and not extract_bits else ""
        assign = f"{var.name} = {cast}{indirect}stack_pointer[{self.base_offset.to_c()}]{bits};"
        if var.condition:
            if var.condition == "1":
                assign = f"{assign}\n"
            elif var.condition == "0":
                return "", Local.unused(var)
            else:
                assign = f"if ({var.condition}) {{ {assign} }}\n"
        else:
            assign = f"{assign}\n"
        in_memory = var.is_array() or var.peek
        return assign, Local(var, not var.is_array(), in_memory, True)

    def push(self, var: Local) -> None:
        self.variables.append(var)
        self.top_offset.push(var.item)
        if var.item.used:
            self.defined.add(var.name)
            var.defined = True

    def define_output_arrays(self, outputs: list[StackItem]) -> str:
        res = []
        top_offset = self.top_offset.copy()
        for var in outputs:
            if var.is_array() and var.used and not var.peek:
                c_offset = top_offset.to_c()
                top_offset.push(var)
                res.append(f"{var.name} = &stack_pointer[{c_offset}];\n")
            else:
                top_offset.push(var)
        return "\n".join(res)

    @staticmethod
    def _do_emit(
        out: CWriter,
        var: StackItem,
        base_offset: StackOffset,
        cast_type: str = "uintptr_t",
        extract_bits: bool = False,
    ) -> None:
        cast = f"({cast_type})" if var.type else ""
        bits = ".bits" if cast and not extract_bits else ""
        if var.condition == "0":
            return
        if var.condition and var.condition != "1":
            out.emit(f"if ({var.condition}) ")
        out.emit(f"stack_pointer[{base_offset.to_c()}]{bits} = {cast}{var.name};\n")

    @staticmethod
    def _do_flush(
        out: CWriter,
        variables: list[Local],
        base_offset: StackOffset,
        top_offset: StackOffset,
        cast_type: str = "uintptr_t",
        extract_bits: bool = False,
    ) -> None:
        out.start_line()
        for var in variables:
            if (
                var.cached
                and not var.in_memory
                and not var.item.peek
                and not var.name in UNUSED
            ):
                Stack._do_emit(out, var.item, base_offset, cast_type, extract_bits)
            base_offset.push(var.item)
        if base_offset.to_c() != top_offset.to_c():
            print("base", base_offset, "top", top_offset)
            assert False
        number = base_offset.to_c()
        if number != "0":
            out.emit(f"stack_pointer += {number};\n")
            out.emit("assert(WITHIN_STACK_BOUNDS());\n")
        out.start_line()

    def flush_locally(
        self, out: CWriter, cast_type: str = "uintptr_t", extract_bits: bool = False
    ) -> None:
        self._do_flush(
            out,
            self.variables[:],
            self.base_offset.copy(),
            self.top_offset.copy(),
            cast_type,
            extract_bits,
        )

    def flush(
        self, out: CWriter, cast_type: str = "uintptr_t", extract_bits: bool = False
    ) -> None:
        self._do_flush(
            out,
            self.variables,
            self.base_offset,
            self.top_offset,
            cast_type,
            extract_bits,
        )
        self.variables = []
        self.base_offset.clear()
        self.top_offset.clear()

    def flush_single_var(
        self,
        out: CWriter,
        var_name: str,
        outputs: list[StackItem],
        cast_type: str = "uintptr_t",
        extract_bits: bool = False,
    ) -> None:
        assert any(var.name == var_name for var in outputs)
        base_offset = self.base_offset.copy()
        top_offset = self.top_offset.copy()
        for var in self.variables:
            base_offset.push(var.item)
        for output in outputs:
            if any(output == v.item for v in self.variables):
                # The variable is already on the stack, such as a peeked value
                # in the tier1 generator
                continue
            if output.name == var_name:
                Stack._do_emit(out, output, base_offset, cast_type, extract_bits)
            base_offset.push(output)
            top_offset.push(output)
        if base_offset.to_c() != top_offset.to_c():
            print("base", base_offset, "top", top_offset)
            assert False

    def peek_offset(self) -> str:
        return self.top_offset.to_c()

    def as_comment(self) -> str:
        return f"/* Variables: {[v.name for v in self.variables]}. Base offset: {self.base_offset.to_c()}. Top offset: {self.top_offset.to_c()} */"


def get_stack_effect(inst: Instruction | PseudoInstruction) -> Stack:
    stack = Stack()

    def stacks(inst: Instruction | PseudoInstruction) -> Iterator[StackEffect]:
        if isinstance(inst, Instruction):
            for uop in inst.parts:
                if isinstance(uop, Uop):
                    yield uop.stack
        else:
            assert isinstance(inst, PseudoInstruction)
            yield inst.stack

    for s in stacks(inst):
        locals: dict[str, Local] = {}
        for var in reversed(s.inputs):
            _, local = stack.pop(var)
            if var.name != "unused":
                locals[local.name] = local
        for var in s.outputs:
            if var.name in locals:
                local = locals[var.name]
            else:
                local = Local.unused(var)
            stack.push(local)
    return stack
