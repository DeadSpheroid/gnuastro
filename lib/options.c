/*********************************************************************
Function to parse options and configuration file values.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#include <time.h>
#include <argp.h>
#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>

#include <gnuastro/wcs.h>
#include <gnuastro/git.h>
#include <gnuastro/txt.h>
#include <gnuastro/ds9.h>
#include <gnuastro/fits.h>
#include <gnuastro/list.h>
#include <gnuastro/data.h>
#include <gnuastro/table.h>
#include <gnuastro/blank.h>
#include <gnuastro/units.h>
#include <gnuastro/color.h>
#include <gnuastro/threads.h>
#include <gnuastro/pointer.h>
#include <gnuastro/arithmetic.h>
#include <gnuastro/interpolate.h>

#include <gnuastro-internal/timing.h>
#include <gnuastro-internal/options.h>
#include <gnuastro-internal/checkset.h>
#include <gnuastro-internal/tableintern.h>




















/**********************************************************************/
/************             Option utilities              ***************/
/**********************************************************************/
int
gal_options_is_last(struct argp_option *option)
{
  return ( option->key==0 && option->name==NULL
           && option->doc==NULL && option->group==0 );
}




int
gal_options_is_category_title(struct argp_option *option)
{
  return ( option->key==0 && option->name==NULL );
}





void
gal_options_add_to_not_given(struct gal_options_common_params *cp,
                             struct argp_option *option)
{
  gal_list_str_add(&cp->novalue_doc, (char *)option->doc, 0);
  gal_list_str_add(&cp->novalue_name, (char *)option->name, 0);
}





void
gal_options_abort_if_mandatory_missing(struct gal_options_common_params *cp)
{
  int namewidth=0;
  gal_list_str_t *tmp;
  char info[5000], *name, *doc;

  /* If there is no mandatory options, then just return. */
  if(cp->novalue_name==NULL)
    return;

  /* Get the maximum width of the given names: */
  for(tmp=cp->novalue_name; tmp!=NULL; tmp=tmp->next)
    if( strlen(tmp->v) > namewidth ) namewidth=strlen(tmp->v);

  /* Print the introductory information. */
  sprintf(info, "to continue, the following options need a value ");
  sprintf(info+strlen(info), "(parenthesis after option name contain its "
          "description):\n\n");

  /* Print the list of options along with their description. */
  while(cp->novalue_name!=NULL)
    {
      doc  = gal_list_str_pop(&cp->novalue_doc);
      name = gal_list_str_pop(&cp->novalue_name);
      sprintf(info+strlen(info), "  %-*s (%s\b)\n", namewidth+4, name, doc);
    }
  sprintf(info+strlen(info), "\n");

  /* Print suggestions, way to solve it. */
  sprintf(info+strlen(info), "Use the command-line or a configuration file "
          "to set value(s).\n\nFor a complete description of command-line "
          "options and configuration files, please see the \"Options\" and "
          "\"Configuration files\" section of the Gnuastro book "
          "respectively. You can read them on the command-line by running "
          "the following commands (type 'SPACE' to flip through pages, type "
          "'Q' to return to the command-line):\n\n"
          "  info gnuastro Options\n"
          "  info gnuastro \"Configuration files\"\n");

  error(EXIT_FAILURE, 0, "%s", info);
}





void
gal_options_width_too_large(double width, size_t dim_num,
                            double pixwidth, double pixscale)
{
  /* Just a warning, will not exit program. */
  error(EXIT_SUCCESS, 0, "WARNING: value %g (requested WCS-based width "
        "along dimension %zu) translates to %.0f pixels on this dataset! "
        "This is most probably not what you wanted! Note that the "
        "dataset's pixel size in this dimension is %g. If you intended "
        "this number to show the width in pixels, please add the "
        "'--widthinpix' option", width, dim_num, pixwidth, pixscale);
}





static char *
options_get_home()
{
  char *home;
  home=getenv("HOME");
  if(home==NULL)
    error(EXIT_FAILURE, 0, "HOME environment variable not defined");
  return home;
}




















/**********************************************************************/
/************     Parser functions for common options   ***************/
/**********************************************************************/
void *
gal_options_check_version(struct argp_option *option, char *arg,
                          char *filename, size_t lineno, void *junk)
{
  char *str;

  /* First see if we are reading or writing. */
  if(lineno==-1)
    {
      /* 'PACKAGE_VERSION' is a static/literal string, but the pointer
         returned by this function will be freed, so we must allocate space
         for it.

         We didn't allocate and give this option a value when we read it
         because it is redundant and much more likely for the option to
         just be present (for a check in a reproduction pipeline for
         example) than for it to be printed. So we don't want to waste
         resources in allocating a redundant value. */
      gal_checkset_allocate_copy(PACKAGE_VERSION, &str);
      return str;
    }

  /* Check if the given value is different from this version. */
  else
    {
      if(arg==NULL)
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix "
              "the problem. The value to 'arg' is NULL", __func__,
              PACKAGE_BUGREPORT);
      else if( strcmp(arg, PACKAGE_VERSION) )
        {
          /* Print an error message and abort. */
          error_at_line(EXIT_FAILURE, 0, filename, lineno, "version "
                        "mis-match: you are running GNU Astronomy "
                        "Utilities (Gnuastro) version '%s'. However, "
                        "the 'onlyversion' option is set to version "
                        "'%s'.\n\n"
                        "This was probably done for reproducibility. "
                        "Therefore, manually removing, or changing, the "
                        "option value might produce errors or unexpected "
                        "results. It is thus strongly advised to build "
                        "Gnuastro %s and re-run this command/script.\n\n"
                        "You can download previously released tar-balls "
                        "from the following URLs respectively:\n\n"
                        "    Stable (version format: X.Y):      "
                        "http://ftpmirror.gnu.org/gnuastro\n"
                        "    Alpha  (version format: X.Y.A-B):  "
                        "http://alpha.gnu.org/gnu/gnuastro\n\n"
                        "Alternatively, you can clone Gnuastro, checkout "
                        "the respective commit (from the version "
                        "number), then bootstrap and build it. Please "
                        "run the following command for more "
                        "information:\n\n"
                        "    $ info gnuastro \"Version controlled "
                        "source\"\n", PACKAGE_VERSION, arg, arg);

          /* Just to avoid compiler warnings for unused variables. The program
             will never reach this point! */
          arg=filename=NULL; lineno=0; option=NULL; junk=NULL;
        }
    }
  return NULL;
}





void *
gal_options_print_citation(struct argp_option *option, char *arg,
                           char *filename, size_t lineno, void *pa)
{
  struct gal_options_common_params *cp=(struct gal_options_common_params *)pa;
  char *gnuastro_acknowledgement;
  char *gnuastro_bibtex=
    "First paper introducing Gnuastro\n"
    "--------------------------------\n"
    "  @ARTICLE{gnuastro,\n"
    "     author = {{Akhlaghi}, Mohammad and {Ichikawa}, Takashi},\n"
    "      title = \"{Noise-based Detection and Segmentation of Nebulous "
    "Objects}\",\n"
    "    journal = {ApJS},\n"
    "  archivePrefix = \"arXiv\",\n"
    "     eprint = {1505.01664},\n"
    "   primaryClass = \"astro-ph.IM\",\n"
    "   keywords = {galaxies: irregular, galaxies: photometry,\n"
    "               galaxies: structure, methods: data analysis,\n"
    "               techniques: image processing, techniques: photometric},\n"
    "       year = 2015,\n"
    "      month = sep,\n"
    "     volume = 220,\n"
    "        eid = {1},\n"
    "      pages = {1},\n"
    "        doi = {10.1088/0067-0049/220/1/1},\n"
    "     adsurl = {https://ui.adsabs.harvard.edu/abs/2015ApJS..220....1A},\n"
    "    adsnote = {Provided by the SAO/NASA Astrophysics Data System}\n"
    "  }";


  /* Print the statements. */
  printf("\nThank you for using %s (%s) %s.\n\n", cp->program_name,
         PACKAGE_NAME, PACKAGE_VERSION);
  printf("Citations and acknowledgement are vital for the continued "
         "work on Gnuastro.\n\n"
         "Please cite the following record(s) and add the acknowledgement "
         "statement below in your work to support us. Please note that "
         "different Gnuastro programs may have different corresponding "
         "papers. Hence, please check all the programs you used. Don't "
         "forget to also include the version as shown above for "
         "reproducibility.\n\n"
         "%s\n\n", gnuastro_bibtex);


  /* Only print the citation for the program if one exists. */
  if(cp->program_bibtex[0]!='\0') printf("%s\n\n", cp->program_bibtex);


  /* Notice for acknowledgements. */
  if( asprintf(&gnuastro_acknowledgement,
               "Acknowledgement\n"
               "---------------\n"
               "This work was partly done using GNU Astronomy Utilities "
               "(Gnuastro, ascl.net/1801.009) version %s. Work on "
               "Gnuastro has been funded by the Japanese Ministry of "
               "Education, Culture, Sports, Science, and Technology "
               "(MEXT) scholarship and its Grant-in-Aid for Scientific "
               "Research (21244012, 24253003), the European Research "
               "Council (ERC) advanced grant 339659-MUSICOS, the "
               "Spanish Ministry of Economy and Competitiveness "
               "(MINECO, grant number AYA2016-76219-P) and the "
               "NextGenerationEU grant through the Recovery and "
               "Resilience Facility project ICTS-MRR-2021-03-CEFCA.",
               PACKAGE_VERSION)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  printf("%s\n", gnuastro_acknowledgement);
  free(gnuastro_acknowledgement);


  /* Print a thank you message. */
  printf("                                               ,\n"
         "                                              {|'--.\n"
         "                                             {{\\    \\\n"
         "      Many thanks from all                   |/`'--./=.\n"
         "      Gnuastro developers!                   `\\.---' `\\\\\n"
         "                                                  |\\  ||\n"
         "                                                  | |//\n"
         "                                                   \\//_/|\n"
         "                                                   //\\__/\n"
         "                                                  //\n"
         "                   (http://www.chris.com/ascii/) |/\n");



  /* Exit the program. */
  exit(EXIT_SUCCESS);

  /* Just to avoid compiler warnings for unused variables. The program
     will never reach this point! */
  arg=filename=NULL; lineno=0; option=NULL;
}





void *
gal_options_check_config(struct argp_option *option, char *arg,
                          char *filename, size_t lineno, void *junk)
{
  char *str;

  /* First see if we are reading or writing. */
  if(lineno==-1)
    {
      gal_checkset_allocate_copy("1", &str);
      return str;
    }

  /* Check if the given value is different from this version. */
  else
    {
      /* If its already set then ignore it. */
      if(option->set) return NULL;

      /* Activate the option and let the user know its activated. */
      (*(uint8_t *)(option->value)) = 1;
      printf("-----------------\n"
             "Parsing of options AFTER '--checkconfig'.\n\n"
             "IMPORTANT: Any option that was parsed before encountering "
             "'--checkconfig', on the command-line or in a configuration "
             "file, is not shown here. It is thus recommended to use this "
             "option before calling any other option.\n"
             "-----------------\n");

      /* Print where this option was first seen: if 'checkconfig' is called
         within a configuration file, 'filename!=NULL' and has an argument
         (=="1"). But on the command-line, it has no argument or
         filename. */
      if(filename)
        printf("%s:\n", filename);
      else
        {
          if(arg)
            error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                  "fix the problem. 'filename==NULL', but 'arg!=NULL'",
                  __func__, PACKAGE_BUGREPORT);
          else
            printf("Command-line:\n");
        }

      /* Just to avoid compiler warnings for unused variables. The program
         will never reach this point! */
      arg=filename=NULL; lineno=0; option=NULL; junk=NULL;
    }
  return NULL;
}





void *
gal_options_read_type(struct argp_option *option, char *arg,
                      char *filename, size_t lineno, void *junk)
{
  char *str;
  if(lineno==-1)
    {
      /* Note that 'gal_data_type_as_string' returns a static string. But
         the output must be an allocated string so we can free it. */
      gal_checkset_allocate_copy(
           gal_type_name( *(uint8_t *)(option->value), 1), &str);
      return str;
    }
  else
    {
      /* If the option is already set, just return. */
      if(option->set) return NULL;

      /* Read the value. */
      if ( (*(uint8_t *)(option->value) = gal_type_from_name(arg) )
           == GAL_TYPE_INVALID )
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "'%s' (value to "
                      "'%s' option) couldn't be recognized as a known "
                      "type.\n\nFor the full list of known types, please "
                      "run the following command (press SPACE key to go "
                      "down, and 'q' to return to the command-line):\n\n"
                      "    $ info gnuastro \"Numeric data types\"\n",
                      arg, option->name);

      /* For no un-used variable warning. This function doesn't need the
         pointer. */
      return junk=NULL;
    }
}





