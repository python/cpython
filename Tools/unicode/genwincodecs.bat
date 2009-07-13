@rem Recreate some python charmap codecs from the Windows function
@rem MultiByteToWideChar.

@cd /d %~dp0
@mkdir build
@rem Arabic DOS code page
c:\python30\python genwincodec.py 720 > build/cp720.py
