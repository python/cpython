# Microsoft Developer Studio Project File - Name="_ssl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=_ssl - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "_ssl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "_ssl.mak" CFG="_ssl - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "_ssl - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "_ssl - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "_ssl - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f _ssl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "_ssl.exe"
# PROP BASE Bsc_Name "_ssl.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "x86-temp-release\_ssl"
# PROP Cmd_Line "python build_ssl.py"
# PROP Rebuild_Opt "-a"
# PROP Target_File "_ssl.pyd"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "_ssl - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "x86-temp-debug\_ssl"
# PROP BASE Intermediate_Dir "x86-temp-debug\_ssl"
# PROP BASE Cmd_Line "NMAKE /f _ssl.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "_ssl_d.pyd"
# PROP BASE Bsc_Name "_ssl_d.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "x86-temp-debug\_ssl"
# PROP Cmd_Line "python_d -u build_ssl.py -d"
# PROP Rebuild_Opt "-a"
# PROP Target_File "_ssl_d.pyd"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "_ssl - Win32 Release"
# Name "_ssl - Win32 Debug"

!IF  "$(CFG)" == "_ssl - Win32 Release"

!ELSEIF  "$(CFG)" == "_ssl - Win32 Debug"

!ENDIF 

# Begin Source File

SOURCE=..\..\Modules\_ssl.c
# End Source File
# End Target
# End Project
