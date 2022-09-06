export ARGS="2017-08-27 52.01 -89.024 12:15 $* --wx test/wx.csv --ffmc 90 --dmc 40 --dc 300 --apcp_0800 0"
cmake -S . -B cmake-build-release -D CMAKE_BUILD_TYPE=Release && cmake --build cmake-build-release -j 48 && time cmake-build-release/tbd ./data/output.release ${ARGS}
