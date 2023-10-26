/*********************************************************************
table -- Functions for I/O on tables.
This is part of GNU Astronomy Utilities (Gnuastro) package.

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
#include <config.h>

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#include <gnuastro/git.h>
#include <gnuastro/txt.h>
#include <gnuastro/blank.h>
#include <gnuastro/table.h>
#include <gnuastro/pointer.h>

#include <gnuastro-internal/timing.h>
#include <gnuastro-internal/checkset.h>
#include <gnuastro-internal/tableintern.h>






/************************************************************************/
/***************            Internal conversions          ***************/
/************************************************************************/
uint8_t
gal_table_displayflt_from_str(char *string)
{
  if(      !strcmp(string, "exp"))
    return GAL_TABLE_DISPLAY_FMT_EXP;
  else if (!strcmp(string, "fixed"))
    return GAL_TABLE_DISPLAY_FMT_FIXED;
  else return GAL_TABLE_DISPLAY_FMT_INVALID;
}





char *
gal_table_displayflt_to_str(uint8_t fmt)
{
  switch(fmt)
    {
    case GAL_TABLE_DISPLAY_FMT_FIXED: return "fixed";
    case GAL_TABLE_DISPLAY_FMT_EXP:   return "exp";
    default:                          return NULL;
    }
}



















/************************************************************************/
/***************         Information about a table        ***************/
/************************************************************************/
/* Store the information of each column in a table (either as a text file
   or as a FITS table) into an array of data structures with 'numcols'
   structures (one data structure for each column). The number of rows is
   stored in 'numrows'.\

   See the DESCRIPTION OF THIS FUNCTION IN THE BOOK FOR a detailed listing
   of the output's elements. */
gal_data_t *
gal_table_info(char *filename, char *hdu, gal_list_str_t *lines,
               size_t *numcols, size_t *numrows, int *tableformat,
               char *hdu_option_name)
{
  /* Get the table format and size (number of columns and rows). */
  if(filename && gal_fits_file_recognized(filename))
    return gal_fits_tab_info(filename, hdu, numcols, numrows, tableformat,
                             hdu_option_name);
  else
    {
      *tableformat=GAL_TABLE_FORMAT_TXT;
      return gal_txt_table_info(filename, lines, numcols, numrows);
    }

  /* Abort with an error if we get to this point. */
  error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s so we can "
        "fix the problem. Control must not have reached the end of this "
        "function", __func__, PACKAGE_BUGREPORT);
  return NULL;
}





void
gal_table_print_info(gal_data_t *allcols, size_t numcols, size_t numrows)
{
  size_t i, mms;
  int Nw=3, nw=4, uw=5, tw=4, twt;/* Initial width from label's width */
  char *typestr, *name, *unit, *comment;

  /* If there aren't any columns, there is no need to print anything. */
  if(numcols==0) return;

  /* Set the widths to print the column information. The width for the
     column number can easily be identified from the logarithm of the
     number of columns. */
  Nw=log10(numcols)+1;
  for(i=0;i<numcols;++i)
    {
      /* Length of name and unit columns. */
      if(allcols[i].name && strlen(allcols[i].name)>nw)
        nw=strlen(allcols[i].name);
      if(allcols[i].unit && strlen(allcols[i].unit)>uw)
        uw=strlen(allcols[i].unit);

      /* For the type, we need to account for vector columns. For strings,
         their length is already accounted for there, so this doesn't
         involve them. Recall that within 'gal_fits_tab_info', we put the
         repeat counter into the 'minmapsize' element of the
         'gal_data_t'. If the repeat of a numerical column is more than 1,
         then we need to add a '[N]' after the type name later. */
      mms=allcols[i].minmapsize;
      twt=strlen(gal_type_name(allcols[i].type, 1));
      if(allcols[i].type!=GAL_TYPE_STRING && mms>1)
        twt += (int)(log10(mms))+1+2; /* 1 for the log, 2 for '()'. */
      if(allcols[i].type && twt>tw) tw=twt;
    }

  /* We want one column space between the columns for readability, not the
     exact length, so increment all the numbers. */
  Nw+=2; nw+=2; uw+=2; tw+=2;

  /* Print these column names. */
  printf("%-*s%-*s%-*s%-*s%s\n", Nw, "---", nw, "----", uw,
         "-----", tw, "----", "-------");
  printf("%-*s%-*s%-*s%-*s%s\n", Nw, "No.", nw, "Name", uw,
         "Units", tw, "Type", "Comment");
  printf("%-*s%-*s%-*s%-*s%s\n", Nw, "---", nw, "----", uw,
         "-----", tw, "----", "-------");

  /* For each column, print the information, then free them. */
  for(i=0;i<numcols;++i)
    {
      /* To help in reading. */
      name    = allcols[i].name;       /* Just defined for easier     */
      unit    = allcols[i].unit;       /* readability. The compiler   */
      comment = allcols[i].comment;    /* optimizer will remove them. */

      /* Set the vector size (if relevant). */
      mms=allcols[i].minmapsize;
      if(allcols[i].type!=GAL_TYPE_STRING && mms>1)
        {
          if( asprintf(&typestr, "%s(%zu)",
                       gal_type_name(allcols[i].type, 1), mms)<0 )
            error(EXIT_FAILURE, 0, "%s: 'astprintf' allocation", __func__);
        }
      else
        gal_checkset_allocate_copy(gal_type_name(allcols[i].type, 1),
                                   &typestr);

      /* Print the actual info. */
      printf("%-*zu%-*s%-*s%-*s%s\n", Nw, i+1,
             nw, name ? name : GAL_BLANK_STRING ,
             uw, unit ? unit : GAL_BLANK_STRING ,
             tw,
             allcols[i].type ? typestr : "--",
             comment ? comment : GAL_BLANK_STRING);
      free(typestr);
    }

  /* Print the number of rows. */
  if(numrows!=GAL_BLANK_SIZE_T)
    printf("--------\nNumber of rows: %zu\n--------\n", numrows);
}




















