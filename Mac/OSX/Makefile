PYTHONBUILDDIR=../..
INSTALLDIR=/Library/Frameworks/Python.framework/Versions/Current
APPINSTALLDIR=/Applications/Python.app

# Items more-or-less copied from the main Makefile
DIRMODE=755
INSTALL=/usr/bin/install -c
INSTALL_PROGRAM=${INSTALL}
INSTALL_SCRIPT= ${INSTALL_PROGRAM}
INSTALL_DATA=	${INSTALL} -m 644
OPT=-g -O3 -Wall -Wstrict-prototypes -no-cpp-precomp -fno-common -dynamic
INCLUDES=-I$(PYTHONBUILDDIR) -I$(PYTHONBUILDDIR)/Include \
	-I$(PYTHONBUILDDIR)/Mac/Include
DEFINES=-DHAVE_CONFIG_H

CFLAGS=$(OPT) $(DEFINES) $(INCLUDES)
LDFLAGS=-framework System -framework Python -framework Carbon \
	-framework Foundation
CC=cc
LD=cc

OBJECTS=$(PYTHONBUILDDIR)/Mac/Python/macmain.o \
	$(PYTHONBUILDDIR)/Mac/Python/macgetargv.o

pythonforbundle: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o pythonforbundle

PYTHON=$(PYTHONBUILDDIR)/python.exe
APPTEMPLATE=$(PYTHONBUILDDIR)/Mac/OSXResources/app
APPSUBDIRS=MacOS Resources Resources/English.lproj
RESOURCEDIR=$(PYTHONBUILDDIR)/Mac/Resources
RESOURCEFILE=python.rsrc
RFCONVERTER=$(PYTHONBUILDDIR)/Mac/Lib/applesingle.py
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
	$(INSTALL_PROGRAM) pythonforbundle $(APPINSTALLDIR)/Contents/MacOS/python
	# Create a temporary version of the resources here
	$(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/dialogs.rsrc dialogs.rsrc
	$(PYTHON) $(RFCONVERTER) -r $(RESOURCEDIR)/errors.rsrc errors.rsrc
	DeRez -useDF -skip ckid dialogs.rsrc > dialogs.r
	DeRez -useDF -skip ckid errors.rsrc > errors.r
	Rez -useDF -o $(RESOURCEFILE) dialogs.r errors.r 
	$(INSTALL_DATA) $(RESOURCEFILE) $(APPINSTALLDIR)/Contents/Resources/$(RESOURCEFILE)
		
LIBDEST=$(INSTALLDIR)/Mac/Lib
LIBSRC=$(PYTHONBUILDDIR)/Mac/Lib
LIBSUBDIRS=Carbon lib-scriptpackages lib-scriptpackages/CodeWarrior lib-scriptpackages/Explorer \
	lib-scriptpackages/Finder lib-scriptpackages/Netscape lib-scriptpackages/StdSuites \
	mkcwproject mkcwproject/template mkcwproject/template-carbon mkcwproject/template-ppc
TOOLSDEST=$(INSTALLDIR)/Mac/Tools
TOOLSSRC=$(PYTHONBUILDDIR)/Mac/Tools
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

	@echo '** Copy the contents of sample_sitecustomize.py (or similar code) into'
	@echo '**' $(INSTALLDIR)/lib/python2.2/sitecustomize.py
	
