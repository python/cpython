---
name: Crash report
about: A hard crash of the interpreter, possibly with a core dump
labels: "type-crash"
---

<!--
  Use this template for hard crashes of the interpreter, segmentation faults, failed C-level assertions, and similar.
  Do not submit this form if you encounter an exception being unexpectedly raised from a Python function.
  Most of the time, these should be filed as bugs, rather than crashes.

  The CPython interpreter is itself written in a different programming language, C.
  For CPython, a "crash" is when Python itself fails, leading to a traceback in the C stack.
-->

**Crash report**

Tell us what happened, ideally including a minimal, reproducible example (https://stackoverflow.com/help/minimal-reproducible-example).

**Error messages**

Enter any relevant error message caused by the crash, including a core dump if there is one.

**Your environment**

<!-- Include as many relevant details as possible about the environment you experienced the bug in -->

- CPython versions tested on:
- Operating system and architecture:

<!--
You can freely edit this text. Remove any lines you believe are unnecessary.
-->
