#!/bin/bash
set -e

FILE_TODO=".git/rebase-merge/git-rebase-todo"

while [ -f "${FILE_TODO}" ]
do
    MSG=$(git log --oneline -n1 | sed "s/[^ ]* //")

    pushd test/output/constant
    REV_SUB=$(git log cpp23 --oneline --grep "${MSG}" | cut -d' ' -f1)
    git checkout ${REV_SUB}
    popd
    git add test/output/constant
    GIT_EDITOR=true git commit --amend
    git rebase --continue
done