/************************************************************************/
/***************               Read a table               ***************/
/************************************************************************/

/* Function to print regular expression error. This is taken from the GNU C
   library manual, with small modifications to fit out style. */
static void
table_regexerrorexit(int errcode, regex_t *compiled, char *input)
{
  char *regexerrbuf;
  size_t length = regerror (errcode, compiled, NULL, 0);

  errno=0;
  regexerrbuf=malloc(length);
  if(regexerrbuf==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for regexerrbuf",
          __func__, length);
  (void) regerror(errcode, compiled, regexerrbuf, length);

  error(EXIT_FAILURE, 0, "%s: regular expression error: %s in value to "
        "'--column' ('-c'): '%s'", __func__, regexerrbuf, input);
}





/* Macro to set the string to search in. */
static char *
table_set_strcheck(gal_data_t *col, int searchin)
{
  switch(searchin)
    {
    case GAL_TABLE_SEARCH_NAME:
      return col->name;

    case GAL_TABLE_SEARCH_UNIT:
      return col->unit;

    case GAL_TABLE_SEARCH_COMMENT:
      return col->comment;

    default:
      error(EXIT_FAILURE, 0, "%s: the code %d to searchin was not "
            "recognized", __func__, searchin);
    }

  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s so we can "
        "address the problem. Control must not have reached the end of "
        "this function", __func__, PACKAGE_BUGREPORT);
  return NULL;
}





