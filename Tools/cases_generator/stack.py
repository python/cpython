import re
from analyzer import StackItem, StackEffect, Instruction, Uop, PseudoInstruction
from collections import defaultdict
from dataclasses import dataclass
from cwriter import CWriter
from typing import Iterator, Tuple

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

    def __repr__(self) -> str:
        return f"Local('{self.item.name}', mem={self.in_memory}, defined={self.defined}, array={self.is_array()})"

    def compact_str(self) -> str:
        mtag = "M" if self.in_memory else ""
        dtag = "D" if self.defined else ""
        atag = "A" if self.is_array() else ""
        return f"'{self.item.name}'{mtag}{dtag}{atag}"

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

    @staticmethod
    def from_memory(defn: StackItem) -> "Local":
        return Local(defn, True, True, True)

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

    def __bool__(self) -> bool:
        self.simplify()
        return bool(self.popped) or bool(self.pushed)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, StackOffset):
            return NotImplemented
        return self.to_c() == other.to_c()


class StackError(Exception):
    pass

def array_or_scalar(var: StackItem | Local) -> str:
    return "array" if var.is_array() else "scalar"

class Stack:
    def __init__(self, extract_bits: bool=True, cast_type: str = "uintptr_t") -> None:
        self.top_offset = StackOffset.empty()
        self.base_offset = StackOffset.empty()
        self.variables: list[Local] = []
        self.defined: set[str] = set()
        self.extract_bits = extract_bits
        self.cast_type = cast_type

    def pop(self, var: StackItem) -> tuple[str, Local]:
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
                        f"Value is declared unused, but is already cached by prior operation as '{popped.name}'"
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
                    popped.in_memory = True
            return defn, Local.redefinition(var, popped)

        self.base_offset.pop(var)
        if var.name in UNUSED or not var.used:
            return "", Local.unused(var)
        self.defined.add(var.name)
        cast = f"({var.type})" if (not indirect and var.type) else ""
        bits = ".bits" if cast and self.extract_bits else ""
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
        return assign, Local.from_memory(var)

    def push(self, var: Local) -> None:
        assert(var not in self.variables)
        self.variables.append(var)
        self.top_offset.push(var.item)
        if var.item.used:
            self.defined.add(var.name)

    @staticmethod
    def _do_emit(
        out: CWriter,
        var: StackItem,
        base_offset: StackOffset,
        cast_type: str,
        extract_bits: bool,
    ) -> None:
        cast = f"({cast_type})" if var.type else ""
        bits = ".bits" if cast and extract_bits else ""
        if var.condition == "0":
            return
        if var.condition and var.condition != "1":
            out.emit(f"if ({var.condition}) ")
        out.emit(f"stack_pointer[{base_offset.to_c()}]{bits} = {cast}{var.name};\n")

    def _adjust_stack_pointer(self, out: CWriter, number: str) -> None:
        if number != "0":
            out.start_line()
            out.emit(f"stack_pointer += {number};\n")
            out.emit("assert(WITHIN_STACK_BOUNDS());\n")

    def flush(self, out: CWriter) -> None:
        out.start_line()
        var_offset = self.base_offset.copy()
        for var in self.variables:
            if (
                var.defined and
                not var.in_memory
            ):
                Stack._do_emit(out, var.item, var_offset, self.cast_type, self.extract_bits)
                var.in_memory = True
            var_offset.push(var.item)
        number = self.top_offset.to_c()
        self._adjust_stack_pointer(out, number)
        self.base_offset -= self.top_offset
        self.top_offset.clear()
        out.start_line()

    def is_flushed(self) -> bool:
        return not self.variables and not self.base_offset and not self.top_offset

    def peek_offset(self) -> str:
        return self.top_offset.to_c()

    def as_comment(self) -> str:
        variables = ", ".join([v.compact_str() for v in self.variables])
        return (
            f"/* Variables: {variables}. base: {self.base_offset.to_c()}. top: {self.top_offset.to_c()} */"
        )

    def copy(self) -> "Stack":
        other = Stack(self.extract_bits, self.cast_type)
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

    def merge(self, other: "Stack", out: CWriter) -> None:
        if len(self.variables) != len(other.variables):
            raise StackError("Cannot merge stacks: differing variables")
        for self_var, other_var in zip(self.variables, other.variables):
            if self_var.name != other_var.name:
                raise StackError(f"Mismatched variables on stack: {self_var.name} and {other_var.name}")
            self_var.defined = self_var.defined and other_var.defined
            self_var.in_memory = self_var.in_memory and other_var.in_memory
        self.align(other, out)


