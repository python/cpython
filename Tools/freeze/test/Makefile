# Makefile to test freeze
# set PYTHON to path of Python interpreter to test
PYTHON=python
# set OUTDIR to the temp directory for freeze
OUTDIR=outdir

test:
	$(PYTHON) ../freeze.py -o $(OUTDIR) ok.py
	make -C $(OUTDIR)
	$(OUTDIR)/ok
	
