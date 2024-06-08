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
#include <config.h>

#include <argp.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>

#include <gnuastro/fit.h>
#include <gnuastro/txt.h>
#include <gnuastro/wcs.h>
#include <gnuastro/fits.h>
#include <gnuastro/tile.h>
#include <gnuastro/array.h>
#include <gnuastro/qsort.h>
#include <gnuastro/blank.h>
#include <gnuastro/table.h>
#include <gnuastro/threads.h>
#include <gnuastro/arithmetic.h>
#include <gnuastro/statistics.h>

#include <gnuastro-internal/timing.h>
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
doc[] = GAL_STRINGS_TOP_HELP_INFO PROGRAM_NAME" will do statistical "
  "analysis on the input dataset (table column or image). All blank "
  "pixels or pixels outside of the given range are ignored. You can "
  "either directly ask for certain statistics in one line/row as "
  "shown below with the same order as requested, or get tables of "
  "different statistical measures like the histogram, cumulative "
  "frequency style and etc. If no particular statistic is "
  "requested, some basic information about the dataset is printed "
  "on the command-line.\n"
  GAL_STRINGS_MORE_HELP_INFO
  /* After the list of options: */
  "\v"
  PACKAGE_NAME" home page: "PACKAGE_URL;






















/**************************************************************/
/*********    Initialize & Parse command-line    **************/
/**************************************************************/
static void
ui_initialize_options(struct statisticsparams *p,
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
  cp->numthreads         = gal_threads_number();
  cp->tl.remainderfrac   = NAN;

  /* Program-specific initializers */
  p->lessthan            = NAN;
  p->lessthan2           = NAN;
  p->onebinstart         = NAN;
  p->onebinstart2        = NAN;
  p->greaterequal        = NAN;
  p->greaterequal2       = NAN;
  p->quantmin            = NAN;
  p->quantmax            = NAN;
  p->mirror              = NAN;
  p->mirrordist          = NAN;
  p->meanmedqdiff        = NAN;
  p->sclipparams[0]      = NAN;
  p->sclipparams[1]      = NAN;
  p->fitmaxpower         = GAL_BLANK_SIZE_T;

  /* Set the mandatory common options. */
  for(i=0; !gal_options_is_last(&cp->coptions[i]); ++i)
    switch(cp->coptions[i].key)
      {
      case GAL_OPTIONS_KEY_LOG:
      case GAL_OPTIONS_KEY_TYPE:
        cp->coptions[i].flags=OPTION_HIDDEN;
        break;

      case GAL_OPTIONS_KEY_SEARCHIN:
      case GAL_OPTIONS_KEY_MINMAPSIZE:
      case GAL_OPTIONS_KEY_TABLEFORMAT:
        cp->coptions[i].mandatory=GAL_OPTIONS_MANDATORY;
        break;
      }
}





/* Parse a single option: */
error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
  struct statisticsparams *p = state->input;

  /* Pass 'gal_options_common_params' into the child parser.  */
  state->child_inputs[0] = &p->cp;

  /* In case the user incorrectly uses the equal sign (for example
     with a short format or with space in the long format, then 'arg'
     start with (if the short version was called) or be (if the long
     version was called with a space) the equal sign. So, here we
     check if the first character of arg is the equal sign, then the
     user is warned and the program is stopped: */
  if(arg && arg[0]=='=')
    argp_error(state, "incorrect use of the equal sign ('='). For "
               "short options, '=' should not be used and for long "
               "options, there should be no space between the "
               "option, equal sign and value");

  /* Set the key to this option. */
  switch(key)
    {
    /* Read the non-option tokens (arguments): */
    case ARGP_KEY_ARG:
      /* The user may give a shell variable that is empty! In that case
         'arg' will be an empty string! We don't want to account for such
         cases (and give a clear error that no input has been given). */
      if(p->inputname)
        argp_error(state, "only one argument (input file) should be "
                   "given");
      else
        if(arg[0]!='\0') p->inputname=arg;
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
  size_t i;
  char *str;
  double *d;
  gal_data_t *inputs=NULL;
  struct statisticsparams *p=(struct statisticsparams *)params;

  /* In case of printing the option values. */
  if(lineno==-1)
    {
      gal_checkset_allocate_copy("1", &str);
      return str;
    }

  /* Some of these options take values and some don't. */
  if(option->type==GAL_OPTIONS_NO_ARG_TYPE)
    {
      /* If this option is given in a configuration file, then 'arg' will
         not be NULL and we don't want to do anything if it is '0'. */
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

      /* Add this option to the print list. */
      gal_list_i32_add(&p->singlevalue, option->key);
    }
  else
    {
      /* Read the string of numbers. */
      inputs=gal_options_parse_list_of_numbers(arg, filename, lineno,
                                               GAL_TYPE_FLOAT64);
      if(inputs->size==0)
        error(EXIT_FAILURE, 0, "'--%s' needs a value", option->name);

      /* Do the appropriate operations with the  */
      d=inputs->array;
      switch(option->key)
        {
        case UI_KEY_CONCENTRATION:
          for(i=0;i<inputs->size;++i)
            {
              if(d[i]>0.5f || d[i]<=0.0f)
                error(EXIT_FAILURE, 0, "%g (value given to "
                      "'--concentration': quantile range around median) "
                      "should be larger than 0 and smaller than 0.5",
                      d[i]);
              gal_list_f64_add(&p->tp_args, d[i]);
              gal_list_i32_add(&p->singlevalue, option->key);
            }
          break;

        case UI_KEY_QUANTILE:
        case UI_KEY_QUANTFUNC:
          /* For the quantile and the quantile function, its possible to
             give any number of arguments, so add the operation index and
             the argument once for each given number. */
          for(i=0;i<inputs->size;++i)
            {
              if(option->key==UI_KEY_QUANTILE && (d[i]<0 || d[i]>1) )
                error_at_line(EXIT_FAILURE, 0, filename, lineno, "values "
                              "to '--quantile' ('-u') must be between 0 "
                              "and 1, you had asked for %g (read from "
                              "'%s')", d[i], arg);
              gal_list_f64_add(&p->tp_args, d[i]);
              gal_list_i32_add(&p->singlevalue, option->key);
            }
          break;

        default:
          error_at_line(EXIT_FAILURE, 0, filename, lineno, "a bug! please "
                        "contact us at %s so we can address the problem. "
                        "the option given to 'ui_add_to_print_in_row' is "
                        "marked as requiring a value, but is not "
                        "recognized", PACKAGE_BUGREPORT);
        }
    }

  return NULL;
}





