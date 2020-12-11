########################################################################################################
#
# Makefile wrapper to build tk on Mac OS X in a way compatible with the tk/macosx Xcode buildsystem
#	uses the standard unix build system in tk/unix (which can be used directly instead of this
#	if you are not using the tk/macosx projects).
#
# Copyright (c) 2002-2008 Daniel A. Steffen <das@users.sourceforge.net>
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
########################################################################################################

#-------------------------------------------------------------------------------------------------------
# customizable settings

DESTDIR			?=
INSTALL_ROOT		?= ${DESTDIR}

BUILD_DIR		?= ${CURDIR}/../../build
SYMROOT			?= ${BUILD_DIR}/${PROJECT}
OBJROOT			?= ${SYMROOT}

EXTRA_CONFIGURE_ARGS	?=
EXTRA_MAKE_ARGS		?=

INSTALL_PATH		?= /Library/Frameworks
APPLICATION_INSTALL_PATH ?= /Applications/Utilities
PREFIX			?= /usr/local
BINDIR			?= ${PREFIX}/bin
LIBDIR			?= ${INSTALL_PATH}
MANDIR			?= ${PREFIX}/man

# tcl build directory (containing tclConfig.sh and Makefile)
TCL_BUILD_DIR		?= ${BUILD_DIR}/tcl/${BUILD_STYLE}
# location of installed tcl, only used if tcl in TCL_BUILD_DIR can't be found
TCL_FRAMEWORK_DIR	?= /Library/Frameworks
TCLSH_DIR		?= ${PREFIX}/bin

# set to non-empty value to install manpages in addition to html help:
INSTALL_MANPAGES	?=

# set to non-empty value to build TkX11 instead of TkAqua:
TK_X11			?=

#-------------------------------------------------------------------------------------------------------
# meta targets

meta			:= all install embedded install-embedded clean distclean test

styles			:= develop deploy

all			:= ${styles}
all			: ${all}

install			:= ${styles:%=install-%}
install			: ${install}
install-%:		action := install-

embedded		:= ${styles:%=embedded-%}
embedded		: embedded-deploy
install-embedded	:= ${embedded:%=install-%}
install-embedded	: install-embedded-deploy

clean			:= ${styles:%=clean-%}
clean			: ${clean}
clean-%:		action := clean-
distclean		:= ${styles:%=distclean-%}
distclean		: ${distclean}
distclean-%:		action := distclean-

test			:= ${styles:%=test-%}
test			: ${test}
test-%:			action := test-

targets			:= $(foreach v,${meta},${$v})

#-------------------------------------------------------------------------------------------------------
# build styles

BUILD_STYLE		=
CONFIGURE_ARGS		=
OBJ_DIR			= ${OBJROOT}/${BUILD_STYLE}

empty			:=
space			:= ${empty} ${empty}
objdir			= $(subst ${space},\ ,${OBJ_DIR})

develop_make_args	:= BUILD_STYLE=Development CONFIGURE_ARGS=--enable-symbols
deploy_make_args	:= BUILD_STYLE=Deployment INSTALL_TARGET=install-strip
embedded_make_args	:= EMBEDDED_BUILD=1
install_make_args	:= INSTALL_BUILD=1

${targets}:
	${MAKE} ${action}${PROJECT} \
	$(foreach s,${styles} embedded install,$(if $(findstring $s,$@),${${s}_make_args}))

#-------------------------------------------------------------------------------------------------------
# project specific settings

PROJECT			:= tk
PRODUCT_NAME		:= Tk

UNIX_DIR		:= ${CURDIR}/../unix
VERSION			:= $(shell awk -F= '/^TK_VERSION/ {print $$2; nextfile}' ${UNIX_DIR}/configure.in)
TCL_VERSION		:= ${VERSION}
wish			:= wish
WISH			= wish${VERSION}

BUILD_TARGET		:= all tktest
INSTALL_TARGET		:= install

