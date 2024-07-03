/*********************************************************************
Spectral lines.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2019-2024 Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

#include <gnuastro/speclines.h>


/*********************************************************************/
/*************        Internal names and codes         ***************/
/*********************************************************************/
/* Return line's name as literal string. */
char *
gal_speclines_line_name(int linecode)
{
  switch(linecode)
    {
    case GAL_SPECLINES_Ne_VIII_770:    return GAL_SPECLINES_NAME_Ne_VIII_770;
    case GAL_SPECLINES_Ne_VIII_780:    return GAL_SPECLINES_NAME_Ne_VIII_780;
    case GAL_SPECLINES_Ly_epsilon:     return GAL_SPECLINES_NAME_Ly_epsilon;
    case GAL_SPECLINES_Ly_delta:       return GAL_SPECLINES_NAME_Ly_delta;
    case GAL_SPECLINES_Ly_gamma:       return GAL_SPECLINES_NAME_Ly_gamma;
    case GAL_SPECLINES_C_III_977:      return GAL_SPECLINES_NAME_C_III_977;
    case GAL_SPECLINES_N_III_990:      return GAL_SPECLINES_NAME_N_III_990;
    case GAL_SPECLINES_N_III_991_51:   return GAL_SPECLINES_NAME_N_III_991_51;
    case GAL_SPECLINES_N_III_991_58:   return GAL_SPECLINES_NAME_N_III_991_58;
    case GAL_SPECLINES_Ly_beta:        return GAL_SPECLINES_NAME_Ly_beta;
    case GAL_SPECLINES_O_VI_1032:      return GAL_SPECLINES_NAME_O_VI_1032;
    case GAL_SPECLINES_O_VI_1038:      return GAL_SPECLINES_NAME_O_VI_1038;
    case GAL_SPECLINES_Ar_I_1067:      return GAL_SPECLINES_NAME_Ar_I_1067;
    case GAL_SPECLINES_Ly_alpha:       return GAL_SPECLINES_NAME_Ly_alpha;
    case GAL_SPECLINES_N_V_1238:       return GAL_SPECLINES_NAME_N_V_1238;
    case GAL_SPECLINES_N_V_1243:       return GAL_SPECLINES_NAME_N_V_1243;
    case GAL_SPECLINES_Si_II_1260:     return GAL_SPECLINES_NAME_Si_II_1260;
    case GAL_SPECLINES_Si_II_1265:     return GAL_SPECLINES_NAME_Si_II_1265;
    case GAL_SPECLINES_O_I_1302:       return GAL_SPECLINES_NAME_O_I_1302;
    case GAL_SPECLINES_C_II_1335:      return GAL_SPECLINES_NAME_C_II_1335;
    case GAL_SPECLINES_C_II_1336:      return GAL_SPECLINES_NAME_C_II_1336;
    case GAL_SPECLINES_Si_IV_1394:     return GAL_SPECLINES_NAME_Si_IV_1394;
    case GAL_SPECLINES_O_IV_1397:      return GAL_SPECLINES_NAME_O_IV_1397;
    case GAL_SPECLINES_O_IV_1400:      return GAL_SPECLINES_NAME_O_IV_1400;
    case GAL_SPECLINES_Si_IV_1403:     return GAL_SPECLINES_NAME_Si_IV_1403;
    case GAL_SPECLINES_N_IV_1486:      return GAL_SPECLINES_NAME_N_IV_1486;
    case GAL_SPECLINES_C_IV_1548:      return GAL_SPECLINES_NAME_C_IV_1548;
    case GAL_SPECLINES_C_IV_1551:      return GAL_SPECLINES_NAME_C_IV_1551;
    case GAL_SPECLINES_He_II_1640:     return GAL_SPECLINES_NAME_He_II_1640;
    case GAL_SPECLINES_O_III_1661:     return GAL_SPECLINES_NAME_O_III_1661;
    case GAL_SPECLINES_O_III_1666:     return GAL_SPECLINES_NAME_O_III_1666;
    case GAL_SPECLINES_N_III_1747:     return GAL_SPECLINES_NAME_N_III_1747;
    case GAL_SPECLINES_N_III_1749:     return GAL_SPECLINES_NAME_N_III_1749;
    case GAL_SPECLINES_Al_III_1855:    return GAL_SPECLINES_NAME_Al_III_1855;
    case GAL_SPECLINES_Al_III_1863:    return GAL_SPECLINES_NAME_Al_III_1863;
    case GAL_SPECLINES_Si_III:         return GAL_SPECLINES_NAME_Si_III;
    case GAL_SPECLINES_C_III_1909:     return GAL_SPECLINES_NAME_C_III_1909;
    case GAL_SPECLINES_N_II_2143:      return GAL_SPECLINES_NAME_N_II_2143;
    case GAL_SPECLINES_O_III_2321:     return GAL_SPECLINES_NAME_O_III_2321;
    case GAL_SPECLINES_C_II_2324:      return GAL_SPECLINES_NAME_C_II_2324;
    case GAL_SPECLINES_C_II_2325:      return GAL_SPECLINES_NAME_C_II_2325;
    case GAL_SPECLINES_Fe_XI_2649:     return GAL_SPECLINES_NAME_Fe_XI_2649;
    case GAL_SPECLINES_He_II_2733:     return GAL_SPECLINES_NAME_He_II_2733;
    case GAL_SPECLINES_Mg_V_2783:      return GAL_SPECLINES_NAME_Mg_V_2783;
    case GAL_SPECLINES_Mg_II_2796:     return GAL_SPECLINES_NAME_Mg_II_2796;
    case GAL_SPECLINES_Mg_II_2803:     return GAL_SPECLINES_NAME_Mg_II_2803;
    case GAL_SPECLINES_Fe_IV_2829:     return GAL_SPECLINES_NAME_Fe_IV_2829;
    case GAL_SPECLINES_Fe_IV_2836:     return GAL_SPECLINES_NAME_Fe_IV_2836;
    case GAL_SPECLINES_Ar_IV_2854:     return GAL_SPECLINES_NAME_Ar_IV_2854;
    case GAL_SPECLINES_Ar_IV_2868:     return GAL_SPECLINES_NAME_Ar_IV_2868;
    case GAL_SPECLINES_Mg_V_2928:      return GAL_SPECLINES_NAME_Mg_V_2928;
    case GAL_SPECLINES_He_I_2945:      return GAL_SPECLINES_NAME_He_I_2945;
    case GAL_SPECLINES_O_III_3133:     return GAL_SPECLINES_NAME_O_III_3133;
    case GAL_SPECLINES_He_I_3188:      return GAL_SPECLINES_NAME_He_I_3188;
    case GAL_SPECLINES_He_II_3203:     return GAL_SPECLINES_NAME_He_II_3203;
    case GAL_SPECLINES_O_III_3312:     return GAL_SPECLINES_NAME_O_III_3312;
    case GAL_SPECLINES_Ne_V_3346:      return GAL_SPECLINES_NAME_Ne_V_3346;
    case GAL_SPECLINES_Ne_V_3426:      return GAL_SPECLINES_NAME_Ne_V_3426;
    case GAL_SPECLINES_O_III_3444:     return GAL_SPECLINES_NAME_O_III_3444;
    case GAL_SPECLINES_N_I_3466_50:    return GAL_SPECLINES_NAME_N_I_3466_50;
    case GAL_SPECLINES_N_I_3466_54:    return GAL_SPECLINES_NAME_N_I_3466_54;
    case GAL_SPECLINES_He_I_3488:      return GAL_SPECLINES_NAME_He_I_3488;
    case GAL_SPECLINES_Fe_VII_3586:    return GAL_SPECLINES_NAME_Fe_VII_3586;
    case GAL_SPECLINES_Fe_VI_3663:     return GAL_SPECLINES_NAME_Fe_VI_3663;
    case GAL_SPECLINES_H_19:           return GAL_SPECLINES_NAME_H_19;
    case GAL_SPECLINES_H_18:           return GAL_SPECLINES_NAME_H_18;
    case GAL_SPECLINES_H_17:           return GAL_SPECLINES_NAME_H_17;
    case GAL_SPECLINES_H_16:           return GAL_SPECLINES_NAME_H_16;
    case GAL_SPECLINES_H_15:           return GAL_SPECLINES_NAME_H_15;
    case GAL_SPECLINES_H_14:           return GAL_SPECLINES_NAME_H_14;
    case GAL_SPECLINES_O_II_3726:      return GAL_SPECLINES_NAME_O_II_3726;
    case GAL_SPECLINES_O_II_3729:      return GAL_SPECLINES_NAME_O_II_3729;
    case GAL_SPECLINES_H_13:           return GAL_SPECLINES_NAME_H_13;
    case GAL_SPECLINES_H_12:           return GAL_SPECLINES_NAME_H_12;
    case GAL_SPECLINES_Fe_VII_3759:    return GAL_SPECLINES_NAME_Fe_VII_3759;
    case GAL_SPECLINES_H_11:           return GAL_SPECLINES_NAME_H_11;
    case GAL_SPECLINES_H_10:           return GAL_SPECLINES_NAME_H_10;
    case GAL_SPECLINES_H_9:            return GAL_SPECLINES_NAME_H_9;
    case GAL_SPECLINES_Fe_V_3839:      return GAL_SPECLINES_NAME_Fe_V_3839;
    case GAL_SPECLINES_Ne_III_3869:    return GAL_SPECLINES_NAME_Ne_III_3869;
    case GAL_SPECLINES_He_I_3889:      return GAL_SPECLINES_NAME_He_I_3889;
    case GAL_SPECLINES_H_8:            return GAL_SPECLINES_NAME_H_8;
    case GAL_SPECLINES_Fe_V_3891:      return GAL_SPECLINES_NAME_Fe_V_3891;
    case GAL_SPECLINES_Fe_V_3911:      return GAL_SPECLINES_NAME_Fe_V_3911;
    case GAL_SPECLINES_Ne_III_3967:     return GAL_SPECLINES_NAME_Ne_III_3967;
    case GAL_SPECLINES_H_epsilon:      return GAL_SPECLINES_NAME_H_epsilon;
    case GAL_SPECLINES_He_I_4026:      return GAL_SPECLINES_NAME_He_I_4026;
    case GAL_SPECLINES_S_II_4069:      return GAL_SPECLINES_NAME_S_II_4069;
    case GAL_SPECLINES_Fe_V_4071:      return GAL_SPECLINES_NAME_Fe_V_4071;
    case GAL_SPECLINES_S_II_4076:      return GAL_SPECLINES_NAME_S_II_4076;
    case GAL_SPECLINES_H_delta:        return GAL_SPECLINES_NAME_H_delta;
    case GAL_SPECLINES_He_I_4144:      return GAL_SPECLINES_NAME_He_I_4144;
    case GAL_SPECLINES_Fe_II_4179:     return GAL_SPECLINES_NAME_Fe_II_4179;
    case GAL_SPECLINES_Fe_V_4181:      return GAL_SPECLINES_NAME_Fe_V_4181;
    case GAL_SPECLINES_Fe_II_4233:     return GAL_SPECLINES_NAME_Fe_II_4233;
    case GAL_SPECLINES_Fe_V_4227:      return GAL_SPECLINES_NAME_Fe_V_4227;
    case GAL_SPECLINES_Fe_II_4287:     return GAL_SPECLINES_NAME_Fe_II_4287;
    case GAL_SPECLINES_Fe_II_4304:     return GAL_SPECLINES_NAME_Fe_II_4304;
    case GAL_SPECLINES_O_II_4317:      return GAL_SPECLINES_NAME_O_II_4317;
    case GAL_SPECLINES_H_gamma:        return GAL_SPECLINES_NAME_H_gamma;
    case GAL_SPECLINES_O_III_4363:     return GAL_SPECLINES_NAME_O_III_4363;
    case GAL_SPECLINES_Ar_XIV:         return GAL_SPECLINES_NAME_Ar_XIV;
    case GAL_SPECLINES_O_II_4415:      return GAL_SPECLINES_NAME_O_II_4415;
    case GAL_SPECLINES_Fe_II_4417:     return GAL_SPECLINES_NAME_Fe_II_4417;
    case GAL_SPECLINES_Fe_II_4452:     return GAL_SPECLINES_NAME_Fe_II_4452;
    case GAL_SPECLINES_He_I_4471:      return GAL_SPECLINES_NAME_He_I_4471;
    case GAL_SPECLINES_Fe_II_4489:     return GAL_SPECLINES_NAME_Fe_II_4489;
    case GAL_SPECLINES_Fe_II_4491:     return GAL_SPECLINES_NAME_Fe_II_4491;
    case GAL_SPECLINES_N_III_4510:     return GAL_SPECLINES_NAME_N_III_4510;
    case GAL_SPECLINES_Fe_II_4523:     return GAL_SPECLINES_NAME_Fe_II_4523;
    case GAL_SPECLINES_Fe_II_4556:     return GAL_SPECLINES_NAME_Fe_II_4556;
    case GAL_SPECLINES_Fe_II_4583:     return GAL_SPECLINES_NAME_Fe_II_4583;
    case GAL_SPECLINES_Fe_II_4584:     return GAL_SPECLINES_NAME_Fe_II_4584;
    case GAL_SPECLINES_Fe_II_4630:     return GAL_SPECLINES_NAME_Fe_II_4630;
    case GAL_SPECLINES_N_III_4634:     return GAL_SPECLINES_NAME_N_III_4634;
    case GAL_SPECLINES_N_III_4641:     return GAL_SPECLINES_NAME_N_III_4641;
    case GAL_SPECLINES_N_III_4642:     return GAL_SPECLINES_NAME_N_III_4642;
    case GAL_SPECLINES_C_III_4647:     return GAL_SPECLINES_NAME_C_III_4647;
    case GAL_SPECLINES_C_III_4650:     return GAL_SPECLINES_NAME_C_III_4650;
    case GAL_SPECLINES_C_III_5651:     return GAL_SPECLINES_NAME_C_III_5651;
    case GAL_SPECLINES_Fe_III_4658:    return GAL_SPECLINES_NAME_Fe_III_4658;
    case GAL_SPECLINES_He_II_4686:     return GAL_SPECLINES_NAME_He_II_4686;
    case GAL_SPECLINES_Ar_IV_4711:     return GAL_SPECLINES_NAME_Ar_IV_4711;
    case GAL_SPECLINES_Ar_IV_4740:     return GAL_SPECLINES_NAME_Ar_IV_4740;
    case GAL_SPECLINES_H_beta:         return GAL_SPECLINES_NAME_H_beta;
    case GAL_SPECLINES_Fe_VII_4893:    return GAL_SPECLINES_NAME_Fe_VII_4893;
    case GAL_SPECLINES_Fe_IV_4903:     return GAL_SPECLINES_NAME_Fe_IV_4903;
    case GAL_SPECLINES_Fe_II_4924:     return GAL_SPECLINES_NAME_Fe_II_4924;
    case GAL_SPECLINES_O_III_4959:     return GAL_SPECLINES_NAME_O_III_4959;
    case GAL_SPECLINES_O_III_5007:     return GAL_SPECLINES_NAME_O_III_5007;
    case GAL_SPECLINES_Fe_II_5018:     return GAL_SPECLINES_NAME_Fe_II_5018;
    case GAL_SPECLINES_Fe_III_5085:    return GAL_SPECLINES_NAME_Fe_III_5085;
    case GAL_SPECLINES_Fe_VI_5146:     return GAL_SPECLINES_NAME_Fe_VI_5146;
    case GAL_SPECLINES_Fe_VII_5159:    return GAL_SPECLINES_NAME_Fe_VII_5159;
    case GAL_SPECLINES_Fe_II_5169:     return GAL_SPECLINES_NAME_Fe_II_5169;
    case GAL_SPECLINES_Fe_VI_5176:     return GAL_SPECLINES_NAME_Fe_VI_5176;
    case GAL_SPECLINES_Fe_II_5198:     return GAL_SPECLINES_NAME_Fe_II_5198;
    case GAL_SPECLINES_N_I_5200:       return GAL_SPECLINES_NAME_N_I_5200;
    case GAL_SPECLINES_Fe_II_5235:     return GAL_SPECLINES_NAME_Fe_II_5235;
    case GAL_SPECLINES_Fe_IV_5236:     return GAL_SPECLINES_NAME_Fe_IV_5236;
    case GAL_SPECLINES_Fe_III_5270:    return GAL_SPECLINES_NAME_Fe_III_5270;
    case GAL_SPECLINES_Fe_II_5276:     return GAL_SPECLINES_NAME_Fe_II_5276;
    case GAL_SPECLINES_Fe_VII_5276:    return GAL_SPECLINES_NAME_Fe_VII_5276;
    case GAL_SPECLINES_Fe_XIV:         return GAL_SPECLINES_NAME_Fe_XIV;
    case GAL_SPECLINES_Ca_V:           return GAL_SPECLINES_NAME_Ca_V;
    case GAL_SPECLINES_Fe_II_5316_62:  return GAL_SPECLINES_NAME_Fe_II_5316_62;
    case GAL_SPECLINES_Fe_II_5316_78:  return GAL_SPECLINES_NAME_Fe_II_5316_78;
    case GAL_SPECLINES_Fe_VI_5335:     return GAL_SPECLINES_NAME_Fe_VI_5335;
    case GAL_SPECLINES_Fe_VI_5424:     return GAL_SPECLINES_NAME_Fe_VI_5424;
    case GAL_SPECLINES_Cl_III_5518:    return GAL_SPECLINES_NAME_Cl_III_5518;
    case GAL_SPECLINES_Cl_III_5538:    return GAL_SPECLINES_NAME_Cl_III_5538;
    case GAL_SPECLINES_Fe_VI_5638:     return GAL_SPECLINES_NAME_Fe_VI_5638;
    case GAL_SPECLINES_Fe_VI_5677:     return GAL_SPECLINES_NAME_Fe_VI_5677;
    case GAL_SPECLINES_C_III_5698:     return GAL_SPECLINES_NAME_C_III_5698;
    case GAL_SPECLINES_Fe_VII_5721:    return GAL_SPECLINES_NAME_Fe_VII_5721;
    case GAL_SPECLINES_N_II_5755:      return GAL_SPECLINES_NAME_N_II_5755;
    case GAL_SPECLINES_C_IV_5801:      return GAL_SPECLINES_NAME_C_IV_5801;
    case GAL_SPECLINES_C_IV_5812:      return GAL_SPECLINES_NAME_C_IV_5812;
    case GAL_SPECLINES_He_I_5876:      return GAL_SPECLINES_NAME_He_I_5876;
    case GAL_SPECLINES_O_I_6046:       return GAL_SPECLINES_NAME_O_I_6046;
    case GAL_SPECLINES_Fe_VII_6087:    return GAL_SPECLINES_NAME_Fe_VII_6087;
    case GAL_SPECLINES_O_I_6300:       return GAL_SPECLINES_NAME_O_I_6300;
    case GAL_SPECLINES_S_III_6312:     return GAL_SPECLINES_NAME_S_III_6312;
    case GAL_SPECLINES_Si_II_6347:     return GAL_SPECLINES_NAME_Si_II_6347;
    case GAL_SPECLINES_O_I_6364:       return GAL_SPECLINES_NAME_O_I_6364;
    case GAL_SPECLINES_Fe_II_6369:     return GAL_SPECLINES_NAME_Fe_II_6369;
    case GAL_SPECLINES_Fe_X:           return GAL_SPECLINES_NAME_Fe_X;
    case GAL_SPECLINES_Fe_II_6516:     return GAL_SPECLINES_NAME_Fe_II_6516;
    case GAL_SPECLINES_N_II_6548:      return GAL_SPECLINES_NAME_N_II_6548;
    case GAL_SPECLINES_H_alpha:        return GAL_SPECLINES_NAME_H_alpha;
    case GAL_SPECLINES_N_II_6583:      return GAL_SPECLINES_NAME_N_II_6583;
    case GAL_SPECLINES_S_II_6716:      return GAL_SPECLINES_NAME_S_II_6716;
    case GAL_SPECLINES_S_II_6731:      return GAL_SPECLINES_NAME_S_II_6731;
    case GAL_SPECLINES_O_I_7002:       return GAL_SPECLINES_NAME_O_I_7002;
    case GAL_SPECLINES_Ar_V:           return GAL_SPECLINES_NAME_Ar_V;
    case GAL_SPECLINES_He_I_7065:      return GAL_SPECLINES_NAME_He_I_7065;
    case GAL_SPECLINES_Ar_III_7136:    return GAL_SPECLINES_NAME_Ar_III_7136;
    case GAL_SPECLINES_Fe_II_7155:     return GAL_SPECLINES_NAME_Fe_II_7155;
    case GAL_SPECLINES_Ar_IV_7171:     return GAL_SPECLINES_NAME_Ar_IV_7171;
    case GAL_SPECLINES_Fe_II_7172:     return GAL_SPECLINES_NAME_Fe_II_7172;
    case GAL_SPECLINES_C_II_7236:      return GAL_SPECLINES_NAME_C_II_7236;
    case GAL_SPECLINES_Ar_IV_7237:     return GAL_SPECLINES_NAME_Ar_IV_7237;
    case GAL_SPECLINES_O_I_7254:       return GAL_SPECLINES_NAME_O_I_7254;
    case GAL_SPECLINES_Ar_IV_7263:     return GAL_SPECLINES_NAME_Ar_IV_7263;
    case GAL_SPECLINES_He_I_7281:      return GAL_SPECLINES_NAME_He_I_7281;
    case GAL_SPECLINES_O_II_7320:      return GAL_SPECLINES_NAME_O_II_7320;
    case GAL_SPECLINES_O_II_7331:      return GAL_SPECLINES_NAME_O_II_7331;
    case GAL_SPECLINES_Ni_II_7378:     return GAL_SPECLINES_NAME_Ni_II_7378;
    case GAL_SPECLINES_Ni_II_7411:     return GAL_SPECLINES_NAME_Ni_II_7411;
    case GAL_SPECLINES_Fe_II_7453:     return GAL_SPECLINES_NAME_Fe_II_7453;
    case GAL_SPECLINES_N_I_7468:       return GAL_SPECLINES_NAME_N_I_7468;
    case GAL_SPECLINES_S_XII:          return GAL_SPECLINES_NAME_S_XII;
    case GAL_SPECLINES_Ar_III_7751:    return GAL_SPECLINES_NAME_Ar_III_7751;
    case GAL_SPECLINES_He_I_7816:      return GAL_SPECLINES_NAME_He_I_7816;
    case GAL_SPECLINES_Ar_I_7868:      return GAL_SPECLINES_NAME_Ar_I_7868;
    case GAL_SPECLINES_Ni_III:         return GAL_SPECLINES_NAME_Ni_III;
    case GAL_SPECLINES_Fe_XI_7892:     return GAL_SPECLINES_NAME_Fe_XI_7892;
    case GAL_SPECLINES_He_II_8237:     return GAL_SPECLINES_NAME_He_II_8237;
    case GAL_SPECLINES_Pa_20:          return GAL_SPECLINES_NAME_Pa_20;
    case GAL_SPECLINES_Pa_19:          return GAL_SPECLINES_NAME_Pa_19;
    case GAL_SPECLINES_Pa_18:          return GAL_SPECLINES_NAME_Pa_18;
    case GAL_SPECLINES_O_I_8446:       return GAL_SPECLINES_NAME_O_I_8446;
    case GAL_SPECLINES_Pa_17:          return GAL_SPECLINES_NAME_Pa_17;
    case GAL_SPECLINES_Ca_II_8498:     return GAL_SPECLINES_NAME_Ca_II_8498;
    case GAL_SPECLINES_Pa_16:          return GAL_SPECLINES_NAME_Pa_16;
    case GAL_SPECLINES_Ca_II_8542:     return GAL_SPECLINES_NAME_Ca_II_8542;
    case GAL_SPECLINES_Pa_15:          return GAL_SPECLINES_NAME_Pa_15;
    case GAL_SPECLINES_Cl_II:          return GAL_SPECLINES_NAME_Cl_II;
    case GAL_SPECLINES_Pa_14:          return GAL_SPECLINES_NAME_Pa_14;
    case GAL_SPECLINES_Fe_II_8617:     return GAL_SPECLINES_NAME_Fe_II_8617;
    case GAL_SPECLINES_Ca_II_8662:     return GAL_SPECLINES_NAME_Ca_II_8662;
    case GAL_SPECLINES_Pa_13:          return GAL_SPECLINES_NAME_Pa_13;
    case GAL_SPECLINES_N_I_8680:       return GAL_SPECLINES_NAME_N_I_8680;
    case GAL_SPECLINES_N_I_8703:       return GAL_SPECLINES_NAME_N_I_8703;
    case GAL_SPECLINES_N_I_8712:       return GAL_SPECLINES_NAME_N_I_8712;
    case GAL_SPECLINES_Pa_12:          return GAL_SPECLINES_NAME_Pa_12;
    case GAL_SPECLINES_Pa_11:          return GAL_SPECLINES_NAME_Pa_11;
    case GAL_SPECLINES_Fe_II_8892:     return GAL_SPECLINES_NAME_Fe_II_8892;
    case GAL_SPECLINES_Pa_10:          return GAL_SPECLINES_NAME_Pa_10;
    case GAL_SPECLINES_S_III_9069:     return GAL_SPECLINES_NAME_S_III_9069;
    case GAL_SPECLINES_Pa_9:           return GAL_SPECLINES_NAME_Pa_9;
    case GAL_SPECLINES_S_III_9531:     return GAL_SPECLINES_NAME_S_III_9531;
    case GAL_SPECLINES_Pa_epsilon:     return GAL_SPECLINES_NAME_Pa_epsilon;
    case GAL_SPECLINES_C_I_9824:       return GAL_SPECLINES_NAME_C_I_9824;
    case GAL_SPECLINES_C_I_9850:       return GAL_SPECLINES_NAME_C_I_9850;
    case GAL_SPECLINES_S_VIII:         return GAL_SPECLINES_NAME_S_VIII;
    case GAL_SPECLINES_He_I_10028:     return GAL_SPECLINES_NAME_He_I_10028;
    case GAL_SPECLINES_He_I_10031:     return GAL_SPECLINES_NAME_He_I_10031;
    case GAL_SPECLINES_Pa_delta:       return GAL_SPECLINES_NAME_Pa_delta;
    case GAL_SPECLINES_S_II_10287:     return GAL_SPECLINES_NAME_S_II_10287;
    case GAL_SPECLINES_S_II_10320:     return GAL_SPECLINES_NAME_S_II_10320;
    case GAL_SPECLINES_S_II_10336:     return GAL_SPECLINES_NAME_S_II_10336;
    case GAL_SPECLINES_Fe_XIII:        return GAL_SPECLINES_NAME_Fe_XIII;
    case GAL_SPECLINES_He_I_10830:     return GAL_SPECLINES_NAME_He_I_10830;
    case GAL_SPECLINES_Pa_gamma:       return GAL_SPECLINES_NAME_Pa_gamma;

    /* Limits. */
    case GAL_SPECLINES_LIMIT_LYMAN:    return GAL_SPECLINES_NAME_LIMIT_LYMAN;
    case GAL_SPECLINES_LIMIT_BALMER:   return GAL_SPECLINES_NAME_LIMIT_BALMER;
    case GAL_SPECLINES_LIMIT_PASCHEN:  return GAL_SPECLINES_NAME_LIMIT_PASCHEN;
    default: return NULL;
    }
  return NULL;
}





