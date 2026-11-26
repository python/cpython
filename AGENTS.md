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

## Opening issues

Do not auto-generate issues with AI tools.

Search GitHub for duplicate issues before opening a new issue. Do not open
duplicates.

```bash
gh search issues --repo python/cpython "<symptom keywords>"
gh search issues --repo python/cpython "<function, module or option name>"
gh issue list --repo python/cpython --state all --label "<topic label>" --search "<short area keywords>"
```
