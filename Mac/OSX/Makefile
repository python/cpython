# This file can be invoked from the various frameworkinstall... targets in the 
# main Makefile. The next couple of variables are overridden on the 
# commandline in that case.

VERSION=2.4
builddir = ../..
srcdir = ../..
prefix=/Library/Frameworks/Python.framework/Versions/$(VERSION)
LIBDEST=$(prefix)/lib/python$(VERSION)
BUILDPYTHON=$(builddir)/python.exe
DESTDIR=
# Test whether to use xcodebuild (preferred) or pbxbuild:
ifeq ($(shell ls /usr/bin/xcodebuild),/usr/bin/xcodebuild)
PBXBUILD=xcodebuild
else
PBXBUILD=pbxbuild
endif

# These are normally glimpsed from the previous set
bindir=/usr/local/bin
PYTHONAPPSPATH=/Applications/MacPython-$(VERSION)
PYTHONAPPSDIR=$(PYTHONAPPSPATH)
APPINSTALLDIR=$(prefix)/Resources/Python.app

# Variables for installing the "normal" unix binaries
INSTALLED_PYTHON=$(prefix)/bin/python
INSTALLED_PYTHONW=$(APPINSTALLDIR)/Contents/MacOS/Python

# Items more-or-less copied from the main Makefile
DIRMODE=755
FILEMODE=644
INSTALL=/usr/bin/install -c
INSTALL_SYMLINK=ln -fsn
INSTALL_PROGRAM=${INSTALL}
INSTALL_SCRIPT= ${INSTALL_PROGRAM}
INSTALL_DATA=	${INSTALL} -m ${FILEMODE}
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
	Resources/English.lproj/Documentation/ide
DOCDIR=$(srcdir)/Mac/OSXResources/app/Resources/English.lproj/Documentation
DOCINDEX=$(DOCDIR)/"Documentation idx"
CACHERSRC=$(srcdir)/Mac/scripts/cachersrc.py
compileall=$(srcdir)/Lib/compileall.py
bundlebuilder=$(srcdir)/Lib/plat-mac/bundlebuilder.py

installapps: install_PythonLauncher install_Python install_BuildApplet install_IDE \
	install_IDLE install_PackageManager checkapplepython

install_PythonLauncher:
	cd $(srcdir)/Mac/OSX/PythonLauncher/PythonLauncher.pbproj ; \
	$(PBXBUILD) -target PythonLauncher -buildstyle Deployment install \
		DSTROOT=$(DESTDIR)/ INSTALL_PATH=$(PYTHONAPPSPATH)

install_Python:
	@if test ! -f $(DOCINDEX); then \
		echo WARNING: you should run Apple Help Indexing Tool on $(DOCDIR); \
	fi
	@for i in $(PYTHONAPPSDIR) $(APPINSTALLDIR) $(APPINSTALLDIR)/Contents; do \
		if test ! -d $(DESTDIR)$$i; then \
			echo "Creating directory $(DESTDIR)$$i"; \
			$(INSTALL) -d -m $(DIRMODE) $(DESTDIR)$$i; \
		fi;\
	done
	@for i in $(APPSUBDIRS); do \
		if test ! -d $(DESTDIR)$(APPINSTALLDIR)/Contents/$$i; then \
			echo "Creating directory $(DESTDIR)$(APPINSTALLDIR)/Contents/$$i"; \
			$(INSTALL) -d -m $(DIRMODE) $(DESTDIR)$(APPINSTALLDIR)/Contents/$$i; \
		else	true; \
		fi; \
	done
	@for d in . $(APPSUBDIRS); \
	do \
		a=$(APPTEMPLATE)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(DESTDIR)$(APPINSTALLDIR)/Contents/$$d; \
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
	$(INSTALL_PROGRAM) $(STRIPFLAG) $(BUILDPYTHON) $(DESTDIR)$(APPINSTALLDIR)/Contents/MacOS/Python

install_IDE:
	@if ! $(BUILDPYTHON) -c "import waste"; then  \
		echo PythonIDE needs the \"waste\" extension module; \
		echo See Mac/OSX/README for details; \
	else \
		echo $(BUILDPYTHON) $(srcdir)/Mac/scripts/BuildApplet.py \
		    --destroot "$(DESTDIR)" \
			--python $(INSTALLED_PYTHONW) \
			--output $(DESTDIR)$(PYTHONAPPSDIR)/PythonIDE.app --noargv \
			$(srcdir)/Mac/Tools/IDE/PythonIDE.py ; \
		$(BUILDPYTHON) $(srcdir)/Mac/scripts/BuildApplet.py \
		    --destroot "$(DESTDIR)" \
			--python $(INSTALLED_PYTHONW) \
			--output $(DESTDIR)$(PYTHONAPPSDIR)/PythonIDE.app --noargv \
			$(srcdir)/Mac/Tools/IDE/PythonIDE.py; \
	fi

install_PackageManager:
	@if ! $(BUILDPYTHON) -c "import waste"; then  \
		echo PackageManager needs the \"waste\" extension module; \
		echo See Mac/OSX/README for details; \
	else \
		echo $(BUILDPYTHON) $(bundlebuilder) \
			--builddir $(DESTDIR)$(PYTHONAPPSDIR)/ \
		    --destroot "$(DESTDIR)" \
			--python $(INSTALLED_PYTHONW) \
			--resource $(srcdir)/Mac/Tools/IDE/PythonIDE.rsrc \
			--mainprogram $(srcdir)/Mac/Tools/IDE/PackageManager.py \
			--iconfile $(srcdir)/Mac/Tools/IDE/PackageManager.icns \
			--creator Pimp build; \
		$(BUILDPYTHON) $(bundlebuilder) \
			--builddir $(DESTDIR)$(PYTHONAPPSDIR)/ \
		    --destroot "$(DESTDIR)" \
			--python $(INSTALLED_PYTHONW) \
			--resource $(srcdir)/Mac/Tools/IDE/PythonIDE.rsrc \
			--mainprogram $(srcdir)/Mac/Tools/IDE/PackageManager.py \
			--iconfile $(srcdir)/Mac/Tools/IDE/PackageManager.icns \
			--creator Pimp build; \
	fi

