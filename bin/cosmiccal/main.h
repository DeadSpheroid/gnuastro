/*********************************************************************
CosmicCalculator - Calculate cosmological parameters
CosmicCalculator is part of GNU Astronomy Utilities (Gnuastro) package.

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
#include <gnuastro/data.h>
#include <gnuastro/list.h>
#include <gnuastro-internal/options.h>

/* Progarm names.  */
#define PROGRAM_NAME   "CosmicCalculator" /* Program full name.       */
#define PROGRAM_EXEC   "astcosmiccal"     /* Program executable name. */
#define PROGRAM_STRING PROGRAM_NAME" (" PACKAGE_NAME ") " PACKAGE_VERSION

/* To avoid crashing the integration program. */
#define MAIN_REDSHIFT_ZERO 1e-20

/* The z~0.007 corresponds to about 30Mpc in Plank 2018 cosmology. */
#define MAIN_REDSHIFT_SIG_HUBBLE_FLOW 0.007





/* Main program parameters structure */
struct cosmiccalparams
{
  /* Other structures: */
  struct gal_options_common_params cp;  /* Common parameters.           */

  /* Input: */
  double              redshift; /* Redshift of interest.                */
  gal_data_t          *obsline; /* Observed wavelength of a line.       */
  double              velocity; /* Velocity of interest.                */
  double                    H0; /* Current expansion rate (km/sec/Mpc). */
  double               olambda; /* Current cosmological constant dens.  */
  double               omatter; /* Current matter density.              */
  double            oradiation; /* Current radiation density.           */
  uint8_t            listlines; /* List the known spectral lines.       */
  uint8_t         listlinesatz; /* List the known spectral lines.       */
  char *              lineunit; /* Unit of numbers for lines.           */

  /* Outputs. */
  gal_list_i32_t     *specific; /* Codes for single row calculations.   */
  gal_list_f64_t *specific_arg; /* Possible arguments for single calcs. */

  /* Internal: */
  time_t               rawtime; /* Starting time of the program.        */
  double        lineunitmultip; /* Multiple for using line units.       */
  uint8_t           haslineatz; /* A flag for sanity checks.            */
};

#endif
