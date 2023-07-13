/*********************************************************************
MakeNoise - Add noise to a dataset.
MakeNoise is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2015-2023 Free Software Foundation, Inc.

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

#include <argp.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <inttypes.h>

#include <gnuastro/wcs.h>
#include <gnuastro/fits.h>
#include <gnuastro/array.h>
#include <gnuastro/table.h>

#include <gnuastro-internal/timing.h>
#include <gnuastro-internal/options.h>
#include <gnuastro-internal/checkset.h>
#include <gnuastro-internal/tableintern.h>
#include <gnuastro-internal/fixedstringmacros.h>

#include "main.h"

#include "ui.h"
#include "authors-cite.h"





/**************************************************************/
/*********      Argp necessary global entities     ************/
/**************************************************************/
/* Definition parameters for the Argp: */
const char *
argp_program_version = PROGRAM_STRING "\n"
                       GAL_STRINGS_COPYRIGHT
                       "\n\nWritten/developed by "PROGRAM_AUTHORS;

const char *
argp_program_bug_address = PACKAGE_BUGREPORT;

static char
args_doc[] = "ASTRdata";

const char
doc[] = GAL_STRINGS_TOP_HELP_INFO PROGRAM_NAME" will add noise to all the "
  "pixels in an input dataset. The noise parameters can be specified with "
  "the options below. \n"
  GAL_STRINGS_MORE_HELP_INFO
  /* After the list of options: */
  "\v"
  PACKAGE_NAME" home page: "PACKAGE_URL;




















/**************************************************************/
/*********    Initialize & Parse command-line    **************/
/**************************************************************/
static void
ui_initialize_options(struct mknoiseparams *p,
                      struct argp_option *program_options,
                      struct argp_option *gal_commonopts_options)
{
  size_t i;
  struct gal_options_common_params *cp=&p->cp;


  /* Set the necessary common parameters structure. */
  cp->program_struct     = p;
  cp->program_name       = PROGRAM_NAME;
  cp->program_exec       = PROGRAM_EXEC;
  cp->program_bibtex     = PROGRAM_BIBTEX;
  cp->program_authors    = PROGRAM_AUTHORS;
  cp->poptions           = program_options;
  cp->coptions           = gal_commonopts_options;


  /* Initialize options for this program. */
  p->sigma               = NAN;
  p->zeropoint           = NAN;
  p->background          = NAN;
  p->instrumental        = NAN;


  /* Modify common options. */
  for(i=0; !gal_options_is_last(&cp->coptions[i]); ++i)
    {
      /* Selet individually */
      switch(cp->coptions[i].key)
        {
        case GAL_OPTIONS_KEY_TYPE:
        case GAL_OPTIONS_KEY_MINMAPSIZE:
          cp->coptions[i].mandatory=GAL_OPTIONS_MANDATORY;
          break;

        case GAL_OPTIONS_KEY_SEARCHIN:
        case GAL_OPTIONS_KEY_TABLEFORMAT:
        case GAL_OPTIONS_KEY_STDINTIMEOUT:
          cp->coptions[i].flags=OPTION_HIDDEN;
          break;
        }

      /* Select by group. */
      switch(cp->coptions[i].group)
        {
        case GAL_OPTIONS_GROUP_TESSELLATION:
          cp->coptions[i].doc=NULL; /* Necessary to remove title. */
          cp->coptions[i].flags=OPTION_HIDDEN;
          break;
        }
    }
}





/* Parse a single option: */
error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
  struct mknoiseparams *p = state->input;

  /* Pass 'gal_options_common_params' into the child parser.  */
  state->child_inputs[0] = &p->cp;

  /* In case the user incorrectly uses the equal sign (for example
     with a short format or with space in the long format, then 'arg'
     start with (if the short version was called) or be (if the long
     version was called with a space) the equal sign. So, here we
     check if the first character of arg is the equal sign, then the
     user is warned and the program is stopped: */
  if(arg && arg[0]=='=')
    argp_error(state, "incorrect use of the equal sign ('='). For short "
               "options, '=' should not be used and for long options, "
               "there should be no space between the option, equal sign "
               "and value");

  /* Set the key to this option. */
  switch(key)
    {
    /* Read the non-option tokens (arguments): */
    case ARGP_KEY_ARG:
      /* The user may give a shell variable that is empty! In that case
         'arg' will be an empty string! We don't want to account for such
         cases (and give a clear error that no input has been given). */
      if(p->inputname)
        argp_error(state, "only one argument (input file) should be given");
      else
        if(arg[0]!='\0') p->inputname=arg;
      break;

    /* This is an option, set its value. */
    default:
      return gal_options_set_from_key(key, arg, p->cp.poptions, &p->cp);
    }

  return 0;
}




















/**************************************************************/
/***************       Sanity Check         *******************/
/**************************************************************/
/* Read and check ONLY the options. When arguments are involved, do the
   check in 'ui_check_options_and_arguments'. */
