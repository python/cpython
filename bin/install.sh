#!/bin/sh
# Installs.

./configure --prefix=/usr/local --with-openssl=/usr/local &&\
 make -s -j10 &&\
 sudo make install
