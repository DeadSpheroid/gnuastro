/*********************************************************************
txt -- functions to deal with plain text files.
This is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <gnuastro/txt.h>
#include <gnuastro/list.h>
#include <gnuastro/units.h>
#include <gnuastro/blank.h>
#include <gnuastro/table.h>
#include <gnuastro/pointer.h>
#include <gnuastro/statistics.h>

#include <gnuastro-internal/checkset.h>
#include <gnuastro-internal/tableintern.h>









/************************************************************************/
/***************           Get table information          ***************/
/************************************************************************/

/* Format of table. Currently these constants are internal to this library,
   we don't want to crowd the name space of the user by having them in the
   header file. */
enum txt_formats_code
{
    TXT_FORMAT_INVALID,

    TXT_FORMAT_TABLE,
    TXT_FORMAT_IMAGE,
};





/* Return one of the 'txt_line_stat' constant values. */
int
gal_txt_line_stat(char *line)
{
  while(*line!='\n')
    {
      switch(*line)
        {
          /* Characters to ignore. */
        case ' ': case ',': case '\t':
          break;
        case '#':
          return GAL_TXT_LINESTAT_COMMENT;
        default:
          return GAL_TXT_LINESTAT_DATAROW;
        }
      ++line;
    }
  return GAL_TXT_LINESTAT_BLANK;
}





/* Remove the spaces around the values, and if the final/trimmed string has
   no length, return NULL. */
char *
gal_txt_trim_space(char *str)
{
  char *end;

  /* If str doesn't point to anything, just return the NULL pointer. */
  if(str==NULL) return NULL;

  /* Remove the spaces before the start of the string. */
  while(isspace(*str)) ++str;

  /* If there was nothing in the string, return NULL. */
  if(*str=='\0') return NULL;

  /* Remove the spaces at the end, and write a possibly new '\0'. */
  end = str + strlen(str) - 1;
  while(end>str && isspace(*end)) --end;
  *(end+1)='\0';

  /* Return the string. */
  return *str=='\0' ? NULL : str;
}





/* Return 1 if the input string contains the 'match' substring. */
int
gal_txt_contains_string(char *full, char *match)
{
  char *c=full;
  size_t ml = match ? strlen(match) : 0;

  /* If the input or output strings are empty, a match can't be defined
     (strncmp will always succeed to find a match). */
  if(ml==0 || full==NULL || *full=='\0') return 0;

  /* Parse the string (character by character) to see if there is a
     match. */
  do if( strncmp(c, match, ml)==0 ) return 1;
  while( *(++c)!='\0' );

  /* If control reaches here, there was no match. */
  return 0;
}




/* Read the vector type and number of elements. */
static int
txt_info_vector_type(char *string, size_t *repeat)
{
  int type, irepeat, *irptr=&irepeat;
  void **iptr=(void **)(&irptr);
  char *c, *rstr=NULL;

  /* See if there is a '(' in the string: we already know that there was a
     ')' at the end of the string before entring this function. After this
     step:
        string --> name of type
        rstr   --> string of number of repeats. */
  for(c=string; *c!='\0'; ++c)
    switch(*c)
      {
      case '(': *c='\0'; rstr=c+1; break;
      case ')': *c='\0';           break;
      }

  /* Read the "repeat" element, if it wasn't set (there was no opening
     parenthesis), or the value in the parenthesis can't be read as an
     integer number, or the given integer was negative, just ignore the
     repeat (by setting it to 1). */
  *repeat = ( ( rstr
                && gal_type_from_string(iptr, rstr, GAL_TYPE_INT)==0
                && irepeat>1 )
              ? irepeat
              : 1 );

  /* Read the type. */
  type=gal_type_from_name(string);
  if(type==GAL_TYPE_INVALID) type=GAL_TYPE_FLOAT64;
  return type;
}





/* Each information comment should have a format like this (replace
   'Column' with 'Image' for 2D arrays):

      # Column N: NAME [UNITS, TYPE(REPEAT), BLANK] COMMENT

  'TYPE' has pre-defined values, and 'N' and 'REPEAT' must be an integer,
  but the rest can contain any characters (including whitespace
  characters). The UNITS, TYPE, BLANK tokens are optional, if not given,
  default values will be set. But if there are comments, then the brackets
  themselves are required to separate the name from the comments.

  Any white space characters before or after the delimiters (':', '[', ']',
  ',') is ignored, but spaces within the values are kept. For example, in
  the two following lines, NAME will be set to 'col name' (even though
  there are extra spaces in the second line, The column unit will be
  set to 'col unit'.

      # Column 2: col name
      # Column 2 :  col name     [ col unit, type ] Column comments.

  When the column type is a string, the number of characters in the string
  is also necessary, for example 'str10'. Without an integer attached, the
  line will be ignored.

  In the case of an error or mis-match, the line will be ignored.

  This function will make a linked list of information about each column
  that has information in the comments. The information on each column
  doesn't have to be in order, for example the information of column 10 can
  be before column 7.
*/
static void
txt_info_from_comment(char *in_line, gal_data_t **datall, char *comm_start,
                      int inplace)
{
  gal_data_t *tmp;
  int index, strw=0;
  int type=GAL_TYPE_FLOAT64;                     /* Default type. */
  char *line, *aline, *tailptr;
  size_t len=strlen(comm_start), repeat;
  char *number=NULL, *name=NULL, *comment=NULL;
  char *inbrackets=NULL, *unit=NULL, *typestr=NULL, *blank=NULL;

  /* Make a copy of the input line if 'inplace==0'. */
  if(inplace) line=aline=in_line;
  else
    {
      /* Because the 'line' pointer will change, we need a pointer to the
         start of the originally allocated lines. This is the purpose of
         'aline' (allocated-line). */
      gal_checkset_allocate_copy(in_line, &aline);
      line=aline;
    }


  /* Only read this comment line if it follows the convention: */
  if( !strncmp(line, comm_start, len) )
    {
      /* Set 'name', 'inbrackets', and 'comment' in the first pass through
         the line. */
      number=line+len;
      while(*line!='\0')
        {
          switch(*line)
            {
            case ':':
              if(name==NULL) { *line='\0'; name=line+1; }
              break;

            case '[':
              if(name && inbrackets==NULL)
                { *line='\0'; inbrackets=line+1; }
              break;

            case ']':
              if(inbrackets && comment==NULL)
                { *line='\0'; comment=line+1; }
              break;

            case '\n':
              *line='\0';
              break;
            }
          ++line;
        }


      /* Read the column number as an integer. If it can't be read as an
         integer, or is zero or negative then just return without adding
         anything to this line. */
      index=strtol(number, &tailptr, 0);
      if(*tailptr!='\0' || index<=0) return;


      /* Read the column name. If no name is given (which is perfectly
         fine: for example '# Column 4: [abc, f32] description'), then
         'name' will be NULL. */
      name=gal_txt_trim_space(name);


      /* If this is a repeated index, ignore it. */
      for(tmp=*datall; tmp!=NULL; tmp=tmp->next)
        if(tmp->status==index)
          return;


      /* If there were brackets, then break it up. */
      if(inbrackets)
        {
          unit=inbrackets;
          while(*inbrackets!='\0')
            {
              if(*inbrackets==',')
                {
                  *inbrackets='\0';
                  if     (typestr==NULL)  typestr = inbrackets+1;
                  else if(blank==NULL)    blank   = inbrackets+1;
                }
              ++inbrackets;
            }
        }


      /* If 'typestr' was given, then check if this is a standard type. If
         'typestr' wasn't specified or couldn't be interpretted, then the
         default double type code will be used (see the variable
         definitions above). Just note that if we are dealing with the
         string type, we have to pull out the number part first. If there
         is no number for a string type, then ignore the line. */
      repeat=1; /* Initialize for each column. */
      if(typestr && *typestr!='\0')
        {
          typestr=gal_txt_trim_space(typestr);
          if( !strncmp(typestr, "str", 3) )
            {
              type=GAL_TYPE_STRING;
              strw=strtol(typestr+3, &tailptr, 0);
              if(*tailptr!='\0' || strw<0) return;
            }
          else
            {
              type=gal_type_from_name(typestr);
              if(type==GAL_TYPE_INVALID)
                {
                  /* See if this is a vector column (that has the format of
                     'f32(N)' for example), by seeing if the last character
                     is a parenthesis or not, we'll do the other checks in
                     a dedicated function. */
                  if(typestr[ strlen(typestr)-1 ] == ')')
                    type=txt_info_vector_type(typestr, &repeat);

                  /* No readable type, just set 64-bit float. */
                  else type=GAL_TYPE_FLOAT64;
                }
            }
        }


      /* Add this column's information into the columns linked list. We
         will define the data structure's array to have zero dimensions (no
         array) by default. If there is a blank value its value will be put
         into the array by 'gal_table_read_blank'. To keep the name, unit,
         and comment strings, trim the white space before and after each
         before using them here. */
      gal_list_data_add_alloc(datall, NULL, type, 0, NULL, NULL, 0,
                              repeat, 1, name, gal_txt_trim_space(unit),
                              gal_txt_trim_space(comment) );


      /* Put the number of this column into the status variable of the data
         structure. If the type is string, then also copy the width into
         the structure. */
      (*datall)->status=index;
      (*datall)->disp_width = type==GAL_TYPE_STRING ? strw : 0;


      /* Write the blank value into the array. Note that this is not the
         final column, we are just collecting information now. If the blank
         value wasn't interpretted into the given type the 'flag' element
         of the dataset will be set and the contents of the 'blank' string
         will be copied into the 'array' element (so it should be
         interpretted as a 'char *'). */
      gal_tableintern_read_blank(*datall, gal_txt_trim_space(blank));
    }

  /* Clean up. */
  if(in_line!=aline) free(aline);
}





