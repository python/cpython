import re
from analyzer import StackItem, StackEffect, Instruction, Uop, PseudoInstruction
from dataclasses import dataclass
from cwriter import CWriter
from typing import Iterator

UNUSED = {"unused"}

# Set this to true for voluminous output showing state of stack and locals
PRINT_STACKS = False

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
class PointerOffset:
    """The offset of a pointer from the reference pointer
        The 'reference pointer' is the address of the physical stack pointer
        at the start of the code section, as if each code section started with
        `const PyStackRef *reference = stack_pointer`
    """
    numeric: int
    positive: tuple[str, ...]
    negative: tuple[str, ...]

    @staticmethod
    def zero() -> "PointerOffset":
        return PointerOffset(0, (), ())

    def pop(self, item: StackItem) -> "PointerOffset":
        return self - PointerOffset.from_item(item)

    def push(self, item: StackItem) -> "PointerOffset":
        return self + PointerOffset.from_item(item)

    @staticmethod
    def from_item(item: StackItem) -> "PointerOffset":
        if not item.size:
            return PointerOffset(1, (), ())
        txt = item.size.strip()
        n: tuple[str, ...] = ()
        p: tuple[str, ...] = ()
        try:
            i = int(txt)
        except ValueError:
            i = 0
            if txt[0] == "+":
                txt = txt[1:]
            if txt[0] == "-":
                n = (txt[1:],)
            else:
                p = (txt,)
        return PointerOffset(i, p, n)

    @staticmethod
    def create(numeric: int, positive: tuple[str, ...], negative: tuple[str, ...]) -> "PointerOffset":
        positive, negative = PointerOffset._simplify(positive, negative)
        return PointerOffset(numeric, positive, negative)

    def __sub__(self, other: "PointerOffset") -> "PointerOffset":
        return PointerOffset.create(
            self.numeric - other.numeric,
            self.positive + other.negative,
            self.negative + other.positive
        )

    def __add__(self, other: "PointerOffset") -> "PointerOffset":
        return PointerOffset.create(
            self.numeric + other.numeric,
            self.positive + other.positive,
            self.negative + other.negative
        )

    def __neg__(self) -> "PointerOffset":
        return PointerOffset(-self.numeric, self.negative, self.positive)

    @staticmethod
    def _simplify(positive: tuple[str, ...], negative: tuple[str, ...]) -> tuple[tuple[str, ...], tuple[str, ...]]:
        p_orig: list[str] = sorted(positive)
        n_orig: list[str] = sorted(negative)
        p_uniq: list[str] = []
        n_uniq: list[str] = []
        while p_orig and n_orig:
            p_item = p_orig.pop()
            n_item = n_orig.pop()
            if p_item > n_item:
                # if p_item > n_item, there can be no element in n matching p_item.
                p_uniq.append(p_item)
                n_orig.append(n_item)
            elif p_item < n_item:
                n_uniq.append(n_item)
                p_orig.append(p_item)
            # Otherwise they are the same and cancel each other out
        return tuple(p_orig + p_uniq), tuple(n_orig + n_uniq)

    def to_c(self) -> str:
        symbol_offset = ""
        for item in self.negative:
            symbol_offset += f" - {maybe_parenthesize(item)}"
        for item in self.positive:
            symbol_offset += f" + {maybe_parenthesize(item)}"
        if symbol_offset and self.numeric == 0:
            res = symbol_offset
        else:
            res = f"{self.numeric}{symbol_offset}"
        if res.startswith(" + "):
            res = res[3:]
        if res.startswith(" - "):
            res = "-" + res[3:]
        return res

    def as_int(self) -> int | None:
        if self.positive or self.negative:
            return None
        return self.numeric

    def __str__(self) -> str:
        return self.to_c()

    def __repr__(self) -> str:
        return f"PointerOffset({self.to_c()})"

