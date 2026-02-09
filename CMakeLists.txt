cmake_minimum_required(VERSION 3.31.6)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(DIR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp)
set(DIR_SRC_COMMON ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/fs)

if(NOT DEFINED CPU_TUNE_ONLY)
  set(CPU_TUNE_ONLY TRUE)
endif()
if(NOT DEFINED CPU_TARGET)
  set(CPU_TARGET "generic")
endif()

if(NOT VCPKG_ROOT)
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
endif()
if(NOT VCPKG_ROOT)
  set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
endif()
# if(NOT VCPKG_DEFAULT_TRIPLET)
#   set(VCPKG_DEFAULT_TRIPLET $ENV{VCPKG_DEFAULT_TRIPLET})
# endif()
message("VCPKG_TARGET_TRIPLET = ${VCPKG_TARGET_TRIPLET}")
if(NOT VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET})
endif()
message("VCPKG_TARGET_TRIPLET = ${VCPKG_TARGET_TRIPLET}")
# if(NOT VCPKG_TARGET_TRIPLET)
#   set(VCPKG_TARGET_TRIPLET ${VCPKG_DEFAULT_TRIPLET})
# endif()
message("VCPKG_DEFAULT_TRIPLET = ${VCPKG_DEFAULT_TRIPLET}")
if (NOT APPLE)
  set(VCPKG_CRT_LINKAGE static)
  set(VCPKG_LIBRARY_LINKAGE static)
endif()
set(VCPKG_BUILD_TYPE release)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

if(WIN32)
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

#   # Enable Hot Reload for MSVC compilers if supported.
#   if(POLICY CMP0141)
#     cmake_policy(SET CMP0141 NEW)
#     string(CONCAT MSVC_OPTIONS "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>"
#            ",$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,"
#            "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
#     set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
#         ${MSVC_OPTIONS})
#   endif()
endif()