/* In the case of a table, the input might not have had information in its
   comments, or the information might not have been complete. So we need to
   go through the first row of data also. In the case of the image, this is
   necessary, because we need to find the second dimension value.

   This function will return the number of tokens in the first row of the
   given text file. If the file is a text table with string columns, the
   contents of the string column will be counted as one token.

   When there is no metadata, each token on the line is a separate column,
   so the "repeat" is always one within this funciton. */
static size_t
txt_info_from_first_row(char *in_line, gal_data_t **datall, int format,
                        int inplace)
{
  double tmpd;
  void *tmpdptr=&tmpd;
  gal_data_t *col, *prev, *tmp;
  char *line, *token, *end, *aline=NULL;
  size_t i, ncol, repeat=1, maxcnum=0, numchecked;

  /* Make a copy of the input line if necessary. */
  if(inplace) line=in_line;
  else
    {
      gal_checkset_allocate_copy(in_line, &line);
      aline=line; /* We are going to change 'line' during this function. */
    }
  end=line+strlen(line);

  /* Remove the line termination character(s) from the end of the line. In
     Unix, the line terminator is just the new-line character, however, in
     some operating systems (like MS Windows), it is two characters:
     carriage return and new-line. To be able to deal with both, we will be
     checking the second last character first, the ASCII code for carriage
     return is 13.

     If the last column is a string, and the given length is larger than
     the available space on the line, we don't want to have the line's
     new-line character. Its better for it to actually be shorter than the
     space. */
  if( end>line+2 && *(end-2)==13 ) *(end-2)='\0';
  else if( *(end-1)=='\n' )        *(end-1)='\0';

  /* Get the maximum number of columns read from the comment
     information. */
  for(col=*datall; col!=NULL; col=col->next)
    maxcnum = maxcnum>col->status ? maxcnum : col->status;

  /* Go over the line check/fill the column information. */
  ncol=0;
  while(++ncol)
    {
      /* If 'line' has already passed the end of the actual string (for
         example when a string column is the last one and its declared
         width is larger than the actual number of characters it has in
         that line). */
      if(line>=end) break;

      /* When we are dealing with a text table, check if there is
         information for this column. For a text image, only the number of
         tokens is important (as the second dimension of the image), so
         just assume there no information. */
      if(format==TXT_FORMAT_TABLE)
        for(col=*datall; col!=NULL; col=col->next)
          { if(col->status==ncol) break; }
      else
        col=NULL;

      /* If there is information for this column, then check if it is a
         string, and if so, don't use 'strtok_r' (because it might have
         delimiters). So manually go ahead in the line till you get to the
         start of the string, then increment the line until the end of the
         space set for the strings. */
      if(col)
        {
          if( col->type==GAL_TYPE_STRING )
            {
              /* Remove all delimiters before the string starts. */
              while(isspace(*line) || *line==',') ++line;

              /* Increment line to the end of the string. */
              line = (token=line) + col->disp_width;

              /* If we haven't reached the end of the line (so 'line<end'),
                 then set a NULL character where the current token is
                 expected to end. In this way, we can use the token (while
                 preserving the line for the rest of the 'while'
                 loop). VERY IMPORTANT: to do this, 'line' should not be
                 '<=end'. If the given width is larger than line, there is
                 no problem, the '\0' of the line will also be used to end
                 this last column. */
              if(line<end)
                {
                  *line++='\0';
                  /* printf(" col %zu: -%s-\n", i, token); */
                }
            }
          else
            {
              /* Repeat is put in minmapsize (when we were reading the
                 column info from comments). */
              for(i=0;i<col->minmapsize;++i)
                {
                  token=strtok_r(ncol==1?line:NULL, GAL_TXT_DELIMITERS,
                                 &line);
                  if(token==NULL) break;
                }
            }
        }
      else
        {
          /* Make sure a token exists in this undefined column. */
          token=strtok_r(ncol==1?line:NULL, GAL_TXT_DELIMITERS, &line);
          if(token==NULL) break;

          /* A token exists. For a table, define a new element in the
             linked list and set the column to the default double type with
             no information, then set its status value to the column
             number. So, for a table, this should be done on every
             column. But for an image, this should only be done once (when
             'datall' has not been defined yet, for example in the column
             information). */
          if( *datall==NULL || format==TXT_FORMAT_TABLE )
            {
              /* Make sure the token is actually a number (or RA/Dec
                 written in Sexagesimal format) and print a good error
                 message when the input isn't actually a number but a
                 string (this test was added because of uncommented
                 metadata)! */
              if( gal_type_from_string( &tmpdptr, token, GAL_TYPE_FLOAT64)
                  && isnan( gal_units_ra_to_degree(token) )
                  && isnan( gal_units_dec_to_degree(token) ) )
                error(EXIT_FAILURE, 0, "'%s' couldn't be read as a "
                      "number (element %zu of first uncommented line)",
                      token, ncol);

              /* Allocate this column's dataset and set it's 'status' to
                 the column number that it corresponds to. */
              gal_list_data_add_alloc(datall, NULL, GAL_TYPE_FLOAT64, 0,
                                      NULL, NULL, 0, repeat, 1, NULL, NULL,
                                      NULL);
              (*datall)->status=ncol;
            }
        }
    }

  /* When looking at a text table, 'ncol' is the number of columns
     (elements in the linked list). But when looking at an image, it is the
     size of the second dimension. To unify things from this step forwards,
     we will thus keep the value of 'ncol' until this point in another
     variable (that will be returned finally), and for an image, change
     'ncol' to 1. This is necsesary in case the user has for example given
     two column information comments on an image plain text file.

     Note that 'ncol' counts from 1, so the total number of tokens is one
     less than 'ncol'. */
  numchecked=ncol-1;
  if(format==TXT_FORMAT_IMAGE) ncol=1;

  /* If the number of columns/images given by the comments is larger than
     the actual number of lines, remove those that have larger numbers from
     the linked list before things get complicated outside of this
     function. */
  if(maxcnum>numchecked)
    {
      prev=NULL;
      col=*datall;
      while(col!=NULL)
        {
          /* This column has no data (was only in comments). */
          if(col->status > numchecked)
            {
              /* This column has to be removed/freed. But we have to make
                 some corrections before freeing it:

                  - When 'prev==NULL', then we still haven't got to the
                    first valid element yet and must free this one, but if
                    we do that, then the main pointer to the start of the
                    list will be lost (we will loose all connections with
                    the chain after leaving this loop). So we need to set
                    that to the next element.

                  - When there actually was a previous element
                    ('prev!=NULL'), then we must correct it's next
                    pointer. Otherwise we will break up the chain. */
              if(prev) prev->next=col->next; else *datall=col->next;
              tmp=col->next;
              gal_data_free(col);
              col=tmp;
            }
          else /* Column has data. */
            {
              prev=col;
              col=col->next;
            }
        }
    }

  /* Return the total number of columns/second-img-dimension. */
  if(inplace==0) free(aline);
  return numchecked;
}





/* In the steps above, we read/set the information for each column. But to
   enforce minimum standard requirements on the user, things were allowed
   to be read very loosely, for example some columns can be not defined
   (and will thus be read as a double type), or they don't necessarily have
   to be given in the same order as the table (in which case, the first
   non-commented line provides basic information like how many columns
   there are). So we just pushed each new read/set column into a linked
   list.

   With this function, we convert that badly orderd linked list into a
   clean and ordered array for much more easier random access during the
   selection/reading of the data in the columns.

   After this function, the list is freed. */
