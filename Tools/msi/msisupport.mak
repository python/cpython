# /OPT: REF and ICF are added by VS.NET by default
# NOWIN98 saves 7k of executable size, at the expense of some
# slowdown on Win98
msisupport.dll:	msisupport.obj
	link.exe /OUT:msisupport.dll /INCREMENTAL:NO /NOLOGO /DLL /MACHINE:X86 /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /OPT:NOWIN98 msisupport.obj msi.lib kernel32.lib

# We request a static CRT, so that there will be no CRT dependencies
# for the target system. We cannot do without a CRT, since it provides
# the DLL entry point.
msisupport.obj:	msisupport.c
	cl /O2 /D WIN32 /D NDEBUG /D _WINDOWS /MT /W3 /c msisupport.c

