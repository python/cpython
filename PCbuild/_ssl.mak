EXTRA_LIBS=

!IFDEF DEBUG
SUFFIX=_d.pyd
TEMP=x86-temp-debug/
CFLAGS=/Od /Zi /MDd /LDd /DDEBUG /D_DEBUG /DWIN32
SSL_LIB_DIR=$(SSL_DIR)/out32.dbg
!ELSE
SUFFIX=.pyd
TEMP=x86-temp-release/
CFLAGS=/Ox /MD /LD /DWIN32
SSL_LIB_DIR=$(SSL_DIR)/out32
!ENDIF

INCLUDES=-I ../Include -I ../PC -I $(SSL_DIR)/inc32

SSL_LIBS=gdi32.lib wsock32.lib user32.lib advapi32.lib /LIBPATH:$(SSL_LIB_DIR) libeay32.lib ssleay32.lib
SSL_SOURCE=../Modules/_ssl.c 

HASH_LIBS=gdi32.lib user32.lib advapi32.lib /libpath:$(SSL_LIB_DIR) libeay32.lib
HASH_SOURCE=../Modules/_hashopenssl.c 

all: _ssl$(SUFFIX) _hashlib$(SUFFIX)

# Split compile/link into two steps to better support VSExtComp
_ssl$(SUFFIX): $(SSL_SOURCE) $(SSL_LIB_DIR)/libeay32.lib $(SSL_LIB_DIR)/ssleay32.lib ../PC/*.h ../Include/*.h
	@if not exist "$(TEMP)/_ssl/." mkdir "$(TEMP)/_ssl"
	cl /nologo /c $(SSL_SOURCE) $(CFLAGS) /Fo$(TEMP)\_ssl\$*.obj $(INCLUDES)
	link /nologo @<<
             /dll /out:_ssl$(SUFFIX) $(TEMP)\_ssl\$*.obj $(SSL_LIBS) $(EXTRA_LIBS)
<<

_hashlib$(SUFFIX): $(HASH_SOURCE) $(SSL_LIB_DIR)/libeay32.lib ../PC/*.h ../Include/*.h
    @if not exist "$(TEMP)/_hashlib/." mkdir "$(TEMP)/_hashlib"
    cl /nologo /c $(HASH_SOURCE) $(CFLAGS) $(EXTRA_CFLAGS) /Fo$(TEMP)\_hashlib\$*.obj $(INCLUDES) 
    link /nologo @<<
	/dll /out:_hashlib$(SUFFIX) $(HASH_LIBS) $(EXTRA_LIBS) $(TEMP)\_hashlib\$*.obj
<<
