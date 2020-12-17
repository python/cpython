# Microsoft Developer Studio Project File - Name="tcl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=tcl - Win32 Debug Static
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "tcl.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "tcl.mak" CFG="tcl - Win32 Debug Static"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "tcl - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "tcl - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE "tcl - Win32 Debug Static" (based on "Win32 (x86) External Target")
!MESSAGE "tcl - Win32 Release Static" (based on "Win32 (x86) External Target")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "tcl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release\tcl_Dynamic"
# PROP BASE Cmd_Line "nmake -nologo -f makefile.vc OPTS=none MSVCDIR=IDE"
# PROP BASE Rebuild_Opt "-a"
# PROP BASE Target_File "Release\tclsh86.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\tcl_Dynamic"
# PROP Cmd_Line "nmake -nologo -f makefile.vc OPTS=threads MSVCDIR=IDE"
# PROP Rebuild_Opt "clean release"
# PROP Target_File "Release\tclsh86t.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "tcl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug\tcl_Dynamic"
# PROP BASE Cmd_Line "nmake -nologo -f makefile.vc OPTS=symbols MSVCDIR=IDE"
# PROP BASE Rebuild_Opt "-a"
# PROP BASE Target_File "Debug\tclsh86g.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\tcl_Dynamic"
# PROP Cmd_Line "nmake -nologo -f makefile.vc OPTS=threads,symbols MSVCDIR=IDE"
# PROP Rebuild_Opt "clean release"
# PROP Target_File "Debug\tclsh86tg.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "tcl - Win32 Debug Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug\tcl_Static"
# PROP BASE Cmd_Line "nmake -nologo -f makefile.vc OPTS=symbols,static MSVCDIR=IDE"
# PROP BASE Rebuild_Opt "-a"
# PROP BASE Target_File "Debug\tclsh86sg.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug\tcl_Static"
# PROP Cmd_Line "nmake -nologo -f makefile.vc OPTS=symbols,static MSVCDIR=IDE"
# PROP Rebuild_Opt "-a"
# PROP Target_File "Debug\tclsh86sg.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "tcl - Win32 Release Static"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release\tcl_Static"
# PROP BASE Cmd_Line "nmake -nologo -f makefile.vc OPTS=static MSVCDIR=IDE"
# PROP BASE Rebuild_Opt "-a"
# PROP BASE Target_File "Release\tclsh86s.exe"
# PROP BASE Bsc_Name ""
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release\tcl_Static"
# PROP Cmd_Line "nmake -nologo -f makefile.vc OPTS=static MSVCDIR=IDE"
# PROP Rebuild_Opt "-a"
# PROP Target_File "Release\tclsh86s.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF

# Begin Target

# Name "tcl - Win32 Release"
# Name "tcl - Win32 Debug"
# Name "tcl - Win32 Debug Static"
# Name "tcl - Win32 Release Static"

!IF  "$(CFG)" == "tcl - Win32 Release"

!ELSEIF  "$(CFG)" == "tcl - Win32 Debug"

!ELSEIF  "$(CFG)" == "tcl - Win32 Debug Static"

!ELSEIF  "$(CFG)" == "tcl - Win32 Release Static"

!ENDIF

# Begin Group "compat"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\compat\dirent.h
# End Source File
# Begin Source File

SOURCE=..\compat\dirent2.h
# End Source File
# Begin Source File

SOURCE=..\compat\dlfcn.h
# End Source File
# Begin Source File

SOURCE=..\compat\fixstrtod.c
# End Source File
# Begin Source File

SOURCE=..\compat\float.h
# End Source File
# Begin Source File

SOURCE=..\compat\gettod.c
# End Source File
# Begin Source File

SOURCE=..\compat\limits.h
# End Source File
# Begin Source File

SOURCE=..\compat\memcmp.c
# End Source File
# Begin Source File

SOURCE=..\compat\opendir.c
# End Source File
# Begin Source File

SOURCE=..\compat\README
# End Source File
# Begin Source File

SOURCE=..\compat\stdlib.h
# End Source File
# Begin Source File

SOURCE=..\compat\string.h
# End Source File
# Begin Source File

SOURCE=..\compat\strncasecmp.c
# End Source File
# Begin Source File

