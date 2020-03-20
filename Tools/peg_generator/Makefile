PYTHON ?= python3.8
CPYTHON ?= ../..
MYPY ?= mypy

GRAMMAR = data/python.gram
TESTFILE = data/cprog.py
TIMEFILE = data/xxl.py
TESTDIR = .
TESTFLAGS = --short

build: peg_parser/parse.c

peg_parser/parse.c: $(GRAMMAR) pegen/*.py peg_parser/pegen.c peg_parser/parse_string.c peg_parser/*.h pegen/grammar_parser.py
	$(PYTHON) -m pegen -q -c $(GRAMMAR) -o peg_parser/parse.c --compile-extension

clean:
	-rm -f peg_parser/*.o peg_parser/*.so peg_parser/parse.c

dump: peg_parser/parse.c
	cat -n $(TESTFILE)
	$(PYTHON) -c "from peg_parser import parse; import ast; t = parse.parse_file('$(TESTFILE)', mode=1); print(ast.dump(t))"

regen-metaparser: pegen/metagrammar.gram pegen/*.py
	$(PYTHON) -m pegen -q -c pegen/metagrammar.gram -o pegen/grammar_parser.py

# Note: These targets really depend on the generated shared object in peg_parser/parse.*.so but
# this has different names in different systems so we are abusing the implicit dependency on
# parse.c by the use of --compile-extension.

.PHONY: test

test: run

run: peg_parser/parse.c
	$(PYTHON) -c "from peg_parser import parse; t = parse.parse_file('$(TESTFILE)'); exec(t)"

compile: peg_parser/parse.c
	$(PYTHON) -c "from peg_parser import parse; t = parse.parse_file('$(TESTFILE)', mode=2)"

parse: peg_parser/parse.c
	$(PYTHON) -c "from peg_parser import parse; t = parse.parse_file('$(TESTFILE)', mode=1)"

check: peg_parser/parse.c
	$(PYTHON) -c "from peg_parser import parse; t = parse.parse_file('$(TESTFILE)', mode=0)"

time: time_compile

time_compile: peg_parser/parse.c
	/usr/bin/time -l $(PYTHON) -c "from peg_parser import parse; parse.parse_file('$(TIMEFILE)', mode=2)"

time_parse: peg_parser/parse.c
	/usr/bin/time -l $(PYTHON) -c "from peg_parser import parse; parse.parse_file('$(TIMEFILE)', mode=1)"

time_check: peg_parser/parse.c
	/usr/bin/time -l $(PYTHON) -c "from peg_parser import parse; parse.parse_file('$(TIMEFILE)', mode=0)"

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
	black pegen test scripts

bench: cpython
	$(MAKE) -s test_global 2>/dev/null
	$(MAKE) -s test_global 2>/dev/null
	$(MAKE) -s test_global 2>/dev/null

# To install clang-format:
#    on mac: "brew install clang-format"
#    on ubuntu: "apt-get install clang-format"
#    on arch: "pacman -S clang"
format-c:
	clang-format peg_parser/pegen.c -i

# To install clang-tidy:
#    on mac:
#       "brew install llvm"
#       Then, create symlinks to the binaries. For example:
#       ln -s "$(brew --prefix llvm)/bin/clang-format" "/usr/local/bin/clang-format"
#       ln -s "$(brew --prefix llvm)/bin/clang-tidy" "/usr/local/bin/clang-tidy"
#    on ubuntu: "apt-get install clang-tidy"
#    on arch: "pacman -S clang"
clang-tidy:
	$(eval COMPILE_OPTIONS = $(shell python-config --cflags))
	clang-tidy peg_parser/pegen.c -fix-errors -fix -checks="readability-braces-around-statements" -- $(COMPILE_OPTIONS) 1>/dev/null

format: format-python format-c

find_max_nesting:
	$(PYTHON) scripts/find_max_nesting.py

tags: TAGS

TAGS: pegen/*.py test/test_pegen.py
	etags pegen/*.py test/test_pegen.py
