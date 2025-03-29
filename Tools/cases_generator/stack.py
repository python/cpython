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
    if var.size:
        return var.size
    else:
        return "1"


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


@dataclass
class Local:
    item: StackItem
    memory_offset: StackOffset | None
    in_local: bool

    def __repr__(self) -> str:
        return f"Local('{self.item.name}', mem={self.memory_offset}, local={self.in_local}, array={self.is_array()})"

    #def compact_str(self) -> str:
        #mtag = "M" if self.memory_offset else ""
        #dtag = "D" if self.in_local else ""
        #atag = "A" if self.is_array() else ""
        #return f"'{self.item.name}'{mtag}{dtag}{atag}"

    compact_str = __repr__

    @staticmethod
    def unused(defn: StackItem, offset: StackOffset) -> "Local":
        return Local(defn, offset, False)

    @staticmethod
    def undefined(defn: StackItem) -> "Local":
        return Local(defn, None, False)

    @staticmethod
    def from_memory(defn: StackItem, offset: StackOffset) -> "Local":
        return Local(defn, offset, True)

    def kill(self) -> None:
        self.in_local = False
        self.memory_offset = None

    def in_memory(self) -> bool:
        return self.memory_offset is not None or self.is_array()

    def is_dead(self) -> bool:
        return not self.in_local and self.memory_offset is None

    def copy(self) -> "Local":
        return Local(
            self.item,
            self.memory_offset,
            self.in_local
        )

    @property
    def size(self) -> str:
        return self.item.size

    @property
    def name(self) -> str:
        return self.item.name

    def is_array(self) -> bool:
        return self.item.is_array()

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Local):
            return NotImplemented
        return (
            self.item is other.item
            and self.memory_offset == other.memory_offset
            and self.in_local == other.in_local
        )


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

    def drop(self, var: StackItem, check_liveness: bool) -> None:
        self.top_offset.pop(var)
        if self.variables:
            popped = self.variables.pop()
            if popped.is_dead() or not var.used:
                return
        if check_liveness:
            raise StackError(f"Dropping live value '{var.name}'")

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
            if popped.name != var.name:
                rename = f"{var.name} = {popped.name};\n"
                popped.item = var
            else:
                rename = ""
            if not popped.in_local:
                assert popped.memory_offset is not None
                if var.is_array():
                    defn = f"{var.name} = &stack_pointer[{self.top_offset.to_c()}];\n"
                else:
                    defn = f"{var.name} = stack_pointer[{self.top_offset.to_c()}];\n"
                    popped.in_local = True
            else:
                defn = rename
            return defn, popped

        self.base_offset.pop(var)
        if var.name in UNUSED or not var.used:
            return "", Local.unused(var, self.base_offset)
        self.defined.add(var.name)
        cast = f"({var.type})" if (not indirect and var.type) else ""
        bits = ".bits" if cast and self.extract_bits else ""
        assign = f"{var.name} = {cast}{indirect}stack_pointer[{self.base_offset.to_c()}]{bits};"
        assign = f"{assign}\n"
        return assign, Local.from_memory(var, self.base_offset.copy())

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
                var.in_local and
                not var.memory_offset and
                not var.is_array()
            ):
                Stack._do_emit(out, var.item, var_offset, self.cast_type, self.extract_bits)
                var.memory_offset = var_offset.copy()
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
            self_var.in_local = self_var.in_local and other_var.in_local
            if other_var.memory_offset is None:
                self_var.memory_offset = None
        self.align(other, out)
        for self_var, other_var in zip(self.variables, other.variables):
            if self_var.memory_offset is not None:
                if self_var.memory_offset != other_var.memory_offset:
                    raise StackError(f"Mismatched stack depths for {self_var.name}: {self_var.memory_offset} and {other_var.memory_offset}")
            elif other_var.memory_offset is None:
                self_var.memory_offset = None


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
            local = Local.unused(var, stack.base_offset)
        stack.push(local)


def get_stack_effect(inst: Instruction | PseudoInstruction) -> Stack:
    stack = Stack()
    for s in stacks(inst):
        apply_stack_effect(stack, s)
    return stack


