/*********************************************************************
Units -- Convert data from one unit to other.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Kartik Ohri <kartikohri13@gmail.com>
Contributing author(s):
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
     Pedram Ashofteh-Ardakani <pedramardakani@pm.me>
Copyright (C) 2020-2024 Free Software Foundation, Inc.

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
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gnuastro/type.h>
#include <gnuastro/pointer.h>



/**********************************************************************/
/****************      Functions to parse strings     *****************/
/**********************************************************************/
/* Parse the input string consisting of numbers separated by given
   delimiter into an array. */
int
gal_units_extract_decimal(char *convert, const char *delimiter,
                          double *args, size_t n)
{
  size_t i = 0;
  char *copy, *token, *end;

  /* Create a copy of the string to be parsed and parse it. This is because
     it will be modified during the parsing. */
  copy=strdup(convert);
  do
    {
      /* Check if the required number of arguments are passed. */
      if(i==n+1)
        {
          free(copy);
          error(0, 0, "%s: input '%s' exceeds maximum number of arguments "
                "(%zu)", __func__, convert, n);
          return 0;
        }

      /* Extract the substring till the next delimiter. */
      token=strtok(i==0?copy:NULL, delimiter);
      if(token)
        {
          /* Parse extracted string as a number, and check if it worked. */
          args[i++] = strtod (token, &end);
          if (*end && *end != *delimiter)
            {
              /* In case a warning is necessary
              error(0, 0, "%s: unable to parse element %zu in '%s'\n",
                    __func__, i, convert);
              */
              free(copy);
              return 0;
            }
        }
    }
  while(token && *token);
  free (copy);

  /* Check if the number of elements parsed. */
  if (i != n)
    {
      /* In case a warning is necessary
      error(0, 0, "%s: input '%s' must contain %lu numbers, but has "
            "%lu numbers\n", __func__, convert, n, i);
      */
      return 0;
    }

  /* Numbers are written, return successfully. */
  return 1;
}


















/**********************************************************************/
/****************      Convert string to decimal      *****************/
/**********************************************************************/

/* Parse the right ascension input as a string in form of hh:mm:ss to a
 * single decimal value calculated by (hh + mm / 60 + ss / 3600 ) * 15. */
double
gal_units_ra_to_degree(char *convert)
{
  double val[3];
  double decimal=0.0;

  /* Check whether the string is successfully parsed. */
  if(gal_units_extract_decimal(convert, ":hms", val, 3))
    {
      /* Check whether the first value is in within limits, and add it. We
         are using 'signbit(val[0])' instead of 'val[0]<0.0f' because
         'val[0]<0.0f' can't distinguish negative zero (-0.0) from an
         unsigned zero (in other words, '-0.0' will be interpretted to be
         positive). For the declinations it is possible (see the comments
         in 'gal_units_dec_to_degree'), so a user may mistakenly give that
         format in Right Ascension. */
      if(signbit(val[0]) || val[0]>24.0) return NAN;
      decimal += val[0];

      /* Check whether value of minutes is within limits, and add it. */
      if(signbit(val[1]) || val[1]>60.0) return NAN;
      decimal += val[1] / 60;

      /* Check whether value of seconds is in within limits, and add it. */
      if(signbit(val[2]) || val[2]>60.0) return NAN;
      decimal += val[2] / 3600;

      /* Convert value to degrees and return. */
      decimal *= 15.0;
      return decimal;
    }
  else return NAN;

  /* Control shouldn't reach this point. If it does, its a bug! */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return NAN;
}





/* Parse the declination input as a string in form of dd:mm:ss to a decimal
 * calculated by (dd + mm / 60 + ss / 3600 ). */