@dataclass
class Local:
    item: StackItem
    memory_offset: PointerOffset | None
    in_local: bool

    def __repr__(self) -> str:
        return f"Local('{self.item.name}', mem={self.memory_offset}, local={self.in_local}, array={self.is_array()})"

    def compact_str(self) -> str:
        mtag = "M" if self.memory_offset else ""
        dtag = "L" if self.in_local else ""
        atag = "A" if self.is_array() else ""
        return f"'{self.item.name}'{mtag}{dtag}{atag}"

    @staticmethod
    def unused(defn: StackItem, offset: PointerOffset | None) -> "Local":
        return Local(defn, offset, False)

    @staticmethod
    def undefined(defn: StackItem) -> "Local":
        return Local(defn, None, False)

    @staticmethod
    def from_memory(defn: StackItem, offset: PointerOffset) -> "Local":
        return Local(defn, offset, True)

    @staticmethod
    def register(name: str) -> "Local":
        item = StackItem(name, "", False, True)
        return Local(item, None, True)

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
    def __init__(self) -> None:
        self.base_offset = PointerOffset.zero()
        self.physical_sp = PointerOffset.zero()
        self.logical_sp = PointerOffset.zero()
        self.variables: list[Local] = []


    def push_cache(self, cached_items:list[str], out: CWriter) -> None:
        for i, name in enumerate(cached_items):
            out.start_line()
            out.emit(f"_PyStackRef _stack_item_{i} = {name};\n")
            self.push(Local.register(f"_stack_item_{i}"))

    def drop(self, var: StackItem, check_liveness: bool) -> None:
        self.logical_sp = self.logical_sp.pop(var)
        if self.variables:
            popped = self.variables.pop()
            if popped.is_dead() or not var.used:
                return
        if check_liveness:
            raise StackError(f"Dropping live value '{var.name}'")

    def pop(self, var: StackItem, out: CWriter) -> Local:
        if self.variables:
            top = self.variables[-1]
            if var.is_array() != top.is_array() or top.size != var.size:
                # Mismatch in variables
                self.clear(out)
        self.logical_sp = self.logical_sp.pop(var)
        indirect = "&" if var.is_array() else ""
        if self.variables:
            popped = self.variables.pop()
            assert var.is_array() == popped.is_array() and popped.size == var.size
            if not var.used:
                return popped
            if popped.name != var.name:
                rename = f"{var.name} = {popped.name};\n"
                popped.item = var
            else:
                rename = ""
            if not popped.in_local:
                if popped.memory_offset is None:
                    popped.memory_offset = self.logical_sp
                assert popped.memory_offset == self.logical_sp, (popped, self.as_comment())
                offset = popped.memory_offset - self.physical_sp
                if var.is_array():
                    defn = f"{var.name} = &stack_pointer[{offset.to_c()}];\n"
                else:
                    defn = f"{var.name} = stack_pointer[{offset.to_c()}];\n"
                    popped.in_local = True
            else:
                defn = rename
            out.emit(defn)
            return popped
        self.base_offset = self.logical_sp
        if var.name in UNUSED or not var.used:
            return Local.unused(var, self.base_offset)
        c_offset = (self.base_offset - self.physical_sp).to_c()
        assign = f"{var.name} = {indirect}stack_pointer[{c_offset}];\n"
        out.emit(assign)
        self._print(out)
        return Local.from_memory(var, self.base_offset)

    def clear(self, out: CWriter) -> None:
        "Flush to memory and clear variables stack"
        self.flush(out)
        self.variables = []
        self.base_offset = self.logical_sp

    def push(self, var: Local) -> None:
        assert(var not in self.variables), var
        self.variables.append(var)
        self.logical_sp = self.logical_sp.push(var.item)

    @staticmethod
    def _do_emit(
        out: CWriter,
        var: StackItem,
        stack_offset: PointerOffset,
    ) -> None:
        out.emit(f"stack_pointer[{stack_offset.to_c()}] = {var.name};\n")

    def _save_physical_sp(self, out: CWriter) -> None:
        if self.physical_sp != self.logical_sp:
            diff = self.logical_sp - self.physical_sp
            out.start_line()
            out.emit(f"stack_pointer += {diff.to_c()};\n")
            out.emit(f"assert(WITHIN_STACK_BOUNDS());\n")
            self.physical_sp = self.logical_sp
            self._print(out)

    def save_variables(self, out: CWriter) -> None:
        out.start_line()
        var_offset = self.base_offset
        for var in self.variables:
            if (
                var.in_local and
                not var.memory_offset and
                not var.is_array()
            ):
                self._print(out)
                var.memory_offset = var_offset
                stack_offset = var_offset - self.physical_sp
                Stack._do_emit(out, var.item, stack_offset)
                self._print(out)
            var_offset = var_offset.push(var.item)

    def flush(self, out: CWriter) -> None:
        self._print(out)
        self.save_variables(out)
        self._save_physical_sp(out)
        out.start_line()

    def is_flushed(self) -> bool:
        for var in self.variables:
            if not var.in_memory():
                return False
        return self.physical_sp == self.logical_sp

    def sp_offset(self) -> str:
        return (self.physical_sp - self.logical_sp).to_c()

    def as_comment(self) -> str:
        variables = ", ".join([v.compact_str() for v in self.variables])
        return (
            f"/* Variables=[{variables}]; base={self.base_offset.to_c()}; sp={self.physical_sp.to_c()}; logical_sp={self.logical_sp.to_c()} */"
        )

    def _print(self, out: CWriter) -> None:
        if PRINT_STACKS:
            out.emit(self.as_comment() + "\n")

    def copy(self) -> "Stack":
        other = Stack()
        other.base_offset = self.base_offset
        other.physical_sp = self.physical_sp
        other.logical_sp = self.logical_sp
        other.variables = [var.copy() for var in self.variables]
        return other

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Stack):
            return NotImplemented
        return (
            self.physical_sp == other.physical_sp
            and self.logical_sp == other.logical_sp
            and self.base_offset == other.base_offset
            and self.variables == other.variables
        )

    def align(self, other: "Stack", out: CWriter) -> None:
        if self.logical_sp != other.logical_sp:
            raise StackError("Cannot align stacks: differing logical top")
        if self.physical_sp == other.physical_sp:
            return
        diff = other.physical_sp - self.physical_sp
        out.start_line()
        out.emit(f"stack_pointer += {diff.to_c()};\n")
        self.physical_sp = other.physical_sp

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
    null = CWriter.null()
    for var in reversed(effect.inputs):
        local = stack.pop(var, null)
        if var.name != "unused":
            locals[local.name] = local
    for var in effect.outputs:
        if var.name in locals:
            local = locals[var.name]
        else:
            local = Local.unused(var, None)
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
    peeks: int
    check_liveness: bool
    spilled: int = 0

    @staticmethod
    def needs_defining(var: Local) -> bool:
        return (
            not var.item.peek and
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
        while len(self.inputs) > self.peeks:
            tos = self.inputs.pop()
            if self.is_live(tos) and self.check_liveness:
                raise StackError(
                    f"Input '{tos.name}' is still live {reason}"
                )
            self.stack.drop(tos.item, self.check_liveness)

    def clear_dead_inputs(self) -> None:
        live = ""
        while len(self.inputs) > self.peeks:
            tos = self.inputs[-1]
            if self.is_live(tos):
                live = tos.name
                break
            self.inputs.pop()
            self.stack.drop(tos.item, self.check_liveness)
        for var in self.inputs[self.peeks:]:
            if not self.is_live(var):
                raise StackError(
                    f"Input '{var.name}' is not live, but '{live}' is"
                )

    def _push_defined_outputs(self) -> None:
        defined_output = ""
        for output in self.outputs:
            if output.in_local and not output.memory_offset and not output.item.peek:
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
        while len(self.outputs) > self.peeks and not self.needs_defining(self.outputs[self.peeks]):
            out = self.outputs.pop(self.peeks)
            self.stack.push(out)

    def locals_cached(self) -> bool:
        for out in self.outputs:
            if out.in_local:
                return True
        return False

    def flush(self, out: CWriter) -> None:
        self._print(out)
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
    def for_uop(stack: Stack, uop: Uop, out: CWriter, check_liveness: bool = True) -> "Storage":
        inputs: list[Local] = []
        peeks: list[Local] = []
        for input in reversed(uop.stack.inputs):
            local = stack.pop(input, out)
            if input.peek:
                peeks.append(local)
            inputs.append(local)
        inputs.reverse()
        peeks.reverse()
        offset = stack.logical_sp - stack.physical_sp
        for ouput in uop.stack.outputs:
            if ouput.is_array() and ouput.used and not ouput.peek:
                c_offset = offset.to_c()
                out.emit(f"{ouput.name} = &stack_pointer[{c_offset}];\n")
            offset = offset.push(ouput)
        for var in inputs:
            stack.push(var)
        outputs = peeks + [ Local.undefined(var) for var in uop.stack.outputs if not var.peek ]
        return Storage(stack, inputs, outputs, len(peeks), check_liveness)

    @staticmethod
    def copy_list(arg: list[Local]) -> list[Local]:
        return [ l.copy() for l in arg ]

    def copy(self) -> "Storage":
        new_stack = self.stack.copy()
        variables = { var.name: var for var in new_stack.variables }
        inputs = [ variables[var.name] for var in self.inputs]
        assert [v.name for v in inputs] == [v.name for v in self.inputs], (inputs, self.inputs)
        return Storage(
            new_stack, inputs, self.copy_list(self.outputs), self.peeks,
            self.check_liveness, self.spilled
        )

    @staticmethod
    def check_names(locals: list[Local]) -> None:
        names: set[str] = set()
        for var in locals:
            if var.name == "unused":
                continue
            if var.name in names:
                raise StackError(f"Duplicate name {var.name}")
            names.add(var.name)

    def sanity_check(self) -> None:
        self.check_names(self.inputs)
        self.check_names(self.outputs)
        self.check_names(self.stack.variables)

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
            self._print(out)
            other._print(out)
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
        if len(self.inputs) > self.peeks and self.check_liveness:
            raise StackError(f"Input variable '{self.inputs[-1].name}' is still live")
        self._push_defined_outputs()
        if self.outputs:
            for out in self.outputs[self.peeks:]:
                if self.needs_defining(out):
                    raise StackError(f"Output variable '{self.outputs[0].name}' is not defined")
                self.stack.push(out)
            self.outputs = []

    def as_comment(self) -> str:
        stack_comment = self.stack.as_comment()
        next_line = "\n                  "
        inputs = ", ".join([var.compact_str() for var in self.inputs])
        outputs = ", ".join([var.compact_str() for var in self.outputs])
        return f"{stack_comment[:-2]}{next_line}inputs: {inputs} outputs: {outputs}*/"

    def _print(self, out: CWriter) -> None:
        if PRINT_STACKS:
            out.emit(self.as_comment() + "\n")

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
                self.stack.save_variables(out)
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
            self.stack.flush(out)
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
            output.memory_offset = lowest.memory_offset  # type: ignore[union-attr]
        else:
            self.stack.flush(out)
        if output is not None:
            self.stack.push(output)
        self.inputs = []
        if output_in_place:
            self.stack.flush(out)
        if output is not None:
            output = self.stack.pop(output.item, out)
