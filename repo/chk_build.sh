#!/bin/bash
v=0
while [ $v -eq 0 ]
do
    [ -f .git/rebase-merge/git-rebase-todo ] \
    && scripts/build.sh $* \
    && GIT_EDITOR=true git rebase --continue
    v=$?
done
