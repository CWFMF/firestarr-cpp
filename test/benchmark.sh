#!/bin/bash
set -e
DIR=$(dirname $(realpath "$0"))
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
for d in 1 3 7 14; do
    OUT_DAYS="${OUT_DAYS}$(printf '%6s' 1-${d})"
    if [[ ${d} -le ${DAYS} ]]; then
        t=`$DIR/test_10N_50651.sh ${d} 2>&1 | grep "Total simulation time" | sed "s/.* \([0-9]*\) seconds.*/\1/" | tail -n1`
        OUT_TIMES="${OUT_TIMES}$(printf '%6s' ${t}s)"
    fi
done
echo "#      # ${OUT_DAYS/1-1/  1}"
NUM=$(git log --oneline | wc -l)
echo "$(printf '# %04d ' ${NUM})$(printf '# %-26s  # ' "${OUT_TIMES}")$(git log --oneline | head -n1)"
