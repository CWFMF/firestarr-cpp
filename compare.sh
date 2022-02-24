export ARGS="2017-08-27 52.01 -89.024 12:15 $* --wx test/wx.csv --ffmc 90 --dmc 40 --dc 300 --apcp_0800 0"
cmake -S . -B cmake-build-debug -D CMAKE_BUILD_TYPE=Debug
cmake -S . -B cmake-build-release -D CMAKE_BUILD_TYPE=Release
cmake --build cmake-build-debug -j 48
cmake --build cmake-build-release -j 48
cmake-build-debug/FireSTARR ./Data/output.debug ${ARGS}
cmake-build-release/FireSTARR ./Data/output.release ${ARGS}
