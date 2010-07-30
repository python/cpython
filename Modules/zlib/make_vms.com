$! make libz under VMS written by
$! Martin P.J. Zinser
$!
$! In case of problems with the install you might contact me at
$! zinser@zinser.no-ip.info(preferred) or
$! zinser@sysdev.deutsche-boerse.com (work)
$!
$! Make procedure history for Zlib
$!
$!------------------------------------------------------------------------------
$! Version history
$! 0.01 20060120 First version to receive a number
$! 0.02 20061008 Adapt to new Makefile.in
$! 0.03 20091224 Add support for large file check
$! 0.04 20100110 Add new gzclose, gzlib, gzread, gzwrite
$! 0.05 20100221 Exchange zlibdefs.h by zconf.h.in
$!
$ on error then goto err_exit
$ set proc/parse=ext
$!
$ true  = 1
$ false = 0
$ tmpnam = "temp_" + f$getjpi("","pid")
$ tt = tmpnam + ".txt"
$ tc = tmpnam + ".c"
$ th = tmpnam + ".h"
$ define/nolog tconfig 'th'
$ its_decc = false
$ its_vaxc = false
$ its_gnuc = false
$ s_case   = False
$!
$! Setup variables holding "config" information
$!
$ Make    = ""
$ name     = "Zlib"
$ version  = "?.?.?"
$ v_string = "ZLIB_VERSION"
$ v_file   = "zlib.h"
$ ccopt   = ""
$ lopts   = ""
$ dnsrl   = ""
$ aconf_in_file = "zconf.h.in#zconf.h_in"
$ conf_check_string = ""
$ linkonly = false
$ optfile  = name + ".opt"
$ libdefs  = ""
$ axp      = f$getsyi("HW_MODEL").ge.1024 .and. f$getsyi("HW_MODEL").lt.4096
$!
$ whoami = f$parse(f$enviornment("Procedure"),,,,"NO_CONCEAL")
$ mydef  = F$parse(whoami,,,"DEVICE")
$ mydir  = f$parse(whoami,,,"DIRECTORY") - "]["
$ myproc = f$parse(whoami,,,"Name") + f$parse(whoami,,,"type")
$!
$! Check for MMK/MMS
$!
$ If F$Search ("Sys$System:MMS.EXE") .nes. "" Then Make = "MMS"
$ If F$Type (MMK) .eqs. "STRING" Then Make = "MMK"
$!
$!
$ gosub find_version
$!
$  open/write topt tmp.opt
$  open/write optf 'optfile'
$!
$ gosub check_opts
$!
$! Look for the compiler used
$!
$ gosub check_compiler
$ close topt
$!
$ if its_decc
$ then
$   ccopt = "/prefix=all" + ccopt
$   if f$trnlnm("SYS") .eqs. ""
$   then
$     if axp
$     then
$       define sys sys$library:
$     else
$       ccopt = "/decc" + ccopt
$       define sys decc$library_include:
$     endif
$   endif
$ endif
$ if its_vaxc .or. its_gnuc
$ then
$    if f$trnlnm("SYS").eqs."" then define sys sys$library:
$ endif
$!
$! Build a fake configure input header
$!
$ open/write conf_hin config.hin
$ write conf_hin "#undef _LARGEFILE64_SOURCE"
$ close conf_hin
$!
$!
$ i = 0
$FIND_ACONF:
$ fname = f$element(i,"#",aconf_in_file)
$ if fname .eqs. "#" then goto AMISS_ERR
$ if f$search(fname) .eqs. ""
$ then
$   i = i + 1
$   goto find_aconf
$ endif
$ open/read/err=aconf_err aconf_in 'fname'
$ open/write aconf zconf.h
$ACONF_LOOP:
$ read/end_of_file=aconf_exit aconf_in line
$ work = f$edit(line, "compress,trim")
$ if f$extract(0,6,work) .nes. "#undef"
$ then
$   if f$extract(0,12,work) .nes. "#cmakedefine"
$   then
$       write aconf line
$   endif
$ else
$   cdef = f$element(1," ",work)
$   gosub check_config
$ endif
$ goto aconf_loop
$ACONF_EXIT:
$ write aconf "#define VMS 1"
$ write aconf "#include <unistd.h>"
$ write aconf "#include <unixio.h>"
$ write aconf "#ifdef _LARGEFILE"
$ write aconf "#define off64_t __off64_t"
$ write aconf "#define fopen64 fopen"
$ write aconf "#define fseeko64 fseeko"
$ write aconf "#define lseek64 lseek"
$ write aconf "#define ftello64 ftell"
$ write aconf "#endif"
$ close aconf_in
$ close aconf
$ if f$search("''th'") .nes. "" then delete 'th';*
$! Build the thing plain or with mms
$!
$ write sys$output "Compiling Zlib sources ..."
$ if make.eqs.""
$  then
$   dele example.obj;*,minigzip.obj;*
$   CALL MAKE adler32.OBJ "CC ''CCOPT' adler32" -
                adler32.c zlib.h zconf.h
