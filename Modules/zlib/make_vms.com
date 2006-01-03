$! make libz under VMS written by
$! Martin P.J. Zinser
$! <zinser@zinser.no-ip.info or zinser@sysdev.deutsche-boerse.com>
$!
$ on error then goto err_exit
$!
$!
$! Just some general constants...
$!
$ true  = 1
$ false = 0
$ tmpnam = "temp_" + f$getjpi("","pid")
$ SAY = "WRITE SYS$OUTPUT"
$!
$! Setup variables holding "config" information
$!
$ Make     = ""
$ name     = "Zlib"
$ version  = "?.?.?"
$ v_string = "ZLIB_VERSION"
$ v_file   = "zlib.h"
$ ccopt    = ""
$ lopts    = ""
$ linkonly = false
$ optfile  = name + ".opt"
$ its_decc = false
$ its_vaxc = false
$ its_gnuc = false
$ axp      = f$getsyi("HW_MODEL").ge.1024
$ s_case   = false
$! Check for MMK/MMS
$!
$ If F$Search ("Sys$System:MMS.EXE") .nes. "" Then Make = "MMS"
$ If F$Type (MMK) .eqs. "STRING" Then Make = "MMK"
$!
$!
$ gosub find_version
$!
$ gosub check_opts
$!
$! Look for the compiler used
$!
$ gosub check_compiler
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
$   CALL MAKE gzio.OBJ "CC ''CCOPT' gzio" -
                gzio.c zutil.h zlib.h zconf.h
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
$   SAY "Make ''name' ''version' with ''Make' "
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
$ CHECK_OPTS:
$ i = 1
$ OPT_LOOP:
$ if i .lt. 9
$ then
$   cparm = f$edit(p'i',"upcase")
$   if cparm .eqs. "DEBUG"
$   then
$     ccopt = ccopt + "/noopt/deb"
$     lopts = lopts + "/deb"
$   endif
$   if f$locate("CCOPT=",cparm) .lt. f$length(cparm)
$   then
$     start = f$locate("=",cparm) + 1
$     len   = f$length(cparm) - start
$     ccopt = ccopt + f$extract(start,len,cparm)
$     if f$locate("AS_IS",f$edit(ccopt,"UPCASE")) .lt. f$length(ccopt) -
         then s_case = true
$   endif
$   if cparm .eqs. "LINK" then linkonly = true
$   if f$locate("LOPTS=",cparm) .lt. f$length(cparm)
$   then
$     start = f$locate("=",cparm) + 1
$     len   = f$length(cparm) - start
$     lopts = lopts + f$extract(start,len,cparm)
$   endif
$   if f$locate("CC=",cparm) .lt. f$length(cparm)
$   then
$     start  = f$locate("=",cparm) + 1
$     len    = f$length(cparm) - start
$     cc_com = f$extract(start,len,cparm)
      if (cc_com .nes. "DECC") .and. -
         (cc_com .nes. "VAXC") .and. -
	 (cc_com .nes. "GNUC")
$     then
$       write sys$output "Unsupported compiler choice ''cc_com' ignored"
$       write sys$output "Use DECC, VAXC, or GNUC instead"
$     else
$     	if cc_com .eqs. "DECC" then its_decc = true
$     	if cc_com .eqs. "VAXC" then its_vaxc = true
$     	if cc_com .eqs. "GNUC" then its_gnuc = true
$     endif
$   endif
$   if f$locate("MAKE=",cparm) .lt. f$length(cparm)
$   then
$     start  = f$locate("=",cparm) + 1
$     len    = f$length(cparm) - start
$     mmks = f$extract(start,len,cparm)
$     if (mmks .eqs. "MMK") .or. (mmks .eqs. "MMS")
$     then
$       make = mmks
$     else
$       write sys$output "Unsupported make choice ''mmks' ignored"
$       write sys$output "Use MMK or MMS instead"
$     endif
$   endif
$   i = i + 1
$   goto opt_loop
$ endif
$ return
$!------------------------------------------------------------------------------
$!
$! Look for the compiler used
$!
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
$   if its_decc then write sys$output "CC compiler check ... Compaq C"
$   if its_vaxc then write sys$output "CC compiler check ... VAX C"
$   if its_gnuc then write sys$output "CC compiler check ... GNU C"
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

OBJS = adler32.obj, compress.obj, crc32.obj, gzio.obj, uncompr.obj, infback.obj\
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
gzio.obj     : gzio.c zutil.h zlib.h zconf.h
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
$ src_check = "OBJS ="
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
$! Analyze Object files for OpenVMS AXP to extract Procedure and Data
$! information to build a symbol vector for a shareable image
$! All the "brains" of this logic was suggested by Hartmut Becker
$! (Hartmut.Becker@compaq.com). All the bugs were introduced by me
$! (zinser@decus.de), so if you do have problem reports please do not
$! bother Hartmut/HP, but get in touch with me
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
$ sort/nodupl d.tmp,f.tmp 'p2'
$ delete a.tmp;*,b.tmp;*,c.tmp;*,d.tmp;*,e.tmp;*,f.tmp;*
$ if f$search("x.tmp") .nes. "" -
	then $ delete x.tmp;*
$!
$ EXIT_AA:
$ if V then set verify
$ endsubroutine
$!------------------------------------------------------------------------------
