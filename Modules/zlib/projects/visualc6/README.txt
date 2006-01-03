Microsoft Developer Studio Project Files, Format Version 6.00 for zlib.

Copyright (C) 2000-2004 Simon-Pierre Cadieux.
Copyright (C) 2004 Cosmin Truta.
For conditions of distribution and use, see copyright notice in zlib.h.


This project builds the zlib binaries as follows:

* Win32_DLL_Release\zlib1.dll       DLL build
* Win32_DLL_Debug\zlib1d.dll        DLL build (debug version)
* Win32_DLL_ASM_Release\zlib1.dll   DLL build using ASM code
* Win32_DLL_ASM_Debug\zlib1d.dll    DLL build using ASM code (debug version)
* Win32_LIB_Release\zlib.lib        static build
* Win32_LIB_Debug\zlibd.lib         static build (debug version)
* Win32_LIB_ASM_Release\zlib.lib    static build using ASM code
* Win32_LIB_ASM_Debug\zlibd.lib     static build using ASM code (debug version)


For more information regarding the DLL builds, please see the DLL FAQ
in ..\..\win32\DLL_FAQ.txt.


To build and test:

1) On the main menu, select "File | Open Workspace".
   Open "zlib.dsw".

2) Select "Build | Set Active Configuration".
   Choose the configuration you wish to build.

3) Select "Build | Clean".

4) Select "Build | Build ... (F7)".  Ignore warning messages about
   not being able to find certain include files (e.g. alloc.h).

5) If you built one of the sample programs (example or minigzip),
   select "Build | Execute ... (Ctrl+F5)".


To use:

1) Select "Project | Settings (Alt+F7)".
   Make note of the configuration names used in your project.
   Usually, these names are "Win32 Release" and "Win32 Debug".

2) In the Workspace window, select the "FileView" tab.
   Right-click on the root item "Workspace '...'".
   Select "Insert Project into Workspace".
   Switch on the checkbox "Dependency of:", and select the name
   of your project.  Open "zlib.dsp".

3) Select "Build | Configurations".
   For each configuration of your project:
   3.1) Choose the zlib configuration you wish to use.
   3.2) Click on "Add".
   3.3) Set the new zlib configuration name to the name used by
        the configuration from the current iteration.

4) Select "Build | Set Active Configuration".
   Choose the configuration you wish to build.

5) Select "Build | Build ... (F7)".

6) If you built an executable program, select
   "Build | Execute ... (Ctrl+F5)".


Note:

To build the ASM-enabled code, you need Microsoft Assembler
(ML.EXE).  You can get it by downloading and installing the
latest Processor Pack for Visual C++ 6.0.
