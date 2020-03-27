UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	PYTHON ?= ../../python
endif
ifeq ($(UNAME_S),Darwin)
	PYTHON ?= ../../python.exe
endif

CPYTHON ?= ../..
MYPY ?= mypy

GRAMMAR = data/python.gram
TESTFILE = data/cprog.py
TIMEFILE = data/xxl.py
TESTDIR = .
TESTFLAGS = --short

build: peg_extension/parse.c

peg_extension/parse.c: $(GRAMMAR) pegen/*.py peg_extension/peg_extension.c ../../Parser/pegen/pegen.c ../../Parser/pegen/parse_string.c ../../Parser/pegen/*.h pegen/grammar_parser.py
	$(PYTHON) -m pegen -q -c $(GRAMMAR) -o peg_extension/parse.c --compile-extension

clean:
	-rm -f peg_extension/*.o peg_extension/*.so peg_extension/parse.c

dump: peg_extension/parse.c
	cat -n $(TESTFILE)
	$(PYTHON) -c "from peg_extension import parse; import ast; t = parse.parse_file('$(TESTFILE)', mode=1); print(ast.dump(t))"

regen-metaparser: pegen/metagrammar.gram pegen/*.py
	$(PYTHON) -m pegen -q -c pegen/metagrammar.gram -o pegen/grammar_parser.py

# Note: These targets really depend on the generated shared object in peg_extension/parse.*.so but
# this has different names in different systems so we are abusing the implicit dependency on
# parse.c by the use of --compile-extension.

.PHONY: test

test: run

run: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TESTFILE)'); exec(t)"

compile: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TESTFILE)', mode=2)"

parse: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TESTFILE)', mode=1)"

check: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TESTFILE)', mode=0)"

stats: peg_extension/parse.c
	$(PYTHON) -c "from peg_extension import parse; t = parse.parse_file('$(TIMEFILE)', mode=0); parse.dump_memo_stats()" >@data
	$(PYTHON) scripts/joinstats.py @data

time: time_compile

time_compile: peg_extension/parse.c
	/usr/bin/time -l $(PYTHON) -c "from peg_extension import parse; parse.parse_file('$(TIMEFILE)', mode=2)"

time_parse: peg_extension/parse.c
	/usr/bin/time -l $(PYTHON) -c "from peg_extension import parse; parse.parse_file('$(TIMEFILE)', mode=1)"

time_check: peg_extension/parse.c
	/usr/bin/time -l $(PYTHON) -c "from peg_extension import parse; parse.parse_file('$(TIMEFILE)', mode=0)"

time_stdlib: time_stdlib_compile

time_stdlib_compile:
	/usr/bin/time -l $(PYTHON) -c "import ast; compile(open('$(TIMEFILE)').read(), '$(TIMEFILE)', 'exec')"

time_stdlib_parse:
	/usr/bin/time -l $(PYTHON) -c "import ast; ast.parse(open('$(TIMEFILE)').read())"

test_local:
	$(PYTHON) scripts/test_parse_directory.py \
		-g data/python.gram \
		-d $(TESTDIR) \
		$(TESTFLAGS) \
		--exclude "*/failset/*" \
		--exclude "*/failset/**" \
		--exclude "*/failset/**/*"

test_global: $(CPYTHON)
	$(PYTHON) scripts/test_parse_directory.py \
		-g data/python.gram \
		-d $(CPYTHON) \
		$(TESTFLAGS) \
		--exclude "*/test2to3/*" \
		--exclude "*/test2to3/**/*" \
		--exclude "*/bad*" \
		--exclude "*/lib2to3/tests/data/*"

mypy: regen-metaparser
	$(MYPY)  # For list of files, see mypy.ini

format-python:
	black pegen scripts

bench: cpython
	$(MAKE) -s test_global 2>/dev/null
	$(MAKE) -s test_global 2>/dev/null
	$(MAKE) -s test_global 2>/dev/null

format: format-python

find_max_nesting:
	$(PYTHON) scripts/find_max_nesting.py

tags: TAGS

TAGS: pegen/*.py test/test_pegen.py
	etags pegen/*.py test/test_pegen.py