static void *
ui_read_quantile_range(struct argp_option *option, char *arg,
                       char *filename, size_t lineno, void *params)
{
  char *str;
  gal_data_t *in;
  struct statisticsparams *p=(struct statisticsparams *)params;

  /* For the '--printparams' ('-P') option:*/
  if(lineno==-1)
    {
      if( isnan(p->quantmax) )
        {
          if( asprintf(&str, "%g", p->quantmin)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&str, "%g,%g", p->quantmin, p->quantmax)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      return str;
    }

  /* Parse the inputs. */
  in=gal_options_parse_list_of_numbers(arg, filename, lineno,
                                       GAL_TYPE_FLOAT64);

  /* Check if there was only two numbers. */
  if(in->size!=1 && in->size!=2)
    error_at_line(EXIT_FAILURE, 0, filename, lineno, "the '--%s' "
                  "option takes one or two values values (separated by "
                  "a comma) to define the range of used values with "
                  "quantiles. However, %zu numbers were read in the "
                  "string '%s' (value to this option).\n\n"
                  "If there is only one number as input, it will be "
                  "interpretted as the lower quantile (Q) range. The "
                  "higher range will be set to the quantile (1-Q). "
                  "When two numbers are given, they will be used as the "
                  "lower and higher quantile range respectively",
                  option->name, in->size, arg);

  /* Read the values in. */
  p->quantmin = ((double *)(in->array))[0];
  if(in->size==2) p->quantmax = ((double *)(in->array))[1];

  /* Make sure the values are between 0 and 1. */
  if( (p->quantmin<0 || p->quantmin>1)
      || ( !isnan(p->quantmax) && (p->quantmax<0 || p->quantmax>1) ) )
    error_at_line(EXIT_FAILURE, 0, filename, lineno, "values to the "
                  "'--quantrange' option must be between 0 and 1 "
                  "(inclusive). Your input was: '%s'", arg);

  /* When only one value is given, make sure it is less than 0.5. */
  if( !isnan(p->quantmax) && p->quantmin>0.5 )
    error(EXIT_FAILURE, 0, "%g>=0.5! When only one value is given to the "
          "'--%s' option, the range is defined as Q and 1-Q. Thus, the "
          "value must be less than 0.5", p->quantmin, option->name);

  /* Clean up and return. */
  gal_data_free(in);
  return NULL;
}




















/**************************************************************/
/***************       Sanity Check         *******************/
/**************************************************************/
/* Check ONLY the options. When arguments are involved, do the check
   in 'ui_check_options_and_arguments'. */
static void
ui_check_only_options(struct statisticsparams *p)
{
  gal_list_i32_t *tmp;
  struct gal_tile_two_layer_params *tl=&p->cp.tl;

  /* Check if the format of the output table is valid, given the type of
     the output. */
  gal_tableintern_check_fits_format(p->cp.output, p->cp.tableformat);


  /* If in tile-mode, we must have at least one single valued option. */
  if(p->ontile && p->singlevalue==NULL)
    error(EXIT_FAILURE, 0, "at least one of the single-value measurements "
          "(for example '--median') must be requested with the '--ontile' "
          "option: there is no value to put in each tile");

  /* Tessellation related options. */
  if( p->ontile || p->sky )
    {
      /* The tile or sky mode cannot be called with any other modes. */
      if( p->asciihist || p->asciicfp || p->histogram || p->histogram2d
          || p->cumulative || p->sigmaclip || p->madclip || p->fitname
          || !isnan(p->mirror) )
        error(EXIT_FAILURE, 0, "'--ontile' or '--sky' cannot be called "
              "with any of the 'particular' calculation options, for "
              "example '--histogram' or '--madclip'. Because these "
              "operators change the order of the data and element "
              "positions are changed, but in the former positions are "
              "significant");

      /* Make sure the tessellation defining options are given. */
      if( tl->tilesize==NULL || tl->numchannels==NULL
          || isnan(tl->remainderfrac) )
         error(EXIT_FAILURE, 0, "'--tilesize', '--numchannels', and "
               "'--remainderfrac' are mandatory options when dealing with "
               "a tessellation (in '--ontile' or '--sky' mode). Atleast "
               "one of these options wasn't given a value.");
    }


  /* In Sky mode, several options are mandatory. */
  if( p->sky )
    {
      /* Mandatory options. */
      if( isnan(p->meanmedqdiff) || isnan(p->sclipparams[0])
          || p->cp.interpmetric==0 || p->cp.interpnumngb==0 )
        error(EXIT_FAILURE, 0, "'--meanmedqdiff', '--sclipparams', "
              "'--interpmetric' and '--interpnumngb' are mandatory when "
              "requesting Sky measurement ('--sky')");

      /* Make sure a reasonable value is given to '--meanmedqdiff'. */
      if(p->meanmedqdiff>0.5)
        error(EXIT_FAILURE, 0, "%g is not acceptable for "
              "'--meanmedqdiff'! This option is the quantile-difference "
              "between the quantile of the mean and 0.5 (the quantile of "
              "the median). Since the range of quantile is from 0.0 to "
              "1.0, the maximum difference can therefore be 0.5. For "
              "more on this option, please see the \"Quantifying signal "
              "in a tile\" section of the Gnuastro book (with this "
              "command: 'info gnuastro \"Quantifying signal in a "
              "tile\"'", p->meanmedqdiff);
      if(p->meanmedqdiff>0.3 && p->cp.quiet==0)
        error(EXIT_SUCCESS, 0, "WARNING: %g is very high for "
              "'--meanmedqdiff'! This option is the quantile-difference "
              "between the quantile of the mean and 0.5 (the quantile "
              "of the median). For more on this option, please see the "
              "\"Quantifying signal in a tile\" section of the Gnuastro "
              "book (with this command: 'info gnuastro \"Quantifying "
              "signal in a tile\"'. To suppress this warning, please use "
              "the '--quiet' option", p->meanmedqdiff);

      /* If a kernel name has been given, we need the HDU. */
      if(p->kernelname && gal_fits_file_recognized(p->kernelname)
         && p->khdu==NULL )
        error(EXIT_FAILURE, 0, "no HDU specified for the kernel image. "
              "When A HDU is necessary for FITS files. You can use the "
              "'--khdu' ('-u') option and give it the HDU number "
              "(starting from zero), extension name, or anything "
              "acceptable by CFITSIO");
    }


  /* Sigma-clipping needs 'sclipparams' and MAD-clipping needs
     'mclipparams' */
  if(p->sigmaclip && isnan(p->sclipparams[0]))
    error(EXIT_FAILURE, 0, "'--sclipparams' is necessary with "
          "'--sigmaclip'. '--sclipparams' takes two values (separated "
          "by a comma) for defining the sigma-clip: the multiple of "
          "sigma, and tolerance (<1) or number of clips (>1).");
  if(p->madclip && isnan(p->mclipparams[0]))
    error(EXIT_FAILURE, 0, "'--mclipparams' is necessary with "
          "'--madclip' (median absolute deviation clipping). "
          "'--mclipparams' takes two values (separated "
          "by a comma) for defining the MAD-clip: the multiple of "
          "MAD, and tolerance (<1) or number of clips (>1).");


  /* If any of the mode measurements are requested, then 'mirrordist' is
     mandatory. */
  for(tmp=p->singlevalue; tmp!=NULL; tmp=tmp->next)
    switch(tmp->v)
      {
      case UI_KEY_MODE:
      case UI_KEY_MODESYM:
      case UI_KEY_MODEQUANT:
      case UI_KEY_MODESYMVALUE:
        if( isnan(p->mirrordist) )
          error(EXIT_FAILURE, 0, "'--mirrordist' is required for the "
                "mode-related single measurements ('--mode', "
                "'--modequant', '--modesym', and '--modesymvalue')");
        break;
      case UI_KEY_SIGCLIPMAD:
      case UI_KEY_SIGCLIPSTD:
      case UI_KEY_SIGCLIPMEAN:
      case UI_KEY_SIGCLIPNUMBER:
      case UI_KEY_SIGCLIPMEDIAN:
        if( isnan(p->sclipparams[0]) )
          error(EXIT_FAILURE, 0, "'--sclipparams' is necessary with "
                "sigma-clipping measurements.\n\n"
                "'--sclipparams' takes two values (separated by a comma) "
                "for defining the sigma-clip: the multiple of sigma, "
                "and tolerance (<1) or number of clips (>1).");
        break;
      }


  /* If less than and greater than are both given, make sure that the value
     to greater than is smaller than the value to less-than. */
  if( !isnan(p->lessthan) && !isnan(p->greaterequal)
      && p->lessthan < p->greaterequal )
    error(EXIT_FAILURE, 0, "the value to '--lessthan' (%g) must be larger "
          "than the value to '--greaterequal' (%g)", p->lessthan,
          p->greaterequal);


  /* Less-than and greater-equal cannot be called together with
     quantrange. */
  if( ( !isnan(p->lessthan) || !isnan(p->greaterequal) )
      && !isnan(p->quantmin) )
    error(EXIT_FAILURE, 0, "'--lessthan' and/or '--greaterequal' cannot "
          "be called together with '--quantrange'");


  /* When binned outputs are requested, make sure that 'numbins' is set. */
  if( (p->histogram
       || p->histogram2d
       || p->cumulative
       || !isnan(p->mirror))
      && p->numbins==0)
    error(EXIT_FAILURE, 0, "'--numbins' isn't set. When the histogram or "
          "cumulative frequency plots are requested, the number of bins "
          "('--numbins') is necessary");
  if( p->histogram2d )
    {
      if( p->numbins2==0 )
        error(EXIT_FAILURE, 0, "'--numbins2' isn't set. When a 2D "
              "histogram is requested, the number of bins in the "
              "second dimension ('--numbins2') is also necessary");
      if( strcmp(p->histogram2d,"table")
          && strcmp(p->histogram2d,"image") )
        error(EXIT_FAILURE, 0, "the value to '--histogram2d' can "
              "either be 'table' or 'image'");
    }

  /* If an ascii plot is requested, check if the ascii number of bins and
     height are given. */
  if( (p->asciihist || p->asciicfp)
      && (p->numasciibins==0 || p->asciiheight==0) )
    error(EXIT_FAILURE, 0, "when an ascii plot is requested, "
          "'--numasciibins' and '--asciiheight' are mandatory, but "
          "at least one of these has not been given");

  /* Find the fitting type code from the input string. */
  if(p->fitname)
    {
      /* Interpret the name. */
      p->fitid=gal_fit_name_to_id(p->fitname);

      /* Wrong name? */
      if(p->fitid==GAL_FIT_INVALID)
        error(EXIT_FAILURE, 0, "'%s' is not a recognized name for "
              "the type of fitting, please see the description of "
              "'--fit' in the manual (you can run `info %s`",
              p->fitname, PROGRAM_EXEC);

      /* Options for polynomial fits. */
      if(    p->fitid==GAL_FIT_POLYNOMIAL
          || p->fitid==GAL_FIT_POLYNOMIAL_ROBUST
          || p->fitid==GAL_FIT_POLYNOMIAL_WEIGHTED )
        {

          /* '--fitmaxpower' is mandatory. */
          if(p->fitmaxpower==GAL_BLANK_SIZE_T)
            error(EXIT_FAILURE, 0, "'--fitmaxpower' is necessary for "
                  "polynomial fitting. This is the maximum power of X "
                  "in the fitted polynomial");

          /* For the robust types, '--fitrobustname' is mandatory. */
          if( p->fitid==GAL_FIT_POLYNOMIAL_ROBUST )
            {
              /* Make sure '--fitrobust' is given at all. */
              if(p->fitrobustname==NULL)
                error(EXIT_FAILURE, 0, "'--fitrobust' is mandatory for "
                      "robust fittings");

              /* Find the ID of the fit. */
              p->fitrobustid=gal_fit_name_robust_to_id(p->fitrobustname);
              if(p->fitrobustid==GAL_FIT_ROBUST_INVALID)
                error(EXIT_FAILURE, 0, "'%s' is not a recognized robust "
                      "function in the polynomial fittings, please see "
                      "the manual for the acceptable names",
                      p->fitrobustname);
            }
        }
    }

  /* In case '--checkskynointep' is given, we want everything to be similar
     to '--checksky' in the initial phases (when necessary, we will
     check 'p->checkskynointerp'. */
  if(p->checkskynointerp) p->checksky=1;

  /* Reverse the list of statistics to print in one row and also the
     arguments, so it has the same order the user wanted. */
  gal_list_f64_reverse(&p->tp_args);
  gal_list_i32_reverse(&p->singlevalue);
}





static void
ui_check_options_and_arguments(struct statisticsparams *p)
{
  if(p->inputname)
    {
      /* If input is FITS. */
      if( (p->isfits=gal_fits_file_recognized(p->inputname)) )
        {
          /* Check if a HDU is given. */
          if( p->cp.hdu==NULL )
            error(EXIT_FAILURE, 0, "no HDU specified. When the input is a "
                  "FITS file, a HDU must also be specified, you can use "
                  "the '--hdu' ('-h') option and give it the HDU number "
                  "(starting from zero), extension name, or anything "
                  "acceptable by CFITSIO");

          /* If its an image, make sure column isn't given (in case the
             user confuses an image with a table). */
          p->hdu_type=gal_fits_hdu_format(p->inputname, p->cp.hdu,
                                          "--hdu");
          if(p->hdu_type==IMAGE_HDU && p->columns)
            error(EXIT_FAILURE, 0, "%s (hdu: %s): is a FITS image "
                  "extension. The '--column' option is only applicable "
                  "to tables.", p->inputname, p->cp.hdu);
        }
    }
}




















/**************************************************************/
/***************       Preparations         *******************/
/**************************************************************/
static void
ui_out_of_range_to_blank(struct statisticsparams *p)
{
  size_t one=1;
  unsigned char flags=GAL_ARITHMETIC_FLAG_NUMOK;
  unsigned char flagsor = ( GAL_ARITHMETIC_FLAG_INPLACE
                            | GAL_ARITHMETIC_FLAG_NUMOK );
  gal_data_t *tmp, *tmp2, *cond_g=NULL, *cond_l=NULL, *cond, *blank, *ref;


  /* Set the dataset that should be used for the condition. */
  ref = p->input;


  /* If the user has given a quantile range, then set the 'greaterequal'
     and 'lessthan' values. */
  if( !isnan(p->quantmin) )
    {
      /* If only one value was given, set the maximum quantile range. */
      if( isnan(p->quantmax) ) p->quantmax = 1 - p->quantmin;

      /* Set the greater-equal value. */
      tmp=gal_statistics_quantile(ref, p->quantmin, 1);
      tmp=gal_data_copy_to_new_type_free(tmp, GAL_TYPE_FLOAT32);
      p->greaterequal=*((float *)(tmp->array));

      /* Set the lower-than value. */
      tmp=gal_statistics_quantile(ref, p->quantmax, 1);
      tmp=gal_data_copy_to_new_type_free(tmp, GAL_TYPE_FLOAT32);
      p->lessthan=*((float *)(tmp->array));
    }


  /* Set the condition. Note that the 'greaterequal' name is for the data
     we want. So we will set the condition based on those that are
     less-than  */
  if(!isnan(p->greaterequal))
    {
      tmp=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL, 0, -1, 1,
                         NULL, NULL, NULL);
      *((float *)(tmp->array)) = p->greaterequal;
      cond_g=gal_arithmetic(GAL_ARITHMETIC_OP_LT, 1, flags, ref, tmp);
      gal_data_free(tmp);
    }
  if( p->input->next && !isnan(p->greaterequal2) )
    {
      tmp=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL, 0, -1, 1,
                         NULL, NULL, NULL);
      *((float *)(tmp->array)) = p->greaterequal2;
      tmp2=gal_arithmetic(GAL_ARITHMETIC_OP_LT, 1, flags, p->input->next,
                          tmp);
      cond_g=gal_arithmetic(GAL_ARITHMETIC_OP_OR, 1, flagsor, cond_g,
                            tmp2);
      gal_data_free(tmp);
      gal_data_free(tmp2);
    }


  /* Same reasoning as above for 'p->greaterthan'. */
  if(!isnan(p->lessthan))
    {
      tmp=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL, 0, -1, 1,
                         NULL, NULL, NULL);
      *((float *)(tmp->array)) = p->lessthan;
      cond_l=gal_arithmetic(GAL_ARITHMETIC_OP_GE, 1, flags, ref, tmp);
      gal_data_free(tmp);
    }
  if(p->input->next && !isnan(p->lessthan2))
    {
      tmp=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL, 0, -1, 1,
                         NULL, NULL, NULL);
      *((float *)(tmp->array)) = p->lessthan2;
      tmp2=gal_arithmetic(GAL_ARITHMETIC_OP_GE, 1, flags, p->input->next,
                          tmp);
      cond_l=gal_arithmetic(GAL_ARITHMETIC_OP_OR, 1, flagsor, cond_l,
                            tmp2);
      gal_data_free(tmp);
      gal_data_free(tmp2);
    }


  /* Now, set the final condition. If both values were specified, then use
     the GAL_ARITHMETIC_OP_OR to merge them into one. */
  switch( !isnan(p->greaterequal) + !isnan(p->lessthan) )
    {
    case 0: return;             /* No condition was specified, return.  */
    case 1:                     /* Only one condition was specified.    */
      cond = isnan(p->greaterequal) ? cond_l : cond_g;
      break;
    case 2:
      cond = gal_arithmetic(GAL_ARITHMETIC_OP_OR, 1, flagsor, cond_l,
                            cond_g);
      gal_data_free(cond_g);
      break;
    }


  /* Allocate a blank value to mask all pixels that don't satisfy the
     condition. */
  blank=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL,
                       0, -1, 1, NULL, NULL, NULL);
  *((float *)(blank->array)) = NAN;


  /* Set all the pixels that satisfy the condition to blank. Note that a
     blank value will be used in the proper type of the input in the
     'where' operator. Also reset the blank flags so they are checked again
     if necessary.*/
  gal_arithmetic(GAL_ARITHMETIC_OP_WHERE, 1, flagsor, p->input,
                 cond, blank);
  p->input->flag &= ~GAL_DATA_FLAG_BLANK_CH;
  p->input->flag &= ~GAL_DATA_FLAG_HASBLANK;
  if(p->input->next)
    {
      gal_arithmetic(GAL_ARITHMETIC_OP_WHERE, 1, flagsor, p->input->next,
                     cond, blank);
      p->input->next->flag &= ~GAL_DATA_FLAG_BLANK_CH;
      p->input->next->flag &= ~GAL_DATA_FLAG_HASBLANK;
    }

  /* Clean up. */
  gal_data_free(cond);
  gal_data_free(blank);
}





