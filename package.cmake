if(TARGET Python::library)
    return()
endif()

add_library(
    Python::library
    SHARED
    IMPORTED
)

set_target_properties(
    Python::library
    PROPERTIES
	    IMPORTED_LOCATION
            "${CMAKE_CURRENT_LIST_DIR}/bin/python37.dll"
        IMPORTED_LOCATION_DEBUG
            "${CMAKE_CURRENT_LIST_DIR}/bin/python37.dll"
        IMPORTED_IMPLIB
            "${CMAKE_CURRENT_LIST_DIR}/bin/python37.lib"
        IMPORTED_IMPLIB_DEBUG
            "${CMAKE_CURRENT_LIST_DIR}/bin/python37.lib"
        INTERFACE_INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_LIST_DIR}/include"
)

link_directories(${CMAKE_CURRENT_LIST_DIR}/bin)
set (PYTHON_BINDIR ${CMAKE_CURRENT_LIST_DIR}/bin/)
set (PYTHON_LIB ${CMAKE_CURRENT_LIST_DIR}/python37.zip)

install(
    FILES
		${PYTHON_LIB}
		${PYTHON_BINDIR}/python37.dll
		${PYTHON_BINDIR}/python3.dll
		${PYTHON_BINDIR}/pyexpat.pyd
		${PYTHON_BINDIR}/select.pyd
		${PYTHON_BINDIR}/unicodedata.pyd
		${PYTHON_BINDIR}/winsound.pyd
		${PYTHON_BINDIR}/xxlimited.pyd
		${PYTHON_BINDIR}/_asyncio.pyd
		${PYTHON_BINDIR}/_bz2.pyd
		${PYTHON_BINDIR}/_ctypes.pyd
		${PYTHON_BINDIR}/_elementtree.pyd
		${PYTHON_BINDIR}/_hashlib.pyd
		${PYTHON_BINDIR}/_lzma.pyd
		${PYTHON_BINDIR}/_msi.pyd
		${PYTHON_BINDIR}/_multiprocessing.pyd
		${PYTHON_BINDIR}/_overlapped.pyd
		${PYTHON_BINDIR}/_socket.pyd
		${PYTHON_BINDIR}/_sqlite3.pyd
		${PYTHON_BINDIR}/_ssl.pyd
		${PYTHON_BINDIR}/_tkinter.pyd
		${PYTHON_BINDIR}/pyshellext.dll
		${PYTHON_BINDIR}/sqlite3.dll
		${PYTHON_BINDIR}/tcl86t.dll
		${PYTHON_BINDIR}/tk86t.dll
    DESTINATION
        .
    COMPONENT
        CNPM_RUNTIME
)



