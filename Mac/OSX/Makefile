# This file can be invoked from the "python.app" target in the 
# main Makefile. The next two variables are overridden on the 
# commandline in that case.

# assume user was invoking from Mac/OSX directory and building in source tree
builddir = ../..
srcdir = ../..
VERSION=2.3

prefix=/Library/Frameworks/Python.framework/Versions/$(VERSION)
PYTHONAPPSDIR=/Applications/Python
APPINSTALLDIR=$(prefix)/Resources/Python.app

# Variables for installing the "normal" unix binaries
UNIXBINDIR=/usr/local/bin
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
DEFINES=-DHAVE_CONFIG_H

CFLAGS=$(OPT) $(DEFINES) $(INCLUDES)
LDFLAGS=-F$(builddir) -framework System -framework Python -framework Carbon \
	-framework Foundation
CC=cc
LD=cc
REZ=/Developer/Tools/Rez
DEREZ=/Developer/Tools/DeRez

PYTHON=$(builddir)/python.exe
APPTEMPLATE=$(srcdir)/Mac/OSXResources/app
APPSUBDIRS=MacOS Resources Resources/English.lproj
RESOURCEDIR=$(srcdir)/Mac/Resources
RESOURCEFILE=python.rsrc
RFCONVERTER=$(srcdir)/Mac/Lib/applesingle.py

install_all: install_PythonLauncher install_Python install_IDE install_BuildApplet

install_PythonLauncher:
	cd $(srcdir)/Mac/OSX/PythonLauncher/PythonLauncher.pbproj ; \
	pbxbuild -target PythonLauncher -buildstyle Deployment DSTROOT=/ install
	
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
	$(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/dialogs.rsrc dialogs.rsrc
	$(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/errors.rsrc errors.rsrc
	$(DEREZ) -useDF -skip ckid dialogs.rsrc > dialogs.r
	$(DEREZ) -useDF -skip ckid errors.rsrc > errors.r
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
		
LIBDEST=$(prefix)/Mac/Lib
LIBSRC=$(srcdir)/Mac/Lib
LIBSUBDIRS= \
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
TOOLSDEST=$(prefix)/Mac/Tools
TOOLSSRC=$(srcdir)/Mac/Tools
TOOLSSUBDIRS=IDE
installmacsubtree:
	@for i in $(LIBDEST) $(TOOLSDEST); \
	do \
		if test ! -d $$i; then \
			echo "Creating directory $$i"; \
			$(INSTALL) -d -m $(DIRMODE) $$i; \
		else	true; \
		fi; \
	done
	@for d in $(LIBSUBDIRS); \
	do \
		a=$(LIBSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(LIBDEST)/$$d; \
		if test ! -d $$b; then \
			echo "Creating directory $$b"; \
			$(INSTALL) -d -m $(DIRMODE) $$b; \
		else	true; \
		fi; \
	done
	@for i in $(LIBSRC)/*.py $(LIBSRC)/*.rsrc; \
	do \
		if test -x $$i; then \
			$(INSTALL_SCRIPT) $$i $(LIBDEST); \
			echo $(INSTALL_SCRIPT) $$i $(LIBDEST); \
		else \
			$(INSTALL_DATA) $$i $(LIBDEST); \
			echo $(INSTALL_DATA) $$i $(LIBDEST); \
		fi; \
	done
	@for d in $(LIBSUBDIRS); \
	do \
		a=$(LIBSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(LIBDEST)/$$d; \
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
	@for d in $(TOOLSSUBDIRS); \
	do \
		a=$(TOOLSSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(TOOLSDEST)/$$d; \
		if test ! -d $$b; then \
			echo "Creating directory $$b"; \
			$(INSTALL) -d -m $(DIRMODE) $$b; \
		else	true; \
		fi; \
	done
	@for d in $(TOOLSSUBDIRS); \
	do \
		a=$(TOOLSSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(TOOLSDEST)/$$d; \
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

	$(INSTALL_DATA) $(srcdir)/Mac/OSX/Mac.pth $(prefix)/lib/python$(VERSION)/site-packages/
	
# Put symlinks "python" and "pythonw" in the standard place
$(INSTALLED_PYTHONW): install_Python
# $(INSTALLED_PYTHON) has to be done by the main Makefile, we cannot do that here.

installunixprograms: $(INSTALLED_PYTHON) $(INSTALLED_PYTHONW) pythonw.sh
	$(INSTALL) -d $(UNIXBINDIR)
	$(INSTALL_SYMLINK) $(INSTALLED_PYTHON) $(UNIXBINDIR)/python
	$(INSTALL) pythonw.sh $(UNIXBINDIR)/pythonw
	
# This is for development purposes: create a Mac.pth that refers to the source
# directories
dontinstallmacsubtree:
	l=`cd $(srcdir)/Mac/Lib; pwd`; \
	echo $$l > $(prefix)/lib/python$(VERSION)/site-packages/Mac.pth ; \
	echo $$l/lib-scriptpackages >> $(prefix)/lib/python$(VERSION)/site-packages/Mac.pth