void *
gal_options_read_color(struct argp_option *option, char *arg,
                       char *filename, size_t lineno, void *junk)
{
  char *str;
  if(lineno==-1)
    {
      /* Note that 'gal_data_type_as_string' returns a static string. But
         the output must be an allocated string so we can free it. */
      gal_checkset_allocate_copy(
           gal_color_id_to_name( *(uint8_t *)(option->value)), &str);
      return str;
    }
  else
    {
      /* If the option is already set, just return. */
      if(option->set) return NULL;

      /* Read the value. */
      if ( (*(uint8_t *)(option->value) = gal_color_name_to_id(arg) )
           == GAL_COLOR_INVALID )
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "'%s' (value to "
                      "'%s' option) couldn't be recognized as a known "
                      "color.\n\nFor the full list of known types, please "
                      "run the following command:\n\n"
                      "    $ astconvertt --listcolors\n",
                      arg, option->name);

      /* For no un-used variable warning. This function doesn't need the
         pointer. */
      return junk=NULL;
    }
}





void *
gal_options_read_searchin(struct argp_option *option, char *arg,
                          char *filename, size_t lineno, void *junk)
{
  char *str;
  if(lineno==-1)
    {
      /* Note that 'gal_data_type_as_string' returns a static string. But
         the output must be an allocated string so we can free it. */
      gal_checkset_allocate_copy(
        gal_tableintern_searchin_as_string( *(uint8_t *)(option->value)),
        &str);
      return str;
    }
  else
    {
      /* If the option is already set, just return. */
      if(option->set) return NULL;

      /* Read the value. */
      if((*(uint8_t *)(option->value)=gal_tableintern_string_to_searchin(arg))
         == GAL_TABLE_SEARCH_INVALID )
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "'%s' (value to "
                      "'%s' option) couldn't be recognized as a known "
                      "table search-in field ('name', 'unit', or "
                      "'comment').\n\n"
                      "For more explanation, please run the following "
                      "command (press SPACE key to go down, and 'q' to "
                      "return to the command-line):\n\n"
                      "    $ info gnuastro \"Selecting table columns\"\n",
                      arg, option->name);

      /* For no un-used variable warning. This function doesn't need the
         pointer. */
      return junk=NULL;
    }
}





void *
gal_options_read_wcslinearmatrix(struct argp_option *option, char *arg,
                                 char *filename, size_t lineno, void *junk)
{
  char *str=NULL;
  uint8_t value=GAL_WCS_LINEAR_MATRIX_INVALID;
  if(lineno==-1)
    {
      /* The output must be an allocated string (will be 'free'd later). */
      value=*(uint8_t *)(option->value);
      switch(value)
        {
        case GAL_WCS_LINEAR_MATRIX_PC: gal_checkset_allocate_copy("pc",
                                                                  &str);
          break;
        case GAL_WCS_LINEAR_MATRIX_CD: gal_checkset_allocate_copy("cd",
                                                                  &str);
          break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
                "to fix the problem. %u is not a recognized WCS rotation "
                "matrix code", __func__, PACKAGE_BUGREPORT, value);
        }
      return str;
    }
  else
    {
      /* If the option is already set, just return. */
      if(option->set) return NULL;

      /* Read the value. */
      if(      !strcmp(arg, "pc") ) value = GAL_WCS_LINEAR_MATRIX_PC;
      else if( !strcmp(arg, "cd") ) value = GAL_WCS_LINEAR_MATRIX_CD;
      else
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "'%s' (value "
                      "to '%s' option) couldn't be recognized as a known "
                      "WCS rotation matrix. Acceptable values are 'pc' "
                      "or 'cd'", arg, option->name);
      *(uint8_t *)(option->value)=value;

      /* For no un-used variable warning. This function doesn't need the
         pointer. */
      return junk=NULL;
    }
}





void *
gal_options_read_tableformat(struct argp_option *option, char *arg,
                             char *filename, size_t lineno, void *junk)
{
  char *str;
  if(lineno==-1)
    {
      /* Note that 'gal_data_type_as_string' returns a static string. But
         the output must be an allocated string so we can free it. */
      gal_checkset_allocate_copy(
        gal_tableintern_format_as_string( *(uint8_t *)(option->value)),
        &str);
      return str;
    }
  else
    {
      /* If the option is already set, then you don't have to do anything. */
      if(option->set) return NULL;

      /* Read the value. */
      if( (*(uint8_t *)(option->value)=gal_tableintern_string_to_format(arg) )
          ==GAL_TABLE_FORMAT_INVALID )
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "'%s' (value to "
                      "'%s' option) couldn't be recognized as a known "
                      "table format field ('txt', 'fits-ascii', or "
                      "'fits-binary').\n\n", arg, option->name);

      /* For no un-used variable warning. This function doesn't need the
         pointer. */
      return junk=NULL;
    }
}





void *
gal_options_read_interpmetric(struct argp_option *option, char *arg,
                              char *filename, size_t lineno, void *junk)
{
  char *str=NULL;
  if(lineno==-1)
    {
      switch(*(uint8_t *)(option->value))
        {
        case GAL_INTERPOLATE_NEIGHBORS_METRIC_RADIAL:
          gal_checkset_allocate_copy("radial", &str);
          break;
        case GAL_INTERPOLATE_NEIGHBORS_METRIC_MANHATTAN:
          gal_checkset_allocate_copy("manhattan", &str);
          break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code %u is not recognized as a "
                "nearest-neighbor interpolation metric", __func__,
                PACKAGE_BUGREPORT, *(uint8_t *)(option->value));
        }
      return str;
    }
  else
    {
      /* If the option is already set, just return. */
      if(option->set) return NULL;

      /* Set the value. */
      if(       !strcmp(arg, "radial") )
        *(uint8_t *)(option->value) = GAL_INTERPOLATE_NEIGHBORS_METRIC_RADIAL;
      else if ( !strcmp(arg, "manhattan") )
        *(uint8_t *)(option->value) = GAL_INTERPOLATE_NEIGHBORS_METRIC_MANHATTAN;
      else
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "'%s' (value to "
                      "'%s' option) isn't valid. Currently only 'radial' "
                      "and 'manhattan' metrics are recognized for nearest "
                      "neighbor interpolation", arg, option->name);

      /* For no un-used variable warning. This function doesn't need the
         pointer. */
      return junk=NULL;
    }
}





/* If the current token (in a 'colon'-separated list) is a sexagesimal
   number, or a normal number, read it as a double, and return the pointer
   to the end of the string (to continue parsing). We have three types of
   strings at this phase:

      12.34,56.78:90.12,34.56:...                   Normal number.
           ^     ^     ^

      1h2m3.45,6d7m8.90:2h3m4.56,7d8m9.01:...       Sexagesimal (hd)
       ^        ^        ^        ^

      1:2:3.45,6:7:8.90:2:3:4.56,7:8:9.01:...       Sexagesimal (colon)
       ^        ^        ^        ^

  This function is called while trying to tokenize such a string and it
  will stop at the highlighed points. Its job is to determine if the token
  its on is sexagesimal string, or if its a normal number. If its a normal
  number, then just return NULL. But if its a sexagesimal string, it should
  return a copy of the string without any futher components (like the comma
  or colon of the full string).

  HOWEVER, this function may also be given a normal colon-separted list,
  like below. In such cases, it will return a NULL (to let the caller know
  that the ':' in the 'tailptr' was a false alarm, and it can use the
  number before the ':' without worrying about it being a sexagesimal
  number.

      ':0.123,4.567'    which is part of    '1.2345,6.789:0.123,4.567' */
static double
gal_options_read_sexagesimal(size_t dim, char *str, char **tailptr,
                             int abort)
{
  double out;
  char *c, *cc, *copy;
  size_t stlen=0, coloncounter;
  int ishd=0, iscolon=0, isra=0, isdec=0;

  /* A small sanity check. */
  if(dim>1)
    {
      if(abort)
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s', "
              "to find the cause of the problem. The value of 'dim' at "
              "this point should be 0 or 1, but it has a value of %zu",
              __func__, PACKAGE_BUGREPORT, dim);
      else return NAN;
    }

  /* Parse the start of this string, until you find exactly what it is. */
  c=str;
  while( *c!='\0' && ishd==0 && iscolon==0 )
    switch(*c++)
      {
      case 'h': ishd=1; isra=1;  break;
      case 'd': ishd=1; isdec=1; break;
      case ':':
        /* A single colon isn't enough to be sure that this is a
           colon-based sexagesimal number. We need at least one more before
           we reach the end of the string, or the next comma. */
        cc=c; /* 'c' is already on the next character! */
        coloncounter=1; /* We have already passed the first colon! */
        while(*cc!='\0' && *cc!=',')
          switch(*cc++) { case ':': coloncounter++; break; }

        /* If the loop above ended in a comma, then it stops and its time
           to check if this is actually a sexagesimal string or not. If we
           have reached the end of the string, and there was only two
           colons, then this is indeed a colon-based sexagesimal string. */
        if( (*cc=='\0' || *cc==',')
            && coloncounter>=2 )
          iscolon=1;

        /* If 'iscolon' is still zero until we reach this point, then we
           should simply return with a NaN, letting the user know that the
           'tailptr' was a false alarm and it can continue with parsing the
           string. */
        if(iscolon==0) return NAN;

        /* Everything is good, continue. */
        if(dim==0) isra=1; else isdec=1;
        break;
      }

  /* Make sure the string could be identified properly. */
  if( (isra==0 && isdec==0) || (ishd==0 && iscolon==0) )
    {
      if(abort)
        error(EXIT_FAILURE, 0, "the first token in the string '%s' "
              "couldn't be parsed as a sexagesimal string", str);
      else return NAN;
    }

  /* Check if the hd-type is given for the proper dimension. */
  if( (isra && dim!=0) || (isdec && dim!=1) )
    {
      if(abort)
        error(EXIT_FAILURE, 0, "the order of sexagesimal coordinates "
              "is wrong! The first should be RA (with a format like "
              "'__h__m__'), and the second should be Dec ('__d__m__')");
      else return NAN;
    }

  /* Depending on the mode, find the length of the full string into a new
     string to give to the unit conversion functions. Note that the
     delimiters are ':', ','. However, if we are on a colon-based
     sexagesimal, we should keep the first two colons and only use the
     third as a delimiter. */
  c=str;
  coloncounter=0;
  while( *c!='\0' && stlen==0)
    switch(*c++)
      {
      case ':':
        if(iscolon)
          { if(++coloncounter == 3) stlen=c-str; }
        else stlen=c-str;
        break;
      case ',': stlen=c-str;
      }

  /* If 'stlen==0', then we are on the last token, and we can simply use
     the 'strlen' function, but because we are assuming the last extra
     character in the loop above, we should add one to 'strlen' to be
     consistant. */
  if(stlen==0) stlen=strlen(str)+1;

  /* Copy the string of this sexagesimal coordinate into a new string to
     pass to the unit conversion function. */
  copy=gal_pointer_allocate(GAL_TYPE_UINT8, stlen+1, 0, __func__,
                            "copy");
  memcpy(copy, str, stlen-1);
  copy[stlen-1]='\0';

  /* Convert the string to degrees and set the 'tailptr' pointer. */
  out = ( isra
          ? gal_units_ra_to_degree(copy)
          : gal_units_dec_to_degree(copy) );
  *tailptr=str+stlen-1;

  /* Sanity check: */
  if(abort && isnan(out))
    error(EXIT_FAILURE, 0, "%s: '%s' couldn't be parsed as a "
          "sexagesimal representation of %s", __func__, copy,
          isra?"RA (right ascension)":"DEC (Declination)");

  /* Clean up, set the 'tailptr' pointer, and return. */
  free(copy);
  return out;
}





/* The input to this function is a string of any number of numbers
   separated by a comma (',') and possibly containing fractions, for
   example: '1,2/3, 4.95'. The output 'gal_data_t' contains the array of
   given values in 'double' type. You can read the number from its 'size'
   element. */