SOURCE=..\compat\strstr.c
# End Source File
# Begin Source File

SOURCE=..\compat\strtod.c
# End Source File
# Begin Source File

SOURCE=..\compat\strtol.c
# End Source File
# Begin Source File

SOURCE=..\compat\strtoul.c
# End Source File
# Begin Source File

SOURCE=..\compat\tclErrno.h
# End Source File
# Begin Source File

SOURCE=..\compat\unistd.h
# End Source File
# Begin Source File

SOURCE=..\compat\waitpid.c
# End Source File
# End Group
# Begin Group "doc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\doc\Access.3
# End Source File
# Begin Source File

SOURCE=..\doc\AddErrInfo.3
# End Source File
# Begin Source File

SOURCE=..\doc\after.n
# End Source File
# Begin Source File

SOURCE=..\doc\Alloc.3
# End Source File
# Begin Source File

SOURCE=..\doc\AllowExc.3
# End Source File
# Begin Source File

SOURCE=..\doc\append.n
# End Source File
# Begin Source File

SOURCE=..\doc\AppInit.3
# End Source File
# Begin Source File

SOURCE=..\doc\array.n
# End Source File
# Begin Source File

SOURCE=..\doc\AssocData.3
# End Source File
# Begin Source File

SOURCE=..\doc\Async.3
# End Source File
# Begin Source File

SOURCE=..\doc\BackgdErr.3
# End Source File
# Begin Source File

SOURCE=..\doc\Backslash.3
# End Source File
# Begin Source File

SOURCE=..\doc\bgerror.n
# End Source File
# Begin Source File

SOURCE=..\doc\binary.n
# End Source File
# Begin Source File

SOURCE=..\doc\BoolObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\break.n
# End Source File
# Begin Source File

SOURCE=..\doc\ByteArrObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\CallDel.3
# End Source File
# Begin Source File

SOURCE=..\doc\case.n
# End Source File
# Begin Source File

SOURCE=..\doc\catch.n
# End Source File
# Begin Source File

SOURCE=..\doc\cd.n
# End Source File
# Begin Source File

SOURCE=..\doc\ChnlStack.3
# End Source File
# Begin Source File

SOURCE=..\doc\clock.n
# End Source File
# Begin Source File

SOURCE=..\doc\close.n
# End Source File
# Begin Source File

SOURCE=..\doc\CmdCmplt.3
# End Source File
# Begin Source File

SOURCE=..\doc\Concat.3
# End Source File
# Begin Source File

SOURCE=..\doc\concat.n
# End Source File
# Begin Source File

SOURCE=..\doc\continue.n
# End Source File
# Begin Source File

SOURCE=..\doc\CrtChannel.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtChnlHdlr.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtCloseHdlr.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtCommand.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtFileHdlr.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtInterp.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtMathFnc.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtObjCmd.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtSlave.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtTimerHdlr.3
# End Source File
# Begin Source File

SOURCE=..\doc\CrtTrace.3
# End Source File
# Begin Source File

SOURCE=..\doc\dde.n
# End Source File
# Begin Source File

SOURCE=..\doc\DetachPids.3
# End Source File
# Begin Source File

SOURCE=..\doc\DoOneEvent.3
# End Source File
# Begin Source File

SOURCE=..\doc\DoubleObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\DoWhenIdle.3
# End Source File
# Begin Source File

SOURCE=..\doc\DString.3
# End Source File
# Begin Source File

SOURCE=..\doc\DumpActiveMemory.3
# End Source File
# Begin Source File

SOURCE=..\doc\Encoding.3
# End Source File
# Begin Source File

SOURCE=..\doc\encoding.n
# End Source File
# Begin Source File

SOURCE=..\doc\Environment.3
# End Source File
# Begin Source File

SOURCE=..\doc\eof.n
# End Source File
# Begin Source File

SOURCE=..\doc\error.n
# End Source File
# Begin Source File

SOURCE=..\doc\Eval.3
# End Source File
# Begin Source File

SOURCE=..\doc\eval.n
# End Source File
# Begin Source File

SOURCE=..\doc\exec.n
# End Source File
# Begin Source File

SOURCE=..\doc\Exit.3
# End Source File
# Begin Source File

SOURCE=..\doc\exit.n
# End Source File
# Begin Source File

