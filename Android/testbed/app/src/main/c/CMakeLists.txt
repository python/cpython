cmake_minimum_required(VERSION 3.4.1)
project(testbed)

# Resolve variables from the command line.
string(
    REPLACE {{triplet}} ${CMAKE_LIBRARY_ARCHITECTURE}
    PYTHON_PREFIX_DIR ${PYTHON_PREFIX_DIR}
)

include_directories(${PYTHON_PREFIX_DIR}/include/python${PYTHON_VERSION})
link_directories(${PYTHON_PREFIX_DIR}/lib)
link_libraries(log python${PYTHON_VERSION})

add_library(main_activity SHARED main_activity.c)