gal_list_sizet_t *
gal_table_list_of_indexs(gal_list_str_t *cols, gal_data_t *allcols,
                         size_t numcols, int searchin, int ignorecase,
                         char *filename, char *hdu, size_t *colmatch)
{
  long tlong;
  int regreturn;
  regex_t *regex;
  gal_list_str_t *tmp;
  gal_list_sizet_t *indexll=NULL;
  size_t i, nummatch, colcount=0, len;
  char *str, *strcheck, *tailptr, *errorstring;

  /* Go over the given columns. */
  if(cols)
    for(tmp=cols; tmp!=NULL; tmp=tmp->next)
      {
        /* Counter for number of columns matched, and length of name. */
        nummatch=0;
        len=strlen(tmp->v);

        /* REGULAR EXPRESSION: the first and last characters are '/'. */
        if( tmp->v[0]=='/' && tmp->v[len-1]=='/' )
          {
            /* Remove the slashes, note that we don't want to change
               'tmp->v' (because it should be freed later). So first we set
               the last character to '\0', then define a new string from
               the first element. */
            tmp->v[len-1]='\0';
            str = tmp->v + 1;

            /* Allocate the regex_t structure: */
            errno=0;
            regex=malloc(sizeof *regex);
            if(regex==NULL)
              error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for "
                    "regex", __func__, sizeof *regex);

            /* First we have to "compile" the string into the regular
               expression, see the "POSIX Regular Expression Compilation"
               section of the GNU C Library.

               About the case of the string: the FITS standard says: "It is
               _strongly recommended_ that every field of the table be
               assigned a unique, case insensitive name with this
               keyword..."  So the column names can be case-sensitive.

               Here, we don't care about the details of a match, the only
               important thing is a match, so we are using the REG_NOSUB
               flag. */
            regreturn=0;
            regreturn=regcomp(regex, str, ( ignorecase
                                            ? RE_SYNTAX_AWK | REG_ICASE
                                            : RE_SYNTAX_AWK ) );
            if(regreturn)
              table_regexerrorexit(regreturn, regex, str);


            /* With the regex structure "compile"d you can go through all
               the column names. Just note that column names are not
               mandatory in the FITS standard, so some (or all) columns
               might not have names, if so 'p->tname[i]' will be NULL. */
            for(i=0;i<numcols;++i)
              {
                strcheck=table_set_strcheck(&allcols[i], searchin);
                if(strcheck && regexec(regex, strcheck, 0, 0, 0)==0)
                  {
                    ++nummatch;
                    gal_list_sizet_add(&indexll, i);
                  }
              }

            /* Free the regex_t structure: */
            regfree(regex);

            /* Put the '/' back into the input string. This is done because
               after this function, the calling program might want to
               inform the user of their exact input string. */
            tmp->v[len-1]='/';
          }


        /* Not regular expression. */
        else
          {
            tlong=strtol(tmp->v, &tailptr, 0);

            /* INTEGER: If the string is an integer, then tailptr should
               point to the null character. If it points to anything else,
               it shows that we are not dealing with an integer (usable as
               a column number). So floating point values are also not
               acceptable. Since it is possible for the users to give zero
               for the column number, we need to read the string as a
               number first, then check it here. */
            if(*tailptr=='\0')
              {
                /* Make sure the number is larger than zero! */
                if(tlong<=0)
                  error(EXIT_FAILURE, 0, "%s: column numbers must be "
                        "positive (not zero or negative). You have asked "
                        "for column number %ld", __func__, tlong);

                /* Check if the given value is not larger than the number
                   of columns in the input catalog (note that the user is
                   counting from 1, not 0!). */
                if(tlong>numcols)
                  error(EXIT_FAILURE, 0, "%s: has %zu columns, but you "
                        "have asked for column number %ld",
                        gal_fits_name_save_as_string(filename, hdu),
                        numcols, tlong);

                /* Everything seems to be fine, put this column number in
                   the output column numbers linked list. Note that
                   internally, the column numbers start from 0, not 1. */
                gal_list_sizet_add(&indexll, tlong-1);
                ++nummatch;
              }



            /* EXACT MATCH: */
            else
              {
                /* Go through all the desired column information and add
                   the column number when there is a match. */
                for(i=0;i<numcols;++i)
                  {
                    /* Check if this column actually has any
                       information. Then do a case-sensitive or insensitive
                       comparison of the strings. */
                    strcheck=table_set_strcheck(&allcols[i], searchin);
                    if(strcheck && ( ignorecase
                                     ? !strcasecmp(tmp->v, strcheck)
                                     : !strcmp(tmp->v, strcheck) ) )
                      {
                        ++nummatch;
                        gal_list_sizet_add(&indexll, i);
                      }
                  }
              }
          }


        /* If there was no match, then report an error. This can only happen
           for string matches, not column numbers, for numbers, the checks
           are done (and program is aborted) before this step. */
        if(nummatch==0)
          {
            if( asprintf(&errorstring, "'%s' didn't match any of the "
                         "column %ss.", tmp->v,
                         gal_tableintern_searchin_as_string(searchin))<0 )
              error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
            gal_tableintern_error_col_selection(filename, hdu, errorstring);
          }


        /* Keep the value of 'nummatch' if the user requested it. */
        if(colmatch) colmatch[colcount++]=nummatch;
      }

  /* cols==NULL */
  else
    for(i=0;i<numcols;++i)
      gal_list_sizet_add(&indexll, i);

  /* Reverse the list. */
  gal_list_sizet_reverse(&indexll);

  /* For a check.
  gal_list_sizet_print(indexll);
  exit(0);
  */

  /* Return the list. */
  return indexll;
}





/* Read the specified columns in a table (named 'filename') into a linked
   list of data structures. If the file is FITS, then 'hdu' will also be
   used, otherwise, 'hdu' is ignored. The information to search for columns
   should be specified by the 'cols' linked list as string values in each
   node of the list, the strings in each node can be a number, an exact
   match to a column name, or a regular expression (in GNU AWK format)
   enclosed in '/ /'. The 'searchin' value comes from the
   'gal_table_where_to_search' enumerator and has to be one of its given
   types. If 'cols' is NULL, then this function will read the full table.

   The output is a linked list with the same order of the cols linked
   list. Note that one column node in the 'cols' list might give multiple
   columns, in this case, the order of output columns that correspond to
   that one input, are in order of the table (which column was read first).
   So the first requested column is the first popped data structure and so
   on. */