double
gal_units_dec_to_degree(char *convert)
{
  int sign;
  double val[3], decimal=0.0;

  /* Parse the values in the input string. */
  if(gal_units_extract_decimal(convert, ":dms", val, 3))
    {
      /* Check whether the first value is in within limits. */
      if(val[0]<-90.0 || val[0]>90.0) return NAN;

      /* If declination is negative, the first value in the array will be
         negative and all other values will be positive. In that case, we
         set sign equal to -1. Therefore, we multiply the first value by
         sign to make it positive. The final answer is again multiplied by
         sign to make its sign same as original.

        We are using 'signbit(val[0])' instead of 'val[0]<0.0f' because
        'val[0]<0.0f' can't distinguish negative zero (-0.0) from an
        unsigned zero (in other words, '-0.0' will be interpretted to be
        positive). In the case of declination, this can happen just below
        the equator (where the declination is less than one degree), for
        example '-00d:12:34'. */
      sign = signbit(val[0]) ? -1 : 1;
      decimal += val[0] * sign;

      /* Check whether value of arc-minutes is in within limits. */
      if(signbit(val[1]) || val[1]>60.0) return NAN;
      decimal += val[1] / 60;

      /* Check whether value of arc-seconds is in within limits. */
      if (signbit(val[2]) || val[2] > 60.0) return NAN;
      decimal += val[2] / 3600;

      /* Make the sign of the decimal value same as input and return. */
      decimal *= sign;
      return decimal;
    }
  else return NAN;

  /* Control shouldn't reach this point. If it does, its a bug! */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return NAN;
}




















/**********************************************************************/
/****************      Convert decimal to string      *****************/
/**********************************************************************/

/* Max-length of output string. */
#define UNITS_RADECSTR_MAXLENGTH 50

/* Parse the right ascension input as a decimal to a string in form of
   hh:mm:ss.ss . */
char *
gal_units_degree_to_ra(double decimal, int usecolon)
{
  size_t nchars;
  int hours=0, minutes=0;
  float seconds=0.0; /* For sub-second accuracy. */

  /* Allocate a long string which is large enough for string of format
     hh:mm:ss.ss and sign. */
  char *ra=gal_pointer_allocate(GAL_TYPE_UINT8, UNITS_RADECSTR_MAXLENGTH,
                                0, __func__, "ra");

  /* Check if decimal value is within bounds otherwise return error. */
  if (decimal<0 || decimal>360)
    {
      error (0, 0, "%s: value of decimal should be between be 0 and 360, "
             "but is %g\n", __func__, decimal);
      return NULL;
    }

  /* Divide decimal value by 15 and extract integer part of decimal value
     to obtain hours. */
  decimal /= 15.0;
  hours = (int)decimal;

  /* Subtract hours from decimal and multiply remaining value by 60 to
     obtain minutes. */
  minutes = (int)((decimal - hours) * 60);

  /* Subtract hours and minutes from decimal and multiply remaining value
     by 3600 to obtain seconds. */
  seconds = (decimal - hours - minutes / 60.0) * 3600;

  /* Format the extracted hours, minutes and seconds as a string with
     leading zeros if required, in hh:mm:ss format. */
  nchars = snprintf(ra, UNITS_RADECSTR_MAXLENGTH-1,
                    usecolon ? "%02d:%02d:%g" : "%02dh%02dm%g",
                    hours, minutes, seconds);
  if(nchars>UNITS_RADECSTR_MAXLENGTH)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to address "
          "the problem. The output string has an unreasonable length of "
          "%zu characters", __func__, PACKAGE_BUGREPORT, nchars);

  /* Return the final string. */
  return ra;
}





