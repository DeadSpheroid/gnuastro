/*********************************************************************
Statistics - Statistical analysis on input dataset.
Statistics is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2015-2024 Free Software Foundation, Inc.

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
#include <gnuastro/data.h>

#include <gnuastro-internal/options.h>

/* Progarm names.  */
#define PROGRAM_NAME   "Statistics"    /* Program full name.       */
#define PROGRAM_EXEC   "aststatistics" /* Program executable name. */
#define PROGRAM_STRING PROGRAM_NAME" (" PACKAGE_NAME ") " PACKAGE_VERSION





/* Input formats. */
enum statistics_input_format
  {
    INPUT_FORMAT_INVALID,

    INPUT_FORMAT_TABLE,
    INPUT_FORMAT_IMAGE,
  };

enum statistics_fit_weight_types
  {
    STATISTICS_FIT_WHT_INVALID,

    STATISTICS_FIT_WHT_STD,
    STATISTICS_FIT_WHT_VAR,
    STATISTICS_FIT_WHT_INVVAR,
  };





/* Main program parameters structure */
struct statisticsparams
{
  /* From command-line */
  struct gal_options_common_params cp; /* Common parameters.             */
  gal_list_i32_t         *singlevalue; /* Single value calculations.     */
  gal_list_f64_t  *tp_args;  /* Arguments for printing.                  */
  char          *inputname;  /* Input filename.                          */
  gal_list_str_t  *columns;  /* Column name or number if input is table. */
  float       greaterequal;  /* Only use values >= this value.           */
  float      greaterequal2;  /* Only use values >= this value (2D hist). */
  float           lessthan;  /* Only use values <  this value.           */
  float          lessthan2;  /* Only use values <  this value (2D hist). */
  float           quantmin;  /* Quantile min or range: from Q to 1-Q.    */
  float           quantmax;  /* Quantile maximum.                        */
  uint8_t           ontile;  /* Do single value calculations on tiles.   */
  uint8_t      interpolate;  /* Use interpolation to fill blank tiles.   */
  char            *fitname;  /* Name of fitting function to use.         */
  char          *fitweight;  /* Input weight is 'std' or 'invvar'.       */
  char        *fitestimate;  /* Values to estimate the fit.              */
  char     *fitestimatecol;  /* Column for values to estimate the fit.   */
  char     *fitestimatehdu;  /* HDU for values to estimate the fit.      */
  size_t       fitmaxpower;  /* Maximum power of polynomial fit.         */
  char      *fitrobustname;  /* Name of robust function.                 */

  uint8_t        asciihist;  /* Print an ASCII histogram.                */
  uint8_t         asciicfp;  /* Print an ASCII cumulative frequency plot.*/
  uint8_t        histogram;  /* Save histogram in output.                */
  char        *histogram2d;  /* Save 2D-histogram as image or table.     */
  uint8_t       cumulative;  /* Save cumulative distibution in output.   */
  double            mirror;  /* Mirror value for hist and CFP.           */
  uint8_t              sky;  /* Find the Sky value over the image.       */
  uint8_t        sigmaclip;  /* Do sigma-clipping over all dataset.      */
  uint8_t          madclip;  /* Do MAD-clipping over all dataset.        */
  gal_data_t      *contour;  /* Levels to show contours.                 */

  size_t           numbins;  /* Number of bins in histogram or CFP.      */
  size_t          numbins2;  /* No. of second-dim bins in 2D histogram.  */
  size_t      numasciibins;  /* Number of bins in ASCII plots.           */
  size_t       asciiheight;  /* Height of ASCII histogram or CFP plots.  */
  uint8_t        normalize;  /* set the sum of all bins to 1.            */
  uint8_t   manualbinrange;  /* Set bin min/max manually, not from data. */
  float        onebinstart;  /* Shift bins to start at this value.       */
  float       onebinstart2;  /* Shift bins to start at this value.       */
  uint8_t        maxbinone;  /* Set the maximum bin to 1.                */
  float         mirrordist;  /* Maximum distance after mirror for mode.  */

  char         *kernelname;  /* File name of kernel to convolve input.   */
  char               *khdu;  /* Kernel HDU.                              */
  float       meanmedqdiff;  /* Mode and median quantile difference.     */
  size_t     outliernumngb;  /* Number of neighbors to define outliers.  */
  float       outliersigma;  /* Multiple of sigma to define outlier.     */
  double   outliersclip[2];  /* Outlier Sigma-clipping params.           */
  size_t       smoothwidth;  /* Width of flat kernel to smooth interpd.  */
  uint8_t         checksky;  /* Save the steps for deriving the Sky.     */
  uint8_t checkskynointerp;  /* Stop --checksky before interpolation.    */
  double    sclipparams[2];  /* Muliple and parameter of sigma clipping. */
  double    mclipparams[2];  /* Muliple and parameter of sigma clipping. */
  uint8_t ignoreblankintiles;/* Ignore input's blank values.             */


  /* Internal */
  uint8_t      inputformat;  /* Format of input dataset.                 */
  int          numoutfiles;  /* Number of output files made in this run. */
  uint8_t        needssort;  /* If sorting is needed.                    */
  gal_data_t        *input;  /* Input data structure.                    */
  gal_data_t       *sorted;  /* Sorted input data structure.             */
  int               isfits;  /* Input is a FITS file.                    */
  int             hdu_type;  /* Type of HDU (image or table).            */
  gal_data_t       *kernel;  /* Kernel for convolution of input for Sky. */
  gal_data_t    *convolved;  /* Convolved input.                         */
  gal_data_t        *sky_t;  /* Sky on each tile.                        */
  gal_data_t        *std_t;  /* Sky standard deviation on each tile.     */
  char       *checkskyname;  /* Name of file for Sky calculation steps.  */
  time_t           rawtime;  /* Starting time of the program.            */
  uint8_t            fitid;  /* ID of desired fit.                       */
  uint8_t      fitrobustid;  /* ID of robust fit type.                   */
  gal_data_t    *fitestval;  /* Values to estimate over fit.             */
  int             fitwhtid;  /* Code for the nature of the weight column.*/
};

#endif