install_IDLE:
	@if ! $(BUILDPYTHON) -c "import _tkinter"; then \
		echo IDLE needs the \"Tkinter\" extension module; \
		echo See Mac/OSX/README for details; \
	else \
		echo $(BUILDPYTHON) $(srcdir)/Mac/scripts/BuildApplet.py \
			--python $(INSTALLED_PYTHONW) \
		    --destroot "$(DESTDIR)" \
			--output $(DESTDIR)$(PYTHONAPPSDIR)/IDLE.app \
			--extra $(srcdir)/Lib/idlelib \
			$(srcdir)/Tools/scripts/idle ; \
		$(BUILDPYTHON) $(srcdir)/Mac/scripts/BuildApplet.py \
			--python $(INSTALLED_PYTHONW) \
		    --destroot "$(DESTDIR)" \
			--output $(DESTDIR)$(PYTHONAPPSDIR)/IDLE.app \
			--extra $(srcdir)/Lib/idlelib:Contents/Resources/idlelib \
			$(srcdir)/Tools/scripts/idle ; \
	fi


install_BuildApplet:
	$(BUILDPYTHON) $(srcdir)/Mac/scripts/BuildApplet.py \
		--destroot "$(DESTDIR)" \
		--python $(INSTALLED_PYTHONW) \
		--output $(DESTDIR)$(PYTHONAPPSDIR)/BuildApplet.app \
		$(srcdir)/Mac/scripts/BuildApplet.py

MACLIBDEST=$(LIBDEST)/plat-mac
MACTOOLSDEST=$(prefix)/Mac/Tools
MACTOOLSSRC=$(srcdir)/Mac/Tools
MACTOOLSSUBDIRS=IDE
installmacsubtree:
	@for i in $(MACTOOLSDEST); \
	do \
		if test ! -d $(DESTDIR)$$i; then \
			echo "Creating directory $(DESTDIR)$$i"; \
			$(INSTALL) -d -m $(DIRMODE) $(DESTDIR)$$i; \
		else	true; \
		fi; \
	done
	@for d in $(MACTOOLSSUBDIRS); \
	do \
		a=$(MACTOOLSSRC)/$$d; \
		if test ! -d $$a; then continue; else true; fi; \
		b=$(DESTDIR)$(MACTOOLSDEST)/$$d; \
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
		b=$(DESTDIR)$(MACTOOLSDEST)/$$d; \
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


	$(BUILDPYTHON) $(CACHERSRC) -v $(DESTDIR)$(MACLIBDEST) $(DESTDIR)$(MACTOOLSDEST)
	$(BUILDPYTHON) -Wi -tt $(compileall) -d $(MACTOOLSDEST) -x badsyntax $(DESTDIR)$(MACTOOLSDEST)
	$(BUILDPYTHON) -O -Wi -tt $(compileall) -d $(MACTOOLSDEST) -x badsyntax $(DESTDIR)$(MACTOOLSDEST)

#
# We use the full name here in stead of $(INSTALLED_PYTHONW), because
# the latter may be overridden by Makefile.jaguar when building for a pre-installed
# /usr/bin/python
$(APPINSTALLDIR)/Contents/MacOS/Python: install_Python

# $(INSTALLED_PYTHON) has to be done by the main Makefile, we cannot do that here.
# At least this rule will give an error if it doesn't exist.

installunixtools:
	$(INSTALL) -d $(DESTDIR)$(bindir)
	$(INSTALL_SYMLINK) $(INSTALLED_PYTHON) $(DESTDIR)$(bindir)/python$(VERSION)
	$(INSTALL_SYMLINK) python$(VERSION) $(DESTDIR)$(bindir)/python
	echo "#!/bin/sh" > pythonw.sh
	echo "exec \"$(INSTALLED_PYTHONW)\" \"\$$@\"" >> pythonw.sh
	$(INSTALL) pythonw.sh $(DESTDIR)$(bindir)/pythonw$(VERSION)
	$(INSTALL_SYMLINK) pythonw$(VERSION) $(DESTDIR)$(bindir)/pythonw

installextras:
	$(INSTALL) -d $(DESTDIR)$(PYTHONAPPSDIR)/Extras
	$(INSTALL) $(srcdir)/Mac/OSX/Extras.ReadMe.txt $(DESTDIR)$(PYTHONAPPSDIR)/Extras/ReadMe
	$(BUILDPYTHON) $(srcdir)/Mac/OSX/Extras.install.py $(srcdir)/Demo \
		$(DESTDIR)$(PYTHONAPPSDIR)/Extras/Demo
	$(BUILDPYTHON) $(srcdir)/Mac/OSX/Extras.install.py $(srcdir)/Tools \
		$(DESTDIR)$(PYTHONAPPSDIR)/Extras/Tools

checkapplepython:
	@if ! $(BUILDPYTHON) $(srcdir)/Mac/OSX/fixapplepython23.py -n; then \
		echo "* WARNING: Apple-installed Python 2.3 will have trouble building extensions from now on."; \
		echo "* WARNING: Run $(srcdir)/Mac/OSX/fixapplepython23.py with \"sudo\" to fix this."; \
	fi
    
