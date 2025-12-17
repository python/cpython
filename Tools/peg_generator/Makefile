UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	PYTHON ?= ../../python
endif
ifeq ($(UNAME_S),Darwin)
	PYTHON ?= ../../python.exe
endif
VENVDIR ?= ./venv
VENVPYTHON ?= $(VENVDIR)/bin/python
CPYTHON ?= ../../Lib
MYPY ?= ./venv/bin/mypy
BLACK ?= ./venv/bin/black

GRAMMAR = ../../Grammar/python.gram
TOKENS = ../../Grammar/Tokens

build: peg_extension/parse.c

peg_extension/parse.c: $(GRAMMAR) $(TOKENS) pegen/*.py peg_extension/peg_extension.c ../../Parser/pegen.c ../../Parser/pegen_errors.c ../../Parser/string_parser.c ../../Parser/action_helpers.c ../../Parser/*.h pegen/grammar_parser.py
	$(VENVPYTHON) -m pegen -q c $(GRAMMAR) $(TOKENS) -o peg_extension/parse.c --compile-extension

clean:
	-rm -f peg_extension/*.o peg_extension/*.so peg_extension/parse.c
	-rm -rf $(VENVDIR)

regen-metaparser: pegen/metagrammar.gram pegen/*.py
	$(VENVPYTHON) -m pegen -q python pegen/metagrammar.gram -o pegen/grammar_parser.py

# Note: These targets really depend on the generated shared object in peg_extension/parse.*.so but
# this has different names in different systems so we are abusing the implicit dependency on
# parse.c by the use of --compile-extension.

venv:
	$(PYTHON) -m venv $(VENVDIR)
	$(VENVPYTHON) -m pip install -U pip setuptools black mypy
	@echo "The venv has been created in the $(VENVDIR) directory"

mypy: regen-metaparser
	$(MYPY)  # For list of files, see mypy.ini

format-python:
	$(BLACK) pegen

format: venv format-python

tags: TAGS

TAGS: pegen/*.py test/test_pegen.py
	etags pegen/*.py test/test_pegen.py