ifneq ($(wildcard $(subst ${space},\ ,${TCL_BUILD_DIR})/tclConfig.sh),)
TCL_DIR			:= ${TCL_BUILD_DIR}
TCL_FRAMEWORK_DIR	:= ${TCL_BUILD_DIR}/..
MAKE_VARS		:=
else
TCL_DIR			:= ${TCL_FRAMEWORK_DIR}/Tcl.framework
TCL_EXE			:= ${TCLSH_DIR}/tclsh${TCL_VERSION}
MAKE_VARS		:= TCL_EXE
export DYLD_FRAMEWORK_PATH := ${TCL_FRAMEWORK_DIR}
endif

export CPPROG		:= cp -p

ifeq (${TK_X11},)
override CONFIGURE_ARGS := ${CONFIGURE_ARGS} --enable-aqua
else
override CONFIGURE_ARGS := ${CONFIGURE_ARGS} --enable-xft
VERSION			:= ${VERSION}-X11
wish			:= ${wish}-X11
override EMBEDDED_BUILD :=
endif

INSTALL_TARGETS		= install-binaries install-libraries
ifeq (${EMBEDDED_BUILD},)
INSTALL_TARGETS		+= install-private-headers install-headers install-demos
endif
ifeq (${INSTALL_BUILD}_${EMBEDDED_BUILD}_${BUILD_STYLE},1__Deployment)
INSTALL_TARGETS		+= html-tk
ifneq (${INSTALL_MANPAGES},)
INSTALL_TARGETS		+= install-doc
endif
endif

MAKE_VARS		+= INSTALL_ROOT INSTALL_TARGETS VERSION
MAKE_ARGS_V		= $(foreach v,${MAKE_VARS},$v='${$v}')

build-${PROJECT}:	target = ${BUILD_TARGET}
install-${PROJECT}:	target = ${INSTALL_TARGET}
clean-${PROJECT} distclean-${PROJECT} test-${PROJECT}: \
			target = $*

DO_MAKE			= +${MAKE} -C "${OBJ_DIR}" ${target} ${MAKE_ARGS_V} ${MAKE_ARGS} ${EXTRA_MAKE_ARGS}

#-------------------------------------------------------------------------------------------------------
# locations for custom tk install actions

ifeq (${INSTALL_BUILD},1)
TOP_DIR			:= ${INSTALL_ROOT}/
APP_DIR			:= ${APPLICATION_INSTALL_PATH}
FMWK_DIR		:= ${LIBDIR}
else
TOP_DIR			:= ${SYMROOT}
APP_DIR			:= .
FMWK_DIR		:= .
endif

TCL_FMWK_DIR		:= ${FMWK_DIR}/Tcl.framework/Versions/${TCL_VERSION}
TK_FMWK_DIR		:= ${FMWK_DIR}/${PRODUCT_NAME}.framework/Versions/${VERSION}

#-------------------------------------------------------------------------------------------------------
# build rules

${PROJECT}:
	${MAKE} install-${PROJECT} INSTALL_ROOT="${OBJ_DIR}/"

${objdir}/Makefile: ${UNIX_DIR}/Makefile.in ${UNIX_DIR}/configure \
		     ${UNIX_DIR}/tkConfig.sh.in Tk-Info.plist.in Wish-Info.plist.in
	mkdir -p "${OBJ_DIR}" && cd "${OBJ_DIR}" && \
	if [ ${UNIX_DIR}/configure -nt config.status ]; then ${UNIX_DIR}/configure -C \
	--prefix="${PREFIX}" --bindir="${BINDIR}" --libdir="${LIBDIR}" \
	--mandir="${MANDIR}" --enable-threads --enable-framework \
	--with-tcl="${TCL_DIR}" \
	${CONFIGURE_ARGS} ${EXTRA_CONFIGURE_ARGS}; else ./config.status; fi
