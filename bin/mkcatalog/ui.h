/*********************************************************************
MakeCatalog - Make a catalog from an input and labeled image.
MakeCatalog is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2016-2024 Free Software Foundation, Inc.

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
#ifndef UI_H
#define UI_H

/* For common options groups. */
#include <gnuastro-internal/options.h>





/* Option groups particular to this program. */
enum program_args_groups
{
  UI_GROUP_UPPERLIMIT = GAL_OPTIONS_GROUP_AFTER_COMMON,
  UI_GROUP_OTHERSETTINGS,
  UI_GROUP_COLUMNS_IDS,
  UI_GROUP_COLUMNS_POSITION_PIXEL,
  UI_GROUP_COLUMNS_POSITION_WCS,
  UI_GROUP_COLUMNS_BRIGHTNESS,
  UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
  UI_GROUP_COLUMNS_MORPHOLOGY,
  UI_GROUP_COLUMNS_ELLIPTICAL,
  UI_GROUP_COLUMNS_VECTOR,
};





/* Available letters for short options:

   a b c f g k n u w x y z
   A B E G H J L O Q R W X Y
*/
enum option_keys_enum
{
  /* With short-option version. */
  UI_KEY_CLUMPSCAT       = 'C',         /* General settings. */
  UI_KEY_VALUESFILE      = 'v',
  UI_KEY_CLUMPSFILE      = 'l',
  UI_KEY_INSKY           = 's',
  UI_KEY_INSTD           = 't',
  UI_KEY_ENVSEED         = 'e',

  UI_KEY_IDS             = 'i',         /* Catalog columns. */
  UI_KEY_HOSTOBJID       = 'j',
  UI_KEY_X               = 'x',
  UI_KEY_Y               = 'y',
  UI_KEY_Z               = 'z',
  UI_KEY_RA              = 'r',
  UI_KEY_DEC             = 'd',
  UI_KEY_MAGNITUDE       = 'm',

  /* Only with long version (start with a value 1000, the rest will be set
     automatically). */
  UI_KEY_VALUESHDU       = 1000,        /* General settings. */
  UI_KEY_CLUMPSHDU,
  UI_KEY_SKYHDU,
  UI_KEY_STDHDU,
  UI_KEY_WITHCLUMPS,
  UI_KEY_FORCEREADSTD,
  UI_KEY_ZEROPOINT,
  UI_KEY_SIGMACLIP,
  UI_KEY_VARIANCE,
  UI_KEY_NOVALINERRORS,
  UI_KEY_SUBTRACTSKY,
  UI_KEY_SFMAGNSIGMA,
  UI_KEY_SFMAGAREA,
  UI_KEY_SPECTRUM,
  UI_KEY_INBETWEENINTS,
  UI_KEY_UPMASKFILE,
  UI_KEY_UPMASKHDU,
  UI_KEY_UPNUM,
  UI_KEY_UPRANGE,
  UI_KEY_UPSIGMACLIP,
  UI_KEY_UPNSIGMA,
  UI_KEY_CHECKUPLIM,
  UI_KEY_NOCLUMPSORT,
  UI_KEY_FRACMAX,
  UI_KEY_SPATIALRESOLUTION,

