cmake_minimum_required(VERSION 3.0)
project(dumbot)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(BINDIR "${CMAKE_INSTALL_PREFIX}/bin")


include_directories(/usr/local/include
                    /usr/local/include/jsoncpp)
include_directories(/usr/local/include/lua53)
link_directories(/usr/local/lib)
link_directories(/usr/local/lib64)
include_directories(lib/winterwind/include)

add_subdirectory(lib)
add_subdirectory(src)
