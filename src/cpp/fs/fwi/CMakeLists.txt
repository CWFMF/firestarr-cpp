# HACK: inherits from other file so don't need more than this?
cmake_minimum_required(VERSION 3.13)

set(DIR_SRC .)

set(LIB_NAME fwi)
file(GLOB FILES_CPP ${DIR_SRC}/*.cpp)
set(FILE_UNSTABLE_CPP ${DIR_SRC}/unstable.cpp)
add_library(${LIB_NAME} ${FILES_CPP})

if(NOT WIN32)
    set_source_files_properties(${FILE_UNSTABLE_CPP} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")
endif()
include_directories(${DIR_SRC})