ifneq (${VERSION},${TCL_VERSION})
	@cd "${OBJ_DIR}" && sed -e 's#/Versions/${TCL_VERSION}#/Versions/${VERSION}#' \
	tkConfig.sh > tkConfig.sh.1 && mv -f tkConfig.sh.1 tkConfig.sh
endif

build-${PROJECT}: ${objdir}/Makefile
	${DO_MAKE}
ifeq (${INSTALL_BUILD},)
# symolic link hackery to trick
# 'make install INSTALL_ROOT=${OBJ_DIR}'
# into building Tk.framework and wish in ${SYMROOT}
	@cd "${OBJ_DIR}" && mkdir -p $(dir $(subst ${space},\ ,./${LIBDIR})) $(dir $(subst ${space},\ ,./${BINDIR})) "${SYMROOT}" && \
	rm -rf "./${LIBDIR}" "./${BINDIR}" && ln -fs "${SYMROOT}" "./${LIBDIR}" && \
	ln -fs "${SYMROOT}" "./${BINDIR}" && ln -fs "${OBJ_DIR}/tktest" "${SYMROOT}"
ifeq (${TK_X11},)
	@rm -f "${OBJ_DIR}/${BINDIR}" && \
	ln -fs Wish.app/Contents/MacOS/Wish "${SYMROOT}/${WISH}"
endif
# Create symbolic link to Tcl.framework in ${SYMROOT}if necessary
	@cd "${SYMROOT}" && if [ ! -e Tcl.framework -o -L Tcl.framework ]; then \
	rm -f Tcl.framework && ln -s "${TCL_FRAMEWORK_DIR}/Tcl.framework" . ; fi
endif

install-${PROJECT}: build-${PROJECT}
ifeq (${EMBEDDED_BUILD}_${INSTALL_ROOT},1_)
	@echo "Cannot install-embedded with empty INSTALL_ROOT !" && false
endif
ifeq (${EMBEDDED_BUILD},1)
	@rm -rf "${INSTALL_ROOT}/${LIBDIR}/Tk.framework"
endif
	${DO_MAKE}
ifeq (${EMBEDDED_BUILD}_${TK_X11},1_)
# workaround bug with 'cp -pRH' on Darwin 6 and earlier
	@if [ "`uname -r | awk -F. '{print $$1}'`" -lt 7 ]; then \
	mkdir -p "${TOP_DIR}"/{"${TCL_FMWK_DIR}","${TK_FMWK_DIR}"}/PrivateHeaders; fi
endif
ifeq (${INSTALL_BUILD},1)
ifeq (${EMBEDDED_BUILD},1)
# if we are embedding frameworks, don't install wish
	@rm -f "${INSTALL_ROOT}${BINDIR}/${WISH}" && \
	rmdir -p "${INSTALL_ROOT}${BINDIR}" 2>&- || true
else
# install wish symbolic link
	@ln -fs ${WISH} "${INSTALL_ROOT}${BINDIR}/${wish}"
endif
endif
ifeq (${BUILD_STYLE}_${EMBEDDED_BUILD},Development_)
# keep copy of debug library around, so that
# Deployment build can be installed on top
# of Development build without overwriting
# the debug library
	@cd "${INSTALL_ROOT}${LIBDIR}/${PRODUCT_NAME}.framework/Versions/${VERSION}" && \
	ln -f "${PRODUCT_NAME}" "${PRODUCT_NAME}_debug"
endif
ifeq (${TK_X11},)
ifeq (${EMBEDDED_BUILD},)
# install Wish.app link in APPLICATION_INSTALL_PATH and setup 'Wish Shell' compatibility links
	@cd "${TOP_DIR}" && if [ -n "${APP_DIR}" ]; then mkdir -p "./${APP_DIR}" && rm -rf "./${APP_DIR}/Wish.app" && \
	ln -fsh "./$$(echo "${APP_DIR}" | sed -e 's#/[^/][^/]*#/..#g')/${FMWK_DIR}/${PRODUCT_NAME}.framework/Resources/Wish.app" "./${APP_DIR}" && \
	ln -fsh Wish.app "./${APP_DIR}/Wish Shell.app"; fi && \
	ln -fsh Wish.app "./${TK_FMWK_DIR}/Resources/Wish Shell.app" && \
	ln -fsh Wish "./${TK_FMWK_DIR}/Resources/Wish.app/Contents/MacOS/Wish Shell"
