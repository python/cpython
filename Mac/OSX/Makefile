# This file can be invoked from the various frameworkinstall... targets in the 
# main Makefile. The next couple of variables are overridden on the 
# commandline in that case.

VERSION=2.3
builddir = ../..
srcdir = ../..
dstroot=/.
prefix=$(dstroot)/Library/Frameworks/Python.framework/Versions/$(VERSION)
LIBDEST=$(prefix)/lib/python$(VERSION)
BUILDPYTHON=$(builddir)/python.exe

# These are normally glimpsed from the previous set
bindir=$(dstroot)/usr/local/bin
PYTHONAPPSPATH=/Applications/MacPython-$(VERSION)
PYTHONAPPSDIR=$(dstroot)$(PYTHONAPPSPATH)
APPINSTALLDIR=$(prefix)/Resources/Python.app

# Variables for installing the "normal" unix binaries
INSTALLED_PYTHON=$(prefix)/bin/python
INSTALLED_PYTHONW=$(APPINSTALLDIR)/Contents/MacOS/python

# Items more-or-less copied from the main Makefile
DIRMODE=755
INSTALL=/usr/bin/install -c
INSTALL_SYMLINK=ln -fs
INSTALL_PROGRAM=${INSTALL}
INSTALL_SCRIPT= ${INSTALL_PROGRAM}
INSTALL_DATA=	${INSTALL} -m 644
LN=ln
STRIPFLAG=-s
##OPT=-g -O3 -Wall -Wstrict-prototypes -Wno-long-double -no-cpp-precomp \
##	-fno-common -dynamic
##INCLUDES=-I$(builddir) -I$(srcdir)/Include -I$(srcdir)/Mac/Include
##DEFINES=
##
##CFLAGS=$(OPT) $(DEFINES) $(INCLUDES)
##LDFLAGS=-F$(builddir) -framework System -framework Python -framework Carbon \
##	-framework Foundation
##CC=cc
##LD=cc
CPMAC=/Developer/Tools/CpMac

APPTEMPLATE=$(srcdir)/Mac/OSXResources/app
APPSUBDIRS=MacOS Resources Resources/English.lproj \
	Resources/English.lproj/Documentation \
	Resources/English.lproj/Documentation/doc \
	Resources/English.lproj/Documentation/macpython_ide_tutorial
DOCDIR=$(srcdir)/Mac/OSXResources/app/Resources/English.lproj/Documentation
DOCINDEX=$(DOCDIR)/"Documentation idx"
CACHERSRC=$(srcdir)/Mac/scripts/cachersrc.py
compileall=$(srcdir)/Lib/compileall.py
bundlebuilder=$(srcdir)/Lib/plat-mac/bundlebuilder.py

installapps: install_PythonLauncher install_Python install_BuildApplet install_IDE \
	install_IDLE install_PackageManager

install_PythonLauncher:
	cd $(srcdir)/Mac/OSX/PythonLauncher/PythonLauncher.pbproj ; \
	pbxbuild -target PythonLauncher -buildstyle Deployment \
		DSTROOT=$(dstroot) INSTALL_PATH=$(PYTHONAPPSPATH) install
	
install_Python:
	@if test ! -f $(DOCINDEX); then \
		echo WARNING: you should run Apple Help Indexing Tool on $(DOCDIR); \
	fi
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
			*idx) \
				echo $(CPMAC) "$$i" $$b; \
				$(CPMAC) "$$i" $$b; \
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
	$(INSTALL_PROGRAM) $(STRIPFLAG) $(BUILDPYTHON) $(APPINSTALLDIR)/Contents/MacOS/python	
	
install_IDE: $(INSTALLED_PYTHONW)
	@if ! $(INSTALLED_PYTHONW) -c "import waste"; then  \
		echo PythonIDE needs the \"waste\" extension module; \
		echo See Mac/OSX/README for details; \
	else \
		echo $(INSTALLED_PYTHONW) $(srcdir)/Mac/scripts/BuildApplet.py \
			--output $(PYTHONAPPSDIR)/PythonIDE.app --noargv \
			$(srcdir)/Mac/Tools/IDE/PythonIDE.py ; \
		$(INSTALLED_PYTHONW) $(srcdir)/Mac/scripts/BuildApplet.py \
			--output $(PYTHONAPPSDIR)/PythonIDE.app --noargv \
			$(srcdir)/Mac/Tools/IDE/PythonIDE.py; \
	fi

