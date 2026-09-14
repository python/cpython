"""Render prepared C rules with rule-local output and cleanup state."""

from collections.abc import Iterator
from contextlib import AbstractContextManager, contextmanager
from dataclasses import dataclass
from typing import Protocol

from pegen.c_generator_model import CAlternative, CRule
from pegen.grammar import RuleKind


class CWriter(Protocol):
    def print(self, *args: object) -> None:
        ...

    def indent(self) -> AbstractContextManager[None]:
        ...


@dataclass(frozen=True, slots=True)
class _CReturnEmitter:
    writer: CWriter
    cleanups: tuple[str, ...] = ()

    def with_cleanup(self, cleanup: str) -> "_CReturnEmitter":
        return _CReturnEmitter(self.writer, (cleanup, *self.cleanups))

    def emit(self, value: str) -> None:
        for cleanup in self.cleanups:
            self.writer.print(cleanup)
        self.writer.print("p->level--;")
        self.writer.print(f"return {value};")

    def check_memory(self, expr: str) -> None:
        self.writer.print(f"if ({expr}) {{")
        with self.writer.indent():
            self.no_memory()
        self.writer.print("}")

    def no_memory(self) -> None:
        self.writer.print("p->error_indicator = 1;")
        self.writer.print("PyErr_NoMemory();")
        self.emit("NULL")


class _LoopBuffer:
    """Temporary repetition storage and the exits that release it."""

    _release = "PyMem_Free(_children);"

    def __init__(self, writer: CWriter, returns: _CReturnEmitter):
        self._print = writer.print
        self._indent = writer.indent
        self._returns = returns
        self.error_returns = returns.with_cleanup(self._release)

    def initialize(self) -> None:
        self._print("void **_children = PyMem_Malloc(sizeof(void *));")
        self._returns.check_memory("!_children")
        self._print("Py_ssize_t _children_capacity = 1;")
        self._print("Py_ssize_t _n = 0;")

    def append(self, value: str) -> None:
        self._print("if (_n == _children_capacity) {")
        with self._indent():
            self._print("_children_capacity *= 2;")
            self._print(
                "void **_new_children = PyMem_Realloc(_children, _children_capacity*sizeof(void *));"
            )
            self._check_memory("!_new_children")
            self._print("_children = _new_children;")
        self._print("}")
        self._print(f"_children[_n++] = {value};")

    def finish(self, *, require_one: bool) -> str:
        if require_one:
            self._print("if (_n == 0 || p->error_indicator) {")
            with self._indent():
                self.error_returns.emit("NULL")
            self._print("}")
        self._print("asdl_seq *_seq = (asdl_seq*)_Py_asdl_generic_seq_new(_n, p->arena);")
        self._check_memory("!_seq")
        self._print("for (Py_ssize_t i = 0; i < _n; i++) asdl_seq_SET_UNTYPED(_seq, i, _children[i]);")
        self._print(self._release)
        return "_seq"

    def _check_memory(self, expr: str) -> None:
        self._print(f"if ({expr}) {{")
        with self._indent():
            self._print(self._release)
            self._returns.no_memory()
        self._print("}")


