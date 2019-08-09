if(
    NOT DEFINED ROOT
    OR NOT DEFINED ARCH
)
    message(
        FATAL_ERROR
        "Assert: ROOT = ${ROOT}; ARCH = ${ARCH}"
    )
endif()

set(
    BUILD
    1
)

if(DEFINED ENV{BUILD_NUMBER})
    set(
        BUILD
        $ENV{BUILD_NUMBER}
    )
endif()

set(
    TAG
    ""
)

if(DEFINED ENV{TAG})
    set(
        TAG
        "$ENV{TAG}"
    )
else()
    find_package(
        Git
    )

    if(Git_FOUND)
        execute_process(
            COMMAND
                ${GIT_EXECUTABLE} rev-parse --short HEAD
            OUTPUT_VARIABLE
                TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(
            TAG
            "_${TAG}"
        )
    endif()
endif()

if(WIN32)
    set(
        TOOLSET
        "msvc-14.0"
    )
elseif(UNIX)
    set(
        TOOLSET
        "gcc"
    )
endif()

set(
    PACKAGE_NAME
    "python-3.7.4-${ARCH}-${BUILD}${TAG}"
)

set(
    BUILD_DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/../PCBuild
)

set(
    CMAKE_INSTALL_PREFIX
    ${ROOT}/${PACKAGE_NAME}
)

set(
    MSBUILD_PARAMS
    "-v:n"
)

message (
	"cmd /C ${BUILD_DIRECTORY}\\build.bat -c Release -p ${BUILD_ARCH}"
)

if(WIN32)
    execute_process(
        COMMAND
            cmd /C "${BUILD_DIRECTORY}/build.bat -c Release -p ${BUILD_ARCH} ${MSBUILD_PARAMS}"
        WORKING_DIRECTORY
            "${BUILD_DIRECTORY}"
    )
    execute_process(
        COMMAND
            cmd /C "${BUILD_DIRECTORY}/build.bat -c Debug -p ${BUILD_ARCH} ${MSBUILD_PARAMS}"
        WORKING_DIRECTORY
            "${BUILD_DIRECTORY}"
    )
	
elseif(UNIX)

endif()

if(DEFINED ENV{TAG})
    set(
        TAG
        "$ENV{TAG}"
    )
endif()

execute_process(
	COMMAND
	    "${BUILD_DIRECTORY}/${FOLDER_ARCH}/python.exe" mkstd.py
	WORKING_DIRECTORY
	    "${CMAKE_CURRENT_LIST_DIR}/.."
)

execute_process(
	COMMAND
	    7z.exe a -r -tzip ../python37.zip *.pyc -x!__pycache__ -x!test -x!ensurepip -x!idlelib -x!venv -x!tests -x!tkinter -x!turtle* -aou
	WORKING_DIRECTORY
	    "${CMAKE_CURRENT_LIST_DIR}/../Lib"
)

file(
    COPY
        package.cmake
	python37.zip
    DESTINATION
        "${ROOT}/${PACKAGE_NAME}"
)

file(
    COPY
        Include/
		PC/pyconfig.h
    DESTINATION
        "${ROOT}/${PACKAGE_NAME}/include"
)

file(
    COPY
        ${BUILD_DIRECTORY}/${FOLDER_ARCH}/
    DESTINATION
        "${ROOT}/${PACKAGE_NAME}/bin"
)

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -E tar cf "${PACKAGE_NAME}.7z" --format=7zip -- "${PACKAGE_NAME}"
    WORKING_DIRECTORY
        "${ROOT}"
)
