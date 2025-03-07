cmake_minimum_required(VERSION 3.4.1)
project(testbed)

set(PREFIX_DIR ${PYTHON_CROSS_DIR}/${CMAKE_LIBRARY_ARCHITECTURE}/prefix)
include_directories(${PREFIX_DIR}/include/python${PYTHON_VERSION})
link_directories(${PREFIX_DIR}/lib)
link_libraries(log python${PYTHON_VERSION})

add_library(main_activity SHARED main_activity.c)
