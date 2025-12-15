// https://publications.gc.ca/collections/collection_2016/rncan-nrcan/Fo133-1-424-eng.pdf
#ifndef FS_FWIREFERENCE_H
#define FS_FWIREFERENCE_H
#include "stdafx.h"
namespace fs::fwireference
{
constexpr auto DEFAULT_LATITUDE = 46.0;
void FFMCcalc(MathSize T, MathSize H, MathSize W, MathSize Ro, MathSize Fo, MathSize& ffmc);
void DMCcalc(
  MathSize T,
  MathSize H,
  MathSize Ro,
  MathSize Po,
  int I,
  MathSize& dmc,
  const MathSize latitude = DEFAULT_LATITUDE
);
void DCcalc(
  MathSize T,
  MathSize Ro,
  MathSize Do,
  int I,
  MathSize& dc,
  const MathSize latitude = DEFAULT_LATITUDE
);
void ISIcalc(MathSize F, MathSize W, MathSize& isi);
void BUIcalc(MathSize P, MathSize D, MathSize& bui);
void FWIcalc(MathSize R, MathSize U, MathSize& fwi);
}
#endif
