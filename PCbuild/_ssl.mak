
!IFDEF DEBUG
MODULE=_ssl_d.pyd
TEMP_DIR=x86-temp-debug/_ssl
CFLAGS=/Od /Zi /MDd /LDd /DDEBUG /D_DEBUG
SSL_LIB_DIR=$(SSL_DIR)/out32.dbg
!ELSE
MODULE=_ssl.pyd
TEMP_DIR=x86-temp-release/_ssl
CFLAGS=/Ox /MD /LD
SSL_LIB_DIR=$(SSL_DIR)/out32
!ENDIF

INCLUDES=-I ../Include -I ../PC -I $(SSL_DIR)/inc32
LIBS=gdi32.lib wsock32.lib /libpath:$(SSL_LIB_DIR) libeay32.lib ssleay32.lib

SOURCE=../Modules/_ssl.c $(SSL_LIB_DIR)/libeay32.lib $(SSL_LIB_DIR)/ssleay32.lib

$(MODULE): $(SOURCE) ../PC/*.h ../Include/*.h
    @if not exist "$(TEMP_DIR)/." mkdir "$(TEMP_DIR)"
    cl /nologo $(SOURCE) $(CFLAGS) /Fo$(TEMP_DIR)\$*.obj $(INCLUDES) /link /out:$(MODULE) $(LIBS)
