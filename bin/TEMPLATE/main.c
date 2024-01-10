/*********************************************************************
TEMPLATE - A minimal set of files and functions to define a program.
TEMPLATE is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad akhlaghi <mohammad@akhlaghi.org>
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
#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include <gnuastro-internal/timing.h>

#include "main.h"

#include "ui.h"
#include "TEMPLATE.h"


/* Main function. */
int
main (int argc, char *argv[])
{
  struct timeval t1;
  struct TEMPLATEparams p={{{0},0},0};

  /* Set the starting time. */
  time(&p.rawtime);
  gettimeofday(&t1, NULL);

  /* Read the input parameters. */
  ui_read_check_inputs_setup(argc, argv, &p);

  /* Run TEMPLATE. */
  TEMPLATE(&p);

  /* Free all non-freed allocations. */
  ui_free_report(&p, &t1);

  /* Return successfully. */
  return EXIT_SUCCESS;
}
