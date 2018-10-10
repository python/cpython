#! /bin/bash

make clean
./configure
make -j
make test
sudo make install
