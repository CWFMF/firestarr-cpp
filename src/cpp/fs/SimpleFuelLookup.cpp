/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "SimpleFuelLookup.h"
#include "FBP45.h"
#include "FuelLookup.h"
#include "Log.h"
#include "RangeIterator.h"
#include "Settings.h"
#include "SimpleFBP.h"
#include "SimpleFuelType.h"
namespace fs::simplefbp
{
string simplify_name(const string_view fuel)
{
  string simple_fuel_name{fuel};
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '-'), simple_fuel_name.end()
  );
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), ' '), simple_fuel_name.end()
  );
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '('), simple_fuel_name.end()
  );
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), ')'), simple_fuel_name.end()
  );
  simple_fuel_name.erase(
    std::remove(simple_fuel_name.begin(), simple_fuel_name.end(), '/'), simple_fuel_name.end()
  );
  std::transform(
    simple_fuel_name.begin(), simple_fuel_name.end(), simple_fuel_name.begin(), ::toupper
  );
  // remove PDF & PC
  const auto pc = simple_fuel_name.find("PC");
  if (string::npos != pc)
  {
    simple_fuel_name.erase(pc);
  }
  const auto pdf = simple_fuel_name.find("PDF");
  if (string::npos != pdf)
  {
    simple_fuel_name.erase(pdf);
  }
  return simple_fuel_name;
}
static const map<const string_view, const string_view> DEFAULT_TYPES{
  {"Spruce-Lichen Woodland", "C-1"},
  {"Boreal Spruce", "C-2"},
  {"Mature Jack or Lodgepole Pine", "C-3"},
  {"Immature Jack or Lodgepole Pine", "C-4"},
  {"Red and White Pine", "C-5"},
  {"Conifer Plantation", "C-6"},
  {"Ponderosa Pine - Douglas-Fir", "C-7"},
  {"Leafless Aspen", "D-1"},
  {"Green Aspen (with BUI Thresholding)", "D-2"},
  {"Aspen", "D-1/D-2"},
  {"Jack or Lodgepole Pine Slash", "S-1"},
  {"White Spruce - Balsam Slash", "S-2"},
  {"Coastal Cedar - Hemlock - Douglas-Fir Slash", "S-3"},
  {"Matted Grass", "O-1a"},
  {"Standing Grass", "O-1b"},
  {"Grass", "O-1"},
  {"Boreal Mixedwood - Leafless", "M-1"},
  {"Boreal Mixedwood - Green", "M-2"},
  {"Boreal Mixedwood", "M-1/M-2"},
  {"Dead Balsam Fir Mixedwood - Leafless", "M-3"},
  {"Dead Balsam Fir Mixedwood - Green", "M-4"},
  {"Dead Balsam Fir Mixedwood", "M-3/M-4"},
  {"Not Available", "Non-fuel"},
  {"Non-fuel", "Non-fuel"},
  {"Water", "Non-fuel"},
  {"Urban", "Non-fuel"},
  {"Unknown", "Non-fuel"},
  {"Unclassified", "D-1/D-2"},
  {"Vegetated Non-Fuel", "M-1/M-2 (25 PC)"},
  {"Boreal Mixedwood - Leafless (00% Conifer)", "M-1 (00 PC)"},
  {"Boreal Mixedwood - Leafless (05% Conifer)", "M-1 (05 PC)"},
  {"Boreal Mixedwood - Leafless (10% Conifer)", "M-1 (10 PC)"},
  {"Boreal Mixedwood - Leafless (15% Conifer)", "M-1 (15 PC)"},
  {"Boreal Mixedwood - Leafless (20% Conifer)", "M-1 (20 PC)"},
  {"Boreal Mixedwood - Leafless (25% Conifer)", "M-1 (25 PC)"},
  {"Boreal Mixedwood - Leafless (30% Conifer)", "M-1 (30 PC)"},
  {"Boreal Mixedwood - Leafless (35% Conifer)", "M-1 (35 PC)"},
  {"Boreal Mixedwood - Leafless (40% Conifer)", "M-1 (40 PC)"},
  {"Boreal Mixedwood - Leafless (45% Conifer)", "M-1 (45 PC)"},
  {"Boreal Mixedwood - Leafless (50% Conifer)", "M-1 (50 PC)"},
  {"Boreal Mixedwood - Leafless (55% Conifer)", "M-1 (55 PC)"},
  {"Boreal Mixedwood - Leafless (60% Conifer)", "M-1 (60 PC)"},
  {"Boreal Mixedwood - Leafless (65% Conifer)", "M-1 (65 PC)"},
  {"Boreal Mixedwood - Leafless (70% Conifer)", "M-1 (70 PC)"},
  {"Boreal Mixedwood - Leafless (75% Conifer)", "M-1 (75 PC)"},
  {"Boreal Mixedwood - Leafless (80% Conifer)", "M-1 (80 PC)"},
  {"Boreal Mixedwood - Leafless (85% Conifer)", "M-1 (85 PC)"},
  {"Boreal Mixedwood - Leafless (90% Conifer)", "M-1 (90 PC)"},
  {"Boreal Mixedwood - Leafless (95% Conifer)", "M-1 (95 PC)"},
  {"Boreal Mixedwood - Green (00% Conifer)", "M-2 (00 PC)"},
  {"Boreal Mixedwood - Green (05% Conifer)", "M-2 (05 PC)"},
  {"Boreal Mixedwood - Green (10% Conifer)", "M-2 (10 PC)"},
  {"Boreal Mixedwood - Green (15% Conifer)", "M-2 (15 PC)"},
  {"Boreal Mixedwood - Green (20% Conifer)", "M-2 (20 PC)"},
  {"Boreal Mixedwood - Green (25% Conifer)", "M-2 (25 PC)"},
  {"Boreal Mixedwood - Green (30% Conifer)", "M-2 (30 PC)"},
  {"Boreal Mixedwood - Green (35% Conifer)", "M-2 (35 PC)"},
  {"Boreal Mixedwood - Green (40% Conifer)", "M-2 (40 PC)"},
  {"Boreal Mixedwood - Green (45% Conifer)", "M-2 (45 PC)"},
  {"Boreal Mixedwood - Green (50% Conifer)", "M-2 (50 PC)"},
  {"Boreal Mixedwood - Green (55% Conifer)", "M-2 (55 PC)"},
  {"Boreal Mixedwood - Green (60% Conifer)", "M-2 (60 PC)"},
  {"Boreal Mixedwood - Green (65% Conifer)", "M-2 (65 PC)"},
  {"Boreal Mixedwood - Green (70% Conifer)", "M-2 (70 PC)"},
  {"Boreal Mixedwood - Green (75% Conifer)", "M-2 (75 PC)"},
  {"Boreal Mixedwood - Green (80% Conifer)", "M-2 (80 PC)"},
  {"Boreal Mixedwood - Green (85% Conifer)", "M-2 (85 PC)"},
  {"Boreal Mixedwood - Green (90% Conifer)", "M-2 (90 PC)"},
  {"Boreal Mixedwood - Green (95% Conifer)", "M-2 (95 PC)"},
  {"Boreal Mixedwood (00% Conifer)", "M-1/M-2 (00 PC)"},
  {"Boreal Mixedwood (05% Conifer)", "M-1/M-2 (05 PC)"},
  {"Boreal Mixedwood (10% Conifer)", "M-1/M-2 (10 PC)"},
  {"Boreal Mixedwood (15% Conifer)", "M-1/M-2 (15 PC)"},
  {"Boreal Mixedwood (20% Conifer)", "M-1/M-2 (20 PC)"},
  {"Boreal Mixedwood (25% Conifer)", "M-1/M-2 (25 PC)"},
  {"Boreal Mixedwood (30% Conifer)", "M-1/M-2 (30 PC)"},
  {"Boreal Mixedwood (35% Conifer)", "M-1/M-2 (35 PC)"},
  {"Boreal Mixedwood (40% Conifer)", "M-1/M-2 (40 PC)"},
  {"Boreal Mixedwood (45% Conifer)", "M-1/M-2 (45 PC)"},
  {"Boreal Mixedwood (50% Conifer)", "M-1/M-2 (50 PC)"},
  {"Boreal Mixedwood (55% Conifer)", "M-1/M-2 (55 PC)"},
  {"Boreal Mixedwood (60% Conifer)", "M-1/M-2 (60 PC)"},
  {"Boreal Mixedwood (65% Conifer)", "M-1/M-2 (65 PC)"},
  {"Boreal Mixedwood (70% Conifer)", "M-1/M-2 (70 PC)"},
  {"Boreal Mixedwood (75% Conifer)", "M-1/M-2 (75 PC)"},
  {"Boreal Mixedwood (80% Conifer)", "M-1/M-2 (80 PC)"},
  {"Boreal Mixedwood (85% Conifer)", "M-1/M-2 (85 PC)"},
  {"Boreal Mixedwood (90% Conifer)", "M-1/M-2 (90 PC)"},
  {"Boreal Mixedwood (95% Conifer)", "M-1/M-2 (95 PC)"},
  {"Dead Balsam Fir Mixedwood - Leafless (00% Dead Fir)", "M-3 (00 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (05% Dead Fir)", "M-3 (05 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (10% Dead Fir)", "M-3 (10 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (15% Dead Fir)", "M-3 (15 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (20% Dead Fir)", "M-3 (20 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (25% Dead Fir)", "M-3 (25 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (30% Dead Fir)", "M-3 (30 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (35% Dead Fir)", "M-3 (35 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (40% Dead Fir)", "M-3 (40 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (45% Dead Fir)", "M-3 (45 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (50% Dead Fir)", "M-3 (50 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (55% Dead Fir)", "M-3 (55 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (60% Dead Fir)", "M-3 (60 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (65% Dead Fir)", "M-3 (65 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (70% Dead Fir)", "M-3 (70 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (75% Dead Fir)", "M-3 (75 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (80% Dead Fir)", "M-3 (80 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (85% Dead Fir)", "M-3 (85 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (90% Dead Fir)", "M-3 (90 PDF)"},
  {"Dead Balsam Fir Mixedwood - Leafless (95% Dead Fir)", "M-3 (95 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (00% Dead Fir)", "M-4 (00 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (05% Dead Fir)", "M-4 (05 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (10% Dead Fir)", "M-4 (10 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (15% Dead Fir)", "M-4 (15 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (20% Dead Fir)", "M-4 (20 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (25% Dead Fir)", "M-4 (25 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (30% Dead Fir)", "M-4 (30 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (35% Dead Fir)", "M-4 (35 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (40% Dead Fir)", "M-4 (40 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (45% Dead Fir)", "M-4 (45 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (50% Dead Fir)", "M-4 (50 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (55% Dead Fir)", "M-4 (55 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (60% Dead Fir)", "M-4 (60 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (65% Dead Fir)", "M-4 (65 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (70% Dead Fir)", "M-4 (70 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (75% Dead Fir)", "M-4 (75 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (80% Dead Fir)", "M-4 (80 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (85% Dead Fir)", "M-4 (85 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (90% Dead Fir)", "M-4 (90 PDF)"},
  {"Dead Balsam Fir Mixedwood - Green (95% Dead Fir)", "M-4 (95 PDF)"},
  {"Dead Balsam Fir Mixedwood (00% Dead Fir)", "M-3/M-4 (00 PDF)"},
  {"Dead Balsam Fir Mixedwood (05% Dead Fir)", "M-3/M-4 (05 PDF)"},
  {"Dead Balsam Fir Mixedwood (10% Dead Fir)", "M-3/M-4 (10 PDF)"},
  {"Dead Balsam Fir Mixedwood (15% Dead Fir)", "M-3/M-4 (15 PDF)"},
  {"Dead Balsam Fir Mixedwood (20% Dead Fir)", "M-3/M-4 (20 PDF)"},
  {"Dead Balsam Fir Mixedwood (25% Dead Fir)", "M-3/M-4 (25 PDF)"},
  {"Dead Balsam Fir Mixedwood (30% Dead Fir)", "M-3/M-4 (30 PDF)"},
  {"Dead Balsam Fir Mixedwood (35% Dead Fir)", "M-3/M-4 (35 PDF)"},
  {"Dead Balsam Fir Mixedwood (40% Dead Fir)", "M-3/M-4 (40 PDF)"},
  {"Dead Balsam Fir Mixedwood (45% Dead Fir)", "M-3/M-4 (45 PDF)"},
  {"Dead Balsam Fir Mixedwood (50% Dead Fir)", "M-3/M-4 (50 PDF)"},
  {"Dead Balsam Fir Mixedwood (55% Dead Fir)", "M-3/M-4 (55 PDF)"},
  {"Dead Balsam Fir Mixedwood (60% Dead Fir)", "M-3/M-4 (60 PDF)"},
  {"Dead Balsam Fir Mixedwood (65% Dead Fir)", "M-3/M-4 (65 PDF)"},
  {"Dead Balsam Fir Mixedwood (70% Dead Fir)", "M-3/M-4 (70 PDF)"},
  {"Dead Balsam Fir Mixedwood (75% Dead Fir)", "M-3/M-4 (75 PDF)"},
  {"Dead Balsam Fir Mixedwood (80% Dead Fir)", "M-3/M-4 (80 PDF)"},
  {"Dead Balsam Fir Mixedwood (85% Dead Fir)", "M-3/M-4 (85 PDF)"},
  {"Dead Balsam Fir Mixedwood (90% Dead Fir)", "M-3/M-4 (90 PDF)"},
  {"Dead Balsam Fir Mixedwood (95% Dead Fir)", "M-3/M-4 (95 PDF)"},
};
// FIX: ensure actual code use in compilation doesn't matter and don't need to be speicified
// manually in sequence
static_assert(0 == INVALID_FUEL_CODE);
static fs::InvalidFuel NULL_FUEL{INVALID_FUEL_CODE, "Non-fuel"};
static fs::InvalidFuel INVALID{1, "Invalid"};
static SimpleFuelC1 C1{2};
static SimpleFuelC2 C2{3};
static SimpleFuelC3 C3{4};
static SimpleFuelC4 C4{5};
static SimpleFuelC5 C5{6};
static SimpleFuelC6 C6{7};
static SimpleFuelC7 C7{8};
static SimpleFuelD1 D1{9};
static SimpleFuelD2 D2{10};
static SimpleFuelO1A O1_A{11};
static SimpleFuelO1B O1_B{12};
static SimpleFuelS1 S1{13};
static SimpleFuelS2 S2{14};
static SimpleFuelS3 S3{15};
static SimpleFuelD1D2 D1_D2{16, &D1, &D2};
static SimpleFuelM1 M1_05{17, "M-1 (05 PC)", 5};
static SimpleFuelM1 M1_10{18, "M-1 (10 PC)", 10};
static SimpleFuelM1 M1_15{19, "M-1 (15 PC)", 15};
static SimpleFuelM1 M1_20{20, "M-1 (20 PC)", 20};
static SimpleFuelM1 M1_25{21, "M-1 (25 PC)", 25};
static SimpleFuelM1 M1_30{22, "M-1 (30 PC)", 30};
static SimpleFuelM1 M1_35{23, "M-1 (35 PC)", 35};
static SimpleFuelM1 M1_40{24, "M-1 (40 PC)", 40};
static SimpleFuelM1 M1_45{25, "M-1 (45 PC)", 45};
static SimpleFuelM1 M1_50{26, "M-1 (50 PC)", 50};
static SimpleFuelM1 M1_55{27, "M-1 (55 PC)", 55};
static SimpleFuelM1 M1_60{28, "M-1 (60 PC)", 60};
static SimpleFuelM1 M1_65{29, "M-1 (65 PC)", 65};
static SimpleFuelM1 M1_70{30, "M-1 (70 PC)", 70};
static SimpleFuelM1 M1_75{31, "M-1 (75 PC)", 75};
static SimpleFuelM1 M1_80{32, "M-1 (80 PC)", 80};
static SimpleFuelM1 M1_85{33, "M-1 (85 PC)", 85};
static SimpleFuelM1 M1_90{34, "M-1 (90 PC)", 90};
static SimpleFuelM1 M1_95{35, "M-1 (95 PC)", 95};
static SimpleFuelM2 M2_05{36, "M-2 (05 PC)", 5};
static SimpleFuelM2 M2_10{37, "M-2 (10 PC)", 10};
static SimpleFuelM2 M2_15{38, "M-2 (15 PC)", 15};
static SimpleFuelM2 M2_20{39, "M-2 (20 PC)", 20};
static SimpleFuelM2 M2_25{40, "M-2 (25 PC)", 25};
static SimpleFuelM2 M2_30{41, "M-2 (30 PC)", 30};
static SimpleFuelM2 M2_35{42, "M-2 (35 PC)", 35};
static SimpleFuelM2 M2_40{43, "M-2 (40 PC)", 40};
static SimpleFuelM2 M2_45{44, "M-2 (45 PC)", 45};
static SimpleFuelM2 M2_50{45, "M-2 (50 PC)", 50};
static SimpleFuelM2 M2_55{46, "M-2 (55 PC)", 55};
static SimpleFuelM2 M2_60{47, "M-2 (60 PC)", 60};
static SimpleFuelM2 M2_65{48, "M-2 (65 PC)", 65};
static SimpleFuelM2 M2_70{49, "M-2 (70 PC)", 70};
static SimpleFuelM2 M2_75{50, "M-2 (75 PC)", 75};
static SimpleFuelM2 M2_80{51, "M-2 (80 PC)", 80};
static SimpleFuelM2 M2_85{52, "M-2 (85 PC)", 85};
static SimpleFuelM2 M2_90{53, "M-2 (90 PC)", 90};
static SimpleFuelM2 M2_95{54, "M-2 (95 PC)", 95};
static SimpleFuelM1M2 M1_M2_05{55, "M-1/M-2 (05 PC)", &M1_05, &M2_05, 5};
static SimpleFuelM1M2 M1_M2_10{56, "M-1/M-2 (10 PC)", &M1_10, &M2_10, 10};
static SimpleFuelM1M2 M1_M2_15{57, "M-1/M-2 (15 PC)", &M1_15, &M2_15, 15};
static SimpleFuelM1M2 M1_M2_20{58, "M-1/M-2 (20 PC)", &M1_20, &M2_20, 20};
static SimpleFuelM1M2 M1_M2_25{59, "M-1/M-2 (25 PC)", &M1_25, &M2_25, 25};
static SimpleFuelM1M2 M1_M2_30{60, "M-1/M-2 (30 PC)", &M1_30, &M2_30, 30};
static SimpleFuelM1M2 M1_M2_35{61, "M-1/M-2 (35 PC)", &M1_35, &M2_35, 35};
static SimpleFuelM1M2 M1_M2_40{62, "M-1/M-2 (40 PC)", &M1_40, &M2_40, 40};
static SimpleFuelM1M2 M1_M2_45{63, "M-1/M-2 (45 PC)", &M1_45, &M2_45, 45};
static SimpleFuelM1M2 M1_M2_50{64, "M-1/M-2 (50 PC)", &M1_50, &M2_50, 50};
static SimpleFuelM1M2 M1_M2_55{65, "M-1/M-2 (55 PC)", &M1_55, &M2_55, 55};
static SimpleFuelM1M2 M1_M2_60{66, "M-1/M-2 (60 PC)", &M1_60, &M2_60, 60};
static SimpleFuelM1M2 M1_M2_65{67, "M-1/M-2 (65 PC)", &M1_65, &M2_65, 65};
static SimpleFuelM1M2 M1_M2_70{68, "M-1/M-2 (70 PC)", &M1_70, &M2_70, 70};
static SimpleFuelM1M2 M1_M2_75{69, "M-1/M-2 (75 PC)", &M1_75, &M2_75, 75};
static SimpleFuelM1M2 M1_M2_80{70, "M-1/M-2 (80 PC)", &M1_80, &M2_80, 80};
static SimpleFuelM1M2 M1_M2_85{71, "M-1/M-2 (85 PC)", &M1_85, &M2_85, 85};
static SimpleFuelM1M2 M1_M2_90{72, "M-1/M-2 (90 PC)", &M1_90, &M2_90, 90};
static SimpleFuelM1M2 M1_M2_95{73, "M-1/M-2 (95 PC)", &M1_95, &M2_95, 95};
static SimpleFuelM3 M3_05{74, "M-3 (05 PDF)", 5};
static SimpleFuelM3 M3_10{75, "M-3 (10 PDF)", 10};
static SimpleFuelM3 M3_15{76, "M-3 (15 PDF)", 15};
static SimpleFuelM3 M3_20{77, "M-3 (20 PDF)", 20};
static SimpleFuelM3 M3_25{78, "M-3 (25 PDF)", 25};
static SimpleFuelM3 M3_30{79, "M-3 (30 PDF)", 30};
static SimpleFuelM3 M3_35{80, "M-3 (35 PDF)", 35};
static SimpleFuelM3 M3_40{81, "M-3 (40 PDF)", 40};
static SimpleFuelM3 M3_45{82, "M-3 (45 PDF)", 45};
static SimpleFuelM3 M3_50{83, "M-3 (50 PDF)", 50};
static SimpleFuelM3 M3_55{84, "M-3 (55 PDF)", 55};
static SimpleFuelM3 M3_60{85, "M-3 (60 PDF)", 60};
static SimpleFuelM3 M3_65{86, "M-3 (65 PDF)", 65};
static SimpleFuelM3 M3_70{87, "M-3 (70 PDF)", 70};
static SimpleFuelM3 M3_75{88, "M-3 (75 PDF)", 75};
static SimpleFuelM3 M3_80{89, "M-3 (80 PDF)", 80};
static SimpleFuelM3 M3_85{90, "M-3 (85 PDF)", 85};
static SimpleFuelM3 M3_90{91, "M-3 (90 PDF)", 90};
static SimpleFuelM3 M3_95{92, "M-3 (95 PDF)", 95};
static SimpleFuelM3 M3_100{93, "M-3 (100 PDF)", 100};
static SimpleFuelM4 M4_05{94, "M-4 (05 PDF)", 5};
static SimpleFuelM4 M4_10{95, "M-4 (10 PDF)", 10};
static SimpleFuelM4 M4_15{96, "M-4 (15 PDF)", 15};
static SimpleFuelM4 M4_20{97, "M-4 (20 PDF)", 20};
static SimpleFuelM4 M4_25{98, "M-4 (25 PDF)", 25};
static SimpleFuelM4 M4_30{99, "M-4 (30 PDF)", 30};
static SimpleFuelM4 M4_35{100, "M-4 (35 PDF)", 35};
static SimpleFuelM4 M4_40{101, "M-4 (40 PDF)", 40};
static SimpleFuelM4 M4_45{102, "M-4 (45 PDF)", 45};
static SimpleFuelM4 M4_50{103, "M-4 (50 PDF)", 50};
static SimpleFuelM4 M4_55{104, "M-4 (55 PDF)", 55};
static SimpleFuelM4 M4_60{105, "M-4 (60 PDF)", 60};
static SimpleFuelM4 M4_65{106, "M-4 (65 PDF)", 65};
static SimpleFuelM4 M4_70{107, "M-4 (70 PDF)", 70};
static SimpleFuelM4 M4_75{108, "M-4 (75 PDF)", 75};
static SimpleFuelM4 M4_80{109, "M-4 (80 PDF)", 80};
static SimpleFuelM4 M4_85{110, "M-4 (85 PDF)", 85};
static SimpleFuelM4 M4_90{111, "M-4 (90 PDF)", 90};
static SimpleFuelM4 M4_95{112, "M-4 (95 PDF)", 95};
static SimpleFuelM4 M4_100{113, "M-4 (100 PDF)", 100};
static SimpleFuelM3M4 M3_M4_05{114, "M-3/M-4 (05 PDF)", &M3_05, &M4_05, 5};
static SimpleFuelM3M4 M3_M4_10{115, "M-3/M-4 (10 PDF)", &M3_10, &M4_10, 10};
static SimpleFuelM3M4 M3_M4_15{116, "M-3/M-4 (15 PDF)", &M3_15, &M4_15, 15};
static SimpleFuelM3M4 M3_M4_20{117, "M-3/M-4 (20 PDF)", &M3_20, &M4_20, 20};
static SimpleFuelM3M4 M3_M4_25{118, "M-3/M-4 (25 PDF)", &M3_25, &M4_25, 25};
static SimpleFuelM3M4 M3_M4_30{119, "M-3/M-4 (30 PDF)", &M3_30, &M4_30, 30};
static SimpleFuelM3M4 M3_M4_35{120, "M-3/M-4 (35 PDF)", &M3_35, &M4_35, 35};
static SimpleFuelM3M4 M3_M4_40{121, "M-3/M-4 (40 PDF)", &M3_40, &M4_40, 40};
static SimpleFuelM3M4 M3_M4_45{122, "M-3/M-4 (45 PDF)", &M3_45, &M4_45, 45};
static SimpleFuelM3M4 M3_M4_50{123, "M-3/M-4 (50 PDF)", &M3_50, &M4_50, 50};
static SimpleFuelM3M4 M3_M4_55{124, "M-3/M-4 (55 PDF)", &M3_55, &M4_55, 55};
static SimpleFuelM3M4 M3_M4_60{125, "M-3/M-4 (60 PDF)", &M3_60, &M4_60, 60};
static SimpleFuelM3M4 M3_M4_65{126, "M-3/M-4 (65 PDF)", &M3_65, &M4_65, 65};
static SimpleFuelM3M4 M3_M4_70{127, "M-3/M-4 (70 PDF)", &M3_70, &M4_70, 70};
static SimpleFuelM3M4 M3_M4_75{128, "M-3/M-4 (75 PDF)", &M3_75, &M4_75, 75};
static SimpleFuelM3M4 M3_M4_80{129, "M-3/M-4 (80 PDF)", &M3_80, &M4_80, 80};
static SimpleFuelM3M4 M3_M4_85{130, "M-3/M-4 (85 PDF)", &M3_85, &M4_85, 85};
static SimpleFuelM3M4 M3_M4_90{131, "M-3/M-4 (90 PDF)", &M3_90, &M4_90, 90};
static SimpleFuelM3M4 M3_M4_95{132, "M-3/M-4 (95 PDF)", &M3_95, &M4_95, 95};
static SimpleFuelM3M4 M3_M4_100{133, "M-3/M-4 (100 PDF)", &M3_100, &M4_100, 100};
static SimpleFuelM1 M1_00{134, "M-1 (00 PC)", 0};
static SimpleFuelM2 M2_00{135, "M-2 (00 PC)", 0};
static SimpleFuelM1M2 M1_M2_00{136, "M-1/M-2 (00 PC)", &M1_00, &M2_00, 0};
static SimpleFuelM3 M3_00{137, "M-3 (00 PDF)", 0};
static SimpleFuelM4 M4_00{138, "M-4 (00 PDF)", 0};
static SimpleFuelM3M4 M3_M4_00{139, "M-3/M-4 (00 PDF)", &M3_00, &M4_00, 0};
static SimpleFuelVariable O1{140, "O-1", &O1_A, &O1_B};
/**
 * \brief Implementation class for SimpleFuelLookup
 */