$   CALL MAKE compress.OBJ "CC ''CCOPT' compress" -
                compress.c zlib.h zconf.h
$   CALL MAKE crc32.OBJ "CC ''CCOPT' crc32" -
                crc32.c zlib.h zconf.h
$   CALL MAKE deflate.OBJ "CC ''CCOPT' deflate" -
                deflate.c deflate.h zutil.h zlib.h zconf.h
$   CALL MAKE gzclose.OBJ "CC ''CCOPT' gzclose" -
                gzclose.c zutil.h zlib.h zconf.h
$   CALL MAKE gzlib.OBJ "CC ''CCOPT' gzlib" -
                gzlib.c zutil.h zlib.h zconf.h
$   CALL MAKE gzread.OBJ "CC ''CCOPT' gzread" -
                gzread.c zutil.h zlib.h zconf.h
$   CALL MAKE gzwrite.OBJ "CC ''CCOPT' gzwrite" -
                gzwrite.c zutil.h zlib.h zconf.h
$   CALL MAKE infback.OBJ "CC ''CCOPT' infback" -
                infback.c zutil.h inftrees.h inflate.h inffast.h inffixed.h
$   CALL MAKE inffast.OBJ "CC ''CCOPT' inffast" -
                inffast.c zutil.h zlib.h zconf.h inffast.h
$   CALL MAKE inflate.OBJ "CC ''CCOPT' inflate" -
                inflate.c zutil.h zlib.h zconf.h infblock.h
$   CALL MAKE inftrees.OBJ "CC ''CCOPT' inftrees" -
                inftrees.c zutil.h zlib.h zconf.h inftrees.h
$   CALL MAKE trees.OBJ "CC ''CCOPT' trees" -
                trees.c deflate.h zutil.h zlib.h zconf.h
$   CALL MAKE uncompr.OBJ "CC ''CCOPT' uncompr" -
                uncompr.c zlib.h zconf.h
$   CALL MAKE zutil.OBJ "CC ''CCOPT' zutil" -
                zutil.c zutil.h zlib.h zconf.h
$   write sys$output "Building Zlib ..."
$   CALL MAKE libz.OLB "lib/crea libz.olb *.obj" *.OBJ
$   write sys$output "Building example..."
$   CALL MAKE example.OBJ "CC ''CCOPT' example" -
                example.c zlib.h zconf.h
$   call make example.exe "LINK example,libz.olb/lib" example.obj libz.olb
$   if f$search("x11vms:xvmsutils.olb") .nes. ""
$   then
$     write sys$output "Building minigzip..."
$     CALL MAKE minigzip.OBJ "CC ''CCOPT' minigzip" -
                minigzip.c zlib.h zconf.h
$     call make minigzip.exe -
                "LINK minigzip,libz.olb/lib,x11vms:xvmsutils.olb/lib" -
                minigzip.obj libz.olb
