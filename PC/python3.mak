$(OutDir)python32.dll:	python3.def $(OutDir)python32stub.lib
	cl /LD /Fe$(OutDir)python3.dll python3dll.c python3.def $(OutDir)python32stub.lib

$(OutDir)python32stub.lib:	python32stub.def
	lib /def:python32stub.def /out:$(OutDir)python32stub.lib /MACHINE:$(MACHINE)

clean:
	del $(OutDir)python3.dll $(OutDir)python3.lib $(OutDir)python32stub.lib $(OutDir)python3.exp $(OutDir)python32stub.exp

rebuild: clean $(OutDir)python32.dll
