#!/bin/bash
NEWER=
if [ -f long.txt ]; then
    NEWER="-newer long.txt"
fi
find src/cpp ${NEWER} -type f \
    | xargs -P $(nproc) -tI {} clang-format --style=file -i {}
grep '.\{81,\}' src/cpp/* > long.txt
