# HACK: inherits from other file so don't need more than this?
cmake_minimum_required(VERSION 3.13)

set(DIR_SRC .)

file(GLOB FILES_CPP ${DIR_SRC}/*.cpp)
set(FILE_UNSTABLE_CPP ${DIR_SRC}/unstable.cpp)
add_library(fs ${FILES_CPP})

if(NOT WIN32)
    set_source_files_properties(${FILE_UNSTABLE_CPP} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

find_package(GeoTIFF CONFIG REQUIRED)
find_package(TIFF REQUIRED)
find_package(PROJ CONFIG REQUIRED)

include_directories(${DIR_SRC})
include_directories(${GEOTIFF_INCLUDE_DIR})
link_libraries(${GEOTIFF_LIBRARIES})
link_libraries(TIFF::TIFF)
link_libraries(PROJ::proj)
