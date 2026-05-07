#!/bin/bash
set -x
# if we aren't commiting then it's hard to compare later
N=$(expr $(ls -1 out/benchmark010 | tail -n1) + 1)
n=$(printf "%03d" $N)
scripts/build.sh Debug \
  && scripts/mk_clean.sh Release \
  && cp firestarr fs_release \
  && git log --oneline -n5 \
  && git diff > out/${n}.diff \
  && (git add -u && git commit -m "WIP: ${N} $@" || echo "No changes but running as ${N}") \
  && ./bench.sh ${N} 1 | tee out/${n}_01day.log \
  && ./bench.sh ${N} 10 | tee out/${n}_10day.log