/* Check if a sorted array is necessary and if so, then make a sorted
   array. */
static void
ui_make_sorted_if_necessary(struct statisticsparams *p)
{
  int is_necessary=0;
  gal_list_i32_t *tmp;

  /* Check in the one-row outputs. */
  for(tmp=p->singlevalue; tmp!=NULL; tmp=tmp->next)
    switch(tmp->v)
      {
      case UI_KEY_MODE:
      case UI_KEY_MEDIAN:
      case UI_KEY_QUANTILE:
      case UI_KEY_QUANTFUNC:
      case UI_KEY_MADCLIPMAD:
      case UI_KEY_SIGCLIPMAD:
      case UI_KEY_MADCLIPSTD:
      case UI_KEY_SIGCLIPSTD:
      case UI_KEY_QUANTOFMEAN:
      case UI_KEY_SIGCLIPMEAN:
      case UI_KEY_MADCLIPMEAN:
      case UI_KEY_MADCLIPNUMBER:
      case UI_KEY_SIGCLIPNUMBER:
      case UI_KEY_MADCLIPMEDIAN:
      case UI_KEY_SIGCLIPMEDIAN:
      case UI_KEY_CONCENTRATION:
        is_necessary=1;
        break;
      }

  /* Check in the rest of the outputs. */
  if( p->sigmaclip || p->madclip || !isnan(p->mirror) )
    is_necessary=1;

  /* Do the sorting, we will keep the sorted array in a separate space,
     since the unsorted nature of the original dataset will help decrease
     floating point errors. If the input is already sorted, we'll just
     point it to the input.*/
  if(is_necessary)
    {
      if( gal_statistics_is_sorted(p->input, 1) )
        p->sorted=p->input;
      else
        {
          p->sorted=gal_data_copy(p->input);
          gal_statistics_sort_increasing(p->sorted);
        }
    }
}





