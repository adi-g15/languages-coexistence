# https://github.com/XiangpengHao/cxx-cmake-example/blob/master/rust_part/CMakeLists.txt
if ( CMAKE_BUILD_TYPE STREQUAL "Release" )
	set(CARGO_CMD cargo build --release)
	set(TARGET_DIR "release")
else()
	set(CARGO_CMD cargo build)
	set(TARGET_DIR "debug")
endif()

if(ENABLE_LTO)
	# You need a fairly new version of clang and lld, for eg. version 12
	set(RUST_FLAGS "-Clinker-plugin-lto" "-Clinker=clang" "-Clink-arg=-fuse-ld=lld")
endif()

set(RUST_LIB "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/librust_lib.a")

set(RUST_CXX "${CMAKE_CURRENT_BINARY_DIR}/rust_lib.cpp")
add_library(rust_lib STATIC ${RUST_CXX})

add_custom_command(
	OUTPUT ${RUST_CXX}
	COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} RUSTFLAGS="${RUST_FLAGS}" ${CARGO_CMD}
	COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/rust_lib/src/lib.rs.cc ${RUST_CXX}
	COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/cxxbridge/rust_lib/src/lib.rs.h ${CMAKE_CURRENT_BINARY_DIR}/rust_lib.h
	DEPENDS src/lib.rs
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	)

find_package(Threads REQUIRED)
target_link_libraries(rust_lib Threads::Threads ${CMAKE_DL_LIBS} ${RUST_LIB})
