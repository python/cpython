# This file can be invoked from the "python.app" target in the 
# main Makefile. The next two variables are overridden on the 
# commandline in that case.

# assume user was invoking from Mac/OSX directory and building in source tree
PYTHONBUILDDIR = ../..
PYTHONSRCDIR = ../..

INSTALLDIR=/Library/Frameworks/Python.framework/Versions/Current
APPINSTALLDIR=/Applications/Python.app

# Variables for installing the "normal" unix binaries
UNIXBINDIR=/usr/local/bin
INSTALLED_PYTHON=$(INSTALLDIR)/bin/python
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
INCLUDES=-I$(PYTHONBUILDDIR) -I$(PYTHONSRCDIR)/Include \
	-I$(PYTHONSRCDIR)/Mac/Include
DEFINES=-DHAVE_CONFIG_H

CFLAGS=$(OPT) $(DEFINES) $(INCLUDES)
LDFLAGS=-framework System -framework Python -framework Carbon \
	-framework Foundation
CC=cc
LD=cc
REZ=/Developer/Tools/Rez
DEREZ=/Developer/Tools/DeRez

OBJECTS=$(PYTHONBUILDDIR)/Mac/Python/macmain.o \
	$(PYTHONBUILDDIR)/Mac/Python/macgetargv.o

pythonforbundle: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o pythonforbundle

PYTHON=$(PYTHONBUILDDIR)/python.exe
APPTEMPLATE=$(PYTHONSRCDIR)/Mac/OSXResources/app
APPSUBDIRS=MacOS Resources Resources/English.lproj
RESOURCEDIR=$(PYTHONSRCDIR)/Mac/Resources
RESOURCEFILE=python.rsrc
RFCONVERTER=$(PYTHONSRCDIR)/Mac/Lib/applesingle.py
install: pythonforbundle
	@for i in $(APPINSTALLDIR) $(APPINSTALLDIR)/Contents; do \
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
	$(INSTALL_PROGRAM) $(STRIPFLAG) pythonforbundle $(APPINSTALLDIR)/Contents/MacOS/python
	# Create a temporary version of the resources here
	$(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/dialogs.rsrc dialogs.rsrc
	$(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/errors.rsrc errors.rsrc
	$(DEREZ) -useDF -skip ckid dialogs.rsrc > dialogs.r
	$(DEREZ) -useDF -skip ckid errors.rsrc > errors.r
	$(REZ) -useDF -o $(RESOURCEFILE) dialogs.r errors.r 
	$(INSTALL_DATA) $(RESOURCEFILE) $(APPINSTALLDIR)/Contents/Resources/$(RESOURCEFILE)
		
LIBDEST=$(INSTALLDIR)/Mac/Lib
LIBSRC=$(PYTHONSRCDIR)/Mac/Lib
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
TOOLSDEST=$(INSTALLDIR)/Mac/Tools
TOOLSSRC=$(PYTHONSRCDIR)/Mac/Tools
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

	$(INSTALL_DATA) $(PYTHONSRCDIR)/Mac/OSX/Mac.pth $(INSTALLDIR)/lib/python2.3/site-packages/
	
# Put symlinks "python" and "pythonw" in the standard place
installunixprograms: $(INSTALLED_PYTHON) pythonw.sh
	$(INSTALL) -d $(UNIXBINDIR)
	$(INSTALL_SYMLINK) $(INSTALLED_PYTHON) $(UNIXBINDIR)/python
	$(INSTALL) pythonw.sh $(UNIXBINDIR)/pythonw
	
# This is for development purposes: put a symlink to the Mac source subtree in the
# framework
symlinkmacsubtree:
	ln -sf `cd $(PYTHONBUILDDIR)/Mac; pwd` $(INSTALLDIR)/Mac

	@echo '** Copy the contents of sample_sitecustomize.py (or similar code) into'
	@echo '**' $(INSTALLDIR)/lib/python2.3/sitecustomize.py


# Rules to build each file in OBJECTS - is there a better way?
$(PYTHONBUILDDIR)/Mac/Python/macmain.o: $(PYTHONSRCDIR)/Mac/Python/macmain.c
	$(CC) $(CFLAGS) -c $(PYTHONSRCDIR)/Mac/Python/macmain.c -o $@

$(PYTHONBUILDDIR)/Mac/Python/macgetargv.o: $(PYTHONSRCDIR)/Mac/Python/macgetargv.c
	$(CC) $(CFLAGS) -c $(PYTHONSRCDIR)/Mac/Python/macgetargv.c -o $@