/* Merge all given columns into one list. This is because people may call
   for different columns in one call to '--column' ('-c', separated by a
   comma), or multiple calls to '-c'. */
void
ui_read_columns_in_one(struct statisticsparams *p)
{
  size_t i;
  gal_data_t *strs;
  char *c, **strarr;
  gal_list_str_t *tmp, *final=NULL;

  /* Go over the separate calls to the '-c' option, see the explaination in
     Table's 'ui_columns_prepare' function for more on every step. */
  for(tmp=p->columns;tmp!=NULL;tmp=tmp->next)
    {
      /* Remove any new-line character. */
      for(c=tmp->v;*c!='\0';++c)
        if(*c=='\\' && *(c+1)=='\n') { *c=' '; *(++c)=' '; }

      /* Read the different comma-separated strings into an array (within a
         'gal_data_t'). */
      strs=gal_options_parse_csv_strings_to_data(tmp->v, NULL, 0);
      strarr=strs->array;

      /* Add each array element to the final list of columns. */
      for(i=0;i<strs->size;++i)
        gal_list_str_add(&final, strarr[i], 1);

      /* Clean up. */
      gal_data_free(strs);
    }

  /* Reverse the list to be in the same order. */
  gal_list_str_reverse(&final);

  /* For a check.
  for(tmp=final;tmp!=NULL;tmp=tmp->next)
    printf("%s\n", tmp->v);
  */

  /* Free the input list and replace it with 'final'. */
  gal_list_str_free(p->columns, 1);
  p->columns=final;
}





