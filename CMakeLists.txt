cmake_minimum_required(VERSION 4.0)
project(NMEA0183 C)

set(CMAKE_C_STANDARD 11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_executable(NMEA0183 ${SRCS})
