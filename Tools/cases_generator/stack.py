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

    def __repr__(self):
        return f"Local('{self.item.name}', in_memory={self.in_memory}, defined={self.defined})"

    @staticmethod
    def unused(defn: StackItem) -> "Local":
        return Local(defn, False, defn.is_array(), False)

    @staticmethod
    def undefined(defn: StackItem) -> "Local":
        array = defn.is_array()
        return Local(defn, not array, array, False)

    @staticmethod
    def redefinition(var: StackItem, prev: "Local") -> "Local":
        assert var.is_array() == prev.is_array()
        return Local(var, prev.cached, prev.in_memory, True)

    def copy(self) -> "Local":
        return Local(
            self.item,
            self.cached,
            self.in_memory,
            self.defined
        )

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

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Local):
            return NotImplemented
        return (
            self.item is other.item
            and self.cached is other.cached
            and self.in_memory is other.in_memory
            and self.defined is other.defined
        )


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

    def as_int(self) -> int | None:
        self.simplify()
        int_offset = 0
        for item in self.popped:
            try:
                int_offset -= int(item)
            except ValueError:
                return None
        for item in self.pushed:
            try:
                int_offset += int(item)
            except ValueError:
                return None
        return int_offset

    def clear(self) -> None:
        self.popped = []
        self.pushed = []

    def __bool__(self):
        self.simplify()
        return bool(self.popped) or bool(self.pushed)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, StackOffset):
            return NotImplemented
        return self.to_c() == other.to_c()


class StackError(Exception):
    pass


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
            if popped.size != var.size:
                raise StackError(
                    f"Size mismatch when popping '{popped.name}' from stack to assign to {var.name}. "
                    f"Expected {var.size} got {popped.size}"
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

    def _adjust_stack_pointer(self, out: CWriter, number: str):
        if number != "0":
            out.emit(f"stack_pointer += {number};\n")
            out.emit("assert(WITHIN_STACK_BOUNDS());\n")

    def flush(
        self, out: CWriter, cast_type: str = "uintptr_t", extract_bits: bool = False
    ) -> None:
        out.start_line()
        var_offset = self.base_offset.copy()
        for var in self.variables:
            if (
                not var.in_memory
                and not var.item.peek
                and not var.name in UNUSED
            ):
                Stack._do_emit(out, var.item, var_offset, cast_type, extract_bits)
                var.in_memory = True
            var_offset.push(var.item)
        number = self.top_offset.to_c()
        self._adjust_stack_pointer(out, number)
        self.base_offset -= self.top_offset
        self.top_offset.clear()
        out.start_line()

    def is_flushed(self):
        return not self.variables and not self.base_offset and not self.top_offset

    def peek_offset(self) -> str:
        return self.top_offset.to_c()

    def as_comment(self) -> str:
        return (
            f"/* Variables: {[v.name for v in self.variables]}. "
            f"Base offset: {self.base_offset.to_c()}. Top offset: {self.top_offset.to_c()} */"
        )

    def copy(self) -> "Stack":
        other = Stack()
        other.top_offset = self.top_offset.copy()
        other.base_offset = self.base_offset.copy()
        other.variables = [var.copy() for var in self.variables]
        other.defined = set(self.defined)
        return other

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Stack):
            return NotImplemented
        return (
            self.top_offset == other.top_offset
            and self.base_offset == other.base_offset
            and self.variables == other.variables
        )

    def align(self, other: "Stack", out: CWriter) -> None:
        if len(self.variables) != len(other.variables):
            raise StackError("Cannot align stacks: differing variables")
        if self.top_offset == other.top_offset:
            return
        diff = self.top_offset - other.top_offset
        try:
            self.top_offset -= diff
            self.base_offset -= diff
            self._adjust_stack_pointer(out, diff.to_c())
        except ValueError:
            raise StackError("Cannot align stacks: cannot adjust stack pointer")

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

