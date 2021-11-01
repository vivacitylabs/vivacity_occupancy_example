cmake_minimum_required(VERSION 3.10)
project(vivacity_occupancy_example C)

set(CMAKE_C_STANDARD 99)

file(REMOVE_RECURSE ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet/vivacity)
file(COPY vivacity-proto-buffet DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet)
file(RENAME ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet/vivacity-proto-buffet ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet/vivacity)
file(COPY vivacity-proto-buffet-options/core DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet/vivacity)

file(GLOB_RECURSE PROTOBUFFET_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet proto-buffet/*.proto)

foreach(proto_file ${PROTOBUFFET_SRCS})
    message("${proto_file}")
    execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/generator/protoc --nanopb_opt=-I${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet --nanopb_out=${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet --plugin=protoc-gen-nanopb=${CMAKE_CURRENT_SOURCE_DIR}/nanopb/generator/protoc-gen-nanopb -I=${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet ${proto_file})
endforeach()

file(GLOB_RECURSE PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet/vivacity/core/*.pb.h)
file(GLOB_RECURSE PROTO_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet/vivacity/core/*.pb.c)
file(GLOB_RECURSE NANOPB_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/*.h)
file(GLOB_RECURSE NANOPB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/*.c)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/nanopb ${CMAKE_CURRENT_SOURCE_DIR}/proto-buffet /usr/include /usr/local/include /usr/local/include/google/protobuf)

set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS}
        PROPERTIES GENERATED TRUE)

message("${PROTO_SRCS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -g -O0")

add_executable(vivacity_occupancy_example src/main.c ${PROTO_SRCS} ${PROTO_HDRS} ${NANOPB_SRC} ${NANOPB_HDRS})