/* Return the code of the given line name. */
int
gal_speclines_line_code(char *name)
{
  if(      !strcmp(name, GAL_SPECLINES_NAME_Ne_VIII_770) )
    return GAL_SPECLINES_Ne_VIII_770;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ne_VIII_780) )
    return GAL_SPECLINES_Ne_VIII_780;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ly_epsilon) )
    return GAL_SPECLINES_Ly_epsilon;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ly_delta) )
    return GAL_SPECLINES_Ly_delta;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ly_gamma) )
    return GAL_SPECLINES_Ly_gamma;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_III_977) )
    return GAL_SPECLINES_C_III_977;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_990) )
    return GAL_SPECLINES_N_III_990;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_991_51) )
    return GAL_SPECLINES_N_III_991_51;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_991_58) )
    return GAL_SPECLINES_N_III_991_58;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ly_beta) )
    return GAL_SPECLINES_Ly_beta;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_VI_1032) )
    return GAL_SPECLINES_O_VI_1032;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_VI_1038) )
    return GAL_SPECLINES_O_VI_1038;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_I_1067) )
    return GAL_SPECLINES_Ar_I_1067;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ly_alpha) )
    return GAL_SPECLINES_Ly_alpha;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_V_1238) )
    return GAL_SPECLINES_N_V_1238;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_V_1243) )
    return GAL_SPECLINES_N_V_1243;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Si_II_1260) )
    return GAL_SPECLINES_Si_II_1260;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Si_II_1265) )
    return GAL_SPECLINES_Si_II_1265;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_I_1302) )
    return GAL_SPECLINES_O_I_1302;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_II_1335) )
    return GAL_SPECLINES_C_II_1335;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_II_1336) )
    return GAL_SPECLINES_C_II_1336;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Si_IV_1394) )
    return GAL_SPECLINES_Si_IV_1394;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_IV_1397) )
    return GAL_SPECLINES_O_IV_1397;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_IV_1400) )
    return GAL_SPECLINES_O_IV_1400;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Si_IV_1403) )
    return GAL_SPECLINES_Si_IV_1403;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_IV_1486) )
    return GAL_SPECLINES_N_IV_1486;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_IV_1548) )
    return GAL_SPECLINES_C_IV_1548;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_IV_1551) )
    return GAL_SPECLINES_C_IV_1551;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_II_1640) )
    return GAL_SPECLINES_He_II_1640;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_1661) )
    return GAL_SPECLINES_O_III_1661;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_1666) )
    return GAL_SPECLINES_O_III_1666;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_1747) )
    return GAL_SPECLINES_N_III_1747;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_1749) )
    return GAL_SPECLINES_N_III_1749;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Al_III_1855) )
    return GAL_SPECLINES_Al_III_1855;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Al_III_1863) )
    return GAL_SPECLINES_Al_III_1863;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Si_III) )
    return GAL_SPECLINES_Si_III;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_III_1909) )
    return GAL_SPECLINES_C_III_1909;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_II_2143) )
    return GAL_SPECLINES_N_II_2143;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_2321) )
    return GAL_SPECLINES_O_III_2321;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_II_2324) )
    return GAL_SPECLINES_C_II_2324;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_II_2325) )
    return GAL_SPECLINES_C_II_2325;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_XI_2649) )
    return GAL_SPECLINES_Fe_XI_2649;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_II_2733) )
    return GAL_SPECLINES_He_II_2733;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Mg_V_2783) )
    return GAL_SPECLINES_Mg_V_2783;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Mg_II_2796) )
    return GAL_SPECLINES_Mg_II_2796;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Mg_II_2803) )
    return GAL_SPECLINES_Mg_II_2803;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_IV_2829) )
    return GAL_SPECLINES_Fe_IV_2829;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_IV_2836) )
    return GAL_SPECLINES_Fe_IV_2836;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_IV_2854) )
    return GAL_SPECLINES_Ar_IV_2854;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_IV_2868) )
    return GAL_SPECLINES_Ar_IV_2868;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Mg_V_2928) )
    return GAL_SPECLINES_Mg_V_2928;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_2945) )
    return GAL_SPECLINES_He_I_2945;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_3133) )
    return GAL_SPECLINES_O_III_3133;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_3188) )
    return GAL_SPECLINES_He_I_3188;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_II_3203) )
    return GAL_SPECLINES_He_II_3203;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_3312) )
    return GAL_SPECLINES_O_III_3312;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ne_V_3346) )
    return GAL_SPECLINES_Ne_V_3346;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ne_V_3426) )
    return GAL_SPECLINES_Ne_V_3426;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_3444) )
    return GAL_SPECLINES_O_III_3444;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_I_3466_50) )
    return GAL_SPECLINES_N_I_3466_50;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_I_3466_54) )
    return GAL_SPECLINES_N_I_3466_54;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_3488) )
    return GAL_SPECLINES_He_I_3488;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VII_3586) )
    return GAL_SPECLINES_Fe_VII_3586;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VI_3663) )
    return GAL_SPECLINES_Fe_VI_3663;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_19) )
    return GAL_SPECLINES_H_19;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_18) )
    return GAL_SPECLINES_H_18;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_17) )
    return GAL_SPECLINES_H_17;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_16) )
    return GAL_SPECLINES_H_16;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_15) )
    return GAL_SPECLINES_H_15;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_14) )
    return GAL_SPECLINES_H_14;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_II_3726) )
    return GAL_SPECLINES_O_II_3726;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_II_3729) )
    return GAL_SPECLINES_O_II_3729;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_13) )
    return GAL_SPECLINES_H_13;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_12) )
    return GAL_SPECLINES_H_12;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VII_3759) )
    return GAL_SPECLINES_Fe_VII_3759;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_11) )
    return GAL_SPECLINES_H_11;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_10) )
    return GAL_SPECLINES_H_10;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_9) )
    return GAL_SPECLINES_H_9;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_V_3839) )
    return GAL_SPECLINES_Fe_V_3839;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ne_III_3869) )
    return GAL_SPECLINES_Ne_III_3869;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_3889) )
    return GAL_SPECLINES_He_I_3889;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_8) )
    return GAL_SPECLINES_H_8;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_V_3891) )
    return GAL_SPECLINES_Fe_V_3891;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_V_3911) )
    return GAL_SPECLINES_Fe_V_3911;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ne_III_3967) )
    return GAL_SPECLINES_Ne_III_3967;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_epsilon) )
    return GAL_SPECLINES_H_epsilon;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_4026) )
    return GAL_SPECLINES_He_I_4026;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_II_4069) )
    return GAL_SPECLINES_S_II_4069;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_V_4071) )
    return GAL_SPECLINES_Fe_V_4071;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_II_4076) )
    return GAL_SPECLINES_S_II_4076;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_delta) )
    return GAL_SPECLINES_H_delta;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_4144) )
    return GAL_SPECLINES_He_I_4144;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4179) )
    return GAL_SPECLINES_Fe_II_4179;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_V_4181) )
    return GAL_SPECLINES_Fe_V_4181;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4233) )
    return GAL_SPECLINES_Fe_II_4233;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_V_4227) )
    return GAL_SPECLINES_Fe_V_4227;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4287) )
    return GAL_SPECLINES_Fe_II_4287;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4304) )
    return GAL_SPECLINES_Fe_II_4304;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_II_4317) )
    return GAL_SPECLINES_O_II_4317;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_gamma) )
    return GAL_SPECLINES_H_gamma;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_4363) )
    return GAL_SPECLINES_O_III_4363;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_XIV) )
    return GAL_SPECLINES_Ar_XIV;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_II_4415) )
    return GAL_SPECLINES_O_II_4415;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4417) )
    return GAL_SPECLINES_Fe_II_4417;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4452) )
    return GAL_SPECLINES_Fe_II_4452;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_4471) )
    return GAL_SPECLINES_He_I_4471;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4489) )
    return GAL_SPECLINES_Fe_II_4489;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4491) )
    return GAL_SPECLINES_Fe_II_4491;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_4510) )
    return GAL_SPECLINES_N_III_4510;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4523) )
    return GAL_SPECLINES_Fe_II_4523;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4556) )
    return GAL_SPECLINES_Fe_II_4556;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4583) )
    return GAL_SPECLINES_Fe_II_4583;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4584) )
    return GAL_SPECLINES_Fe_II_4584;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4630) )
    return GAL_SPECLINES_Fe_II_4630;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_4634) )
    return GAL_SPECLINES_N_III_4634;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_4641) )
    return GAL_SPECLINES_N_III_4641;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_III_4642) )
    return GAL_SPECLINES_N_III_4642;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_III_4647) )
    return GAL_SPECLINES_C_III_4647;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_III_4650) )
    return GAL_SPECLINES_C_III_4650;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_III_5651) )
    return GAL_SPECLINES_C_III_5651;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_III_4658) )
    return GAL_SPECLINES_Fe_III_4658;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_II_4686) )
    return GAL_SPECLINES_He_II_4686;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_IV_4711) )
    return GAL_SPECLINES_Ar_IV_4711;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_IV_4740) )
    return GAL_SPECLINES_Ar_IV_4740;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_beta) )
    return GAL_SPECLINES_H_beta;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VII_4893) )
    return GAL_SPECLINES_Fe_VII_4893;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_IV_4903) )
    return GAL_SPECLINES_Fe_IV_4903;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_4924) )
    return GAL_SPECLINES_Fe_II_4924;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_4959) )
    return GAL_SPECLINES_O_III_4959;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_III_5007) )
    return GAL_SPECLINES_O_III_5007;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_5018) )
    return GAL_SPECLINES_Fe_II_5018;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_III_5085) )
    return GAL_SPECLINES_Fe_III_5085;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VI_5146) )
    return GAL_SPECLINES_Fe_VI_5146;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VII_5159) )
    return GAL_SPECLINES_Fe_VII_5159;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_5169) )
    return GAL_SPECLINES_Fe_II_5169;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VI_5176) )
    return GAL_SPECLINES_Fe_VI_5176;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_5198) )
    return GAL_SPECLINES_Fe_II_5198;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_I_5200) )
    return GAL_SPECLINES_N_I_5200;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_5235) )
    return GAL_SPECLINES_Fe_II_5235;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_IV_5236) )
    return GAL_SPECLINES_Fe_IV_5236;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_III_5270) )
    return GAL_SPECLINES_Fe_III_5270;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_5276) )
    return GAL_SPECLINES_Fe_II_5276;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VII_5276) )
    return GAL_SPECLINES_Fe_VII_5276;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_XIV) )
    return GAL_SPECLINES_Fe_XIV;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ca_V) )
    return GAL_SPECLINES_Ca_V;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_5316_62) )
    return GAL_SPECLINES_Fe_II_5316_62;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_5316_78) )
    return GAL_SPECLINES_Fe_II_5316_78;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VI_5335) )
    return GAL_SPECLINES_Fe_VI_5335;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VI_5424) )
    return GAL_SPECLINES_Fe_VI_5424;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Cl_III_5518) )
    return GAL_SPECLINES_Cl_III_5518;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Cl_III_5538) )
    return GAL_SPECLINES_Cl_III_5538;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VI_5638) )
    return GAL_SPECLINES_Fe_VI_5638;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VI_5677) )
    return GAL_SPECLINES_Fe_VI_5677;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_III_5698) )
    return GAL_SPECLINES_C_III_5698;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VII_5721) )
    return GAL_SPECLINES_Fe_VII_5721;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_II_5755) )
    return GAL_SPECLINES_N_II_5755;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_IV_5801) )
    return GAL_SPECLINES_C_IV_5801;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_IV_5812) )
    return GAL_SPECLINES_C_IV_5812;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_5876) )
    return GAL_SPECLINES_He_I_5876;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_I_6046) )
    return GAL_SPECLINES_O_I_6046;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_VII_6087) )
    return GAL_SPECLINES_Fe_VII_6087;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_I_6300) )
    return GAL_SPECLINES_O_I_6300;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_III_6312) )
    return GAL_SPECLINES_S_III_6312;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Si_II_6347) )
    return GAL_SPECLINES_Si_II_6347;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_I_6364) )
    return GAL_SPECLINES_O_I_6364;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_6369) )
    return GAL_SPECLINES_Fe_II_6369;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_X) )
    return GAL_SPECLINES_Fe_X;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_6516) )
    return GAL_SPECLINES_Fe_II_6516;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_II_6548) )
    return GAL_SPECLINES_N_II_6548;
  else if( !strcmp(name, GAL_SPECLINES_NAME_H_alpha) )
    return GAL_SPECLINES_H_alpha;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_II_6583) )
    return GAL_SPECLINES_N_II_6583;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_II_6716) )
    return GAL_SPECLINES_S_II_6716;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_II_6731) )
    return GAL_SPECLINES_S_II_6731;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_I_7002) )
    return GAL_SPECLINES_O_I_7002;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_V) )
    return GAL_SPECLINES_Ar_V;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_7065) )
    return GAL_SPECLINES_He_I_7065;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_III_7136) )
    return GAL_SPECLINES_Ar_III_7136;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_7155) )
    return GAL_SPECLINES_Fe_II_7155;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_IV_7171) )
    return GAL_SPECLINES_Ar_IV_7171;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_7172) )
    return GAL_SPECLINES_Fe_II_7172;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_II_7236) )
    return GAL_SPECLINES_C_II_7236;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_IV_7237) )
    return GAL_SPECLINES_Ar_IV_7237;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_I_7254) )
    return GAL_SPECLINES_O_I_7254;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_IV_7263) )
    return GAL_SPECLINES_Ar_IV_7263;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_7281) )
    return GAL_SPECLINES_He_I_7281;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_II_7320) )
    return GAL_SPECLINES_O_II_7320;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_II_7331) )
    return GAL_SPECLINES_O_II_7331;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ni_II_7378) )
    return GAL_SPECLINES_Ni_II_7378;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ni_II_7411) )
    return GAL_SPECLINES_Ni_II_7411;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_7453) )
    return GAL_SPECLINES_Fe_II_7453;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_I_7468) )
    return GAL_SPECLINES_N_I_7468;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_XII) )
    return GAL_SPECLINES_S_XII;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_III_7751) )
    return GAL_SPECLINES_Ar_III_7751;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_7816) )
    return GAL_SPECLINES_He_I_7816;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ar_I_7868) )
    return GAL_SPECLINES_Ar_I_7868;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ni_III) )
    return GAL_SPECLINES_Ni_III;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_XI_7892) )
    return GAL_SPECLINES_Fe_XI_7892;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_II_8237) )
    return GAL_SPECLINES_He_II_8237;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_20) )
    return GAL_SPECLINES_Pa_20;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_19) )
    return GAL_SPECLINES_Pa_19;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_18) )
    return GAL_SPECLINES_Pa_18;
  else if( !strcmp(name, GAL_SPECLINES_NAME_O_I_8446) )
    return GAL_SPECLINES_O_I_8446;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_17) )
    return GAL_SPECLINES_Pa_17;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ca_II_8498) )
    return GAL_SPECLINES_Ca_II_8498;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_16) )
    return GAL_SPECLINES_Pa_16;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ca_II_8542) )
    return GAL_SPECLINES_Ca_II_8542;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_15) )
    return GAL_SPECLINES_Pa_15;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Cl_II) )
    return GAL_SPECLINES_Cl_II;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_14) )
    return GAL_SPECLINES_Pa_14;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_8617) )
    return GAL_SPECLINES_Fe_II_8617;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Ca_II_8662) )
    return GAL_SPECLINES_Ca_II_8662;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_13) )
    return GAL_SPECLINES_Pa_13;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_I_8680) )
    return GAL_SPECLINES_N_I_8680;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_I_8703) )
    return GAL_SPECLINES_N_I_8703;
  else if( !strcmp(name, GAL_SPECLINES_NAME_N_I_8712) )
    return GAL_SPECLINES_N_I_8712;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_12) )
    return GAL_SPECLINES_Pa_12;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_11) )
    return GAL_SPECLINES_Pa_11;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_II_8892) )
    return GAL_SPECLINES_Fe_II_8892;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_10) )
    return GAL_SPECLINES_Pa_10;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_III_9069) )
    return GAL_SPECLINES_S_III_9069;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_9) )
    return GAL_SPECLINES_Pa_9;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_III_9531) )
    return GAL_SPECLINES_S_III_9531;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_epsilon) )
    return GAL_SPECLINES_Pa_epsilon;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_I_9824) )
    return GAL_SPECLINES_C_I_9824;
  else if( !strcmp(name, GAL_SPECLINES_NAME_C_I_9850) )
    return GAL_SPECLINES_C_I_9850;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_VIII) )
    return GAL_SPECLINES_S_VIII;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_10028) )
    return GAL_SPECLINES_He_I_10028;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_10031) )
    return GAL_SPECLINES_He_I_10031;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_delta) )
    return GAL_SPECLINES_Pa_delta;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_II_10287) )
    return GAL_SPECLINES_S_II_10287;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_II_10320) )
    return GAL_SPECLINES_S_II_10320;
  else if( !strcmp(name, GAL_SPECLINES_NAME_S_II_10336) )
    return GAL_SPECLINES_S_II_10336;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Fe_XIII) )
    return GAL_SPECLINES_Fe_XIII;
  else if( !strcmp(name, GAL_SPECLINES_NAME_He_I_10830) )
    return GAL_SPECLINES_He_I_10830;
  else if( !strcmp(name, GAL_SPECLINES_NAME_Pa_gamma) )
    return GAL_SPECLINES_Pa_gamma;

  /* Limits. */
  else if( !strcmp(name, GAL_SPECLINES_NAME_LIMIT_LYMAN) )
    return GAL_SPECLINES_LIMIT_LYMAN;
  else if( !strcmp(name, GAL_SPECLINES_NAME_LIMIT_BALMER) )
    return GAL_SPECLINES_LIMIT_BALMER;
  else if( !strcmp(name, GAL_SPECLINES_NAME_LIMIT_PASCHEN) )
    return GAL_SPECLINES_LIMIT_PASCHEN;

  /* Invalid. */
  else return GAL_SPECLINES_INVALID;
  return GAL_SPECLINES_INVALID;
}





