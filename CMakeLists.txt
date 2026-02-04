cmake_minimum_required(VERSION 3.31.6)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(DIR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp)
set(DIR_SRC_COMMON ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/fs)
set(FILE_ENV ${CMAKE_CURRENT_SOURCE_DIR}/.env)

if(NOT VCPKG_ROOT)
  set(VCPKG_ROOT $ENV{VCPKG_ROOT})
endif()
if(NOT VCPKG_ROOT)
  set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg")
endif()
if (LINUX)
  set(VCPKG_CRT_LINKAGE static)
  set(VCPKG_LIBRARY_LINKAGE static)
else()
  set(VCPKG_CRT_LINKAGE dynamic)
  set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
set(VCPKG_BUILD_TYPE release)
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
  CACHE STRING "Vcpkg toolchain file")

if(WIN32)
  set(BUILD_SHARED_LIBS OFF)
  set(VCPKG_LIBRARY_LINKAGE static)
  # FIX: unsure if this matters but doesn't break things on arm64
  # set(VCPKG_TARGET_TRIPLET x64-windows-static)
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


# HACK: architecture that works on azure batch and local machine
# set(ARCH_PREFERRED "skylake-avx512")

message("CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

project(firestarr)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

include(CheckCXXCompilerFlag)

# -mcpu is deprecated on some and required on others
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH)
if(COMPILER_SUPPORTS_MARCH)
  set(MARCH "march")
else()
  set(MARCH "mcpu")
endif()

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
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # HACK: if compiler supports ARCH_PREFERRED on this then use that else native
    execute_process(COMMAND bash -c "(${CMAKE_CXX_COMPILER} -${MARCH}=native -Q --help=target | grep ${ARCH_PREFERRED} > /dev/null && echo -n ${ARCH_PREFERRED}) || (echo -n 'native')"
            OUTPUT_VARIABLE USE_ARCH
            ERROR_QUIET)
    if (USE_ARCH)
      add_compile_options(-${MARCH}=${USE_ARCH})
      message("USE_ARCH = ${USE_ARCH}")
    endif()
  endif()
  if(CMAKE_VERBOSE_MAKEFILE)
    add_compile_options(-fopt-info-vec-all)
  endif()
  add_compile_options(
    "$<$<CONFIG:Release,RelWithDebInfo>:-O3;>"
    "$<$<CONFIG:MinSizeRel>:-Os;>"
    "$<$<CONFIG:Debug>:-O0;>")
  if(APPLE)
    check_cxx_compiler_flag(-mno-sve HAVE_NO_SVE)
    if(HAVE_NO_SVE)
      message("Disabling SVE")
      add_compile_options(-mno-sve)
    endif()
  endif()
  check_cxx_compiler_flag(-mno-avx512f HAVE_NO_AVX512F)
  if(HAVE_NO_AVX512F)
    message("Disabling AVX512 because performance is worse and results are wildly different with it on")
    add_compile_options(-mno-avx512f)
    # FIX: disable to avoid for now:
    #   clang++: warning: -ltcmalloc: 'linker' input unused [-Wunused-command-line-argument]
    # add_compile_options("$<$<CONFIG:Debug,RelWithDebInfo>:-ltcmalloc;>")
  endif()
  add_compile_options(
    "$<$<CONFIG:Debug,RelWithDebInfo>:-g;-fno-omit-frame-pointer;>")
  # HACK: only use ASAN with gcc
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    if(USE_ASAN)
      message("Using ASAN")
      set(ASAN_ARGS
        address pointer-compare leak
        # thread
        # float-cast-overflow
        undefined shift-exponent shift-base integer-divide-by-zero unreachable
        vla-bound null return signed-integer-overflow bounds bounds-strict
        alignment object-size float-divide-by-zero nonnull-attribute
        returns-nonnull-attribute bool enum vptr pointer-overflow builtin)
      list(TRANSFORM ASAN_ARGS REPLACE "([^ ]+)" "-fsanitize=\\1")
      set(ASAN_ARGS
        ${ASAN_ARGS}
        -fsanitize-address-use-after-scope
        -Og
        -fno-omit-frame-pointer
        -fno-ipa-icf
        -fno-optimize-sibling-calls)
      # message("${ASAN_ARGS}")
      add_compile_options(${ASAN_ARGS})
    endif()
  endif()
endif()
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

# HACK: look for version in parent folder .env
if(EXISTS "${FILE_ENV}")
  execute_process(COMMAND git log -n1 --pretty=%h ${FILE_ENV} OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE HASH_ENV)
  file(STRINGS "${FILE_ENV}" CONFIG REGEX "^[ ]*[A-Za-z0-9_]+[ ]*=")
  list(TRANSFORM CONFIG STRIP)
  list(TRANSFORM CONFIG REPLACE "([^=]+)=[ ]*(.*)" "set(\\1 \"\\2\")\n")
  message(${CONFIG})
  cmake_language(EVAL CODE ${CONFIG})
  message("Parsed config")
else()
  message(WARNING "VERSION IS NOT SET")
  # no version set
  set(VERSION "?.?.?.?")
endif()

execute_process(COMMAND git rev-parse --short --verify HEAD OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE HASH)
execute_process(COMMAND git diff-index --quiet HEAD RESULT_VARIABLE GIT_CHANGED)

if (NOT "${HASH_ENV}" STREQUAL "${HASH}")
  # add + to version if .env isn't from current commit
  set(VERSION "${VERSION}+")
endif()

set(MODIFIED_TIME "")