static gal_data_t *
txt_infoll_to_array(gal_data_t *datall, size_t *numdata)
{
  size_t i, numc=0, ind;
  gal_data_t *data, *dataarr;

  /* First find the total number of columns. */
  numc=gal_list_data_number(datall);

  /* Conversion to an array is only necessary when there is more than one
     element in the list. */
  if(numc>1)
    {
      /* Allocate the array. */
      dataarr=gal_data_array_calloc(numc);

      /* Put each dataset/column into its proper place in the array. */
      while(datall!=NULL)
        {
          /* Pop the top element. */
          data=gal_list_data_pop(&datall);

          /* The 'status' value is the number of the column (counting from
             1, not 0). */
          ind=data->status-1;

          /* Put all the information from 'data' into the respective part
             of the array. About the pointers, instead of having to
             allocate them again, we will just set them to NULL so
             'gal_data_free' doesn't remove them. */
          dataarr[ind].flag       = data->flag;    data->flag=0;
          dataarr[ind].name       = data->name;    data->name=NULL;
          dataarr[ind].unit       = data->unit;    data->unit=NULL;
          dataarr[ind].array      = data->array;   data->array=NULL;
          dataarr[ind].comment    = data->comment; data->comment=NULL;

          dataarr[ind].ndim       = 0;
          dataarr[ind].size       = 0;
          dataarr[ind].dsize      = NULL;
          dataarr[ind].type       = data->type;
          dataarr[ind].disp_width = data->disp_width;
          dataarr[ind].minmapsize = data->minmapsize; /* "repeat" */

          /* Clean up. */
          gal_data_free(data);
        }
    }
  else
    dataarr=datall;

  /* Set the 'next' pointer of each column. */
  for(i=0;i<numc;++i)
    dataarr[i].next = (i==numc-1) ? NULL : &dataarr[i+1];

  /* Return the array of all column information and put the number of
     columns into the given pointer. */
  *numdata=numc;
  return dataarr;
}





static void
txt_get_info_line(char *line, gal_data_t **datall, char *comm_start,
                  int *firstlinedone, int format, size_t *dsize,
                  int inplace)
{
  size_t numtokens;

  switch( gal_txt_line_stat(line) )
    {
    /* Line is a comment, see if it has formatted information. */
    case GAL_TXT_LINESTAT_COMMENT:
      txt_info_from_comment(line, datall, comm_start, inplace);
      break;

    /* Line is actual data, use it to fill in the gaps. */
    case GAL_TXT_LINESTAT_DATAROW:
      ++dsize[0];
      if(*firstlinedone==0)
        {
          *firstlinedone=1;
          numtokens=txt_info_from_first_row(line, datall, format, inplace);
          if(format==TXT_FORMAT_IMAGE) dsize[1]=numtokens;
        }
      break;

      /* We also have the case of GAL_TXT_LINESTAT_BLANK, but we don't
         need to do anything about it. */
    }
}





/* Return the information about a text file table. If there were no
   readable rows, it will return NULL. */
static gal_data_t *
txt_get_info(char *filename, gal_list_str_t *lines, int format,
             size_t *numdata, size_t *dsize)
{
  FILE *fp;
  gal_list_str_t *tmp;
  gal_data_t *datall=NULL;
  int test, firstlinedone=0;
  char *line, *format_err="empty", *comm_start;
  size_t linelen=10; /* 'linelen' will be increased by 'getline'. */

  /* 'filename' and 'lines' cannot both be non-NULL. */
  test = (filename!=NULL) + (lines!=NULL);
  if( test!=1 )
    error(EXIT_FAILURE, 0, "%s: one of the 'filename' and 'lines' "
          "arguments must be NULL, but they are both %s", __func__,
          test==2 ? "non-NULL" : "NULL");

  /* Set the constant strings. */
  switch(format)
    {
    case TXT_FORMAT_TABLE: format_err="table";comm_start="# Column ";break;
    case TXT_FORMAT_IMAGE: format_err="image";comm_start="# Image "; break;
    default:
      error(EXIT_FAILURE, 0, "%s: code %d not recognized",
            __func__, format);
    }

  /* Initialize the first 'dsize' element. */
  dsize[0]=0;

  /* Parse the file or go over the lines. */
  if(filename)
    {
      /* Open the file. */
      errno=0;
      fp=fopen(filename, "r");
      if(fp==NULL)
        error(EXIT_FAILURE, errno, "%s: couldn't open to read as a plain "
              "text %s (from Gnuastro's '%s')", filename, format_err,
              __func__);


      /* Allocate the space necessary to keep each line as we parse
         it. Note that 'getline' is going to later 'realloc' this space to
         fit the line length. */
      errno=0;
      line=malloc(linelen*sizeof *line);
      if(line==NULL)
        error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for line",
              __func__, linelen*sizeof *line);


      /* Read the comments of the line for possible information about the
         lines, but also confirm/complete the info by parsing the first
         uncommented line. */
      while( getline(&line, &linelen, fp) != -1 )
        txt_get_info_line(line, &datall, comm_start, &firstlinedone,
                          format, dsize, 1);


      /* Clean up and close the file. */
      free(line);
      errno=0;
      if(fclose(fp))
        error(EXIT_FAILURE, errno, "%s: couldn't close file after reading "
              "plain text %s information in %s", filename, format_err,
              __func__);
    }
  else
    {
      for(tmp=lines; tmp!=NULL; tmp=tmp->next)
        txt_get_info_line(tmp->v, &datall, comm_start, &firstlinedone,
                          format, dsize, 0);
    }

  /* The final dataset linked list can have any order (depending on how the
     user gave column information in tables for example). So here, we will
     convert the list into a nicely sorted array, note that this function
     frees list as part of the process. */
  return txt_infoll_to_array(datall, numdata);
}





/* Get the information of each column in a text file. */
gal_data_t *
gal_txt_table_info(char *filename, gal_list_str_t *lines, size_t *numcols,
                   size_t *numrows)
{
  return txt_get_info(filename, lines, TXT_FORMAT_TABLE, numcols, numrows);
}





/* Get the information of a 2D array in a text file. */
gal_data_t *
gal_txt_image_info(char *filename, gal_list_str_t *lines, size_t *numimg,
                   size_t *dsize)
{
  return txt_get_info(filename, lines, TXT_FORMAT_IMAGE, numimg, dsize);
}





















/************************************************************************/
/***************             Read a txt table             ***************/
/************************************************************************/
static gal_data_t *
txt_blocklist_add(gal_data_t *list, gal_data_t *newnode)
{
  newnode->block=list;
  return newnode;
}




#if 0
static size_t
txt_blocklist_number(gal_data_t *list)
{
  size_t num=0;  while(list!=NULL) { ++num; list=list->block; }
  return num;
}
#endif