void
ui_read_columns(struct statisticsparams *p)
{
  int tformat;
  gal_data_t *cols, *tmp, *cinfo;
  size_t ncols, nrows, counter=0;
  gal_list_str_t *lines=gal_options_check_stdin(p->inputname,
                                                p->cp.stdintimeout,
                                                "input");

  /* Merge possibly multiple calls to '-c' (each possibly separated by a
     coma) into one list. */
  ui_read_columns_in_one(p);

  /* If any columns are specified, and fitting hasn't been requested, make
     sure there is a maximum of two columns.  */
  if(p->fitname==NULL && gal_list_str_number(p->columns)>2)
    error(EXIT_FAILURE, 0, "%zu input columns were given but currently a "
          "maximum of two columns are supported (two columns only for "
          "special operations, the majority of operations are on a single "
          "column)", gal_list_str_number(p->columns));

  /* If no column is specified, Statistics will abort and an error will be
     printed when the table has more than one column. If there is only one
     column, there is no need to specify any, so Statistics will use it. */
  if(p->columns==NULL)
    {
      /* Get the basic table information. */
      cinfo=gal_table_info(p->inputname, p->cp.hdu, lines, &ncols, &nrows,
                           &tformat, "NONE");
      gal_data_array_free(cinfo, ncols, 1);

      /* See how many columns it has and take the proper action. */
      switch(ncols)
        {
        case 0:
          error(EXIT_FAILURE, 0, "%s contains no usable columns",
                ( p->inputname
                  ? gal_checkset_dataset_name(p->inputname, p->cp.hdu)
                  : "Standard input" ));
        case 1:
          gal_list_str_add(&p->columns, "1", 1);
          break;
        default:
          error(EXIT_FAILURE, 0, "%s is a table containing more than one "
                "column. However, the specific column to work on isn't "
                "specified.\n\n"
                "Please use the '--column' ('-c') option to specify a "
                "column. You can either give it the column number "
                "(couting from 1), or a match/search in its meta-data "
                "(e.g., column names).\n\n"
                "For more information, please run the following command "
                "(press the 'SPACE' key to go down and 'q' to return to "
                "the command-line):\n\n"
                "    $ info gnuastro \"Selecting table columns\"\n",
                ( p->inputname
                  ? gal_checkset_dataset_name(p->inputname, p->cp.hdu)
                  : "Standard input" ));
        }

    }

  /* Read the desired column(s). */
  cols=gal_table_read(p->inputname, p->cp.hdu, lines, p->columns,
                      p->cp.searchin, p->cp.ignorecase, p->cp.numthreads,
                      p->cp.minmapsize, p->cp.quietmmap, NULL, "--hdu");
  gal_list_str_free(lines, 1);
  p->input=cols;

  /* If the input was from standard input, we'll set the input name to be
     'stdin' (for future reporting). */
  if(p->inputname==NULL)
    gal_checkset_allocate_copy("stdin", &p->inputname);

  /* Print an error if there are too many columns: */
  if(gal_list_str_number(p->columns)!=gal_list_data_number(cols))
    gal_tableintern_error_col_selection(p->inputname, p->cp.hdu, "too "
                 "many columns were selected by the given values to the "
                 "'--column' (or '-c'). In other words, the number of "
                 "columns that were read, are more than the number of "
                 "columns given to '--column' (this can happen due to "
                 "columns with same name for example)");

  /* Make sure all columns have a usable datatype. */
  for(tmp=cols; tmp!=NULL; tmp=tmp->next)
    {
      switch(tmp->type)
        {
        case GAL_TYPE_BIT:
        case GAL_TYPE_STRLL:
        case GAL_TYPE_STRING:
        case GAL_TYPE_COMPLEX32:
        case GAL_TYPE_COMPLEX64:
          error(EXIT_FAILURE, 0, " read column number %zu has a %s type, "
                "which is not currently supported by %s", counter,
                gal_type_name(tmp->type, 1), PROGRAM_NAME);
        }
    }

  /* For a check:
  printf("Number of input columns: %zu\n", gal_list_data_number(p->input));
  exit(0);
  */
}





