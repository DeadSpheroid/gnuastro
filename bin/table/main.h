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
#ifndef MAIN_H
#define MAIN_H

/* Include necessary headers */
#include <gsl/gsl_rng.h>
#include <gnuastro/data.h>

#include <gnuastro-internal/options.h>
#include <gnuastro-internal/arithmetic-set.h>

/* Progarm names.  */
#define PROGRAM_NAME   "Table"         /* Program full name.       */
#define PROGRAM_EXEC   "asttable"      /* Program executable name. */
#define PROGRAM_STRING PROGRAM_NAME" (" PACKAGE_NAME ") " PACKAGE_VERSION

/* Row selection types. */
enum select_types
{
 /* Different types of row-selection */
 SELECT_TYPE_RANGE,             /* 0 by C standard */
 SELECT_TYPE_INPOLYGON,
 SELECT_TYPE_OUTPOLYGON,
 SELECT_TYPE_EQUAL,
 SELECT_TYPE_NOTEQUAL,
 SELECT_TYPE_NOBLANK,

 /* This marks the total number of row-selection criteria. */
 SELECT_TYPE_NUMBER,
};



/* Basic structure. */
struct list_select
{
  gal_data_t          *col;
  int                 type;
  struct list_select *next;
};

struct arithmetic_token
{
  /* First layer elements. */
  int            operator;  /* OPERATOR: Code of operator.                */
  int               inlib;  /* OPERATOR: if operator is in library.       */
  size_t     num_operands;  /* OPERATOR: Number of required operands.     */
  size_t            index;  /* OPERAND: Index in requested columns.       */
  char       *id_at_usage;  /* OPERAND: col identifier at usage time.     */
  size_t     num_at_usage;  /* OPERAND: col number at usage.              */
  gal_data_t    *constant;  /* OPERAND: a constant/single number.         */
  gal_data_t     *loadcol;  /* OPERAND: column from another file.         */
  char          *name_def;  /* Name given to the 'set-' operator.         */
  char          *name_use;  /* If this a usage of a name.                 */
  struct arithmetic_token *next;  /* Pointer to next token.               */
};

/* For arithmetic operations. */
struct column_pack
{
  size_t                    start; /* Starting ind. in requested columns. */
  size_t                numsimple; /* Num. of simple columns (no change). */
  struct arithmetic_token  *arith; /* Arithmetic tokens to use.           */
  struct column_pack        *next; /* Next output column.                 */
};





/* Main program parameters structure */
struct tableparams
{
  /* From command-line */
  struct gal_options_common_params cp; /* Common parameters.            */
  char              *filename;  /* Input filename.                      */
  char               *wcsfile;  /* File with WCS.                       */
  char                *wcshdu;  /* HDU in file with WCS.                */
  gal_list_str_t     *columns;  /* List of given columns.               */
  uint8_t         information;  /* ==1: only print FITS information.    */
  uint8_t     colinfoinstdout;  /* ==1: print column metadata in CL.    */
  uint8_t            rowfirst;  /* Do row-based operations first.       */
  gal_data_t           *range;  /* Range to limit output.               */
  gal_data_t       *inpolygon;  /* Columns to check if inside polygon.  */
  gal_data_t      *outpolygon;  /* Columns to check if outside polygon. */
  gal_data_t         *polygon;  /* Values of vertices of the polygon.   */
  gal_data_t           *equal;  /* Values to keep in output.            */
  gal_data_t        *notequal;  /* Values to not include in output.     */
  gal_list_str_t   *noblankll;  /* Remove rows with blank values.       */
  gal_list_str_t  *noblankend;  /* Similar to noblank but at end.       */
  char                  *sort;  /* Column name or number for sorting.   */
  uint8_t          descending;  /* Sort columns in descending order.    */
  size_t                 head;  /* Output only the no. of top rows.     */
  size_t                 tail;  /* Output only the no. of bottom rows.  */
  gal_data_t        *rowrange;  /* Output rows in row-counter range.    */
  size_t            rowrandom;  /* Number of rows to show randomly.     */
  uint8_t             envseed;  /* Use the environment for random seed. */
  gal_list_str_t *catcolumnfile;/* Filename to concat column wise.      */
  gal_list_str_t *catcolumnhdu; /* HDU/extension for the catcolumn.     */
  gal_list_str_t  *catcolumns;  /* List of columns to concatenate.      */
  uint8_t    catcolumnrawname;  /* Don't modify name of appended col.   */
  gal_data_t      *fromvector;  /* Extract columns from a vector column.*/
  uint8_t         keepvectfin;  /* Keep in.s --tovector & --fromvector. */
  gal_list_str_t    *tovector;  /* Merge columns into a vector column.  */
  uint8_t           transpose;  /* Transpose vector columns.            */
  gal_list_str_t  *catrowfile;  /* Filename to concat column wise.      */
  gal_list_str_t   *catrowhdu;  /* HDU/extension for the catcolumn.     */
  gal_data_t     *colmetadata;  /* Set column metadata.                 */
  uint8_t             txteasy;  /* Easy/simple to ready txt output.     */
  char          *txtf32fmtstr;  /* Floating point formats (exp, flt).   */
  char          *txtf64fmtstr;  /* Floating point formats (exp, flt).   */
  int         txtf32precision;  /* Precision of float32 in text.        */
  int         txtf64precision;  /* Precision of float32 in text.        */

  /* Internal. */
  struct column_pack *colpack;  /* Output column packages.              */
  gal_data_t         *noblank;  /* Remove rows that have blank.         */
  gal_data_t           *table;  /* Linked list of output table columns. */
  struct wcsprm          *wcs;  /* WCS structure for conversion.        */
  int                    nwcs;  /* Number of WCS structures.            */
  gal_data_t      *allcolinfo;  /* Information of all the columns.      */
  gal_data_t         *sortcol;  /* Column to define a sorting.          */
  int               selection;  /* Any row-selection is requested.      */
  gal_data_t          *select;  /* Select rows for output.              */
  struct list_select *selectcol; /* Column to define a range.           */
  uint8_t            freesort;  /* If the sort column should be freed.  */
  uint8_t         *freeselect;  /* If selection columns should be freed.*/
  uint8_t              sortin;  /* If the sort column is in the output. */
  time_t              rawtime;  /* Starting time of the program.        */
  gal_data_t       **colarray;  /* Array of columns, with arithmetic.   */
  size_t          numcolarray;  /* Number of elements in 'colarray'.    */
  gsl_rng                *rng;  /* Main random number generator.        */
  const char        *rng_name;  /* Name of random number generator.     */
  unsigned long int  rng_seed;  /* Random number generator seed.        */
  size_t            *colmatch;  /* Number of matches found for columns. */
  uint8_t        txtf32format;  /* Floating point formats (exp, flt).   */
  uint8_t        txtf64format;  /* Floating point formats (exp, flt).   */

  /* For arithmetic operators. */
  gal_list_str_t  *wcstoimg_p;  /* Pointer to the node.                 */
  gal_list_str_t  *imgtowcs_p;  /* Pointer to the node.                 */
  size_t             wcstoimg;  /* Output column no, for conversion.    */
  size_t             imgtowcs;  /* Output column no, for conversion.    */
};

#endif
