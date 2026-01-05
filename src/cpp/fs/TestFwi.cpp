/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "TestFwi.h"
#include "ArgumentParser.h"
#include "FWI.h"
#include "FwiReference.h"
#include "Log.h"
#include "unstable.h"
#include "Util.h"
#include "Weather.h"
namespace fs::testing
{
using namespace std;
using namespace fs::fwireference;
constexpr auto DEFAULT_LATITUDE = 46.0;
int test_fwi_file(
  const char* file_in,
  const char* file_out,
  const MathSize latitude = DEFAULT_LATITUDE
)
{
  string line;
  MathSize temp, rhum, wind, prcp;
  int month, day;
  /* Initialize FMC, DMC, and DC */
  Ffmc ffmc0{85.0};
  Dmc dmc0{6.0};
  Dc dc0{15.0};
  Ffmc ffmc0_{ffmc0};
  Dmc dmc0_{dmc0};
  Dc dc0_{dc0};
  /* Open input and output files */
  ifstream inputFile(file_in);
  if (!inputFile.is_open())
  {
    cout << "Unable to open input data file";
    return -1;
  }
  ofstream outputFile;
  if (nullptr != file_out)
  {
    outputFile.open(file_out);
    if (!outputFile.is_open())
    {
      cout << "Unable to open output data file";
      return 1;
    }
  }
  logging::debug("Testing FWI generated from %s for latitude %g", file_in, latitude);
  const Latitude latitude_{latitude};
  /* Main loop for calculating indices */
  while (getline(inputFile, line))
  {
    istringstream ss(line);
    ss >> month >> day >> temp >> rhum >> wind >> prcp;
    const auto month_{Month::from_ordinal(month)};
    static constexpr MathSize EPSILON{std::numeric_limits<MathSize>::epsilon()};
    Temperature temp_{temp};
    RelativeHumidity rhum_{rhum};
    Speed wind_{wind};
    Precipitation prcp_{prcp};
    auto ffmc{FFMCcalc(temp_, rhum_, wind_, prcp_, ffmc0)};
    Ffmc ffmc_{temp_, rhum_, wind_, prcp_, ffmc0_};
    logging::check_tolerance(EPSILON, ffmc.value, ffmc_.value, "ffmc");
    auto dmc{DMCcalc(temp_, rhum_, prcp_, dmc0_, month_, latitude_)};
    Dmc dmc_{temp_, rhum_, prcp_, dmc0_, month, latitude};
    logging::check_tolerance(EPSILON, dmc.value, dmc_.value, "dmc");
    auto dc{DCcalc(temp_, prcp_, dc0_, month_, latitude_)};
    Dc dc_{temp_, prcp_, dc0_, month, latitude};
    logging::check_tolerance(EPSILON, dc.value, dc_.value, "dc");
    auto isi{ISIcalc(ffmc, wind_)};
    Isi isi_{wind_, ffmc_};
    logging::check_tolerance(EPSILON, isi.value, isi_.value, "isi");
    auto bui{BUIcalc(dmc, dc)};
    Bui bui_{dmc_, dc_};
    logging::check_tolerance(EPSILON, bui.value, bui_.value, "bui");
    auto fwi{FWIcalc(isi, bui)};
    Fwi fwi_{isi_, bui_};
    logging::check_tolerance(EPSILON, fwi.value, fwi_.value, "fwi");
    ffmc0 = ffmc;
    dmc0 = dmc;
    dc0 = dc;
    ffmc0_ = ffmc_;
    dmc0_ = dmc_;
    dc0_ = dc_;
    logging::verbose("%0.1f %0.1f %0.1f %0.1f %0.1f %0.1f", ffmc, dmc, dc, isi, bui, fwi);
    if (nullptr != file_out)
    {
      outputFile << std::format(
        "{:0.1f} {:0.1f} {:0.1f} {:0.1f} {:0.1f} {:0.1f}",
        ffmc.value,
        dmc.value,
        dc.value,
        isi.value,
        bui.value,
        fwi.value
      ) << endl;
      // outputFile << ffmc << ' ' << dmc << ' ' << dc << ' ' << isi << ' ' << bui << ' ' << fwi <<
      // endl;
    }
  }
  inputFile.close();
  if (nullptr != file_out)
  {
    outputFile.close();
  }
  return 0;
}
int compare_files(const string file0, const string file1)
{
  logging::info("Comparing %s to %s", file0.c_str(), file1.c_str());
  ifstream input0(file0.c_str());
  if (!input0.is_open())
  {
    cout << "Unable to open input data file " << file0;
    return -1;
  }
  ifstream input1(file1.c_str());
  if (!input1.is_open())
  {
    cout << "Unable to open input data file " << file1;
    return 1;
  }
  string line0;
  string line1;
  while (getline(input0, line0))
  {
    if (!getline(input1, line1))
    {
      return 1;
    }
    if (auto cmp = line0.compare(line1); 0 != cmp)
    {
      logging::error("\t%s\n!=\t%s", line0.c_str(), line1.c_str());
      return cmp;
    }
  }
  if (getline(input1, line1))
  {
    return -1;
  }
  return 0;
}
int test_fwi_files(const string file_expected, const string file_in, const string file_out)
{
  // FIX: other tests use test/input but want to switch to test/data
  if (auto ret = test_fwi_file(file_in.c_str(), file_out.c_str()); ret != 0)
  {
    logging::error("Generating %s from %s failed", file_out.c_str(), file_in.c_str());
    return ret;
  }
  if (auto cmp = compare_files(file_expected, file_out); 0 != cmp)
  {
    logging::error(
      "Comparison between files [%s, %s] gives %d", file_expected.c_str(), file_out.c_str(), cmp
    );
    return cmp;
  }
  return 0;
}
int test_fwi(const int argc, const char* const argv[])
{
  // HACK: parser happens before this
  std::ignore = argc;
  std::ignore = argv;
  constexpr auto FILE_EXPECTED{"test/data/fwi/fwi_out.txt"};
  constexpr auto FILE_IN{"test/data/fwi/fwi_in.txt"};
  constexpr auto FILE_OUT{"test/output/fwi/fwi_out.txt"};
  constexpr auto LATITUDE_MIN{-90.0};
  constexpr auto LATITUDE_MAX{90.0};
  constexpr auto STEP{0.001};
  make_directory_recursive("test/output/fwi");
  if (const auto ret = test_fwi_files(FILE_EXPECTED, FILE_IN, FILE_OUT); 0 != ret)
  {
    return ret;
  }
  logging::info(
    "Testing across latitude range [%+g, %+g] with step %g", LATITUDE_MIN, LATITUDE_MAX, STEP
  );
  auto check_latitude = [](const MathSize latitude) {
    if (auto ret = test_fwi_file(FILE_IN, nullptr, latitude); ret != 0)
    {
      logging::error("Generating from %s with latitude %g failed", FILE_IN, latitude);
      return ret;
    }
    return 0;
  };
  MathSize latitude = LATITUDE_MIN;
  while (LATITUDE_MAX > latitude)
  {
    if (auto ret = check_latitude(latitude); 0 != ret)
    {
      return ret;
    }
    latitude += STEP;
  }
  if (auto ret = check_latitude(LATITUDE_MAX); 0 != ret)
  {
    return ret;
  }
  logging::note("Testing FWI calculations succeeded");
  return 0;
}
}