class SimpleFuelLookupImpl
{
public:
  // do it this way so the entire array is filled with a single invalid fuel
  /**
   * \brief Construct by reading from a file
   * \param filename File to read from. Uses .lut format from Prometheus
   */
  explicit SimpleFuelLookupImpl(const char* filename)
    : fuel_types_(new array<const FuelType*, numeric_limits<FuelSize>::max()>{&INVALID})
  {
    for (auto i : SimpleFuelLookup::Fuels)
    {
      emplaceFuel(i);
    }
    // HACK: use offset from base fuel type
    const auto pc = Settings::defaultPercentConifer();
    logging::check_fatal(
      0 >= pc || 100 <= pc || (pc % 5) != 0, "Invalid default percent conifer (%d)", pc
    );
    const auto pc_offset = (static_cast<size_t>(pc) / 5) - 1;
    emplaceFuel("M-1", SimpleFuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M1_05)));
    emplaceFuel("M-2", SimpleFuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M2_05)));
    emplaceFuel("M-1/M-2", SimpleFuelLookup::Fuels.at(pc_offset + FuelType::safeCode(&M1_M2_05)));
    // 0 PC/PDF makes these effectively D1/D2 because of how the equations work
    emplaceFuel("M-1 (00 PC)", &D1_D2);
    emplaceFuel("M-2 (00 PC)", &D1_D2);
    emplaceFuel("M-1/M-2 (00 PC)", &D1_D2);
    emplaceFuel("M-3 (00 PDF)", &D1_D2);
    emplaceFuel("M-4 (00 PDF)", &D1_D2);
    emplaceFuel("M-3/M-4 (00 PDF)", &D1_D2);
    const auto pdf = Settings::defaultPercentDeadFir();
    logging::check_fatal(
      0 > pdf || 100 < pdf || (pdf % 5) != 0, "Invalid default percent dead fir (%d)", pdf
    );
    const auto pdf_offset = static_cast<size_t>(pdf) / 5 - 1;
    emplaceFuel("M-3", SimpleFuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M3_05)));
    emplaceFuel("M-4", SimpleFuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M4_05)));
    emplaceFuel("M-3/M-4", SimpleFuelLookup::Fuels.at(pdf_offset + FuelType::safeCode(&M3_M4_05)));
    ifstream in;
    in.open(filename);
    bool read_ok = false;
    if (in.is_open())
    {
      string str;
      logging::info("Reading fuel lookup table from '%s'", filename);
      // read header line
      getline(in, str);
      while (getline(in, str))
      {
        istringstream iss(str);
        if (getline(iss, str, ','))
        {
          // grid_value
          const auto value = static_cast<FuelSize>(stoi(str));
          // export_value
          getline(iss, str, ',');
          // descriptive_name
          getline(iss, str, ',');
          const auto name = str;
          // fuel_type
          getline(iss, str, ',');
          const auto fuel = str;
          logging::debug("Fuel %s has code %d", fuel.c_str(), value);
          const auto by_name = fuel_by_name_.find(str);
          if (by_name != fuel_by_name_.end())
          {
            const auto fuel_obj = (*by_name).second;
            fuel_types_->at(value) = fuel_obj;
            // HACK: can't figure out how to compare to fuel from .find() result so keep using .at()
            // for now
            if (!DEFAULT_TYPES.contains(name) || DEFAULT_TYPES.at(name) != fuel
                || "Not Available" == name || "Non-fuel" == name || "Unclassified" == name
                || "Urban" == name || "Unknown" == name || "Vegetated Non-Fuel" == name)
            {
              logging::note(
                "Fuel (%d, '%s') is treated like '%s' with internal code %d",
                value,
                name.c_str(),
                fuel.c_str(),
                fuel_obj->code()
              );
            }
            else
            {
              logging::debug(
                "Fuel (%d, '%s') is treated like '%s' with internal code %d",
                value,
                name.c_str(),
                fuel.c_str(),
                fuel_obj->code()
              );
            }
            auto emplaced = fuel_grid_codes_.try_emplace(fuel_obj, value);
            if (!emplaced.second)
            {
              logging::debug(
                "Fuel (%d, '%s') is treated like '%s' with internal code %d and tried to replace value %d for %d",
                value,
                name.c_str(),
                fuel.c_str(),
                fuel_obj->code(),
                value,
                emplaced.first->second
              );
            }
            fuel_good_values_[value].push_back(str);
          }
          else
          {
            logging::warning("Unknown fuel type '%s' in fuel lookup table", str.c_str());
            fuel_bad_values_[value].push_back(str);
          }
          read_ok = true;
        }
      }
      in.close();
    }
    if (!read_ok)
    {
      logging::fatal("Unable to read file %s", filename);
    }
  }
  ~SimpleFuelLookupImpl() { delete fuel_types_; }
  /**
   * \brief Put fuel into lookup table based on name
   * \param fuel FuelType to put into lookup table based on its name
   */
  void emplaceFuel(const FuelType* fuel) { emplaceFuel(FuelType::safeName(fuel), fuel); }
  /**
   * \brief Put fuel into lookup table based on name
   * \param name Name to use for FuelType in lookup table
   * \param fuel FuelType to put into lookup table
   */
  void emplaceFuel(const string_view name, const FuelType* fuel)
  {
    fuel_by_name_.emplace(name, fuel);
    const auto simple_name = simplify_name(fuel->name());
    logging::verbose(
      "'%s' being registered as '%s' with simplified name '%s'",
      fuel->name(),
      string(name).c_str(),
      string(simple_name).c_str()
    );
    fuel_by_simplified_name_.emplace(simple_name, fuel);
  }
  /**
   * \brief Create a set of all FuelTypes used in this lookup table
   * \return Set of all FuelTypes used in this lookup table
   */
  set<const FuelType*> usedFuels() const
  {
    set<const FuelType*> result{};
    for (const auto& kv : used_by_name_)
    {
      if (kv.second)
      {
        result.insert(fuel_by_name_.at(kv.first));
      }
    }
    return result;
  }
  SimpleFuelLookupImpl(const SimpleFuelLookupImpl& rhs) = delete;
  SimpleFuelLookupImpl(SimpleFuelLookupImpl&& rhs) = delete;
  SimpleFuelLookupImpl& operator=(const SimpleFuelLookupImpl& rhs) = delete;
  SimpleFuelLookupImpl& operator=(SimpleFuelLookupImpl&& rhs) = delete;
  /**
   * \brief Look up a FuelType based on the given code
   * \param value Value to use for lookup
   * \param nodata Value that represents no data
   * \return FuelType based on the given code
   */
  const FuelType* codeToFuel(const FuelSize value, const FuelSize nodata) const
  {
    // NOTE: this should be looking things up based on the .lut codes
    if (nodata == value)
    {
      return nullptr;
    }
    const auto result = fuel_types_->at(static_cast<size_t>(value));
    if (nullptr != result)
    {
      // use [] for insert or change
      used_by_name_[FuelType::safeName(result)] = true;
    }
    return result;
  }
  /**
   * \brief List all fuels and their codes
   */
  void listFuels() const
  {
    for (const auto& kv : fuel_good_values_)
    {
      for (const auto& name : kv.second)
      {
        logging::note("%ld => %s", kv.first, name.c_str());
      }
    }
  }
  /**
   * \brief Look up the original grid code for a FuelType
   * \param value Value to use for lookup
   * \return Original grid code for the FuelType
   */
  FuelSize fuelToCode(const FuelType* const value) const
  {
    if (nullptr == value)
    {
      // HACK: for now assume 0 is always the invalid fuel type
      return INVALID_FUEL_CODE;
    }
    const auto seek = fuel_grid_codes_.find(value);
    if (seek != fuel_grid_codes_.end())
    {
      return seek->second;
    }
    logging::warning(
      "Invalid FuelType lookup: (%s, %ld) was never used in grid with %ld fuel codes defined",
      value->name(),
      value->code(),
      fuel_grid_codes_.size()
    );
    throw runtime_error("Converting fuel that wasn't in input grid to code");
    return 0;
  }
  /**
   * \brief Look up a FuelType based on the given name
   * \param name Name of the fuel to find
   * \return FuelType based on the given name
   */
  const FuelType* byName(const string_view name) const
  {
    const auto seek = fuel_by_name_.find(name);
    if (seek != fuel_by_name_.end())
    {
      return seek->second;
    }
    return nullptr;
  }
  /**
   * \brief Look up a FuelType based on the given simplified name
   * \param name Simplified name of the fuel to find
   * \return FuelType based on the given name
   */
  const FuelType* bySimplifiedName(const string_view name) const
  {
    const auto seek = fuel_by_simplified_name_.find(name);
    if (seek != fuel_by_simplified_name_.end())
    {
      return seek->second;
    }
    return nullptr;
  }

