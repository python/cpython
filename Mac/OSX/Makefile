# This file can be invoked from the various frameworkinstall... targets in the 
# main Makefile. The next couple of variables are overridden on the 
# commandline in that case.

VERSION=2.3
builddir = ../..
srcdir = ../..
LIBDEST=$(prefix)/lib/python$(VERSION)
prefix=/Library/Frameworks/Python.framework/Versions/$(VERSION)
dstroot=/.

# These are normally glimpsed from the previous set
bindir=$(dstroot)/usr/local/bin
PYTHONAPPSDIR=$(dstroot)/Applications/Python
APPINSTALLDIR=$(prefix)/Resources/Python.app

# Variables for installing the "normal" unix binaries
INSTALLED_PYTHON=$(prefix)/bin/python
INSTALLED_PYTHONW=$(APPINSTALLDIR)/Contents/MacOS/python

# Items more-or-less copied from the main Makefile
DIRMODE=755
INSTALL=/usr/bin/install -c
INSTALL_SYMLINK=/usr/bin/install -l as
INSTALL_PROGRAM=${INSTALL}
INSTALL_SCRIPT= ${INSTALL_PROGRAM}
INSTALL_DATA=	${INSTALL} -m 644
STRIPFLAG=-s
OPT=-g -O3 -Wall -Wstrict-prototypes -Wno-long-double -no-cpp-precomp \
	-fno-common -dynamic
INCLUDES=-I$(builddir) -I$(srcdir)/Include -I$(srcdir)/Mac/Include
DEFINES=

CFLAGS=$(OPT) $(DEFINES) $(INCLUDES)
LDFLAGS=-F$(builddir) -framework System -framework Python -framework Carbon \
	-framework Foundation
CC=cc
LD=cc
REZ=/Developer/Tools/Rez
DEREZ=/Developer/Tools/DeRez
CPMAC=/Developer/Tools/CpMac

PYTHON=$(builddir)/python.exe
APPTEMPLATE=$(srcdir)/Mac/OSXResources/app
APPSUBDIRS=MacOS Resources Resources/English.lproj
RESOURCEDIR=$(srcdir)/Mac/Resources
RESOURCEFILE=python.rsrc
RFCONVERTER=$(srcdir)/Mac/Lib/applesingle.py
CACHERSRC=$(srcdir)/Mac/scripts/cachersrc.py

installapps: install_PythonLauncher install_Python install_BuildApplet install_IDE

install_PythonLauncher:
	cd $(srcdir)/Mac/OSX/PythonLauncher/PythonLauncher.pbproj ; \
	pbxbuild -target PythonLauncher -buildstyle Deployment DSTROOT=$(dstroot) install
	
