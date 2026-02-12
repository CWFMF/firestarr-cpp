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
  set(VERSION "?.?.?.?")
endif()

set(MODIFIED_TIME "")

file(GLOB_RECURSE FILES_USED ${DIR_SRC}/*)
file(GLOB_RECURSE FILES_CMAKE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*)
list(FILTER FILES_USED EXCLUDE REGEX version*)
list(APPEND FILES_USED ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg.json ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-configuration.json ${FILES_CMAKE})

set(FILE_VERSION_CPP ${CMAKE_BINARY_DIR}/version.cpp)

# calculate hash from files that matter
set(HASHES)
# if modified from committed version then look at file timestamps
foreach(file_path IN LISTS FILES_USED)
  file(SHA512 ${file_path} HASH_FILE)
  message("${file_path} ${HASH_FILE}")
  list(APPEND HASHES ${HASH_FILE})
  file(TIMESTAMP "${file_path}" LAST_MOD_TIME "${FMT_TIME}")
  if ("${LAST_MOD_TIME}" STRGREATER "${MODIFIED_TIME}")
    message("Change in ${file_path}")
    set(MODIFIED_TIME "${LAST_MOD_TIME}")
  endif()
endforeach()
list(JOIN HASHES "" ALL_HASHES)
string(SHA512 FULL_HASH ${ALL_HASHES})

string(TIMESTAMP COMPILE_TIME "${FMT_TIME}" UTC)

string(SUBSTRING ${FULL_HASH} 0 8 HASH)

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