message("Compiler is ${CMAKE_CXX_COMPILER}")
execute_process(COMMAND ${CMAKE_CXX_COMPILER})
if (WIN32)
  message("Not setting compiler flags so binary works across windows")
else()
  message("Compiler flags for target with ${CMAKE_CXX_COMPILER} are:")
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    execute_process(COMMAND g++ -${MARCH}=native -Q --help=target)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    execute_process(COMMAND clang -E - -${MARCH}=native "-###")
  endif()
endif()

file(GLOB_RECURSE FILES_USED ${DIR_SRC}/*.cpp ${DIR_SRC}/*.h)
list(FILTER FILES_USED EXCLUDE REGEX version*)

set(FILE_VERSION_H ${DIR_SRC_COMMON}/version.h)
set(FILE_UNSTABLE_CPP ${DIR_SRC_COMMON}/unstable.cpp)

# is anything in git changed?
if ("${GIT_CHANGED}" STREQUAL "0")
  # use time from git commit since nothing is different
  execute_process(COMMAND git log -1 --pretty=%ad --date=format:%Y-%m-%dT%H:%M:%SZ OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE MODIFIED_TIME)
else()
  # either changed or git didn't work
  if (NOT "" STREQUAL "${HASH}")
    # add + to hash if modified from committed version
    set(HASH "${HASH}+")
  endif()
  # if modified from committed version then look at file timestamps
  foreach(file_path IN LISTS FILES_USED)
    file(TIMESTAMP "${file_path}" LAST_MOD_TIME "${FMT_TIME}")
    if ("${LAST_MOD_TIME}" STRGREATER "${MODIFIED_TIME}")
      message("Change in ${file_path}")
      set(MODIFIED_TIME "${LAST_MOD_TIME}")
    endif()
  endforeach()
endif()

string(TIMESTAMP COMPILE_TIME "${FMT_TIME}" UTC)

message("HASH = ${HASH}")
message("MODIFIED_TIME = ${MODIFIED_TIME}")
message("VERSION = ${VERSION}")
message("COMPILE_TIME = ${COMPILE_TIME}")

if (NOT "" STREQUAL "${HASH}")
  set(HASH "[${HASH}]")
endif()

set (SPECIFIC_REVISION "v${VERSION} ${HASH} <${MODIFIED_TIME}>")
message("${SPECIFIC_REVISION}")
set(VERSION_CODE "constexpr char SPECIFIC_REVISION[]{\"${SPECIFIC_REVISION}\"};\n")
if(EXISTS FILE_VERSION_H)
  file(STRINGS ${FILE_VERSION_H} VERSION_CODE_OLD)
  # HACK: file(READ ...) is reacting to special characters, so compare substring
  string(FIND "${SPECIFIC_REVISION}" "${VERSION_CODE_OLD}" SAME_CODE)
endif()
# if matched exiting file don't write
if (NOT 0 EQUAL SAME_CODE)
  file(WRITE ${FILE_VERSION_H} "${VERSION_CODE}")
endif()

set_source_files_properties(${FILE_UNSTABLE_CPP} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")

if (LINUX)
  if(NOT ASAN_ARGS)
    # HACK: breaks if g++ is compiler even if clang++ exists
    message("CXX = ${CXX}")
    message("CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
    message("CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      # HACK: prevent failure if clang-tidy or requirements aren't installed
      find_program(CLANG_COMPILER clang++)
      find_program(CLANG_TIDY clang-tidy)
      find_program(CLANG_APPLY_REPLACEMENTS clang-apply-replacements)
      find_program(CLANG_FORMAT clang-format)
      message("CLANG_COMPILER = ${CLANG_COMPILER}")
      message("CLANG_TIDY = ${CLANG_TIDY}")
      message("CLANG_APPLY_REPLACEMENTS = ${CLANG_APPLY_REPLACEMENTS}")
      message("CLANG_FORMAT = ${CLANG_FORMAT}")
      set(DIR_CLANG clang-tidy-fixes)
      if(
        "CLANG_COMPILER-NOTFOUND" STREQUAL "${CLANG_COMPILER}"
        OR "CLANG_TIDY-NOTFOUND" STREQUAL "${CLANG_TIDY}"
        OR "CLANG_APPLY_REPLACEMENTS-NOTFOUND" STREQUAL "${CLANG_APPLY_REPLACEMENTS}"
        OR "CLANG_FORMAT-NOTFOUND" STREQUAL "${CLANG_FORMAT}"
      )
        message("clang-tidy requirements not found so not running")
      else()
        message("clang-tidy requirements found so running")
        set(CMAKE_CXX_CLANG_TIDY_EXPORT_FIXES_DIR "${DIR_CLANG}")
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY};-config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy")

        # HACK: using found path seems to cause issues but we know they exist
        add_custom_target(apply_clang_tidy_fixes ALL
          COMMAND ${CLANG_APPLY_REPLACEMENTS} "${DIR_CLANG}"
          DEPENDS ${PROJECT_NAME}
        )

        # HACK: clang-tidy not applying format?
        add_custom_target(clang_format ALL
          COMMAND ${CLANG_FORMAT} --style=file -i ${FILES_USED}
          DEPENDS ${PROJECT_NAME}
        )
      endif()
    endif()
  endif()
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
  add_executable(${bin} ${DIR_SRC}/${bin}.cpp)
# FIX: wrap anything new that might be breaking windows workflow
if(LINUX)
  target_include_directories(${bin} PRIVATE ${DIR_SRC_COMMON})
else()
  target_include_directories(${bin} PUBLIC ${DIR_SRC_COMMON})
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
