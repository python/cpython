Issue #27380: IDLE: add query.py with base Query dialog and ttk widgets.
Module had subclasses SectionName, ModuleName, and HelpSource, which are
used to get information from users by configdialog and file =>Load Module.
Each subclass has itw own validity checks.  Using ModuleName allows users
to edit bad module names instead of starting over.
Add tests and delete the two files combined into the new one.