@dataclass
class Storage:

    stack: Stack
    inputs: list[Local]
    outputs: list[Local]
    check_liveness: bool
    spilled: int = 0

    @staticmethod
    def needs_defining(var: Local) -> bool:
        return (
            not var.in_local and
            not var.is_array() and
            var.name != "unused"
        )

    @staticmethod
    def is_live(var: Local) -> bool:
        return (
            var.name != "unused" and
            (
                var.in_local or
                var.memory_offset is not None
            )
        )

    def clear_inputs(self, reason:str) -> None:
        while self.inputs:
            tos = self.inputs.pop()
            if self.is_live(tos) and self.check_liveness:
                raise StackError(
                    f"Input '{tos.name}' is still live {reason}"
                )
            self.stack.drop(tos.item, self.check_liveness)

    def clear_dead_inputs(self) -> None:
        live = ""
        while self.inputs:
            tos = self.inputs[-1]
            if self.is_live(tos):
                live = tos.name
                break
            self.inputs.pop()
            self.stack.drop(tos.item, self.check_liveness)
        for var in self.inputs:
            if not self.is_live(var):
                raise StackError(
                    f"Input '{var.name}' is not live, but '{live}' is"
                )

    def _push_defined_outputs(self) -> None:
        defined_output = ""
        for output in self.outputs:
            if output.in_local and not output.memory_offset:
                defined_output = output.name
        if not defined_output:
            return
        self.clear_inputs(f"when output '{defined_output}' is defined")
        undefined = ""
        for out in self.outputs:
            if out.in_local:
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
            if out.in_local:
                return True
        return False

    def flush(self, out: CWriter) -> None:
        self.clear_dead_inputs()
        self._push_defined_outputs()
        self.stack.flush(out)

    def save(self, out: CWriter) -> None:
        assert self.spilled >= 0
        if self.spilled == 0:
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
    def for_uop(stack: Stack, uop: Uop, check_liveness: bool = True) -> tuple[list[str], "Storage"]:
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
        return code_list, Storage(stack, inputs, outputs, check_liveness)

    @staticmethod
    def copy_list(arg: list[Local]) -> list[Local]:
        return [ l.copy() for l in arg ]

    def copy(self) -> "Storage":
        new_stack = self.stack.copy()
        variables = { var.name: var for var in new_stack.variables }
        inputs = [ variables[var.name] for var in self.inputs]
        assert [v.name for v in inputs] == [v.name for v in self.inputs], (inputs, self.inputs)
        return Storage(
            new_stack, inputs, self.copy_list(self.outputs),
            self.check_liveness, self.spilled
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
            if var.in_local and not var.memory_offset:
                return False
        return self.stack.is_flushed()

    def merge(self, other: "Storage", out: CWriter) -> None:
        self.sanity_check()
        if len(self.inputs) != len(other.inputs):
            self.clear_dead_inputs()
            other.clear_dead_inputs()
        if len(self.inputs) != len(other.inputs) and self.check_liveness:
            diff = self.inputs[-1] if len(self.inputs) > len(other.inputs) else other.inputs[-1]
            raise StackError(f"Unmergeable inputs. Differing state of '{diff.name}'")
        for var, other_var in zip(self.inputs, other.inputs):
            if var.in_local != other_var.in_local:
                raise StackError(f"'{var.name}' is cleared on some paths, but not all")
        if len(self.outputs) != len(other.outputs):
            self._push_defined_outputs()
            other._push_defined_outputs()
        if len(self.outputs) != len(other.outputs):
            var = self.outputs[0] if len(self.outputs) > len(other.outputs) else other.outputs[0]
            raise StackError(f"'{var.name}' is set on some paths, but not all")
        for var, other_var in zip(self.outputs, other.outputs):
            if var.memory_offset is None:
                other_var.memory_offset = None
            elif other_var.memory_offset is None:
                var.memory_offset = None
        self.stack.merge(other.stack, out)
        self.sanity_check()

    def push_outputs(self) -> None:
        if self.spilled:
            raise StackError(f"Unbalanced stack spills")
        self.clear_inputs("at the end of the micro-op")
        if self.inputs and self.check_liveness:
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
        return f"{stack_comment[:-2]}{next_line}inputs: {inputs}{next_line}outputs: {outputs}*/"

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
                self.stack.flush(out)
                out.emit(f"{close}(tmp);\n")
            else:
                out.emit(f"{close}({name});\n")

        def close_variable(var: Local, overwrite: str) -> None:
            nonlocal tmp_defined
            close = "PyStackRef_CLOSE"
            if "null" in var.name:
                close = "PyStackRef_XCLOSE"
            var.memory_offset = None
            self.save(out)
            out.start_line()
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
                close_named(close, var.name, overwrite)
            self.reload(out)

        self.clear_dead_inputs()
        if not self.inputs:
            return
        lowest = self.inputs[0]
        output: Local | None = None
        for var in self.outputs:
            if var.is_array():
                if len(self.inputs) > 1:
                    raise StackError("Cannot call DECREF_INPUTS with array output and more than one input")
                output = var
            elif var.in_local:
                if output is not None:
                    raise StackError("Cannot call DECREF_INPUTS with more than one live output")
                output = var
        self.stack.flush(out)
        if output is not None:
            if output.is_array():
                assert len(self.inputs) == 1
                self.stack.drop(self.inputs[0].item, False)
                self.stack.push(output)
                self.stack.flush(out)
                close_variable(self.inputs[0], "")
                self.stack.drop(output.item, self.check_liveness)
                self.inputs = []
                return
            if var_size(lowest.item) != var_size(output.item):
                raise StackError("Cannot call DECREF_INPUTS with live output not matching first input size")
            lowest.in_local = True
            close_variable(lowest, output.name)
            assert lowest.memory_offset is not None
        for input in reversed(self.inputs[1:]):
            close_variable(input, "PyStackRef_NULL")
        if output is None:
            close_variable(self.inputs[0], "PyStackRef_NULL")
        for input in reversed(self.inputs[1:]):
            input.kill()
            self.stack.drop(input.item, self.check_liveness)
        if output is None:
            self.inputs[0].kill()
        self.stack.drop(self.inputs[0].item, False)
        output_in_place = self.outputs and output is self.outputs[0] and lowest.memory_offset is not None
        if output_in_place:
            output.memory_offset = lowest.memory_offset.copy()  # type: ignore[union-attr]
        else:
            self.stack.flush(out)
        if output is not None:
            self.stack.push(output)
        self.inputs = []
        if output_in_place:
            self.stack.flush(out)
        if output is not None:
            code, output = self.stack.pop(output.item)
            out.emit(code)
