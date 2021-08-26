set(CMAKE_C_COMPILER "clang" CACHE INTERNAL STRING)
set(CMAKE_CXX_COMPILER "clang++" CACHE INTERNAL STRING)
set(CMAKE_RANLIB "llvm-ranlib" CACHE INTERNAL STRING)
set(CMAKE_AR "llvm-ar" CACHE INTERNAL STRING)
set(CMAKE_AS "llvm-as" CACHE INTERNAL STRING)
set(CMAKE_LINKER "ld.lld" CACHE INTERNAL STRING)

execute_process(
    COMMAND bash -c "dirname `whereis ${CMAKE_LINKER} | tr -s ' ' '\n' | grep ${CMAKE_LINKER}`"
    OUTPUT_VARIABLE cmake_linker_dir
)
string(REGEX REPLACE "\n$" "" cmake_linker_dir "${cmake_linker_dir}")
set(cmake_linker_with_dir "${cmake_linker_dir}/${CMAKE_LINKER}" CACHE INTERNAL STRING)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

# Clang as linker
# --no-pie disable position independent executable, which is required when building
# statically linked executables.
set(CMAKE_CXX_LINK_EXECUTABLE "clang++ --target=${triple} -Wl,--no-pie --sysroot=${CMAKE_SYSROOT} ${CMAKE_CXX_FLAGS} -fuse-ld=${cmake_linker_with_dir} <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> <OBJECTS> -o  <TARGET> ")
set(CMAKE_CXX_CREATE_SHARED_LIBRARY "clang++ -Wl, --target=${triple} --sysroot=${CMAKE_SYSROOT} ${CMAKE_CXX_FLAGS} -fuse-ld=${cmake_linker_with_dir} -shared <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> <OBJECTS> -o  <TARGET> ")
#
# Do not use CMAKE_CXX_CREATE_STATIC_LIBRARY -- it is created automatically
# by cmake using ar and ranlib
#
#set(CMAKE_CXX_CREATE_STATIC_LIBRARY "clang++ -Wl,--no-pie,--no-export-dynamic,-v -v --target=${triple} --sysroot=${CMAKE_SYSROOT} ${CMAKE_CXX_FLAGS} -fuse-ld=ld.lld <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> <OBJECTS> -o  <TARGET> ")

## Linker as linker
unset(cmake_linker_with_dir)
unset(cmake_linker_dir)