static void
txt_read_token(gal_data_t *data, gal_data_t *info, char *token,
               size_t i, char *filename, size_t lineno, size_t toknum)
{
  char   *tailptr, emptystr[1]="\0";
  char     **str = data->array, **strb;
  uint8_t    *uc = data->array,   *ucb;
  int8_t      *c = data->array,    *cb;
  uint16_t   *us = data->array,   *usb;
  int16_t     *s = data->array,    *sb;
  uint32_t   *ui = data->array,   *uib;
  int32_t    *ii = data->array,    *ib;
  uint64_t   *ul = data->array,   *ulb;
  int64_t     *l = data->array,    *lb;
  float       *f = data->array,    *fb;
  double      *d = data->array,    *db;

  /* See if this token is blank. */
  int isblankstr = ( info->flag & GAL_TABLEINTERN_FLAG_ARRAY_IS_BLANK_STRING
                     ? ( strcmp(info->array,token)==0 ? 1 : 0 )
                     : 0);

  /* If the string is equal to the given blank string, then just write
     blank and don't bother parsing the token. */
  if(isblankstr)
    {
      switch(data->type)
        {
        case GAL_TYPE_STRING:
          free(str[i]);
          gal_checkset_allocate_copy(GAL_BLANK_STRING, &str[i]); break;
        case GAL_TYPE_UINT8:  uc[i] = GAL_BLANK_UINT8;   break;
        case GAL_TYPE_INT8:    c[i] = GAL_BLANK_INT8;    break;
        case GAL_TYPE_UINT16: us[i] = GAL_BLANK_UINT16;  break;
        case GAL_TYPE_INT16:   s[i] = GAL_BLANK_INT16;   break;
        case GAL_TYPE_UINT32: ui[i] = GAL_BLANK_UINT32;  break;
        case GAL_TYPE_INT32:  ii[i] = GAL_BLANK_INT32;   break;
        case GAL_TYPE_UINT64: ul[i] = GAL_BLANK_UINT64;  break;
        case GAL_TYPE_INT64:   l[i] = GAL_BLANK_INT64;   break;
        case GAL_TYPE_FLOAT32: f[i] = GAL_BLANK_FLOAT32; break;
        case GAL_TYPE_FLOAT64: d[i] = GAL_BLANK_FLOAT64; break;
        default:
          error(EXIT_FAILURE, 0, "%s: type code %d not recognized in "
                "'blankstr' switch", __func__, data->type);
        }
    }

  /* Parse the token into the column's dataset. */
  else
    {
      switch(data->type)
        {
        case GAL_TYPE_STRING:
          gal_checkset_allocate_copy(gal_txt_trim_space(token), &str[i]);
          if( (strb=info->array) && !strcmp( *strb, str[i] ) )
            {
              free(str[i]);
              gal_checkset_allocate_copy(GAL_BLANK_STRING, &str[i]);
            }
          break;

        case GAL_TYPE_UINT8:
          uc[i]=strtol(token, &tailptr, 10);
          if( (ucb=info->array) && *ucb==uc[i] )
            uc[i]=GAL_BLANK_UINT8;
          break;

        case GAL_TYPE_INT8:
          c[i]=strtol(token, &tailptr, 10);
          if( (cb=info->array) && *cb==c[i] )
            c[i]=GAL_BLANK_INT8;
          break;

        case GAL_TYPE_UINT16:
          us[i]=strtol(token, &tailptr, 10);
          if( (usb=info->array) && *usb==us[i] )
            us[i]=GAL_BLANK_UINT16;
          break;

        case GAL_TYPE_INT16:
          s[i]=strtol(token, &tailptr, 10);
          if( (sb=info->array) && *sb==s[i] )
            s[i]=GAL_BLANK_INT16;
          break;

        case GAL_TYPE_UINT32:
          ui[i]=strtol(token, &tailptr, 10);
          if( (uib=info->array) && *uib==ui[i] )
            ui[i]=GAL_BLANK_UINT32;
          break;

        case GAL_TYPE_INT32:
          ii[i]=strtol(token, &tailptr, 10);
          if( (ib=info->array) && *ib==ii[i] )
            ii[i]=GAL_BLANK_INT32;
          break;

        case GAL_TYPE_UINT64:
          ul[i]=strtoul(token, &tailptr, 10);
          if( (ulb=info->array) && *ulb==ul[i] )
            ul[i]=GAL_BLANK_UINT64;
          break;

        case GAL_TYPE_INT64:
          l[i]=strtol(token, &tailptr, 10);
          if( (lb=info->array) && *lb==l[i] )
            l[i]=GAL_BLANK_INT64;
          break;

          /* For the blank value of floating point types, we need to make
             sure it isn't a NaN, because a NaN value will fail on any
             condition check (even '=='). If it isn't NaN, then we can
             compare the values. */
        case GAL_TYPE_FLOAT32:
          f[i]=strtod(token, &tailptr);
          if( (*tailptr=='h' || *tailptr=='d') && isdigit(*(tailptr+1)) )
            {
              f[i] = ( *tailptr=='h'
                       ? gal_units_ra_to_degree(token)
                       : gal_units_dec_to_degree(token) );
              if( !isnan(f[i]) ) tailptr=emptystr;
            }
          if( (fb=info->array)
              && ( (isnan(*fb) && isnan(f[i])) || *fb==f[i] ) )
            f[i]=GAL_BLANK_FLOAT32;
          break;

        /* In astronomical datasets, it can happen that a column is in the
           format of __h__m__s or __d__m__s (where every '_' is a digit),
           in these cases, they are actually coordinates (RA for first, Dec
           for second). */
        case GAL_TYPE_FLOAT64:
          d[i]=strtod(token, &tailptr);
          if( (*tailptr=='h' || *tailptr=='d') && isdigit(*(tailptr+1)) )
            {
              d[i] = ( *tailptr=='h'
                       ? gal_units_ra_to_degree(token)
                       : gal_units_dec_to_degree(token) );
              if( !isnan(d[i]) ) tailptr=emptystr;
            }
          if( (db=info->array)
              && ( (isnan(*db) && isnan(d[i])) || *db==d[i] ) )
            d[i]=GAL_BLANK_FLOAT64;
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
                __func__, data->type);
        }

      /* If a number couldn't be read properly, then report an error. */
      if(data->type!=GAL_TYPE_STRING && *tailptr!='\0')
        {
          if( tailptr!=token
              && isdigit(*(tailptr-1))
              && *tailptr==':'
              && isdigit(*(tailptr+1)) )
            error_at_line(EXIT_FAILURE, 0, filename, lineno, "token %zu "
                          "('%s') couldn't be read as a '%s' number.\n\n"
                          "If it was meant to be celestial coordinates "
                          "(RA or Dec), please use the '_h_m_' format "
                          "for RA or '_d_m_' for Dec. The '_:_:_' format "
                          "is ambiguous (can be used for both RA and "
                          "Dec). Alternatively, you can use the column "
                          "arithmetic operators 'ra-to-degree' or "
                          "'dec-to-degree' of 'asttable' which also "
                          "accept the '_:_:_' format. However, the "
                          "'ra-to-degree' or 'dec-to-degree' operators "
                          "require the column to be identified as a "
                          "string with metadata. Please run the command "
                          "below to learn more about column metadata and "
                          "columns with string contents (it is easier to "
                          "just use the '_h_m_' or '_d_m_' formats which "
                          "will be automatically converted to degrees "
                          "without any operators or metadata):\n\n"
                          "   $ info gnuastro \"Gnuastro text table\"",
                          toknum, token,
                          gal_type_name(data->type, 1) );
          else
            error_at_line(EXIT_FAILURE, 0, filename, lineno, "column %zu "
                          "('%s') couldn't be read as a '%s' number",
                          toknum, token, gal_type_name(data->type, 1) );
        }
    }
}





static void
txt_fill(char *in_line, gal_data_t **tokeninout, size_t ntokforout,
         gal_data_t **tokenininfo, size_t *tokenvecind,
         size_t rowind, char *filename, size_t lineno, int inplace,
         int format)
{
  gal_data_t *otmp;
  int notenoughcols=0;
  size_t len, n=0, ind, strwidth;
  char *end, *line, *aline, *tmpstr;

  /* Make a copy of the input line if necessary. */
  if(inplace) line=in_line;
  else gal_checkset_allocate_copy(in_line, &line);
  end=line+strlen(line);
  aline=line; /* The 'line' pointer will be shifted. */

  /* Remove the new-line character from the line. For more, see the top the
     explanations in 'txt_info_from_first_row': 13 is the ASCII code for
     the carriage return. */
  if( end>line+2 && *(end-2)==13 ) *(end-2)='\0';
  else if( *(end-1)=='\n' )        *(end-1)='\0';


  /* Start parsing the line, token by token. Break out of the parsing if we
     don't need the columns any more. The table might contain many more
     columns, but when they aren't needed, there is no point in tokenizing
     them. Note that 'ntokforout' is the number of the last input token
     that is used in the output, so it is inclusive. */
  while(n<=ntokforout)
    {
      /* Set the pointer to the start of this token/column. See
         explanations in 'txt_info_from_first_row'. Note that an image has
         a single 'info' element for the whole array, while a table has one
         for each column. */
      if( format==TXT_FORMAT_TABLE
          && tokenininfo[n]->type == GAL_TYPE_STRING )
        {
          /* Remove any delimiters and stop at the first non-delimiter. If
             we have reached the end of the line then its an error, because
             we were expecting a column here (recall that empty lines are
             skipped before reaching this point). */
          while(isspace(*line) || *line==',') ++line;
          if(*line=='\0') {notenoughcols=1; break;}

          /* We are at the start of the string. Allocate space for, and
             copy the necessary number of characters into the 'tmpstr'
             string. We need to allocate this because the string column may
             be immediately (next character) followed by the next
             column. This leaves us no space to copy the '\0'
             character. Therefore we will add '\0' after 'strncpy'.

             Also, it may happen (for example with plain-text editors that
             remove trailing white space) that the width defined for the
             last column becomes larger than the actual length of the
             line. We should therefore first check how many characters we
             should actually copy (may be less than 'disp_width'). See
             https://savannah.gnu.org/bugs/index.php?62720

             If this token should be used, then its 'tokeninout' will be
             non-NULL. */
          strwidth=tokenininfo[n]->disp_width;
          if(tokeninout[n])
            {
              /* Copy the full string column into a "standard" string
                 (which terminates with a '\0'). */
              len = (line+strwidth)<end ? strwidth : end-line;
              tmpstr=gal_pointer_allocate(GAL_TYPE_UINT8, len+1, 0,
                                          __func__, "tmpstr");
              strncpy(tmpstr, line, len);
              tmpstr[len]='\0';

              /* Write it into all the output columns that need it (recall
                 that if more than one output column needs a token, it is
                 placed in the 'block' elements, 'next' is already
                 assocated to the next column's pointer). */
              for(otmp=tokeninout[n]; otmp!=NULL; otmp=otmp->block)
                txt_read_token(otmp, tokenininfo[n], tmpstr, rowind,
                               filename, lineno, n);

              /* For a check.
              printf("%s: Wrote '%s' into memory\n", __func__, tmpstr);
              */

              /* Clean up. */
              free(tmpstr);
            }

          /* Increment the line pointer to the end of this string. */
          line += strwidth;
        }
      else
        {
          /* If we have reached the end of the line, then 'strtok_r' will
             return a NULL pointer. */
          tmpstr=strtok_r(n==0?line:NULL, GAL_TXT_DELIMITERS, &line);
          if(tmpstr==NULL) {notenoughcols=1; break;}

          /* Convert and write the string to the desired output. */
          if(format==TXT_FORMAT_TABLE)
            {
              ind = rowind * tokenininfo[n]->minmapsize + tokenvecind[n];
              for(otmp=tokeninout[n]; otmp!=NULL; otmp=otmp->block)
                txt_read_token(otmp, tokenininfo[n], tmpstr, ind,
                               filename, lineno, n);
            }
          else /* An image */
            txt_read_token(tokeninout[0], tokenininfo[0], tmpstr,
                           rowind*tokeninout[0]->dsize[1]+n,
                           filename, lineno, n);

          /* For a check.
          printf("%s: Wrote '%s' into memory\n", __func__, tmpstr);
          */
        }

      /* Increment the token counter. */
      ++n;
    }

  /* Report an error if there weren't enough columns. */
  if(notenoughcols)
    error_at_line(EXIT_FAILURE, 0, filename, lineno, "not enough columns "
                  "in this line");

  /* Clean up. */
  if(aline!=in_line) free(aline);
}





