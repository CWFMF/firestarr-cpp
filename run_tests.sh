343
for TEST in DUFF FBP FWI
do
  echo "Building ${TEST}" \
    && rm -f ./firestarr \
    && scripts/mk_clean.sh -DTEST_${TEST}=1 > /dev/null 2>&1 \
    && ./firestarr -v $*
  RET=$?
  if [ "$RET" -ne 0 ]; then
    exit $RET
  fi
done