SOURCE=..\doc\expr.n
# End Source File
# Begin Source File

SOURCE=..\doc\ExprLong.3
# End Source File
# Begin Source File

SOURCE=..\doc\ExprLongObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\fblocked.n
# End Source File
# Begin Source File

SOURCE=..\doc\fconfigure.n
# End Source File
# Begin Source File

SOURCE=..\doc\fcopy.n
# End Source File
# Begin Source File

SOURCE=..\doc\file.n
# End Source File
# Begin Source File

SOURCE=..\doc\fileevent.n
# End Source File
# Begin Source File

SOURCE=..\doc\filename.n
# End Source File
# Begin Source File

SOURCE=..\doc\FileSystem.3
# End Source File
# Begin Source File

SOURCE=..\doc\FindExec.3
# End Source File
# Begin Source File

SOURCE=..\doc\flush.n
# End Source File
# Begin Source File

SOURCE=..\doc\for.n
# End Source File
# Begin Source File

SOURCE=..\doc\foreach.n
# End Source File
# Begin Source File

SOURCE=..\doc\format.n
# End Source File
# Begin Source File

SOURCE=..\doc\GetCwd.3
# End Source File
# Begin Source File

SOURCE=..\doc\GetHostName.3
# End Source File
# Begin Source File

SOURCE=..\doc\GetIndex.3
# End Source File
# Begin Source File

SOURCE=..\doc\GetInt.3
# End Source File
# Begin Source File

SOURCE=..\doc\GetOpnFl.3
# End Source File
# Begin Source File

SOURCE=..\doc\gets.n
# End Source File
# Begin Source File

SOURCE=..\doc\GetStdChan.3
# End Source File
# Begin Source File

SOURCE=..\doc\GetVersion.3
# End Source File
# Begin Source File

SOURCE=..\doc\glob.n
# End Source File
# Begin Source File

SOURCE=..\doc\global.n
# End Source File
# Begin Source File

SOURCE=..\doc\Hash.3
# End Source File
# Begin Source File

SOURCE=..\doc\history.n
# End Source File
# Begin Source File

SOURCE=..\doc\http.n
# End Source File
# Begin Source File

SOURCE=..\doc\if.n
# End Source File
# Begin Source File

SOURCE=..\doc\incr.n
# End Source File
# Begin Source File

SOURCE=..\doc\info.n
# End Source File
# Begin Source File

SOURCE=..\doc\Init.3
# End Source File
# Begin Source File

SOURCE=..\doc\InitStubs.3
# End Source File
# Begin Source File

SOURCE=..\doc\Interp.3
# End Source File
# Begin Source File

SOURCE=..\doc\interp.n
# End Source File
# Begin Source File

SOURCE=..\doc\IntObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\join.n
# End Source File
# Begin Source File

SOURCE=..\doc\lappend.n
# End Source File
# Begin Source File

SOURCE=..\doc\library.n
# End Source File
# Begin Source File

SOURCE=..\doc\lindex.n
# End Source File
# Begin Source File

SOURCE=..\doc\LinkVar.3
# End Source File
# Begin Source File

SOURCE=..\doc\linsert.n
# End Source File
# Begin Source File

SOURCE=..\doc\list.n
# End Source File
# Begin Source File

SOURCE=..\doc\ListObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\llength.n
# End Source File
# Begin Source File

SOURCE=..\doc\load.n
# End Source File
# Begin Source File

SOURCE=..\doc\lrange.n
# End Source File
# Begin Source File

SOURCE=..\doc\lreplace.n
# End Source File
# Begin Source File

SOURCE=..\doc\lsearch.n
# End Source File
# Begin Source File

SOURCE=..\doc\lsort.n
# End Source File
# Begin Source File

SOURCE=..\doc\man.macros
# End Source File
# Begin Source File

SOURCE=..\doc\memory.n
# End Source File
# Begin Source File

SOURCE=..\doc\msgcat.n
# End Source File
# Begin Source File

SOURCE=..\doc\namespace.n
# End Source File
# Begin Source File

SOURCE=..\doc\Notifier.3
# End Source File
# Begin Source File

SOURCE=..\doc\Object.3
# End Source File
# Begin Source File