gal_data_t *
gal_options_parse_list_of_numbers(char *string, char *filename,
                                  size_t lineno, uint8_t type)
{
  size_t num=0;
  gal_data_t *out;
  char *c=string, *tailptr;
  gal_list_f64_t *list=NULL;
  double numerator=NAN, denominator=NAN, tmp, ttmp;

  /* The nature of the arrays/numbers read here is very small, so since
     'p->cp.minmapsize' might not have been read yet, we will set it to -1
     (largest size_t number), so the values are kept in memory. */
  int quietmmap=1;
  size_t minmapsize=-1;

  /* If we have an empty string, just return NULL. */
  if(string==NULL || *string=='\0') return NULL;

  /* Go through the input character by character. */
  while(string && *c!='\0')
    {
      switch(*c)
        {

        /* Ignore space or tab. */
        case ' ':
        case '\t':
          ++c;
          break;

        /* Comma or Colon mark the transition to the next number. */
        case ',':
        case ':':
          if(isnan(numerator))
            error_at_line(EXIT_FAILURE, 0, filename, lineno, "a number "
                          "must be given before ','. You have given: '%s'",
                          string);
          gal_list_f64_add(&list, isnan(denominator)
                           ? numerator : numerator/denominator);
          numerator=denominator=NAN;
          ++num;
          ++c;
          break;

        /* Divide two numbers. */
        case '/':
          if( isnan(numerator) || !isnan(denominator) )
            error_at_line(EXIT_FAILURE, 0, filename, lineno, "'/' must "
                          "only be between two numbers and used for "
                          "division. But you have given '%s'", string);
          ++c;
          break;

        /* Extra dot is an error (cases like 2.5.5). Valid '.'s will be
           read by 'strtod'. */
        case '.':
          error_at_line(EXIT_FAILURE, 0, filename, lineno,
                        "extra '.' in '%s'", string);
          break;

        /* Read the number. */
        default:

          /* Parse the string. */
          ttmp=NAN;
          tmp=strtod(c, &tailptr);
          if(*tailptr!=',' && *tailptr!='/' && *tailptr!='\0')
            {
              /* See if the user has given a sexagesimal value (that can't
                 be easily read with 'strtod'). */
              ttmp=gal_options_read_sexagesimal(num%2, c, &tailptr, 0);
              if(isnan(ttmp))
                {
                  /* This happens in cases like the values to Table's
                     '--range=NAME,5:10'. In such cases, we do have a
                     colon, but don't have a sexagesimal number. So we
                     shouldn't abort the program! */
                  if(tailptr[0]!=':')
                    error_at_line(EXIT_FAILURE, 0, filename, lineno,
                                  "the '%s' component of '%s' couldn't "
                                  "be parsed as a usable number", c,
                                  string);
                }
              else tmp=ttmp;
            }

          /* See if the number should be put in the numerator or
             denominator. */
          if( isnan(numerator) ) numerator=tmp;
          else
            {
              if(isnan(denominator)) denominator=tmp;
              else error_at_line(EXIT_FAILURE, 0, filename, lineno,
                                 "more than two numbers in each "
                                 "element.");
            }

          /* Set 'c' to tailptr. */
          c=tailptr;
        }
    }


  /* If the last number wasn't finished by a ',', add the read value to the
     list. */
  if( !isnan(numerator) )
    {
      ++num;
      gal_list_f64_add(&list, isnan(denominator)
                       ? numerator : numerator/denominator);
    }

  /* Convert the list into the desired type. */
  gal_list_f64_reverse(&list);
  out=gal_list_f64_to_data(list, type, minmapsize, quietmmap);

  /* Clean up and return. */
  gal_list_f64_free(list);
  return out;
}





/* Replacement characters for commented comma (ASCII code 14 for "Shift
   out") or colon (ASCII code 15 for "Shift in"). These are chosen as
   non-printable ASCII characters, that user's will not be typing. */
#define OPTIONS_COMMENTED_COMMA 14
#define OPTIONS_COMMENTED_COLON 15
gal_data_t *
gal_options_parse_list_of_strings(char *string, char *filename,
                                  size_t lineno)
{
  size_t num;
  gal_data_t *out;
  int needscorrection;
  gal_list_str_t *list=NULL, *tll;
  char *c, *d, *cp, *token, **strarr, delimiters[]=",:";

  /* The nature of the arrays/numbers read here is very small, so since
     'p->cp.minmapsize' might not have been read yet, we will set it to -1
     (largest size_t number), so the values are kept in memory. */
  int quietmmap=1;
  size_t minmapsize=-1;

  /* If we have an empty string, just return NULL. */
  if(string==NULL || *string=='\0') return NULL;

  /* Make a copy of the input string, remove all commented delimiters
     (those with a preceding '\'). */
  gal_checkset_allocate_copy(string, &cp);
  for(c=cp; *c!='\0'; c++)
    if(*c=='\\' && c[1]!='\0')
      {
        /* If the next character (after the '\') is a delimiter, we need to
           replace it with a non-delimiter (and not-typed!) character and
           shift the whole string back by one character to simplify future
           steps. */
        needscorrection=0;
        switch(c[1])
          {
          case ',': *c=OPTIONS_COMMENTED_COMMA; needscorrection=1; break;
          case ':': *c=OPTIONS_COMMENTED_COLON; needscorrection=1; break;
          }
        if(needscorrection)
          { for(d=c+2; *d!='\0'; ++d) {*(d-1)=*d;} *(d-1)='\0'; }
      }

  /* Start separating the tokens. */
  token=strtok(cp, delimiters);
  gal_list_str_add(&list, token, 1);
  while(token!=NULL)
    {
      token=strtok(NULL, delimiters);
      if(token!=NULL)
        gal_list_str_add(&list, token, 1);
    }

  /* Allocate the output dataset (array containing all the given
     strings). */
  num=gal_list_str_number(list);
  out=gal_data_alloc(NULL, GAL_TYPE_STRING, 1, &num, NULL, 0,
                     minmapsize, quietmmap, NULL, NULL, NULL);

  /* Fill the output dataset. */
  strarr=out->array;
  for(tll=list;tll!=NULL;tll=tll->next)
    {
      /* If we had commented delimiters, we need to set them back to their
         original/typed forms. */
      for(c=tll->v; *c!='\0'; ++c)
        switch(*c)
          {
          case OPTIONS_COMMENTED_COMMA: *c=','; break;
          case OPTIONS_COMMENTED_COLON: *c=':'; break;
          }

      /* Put the pointer of the string in the output array. */
      strarr[--num]=tll->v;
    }

  /* Clean up and return. Note that we don't want to free the values in the
     list, the elements in 'out->array' point to them and will later use
     them. */
  free(cp);
  gal_list_str_free(list, 0);
  return out;
}





static int
options_string_has_space(char *string)
{
  char *c;

  /* If a space character is present, just return. */
  for(c=string;*c!='\0';++c) if(*c==' ' || *c=='\t') return 1;

  /* If control reaches here, there was no space character. */
  return 0;
}




#if 0
static char *
options_print_liststr_as_csv(void *inval)
{
  char *out=NULL;
  gal_list_str_t *tmp, *values=*(gal_list_str_t **)(option->value);

  for(tmp=values;tmp!=NULL;tmp=tmp->next)
    printf("%s: %s\n", __func__, tmp->v);

  /* Return output. */
  return out;
}
#endif





/* NOTE: output is REVERSED (since the option parsing automatically
   reverses STRLLs at its end). */
gal_list_str_t *
gal_options_parse_csv_strings_to_list(char *string, char *filename,
                                      size_t lineno)
{
  char *str=NULL;
  char *cc, *c=string;
  gal_list_str_t *list=NULL;

  /* Remove any possibly commented new-line where we have a backslash
     followed by a new-line character (replace the two characters with two
     single space characters). This can happen with the "--column='arith'"
     feature in Table, when it gets long (bug #58371). But to be general in
     other cases too, we'll just correct it here. */
  for(c=string;*c!='\0';++c)
    if(*c=='\\' && *(c+1)=='\n') { *c=' '; *(++c)=' '; }

  /* Go through the input character by character. */
  c=string;
  while(string && *c!='\0')
    {
      switch(*c)
        {
        /* Comma marks the transition to the next string. */
        case ',':

          /* The whole string can't start with a comma. */
          if(str==NULL)
            {
              if(filename)
                error_at_line(EXIT_FAILURE, 0, filename, lineno, "a "
                              "string must exist before the first ','. "
                              "You have given: '%s'", string);
              else
                error(EXIT_FAILURE, 0, "a string must exist before the "
                      "first ','. You have given: '%s'", string);
            }

          /* If the previous character was a '\', this coma isn't a
             delimiter, but is actually within the string. So shift all the
             characters in the string one backward to remove the
             backslah and ignore the comma as a delimiter. */
          if(*(c-1)=='\\') for(cc=c-1;*cc!='\0';++cc) *cc=*(cc+1);
          else
            {
              *c='\0';
              gal_list_str_add(&list, str, 1);
              str=NULL;  /* Mark that the next character is the start. */
            }
          break;

        /* If the character isn't a coma, it is either in the middle of a
           string at the start of it. If 'str==NULL', then it is at the
           start. */
        default: if(str==NULL) str=c;
        }

      /* Increment C. */
      ++c;
    }

  /* If the last element wasn't a comma, the last string hasn't been added
     to the list yet. */
  if(str) gal_list_str_add(&list, str, 1);

  /* Return the reversed list (the option parsing tools automatically
     reverse STRLLs at the end). */
  return list;
}





/* For options that can take multiple values multiple times:

    --option=val1,val2 --option=val3,val4,val5 ...

   We want to merge/append all the separate values into separate tokens of
   a single 'gal_list_str_t'.
 */
void *
gal_options_parse_csv_strings_append(struct argp_option *option, char *arg,
                                     char *filename, size_t lineno,
                                     void *junk)
{
  gal_list_str_t *olist, *alist;

  /* We want to print the values. */
  if(lineno==-1)
    return gal_list_str_cat(*(gal_list_str_t **)(option->value), ',');

  /* We want to read the values. */
  else
    {
      /* Read all the values given to this instance of the option. */
      alist=gal_options_parse_csv_strings_to_list(arg, filename, lineno);

      /* Get the output list and update it by putting the new list before
         it (note that the option parsing infrastructure will automatically
         reverse STRLLs. */
      olist=*(gal_list_str_t **)(option->value);
      gal_list_str_last(alist)->next=olist;
      *(gal_list_str_t **)(option->value)=alist;

      /* Return a NULL pointer. */
      return NULL;
    }
}





/* The input to this function is a string of any number of strings
   separated by a comma (',') for example: 'a,abc,abcd'. The output
   'gal_data_t' contains the array of given strings. You can read the
   number of inputs from its 'size' element. */
gal_data_t *
gal_options_parse_csv_strings_to_data(char *string, char *filename,
                                      size_t lineno)
{
  size_t i, num;
  gal_data_t *out;
  gal_list_str_t *list=NULL, *tstrll=NULL;


  /* The nature of the arrays/numbers read here is very small, so since
     'p->cp.minmapsize' might not have been read yet, we will set it to -1
     (largest size_t number), so the values are kept in memory. */
  int quietmmap=1;
  size_t minmapsize=-1;

  /* Extract the separate tokens in the CSV string. */
  list=gal_options_parse_csv_strings_to_list(string, filename, lineno);

  /* Allocate the output data structure and fill it up. */
  if(list)
    {
      i=num=gal_list_str_number(list);
      out=gal_data_alloc(NULL, GAL_TYPE_STRING, 1, &num, NULL, 0,
                         minmapsize, quietmmap, NULL, NULL, NULL);
      for(tstrll=list;tstrll!=NULL;tstrll=tstrll->next)
        ((char **)(out->array))[--i]=tstrll->v;
    }
  else
    {
      /* It is not possible to allocate a dataset with a size of 0 along
         any dimension (in C it's possible, but conceptually it isn't). So,
         we'll allocate space for one element, then free it. */
      i=1;
      out=gal_data_alloc(NULL, GAL_TYPE_STRING, 1, &i, NULL, 0,
                         minmapsize, quietmmap, NULL, NULL, NULL);
      out->size=out->dsize[0]=0;
      free(out->array);
      out->array=NULL;
    }


  /* Clean up and return. Note that we don't want to free the space of
     each string becuse it has been passed. */
  gal_list_str_free(list, 0);
  return out;
}





/* 'arg' is the value given to an option. It contains multiple strings
   separated by a comma (','). This function will parse 'arg' and make a
   'gal_data_t' array of strings from it. The output 'gal_data_t' will be
   put in 'option->value'. */
void *
gal_options_parse_csv_strings(struct argp_option *option, char *arg,
                              char *filename, size_t lineno, void *junk)
{
  size_t nc;
  char **strarr;
  int i, has_space=0;
  gal_data_t *values;
  char *str, sstr[GAL_OPTIONS_STATIC_MEM_FOR_VALUES];

  /* We want to print the stored values. */
  if(lineno==-1)
    {
      /* Set the pointer to the values dataset. */
      values = *(gal_data_t **)(option->value);

      /* See if there are any space characters in the final string. */
      strarr=values->array;
      for(i=0;i<values->size;++i)
        if(has_space==0) has_space=options_string_has_space(strarr[i]);

      /* If there is a space, the string must start wth quotation marks. */
      nc = has_space ? 1 : 0;
      if(has_space) {sstr[0]='"'; sstr[1]='\0';}

      /* Write each string into the output string. */
      for(i=0;i<values->size;++i)
        {
          if( nc > GAL_OPTIONS_STATIC_MEM_FOR_VALUES-100 )
            error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s "
                  "so we can address the problem. The number of necessary "
                  "characters in the statically allocated string has "
                  "become too close to %d", __func__, PACKAGE_BUGREPORT,
                  GAL_OPTIONS_STATIC_MEM_FOR_VALUES);
          nc += sprintf(sstr+nc, "%s,", strarr[i]);
        }

      /* If there was a space, we need a quotation mark at the end of the
         string. */
      if(has_space) { sstr[nc-1]='"'; sstr[nc]='\0'; }
      else            sstr[nc-1]='\0';

      /* Copy the string into a dynamically allocated space, because it
         will be freed later. */
      gal_checkset_allocate_copy(sstr, &str);
      return str;
    }

  /* We want to read the user's string. */
  else
    {
      /* If the option is already set, just return. */
      if(option->set) return NULL;

      /* Make sure an argument is actually given. */
      if(*arg=='\0')
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "no value "
                      "given to '--%s'", option->name);

      /* Read the values. */
      values=gal_options_parse_csv_strings_to_data(arg, filename, lineno);

      /* Put the values into the option and return NULL (the return value
         is only for cases where the user wants to see the values, not set
         them). */
      *(gal_data_t **)(option->value) = values;
      return NULL;
    }
}





