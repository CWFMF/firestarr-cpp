#!/bin/bash
DIR="$(dirname ${0})"
${DIR}/mk_clean.sh $@ -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON
