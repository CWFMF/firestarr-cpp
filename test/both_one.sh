#!/bin/bash
scripts/mk_clean.sh
RET0=$?
RESULT=$RET0
if [ "0" -ne "${RET0}" ]; then
  echo "Error during compilation"
else
  test/hourly.sh
  RET1=$?
  if [ "0" -ne "${RET1}" ]; then
    RESULT=$RET1
  fi
  test/constant_one.sh
  RET2=$?
  if [ "0" -ne "${RET2}" ]; then
    RESULT=$RET2
  fi
fi

exit ${RESULT}
