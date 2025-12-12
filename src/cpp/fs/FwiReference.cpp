// https://publications.gc.ca/collections/collection_2016/rncan-nrcan/Fo133-1-424-eng.pdf
#include "FwiReference.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include "FWI.h"
#include "Log.h"
#include "unstable.h"
#include "Util.h"
#include "Weather.h"
namespace fs::fwireference
{
using namespace std;
void FFMCcalc(float T, float H, float W, float Ro, float Fo, float& ffmc)
{
  float Mo, Rf, Ed, Ew, M, Kl, Kw, Mr, Ko, Kd;
  // fix precision
  // Mo = 147.2 * (101. - Fo) / (59.5 + Fo); /*Eq. 1 in */
  Mo = ffmc_to_moisture(Fo);
  if (Ro > 0.5)
  {                /*van Wagner and Pickett (1985)*/
    Rf = Ro - 0.5; /*Eq.2*/
    if (Mo <= 150.)
      Mr = Mo + 42.5 * Rf * (exp(-100. / (251. - Mo))) * (1 - exp(-6.93 / Rf)); /*Eq. 3a*/
    else
      Mr = Mo + 42.5 * Rf * (exp(-100. / (251. - Mo))) * (1 - exp(-6.93 / Rf))
         + .0015 * pow(Mo - 150., 2.) * pow(Rf, .5); /*Eq. 3b*/
    if (Mr > 250.)
      Mr = 250.;
    Mo = Mr;
  }
  Ed = 0.942 * pow(H, .679) + 11. * exp((H - 100.) / 10.)
     + .18 * (21.1 - T) * (1. - exp(-.115 * H)); /*Eq. 4*/
  if (Mo > Ed)
  {
    Ko =
      0.424 * (1. - pow(H / 100., 1.7)) + 0.0694 * pow(W, .5) * (1. - pow(H / 100., 8.)); /*Eq. 6a*/
    Kd = Ko * .581 * exp(0.0365 * T);                                                     /*Eq. 6b*/
    M = Ed + (Mo - Ed) * pow(10., -Kd);                                                   /*Eq. 8*/
  }
  else
  {
    Ew = 0.618 * pow(H, .753) + 10. * exp((H - 100.) / 10.)
       + .18 * (21.1 - T) * (1. - exp(-.115 * H)); /*Eq. 5*/
    if (Mo < Ew)
    {
      Kl = 0.424 * (1. - pow((100. - H) / 100., 1.7))
         + 0.0694 * pow(W, .5) * (1 - pow((100. - H) / 100., 8.)); /*Eq. 7a*/
      Kw = Kl * .581 * exp(0.0365 * T);                            /*Eq. 7b*/
      M = Ew - (Ew - Mo) * pow(10., -Kw);                          /*Eq. 9*/
    }
    else
      M = Mo;
  }
  /*Finally calculate FFMC */
  // fix precision
  // ffmc = (59.5 * (250.0 - M)) / (147.2 + M);
  ffmc = moisture_to_ffmc(M).value;
  /*..............................*/
  /*Make sure 0. <= FFMC <= 101.0 */
  /*..............................*/
  if (ffmc > 101.0)
    ffmc = 101.0;
  if (ffmc <= 0.0)
    ffmc = 0.0;
}
void DMCcalc(float T, float H, float Ro, float Po, int I, float& dmc)
{
  float Re, Mo, Mr, K, B, P, Pr;
  float Le[] = {6.5, 7.5, 9., 12.8, 13.9, 13.9, 12.4, 10.9, 9.4, 8., 7., 6.};
  if (T >= -1.1)
    K = 1.894 * (T + 1.1) * (100. - H) * Le[I - 1] * 0.0001;
  else
    K = 0.;
  /*Eq. 16*/
  /*Eq. 17*/
  if (Ro <= 1.5)
    Pr = Po;
  else
  {
    Re = 0.92 * Ro - 1.27;              /*Eq. 11*/
    Mo = 20. + 280.0 / exp(0.023 * Po); /*Eq. 12*/
    if (Po <= 33.)
      B = 100. / (.5 + .3 * Po); /*Eq. 13a*/
    else
    {
      if (Po <= 65.)
        B = 14. - 1.3 * log(Po); /*Eq. 13b*/
      else
        B = 6.2 * log(Po) - 17.2; /*Eq. 13c*/
    }
    Mr = Mo + 1000. * Re / (48.77 + B * Re); /*Eq. 14*/
    Pr = 43.43 * (5.6348 - log(Mr - 20.));   /*Eq. 15*/
  }
  if (Pr < 0.)
    Pr = 0.0;
  P = Pr + K;
  if (P <= 0.0)
    P = 0.0;
  dmc = P;
}
void DCcalc(float T, float Ro, float Do, int I, float& dc)
{
  float Rd, Qo, Qr, V, D, Dr;
  float Lf[] = {-1.6, -1.6, -1.6, .9, 3.8, 5.8, 6.4, 5., 2.4, .4, -1.6, -1.6};
  if (Ro > 2.8)
  {
    Rd = 0.83 * (Ro)-1.27;       /*Eq. 18*/
    Qo = 800. * exp(-Do / 400.); /*Eq. 19*/
    Qr = Qo + 3.937 * Rd;        /*Eq. 20*/
    Dr = 400. * log(800. / Qr);  /*Eq. 21*/
    if (Dr > 0.)
      Do = Dr;
    else
      Do = 0.0;
  }
  if (T > -2.8) /*Eq. 22*/
    V = 0.36 * (T + 2.8) + Lf[I - 1];
  else
    V = Lf[I - 1];
  if (V < 0.) /*Eq. 23*/
    V = .0;
  dc = Do + 0.5 * V;
}
void ISIcalc(float F, float W, float& isi)
{
  float Fw, M, Ff;
  // fix precision
  // M = 147.2 * (101 - F) / (59.5 + F);                         /*Eq. 1*/
  M = ffmc_to_moisture(F);
  Fw = exp(0.05039 * W);                                      /*Eq. 24*/
  Ff = 91.9 * exp(-.1386 * M) * (1. + pow(M, 5.31) / 4.93E7); /*Eq. 25*/
  isi = 0.208 * Fw * Ff;                                      /*Eq. 26*/
}
void BUIcalc(float P, float D, float& bui)
{
  if (P <= .4 * D)
    bui = 0.8 * P * D / (P + .4 * D);
  else
    bui = P - (1. - .8 * D / (P + .4 * D)) * (.92 + pow(.0114 * P, 1.7));
  /*Eq. 27a*/
  /*Eq. 27b*/
  if (bui <= 0.0)
    bui = 0.0;
}
void FWIcalc(float R, float U, float& fwi)
{
  float Fd, B, S;
  if (U <= 80.)
    Fd = .626 * pow(U, .809) + 2.; /*Eq. 28a*/
  else
    Fd = 1000. / (25. + 108.64 * exp(-.023 * U)); /*Eq. 28b*/
  B = .1 * R * Fd;                                /*Eq. 29*/
  if (B > 1.)
    fwi = exp(2.72 * pow(.434 * log(B), .647)); /*Eq. 30a*/
  else                                          /*Eq. 30b*/
    fwi = B;
}
int test_fwi_file(const string file_in, const string file_out)
{
  string line;
  float temp, rhum, wind, prcp, x, y;
  float ffmc, ffmc0, dmc, dmc0, dc, dc0, isi, bui, fwi;
  int month, day;
  /* Initialize FMC, DMC, and DC */
  ffmc0 = 85.0;
  dmc0 = 6.0;
  dc0 = 15.0;
  Ffmc ffmc0_{ffmc0};
  Dmc dmc0_{dmc0};
  Dc dc0_{dc0};
  /* Open input and output files */
  ifstream inputFile(file_in.c_str());
  if (!inputFile.is_open())
  {
    cout << "Unable to open input data file";
    return -1;
  }
  ofstream outputFile(file_out.c_str());
  if (!outputFile.is_open())
  {
    cout << "Unable to open output data file";
    return 1;
  }
  /* Main loop for calculating indices */
  while (getline(inputFile, line))
  {
    istringstream ss(line);
    ss >> month >> day >> temp >> rhum >> wind >> prcp;
    static constexpr MathSize EPSILON{1e-4f};
    static constexpr MathSize LATITUDE{50};
    Temperature temp_{temp};
    RelativeHumidity rhum_{rhum};
    Speed wind_{wind};
    Precipitation prcp_{prcp};
    FFMCcalc(temp, rhum, wind, prcp, ffmc0, ffmc);
    Ffmc ffmc_{temp_, rhum_, wind_, prcp_, ffmc0_};
    logging::check_tolerance(EPSILON, ffmc, ffmc_.value, "ffmc");
    DMCcalc(temp, rhum, prcp, dmc0, month, dmc);
    Dmc dmc_{temp_, rhum_, prcp_, dmc0_, month, LATITUDE};
    logging::check_tolerance(EPSILON, dmc, dmc_.value, "dmc");
    DCcalc(temp, prcp, dc0, month, dc);
    Dc dc_{temp_, prcp_, dc0_, month, LATITUDE};
    logging::check_tolerance(EPSILON, dc, dc_.value, "dc");
    ISIcalc(ffmc, wind, isi);
    Isi isi_{wind_, ffmc_};
    logging::check_tolerance(EPSILON, isi, isi_.value, "isi");
    BUIcalc(dmc, dc, bui);
    Bui bui_{dmc_, dc_};
    logging::check_tolerance(EPSILON, bui, bui_.value, "bui");
    FWIcalc(isi, bui, fwi);
    Fwi fwi_{isi_, bui_};
    logging::check_tolerance(EPSILON, fwi, fwi_.value, "fwi");
    ffmc0 = ffmc;
    dmc0 = dmc;
    dc0 = dc;
    ffmc0_ = ffmc_;
    dmc0_ = dmc_;
    dc0_ = dc_;
    printf("%0.1f %0.1f %0.1f %0.1f %0.1f %0.1f\n", ffmc, dmc, dc, isi, bui, fwi);
    outputFile << std::format(
      "{:0.1f} {:0.1f} {:0.1f} {:0.1f} {:0.1f} {:0.1f}", ffmc, dmc, dc, isi, bui, fwi
    ) << endl;
    // outputFile << ffmc << ' ' << dmc << ' ' << dc << ' ' << isi << ' ' << bui << ' ' << fwi <<
    // endl;
  }
  inputFile.close();
  outputFile.close();
  return 0;
}
int compare_files(const string file0, const string file1)
{
  printf("Comparing %s to %s\n", file0.c_str(), file1.c_str());
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
      printf("\t%s\n!=\t%s\n", line0.c_str(), line1.c_str());
      return cmp;
    }
    // printf("%s\n", line0.c_str());
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
  if (auto ret = test_fwi_file(file_in, file_out); ret != 0)
  {
    printf("Generating %s from %s failed\n", file_out.c_str(), file_in.c_str());
    return ret;
  }
  if (auto cmp = compare_files(file_expected, file_out); 0 != cmp)
  {
    printf(
      "Comparison between files [%s, %s] gives %d\n", file_expected.c_str(), file_out.c_str(), cmp
    );
    return cmp;
  }
  return 0;
}
int test_fwi(const int argc, const char* const argv[])
{
  std::ignore = argc;
  std::ignore = argv;
  make_directory("test/output/fwi");
  return test_fwi_files(
    "test/data/fwi/fwi_out.txt", "test/data/fwi/fwi_in.txt", "test/output/fwi/fwi_out.txt"
  );
}
}
