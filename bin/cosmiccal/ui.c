/*********************************************************************
CosmicCalculator - Calculate cosmological parameters
CosmicCalculator is part of GNU Astronomy Utilities (Gnuastro) package.

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
#include <config.h>

#include <argp.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>

#include <gnuastro/fits.h>
#include <gnuastro/table.h>
#include <gnuastro/pointer.h>
#include <gnuastro/speclines.h>
#include <gnuastro/cosmology.h>

#include <gnuastro-internal/timing.h>
#include <gnuastro-internal/options.h>
#include <gnuastro-internal/checkset.h>
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
args_doc[] = "";

const char
doc[] = GAL_STRINGS_TOP_HELP_INFO PROGRAM_NAME" will do cosmological "
  "calculations. If no redshfit is specified, it will only print the main "
  "input parameters. If only a redshift is given, it will print a table of "
  "all calculations. If any of the single row calculations are requested, "
  "only their values will be printed with a single space between each.\n"
  GAL_STRINGS_MORE_HELP_INFO
  /* After the list of options: */
  "\v"
  PACKAGE_NAME" home page: "PACKAGE_URL;




















/**************************************************************/
/*********    Initialize & Parse command-line    **************/
/**************************************************************/
static void
ui_initialize_options(struct cosmiccalparams *p,
                      struct argp_option *program_options,
                      struct argp_option *gal_commonopts_options)
{
  size_t i;
  struct gal_options_common_params *cp=&p->cp;

  /* Set the necessary common parameters structure. */
  cp->program_struct     = p;
  cp->poptions           = program_options;
  cp->program_name       = PROGRAM_NAME;
  cp->program_exec       = PROGRAM_EXEC;
  cp->program_bibtex     = PROGRAM_BIBTEX;
  cp->program_authors    = PROGRAM_AUTHORS;
  cp->coptions           = gal_commonopts_options;

  /* Program specific initializations. */
  p->redshift            = NAN;
  p->velocity            = NAN;

  /* Modify the common options. */
  for(i=0; !gal_options_is_last(&cp->coptions[i]); ++i)
    {
      /* Select by group. */
      switch(cp->coptions[i].group)
        {
        case GAL_OPTIONS_GROUP_OUTPUT:
        case GAL_OPTIONS_GROUP_TESSELLATION:
          cp->coptions[i].doc=NULL; /* Necessary to remove title. */
          cp->coptions[i].flags=OPTION_HIDDEN;
          break;
        }

      /* Select specific options. */
      switch(cp->coptions[i].key)
        {
        case GAL_OPTIONS_KEY_HDU:
        case GAL_OPTIONS_KEY_LOG:
        case GAL_OPTIONS_KEY_TYPE:
        case GAL_OPTIONS_KEY_QUIET:
        case GAL_OPTIONS_KEY_SEARCHIN:
        case GAL_OPTIONS_KEY_NUMTHREADS:
        case GAL_OPTIONS_KEY_IGNORECASE:
        case GAL_OPTIONS_KEY_TABLEFORMAT:
        case GAL_OPTIONS_KEY_STDINTIMEOUT:
          cp->coptions[i].flags=OPTION_HIDDEN;
          break;
        }
    }
}





/* Parse a single option: */
error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
  struct cosmiccalparams *p = state->input;

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
      if(arg[0]!='\0')
        argp_error(state, "currently %s doesn't take any arguments",
                   PROGRAM_NAME);
      break;


    /* This is an option, set its value. */
    default:
      return gal_options_set_from_key(key, arg, p->cp.poptions, &p->cp);
    }

  return 0;
}





