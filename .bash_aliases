alias wcl='wc -l'
alias glo='git log --oneline'
alias gs='git status'
alias gd='git diff'
alias gap='git add -p'
alias vg='valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose'


alias fcc='./fcc.sh'
alias fs='./find_shortest.sh'
alias gp='./grep_prev.sh'
alias fp='./find_prev.sh'
alias pg='grep "change #" prev.log | sort -nk3'

alias di='diff -rq -x firestarr.log test/output/constant ../data/initial/20250927 | grep -v "Only in"'

alias rt='rm -rf test/output/constant && git restore --staged test/output/constant && git restore test/output/constant'

alias mc='scripts/mk_clean.sh'
alias b='scripts/build.sh'

alias next='scripts/mk_clean.sh && git add src/cpp && GIT_EDITOR=true git rebase --continue'
alias nt='test/constant_all.sh && git add src/cpp && GIT_EDITOR=true git rebase --continue'
alias nc='test/constant_one.sh && git add src/cpp && GIT_EDITOR=true git rebase --continue'
alias nb='scripts/mk_clean.sh && git add src/cpp && GIT_EDITOR=true git rebase --continue && ./chk_build.sh'

alias wd='watch -n1 "find test/output/constant -type d | wc -l"'
alias wr='watch -n1 "wc -l .git/rebase-merge/git-rebase-todo"'