private:
  /**
   * \brief Array of all possible fuel types
   */
  array<const FuelType*, numeric_limits<FuelSize>::max()>* fuel_types_{nullptr};
  /**
   * \brief Map of FuelType to (first) original grid value
   */
  unordered_map<const FuelType*, FuelSize> fuel_grid_codes_{};
  /**
   * \brief Map of fuel name to FuelType
   */
  string_map<const FuelType*> fuel_by_name_{};
  /**
   * \brief Map of simplified fuel name to FuelType
   */
  string_map<const FuelType*> fuel_by_simplified_name_{};
  /**
   * \brief Map of fuel name to whether or not it is used in this simulation
   */
  mutable string_map<bool> used_by_name_{};
  /**
   * \brief Codes from input .lut that were for fuel types that are implemented
   */
  unordered_map<FuelSize, vector<string>> fuel_good_values_{};
  /**
   * \brief Codes from input .lut that were for fuel types that are not implemented
   */
  unordered_map<FuelSize, vector<string>> fuel_bad_values_{};
};
const FuelType* SimpleFuelLookup::codeToFuel(const FuelSize value, const FuelSize nodata) const
{
  return impl_->codeToFuel(value, nodata);
}
void SimpleFuelLookup::listFuels() const { impl_->listFuels(); }
FuelSize SimpleFuelLookup::fuelToCode(const FuelType* const value) const
{
  return impl_->fuelToCode(value);
}
const FuelType* SimpleFuelLookup::operator()(const FuelSize value, const FuelSize nodata) const
{
  return codeToFuel(value, nodata);
}
set<const FuelType*> SimpleFuelLookup::usedFuels() const { return impl_->usedFuels(); }
const FuelType* SimpleFuelLookup::byName(const string_view name) const
{
  return impl_->byName(name);
}
const FuelType* SimpleFuelLookup::bySimplifiedName(const string_view name) const
{
  return impl_->bySimplifiedName(name);
}
SimpleFuelLookup::SimpleFuelLookup(const char* filename)
  : impl_(make_shared<SimpleFuelLookupImpl>(filename))
{ }
const array<const FuelType*, NUMBER_OF_FUELS> SimpleFuelLookup::Fuels{
  &NULL_FUEL, &INVALID,  &C1,       &C2,        &C3,       &C4,       &C5,       &C6,
  &C7,        &D1,       &D2,       &O1_A,      &O1_B,     &S1,       &S2,       &S3,
  &D1_D2,     &M1_05,    &M1_10,    &M1_15,     &M1_20,    &M1_25,    &M1_30,    &M1_35,
  &M1_40,     &M1_45,    &M1_50,    &M1_55,     &M1_60,    &M1_65,    &M1_70,    &M1_75,
  &M1_80,     &M1_85,    &M1_90,    &M1_95,     &M2_05,    &M2_10,    &M2_15,    &M2_20,
  &M2_25,     &M2_30,    &M2_35,    &M2_40,     &M2_45,    &M2_50,    &M2_55,    &M2_60,
  &M2_65,     &M2_70,    &M2_75,    &M2_80,     &M2_85,    &M2_90,    &M2_95,    &M1_M2_05,
  &M1_M2_10,  &M1_M2_15, &M1_M2_20, &M1_M2_25,  &M1_M2_30, &M1_M2_35, &M1_M2_40, &M1_M2_45,
  &M1_M2_50,  &M1_M2_55, &M1_M2_60, &M1_M2_65,  &M1_M2_70, &M1_M2_75, &M1_M2_80, &M1_M2_85,
  &M1_M2_90,  &M1_M2_95, &M3_05,    &M3_10,     &M3_15,    &M3_20,    &M3_25,    &M3_30,
  &M3_35,     &M3_40,    &M3_45,    &M3_50,     &M3_55,    &M3_60,    &M3_65,    &M3_70,
  &M3_75,     &M3_80,    &M3_85,    &M3_90,     &M3_95,    &M3_100,   &M4_05,    &M4_10,
  &M4_15,     &M4_20,    &M4_25,    &M4_30,     &M4_35,    &M4_40,    &M4_45,    &M4_50,
  &M4_55,     &M4_60,    &M4_65,    &M4_70,     &M4_75,    &M4_80,    &M4_85,    &M4_90,
  &M4_95,     &M4_100,   &M3_M4_00, &M3_M4_05,  &M3_M4_10, &M3_M4_15, &M3_M4_20, &M3_M4_25,
  &M3_M4_30,  &M3_M4_35, &M3_M4_40, &M3_M4_45,  &M3_M4_50, &M3_M4_55, &M3_M4_60, &M3_M4_65,
  &M3_M4_70,  &M3_M4_75, &M3_M4_80, &M3_M4_85,  &M3_M4_90, &M3_M4_95, &M1_00,    &M2_00,
  &M1_M2_00,  &M3_00,    &M4_00,    &M3_M4_100, &O1,
};
MathSize compare_by_season(
  const SimpleFuelVariable& fuel,
  const function<MathSize(const SimpleFuelType&)>& fct
)
{
  // HACK: no way to tell which is which, so let's assume they have to be the same??
  // HACK: use a function so that DEBUG section doesn't get out of sync
  const auto for_spring = fct(*fuel.spring());
#ifdef DEBUG_FUEL_VARIABLE
  const auto for_summer = fct(*fuel.summer());
  logging::check_fatal(for_spring != for_summer, "Expected spring and summer cfb to be identical");
#endif
  return for_spring;
}
}
namespace fs::testing
{
// FIX: this was used to compare to the old template version, but doesn't work now
//      left for reference for now so idea could be used for more tests
using fs::fuel::FuelBase;
using fs::fuel::FuelType;
// check %, so 1 decimal is fine
static constexpr MathSize EPSILON = 1e-1f;
auto check_equal(const auto& lhs, const auto& rhs, const char* name)
{
  logging::check_equal_verbose(logging::LOG_DEBUG, lhs, rhs, name);
}
template <class TypeA, class TypeB>
int compare_fuel_valid(const string name, const TypeA& a, const TypeB& b, const char* msg = "")
{
  logging::info("Checking %s: %s", name.c_str(), msg);
  //
  // FuelType
  //
  const auto a0 = a.isValid();
  const auto b0 = b.isValid();
  check_equal(a0, b0, "isValid");
  check_equal(a.name(), b.name(), "name");
  check_equal(a.code(), b.code(), "code");
  return 0;
}
// use vectors so FuelCompareOptions can assign any of these directly
static const auto BUI_RANGE_DEFAULTS = range(0.0, 300.0, 7.0);
static const auto DC_RANGE_DEFAULTS = range(0.0, 1000.0, 7.0);
static const vector<MathSize> DC_VALUES_GRASS{0, 10, 50, 100, 400, 499, 500, 501, 1000};
static const auto RANGE_MC_FRACTION = range(-1, 3, 0.0001);
static const auto RANGE_WIND_SPEED = range(0, 200, 0.01);
static const auto RANGE_BUI_EFFECT = range(-1, 300, 0.01);
static const auto RANGE_CFB = range(0, 100, 0.01);
static const auto RANGE_ISI = range(0, 250, 0.1);
struct FuelCompareOptions
{
  // HACK: can't figure out how to refer to a range so just use vectors
  // single values for defaults
  const vector<int> nd_values{80};
  const vector<MathSize> bui_values{60};
  const vector<MathSize> dc_values{200};
};
static const FuelCompareOptions FUEL_COMPARE_DEFAULT{};
static const FuelCompareOptions FUEL_COMPARE_DECIDUOUS{.bui_values = BUI_RANGE_DEFAULTS};
int compare_spread(
  const string name,
  const FuelType* a,
  const FuelType* b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  static const DurationSize TIME{INVALID_TIME};
  // HACK: 0.0 is causing offsets to be generated in grass
  static const MathSize MIN_ROS{1E-6};
  static const MathSize CELL_SIZE{100.0};
  static const vector<SlopeSize> slopes{0, 15, 30};
  static const vector<AspectSize> aspects{0, 15, 25, 35, 45, 55};
  // duration is in minutes
  auto show_offset = [=](const ROSOffset& o) {
    const auto intensity{std::get<0>(o)};
    const auto ros{std::get<1>(o)};
    const auto direction{std::get<2>(o)};
    // offsets are in fraction of a cell per minute
    const auto offset{std::get<3>(o)};
    printf(
      " (%d kW/m; %0.6f m/min @%03d == (%g, %g))",
      intensity,
      ros,
      direction.asDegreesSize(),
      offset.first * CELL_SIZE,
      offset.second * CELL_SIZE
    );
  };
  static const auto FFMC_RANGE = range(0.0, 101.0, 1.0);
  static const auto DMC_SMALL_RANGE = range(0.0, 200.0, 47.0);
  static const auto DMC_RANGE = range(0.0, 200.0, 3.0);
  size_t count_comparisons{0};
  logging::debug(
    "compare_spread(%s, %s, %s)", name.c_str(), FuelType::safeName(a), FuelType::safeName(b)
  );
  // HACK: use less options for things with nd values (just grass?)
  const auto dmc_values = options.nd_values.size() > 1 ? DMC_SMALL_RANGE : DMC_RANGE;
  for (auto nd : options.nd_values)
  {
    logging::extensive("nd %d", nd);
    for (auto ffmc : FFMC_RANGE)
    {
      logging::extensive("ffmc %f", ffmc);
      for (auto dmc : dmc_values)
      {
        logging::extensive("dmc %f", dmc);
        // for (auto bui : options.bui_values)
        {
          for (auto dc : options.dc_values)
          // for (auto dc : range(0.0, std::ranges::max(options.dc_values), 17.0))
          {
            logging::extensive("dc %f", dc);
            const FwiWeather weather{Weather::Invalid(), Ffmc{ffmc}, Dmc{dmc}, Dc{dc}};
            for (auto slope : slopes)
            {
              logging::extensive("slope %d", slope);
              for (auto aspect : aspects)
              {
                ++count_comparisons;
                logging::extensive("aspect %d", aspect);
                // HACK: this constructor ignores fuel part of this
                const auto key = Cell::key(Cell::hashCell(slope, aspect, 0));
                const SpreadInfo spread_a{a, TIME, MIN_ROS, CELL_SIZE, key, nd, &weather, &weather};
                const SpreadInfo spread_b{b, TIME, MIN_ROS, CELL_SIZE, key, nd, &weather, &weather};
                const auto offsets_a = spread_a.offsets();
                const auto offsets_b = spread_b.offsets();
                const auto head_ros = spread_a.headRos();
                static constexpr MathSize ROS_MINIMAL{1.0};
                logging::verbose(
                  "compare_spread() [%ld] %sspreading for ffmc:%f; dmc:%f; dc:%f; nd:%d; slope:%d; aspect: %d",
                  count_comparisons,
                  spread_a.isNotSpreading() ? "not "
                  : head_ros < ROS_MINIMAL  ? "minimal "
                                            : "",
                  ffmc,
                  dmc,
                  dc,
                  nd,
                  slope,
                  aspect
                );
                const auto show_offsets =
                  logging::Log::getLogLevel() <= logging::LOG_VERBOSE && head_ros >= ROS_MINIMAL;
                if (offsets_a.size() != offsets_b.size())
                {
                  logging::error(
                    "compare_spread() size failed for name: %s; ffmc:%f; dmc:%f; dc:%f; nd:%d; slope:%d; aspect: %d",
                    name.c_str(),
                    ffmc,
                    dmc,
                    dc,
                    nd,
                    slope,
                    aspect
                  );
                  if (offsets_a.size() < offsets_b.size())
                  {
                    logging::error("compare_spread() size == -1");
                    return -1;
                  }
                  if (offsets_a.size() > offsets_b.size())
                  {
                    logging::error("compare_spread() size == 1");
                    return 1;
                  }
                }
                if (show_offsets)
                {
                  printf("Offsets are: [");
                }
                for (size_t i = 0; i < offsets_a.size(); ++i)
                {
                  const auto pt_a{offsets_a.at(i)};
                  const auto pt_b{offsets_b.at(i)};
                  if (show_offsets)
                  {
                    show_offset(pt_a);
                  }
                  if (auto cmp_pt = pt_a <=> pt_b; std::weak_ordering::equivalent != cmp_pt)
                  {
                    if (!show_offsets)
                    {
                      show_offset(pt_a);
                    }
                    printf(" != ");
                    show_offset(pt_b);
                    printf("\n");
                    logging::error(
                      "compare_spread() pt failed for name: %s; ffmc:%f; dmc:%f; dc:%f; nd:%d; slope:%d; aspect: %d",
                      name.c_str(),
                      ffmc,
                      dmc,
                      dc,
                      nd,
                      slope,
                      aspect
                    );
                    if (std::weak_ordering::less == cmp_pt)
                    {
                      logging::error("compare_spread() pt == -1");
                      return -1;
                    }
                    if (std::weak_ordering::greater == cmp_pt)
                    {
                      logging::error("compare_spread() pt == 1");
                      return 1;
                    }
                  }
                }
                if (show_offsets)
                {
                  printf("]\n");
                }
              }
            }
          }
        }
      }
    }
  }
  logging::debug("compare_spread() == 0 with %ld comparisons", count_comparisons);
  return 0;
}
template <class TypeA, class TypeB>
int compare_fuel_basic(
  const string name,
  const TypeA& a,
  const TypeB& b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  const auto n_nd = options.nd_values.size();
  const auto n_bui = options.bui_values.size();
  const auto n_dc = options.dc_values.size();
  const auto n_isi = RANGE_ISI.size();
  static const auto n_mc_fraction = RANGE_MC_FRACTION.size();
  static const auto n_wind_speed = RANGE_WIND_SPEED.size();
  static const auto n_bui_effect = RANGE_BUI_EFFECT.size();
  static const auto n_cfb = RANGE_CFB.size();
  const auto n_loop = n_nd * n_bui * n_dc * n_isi;
  const auto n_total = n_loop + n_mc_fraction + n_wind_speed + n_bui_effect + n_cfb;
  const auto msg = std::format(
    "(({} nds X {} buis X {} dcs X {} isis) + {} mc_fractions + {} wind_speeds' + {} bui_effects + {} cfbs = {} combinations",
    n_nd,
    n_bui,
    n_dc,
    n_isi,
    n_mc_fraction,
    n_wind_speed,
    n_bui_effect,
    n_cfb,
    n_total
  );
  if (const auto cmp = compare_fuel_valid(name, a, b, msg.c_str()); 0 != cmp)
  {
    return cmp;
  }
  //
  // FuelType
  //
  // check_equal(a.isValid(), b.isValid(), "isValid");
  check_equal(FuelType::safeCode(&a), FuelType::safeCode(&b), "safeCode");
  check_equal(FuelType::safeName(&a), FuelType::safeName(&b), "safeName");
  // static constexpr MathSize criticalRos(const MathSize sfc, const MathSize csi)
  // static constexpr bool isCrown(const MathSize csi, const MathSize sfi)
  check_equal(a.cfl(), b.cfl(), "cfl");
  check_equal(a.canCrown(), b.canCrown(), "canCrown");
  // MathSize grass_curing(const int, const FwiWeather&) const
  check_equal(a.cbh(), b.cbh(), "cbh");
  // MathSize crownFractionBurned(MathSize rss, MathSize rso) const noexcept
  check_range(
    "probabilityPeat()",
    "mc_fraction",
    [&](const auto& v) { return a.probabilityPeat(v); },
    [&](const auto& v) { return b.probabilityPeat(v); },
    EPSILON,
    RANGE_MC_FRACTION
  );
  // ThresholdSize survivalProbability(const FwiWeather& wx) const noexcept
  check_range(
    "buiEffect()",
    "bui",
    [&](const auto& v) { return a.buiEffect(v); },
    [&](const auto& v) { return b.buiEffect(v); },
    EPSILON,
    RANGE_BUI_EFFECT
  );
  check_range(
    "crownConsumption()",
    "cfb",
    [&](const auto& v) { return a.crownConsumption(v); },
    [&](const auto& v) { return b.crownConsumption(v); },
    EPSILON,
    RANGE_CFB
  );
  // MathSize calculateRos(int nd, const FwiWeather& wx, MathSize isi) const
  // need to check breakpoints
  // - BUI 80 (D2)
  // - DC 500 (O1)
  // FIX: use some weird increments to do less but not always have __0.0
  for (auto nd : options.nd_values)
  {
    for (auto bui : options.bui_values)
    {
      // logging::verbose("bui %f", bui);
      // for (auto dc : range(0.0, 2000.0, 7.0))
      for (auto dc : options.dc_values)
      {
        // logging::verbose("dc %f", dc);
        const FwiWeather wx{
          Weather::Zero(), Ffmc::Zero(), Dmc::Zero(), Dc{dc}, Isi::Zero(), Bui{bui}, Fwi::Zero()
        };
        const string msg = logging::Log::getLogLevel() >= logging::LOG_VERBOSE
                           ? std::format("calculateRos(nd={}, bui={}, dc={})", nd, bui, dc)
                           : "calculateRos()";
        check_range(
          msg.c_str(),
          "isi",
          [&](const auto& v) { return a.calculateRos(nd, wx, v); },
          [&](const auto& v) { return b.calculateRos(nd, wx, v); },
          EPSILON,
          RANGE_ISI
        );
      }
    }
  }
  // MathSize calculateIsf(const SpreadInfo& spread, MathSize isi)
  // MathSize surfaceFuelConsumption(const SpreadInfo& spread) const
  check_range(
    "lengthToBreadth()",
    "ws",
    [&](const auto& v) { return a.lengthToBreadth(v); },
    [&](const auto& v) { return b.lengthToBreadth(v); },
    EPSILON,
    RANGE_WIND_SPEED
  );
  // MathSize finalRos(const SpreadInfo& spread, MathSize isi, MathSize cfb, MathSize rss) const
  // MathSize criticalSurfaceIntensity(const SpreadInfo& spread) const
  // check_equal(a.name(), b.name(), "name");
  // check_equal(a.code(), b.code(), "code");
  // return 0;
  return compare_spread(
    name, static_cast<const FuelType*>(&a), static_cast<const FuelType*>(&b), options
  );
}
template <class TypeA, class TypeB>
int compare_fuel(
  const string name,
  const TypeA& a,
  const TypeB& b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  if (const auto cmp = compare_fuel_basic(name, a, b, options); 0 != cmp)
  {
    return cmp;
  }
  //
  // FuelBase
  //
  check_equal(a.bulkDensity(), b.bulkDensity(), "bulkDensity");
  check_equal(a.inorganicPercent(), b.inorganicPercent(), "inorganicPercent");
  check_equal(a.duffDepth(), b.duffDepth(), "duffDepth");
  testing::compare_duff("duffDmcType", *a.duffDmcType(), *b.duffDmcType(), logging::LOG_DEBUG);
  testing::compare_duff("duffFfmcType", *a.duffFfmcType(), *b.duffFfmcType(), logging::LOG_DEBUG);
  check_equal(a.ffmcRatio(), b.ffmcRatio(), "ffmcRatio");
  check_equal(a.dmcRatio(), b.dmcRatio(), "dmcRatio");
  //
  // StandardFuel
  //
  check_range(
    "rosBasic()",
    "isi",
    [&](const auto& v) { return a.rosBasic(v); },
    [&](const auto& v) { return b.rosBasic(v); },
    EPSILON,
    RANGE_ISI
  );
  // MathSize limitIsf(const MathSize mu, const MathSize rsf) const noexcept
  check_equal(a.bui0(), b.bui0(), "bui0");
  check_equal(a.a(), b.a(), "a");
  check_equal(a.negB(), b.negB(), "negB");
  check_equal(a.c(), b.c(), "c");
  // static constexpr MathSize crownRateOfSpread(const MathSize isi, const MathSize fmc)
  // noexcept
  check_equal(a.logQ(), b.logQ(), "logQ");
  return 0;
}
template <class TypeA, class TypeB>
int compare_fuel_variable(
  const string name,
  const TypeA& a,
  const TypeB& b,
  const FuelCompareOptions options = FUEL_COMPARE_DEFAULT
)
{
  assert(a.summer() != a.spring());
  assert(b.summer() != b.spring());
  // FIX: calling functions of FuelVariable should throw, but don't bother checking that
  if (const auto cmp = compare_fuel_basic(name, *a.summer(), *b.summer(), options); 0 != cmp)
  {
    return cmp;
  }
  return compare_fuel_basic(name, *a.spring(), *b.spring(), options);
}
// static constexpr FuelCompareOptions FUEL_COMPARE_DECIDUOUS{.dc_values = DC_DEFAULT_SINGLE};
vector<int> find_nd_values()
{
  // CHECK: FIX: how are nd values 400+?
  set<int> nd_ref_values{};
  set<int> nd_values{};
  static constexpr MathSize BOUNDS_CANADA_LAT_MIN = 41;
  static constexpr MathSize BOUNDS_CANADA_LAT_MAX = 84;
  static constexpr MathSize BOUNDS_CANADA_LON_MIN = -141;
  static constexpr MathSize BOUNDS_CANADA_LON_MAX = -52;
  // FIX: use some weird increments to do less but not always have __.0
  static constexpr MathSize DEGREE_INCREMENT = 0.3;
  // static constexpr MathSize ELEVATION_CANADA_MAX = 5959;
  static constexpr MathSize ELEVATION_EARTH_MIN = -418;
  static constexpr MathSize ELEVATION_EARTH_MAX = 8848;
  static constexpr MathSize ELEVATION_INCREMENT = 100;
  // - nd for different latitudes
  //   - elevation 0
  // const auto latitudes = range(-90.0, 90.0, DEGREE_INCREMENT);
  const auto latitudes = range(BOUNDS_CANADA_LAT_MIN, BOUNDS_CANADA_LAT_MAX, DEGREE_INCREMENT);
  // const auto longitudes = range(-180.0, 180.0, DEGREE_INCREMENT);
  const auto longitudes = range(BOUNDS_CANADA_LON_MIN, BOUNDS_CANADA_LON_MAX, DEGREE_INCREMENT);
  const auto elevations = range(ELEVATION_EARTH_MIN, ELEVATION_EARTH_MAX, ELEVATION_INCREMENT);
  logging::info(
    "Checking (%ld latitudes X %ld longitudes X %ld elevations) = %ld combinations",
    latitudes.size(),
    longitudes.size(),
    elevations.size(),
    latitudes.size() * longitudes.size() * elevations.size()
  );
  for (auto latitude : latitudes)
  {
    for (auto longitude : longitudes)
    {
      for (auto elevation : elevations)
      {
        const Point pt{latitude, longitude};
        const auto nd_ref = calculate_nd_ref_for_point(elevation, pt);
        nd_ref_values.emplace(nd_ref);
        logging::extensive(
          "now have %ld values for nd: %g, %g, %g gives nd_ref %d",
          nd_ref_values.size(),
          latitude,
          longitude,
          elevation,
          nd_ref
        );
      }
    }
  }
  for (int day : range_int(0, 366, 1))
  {
    for (auto nd_ref : nd_ref_values)
    {
      logging::extensive("jd %d", day);
      // from calculate_nd_for_point(const Day day, const int elevation, const Point& point)
      const auto nd = static_cast<int>(abs(day - nd_ref));
      nd_values.emplace(nd);
    }
  }
  auto min_nd = std::numeric_limits<int>::max();
  auto max_nd = std::numeric_limits<int>::min();
  for (auto nd : nd_values)
  {
    min_nd = min(min_nd, nd);
    max_nd = max(max_nd, nd);
  }
  const bool is_consecutive{static_cast<size_t>(max_nd - min_nd + 1) == nd_values.size()};
  logging::info(
    "Have %ld nd values between %d and %d that are %s",
    nd_values.size(),
    min_nd,
    max_nd,
    is_consecutive ? "consecutive" : "non-consecutive"
  );
  return {nd_values.begin(), nd_values.end()};
}
int test_fbp(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  logging::info("Testing FBP");
  const auto nd_all_values = find_nd_values();
  // HACK: initialize here so nd_all_values is set
  static const FuelCompareOptions FUEL_COMPARE_GRASS{
    .nd_values = nd_all_values, .dc_values = DC_VALUES_GRASS
  };
  // for (size_t i = 0; i < FuelLookup::Fuels.size(); ++i)
  // {
  //   auto& a = *simplefbp::SimpleFuels[i];
  //   auto& b = *dynamic_cast<fs::fuel::FuelBase*>(FuelLookup::Fuels[i]);
  //   compare(a.name(), a, b);
  //   // compare("", *simplefbp::SimpleFuels[i], *FuelLookup::Fuels[i]);
  // }
  auto i = 0;
  compare_fuel_valid("Non-fuel", simplefbp::NULL_FUEL, *FuelLookup::Fuels[i++], "basic test only");
  compare_fuel_valid("Invalid", simplefbp::INVALID, *FuelLookup::Fuels[i++], "basic test only");
  compare_fuel("C1", simplefbp::C1, *dynamic_cast<const fs::fuel::FuelC1*>(FuelLookup::Fuels[i++]));
  compare_fuel("C2", simplefbp::C2, *dynamic_cast<const fs::fuel::FuelC2*>(FuelLookup::Fuels[i++]));
  compare_fuel("C3", simplefbp::C3, *dynamic_cast<const fs::fuel::FuelC3*>(FuelLookup::Fuels[i++]));
  compare_fuel("C4", simplefbp::C4, *dynamic_cast<const fs::fuel::FuelC4*>(FuelLookup::Fuels[i++]));
  compare_fuel("C5", simplefbp::C5, *dynamic_cast<const fs::fuel::FuelC5*>(FuelLookup::Fuels[i++]));
  compare_fuel("C6", simplefbp::C6, *dynamic_cast<const fs::fuel::FuelC6*>(FuelLookup::Fuels[i++]));
  compare_fuel("C7", simplefbp::C7, *dynamic_cast<const fs::fuel::FuelC7*>(FuelLookup::Fuels[i++]));
  compare_fuel(
    "D1",
    simplefbp::D1,
    *dynamic_cast<const fs::fuel::FuelD1*>(FuelLookup::Fuels[i++]),
    FUEL_COMPARE_DECIDUOUS
  );
  compare_fuel(
    "D2",
    simplefbp::D2,
    *dynamic_cast<const fs::fuel::FuelD2*>(FuelLookup::Fuels[i++]),
    FUEL_COMPARE_DECIDUOUS
  );
  compare_fuel(
    "O1_A",
    simplefbp::O1_A,
    *dynamic_cast<const fs::fuel::FuelO1A*>(FuelLookup::Fuels[i++]),
    FUEL_COMPARE_GRASS
  );
  compare_fuel(
    "O1_B",
    simplefbp::O1_B,
    *dynamic_cast<const fs::fuel::FuelO1B*>(FuelLookup::Fuels[i++]),
    FUEL_COMPARE_GRASS
  );
  compare_fuel("S1", simplefbp::S1, *dynamic_cast<const fs::fuel::FuelS1*>(FuelLookup::Fuels[i++]));
  compare_fuel("S2", simplefbp::S2, *dynamic_cast<const fs::fuel::FuelS2*>(FuelLookup::Fuels[i++]));
  compare_fuel("S3", simplefbp::S3, *dynamic_cast<const fs::fuel::FuelS3*>(FuelLookup::Fuels[i++]));
  compare_fuel_variable(
    "D1_D2",
    simplefbp::D1_D2,
    *dynamic_cast<const fs::fuel::FuelD1D2*>(FuelLookup::Fuels[i++]),
    FUEL_COMPARE_DECIDUOUS
  );
  compare_fuel(
    "M1_05", simplefbp::M1_05, *dynamic_cast<const fs::fuel::FuelM1<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_10", simplefbp::M1_10, *dynamic_cast<const fs::fuel::FuelM1<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_15", simplefbp::M1_15, *dynamic_cast<const fs::fuel::FuelM1<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_20", simplefbp::M1_20, *dynamic_cast<const fs::fuel::FuelM1<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_25", simplefbp::M1_25, *dynamic_cast<const fs::fuel::FuelM1<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_30", simplefbp::M1_30, *dynamic_cast<const fs::fuel::FuelM1<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_35", simplefbp::M1_35, *dynamic_cast<const fs::fuel::FuelM1<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_40", simplefbp::M1_40, *dynamic_cast<const fs::fuel::FuelM1<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_45", simplefbp::M1_45, *dynamic_cast<const fs::fuel::FuelM1<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_50", simplefbp::M1_50, *dynamic_cast<const fs::fuel::FuelM1<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_55", simplefbp::M1_55, *dynamic_cast<const fs::fuel::FuelM1<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_60", simplefbp::M1_60, *dynamic_cast<const fs::fuel::FuelM1<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_65", simplefbp::M1_65, *dynamic_cast<const fs::fuel::FuelM1<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_70", simplefbp::M1_70, *dynamic_cast<const fs::fuel::FuelM1<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_75", simplefbp::M1_75, *dynamic_cast<const fs::fuel::FuelM1<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_80", simplefbp::M1_80, *dynamic_cast<const fs::fuel::FuelM1<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_85", simplefbp::M1_85, *dynamic_cast<const fs::fuel::FuelM1<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_90", simplefbp::M1_90, *dynamic_cast<const fs::fuel::FuelM1<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_95", simplefbp::M1_95, *dynamic_cast<const fs::fuel::FuelM1<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_05", simplefbp::M2_05, *dynamic_cast<const fs::fuel::FuelM2<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_10", simplefbp::M2_10, *dynamic_cast<const fs::fuel::FuelM2<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_15", simplefbp::M2_15, *dynamic_cast<const fs::fuel::FuelM2<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_20", simplefbp::M2_20, *dynamic_cast<const fs::fuel::FuelM2<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_25", simplefbp::M2_25, *dynamic_cast<const fs::fuel::FuelM2<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_30", simplefbp::M2_30, *dynamic_cast<const fs::fuel::FuelM2<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_35", simplefbp::M2_35, *dynamic_cast<const fs::fuel::FuelM2<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_40", simplefbp::M2_40, *dynamic_cast<const fs::fuel::FuelM2<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_45", simplefbp::M2_45, *dynamic_cast<const fs::fuel::FuelM2<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_50", simplefbp::M2_50, *dynamic_cast<const fs::fuel::FuelM2<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_55", simplefbp::M2_55, *dynamic_cast<const fs::fuel::FuelM2<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_60", simplefbp::M2_60, *dynamic_cast<const fs::fuel::FuelM2<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_65", simplefbp::M2_65, *dynamic_cast<const fs::fuel::FuelM2<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_70", simplefbp::M2_70, *dynamic_cast<const fs::fuel::FuelM2<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_75", simplefbp::M2_75, *dynamic_cast<const fs::fuel::FuelM2<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_80", simplefbp::M2_80, *dynamic_cast<const fs::fuel::FuelM2<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_85", simplefbp::M2_85, *dynamic_cast<const fs::fuel::FuelM2<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_90", simplefbp::M2_90, *dynamic_cast<const fs::fuel::FuelM2<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_95", simplefbp::M2_95, *dynamic_cast<const fs::fuel::FuelM2<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_05",
    simplefbp::M1_M2_05,
    *dynamic_cast<const fs::fuel::FuelM1M2<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_10",
    simplefbp::M1_M2_10,
    *dynamic_cast<const fs::fuel::FuelM1M2<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_15",
    simplefbp::M1_M2_15,
    *dynamic_cast<const fs::fuel::FuelM1M2<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_20",
    simplefbp::M1_M2_20,
    *dynamic_cast<const fs::fuel::FuelM1M2<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_25",
    simplefbp::M1_M2_25,
    *dynamic_cast<const fs::fuel::FuelM1M2<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_30",
    simplefbp::M1_M2_30,
    *dynamic_cast<const fs::fuel::FuelM1M2<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_35",
    simplefbp::M1_M2_35,
    *dynamic_cast<const fs::fuel::FuelM1M2<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_40",
    simplefbp::M1_M2_40,
    *dynamic_cast<const fs::fuel::FuelM1M2<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_45",
    simplefbp::M1_M2_45,
    *dynamic_cast<const fs::fuel::FuelM1M2<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_50",
    simplefbp::M1_M2_50,
    *dynamic_cast<const fs::fuel::FuelM1M2<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_55",
    simplefbp::M1_M2_55,
    *dynamic_cast<const fs::fuel::FuelM1M2<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_60",
    simplefbp::M1_M2_60,
    *dynamic_cast<const fs::fuel::FuelM1M2<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_65",
    simplefbp::M1_M2_65,
    *dynamic_cast<const fs::fuel::FuelM1M2<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_70",
    simplefbp::M1_M2_70,
    *dynamic_cast<const fs::fuel::FuelM1M2<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_75",
    simplefbp::M1_M2_75,
    *dynamic_cast<const fs::fuel::FuelM1M2<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_80",
    simplefbp::M1_M2_80,
    *dynamic_cast<const fs::fuel::FuelM1M2<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_85",
    simplefbp::M1_M2_85,
    *dynamic_cast<const fs::fuel::FuelM1M2<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_90",
    simplefbp::M1_M2_90,
    *dynamic_cast<const fs::fuel::FuelM1M2<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_95",
    simplefbp::M1_M2_95,
    *dynamic_cast<const fs::fuel::FuelM1M2<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_05", simplefbp::M3_05, *dynamic_cast<const fs::fuel::FuelM3<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_10", simplefbp::M3_10, *dynamic_cast<const fs::fuel::FuelM3<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_15", simplefbp::M3_15, *dynamic_cast<const fs::fuel::FuelM3<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_20", simplefbp::M3_20, *dynamic_cast<const fs::fuel::FuelM3<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_25", simplefbp::M3_25, *dynamic_cast<const fs::fuel::FuelM3<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_30", simplefbp::M3_30, *dynamic_cast<const fs::fuel::FuelM3<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_35", simplefbp::M3_35, *dynamic_cast<const fs::fuel::FuelM3<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_40", simplefbp::M3_40, *dynamic_cast<const fs::fuel::FuelM3<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_45", simplefbp::M3_45, *dynamic_cast<const fs::fuel::FuelM3<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_50", simplefbp::M3_50, *dynamic_cast<const fs::fuel::FuelM3<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_55", simplefbp::M3_55, *dynamic_cast<const fs::fuel::FuelM3<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_60", simplefbp::M3_60, *dynamic_cast<const fs::fuel::FuelM3<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_65", simplefbp::M3_65, *dynamic_cast<const fs::fuel::FuelM3<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_70", simplefbp::M3_70, *dynamic_cast<const fs::fuel::FuelM3<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_75", simplefbp::M3_75, *dynamic_cast<const fs::fuel::FuelM3<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_80", simplefbp::M3_80, *dynamic_cast<const fs::fuel::FuelM3<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_85", simplefbp::M3_85, *dynamic_cast<const fs::fuel::FuelM3<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_90", simplefbp::M3_90, *dynamic_cast<const fs::fuel::FuelM3<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_95", simplefbp::M3_95, *dynamic_cast<const fs::fuel::FuelM3<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_100", simplefbp::M3_100, *dynamic_cast<const fs::fuel::FuelM3<100>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_05", simplefbp::M4_05, *dynamic_cast<const fs::fuel::FuelM4<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_10", simplefbp::M4_10, *dynamic_cast<const fs::fuel::FuelM4<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_15", simplefbp::M4_15, *dynamic_cast<const fs::fuel::FuelM4<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_20", simplefbp::M4_20, *dynamic_cast<const fs::fuel::FuelM4<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_25", simplefbp::M4_25, *dynamic_cast<const fs::fuel::FuelM4<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_30", simplefbp::M4_30, *dynamic_cast<const fs::fuel::FuelM4<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_35", simplefbp::M4_35, *dynamic_cast<const fs::fuel::FuelM4<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_40", simplefbp::M4_40, *dynamic_cast<const fs::fuel::FuelM4<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_45", simplefbp::M4_45, *dynamic_cast<const fs::fuel::FuelM4<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_50", simplefbp::M4_50, *dynamic_cast<const fs::fuel::FuelM4<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_55", simplefbp::M4_55, *dynamic_cast<const fs::fuel::FuelM4<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_60", simplefbp::M4_60, *dynamic_cast<const fs::fuel::FuelM4<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_65", simplefbp::M4_65, *dynamic_cast<const fs::fuel::FuelM4<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_70", simplefbp::M4_70, *dynamic_cast<const fs::fuel::FuelM4<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_75", simplefbp::M4_75, *dynamic_cast<const fs::fuel::FuelM4<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_80", simplefbp::M4_80, *dynamic_cast<const fs::fuel::FuelM4<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_85", simplefbp::M4_85, *dynamic_cast<const fs::fuel::FuelM4<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_90", simplefbp::M4_90, *dynamic_cast<const fs::fuel::FuelM4<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_95", simplefbp::M4_95, *dynamic_cast<const fs::fuel::FuelM4<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_100", simplefbp::M4_100, *dynamic_cast<const fs::fuel::FuelM4<100>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_00",
    simplefbp::M3_M4_00,
    *dynamic_cast<const fs::fuel::FuelM3M4<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_05",
    simplefbp::M3_M4_05,
    *dynamic_cast<const fs::fuel::FuelM3M4<5>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_10",
    simplefbp::M3_M4_10,
    *dynamic_cast<const fs::fuel::FuelM3M4<10>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_15",
    simplefbp::M3_M4_15,
    *dynamic_cast<const fs::fuel::FuelM3M4<15>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_20",
    simplefbp::M3_M4_20,
    *dynamic_cast<const fs::fuel::FuelM3M4<20>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_25",
    simplefbp::M3_M4_25,
    *dynamic_cast<const fs::fuel::FuelM3M4<25>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_30",
    simplefbp::M3_M4_30,
    *dynamic_cast<const fs::fuel::FuelM3M4<30>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_35",
    simplefbp::M3_M4_35,
    *dynamic_cast<const fs::fuel::FuelM3M4<35>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_40",
    simplefbp::M3_M4_40,
    *dynamic_cast<const fs::fuel::FuelM3M4<40>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_45",
    simplefbp::M3_M4_45,
    *dynamic_cast<const fs::fuel::FuelM3M4<45>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_50",
    simplefbp::M3_M4_50,
    *dynamic_cast<const fs::fuel::FuelM3M4<50>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_55",
    simplefbp::M3_M4_55,
    *dynamic_cast<const fs::fuel::FuelM3M4<55>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_60",
    simplefbp::M3_M4_60,
    *dynamic_cast<const fs::fuel::FuelM3M4<60>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_65",
    simplefbp::M3_M4_65,
    *dynamic_cast<const fs::fuel::FuelM3M4<65>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_70",
    simplefbp::M3_M4_70,
    *dynamic_cast<const fs::fuel::FuelM3M4<70>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_75",
    simplefbp::M3_M4_75,
    *dynamic_cast<const fs::fuel::FuelM3M4<75>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_80",
    simplefbp::M3_M4_80,
    *dynamic_cast<const fs::fuel::FuelM3M4<80>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_85",
    simplefbp::M3_M4_85,
    *dynamic_cast<const fs::fuel::FuelM3M4<85>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_90",
    simplefbp::M3_M4_90,
    *dynamic_cast<const fs::fuel::FuelM3M4<90>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_95",
    simplefbp::M3_M4_95,
    *dynamic_cast<const fs::fuel::FuelM3M4<95>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M1_00", simplefbp::M1_00, *dynamic_cast<const fs::fuel::FuelM1<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M2_00", simplefbp::M2_00, *dynamic_cast<const fs::fuel::FuelM2<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M1_M2_00",
    simplefbp::M1_M2_00,
    *dynamic_cast<const fs::fuel::FuelM1M2<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M3_00", simplefbp::M3_00, *dynamic_cast<const fs::fuel::FuelM3<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel(
    "M4_00", simplefbp::M4_00, *dynamic_cast<const fs::fuel::FuelM4<0>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "M3_M4_100",
    simplefbp::M3_M4_100,
    *dynamic_cast<const fs::fuel::FuelM3M4<100>*>(FuelLookup::Fuels[i++])
  );
  compare_fuel_variable(
    "O1",
    simplefbp::O1,
    *dynamic_cast<const fs::fuel::FuelO1*>(FuelLookup::Fuels[i++]),
    FUEL_COMPARE_GRASS
  );
  check_equal(NUMBER_OF_FUELS, i, "Number of fuels");
  return 0;
}
}