def stacks(inst: Instruction | PseudoInstruction) -> Iterator[StackEffect]:
    if isinstance(inst, Instruction):
        for uop in inst.parts:
            if isinstance(uop, Uop):
                yield uop.stack
    else:
        assert isinstance(inst, PseudoInstruction)
        yield inst.stack


def apply_stack_effect(stack: Stack, effect: StackEffect) -> None:
    locals: dict[str, Local] = {}
    for var in reversed(effect.inputs):
        _, local = stack.pop(var)
        if var.name != "unused":
            locals[local.name] = local
    for var in effect.outputs:
        if var.name in locals:
            local = locals[var.name]
        else:
            local = Local.unused(var)
        stack.push(local)


def get_stack_effect(inst: Instruction | PseudoInstruction) -> Stack:
    stack = Stack()
    for s in stacks(inst):
        apply_stack_effect(stack, s)
    return stack


def get_stack_effects(inst: Instruction | PseudoInstruction) -> list[Stack]:
    """Returns a list of stack effects after each uop"""
    result = []
    stack = Stack()
    for s in stacks(inst):
        apply_stack_effect(stack, s)
        result.append(stack.copy())
    return result


@dataclass
class Storage:

    stack: Stack
    inputs: list[Local]
    outputs: list[Local]
    peeks: list[Local]
    spilled: int = 0

    @staticmethod
    def needs_defining(var: Local) -> bool:
        return (
            not var.defined and
            not var.is_array() and
            var.name != "unused"
        )

    @staticmethod
    def is_live(var: Local) -> bool:
        return (
            var.defined and
            var.name != "unused"
        )

    def first_input_not_cleared(self) -> str:
        for input in self.inputs:
            if input.defined:
                return input.name
        return ""

    def clear_inputs(self, reason:str) -> None:
        while self.inputs:
            tos = self.inputs.pop()
            if self.is_live(tos) and not tos.is_array():
                raise StackError(
                    f"Input '{tos.name}' is still live {reason}"
                )
            self.stack.pop(tos.item)

    def clear_dead_inputs(self) -> None:
        live = ""
        while self.inputs:
            tos = self.inputs[-1]
            if self.is_live(tos):
                live = tos.name
                break
            self.inputs.pop()
            self.stack.pop(tos.item)
        for var in self.inputs:
            if not var.defined and not var.is_array() and var.name != "unused":
                raise StackError(
                    f"Input '{var.name}' is not live, but '{live}' is"
                )

    def _push_defined_outputs(self) -> None:
        defined_output = ""
        for output in self.outputs:
            if output.defined and not output.in_memory:
                defined_output = output.name
        if not defined_output:
            return
        self.clear_inputs(f"when output '{defined_output}' is defined")
        undefined = ""
        for out in self.outputs:
            if out.defined:
                if undefined:
                    f"Locals not defined in stack order. "
                    f"Expected '{undefined}' to be defined before '{out.name}'"
            else:
                undefined = out.name
        while self.outputs and not self.needs_defining(self.outputs[0]):
            out = self.outputs.pop(0)
            self.stack.push(out)

    def locals_cached(self) -> bool:
        for out in self.outputs:
            if out.defined:
                return True
        return False

    def flush(self, out: CWriter) -> None:
        self.clear_dead_inputs()
        self._push_defined_outputs()
        self.stack.flush(out)

    def save(self, out: CWriter) -> None:
        assert self.spilled >= 0
        if self.spilled == 0:
            self.flush(out)
            out.start_line()
            out.emit_spill()
        self.spilled += 1

    def save_inputs(self, out: CWriter) -> None:
        assert self.spilled >= 0
        if self.spilled == 0:
            self.clear_dead_inputs()
            self.stack.flush(out)
            out.start_line()
            out.emit_spill()
        self.spilled += 1

    def reload(self, out: CWriter) -> None:
        if self.spilled == 0:
            raise StackError("Cannot reload stack as it hasn't been saved")
        assert self.spilled > 0
        self.spilled -= 1
        if self.spilled == 0:
            out.start_line()
            out.emit_reload()

    @staticmethod
    def for_uop(stack: Stack, uop: Uop) -> tuple[list[str], "Storage"]:
        code_list: list[str] = []
        inputs: list[Local] = []
        peeks: list[Local] = []
        for input in reversed(uop.stack.inputs):
            code, local = stack.pop(input)
            code_list.append(code)
            if input.peek:
                peeks.append(local)
            else:
                inputs.append(local)
        inputs.reverse()
        peeks.reverse()
        for peek in peeks:
            stack.push(peek)
        top_offset = stack.top_offset.copy()
        for ouput in uop.stack.outputs:
            if ouput.is_array() and ouput.used and not ouput.peek:
                c_offset = top_offset.to_c()
                top_offset.push(ouput)
                code_list.append(f"{ouput.name} = &stack_pointer[{c_offset}];\n")
            else:
                top_offset.push(ouput)
        for var in inputs:
            stack.push(var)
        outputs = [ Local.undefined(var) for var in uop.stack.outputs if not var.peek ]
        return code_list, Storage(stack, inputs, outputs, peeks)

    @staticmethod
    def copy_list(arg: list[Local]) -> list[Local]:
        return [ l.copy() for l in arg ]

    def copy(self) -> "Storage":
        new_stack = self.stack.copy()
        variables = { var.name: var for var in new_stack.variables }
        inputs = [ variables[var.name] for var in self.inputs]
        assert [v.name for v in inputs] == [v.name for v in self.inputs], (inputs, self.inputs)
        return Storage(
            new_stack, inputs,
            self.copy_list(self.outputs), self.copy_list(self.peeks), self.spilled
        )

    def sanity_check(self) -> None:
        names: set[str] = set()
        for var in self.inputs:
            if var.name in names:
                raise StackError(f"Duplicate name {var.name}")
            names.add(var.name)
        names = set()
        for var in self.outputs:
            if var.name in names:
                raise StackError(f"Duplicate name {var.name}")
            names.add(var.name)
        names = set()
        for var in self.stack.variables:
            if var.name in names:
                raise StackError(f"Duplicate name {var.name}")
            names.add(var.name)

    def is_flushed(self) -> bool:
        for var in self.outputs:
            if var.defined and not var.in_memory:
                return False
        return self.stack.is_flushed()

    def merge(self, other: "Storage", out: CWriter) -> None:
        self.sanity_check()
        if len(self.inputs) != len(other.inputs):
            self.clear_dead_inputs()
            other.clear_dead_inputs()
        if len(self.inputs) != len(other.inputs):
            diff = self.inputs[-1] if len(self.inputs) > len(other.inputs) else other.inputs[-1]
            raise StackError(f"Unmergeable inputs. Differing state of '{diff.name}'")
        for var, other_var in zip(self.inputs, other.inputs):
            if var.defined != other_var.defined:
                raise StackError(f"'{var.name}' is cleared on some paths, but not all")
        if len(self.outputs) != len(other.outputs):
            self._push_defined_outputs()
            other._push_defined_outputs()
        if len(self.outputs) != len(other.outputs):
            var = self.outputs[0] if len(self.outputs) > len(other.outputs) else other.outputs[0]
            raise StackError(f"'{var.name}' is set on some paths, but not all")
        self.stack.merge(other.stack, out)
        self.sanity_check()

    def push_outputs(self) -> None:
        if self.spilled:
            raise StackError(f"Unbalanced stack spills")
        self.clear_inputs("at the end of the micro-op")
        if self.inputs:
            raise StackError(f"Input variable '{self.inputs[-1].name}' is still live")
        self._push_defined_outputs()
        if self.outputs:
            for out in self.outputs:
                if self.needs_defining(out):
                    raise StackError(f"Output variable '{self.outputs[0].name}' is not defined")
                self.stack.push(out)
            self.outputs = []

    def as_comment(self) -> str:
        stack_comment = self.stack.as_comment()
        next_line = "\n               "
        inputs = ", ".join([var.compact_str() for var in self.inputs])
        outputs = ", ".join([var.compact_str() for var in self.outputs])
        peeks = ", ".join([var.name for var in self.peeks])
        return f"{stack_comment[:-2]}{next_line}inputs: {inputs}{next_line}outputs: {outputs}{next_line}peeks: {peeks} */"

    def close_inputs(self, out: CWriter) -> None:
        tmp_defined = False
        def close_named(close: str, name: str, overwrite: str) -> None:
            nonlocal tmp_defined
            if overwrite:
                if not tmp_defined:
                    out.emit("_PyStackRef ")
                    tmp_defined = True
                out.emit(f"tmp = {name};\n")
                out.emit(f"{name} = {overwrite};\n")
                if not var.is_array():
                    var.in_memory = False
                    self.flush(out)
                out.emit(f"{close}(tmp);\n")
            else:
                out.emit(f"{close}({name});\n")

        def close_variable(var: Local, overwrite: str) -> None:
            nonlocal tmp_defined
            close = "PyStackRef_CLOSE"
            if "null" in var.name or var.condition and var.condition != "1":
                close = "PyStackRef_XCLOSE"
            if var.size:
                if var.size == "1":
                    close_named(close, f"{var.name}[0]", overwrite)
                else:
                    if overwrite and not tmp_defined:
                        out.emit("_PyStackRef tmp;\n")
                        tmp_defined = True
                    out.emit(f"for (int _i = {var.size}; --_i >= 0;) {{\n")
                    close_named(close, f"{var.name}[_i]", overwrite)
                    out.emit("}\n")
            else:
                if var.condition and var.condition == "0":
                    return
                close_named(close, var.name, overwrite)

        self.clear_dead_inputs()
        if not self.inputs:
            return
        output: Local | None = None
        for var in self.outputs:
            if var.is_array():
                if len(self.inputs) > 1:
                    raise StackError("Cannot call DECREF_INPUTS with multiple live input(s) and array output")
            elif var.defined:
                if output is not None:
                    raise StackError("Cannot call DECREF_INPUTS with more than one live output")
                output = var
        self.save_inputs(out)
        if output is not None:
            lowest = self.inputs[0]
            if lowest.is_array():
                try:
                    size = int(lowest.size)
                except:
                    size = -1
                if size <= 0:
                    raise StackError("Cannot call DECREF_INPUTS with non fixed size array as lowest input on stack")
                if size > 1:
                    raise StackError("Cannot call DECREF_INPUTS with array size > 1 as lowest input on stack")
                output.defined = False
                close_variable(lowest, output.name)
            else:
                lowest.in_memory = False
                output.defined = False
                close_variable(lowest, output.name)
        to_close = self.inputs[: 0 if output is not None else None: -1]
        if len(to_close) == 1 and not to_close[0].is_array():
            self.reload(out)
            to_close[0].defined = False
            self.flush(out)
            self.save_inputs(out)
            close_variable(to_close[0], "")
            self.reload(out)
        else:
            for var in to_close:
                assert var.defined or var.is_array()
                close_variable(var, "PyStackRef_NULL")
            self.reload(out)
        for var in self.inputs:
            var.defined = False
        if output is not None:
            output.defined = True
            # MyPy false positive
            lowest.defined = False  # type: ignore[possibly-undefined]
        self.flush(out)