/* Some options can be called multiple times on the command-line, but
   within the program, all the values must be merged into one. For example
   '--column=1 --column=2,3 --column=4,5,6'. In this example, the output
   'gal_list_str_t' will have 3 nodes, but we actually want a list that has
   6 nodes. */
void
gal_options_merge_list_of_csv(gal_list_str_t **list)
{
  size_t i;
  gal_data_t *strs;
  char *c, **strarr;
  gal_list_str_t *tmp, *in=*list, *out=NULL;

  /* Incase the input is NULL, this option doesn't need to do anything. */
  if(in==NULL) return;

  /* Go over each input and add it to the list. */
  for(tmp=in; tmp!=NULL; tmp=tmp->next)
    {
      /* Remove any possibly commented new-line where we have a backslash
         followed by a new-line character (replace the two characters with
         two single space characters). This can happen with options have
         have longer scripts and the user is forced to break the line with
         a '\' followed by newline. */
      for(c=tmp->v; *c!='\0'; ++c)
        if(*c=='\\' && *(c+1)=='\n') { *c=' '; *(++c)=' '; }

      /* Read the different comma-separated strings into an array (within a
         'gal_data_t'). */
      strs=gal_options_parse_csv_strings_to_data(tmp->v, NULL, 0);
      strarr=strs->array;

      /* Go through all the items and add the pointers to the output
         list. We won't re-allocate the string, we'll just set it to NULL
         in the array. */
      for(i=0;i<strs->size;++i)
        {
          gal_list_str_add(&out, strarr[i], 0);
          strarr[i]=NULL;
        }

      /* Clean up. */
      gal_data_free(strs);
    }

  /* Free the input list and reverse the output list to be in the same
     input order. */
  gal_list_str_free(in, 1);
  gal_list_str_reverse(&out);

  /* Reset the input pointer. */
  *list=out;
}





/* Parse the given string into a series of size values (integers, stored as
   an array of size_t). The output array will be stored in the 'value'
   element of the option. The last element of the array is
   'GAL_BLANK_SIZE_T' to allow finding the number of elements within it
   later (similar to a string which terminates with a '\0' element). */
void *
gal_options_parse_sizes_reverse(struct argp_option *option, char *arg,
                                char *filename, size_t lineno, void *junk)
{
  int i;
  double *v;
  gal_data_t *values;
  size_t nc, num, *array;
  char *str, sstr[GAL_OPTIONS_STATIC_MEM_FOR_VALUES];

  /* We want to print the stored values. */
  if(lineno==-1)
    {
      /* Find the number of elements within the array. */
      array = *(size_t **)(option->value);
      for(i=0; array[i]!=-1; ++i);
      num=i;

      /* Write all the dimensions into the static string. */
      nc=0;
      for(i=num-1;i>=0;--i)
        {
          if( nc > GAL_OPTIONS_STATIC_MEM_FOR_VALUES-100 )
            error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s "
                  "so we can address the problem. The number of necessary "
                  "characters in the statically allocated string has "
                  "become too close to %d", __func__, PACKAGE_BUGREPORT,
                  GAL_OPTIONS_STATIC_MEM_FOR_VALUES);
          nc += sprintf(sstr+nc, "%zu,", array[i]);
        }
      sstr[nc-1]='\0';

      /* Copy the string into a dynamically allocated space, because it
         will be freed later. */
      gal_checkset_allocate_copy(sstr, &str);
      return str;
    }

  /* We want to read the user's string. */
  else
    {
      /* If the option is already set, just return. */
      if(option->set) return NULL;

      /* Make sure an argument is actually given. */
      if(*arg=='\0')
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "no value "
                      "given to '--%s'", option->name);

      /* Read the values. */
      values=gal_options_parse_list_of_numbers(arg, filename, lineno,
                                               GAL_TYPE_FLOAT64);

      /* Check if the values are an integer. */
      v=values->array;
      for(i=0;i<values->size;++i)
        {
          if(v[i]<0)
            error_at_line(EXIT_FAILURE, 0, filename, lineno, "a given "
                          "value in '%s' (%g) is not 0 or positive. The "
                          "values to the '--%s' option must be positive",
                          arg, v[i], option->name);

          if(ceil(v[i]) != v[i])
            error_at_line(EXIT_FAILURE, 0, filename, lineno, "a given "
                          "value in '%s' (%g) is not an integer. The "
                          "values to the '--%s' option must be integers",
                          arg, v[i], option->name);
        }

      /* Write the values into an allocated size_t array and finish it with
         a '-1' so the total number can be found later. */
      num=values->size;
      array=gal_pointer_allocate(GAL_TYPE_SIZE_T, num+1, 0, __func__,
                                 "array");
      for(i=0;i<num;++i) array[num-1-i]=v[i];
      array[num] = GAL_BLANK_SIZE_T;

      /* Put the array of size_t into the option, clean up and return. */
      *(size_t **)(option->value) = array;
      gal_data_free(values);
      return NULL;
    }
}





/* Parse options with values of a list of numbers. */
void *
gal_options_parse_csv_float64(struct argp_option *option, char *arg,
                              char *filename, size_t lineno, void *junk)
{
  size_t i, nc;
  double *darray;
  gal_data_t *values;
  char *str, sstr[GAL_OPTIONS_STATIC_MEM_FOR_VALUES];

  /* We want to print the stored values. */
  if(lineno==-1)
    {
      /* Set the pointer to the values dataset. */
      values = *(gal_data_t **)(option->value);
      darray=values->array;

      /* Write each string into the output string. */
      nc=0;
      for(i=0;i<values->size;++i)
        {
          if( nc > GAL_OPTIONS_STATIC_MEM_FOR_VALUES-100 )
            error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s "
                  "so we can address the problem. The number of necessary "
                  "characters in the statically allocated string has "
                  "become too close to %d", __func__, PACKAGE_BUGREPORT,
                  GAL_OPTIONS_STATIC_MEM_FOR_VALUES);
          nc += sprintf(sstr+nc, "%g,", darray[i]);
        }
      sstr[nc-1]='\0';

      /* Copy the string into a dynamically allocated space, because it
         will be freed later. */
      gal_checkset_allocate_copy(sstr, &str);
      return str;
    }

  /* We want to read the user's string. */
  else
    {
      /* If the option is already set, just return. */
      if(option->set) return NULL;

      /* Make sure an argument is actually given. */
      if(*arg=='\0')
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "no value "
                      "given to '--%s'", option->name);

      /* Read the values. */
      values=gal_options_parse_list_of_numbers(arg, filename, lineno,
                                               GAL_TYPE_FLOAT64);

      /* Put the values into the option. */
      *(gal_data_t **)(option->value) = values;

      /* The return value is only for printing mode, so we can return
         NULL after reading is complete. */
      return NULL;
    }
}





/* Two numbers must be provided as an argument. This function will read
   them as the sigma-clipping multiple and parameter and store the two in a
   2-element array. 'option->value' must point to an already allocated
   2-element array of double type. */
void *
gal_options_read_sigma_clip(struct argp_option *option, char *arg,
                            char *filename, size_t lineno, void *junk)
{
  char *str;
  gal_data_t *in;
  double *sigmaclip=option->value;

  /* Caller wants to print the option values. */
  if(lineno==-1)
    {
      if( asprintf(&str, "%g,%g", sigmaclip[0], sigmaclip[1])<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
      return str;
    }

  /* Caller wants to read the values into memory, so parse the inputs. */
  in=gal_options_parse_list_of_numbers(arg, filename, lineno,
                                       GAL_TYPE_FLOAT64);

  /* Check if there was only two numbers. */
  if(in->size!=2)
    error_at_line(EXIT_FAILURE, 0, filename, lineno, "the '--%s' "
                  "option takes two values (separated by a comma) for "
                  "defining the sigma-clip. However, %zu numbers were "
                  "read in the string '%s' (value to this option).\n\n"
                  "The first number is the multiple of sigma, and the "
                  "second is either the tolerance (if its is less than "
                  "1.0), or a specific number of times to clip (if it "
                  "is equal or larger than 1.0).", option->name, in->size,
                  arg);

  /* Copy the sigma clip parameters into the space the caller has given (as
     the 'value' element of 'option'). */
  memcpy(option->value, in->array, 2*sizeof *sigmaclip);

  /* Multiple of sigma must be positive. */
  if( sigmaclip[0] <= 0 )
    error_at_line(EXIT_FAILURE, 0, filename, lineno, "the first value to "
                  "the '--%s' option (multiple of sigma), must be "
                  "greater than zero. From the string '%s' (value to "
                  "this option), you have given a value of %g for the "
                  "first value", option->name, arg, sigmaclip[0]);

  /* Second value must also be positive. */
  if( sigmaclip[1] <= 0 )
    error_at_line(EXIT_FAILURE, 0, filename, lineno, "the second value "
                  "to the '--%s' option (tolerance to stop clipping or "
                  "number of clips), must be greater than zero. From "
                  "the string '%s' (value to this option), you have "
                  "given a value of %g for the second value",
                  option->name, arg, sigmaclip[1]);

  /* if the second value is larger or equal to 1.0, it must be an
     integer. */
  if( sigmaclip[1] >= 1.0f && ceil(sigmaclip[1]) != sigmaclip[1])
    error_at_line(EXIT_FAILURE, 0, filename, lineno, "when the second "
                  "value to the '--%s' option is >=1, it is interpretted "
                  "as an absolute number of clips. So it must be an "
                  "integer. However, your second value is a floating "
                  "point number: %g (parsed from '%s')", option->name,
                  sigmaclip[1], arg);

  /* Clean up and return. */
  gal_data_free(in);
  return NULL;
}





/* Parse name and (string/float64) values:  name,value1,value2,value3,...

   The output is a 'gal_data_t', where the 'name' is the given name and the
   values are in its array (of 'char *' or 'float64' type). */
static void *
gal_options_parse_name_and_values(struct argp_option *option, char *arg,
                                  char *filename, size_t lineno, void *junk,
                                  int str0_f641_sz2)
{
  double *darray=NULL;
  size_t i, nc, *sizarr=NULL;
  gal_data_t *tmp, *existing, *dataset;
  char *c, *name, *values, **strarr=NULL;
  char *str, sstr[GAL_OPTIONS_STATIC_MEM_FOR_VALUES];

  /* We want to print the stored values. */
  if(lineno==-1)
    {
      /* Set the value pointer to 'existing'. */
      existing=*(gal_data_t **)(option->value);
      switch(str0_f641_sz2)
        {
        case 0: strarr = existing->array; break;
        case 1: darray = existing->array; break;
        case 2: sizarr = existing->array; break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! please contact us at '%s' "
                "to fix the problem. The code '%d' isn't acceptable for "
                "'str0_f641_si2'", __func__, PACKAGE_BUGREPORT,
                str0_f641_sz2);
        }

      /* First write the name. */
      nc=0;
      nc += sprintf(sstr+nc, "%s,", existing->name);

      /* Write the values into a string. */
      for(i=0;i<existing->size;++i)
        {
          if( nc > GAL_OPTIONS_STATIC_MEM_FOR_VALUES-100 )
            error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s "
                  "so we can address the problem. The number of "
                  "necessary characters in the statically allocated "
                  "string has become too close to %d", __func__,
                  PACKAGE_BUGREPORT, GAL_OPTIONS_STATIC_MEM_FOR_VALUES);
          switch(str0_f641_sz2)
            {
            case 0: nc += sprintf(sstr+nc, "%s,",  strarr[i]); break;
            case 1: nc += sprintf(sstr+nc, "%g,",  darray[i]); break;
            case 2: nc += sprintf(sstr+nc, "%zu,", sizarr[i]); break;
            } /* No default necessary: valid value confirmed above. */
        }

      /* Finish the string. */
      sstr[nc-1]='\0';

      /* Copy the string into a dynamically allocated space, because it
         will be freed later. */
      gal_checkset_allocate_copy(sstr, &str);
      return str;
    }
  else
    {
      /* Make sure an argument is actually given. */
      if(*arg=='\0')
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "no value "
                      "given to '--%s'", option->name);

      /* Parse until the comma or the end of the string. */
      c=arg; while(*c!='\0' && *c!=',') ++c;
      values = (*c=='\0') ? NULL : c+1;

      /* Name of the dataset (note that 'c' is already pointing the end of
         the 'name' and 'values' points to the next character). So we can
         safely set 'c' to '\0' to have the 'name'. */
      *c='\0';
      gal_checkset_allocate_copy(arg, &name);

      /* Read the values. */
      switch(str0_f641_sz2)
        {
        case 0:
          dataset=gal_options_parse_list_of_strings(values, filename,
                                                    lineno);
          break;
        case 1:
          dataset=gal_options_parse_list_of_numbers(values, filename,
                                                    lineno,
                                                    GAL_TYPE_FLOAT64);
          break;
        case 2:
          dataset=gal_options_parse_list_of_numbers(values, filename,
                                                    lineno,
                                                    GAL_TYPE_SIZE_T);
          break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! please contact us at '%s' "
                "to fix the problem. The code '%d' isn't acceptable for "
                "'str0_f641_si2'", __func__, PACKAGE_BUGREPORT,
                str0_f641_sz2);
        }

      /* If there actually was a string of numbers, add the dataset to the
         rest. */
      if(dataset)
        {
          dataset->name=name;

          /* Add the given dataset to the end of an existing dataset. */
          existing = *(gal_data_t **)(option->value);
          if(existing)
            {
              for(tmp=existing;tmp!=NULL;tmp=tmp->next)
                if(tmp->next==NULL) { tmp->next=dataset; break; }
            }
          else
            *(gal_data_t **)(option->value) = dataset;

          /* For a check.
             printf("arg: %s\n", arg);
             darray=dataset->array;
             for(i=0;i<dataset->size;++i) printf("%f\n", darray[i]);
             exit(0);
          */
        }
      else
        error(EXIT_FAILURE, 0, "'--%s' requires a series of %s "
              "(separated by ',' or ':') following its first argument, "
              "please run with '--help' for more information",
              option->name,
              ( str0_f641_sz2
                ? (str0_f641_sz2==1?"numbers":"integers")
                : "strings" ));

      /* Our job is done, return NULL. */
      return NULL;
    }
}