/* Allocate the datasets to help parse each token. */
static void
txt_read_prepare_alloc(gal_data_t ***tokeninout_out,
                       gal_data_t ***tokenininfo_out,
                       size_t **tokenvecind_out,
                       size_t number)
{
  size_t *tokenvecind;
  gal_data_t **tokeninout, **tokenininfo;

  errno=0;
  *tokeninout_out=tokeninout=calloc(number, sizeof *tokeninout);
  if(tokeninout==NULL)
    error(EXIT_FAILURE, errno, "%s: couldn't allocate %zu bytes for "
          "'tokeninout'", __func__, number * sizeof *tokeninout);

  errno=0;
  *tokenininfo_out=tokenininfo=calloc(number, sizeof *tokenininfo);
  if(tokenininfo==NULL)
    error(EXIT_FAILURE, errno, "%s: couldn't allocate %zu bytes for "
          "'tokenininfo'", __func__, number * sizeof *tokenininfo);

  if(tokenvecind_out)
    {
      errno=0;
      *tokenvecind_out=tokenvecind=calloc(number, sizeof *tokenvecind);
      if(tokenvecind==NULL)
        error(EXIT_FAILURE, errno, "%s: couldn't allocate %zu bytes for "
              "'tokenvecind'", __func__, number * sizeof *tokenvecind);
    }
}





static gal_data_t *
txt_read_prepare_table(gal_data_t *info, size_t *indsize,
                       gal_list_sizet_t *indexll, size_t minmapsize,
                       int quietmmap, gal_data_t ***tokeninout_out,
                       size_t *ntokforout, gal_data_t ***tokenininfo_out,
                       size_t **tokenvecind_out)
{
  size_t *tokenvecind;
  gal_list_sizet_t *ind;
  size_t i, r, ndim, colc, tokc, repeat, ntokens=0, colendtok;
  size_t dsize[2]={indsize[0]?indsize[0]:1,GAL_BLANK_SIZE_T};
  gal_data_t *tmp, *idata, **tokeninout, **tokenininfo, *out=NULL;

  /* Find how many tokens (columns, but before accounting for vectors)
     there are in the input. Then allocate an array of 'gal_data_t *'
     so we can keep track of which pre-vector-column should be put into
     which output dataset. */
  for(tmp=info; tmp!=NULL; tmp=tmp->next) ntokens+=tmp->minmapsize;
  txt_read_prepare_alloc(tokeninout_out, tokenininfo_out, tokenvecind_out,
                         ntokens);
  tokenininfo=*tokenininfo_out;
  tokenvecind=*tokenvecind_out;
  tokeninout=*tokeninout_out;

  /* Go over the requested columns from their index. */
  for(ind=indexll; ind!=NULL; ind=ind->next)
    {
      /* To help in reading. */
      idata=&info[ind->v];

      /* Allocate the necessary space. If there are no rows, we are setting
         a 1-element array to avoid any allocation errors (minmapsize,
         which holds the "repeat", will be 1 for non-vector column). Then
         we are freeing the allocated spaces and correcting the sizes. */
      ndim = (repeat=dsize[1]=idata->minmapsize)==1 ? 1 : 2;
      gal_list_data_add_alloc(&out, NULL, idata->type, ndim, dsize,
                              NULL, 0, minmapsize, quietmmap,
                              idata->name, idata->unit, idata->comment);
      out->disp_width=idata->disp_width;

      /* If there were no actual rows ('numrows'==0), free the
         allocated spaces and correct the size. */
      if(indsize[0]==0)
        {
          out->size=0;
          free(out->array);
          free(out->dsize);
          out->dsize=out->array=NULL;
        }

      /* Find the input token (of each line) that each input column starts
         at. This needs special attention because vector columns can have
         multiple tokens in one column. */
      colc=tokc=0;
      for(tmp=info; colc<ind->v; tmp=tmp->next)
        { tokc+=tmp->minmapsize; ++colc; }

      /* For a check:
      printf("%s: input col %zu starts at token %-2zu and is %zu token(s) "
             "wide [counts from 1]\n", __func__, ind->v+1, tokc+1,
             repeat);
      */

      /* Set the pointer of this output dataset in the 'tokeninout'
         array. If this token should be used in multiple output columns,
         then add them to the 'block' pointer (which is not relevant here,
         while 'next' is used to link the various columns). Note that all
         elements of 'tokeninout' have been initialized to NULL with the
         'calloc' function above, so we can safely use it as a list. */
      for(i=0;i<repeat;++i)
        tokeninout[tokc+i]=txt_blocklist_add(tokeninout[tokc+i], out);
    }

  /* Reverse the list to be in the same order as the output. */
  gal_list_data_reverse(&out);

  /* Find the last input token that is useful for the output (to avoid
     unnecessarily tokenizing and parsing the rest each line). If this
     column shouldn't be read, then just put a pointer to its 'info'
     structure, so we still know its metadata (to skip in the case of
     strings or vectors). */
  colc=r=0;
  colendtok=info[colc].minmapsize;
  for(tokc=0;tokc<ntokens;++tokc)
    {
      /* If we have reached the last token of this column, then increment
         the column counter and its last token. */
      if(tokc>=colendtok) {r=0; ++colc; colendtok+=info[colc].minmapsize;}

      /* For a check:
      printf("Token %-3zu belongs to column %zu\n", tokc+1, colc+1);
      */

      /* If this token should be read, everything has already been
         allocated above, so just keep its counter to find the last
         necessary token. */
      if(tokeninout[tokc]) {*ntokforout=tokc; tokenvecind[tokc]=r;}
      else tokenvecind[tokc]=GAL_BLANK_SIZE_T;

      /* Set the pointer to the information list (necessary for all
         columns, whether they are to be used or not). */
      tokenininfo[tokc]=&info[colc];

      /* Increment the repeat counter. */
      ++r;
    }

  /* For a check (also un-comment the 'txt_blocklist_number' function).
  printf("Input token --> number of output columns it is written to "
         "[counts from 1]\n");
  for(i=0;i<ntokens;++i)
    printf("%-12zu --> %-5zu (vector: %zu)\n", i+1,
           txt_blocklist_number(tokeninout[i]), tokenvecind[i]);
  printf("Last usable token ('ntokforout'): %zu\n", *ntokforout+1);
  */
  return out;
}





static gal_data_t *
txt_read_prepare_img(gal_data_t *info, size_t *indsize,
                     size_t minmapsize, int quietmmap,
                     gal_data_t ***tokeninout_out, size_t *ntokforout,
                     gal_data_t ***tokenininfo_out)
{
  gal_data_t *out;

  /* Make sure that the input isn't a list. */
  if(info->next)
    error(EXIT_FAILURE, 0, "%s: currently reading only one image (2d "
          "array) from a text file is possible, the 'info' input has "
          "more than one element", __func__);

  /* Allocate the output. */
  out=gal_data_alloc(NULL, info->type, 2, indsize, NULL, 0,
                     minmapsize, quietmmap, info->name, info->unit,
                     info->comment);

  /* Allocate the token reading pointers, set them and return. */
  txt_read_prepare_alloc(tokeninout_out, tokenininfo_out, NULL, 1);
  *ntokforout=out->dsize[1]-1;  /* Token counting begins from 0. */
  (*tokenininfo_out)[0]=info;
  (*tokeninout_out)[0]=out;
  return out;
}





static gal_data_t *
txt_read_prepare(gal_data_t *info, size_t *indsize,
                 gal_list_sizet_t *indexll, size_t minmapsize,
                 int quietmmap, int format, char **line,
                 size_t linelen, gal_data_t ***tokeninout,
                 size_t *ntokforout, gal_data_t ***tokenininfo,
                 size_t **tokenvecind)
{
  gal_data_t *out;

  /* Allocate the output. */
  switch(format)
    {
    case TXT_FORMAT_TABLE:
      out=txt_read_prepare_table(info, indsize, indexll, minmapsize,
                                 quietmmap, tokeninout, ntokforout,
                                 tokenininfo, tokenvecind);
      break;
    case TXT_FORMAT_IMAGE:
      *tokenvecind=NULL;     /* Not necessary in an image.         */
      out=txt_read_prepare_img(info, indsize, minmapsize, quietmmap,
                               tokeninout, ntokforout, tokenininfo);
      break;
    default: /* Format not recognized. */
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
            "fix the problem. The format code %d is not recognized",
            __func__, PACKAGE_BUGREPORT, format);
    }

  /* Allocate the space necessary to keep a copy of each line as we parse
     it. Note that 'getline' is going to later 'realloc' this space to fit
     the line length. */
  errno=0;
  *line=malloc(linelen*sizeof *line);
  if(*line==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for 'line'",
          __func__, linelen*sizeof **line);

  /* Return the output dataset. */
  return out;
}





