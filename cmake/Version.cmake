cmake_minimum_required(VERSION 3.31.6)

set(FILE_ENV ${CMAKE_CURRENT_SOURCE_DIR}/.env)


# HACK: look for version in parent folder .env
if(EXISTS "${FILE_ENV}")
  file(STRINGS "${FILE_ENV}" CONFIG REGEX "^[ ]*[A-Za-z0-9_]+[ ]*=")
  list(TRANSFORM CONFIG STRIP)
  list(TRANSFORM CONFIG REPLACE "([^=]+)=[ ]*(.*)" "set(\\1 \"\\2\")\n")
  message(${CONFIG})
  cmake_language(EVAL CODE ${CONFIG})
  message("Parsed config")
else()
  message(WARNING "VERSION IS NOT SET")
  # no version set
  set(VERSION "?.?")
endif()

set(MODIFIED_TIME "")

file(GLOB_RECURSE FILES_USED ${DIR_SRC}/*)
file(GLOB_RECURSE FILES_CMAKE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*)
list(FILTER FILES_USED EXCLUDE REGEX version*)
list(APPEND FILES_USED ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-configuration.json ${FILES_CMAKE})

set(FILE_VERSION_CPP ${CMAKE_BINARY_DIR}/version.cpp)

set(HASH_PREFIX "")
set(HASH_SUFFIX "")
set(MODIFIED_TIME "")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  if(EXISTS "${FILE_ENV}")
    # check when .env last changed - if not this commit then this is version+
    execute_process(COMMAND git log -n1 --pretty=%H ${FILE_ENV} OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE VERSION_HASH)
  endif()
  execute_process(COMMAND git rev-parse --verify HEAD OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE FULL_HASH)
  message("VERSION_HASH = ${VERSION_HASH}")
  message("FULL_HASH = ${FULL_HASH}")
  if (NOT "${VERSION_HASH}" STREQUAL "${FULL_HASH}")
    # add + to version if .env isn't from current commit
    set(VERSION "${VERSION}+")
  endif()
  # is anything in git changed?
  list(JOIN FILES_USED " " FILES_USED_STRING)
  execute_process(COMMAND git diff-index HEAD -- ${FILES_USED_STRING} RESULT_VARIABLE GIT_CHANGED)
  message("GIT_CHANGED = ${GIT_CHANGED}")
  if ("${GIT_CHANGED}" STREQUAL "0")
    # use time from git commit since nothing is different
    execute_process(COMMAND git log -1 --pretty=%ad --date=format:%Y-%m-%dT%H:%M:%SZ OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE MODIFIED_TIME)
  else()
    set(HASH_SUFFIX "+")
  endif()
else()
  # will never have a '+' for HASH_SUFFIX because it's based on exact files
  set(HASH_PREFIX "file:")
  list(JOIN HASHES "" ALL_HASHES)
  string(SHA512 FULL_HASH ${ALL_HASHES})
endif()

if(NOT MODIFIED_TIME)
  # calculate hash from files that matter
  set(HASHES)
  # if modified from committed version then look at file timestamps
  foreach(file_path IN LISTS FILES_USED)
    file(SHA512 ${file_path} HASH_FILE)
    if(CMAKE_VERBOSE_MAKEFILE)
      message("${file_path} ${HASH_FILE}")
    endif()
    list(APPEND HASHES ${HASH_FILE})
    file(TIMESTAMP "${file_path}" LAST_MOD_TIME "${FMT_TIME}")
    if ("${LAST_MOD_TIME}" STRGREATER "${MODIFIED_TIME}")
      set(MODIFIED_TIME "${LAST_MOD_TIME}")
    endif()
  endforeach()
endif()

string(SUBSTRING ${FULL_HASH} 0 10 HASH)
set(HASH "${HASH_PREFIX}${HASH}${HASH_SUFFIX}")
set(FULL_HASH "${HASH_PREFIX}${FULL_HASH}${HASH_SUFFIX}")
string(TIMESTAMP COMPILE_TIME "${FMT_TIME}" UTC)

set(COMPILED_ON "${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_SYSTEM}-${CMAKE_CXX_COMPILER_ID}")

message("FULL_HASH = ${FULL_HASH}")
message("HASH = ${HASH}")
message("MODIFIED_TIME = ${MODIFIED_TIME}")
message("VERSION = ${VERSION}")
message("COMPILE_TIME = ${COMPILE_TIME}")
message("COMPILED_ON = ${COMPILED_ON}")

if (NOT "" STREQUAL "${HASH}")
  set(HASH "[${HASH}]")
endif()

set (SPECIFIC_REVISION "v${VERSION} ${HASH} <${MODIFIED_TIME}>")
message("${SPECIFIC_REVISION}")
set(VERSION_CODE "extern \"C\" const char* const SPECIFIC_REVISION{\"${SPECIFIC_REVISION}\"}\;")
list(APPEND VERSION_CODE "extern \"C\" const char* const FULL_HASH{\"${FULL_HASH}\"}\;")
list(APPEND VERSION_CODE "extern \"C\" const char* const COMPILED_ON{\"${COMPILED_ON}\"}\;")
list(JOIN VERSION_CODE "\n" VERSION_CODE)
if(EXISTS FILE_VERSION_CPP)
  file(STRINGS ${FILE_VERSION_CPP} VERSION_CODE_OLD)
  # HACK: file(READ ...) is reacting to special characters, so compare substring
  string(FIND "${SPECIFIC_REVISION}" "${VERSION_CODE_OLD}" SAME_CODE)
endif()
# if matched exiting file don't write
if (NOT 0 EQUAL SAME_CODE)
  file(WRITE ${FILE_VERSION_CPP} "${VERSION_CODE}")
endif()