void *
gal_options_parse_name_and_strings(struct argp_option *option, char *arg,
                                   char *filename, size_t lineno,
                                   void *junk)
{
  return gal_options_parse_name_and_values(option, arg, filename, lineno,
                                           junk, 0);
}





void *
gal_options_parse_name_and_float64s(struct argp_option *option, char *arg,
                                    char *filename, size_t lineno,
                                    void *junk)
{
  return gal_options_parse_name_and_values(option, arg, filename, lineno,
                                           junk, 1);
}





void *
gal_options_parse_name_and_sizets(struct argp_option *option, char *arg,
                                  char *filename, size_t lineno,
                                  void *junk)
{
  return gal_options_parse_name_and_values(option, arg, filename, lineno,
                                           junk, 2);
}





/* Parse strings like this: 'num1,num2:num3,n4:num5,num6' and return it as
   a data container array: all elements are simply placed after each other
   in the array. */
gal_data_t *
gal_options_parse_colon_sep_csv_raw(char *instring, char *filename,
                                    size_t lineno)
{
  gal_data_t *out;
  size_t dim=0, size;
  char *pt, *tailptr;
  double read, sread, *array;
  gal_list_f64_t *vertices=NULL;

  /* Parse the string. */
  pt=instring;
  while(*pt!='\0')
    {
      switch(*pt)
        {
        case ',':
          ++dim;
          if(dim==2)
            error_at_line(EXIT_FAILURE, 0, filename, lineno,
                          "Extra ',' in '%s'", instring);
          ++pt;
          break;
        case ':':
          if(dim==0)
            error_at_line(EXIT_FAILURE, 0, filename, lineno,
                          "not enough coordinates for at least "
                          "one polygon vertex (in '%s')", instring);
          dim=0;
          ++pt;
          break;
        default:
          break;
        }

      /* strtod will skip white spaces if they are before a number,
         but not when they are before a : or ,. So we need to remove
         all white spaces. White spaces are usually put beside each
         other, so if one is encountered, go along the string until
         the white space characters finish. */
      if(isspace(*pt))
        ++pt;
      else
        {
          /* Read the number: */
          read=strtod(pt, &tailptr);

          /* Check if there actually was a number.
          printf("\n\n------\n%zu: %f (%s)\n", dim, read, tailptr);
          */

          /* Make sure if a number was read at all? */
          if(tailptr==pt) /* No number was read! */
            error_at_line(EXIT_FAILURE, 0, filename, lineno,
                          "%s could not be parsed as a floating point "
                          "number", tailptr);

          /* Check if there are no extra characters in the number, for
             example we don't have a case like '1.00132.17', or
             1.01i:2.0. Such errors are not uncommon when typing large
             numbers, and if ignored, they can lead to unpredictable
             results, so its best to abort and inform the user. */
          if( *tailptr!='\0'
              && !isspace(*tailptr)
              && strchr(":,hd", *tailptr)==NULL )
            error_at_line(EXIT_FAILURE, 0, filename, lineno,
                          "'%s' is an invalid floating point number "
                          "sequence in the value to the '--polygon' "
                          "option, error detected at '%s'", pt, tailptr);

          /* Here we need to check if we are dealing with a sexagesimal
             string, or just a normal number. If its just a normal
             number. Note that 'sread' must be initialized to NaN in case
             'tailptr' is either ',' or '\0'. */
          sread=NAN;
          if(*tailptr!=',' && *tailptr!='\0')
            sread=gal_options_read_sexagesimal(dim, pt, &tailptr, 0);
          if(!isnan(sread)) read=sread;

          /* Add the read coordinate to the list of coordinates. */
          gal_list_f64_add(&vertices, read);

          /* The job here is done, start from tailptr. */
          pt=tailptr;
        }
    }

  /* Convert the list to an array, put it in a data structure, clean up and
     return. */
  array=gal_list_f64_to_array(vertices, 1, &size);
  out=gal_data_alloc(array, GAL_TYPE_FLOAT64, 1, &size, NULL, 0, -1, 1,
                     NULL, NULL, NULL);
  gal_list_f64_free(vertices);
  return out;
}





/* Parse strings that are given to a function in this format
   'num1,num2:num3,n4:num5,num6'. */
void *
gal_options_parse_colon_sep_csv(struct argp_option *option, char *arg,
                                char *filename, size_t lineno, void *junk)
{
  double *darray;
  size_t i, nc, size;
  gal_data_t *tmp, *dataset, *existing;
  char *str, sstr[GAL_OPTIONS_STATIC_MEM_FOR_VALUES];

  /* We want to print the stored values. */
  if(lineno==-1)
    {
      /* Set the value pointer to 'existing'. */
      existing=*(gal_data_t **)(option->value);
      darray=existing->array;

      /* Start printing the values. */
      nc=0;
      size=existing->size;
      for(i=0;i<size;i+=2)
        {
          /* Make sure we aren't passing the allocated space. */
          if( nc > GAL_OPTIONS_STATIC_MEM_FOR_VALUES-100 )
            error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s "
                  "so we can address the problem. The number of necessary "
                  "characters in the statically allocated string has "
                  "become too close to %d", __func__, PACKAGE_BUGREPORT,
                  GAL_OPTIONS_STATIC_MEM_FOR_VALUES);

          /* Print the two values in the expected format. */
          nc += sprintf(sstr+nc, "%.6f,%.6f%s", darray[i], darray[i+1],
                        (i==(size-2) ? "" : ":") );
        }

      /* Finish the string. */
      sstr[nc-1]='\0';

      /* Copy the string into a dynamically allocated space, because it
         will be freed later. */
      gal_checkset_allocate_copy(sstr, &str);
      return str;
    }
  else
    {
      /* Make sure an argument is actually given. */
      if(*arg=='\0')
        error_at_line(EXIT_FAILURE, 0, filename, lineno, "no value "
                      "given to '--%s'", option->name);

      /* Check if the argument is a string (it contains a ':' or a ',') or
         a filename. Then parse the desired format and put it in this
         option's pointer. */
      if( strchr(arg, ',')==NULL && strchr(arg, ':')==NULL )
        dataset=gal_ds9_reg_read_polygon(arg);
      else
        dataset=gal_options_parse_colon_sep_csv_raw(arg, filename, lineno);

      /* Add the given dataset to the end of an existing dataset. */
      existing = *(gal_data_t **)(option->value);
      if(existing)
        {
          for(tmp=existing;tmp!=NULL;tmp=tmp->next)
            if(tmp->next==NULL) { tmp->next=dataset; break; }
        }
      else
        *(gal_data_t **)(option->value) = dataset;

      /* In this scenario, there is no NULL value. */
      return NULL;
    }
}
















/**********************************************************************/
/************              Option actions               ***************/
/**********************************************************************/
/* The option value has been read and put into the 'value' field of the
   'argp_option' structure. This function will use the 'range' field to
   define a check and abort with an error if the value is not in the given
   range. It also takes the 'arg' so it can be used for good error message
   (showing the value that could not be read). */
static void
options_sanity_check(struct argp_option *option, char *arg,
                     char *filename, size_t lineno)
{
  size_t dsize=1;
  char *message=NULL;
  int mcflag=GAL_ARITHMETIC_FLAGS_BASIC;
  int operator1=GAL_ARITHMETIC_OP_INVALID;
  int operator2=GAL_ARITHMETIC_OP_INVALID;
  int multicheckop=GAL_ARITHMETIC_OP_INVALID;
  gal_data_t *value, *ref1=NULL, *ref2=NULL, *check1, *check2;

  /* Currently, this function is only for numeric types, so if the value is
     string type, or its 'range' field is 'GAL_OPTIONS_RANGE_ANY', then
     just return without any checks. */
  if( option->type==GAL_TYPE_STRING
      || option->type==GAL_TYPE_STRLL
      || option->range==GAL_OPTIONS_RANGE_ANY )
    return;

  /* Put the option value into a data structure. */
  value=gal_data_alloc(option->value, option->type, 1, &dsize, NULL,
                       0, -1, 1, NULL, NULL, NULL);

  /* Set the operator(s) and operands: */
  switch(option->range)
    {

    case GAL_OPTIONS_RANGE_GT_0:
      message="greater than zero";
      ref1=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      *(unsigned char *)(ref1->array)=0;
      operator1=GAL_ARITHMETIC_OP_GT;
      ref2=NULL;
      break;


    case GAL_OPTIONS_RANGE_GE_0:
      message="greater or equal to zero";
      ref1=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      *(unsigned char *)(ref1->array)=0;
      operator1=GAL_ARITHMETIC_OP_GE;
      ref2=NULL;
      break;


    case GAL_OPTIONS_RANGE_0_OR_1:
      message="either 0 or 1";
      ref1=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      ref2=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      *(unsigned char *)(ref1->array)=0;
      *(unsigned char *)(ref2->array)=1;

      operator1=GAL_ARITHMETIC_OP_EQ;
      operator2=GAL_ARITHMETIC_OP_EQ;
      multicheckop=GAL_ARITHMETIC_OP_OR;
      break;


    case GAL_OPTIONS_RANGE_GE_0_LE_1:
      message="between zero and one (inclusive)";
      ref1=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      ref2=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      *(unsigned char *)(ref1->array)=0;
      *(unsigned char *)(ref2->array)=1;

      operator1=GAL_ARITHMETIC_OP_GE;
      operator2=GAL_ARITHMETIC_OP_LE;
      multicheckop=GAL_ARITHMETIC_OP_AND;
      break;


    case GAL_OPTIONS_RANGE_GE_0_LT_1:
      message="between zero (inclusive) and one (exclusive)";
      ref1=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      ref2=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      *(unsigned char *)(ref1->array)=0;
      *(unsigned char *)(ref2->array)=1;

      operator1=GAL_ARITHMETIC_OP_GE;
      operator2=GAL_ARITHMETIC_OP_LT;
      multicheckop=GAL_ARITHMETIC_OP_AND;
      break;


    case GAL_OPTIONS_RANGE_GT_0_LT_1:
      message="between zero and one (not inclusive)";
      ref1=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      ref2=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      *(unsigned char *)(ref1->array)=0;
      *(unsigned char *)(ref2->array)=1;

      operator1=GAL_ARITHMETIC_OP_GT;
      operator2=GAL_ARITHMETIC_OP_LT;
      multicheckop=GAL_ARITHMETIC_OP_AND;
      break;


    case GAL_OPTIONS_RANGE_GT_0_ODD:
      message="greater than zero and odd";
      ref1=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      ref2=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      *(unsigned char *)(ref1->array)=0;
      *(unsigned char *)(ref2->array)=2;

      operator1=GAL_ARITHMETIC_OP_GT;
      operator2=GAL_ARITHMETIC_OP_MODULO;
      multicheckop=GAL_ARITHMETIC_OP_AND;
      break;

    case GAL_OPTIONS_RANGE_0_OR_ODD:
      message="greater than, or equal to, zero and odd";
      ref1=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      ref2=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &dsize, NULL,
                          0, -1, 1, NULL, NULL, NULL);
      *(unsigned char *)(ref1->array)=0;
      *(unsigned char *)(ref2->array)=2;

      operator1=GAL_ARITHMETIC_OP_EQ;
      operator2=GAL_ARITHMETIC_OP_MODULO;
      multicheckop=GAL_ARITHMETIC_OP_OR;
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: range code %d not recognized",
            __func__, option->range);
    }


  /* Use the arithmetic library to check for the condition. We don't want
     to free the value or change its value, so when dealing with the value
     directly, we won't use the 'GAL_ARITHMETIC_FREE', or
     'GAL_ARITHMETIC_INPLACE' flags. But we will do this when there are
     multiple checks so from the two check data structures, we only have
     one remaining. */
  check1=gal_arithmetic(operator1, 1, GAL_ARITHMETIC_FLAG_NUMOK,
                        value, ref1);
  if(ref2)
    {
      check2=gal_arithmetic(operator2, 1, GAL_ARITHMETIC_FLAG_NUMOK,
                            value, ref2);
      check1=gal_arithmetic(multicheckop, 1, mcflag, check1, check2);
    }


  /* If the final check is not successful, then print an error. */
  if( *(unsigned char *)(check1->array)==0 )
    error_at_line(EXIT_FAILURE, 0, filename, lineno,
                  "value to option '%s' must be %s, but the given value "
                  "is '%s'. Recall that '%s' is '%s'", option->name,
                  message, arg, option->name, option->doc);


  /* Clean up and finish. Note that we used the actual value pointer in the
     data structure, so first we need to set it to NULL, so 'gal_data_free'
     doesn't free it, we need it for later (for example to print the option
     values). */
  value->array=NULL;
  gal_data_free(ref1);
  gal_data_free(ref2);
  gal_data_free(value);
  gal_data_free(check1);
}





