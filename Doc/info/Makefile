# Generate the Python "info" documentation.

# These is only used for .info generation:
EMACS=		emacs
MAKEINFO=	makeinfo
PYTHON=		python
PARTPARSE=	$(PYTHON) ../tools/partparse.py


all:	python-lib.info


.PRECIOUS:	python-lib.texi

# The sed script in this target fixes a really nasty little condition in
# libcgi.tex where \e has to be used in a group to get the right behavior,
# and makeinfo can't handle a group without a leading @command.  But at
# least the info file gets generated.

lib1.texi: $(LIBFILES) texipre.dat texipost.dat ../tools/partparse.py
	$(PARTPARSE) -o $@ `../tools/whichlibs`
	sed 's/"{\\}n{\\}n/"\\n\\n/' $@ >temp.texi
	mv temp.texi $@

python-lib.texi: lib1.texi ../tools/fix.el
	cp lib1.texi temp.texi
	$(EMACS) -batch -l ../tools/fix.el -f save-buffer -kill
	mv temp.texi $@

python-lib.info: python-lib.texi
	$(MAKEINFO) --footnote-style end --fill-column 72 \
		    --paragraph-indent 0 $<