static void *
ui_add_to_single_value(struct argp_option *option, char *arg,
                      char *filename, size_t lineno, void *params)
{
  int linecode;
  double *dptr, val=NAN;
  gal_list_str_t *ltmp, *lines;
  struct cosmiccalparams *p = (struct cosmiccalparams *)params;

  /* In case of printing the option values. */
  if(lineno==-1)
    error(EXIT_FAILURE, 0, "currently the options to be printed in one row "
          "(like '--age', '--luminositydist', and etc) do not support "
          "printing with the '--printparams' ('-P'), or writing into "
          "configuration files due to lack of time when implementing "
          "these features. You can put them into configuration files "
          "manually. Please get in touch with us at '%s', so we can "
          "implement it", PACKAGE_BUGREPORT);

  /* If this option is given in a configuration file, then 'arg' will not
     be NULL and we don't want to do anything if it is '0'. */
  switch(option->key)
    {
    /* Options with arguments. */
    case UI_KEY_LINEATZ:

      /* Make sure an argument is given. */
      if(arg==NULL)
        error(EXIT_FAILURE, 0, "option '--lineatz' needs an argument");

      /* The input can be a coma-separated string. */
      lines=gal_options_parse_csv_strings_to_list(arg, filename, lineno);
      gal_list_str_reverse(&lines); /* the function returns the raw list. */
      for(ltmp=lines;ltmp!=NULL;ltmp=ltmp->next)
        {
          /* If the argument is a number, read it, if not, see if it is a
             known specral line name. */
          dptr=&val;
          if( gal_type_from_string((void **)(&dptr), ltmp->v,
                                   GAL_TYPE_FLOAT64) )
            {
              linecode=gal_speclines_line_code(ltmp->v);
              if(linecode==GAL_SPECLINES_INVALID)
                error(EXIT_FAILURE, 0, "'%s' not a known spectral line name",
                      ltmp->v);
              val=gal_speclines_line_angstrom(linecode);
            }
          gal_list_f64_add(&p->specific_arg, val);

          /* Add this option to the print list and return. */
          gal_list_i32_add(option->value, option->key);
        }

      /* Activate the flag for sanity checks later. */
      p->haslineatz=1;
      break;

    /* Options without arguments. */
    default:
      if(arg)
        {
          /* Make sure the value is only '0' or '1'. */
          if( arg[1]!='\0' && *arg!='0' && *arg!='1' )
            error_at_line(EXIT_FAILURE, 0, filename, lineno, "the '--%s' "
                          "option takes no arguments. In a configuration "
                          "file it can only have the values '1' or '0', "
                          "indicating if it should be used or not",
                          option->name);

          /* Only proceed if the (possibly given) argument is 1. */
          if(arg[0]=='0' && arg[1]=='\0') return NULL;
        }

      /* Add this option to the print list and return. */
      gal_list_i32_add(option->value, option->key);
    }
  return NULL;
}





/* Parse the observed line properties: LINE,OBSERVED_WAVELENGHT. */
void *
ui_parse_obsline(struct argp_option *option, char *arg,
                 char *filename, size_t lineno, void *junk)
{
  size_t nc, two=2;
  char *c, *linename;
  gal_data_t *obsline, *tobsline;
  double *dptr, *tdptr, manualwl=NAN;
  char *str, sstr[GAL_OPTIONS_STATIC_MEM_FOR_VALUES];

  /* We want to print the stored values. */
  if(lineno==-1)
    {
      /* Set the value pointer to 'obsline'. */
      obsline=*(gal_data_t **)(option->value);
      dptr = obsline->array;

      /* First write the line name into the output string. */
      nc=0;
      linename=gal_speclines_line_name(obsline->status);
      nc += sprintf(sstr+nc, "%s,", linename);

      /* Write the observed wavelength. */
      sprintf(sstr+nc, "%g", dptr[0]);

      /* Copy the string into a dynamically allocated space, because it
         will be freed later.*/
      gal_checkset_allocate_copy(sstr, &str);
      return str;
    }
  else
    {
      /* The first part of 'arg' (before the first comma) is not
         necessarily a number. So we need to separate the first part from
         the rest.*/
      linename=arg;
      c=arg; while(*c!='\0' && *c!=',') ++c;
      arg = (*c=='\0') ? NULL : c+1;
      *c='\0';

      /* Read the parameters. */
      obsline=gal_options_parse_list_of_numbers(arg, filename, lineno,
                                                GAL_TYPE_FLOAT64);

      /* Only one number must be given as second argument. */
      if(obsline==NULL || obsline->size!=1)
        error(EXIT_FAILURE, 0, "Wrong format given to '--obsline'. Only "
              "two values (line name/wavelengh, and observed wavelengh) "
              "must be given to it");

      /* If a wavelength is given directly as a number (not a name), then
         put that number in a second element of the array. */
      dptr=&manualwl;
      if( gal_type_from_string((void **)(&dptr), linename,
                               GAL_TYPE_FLOAT64) )
        { /* 'linename' isn't a number. */
          obsline->status=gal_speclines_line_code(linename);
          if(obsline->status==GAL_SPECLINES_INVALID)
            error(EXIT_FAILURE, 0, "'%s' not recognized as a standard "
                  "spectral line name", linename);
        }
      else
        { /* 'linename' is a number. */

          /* Allocate the new space. */
          tobsline=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &two, NULL,
                                  0, -1, 1, NULL, NULL, NULL);
          tobsline->status=GAL_SPECLINES_INVALID;

          /* Write the values into tptr. */
          dptr=obsline->array;
          tdptr=tobsline->array;
          tdptr[0]=dptr[0];
          tdptr[1]=manualwl;

          /* Free the old dataset and use the new one. */
          gal_data_free(obsline);
          obsline=tobsline;
        }

      /* Point 'option->value' to the dataset. */
      *(gal_data_t **)(option->value) = obsline;

      /* Our job is done, return NULL. */
      return NULL;
    }
}




