static void
gal_options_read_check(struct argp_option *option, char *arg,
                       char *filename, size_t lineno,
                       struct gal_options_common_params *cp)
{
  void *topass;

  /* If a function is defined, leave everything to the function. */
  if(option->func)
    {
      /* For the functions that are defined here (for all programs) and
         need the last pointer, we must pass the 'cp' pointer. For the
         rest, we must pass the 'cp->program_struct'. */
      switch(option->key)
        {
        case GAL_OPTIONS_KEY_CITE:
        case GAL_OPTIONS_KEY_CONFIG:
          topass=cp;
          break;
        default:
          topass=cp->program_struct;
        }

      /* Call the function to parse the value, flag the option as set and
         return (except for the '--config' option, which must always be
         unset). */
      option->func(option, arg, filename, lineno, topass);
      if(option->key!=GAL_OPTIONS_KEY_CONFIG) option->set=GAL_OPTIONS_SET;

      /* The '--config' option is printed for '--checkconfig' by its
         function ('gal_options_call_parse_config_file'), so must be
         ignored here. */
      if(cp->checkconfig && option->key!=GAL_OPTIONS_KEY_CONFIG)
        printf("  %-25s%s\n", option->name, arg?arg:"ACTIVATED");
      return;
    }


  /* Check if an argument is actually given (only options given on the
     command-line can have a NULL arg value). */
  if(arg)
    {
      if(option->type==GAL_TYPE_STRLL)
        gal_list_str_add(option->value, arg, 1);
      else
        {
          /* If the option is already set, ignore the given value. */
          if(option->set==GAL_OPTIONS_SET)
            {
              if(cp->checkconfig)
                printf("  %-25s--ALREADY-SET--\n", option->name);
              return;
            }

          /* Read the string argument into the value. */
          if( gal_type_from_string(&option->value, arg, option->type) )
            /* Fortunately 'error_at_line' will behave like 'error' when
               the filename is NULL (the option was read from a
               command-line). */
            error_at_line(EXIT_FAILURE, 0, filename, lineno,
                          "'%s' (value to option '--%s') couldn't be read "
                          "into the proper numerical type. Common causes "
                          "for this error are:\n"
                          "  - It contains non-numerical characters.\n"
                          "  - It is negative, but the expected value is "
                          "positive.\n"
                          "  - It is floating point, but the expected "
                          "value is an integer.\n"
                          "  - The previous option required a value, but "
                          "you forgot to give it one, so the next option's "
                          "name(+value, if there are no spaces between "
                          "them) is read as the value of the previous "
                          "option.", arg, option->name);

          /* Do a sanity check on the value. */
          options_sanity_check(option, arg, filename, lineno);
        }
    }
  else
    {
      /* If the option is already set, ignore the given value. */
      if(option->set==GAL_OPTIONS_SET)
        {
          if(cp->checkconfig)
            printf("  %-25s--ALREADY-SET--\n", option->name);
          return;
        }

      /* Make sure the option has the type set for options with no
         argument. So, give it a value of 1. */
      if(option->type==GAL_OPTIONS_NO_ARG_TYPE)
        *(uint8_t *)(option->value)=1;
      else
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
              "correct it. Options with no arguments, must have "
              "type '%s'. However, the '%s' option has type %s",
              __func__, PACKAGE_BUGREPORT,
              gal_type_name(GAL_OPTIONS_NO_ARG_TYPE, 1),
              option->name, gal_type_name(option->type, 1));
    }


  /* If the user wanted to check the value. */
  if(cp->checkconfig)
    printf("  %-25s%s\n", option->name,
           (arg && option->type!=GAL_OPTIONS_NO_ARG_TYPE)?arg:"ACTIVATED");


  /* Flip the 'set' flag to 'GAL_OPTIONS_SET'. */
  option->set=GAL_OPTIONS_SET;
}



















/**********************************************************************/
/************            Command-line options           ***************/
/**********************************************************************/
/* Set the value given to the command-line, where we have the integer 'key'
   of the option, not its long name as a string. */
error_t
gal_options_set_from_key(int key, char *arg, struct argp_option *options,
                         struct gal_options_common_params *cp)
{
  size_t i;

  /* Go through all the options and find the one that should keep this
     value, then put its value into the appropriate key. Note that the
     options array finishs with an all zero element, so we don't need to
     know the number before hand. */
  for(i=0;1;++i)
    {
      /* Check if the key corresponds to this option. */
      if( options[i].key==key )
        {
          /* It may happen that the user puts a space after the '=' sign of
             a long option (for example '--hdu= 1'). In this case, Argp
             will give an empty string to the option and associate the
             value as a new argument, leading to a confusing error message
             that only one argument should be given (by programs that need
             a single argument). So we should check for this condition
             here. */
          if(arg && arg[0]=='\0' && cp->quiet==0)
            error(EXIT_SUCCESS, 0, "WARNING: no value given to the "
                  "'--%s' option. In other words, its value is an "
                  "empty string. This may result in undefined behavior "
                  "(usually a crash in an un-expected part of the "
                  "program). It can happen when you use an undefined "
                  "shell variable or if there is an empty space after "
                  "the '=' sign of long options (for example '--hdu= 1', "
                  "note the space between the '=' and the '1'; the "
                  "correct format in such cases is either '--hdu=1' "
                  "or '--hdu 1'). To suppress this warning, please use "
                  "the '--quiet' (or '-q') option before this option",
                  options[i].name);

          /* When options are read from keys (by this function), they are
             read from the command-line. On the command-line, the last
             invokation of the option is important. Especially in contexts
             like scripts, this is important because you can change a given
             command-line option (that is not a linked list) by calling it
             a second time, instead of going back and changing the first
             value.

             As a result, only when searching for options on the
             command-line, a second value to the same option will replace
             the first one. This will not happen in configuration files. */
          if(options[i].set && gal_type_is_list(options[i].type)==0)
            options[i].set=GAL_OPTIONS_NOT_SET;

          /* Parse the value. */
          gal_options_read_check(&options[i], arg, NULL, 0, cp);

          /* We have found and set the value given to this option, so just
             return success (an error_t of 0 means success). */
          return 0;
        }
      else
        {
          /* The last option has all its values set to zero. */
          if(gal_options_is_last(&options[i]))
            return ARGP_ERR_UNKNOWN;
        }
    }
}





error_t
gal_options_common_argp_parse(int key, char *arg, struct argp_state *state)
{
  struct gal_options_common_params *cp=state->input;

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

  /* Read the options. */
  return gal_options_set_from_key(key, arg, cp->coptions, cp);
}






char *
gal_options_stdin_error(long stdintimeout, int precedence, char *name)
{
  char *out;

  if( asprintf(&out, "no %s!\n\n"
               "The %s can be read from a file (specified as an argument), "
               "or the standard input.%s Standard input can come from a "
               "pipe (output of another program) or typed on the "
               "command-line before %ld micro-seconds (configurable with "
               "the '--stdintimeout' option).", name, name,
               ( precedence
                 ? " If both are provided, a file takes precedence."
                 : "" ), stdintimeout )<0 )
    error(EXIT_FAILURE, 0, "%s: 'asprintf' allocation error", __func__);

  return out;
}





/* Make the notice that is printed before program terminates, when no input
   is given and Standard input is also available. */
gal_list_str_t *
gal_options_check_stdin(char *inputname, long stdintimeout, char *name)
{
  gal_list_str_t *lines=inputname ? NULL : gal_txt_stdin_read(stdintimeout);

  /* See if atleast one of the two inputs is given. */
  if(inputname==NULL && lines==NULL)
    error( EXIT_FAILURE, 0, "%s", gal_options_stdin_error(stdintimeout,
                                                          1, name));

  /* Return the output. */
  return lines;
}




















/**********************************************************************/
/************            Configuration files            ***************/
/**********************************************************************/

/* Read the option and the argument from the line and return. */
static void
options_read_name_arg(char *line, char *filename, size_t lineno,
                      char **name, char **arg)
{
  int notyetfinished=1, inword=0, inquote=0;

  /* Initialize name and value: */
  *arg=NULL;
  *name=NULL;

  /* Go through the characters and set the values: */
  do
    switch(*line)
      {
      case ' ': case '=': case '\t': case '\v': case '\n': case '\r':
        if(inword) /* Only considered in a word, not in a quote. */
          {
            inword=0;
            *line='\0';
            if(*arg && inquote==0)
              notyetfinished=0;
          }
        break;
      case '#':
        notyetfinished=0;
        break;
      case '"':
        if(inword)
          error_at_line(EXIT_FAILURE, 0, filename, lineno,
                        "Quotes have to be surrounded by whitespace "
                        "characters (space, tab, new line, etc).");
        if(inquote)
          {
            *line='\0';
            inquote=0;
            notyetfinished=0;
          }
        else
          {
            if(*name==NULL)
              error_at_line(EXIT_FAILURE, 0, filename, lineno,
                            "option name should not start with "
                            "double quotes (\").");
            inquote=1;
            *arg=line+1;
          }
        break;
      default:
        if(inword==0 && inquote==0)
          {
            if(*name==NULL)
              *name=line;
            else  /* *name is set, now assign *arg. */
              *arg=line;
            inword=1;
          }
        break;
      }
  while(*(++line)!='\0' && notyetfinished);

  /* In the last line of the file, there is no new line to be
     converted to a '\0' character! So if value has been assigned, we
     are not in a quote and the line has finished, it means the given
     value has also finished. */
  if(*line=='\0' && *arg && inquote==0)
    notyetfinished=0;

  /* This was a blank line: */
  if(*name==NULL && *arg==NULL)
    return;

  /* Name or value were set but not yet finished. */
  if(notyetfinished)
    error_at_line(EXIT_FAILURE, 0, filename, lineno,
                  "line finished before option name and value could "
                  "be read.");
}





static int
options_set_from_name(char *name, char *arg,  struct argp_option *options,
                      struct gal_options_common_params *cp, char *filename,
                      size_t lineno)
{
  size_t i;

  /* Go through all the options and find the one that should keep this
     value, then put its value into the appropriate key. Note that the
     options array finishs with an all zero element, so we don't need to
     know the number before hand. */
  for(i=0;1;++i)
    {
      /* Check if the key corresponds to this option. */
      if( gal_checkset_noprefix_isequal(name, cp->configprefix,
                                        options[i].name) )
        {
          /* Ignore this option and its value. This can happen in several
             situations:

               - Not all common options are used by all programs. When a
                 program doesn't use an option, it will be given an
                 'OPTION_HIDDEN' flag. There is no point in reading the
                 values of such options.

               - When the option already has a value AND it ISN'T a linked
                 list. */
          if( options[i].flags==OPTION_HIDDEN
              || ( options[i].set
                   && !gal_type_is_list(options[i].type ) ) )
            {
              if(cp->checkconfig)
                printf("  %-25s%s\n", name, ( options[i].flags==OPTION_HIDDEN
                                              ? "--IGNORED--"
                                              : "--ALREADY-SET--" ) );
              return 0;
            }

          /* Read the value into the option and do a sanity check. */
          gal_options_read_check(&options[i], arg, filename, lineno, cp);

          /* We have found and set the value given to this option, so just
             return success (an error_t of 0 means success). */
          return 0;
        }
      else
        {
          /* The last option has all its values set to zero. If we get to
             this point then the given name was not recognized and this
             function will return a 1. */
          if(gal_options_is_last(&options[i]))
            return 1;
        }
    }
}





/* If the last config option has a value which is 1, then in some previous
   configuration file the user has asked to stop parsing configuration
   files. In that case, don't read this configuration file. */
static int
options_lastconfig_has_been_called(struct argp_option *coptions)
{
  size_t i;

  for(i=0; !gal_options_is_last(&coptions[i]); ++i)
    if( coptions[i].key == GAL_OPTIONS_KEY_LASTCONFIG
        && coptions[i].set
        && *((unsigned char *)(coptions[i].value)) )
      return 1;
  return 0;
}





