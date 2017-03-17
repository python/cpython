Issue #26823: traceback.StackSummary.format now abbreviates large sections of
repeated lines as "[Previous line repeated {count} more times]" (this change
then further affects other traceback display operations in the module). Patch
by Emanuel Barry.