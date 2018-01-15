# vxworks.mak - for python
#
# Copyright 2017, Wind River Systems, Inc.
#
# This software is released under the terms of the 
# Python Software Foundation License Version 2
# included in this repository in the LICENSE file
# or at https://docs.python.org/3/license.html
#
# modification history
# -------------------- 
# 23sep17,brk  create

RTP_BASE_DIR = $(VXE)

ADDED_LIBS = -lunix

#.NOTPARALLEL: 

.PHONY: python_default  install build

python_default : install 

include $(WIND_USR_MK)/rules.env.sh.mk

PY_VX_HOST := $(strip $(WIND_HOST_TYPE))

ifeq 'x86-win32' '$(PY_VX_HOST)'
PYTHON_BUILD = i686-pc-cygwin
else
PYTHON_BUILD = i686-linux-gnu
endif

ADDED_CONFIGURE =  --build=$(PYTHON_BUILD)  --cache-file=config.vx.app --with-libm=no
ADDED_CONFIGURE +=  --with-ensurepip=no --with-suffix=.vxe

SUBDIRS = dummy
EXCLUDE_SUBDIRS = dummy

ifdef _WRS_CONFIG_PYTHON_LIBS_STATIC
#
# automake has no varible for static link specific flags, so we addd to LDFLAGS 
# pybuilddir.txt target generates _sysconfigdata_m_vxworks_.py needed for testing and install
# presence of dlopen() causes define of HAVE_DYNAMIC_LOADING
#
ADDED_CONFIGURE +=  ac_cv_func_dlopen=no LDFLAGS='$(LDFLAGS) $(LDFLAGS_STATIC)' 
AUTOCONF_MAKE_TARGET = python.vxe  pybuilddir.txt
else
ADDED_CONFIGURE +=  ac_cv_func_dlopen=yes
MAKE_INSTALL_ARGS += sharedinstall oldsharedinstall
endif

MAKE_INSTALL_ARGS += libinstall bininstall

install : build
	@echo building $(VSBL_NAME): Installing $(VXE)
	$(MAKE) -f Makefile $(MAKE_INSTALL_ARGS)

$(ROOT_DIR)/share : $(ROOT_DIR)

$(ROOT_DIR)/share $(ROOT_DIR): 
	@test -d $@ || mkdir -p $@

#  share/site.config shell file contains cached values for autoconfig
$(ROOT_CONFIG_SITE) : $(ROOT_DIR)/share 
	@cp $(WIND_BASE)/build/misc/$(notdir $@ ) $@

Makefile : $(ROOT_CONFIG_SITE) $(AUTO_INCLUDE_VSB_CONFIG_QUOTE) $(VXWORKS_ENV_SH)  $(__AUTO_INCLUDE_LIST_UFILE) 
	@echo  building $(VSBL_NAME): Configuring $(VXE)
	. ./$(VXWORKS_ENV_SH) && \
	./configure --host=$(C_ARCH)-wrs-vxworks --prefix=$(ROOT_DIR) \
	--bindir=$(ROOT_DIR)/$(TOOL_SPECIFIC_DIR)/bin --includedir=$(VSB_DIR)/usr/h/public \
	--libdir=$(LIBDIR)/$(TOOL_COMMON_DIR)  $(ADDED_CONFIGURE)

build : Makefile 
	@echo  building $(VSBL_NAME): $(VXE) from autoconf Makefile
	$(MAKE) -f Makefile $(AUTOCONF_MAKE_TARGET)