static void
options_parse_file(char *filename,  struct gal_options_common_params *cp,
                   int warning)
{
  FILE *fp;
  char *line, *name, *arg;
  size_t linelen=10, lineno=0;


  /* If 'lastconfig' was called prior to this file, then just return and
     ignore this configuration file. */
  if( options_lastconfig_has_been_called(cp->coptions) )
    return;


  /* Open the file. If the file doesn't exist or can't be opened, then just
     ignore the configuration file and return. */
  errno=0;
  fp=fopen(filename, "r");
  if(fp==NULL)
    {
      /* Print a warning. */
      if(warning && cp->quiet==0)
        error(EXIT_SUCCESS, errno, "%s", filename);
      return;
    }


  /* If necessary, print the configuration file name. */
  if(cp->checkconfig)
    printf("%s:\n", filename);


  /* Allocate the space necessary to keep a copy of each line as we parse
     it. Note that 'getline' is going to later 'realloc' this space to fit
     the line length. */
  errno=0;
  line=malloc(linelen*sizeof *line);
  if(line==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for 'line'",
          __func__, linelen*sizeof *line);


  /* Read the parameters line by line. */
  while( getline(&line, &linelen, fp) != -1 )
    {
      ++lineno;
      if( gal_txt_line_stat(line) == GAL_TXT_LINESTAT_DATAROW )
        {
          /* Get the option name and argument/value. */
          options_read_name_arg(line, filename, lineno, &name, &arg);

          /* First look into this program's options, if the option isn't
             found there, 'options_set_from_name' will return 1. So the
             condition will succeed and we will start looking into the
             common options, if it isn't found there either, then report an
             error. */
          if( options_set_from_name(name, arg, cp->poptions, cp,
                                    filename, lineno) )
            if( options_set_from_name(name, arg, cp->coptions, cp,
                                      filename, lineno) )
              error_at_line(EXIT_FAILURE, 0, filename, lineno,
                            "unrecognized option '%s', for the full list "
                            "of options, please run with '--help'", name);
        }
    }


  /* Close the file. */
  errno=0;
  if(fclose(fp))
    error(EXIT_FAILURE, errno, "%s: couldn't close after reading as "
          "a configuration file in %s", filename, __func__);

  /* Clean up and return. */
  free(line);
}




/* This function will be used when the '--config' option is called. */
void *
gal_options_call_parse_config_file(struct argp_option *option, char *arg,
                                   char *filename, size_t lineno, void *c)
{
  struct gal_options_common_params *cp=(struct gal_options_common_params *)c;

  /* The '--config' option is a special function when it comes to
     '--checkconfig': we'll have to write its value before interpretting
     it. */
  if(cp->checkconfig)
    {
      printf("  %-25s%s\n", option->name, arg);
      printf("............................\n");
    }

  /* Call the confguration file parser. */
  options_parse_file(arg, cp, 1);

  /* Ending boundary of this file's options. */
  if(cp->checkconfig)
      printf("............................\n");

  /* Just to avoid compiler warnings, then return, note that all pointers
     are just copies. */
  option=NULL; filename=NULL; lineno=0;
  return NULL;
}





/* Read the configuration files and put the values of the options not given
   into it. The directories containing the configuration files are fixed
   for all the programs.

    - 'SYSCONFIG_DIR' is passed onto the library functions at compile time
      from the command-line. You can search for it in the outputs of
      'make'. The main reason is that we want the the user still has the
      chance to change the installation directory after 'configure'.

    - 'USERCONFIG_DIR' is defined in 'config.h'.

    - 'CURDIRCONFIG_DIR' is defined in 'config.h'. */
