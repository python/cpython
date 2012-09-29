$(OutDir)python3.dll:	python3.def $(OutDir)python34stub.lib
	cl /LD /Fe$(OutDir)python3.dll python3dll.c python3.def $(OutDir)python34stub.lib

$(OutDir)python34stub.lib:	python34stub.def
	lib /def:python34stub.def /out:$(OutDir)python34stub.lib /MACHINE:$(MACHINE)

clean:
	del $(OutDir)python3.dll $(OutDir)python3.lib $(OutDir)python34stub.lib $(OutDir)python3.exp $(OutDir)python34stub.exp

rebuild: clean $(OutDir)python3.dll
