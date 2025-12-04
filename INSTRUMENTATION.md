# Firmament2 Instrumentation

Gates:
- `FIRMAMENT2_ENABLE=1`             # emit events
- `FIRMAMENT2_INCLUDE_CODE_META=1`  # include metadata in CODE_* events

Emitters:
- Tokenizer: Parser/lexer/lexer.c → `emit_tokenizer_event_json` via `_PyTokenizer_Get`
- AST: Parser/asdl_c.py → Python/Python-ast.c → `emit_ast_event_json`
- Source scope: Python/compile.c & Python/pythonrun.c → `_firm2_source_begin/_end`
- Code lifecycle: Objects/codeobject.c → `_firm2_emit_code_create_meta/_destroy_meta` (+ `co_extra` provenance)
- Frame lifecycle: Python/ceval.c → `_firm2_emit_frame_event`
- Common envelope everywhere: `event_id`, `pid`, `tid`, `ts_ns`
