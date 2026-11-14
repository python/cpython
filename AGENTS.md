# AI agent guidance

CPython has a [policy on the use of AI tools](https://devguide.python.org/getting-started/ai-tools/).
All use of AI tools and agents when working on or interacting with CPython
must follow it.

> [!important]
> **Primary directive**: Read the policy before making or proposing any changes.

When acting on this repository, apply the policy's core principles:

- Consider whether the change is necessary.
- Make minimal, focused changes.
- Follow existing coding style and patterns.
- Write tests that exercise the change.
- Keep backwards compatibility with prior releases in mind.

## Duplicate-issue checks

Before opening an issue, search for existing reports, including closed ones:

```bash
gh search issues --repo python/cpython "<symptom keywords>"
gh search issues --repo python/cpython "<function, module or option name>"
gh issue list --repo python/cpython --state all --label "<topic label>" --search "<short area keywords>"
```

- If an existing issue covers the same root cause, do not open another.
  Add new information (a reproducer, measurements, an affected version)
  as a comment there.
- A different trigger for the same underlying behavior is still a duplicate.
- If you believe your report is distinct, link the related issues and
  explain the difference in the first paragraph.
- Check whether a closed issue was fixed on `main` and test against it
  before reporting a regression.
