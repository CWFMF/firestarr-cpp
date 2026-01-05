for TEST in test_duff test_fbp test_fwi
do
  bin="./${TEST}"
  echo "Building ${TEST}" \
    && rm -f "${bin}" \
    && scripts/mk_clean.sh ${TEST} > /dev/null 2>&1 \
    && "${bin}" -v $* \
    && rm -f "${bin}"
  RET=$?
  if [ "$RET" -ne 0 ]; then
    exit $RET
  fi
done