static gal_data_t *
txt_read(char *filename, gal_list_str_t *lines, size_t *indsize,
         gal_data_t *info, gal_list_sizet_t *indexll, size_t minmapsize,
         int quietmmap, int format)
{
  FILE *fp;
  int test;
  char *line;
  gal_list_str_t *tmp;
  size_t ntokforout=0, rowind=0, lineno=0, *tokenvecind;
  gal_data_t *out=NULL, *ocol, **tokeninout, **tokenininfo;
  size_t linelen=10;        /* 'linelen' will be increased by 'getline'. */

  /* 'filename' and 'lines' cannot both be non-NULL. */
  test = (filename!=NULL) + (lines!=NULL);
  if( test!=1 )
    error(EXIT_FAILURE, 0, "%s: one of the 'filename' and 'lines' "
          "arguments must be NULL, but they are both %s", __func__,
          test==2 ? "non-NULL" : "NULL");

  /* Necessary preparations/allocations. */
  out=txt_read_prepare(info, indsize, indexll, minmapsize, quietmmap,
                       format, &line, linelen, &tokeninout, &ntokforout,
                       &tokenininfo, &tokenvecind);

  /* Read the input line by line. */
  if(filename) /* Input from a file. */
    {
      /* Open the file. */
      errno=0;
      fp=fopen(filename, "r");
      if(fp==NULL)
        error(EXIT_FAILURE, errno, "%s: couldn't open to read as a text "
              "table in %s", filename, __func__);

      /* Read the file, line by line. */
      while( getline(&line, &linelen, fp) != -1 )
        {
          ++lineno;
          if( gal_txt_line_stat(line) == GAL_TXT_LINESTAT_DATAROW )
            txt_fill(line, tokeninout, ntokforout, tokenininfo,
                     tokenvecind, rowind++, filename, lineno, 1, format);
        }

      /* Clean up and close the file. */
      errno=0;
      if(fclose(fp))
        error(EXIT_FAILURE, errno, "%s: couldn't close file after reading "
              "ASCII table information in %s", filename, __func__);
    }

  else /* Input from standard input. */
    for(tmp=lines; tmp!=NULL; tmp=tmp->next)
      {
        /* To read for standard output, we are setting 'inplace' to zero
           because there may only be a single copy of the input. */
        ++lineno;
        if( gal_txt_line_stat(tmp->v) == GAL_TXT_LINESTAT_DATAROW )
          txt_fill(tmp->v, tokeninout, ntokforout, tokenininfo,
                   tokenvecind, rowind++, filename, lineno, 0, format);
      }

  /* The 'block' pointer of the output datasets has been been used above if
     an input column was used more than once in the output. It is no longer
     necessary and being non-NULL can cause problems for the users of the
     columns (because it has a special meaning in Gnuastro, outside of
     tables, see 'lib/data.h'), so we should set them all to NULL. */
  for(ocol=out;ocol!=NULL;ocol=ocol->next) ocol->block=NULL;

  /* Clean up the allocations of 'txt_read_prepare' and return. */
  if(format==TXT_FORMAT_TABLE) free(tokeninout);
  free(tokenininfo);
  free(tokenvecind);
  free(line);
  return out;
}





gal_data_t *
gal_txt_table_read(char *filename, gal_list_str_t *lines, size_t numrows,
                   gal_data_t *colinfo, gal_list_sizet_t *indexll,
                   size_t minmapsize, int quietmmap)
{
  return txt_read(filename, lines, &numrows, colinfo, indexll, minmapsize,
                  quietmmap, TXT_FORMAT_TABLE);
}





gal_data_t *
gal_txt_image_read(char *filename, gal_list_str_t *lines,
                   size_t minmapsize, int quietmmap)
{
  size_t numimg, dsize[2];
  gal_data_t *img, *imginfo;
  gal_list_sizet_t *indexll=NULL;

  /* Get the image information. */
  imginfo=gal_txt_image_info(filename, lines, &numimg, dsize);

  /* Read the table. */
  img=txt_read(filename, lines, dsize, imginfo, indexll, minmapsize,
               quietmmap, TXT_FORMAT_IMAGE);

  /* Clean up and return. */
  gal_data_free(imginfo);
  return img;
}




/* See if there is anything in the standard input already. This function is
   modeled on the solution provided in:

   https://stackoverflow.com/questions/3711830/set-a-timeout-for-reading-stdin */
static int
txt_stdin_has_contents(long timeout_microsec)
{
  int sout;
  fd_set fds;
  struct timeval tv;

  /* Set the timeout time. We need to put the number of seconds in 'tv_sec'
     and the remaining microseconds in 'tv_usec' (this cannot be larger
     than one million, otherwise 'select' is going to abort with an
     error). */
  tv.tv_sec  = timeout_microsec/1000000;
  tv.tv_usec = timeout_microsec%1000000;

  /* Initialize 'fd_set'. */
  FD_ZERO(&fds);

  /* Set standard input (STDIN_FILENO is 0) as the FD that must be read. */
  FD_SET(STDIN_FILENO, &fds);

  /* From Glibc: "The 'select' function blocks the calling process until
     there is activity on any of the specified sets of file descriptors, or
     until the timeout period has expired". 'select' takes the last file
     descriptor value+1 in the 'FD_SET' above as first argument. If the
     second (reading), third (writing) and fourth (exception) are not NULL,
     then it will check for the respective property(s).

     When successful (file descriptor has input for the desired action),
     'select' will return 1. When the timeout has been reached, it will
     return 0 and when there was an error it will return -1. If there is an
     error, we'll abort the program and ask the user to contact us (its a
     bug). */
  errno=0;
  sout=select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  if(sout==-1)
    error(EXIT_FAILURE, errno, "%s: a bug! Please contact us at '%s' "
          "to fix the problem. The 'select' function has detected an "
          "error", __func__, PACKAGE_BUGREPORT);

  /* By this point, 'sout' only has a value of 1 (stdin is ready for
     reading) or 0 (timeout was reached with no change, so it should't be
     used). So simply return the value of 'sout'. */
  return sout;
}




/* Read each line of the standard input into a linked list of strings. */
gal_list_str_t *
gal_txt_stdin_read(long timeout_microsec)
{
  char *line;
  gal_list_str_t *out=NULL;
  size_t lineno=0, linelen=10;/* 'getline' will increase 'linelen'. */

  /* Only continue if standard input has any contents. */
  if( txt_stdin_has_contents(timeout_microsec) )
    {
      /* Allocate the space necessary to keep a copy of each line as we
         parse it. Note that 'getline' is going to later 'realloc' this
         space to fit the line length. */
      errno=0;
      line=malloc(linelen*sizeof *line);
      if(line==NULL)
        error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for 'line'",
              __func__, linelen*sizeof *line);

      /* Read the whole standard input. We are using getline because it can
         deal with a 'NULL' in the input, while also handing allocation
         issues while reading (allocating by line, not by a fixed buffer
         size). */
      while( getline(&line, &linelen, stdin) != -1 )
        {
          /* To help in reporting (when necessary), keep a count of how
             many lines we have. */
          ++lineno;

          /* Add the line to the output list. */
          gal_list_str_add(&out, line, 1);
        }

      /* Reverse the list (to be the same order as input). */
      gal_list_str_reverse(&out);

      /* Clean up. */
      free(line);
    }

  /* Return the result. */
  return out;
}





gal_list_str_t *
gal_txt_read_to_list(char *filename)
{
  FILE *fp;
  gal_list_str_t *out=NULL;
  char *line, *format_err="empty";
  size_t linelen=10; /* 'linelen' will be increased by 'getline'. */

  /* Make sure an input filename is given. */
  if(filename==NULL) return NULL;

  /* Open the file. */
  errno=0;
  fp=fopen(filename, "r");
  if(fp==NULL)
    error(EXIT_FAILURE, errno, "%s: couldn't open to read as a plain "
          "text %s (from Gnuastro's '%s')", filename, format_err,
          __func__);

  /* Allocate the space necessary to keep each line as we parse it. Note
     that 'getline' is going to later 'realloc' this space to fit the line
     length. */
  errno=0;
  line=malloc(linelen*sizeof *line);
  if(line==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for line",
          __func__, linelen*sizeof *line);


  /* Read the comments of the line for possible information about the
     lines, but also confirm/complete the info by parsing the first
     uncommented line. */
  while( getline(&line, &linelen, fp) != -1 )
    {
      if(out==NULL)
        out=gal_list_str_extract(line);
      else
        gal_list_str_last(out)->next=gal_list_str_extract(line);
    }

  /* Return the output. */
  return out;
}



















