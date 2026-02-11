#!/bin/bash

num="$1"
if [ -z "${num}" ]; then
  num=1
fi

./find_prev.sh $(./count_lines.sh \
  | head -n${num} \
  | tail -n1 \
  | cut -d' ' -f1) \
  > prev.log
grep "last change before" prev.log \
  | head -n1 \
  | sed "s/.*last change before \([^ ]*\).*/\1/" \
  | xargs -I{} git log --oneline -n1 {}
grep "change #" prev.log \
  | sort -nk3
code prev.log
