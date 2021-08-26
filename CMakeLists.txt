cmake_minimum_required(VERSION 3.16)

project(encryption_n_more)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
include(CMakeCargo)

enable_language(Rust)

include(CPM)
CPMAddPackage("gh:p-ranav/argparse@2.1")
CPMAddPackage(
	GITHUB_REPOSITORY zeromq/cppzmq
	VERSION 4.7.1
	OPTIONS "CPPZMQ_BUILD_TESTS NO"
	)
CPMAddPackage("gh:pybind/pybind11@2.7.1")

add_executable(client "src/client.cpp")
add_executable(server "src/server.cpp")

# Link argparse
target_link_libraries(client argparse)

# Prepare pybind and python files
file(COPY "python_lib/lib.py" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(client argparse pybind11::embed)
target_link_libraries(server argparse pybind11::embed)

add_subdirectory(rust-lib)

include_directories("include")
include_directories("rust-lib")

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
target_link_libraries(client Threads::Threads OpenSSL::SSL OpenSSL::Crypto cppzmq rust-lib ${CMAKE_DL_LIBS})
target_link_libraries(server Threads::Threads OpenSSL::SSL OpenSSL::Crypto cppzmq rust-lib ${CMAKE_DL_LIBS})

message("TIP: IF THERE IS LINKER ERROR FOR ssl AND crypto SYMBOLS, INCLUDE THESE ON LINUX, BUT \"\"\"REMOVE FROM WINDOWS\"\"\"...")
target_link_libraries(client ssl crypto)
target_link_libraries(server ssl crypto)

# Extras...
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