else
# if we are embedding frameworks, move them into the app and fix their install names
	@cd "${TOP_DIR}" && \
	rm -rf "./${APP_DIR}/Wish.app" && mkdir -p "./${APP_DIR}" && \
	mv -f "./${TK_FMWK_DIR}/Resources/Wish.app" "./${APP_DIR}" && \
	ln -fsh Wish.app "./${APP_DIR}/Wish Shell.app" && \
	rm -rf "./${APP_DIR}/Wish.app/Contents/Frameworks" && \
	mkdir -p "./${APP_DIR}/Wish.app/Contents/Frameworks" && \
	${CPPROG} -RH "./${FMWK_DIR}"/T{cl,k}.framework "./${APP_DIR}/Wish.app/Contents/Frameworks" && \
	cd "./${APP_DIR}/Wish.app/Contents" && \
	rm -rf Frameworks/Tcl.framework/{,/Versions/${TCL_VERSION}}/{Headers,PrivateHeaders,*_debug,lib*.a,*Config.sh} && \
	rm -rf Frameworks/Tk.framework/{,/Versions/${VERSION}}/{Headers,PrivateHeaders,*_debug,lib*.a,*Config.sh} && \
	fix_install_id ( ) { \
	    chmod -RH a+w "$$1"; \
	    install_name_tool -id $$(otool -L "$$1" | awk "/$$2\.framework.*[^:]\$$/ {sub(\"^.*/Frameworks\",\"@executable_path/../Frameworks\",\$$1); print \$$1}") "$$1"; \
	    chmod -RH a-w "$$1"; \
	} && \
	fix_install_name ( ) { \
	    chmod -RH a+w "$$1"; \
	    install_name_tool -change $$(otool -L "$$1" | awk "/$$2\.framework.*[^:]\$$/ {print \$$1; sub(\"^.*/Frameworks\",\"@executable_path/../Frameworks\",\$$1); print \$$1}") "$$1"; \
	    chmod -RH a-w "$$1"; \
	} && \
	fix_install_id Frameworks/Tcl.framework/Tcl Tcl && fix_install_id Frameworks/Tk.framework/Tk Tk && \
	fix_install_name MacOS/Wish Tcl && fix_install_name MacOS/Wish Tk
ifeq (${INSTALL_BUILD},1)
	@cd "${TOP_DIR}" && rm -rf "./${FMWK_DIR}"/T{cl,k}.framework && rmdir -p "./${FMWK_DIR}" 2>&- || true
endif
endif
endif

clean-${PROJECT}: %-${PROJECT}:
	${DO_MAKE}
	rm -rf "${SYMROOT}"/{${PRODUCT_NAME}.framework,${WISH},tktest}
	rm -f "${OBJ_DIR}"{"${LIBDIR}","${BINDIR}"} && \
	rmdir -p "${OBJ_DIR}"$(dir $(subst ${space},\ ,${LIBDIR})) 2>&- || true && \
	rmdir -p "${OBJ_DIR}"$(dir $(subst ${space},\ ,${BINDIR})) 2>&- || true

distclean-${PROJECT}: %-${PROJECT}: clean-${PROJECT}
	${DO_MAKE}
	rm -rf "${OBJ_DIR}"

test-${PROJECT}: %-${PROJECT}: build-${PROJECT}
	${DO_MAKE}

#-------------------------------------------------------------------------------------------------------

.PHONY: ${meta} ${targets} ${PROJECT} build-${PROJECT} install-${PROJECT} \
	clean-${PROJECT} distclean-${PROJECT}

.NOTPARALLEL:

#-------------------------------------------------------------------------------------------------------
