cmake_minimum_required(VERSION 3.13)

project(c_lib_cert C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} src/cert.c include/cert.h)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE C)

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>/include)

find_package(OpenSSL REQUIRED COMPONENTS SSL Crypto)

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL OpenSSL::Crypto)
# In case above doesn't work... MAY CAUSE PROBLEMS ON WINDOWS, REMOVE THIS THEN
target_link_libraries(${PROJECT_NAME} ssl crypto)
