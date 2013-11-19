$(OutDir)python3.dll:	python3.def $(OutDir)python33stub.lib
	cl /LD /Fe$(OutDir)python3.dll python3dll.c python3.def $(OutDir)python33stub.lib

$(OutDir)python33stub.lib:	python33stub.def
	lib /def:python33stub.def /out:$(OutDir)python33stub.lib /MACHINE:$(MACHINE)

clean:
	IF EXIST $(OutDir)python3.dll del $(OutDir)python3.dll
	IF EXIST $(OutDir)python3.lib del $(OutDir)python3.lib
	IF EXIST $(OutDir)python33stub.lib del $(OutDir)python33stub.lib
	IF EXIST $(OutDir)python3.exp del $(OutDir)python3.exp
	IF EXIST $(OutDir)python33stub.exp del $(OutDir)python33stub.exp

rebuild: clean $(OutDir)python3.dll
