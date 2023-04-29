/*********************************************************************
MakeCatalog - Make a catalog from an input and labeled image.
MakeCatalog is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2016-2023 Free Software Foundation, Inc.

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
#ifndef ARGS_H
#define ARGS_H






/* Array of acceptable options. */
struct argp_option program_options[] =
  {
    /* Input options. */
    {
      "clumpsfile",
      UI_KEY_CLUMPSFILE,
      "FITS",
      0,
      "Dataset containing clump labels.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->clumpsfile,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "clumpshdu",
      UI_KEY_CLUMPSHDU,
      "STR",
      0,
      "Clump labels extension name or number.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->clumpshdu,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "valuesfile",
      UI_KEY_VALUESFILE,
      "FITS",
      0,
      "Values/brightness dataset.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->valuesfile,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "valueshdu",
      UI_KEY_VALUESHDU,
      "STR",
      0,
      "Name or number of extension containing values.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->valueshdu,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "insky",
      UI_KEY_INSKY,
      "FITS/FLT",
      0,
      "Input Sky value or file.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->skyfile,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "skyhdu",
      UI_KEY_SKYHDU,
      "STR",
      0,
      "Sky image extension name or number.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->skyhdu,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "subtractsky",
      UI_KEY_SUBTRACTSKY,
      0,
      0,
      "Subtract the Sky dataset from the values.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->subtractsky,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "instd",
      UI_KEY_INSTD,
      "STR/FLT",
      0,
      "Sky standard deviation value or dataset.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->stdfile,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "stdhdu",
      UI_KEY_STDHDU,
      "STR",
      0,
      "Sky STD extension name or number.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->stdhdu,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "variance",
      UI_KEY_VARIANCE,
      0,
      0,
      "STD input dataset is actually variance.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->variance,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "forcereadstd",
      UI_KEY_FORCEREADSTD,
      0,
      0,
      "Read STD even if no columns need it.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->forcereadstd,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "zeropoint",
      UI_KEY_ZEROPOINT,
      "FLT",
      0,
      "Zeropoint magnitude of input dataset.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->zeropoint,
      GAL_TYPE_FLOAT32,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "sigmaclip",
      UI_KEY_SIGMACLIP,
      "FLT,FLT",
      0,
      "Sigma-clip column multiple and tolerance.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->sigmaclip,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_read_sigma_clip
    },



    /* Output. */
    {
      "clumpscat",
      UI_KEY_CLUMPSCAT,
      0,
      0,
      "Make a clumps catalog also.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->clumpscat,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "noclumpsort",
      UI_KEY_NOCLUMPSORT,
      0,
      0,
      "Don't sort the clumps catalog by ID.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->noclumpsort,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "sfmagnsigma",
      UI_KEY_SFMAGNSIGMA,
      "FLT",
      0,
      "Surface brightness multiple of Sky STD.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->sfmagnsigma,
      GAL_TYPE_FLOAT32,
      GAL_OPTIONS_RANGE_GT_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "sfmagarea",
      UI_KEY_SFMAGAREA,
      "FLT",
      0,
      "Surface brightness area (in arcseconds^2).",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->sfmagarea,
      GAL_TYPE_FLOAT32,
      GAL_OPTIONS_RANGE_GT_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "inbetweenints",
      UI_KEY_INBETWEENINTS,
      0,
      0,
      "Keep rows (integer ids) with no labels.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->inbetweenints,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    /* Upper limit magnitude configurations. */
    {
      0, 0, 0, 0,
      "Upper limit magnitude settings:",
      UI_GROUP_UPPERLIMIT
    },
    {
      "upmaskfile",
      UI_KEY_UPMASKFILE,
      "FITS",
      0,
      "Mask image file name only for upper limit.",
      UI_GROUP_UPPERLIMIT,
      &p->upmaskfile,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "upmaskhdu",
      UI_KEY_UPMASKHDU,
      "STR",
      0,
      "Mask image HDU only for upper limit.",
      UI_GROUP_UPPERLIMIT,
      &p->upmaskhdu,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "upnum",
      UI_KEY_UPNUM,
      "INT",
      0,
      "Number of randomly positioned samples",
      UI_GROUP_UPPERLIMIT,
      &p->upnum,
      GAL_TYPE_SIZE_T,
      GAL_OPTIONS_RANGE_GT_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "uprange",
      UI_KEY_UPRANGE,
      "INT,INT",
      0,
      "Range of random positions (pix) around target.",
      UI_GROUP_UPPERLIMIT,
      &p->uprange,
      GAL_TYPE_SIZE_T,
      GAL_OPTIONS_RANGE_GT_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_sizes_reverse
    },
    {
      "envseed",
      UI_KEY_ENVSEED,
      0,
      0,
      "Use GSL_RNG_SEED environment variable for seed.",
      UI_GROUP_UPPERLIMIT,
      &p->envseed,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "upsigmaclip",
      UI_KEY_UPSIGMACLIP,
      "FLT,FLT",
      0,
      "Sigma multiple and, tolerance or number.",
      UI_GROUP_UPPERLIMIT,
      &p->upsigmaclip,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_read_sigma_clip
    },
    {
      "upnsigma",
      UI_KEY_UPNSIGMA,
      "FLT",
      0,
      "Multiple of sigma to define upperlimit.",
      UI_GROUP_UPPERLIMIT,
      &p->upnsigma,
      GAL_TYPE_FLOAT32,
      GAL_OPTIONS_RANGE_GT_0,
      GAL_OPTIONS_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "checkuplim",
      UI_KEY_CHECKUPLIM,
      "INT[,INT]",
      0,
      "Check random distribution for one label.",
      UI_GROUP_UPPERLIMIT,
      &p->checkuplim,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_GT_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_check_upperlimit
    },





    /* Other column configurations. */
    {
      0, 0, 0, 0,
      "Settings for other columns:",
      UI_GROUP_OTHERSETTINGS
    },
    {
      "frac-max",
      UI_KEY_FRACMAX,
      "FLT[,FLT]",
      0,
      "Fraction(s) in --frac-max* options.",
      UI_GROUP_OTHERSETTINGS,
      &p->fracmax,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_csv_float64
    },
    {
      "spatialresolution",
      UI_KEY_SPATIALRESOLUTION,
      "FLT",
      0,
      "Spatial resolution (for surf. brightness err).",
      UI_GROUP_OTHERSETTINGS,
      &p->spatialresolution,
      GAL_TYPE_FLOAT32,
      GAL_OPTIONS_RANGE_GE_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
    },





    /* ID related columns. */
    {
      0, 0, 0, 0,
      "Identifier columns",
      UI_GROUP_COLUMNS_IDS
    },
    {  /* 'ids' is not a unique column, it is a combination of several
          columns. */
      "ids",
      UI_KEY_IDS,
      0,
      0,
      "All IDs of objects and clumps.",
      UI_GROUP_COLUMNS_IDS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "obj-id",
      UI_KEY_OBJID,
      0,
      0,
      "Object label/ID.",
      UI_GROUP_COLUMNS_IDS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "host-obj-id",
      UI_KEY_HOSTOBJID,
      0,
      0,
      "ID of object hosting this clump.",
      UI_GROUP_COLUMNS_IDS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "id-in-host-obj",
      UI_KEY_IDINHOSTOBJ,
      0,
      0,
      "ID of clump in host object.",
      UI_GROUP_COLUMNS_IDS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },





    /* Position related columns (pixel). */
    {
      0, 0, 0, 0,
      "Positional (pixel/image) measurements",
      UI_GROUP_COLUMNS_POSITION_PIXEL
    },
    {
      "x",
      UI_KEY_X,
      0,
      0,
      "Flux weighted center in first FITS axis.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "y",
      UI_KEY_Y,
      0,
      0,
      "Flux weighted center in second FITS axis.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "z",
      UI_KEY_Z,
      0,
      0,
      "Flux weighted center in third FITS axis.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-x",
      UI_KEY_GEOX,
      0,
      0,
      "Geometric center in first FITS axis.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-y",
      UI_KEY_GEOY,
      0,
      0,
      "Geometric center in second FITS axis.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-z",
      UI_KEY_GEOZ,
      0,
      0,
      "Geometric center in third FITS axis.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "min-val-x",
      UI_KEY_MINVALX,
      0,
      0,
      "Minimum value's X axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "max-val-x",
      UI_KEY_MAXVALX,
      0,
      0,
      "Maximum value's X axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "min-val-y",
      UI_KEY_MINVALY,
      0,
      0,
      "Minimum value's Y axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "max-val-y",
      UI_KEY_MAXVALY,
      0,
      0,
      "Maximum value's Y axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "min-val-z",
      UI_KEY_MINVALZ,
      0,
      0,
      "Minimum value's Z axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "max-val-z",
      UI_KEY_MAXVALZ,
      0,
      0,
      "Maximum value's Z axis position",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "min-x",
      UI_KEY_MINX,
      0,
      0,
      "Minimum X axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "max-x",
      UI_KEY_MAXX,
      0,
      0,
      "Maximum X axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "min-y",
      UI_KEY_MINY,
      0,
      0,
      "Minimum Y axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "max-y",
      UI_KEY_MAXY,
      0,
      0,
      "Maximum Y axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "min-z",
      UI_KEY_MINZ,
      0,
      0,
      "Minimum Z axis position",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "max-z",
      UI_KEY_MAXZ,
      0,
      0,
      "Maximum Z axis position.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-x",
      UI_KEY_CLUMPSX,
      0,
      0,
      "Flux.wht center of all clumps in obj. (X).",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-y",
      UI_KEY_CLUMPSY,
      0,
      0,
      "Flux.wht center of all clumps in obj. (Y).",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-z",
      UI_KEY_CLUMPSZ,
      0,
      0,
      "Flux.wht center of all clumps in obj. (Z).",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-geo-x",
      UI_KEY_CLUMPSGEOX,
      0,
      0,
      "Geometric center of all clumps in obj. (X).",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-geo-y",
      UI_KEY_CLUMPSGEOY,
      0,
      0,
      "Geometric center of all clumps in obj. (Y).",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-geo-z",
      UI_KEY_CLUMPSGEOZ,
      0,
      0,
      "Geometric center of all clumps in obj. (Z).",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },





    /* Position related columns (WCS). */
    {
      0, 0, 0, 0,
      "Positional (WCS) measurements",
      UI_GROUP_COLUMNS_POSITION_WCS
    },
    {
      "ra",
      UI_KEY_RA,
      0,
      0,
      "Flux weighted center right ascension.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "dec",
      UI_KEY_DEC,
      0,
      0,
      "Flux weighted center declination.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "w1",
      UI_KEY_W1,
      0,
      0,
      "Flux weighted center in first WCS axis.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "w2",
      UI_KEY_W2,
      0,
      0,
      "Flux weighted center in second WCS axis.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "w3",
      UI_KEY_W3,
      0,
      0,
      "Flux weighted center in third WCS axis.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-w1",
      UI_KEY_GEOW1,
      0,
      0,
      "Geometric center in first WCS axis.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-w2",
      UI_KEY_GEOW2,
      0,
      0,
      "Geometric center in second WCS axis.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-w3",
      UI_KEY_GEOW2,
      0,
      0,
      "Geometric center in third WCS axis.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-w1",
      UI_KEY_CLUMPSW1,
      0,
      0,
      "Flux.wht center of all clumps in 1st WCS.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-w2",
      UI_KEY_CLUMPSW2,
      0,
      0,
      "Flux.wht center of all clumps in 2nd WCS.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-w3",
      UI_KEY_CLUMPSW3,
      0,
      0,
      "Flux.wht center of all clumps in 3rd WCS.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-geo-w1",
      UI_KEY_CLUMPSGEOW1,
      0,
      0,
      "Geometric center of all clumps in 1st WCS.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-geo-w2",
      UI_KEY_CLUMPSGEOW2,
      0,
      0,
      "Geometric center of all clumps in 2nd WCS.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-geo-w3",
      UI_KEY_CLUMPSGEOW3,
      0,
      0,
      "Geometric center of all clumps in 3rd WCS.",
      UI_GROUP_COLUMNS_POSITION_WCS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },





    /* Brightness/pixel-value related columns. */
    {
      0, 0, 0, 0,
      "Brightness/magnitude (only using pixel value/error) measurements",
      UI_GROUP_COLUMNS_BRIGHTNESS
    },
    {
      "sum",
      UI_KEY_SUM,
      0,
      0,
      "Sum of pixel values in each label.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sum-error",
      UI_KEY_SUMERROR,
      0,
      0,
      "Error (1-sigma) in measuring sum.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-sum",
      UI_KEY_CLUMPSSUM,
      0,
      0,
      "Brightness of clumps in an object.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sum-no-river",
      UI_KEY_SUMNORIVER,
      0,
      0,
      "Sky (not river) subtracted clump sum.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "mean",
      UI_KEY_MEAN,
      0,
      0,
      "Mean of values in object/clump.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "std",
      UI_KEY_STD,
      0,
      0,
      "Standard dev. of values in object/clump.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "median",
      UI_KEY_MEDIAN,
      0,
      0,
      "Median of values in object/clump.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "maximum",
      UI_KEY_MAXIMUM,
      0,
      0,
      "Maximum value (mean of top three pixels)",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "magnitude",
      UI_KEY_MAGNITUDE,
      0,
      0,
      "Total magnitude of objects or clumps.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "magnitude-error",
      UI_KEY_MAGNITUDEERROR,
      0,
      0,
      "Magnitude error of objects or clumps.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-magnitude",
      UI_KEY_CLUMPSMAGNITUDE,
      0,
      0,
      "Magnitude of all clumps in object.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "upperlimit",
      UI_KEY_UPPERLIMIT,
      0,
      0,
      "Upper-limit value, use other options to config.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "upperlimit-mag",
      UI_KEY_UPPERLIMITMAG,
      0,
      0,
      "Upper-limit mag. use other options to config.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "upperlimit-onesigma",
      UI_KEY_UPPERLIMITONESIGMA,
      0,
      0,
      "Upper-limit one sigma value.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "upperlimit-sigma",
      UI_KEY_UPPERLIMITSIGMA,
      0,
      0,
      "Place in random distribution (sigma multiple).",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "upperlimit-quantile",
      UI_KEY_UPPERLIMITQUANTILE,
      0,
      0,
      "Quantile in random distribution (max 1).",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "upperlimit-skew",
      UI_KEY_UPPERLIMITSKEW,
      0,
      0,
      "(Mean-Median)/STD of random distribution.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "river-mean",
      UI_KEY_RIVERMEAN,
      0,
      0,
      "Mean river value surrounding a clump.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "river-num",
      UI_KEY_RIVERNUM,
      0,
      0,
      "Number of river pixels around a clump.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sn",
      UI_KEY_SN,
      0,
      0,
      "Signal to noise ratio of objects or clumps.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sky",
      UI_KEY_SKY,
      0,
      0,
      "Sky value (per pixel).",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sky-std",
      UI_KEY_SKYSTD,
      0,
      0,
      "Sky standard deviation (per pixel).",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sigclip-number",
      UI_KEY_SIGCLIPNUMBER,
      0,
      0,
      "Number of pixels in Sigma-clipped measurement.",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sigclip-median",
      UI_KEY_SIGCLIPMEDIAN,
      0,
      0,
      "Median after Sigma-clipping",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sigclip-mean",
      UI_KEY_SIGCLIPMEAN,
      0,
      0,
      "Mean after Sigma-clipping",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sigclip-std",
      UI_KEY_SIGCLIPSTD,
      0,
      0,
      "Standard deviation after Sigma-clipping",
      UI_GROUP_COLUMNS_BRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },





    /* Brightness/pixel-value related columns. */
    {
      0, 0, 0, 0,
      "Surface brightness measurements (all: mag/arcsec^2)",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS
    },
    {
      "sb",
      UI_KEY_SB,
      0,
      0,
      "Surface brightness.",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sb-error",
      UI_KEY_SBERROR,
      0,
      0,
      "Surface brightness error from STD/VAR image.",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "upperlimit-sb",
      UI_KEY_UPPERLIMITSB,
      0,
      0,
      "Upper-limit surface brightness.",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sigclip-mean-sb",
      UI_KEY_SIGCLIPMEANSB,
      0,
      0,
      "Surface brightness of sigclip-mean (1 pix area).",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sigclip-mean-sb-delta",
      UI_KEY_SIGCLIPMEANSBDELTA,
      0,
      0,
      "sigclip-mean-sb delta from sigclip'd STD.",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sigclip-std-sb",
      UI_KEY_SIGCLIPSTDSB,
      0,
      0,
      "Surface brightness of sigclip-std (1 pix area).",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "half-sum-sb",
      UI_KEY_HALFSUMSB,
      0,
      0,
      "Surface brightness within --halfsumarea.",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "half-max-sb",
      UI_KEY_HALFMAXSB,
      0,
      0,
      "Surface brightness within half the maximum.",
      UI_GROUP_COLUMNS_SURFACEBRIGHTNESS,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },





    /* Morphology/shape related columns. */
    {
      0, 0, 0, 0,
      "Morphology/shape (non-parametric) measurements",
      UI_GROUP_COLUMNS_MORPHOLOGY
    },
    {
      "num-clumps",
      UI_KEY_NUMCLUMPS,
      0,
      0,
      "Number of clumps in this object.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "area",
      UI_KEY_AREA,
      0,
      0,
      "Number of non-blank valued pixels.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "area-arcsec2",
      UI_KEY_AREAARCSEC2,
      0,
      0,
      "Area of labeled region in arcsec^2.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "area-min-val",
      UI_KEY_MINVALNUM,
      0,
      0,
      "Number of pixels used in '--min-val-*'.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "area-max-val",
      UI_KEY_MAXVALNUM,
      0,
      0,
      "Number of pixels used in '--max-val-*'.",
      UI_GROUP_COLUMNS_POSITION_PIXEL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "area-xy",
      UI_KEY_AREAXY,
      0,
      0,
      "Projected area in first two dimensions.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "clumps-area",
      UI_KEY_CLUMPSAREA,
      0,
      0,
      "Non-blank area covered by clumps.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "weight-area",
      UI_KEY_WEIGHTAREA,
      0,
      0,
      "Area used for value weighted positions.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-area",
      UI_KEY_GEOAREA,
      0,
      0,
      "Area labled region (irrespective of value).",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-area-xy",
      UI_KEY_GEOAREAXY,
      0,
      0,
      "Projected geo-area in first two dimensions.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "fwhm",
      UI_KEY_FWHM,
      0,
      0,
      "Full width at half max (non-parametric).",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "half-max-area",
      UI_KEY_HALFMAXAREA,
      0,
      0,
      "No. pixels valued above half the max.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "half-max-radius",
      UI_KEY_HALFMAXRADIUS,
      0,
      0,
      "Radius at half the maximum (non-parametric).",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "half-max-sum",
      UI_KEY_HALFMAXSUM,
      0,
      0,
      "Sum of pixels above half the maximum.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "half-sum-area",
      UI_KEY_HALFSUMAREA,
      0,
      0,
      "Area containing half of --brightness.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "half-sum-radius",
      UI_KEY_HALFSUMRADIUS,
      0,
      0,
      "Radius calculated from --halfsumarea.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "frac-max1-sum",
      UI_KEY_FRACMAX1SUM,
      0,
      0,
      "Sum of pixels brighter than 1st frac. of max.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "frac-max2-sum",
      UI_KEY_FRACMAX2SUM,
      0,
      0,
      "Sum of pixels brighter than 2nd frac. of max.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "frac-max1-area",
      UI_KEY_FRACMAX1AREA,
      0,
      0,
      "Area containing 1st fraction of maximum.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "frac-max2-area",
      UI_KEY_FRACMAX2AREA,
      0,
      0,
      "Area containing 2nd fraction of maximum.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "frac-max1-radius",
      UI_KEY_FRACMAX1RADIUS,
      0,
      0,
      "Radius calculated from --fracmaxarea1.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "frac-max2-radius",
      UI_KEY_FRACMAX2RADIUS,
      0,
      0,
      "Radius calculated from --fracmaxarea2.",
      UI_GROUP_COLUMNS_MORPHOLOGY,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },





    /* Elliptical measurements columns. */
    {
      0, 0, 0, 0,
      "Morphology/shape (elliptical) measurements",
      UI_GROUP_COLUMNS_ELLIPTICAL
    },
    {
      "semi-major",
      UI_KEY_SEMIMAJOR,
      0,
      0,
      "RMS along major axis (in pixels).",
      UI_GROUP_COLUMNS_ELLIPTICAL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "semi-minor",
      UI_KEY_SEMIMINOR,
      0,
      0,
      "RMS along minor axis (in pixels).",
      UI_GROUP_COLUMNS_ELLIPTICAL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "axis-ratio",
      UI_KEY_AXISRATIO,
      0,
      0,
      "Flux weighted axis ratio.",
      UI_GROUP_COLUMNS_ELLIPTICAL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "position-angle",
      UI_KEY_POSITIONANGLE,
      0,
      0,
      "Flux weighted position angle.",
      UI_GROUP_COLUMNS_ELLIPTICAL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-semi-major",
      UI_KEY_GEOSEMIMAJOR,
      0,
      0,
      "Geometric RMS along major axis (ignoring value).",
      UI_GROUP_COLUMNS_ELLIPTICAL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-semi-minor",
      UI_KEY_GEOSEMIMINOR,
      0,
      0,
      "Geometric RMS along minor axis (ignoring value).",
      UI_GROUP_COLUMNS_ELLIPTICAL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-axis-ratio",
      UI_KEY_GEOAXISRATIO,
      0,
      0,
      "Geometric (ignoring values, only lab) axis ratio.",
      UI_GROUP_COLUMNS_ELLIPTICAL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "geo-position-angle",
      UI_KEY_GEOPOSITIONANGLE,
      0,
      0,
      "Geometric (ignoring values, only lab) pos. angle.",
      UI_GROUP_COLUMNS_ELLIPTICAL,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },





    /* Multi-valued measurements. */
    {
      0, 0, 0, 0,
      "Vector (multi-valued) measurements",
      UI_GROUP_COLUMNS_VECTOR
    },
    {
      "sum-in-slice",
      UI_KEY_SUMINSLICE,
      0,
      0,
      "[3D input] Sum of values in each slice.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sum-err-in-slice",
      UI_KEY_SUMERRINSLICE,
      0,
      0,
      "[3D input] Error in '--sum-in-slice'.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "area-in-slice",
      UI_KEY_AREAINSLICE,
      0,
      0,
      "[3D input] Number of labeled in each slice.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sum-proj-in-slice",
      UI_KEY_SUMPROJINSLICE,
      0,
      0,
      "[3D input] Sum of projected area in each slice.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "area-proj-in-slice",
      UI_KEY_AREAPROJINSLICE,
      0,
      0,
      "[3D input] Num. voxels in '--sum-proj-in-slice'.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sum-proj-err-in-slice",
      UI_KEY_SUMPROJERRINSLICE,
      0,
      0,
      "[3D input] Error of '--sum-proj-in-slice'.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "area-other-in-slice",
      UI_KEY_AREAOTHERINSLICE,
      0,
      0,
      "[3D input] Area of other lab. in projected area.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sum-other-in-slice",
      UI_KEY_SUMOTHERINSLICE,
      0,
      0,
      "[3D input] Sum of other lab. in projected area.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },
    {
      "sum-other-err-in-slice",
      UI_KEY_SUMOTHERERRINSLICE,
      0,
      0,
      "[3D input] Area in '--sum-other-in-slice'.",
      UI_GROUP_COLUMNS_VECTOR,
      0,
      GAL_TYPE_INVALID,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      ui_column_codes_ll
    },




    {0}
  };





/* Define the child argp structure
   -------------------------------

   NOTE: these parts can be left untouched.*/
struct argp
gal_options_common_child = {gal_commonopts_options,
                            gal_options_common_argp_parse,
                            NULL, NULL, NULL, NULL, NULL};

/* Use the child argp structure in list of children (only one for now). */
struct argp_child
children[]=
{
  {&gal_options_common_child, 0, NULL, 0},
  {0, 0, 0, 0}
};

/* Set all the necessary argp parameters. */
struct argp
thisargp = {program_options, parse_opt, args_doc, doc, children, NULL, NULL};
#endif
