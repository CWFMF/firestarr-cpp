USER=$(gh api user | jq -r ".login")
run_job() {
    # need to bind /etc/ssl/certs so ssl doesn't fail
    gh act -a ${USER} --container-options "-v /etc/ssl/certs:/etc/ssl/certs:ro" -s GHCR_TOKEN=${GHCR_TOKEN} -j $*
}
run_job cmake-test-all $*
