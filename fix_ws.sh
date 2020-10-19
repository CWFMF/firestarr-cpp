set -e
#!/bin/bash
# git commit --amend && git rebase --continue > git.log 2>&1
# REV=$(cat git.log | grep "Could not apply" | sed "s/Could not apply \([^.]*\).*/\1/g")
# # cat git.log \
# #     | grep CONFLICT \
# #     | sed "s/.*conflict in \([^\t\n\r]*\)/\1/g" \
# #     | xargs -tI {} git restore --source=$REV {}

git status > git.log
REV=$(head -n 4 git.log | tail -n1 | sed "s/ *[^ ]* \([^ ]*\) .*/\1/")
cat git.log \
    | grep "both modified:" \
    | sed "s/.*both modified: *//g" \
    | xargs -tI {} git restore --source=$REV {}
