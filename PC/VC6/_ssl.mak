
!IFDEF DEBUG
MODULE=_ssl_d.pyd
TEMP_DIR=x86-temp-debug/_ssl
CFLAGS=/Od /Zi /MDd /LDd /DDEBUG /D_DEBUG /DWIN32
LFLAGS=/nodefaultlib:"msvcrt"
!ELSE
MODULE=_ssl.pyd
TEMP_DIR=x86-temp-release/_ssl
CFLAGS=/Ox /MD /LD /DWIN32
LFLAGS=
!ENDIF

INCLUDES=-I ../../Include -I .. -I $(SSL_DIR)/inc32
SSL_LIB_DIR=$(SSL_DIR)/out32
LIBS=gdi32.lib wsock32.lib user32.lib advapi32.lib /libpath:$(SSL_LIB_DIR) libeay32.lib ssleay32.lib

SOURCE=../../Modules/_ssl.c $(SSL_LIB_DIR)/libeay32.lib $(SSL_LIB_DIR)/ssleay32.lib

$(MODULE): $(SOURCE) ../*.h ../../Include/*.h
    @if not exist "$(TEMP_DIR)/." mkdir "$(TEMP_DIR)"
    cl /nologo $(SOURCE) $(CFLAGS) /Fo$(TEMP_DIR)\$*.obj $(INCLUDES) /link /out:$(MODULE) $(LIBS) $(LFLAGS)
