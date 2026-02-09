cmake_minimum_required(VERSION 3.31.6)

set(FILE_ENV ${CMAKE_CURRENT_SOURCE_DIR}/.env)


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

file(GLOB_RECURSE FILES_USED ${DIR_SRC}/*.cpp ${DIR_SRC}/*.h)
list(FILTER FILES_USED EXCLUDE REGEX version*)

set(FILE_VERSION_CPP ${DIR_SRC}/version.cpp)

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
set(VERSION_CODE "extern \"C\" const char* const SPECIFIC_REVISION{\"${SPECIFIC_REVISION}\"};\n")
if(EXISTS FILE_VERSION_CPP)
  file(STRINGS ${FILE_VERSION_CPP} VERSION_CODE_OLD)
  # HACK: file(READ ...) is reacting to special characters, so compare substring
  string(FIND "${SPECIFIC_REVISION}" "${VERSION_CODE_OLD}" SAME_CODE)
endif()
# if matched exiting file don't write
if (NOT 0 EQUAL SAME_CODE)
  file(WRITE ${FILE_VERSION_CPP} "${VERSION_CODE}")
endif()
