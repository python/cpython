#!/bin/bash 
# This is a generated file. Do not edit.
# Contents set the cross build environment for the VxWorks Source Build project:
# /home/buildbot/other/vxworks/workbench-4/workspace/vxlib          
 export CPU='SIMLINUX' 
 export TOOL='gnu' 
 export VSB_DIR='/home/buildbot/other/vxworks/workbench-4/workspace/vxlib' 
 export ROOT_DIR='/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/root' 
 export CC='/home/buildbot/other/vxworks/compilers/gnu-4.8.1.10/x86-linux2/bin/ccpentium' 
 export CPP='/home/buildbot/other/vxworks/compilers/gnu-4.8.1.10/x86-linux2/bin/ccpentium -E -P' 
 export CXX='/home/buildbot/other/vxworks/compilers/gnu-4.8.1.10/x86-linux2/bin/c++pentium' 
 export AR='/home/buildbot/other/vxworks/compilers/gnu-4.8.1.10/x86-linux2/bin/arpentium' 
 export AS='/home/buildbot/other/vxworks/compilers/gnu-4.8.1.10/x86-linux2/bin/aspentium' 
 export NM='nmpentium' 
 export READELF='readelfpentium' 
 export STRIP='strippentium -g' 
 export RPATH='/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/lib/common/PIC' 
 export SHLIB_LD='' 
 export LDFLAGS_SO='-fpic -D__SO_PICABILINUX__  -shared -L/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/lib/common/PIC' 
 export LIBPATH='/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/lib/common' 
 export CCSHARED='-fpic -D__SO_PICABILINUX__ ' 
 export CFLAGS='-m32 -mfpmath=387 -fno-omit-frame-pointer -fno-builtin -mrtp -fno-strict-aliasing -std=c99 -fasm -fno-implicit-fp -O2 -fno-defer-pop -Wall'      
 export CPPFLAGS=' -include/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/3pp/PYTHON/cpython/autoconf_vsbConfig_quote.h -D_C99 -D_HAS_C9X @/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/mk/_vsbuinc_cmdgnuPYTHON -D_VX_CPU=_VX_SIMLINUX -D_VX_TOOL_FAMILY=gnu -D_VX_TOOL=gnu -D_nq_VSB_CONFIG_FILE=/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/h/config/vsbConfig.h -I/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/share/h -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/system -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/public -D_VSB_PUBLIC_HDR_DIR=/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/public -I.'    
 export LD='/home/buildbot/other/vxworks/compilers/gnu-4.8.1.10/x86-linux2/bin/c++pentium'          
 export LDSHARED='/home/buildbot/other/vxworks/compilers/gnu-4.8.1.10/x86-linux2/bin/c++pentium -fpic -D__SO_PICABILINUX__  -shared -L/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/lib/common/PIC'    
 export LDCXXSHARED='/home/buildbot/other/vxworks/compilers/gnu-4.8.1.10/x86-linux2/bin/c++pentium -fpic -D__SO_PICABILINUX__  -shared -L/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/lib/common/PIC' 
 export LDFLAGS='-m32 -mfpmath=387 -fno-omit-frame-pointer -fno-builtin -ansi -mrtp -fno-strict-aliasing -std=c++11 -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/public/c++11 -fno-implicit-fp -O2 -fno-defer-pop -Wall -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/system -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/public -Wl,--defsym,__wrs_rtp_base=0x68000000 -L/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/lib/common -Wl,-z,common-page-size=8192'     
 export CXXFLAGS='-m32 -mfpmath=387 -fno-omit-frame-pointer -fno-builtin -ansi -mrtp -fno-strict-aliasing -std=c++11 -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/public/c++11 -fno-implicit-fp -O2 -fno-defer-pop -Wall -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/system -isystem/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/h/public'    
 export LIBS='-lunix  -lsupc++  -lstdc++11 '        
 export SHLIBS=' -Wl,-lc'      
        
 export LDFLAGS_STATIC='-Wl,-u,__tls__ -Wl,-T,/home/buildbot/other/vxworks/vxworks-7/build/tool/gnu_4_8_1_8/usr/ldscript.vxe.ilp32.simcommon' 
 export LDFLAGS_DYNAMIC='-non-static  -Wl,-lc'
 export LINKFORSHARED='-non-static'
 export CONFIG_SITE='./config.vx.app /home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/root/share/config.site' 
 export cross_compiling=yes  
 export host=x86-wrs-vxworks 
 export prefix=/home/buildbot/other/vxworks/workbench-4/workspace/vxlib/usr/root  