SOURCE=..\doc\ObjectType.3
# End Source File
# Begin Source File

SOURCE=..\doc\open.n
# End Source File
# Begin Source File

SOURCE=..\doc\OpenFileChnl.3
# End Source File
# Begin Source File

SOURCE=..\doc\OpenTcp.3
# End Source File
# Begin Source File

SOURCE=..\doc\package.n
# End Source File
# Begin Source File

SOURCE=..\doc\packagens.n
# End Source File
# Begin Source File

SOURCE=..\doc\Panic.3
# End Source File
# Begin Source File

SOURCE=..\doc\ParseCmd.3
# End Source File
# Begin Source File

SOURCE=..\doc\pid.n
# End Source File
# Begin Source File

SOURCE=..\doc\pkgMkIndex.n
# End Source File
# Begin Source File

SOURCE=..\doc\PkgRequire.3
# End Source File
# Begin Source File

SOURCE=..\doc\Preserve.3
# End Source File
# Begin Source File

SOURCE=..\doc\PrintDbl.3
# End Source File
# Begin Source File

SOURCE=..\doc\proc.n
# End Source File
# Begin Source File

SOURCE=..\doc\puts.n
# End Source File
# Begin Source File

SOURCE=..\doc\pwd.n
# End Source File
# Begin Source File

SOURCE=..\doc\re_syntax.n
# End Source File
# Begin Source File

SOURCE=..\doc\read.n
# End Source File
# Begin Source File

SOURCE=..\doc\RecEvalObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\RecordEval.3
# End Source File
# Begin Source File

SOURCE=..\doc\RegExp.3
# End Source File
# Begin Source File

SOURCE=..\doc\regexp.n
# End Source File
# Begin Source File

SOURCE=..\doc\registry.n
# End Source File
# Begin Source File

SOURCE=..\doc\regsub.n
# End Source File
# Begin Source File

SOURCE=..\doc\rename.n
# End Source File
# Begin Source File

SOURCE=..\doc\return.n
# End Source File
# Begin Source File

SOURCE=..\doc\safe.n
# End Source File
# Begin Source File

SOURCE=..\doc\SaveResult.3
# End Source File
# Begin Source File

SOURCE=..\doc\scan.n
# End Source File
# Begin Source File

SOURCE=..\doc\seek.n
# End Source File
# Begin Source File

SOURCE=..\doc\set.n
# End Source File
# Begin Source File

SOURCE=..\doc\SetErrno.3
# End Source File
# Begin Source File

SOURCE=..\doc\SetRecLmt.3
# End Source File
# Begin Source File

SOURCE=..\doc\SetResult.3
# End Source File
# Begin Source File

SOURCE=..\doc\SetVar.3
# End Source File
# Begin Source File

SOURCE=..\doc\Signal.3
# End Source File
# Begin Source File

SOURCE=..\doc\Sleep.3
# End Source File
# Begin Source File

SOURCE=..\doc\socket.n
# End Source File
# Begin Source File

SOURCE=..\doc\source.n
# End Source File
# Begin Source File

SOURCE=..\doc\SourceRCFile.3
# End Source File
# Begin Source File

SOURCE=..\doc\split.n
# End Source File
# Begin Source File

SOURCE=..\doc\SplitList.3
# End Source File
# Begin Source File

SOURCE=..\doc\SplitPath.3
# End Source File
# Begin Source File

SOURCE=..\doc\StaticPkg.3
# End Source File
# Begin Source File

SOURCE=..\doc\StdChannels.3
# End Source File
# Begin Source File

SOURCE=..\doc\string.n
# End Source File
# Begin Source File

SOURCE=..\doc\StringObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\StrMatch.3
# End Source File
# Begin Source File

SOURCE=..\doc\subst.n
# End Source File
# Begin Source File

SOURCE=..\doc\SubstObj.3
# End Source File
# Begin Source File

SOURCE=..\doc\switch.n
# End Source File
# Begin Source File

SOURCE=..\doc\Tcl.n
# End Source File
# Begin Source File

SOURCE=..\doc\Tcl_Main.3
# End Source File
# Begin Source File

SOURCE=..\doc\TCL_MEM_DEBUG.3
# End Source File
# Begin Source File

SOURCE=..\doc\tclsh.1
# End Source File
# Begin Source File

