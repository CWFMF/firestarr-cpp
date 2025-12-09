/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Duff.h"
#ifdef TEST_DUFF
#include "DuffSimple.h"
#include "Log.h"
#endif
namespace fs::duff
{
#ifdef TEST_DUFF
// FIX: this was used to compare to the old template version, but doesn't work now
//      left for reference for now so idea could be used for more tests
using duff::DuffType;
constexpr int RESOLUTION = 10000;
constexpr MathSize RANGE = 250.0;
// check %, so 1 decimal is fine
static constexpr MathSize EPSILON = 1e-1f;
template <int Ash, int Rho, int B0, int B1, int B2, int B3>
int compare(const string name, const DuffSimple& a, const DuffType<Ash, Rho, B0, B1, B2, B3>& b)
{
  logging::info("Checking %s", name.c_str());
  logging::check_equal(a.ash, b.ash(), "ash");
  logging::check_equal(a.rho, b.rho(), "rho");
  logging::check_equal(a.b0, b.b0(), "b0");
  logging::check_equal(a.b1, b.b1(), "b1");
  logging::check_equal(a.b2, b.b2(), "b2");
  logging::check_equal(a.b3, b.b3(), "b3");
  for (auto i = 0; i < RESOLUTION; ++i)
  {
    const MathSize mc = RANGE * i / RESOLUTION;
    const auto msg = std::format("probability of survival (mc = {})", mc);
    logging::check_tolerance(
      EPSILON, a.probabilityOfSurvival(mc), b.probabilityOfSurvival(mc), msg.c_str()
    );
  }
  return 0;
}
int test_duff(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  logging::info("Testing Duff");
  compare("SphagnumUpper", duffsimple::SphagnumUpper, duff::SphagnumUpper);
  compare("FeatherMoss", duffsimple::FeatherMoss, duff::FeatherMoss);
  compare("Reindeer", duffsimple::Reindeer, duff::Reindeer);
  compare("WhiteSpruce", duffsimple::WhiteSpruce, duff::WhiteSpruce);
  compare("Peat", duffsimple::Peat, duff::Peat);
  compare("PeatMuck", duffsimple::PeatMuck, duff::PeatMuck);
  compare("PineSeney", duffsimple::PineSeney, duff::PineSeney);
  compare("SprucePine", duffsimple::SprucePine, duff::SprucePine);
  return 0;
}
#endif
}
