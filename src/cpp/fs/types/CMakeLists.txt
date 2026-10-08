# HACK: inherits from other file so don't need more than this?
cmake_minimum_required(VERSION 3.13)

set(DIR_SRC .)

set(LIB_NAME types)
file(GLOB FILES_CPP ${DIR_SRC}/*.cpp)
add_library(${LIB_NAME} ${FILES_CPP})

include_directories(${DIR_SRC})
