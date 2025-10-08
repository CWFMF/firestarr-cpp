#!/bin/bash

test/both_all.sh

git add test/output \
&& GIT_EDITOR=true git commit --amend \
&& GIT_EDITOR=true git rebase --continue \
&& ./find_bad.sh