/* Parse the declination input as a decimal to a string in form of dd:mm:ss . */
char *
gal_units_degree_to_dec(double decimal, int usecolon)
{
  size_t nchars;
  float arc_seconds=0.0;
  int sign, degrees=0, arc_minutes=0;

  /* Allocate string of fixed length which is large enough for string of
   * format hh:mm:ss.ss and sign. */
  char *dec=gal_pointer_allocate(GAL_TYPE_UINT8, UNITS_RADECSTR_MAXLENGTH,
                                 0, __func__, "ra");

  /* Check if decimal value is within bounds otherwise return error. */
  if(decimal<-90 || decimal>90)
    {
      error (0, 0, "%s: value of decimal should be between be -90 and 90, "
             "but is %g\n", __func__, decimal);
      return NULL;
    }

  /* If declination is negative, we set 'sign' equal to -1. We multiply the
     decimal by to make sure it is positive. We then extract degrees,
     arc-minutes and arc-seconds from the decimal. Finally, we add a minus
     sign in beginning of string if input was negative. */
  sign = decimal<0.0 ? -1 : 1;
  decimal *= sign;

  /* Extract integer part of decimal value to obtain degrees. */
  degrees=(int)decimal;

  /* Subtract degrees from decimal and multiply remaining value by 60 to
     obtain arc-minutes. */
  arc_minutes=(int)( (decimal - degrees) * 60 );

  /* Subtract degrees and arc-minutes from decimal and multiply remaining
     value by 3600 to obtain arc-seconds. */
  arc_seconds = (decimal - degrees - arc_minutes / 60.0) * 3600;

  /* Format the extracted degrees, arc-minutes and arc-seconds as a string
     with leading zeros if required, in hh:mm:ss format with correct
     sign. */
  nchars = snprintf(dec, UNITS_RADECSTR_MAXLENGTH-1,
                    usecolon ? "%s%02d:%02d:%g" : "%s%02dd%02dm%g",
                    sign<0?"-":"+", degrees, arc_minutes, arc_seconds);
  if(nchars>UNITS_RADECSTR_MAXLENGTH)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to address "
          "the problem. The output string has an unreasonable length of "
          "%zu characters", __func__, PACKAGE_BUGREPORT, nchars);

  /* Return the final string. */
  return dec;
}




















/**********************************************************************/
/****************          Flux conversions           *****************/
/**********************************************************************/

/* Convert counts to magnitude using the given zeropoint. */
double
gal_units_counts_to_mag(double counts, double zeropoint)
{
  return ( counts > 0.0f
           ? ( -2.5f * log10(counts) + zeropoint )
           : NAN );
}





/* Convert magnitude to counts using the given zeropoint. */
double
gal_units_mag_to_counts(double mag, double zeropoint)
{
  return pow(10, (mag - zeropoint)/(-2.5f));
}




double
gal_units_mag_to_sb(double mag, double area_arcsec2)
{
  return mag+2.5*log10(area_arcsec2);
}





double
gal_units_sb_to_mag(double sb, double area_arcsec2)
{
  return sb-2.5*log10(area_arcsec2);
}





double
gal_units_counts_to_sb(double counts, double zeropoint,
                       double area_arcsec2)
{
  return gal_units_mag_to_sb(
                   gal_units_counts_to_mag(counts, zeropoint),
                   area_arcsec2);
}





double
gal_units_sb_to_counts(double sb, double zeropoint,
                       double area_arcsec2)
{
  return gal_units_mag_to_counts(
                   gal_units_sb_to_mag(sb, area_arcsec2),
                   zeropoint);
}




/* Convert Pixel values to Janskys with an AB-magnitude based
   zero-point. See the "Brightness, Flux, Magnitude and Surface
   brightness". */
double
gal_units_counts_to_jy(double counts, double zeropoint_ab)
{
  return counts * 3631 * pow(10, -1 * zeropoint_ab / 2.5);
}





/* Convert Janskys to pixel values with an AB-magnitude based
   zero-point. See the "Brightness, Flux, Magnitude and Surface
   brightness". */
double
gal_units_jy_to_counts(double jy, double zeropoint_ab)
{
  return jy / 3631 / pow(10, -1 * zeropoint_ab / 2.5);
}





/* Convert counts to a custom zero point. The job of this function is
   equivalent to the double-call bellow. We just don't want to repeat some
   extra multiplication/divisions.

     gal_units_jy_to_counts(gal_units_counts_to_jy(counts, zeropoint_in),
                            custom_out)
*/
double
gal_units_zeropoint_change(double counts, double zeropoint_in,
                           double zeropoint_out)
{
  return ( counts
           * pow(10, -1 * zeropoint_in  / 2.5)
           / pow(10, -1 * zeropoint_out / 2.5) );
}





