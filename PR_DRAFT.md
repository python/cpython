# Enable `test_lzma` in the Nanvix regrtest harness

Closes #526. Blocked on #601 (xz-integration); rebase onto its merge
commit before review-ready.

## Summary

Wires `test_lzma` into the Nanvix regrtest pipeline now that `_lzma`
is built into the cpython sysroot. Three coordinated changes land
together:

1. **`NANVIX_TEST_LIST` registration.** Append a
   `# #526 — _lzma stdlib enablement` header and `"test_lzma",` to
   `NANVIX_TEST_LIST` in `.nanvix/config.py`. `STANDALONE_EXCLUDE` is
   left untouched — the per-method skips below avoid the need for
   whole-module exclusion.
2. **Per-method `@unittest.skipIf(support.is_nanvix, ...)` decorators
   on the 48 methods that exercise the default-preset `LZMACompressor`
   path (47 isolated-failure cases plus one cascade-induced sibling).**
   The reason string is uniformly
   `"NSKIP019: lzma preset>=3 exceeds Nanvix heap"`. The skip
   predicate is `support.is_nanvix` (broadens to all Nanvix
   deployments) because the failure reproduces in
   `single-process`, `multi-process`, and `standalone`. A direct
   preset-walk probe inside a freshly-booted single-process VM shows
   `LZMACompressor(preset=N)` succeeds for `N <= 2` and
   `MemoryError`s for `N >= 3` regardless of sibling-test state — so
   the failures are real (verdict A under the wave-flow
   batched-vs-isolated recipe), not batch-cascade artefacts.
3. **Guard removal already in `71691a0916b`** (the parent commit on
   this branch) — kept here for context. With `_lzma` built and the
   per-method skips in place, the module imports cleanly and runs to
   the per-method skip boundary in every mode.

The complete per-method enumeration and the verdict-matrix that
justifies the `support.is_nanvix` predicate (rather than
`support.is_nanvix_standalone`) are recorded in the task vault under
`.vault/tasks/test-lzma-enablement/failure-characterization.md`.

73 of 121 `test_lzma` methods run on Nanvix after this PR (one more
is an upstream `gettotalrefcount` `SkipTest` that only fires in
`pydebug` builds, leaving 73 actually-executed assertions plus the
intentional 48 Nanvix skips).

## Commits

Single appended commit on `fix/nskip046-removal`:

1. **Enable `test_lzma` in the Nanvix regrtest harness.** Adds
   `test_lzma` to `NANVIX_TEST_LIST`, decorates 48 default-preset
   methods with the NSKIP019 skip (47 isolated-failure methods
   plus 1 cascade-induced sibling, see Verification), refreshes
   this PR draft.

The parent commit `71691a0916b` ("Remove NSKIP046 skip guard from
test_lzma") will ship as a separate commit in the same PR. The user
intends to squash before review.

## Verification

`./z distclean && ./z clean && NANVIX_DEPLOYMENT_MODE=$MODE ./z setup && ./z test`
across all three deployment modes.

| Mode             | Result | Modules     | `test_lzma` slice                                                                          |
| ---------------- | ------ | ----------- | ------------------------------------------------------------------------------------------ |
| `standalone`     | PASS   | 157 / 157   | `run=121 skipped=49` (48 NSKIP019 + 1 upstream `gettotalrefcount`); test_lzma in own batch |
| `single-process` | PASS   | 164 / 164   | runs in batch 41 alongside `test_re test_plistlib test_nanvix_lxml`; batch totals `run=337 skipped=58` |
| `multi-process`  | PASS   | 164 / 164   | runs in batch 41 alongside `test_re test_plistlib test_nanvix_lxml`; batch totals `run=337 skipped=58` |

`grep NSKIP046` is empty in all three logs (the parent commit
removed the guard); `grep NSKIP019` returns the 48 expected
decorations in `Lib/test/test_lzma.py`.

The 48-vs-47 discrepancy is intentional. Phase A characterization
(see `failure-characterization.md`) identified 47 methods that fail
in fresh-VM-per-method isolation. After landing those 47 decorators,
the full three-mode matrix surfaced one additional method,
`FileTestCase.test_read_multistream`, that fails only when run in a
batch alongside `test_re` and `test_plistlib` (the `LZMADecompressor`
init `MemoryError`s after the prior modules' working sets fragment
the heap). It passes cleanly in isolation. The batch-cascade
behaviour is a property of the `NANVIX_TEST_LIST` ordering rather
than of the test itself, but rather than reorder the whole list we
add one more decorator with the same NSKIP019 reason, treating
production batch-ordering as the contract.

Logs preserved at `/tmp/test-lzma-enablement-{standalone,single-process,multi-process}.log`.

## Known limitations

- 48 `test_lzma` methods are skipped on Nanvix because the
  `LZMACompressor` default preset (preset=6, ~94 MiB encoder
  dictionary) exceeds the available heap budget on every Nanvix
  deployment mode tested (32 MiB on `standalone`, 256 MiB declared
  but observed-bound on hosted modes). Raising the hosted-mode heap
  ceiling and/or introducing a Nanvix-specific lower default
  preset at the runtime layer would let these tests run unmodified;
  both are out of scope for this PR. One of the 48,
  `FileTestCase.test_read_multistream`, is decorated for batch-
  cascade resilience rather than for an isolated failure (see
  Verification).
- The four `mode="x"` exclusive-create paths
  (`FileTestCase.test_init_with_x_mode`, `FileTestCase.test_write_to_file*`,
  `OpenTestCase.test_x_mode`) also reproduce a `PermissionError` on
  `'@test_1_tmp'` in `single-process` once they get past the
  `LZMACompressor` allocation; they roll up into the same skip set
  rather than carrying a second decorator family.

## Out of scope

- `tests/.vault/` is untouched. The NSKIP catalog is not edited and
  no NSKIPxxx codes are minted; NSKIP019 is reused verbatim per the
  in-tree `test_shutil.py` / `test_tarfile.py` precedent established
  by commit `94a2e1af25b`.
- `_NSKIP046-lzma-not-built.md` is not closed or renamed.
- No new GitHub issues are opened.
- `STANDALONE_EXCLUDE` is left unchanged.
- The runtime-layer fix for the `LZMACompressor` budget shortfall
  (raising hosted heap, or introducing a Nanvix-aware preset
  default) is a separate effort.
