python_icon.exe:	py.res empty.obj
	link /out:python_icon.exe /machine:x86 /subsystem:windows py.res empty.obj

py.res:	py.ico pyc.ico pycon.ico icons.rc
	rc /fo py.res icons.rc

empty.obj:	empty.c
	cl /c empty.c