/* Return the wavelength (in Angstroms) of given line. */
double
gal_speclines_line_angstrom(int linecode)
{
  switch(linecode)
    {
    case GAL_SPECLINES_Ne_VIII_770:   return GAL_SPECLINES_ANGSTROM_Ne_VIII_770;
    case GAL_SPECLINES_Ne_VIII_780:   return GAL_SPECLINES_ANGSTROM_Ne_VIII_780;
    case GAL_SPECLINES_Ly_epsilon:    return GAL_SPECLINES_ANGSTROM_Ly_epsilon;
    case GAL_SPECLINES_Ly_delta:      return GAL_SPECLINES_ANGSTROM_Ly_delta;
    case GAL_SPECLINES_Ly_gamma:      return GAL_SPECLINES_ANGSTROM_Ly_gamma;
    case GAL_SPECLINES_C_III_977:     return GAL_SPECLINES_ANGSTROM_C_III_977;
    case GAL_SPECLINES_N_III_990:     return GAL_SPECLINES_ANGSTROM_N_III_990;
    case GAL_SPECLINES_N_III_991_51:  return GAL_SPECLINES_ANGSTROM_N_III_991_51;
    case GAL_SPECLINES_N_III_991_58:  return GAL_SPECLINES_ANGSTROM_N_III_991_58;
    case GAL_SPECLINES_Ly_beta:       return GAL_SPECLINES_ANGSTROM_Ly_beta;
    case GAL_SPECLINES_O_VI_1032:     return GAL_SPECLINES_ANGSTROM_O_VI_1032;
    case GAL_SPECLINES_O_VI_1038:     return GAL_SPECLINES_ANGSTROM_O_VI_1038;
    case GAL_SPECLINES_Ar_I_1067:     return GAL_SPECLINES_ANGSTROM_Ar_I_1067;
    case GAL_SPECLINES_Ly_alpha:      return GAL_SPECLINES_ANGSTROM_Ly_alpha;
    case GAL_SPECLINES_N_V_1238:      return GAL_SPECLINES_ANGSTROM_N_V_1238;
    case GAL_SPECLINES_N_V_1243:      return GAL_SPECLINES_ANGSTROM_N_V_1243;
    case GAL_SPECLINES_Si_II_1260:    return GAL_SPECLINES_ANGSTROM_Si_II_1260;
    case GAL_SPECLINES_Si_II_1265:    return GAL_SPECLINES_ANGSTROM_Si_II_1265;
    case GAL_SPECLINES_O_I_1302:      return GAL_SPECLINES_ANGSTROM_O_I_1302;
    case GAL_SPECLINES_C_II_1335:     return GAL_SPECLINES_ANGSTROM_C_II_1335;
    case GAL_SPECLINES_C_II_1336:     return GAL_SPECLINES_ANGSTROM_C_II_1336;
    case GAL_SPECLINES_Si_IV_1394:    return GAL_SPECLINES_ANGSTROM_Si_IV_1394;
    case GAL_SPECLINES_O_IV_1397:     return GAL_SPECLINES_ANGSTROM_O_IV_1397;
    case GAL_SPECLINES_O_IV_1400:     return GAL_SPECLINES_ANGSTROM_O_IV_1400;
    case GAL_SPECLINES_Si_IV_1403:    return GAL_SPECLINES_ANGSTROM_Si_IV_1403;
    case GAL_SPECLINES_N_IV_1486:     return GAL_SPECLINES_ANGSTROM_N_IV_1486;
    case GAL_SPECLINES_C_IV_1548:     return GAL_SPECLINES_ANGSTROM_C_IV_1548;
    case GAL_SPECLINES_C_IV_1551:     return GAL_SPECLINES_ANGSTROM_C_IV_1551;
    case GAL_SPECLINES_He_II_1640:    return GAL_SPECLINES_ANGSTROM_He_II_1640;
    case GAL_SPECLINES_O_III_1661:    return GAL_SPECLINES_ANGSTROM_O_III_1661;
    case GAL_SPECLINES_O_III_1666:    return GAL_SPECLINES_ANGSTROM_O_III_1666;
    case GAL_SPECLINES_N_III_1747:    return GAL_SPECLINES_ANGSTROM_N_III_1747;
    case GAL_SPECLINES_N_III_1749:    return GAL_SPECLINES_ANGSTROM_N_III_1749;
    case GAL_SPECLINES_Al_III_1855:   return GAL_SPECLINES_ANGSTROM_Al_III_1855;
    case GAL_SPECLINES_Al_III_1863:   return GAL_SPECLINES_ANGSTROM_Al_III_1863;
    case GAL_SPECLINES_Si_III:        return GAL_SPECLINES_ANGSTROM_Si_III;
    case GAL_SPECLINES_C_III_1909:    return GAL_SPECLINES_ANGSTROM_C_III_1909;
    case GAL_SPECLINES_N_II_2143:     return GAL_SPECLINES_ANGSTROM_N_II_2143;
    case GAL_SPECLINES_O_III_2321:    return GAL_SPECLINES_ANGSTROM_O_III_2321;
    case GAL_SPECLINES_C_II_2324:     return GAL_SPECLINES_ANGSTROM_C_II_2324;
    case GAL_SPECLINES_C_II_2325:     return GAL_SPECLINES_ANGSTROM_C_II_2325;
    case GAL_SPECLINES_Fe_XI_2649:    return GAL_SPECLINES_ANGSTROM_Fe_XI_2649;
    case GAL_SPECLINES_He_II_2733:    return GAL_SPECLINES_ANGSTROM_He_II_2733;
    case GAL_SPECLINES_Mg_V_2783:     return GAL_SPECLINES_ANGSTROM_Mg_V_2783;
    case GAL_SPECLINES_Mg_II_2796:    return GAL_SPECLINES_ANGSTROM_Mg_II_2796;
    case GAL_SPECLINES_Mg_II_2803:    return GAL_SPECLINES_ANGSTROM_Mg_II_2803;
    case GAL_SPECLINES_Fe_IV_2829:    return GAL_SPECLINES_ANGSTROM_Fe_IV_2829;
    case GAL_SPECLINES_Fe_IV_2836:    return GAL_SPECLINES_ANGSTROM_Fe_IV_2836;
    case GAL_SPECLINES_Ar_IV_2854:    return GAL_SPECLINES_ANGSTROM_Ar_IV_2854;
    case GAL_SPECLINES_Ar_IV_2868:    return GAL_SPECLINES_ANGSTROM_Ar_IV_2868;
    case GAL_SPECLINES_Mg_V_2928:     return GAL_SPECLINES_ANGSTROM_Mg_V_2928;
    case GAL_SPECLINES_He_I_2945:     return GAL_SPECLINES_ANGSTROM_He_I_2945;
    case GAL_SPECLINES_O_III_3133:    return GAL_SPECLINES_ANGSTROM_O_III_3133;
    case GAL_SPECLINES_He_I_3188:     return GAL_SPECLINES_ANGSTROM_He_I_3188;
    case GAL_SPECLINES_He_II_3203:    return GAL_SPECLINES_ANGSTROM_He_II_3203;
    case GAL_SPECLINES_O_III_3312:    return GAL_SPECLINES_ANGSTROM_O_III_3312;
    case GAL_SPECLINES_Ne_V_3346:     return GAL_SPECLINES_ANGSTROM_Ne_V_3346;
    case GAL_SPECLINES_Ne_V_3426:     return GAL_SPECLINES_ANGSTROM_Ne_V_3426;
    case GAL_SPECLINES_O_III_3444:    return GAL_SPECLINES_ANGSTROM_O_III_3444;
    case GAL_SPECLINES_N_I_3466_50:   return GAL_SPECLINES_ANGSTROM_N_I_3466_50;
    case GAL_SPECLINES_N_I_3466_54:   return GAL_SPECLINES_ANGSTROM_N_I_3466_54;
    case GAL_SPECLINES_He_I_3488:     return GAL_SPECLINES_ANGSTROM_He_I_3488;
    case GAL_SPECLINES_Fe_VII_3586:   return GAL_SPECLINES_ANGSTROM_Fe_VII_3586;
    case GAL_SPECLINES_Fe_VI_3663:    return GAL_SPECLINES_ANGSTROM_Fe_VI_3663;
    case GAL_SPECLINES_H_19:          return GAL_SPECLINES_ANGSTROM_H_19;
    case GAL_SPECLINES_H_18:          return GAL_SPECLINES_ANGSTROM_H_18;
    case GAL_SPECLINES_H_17:          return GAL_SPECLINES_ANGSTROM_H_17;
    case GAL_SPECLINES_H_16:          return GAL_SPECLINES_ANGSTROM_H_16;
    case GAL_SPECLINES_H_15:          return GAL_SPECLINES_ANGSTROM_H_15;
    case GAL_SPECLINES_H_14:          return GAL_SPECLINES_ANGSTROM_H_14;
    case GAL_SPECLINES_O_II_3726:     return GAL_SPECLINES_ANGSTROM_O_II_3726;
    case GAL_SPECLINES_O_II_3729:     return GAL_SPECLINES_ANGSTROM_O_II_3729;
    case GAL_SPECLINES_H_13:          return GAL_SPECLINES_ANGSTROM_H_13;
    case GAL_SPECLINES_H_12:          return GAL_SPECLINES_ANGSTROM_H_12;
    case GAL_SPECLINES_Fe_VII_3759:   return GAL_SPECLINES_ANGSTROM_Fe_VII_3759;
    case GAL_SPECLINES_H_11:          return GAL_SPECLINES_ANGSTROM_H_11;
    case GAL_SPECLINES_H_10:          return GAL_SPECLINES_ANGSTROM_H_10;
    case GAL_SPECLINES_H_9:           return GAL_SPECLINES_ANGSTROM_H_9;
    case GAL_SPECLINES_Fe_V_3839:     return GAL_SPECLINES_ANGSTROM_Fe_V_3839;
    case GAL_SPECLINES_Ne_III_3869:   return GAL_SPECLINES_ANGSTROM_Ne_III_3869;
    case GAL_SPECLINES_He_I_3889:     return GAL_SPECLINES_ANGSTROM_He_I_3889;
    case GAL_SPECLINES_H_8:           return GAL_SPECLINES_ANGSTROM_H_8;
    case GAL_SPECLINES_Fe_V_3891:     return GAL_SPECLINES_ANGSTROM_Fe_V_3891;
    case GAL_SPECLINES_Fe_V_3911:     return GAL_SPECLINES_ANGSTROM_Fe_V_3911;
    case GAL_SPECLINES_Ne_III_3967:   return GAL_SPECLINES_ANGSTROM_Ne_III_3967;
    case GAL_SPECLINES_H_epsilon:     return GAL_SPECLINES_ANGSTROM_H_epsilon;
    case GAL_SPECLINES_He_I_4026:     return GAL_SPECLINES_ANGSTROM_He_I_4026;
    case GAL_SPECLINES_S_II_4069:     return GAL_SPECLINES_ANGSTROM_S_II_4069;
    case GAL_SPECLINES_Fe_V_4071:     return GAL_SPECLINES_ANGSTROM_Fe_V_4071;
    case GAL_SPECLINES_S_II_4076:     return GAL_SPECLINES_ANGSTROM_S_II_4076;
    case GAL_SPECLINES_H_delta:       return GAL_SPECLINES_ANGSTROM_H_delta;
    case GAL_SPECLINES_He_I_4144:     return GAL_SPECLINES_ANGSTROM_He_I_4144;
    case GAL_SPECLINES_Fe_II_4179:    return GAL_SPECLINES_ANGSTROM_Fe_II_4179;
    case GAL_SPECLINES_Fe_V_4181:     return GAL_SPECLINES_ANGSTROM_Fe_V_4181;
    case GAL_SPECLINES_Fe_II_4233:    return GAL_SPECLINES_ANGSTROM_Fe_II_4233;
    case GAL_SPECLINES_Fe_V_4227:     return GAL_SPECLINES_ANGSTROM_Fe_V_4227;
    case GAL_SPECLINES_Fe_II_4287:    return GAL_SPECLINES_ANGSTROM_Fe_II_4287;
    case GAL_SPECLINES_Fe_II_4304:    return GAL_SPECLINES_ANGSTROM_Fe_II_4304;
    case GAL_SPECLINES_O_II_4317:     return GAL_SPECLINES_ANGSTROM_O_II_4317;
    case GAL_SPECLINES_H_gamma:       return GAL_SPECLINES_ANGSTROM_H_gamma;
    case GAL_SPECLINES_O_III_4363:    return GAL_SPECLINES_ANGSTROM_O_III_4363;
    case GAL_SPECLINES_Ar_XIV:        return GAL_SPECLINES_ANGSTROM_Ar_XIV;
    case GAL_SPECLINES_O_II_4415:     return GAL_SPECLINES_ANGSTROM_O_II_4415;
    case GAL_SPECLINES_Fe_II_4417:    return GAL_SPECLINES_ANGSTROM_Fe_II_4417;
    case GAL_SPECLINES_Fe_II_4452:    return GAL_SPECLINES_ANGSTROM_Fe_II_4452;
    case GAL_SPECLINES_He_I_4471:     return GAL_SPECLINES_ANGSTROM_He_I_4471;
    case GAL_SPECLINES_Fe_II_4489:    return GAL_SPECLINES_ANGSTROM_Fe_II_4489;
    case GAL_SPECLINES_Fe_II_4491:    return GAL_SPECLINES_ANGSTROM_Fe_II_4491;
    case GAL_SPECLINES_N_III_4510:    return GAL_SPECLINES_ANGSTROM_N_III_4510;
    case GAL_SPECLINES_Fe_II_4523:    return GAL_SPECLINES_ANGSTROM_Fe_II_4523;
    case GAL_SPECLINES_Fe_II_4556:    return GAL_SPECLINES_ANGSTROM_Fe_II_4556;
    case GAL_SPECLINES_Fe_II_4583:    return GAL_SPECLINES_ANGSTROM_Fe_II_4583;
    case GAL_SPECLINES_Fe_II_4584:    return GAL_SPECLINES_ANGSTROM_Fe_II_4584;
    case GAL_SPECLINES_Fe_II_4630:    return GAL_SPECLINES_ANGSTROM_Fe_II_4630;
    case GAL_SPECLINES_N_III_4634:    return GAL_SPECLINES_ANGSTROM_N_III_4634;
    case GAL_SPECLINES_N_III_4641:    return GAL_SPECLINES_ANGSTROM_N_III_4641;
    case GAL_SPECLINES_N_III_4642:    return GAL_SPECLINES_ANGSTROM_N_III_4642;
    case GAL_SPECLINES_C_III_4647:    return GAL_SPECLINES_ANGSTROM_C_III_4647;
    case GAL_SPECLINES_C_III_4650:    return GAL_SPECLINES_ANGSTROM_C_III_4650;
    case GAL_SPECLINES_C_III_5651:    return GAL_SPECLINES_ANGSTROM_C_III_5651;
    case GAL_SPECLINES_Fe_III_4658:   return GAL_SPECLINES_ANGSTROM_Fe_III_4658;
    case GAL_SPECLINES_He_II_4686:    return GAL_SPECLINES_ANGSTROM_He_II_4686;
    case GAL_SPECLINES_Ar_IV_4711:    return GAL_SPECLINES_ANGSTROM_Ar_IV_4711;
    case GAL_SPECLINES_Ar_IV_4740:    return GAL_SPECLINES_ANGSTROM_Ar_IV_4740;
    case GAL_SPECLINES_H_beta:        return GAL_SPECLINES_ANGSTROM_H_beta;
    case GAL_SPECLINES_Fe_VII_4893:   return GAL_SPECLINES_ANGSTROM_Fe_VII_4893;
    case GAL_SPECLINES_Fe_IV_4903:    return GAL_SPECLINES_ANGSTROM_Fe_IV_4903;
    case GAL_SPECLINES_Fe_II_4924:    return GAL_SPECLINES_ANGSTROM_Fe_II_4924;
    case GAL_SPECLINES_O_III_4959:    return GAL_SPECLINES_ANGSTROM_O_III_4959;
    case GAL_SPECLINES_O_III_5007:    return GAL_SPECLINES_ANGSTROM_O_III_5007;
    case GAL_SPECLINES_Fe_II_5018:    return GAL_SPECLINES_ANGSTROM_Fe_II_5018;
    case GAL_SPECLINES_Fe_III_5085:   return GAL_SPECLINES_ANGSTROM_Fe_III_5085;
    case GAL_SPECLINES_Fe_VI_5146:    return GAL_SPECLINES_ANGSTROM_Fe_VI_5146;
    case GAL_SPECLINES_Fe_VII_5159:   return GAL_SPECLINES_ANGSTROM_Fe_VII_5159;
    case GAL_SPECLINES_Fe_II_5169:    return GAL_SPECLINES_ANGSTROM_Fe_II_5169;
    case GAL_SPECLINES_Fe_VI_5176:    return GAL_SPECLINES_ANGSTROM_Fe_VI_5176;
    case GAL_SPECLINES_Fe_II_5198:    return GAL_SPECLINES_ANGSTROM_Fe_II_5198;
    case GAL_SPECLINES_N_I_5200:      return GAL_SPECLINES_ANGSTROM_N_I_5200;
    case GAL_SPECLINES_Fe_II_5235:    return GAL_SPECLINES_ANGSTROM_Fe_II_5235;
    case GAL_SPECLINES_Fe_IV_5236:    return GAL_SPECLINES_ANGSTROM_Fe_IV_5236;
    case GAL_SPECLINES_Fe_III_5270:   return GAL_SPECLINES_ANGSTROM_Fe_III_5270;
    case GAL_SPECLINES_Fe_II_5276:    return GAL_SPECLINES_ANGSTROM_Fe_II_5276;
    case GAL_SPECLINES_Fe_VII_5276:   return GAL_SPECLINES_ANGSTROM_Fe_VII_5276;
    case GAL_SPECLINES_Fe_XIV:        return GAL_SPECLINES_ANGSTROM_Fe_XIV;
    case GAL_SPECLINES_Ca_V:          return GAL_SPECLINES_ANGSTROM_Ca_V;
    case GAL_SPECLINES_Fe_II_5316_62: return GAL_SPECLINES_ANGSTROM_Fe_II_5316_62;
    case GAL_SPECLINES_Fe_II_5316_78: return GAL_SPECLINES_ANGSTROM_Fe_II_5316_78;
    case GAL_SPECLINES_Fe_VI_5335:    return GAL_SPECLINES_ANGSTROM_Fe_VI_5335;
    case GAL_SPECLINES_Fe_VI_5424:    return GAL_SPECLINES_ANGSTROM_Fe_VI_5424;
    case GAL_SPECLINES_Cl_III_5518:   return GAL_SPECLINES_ANGSTROM_Cl_III_5518;
    case GAL_SPECLINES_Cl_III_5538:   return GAL_SPECLINES_ANGSTROM_Cl_III_5538;
    case GAL_SPECLINES_Fe_VI_5638:    return GAL_SPECLINES_ANGSTROM_Fe_VI_5638;
    case GAL_SPECLINES_Fe_VI_5677:    return GAL_SPECLINES_ANGSTROM_Fe_VI_5677;
    case GAL_SPECLINES_C_III_5698:    return GAL_SPECLINES_ANGSTROM_C_III_5698;
    case GAL_SPECLINES_Fe_VII_5721:   return GAL_SPECLINES_ANGSTROM_Fe_VII_5721;
    case GAL_SPECLINES_N_II_5755:     return GAL_SPECLINES_ANGSTROM_N_II_5755;
    case GAL_SPECLINES_C_IV_5801:     return GAL_SPECLINES_ANGSTROM_C_IV_5801;
    case GAL_SPECLINES_C_IV_5812:     return GAL_SPECLINES_ANGSTROM_C_IV_5812;
    case GAL_SPECLINES_He_I_5876:     return GAL_SPECLINES_ANGSTROM_He_I_5876;
    case GAL_SPECLINES_O_I_6046:      return GAL_SPECLINES_ANGSTROM_O_I_6046;
    case GAL_SPECLINES_Fe_VII_6087:   return GAL_SPECLINES_ANGSTROM_Fe_VII_6087;
    case GAL_SPECLINES_O_I_6300:      return GAL_SPECLINES_ANGSTROM_O_I_6300;
    case GAL_SPECLINES_S_III_6312:    return GAL_SPECLINES_ANGSTROM_S_III_6312;
    case GAL_SPECLINES_Si_II_6347:    return GAL_SPECLINES_ANGSTROM_Si_II_6347;
    case GAL_SPECLINES_O_I_6364:      return GAL_SPECLINES_ANGSTROM_O_I_6364;
    case GAL_SPECLINES_Fe_II_6369:    return GAL_SPECLINES_ANGSTROM_Fe_II_6369;
    case GAL_SPECLINES_Fe_X:          return GAL_SPECLINES_ANGSTROM_Fe_X;
    case GAL_SPECLINES_Fe_II_6516:    return GAL_SPECLINES_ANGSTROM_Fe_II_6516;
    case GAL_SPECLINES_N_II_6548:     return GAL_SPECLINES_ANGSTROM_N_II_6548;
    case GAL_SPECLINES_H_alpha:       return GAL_SPECLINES_ANGSTROM_H_alpha;
    case GAL_SPECLINES_N_II_6583:     return GAL_SPECLINES_ANGSTROM_N_II_6583;
    case GAL_SPECLINES_S_II_6716:     return GAL_SPECLINES_ANGSTROM_S_II_6716;
    case GAL_SPECLINES_S_II_6731:     return GAL_SPECLINES_ANGSTROM_S_II_6731;
    case GAL_SPECLINES_O_I_7002:      return GAL_SPECLINES_ANGSTROM_O_I_7002;
    case GAL_SPECLINES_Ar_V:          return GAL_SPECLINES_ANGSTROM_Ar_V;
    case GAL_SPECLINES_He_I_7065:     return GAL_SPECLINES_ANGSTROM_He_I_7065;
    case GAL_SPECLINES_Ar_III_7136:   return GAL_SPECLINES_ANGSTROM_Ar_III_7136;
    case GAL_SPECLINES_Fe_II_7155:    return GAL_SPECLINES_ANGSTROM_Fe_II_7155;
    case GAL_SPECLINES_Ar_IV_7171:    return GAL_SPECLINES_ANGSTROM_Ar_IV_7171;
    case GAL_SPECLINES_Fe_II_7172:    return GAL_SPECLINES_ANGSTROM_Fe_II_7172;
    case GAL_SPECLINES_C_II_7236:     return GAL_SPECLINES_ANGSTROM_C_II_7236;
    case GAL_SPECLINES_Ar_IV_7237:    return GAL_SPECLINES_ANGSTROM_Ar_IV_7237;
    case GAL_SPECLINES_O_I_7254:      return GAL_SPECLINES_ANGSTROM_O_I_7254;
    case GAL_SPECLINES_Ar_IV_7263:    return GAL_SPECLINES_ANGSTROM_Ar_IV_7263;
    case GAL_SPECLINES_He_I_7281:     return GAL_SPECLINES_ANGSTROM_He_I_7281;
    case GAL_SPECLINES_O_II_7320:     return GAL_SPECLINES_ANGSTROM_O_II_7320;
    case GAL_SPECLINES_O_II_7331:     return GAL_SPECLINES_ANGSTROM_O_II_7331;
    case GAL_SPECLINES_Ni_II_7378:    return GAL_SPECLINES_ANGSTROM_Ni_II_7378;
    case GAL_SPECLINES_Ni_II_7411:    return GAL_SPECLINES_ANGSTROM_Ni_II_7411;
    case GAL_SPECLINES_Fe_II_7453:    return GAL_SPECLINES_ANGSTROM_Fe_II_7453;
    case GAL_SPECLINES_N_I_7468:      return GAL_SPECLINES_ANGSTROM_N_I_7468;
    case GAL_SPECLINES_S_XII:         return GAL_SPECLINES_ANGSTROM_S_XII;
    case GAL_SPECLINES_Ar_III_7751:   return GAL_SPECLINES_ANGSTROM_Ar_III_7751;
    case GAL_SPECLINES_He_I_7816:     return GAL_SPECLINES_ANGSTROM_He_I_7816;
    case GAL_SPECLINES_Ar_I_7868:     return GAL_SPECLINES_ANGSTROM_Ar_I_7868;
    case GAL_SPECLINES_Ni_III:        return GAL_SPECLINES_ANGSTROM_Ni_III;
    case GAL_SPECLINES_Fe_XI_7892:    return GAL_SPECLINES_ANGSTROM_Fe_XI_7892;
    case GAL_SPECLINES_He_II_8237:    return GAL_SPECLINES_ANGSTROM_He_II_8237;
    case GAL_SPECLINES_Pa_20:         return GAL_SPECLINES_ANGSTROM_Pa_20;
    case GAL_SPECLINES_Pa_19:         return GAL_SPECLINES_ANGSTROM_Pa_19;
    case GAL_SPECLINES_Pa_18:         return GAL_SPECLINES_ANGSTROM_Pa_18;
    case GAL_SPECLINES_O_I_8446:      return GAL_SPECLINES_ANGSTROM_O_I_8446;
    case GAL_SPECLINES_Pa_17:         return GAL_SPECLINES_ANGSTROM_Pa_17;
    case GAL_SPECLINES_Ca_II_8498:    return GAL_SPECLINES_ANGSTROM_Ca_II_8498;
    case GAL_SPECLINES_Pa_16:         return GAL_SPECLINES_ANGSTROM_Pa_16;
    case GAL_SPECLINES_Ca_II_8542:    return GAL_SPECLINES_ANGSTROM_Ca_II_8542;
    case GAL_SPECLINES_Pa_15:         return GAL_SPECLINES_ANGSTROM_Pa_15;
    case GAL_SPECLINES_Cl_II:         return GAL_SPECLINES_ANGSTROM_Cl_II;
    case GAL_SPECLINES_Pa_14:         return GAL_SPECLINES_ANGSTROM_Pa_14;
    case GAL_SPECLINES_Fe_II_8617:    return GAL_SPECLINES_ANGSTROM_Fe_II_8617;
    case GAL_SPECLINES_Ca_II_8662:    return GAL_SPECLINES_ANGSTROM_Ca_II_8662;
    case GAL_SPECLINES_Pa_13:         return GAL_SPECLINES_ANGSTROM_Pa_13;
    case GAL_SPECLINES_N_I_8680:      return GAL_SPECLINES_ANGSTROM_N_I_8680;
    case GAL_SPECLINES_N_I_8703:      return GAL_SPECLINES_ANGSTROM_N_I_8703;
    case GAL_SPECLINES_N_I_8712:      return GAL_SPECLINES_ANGSTROM_N_I_8712;
    case GAL_SPECLINES_Pa_12:         return GAL_SPECLINES_ANGSTROM_Pa_12;
    case GAL_SPECLINES_Pa_11:         return GAL_SPECLINES_ANGSTROM_Pa_11;
    case GAL_SPECLINES_Fe_II_8892:    return GAL_SPECLINES_ANGSTROM_Fe_II_8892;
    case GAL_SPECLINES_Pa_10:         return GAL_SPECLINES_ANGSTROM_Pa_10;
    case GAL_SPECLINES_S_III_9069:    return GAL_SPECLINES_ANGSTROM_S_III_9069;
    case GAL_SPECLINES_Pa_9:          return GAL_SPECLINES_ANGSTROM_Pa_9;
    case GAL_SPECLINES_S_III_9531:    return GAL_SPECLINES_ANGSTROM_S_III_9531;
    case GAL_SPECLINES_Pa_epsilon:    return GAL_SPECLINES_ANGSTROM_Pa_epsilon;
    case GAL_SPECLINES_C_I_9824:      return GAL_SPECLINES_ANGSTROM_C_I_9824;
    case GAL_SPECLINES_C_I_9850:      return GAL_SPECLINES_ANGSTROM_C_I_9850;
    case GAL_SPECLINES_S_VIII:        return GAL_SPECLINES_ANGSTROM_S_VIII;
    case GAL_SPECLINES_He_I_10028:    return GAL_SPECLINES_ANGSTROM_He_I_10028;
    case GAL_SPECLINES_He_I_10031:    return GAL_SPECLINES_ANGSTROM_He_I_10031;
    case GAL_SPECLINES_Pa_delta:      return GAL_SPECLINES_ANGSTROM_Pa_delta;
    case GAL_SPECLINES_S_II_10287:    return GAL_SPECLINES_ANGSTROM_S_II_10287;
    case GAL_SPECLINES_S_II_10320:    return GAL_SPECLINES_ANGSTROM_S_II_10320;
    case GAL_SPECLINES_S_II_10336:    return GAL_SPECLINES_ANGSTROM_S_II_10336;
    case GAL_SPECLINES_Fe_XIII:       return GAL_SPECLINES_ANGSTROM_Fe_XIII;
    case GAL_SPECLINES_He_I_10830:    return GAL_SPECLINES_ANGSTROM_He_I_10830;
    case GAL_SPECLINES_Pa_gamma:      return GAL_SPECLINES_ANGSTROM_Pa_gamma;

    /* Limits. */
    case GAL_SPECLINES_LIMIT_LYMAN:    return GAL_SPECLINES_ANGSTROM_LIMIT_LYMAN;
    case GAL_SPECLINES_LIMIT_BALMER:   return GAL_SPECLINES_ANGSTROM_LIMIT_BALMER;
    case GAL_SPECLINES_LIMIT_PASCHEN:  return GAL_SPECLINES_ANGSTROM_LIMIT_PASCHEN;

    default:
      error(EXIT_FAILURE, 0, "%s: '%d' not recognized line identifier",
            __func__, linecode);
    }
  return NAN;
}




















/*********************************************************************/
/*************             Redshifted lines            ***************/
/*********************************************************************/
double
gal_speclines_line_redshift(double obsline, double restline)
{
  return (obsline/restline)-1;
}





double
gal_speclines_line_redshift_code(double obsline, int linecode)
{
  double restline=gal_speclines_line_angstrom(linecode);
  return (obsline/restline)-1;
}
