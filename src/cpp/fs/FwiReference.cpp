/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FwiReference.h"
#include "FWI.h"
namespace fs::fwireference
{
using namespace std;
const auto LATITUDE_INNER = 10.0;
const auto LATITUDE_MIDDLE = 30.0;
MathSize FFMCcalc(
  MathSize temperature,
  MathSize relative_humidity,
  MathSize wind_speed,
  MathSize rain_24hr,
  MathSize ffmc_previous
)
{
  MathSize Mo, Rf, Ed, Ew, M, Kl, Kw, Mr, Ko, Kd;
  // fix precision
  // Mo = 147.2 * (101. - Fo) / (59.5 + Fo); /*Eq. 1 in */
  Mo = ffmc_to_moisture(ffmc_previous);
  if (rain_24hr > 0.5)
  {                       /*van Wagner and Pickett (1985)*/
    Rf = rain_24hr - 0.5; /*Eq.2*/
    if (Mo <= 150.)
      Mr = Mo + 42.5 * Rf * (exp(-100. / (251. - Mo))) * (1 - exp(-6.93 / Rf)); /*Eq. 3a*/
    else
      Mr = Mo + 42.5 * Rf * (exp(-100. / (251. - Mo))) * (1 - exp(-6.93 / Rf))
         + .0015 * pow(Mo - 150., 2.) * pow(Rf, .5); /*Eq. 3b*/
    if (Mr > 250.)
      Mr = 250.;
    Mo = Mr;
  }
  Ed = 0.942 * pow(relative_humidity, .679) + 11. * exp((relative_humidity - 100.) / 10.)
     + .18 * (21.1 - temperature) * (1. - exp(-.115 * relative_humidity)); /*Eq. 4*/
  if (Mo > Ed)
  {
    Ko = 0.424 * (1. - pow(relative_humidity / 100., 1.7))
       + 0.0694 * pow(wind_speed, .5) * (1. - pow(relative_humidity / 100., 8.)); /*Eq. 6a*/
    Kd = Ko * .581 * exp(0.0365 * temperature);                                   /*Eq. 6b*/
    M = Ed + (Mo - Ed) * pow(10., -Kd);                                           /*Eq. 8*/
  }
  else
  {
    Ew = 0.618 * pow(relative_humidity, .753) + 10. * exp((relative_humidity - 100.) / 10.)
       + .18 * (21.1 - temperature) * (1. - exp(-.115 * relative_humidity)); /*Eq. 5*/
    if (Mo < Ew)
    {
      Kl = 0.424 * (1. - pow((100. - relative_humidity) / 100., 1.7))
         + 0.0694 * pow(wind_speed, .5)
             * (1 - pow((100. - relative_humidity) / 100., 8.)); /*Eq. 7a*/
      Kw = Kl * .581 * exp(0.0365 * temperature);                /*Eq. 7b*/
      M = Ew - (Ew - Mo) * pow(10., -Kw);                        /*Eq. 9*/
    }
    else
      M = Mo;
  }
  /*Finally calculate FFMC */
  // fix precision
  // ffmc = (59.5 * (250.0 - M)) / (147.2 + M);
  auto ffmc = moisture_to_ffmc(M).value;
  /*..............................*/
  /*Make sure 0. <= FFMC <= 101.0 */
  /*..............................*/
  if (ffmc > 101.0)
    ffmc = 101.0;
  if (ffmc <= 0.0)
    ffmc = 0.0;
  return ffmc;
}
MathSize DMCcalc(
  MathSize temperature,
  MathSize relative_humidity,
  MathSize rain_24hr,
  MathSize dmc_previous,
  int month,
  const MathSize latitude
)
{
  MathSize Re, Mo, Mr, K, B, P, Pr;
  // // # Reference latitude for DMC day length adjustment
  // // # 46N: Canadian standard, latitude >= 30N   (Van Wagner 1987)
  // MathSize Le0[] = {6.5, 7.5, 9, 12.8, 13.9, 13.9, 12.4, 10.9, 9.4, 8, 7, 6};
  // // # 20N: For 30 > latitude >= 10
  // //       lat <= 30 & lat > 10,
  // MathSize Le1[] = {7.9, 8.4, 8.9, 9.5, 9.9, 10.2, 10.1, 9.7, 9.1, 8.6, 8.1, 7.8};
  // // # 20S: For -10 > latitude >= -30
  // //       lat <= -10 & lat > -30,
  // MathSize Le2[] = {10.1, 9.6, 9.1, 8.5, 8.1, 7.8, 7.9, 8.3, 8.9, 9.4, 9.9, 10.2};
  // // # 40S: For -30 > latitude
  // //      lat <= -30 & lat >= -90,
  // MathSize Le3[] = {11.5, 10.5, 9.2, 7.9, 6.8, 6.2, 6.5, 7.4, 8.7, 10, 11.2, 11.8};
  // // # For latitude near the equator, we simple use a factor of 9 for all months
  // //      lat <= 10 & lat > -10,
  // # Reference latitude for DMC day length adjustment
  // # 46N: Canadian standard, latitude >= 30N   (Van Wagner 1987)
  MathSize DAY_LENGTH46_N[] = {6.5, 7.5, 9, 12.8, 13.9, 13.9, 12.4, 10.9, 9.4, 8, 7, 6};
  // # 20N: For 30 > latitude >= 10
  //       lat <= 30 & lat > 10,
  MathSize DAY_LENGTH20_N[] = {7.9, 8.4, 8.9, 9.5, 9.9, 10.2, 10.1, 9.7, 9.1, 8.6, 8.1, 7.8};
  // # 20S: For -10 > latitude >= -30
  //       lat <= -10 & lat > -30,
  MathSize DAY_LENGTH20_S[] = {10.1, 9.6, 9.1, 8.5, 8.1, 7.8, 7.9, 8.3, 8.9, 9.4, 9.9, 10.2};
  // # 40S: For -30 > latitude
  //      lat <= -30 & lat >= -90,
  MathSize DAY_LENGTH40_S[] = {11.5, 10.5, 9.2, 7.9, 6.8, 6.2, 6.5, 7.4, 8.7, 10, 11.2, 11.8};
  // # For latitude near the equator, we simple use a factor of 9 for all months
  //      lat <= 10 & lat > -10,
  // const auto le =
  //   LATITUDE_INNER > abs(latitude)
  //     ? 9
  //     : ((latitude <= 10 ? (latitude <= -30 ? Le3 : Le2) : (latitude >= 30 ? Le0 : Le1))[I - 1]);
  const auto le = LATITUDE_INNER > abs(latitude)
                  ? 9.0
                  : (latitude >= LATITUDE_MIDDLE    ? DAY_LENGTH46_N
                     : latitude >= LATITUDE_INNER   ? DAY_LENGTH20_N
                     : latitude <= -LATITUDE_MIDDLE ? DAY_LENGTH40_S
                                                    : DAY_LENGTH20_S)[month - 1];
  if (temperature >= -1.1)
    K = 1.894 * (temperature + 1.1) * (100. - relative_humidity) * le * 0.0001;
  else
    K = 0.;
  /*Eq. 16*/
  /*Eq. 17*/
  if (rain_24hr <= 1.5)
    Pr = dmc_previous;
  else
  {
    Re = 0.92 * rain_24hr - 1.27;                 /*Eq. 11*/
    Mo = 20. + 280.0 / exp(0.023 * dmc_previous); /*Eq. 12*/
    if (dmc_previous <= 33.)
      B = 100. / (.5 + .3 * dmc_previous); /*Eq. 13a*/
    else
    {
      if (dmc_previous <= 65.)
        B = 14. - 1.3 * log(dmc_previous); /*Eq. 13b*/
      else
        B = 6.2 * log(dmc_previous) - 17.2; /*Eq. 13c*/
    }
    Mr = Mo + 1000. * Re / (48.77 + B * Re); /*Eq. 14*/
    Pr = 43.43 * (5.6348 - log(Mr - 20.));   /*Eq. 15*/
  }
  if (Pr < 0.)
    Pr = 0.0;
  P = Pr + K;
  if (P <= 0.0)
    P = 0.0;
  return P;
}
MathSize DCcalc(
  MathSize temperature,
  MathSize rain_24hr,
  MathSize dc_previous,
  int month,
  MathSize latitude
)
{
  MathSize Rd, Qo, Qr, V, D, Dr;
  // Day length factor for DC Calculations
  // 20N: North of 20 degrees N
  MathSize LfN[] = {-1.6, -1.6, -1.6, 0.9, 3.8, 5.8, 6.4, 5, 2.4, 0.4, -1.6, -1.6};
  // 20S: South of 20 degrees S
  MathSize LfS[] = {6.4, 5, 2.4, 0.4, -1.6, -1.6, -1.6, -1.6, -1.6, 0.9, 3.8, 5.8};
  if (rain_24hr > 2.8)
  {
    Rd = 0.83 * (rain_24hr)-1.27;         /*Eq. 18*/
    Qo = 800. * exp(-dc_previous / 400.); /*Eq. 19*/
    Qr = Qo + 3.937 * Rd;                 /*Eq. 20*/
    Dr = 400. * log(800. / Qr);           /*Eq. 21*/
    if (Dr > 0.)
      dc_previous = Dr;
    else
      dc_previous = 0.0;
  }
  // Near the equator, we just use 1.4 for all months.
  const auto lf =
    abs(latitude) < LATITUDE_INNER ? 1.4 : (latitude >= LATITUDE_INNER ? LfN : LfS)[month - 1];
  if (temperature > -2.8)
  { /*Eq. 22*/
    V = 0.36 * (temperature + 2.8) + lf;
  }
  else
    V = lf;
  if (V < 0.) /*Eq. 23*/
    V = .0;
  return dc_previous + 0.5 * V;
}
MathSize ISIcalc(MathSize ffmc, MathSize wind_speed)
{
  MathSize Fw, M, Ff;
  // fix precision
  // M = 147.2 * (101 - F) / (59.5 + F);                         /*Eq. 1*/
  M = ffmc_to_moisture(ffmc);
  Fw = exp(0.05039 * wind_speed);                             /*Eq. 24*/
  Ff = 91.9 * exp(-.1386 * M) * (1. + pow(M, 5.31) / 4.93E7); /*Eq. 25*/
  return 0.208 * Fw * Ff;                                     /*Eq. 26*/
}
MathSize BUIcalc(MathSize dmc, MathSize dc)
{
  MathSize bui;
  if (dmc <= .4 * dc)
    bui = 0.8 * dmc * dc / (dmc + .4 * dc);
  else
    bui = dmc - (1. - .8 * dc / (dmc + .4 * dc)) * (.92 + pow(.0114 * dmc, 1.7));
  /*Eq. 27a*/
  /*Eq. 27b*/
  if (bui <= 0.0)
    bui = 0.0;
  return bui;
}
MathSize FWIcalc(MathSize isi, MathSize bui)
{
  MathSize fwi;
  MathSize Fd, B, S;
  if (bui <= 80.)
    Fd = .626 * pow(bui, .809) + 2.; /*Eq. 28a*/
  else
    Fd = 1000. / (25. + 108.64 * exp(-.023 * bui)); /*Eq. 28b*/
  B = .1 * isi * Fd;                                /*Eq. 29*/
  if (B > 1.)
    fwi = exp(2.72 * pow(.434 * log(B), .647)); /*Eq. 30a*/
  else                                          /*Eq. 30b*/
    fwi = B;
  return fwi;
}
}
