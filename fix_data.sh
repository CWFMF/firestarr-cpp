set -e
#!/bin/bash
# git commit --amend && git rebase --continue > git.log 2>&1
# REV=$(cat git.log | grep "Could not apply" | sed "s/Could not apply \([^.]*\).*/\1/g")
# # cat git.log \
# #     | grep CONFLICT \
# #     | sed "s/.*conflict in \([^\t\n\r]*\)/\1/g" \
# #     | xargs -tI {} git restore --source=$REV {}

git status > git.log
REV=$(sed -n "/Next commands/q;p;" git.log | grep -E "^   " | sed "s/^[ \t]*//g" | tail -n1 | cut -d' ' -f2)
# cat git.log \
#     | grep "modified:" \
#     | sed "s/.*modified: *//g" \
#     | xargs -tI {} git restore --source=$REV {}
# git restore --staged test/output/constant
rm -rf test/output/constant
git restore --staged  --source=$REV test/output/constant
