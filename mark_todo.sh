#!/bin/bash
git log --oneline cpp23 $* \
    | sed "s/ .*//g" \
    | xargs -tI {} sed -i "s/pick {}/edit {}/" .git/rebase-merge/git-rebase-todo
