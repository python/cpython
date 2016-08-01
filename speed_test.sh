#!/usr/bin/env bash

echo "dictkeys & dictkeys"
./python \
    -m timeit \
    -s "dk1 = dict.fromkeys(range(10000000), 0).keys(); dk2 = {0: 0}.keys()" \
    "dk1 & dk2"

./python \
    -m timeit \
    -s "dk1 = dict.fromkeys(range(10000000), 0).keys(); dk2 = {0: 0}.keys()" \
    "dk2 & dk1"


echo "dictkeys & dictitems"
./python \
    -m timeit \
    -s "dk = {(i, i): 0 for i in range(10000000)}.keys(); di = {0: 0}.items()" \
    "dk & di"

./python \
    -m timeit \
    -s "dk = {(i, i): 0 for i in range(10000000)}.keys(); di = {0: 0}.items()" \
    "di & dk"

./python \
    -m timeit \
    -s "di = dict.fromkeys(range(10000000), 0).items(); dk = {(0, 0): 0}.keys()" \
    "dk & di"

./python \
    -m timeit \
    -s "di = dict.fromkeys(range(10000000), 0).items(); dk = {(0, 0): 0}.keys()" \
    "di & dk"



echo "dictkeys & set"
./python \
    -m timeit \
    -s "dk = dict.fromkeys(range(10000000), 0).keys(); s = {0}" \
    "dk & s"

./python \
    -m timeit \
    -s "dk = dict.fromkeys(range(10000000), 0).keys(); s = {0}" \
    "s & dk"

./python \
    -m timeit \
    -s "s = set(range(10000000)); dk = {0: 0}.keys()" \
    "dk & s"

./python \
    -m timeit \
    -s "s = set(range(10000000)); dk = {0: 0}.keys()" \
    "s & dk"




echo "dictitems & dictitems"
./python \
    -m timeit \
    -s "di1 = dict.fromkeys(range(10000000), 0).items(); di2 = {0: 0}.items()" \
    "di1 & di2"

./python \
    -m timeit \
    -s "di1 = dict.fromkeys(range(10000000), 0).items(); di2 = {0: 0}.items()" \
    "di2 & di1"


echo "dictitems & set"
./python \
    -m timeit \
    -s "di = dict.fromkeys(range(10000000), 0).items(); s = {(0, 0)}" \
    "di & s"

./python \
    -m timeit \
    -s "di = dict.fromkeys(range(10000000), 0).items(); s = {(0, 0)}" \
    "s & di"

./python \
    -m timeit \
    -s "s = {(i, i) for i in range(10000000)}; di = {0: 0}.items()" \
    "s & di"

./python \
    -m timeit \
    -s "s = {(i, i) for i in range(10000000)}; di = {0: 0}.items()" \
    "di & s"


# RESULTS:
# dictkeys & dictkeys
# 1000000 loops, best of 3: 0.466 usec per loop
# 1000000 loops, best of 3: 0.466 usec per loop
# dictkeys & dictitems
# 1000000 loops, best of 3: 0.581 usec per loop
# 1000000 loops, best of 3: 0.775 usec per loop
# 1000000 loops, best of 3: 0.665 usec per loop
# 1000000 loops, best of 3: 0.688 usec per loop
# dictkeys & set
# 1000000 loops, best of 3: 0.609 usec per loop
# 1000000 loops, best of 3: 0.643 usec per loop
# 1000000 loops, best of 3: 0.89 usec per loop
# 1000000 loops, best of 3: 0.685 usec per loop
# dictitems & dictitems
# 1000000 loops, best of 3: 0.543 usec per loop
# 1000000 loops, best of 3: 0.544 usec per loop
# dictitems & set
# 1000000 loops, best of 3: 0.659 usec per loop
# 1000000 loops, best of 3: 0.684 usec per loop
# 1000000 loops, best of 3: 0.832 usec per loop
# 1000000 loops, best of 3: 0.796 usec per loop
