cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(WIN32)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")
endif()

project(tbd)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++23")
if (WIN32)
    # don't set -Wextra
    set(CMAKE_CXX_FLAGS "-Wall")
else()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    # set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wfatal-errors")
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
    add_compile_options(
        "$<$<CONFIG:RELEASE>:-O3;>"
        "$<$<CONFIG:DEBUG>:-O0;-fno-omit-frame-pointer;-g;>"
)
endif()

add_definitions(-D_POSIX_C_SOURCE=200809L)

# add version to version.cpp from environment variable so C++ can compile it in
set(VERSION $ENV{VERSION})
if ("${VERSION}" STREQUAL "")
    message(FATAL_ERROR "VERSION IS NOT SET")
    exit()
endif()
message("VERSION = ${VERSION}")
execute_process(COMMAND date -u --rfc-3339=seconds
                OUTPUT_VARIABLE COMPILE_DATE
                ERROR_QUIET)
string(REGEX REPLACE "(.*) (.*)\\+00:00\n$" "\\1T\\2Z" COMPILE_DATE "${COMPILE_DATE}")
message("COMPILE_DATE = ${COMPILE_DATE}")
set(VERSION_CODE "const char* VERSION = \"${VERSION}\";\nconst char* COMPILE_DATE = \"${COMPILE_DATE}\";\n")
if(EXISTS src/cpp/version.cpp)
    file(READ src/cpp/version.cpp VERSION_CODE_OLD)
else()
    set(VERSION_CODE_OLD "")
endif()

if (NOT "${VERSION_CODE}" STREQUAL "${VERSION_CODE_OLD}")
    file(WRITE src/cpp/version.cpp "${VERSION_CODE}")
endif()

file(GLOB SOURCES src/cpp/*.cpp)
set_source_files_properties(src/cpp/unstable.cpp PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")


add_executable(${PROJECT_NAME} ${SOURCES})
# ./usr/lib/x86_64-linux-gnu/cmake/TBB/TBBConfig.cmake

# list(APPEND CMAKE_MODULE_PATH "deps/tbb/cmake/")
list(APPEND CMAKE_MODULE_PATH "/usr/lib/x86_64-linux-gnu/cmake")

# find_package(Threads REQUIRED)
# target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff Threads::Threads)

find_package(TBB REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff TBB::tbb)


# find_package(TBB REQUIRED)
# # target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff Threads::Threads TBB:tbb)
# target_link_libraries(${PROJECT_NAME} PUBLIC geotiff tiff Threads::Threads tbb)
# # target_link_libraries(${PROJECT_NAME} PUBLIC pthread tbb geotiff tiff)

add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ../)