  UI_KEY_OBJID,                         /* Catalog columns. */
  UI_KEY_IDINHOSTOBJ,
  UI_KEY_AREA,
  UI_KEY_AREAARCSEC2,
  UI_KEY_SB,
  UI_KEY_SBERROR,
  UI_KEY_AREAXY,
  UI_KEY_NUMCLUMPS,
  UI_KEY_CLUMPSAREA,
  UI_KEY_WEIGHTAREA,
  UI_KEY_GEOAREA,
  UI_KEY_GEOAREAXY,
  UI_KEY_GEOX,
  UI_KEY_GEOY,
  UI_KEY_GEOZ,
  UI_KEY_MINVALX,
  UI_KEY_MAXVALX,
  UI_KEY_MINVALY,
  UI_KEY_MAXVALY,
  UI_KEY_MINVALZ,
  UI_KEY_MAXVALZ,
  UI_KEY_MINVALNUM,
  UI_KEY_MAXVALNUM,
  UI_KEY_CLUMPSX,
  UI_KEY_CLUMPSY,
  UI_KEY_CLUMPSZ,
  UI_KEY_CLUMPSGEOX,
  UI_KEY_CLUMPSGEOY,
  UI_KEY_CLUMPSGEOZ,
  UI_KEY_MINX,
  UI_KEY_MAXX,
  UI_KEY_MINY,
  UI_KEY_MAXY,
  UI_KEY_MINZ,
  UI_KEY_MAXZ,
  UI_KEY_W1,
  UI_KEY_W2,
  UI_KEY_W3,
  UI_KEY_GEOW1,
  UI_KEY_GEOW2,
  UI_KEY_GEOW3,
  UI_KEY_CLUMPSW1,
  UI_KEY_CLUMPSW2,
  UI_KEY_CLUMPSW3,
  UI_KEY_CLUMPSGEOW1,
  UI_KEY_CLUMPSGEOW2,
  UI_KEY_CLUMPSGEOW3,
  UI_KEY_SUM,
  UI_KEY_SUMERROR,
  UI_KEY_CLUMPSSUM,
  UI_KEY_SUMNORIVER,
  UI_KEY_STD,
  UI_KEY_MEAN,
  UI_KEY_MEANERROR,
  UI_KEY_MEDIAN,
  UI_KEY_MAXIMUM,
  UI_KEY_MAGNITUDEERROR,
  UI_KEY_SN,
  UI_KEY_CLUMPSMAGNITUDE,
  UI_KEY_UPPERLIMIT,
  UI_KEY_UPPERLIMITSB,
  UI_KEY_UPPERLIMITMAG,
  UI_KEY_UPPERLIMITONESIGMA,
  UI_KEY_UPPERLIMITSIGMA,
  UI_KEY_UPPERLIMITQUANTILE,
  UI_KEY_UPPERLIMITSKEW,
  UI_KEY_RIVERMEAN,
  UI_KEY_RIVERNUM,
  UI_KEY_RIVERMIN,
  UI_KEY_RIVERMAX,
  UI_KEY_SKY,
  UI_KEY_SKYSTD,
  UI_KEY_SIGCLIPNUMBER,
  UI_KEY_SIGCLIPMEDIAN,
  UI_KEY_SIGCLIPMEAN,
  UI_KEY_SIGCLIPSTD,
  UI_KEY_SIGCLIPMEANSB,
  UI_KEY_SIGCLIPMEANSBDELTA,
  UI_KEY_SIGCLIPSTDSB,
  UI_KEY_SEMIMAJOR,
  UI_KEY_SEMIMINOR,
  UI_KEY_POSITIONANGLE,
  UI_KEY_AXISRATIO,
  UI_KEY_GEOSEMIMAJOR,
  UI_KEY_GEOSEMIMINOR,
  UI_KEY_GEOAXISRATIO,
  UI_KEY_GEOPOSITIONANGLE,
  UI_KEY_FWHM,
  UI_KEY_HALFMAXAREA,
  UI_KEY_HALFMAXRADIUS,
  UI_KEY_HALFMAXSUM,
  UI_KEY_HALFMAXSB,
  UI_KEY_HALFSUMAREA,
  UI_KEY_HALFSUMSB,
  UI_KEY_HALFSUMRADIUS,
  UI_KEY_FRACMAX1SUM,
  UI_KEY_FRACMAX2SUM,
  UI_KEY_FRACMAX1AREA,
  UI_KEY_FRACMAX2AREA,
  UI_KEY_FRACMAX1RADIUS,
  UI_KEY_FRACMAX2RADIUS,

  /* Vector columns */
  UI_KEY_SUMINSLICE,
  UI_KEY_SUMERRINSLICE,
  UI_KEY_AREAINSLICE,
  UI_KEY_SUMPROJINSLICE,
  UI_KEY_AREAPROJINSLICE,
  UI_KEY_SUMPROJERRINSLICE,
  UI_KEY_AREAOTHERINSLICE,
  UI_KEY_SUMOTHERINSLICE,
  UI_KEY_SUMOTHERERRINSLICE,
};





void
ui_read_check_inputs_setup(int argc, char *argv[], struct mkcatalogparams *p);

void
ui_free_report(struct mkcatalogparams *p, struct timeval *t1);

#endif
