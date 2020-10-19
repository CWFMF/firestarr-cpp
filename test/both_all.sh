#!/bin/bash
scripts/mk_clean.sh
RET0=$?
RESULT=$RET0
if [ "0" -ne "${RET0}" ]; then
  echo "Error during compilation"
else
  test/slow.sh
  RET1=$?
  if [ "0" -ne "${RET1}" ]; then
    RESULT=$RET1
  fi
  test/hourly.sh
  RET2=$?
  if [ "0" -ne "${RET2}" ]; then
    RESULT=$RET2
  fi
  test/10N_50561.sh
  RET3=$?
  if [ "0" -ne "${RET3}" ]; then
    RESULT=$RET3
  fi
  test/constant_all.sh
  RET4=$?
  if [ "0" -ne "${RET4}" ]; then
    RESULT=$RET4
  fi
fi

exit ${RESULT}