$   endif
$  else
$   gosub crea_mms
$   write sys$output "Make ''name' ''version' with ''Make' "
$   'make'
$  endif
$!
$! Alpha gets a shareable image
$!
$ If axp
$ Then
$   gosub crea_olist
$   write sys$output "Creating libzshr.exe"
$   call anal_obj_axp modules.opt _link.opt
$   if s_case
$   then
$      open/append optf modules.opt
$      write optf "case_sensitive=YES"
$      close optf
$   endif
$   LINK_'lopts'/SHARE=libzshr.exe modules.opt/opt,_link.opt/opt
$ endif
$ write sys$output "Zlib build completed"
$ exit
$CC_ERR:
$ write sys$output "C compiler required to build ''name'"
$ goto err_exit
$ERR_EXIT:
$ set message/facil/ident/sever/text
$ close/nolog optf
$ close/nolog topt
$ close/nolog conf_hin
$ close/nolog aconf_in
$ close/nolog aconf
$ close/nolog out
$ close/nolog min
$ close/nolog mod
$ close/nolog h_in
$ write sys$output "Exiting..."
$ exit 2
$!
$!
$MAKE: SUBROUTINE   !SUBROUTINE TO CHECK DEPENDENCIES
$ V = 'F$Verify(0)
$! P1 = What we are trying to make
$! P2 = Command to make it
$! P3 - P8  What it depends on
$
$ If F$Search(P1) .Eqs. "" Then Goto Makeit
$ Time = F$CvTime(F$File(P1,"RDT"))
$arg=3
$Loop:
$       Argument = P'arg
$       If Argument .Eqs. "" Then Goto Exit
$       El=0
$Loop2:
$       File = F$Element(El," ",Argument)
$       If File .Eqs. " " Then Goto Endl
$       AFile = ""
$Loop3:
$       OFile = AFile
$       AFile = F$Search(File)
$       If AFile .Eqs. "" .Or. AFile .Eqs. OFile Then Goto NextEl
$       If F$CvTime(F$File(AFile,"RDT")) .Ges. Time Then Goto Makeit
$       Goto Loop3
$NextEL:
$       El = El + 1
$       Goto Loop2
$EndL:
$ arg=arg+1
$ If arg .Le. 8 Then Goto Loop
$ Goto Exit
$
$Makeit:
$ VV=F$VERIFY(0)
$ write sys$output P2
$ 'P2
$ VV='F$Verify(VV)
$Exit:
$ If V Then Set Verify
$ENDSUBROUTINE
$!------------------------------------------------------------------------------
$!
$! Check command line options and set symbols accordingly
$!
$!------------------------------------------------------------------------------
$! Version history
$! 0.01 20041206 First version to receive a number
$! 0.02 20060126 Add new "HELP" target
$ CHECK_OPTS:
$ i = 1
$ OPT_LOOP:
$ if i .lt. 9
$ then
$   cparm = f$edit(p'i',"upcase")
$!
$! Check if parameter actually contains something
$!
$   if f$edit(cparm,"trim") .nes. ""
$   then
$     if cparm .eqs. "DEBUG"
$     then
$       ccopt = ccopt + "/noopt/deb"
$       lopts = lopts + "/deb"
$     endif
$     if f$locate("CCOPT=",cparm) .lt. f$length(cparm)
$     then
$       start = f$locate("=",cparm) + 1
$       len   = f$length(cparm) - start
$       ccopt = ccopt + f$extract(start,len,cparm)
$       if f$locate("AS_IS",f$edit(ccopt,"UPCASE")) .lt. f$length(ccopt) -
          then s_case = true
$     endif
$     if cparm .eqs. "LINK" then linkonly = true
$     if f$locate("LOPTS=",cparm) .lt. f$length(cparm)
$     then
$       start = f$locate("=",cparm) + 1
$       len   = f$length(cparm) - start
$       lopts = lopts + f$extract(start,len,cparm)
$     endif
$     if f$locate("CC=",cparm) .lt. f$length(cparm)
$     then
$       start  = f$locate("=",cparm) + 1
$       len    = f$length(cparm) - start
$       cc_com = f$extract(start,len,cparm)
        if (cc_com .nes. "DECC") .and. -
           (cc_com .nes. "VAXC") .and. -
           (cc_com .nes. "GNUC")
