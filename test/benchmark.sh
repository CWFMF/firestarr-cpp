#!/bin/bash
# DATES="1 3 7 14"
DATES="1 3 7 14"
TIME_LIMIT=36000

IS_PASTED=
if [[ "$0" =~ "/bash" ]]; then
  DIR_TEST=`realpath test`
  IS_PASTED=1
else
  # set -e
  DIR_TEST="$(dirname $(realpath "$0"))"
fi
TEST_SH=${DIR_TEST}/10N_50651.sh
# TEST_SH=${DIR_TEST}/slow.sh

DAYS=14
if [ "" != "$1" ]; then
    DAYS=$1
  if [ ! "${DAYS}" -gt 0 ] || [ ! "${DAYS}" -le 14 ]; then
    echo "Number of days must be an integer between 1 and 14 inclusive but got: ${DAYS}"
    exit
  fi
fi
OUT_DAYS=""
OUT_TIMES=""
((sleep ${TIME_LIMIT} && killall firestarr) > /dev/null 2>&1) &
for d in ${DATES}; do
    OUT_DAYS="${OUT_DAYS}$(printf '%6s' 1-${d})"
    if [[ ${d} -le ${DAYS} ]]; then
        # HACK: kill if running for too long
        output=$(${TEST_SH} ${d} 2>&1)
        RET=$?
        if [ "0" -eq "${RET}" ]; then
          t=$(echo "${output}" | grep "Total simulation time" | sed "s/.* \([0-9]*\) seconds.*/\1/" | tail -n1)
          OUT_TIMES="${OUT_TIMES}$(printf '%6s' ${t}s)"
        else
          DAYS=-1
        fi
    fi
done
killall sleep > /dev/null 2>&1
echo "#      # ${OUT_DAYS/1-1/  1}"
NUM=$(git log --oneline | wc -l)
echo "$(printf '# %04d ' ${NUM})$(printf '# %-26s  # ' "${OUT_TIMES}")$(git log --oneline | head -n1)"