class CRuleEmitter:
    def __init__(self, writer: CWriter, rule: CRule, *, debug: bool = False):
        self._writer = writer
        self._print = writer.print
        self._indent = writer.indent
        self._rule = rule
        self._debug = debug
        self._returns = _CReturnEmitter(writer)

    def emit(self) -> None:
        rule = self._rule
        signature = rule.signature
        result_type = signature.c_return_type
        for line in rule.text.splitlines():
            self._print(f"// {line}")
        if rule.left_recursive and rule.leader:
            self._print(f"static {result_type} {signature.name}_raw(Parser *);")
        self._print(f"static {result_type}")
        self._print(f"{signature.name}_rule(Parser *p)")
        if rule.left_recursive and rule.leader:
            self._emit_left_recursive_wrapper()
        self._print("{")
        with self._invalid_rule_context():
            match signature.kind:
                case RuleKind.LOOP0 | RuleKind.LOOP1:
                    self._emit_loop_body()
                case RuleKind.NORMAL | RuleKind.GATHER:
                    self._emit_rule_body()
        self._print("}")

    @contextmanager
    def _invalid_rule_context(self) -> Iterator[None]:
        if not self._rule.disable_invalid_rules:
            yield
            return
        with self._indent():
            self._print("int _prev_call_invalid = p->call_invalid_rules;")
            self._print("p->call_invalid_rules = 0;")
        previous_returns = self._returns
        self._returns = previous_returns.with_cleanup(
            "p->call_invalid_rules = _prev_call_invalid;"
        )
        try:
            yield
        finally:
            self._returns = previous_returns

    def _emit_left_recursive_wrapper(self) -> None:
        signature = self._rule.signature
        result_type = signature.c_return_type
        self._print("{")
        with self._indent():
            self._emit_recursion_check()
            self._print(f"{result_type} _res = NULL;")
            self._print(f"if (_PyPegen_is_memoized(p, {signature.name}_type, &_res)) {{")
            with self._indent():
                self._returns.emit("_res")
            self._print("}")
            self._print("int _mark = p->mark;")
            self._print("int _resmark = p->mark;")
            self._print(f"Memo *_memo = _PyPegen_insert_memo_direct(p, _mark, {signature.name}_type);")
            self._print("if (_memo == NULL) {")
            with self._indent():
                self._returns.emit("NULL")
            self._print("}")
            self._print("while (1) {")
            with self._indent():
                self._print("_memo->node = _res;")
                self._print("_memo->mark = p->mark;")
                self._print("p->mark = _mark;")
                self._print(f"void *_raw = {signature.name}_raw(p);")
                self._print("if (p->error_indicator) {")
                with self._indent():
                    self._returns.emit("NULL")
                self._print("}")
                self._print("if (_raw == NULL || p->mark <= _resmark)")
                with self._indent():
                    self._print("break;")
                self._print("_resmark = p->mark;")
                self._print("_res = _raw;")
            self._print("}")
            self._print("p->mark = _resmark;")
            self._returns.emit("_res")
        self._print("}")
        self._print(f"static {result_type}")
        self._print(f"{signature.name}_raw(Parser *p)")

    def _emit_rule_body(self) -> None:
        signature = self._rule.signature
        memoize = self._rule.memoize
        result_type = signature.c_return_type

        with self._indent():
            self._emit_recursion_check()
            self._emit_error_check()
            self._print(f"{result_type} _res = NULL;")
            if memoize:
                self._print(f"if (_PyPegen_is_memoized(p, {signature.name}_type, &_res)) {{")
                with self._indent():
                    self._returns.emit("_res")
                self._print("}")
            self._print("int _mark = p->mark;")
            if self._rule.uses_locations:
                self._emit_token_start_metadata()
            for alt in self._rule.alternatives:
                with self._alternative(alt):
                    self._emit_normal_alt(alt)
            if self._debug:
                self._print(f'D(fprintf(stderr, "Fail at %d: {signature.name}\\n", p->mark));')
            self._print("_res = NULL;")
        self._print("  done:")
        with self._indent():
            if memoize:
                self._print(f"_PyPegen_insert_memo(p, _mark, {signature.name}_type, _res);")
            self._returns.emit("_res")

    def _emit_loop_body(self) -> None:
        rule = self._rule
        signature = rule.signature
        buffer = _LoopBuffer(self._writer, self._returns)
        with self._indent():
            self._emit_recursion_check()
            self._emit_error_check()
            self._print("void *_res = NULL;")
            if rule.memoize:
                self._print(f"if (_PyPegen_is_memoized(p, {signature.name}_type, &_res)) {{")
                with self._indent():
                    self._returns.emit("_res")
                self._print("}")
            self._print("int _mark = p->mark;")
            if rule.memoize:
                self._print("int _start_mark = p->mark;")
            buffer.initialize()
            if rule.uses_locations:
                self._emit_token_start_metadata()
            alt, = rule.alternatives
            with self._alternative(alt):
                self._emit_loop_alt(alt, buffer)
            result = buffer.finish(require_one=signature.kind is RuleKind.LOOP1)
            if rule.memoize:
                self._print(f"_PyPegen_insert_memo(p, _start_mark, {signature.name}_type, {result});")
            self._returns.emit(result)

    @contextmanager
    def _alternative(self, alt: CAlternative) -> Iterator[None]:
        rulename = self._rule.signature.name
        if alt.requires_invalid_rules:
            self._print(f"if (p->call_invalid_rules) {{ // {alt.text}")
        else:
            self._print(f"{{ // {alt.text}")
        with self._indent():
            self._emit_error_check()
            node_str = alt.text.replace('"', '\\"')
            self._print(
                f'D(fprintf(stderr, "%*c> {rulename}[%d-%d]: %s\\n", p->level, \' \', _mark, p->mark, "{node_str}"));'
            )
            for variable in sorted(alt.variables, key=lambda var: var.name):
                ctype = variable.type + " " if variable.type else "void *"
                initializer = (
                    f" = {variable.initializer}" if variable.initializer is not None else ""
                )
                self._print(f"{ctype}{variable.name}{initializer};")
                if variable.unused:
                    self._print(f"UNUSED({variable.name}); // Silence compiler warnings")

            yield

            self._print("p->mark = _mark;")
            self._print(
                f"D(fprintf(stderr, \"%*c%s {rulename}[%d-%d]: %s failed!\\n\", p->level, ' ',\n"
                f'                  p->error_indicator ? "ERROR!" : "-", _mark, p->mark, "{node_str}"));'
            )
            if alt.cut_variable is not None:
                self._print(f"if ({alt.cut_variable}) {{")
                with self._indent():
                    self._returns.emit("NULL")
                self._print("}")
        self._print("}")

    def _emit_conditions(self, keyword: str, alt: CAlternative) -> None:
        self._print(f"{keyword} (")
        with self._indent():
            for index, call in enumerate(alt.calls):
                if index:
                    self._print("&&")
                self._print(call)
        self._print(")")

    def _emit_normal_alt(self, alt: CAlternative) -> None:
        rulename = self._rule.signature.name
        self._emit_conditions(keyword="if", alt=alt)
        self._print("{")
        with self._indent():
            node_str = alt.text.replace('"', '\\"')
            self._print(
                f'D(fprintf(stderr, "%*c+ {rulename}[%d-%d]: %s succeeded!\\n", p->level, \' \', _mark, p->mark, "{node_str}"));'
            )
            self._emit_alt_action(alt, self._returns)

            self._print("goto done;")
        self._print("}")

    def _emit_loop_alt(self, alt: CAlternative, buffer: _LoopBuffer) -> None:
        self._emit_conditions(keyword="while", alt=alt)
        self._print("{")
        with self._indent():
            self._emit_alt_action(alt, buffer.error_returns)
            buffer.append("_res")
            self._print("_mark = p->mark;")
        self._print("}")

    def _emit_alt_action(self, alt: CAlternative, error_returns: _CReturnEmitter) -> None:
        # Location failures and explicit-action failures have distinct cleanup
        # paths in the generated parser. Keep their return contexts separate.
        if alt.uses_locations:
            self._emit_token_end_metadata()
        if not alt.action.checked:
            self._emit_action_debug(alt)
        self._print(f"_res = {alt.action.expression};")
        if alt.action.checked:
            self._print("if ((_res == NULL || p->error_indicator) && PyErr_Occurred()) {")
            with self._indent():
                self._print("p->error_indicator = 1;")
                error_returns.emit("NULL")
            self._print("}")
            self._emit_action_debug(alt)

    def _emit_action_debug(self, alt: CAlternative) -> None:
        if self._debug and alt.action.debug_message:
            self._print(
                f'D(fprintf(stderr, "{alt.action.debug_message}\\n", _mark, p->mark, "{alt.text}"));'
            )

    def _emit_token_start_metadata(self) -> None:
        self._print("if (p->mark == p->fill && _PyPegen_fill_token(p) < 0) {")
        with self._indent():
            self._print("p->error_indicator = 1;")
            self._returns.emit("NULL")
        self._print("}")
        self._print("int _start_lineno = p->tokens[_mark]->lineno;")
        self._print("UNUSED(_start_lineno); // Only used by EXTRA macro")
        self._print("int _start_col_offset = p->tokens[_mark]->col_offset;")
        self._print("UNUSED(_start_col_offset); // Only used by EXTRA macro")

    def _emit_token_end_metadata(self) -> None:
        self._print("Token *_token = _PyPegen_get_last_nonnwhitespace_token(p);")
        self._print("if (_token == NULL) {")
        with self._indent():
            self._returns.emit("NULL")
        self._print("}")
        self._print("int _end_lineno = _token->end_lineno;")
        self._print("UNUSED(_end_lineno); // Only used by EXTRA macro")
        self._print("int _end_col_offset = _token->end_col_offset;")
        self._print("UNUSED(_end_col_offset); // Only used by EXTRA macro")

    def _emit_error_check(self) -> None:
        self._print("if (p->error_indicator) {")
        with self._indent():
            self._returns.emit("NULL")
        self._print("}")

    def _emit_recursion_check(self) -> None:
        self._print("if (p->level++ == MAXSTACK || _Py_ReachedRecursionLimitWithMargin(PyThreadState_Get(), 1)) {")
        with self._indent():
            self._print("_Pypegen_stack_overflow(p);")
        self._print("}")
