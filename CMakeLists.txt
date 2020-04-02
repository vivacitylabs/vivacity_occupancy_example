cmake_minimum_required(VERSION 3.10)
project(vivacity_occupancy_example C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/cmake)
set(NANOPB_SRC_ROOT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/nanopb)
set(NANOPB_GENERATOR_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/generator)

find_package(Nanopb REQUIRED)
include_directories(${NANOPB_INCLUDE_DIRS})

file(GLOB PROTOBUFFET_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet/*.proto)

nanopb_generate_cpp(PROTO_SRCS PROTO_HDRS RELPATH ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet ${PROTOBUFFET_OPTIONS} ${PROTOBUFFET_SRCS})

include_directories(${CMAKE_CURRENT_BINARY_DIR} /usr/include /usr/local/include /usr/local/include/google/protobuf)

set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
        PROPERTIES GENERATED TRUE)


message("${PROTO_SRCS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -g -O0")

add_executable(vivacity_occupancy_example src/main.c ${PROTO_SRCS} ${PROTO_HDRS})