static void
ui_read_check_only_options(struct mknoiseparams *p)
{
  /* At leaset one of '--sigma' or '--background' are necessary. */
  if( isnan(p->sigma) && isnan(p->background) )
    error(EXIT_FAILURE, 0, "noise not defined: please define the noise "
          "level with either '--sigma' (for a fixed noise STD for all "
          "the pixels, irrespective of their value) or '--background' "
          "(to use in a Poisson noise model, where the noise will differ "
          "based on pixel value)");


  /* If a background magnitude is given ('--bgbrightness' hasn't been
     called), the zeropoint is necessary. */
  if( !isnan(p->background) )
    {
      /* Make sure that the background can be interpretted properly. */
      if( p->bgnotmag==0 && isnan(p->zeropoint) )
        error(EXIT_FAILURE, 0, "missing background information. When the "
              "noise is identified by the background, a zeropoint magnitude "
              "is mandatory. Please use the '--zeropoint' option to specify "
              "a zeropoint magnitude. Alternatively, if your background value "
              "is brightness (which is in linear scale and doesn't need a "
              "zeropoint), please use '--bgnotmag'");

      /* If the background is in units of magnitudes, convert it to
         brightness. */
      if( p->bgnotmag==0 )
        p->background = pow(10, (p->zeropoint-p->background)/2.5f);

      /* Make sure that the background is larger than 1 (where Poisson
         noise is actually defined). */
      if( p->background < 1 )
        error(EXIT_FAILURE, 0, "background value is smaller than 1. "
              "Poisson noise is only defined on a positive distribution "
              "with values larger than 1. You can use the '--sigma' "
              "option to add a fixed noise level (with any positive value) "
              "to any pixel.");
    }
}





static void
ui_check_options_and_arguments(struct mknoiseparams *p)
{
  /* Make sure an input file name was given and if it was a FITS file, that
     a HDU is also given. */
  if(p->inputname)
    {
      if( gal_fits_file_recognized(p->inputname) && p->cp.hdu==NULL )
        error(EXIT_FAILURE, 0, "no HDU specified. When the input is a FITS "
              "file, a HDU must also be specified, you can use the '--hdu' "
              "('-h') option and give it the HDU number (starting from "
              "zero), extension name, or anything acceptable by CFITSIO");

    }
  else
    error(EXIT_FAILURE, 0, "no input file is specified");
}




















/**************************************************************/
/***************       Preparations         *******************/
/**************************************************************/
void
ui_preparations(struct mknoiseparams *p)
{
  /* Read the input image as a double type */
  p->input=gal_array_read_one_ch_to_type(p->inputname, p->cp.hdu, NULL,
                                         GAL_TYPE_FLOAT64,
                                         p->cp.minmapsize,
                                         p->cp.quietmmap, "--hdu");
  p->input->wcs=gal_wcs_read(p->inputname, p->cp.hdu,
                             p->cp.wcslinearmatrix, 0, 0, &p->input->nwcs,
                             "--hdu");
  p->input->ndim=gal_dimension_remove_extra(p->input->ndim, p->input->dsize,
                                            p->input->wcs);


  /* If we are dealing with an input table, make sure the format of the
     output table is valid, given the type of the output. */
  if(p->input->ndim==1)
    gal_tableintern_check_fits_format(p->cp.output, p->cp.tableformat);


  /* Set the output name: */
  if(p->cp.output)
    gal_checkset_writable_remove(p->cp.output, p->inputname, 0,
                                 p->cp.dontdelete);
  else
    p->cp.output=gal_checkset_automatic_output(&p->cp, p->inputname,
                                               "_noised.fits");

  /* Allocate the random number generator: */
  p->rng=gal_checkset_gsl_rng(p->envseed, &p->rng_name, &p->rng_seed);
}



















/**************************************************************/
/************         Set the parameters          *************/
/**************************************************************/

void
ui_read_check_inputs_setup(int argc, char *argv[], struct mknoiseparams *p)
{
  struct gal_options_common_params *cp=&p->cp;
  char message[GAL_TIMING_VERB_MSG_LENGTH_V];


  /* Include the parameters necessary for argp from this program ('args.h')
     and for the common options to all Gnuastro ('commonopts.h'). We want
     to directly put the pointers to the fields in 'p' and 'cp', so we are
     simply including the header here to not have to use long macros in
     those headers which make them hard to read and modify. This also helps
     in having a clean environment: everything in those headers is only
     available within the scope of this function. */
#include <gnuastro-internal/commonopts.h>
#include "args.h"


  /* Initialize the options and necessary information.  */
  ui_initialize_options(p, program_options, gal_commonopts_options);


  /* Read the command-line options and arguments. */
  errno=0;
  if(argp_parse(&thisargp, argc, argv, 0, 0, p))
    error(EXIT_FAILURE, errno, "parsing arguments");


  /* Read the configuration files and set the common values. */
  gal_options_read_config_set(&p->cp);


  /* Read the options into the program's structure, and check them and
     their relations prior to printing. */
  ui_read_check_only_options(p);


  /* Print the option values if asked. Note that this needs to be done
     after the option checks so un-sane values are not printed in the
     output state. */
  gal_options_print_state(&p->cp);


  /* Prepare all the options as FITS keywords to write in output later. */
  gal_options_as_fits_keywords(&p->cp);


  /* Check that the options and arguments fit well with each other. Note
     that arguments don't go in a configuration file. So this test should
     be done after (possibly) printing the option values. */
  ui_check_options_and_arguments(p);


  /* Read/allocate all the necessary starting arrays. */
  ui_preparations(p);


  /* Everything is ready, notify the user of the program starting. */
  if(!p->cp.quiet)
    {
      printf(PROGRAM_NAME" "PACKAGE_VERSION" started on %s",
             ctime(&p->rawtime));
      sprintf(message, "Random number generator type: %s", p->rng_name);
      gal_timing_report(NULL, message, 1);
      sprintf(message, "Random number generator seed: %lu", p->rng_seed);
      gal_timing_report(NULL, message, 1);
    }
}




















/**************************************************************/
/************      Free allocated, report         *************/
/**************************************************************/
void
ui_free_report(struct mknoiseparams *p, struct timeval *t1)
{
  /* Free the allocated arrays: */
  free(p->cp.hdu);
  free(p->cp.output);
  gsl_rng_free(p->rng);
  gal_data_free(p->input);

  /* Print the final message. */
  if(!p->cp.quiet)
    gal_timing_report(t1, PROGRAM_NAME" finished in: ", 0);
}
