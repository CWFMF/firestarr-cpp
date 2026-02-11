#!/bin/bash

find_lines() {
  for rev in $(git log wip --oneline | grep -vE "(FEATURE|BUGFIX):" | cut -d' ' -f1);
  do
    lines=$(git show -v $rev | wc -l | awk '{printf("%10d", $1)}')
    echo "$rev $lines"
  done
}

find_lines | sort -nk2
