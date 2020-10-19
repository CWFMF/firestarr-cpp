#!/bin/bash
DIR="$(dirname ${0})"
${DIR}/build.sh --clean-first ${@}
