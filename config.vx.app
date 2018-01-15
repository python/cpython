#! /bin/sh
# config.vx.app - autoconf configuration file for VxWorks
#
# Copyright (c) 2017  Wind River Systems, Inc.
#
# This software is released under the terms of the 
# Python Software Foundation License Version 2
# included in this repository in the LICENSE file
# or at https://docs.python.org/3/license.html
#
# modification history
# -------------------- 
# 23sep17,brk  create
	

	#defined in UNIX layer, but just a wrapper to select()
	ac_cv_func_poll=no
	 
	# forces define of PY_FORMAT_LONG_LONG (Python)
	ac_cv_have_long_long_format=yes

	#Python wants these explicit when cross compiling
	
	ac_cv_file__dev_ptmx=no
	ac_cv_file__dev_ptc=no
	
	ac_cv_buggy_getaddrinfo=no
	
	ac_cv_func_gettimeofday=yes
	
	#ignore empty header in UNIX layer
	ac_cv_header_langinfo_h=no
	
	#test failing though header is in UNIX layer
	#perhaps include paths truncated in configure?
	ac_cv_header_sys_times_h=yes
	

        #avoid not finding pthread in various extra libs, 
        ac_cv_pthread_is_default=yes

        #gcc fails to compile endian test
        case $host in 
                 ppc*-vxworks )
                        ac_cv_c_bigendian=yes ;;
                 arm*-vxworks )
                        ac_cv_c_bigendian=no ;;
                 x86*-vxworks )
                        ac_cv_c_bigendian=no ;;
                 mips*-vxworks )
                        ac_cv_c_bigendian=yes ;;
        esac


