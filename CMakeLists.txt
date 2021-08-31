cmake_minimum_required(VERSION 3.16)

project(encryption_n_more)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include(CPM)

# Dropped CMakeCargo
cmake_policy(SET CMP0097 NEW)	# Do not update/init submodules when GIT_SUBMODULE = "" in ExternalProject_Add

CPMAddPackage("gh:p-ranav/argparse@2.1")
CPMAddPackage(
	GITHUB_REPOSITORY zeromq/cppzmq
	VERSION 4.7.1
	GIT_SUBMODULES ""
	OPTIONS "CPPZMQ_BUILD_TESTS OFF"
	)
CPMAddPackage("gh:pybind/pybind11@2.7.1")
CPMAddPackage("gh:fmtlib/fmt#8.0.1")
CPMAddPackage("gh:adi-g15/cppcodec@0.3")

add_executable(client "src/client.cpp")
add_executable(server "src/server.cpp")

# Link argparse
target_link_libraries(client argparse cppcodec)

# Prepare pybind and python files
file(COPY "python_lib/lib.py" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(client argparse pybind11::embed fmt)
target_link_libraries(server pybind11::embed)

# https://github.com/XiangpengHao/cxx-cmake-example
option(ENABLE_LTO "Enable cross language linking time optimization" OFF)	# When available, use LTO, it almost nulls the 'huge' overhead of FFI between these (see https://github.com/XiangpengHao/cxx-cmake-example for metrics)

if(ENABLE_LTO)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT supported OUTPUT error)
	if(supported)
		message(STATUS "IPO / LTO enabled")
		set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
		add_link_options(-fuse-ld=lld)
	else()
		message(STATUS "IPO / LTO not supported: <${error}>")
	endif()
endif()

add_subdirectory(c_lib)
add_subdirectory(rust_lib)

message("Cmake build type is ${CMAKE_BUILD_TYPE}")
include_directories(${CMAKE_BINARY_DIR}/rust_lib)
include_directories(include)

target_link_libraries(client c_lib_cert rust_lib)
target_link_libraries(server c_lib_cert rust_lib)

# These additional dependencies need to be linked for the C library or something... or atleast this CMake-Rust to work
if (WIN32)
	target_link_libraries(client ws2_32 userenv advapi32)
	target_link_libraries(server ws2_32 userenv advapi32)
elseif (APPLE)
	target_link_libraries(client m c System resolv)
	target_link_libraries(server m c System resolv)
endif()

find_package(Protobuf REQUIRED)
#include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/payload.proto proto/grpc.proto)
target_sources(client PUBLIC ${PROTO_SRCS} ${PROTO_HDRS})
target_sources(server PUBLIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(client ${PROTOBUF_LIBRARIES})
target_link_libraries(server ${PROTOBUF_LIBRARIES})

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
find_package(Threads REQUIRED)

# dlsym symbol not found error, remove this if it fails to find dl (libdl is provided by the GNU C library glibc)
target_link_libraries(client Threads::Threads OpenSSL::Crypto cppzmq ${CMAKE_DL_LIBS})
target_link_libraries(server Threads::Threads OpenSSL::Crypto cppzmq ${CMAKE_DL_LIBS})

message("TIP: IF THERE IS LINKER ERROR FOR ssl AND crypto SYMBOLS, INCLUDE THESE ON LINUX, BUT \"\"\"REMOVE FROM WINDOWS\"\"\"...")
target_link_libraries(client crypto)
target_link_libraries(server crypto)

# Extras... https://stackanswers.net/questions/best-way-to-portably-set-compiler-options-like-wall-and-pedantic-in-cmake
include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
    if(flag_already_set EQUAL -1)
        check_cxx_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

#enable_cxx_compiler_flag_if_supported("-Wall")
#enable_cxx_compiler_flag_if_supported("-Wextra")
enable_cxx_compiler_flag_if_supported("-pedantic")

