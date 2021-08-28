cmake_minimum_required(VERSION 3.16)

project(encryption_n_more)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Dropped CMakeCargo
cmake_policy(SET CMP0097 NEW)	# Do not update/init submodules when GIT_SUBMODULE = "" in ExternalProject_Add

include(CPM)
CPMAddPackage("gh:p-ranav/argparse@2.1")
CPMAddPackage(
	GITHUB_REPOSITORY zeromq/cppzmq
	VERSION 4.7.1
	GIT_SUBMODULES ""
	GIT_SUBMODULES_RECURSE OFF
	OPTIONS "CPPZMQ_BUILD_TESTS OFF"
	)
CPMAddPackage("gh:pybind/pybind11@2.7.1")
CPMAddPackage(
	GITHUB_REPOSITORY nlohmann/json
	VERSION 3.10.1
	GIT_SUBMODULES_RECURSE OFF
	GIT_SUBMODULES ""
	OPTIONS "JSON_BuildTests OFF"
	)
CPMAddPackage(
	GITHUB_REPOSITORY adi-g15/cppcodec
	VERSION 0.3
	GIT_SUBMODULES ""
	)

add_executable(client "src/client.cpp")
add_executable(server "src/server.cpp")

# Link argparse
target_link_libraries(client argparse cppcodec)

# Prepare pybind and python files
file(COPY "python_lib/lib.py" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(client argparse pybind11::embed nlohmann_json)
target_link_libraries(server argparse pybind11::embed nlohmann_json)

# https://github.com/XiangpengHao/cxx-cmake-example
option(ENABLE_LTO "Enable cross language linking time optimization" ON)	# When available, use LTO, it almost nulls the 'huge' overhead of FFI between these (see https://github.com/XiangpengHao/cxx-cmake-example for metrics)

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

add_subdirectory(rust-cxx-interop)

include_directories(${CMAKE_BINARY_DIR}/rust-cxx-interop)
include_directories(include)

target_link_libraries(client rust_cxx_interop)
target_link_libraries(server rust_cxx_interop)

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

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/payload.proto)
target_sources(client PUBLIC ${PROTO_SRCS} ${PROTO_HDRS})
target_sources(server PUBLIC ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(client ${PROTOBUF_LIBRARIES})
target_link_libraries(server ${PROTOBUF_LIBRARIES})

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)
find_package(Threads REQUIRED)

# dlsym symbol not found error, remove this if it fails to find dl (libdl is provided by the GNU C library glibc)
target_link_libraries(client Threads::Threads OpenSSL::SSL OpenSSL::Crypto cppzmq ${CMAKE_DL_LIBS})
target_link_libraries(server Threads::Threads OpenSSL::SSL OpenSSL::Crypto cppzmq ${CMAKE_DL_LIBS})

message("TIP: IF THERE IS LINKER ERROR FOR ssl AND crypto SYMBOLS, INCLUDE THESE ON LINUX, BUT \"\"\"REMOVE FROM WINDOWS\"\"\"...")
target_link_libraries(client ssl crypto)
target_link_libraries(server ssl crypto)

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

