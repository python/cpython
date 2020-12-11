# Most of it was copied from tcl's rule.vc

TOOLS32 = $(VCINSTALLDIR)

!if $(DEBUG)
BUILDDIRTOP = Debug
NODEBUG = 0
TCL_DBGX = g
!else
BUILDDIRTOP = Release
NODEBUG = 1
!endif

VCVER=0
!if ![echo VCVERSION=_MSC_VER > vercl.x] \
    && ![cl -nologo -TC -P vercl.x $(ERRNULL)]
!include vercl.i
!if $(VCVERSION) >= 1600
VCVER=10
!elseif $(VCVERSION) >= 1500
VCVER=9
!elseif $(VCVERSION) >= 1400
VCVER=8
!elseif $(VCVERSION) >= 1300
VCVER=7
!elseif $(VCVERSION) >= 1200
VCVER=6
!endif
!endif

!if "$(MACHINE)" != "IX86"
BUILDDIRTOP =$(BUILDDIRTOP)_$(MACHINE)
!endif
!if $(VCVER) > 6
BUILDDIRTOP =$(BUILDDIRTOP)_VC$(VCVER)
!endif

TCL_MAJOR=8
TCL_MINOR=6
TCL_PATCH=6

# Since MSVC8 we must deal with manifest resources.
_VC_MANIFEST_EMBED_EXE=
_VC_MANIFEST_EMBED_DLL=
!if $(VCVERSION) >= 1400
_VC_MANIFEST_EMBED_EXE=if exist $@.manifest mt -nologo -manifest $@.manifest -outputresource:$@;1
_VC_MANIFEST_EMBED_DLL=if exist $@.manifest mt -nologo -manifest $@.manifest -outputresource:$@;2
!endif

!include makefile.vc
