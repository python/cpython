# Microsoft Developer Studio Project File - Name="thread" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=thread - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "thread_win.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "thread_win.mak" CFG="thread - Win32 Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "thread - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "thread - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "thread - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f thread.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "thread.exe"
# PROP BASE Bsc_Name "thread.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "nmake -nologo -f makefile.vc TCLDIR=E:\tcl MSVCDIR=IDE"
# PROP Rebuild_Opt "-a"
# PROP Target_File "Release\thread27.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "thread - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f thread.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "thread.exe"
# PROP BASE Bsc_Name "thread.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "nmake -nologo -f makefile.vc OPTS=symbols TCLDIR=E:\tcl MSVCDIR=IDE"
# PROP Rebuild_Opt "-a"
# PROP Target_File "Debug\thread27d.dll"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF

# Begin Target

# Name "thread - Win32 Release"
# Name "thread - Win32 Debug"

!IF  "$(CFG)" == "thread - Win32 Release"

!ELSEIF  "$(CFG)" == "thread - Win32 Debug"

!ENDIF

ROOT=..

# Begin Group "generic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=$(ROOT)\generic\threadNs.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\psGdbm.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\psGdbm.h
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\tclThread.h
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\tclThreadInt.h
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\tclXkeylist.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\tclXkeylist.h
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadCmd.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadPoolCmd.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadSpCmd.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadSvCmd.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadSvCmd.h
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadSvKeylistCmd.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadSvKeylistCmd.h
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadSvListCmd.c
# End Source File
# Begin Source File

SOURCE=$(ROOT)\generic\threadSvListCmd.h
# End Source File
# End Group
# Begin Group "doc"

# PROP Default_Filter ""
# Begin Group "html"

# PROP Default_Filter ""
# Begin Source File

SOURCE=$(ROOT)\doc\html\thread.html
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\html\tpool.html
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\html\tsv.html
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\html\ttrace.html
# End Source File
# End Group
# Begin Group "man"

# PROP Default_Filter ""
# Begin Source File

SOURCE=$(ROOT)\doc\man\thread.n
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\man\tpool.n
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\man\tsv.n
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\man\ttrace.n
# End Source File
# End Group
# Begin Source File

SOURCE=$(ROOT)\doc\format.tcl
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\man.macros
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\thread.man
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\tpool.man
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\tsv.man
# End Source File
# Begin Source File

SOURCE=$(ROOT)\doc\ttrace.man
# End Source File
# End Group
# Begin Group "win"

# PROP Default_Filter ""
# Begin Group "vc"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\makefile.vc
# End Source File
# Begin Source File

SOURCE=.\nmakehlp.c
# End Source File
# Begin Source File

SOURCE=.\pkg.vc
# End Source File
# Begin Source File

SOURCE=.\README.vc.txt
# End Source File
# Begin Source File

SOURCE=.\rules.vc
# End Source File
# End Group
# Begin Source File

SOURCE=$(ROOT)\win\README.txt
# End Source File
# Begin Source File

SOURCE=$(ROOT)\win\thread.rc
# End Source File
# Begin Source File

SOURCE=$(ROOT)\win\threadWin.c
# End Source File
# End Group
# Begin Source File

SOURCE=$(ROOT)\ChangeLog
# End Source File
# Begin Source File

SOURCE=$(ROOT)\license.terms
# End Source File
# Begin Source File

SOURCE=$(ROOT)\README
# End Source File
# End Target
# End Project
