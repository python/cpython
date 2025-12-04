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

## Build, install, and test on Linux

The emitters are part of the normal CPython binary. On a Debian/Ubuntu system, install prerequisites and build the tree at the repository root:

```bash
sudo apt update
sudo apt install build-essential gdb lcov pkg-config \
     libbz2-dev libffi-dev libgdbm-dev libgdbm-compat-dev \
     liblzma-dev libncursesw5-dev libreadline6-dev libsqlite3-dev \
     libssl-dev tk-dev uuid-dev xz-utils zlib1g-dev

./configure
make -j$(nproc)
make test         # optional, runs the standard CPython suite
sudo make install # installs the instrumented interpreter as python3
```

To exercise the emitters, set `FIRMAMENT2_ENABLE=1` and run any script. The events print to stdout in NDJSON format.

- **Tokenizer NDJSON** (one line per token):
  ```bash
  FIRMAMENT2_ENABLE=1 ./python -c "x = 1 + 2" > tokens.ndjson
  head tokens.ndjson
  ```

- **AST NDJSON** (one line per node):
  ```bash
  FIRMAMENT2_ENABLE=1 ./python - <<'PY' > ast.ndjson
  def add(a, b):
      return a + b
  PY
  head ast.ndjson
  ```

The emitted JSON includes the common envelope (`event_id`, `pid`, `tid`, `ts_ns`) plus the emitter-specific payload fields.