SOURCE=..\doc\tcltest.n
# End Source File
# Begin Source File

SOURCE=..\doc\tclvars.n
# End Source File
# Begin Source File

SOURCE=..\doc\tell.n
# End Source File
# Begin Source File

SOURCE=..\doc\Thread.3
# End Source File
# Begin Source File

SOURCE=..\doc\time.n
# End Source File
# Begin Source File

SOURCE=..\doc\ToUpper.3
# End Source File
# Begin Source File

SOURCE=..\doc\trace.n
# End Source File
# Begin Source File

SOURCE=..\doc\TraceVar.3
# End Source File
# Begin Source File

SOURCE=..\doc\Translate.3
# End Source File
# Begin Source File

SOURCE=..\doc\UniCharIsAlpha.3
# End Source File
# Begin Source File

SOURCE=..\doc\unknown.n
# End Source File
# Begin Source File

SOURCE=..\doc\unset.n
# End Source File
# Begin Source File

SOURCE=..\doc\update.n
# End Source File
# Begin Source File

SOURCE=..\doc\uplevel.n
# End Source File
# Begin Source File

SOURCE=..\doc\UpVar.3
# End Source File
# Begin Source File

SOURCE=..\doc\upvar.n
# End Source File
# Begin Source File

SOURCE=..\doc\Utf.3
# End Source File
# Begin Source File

SOURCE=..\doc\variable.n
# End Source File
# Begin Source File

SOURCE=..\doc\vwait.n
# End Source File
# Begin Source File

SOURCE=..\doc\while.n
# End Source File
# Begin Source File

SOURCE=..\doc\WrongNumArgs.3
# End Source File
# End Group
# Begin Group "generic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\generic\README
# End Source File
# Begin Source File

SOURCE=..\generic\regc_color.c
# End Source File
# Begin Source File

SOURCE=..\generic\regc_cvec.c
# End Source File
# Begin Source File

SOURCE=..\generic\regc_lex.c
# End Source File
# Begin Source File

SOURCE=..\generic\regc_locale.c
# End Source File
# Begin Source File

SOURCE=..\generic\regc_nfa.c
# End Source File
# Begin Source File

SOURCE=..\generic\regcomp.c
# End Source File
# Begin Source File

SOURCE=..\generic\regcustom.h
# End Source File
# Begin Source File

SOURCE=..\generic\rege_dfa.c
# End Source File
# Begin Source File

SOURCE=..\generic\regerror.c
# End Source File
# Begin Source File

SOURCE=..\generic\regerrs.h
# End Source File
# Begin Source File

SOURCE=..\generic\regex.h
# End Source File
# Begin Source File

SOURCE=..\generic\regexec.c
# End Source File
# Begin Source File

SOURCE=..\generic\regfree.c
# End Source File
# Begin Source File

SOURCE=..\generic\regfronts.c
# End Source File
# Begin Source File

SOURCE=..\generic\regguts.h
# End Source File
# Begin Source File

SOURCE=..\generic\tcl.decls
# End Source File
# Begin Source File

SOURCE=..\generic\tcl.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclAlloc.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclAsync.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclBasic.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclBinary.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclCkalloc.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclClock.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclCmdAH.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclCmdIL.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclCmdMZ.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclCompCmds.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclCompExpr.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclCompile.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclCompile.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclDate.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclDecls.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclEncoding.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclEnv.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclEvent.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclExecute.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclFCmd.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclFileName.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclGet.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclGetDate.y
# End Source File
# Begin Source File

SOURCE=..\generic\tclHash.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclHistory.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclIndexObj.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclInt.decls
# End Source File
# Begin Source File

SOURCE=..\generic\tclInt.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclIntDecls.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclInterp.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclIntPlatDecls.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclIO.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclIO.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclIOCmd.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclIOGT.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclIOSock.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclIOUtil.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclLink.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclListObj.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclLiteral.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclLoad.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclLoadNone.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclMain.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclNamesp.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclNotify.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclObj.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclPanic.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclParse.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclPipe.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclPkg.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclPlatDecls.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclPort.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclPosixStr.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclPreserve.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclProc.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclRegexp.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclRegexp.h
# End Source File
# Begin Source File

