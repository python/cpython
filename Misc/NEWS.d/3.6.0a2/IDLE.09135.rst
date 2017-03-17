Issue #27196: Stop 'ThemeChanged' warnings when running IDLE tests.
These persisted after other warnings were suppressed in #20567.
Apply Serhiy Storchaka's update_idletasks solution to four test files.
Record this additional advice in idle_test/README.txt