cmake_minimum_required(VERSION 3.14)
add_executable( Transpose Transpose.cpp )
target_link_libraries( Transpose ${OPENCL_LIBRARIES} )

configure_file(Transpose.cl ${CMAKE_CURRENT_BINARY_DIR}/Transpose.cl COPYONLY)
