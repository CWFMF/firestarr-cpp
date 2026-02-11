#!/bin/bash
DIR_OLD=../data/initial/20250920
DIR_NEW=test/output/constant
any_diff=0
for f in $(find "${DIR_NEW}" -type f | grep -v "firestarr.log" | grep -v ".prj" | grep -v "xml" | sed "s@${DIR_NEW}/@@g")
do
  f_old=${DIR_OLD}/${f}
  f_new=${DIR_NEW}/${f}
  if [[ "${f}" == *.asc ]]; then
    # this does the message for us
    diff -q $1 "${f_old}" "${f_new}"
  else
    # 0 is true, 1 is false
    is_same=1
    gdalinfo -stats "${f_old}" 2>&1 | grep "STATISTICS_VALID_PERCENT=0" > /dev/null
    r0=$?
    if [ 0 -eq ${r0} ]; then
      # empty raster
      gdalinfo -stats "${f_new}" 2>&1 | grep "STATISTICS_VALID_PERCENT=0" > /dev/null
      is_same=$?
    else
      gdalcompare.py -skip_binary -skip_metadata "${f_new}" "${f_old}" 2>&1 | grep "Differences Found: 0" > /dev/null
      is_same=$?
    fi
    if [ 0 -ne "${is_same}" ]; then
      echo "Files ${f_old} and ${f_new} differ"
      any_diff=1
    elif [ "$1" == "-s" ]; then
      echo "Files ${f_old} and ${f_new} are identical"
    fi
  fi
done

if [ 0 -ne ${any_diff} ]; then
  echo "${DIR_OLD} and ${DIR_NEW} differ"
elif [ "$1" == "-s" ]; then
  echo "${DIR_OLD} and ${DIR_NEW} are identical"
fi
