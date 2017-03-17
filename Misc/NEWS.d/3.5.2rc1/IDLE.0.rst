- Issue #5124: Paste with text selected now replaces the selection on X11.
  This matches how paste works on Windows, Mac, most modern Linux apps,
  and ttk widgets.  Original patch by Serhiy Storchaka.

- Issue #24759: Make clear in idlelib.idle_test.__init__ that the directory
  is a private implementation of test.test_idle and tool for maintainers.

- Issue #27196: Stop 'ThemeChanged' warnings when running IDLE tests.
  These persisted after other warnings were suppressed in #20567.
  Apply Serhiy Storchaka's update_idletasks solution to four test files.
  Record this additional advice in idle_test/README.txt

- Issue #20567: Revise idle_test/README.txt with advice about avoiding
  tk warning messages from tests.  Apply advice to several IDLE tests.

- Issue #27117: Make colorizer htest and turtledemo work with dark themes.
  Move code for configuring text widget colors to a new function.

- Issue #26673: When tk reports font size as 0, change to size 10.
  Such fonts on Linux prevented the configuration dialog from opening.

- Issue #21939: Add test for IDLE's percolator.
  Original patch by Saimadhav Heblikar.

- Issue #21676: Add test for IDLE's replace dialog.
  Original patch by Saimadhav Heblikar.

- Issue #18410: Add test for IDLE's search dialog.
  Original patch by Westley Martínez.

- Issue #21703: Add test for IDLE's undo delegator.
  Original patch by Saimadhav Heblikar .

- Issue #27044: Add ConfigDialog.remove_var_callbacks to stop memory leaks.

- Issue #23977: Add more asserts to test_delegator.

- Issue #20640: Add tests for idlelib.configHelpSourceEdit.
  Patch by Saimadhav Heblikar.

- In the 'IDLE-console differences' section of the IDLE doc, clarify
  how running with IDLE affects sys.modules and the standard streams.

- Issue #25507: fix incorrect change in IOBinding that prevented printing.
  Augment IOBinding htest to include all major IOBinding functions.

- Issue #25905: Revert unwanted conversion of ' to ’ RIGHT SINGLE QUOTATION
  MARK in README.txt and open this and NEWS.txt with 'ascii'.
  Re-encode CREDITS.txt to utf-8 and open it with 'utf-8'.

