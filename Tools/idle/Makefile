# Makefile to build the interrupt module, which is a C extension

PYTHON=python2.3

all: interrupt.so

interrupt.so: interruptmodule.c
	$(PYTHON) setup.py build_ext -i
