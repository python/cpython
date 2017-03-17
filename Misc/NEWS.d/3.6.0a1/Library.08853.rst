Issue #21827: Fixed textwrap.dedent() for the case when largest common
whitespace is a substring of smallest leading whitespace.
Based on patch by Robert Li.