- Issue #24467: Fixed possible buffer over-read in bytearray. The bytearray
  object now always allocates place for trailing null byte and it's buffer now
  is always null-terminated.

- Upgrade to Unicode 8.0.0.

- Issue #24345: Add Py_tp_finalize slot for the stable ABI.

- Issue #24400: Introduce a distinct type for PEP 492 coroutines; add
  types.CoroutineType, inspect.getcoroutinestate, inspect.getcoroutinelocals;
  coroutines no longer use CO_GENERATOR flag; sys.set_coroutine_wrapper
  works only for 'async def' coroutines; inspect.iscoroutine no longer
  uses collections.abc.Coroutine, it's intended to test for pure 'async def'
  coroutines only; add new opcode: GET_YIELD_FROM_ITER; fix generators wrapper
  used in types.coroutine to be instance of collections.abc.Generator;
  collections.abc.Awaitable and collections.abc.Coroutine can no longer
  be used to detect generator-based coroutines--use inspect.isawaitable
  instead.

- Issue #24450: Add gi_yieldfrom to generators and cr_await to coroutines.
  Contributed by Benno Leslie and Yury Selivanov.

- Issue #19235: Add new RecursionError exception. Patch by Georg Brandl.

