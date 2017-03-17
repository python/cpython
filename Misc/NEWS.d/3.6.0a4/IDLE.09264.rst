Issue #27714: text_textview and test_autocomplete now pass when re-run
in the same process.  This occurs when test_idle fails when run with the
-w option but without -jn.  Fix warning from test_config.