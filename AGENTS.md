# AGENTS.md

Guidance for future agent work in this CPython checkout.

## Work From Repo Evidence

- Start by reading the local code, tests, and recent commits touching the same area. CPython patches are usually narrow and issue-scoped.
- Prefer existing local patterns over new abstractions. If nearby code uses a helper, macro, assertion style, or test support utility, follow it.
- Keep unrelated cleanup out of the patch. Reviewers expect a clear line from issue to code to tests to NEWS/docs.
- Before changing behavior, make the current failure visible with a focused reproducer or a targeted test.

## C Code Changes

- Treat every new object-producing call as an error path. If a call can fail, either propagate the exception or deliberately avoid allocation.
- Be explicit about reference ownership. Pair new references with cleanup on every path, and avoid converting borrowed/constant data into owned objects unless needed.
- For OOM or NULL-handling fixes, preserve the original exception semantics and add only the minimum logic required.
- Do not rely on string formatting, `__qualname__`, or naming conventions when an exact runtime check is available.
- Keep comments sparse and useful. Add comments for non-obvious heuristics and ownership decisions, not for code that reads directly.

## Tests

- Add regression tests next to existing tests for the affected behavior, and keep assertions exact when CPython already has stable wording.
- Include both the positive case and the closest false-positive cases. Descriptor and binding changes usually need instance, classmethod, staticmethod, and metaclass coverage.
- Use descriptive fixtures and method names that make failure output understandable.
- When changing error handling, test the precise error message or exception path rather than only testing that an exception was raised.
- For test support improvements, prefer shared helpers over repeating skip or environment logic in many files.

## Docs And NEWS

- User-visible behavior changes normally need a `Misc/NEWS.d/next/...` entry.
- If a reviewer asks for a What's New note, keep its wording aligned with the NEWS entry, with contributor credit in What's New only.
- Use CPython reST roles such as `:exc:`, `:func:`, and `:gh:` where appropriate.
- Documentation changes should describe the behavior, not the implementation detail.

## Reviews And PR Hygiene

- Fetch and read the full PR conversation before responding to review feedback. Inline review threads may contain follow-up comments that change the desired fix.
- Group review feedback by behavior, not by file. Make sure each requested edge case is either covered by code/tests or explicitly called out as intentionally not addressed.
- After requested changes are made, rerun focused tests and summarize exactly which reviewer concerns were addressed.
- If Bedevere requests the standard review phrase, use it only after the relevant commits are pushed to the PR branch.

## Commands

- Build after C changes:
  ```sh
  make -j8
  ```
- Run a focused test module:
  ```sh
  ./python.exe -m test test_call
  ```
- Check whitespace before committing:
  ```sh
  git diff --check
  ```
- Inspect recent area-specific history:
  ```sh
  git log --oneline --max-count=80 -- Lib/test Python Modules Objects Doc Misc
  ```

## Commits

- Use CPython-style commit messages when possible: `gh-NNNNNN: Short imperative summary`.
- Keep commits reviewable. A small follow-up commit is fine for review-driven naming, docs wording, or missing coverage.
- Do not add promotional or generated-by footers to commits.
- Stage only files intended for the PR. Leave local planning notes and scratch files untracked unless explicitly requested.
