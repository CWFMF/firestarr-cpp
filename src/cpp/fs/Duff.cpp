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
int test_duff(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  logging::info("Testing Duff");
  compare_duff("SphagnumUpper", duffsimple::SphagnumUpper, duff::SphagnumUpper);
  compare_duff("FeatherMoss", duffsimple::FeatherMoss, duff::FeatherMoss);
  compare_duff("Reindeer", duffsimple::Reindeer, duff::Reindeer);
  compare_duff("WhiteSpruce", duffsimple::WhiteSpruce, duff::WhiteSpruce);
  compare_duff("Peat", duffsimple::Peat, duff::Peat);
  compare_duff("PeatMuck", duffsimple::PeatMuck, duff::PeatMuck);
  compare_duff("PineSeney", duffsimple::PineSeney, duff::PineSeney);
  compare_duff("SprucePine", duffsimple::SprucePine, duff::SprucePine);
  return 0;
}
#endif
}
