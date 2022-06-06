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
MYPY ?= mypy

GRAMMAR = ../../Grammar/python.gram
TOKENS = ../../Grammar/Tokens
TESTFILE = data/cprog.py
TIMEFILE = data/xxl.py
TESTDIR = .
TESTFLAGS = --short

data/xxl.py:
	$(PYTHON) -m zipfile -e data/xxl.zip data

build: peg_extension/parse.c

peg_extension/parse.c: $(GRAMMAR) $(TOKENS) pegen/*.py peg_extension/peg_extension.c ../../Parser/pegen.c ../../Parser/pegen_errors.c ../../Parser/string_parser.c ../../Parser/action_helpers.c ../../Parser/*.h pegen/grammar_parser.py
	$(PYTHON) -m pegen -q c $(GRAMMAR) $(TOKENS) -o peg_extension/parse.c --compile-extension

clean:
	-rm -f peg_extension/*.o peg_extension/*.so peg_extension/parse.c
	-rm -f data/xxl.py
	-rm -rf $(VENVDIR)

dump: peg_extension/parse.c
	cat -n $(TESTFILE)
	$(PYTHON) -c "from peg_extension import parse; import ast; t = parse.parse_file('$(TESTFILE)', mode=1); print(ast.dump(t))"

regen-metaparser: pegen/metagrammar.gram pegen/*.py
	$(PYTHON) -m pegen -q python pegen/metagrammar.gram -o pegen/grammar_parser.py

# Note: These targets really depend on the generated shared object in peg_extension/parse.*.so but
# this has different names in different systems so we are abusing the implicit dependency on
# parse.c by the use of --compile-extension.

.PHONY: test

venv:
	$(PYTHON) -m venv $(VENVDIR)
	$(VENVPYTHON) -m pip install -U pip setuptools
	$(VENVPYTHON) -m pip install -r requirements.pip
	@echo "The venv has been created in the $(VENVDIR) directory"

test: run

run: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TESTFILE)'); exec(t)"

compile: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TESTFILE)', mode=2)"

parse: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TESTFILE)', mode=1)"

check: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TESTFILE)', mode=0)"

stats: peg_extension/parse.c data/xxl.py
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TIMEFILE)', mode=0); parse.dump_memo_stats()" >@data
	$(PYTHON) scripts/joinstats.py @data

time: time_compile

time_compile: venv data/xxl.py
	$(VENVPYTHON) scripts/benchmark.py --target=xxl compile

time_parse: venv data/xxl.py
	$(VENVPYTHON) scripts/benchmark.py --target=xxl parse

time_peg_dir: venv
	$(VENVPYTHON) scripts/test_parse_directory.py \
		-d $(TESTDIR) \
		$(TESTFLAGS) \
		--exclude "*/failset/*" \
		--exclude "*/failset/**" \
		--exclude "*/failset/**/*"

time_stdlib: $(CPYTHON) venv
	$(VENVPYTHON) scripts/test_parse_directory.py \
		-d $(CPYTHON) \
		$(TESTFLAGS) \
		--exclude "*/bad*" \
		--exclude "*/lib2to3/tests/data/*"

mypy: regen-metaparser
	$(MYPY)  # For list of files, see mypy.ini

format-python:
	black pegen scripts

format: format-python

find_max_nesting:
	$(PYTHON) scripts/find_max_nesting.py

tags: TAGS

TAGS: pegen/*.py test/test_pegen.py
	etags pegen/*.py test/test_pegen.py