/************************************************************************/
/***************              Write to txt                ***************/
/************************************************************************/
static void
txt_fmts_for_printf_norm(gal_data_t *data, char *fmta, char *lng,
                         char *fmt, int leftadjust)
{
  /* Strings should be treated as if they don't have negative. */
  int hasneg = ( data->type==GAL_TYPE_STRING
                 ? 0
                 : gal_statistics_has_negative(data) );

  /* The space in the end of 'fmts[i*FMTS_COLS]' is to ensure that the
     columns don't merge, even if the printed string is larger than the
     expected width. */
  if(data->disp_precision == GAL_BLANK_INT)
    sprintf(fmta, hasneg ? "%% %s%d%s%s " : "%%%s%d%s%s ",
            leftadjust ? "-" : "",
            data->disp_width, lng, fmt);
  else
    sprintf(fmta, hasneg ? "%% %s%d.%d%s%s " : "%%%s%d.%d%s%s ",
            leftadjust ? "-" : "",
            data->disp_width, data->disp_precision, lng, fmt);
}





static void
txt_fmts_for_printf_last(gal_data_t *data, char *fmta, char *lng,
                         char *fmt)
{
  /* Strings should be treated as if they don't have negative. */
  int hasneg = ( data->type==GAL_TYPE_STRING
                 ? 0
                 : gal_statistics_has_negative(data) );

  /* See 'txt_fmts_for_printf_norm'. */
  if(data->disp_precision == GAL_BLANK_INT)
    sprintf(fmta, hasneg ? "%% %s%s" : "%%%s%s", lng, fmt);
  else
    sprintf(fmta, hasneg ? "%% .%d%s%s" : "%%.%d%s%s",
            data->disp_precision, lng, fmt);
}





/* Make an array of 3 strings for each column (in practice a two
   dimensional array with 3 columns in a row for each input column). The
   columns are:

     Column 0: Printf format string.
     Column 1: Gnuastro type string (in plain text format).
     Column 2: Blank value string.
     Column 3: Format for last vector column. */
#define FMTS_COLS 4
static char **
txt_fmts_for_printf(gal_data_t *datall, int leftadjust, int tab0_img1)
{
  char **fmts;
  gal_data_t *data;
  size_t i=0, num=0;
  char fmt[2], lng[3];

  /* Allocate space for the output. */
  for(data=datall;data!=NULL;data=data->next) ++num;
  errno=0;
  fmts=malloc(FMTS_COLS*num*sizeof *fmts);
  if(fmts==NULL)
    error(EXIT_FAILURE, errno, "%s: %zu bytes for fmts",
          __func__, FMTS_COLS*num*sizeof *fmts);

  /* Go over all the columns and make their formats. */
  for(data=datall;data!=NULL;data=data->next)
    {
      /* First allocate the necessary space to keep the string. */
      errno=0;
      fmts[ i*FMTS_COLS   ] = malloc(GAL_TXT_MAX_FMT_LENGTH*sizeof **fmts);
      fmts[ i*FMTS_COLS+1 ] = malloc(GAL_TXT_MAX_FMT_LENGTH*sizeof **fmts);
      fmts[ i*FMTS_COLS+3 ] = malloc(GAL_TXT_MAX_FMT_LENGTH*sizeof **fmts);
      if( fmts[i*FMTS_COLS]==NULL
          || fmts[i*FMTS_COLS+1]==NULL
          || fmts[i*FMTS_COLS+3]==NULL )
        error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for "
              "fmts[%zu] or fmts[%zu]", __func__,
              GAL_TXT_MAX_FMT_LENGTH*sizeof **fmts, i*FMTS_COLS,
              i*FMTS_COLS+1);

      /* If we have a blank value, get the blank value as a string and
         adjust the width. */
      fmts[ i*FMTS_COLS+2 ] = ( gal_blank_present(data, 0)
                                ? gal_blank_as_string(data->type, 0)
                                : NULL );

      /* Fill in the printing paramters. */
      gal_tableintern_col_print_info(data, GAL_TABLE_FORMAT_TXT, fmt, lng);

      /* Adjust the width if a blank string was defined. */
      if(fmts[i*FMTS_COLS+2])
        data->disp_width = ( strlen(fmts[i*FMTS_COLS+2]) > data->disp_width
                             ? strlen(fmts[i*FMTS_COLS+2])
                             : data->disp_width );

      /* Set the string for the Gnuastro type. For strings, we also need to
         write the maximum number of characters. */
      if(data->type==GAL_TYPE_STRING)
        sprintf(fmts[i*FMTS_COLS+1], "%s%d", gal_type_name(data->type, 0),
                data->disp_width);
      else
        strcpy(fmts[i*FMTS_COLS+1], gal_type_name(data->type, 0));

      /* Print the result into the allocated string. */
      if(data->next) /* Not last column. */
        txt_fmts_for_printf_norm(data, fmts[i*FMTS_COLS], lng, fmt,
                                 leftadjust);
      else /* Last column. */
        {
          /* For vector columns in a table that are also the last column,
             we need both the normal format and the last column format.*/
          if(data->ndim==2)
            {
              txt_fmts_for_printf_norm(data, fmts[i*FMTS_COLS], lng, fmt,
                                       leftadjust);
              txt_fmts_for_printf_last(data, fmts[i*FMTS_COLS+3], lng, fmt);
            }
          else /* Last column is not a vector. */
            {
              txt_fmts_for_printf_last(data, fmts[i*FMTS_COLS], lng, fmt);
              fmts[i*FMTS_COLS+3][0]='\0';
            }
        }

      /* Increment the column counter. */
      ++i;
    }

  /* Return the array. */
  return fmts;
}





static void
txt_print_value(FILE *fp, gal_data_t *data, size_t ind, char *fmt)
{
  void *a=data->array;

  switch(data->type)
    {
      /* Numerical types. */
    case GAL_TYPE_UINT8:   fprintf(fp, fmt, ((uint8_t *) a)[ind]); break;
    case GAL_TYPE_INT8:    fprintf(fp, fmt, ((int8_t *)  a)[ind]); break;
    case GAL_TYPE_UINT16:  fprintf(fp, fmt, ((uint16_t *)a)[ind]); break;
    case GAL_TYPE_INT16:   fprintf(fp, fmt, ((int16_t *) a)[ind]); break;
    case GAL_TYPE_UINT32:  fprintf(fp, fmt, ((uint32_t *)a)[ind]); break;
    case GAL_TYPE_INT32:   fprintf(fp, fmt, ((int32_t *) a)[ind]); break;
    case GAL_TYPE_UINT64:  fprintf(fp, fmt, ((uint64_t *)a)[ind]); break;
    case GAL_TYPE_INT64:   fprintf(fp, fmt, ((int64_t *) a)[ind]); break;
    case GAL_TYPE_FLOAT32: fprintf(fp, fmt, ((float *)   a)[ind]); break;
    case GAL_TYPE_FLOAT64: fprintf(fp, fmt, ((double *)  a)[ind]); break;

      /* Special consideration for strings. */
    case GAL_TYPE_STRING:
      if( !strcmp( ((char **)a)[ind], GAL_BLANK_STRING ) )
        fprintf(fp, fmt, GAL_BLANK_STRING);
      else
        fprintf(fp, fmt, ((char **)a)[ind]);
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, data->type);
    }
}