message("CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

project(firestarr)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

include(cmake/TuneArchCPU.cmake)

if (WIN32)
  # HACK: use same settings for RelWitDebInfo since can't figure out how to work with vcpkg
  add_compile_options(
    "$<$<CONFIG:Release,RelWithDebInfo>:/O2>"
    "$<$<CONFIG:MinSizeRel>:/Os>"
    "$<$<CONFIG:Debug>:/Od>")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # HACK: avoid error for now
  # fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
  add_compile_options(
    "$<$<CONFIG:Debug>:/bigobj>")
else()
  if(CMAKE_VERBOSE_MAKEFILE)
    add_compile_options(-fopt-info-vec-all)
  endif()
  add_compile_options(
    "$<$<CONFIG:Release,RelWithDebInfo>:-O3;>"
    "$<$<CONFIG:MinSizeRel>:-Os;>"
    "$<$<CONFIG:Debug>:-O0;>")
  # FIX: disable to avoid for now:
  #   clang++: warning: -ltcmalloc: 'linker' input unused [-Wunused-command-line-argument]
  # add_compile_options("$<$<CONFIG:Debug,RelWithDebInfo>:-ltcmalloc;>")
  add_compile_options(
    "$<$<CONFIG:Debug,RelWithDebInfo>:-g;-fno-omit-frame-pointer;>")
  include(cmake/ASAN.cmake)
endif()
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

include(cmake/Version.cmake)

set(FILE_UNSTABLE_CPP ${DIR_SRC_COMMON}/unstable.cpp)
set_source_files_properties(${FILE_UNSTABLE_CPP} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")

if(NOT ASAN_ARGS)
  include(cmake/TidyFormat.cmake)
endif()

# set compiler warnings
include(cmake/CompilerWarnings.cmake)
add_compile_options(${PROJECT_WARNINGS_CXX})

find_package(GeoTIFF CONFIG REQUIRED)
find_package(TIFF REQUIRED)
# FIX: wrap anything new that might be breaking windows workflow
if(LINUX)
  # # PROJ needs this unless we can figure out how to turn net lookup off
  # find_package(CURL CONFIG REQUIRED)
  find_package(SQLite3 REQUIRED)
endif()
find_package(PROJ CONFIG REQUIRED)
# link_libraries(geotiff tiff PROJ::proj)

if (WIN32)
  add_compile_options(/permissive- /EHsc)
  add_link_options(/NODEFAULTLIB:MSVCRT)
# else()
#   list(APPEND CMAKE_MODULE_PATH "/usr/lib/x86_64-linux-gnu/cmake")
endif()

enable_testing()

add_subdirectory(${DIR_SRC_COMMON})

foreach(bin ${PROJECT_NAME} test_duff test_fwi test_fbp)
  message("Adding binary for ${bin}")
  # include file with main() for each bin
  add_executable(${bin} ${DIR_SRC}/${bin}.cpp ${FILE_VERSION_CPP})
# FIX: wrap anything new that might be breaking windows workflow
if(LINUX)
  target_include_directories(${bin} PRIVATE ${DIR_SRC_COMMON} ${DIR_SRC})
else()
  target_include_directories(${bin} PUBLIC ${DIR_SRC_COMMON} ${DIR_SRC})
endif()
  target_include_directories(${bin} PRIVATE ${GEOTIFF_INCLUDE_DIR})
  target_link_libraries(${bin} PUBLIC fs)
  target_link_libraries(${bin} PRIVATE ${GEOTIFF_LIBRARIES})
  target_link_libraries(${bin} PRIVATE TIFF::TIFF)
  target_link_libraries(${bin} PRIVATE PROJ::proj)
# FIX: wrap anything new that might be breaking windows workflow
if(LINUX)
  target_link_libraries(${bin} PRIVATE SQLite::SQLite3)
  target_link_libraries(${bin} PRIVATE -static)
  target_link_libraries(${bin} PRIVATE -static-libgcc -static-libstdc++)
endif()
# FIX: wrap anything new that might be breaking windows workflow
  if (ASAN_ARGS)
if(LINUX)
    target_link_options(${bin} BEFORE PRIVATE ${ASAN_ARGS})
else()
    target_link_options(${bin} BEFORE PUBLIC ${ASAN_ARGS})
endif()
  endif()
  add_custom_command(
    TARGET ${bin}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${bin}> ../)
  # HACK: ensure built for testing
  add_test(build_${bin}
      "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target ${bin})
  set_tests_properties(build_${bin}
    PROPERTIES DEPENDS ${PROJECT_NAME})
  if (APPLE)
    target_link_options(${bin} PRIVATE LINKER:-no_warn_duplicate_libraries)
  endif()
  if(NOT "${PROJECT_NAME}" STREQUAL "${bin}")
    set_tests_properties(build_${bin}
      PROPERTIES FIXTURES_SETUP ${bin}_fixture)
    # HACK: run tests from root so settings.ini is correct
    set(bin_dir "$<TARGET_FILE_DIR:${bin}>/..")
    set(bin_path "${bin_dir}/${bin}")
    add_test(
      NAME ${bin}
      WORKING_DIRECTORY "${bin_dir}"
      COMMAND "${bin_path}" -v)
    if(WIN32)
      set_property(
        TEST ${bin}
        PROPERTY ENVIRONMENT_MODIFICATION PATH=path_list_append:$<TARGET_RUNTIME_DLL_DIRS:${bin}>)
    endif()
    set_tests_properties(${bin}
      PROPERTIES FIXTURES_REQUIRED ${bin}_fixture)
  endif()
endforeach()


# HACK: find first copy of proj.db and copy to same directory as binary
file(GLOB_RECURSE PROJ_DBS ${CMAKE_BINARY_DIR} proj.db)
list(GET PROJ_DBS 0 PROJ_DB)
message("proj.db path is ${PROJ_DB}")
add_custom_command(
  TARGET ${PROJECT_NAME}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy "${PROJ_DB}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../")
