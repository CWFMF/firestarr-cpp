// https://publications.gc.ca/collections/collection_2016/rncan-nrcan/Fo133-1-424-eng.pdf
#ifndef FS_FWIREFERENCE_H
#define FS_FWIREFERENCE_H
#include "stdafx.h"
namespace fs::fwireference
{
constexpr auto DEFAULT_LATITUDE = 46.0;
MathSize FFMCcalc(MathSize T, MathSize H, MathSize W, MathSize Ro, MathSize Fo);
MathSize DMCcalc(
  MathSize T,
  MathSize H,
  MathSize Ro,
  MathSize Po,
  int I,
  const MathSize latitude = DEFAULT_LATITUDE
);
MathSize DCcalc(
  MathSize T,
  MathSize Ro,
  MathSize Do,
  int I,
  const MathSize latitude = DEFAULT_LATITUDE
);
MathSize ISIcalc(MathSize F, MathSize W);
MathSize BUIcalc(MathSize P, MathSize D);
MathSize FWIcalc(MathSize R, MathSize U);
}
#endif