install_PackageManager: $(INSTALLED_PYTHONW)
	@if ! $(INSTALLED_PYTHONW) -c "import waste"; then  \
		echo PackageManager needs the \"waste\" extension module; \
		echo See Mac/OSX/README for details; \
	else \
		echo $(INSTALLED_PYTHONW) $(bundlebuilder) \
			--builddir $(PYTHONAPPSDIR)/ \
			--resource $(srcdir)/Mac/Tools/IDE/PythonIDE.rsrc \
			--mainprogram $(srcdir)/Mac/Tools/IDE/PackageManager.py \
			--iconfile $(srcdir)/Mac/Tools/IDE/PackageManager.icns \
			--creator Pimp build; \
		$(INSTALLED_PYTHONW) $(bundlebuilder) \
			--builddir $(PYTHONAPPSDIR)/ \
			--resource $(srcdir)/Mac/Tools/IDE/PythonIDE.rsrc \
			--mainprogram $(srcdir)/Mac/Tools/IDE/PackageManager.py \
			--iconfile $(srcdir)/Mac/Tools/IDE/PackageManager.icns \
			--creator Pimp build; \
	fi

install_IDLE: $(INSTALLED_PYTHONW)
	@if ! $(INSTALLED_PYTHONW) -c "import _tkinter"; then \
		echo IDLE needs the \"Tkinter\" extension module; \
		echo See Mac/OSX/README for details; \
	else \
		echo $(INSTALLED_PYTHONW) $(srcdir)/Mac/scripts/BuildApplet.py \
			--output $(PYTHONAPPSDIR)/IDLE.app \
			--extra $(srcdir)/Tools/idle \
			$(srcdir)/Tools/idle/idle ; \
		$(INSTALLED_PYTHONW) $(srcdir)/Mac/scripts/BuildApplet.py \
			--output $(PYTHONAPPSDIR)/IDLE.app \
			--extra $(srcdir)/Tools/idle:Contents/Resources/idlelib \
			$(srcdir)/Tools/idle/idle ; \
	fi

		
install_BuildApplet: $(INSTALLED_PYTHONW)
	$(INSTALLED_PYTHONW) $(srcdir)/Mac/scripts/BuildApplet.py \
		--output $(PYTHONAPPSDIR)/BuildApplet.app \
		$(srcdir)/Mac/scripts/BuildApplet.py
		
MACLIBDEST=$(LIBDEST)/plat-mac
MACTOOLSDEST=$(prefix)/Mac/Tools
MACTOOLSSRC=$(srcdir)/Mac/Tools
MACTOOLSSUBDIRS=IDE
installmacsubtree:
	@for i in $(MACTOOLSDEST); \
	do \
		if test ! -d $$i; then \
			echo "Creating directory $$i"; \
			$(INSTALL) -d -m $(DIRMODE) $$i; \
		else	true; \
		fi; \
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
	

	$(BUILDPYTHON) $(CACHERSRC) -v $(MACLIBDEST) $(MACTOOLSDEST)
	$(BUILDPYTHON) -Wi -tt $(compileall) -x badsyntax $(MACTOOLSDEST)
	$(BUILDPYTHON) -O -Wi -tt $(compileall) -x badsyntax $(MACTOOLSDEST)

#
# We use the full name here in stead of $(INSTALLED_PYTHONW), because
# the latter may be overridden by Makefile.jaguar when building for a pre-installed
# /usr/bin/python
$(APPINSTALLDIR)/Contents/MacOS/python: install_Python

# $(INSTALLED_PYTHON) has to be done by the main Makefile, we cannot do that here.
# At least this rule will give an error if it doesn't exist.

installunixtools: $(INSTALLED_PYTHON) $(INSTALLED_PYTHONW)
	$(INSTALL) -d $(bindir)
	$(INSTALL_SYMLINK) $(INSTALLED_PYTHON) $(bindir)/python$(VERSION)
	$(INSTALL_SYMLINK) python$(VERSION) $(bindir)/python
	echo "#!/bin/sh" > pythonw.sh
	echo "exec \"$(INSTALLED_PYTHONW)\" \"\$$@\"" >> pythonw.sh
	$(INSTALL) pythonw.sh $(bindir)/pythonw$(VERSION)
	$(INSTALL_SYMLINK) pythonw$(VERSION) $(bindir)/pythonw