$       then
$         write sys$output "Unsupported compiler choice ''cc_com' ignored"
$         write sys$output "Use DECC, VAXC, or GNUC instead"
$       else
$         if cc_com .eqs. "DECC" then its_decc = true
$         if cc_com .eqs. "VAXC" then its_vaxc = true
$         if cc_com .eqs. "GNUC" then its_gnuc = true
$       endif
$     endif
$     if f$locate("MAKE=",cparm) .lt. f$length(cparm)
$     then
$       start  = f$locate("=",cparm) + 1
$       len    = f$length(cparm) - start
$       mmks = f$extract(start,len,cparm)
$       if (mmks .eqs. "MMK") .or. (mmks .eqs. "MMS")
$       then
$         make = mmks
$       else
$         write sys$output "Unsupported make choice ''mmks' ignored"
$         write sys$output "Use MMK or MMS instead"
$       endif
$     endif
$     if cparm .eqs. "HELP" then gosub bhelp
$   endif
$   i = i + 1
$   goto opt_loop
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! Look for the compiler used
$!
$! Version history
$! 0.01 20040223 First version to receive a number
$! 0.02 20040229 Save/set value of decc$no_rooted_search_lists
$! 0.03 20060202 Extend handling of GNU C
$! 0.04 20090402 Compaq -> hp
$CHECK_COMPILER:
$ if (.not. (its_decc .or. its_vaxc .or. its_gnuc))
$ then
$   its_decc = (f$search("SYS$SYSTEM:DECC$COMPILER.EXE") .nes. "")
$   its_vaxc = .not. its_decc .and. (F$Search("SYS$System:VAXC.Exe") .nes. "")
$   its_gnuc = .not. (its_decc .or. its_vaxc) .and. (f$trnlnm("gnu_cc") .nes. "")
$ endif
$!
$! Exit if no compiler available
$!
$ if (.not. (its_decc .or. its_vaxc .or. its_gnuc))
$ then goto CC_ERR
$ else
$   if its_decc
$   then
$     write sys$output "CC compiler check ... hp C"
$     if f$trnlnm("decc$no_rooted_search_lists") .nes. ""
$     then
$       dnrsl = f$trnlnm("decc$no_rooted_search_lists")
$     endif
$     define/nolog decc$no_rooted_search_lists 1
$   else
$     if its_vaxc then write sys$output "CC compiler check ... VAX C"
$     if its_gnuc
$     then
$         write sys$output "CC compiler check ... GNU C"
$         if f$trnlnm(topt) then write topt "gnu_cc:[000000]gcclib.olb/lib"
$         if f$trnlnm(optf) then write optf "gnu_cc:[000000]gcclib.olb/lib"
$         cc = "gcc"
$     endif
$     if f$trnlnm(topt) then write topt "sys$share:vaxcrtl.exe/share"
$     if f$trnlnm(optf) then write optf "sys$share:vaxcrtl.exe/share"
$   endif
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! If MMS/MMK are available dump out the descrip.mms if required
$!
$CREA_MMS:
$ write sys$output "Creating descrip.mms..."
$ create descrip.mms
$ open/append out descrip.mms
$ copy sys$input: out
$ deck
# descrip.mms: MMS description file for building zlib on VMS
# written by Martin P.J. Zinser
# <zinser@zinser.no-ip.info or zinser@sysdev.deutsche-boerse.com>

OBJS = adler32.obj, compress.obj, crc32.obj, gzclose.obj, gzlib.obj\
       gzread.obj, gzwrite.obj, uncompr.obj, infback.obj\
       deflate.obj, trees.obj, zutil.obj, inflate.obj, \
       inftrees.obj, inffast.obj

$ eod
$ write out "CFLAGS=", ccopt
$ write out "LOPTS=", lopts
$ copy sys$input: out
$ deck

all : example.exe minigzip.exe libz.olb
        @ write sys$output " Example applications available"

libz.olb : libz.olb($(OBJS))
	@ write sys$output " libz available"

example.exe : example.obj libz.olb
              link $(LOPTS) example,libz.olb/lib

minigzip.exe : minigzip.obj libz.olb
              link $(LOPTS) minigzip,libz.olb/lib,x11vms:xvmsutils.olb/lib

clean :
	delete *.obj;*,libz.olb;*,*.opt;*,*.exe;*


