/*********************************************************************
Table - View and manipulate a FITS table structures.
Table is part of GNU Astronomy Utilities (Gnuastro) package.

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
    {
      "column",
      UI_KEY_COLUMN,
      "STR",
      0,
      "Column number (counting from 1) or search string.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->columns,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_csv_strings_append
    },
    {
      "wcsfile",
      UI_KEY_WCSFILE,
      "FITS",
      0,
      "File with WCS if conversion is requested.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->wcsfile,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "wcshdu",
      UI_KEY_WCSHDU,
      "STR",
      0,
      "HDU in file with WCS for conversion.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->wcshdu,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "catcolumnfile",
      UI_KEY_CATCOLUMNFILE,
      "FITS/TXT",
      0,
      "File(s) to be concatenated by column.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->catcolumnfile,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "catcolumnhdu",
      UI_KEY_CATCOLUMNHDU,
      "STR/INT",
      0,
      "HDU/Extension(s) in catcolumnfile.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->catcolumnhdu,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "catcolumns",
      UI_KEY_CATCOLUMNS,
      "STR",
      0,
      "Columns to use in --catcolumnfile.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->catcolumns,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "catrowfile",
      UI_KEY_CATROWFILE,
      "FITS/TXT",
      0,
      "File(s) to be concatenated by row.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->catrowfile,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "catrowhdu",
      UI_KEY_CATROWHDU,
      "STR/INT",
      0,
      "HDU/Extension(s) in --catrowfile.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->catrowhdu,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    /* Output. */
    {
      "information",
      UI_KEY_INFORMATION,
      0,
      0,
      "Only print input table's information.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->information,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "info-num-rows",
      UI_KEY_INFONUMROWS,
      0,
      0,
      "Only print input table's number of rows.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->infonumrows,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "info-num-cols",
      UI_KEY_INFONUMCOLS,
      0,
      0,
      "Only print input table's number of columns.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->infonumcols,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "colinfoinstdout",
      UI_KEY_COLINFOINSTDOUT,
      0,
      0,
      "Column info/metadata when printing to stdout.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->colinfoinstdout,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "txteasy",
      UI_KEY_TXTEASY,
      0,
      0,
      "Short for '-Afixed -B6 -ffixed -p3'.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->txteasy,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "txtf32format",
      UI_KEY_TXTF32FORMAT,
      "STR",
      0,
      "Text output float32 format: 'fixed' or 'exp'.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->txtf32fmtstr,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "txtf64format",
      UI_KEY_TXTF64FORMAT,
      "STR",
      0,
      "Text output float64 format: 'fixed' or 'exp'.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->txtf64fmtstr,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "txtf32precision",
      UI_KEY_TXTF32PRECISION,
      "INT",
      0,
      "Text output float32 precision.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->txtf32precision,
      GAL_TYPE_INT,
      GAL_OPTIONS_RANGE_GE_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "txtf64precision",
      UI_KEY_TXTF64PRECISION,
      "INT",
      0,
      "Text output float64 precision.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->txtf64precision,
      GAL_TYPE_INT,
      GAL_OPTIONS_RANGE_GE_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    /* Operation precendence */
    {
      0, 0, 0, 0,
      "Precedence (default: column operations first)",
      UI_GROUP_PRECEDENCE
    },
    {
      "rowfirst",
      UI_KEY_ROWFIRST,
      0,
      0,
      "Apply row-based operations first.",
      UI_GROUP_PRECEDENCE,
      &p->rowfirst,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    /* Output columns. */
    {
      0, 0, 0, 0,
      "Columns in output:",
      UI_GROUP_OUTCOLS
    },
    {
      "catcolumnrawname",
      UI_KEY_CATCOLUMNRAWNAME,
      0,
      0,
      "Don't touch column names of --catcolumnfile.",
      UI_GROUP_OUTCOLS,
      &p->catcolumnrawname,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "colmetadata",
      UI_KEY_COLMETADATA,
      "ID,S,S,S",
      0,
      "Column metadata (S=STR: Name, Unit, Comments).",
      UI_GROUP_OUTCOLS,
      &p->colmetadata,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_name_and_strings
    },
    {
      "tovector",
      UI_KEY_TOVECTOR,
      "STR,STR[,STR]",
      0,
      "Merge column(s) into a vector column.",
      UI_GROUP_OUTCOLS,
      &p->tovector,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "fromvector",
      UI_KEY_FROMVECTOR,
      "STR,INT[,INT]",
      0,
      "Extract column(s) from a vector column.",
      UI_GROUP_OUTCOLS,
      &p->fromvector,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_name_and_sizets
    },
    {
      "keepvectfin",
      UI_KEY_KEEPVECTFIN,
      0,
      0,
      "Keep inputs of '--tovector' & '--fromvector'.",
      UI_GROUP_OUTCOLS,
      &p->keepvectfin,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "transpose",
      UI_KEY_TRANSPOSE,
      0,
      0,
      "Transpose table (must only contain vector cols).",
      UI_GROUP_OUTCOLS,
      &p->transpose,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },




    /* Output Rows */
    {
      0, 0, 0, 0,
      "Rows in output:",
      UI_GROUP_OUTROWS
    },
    {
      "range",
      UI_KEY_RANGE,
      "STR,FLT:FLT",
      0,
      "Column, and range to limit output.",
      UI_GROUP_OUTROWS,
      &p->range,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_name_and_float64s
    },
    {
      "inpolygon",
      UI_KEY_INPOLYGON,
      "STR,STR",
      0,
      "Coord. columns that are inside '--polygon'.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->inpolygon,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_csv_strings
    },
    {
      "outpolygon",
      UI_KEY_OUTPOLYGON,
      "STR,STR",
      0,
      "Coord. columns that are outside '--polygon'.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->outpolygon,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_csv_strings
    },
    {
      "polygon",
      UI_KEY_POLYGON,
      "FLT,FLT[:...]",
      0,
      "Polygon vertices, also a DS9 region file.",
      UI_GROUP_OUTROWS,
      &p->polygon,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_colon_sep_csv
    },
    {
      "equal",
      UI_KEY_EQUAL,
      "STR,FLT[,...]",
      0,
      "Column, values to keep in output.",
      UI_GROUP_OUTROWS,
      &p->equal,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_name_and_strings
    },
    {
      "notequal",
      UI_KEY_NOTEQUAL,
      "STR,FLT[,...]",
      0,
      "Column, values to remove from output.",
      UI_GROUP_OUTROWS,
      &p->notequal,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_name_and_strings
    },
    {
      "sort",
      UI_KEY_SORT,
      "STR/INT",
      0,
      "Column name or number for sorting.",
      UI_GROUP_OUTROWS,
      &p->sort,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "descending",
      UI_KEY_DESCENDING,
      0,
      0,
      "Sort in descending order: largets first.",
      UI_GROUP_OUTROWS,
      &p->descending,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "head",
      UI_KEY_HEAD,
      "INT",
      0,
      "Only return given number of top rows.",
      UI_GROUP_OUTROWS,
      &p->head,
      GAL_TYPE_SIZE_T,
      GAL_OPTIONS_RANGE_GE_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "tail",
      UI_KEY_TAIL,
      "INT",
      0,
      "Only return given number of bottom rows.",
      UI_GROUP_OUTROWS,
      &p->tail,
      GAL_TYPE_SIZE_T,
      GAL_OPTIONS_RANGE_GE_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "rowrange",
      UI_KEY_ROWRANGE,
      "INT,INT",
      0,
      "Only rows in this row-counter range.",
      UI_GROUP_OUTROWS,
      &p->rowrange,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_GE_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
      gal_options_parse_csv_float64
    },
    {
      "rowrandom",
      UI_KEY_ROWRANDOM,
      "INT",
      0,
      "Number of rows to select randomly.",
      UI_GROUP_OUTROWS,
      &p->rowrandom,
      GAL_TYPE_SIZE_T,
      GAL_OPTIONS_RANGE_GE_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET,
    },
    {
      "envseed",
      UI_KEY_ENVSEED,
      0,
      0,
      "Use GSL_RNG_SEED env. for '--rowrandom'.",
      UI_GROUP_OUTROWS,
      &p->envseed,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "noblank",
      UI_KEY_NOBLANK,
      "STR[,STR]",
      0,
      "Remove rows with blank in given columns.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->noblankll,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "noblankend",
      UI_KEY_NOBLANKEND,
      "STR[,STR]",
      0,
      "Sim. to --noblank, at end (e.g., after arith.).",
      GAL_OPTIONS_GROUP_INPUT,
      &p->noblankend,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    /* End. */
    {0}
  };





/* Define the child argp structure. */
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