gal_data_t *
gal_table_read(char *filename, char *hdu, gal_list_str_t *lines,
               gal_list_str_t *cols, int searchin, int ignorecase,
               size_t numthreads, size_t minmapsize, int quietmmap,
               size_t *colmatch, char *hdu_option_name)
{
  int tableformat;
  gal_list_sizet_t *indexll;
  size_t i, numcols, numrows;
  gal_data_t *allcols, *out=NULL;

  /* First get the information of all the columns. */
  allcols=gal_table_info(filename, hdu, lines, &numcols, &numrows,
                         &tableformat, hdu_option_name);

  /* If there was no actual data in the file, then return NULL. */
  if(allcols==NULL) return NULL;

  /* Get the list of indexs in the same order as the input list. */
  indexll=gal_table_list_of_indexs(cols, allcols, numcols, searchin,
                                   ignorecase, filename, hdu, colmatch);

  /* Depending on the table format, read the columns into the output
     structure. Also note that after these functions, the 'indexll' will be
     all freed (each popped element is actually freed). */
  switch(tableformat)
    {
    case GAL_TABLE_FORMAT_TXT:
      out=gal_txt_table_read(filename, lines, numrows, allcols, indexll,
                             minmapsize, quietmmap);
      break;

    case GAL_TABLE_FORMAT_AFITS:
    case GAL_TABLE_FORMAT_BFITS:
      out=gal_fits_tab_read(filename, hdu, numrows, allcols, indexll,
                            numthreads, minmapsize, quietmmap,
                            hdu_option_name);
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: table format code %d not recognized for "
            "'tableformat'", __func__, tableformat);
    }

  /* Clean up. */
  for(i=0;i<numcols;++i)
    gal_data_free_contents(&allcols[i]);
  free(allcols);
  gal_list_sizet_free(indexll);

  /* Return the final linked list. */
  return out;
}




















/************************************************************************/
/***************              Write a table               ***************/
/************************************************************************/
/* Write the basic information that is necessary by each program into the
   comments field. Note that the 'comments' has to be already sorted in the
   proper order. */