void
ui_preparations_fitestimate(struct statisticsparams *p)
{
  size_t one=1;
  double dbl, *dptr=&dbl;
  gal_list_str_t *fecols=NULL;

  if( gal_type_from_string((void **)(&dptr), p->fitestimate,
                           GAL_TYPE_FLOAT64) )
    {
      /* If the value was "self", then we should put the input file name,
         its HDU and the first column in required values so they are fully
         read. We aren't using the read 'p->input' because rows with a
         blank value have been removed there. */
      if( !strcmp(p->fitestimate, "self") )
        {
          free(p->fitestimate);
          free(p->fitestimatehdu);
          free(p->fitestimatecol);
          gal_checkset_allocate_copy(p->inputname, &p->fitestimate);
          gal_checkset_allocate_copy(p->cp.hdu, &p->fitestimatehdu);
          gal_checkset_allocate_copy(p->columns->v, &p->fitestimatecol);
        }

      /* Make sure a HDU is specified. We need to do this here (not
         in 'ui_check_only_options') because only here we know
         that the user specified a file not a value. */
      if( gal_fits_name_is_fits(p->fitestimate) && p->fitestimatehdu==NULL )
        error(EXIT_FAILURE, 0, "no HDU specified for '%s' (given to "
              "'--fitestimate'). Please use the '--fitestimatehdu' "
              "option to specify the HDU", p->fitestimate);

      /* Make sure a column is specified. We need to do this here (not
         in 'ui_check_only_options') because only here we know
         that the user specified a file not a value. */
      if( p->fitestimatecol==NULL )
        error(EXIT_FAILURE, 0, "no column specified for '%s' (given to "
              "'--fitestimate'). Please use the '--fitestimatecol' "
              "option to specify the column", p->fitestimate);

      /* Given string couldn't be read as a number, so try reading it
         as a table. */
      gal_list_str_add(&fecols, p->fitestimatecol, 1);
      p->fitestval=gal_table_read(p->fitestimate, p->fitestimatehdu,
                                  NULL, fecols,
                                  p->cp.searchin, p->cp.ignorecase, 1,
                                  p->cp.minmapsize, p->cp.quietmmap,
                                  NULL, "--fitestimatehdu");

      /* If more than one column matched, inform the user. */
      if(p->fitestval->next)
        gal_tableintern_error_col_selection(p->fitestimate,
            p->fitestimatehdu, "More than one column matched "
            "the value given to '--fitestimatecol'.");
    }
  else
    {
      /* Given string could be read as a double (in variable 'd'). */
      p->fitestval=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &one,
                                  NULL, 0, -1, 1, NULL, NULL, NULL);
      ((double *)(p->fitestval->array))[0]=dbl;
    }

  /* Make sure 'fitestval' has a double type. */
  p->fitestval=gal_data_copy_to_new_type_free(p->fitestval,
                                              GAL_TYPE_FLOAT64);
}





