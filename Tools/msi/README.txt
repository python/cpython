Packaging Python as a Microsoft Installer Package (MSI)
=======================================================

Using this library, Python can be packaged as a MS-Windows
MSI file. To generate an installer package, you need
a build tree. By default, the build tree root directory
is assumed to be in "../..". This location can be changed
by adding a file config.py; see the beginning of msi.py
for additional customization options.

The packaging process assumes that binaries have been 
generated according to the instructions in PCBuild/README.txt,
and that you have either Visual Studio or the Platform SDK
installed. In addition, you need the Python COM extensions,
either from PythonWin, or from ActivePython.

To invoke the script, open a cmd.exe window which has 
cabarc.exe in its PATH (e.g. "Visual Studio .NET 2003
Command Prompt"). Then invoke

<path-to-python.exe> msi.py

If everything succeeds, pythonX.Y.Z.msi is generated
in the current directory.