double
gal_units_counts_to_nanomaggy(double counts, double zeropoint_ab)
{
  return gal_units_zeropoint_change(counts, zeropoint_ab, 22.5);
}





double
gal_units_nanomaggy_to_counts(double counts, double zeropoint_ab)
{
  return gal_units_zeropoint_change(counts, 22.5, zeropoint_ab);
}





double
gal_units_jy_to_mag(double jy)
{
  double zp=0;
  return gal_units_counts_to_mag(gal_units_jy_to_counts(jy, zp),zp);
}





/* Converting Janskys ($f(\nu)$ or flux in units of frequency) to
   $f(\lambda)$ (or wavelength flux density). In the equations below, we'll
   write $\nu$ as 'n' and '\lambda' as 'l'. The speed of light is written
   as 'c'.

   Basics:

   1. c = 2.99792458e+08 m/s = 2.99792458e+18 A*Hz

   2. One Jansky is defined as 10^{-23} erg/s/cm^2/Hz; see
      https://en.wikipedia.org/wiki/Jansky

   3. The speed of light connects the wavelength and frequency of photons:
      c=l*n. So n=c/l and taking the derivative: dn=(c/(l*l))*dl or
      dn/dl=c/(l*l). Inserting physical values and units:

        dn/dl = (2.99792458e+18 A*Hz)/(l*l A^2) = 2.99792458e+18/(l^2) Hz/A.

   4. To convert a function of A into a function of B, where A and B are
      also related to each other, we have the following equation: f(A) =
      dB/dA * f(B).

   5. Using 2 (definition of Jansky as f(n)) and 3 in 4, we get:

      f(l) = 2.99792458e+18/(l^2) Hz/A * 10^{-23} erg/s/cm^2/Hz
           = 2.99792458e-5/(l^2) erg/s/cm^2/A
*/
double
gal_units_jy_to_wavelength_flux_density(double jy, double angstrom)
{
  return jy * 2.99792458e-05 / (angstrom*angstrom);
}





double
gal_units_mag_to_jy(double mag)
{
  double zp=0;
  return gal_units_counts_to_jy(gal_units_mag_to_counts(mag, zp),zp);
}




















/**********************************************************************/
/****************         Distance conversions        *****************/
/**********************************************************************/
/* Convert Astronomical Units (AU) to Parsecs (PC). From the definition of
   Parsecs, 648000/pi AU = 1 PC. The mathematical constant 'PI' is imported
   from the GSL as M_PI. So: */
double
gal_units_au_to_pc(double au)
{
  return au / 648000.0f * M_PI;
}





/* Convert Parsecs (PC) to Astronomical units (AU), see comment of
   'gal_units_au_to_pc'. */
double
gal_units_pc_to_au(double au)
{
  return au * 648000.0f / M_PI;
}





/* Convert Light-years to Parsecs, according to
   https://en.wikipedia.org/wiki/Light-year#Definitions:

   1 light-year = 9460730472580800 metres (exactly)
                ~ 9.461 petametres
                ~ 9.461 trillion kilometres (5.879 trillion miles)
                ~ 63241.077 astronomical units
                ~ 0.306601 parsecs  */
double
gal_units_ly_to_pc(double ly)
{
  return ly * 0.306601f;
}





/* Convert Parsecs to Light-years (see comment of gal_units_ly_to_pc). */
double
gal_units_pc_to_ly(double pc)
{
  return pc / 0.306601f;
}





/* Convert Astronomical Units to Light-years (see comment of
   gal_units_ly_to_pc). */
double
gal_units_au_to_ly(double au)
{
  return au / 63241.077f;
}





/* Convert Light-years to Astronomical Units (see comment of
   gal_units_ly_to_pc). */
double
gal_units_ly_to_au(double ly)
{
  return ly * 63241.077f;
}
