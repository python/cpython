#!/usr/bin/env bash

# Any PyObject exposed via the public API is problematic since it must
# be made per-interpreter.  This involves the following:
#
# singletons:
#  - None
#  - True
#  - False
#  - NotImplemented
#  - Ellipsis
# PyTypeObject:
#  - PyExc*  [97]
#  - static types  [81]
#
# In the non-stable API we could use #defines to do the conversion
# transparently (though Py_None is perhaps problematic for performance
# reasons).  However, we can't take that approach with the stable API.
# That means we must find all functions (& macros) in the stable API
# (and probably the full public API, for sanity sake) and adjust them.
# This will involve internally converting from the public object to the
# corresponding per-interpreter object.
#
# Note that the only place this solution fails is with direct pointer
# equality checks with the public objects.

# XXX What about saying that the stable API is not sub-interpreter
# compatible?


function run_capi() {
    ./python Tools/c-analyzer/c-analyzer.py capi \
        --no-progress \
        --group-by kind \
        --func --inline --macro \
        --no-show-empty \
        --ignore '<must-resolve.ignored>' \
        $@
}

echo ''
echo '#################################################'
echo '# All API'
echo '#################################################'
run_capi --format summary Include/*.h Include/cpython/*.h
run_capi --format table Include/*.h Include/cpython/*.h
echo ''
echo ''
echo '#################################################'
echo '# stable API'
echo '#################################################'
echo ''
echo '# public:'
run_capi --format summary --public --no-show-empty Include/*.h
echo ''
echo '# private:'
run_capi --format summary --private --no-show-empty Include/*.h
echo ''
run_capi --format full -v Include/*.h
#run_capi --format full -v --public Include/*.h
#run_capi --format full -v --private Include/*.h
echo ''
echo '#################################################'
echo '# cpython API'
echo '#################################################'
echo ''
echo '# public:'
run_capi --format summary --public --no-show-empty Include/cpython/*.h
echo ''
echo '# private:'
run_capi --format summary --private --no-show-empty Include/cpython/*.h
echo ''
run_capi --format full -v Include/cpython/*.h
#run_capi --format full -v --public Include/cpython/*.h
#run_capi --format full -v --private Include/cpython/*.h