# Other dependencies.
adler32.obj  : adler32.c zutil.h zlib.h zconf.h
compress.obj : compress.c zlib.h zconf.h
crc32.obj    : crc32.c zutil.h zlib.h zconf.h
deflate.obj  : deflate.c deflate.h zutil.h zlib.h zconf.h
example.obj  : example.c zlib.h zconf.h
gzclose.obj  : gzclose.c zutil.h zlib.h zconf.h
gzlib.obj    : gzlib.c zutil.h zlib.h zconf.h
gzread.obj   : gzread.c zutil.h zlib.h zconf.h
gzwrite.obj  : gzwrite.c zutil.h zlib.h zconf.h
inffast.obj  : inffast.c zutil.h zlib.h zconf.h inftrees.h inffast.h
inflate.obj  : inflate.c zutil.h zlib.h zconf.h
inftrees.obj : inftrees.c zutil.h zlib.h zconf.h inftrees.h
minigzip.obj : minigzip.c zlib.h zconf.h
trees.obj    : trees.c deflate.h zutil.h zlib.h zconf.h
uncompr.obj  : uncompr.c zlib.h zconf.h
zutil.obj    : zutil.c zutil.h zlib.h zconf.h
infback.obj  : infback.c zutil.h inftrees.h inflate.h inffast.h inffixed.h
$ eod
$ close out
$ return
$!------------------------------------------------------------------------------
$!
$! Read list of core library sources from makefile.in and create options
$! needed to build shareable image
$!
$CREA_OLIST:
$ open/read min makefile.in
$ open/write mod modules.opt
$ src_check = "OBJC ="
$MRLOOP:
$ read/end=mrdone min rec
$ if (f$extract(0,6,rec) .nes. src_check) then goto mrloop
$ rec = rec - src_check
$ gosub extra_filnam
$ if (f$element(1,"\",rec) .eqs. "\") then goto mrdone
$MRSLOOP:
$ read/end=mrdone min rec
$ gosub extra_filnam
$ if (f$element(1,"\",rec) .nes. "\") then goto mrsloop
$MRDONE:
$ close min
$ close mod
$ return
$!------------------------------------------------------------------------------
$!
$! Take record extracted in crea_olist and split it into single filenames
$!
$EXTRA_FILNAM:
$ myrec = f$edit(rec - "\", "trim,compress")
$ i = 0
$FELOOP:
$ srcfil = f$element(i," ", myrec)
$ if (srcfil .nes. " ")
$ then
$   write mod f$parse(srcfil,,,"NAME"), ".obj"
$   i = i + 1
$   goto feloop
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! Find current Zlib version number
$!
$FIND_VERSION:
$ open/read h_in 'v_file'
$hloop:
$ read/end=hdone h_in rec
$ rec = f$edit(rec,"TRIM")
$ if (f$extract(0,1,rec) .nes. "#") then goto hloop
$ rec = f$edit(rec - "#", "TRIM")
$ if f$element(0," ",rec) .nes. "define" then goto hloop
$ if f$element(1," ",rec) .eqs. v_string
$ then
$   version = 'f$element(2," ",rec)'
$   goto hdone
$ endif
$ goto hloop
$hdone:
$ close h_in
$ return
$!------------------------------------------------------------------------------
$!
$CHECK_CONFIG:
$!
$ in_ldef = f$locate(cdef,libdefs)
$ if (in_ldef .lt. f$length(libdefs))
$ then
$   write aconf "#define ''cdef' 1"
$   libdefs = f$extract(0,in_ldef,libdefs) + -
              f$extract(in_ldef + f$length(cdef) + 1, -
                        f$length(libdefs) - in_ldef - f$length(cdef) - 1, -
                        libdefs)
$ else
$   if (f$type('cdef') .eqs. "INTEGER")
$   then
$     write aconf "#define ''cdef' ", 'cdef'
$   else
$     if (f$type('cdef') .eqs. "STRING")
$     then
$       write aconf "#define ''cdef' ", """", '''cdef'', """"
$     else
$       gosub check_cc_def
$     endif
$   endif
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! Check if this is a define relating to the properties of the C/C++
$! compiler
$!
$ CHECK_CC_DEF:
$ if (cdef .eqs. "_LARGEFILE64_SOURCE")
$ then
$   copy sys$input: 'tc'
$   deck
#include "tconfig"
#define _LARGEFILE
#include <stdio.h>

int main(){
FILE *fp;
  fp = fopen("temp.txt","r");
  fseeko(fp,1,SEEK_SET);
  fclose(fp);
}

$   eod
$   test_inv = false
$   comm_h = false
$   gosub cc_prop_check
$   return
$ endif
$ write aconf "/* ", line, " */"
$ return
$!------------------------------------------------------------------------------
$!
$! Check for properties of C/C++ compiler
$!
$! Version history
$! 0.01 20031020 First version to receive a number
$! 0.02 20031022 Added logic for defines with value
$! 0.03 20040309 Make sure local config file gets not deleted
$! 0.04 20041230 Also write include for configure run
$! 0.05 20050103 Add processing of "comment defines"
$CC_PROP_CHECK:
$ cc_prop = true
$ is_need = false
$ is_need = (f$extract(0,4,cdef) .eqs. "NEED") .or. (test_inv .eq. true)
$ if f$search(th) .eqs. "" then create 'th'
$ set message/nofac/noident/nosever/notext
$ on error then continue
$ cc 'tmpnam'
$ if .not. ($status)  then cc_prop = false
$ on error then continue
$! The headers might lie about the capabilities of the RTL
$ link 'tmpnam',tmp.opt/opt
$ if .not. ($status)  then cc_prop = false
$ set message/fac/ident/sever/text
$ on error then goto err_exit
$ delete/nolog 'tmpnam'.*;*/exclude='th'
$ if (cc_prop .and. .not. is_need) .or. -
     (.not. cc_prop .and. is_need)
$ then
$   write sys$output "Checking for ''cdef'... yes"
$   if f$type('cdef_val'_yes) .nes. ""
$   then
$     if f$type('cdef_val'_yes) .eqs. "INTEGER" -
         then call write_config f$fao("#define !AS !UL",cdef,'cdef_val'_yes)
$     if f$type('cdef_val'_yes) .eqs. "STRING" -
         then call write_config f$fao("#define !AS !AS",cdef,'cdef_val'_yes)
$   else
$     call write_config f$fao("#define !AS 1",cdef)
$   endif
$   if (cdef .eqs. "HAVE_FSEEKO") .or. (cdef .eqs. "_LARGE_FILES") .or. -
       (cdef .eqs. "_LARGEFILE64_SOURCE") then -
      call write_config f$string("#define _LARGEFILE 1")
$ else
$   write sys$output "Checking for ''cdef'... no"
$   if (comm_h)
$   then
      call write_config f$fao("/* !AS */",line)
$   else
$     if f$type('cdef_val'_no) .nes. ""
$     then
$       if f$type('cdef_val'_no) .eqs. "INTEGER" -
           then call write_config f$fao("#define !AS !UL",cdef,'cdef_val'_no)
$       if f$type('cdef_val'_no) .eqs. "STRING" -
           then call write_config f$fao("#define !AS !AS",cdef,'cdef_val'_no)
$     else
$       call write_config f$fao("#undef !AS",cdef)
$     endif
$   endif
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! Check for properties of C/C++ compiler with multiple result values
$!
$! Version history
$! 0.01 20040127 First version
$! 0.02 20050103 Reconcile changes from cc_prop up to version 0.05
$CC_MPROP_CHECK:
$ cc_prop = true
$ i    = 1
$ idel = 1
$ MT_LOOP:
$ if f$type(result_'i') .eqs. "STRING"
$ then
$   set message/nofac/noident/nosever/notext
$   on error then continue
$   cc 'tmpnam'_'i'
$   if .not. ($status)  then cc_prop = false
$   on error then continue
$! The headers might lie about the capabilities of the RTL
$   link 'tmpnam'_'i',tmp.opt/opt
$   if .not. ($status)  then cc_prop = false
$   set message/fac/ident/sever/text
$   on error then goto err_exit
$   delete/nolog 'tmpnam'_'i'.*;*
$   if (cc_prop)
$   then
$     write sys$output "Checking for ''cdef'... ", mdef_'i'
$     if f$type(mdef_'i') .eqs. "INTEGER" -
         then call write_config f$fao("#define !AS !UL",cdef,mdef_'i')
$     if f$type('cdef_val'_yes) .eqs. "STRING" -
         then call write_config f$fao("#define !AS !AS",cdef,mdef_'i')
$     goto msym_clean
$   else
$     i = i + 1
$     goto mt_loop
$   endif
$ endif
$ write sys$output "Checking for ''cdef'... no"
$ call write_config f$fao("#undef !AS",cdef)
$ MSYM_CLEAN:
$ if (idel .le. msym_max)
$ then
$   delete/sym mdef_'idel'
$   idel = idel + 1
$   goto msym_clean
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! Analyze Object files for OpenVMS AXP to extract Procedure and Data
$! information to build a symbol vector for a shareable image
$! All the "brains" of this logic was suggested by Hartmut Becker
$! (Hartmut.Becker@compaq.com). All the bugs were introduced by me
$! (zinser@zinser.no-ip.info), so if you do have problem reports please do not
$! bother Hartmut/HP, but get in touch with me
$!
$! Version history
$! 0.01 20040406 Skip over shareable images in option file
$! 0.02 20041109 Fix option file for shareable images with case_sensitive=YES
$! 0.03 20050107 Skip over Identification labels in option file
$! 0.04 20060117 Add uppercase alias to code compiled with /name=as_is
$!
$ ANAL_OBJ_AXP: Subroutine
$ V = 'F$Verify(0)
$ SAY := "WRITE_ SYS$OUTPUT"
$
$ IF F$SEARCH("''P1'") .EQS. ""
$ THEN
$    SAY "ANAL_OBJ_AXP-E-NOSUCHFILE:  Error, inputfile ''p1' not available"
$    goto exit_aa
$ ENDIF
$ IF "''P2'" .EQS. ""
$ THEN
$    SAY "ANAL_OBJ_AXP:  Error, no output file provided"
$    goto exit_aa
$ ENDIF
$
$ open/read in 'p1
$ create a.tmp
$ open/append atmp a.tmp
$ loop:
$ read/end=end_loop in line
$ if f$locate("/SHARE",f$edit(line,"upcase")) .lt. f$length(line)
$ then
$   write sys$output "ANAL_SKP_SHR-i-skipshare, ''line'"
$   goto loop
$ endif
$ if f$locate("IDENTIFICATION=",f$edit(line,"upcase")) .lt. f$length(line)
$ then
$   write sys$output "ANAL_OBJ_AXP-i-ident: Identification ", -
                     f$element(1,"=",line)
$   goto loop
$ endif
$ f= f$search(line)
$ if f .eqs. ""
$ then
$	write sys$output "ANAL_OBJ_AXP-w-nosuchfile, ''line'"
$	goto loop
$ endif
$ define/user sys$output nl:
$ define/user sys$error nl:
$ anal/obj/gsd 'f /out=x.tmp
$ open/read xtmp x.tmp
$ XLOOP:
$ read/end=end_xloop xtmp xline
$ xline = f$edit(xline,"compress")
$ write atmp xline
$ goto xloop
$ END_XLOOP:
$ close xtmp
$ goto loop
$ end_loop:
$ close in
$ close atmp
$ if f$search("a.tmp") .eqs. "" -
	then $ exit
$ ! all global definitions
$ search a.tmp "symbol:","EGSY$V_DEF 1","EGSY$V_NORM 1"/out=b.tmp
$ ! all procedures
$ search b.tmp "EGSY$V_NORM 1"/wind=(0,1) /out=c.tmp
$ search c.tmp "symbol:"/out=d.tmp
$ define/user sys$output nl:
$ edito/edt/command=sys$input d.tmp
sub/symbol: "/symbol_vector=(/whole
sub/"/=PROCEDURE)/whole
exit
$ ! all data
$ search b.tmp "EGSY$V_DEF 1"/wind=(0,1) /out=e.tmp
$ search e.tmp "symbol:"/out=f.tmp
$ define/user sys$output nl:
$ edito/edt/command=sys$input f.tmp
sub/symbol: "/symbol_vector=(/whole
sub/"/=DATA)/whole
exit
$ sort/nodupl d.tmp,f.tmp g.tmp
$ open/read raw_vector g.tmp
$ open/write case_vector 'p2'
$ RAWLOOP:
$ read/end=end_rawloop raw_vector raw_element
$ write case_vector raw_element
$ if f$locate("=PROCEDURE)",raw_element) .lt. f$length(raw_element)
$ then
$     name = f$element(1,"=",raw_element) - "("
$     if f$edit(name,"UPCASE") .nes. name then -
          write case_vector f$fao(" symbol_vector=(!AS/!AS=PROCEDURE)", -
	                          f$edit(name,"UPCASE"), name)
$ endif
$ if f$locate("=DATA)",raw_element) .lt. f$length(raw_element)
$ then
$     name = f$element(1,"=",raw_element) - "("
$     if f$edit(name,"UPCASE") .nes. name then -
          write case_vector f$fao(" symbol_vector=(!AS/!AS=DATA)", -
	                          f$edit(name,"UPCASE"), name)
$ endif
$ goto rawloop
$ END_RAWLOOP:
$ close raw_vector
$ close case_vector
$ delete a.tmp;*,b.tmp;*,c.tmp;*,d.tmp;*,e.tmp;*,f.tmp;*,g.tmp;*
$ if f$search("x.tmp") .nes. "" -
	then $ delete x.tmp;*
$!
$ EXIT_AA:
$ if V then set verify
$ endsubroutine
$!------------------------------------------------------------------------------
$!
$! Write configuration to both permanent and temporary config file
$!
$! Version history
$! 0.01 20031029 First version to receive a number
$!
$WRITE_CONFIG: SUBROUTINE
$  write aconf 'p1'
$  open/append confh 'th'
$  write confh 'p1'
$  close confh
$ENDSUBROUTINE
$!------------------------------------------------------------------------------
