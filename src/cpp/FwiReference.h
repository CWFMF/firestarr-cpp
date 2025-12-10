// https://publications.gc.ca/collections/collection_2016/rncan-nrcan/Fo133-1-424-eng.pdf
#ifndef FS_FWIREFRENCE_H
#define FS_FWIREFRENCE_H
#define TEST_FWI 1
// #undef TEST_FWI
namespace fs::fwireference
{
void FFMCcalc(float T, float H, float W, float Ro, float Fo, float& ffmc);
void DMCcalc(float T, float H, float Ro, float Po, int I, float& dmc);
void DCcalc(float T, float Ro, float Do, int I, float& dc);
void ISIcalc(float F, float W, float& isi);
void BUIcalc(float P, float D, float& bui);
void FWIcalc(float R, float U, float& fwi);
#ifdef TEST_FWI
int test_fwi(const int argc, const char* const argv[]);
#endif
}
#endif
