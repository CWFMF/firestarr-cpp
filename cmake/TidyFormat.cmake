cmake_minimum_required(VERSION 3.31.6)

if (LINUX)
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