static void
txt_write_metadata(FILE *fp, gal_data_t *datall, char **fmts,
                   int tab0_img1)
{
  gal_data_t *data;
  size_t i, j, num=0;
  char *c, *tmp, *nstr, *tstr;
  int nlen, twt, nw=0, uw=0, tw=0, bw=0;

  /* Get the maximum width for each information field. */
  for(data=datall;data!=NULL;data=data->next)
    {
      ++num;
      if( data->name && strlen(data->name)>nw ) nw=strlen(data->name);
      if( data->unit && strlen(data->unit)>uw ) uw=strlen(data->unit);
    }
  data=datall;
  for(i=0;i<num;++i)
    {
      /* Width of blank element. */
      if( (tmp=fmts[ i*FMTS_COLS+2 ]) )      /* If it isn't NULL. */
        bw = strlen(tmp) > bw ? strlen(tmp) : bw;

      /* Width of type element. */
      if( (tmp=fmts[ i*FMTS_COLS+1 ]) )      /* If it isn't NULL. */
        {
          twt=strlen(tmp);
          /* A 2D col with second dim of 1 is just a normal 1D col. */
          if(tab0_img1==0 && data->ndim==2 && data->dsize[1]>1)
            twt+=(int)(log10(data->dsize[1]))+1+2; /* +1 for 0 to 10. */
          tw = twt > tw ? twt : tw;                /* +2 for the '()'.*/
        }

      /* Go onto the next data element. */
      data=data->next;
    }

  /* When there are more than 9 columns, we don't want to have cases
     like '# Column 1 :' (note the space between '1' and ':', this
     space won't exist for the 2 digit colum numbers).

     To do this, we are first allocating and printing a string long
     enough to keep the final column's 'N:'. Then, for each column, we
     print only the number into the allocated space and put the ':' in
     manually immediately after the number. Note that the initial
     'asprintf' put a '\0' in the allocated space, so we can safely
     over-write the one that 'sprintf' puts with a ':' for the columns
     that have the same number of digits as the final column. */
  i=0;
  if( asprintf(&nstr, "%zu:", num)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
  nlen=strlen(nstr);
  for(data=datall; data!=NULL; data=data->next)
    {
      /* Initialize the string to an empty string, print the number into
         the number string, then add the ':' immediately after the
         number. */
      for(c=nstr;*c!='\0';++c) {*c=' ';} sprintf(nstr, "%zu", i+1);
      for(j=1;j<nlen;++j)
        if(!isdigit(nstr[j])) nstr[j] = isdigit(nstr[j-1]) ? ':' : ' ';

      /* For the type, we need to account for vector clumns. Note that a 2D
         col with second dim of 1 is just a normal 1D col. */
      if(tab0_img1==0 && data->ndim==2 && data->dsize[1]>1)
        { if( asprintf(&tstr, "%s(%zu)", fmts[i*FMTS_COLS+1],
                       data->dsize[1])<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__); }
      else
        { if( asprintf(&tstr, "%s", fmts[i*FMTS_COLS+1])<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__); }

      /* Now print the full information. */
      fprintf(fp, "# %s %s %-*s [%-*s,%-*s,%-*s] %s\n",
              tab0_img1 ? "Image" : "Column", nstr,
              nw, data->name ? data->name    : "",
              uw, data->unit ? data->unit    : "",
              tw, fmts[i*FMTS_COLS+1] ? tstr : "",
              bw, fmts[i*FMTS_COLS+2] ? fmts[i*FMTS_COLS+2] : "",
              data->comment ? data->comment : "");
      free(tstr);
      ++i;
    }

  /* Clean up and return. */
  free(nstr);
}





static void
txt_write_keys(FILE *fp, struct gal_fits_list_key_t *keylist,
               int freekeys)
{
  char *ending;
  char *valuestr;
  gal_fits_list_key_t *tmp, *ttmp;

  tmp=keylist;
  while(tmp!=NULL)
    {
      /* If a title is requested, only put a title. */
      if(tmp->title)
        {
          fprintf(fp, "# -------------\n# %s\n# -------------\n",
                  tmp->title);
          if(tmp->tfree) free(tmp->title);
        }
      else if (tmp->fullcomment)
        {
          fprintf(fp, "# %s\n", tmp->fullcomment);
          if(tmp->fcfree) free(tmp->fullcomment);
        }
      else
        {
          /* For a string type, we need to return a pointer to the
             string. */
          valuestr = ( tmp->type==GAL_TYPE_STRING
                       ? tmp->value
                       : gal_type_to_string(tmp->value, tmp->type, 1) );

          /* If a comment is requested, parepare it. */
          ending=NULL;
          if(tmp->unit)
            {
              if( asprintf(&ending, " / [%s] %s", tmp->unit,
                           tmp->comment?tmp->comment:"")==-1 )
                error(EXIT_FAILURE, errno, "%s: asprintf error for name",
                      __func__);
            }
          else if(tmp->comment)
            {
              if( asprintf(&ending, " / %s", tmp->comment)==-1 )
                error(EXIT_FAILURE, errno, "%s: asprintf error for name",
                      __func__);
            }

          /* Write the keyword value. */
          fprintf(fp, "# [key] %s: %s%s\n", tmp->keyname,
                  valuestr, ending?ending:"");

          /* Clean up. */
          if(ending)     free(ending);
          if(tmp->kfree) free(tmp->keyname);
          if(tmp->vfree) free(tmp->value);
          if(tmp->cfree) free(tmp->comment);
          if(tmp->ufree) free(tmp->unit);
        }

      /* Keep the pointer to the next keyword and free the allocated
         space for this keyword. */
      ttmp=tmp->next;
      free(tmp);
      tmp=ttmp;
    }
}





void
gal_txt_write(gal_data_t *input, struct gal_fits_list_key_t *keylist,
              gal_list_str_t *comment, char *filename,
              uint8_t colinfoinstdout, int tab0_img1, int freekeys)
{
  FILE *fp;
  char **fmts;
  gal_list_str_t *strt;
  size_t i, j, k, num=0, d1;
  gal_data_t *data, *nextimg=NULL;

  /* Make sure input is valid. */
  if(input==NULL) error(EXIT_FAILURE, 0, "%s: input is NULL", __func__);


  /* Currently only 1 and 2 dimension datasets are acceptable. */
  if( input->ndim!=1 && input->ndim!=2 )
    error(EXIT_FAILURE, 0, "%s: only 1 and 2 dimensional datasets are "
          "currently supported. The input dataset has %zu dimensions",
          __func__, input->ndim);


  /* For an image, we currently don't accept a list, we can only print one
     column. So keep the next pointer separately and restore it after the
     job of this function is finished. */
  if(tab0_img1)
    {
      nextimg=input->next;
      input->next=NULL;
    }


  /* Find the number of columns, do a small sanity check, and get the
     maximum width of the name and unit string if they are present. */
  for(data=input;data!=NULL;data=data->next)
    {
      /* Count. */
      ++num;

      /* Check if the dimensionality and size is the same for all the
         elements. The 'input->dsize && data->dsize' conditions are because
         we may have fully empty tables (where 'dsize==NULL'). In this
         case, we want to continue with printing, and there is no
         problem. */
      if( input!=data && input->dsize && data->dsize
          && input->dsize[0]!=data->dsize[0] )
        error(EXIT_FAILURE, 0, "%s: the input list of datasets must "
              "have the same sizes (dimensions and length along each "
              "dimension)", __func__);
    }


  /* Prepare the necessary formats for each column, then allocate the space
     for the full list and concatenate all the separate inputs into it. */
  fmts=txt_fmts_for_printf(input, 1, tab0_img1);


  /* Set the output FILE pointer: if it isn't NULL, its an actual file,
     otherwise, its the standard output. */
  if(filename)
    {
      /* Make sure the file doesn't already exist. */
      if( gal_checkset_check_file_return(filename) )
        error(EXIT_FAILURE, 0, "%s: %s already exists. For safety, "
              "this function will not over-write an existing file. "
              "Please delete it before calling this function",
              __func__, filename);

      /* Open the output file. */
      errno=0;
      fp=fopen(filename, "w");
      if(fp==NULL)
        error(EXIT_FAILURE, errno, "%s: couldn't be open to write text "
              "table by %s", filename, __func__);

      /* Write the comments if there were any. */
      for(strt=comment; strt!=NULL; strt=strt->next)
        fprintf(fp, "# %s\n", strt->v);

      /* Write the keywords. */
      if(keylist) txt_write_keys(fp, keylist, freekeys);
    }
  else
    fp=stdout;


  /* Write the meta-data if necessary. */
  if(filename ? 1 : colinfoinstdout)
    txt_write_metadata(fp, input, fmts, tab0_img1);


  /* Print row-by-row (if we actually have data to print!). */
  if(input->array)
    {
      if(tab0_img1) /* Image. */
        for(i=0;i<input->dsize[0];++i)
          {
            d1=input->dsize[1];
            for(j=0;j<d1;++j)
              txt_print_value(fp, input, i*d1+j, fmts[j==d1-1 ? 3 : 0]);
            fprintf(fp, "\n");
          }
      else /* Table. */
        {
          for(i=0;i<input->dsize[0];++i)                  /* Row.    */
            {
              k=0; /* Column counter. */
              for(data=input;data!=NULL;data=data->next)  /* Column. */
                {
                  if(data->ndim>1)  /* Vector column. */
                    {
                      d1=data->dsize[1];
                      for(j=0;j<d1;++j)
                        txt_print_value(fp, data, i*d1+j,
                          fmts[ k * FMTS_COLS
                      /* Last of vector column has a different format. */
                                + (j==d1-1 && data->next==NULL ? 3 : 0) ]);
                    }
                  else /* Non-vector column: simple! */
                    txt_print_value(fp, data, i, fmts[k * FMTS_COLS]);
                  ++k;
                }
              fprintf(fp, "\n");
            }
        }
    }


  /* Clean up. */
  for(i=0;i<num;++i)
    {
      free(fmts[i*FMTS_COLS]);
      free(fmts[i*FMTS_COLS+1]);
      free(fmts[i*FMTS_COLS+2]);
      free(fmts[i*FMTS_COLS+3]);
    }
  free(fmts);


  /* Close the output file. */
  if(filename)
    {
      errno=0;
      if(fclose(fp))
        error(EXIT_FAILURE, errno, "%s: couldn't close file after "
              "writing of text table in %s", filename, __func__);
    }

  /* Restore the next pointer for an image. */
  if(nextimg) input->next=nextimg;
}