void
ui_preparations_fit_weight(struct statisticsparams *p)
{
  double *d, *df;
  gal_data_t *wht;

  /* This is only necessary for fitting models that require a weight. */
  switch(p->fitid)
    {
    case GAL_FIT_LINEAR_WEIGHTED:
    case GAL_FIT_POLYNOMIAL_WEIGHTED:
    case GAL_FIT_LINEAR_NO_CONSTANT_WEIGHTED:

      /* Basic sanity check first. */
      if( gal_list_data_number(p->input)<3)
        error(EXIT_FAILURE, 0, "no weight column specified! A "
              "weight-based fit needs a third input column");
      if(p->fitweight==0)
        error(EXIT_FAILURE, 0, "the nature of the input weights for "
              "fitting haven't been specified. Please use the "
              "'--fitweight' option to specify this. It can take "
              "values of 'std' (if the weight column is the standard "
              "deviation), 'var' (for variance) and 'inv-var' "
              "(inverse-variance) which is direct");

      /* Convert the third dataset to double-precision. */
      wht=gal_data_copy_to_new_type_free(p->input->next->next,
                                         GAL_TYPE_FLOAT64);
      p->input->next->next=wht;

      /* Based on the input nature, convert it to the inverse of the
         variance. */
      d=wht->array;
      if( !strcmp(p->fitweight, "std") )         /* Standard deviation. */
        {
          p->fitwhtid=STATISTICS_FIT_WHT_STD;
          df=d+wht->size; do *d=1/(*d * *d); while(++d<df);
        }
      else if ( !strcmp(p->fitweight, "var") )             /* Variance. */
        {
          p->fitwhtid=STATISTICS_FIT_WHT_VAR;
          df=d+wht->size; do *d = 1 / *d; while(++d<df);
        }
      else if ( !strcmp(p->fitweight, "inv-var") ) /* Inverse variance. */
        p->fitwhtid=STATISTICS_FIT_WHT_INVVAR;
      else                                    /* Not recognized: error! */
        error(EXIT_FAILURE, 0, "'%s' is not a recognized weight-type! "
              "Please use either 'std' (standard deviation), 'var' "
              "(variance) or 'inv-var' (inverse variance)", p->fitweight);
      break;
    }
}





