$(OutDir)python3.dll:	python3.def $(OutDir)python33stub.lib
	cl /LD /Fe$(OutDir)python3.dll python3dll.c python3.def $(OutDir)python33stub.lib

$(OutDir)python33stub.lib:	python33stub.def
	lib /def:python33stub.def /out:$(OutDir)python33stub.lib /MACHINE:$(MACHINE)

clean:
	del $(OutDir)python3.dll $(OutDir)python3.lib $(OutDir)python33stub.lib $(OutDir)python3.exp $(OutDir)python33stub.exp

rebuild: clean $(OutDir)python3.dll