SOURCE=..\generic\tclResolve.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclResult.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclScan.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclStringObj.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclStubInit.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclStubLib.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclOOStubLib.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclTomMathStubLib.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclTest.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclTestObj.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclTestProcBodyObj.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclThread.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclThreadJoin.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclThreadTest.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclTimer.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclUniData.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclUtf.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclUtil.c
# End Source File
# Begin Source File

SOURCE=..\generic\tclVar.c
# End Source File
# End Group
# Begin Group "library"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\library\auto.tcl
# End Source File
# Begin Source File

SOURCE=..\library\history.tcl
# End Source File
# Begin Source File

SOURCE=..\library\init.tcl
# End Source File
# Begin Source File

SOURCE=..\library\ldAout.tcl
# End Source File
# Begin Source File

SOURCE=..\library\package.tcl
# End Source File
# Begin Source File

SOURCE=..\library\parray.tcl
# End Source File
# Begin Source File

SOURCE=..\library\safe.tcl
# End Source File
# Begin Source File

SOURCE=..\library\tclIndex
# End Source File
# Begin Source File

SOURCE=..\library\word.tcl
# End Source File
# End Group
# Begin Group "mac"

# PROP Default_Filter ""
# End Group
# Begin Group "tests"

# PROP Default_Filter ""
# End Group
# Begin Group "tools"

# PROP Default_Filter ""
# End Group
# Begin Group "unix"

# PROP Default_Filter ""
# End Group
# Begin Group "win"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\aclocal.m4
# End Source File
# Begin Source File

SOURCE=.\cat.c
# End Source File
# Begin Source File

SOURCE=.\configure
# End Source File
# Begin Source File

SOURCE=.\configure.in
# End Source File
# Begin Source File

SOURCE=.\Makefile.in
# End Source File
# Begin Source File

SOURCE=.\makefile.vc
# End Source File
# Begin Source File

SOURCE=.\mkd.bat
# End Source File
# Begin Source File

SOURCE=.\README
# End Source File
# Begin Source File

SOURCE=.\README.binary
# End Source File
# Begin Source File

SOURCE=.\rmd.bat
# End Source File
# Begin Source File

SOURCE=.\rules.vc
# End Source File
# Begin Source File

SOURCE=.\tcl.hpj.in
# End Source File
# Begin Source File

SOURCE=.\tcl.m4
# End Source File
# Begin Source File

SOURCE=.\tcl.rc
# End Source File
# Begin Source File

SOURCE=.\tclAppInit.c
# End Source File
# Begin Source File

SOURCE=.\tclConfig.sh.in
# End Source File
# Begin Source File

SOURCE=.\tclsh.ico
# End Source File
# Begin Source File

SOURCE=.\tclsh.rc
# End Source File
# Begin Source File

SOURCE=.\tclWin32Dll.c
# End Source File
# Begin Source File

SOURCE=.\tclWinChan.c
# End Source File
# Begin Source File

SOURCE=.\tclWinConsole.c
# End Source File
# Begin Source File

SOURCE=.\tclWinDde.c
# End Source File
# Begin Source File

SOURCE=.\tclWinError.c
# End Source File
# Begin Source File

SOURCE=.\tclWinFCmd.c
# End Source File
# Begin Source File

SOURCE=.\tclWinFile.c
# End Source File
# Begin Source File

SOURCE=.\tclWinInit.c
# End Source File
# Begin Source File

SOURCE=.\tclWinInt.h
# End Source File
# Begin Source File

SOURCE=.\tclWinLoad.c
# End Source File
# Begin Source File

SOURCE=.\tclWinNotify.c
# End Source File
# Begin Source File

SOURCE=.\tclWinPipe.c
# End Source File
# Begin Source File

SOURCE=.\tclWinPort.h
# End Source File
# Begin Source File

SOURCE=.\tclWinReg.c
# End Source File
# Begin Source File

SOURCE=.\tclWinSerial.c
# End Source File
# Begin Source File

SOURCE=.\tclWinSock.c
# End Source File
# Begin Source File

SOURCE=.\tclWinTest.c
# End Source File
# Begin Source File

SOURCE=.\tclWinThrd.c
# End Source File
# Begin Source File

SOURCE=.\tclWinTime.c
# End Source File
# End Group
# End Target
# End Project
