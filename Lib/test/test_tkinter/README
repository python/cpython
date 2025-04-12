Writing new tests
=================

Precaution
----------

    New tests should always use only one Tk window at once, like all the
    current tests do. This means that you have to destroy the current window
    before creating another one, and clean up after the test. The motivation
    behind this is that some tests may depend on having its window focused
    while it is running to work properly, and it may be hard to force focus
    on your window across platforms (right now only test_traversal at
    test_ttk.test_widgets.NotebookTest depends on this).

