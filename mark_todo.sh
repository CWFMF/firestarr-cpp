#!/bin/bash
git log cpp23 --oneline "$@" \
    | sed "s/ .*//g" \
    | xargs -tI {} sed -i "s/pick {}/edit {}/" .git/rebase-merge/git-rebase-todo
