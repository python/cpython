Issue #26837: assertSequenceEqual() now correctly outputs non-stringified
differing items (like bytes in the -b mode).  This affects assertListEqual()
and assertTupleEqual().