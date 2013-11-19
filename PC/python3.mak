$(OutDir)python3.dll:	python3.def $(OutDir)python34stub.lib
	cl /LD /Fe$(OutDir)python3.dll python3dll.c python3.def $(OutDir)python34stub.lib

$(OutDir)python34stub.lib:	python34stub.def
	lib /def:python34stub.def /out:$(OutDir)python34stub.lib /MACHINE:$(MACHINE)

clean:
	IF EXIST $(OutDir)python3.dll del $(OutDir)python3.dll
	IF EXIST $(OutDir)python3.lib del $(OutDir)python3.lib
	IF EXIST $(OutDir)python34stub.lib del $(OutDir)python34stub.lib
	IF EXIST $(OutDir)python3.exp del $(OutDir)python3.exp
	IF EXIST $(OutDir)python34stub.exp del $(OutDir)python34stub.exp

rebuild: clean $(OutDir)python3.dll
