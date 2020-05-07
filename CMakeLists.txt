cmake_minimum_required(VERSION 3.10)
project(vivacity_occupancy_example C)

set(CMAKE_C_STANDARD 99)

file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/proto)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto/vivacity)
file(COPY proto-buffet DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/proto)
file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/proto/proto-buffet ${CMAKE_CURRENT_SOURCE_DIR}/proto/vivacity)

file(GLOB_RECURSE PROTOBUFFET_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/proto proto/*.proto)

foreach(proto_file ${PROTOBUFFET_SRCS})
    message("${proto_file}")
    execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/generator/protoc --nanopb_out=${CMAKE_CURRENT_SOURCE_DIR}/proto --plugin=protoc-gen-nanopb=${CMAKE_CURRENT_SOURCE_DIR}/nanopb/generator/protoc-gen-nanopb -I=${CMAKE_CURRENT_SOURCE_DIR}/proto ${proto_file})
endforeach()

file(GLOB_RECURSE PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/proto/vivacity/*.pb.h)
file(GLOB_RECURSE PROTO_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/proto/vivacity/*.pb.c)
file(GLOB_RECURSE NANOPB_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/*.h)
file(GLOB_RECURSE NANOPB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/*.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/nanopb ${CMAKE_CURRENT_SOURCE_DIR}/proto /usr/include /usr/local/include /usr/local/include/google/protobuf)

set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
        PROPERTIES GENERATED TRUE)

message("${PROTO_SRCS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -g -O0")

add_executable(vivacity_occupancy_example src/main.c ${PROTO_SRCS} ${PROTO_HDRS} ${NANOPB_SRC} ${NANOPB_HDRS})