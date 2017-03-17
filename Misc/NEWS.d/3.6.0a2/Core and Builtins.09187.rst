Issue #27286: Fixed compiling BUILD_MAP_UNPACK_WITH_CALL opcode.  Calling
function with generalized unpacking (PEP 448) and conflicting keyword names
could cause undefined behavior.