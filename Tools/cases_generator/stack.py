import sys
from analyzer import StackItem
from dataclasses import dataclass
from formatting import maybe_parenthesize

def var_size(var):
    if var.condition:
        # Special case simplification
        if var.condition == "oparg & 1" and var.size == "1":
            return f"({var.condition})"
        else:
            return f"(({var.condition}) ? {var.size} : 0)"
    else:
        return var.size

@dataclass
class StackOffset:
    "The stack offset of the virtual base of the stack from the physical stack pointer"

    def __init__(self):
        self.popped: list[str] = []
        self.pushed: list[str] = []

    def pop(self, item: StackItem):
        self.popped.append(var_size(item))

    def push(self, item: StackItem):
        self.pushed.append(var_size(item))

    def simplify(self):
        if not self.popped or not self.pushed:
            return
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
                self.popped.append(pop)
                pushed.append(push)
            else:
                self.pushed.append(push)
                popped.append(pop)
        self.popped.extend(popped)
        self.pushed.extend(pushed)

    def to_c(self):
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

    def clear(self):
        self.popped = []
        self.pushed = []
