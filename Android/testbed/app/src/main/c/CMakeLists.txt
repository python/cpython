cmake_minimum_required(VERSION 3.4.1)
project(testbed)

set(PYTHON_PREFIX_SUBDIR ${PYTHON_PREFIX_DIR}/${CMAKE_LIBRARY_ARCHITECTURE})
include_directories(${PYTHON_PREFIX_SUBDIR}/include/python${PYTHON_VERSION})
link_directories(${PYTHON_PREFIX_SUBDIR}/lib)
link_libraries(log python${PYTHON_VERSION})

add_library(main_activity SHARED main_activity.c)
