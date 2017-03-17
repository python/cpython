- Issue #15308: Add 'interrupt execution' (^C) to Shell menu.
  Patch by Roger Serwy, updated by Bayard Randel.

- Issue #27922: Stop IDLE tests from 'flashing' gui widgets on the screen.

- Add version to title of IDLE help window.

- Issue #25564: In section on IDLE -- console differences, mention that
  using exec means that __builtins__ is defined for each statement.

- Issue #27714: text_textview and test_autocomplete now pass when re-run
  in the same process.  This occurs when test_idle fails when run with the
  -w option but without -jn.  Fix warning from test_config.

- Issue #25507: IDLE no longer runs buggy code because of its tkinter imports.
  Users must include the same imports required to run directly in Python.

- Issue #27452: add line counter and crc to IDLE configHandler test dump.

- Issue #27365: Allow non-ascii chars in IDLE NEWS.txt, for contributor names.

- Issue #27245: IDLE: Cleanly delete custom themes and key bindings.
  Previously, when IDLE was started from a console or by import, a cascade
  of warnings was emitted.  Patch by Serhiy Storchaka.

