#!/bin/bash
D=$(date +"%Y%m%d_%H%M")
git branch cpp23_${D}
git rebase -i --committer-date-is-author-date --empty=drop $*