void
gal_table_comments_add_intro(gal_list_str_t **comments,
                             char *program_string, time_t *rawtime)
{
  char gitdescribe[100], *tmp;

  /* Get the Git description in the running folder. */
  tmp=gal_git_describe();
  if(tmp) { sprintf(gitdescribe, " from %s,", tmp); free(tmp); }
  else      gitdescribe[0]='\0';

  /* Git version and time of program's starting, this will be the second
     line. Note that ctime puts a '\n' at the end of its string, so we'll
     have to remove that. Also, note that since we are allocating 'msg', we
     are setting the allocate flag of 'gal_list_str_add' to 0. */
  if( asprintf(&tmp, "Created%s on %s", gitdescribe, ctime(rawtime))<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  tmp[ strlen(tmp)-1 ]='\0';
  gal_list_str_add(comments, tmp, 0);

  /* Program name: this will be the top of the list (first line). We will
     need to set the allocation flag for this one, because program_string
     is usually statically allocated. */
  if(program_string)
    gal_list_str_add(comments, program_string, 1);
}





/* The input is a linked list of data structures and some comments. The
   table will then be written into 'filename' with a format that is
   specified by 'tableformat'. */
void
gal_table_write(gal_data_t *cols, struct gal_fits_list_key_t **keylist,
                gal_list_str_t *comments, int tableformat, char *filename,
                char *extname, uint8_t colinfoinstdout)
{
  /* If a filename was given, then the tableformat is relevant and must be
     used. When the filename is empty, a text table must be printed on the
     standard output (on the command-line). */
  if(filename)
    {
      if(gal_fits_name_is_fits(filename))
        gal_fits_tab_write(cols, comments, tableformat, filename, extname,
                           keylist);
      else
        gal_txt_write(cols, keylist, comments, filename,
                      colinfoinstdout, 0);
    }
  else
    /* Write to standard output. */
    gal_txt_write(cols, keylist, comments, filename, colinfoinstdout, 0);
}





void
gal_table_write_log(gal_data_t *logll, char *program_string,
                    time_t *rawtime, gal_list_str_t *comments,
                    char *filename, int quiet)
{
  char *msg;

  /* Write all the comments into "?". */
  gal_table_comments_add_intro(&comments, program_string, rawtime);

  /* Write the log file to disk. */
  gal_table_write(logll, NULL, comments, GAL_TABLE_FORMAT_TXT,
                  filename, "LOG", 0);

  /* In verbose mode, print the information. */
  if(!quiet)
    {
      if( asprintf(&msg, "%s created.", filename)<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
      gal_timing_report(NULL, msg, 1);
      free(msg);
    }
}




















/************************************************************************/
/***************            Column operation              ***************/
/************************************************************************/
gal_data_t *
gal_table_col_vector_extract(gal_data_t *vector, gal_list_sizet_t *indexs)
{
  uint8_t type;
  gal_data_t *out=NULL;
  size_t i, vw, vh, ind;
  char *name, *basename;
  gal_list_sizet_t *tind;

  /* Basic sanity checks. */
  if(vector==NULL) return NULL;
  if(indexs==NULL) return NULL;
  if(vector->ndim!=2)
    error(EXIT_FAILURE, 0, "%s: the input 'vector' must have 2 "
          "dimensions but has %zu dimensions", __func__, vector->ndim);
  for(tind=indexs;tind!=NULL;tind=tind->next)
    if(tind->v > vector->dsize[1])
      error(EXIT_FAILURE, 0, "%s: the input vector has %zu elements but "
            "you have asked for index %zu (counting from zero)", __func__,
            vector->dsize[1], tind->v);

  /* Allocate the output columns. */
  type=vector->type;
  vh=vector->dsize[0];
  vw=vector->dsize[1];
  for(tind=indexs;tind!=NULL;tind=tind->next)
    {
      /* If the vector has a name, add a counter after it. If it doesn't
         have a name, just use 'VECTOR'. Because index couting starts from
         0, but column counters in tables start with 1, we'll add one to
         the index. */
      ind=tind->v;
      basename = vector->name ? vector->name : "VECTOR";
      if( asprintf(&name, "%s-%zu", basename, ind+1)<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf alloc of 'name'",  __func__);

      /* Allocate the output and fill it. */
      gal_list_data_add_alloc(&out, NULL, type, 1, &vh, NULL, 1,
                              vector->minmapsize, vector->quietmmap,
                              name, vector->unit, vector->comment);
      for(i=0;i<vh;++i)
        memcpy(gal_pointer_increment(out->array,    i,        type),
               gal_pointer_increment(vector->array, i*vw+ind, type),
               gal_type_sizeof(type));

      /* Clean up. */
      free(name);
    }

  /* Reverse the list to be in the same order as the indexs, and return. */
  gal_list_data_reverse(&out);
  return out;
}





gal_data_t *
gal_table_cols_to_vector(gal_data_t *list)
{
  gal_data_t *tmp, *out;
  char *name, *unit=NULL, *inname=NULL;
  size_t i, j, dsize[2], num=gal_list_data_number(list);

  /* If the list is empty or just has a single column, return itself. */
  if(num<2) return list;

  /* Go over the inputs na make sure they are all single dimensional and
     have the same size. */
  for(tmp=list;tmp!=NULL;tmp=tmp->next)
    {
      /* First, a sanity check. */
      if(tmp->ndim!=1
         || tmp->type!=list->type
         || tmp->dsize[0]!=list->dsize[0])
        error(EXIT_FAILURE, 0, "%s: inputs should all be single-valued "
              "columns (one dimension) and have the same size and type",
              __func__);

      /* Find the first one with a name. */
      if(tmp->unit && unit==NULL)   unit=tmp->unit;
      if(tmp->name && inname==NULL) inname=tmp->name;
    }

  /* Set the name based on the input. */
  if( asprintf(&name, "%s-VECTOR", inname?inname:"TO")<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);

  /* Allocate the output dataset. */
  dsize[1]=num;
  dsize[0]=list->size;
  out=gal_data_alloc(NULL, list->type, 2, dsize, NULL, 0, list->minmapsize,
                     list->quietmmap, name, unit,
                     "Vector by merging multiple columns.");

  /* Fill the output dataset. */
  j=0;
  for(tmp=list;tmp!=NULL;tmp=tmp->next)
    {
      for(i=0;i<tmp->dsize[0];++i)
        memcpy(gal_pointer_increment(out->array, i*num+j, out->type),
               gal_pointer_increment(tmp->array, i,       out->type),
               gal_type_sizeof(out->type));
      ++j;
    }

  /* Clean up and return. */
  free(name);
  return out;
}
