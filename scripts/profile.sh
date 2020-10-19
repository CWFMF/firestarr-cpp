DIR_PERF=${TEMPDIR}/perf
DIR_ROOT="$(realpath $(dirname $(realpath "$0"))/..)"
DIR_BUILD="${DIR_ROOT}/build"
FILE_PERF=${DIR_PERF}/perf.data
FREQ=100
DIR_FG=/appl/FlameGraph
sudo sysctl -w kernel.perf_event_paranoid=1
[ ! -d "${DIR_PERF}" ] && mkdir -p "${DIR_PERF}"
# don't make this an actual submodule because it's just this script that needs it
[ ! -d "${DIR_FG}" ] && sudo apt install git && sudo git clone https://github.com/brendangregg/FlameGraph.git ${DIR_FG} && sudo chown -R ${USER}:${USER} ${DIR_FG}
rm -rf ${DIR_BUILD}
cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo -S${DIR_ROOT} -B${DIR_BUILD} -G "Unix Makefiles" > /dev/null 2>&1
cmake --build ${DIR_BUILD} --config RelWithDebInfo --target all -j $(nproc) -- > /dev/null 2>&1
cp ${DIR_BUILD}/firestarr ${DIR_ROOT}/
time perf record -o ${FILE_PERF} -F ${FREQ} -g --call-graph=dwarf -- $* > ${DIR_PERF}/run.log
chown ${USER}:${USER} ${FILE_PERF}
perf script -i ${FILE_PERF} | ${DIR_FG}/stackcollapse-perf.pl | ${DIR_FG}/flamegraph.pl > flame.html
