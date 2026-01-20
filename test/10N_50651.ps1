$vsPath = vswhere -all -products * -requires Microsoft.Component.MSBuild -property installationpath
Import-Module "${vsPath}\\Common7\\Tools\\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "$vsPath" -SkipAutomaticLocation -Arch amd64

$CMAKE="${vsPath}\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe"
# rm -r -Force ./build
&${CMAKE} -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE --no-warn-unused-cli -S . -B ./build -G "Visual Studio 17 2022" -T host=x64 -A x64
&${CMAKE} --build ./build --config Release --target ALL_BUILD --parallel

# cmake --preset Release
# cmake --build --preset Release --parallel
$DIR_IN="test/input/10N_50651"
$DIR_OUT="test/output/10N_50651"

rm -r -Force "${DIR_OUT}"
mkdir "${DIR_OUT}"
# ./firestarr.exe "${DIR_OUT}" 2024-06-03 58.81228184403946 -122.9117103995713 01:00 --ffmc 89.9 --dmc 59.5 --dc 450.9 --apcp_prev 0 -v --wx test/input/10N_50651/firestarr_10N_50651_wx.csv --output_date_offsets [1] --tz -5 --perim test/input/10N_50651/10N_50651.tif --raster-root "${DIR_IN}" -v -v
./firestarr.exe "${DIR_OUT}" `
  2024-06-03 `
  58.81228184403946 -122.9117103995713 `
  01:00 `
  --ffmc 89.9 `
  --dmc 59.5 `
  --dc 450.9 `
  --apcp_prev 0 `
  --wx test/input/10N_50651/firestarr_10N_50651_wx.csv `
  --output_date_offsets [1] `
  --tz -5 `
  --perim test/input/10N_50651/10N_50651.tif `
  -v `
  --raster-root "${DIR_IN}"