/**************************************************************/
/***************       Sanity Check         *******************/
/**************************************************************/
/* Check ONLY the options. When arguments are involved, do the check
   in 'ui_check_options_and_arguments'. */
static void
ui_check_only_options(struct cosmiccalparams *p)
{
  int hasobsline=p->obsline!=NULL;
  int hasredshift=!isnan(p->redshift);
  int hasvelocity=!isnan(p->velocity);
  double sum = p->olambda + p->omatter + p->oradiation;

  /* Check if the density fractions add up to 1 (within floating point
     error). */
  if( (sum > (1+1e-8) || sum < (1-1e-8)) && p->cp.quiet==0)
    error(EXIT_SUCCESS, 0, "WARNING: non-flat FLRW model: the curvature "
          "density parameter is %.8f; therefore angular diameter based "
          "distances like '--angulardiamdist' or '--arcsectandist' will "
          "be wrong in Gnuastro's current implementation; see "
          "https://savannah.gnu.org/bugs/?65195. This warning message "
          "can be disabled with '--quiet'", 1.0-sum);

  /* Make sure that '--listlines' and '--listlinesatz' aren't called
     together. */
  if(p->listlines && p->listlinesatz)
    error(EXIT_FAILURE, 0, "'--listlines' and '--listlinesatz' can't be "
          "called together");

  /* If any of the line options are requested, make sure that 'lineunit' is
     also given. */
  if( (p->listlines || p->listlinesatz || p->haslineatz || hasobsline)
      && p->lineunit==0 )
    error(EXIT_FAILURE, 0, "no '--lineunit' specified! For the "
          "operations on lines, it is necessary to specify the unit "
          "of the reported wavelength with this option. See the output "
          "of '--help' for acceptable values");

  /* Make sure that '--redshift' and '--obsline' aren't called together. */
  if( (hasredshift + hasvelocity + hasobsline) > 1 )
    error(EXIT_FAILURE, 0, "only one of '--redshift', '--velocity', or "
          "'--obsline' can be called in each run");
}




















/**************************************************************/
/***************       Preparations         *******************/
/**************************************************************/
static void
ui_list_lines(struct cosmiccalparams *p)
{
  double ang;
  size_t i, j;

  /* Make sure '--lineunit' is given. */
  if(p->lineunit==NULL)
    error(EXIT_FAILURE, 0, "no unit specified for the wavelength of the "
          "spectral lines. Please use '--lineunit' to specify your "
          "desired unit");

  /* Print basic information. Note that '--listlinesatz' is requested, also
     print the redshift used. */
  if(p->cp.quiet==0)
    {
      printf("# %s\n", PROGRAM_STRING);
      if(p->listlinesatz)
        printf("# Assumed redshift: %g\n", p->redshift);
      printf("# Source of rest frame wavelenghts (retrieved on 2023-01-15):\n"
             "#     http://astronomy.nmsu.edu/drewski/tableofemission"
             "lines.html\n");

      /* Print column metadata. */
      printf("# Column 1: Wavelength [%s,f32] %s.\n", p->lineunit,
             ( p->listlinesatz
               ? "Line wavelength at assumed redshift"
               : "Rest frame wavelength of the line"));
      printf("# Column 2: Name       [name,  str15] Line name in Gnuastro.\n");
    }

  /* Print the line information. */
  for(i=1;i<GAL_SPECLINES_NUMBER;++i)
    {
      /* Get the wavelength (in angstroms) for this line and print it. */
      ang=gal_speclines_line_angstrom(i);
      printf("%-15.3f %s\n",
             ( p->listlinesatz
               ? ( ang * (1+p->redshift) ) : ang ) * p->lineunitmultip,
             gal_speclines_line_name(i));
    }

  /* Print the break locations. */
  if(p->cp.quiet==0) printf("\n# Hydrogen series limits:\n");
  for(i=0;i<GAL_SPECLINES_LIMIT_NUMBER;++i)
    {
      /* Get the wavelength (in angstroms) for this line and print it. */
      j=i+GAL_SPECLINES_LIMIT_LYMAN;
      ang=gal_speclines_line_angstrom(j);
      printf("%-15.3f %s\n",
             ( p->listlinesatz
               ? ( ang * (1+p->redshift) ) : ang ) * p->lineunitmultip,
             gal_speclines_line_name(j));
    }

  /* Abort the program successfully. */
  exit(EXIT_SUCCESS);
}





