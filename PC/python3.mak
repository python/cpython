$(OutDir)python3.dll:	python3.def $(OutDir)python35stub.lib
	cl /LD /Fe$(OutDir)python3.dll python3dll.c python3.def $(OutDir)python35stub.lib

$(OutDir)python35stub.lib:	python35stub.def
	lib /def:python35stub.def /out:$(OutDir)python35stub.lib /MACHINE:$(MACHINE)

clean:
	IF EXIST $(OutDir)python3.dll del $(OutDir)python3.dll
	IF EXIST $(OutDir)python3.lib del $(OutDir)python3.lib
	IF EXIST $(OutDir)python35stub.lib del $(OutDir)python35stub.lib
	IF EXIST $(OutDir)python3.exp del $(OutDir)python3.exp
	IF EXIST $(OutDir)python35stub.exp del $(OutDir)python35stub.exp

rebuild: clean $(OutDir)python3.dll