install_Python: $(PYTHON)
	@for i in $(PYTHONAPPSDIR) $(APPINSTALLDIR) $(APPINSTALLDIR)/Contents; do \
		if test ! -d $$i; then \
			echo "Creating directory $$i"; \
			$(INSTALL) -d -m $(DIRMODE) $$i; \
		fi;\
	done
	@for i in $(APPSUBDIRS); do \
		if test ! -d $(APPINSTALLDIR)/Contents/$$i; then \
			echo "Creating directory $(APPINSTALLDIR)/Contents/$$i"; \
			$(INSTALL) -d -m $(DIRMODE) $(APPINSTALLDIR)/Contents/$$i; \
		else	true; \
		fi; \
	done
	@for d in . $(APPSUBDIRS); \
	do \
		a=$(APPTEMPLATE)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(APPINSTALLDIR)/Contents/$$d; \
		for i in $$a/*; \
		do \
			case $$i in \
			*CVS) ;; \
			*.py[co]) ;; \
			*.orig) ;; \
			*~) ;; \
			*) \
				if test -d $$i; then continue; fi; \
				if test -x $$i; then \
				    echo $(INSTALL_SCRIPT) $$i $$b; \
				    $(INSTALL_SCRIPT) $$i $$b; \
				else \
				    echo $(INSTALL_DATA) $$i $$b; \
				    $(INSTALL_DATA) $$i $$b; \
				fi;; \
			esac; \
		done; \
	done
	$(INSTALL_PROGRAM) $(STRIPFLAG) $(PYTHON) $(APPINSTALLDIR)/Contents/MacOS/python
# Create a temporary version of the resources here
# Note that the resource files can either be real 2-fork resource files
# or AppleSingle encoded files.
	@if test -s $(RESOURCEDIR)/dialogs.rsrc; then \
		echo $(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/dialogs.rsrc dialogs.rsrc ;\
		$(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/dialogs.rsrc dialogs.rsrc ; \
		echo $(DEREZ) -useDF -skip ckid dialogs.rsrc > dialogs.r ; \
		$(DEREZ) -useDF -skip ckid dialogs.rsrc > dialogs.r ; \
	else \
		echo $(DEREZ) -skip ckid $(RESOURCEDIR)/dialogs.rsrc > dialogs.r ; \
		$(DEREZ) -skip ckid $(RESOURCEDIR)/dialogs.rsrc > dialogs.r ; \
	fi
	@if test -s $(RESOURCEDIR)/errors.rsrc; then \
		echo $(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/errors.rsrc errors.rsrc ;\
		$(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/errors.rsrc errors.rsrc ; \
		echo $(DEREZ) -useDF -skip ckid errors.rsrc > errors.r ; \
		$(DEREZ) -useDF -skip ckid errors.rsrc > errors.r ; \
	else \
		echo $(DEREZ) -skip ckid $(RESOURCEDIR)/errors.rsrc > errors.r ; \
		$(DEREZ) -skip ckid $(RESOURCEDIR)/errors.rsrc > errors.r ; \
	fi
	$(REZ) -useDF -o $(RESOURCEFILE) dialogs.r errors.r 
	$(INSTALL_DATA) $(RESOURCEFILE) $(APPINSTALLDIR)/Contents/Resources/$(RESOURCEFILE)
	
install_IDE: $(INSTALLED_PYTHONW)
	@if $(INSTALLED_PYTHONW) -c "import waste"; then ; else \
		echo PythonIDE needs the \"waste\" extension module; \
		echo See Mac/OSX/README for details; \
		exit 1; \
	fi
	$(INSTALLED_PYTHONW) $(srcdir)/Mac/scripts/BuildApplet.py \
		--output $(PYTHONAPPSDIR)/PythonIDE.app --noargv \
		$(srcdir)/Mac/Tools/IDE/PythonIDE.py
		
install_BuildApplet: $(INSTALLED_PYTHONW)
	$(INSTALLED_PYTHONW) $(srcdir)/Mac/scripts/BuildApplet.py \
		--output $(PYTHONAPPSDIR)/BuildApplet.app \
		$(srcdir)/Mac/scripts/BuildApplet.py
		
MACLIBDEST=$(prefix)/Mac/Lib
MACLIBSRC=$(srcdir)/Mac/Lib
MACLIBSUBDIRS= \
	Carbon \
	lib-scriptpackages \
	lib-scriptpackages/_builtinSuites \
	lib-scriptpackages/CodeWarrior \
	lib-scriptpackages/Explorer \
	lib-scriptpackages/Finder \
	lib-scriptpackages/Netscape \
	lib-scriptpackages/StdSuites \
	lib-scriptpackages/Terminal \
	mkcwproject \
	mkcwproject/template \
	mkcwproject/template-carbon \
	mkcwproject/template-ppc
MACTOOLSDEST=$(prefix)/Mac/Tools
MACTOOLSSRC=$(srcdir)/Mac/Tools
MACTOOLSSUBDIRS=IDE
installmacsubtree: $(INSTALLED_PYTHON)
	@for i in $(MACLIBDEST) $(MACTOOLSDEST); \
	do \
		if test ! -d $$i; then \
			echo "Creating directory $$i"; \
			$(INSTALL) -d -m $(DIRMODE) $$i; \
		else	true; \
		fi; \
	done
	@for d in $(MACLIBSUBDIRS); \
	do \
		a=$(MACLIBSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(MACLIBDEST)/$$d; \
		if test ! -d $$b; then \
			echo "Creating directory $$b"; \
			$(INSTALL) -d -m $(DIRMODE) $$b; \
		else	true; \
		fi; \
	done
	@for i in $(MACLIBSRC)/*.py; \
	do \
		if test -x $$i; then \
			$(INSTALL_SCRIPT) $$i $(MACLIBDEST); \
			echo $(INSTALL_SCRIPT) $$i $(MACLIBDEST); \
		else \
			$(INSTALL_DATA) $$i $(MACLIBDEST); \
			echo $(INSTALL_DATA) $$i $(MACLIBDEST); \
		fi; \
	done
##	@for i in $(MACLIBSRC)/*.rsrc; \
##	do \
##		echo $(CPMAC) $$i $$b ; \
##		$(CPMAC) $$i $$b ; \
##	done
	@for d in $(MACLIBSUBDIRS); \
	do \
		a=$(MACLIBSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(MACLIBDEST)/$$d; \
		for i in $$a/*; \
		do \
			case $$i in \
			*CVS) ;; \
			*.py[co]) ;; \
			*.orig) ;; \
			*~) ;; \
			*.rsrc) \
				echo $(CPMAC) $$i $$b ; \
				$(CPMAC) $$i $$b ; \
				;; \
			*) \
				if test -d $$i; then continue; fi; \
				if test -x $$i; then \
				    echo $(INSTALL_SCRIPT) $$i $$b; \
				    $(INSTALL_SCRIPT) $$i $$b; \
				else \
				    echo $(INSTALL_DATA) $$i $$b; \
				    $(INSTALL_DATA) $$i $$b; \
				fi;; \
			esac; \
		done; \
	done
	@for d in $(MACTOOLSSUBDIRS); \
	do \
		a=$(MACTOOLSSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(MACTOOLSDEST)/$$d; \
		if test ! -d $$b; then \
			echo "Creating directory $$b"; \
			$(INSTALL) -d -m $(DIRMODE) $$b; \
		else	true; \
		fi; \
	done
	@for d in $(MACTOOLSSUBDIRS); \
	do \
		a=$(MACTOOLSSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(MACTOOLSDEST)/$$d; \
		for i in $$a/*; \
		do \
			case $$i in \
			*CVS) ;; \
			*.py[co]) ;; \
			*.orig) ;; \
			*~) ;; \
			*.rsrc) \
				echo $(CPMAC) $$i $$b ; \
				$(CPMAC) $$i $$b ; \
				;; \
			*) \
				if test -d $$i; then continue; fi; \
				if test -x $$i; then \
				    echo $(INSTALL_SCRIPT) $$i $$b; \
				    $(INSTALL_SCRIPT) $$i $$b; \
				else \
				    echo $(INSTALL_DATA) $$i $$b; \
				    $(INSTALL_DATA) $$i $$b; \
				fi;; \
			esac; \
		done; \
	done
	
	$(INSTALL_DATA) $(srcdir)/Mac/OSX/Mac.pth $(LIBDEST)/site-packages/

	$(PYTHON) $(CACHERSRC) -v $(MACLIBDEST) $(MACTOOLSDEST)
	$(INSTALLED_PYTHON) $(srcdir)/Lib/compileall.py $(MACLIBDEST) $(MACTOOLSDEST)
	$(INSTALLED_PYTHON) -O $(srcdir)/Lib/compileall.py $(MACLIBDEST) $(MACTOOLSDEST)

	
# Put symlinks "python" and "pythonw" in the standard place
$(INSTALLED_PYTHONW): install_Python

# $(INSTALLED_PYTHON) has to be done by the main Makefile, we cannot do that here.
# At least this rule will give an error if it doesn't exist.

installunixtools: $(INSTALLED_PYTHON) $(INSTALLED_PYTHONW) $(srcdir)/Mac/OSX/pythonw.sh
	$(INSTALL) -d $(bindir)
	$(INSTALL_SYMLINK) $(INSTALLED_PYTHON) $(bindir)/python
	$(INSTALL) $(srcdir)/Mac/OSX/pythonw.sh $(bindir)/pythonw
	
# This is for development purposes: create a Mac.pth that refers to the source
# directories
dontinstallmacsubtree:
	l=`cd $(srcdir)/Mac/Lib; pwd`; \
	echo $$l > $(MACLIBDEST)/site-packages/Mac.pth ; \
	echo $$l/lib-scriptpackages >> $(MACLIBDEST)/site-packages/Mac.pth
