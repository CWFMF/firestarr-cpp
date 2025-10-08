#!/bin/bash
scripts/mk_clean.sh
RET=$?
RESULT=$RET
if [ "0" -ne "${RET}" ]; then
  echo "Error during compilation"
else
  test/hourly.sh
  RET=$?
  if [ "0" -ne "${RET}" ]; then
    RESULT=$RET
  fi
  test/perim.sh
  RET=$?
  if [ "0" -ne "${RET}" ]; then
    RESULT=$RET
  fi
  test/constant_one.sh
  RET=$?
  if [ "0" -ne "${RET}" ]; then
    RESULT=$RET
  fi
fi

exit ${RESULT}