void
ui_preparations(struct statisticsparams *p)
{
  gal_data_t *check;
  int keepinputdir=p->cp.keepinputdir;
  struct gal_options_common_params *cp=&p->cp;
  struct gal_tile_two_layer_params *tl=&cp->tl;
  char *checkbasename = p->cp.output ? p->cp.output : p->inputname;

  /* Change 'keepinputdir' based on if an output name was given. */
  p->cp.keepinputdir = p->cp.output ? 1 : 0;

  /* Read the input. */
  if(p->isfits && p->hdu_type==IMAGE_HDU)
    {
      p->inputformat=INPUT_FORMAT_IMAGE;
      p->input=gal_array_read_one_ch(p->inputname, cp->hdu, NULL,
                                     cp->minmapsize, p->cp.quietmmap,
                                     "--hdu");
      p->input->wcs=gal_wcs_read(p->inputname, cp->hdu,
                                 p->cp.wcslinearmatrix, 0, 0,
                                 &p->input->nwcs, "--hdu");
      p->input->ndim=gal_dimension_remove_extra(p->input->ndim,
                                                p->input->dsize,
                                                p->input->wcs);
    }
  else
    {
      /* Read the table columns. */
      ui_read_columns(p);
      p->inputformat=INPUT_FORMAT_TABLE;

      /* Two columns can only be given with 2D histogram mode. */
      if(p->histogram2d==0 && p->fitname==NULL && p->input->next!=NULL)
        error(EXIT_FAILURE, 0, "multi-column input is currently only "
              "supported for 2D histogram or fitting modes");
    }

  /* Read the convolution kernel if necessary. */
  if(p->sky && p->kernelname)
    {
      p->kernel=gal_fits_img_read_kernel(p->kernelname, p->khdu,
                                         cp->minmapsize, p->cp.quietmmap,
                                         "--khdu");
      p->kernel->ndim=gal_dimension_remove_extra(p->kernel->ndim,
                                                 p->kernel->dsize, NULL);
    }

  /* Tile and channel sanity checks and preparations. */
  if(p->ontile || p->sky)
    {
      /* Check the tiles and make the tile structure. */
      gal_tile_full_sanity_check(p->inputname, p->cp.hdu, p->input, tl);
      gal_tile_full_two_layers(p->input, tl);
      gal_tile_full_permutation(tl);

      /* Make the tile check image if requested. */
      if(tl->checktiles)
        {
          tl->tilecheckname=gal_checkset_automatic_output(cp,
                                                          checkbasename,
                                                          "_tiled.fits");
          check=gal_tile_block_check_tiles(tl->tiles);
          if(p->inputformat==INPUT_FORMAT_IMAGE)
            gal_fits_img_write(check, tl->tilecheckname, NULL, 0);
          else
            {
              gal_checkset_writable_remove(tl->tilecheckname, p->inputname,
                                           0, cp->dontdelete);
              gal_table_write(check, NULL, NULL, cp->tableformat,
                              tl->tilecheckname, "TABLE", 0, 0);
            }
          gal_data_free(check);
        }

      /* Set the check-sky output name;  */
      if(p->sky && p->checksky)
        p->checkskyname=gal_checkset_automatic_output(cp, checkbasename,
                                                      "_sky_steps.fits");
    }

  /* Set the out-of-range values in the input to blank. */
  ui_out_of_range_to_blank(p);

  /* If we are not to work on tiles, then re-order and change the input. */
  if(p->ontile==0 && p->sky==0 && p->contour==NULL)
    {
      /* Only keep the elements we want. Note that if we have more than one
         column, we need to move the same rows in both (otherwise their
         widths won't be equal). */
      if(p->input->next) gal_blank_remove_rows(p->input, NULL, 0);
      else               gal_blank_remove(p->input);

      /* If all elements are blank and the user has not asked for
         Statistics to be quiet, then let the user know. */
      if(!p->cp.quiet && p->input->size==0)
        error(EXIT_SUCCESS, 0, "WARNING: %s: no usable (non-blank) data. "
              "If there is data in the input, maybe the '--greaterequal' "
              "or '--lessthan' options need to be adjusted. To remove "
              "this warning, use the '--quiet' (or '-q') option",
              gal_fits_name_save_as_string(p->inputname, cp->hdu));

      /* Make the sorted array if necessary. */
      ui_make_sorted_if_necessary(p);

      /* Set the number of output files. */
      if( !isnan(p->mirror) )             ++p->numoutfiles;
      if( p->histogram || p->cumulative ) ++p->numoutfiles;
    }

  /* Reset 'keepinputdir' to what it originally was. */
  p->cp.keepinputdir=keepinputdir;

  /* Make sure the output doesn't already exist. */
  gal_checkset_writable_remove(p->cp.output, p->inputname, p->cp.keep,
                               p->cp.dontdelete);

  /* Set the fit-estimate column, and prepare the weight based on the
     user's specification.*/
  ui_preparations_fit_weight(p);
  if(p->fitestimate) ui_preparations_fitestimate(p);
}



















/**************************************************************/
/************         Set the parameters          *************/
/**************************************************************/

void
ui_read_check_inputs_setup(int argc, char *argv[],
                           struct statisticsparams *p)
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


  /* Check that the options and arguments fit well with each other. Note
     that arguments don't go in a configuration file. So this test should
     be done after (possibly) printing the option values. */
  ui_check_options_and_arguments(p);


  /* Read/allocate all the necessary starting arrays. */
  ui_preparations(p);


  /* Prepare all the options as FITS keywords to write in output
     later. Note that in some modes, there is no output file, and
     'ui_add_to_single_value' isn't yet prepared. */
  if( (p->singlevalue && p->ontile) || p->sky || p->histogram \
      || p->cumulative)
    gal_options_as_fits_keywords(&p->cp);
}




















/**************************************************************/
/************      Free allocated, report         *************/
/**************************************************************/
void
ui_free_report(struct statisticsparams *p)
{
  /* Free the allocated arrays: */
  free(p->cp.hdu);
  free(p->cp.output);
  gal_list_data_free(p->input);
  gal_list_f64_free(p->tp_args);
  gal_list_i32_free(p->singlevalue);
  gal_tile_full_free_contents(&p->cp.tl);
  if(p->sorted!=p->input) gal_data_free(p->sorted);
}