static void
gal_options_parse_config_files(struct gal_options_common_params *cp)
{
  char *home;
  char *filename;

  /* A small sanity check because in multiple places, we have assumed the
     on/off options have a type of 'unsigned char'. */
  if(GAL_OPTIONS_NO_ARG_TYPE != GAL_TYPE_UINT8)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s so we can fix "
          "the problem. 'GAL_OPTIONS_NO_ARG_TYPE' must be the "
          "'uint8' type", __func__, PACKAGE_BUGREPORT);

  /* The program's current directory configuration file. */
  if( asprintf(&filename, ".%s/%s.conf", PACKAGE, cp->program_exec)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  options_parse_file(filename, cp, 0);
  free(filename);

  /* Common options configuration file. */
  if( asprintf(&filename, ".%s/%s.conf", PACKAGE, PACKAGE)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  options_parse_file(filename, cp, 0);
  free(filename);

  /* Read the home environment variable. */
  home=options_get_home();

  /* The program's user-wide configuration file. */
  if( asprintf(&filename, "%s/%s/%s.conf", home, USERCONFIG_DIR,
               cp->program_exec)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  options_parse_file(filename, cp, 0);
  free(filename);

  /* Common options user-wide configuration file. */
  if( asprintf(&filename, "%s/%s/%s.conf", home, USERCONFIG_DIR, PACKAGE)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  options_parse_file(filename, cp, 0);
  free(filename);

  /* The program's system-wide configuration file. */
  if( asprintf(&filename, "%s/%s.conf", SYSCONFIG_DIR, cp->program_exec)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  options_parse_file(filename, cp, 0);
  free(filename);

  /* Common options system-wide configuration file. */
  if( asprintf(&filename, "%s/%s.conf", SYSCONFIG_DIR, PACKAGE)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  options_parse_file(filename, cp, 0);
  free(filename);
}





static void
options_reverse_lists_check_mandatory(struct gal_options_common_params *cp,
                                      struct argp_option *options)
{
  size_t i;

  for(i=0; !gal_options_is_last(&options[i]); ++i)
    {
      if(options[i].set)
        switch(options[i].type)
          {
          case GAL_TYPE_STRLL:
            gal_list_str_reverse( (gal_list_str_t **)(options[i].value) );
            break;
          }
      else if(options[i].mandatory==GAL_OPTIONS_MANDATORY)
        gal_options_add_to_not_given(cp, &options[i]);
    }
}





void
gal_options_read_low_level_checks(struct gal_options_common_params *cp)
{
  size_t suggested_mmap=10000000;

  /* If 'numthreads' is 0, use the number of threads available to the
     system. */
  if(cp->numthreads==0)
    cp->numthreads=gal_threads_number();

  /* If 'minmapsize==0' and quiet isn't given, print a warning. */
  if(cp->minmapsize==0 && cp->quiet==0)
    {
      fprintf(stderr, "\n\n"
              "========= WARNING =========\n"
              "Minimum size to map an allocated space outside of RAM is "
              "not set, or set to zero. This can greatly slow down the "
              "processing of a program or cause strange crashes (recall "
              "that the number of files that can be memory-mapped is "
              "limited).\n\n"

              "On modern systems (with RAM larger than a giga-byte), it "
              "should be fine to set it to %zu (10 million bytes or 10Mb) "
              "with the command below. In this manner, only arrays that "
              "are larger than this will be memory-mapped and smaller "
              "arrays (which are much more numerous) will be allocated and "
              "freed in the RAM.\n\n"

              "     --minmapsize=%zu\n\n"

              "[This warning can be disabled with the '--quiet' (or '-q') "
              "option.]\n"
              "===========================\n\n", suggested_mmap,
              suggested_mmap);
    }

  /* If the user wanted to check the parsing of configuration files, then
     the program must stop here. */
  if(cp->checkconfig) exit(0);
}





/* Read all configuration files and set common options. */
void
gal_options_read_config_set(struct gal_options_common_params *cp)
{
  /* Parse all the configuration files. */
  gal_options_parse_config_files(cp);

  /* Reverse the order of all linked list type options so the popping order
     is the same as the user's input order. We need to do this here because
     when printing those options, their order matters. */
  options_reverse_lists_check_mandatory(cp, cp->poptions);
  options_reverse_lists_check_mandatory(cp, cp->coptions);

  /* Abort if any of the mandatory options are not set. */
  gal_options_abort_if_mandatory_missing(cp);

  /* Low-level/basic checks before passing control back to program. */
  gal_options_read_low_level_checks(cp);
}




















/**********************************************************************/
/************              Printing/Writing             ***************/
/**********************************************************************/
/* We don't want to print the values of configuration specific options and
   the output option. The output value is assumed to be specific to each
   input, and the configuration options are for reading the configuration,
   not writing it. */
static int
option_is_printable(struct argp_option *option)
{
  /* Use non-key fields:

       - If option is hidden (not relevant to this program).

       - Options with an INVALID type are not to be printed (they are
         probably processed to a higher level value with functions). */
  if( (option->flags & OPTION_HIDDEN)
      || option->type==GAL_TYPE_INVALID )
    return 0;

  /* Then check if it is a pre-program option. */
  switch(option->key)
    {
    case GAL_OPTIONS_KEY_OUTPUT:
    case GAL_OPTIONS_KEY_CITE:
    case GAL_OPTIONS_KEY_PRINTPARAMS:
    case GAL_OPTIONS_KEY_CONFIG:
    case GAL_OPTIONS_KEY_SETDIRCONF:
    case GAL_OPTIONS_KEY_SETUSRCONF:
    case GAL_OPTIONS_KEY_LASTCONFIG:
      return 0;
    }

  /* Everything is fine, print the option. */
  return 1;
}





/* For a given type, print the value in 'ptr' in a space of 'width'
   elements. If 'width==0', then return the width necessary to print the
   value. */
static int
options_print_any_type(struct argp_option *option, void *ptr, int type,
                       int width, FILE *fp,
                       struct gal_options_common_params *cp)
{
  char *str;

  /* Write the value into a string. */
  str = ( option->func
          ? option->func(option, NULL, NULL, (size_t)(-1), cp->program_struct)
          : gal_type_to_string(ptr, type, 1) );

  /* If only the width was desired, don't actually print the string, just
     return its length. Otherwise, print it. */
  if(width)
    fprintf(fp, "%-*s ", width, str);
  else
    width=strlen(str);

  /* Free the allocated space and return. */
  free(str);
  return width;
}





/* An option structure is given, return its name and value print
   lengths. */
static void
options_correct_max_lengths(struct argp_option *option, int *max_nlen,
                            int *max_vlen,
                            struct gal_options_common_params *cp)
{
  int vlen;
  gal_list_str_t *tmp;

  /* Invalid types are set for functions that don't save the raw user
     input, but do higher-level analysis on them for storing. */
  if(option->type==GAL_TYPE_INVALID) return;

  /* Get the length of the value and save its length length if its
     larger than the widest value. */
  if(gal_type_is_list(option->type))
    {
      /* A small sanity check. */
      if(option->type!=GAL_TYPE_STRLL)
        error(EXIT_FAILURE, 0, "%s: currently only string linked lists "
              "are acceptable for printing", __func__);

      /* Check each node, one by one. */
      for(tmp=*(gal_list_str_t **)(option->value);
          tmp!=NULL; tmp=tmp->next)
        {
          /* Get the length of this node: */
          vlen=options_print_any_type(option, &tmp->v, GAL_TYPE_STRING,
                                      0, NULL, cp);

          /* If its larger than the maximum length, then put it in. */
          if( vlen > *max_vlen )
            *max_vlen=vlen;
        }
    }
  else
    {
      vlen=options_print_any_type(option, option->value, option->type,
                                  0, NULL, cp);
      if( vlen > *max_vlen )
        *max_vlen=vlen;
    }

  /* If the name of this option is larger than all existing, set its
     length as the largest name length. */
  if( strlen(option->name) > *max_nlen )
    *max_nlen = strlen(option->name);
}





/* To print the options nicely, we need the maximum lengths of the options
   and their values. */
static void
options_set_lengths(struct argp_option *poptions,
                    struct argp_option *coptions,
                    int *namelen, int *valuelen,
                    struct gal_options_common_params *cp)
{
  int i, max_nlen=0, max_vlen=0;

  /* For program specific options. */
  for(i=0; !gal_options_is_last(&poptions[i]); ++i)
    if(poptions[i].name && poptions[i].set)
      options_correct_max_lengths(&poptions[i], &max_nlen, &max_vlen, cp);

  /* For common options. Note that the options that will not be printed are
     in this category, so we also need to check them. The detailed steps
     are the same as before. */
  for(i=0; !gal_options_is_last(&coptions[i]); ++i)
    if( coptions[i].name && coptions[i].set
        && option_is_printable(&coptions[i]) )
      options_correct_max_lengths(&coptions[i], &max_nlen, &max_vlen, cp);

  /* Save the final values in the output pointers. */
  *namelen  = max_nlen;
  *valuelen = ( max_vlen < GAL_OPTIONS_MAX_VALUE_LEN
                ? max_vlen
                : GAL_OPTIONS_MAX_VALUE_LEN );
}





/* The '#' before the 'doc' string are not required by the configuration
   file parser when the documentation string fits in a line. However, when
   the 'doc' string is longer than 80 characters, it will be cut between
   multiple lines and without the '#', the start of the line will be read
   as an option. */
static void
options_print_doc(FILE *fp, const char *doc, int nvwidth)
{
  size_t len=strlen(doc);

  /* The '+3' is because of the three extra spaces in this line: one before
     the variable name, one after it and one after the value. */
  int i, prewidth=nvwidth+3, width=77-prewidth, cwidth;

  /* We only want the formatting when writing to stdout. */
  if(len<width)
    fprintf(fp, "# %s\n", doc);
  else
    {
      /* If the break is in the middle of a word, then pull set it before
         the word starts. */
      cwidth=width; while( doc[cwidth]!=' ' ) --cwidth;
      fprintf(fp, "# %.*s\n", cwidth, doc);
      i=cwidth;

      /* Go over the rest of the line. */
      while(i<len)
        {
          /* Remove any possible space before the first word. */
          while( doc[i]==' ' ) ++i;

          /* Check if the line break won't fall in the middle of a word. */
          cwidth=width;
          if( i+cwidth<len) while( doc[i+cwidth]!=' ' ) --cwidth;
          fprintf(fp, "%*s# %.*s\n", prewidth, "", cwidth, &doc[i]);
          i+=cwidth;
        }
    }
}





static void
options_print_all_in_group(struct argp_option *options, int groupint,
                           int namelen, int valuelen, FILE *fp,
                           struct gal_options_common_params *cp)
{
  size_t i;
  gal_list_str_t *tmp;
  int namewidth=namelen+1, valuewidth=valuelen+1;

  /* Go over all the options. */
  for(i=0; !gal_options_is_last(&options[i]); ++i)
    if( options[i].group == groupint           /* Is in this group.        */
        && options[i].set                      /* Has been given a value.  */
        && option_is_printable(&options[i]) )  /* Is relevant for printing.*/
      {
        /* Linked lists. */
        if(gal_type_is_list(options[i].type))
          for(tmp=*(gal_list_str_t **)(options[i].value);
              tmp!=NULL; tmp=tmp->next)
            {
              fprintf(fp, " %-*s ", namewidth, options[i].name);
              options_print_any_type(&options[i], &tmp->v,
                                     GAL_TYPE_STRING, valuewidth,
                                     fp, cp);
              options_print_doc(fp, options[i].doc, namewidth+valuewidth);
            }

        /* Normal types. */
        else
          {
            fprintf(fp, " %-*s ", namewidth, options[i].name);
            options_print_any_type(&options[i], options[i].value,
                                   options[i].type, valuewidth, fp, cp);
            options_print_doc(fp, options[i].doc, namewidth+valuewidth);
          }
      }
}





static void
options_print_all(struct gal_options_common_params *cp, char *dirname,
                  const char *optionname)
{
  size_t i;
  FILE *fp;
  int errnum;
  time_t rawtime;
  char *topicstr, *filename;
  gal_list_i32_t *group=NULL;
  gal_list_str_t *topic=NULL;
  int groupint, namelen, valuelen;
  struct argp_option *coptions=cp->coptions, *poptions=cp->poptions;

  /* If the configurations are to be written to a file, then do the
     preparations. */
  if(dirname)
    {
      /* Make the host directory if it doesn't already exist. */
      if( (errnum=gal_checkset_mkdir(dirname)) )
        error(EXIT_FAILURE, errnum, "making %s for configuration files",
              dirname);

      /* Prepare the full filename: */
      if( asprintf(&filename, "%s/%s.conf", dirname, cp->program_exec)<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);

      /* Remove the file if it already exists. */
      gal_checkset_writable_remove(filename, NULL, 0, 0);

      /* Open the file for writing. */
      errno=0;
      fp=fopen(filename, "w");
      if(fp==NULL)
        error(EXIT_FAILURE, errno, "%s: couldn't open to write "
              "configuration file in %s", dirname, __func__);

      /* Print the basic information as comments in the file first. */
      time(&rawtime);
      fprintf(fp,
              "# %s (%s) %s.\n"
              "# Written at %s#\n"
              "#  - Empty lines are ignored.\n"
              "#  - Lines starting with '#' are ignored.\n"
              "#  - The long option name is followed by a value.\n"
              "#  - The name and value should be separated by atleast\n"
              "#    one white space character (for example space or tab).\n"
              "#  - If the value has space, enclose the whole value in\n"
              "#    double quotation (\") signs.\n"
              "#  - After the value, the rest of the line is ignored.\n"
              "#\n# Run 'info %s' for a more elaborate description of each "
              "option.\n",
              cp->program_name, PACKAGE_NAME, PACKAGE_VERSION,
              ctime(&rawtime), cp->program_exec);
    }
  else fp=stdout;

  /* Parse all the options with a title, note that the 'Input', 'Output'
     and 'Operating mode' options are defined in the common options, while
     the (possible) other groups are in the program specific options. We
     will only be dealing with the 'topics' linked list in this function
     and the strings in 'poption' are statically allocated, so its fine to
     not waste CPU cycles allocating and freeing. */
  for(i=0; !gal_options_is_last(&coptions[i]); ++i)
    if(coptions[i].name==NULL && coptions[i].key==0 && coptions[i].doc)
      {
        /* The '(char *)' is because '.doc' is a constant and this helps
           remove the compiler warning. */
        gal_list_i32_add(&group, coptions[i].group);
        gal_list_str_add(&topic, (char *)coptions[i].doc, 0);
      }
  for(i=0; !gal_options_is_last(&poptions[i]); ++i)
    if(poptions[i].name==NULL && poptions[i].key==0 && poptions[i].doc)
      {
        gal_list_i32_add(&group, poptions[i].group);
        gal_list_str_add(&topic, (char *)poptions[i].doc, 0);
      }

  /* Reverse the linked lists to get the same input order. */
  gal_list_str_reverse(&topic);
  gal_list_i32_reverse(&group);

  /* Get the maximum width of names and values. */
  options_set_lengths(poptions, coptions, &namelen, &valuelen, cp);

  /* Go over each topic and print every option that is in this group. */
  while(topic)
    {
      /* Pop the nodes from the linked list. */
      groupint = gal_list_i32_pop(&group);
      topicstr = gal_list_str_pop(&topic);

      /* First print the topic, */
      fprintf(fp, "\n# %s\n", topicstr);
      /*
      fprintf(fp, "# ");
      i=0; while(i++<strlen(topicstr)) fprintf(fp, "%c", '-');
      fprintf(fp, "\n");
      */
      /* Then, print all the options that are in this group. */
      options_print_all_in_group(coptions, groupint, namelen, valuelen,
                                 fp, cp);
      options_print_all_in_group(poptions, groupint, namelen, valuelen,
                                 fp, cp);
    }

  /* Let the user know. */
  if(dirname)
    {
      printf("\nNew/updated configuration file:\n\n  %s\n\n"
             "You may inspect it with 'cat %s'.\n"
             "You may use your favorite text editor to modify it later.\n"
             "Or, run %s again with new values for the options and '--%s'.\n",
             filename, filename, cp->program_name, optionname);
      free(filename);
    }

  /* Exit the program successfully. */
  exit(EXIT_SUCCESS);
}





#define OPTIONS_UINT8VAL *(uint8_t *)(cp->coptions[i].value)
void
gal_options_print_state(struct gal_options_common_params *cp)
{
  size_t i;
  unsigned char sum=0;
  char *home, *dirname;


  /* A sanity check is necessary first. We want to make sure that the user
     hasn't called more than one of these options. */
  for(i=0; !gal_options_is_last(&cp->coptions[i]); ++i)
    if(cp->coptions[i].set)
      switch(cp->coptions[i].key)
        {
        case GAL_OPTIONS_KEY_PRINTPARAMS:
        case GAL_OPTIONS_KEY_SETDIRCONF:
        case GAL_OPTIONS_KEY_SETUSRCONF:

          /* Note that these options can have a value of 1 (enabled) or 0
             (explicitly disabled). Therefore the printing should only be
             done if they have a value of 1. This is why we have defined
             the 'OPTIONS_UINT8VAL' macro above. */
          sum += OPTIONS_UINT8VAL;
        }


  /* See if the values should be printed and if so, where. */
  switch(sum)
    {
    /* No printing option has been called, so just return. */
    case 0:  return;

    /* (Only) one of the printing options has been called, so we'll need to
       print the values in the proper place. */
    case 1:
      for(i=0; !gal_options_is_last(&cp->coptions[i]); ++i)
        if(cp->coptions[i].set && OPTIONS_UINT8VAL)
          switch(cp->coptions[i].key)
            {
            case GAL_OPTIONS_KEY_PRINTPARAMS:
              options_print_all(cp, NULL, NULL);
              break;

            case GAL_OPTIONS_KEY_SETDIRCONF:
              if( asprintf(&dirname, ".%s", PACKAGE)<0 )
                error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
              options_print_all(cp, dirname, cp->coptions[i].name);
              free(dirname);
              break;

            case GAL_OPTIONS_KEY_SETUSRCONF:
              home=options_get_home();
              if( asprintf(&dirname, "%s/%s", home, USERCONFIG_DIR)<0 )
                error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
              options_print_all(cp, dirname, cp->coptions[i].name);
              free(dirname);
              break;
            }
      break;

    /* More than one of the printing options has been called. */
    default:
      error(EXIT_FAILURE, 0, "only one of the 'printparams', 'setdirconf' "
            "and 'setusrconf' options can be called in each run");
    }
}





/* Main working function for common and program specific options. */
static void
options_as_fits_keywords_write(gal_fits_list_key_t **keys,
                               struct argp_option *options,
                               struct gal_options_common_params *cp)
{
  size_t i;
  void *vptr;
  int vptrfree;
  uint8_t vtype;
  gal_list_str_t *tmp;
  char *name, *doc, *strval;

  for(i=0; !gal_options_is_last(&options[i]); ++i)
    if( options[i].set && option_is_printable(&options[i]) )
      {
        /* Linked lists (multiple calls to an option). */
        if(gal_type_is_list(options[i].type))
          for(tmp=*(gal_list_str_t **)(options[i].value);
              tmp!=NULL; tmp=tmp->next)
            {
              /* 'name' and 'doc' have a 'const' qualifier. */
              gal_checkset_allocate_copy(tmp->v, &strval);
              gal_checkset_allocate_copy(options[i].doc,  &doc);
              gal_checkset_allocate_copy(options[i].name, &name);
              gal_fits_key_list_add(keys, GAL_TYPE_STRING, name, 1,
                                    strval, 1, doc, 1, NULL, 0);
            }

        /* Normal (single element) types. */
        else
          {
            /* If the option is associated with a special function for
               reading and writing, we'll need to write the value as a
               string. */
            vptrfree=0;
            if(options[i].func)
              {
                vptrfree=1;
                vtype=GAL_TYPE_STRING;
                vptr=options[i].func(&options[i], NULL, NULL,
                                     (size_t)(-1), cp->program_struct);
              }
            else
              {
                vtype=options[i].type;
                vptr = ( vtype==GAL_TYPE_STRING
                         ? *((char **)(options[i].value))
                         : options[i].value );
              }

            /* Write the keyword. Note that 'name' and 'doc' have a 'const'
               qualifier. */
            gal_checkset_allocate_copy(options[i].name, &name);
            if(vtype==GAL_TYPE_STRING && strlen(vptr)>FLEN_KEYWORD)
              gal_fits_key_write_filename(name, vptr, keys, 1,
                                          cp->quiet);
            else
              {
                gal_checkset_allocate_copy(options[i].doc,  &doc);
                gal_fits_key_list_add(keys, vtype, name, 1, vptr,
                                      vptrfree, doc, 1, NULL, 0);
              }
          }
      }
}




/* Write all options as FITS keywords. */
void
gal_options_as_fits_keywords(struct gal_options_common_params *cp)
{
  char *pname_upper, *extname;

  /* If the user doesn't want any configuration, return (so the pointer
     remains NULL and nothing is written). */
  if( cp->outfitsnoconfig ) return;

  /* Add the extension name (to be all-caps). Note that 'toupper' will make
     it upper-case inplace (which is not intended here). */
  gal_checkset_allocate_copy(cp->program_name, &pname_upper);
  gal_checkset_string_case_change(pname_upper, 1);
  extname=gal_checkset_malloc_cat(pname_upper, "-CONFIG");
  gal_fits_key_list_add(&cp->ckeys, GAL_TYPE_STRING, "EXTNAME", 0,
                        extname, 1, "HDU name", 0, NULL, 0);
  free(pname_upper);

  /* Title for general configuration keywords. */
  gal_fits_key_list_title_add(&cp->ckeys, "Option values", 0);

  /* Write all the command and program-specific options. */
  options_as_fits_keywords_write(&cp->ckeys, cp->coptions, cp);
  options_as_fits_keywords_write(&cp->ckeys, cp->poptions, cp);

  /* Write the library version information into the configuration
     keywords. */
  if(    cp->outfitsnodate==0
      || cp->outfitsnoversions==0
      || cp->outfitsnocommit==0 )
    {
      if(cp->outfitsnodate==0)
        gal_fits_key_list_title_add(&cp->ckeys, "Versions and date", 0);
      else
        gal_fits_key_list_title_add(&cp->ckeys, "Versions", 0);
      if(cp->outfitsnodate==0)
        gal_fits_key_list_add_date(&cp->ckeys, "Date processing started.");
      if(cp->outfitsnoversions==0)
        gal_fits_key_list_add_software_versions(&cp->ckeys);
      if(cp->outfitsnocommit==0)
        gal_fits_key_list_add_git_commit(&cp->ckeys);
    }

  /* Reverse the list (its a first-in-first-out list). */
  gal_fits_key_list_reverse(&cp->ckeys);
}