static void
ui_preparations_lineunit(struct cosmiccalparams *p)
{
  if(      !strcmp(p->lineunit, "m")        ) p->lineunitmultip=1e-10;
  else if( !strcmp(p->lineunit, "nano-m")   ) p->lineunitmultip=0.1;
  else if( !strcmp(p->lineunit, "micro-m")  ) p->lineunitmultip=1e-4;
  else if( !strcmp(p->lineunit, "angstrom") ) p->lineunitmultip=1;
  else
    error(EXIT_FAILURE, 0, "invalid value '%s' to '--lineunit'! Please "
          "re-run this command with '--help' to see the acceptable "
          "values", p->lineunit);
}





static void
ui_preparations(struct cosmiccalparams *p)
{
  double *obsline = p->obsline ? p->obsline->array : NULL;

  /* Make sure that atleast one of '--redshift', '--obsline', or
     '--velocity' are given (different ways to set/estimate the
     redshift). However, when '--listlines' is called we don't need a
     redshift and the program can run without any of the three options
     above. */
  if(isnan(p->redshift) && p->obsline==NULL && isnan(p->velocity)
     && p->listlines==0 )
    error(EXIT_FAILURE, 0, "no redshift/velocity specified! Please use "
          "'--redshift', '--velocity' (in km/s), or '--obsline' to specify "
          "a redshift, run with '--help' for more");

  /* If a line unit is given find the factor that should be multiplied. */
  if(p->lineunit || p->obsline) ui_preparations_lineunit(p);

  /* If '--listlines' is given, print them and abort the program
     successfully, don't continue with the preparations. Note that
     '--listlines' is the rest-frame lines. So we don't need any
     redshift. */
  if(p->listlines) ui_list_lines(p);

  /* If '--obsline' has been given, set the redshift based on it (it can't
     be called with '--velocity'). */
  if(p->obsline)
    p->redshift = ( (p->obsline->status==GAL_SPECLINES_INVALID)
                    ? gal_speclines_line_redshift(obsline[0], obsline[1])
                    : gal_speclines_line_redshift_code( (obsline[0]
                                                         / p->lineunitmultip),
                                                        p->obsline->status) );

  /* If '--velocity' has been given, set the redshift based on it (it can't
     be called with '--obsline'). */
  if( !isnan(p->velocity) )
    p->redshift = gal_cosmology_z_from_velocity(p->velocity);

  /* Currently GSL will fail for z=0. So if a value of zero is given (bug
     #56299). As a work-around, in such cases, we'll change it to an
     extremely small value. NOTE: This has to be after the 'obsline'
     check.*/
  if(p->redshift==0.0f) p->redshift=MAIN_REDSHIFT_ZERO;

  /* In case the redshift is negative, print an error. It will be detected
     and abort the program, prior to this if given directly, but can also
     happen with '--obsline' for example). */
  if(p->redshift<0)
    error(EXIT_FAILURE, 0, "the selected redshift (%.4f) is negative! "
          "This can happen when you give unreasonable values to indirect "
          "ways of specifying the redshift. For example calling "
          "'--obsline=H-alpha,2000'. When calling '--obsline', please "
          "make sure that your observed line is redder than the rest-frame "
          "wavelength of the given line", p->redshift);

  /* Now that we have the redshift, we can print the 'listlinesatz'
     option. */
  if(p->listlinesatz) ui_list_lines(p);

  /* The list is filled out in a first-in-last-out order. By the time
     control reaches here, the list is finalized. So we should just reverse
     it so the user gets values in the same order they requested them. */
  gal_list_i32_reverse(&p->specific);
  gal_list_f64_reverse(&p->specific_arg);
}



















/**************************************************************/
/************         Set the parameters          *************/
/**************************************************************/

void
ui_read_check_inputs_setup(int argc, char *argv[], struct cosmiccalparams *p)
{
  struct gal_options_common_params *cp=&p->cp;


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


  /* Sanity check only on options. */
  ui_check_only_options(p);


  /* Print the option values if asked. Note that this needs to be done
     after the option checks so un-sane values are not printed in the
     output state. */
  gal_options_print_state(&p->cp);


  /* Read/allocate all the necessary starting arrays. */
  ui_preparations(p);
}