@dataclass
class Storage:

    stack: Stack
    outputs: list[Local]

    def _push_defined_locals(self) -> None:
        while self.outputs:
            if not self.outputs[0].defined:
                break
            out = self.outputs.pop(0)
            self.stack.push(out)
        undefined = ""
        for out in self.outputs:
            if out.is_array():
                continue
            if out.defined:
                raise StackError(
                    f"Locals not defined in stack order. "
                    f"Expected  '{out.name}' is defined before '{undefined}'"
                )
            else:
                undefined = out.name

    def flush(self, out: CWriter) -> None:
        self._push_defined_locals()
        self.stack.flush(out)

    @staticmethod
    def for_uop(stack: Stack, uop: Uop, locals: dict[str, Local]) -> "Storage":
        outputs: list[Local] = []
        for var in uop.stack.outputs:
            if not var.peek:
                if var.name in locals:
                    local = locals[var.name]
                elif var.name == "unused":
                    local = Local.unused(var)
                else:
                    local = Local.undefined(var)
                outputs.append(local)
        return Storage(stack, outputs)

    def copy(self) -> "Storage":
        return Storage(self.stack.copy(), [ l.copy() for l in self.outputs])

    def sanity_check(self):
        names: set[str] = set()
        for var in self.stack.variables:
            if var.name in names:
                raise StackError(f"Duplicate name {var.name}")
            names.add(var.name)
        for var in self.outputs:
            if var.name in names:
                raise StackError(f"Duplicate name {var.name}")
            names.add(var.name)

    def is_flushed(self):
        for var in self.outputs:
            if var.defined and not var.in_memory:
                return False
        return self.stack.is_flushed()

    def merge(self, other: "Storage", out: CWriter) -> None:
        self.sanity_check()
        locally_defined_set: set[str] = set()
        for var in self.outputs:
            if var.defined and not var.in_memory:
                locally_defined_set.add(var.name)
        for var in other.outputs:
            if var.defined and not var.in_memory:
                locally_defined_set.add(var.name)
        while self.stack.variables and self.stack.variables[0].item.name in locally_defined_set:
            code, var = self.stack.pop(self.stack.variables[0].item)
            out.emit(code)
            self.outputs.append(var)
        while other.stack.variables and other.stack.variables[0].item.name in locally_defined_set:
            code, var = other.stack.pop(other.stack.variables[0].item)
            assert code == ""
            other.outputs.append(var)
        s1, s2 = self.stack, other.stack
        l1, l2 = self.outputs, other.outputs
        if len(s1.variables) != len(s2.variables):
            # Make sure s2 is the larger stack.
            if len(s1.variables) > len(s2.variables):
                s1, s2 = s2, s1
                l1, l2 = l2, l1
            while len(s2.variables) > len(s1.variables):
                top = s2.variables[-1]
                if top.defined:
                    code, var = s2.pop(top.item)
                    assert code == "" and var == top
                    l2.insert(0, top)
                else:
                    for l in l1:
                        if l.name == top.name:
                            break
                    else:
                        raise StackError(f"Missing local {top.name} when attempting to merge storage")
                    if l.in_memory:
                        s1.push(l)
                        l1.remove(l)
                    else:
                        raise StackError(f"Local {top.name} is not in memory, so cannot be merged")
        # Now merge locals:
        self_live = [var for var in self.outputs if not var.item.peek and var.defined]
        other_live = [var for var in other.outputs if not var.item.peek and var.defined]
        self.stack.align(other.stack, out)
        if len(self_live) != len(other_live):
            if other.stack.is_flushed():
                self.stack.flush(out)
                return self
            else:
                raise StackError(f"Mismatched locals: {self_live} and {other_live}")
        for self_out, other_out in zip(self_live, other_live):
            if self_out.name != other_out.name:
                raise StackError(f"Mismatched local: {self_out.name} and {other_out.name}")
            if self_out.defined ^ other_out.defined:
                raise StackError(f"Local {self_out.name} defined on only one path")
            self_out.in_memory = other_out.in_memory = self_out.in_memory and other_out.in_memory
            self_out.cached = other_out.cached = self_out.cached and other_out.cached
        self.sanity_check()

    def as_comment(self) -> str:
        stack_comment = self.stack.as_comment()
        return stack_comment[:-2] + "\n    LOCALS: " + str(self.outputs) + " */"
