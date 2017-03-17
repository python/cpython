Issue #27245: IDLE: Cleanly delete custom themes and key bindings.
Previously, when IDLE was started from a console or by import, a cascade
of warnings was emitted.  Patch by Serhiy Storchaka.