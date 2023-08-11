/*********************************************************************
Functions to work with FITS image data.
This is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <time.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gsl/gsl_version.h>

#include <gnuastro/git.h>
#include <gnuastro/txt.h>
#include <gnuastro/wcs.h>
#include <gnuastro/list.h>
#include <gnuastro/fits.h>
#include <gnuastro/tile.h>
#include <gnuastro/blank.h>
#include <gnuastro/threads.h>
#include <gnuastro/pointer.h>

#include <gnuastro-internal/checkset.h>
#include <gnuastro-internal/tableintern.h>
#include <gnuastro-internal/fixedstringmacros.h>










/*************************************************************
 **************    Internal necessary functions   ************
 *************************************************************/
static void
fits_tab_write_col(fitsfile *fptr, gal_data_t *col, int tableformat,
                   size_t *colind, char *tform, char *filename);










/*************************************************************
 **************        Reporting errors:       ***************
 *************************************************************/
void
gal_fits_io_error(int status, char *message)
{
  char defmessage[]="Error in CFITSIO, see above.";
  if(status)
    {
      fits_report_error(stderr, status);
      if(message)
        error(EXIT_FAILURE, 0, "%s", message);
      else
        error(EXIT_FAILURE, 0, "%s", defmessage);
    }
}




















/*************************************************************
 ************** FITS name and file identification ************
 *************************************************************/
/* IMPORTANT NOTE: if other compression suffixes are add to this function,
   include them in 'gal_checkset_automatic_output', so the compression
   suffix can be skipped when the user doesn't specify an output
   filename. */
int
gal_fits_name_is_fits(char *name)
{
  size_t len;

  if(name)
    {
      len=strlen(name);
      if ( (    len>=3 && strcmp(&name[len-3], "fit"     ) == 0 )
           || ( len>=4 && strcmp(&name[len-4], "fits"    ) == 0 )
           || ( len>=7 && strcmp(&name[len-7], "fits.gz" ) == 0 )
           || ( len>=6 && strcmp(&name[len-6], "fits.Z"  ) == 0 )
           || ( len>=3 && strcmp(&name[len-3], "imh"     ) == 0 )
           || ( len>=7 && strcmp(&name[len-7], "fits.fz" ) == 0 ) )
        return 1;
      else
        return 0;
    }
  else return 0;
}





/* IMPORTANT NOTE: if other compression suffixes are added to this function,
   include them in 'gal_checkset_automatic_output', so the compression
   suffix can be skipped when the user doesn't specify an output
   filename. */
int
gal_fits_suffix_is_fits(char *suffix)
{
  char *nodot;

  if(suffix)
    {
      nodot=suffix[0]=='.' ? (suffix+1) : suffix;
      if ( strcmp(   nodot, "fit"     ) == 0
           || strcmp(nodot, "fits"    ) == 0
           || strcmp(nodot, "fits.gz" ) == 0
           || strcmp(nodot, "fits.Z"  ) == 0
           || strcmp(nodot, "imh"     ) == 0
           || strcmp(nodot, "fits.fz" ) == 0 )
        return 1;
      else
        return 0;
    }
  else return 0;
}





/* If the name is a FITS name, then put a '(hdu: ...)' after it and return
   the string. If it isn't a FITS file, just print the name. Note that the
   space is allocated. */
char *
gal_fits_name_save_as_string(char *filename, char *hdu)
{
  char *name;

  /* Small sanity check. */
  if(filename==NULL)
    gal_checkset_allocate_copy("stdin", &name);
  else
    {
      if( gal_fits_name_is_fits(filename) )
        {
          if( asprintf(&name, "%s (hdu: %s)", filename, hdu)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else gal_checkset_allocate_copy(filename, &name);
    }
  return name;
}





int
gal_fits_file_recognized(char *filename)
{
  FILE *tmpfile;
  fitsfile *fptr;
  int out=0, status=0;

  /* Opening a FITS file can be CPU intensive (for example when its
     compressed). So if the name has the correct suffix, we'll trust the
     suffix. In this way, if the file name is correct, but the contents
     doesn't follow the FITS standard, the opening function will complain
     if its not a FITS file when trying to open it for its usage. */
  if( gal_fits_name_is_fits(filename) ) return 1;

  /* CFITSIO has some special conventions for file names (for example if
     its '-', which can happen in the Arithmetic program). So before
     passing to CFITSIO, we'll check if a file is actually associated with
     the string. For another example see the comments in
     'gal_fits_hdu_open' about a '.gz' suffix. */
  errno=0;
  tmpfile = fopen(filename, "r");
  if(tmpfile) /* The file existed and opened. */
    {
      /* Close the opened filed. */
      if(fclose(tmpfile)==EOF)
        error(EXIT_FAILURE, errno, "%s", filename);

      /* Open the file with CFITSIO to see if its actually a FITS file. */
      fits_open_file(&fptr, filename, READONLY, &status);

      /* If it opened successfully then status will be zero and we can
         safely close the file. Otherwise, this is not a recognized FITS
         file for CFITSIO and we should just return 0. */
      if(status==0)
        {
          if( fits_close_file(fptr, &status) )
            gal_fits_io_error(status, NULL);
          out=1;
        }
    }

  /* Return the final value. */
  return out;
}




















/*************************************************************
 **************           Type codes           ***************
 *************************************************************/
uint8_t
gal_fits_bitpix_to_type(int bitpix)
{
  switch(bitpix)
    {
    case BYTE_IMG:                  return GAL_TYPE_UINT8;
    case SBYTE_IMG:                 return GAL_TYPE_INT8;
    case USHORT_IMG:                return GAL_TYPE_UINT16;
    case SHORT_IMG:                 return GAL_TYPE_INT16;
    case ULONG_IMG:                 return GAL_TYPE_UINT32;
    case LONG_IMG:                  return GAL_TYPE_INT32;
    case LONGLONG_IMG:              return GAL_TYPE_INT64;
    case FLOAT_IMG:                 return GAL_TYPE_FLOAT32;
    case DOUBLE_IMG:                return GAL_TYPE_FLOAT64;
    default:
      error(EXIT_FAILURE, 0, "%s: bitpix value of %d not recognized",
            __func__, bitpix);
    }
  return 0;
}





int
gal_fits_type_to_bitpix(uint8_t type)
{
  switch(type)
    {
    case GAL_TYPE_UINT8:       return BYTE_IMG;
    case GAL_TYPE_INT8:        return SBYTE_IMG;
    case GAL_TYPE_UINT16:      return USHORT_IMG;
    case GAL_TYPE_INT16:       return SHORT_IMG;
    case GAL_TYPE_UINT32:      return ULONG_IMG;
    case GAL_TYPE_INT32:       return LONG_IMG;
    case GAL_TYPE_INT64:       return LONGLONG_IMG;
    case GAL_TYPE_FLOAT32:     return FLOAT_IMG;
    case GAL_TYPE_FLOAT64:     return DOUBLE_IMG;

    case GAL_TYPE_BIT:
    case GAL_TYPE_STRLL:
    case GAL_TYPE_STRING:
    case GAL_TYPE_UINT64:
    case GAL_TYPE_COMPLEX32:
    case GAL_TYPE_COMPLEX64:
      error(EXIT_FAILURE, 0, "%s: type %s not recognized for FITS image "
            "BITPIX", __func__, gal_type_name(type, 1));

    default:
      error(EXIT_FAILURE, 0, "%s: type value of %d not recognized",
            __func__, type);
    }
  return 0;
}





/* The values to the TFORM header keywords of FITS binary tables are single
   letter capital letters, but that is useless in identifying the data type
   of the column. So this function will do the conversion based on the
   CFITSIO manual. */
char
gal_fits_type_to_bin_tform(uint8_t type)
{
  switch(type)
    {
    /* Recognized by CFITSIO. */
    case GAL_TYPE_STRING:      return 'A';
    case GAL_TYPE_BIT:         return 'X';
    case GAL_TYPE_UINT8:       return 'B';
    case GAL_TYPE_INT8:        return 'S';
    case GAL_TYPE_UINT16:      return 'U';
    case GAL_TYPE_INT16:       return 'I';
    case GAL_TYPE_UINT32:      return 'V';
    case GAL_TYPE_INT32:       return 'J';
    case GAL_TYPE_INT64:       return 'K';
    case GAL_TYPE_FLOAT32:     return 'E';
    case GAL_TYPE_FLOAT64:     return 'D';
    case GAL_TYPE_COMPLEX32:   return 'C';
    case GAL_TYPE_COMPLEX64:   return 'M';

    /* Not recognized by CFITSIO. */
    case GAL_TYPE_UINT64:
      error(EXIT_FAILURE, 0, "%s: type %s not recognized for FITS binary "
            "table TFORM", __func__, gal_type_name(type, 1));
      break;

    /* Wrong type code. */
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized", __func__, type);
    }

  error(EXIT_FAILURE, 0, "%s: s bug! Please contact us so we can fix this. "
        "Control must not reach the end of this function", __func__);
  return '\0';
}





int
gal_fits_type_to_datatype(uint8_t type)
{
  int w=0;

  switch(type)
    {
    /* Recognized CFITSIO types. */
    case GAL_TYPE_BIT:              return TBIT;
    case GAL_TYPE_UINT8:            return TBYTE;
    case GAL_TYPE_INT8:             return TSBYTE;
    case GAL_TYPE_FLOAT32:          return TFLOAT;
    case GAL_TYPE_FLOAT64:          return TDOUBLE;
    case GAL_TYPE_COMPLEX32:        return TCOMPLEX;
    case GAL_TYPE_COMPLEX64:        return TDBLCOMPLEX;
    case GAL_TYPE_STRING:           return TSTRING;

    /* Types that depend on the host system. The C standard says that the
       'short', 'int' and 'long' types are ATLEAST 2, 2, 4 bytes, so to be
       safe, we will check all of them for the 32-bit types. */
    case GAL_TYPE_UINT16:
      w=2;
      if     ( sizeof(short)    == w )   return TUSHORT;
      else if( sizeof(int)      == w )   return TUINT;
      break;

    case GAL_TYPE_INT16:
      w=2;
      if     ( sizeof(short)    == w )   return TSHORT;
      else if( sizeof(int)      == w )   return TINT;
      break;

    /* On 32-bit systems, the length of 'int' and 'long' are both
       32-bits. But CFITSIO's LONG type is preferred because it is designed
       to be 32-bit. Its 'INT' type is not clearly defined and caused
       problems when reading keywords. */
    case GAL_TYPE_UINT32:
      w=4;
      if     ( sizeof(long)     == w )   return TULONG;
      else if( sizeof(int)      == w )   return TUINT;
      else if( sizeof(short)    == w )   return TUSHORT;
      break;

    /* Similar to UINT32 above. */
    case GAL_TYPE_INT32:
      w=4;
      if     ( sizeof(long)     == w )   return TLONG;
      else if( sizeof(int)      == w )   return TINT;
      else if( sizeof(short)    == w )   return TSHORT;
      break;

    case GAL_TYPE_UINT64:
      w=8;
      if     ( sizeof(long)     == w )   return TULONG;
      break;

    case GAL_TYPE_INT64:
      w=8;
      if     ( sizeof(long)     == w )   return TLONG;
      else if( sizeof(LONGLONG) == w )   return TLONGLONG;
      break;

    /* Wrong type. */
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d is not a recognized",
            __func__, type);
    }

  /* If control reaches, here, there was a problem with the host types. */
  if(w)
    error(EXIT_FAILURE, 0, "%s: this system doesn't have a %d byte integer "
          "type, so type '%s' cannot be written to FITS", __func__, w,
          gal_type_name(type, 1));
  else
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s so we can "
          "fix the problem. Control must not have reached the end for the "
          "given type '%s'", __func__, PACKAGE_BUGREPORT,
          gal_type_name(type, 1));
  return -1;
}





uint8_t
gal_fits_datatype_to_type(int datatype, int is_table_column)
{
  int inttype;

  switch(datatype)
    {
    case TBIT:            return GAL_TYPE_BIT;
    case TBYTE:           return GAL_TYPE_UINT8;
    case TSBYTE:          return GAL_TYPE_INT8;
    case TFLOAT:          return GAL_TYPE_FLOAT32;
    case TDOUBLE:         return GAL_TYPE_FLOAT64;
    case TCOMPLEX:        return GAL_TYPE_COMPLEX32;
    case TDBLCOMPLEX:     return GAL_TYPE_COMPLEX64;
    case TSTRING:         return GAL_TYPE_STRING;

    /* Sizes that depend on the host system. */
    case TUSHORT:
      switch( sizeof(short) )
        {
        case 2:           return GAL_TYPE_UINT16;
        case 4:           return GAL_TYPE_UINT32;
        case 8:           return GAL_TYPE_UINT64;
        }
      break;

    case TSHORT:
      switch( sizeof(short) )
        {
        case 2:           return GAL_TYPE_INT16;
        case 4:           return GAL_TYPE_INT32;
        case 8:           return GAL_TYPE_INT64;
        }
      break;

    case TUINT:
      switch( sizeof(int) )
        {
        case 2:           return GAL_TYPE_UINT16;
        case 4:           return GAL_TYPE_UINT32;
        case 8:           return GAL_TYPE_UINT64;
        }
      break;

    case TINT:
      switch( sizeof(int) )
        {
        case 2:           return GAL_TYPE_INT16;
        case 4:           return GAL_TYPE_INT32;
        case 8:           return GAL_TYPE_INT64;
        }
      break;

    case TULONG:
      switch( sizeof(long) )
        {
        case 4:           return GAL_TYPE_UINT32;
        case 8:           return GAL_TYPE_UINT64;
        }
      break;

    case TLONG: /* ==TINT32BIT when in a table column. */
      if(is_table_column) return GAL_TYPE_INT32;
      else
        switch( sizeof(long) )
          {
          case 4:         return GAL_TYPE_INT32;
          case 8:         return GAL_TYPE_INT64;
          }
      break;

    case TLONGLONG:
      return GAL_TYPE_INT64;
      break;

    /* The TLOGICAL depends on the context: for keywords, it is int32, for
       table columns, it is int8. */
    case TLOGICAL:
      switch( sizeof(int) )
        {
        case 2: inttype=GAL_TYPE_INT16;  break;
        case 4: inttype=GAL_TYPE_INT32;  break;
        case 8: inttype=GAL_TYPE_INT64;  break;
        }
      return is_table_column ? GAL_TYPE_INT8 : inttype;
      break;

    /* A bug! */
    default:
      error(EXIT_FAILURE, 0, "%s: %d is not a recognized CFITSIO datatype",
            __func__, datatype);
    }

  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s so we can fix "
        "this. Control must not have reached the end of this function.",
        __func__, PACKAGE_BUGREPORT);
  return GAL_TYPE_INVALID;
}





/* When there is a BZERO or TZERO and BSCALE or TSCALE keywords, then the
   type that must be used to store the actual values of the pixels may be
   different from the type from BITPIX. This function does the necessary
   corrections. */
static void
fits_type_correct(int *type, double bscale, char *bzero_str)
{
  double bzero;
  int tofloat=1;
  char *tailptr, *bzero_u64="9223372036854775808";

  /* If bzero_str is given and 'bscale=1.0' the case might be that we are
     dealing with an integer dataset that just needs a different sign. */
  if(bzero_str && bscale==1.0f)
    {
      /* Read the 'bzero' string as a 'double' number. */
      bzero  = strtod(bzero_str, &tailptr);
      if(tailptr==bzero_str)
        error(EXIT_FAILURE, 0, "%s: BZERO value '%s' couldn't be "
              "parsed as a number", __func__, bzero_str);

      /* Work based on type. For the default conversions defined by the
         FITS standard to change the signs of integers, make the proper
         correction, otherwise set the type to float. */
      switch(*type)
        {
        case GAL_TYPE_UINT8:
          if(bzero == -128.0f)      { *type = GAL_TYPE_INT8;   tofloat=0; }
          break;

        case GAL_TYPE_INT16:
          if(bzero == 32768)        { *type = GAL_TYPE_UINT16; tofloat=0; }
          break;

        case GAL_TYPE_INT32:
          if(bzero == 2147483648LU) { *type = GAL_TYPE_UINT32; tofloat=0; }
          break;

        /* The 'bzero' value for unsigned 64-bit integers has 19 decimal
           digits, but a 64-bit floating point ('double' type) can only
           safely store 15 decimal digits. As a result, the safest way to
           check the 'bzero' value for this type is to compare it as a
           string. But all integers nearby (for example
           '9223372036854775807') get rounded to this same value (when
           stored as 'double'). So we will also check the parsed number and
           if it equals this number, a warning will be printed. */
        case GAL_TYPE_INT64:
          if( !strcmp(bzero_str, bzero_u64) )
            {*type = GAL_TYPE_UINT64; tofloat=0;}
          else
            if( bzero == 9223372036854775808LLU )
              {
                fprintf(stderr, "\nWARNING in %s: the BZERO header keyword "
                        "value ('%s') is very close (but not exactly equal) "
                        "to '%s'. The latter value in the FITS standard is "
                        "used to identify that the dataset should be read as "
                        "unsigned 64-bit integers instead of signed 64-bit "
                        "integers. Depending on your version of CFITSIO, "
                        "it may be read as a signed or unsigned 64-bit "
                        "integer array\n\n", __func__, bzero_str, bzero_u64);
                tofloat=0;
              }
          break;

          /* For the other types (when 'BSCALE=1.0f'), currently no
             correction is necessary, maybe later we can check if the
             scales are integers and set the integer output type to the
             smallest type that can allow the scaled values. */
        default: tofloat=0;
        }
    }

  /* If the type must be a float, then do the conversion. */
  if(tofloat) *type=GAL_TYPE_FLOAT32;
}




















/**************************************************************/
/**********                  HDU                   ************/
/**************************************************************/
fitsfile *
gal_fits_open_to_write(char *filename)
{
  int status=0;
  long naxes=0;
  fitsfile *fptr;

  /* When the file exists just open it. Otherwise, create the file. But we
     want to leave the first extension as a blank extension and put the
     image in the next extension to be consistent between tables and
     images. */
  if(access(filename,F_OK) == -1 )
    {
      /* Create the file. */
      if( fits_create_file(&fptr, filename, &status) )
        gal_fits_io_error(status, NULL);

      /* Create blank extension. */
      if( fits_create_img(fptr, BYTE_IMG, 0, &naxes, &status) )
        gal_fits_io_error(status, NULL);

      /* Close the blank extension. */
      if( fits_close_file(fptr, &status) )
        gal_fits_io_error(status, NULL);
    }

  /* Open the file, ready for later steps. */
  if( fits_open_file(&fptr, filename, READWRITE, &status) )
    gal_fits_io_error(status, NULL);

  /* Return the pointer. */
  return fptr;
}





size_t
gal_fits_hdu_num(char *filename)
{
  fitsfile *fptr;
  int num, status=0;

  /* Make sure the given file exists: CFITSIO adds '.gz' silently (see more
     in the comments within 'gal_fits_hdu_open')*/
  gal_checkset_check_file(filename);

  /* We don't need to check for an error everytime, because we don't
     make any non CFITSIO usage of the output. It is necessary to
     check every CFITSIO call, only when you will need to use the
     outputs. */
  fits_open_file(&fptr, filename, READONLY, &status);

  fits_get_num_hdus(fptr, &num, &status);

  fits_close_file(fptr, &status);

  gal_fits_io_error(status, NULL);

  return num;
}





/* Calculate the datasum of the given HDU in the given file. */
unsigned long
gal_fits_hdu_datasum(char *filename, char *hdu, char *hdu_option_name)
{
  int status=0;
  fitsfile *fptr;
  unsigned long datasum;

  /* Read the desired extension (necessary for reading the rest). */
  fptr=gal_fits_hdu_open(filename, hdu, READONLY, 1,
                         hdu_option_name);

  /* Calculate the datasum. */
  datasum=gal_fits_hdu_datasum_ptr(fptr);

  /* Close the file and return. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, "closing file");
  return datasum;
}





/* Calculate the FITS standard datasum for the opened FITS pointer. */
unsigned long
gal_fits_hdu_datasum_ptr(fitsfile *fptr)
{
  int status=0;
  unsigned long datasum, hdusum;

  /* Calculate the checksum and datasum of the opened HDU. */
  fits_get_chksum(fptr, &datasum, &hdusum, &status);
  gal_fits_io_error(status, "estimating datasum");

  /* Return the datasum. */
  return datasum;
}





/* Given the filename and HDU, this function will return the CFITSIO code
   for the type of data it contains (table, or image). The CFITSIO codes
   are:

       IMAGE_HDU:    An image HDU.
       ASCII_TBL:    An ASCII table HDU.
       BINARY_TBL:   BINARY TABLE HDU.       */
int
gal_fits_hdu_format(char *filename, char *hdu, char *hdu_option_name)
{
  fitsfile *fptr;
  int hdutype, status=0;

  /* Open the HDU. */
  fptr=gal_fits_hdu_open(filename, hdu, READONLY, 1,
                         hdu_option_name);

  /* Check the type of the given HDU: */
  if (fits_get_hdu_type(fptr, &hdutype, &status) )
    gal_fits_io_error(status, NULL);

  /* Clean up and return. */
  if( fits_close_file(fptr, &status) )
    gal_fits_io_error(status, NULL);
  return hdutype;
}





/* Return 1 if the HDU appears to be a HEALpix grid. */
int
gal_fits_hdu_is_healpix(fitsfile *fptr)
{
  long value;
  int hdutype, status=0;

  /* An ASCII table can't be a healpix table. */
  if (fits_get_hdu_type(fptr, &hdutype, &status) )
    gal_fits_io_error(status, NULL);
  if(hdutype==ASCII_TBL) return 0;

  /* If all these keywords are present, then 'status' will be 0
     afterwards. */
  fits_read_key_lng(fptr, "NSIDE",    &value, 0x0, &status);
  fits_read_key_lng(fptr, "FIRSTPIX", &value, 0x0, &status);
  fits_read_key_lng(fptr, "LASTPIX",  &value, 0x0, &status);
  return !status;
}





/* Open a given HDU and return the FITS pointer. 'iomode' determines how
   the FITS file will be opened: only to read or to read and write. You
   should use the macros given by the CFITSIO header:

     READONLY:   read-only.
     READWRITE:  read and write.         */
fitsfile *
gal_fits_hdu_open(char *filename, char *hdu, int iomode, int exitonerror,
                  char *hdu_option_name)
{
  int status=0;
  char *ffname;
  fitsfile *fptr;
  char *hduon = hdu_option_name ? hdu_option_name : "--hdu";

  /* Make sure the file exists. This is necessary because CFITSIO
     automatically appends a '.gz' when a file with that extension already
     exists! For example if the user asked for 'abc.fits', but the
     directory only includes 'abc.fits.gz', CFITSIO will open that instead
     (without any status value). */
  gal_checkset_check_file(filename);

  /* Add hdu to filename: */
  if( asprintf(&ffname, "%s[%s#]", filename, hdu)<0 )
    {
      if(exitonerror)
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
      else return NULL;
    }

  /* Open the FITS file: */
  if( fits_open_file(&fptr, ffname, iomode, &status) )
    {
      switch(status)
        {
        /* Since the default HDU is '1', when the file only has one
           extension, this error is common, so we will put a special
           notice. */
        case END_OF_FILE:
          if( hdu[0]=='1' && hdu[1]=='\0' )
            {
              if(exitonerror)
                error(EXIT_FAILURE, 0, "%s: only has one HDU.\n\n"
                      "You should inform this program to look for your "
                      "desired input data in the primary HDU with the "
                      "'%s=0' option. For more, see the FOOTNOTE "
                      "below.\n\n"
                      "Pro TIP: if your desired HDU has a name (value to "
                      "'EXTNAME' keyword), it is best to just use that "
                      "name with '%s' instead of relying on a counter. "
                      "You can see the list of HDUs in a FITS file (with "
                      "their data format, type, size and possibly HDU "
                      "name) using Gnuastro's 'astfits' program, for "
                      "example:\n\n"
                      "    astfits %s\n\n"
                      "FOOTNOTE -- When writing a new FITS file, "
                      "Gnuastro leaves the pimary HDU only for metadata. "
                      "The output datasets (tables, images or cubes) are "
                      "written after the primary HDU. In this way the "
                      "keywords of the the first HDU can be used as "
                      "metadata of the whole file (which may contain "
                      "many extensions, this is stipulated in the FITS "
                      "standard). Usually the primary HDU keywords "
                      "contains the option names and values that the "
                      "program was run with. Because of this, Gnuastro's "
                      "default HDU to read data in a FITS file is the "
                      "second (or '%s=1'). This error is commonly "
                      "caused when the FITS file wasn't created by "
                      "Gnuastro or by a program respecting this "
                      "convention.", filename, hduon, hduon, filename,
                      hduon);
              else return NULL;
            }
          break;

        /* Generic error below is fine for this case. */
        case BAD_HDU_NUM:
          break;

        /* In case an un-expected error occurs, use the general CFITSIO
           reporting that we have already prepared. */
        default:
          if(exitonerror)
            gal_fits_io_error(status, "opening the given extension/HDU "
                              "in the given file");
          else return NULL;
        }

      if(exitonerror)
        error(EXIT_FAILURE, 0, "%s: extension/HDU '%s' doesn't exist. "
              "Please run the following command to see the "
              "extensions/HDUs in '%s':\n\n"
              "    $ astfits %s\n\n"
              "The respective HDU number (or name, when present) may be "
              "used with the '%s' option to open your desired input here. "
              "If you are using counters/numbers to identify your HDUs, "
              "note that since Gnuastro uses CFITSIO for FITS "
              "input/output, HDU counting starts from 0", filename, hdu,
              filename, filename, hduon);
      else return NULL;
    }

  /* Clean up and the pointer. */
  free(ffname);
  return fptr;
}





/* Check the desired HDU in a FITS image and also if it has the
   desired type. */
fitsfile *
gal_fits_hdu_open_format(char *filename, char *hdu, int img0_tab1,
                         char *hdu_option_name)
{
  fitsfile *fptr;
  int status=0, hdutype;

  /* A small sanity check. */
  if(hdu==NULL)
    error(EXIT_FAILURE, 0, "no HDU specified for %s", filename);

  /* Open the HDU. */
  fptr=gal_fits_hdu_open(filename, hdu, READONLY, 1, hdu_option_name);

  /* Check the type of the given HDU: */
  if (fits_get_hdu_type(fptr, &hdutype, &status) )
    gal_fits_io_error(status, NULL);

  /* Check if the type of the HDU is the expected type. We could have
     written these as && conditions, but this is easier to read, it makes
     no meaningful difference to the compiler. */
  if(img0_tab1)
    {
      if(hdutype==IMAGE_HDU)
        error(EXIT_FAILURE, 0, "%s (hdu: %s): is not a table",
              filename, hdu);
    }
  else
    {
      if(hdutype!=IMAGE_HDU)
        {
          /* Let the user know. */
          if( gal_fits_hdu_is_healpix(fptr) )
            error(EXIT_FAILURE, 0, "%s (hdu: %s): appears to be a HEALPix table "
                  "(which is a 2D dataset on a spherical surface: stored as "
                  "a 1D table). You can use the 'HPXcvt' command-line utility "
                  "to convert it to a 2D image that can easily be used by "
                  "other programs. 'HPXcvt' is built and installed as part of "
                  "WCSLIB (which is a mandatory dependency of Gnuastro, so "
                  "you should already have it), run 'man HPXcvt' for more",
                  filename, hdu);
          else
            error(EXIT_FAILURE, 0, "%s (hdu: %s): not an image",
                  filename, hdu);
        }
    }

  /* Clean up and return. */
  return fptr;
}




















/**************************************************************/
/**********            Header keywords             ************/
/**************************************************************/
int
gal_fits_key_exists_fptr(fitsfile *fptr, char *keyname)
{
  int status=0;
  char card[FLEN_CARD];
  fits_read_card(fptr, keyname, card, &status);
  return status==0;
}





/* Return allocated pointer to the blank value to use in a FITS file header
   keyword. */
void *
gal_fits_key_img_blank(uint8_t type)
{
  void *out=NULL, *tocopy=NULL;
  uint8_t u8=0;          /* Equivalent of minimum in signed   with BZERO. */
  int16_t s16=INT16_MAX; /* Equivalend of maximum in unsigned with BZERO. */
  int32_t s32=INT32_MAX; /* Equivalend of maximum in unsigned with BZERO. */
  int64_t s64=INT64_MAX; /* Equivalend of maximum in unsigned with BZERO. */

  switch(type)
    {
    /* Types with no special treatment: */
    case GAL_TYPE_BIT:
    case GAL_TYPE_UINT8:
    case GAL_TYPE_INT16:
    case GAL_TYPE_INT32:
    case GAL_TYPE_INT64:
    case GAL_TYPE_FLOAT32:
    case GAL_TYPE_FLOAT64:
    case GAL_TYPE_COMPLEX32:
    case GAL_TYPE_COMPLEX64:
    case GAL_TYPE_STRING:
    case GAL_TYPE_STRLL:
      out = gal_blank_alloc_write(type);
      break;

    /* Types that need values from their opposite-signed types. */
    case GAL_TYPE_INT8:      tocopy=&u8;      break;
    case GAL_TYPE_UINT16:    tocopy=&s16;     break;
    case GAL_TYPE_UINT32:    tocopy=&s32;     break;
    case GAL_TYPE_UINT64:    tocopy=&s64;     break;

    default:
      error(EXIT_FAILURE, 0, "%s: type code %u not recognized as a Gnuastro "
            "data type", __func__, type);
    }

  /* If 'gal_blank_alloc_write' wasn't used (copy!=NULL), then allocate the
     necessary space and fill it in. Note that the width of the signed and
     unsigned values doesn't differ, so we can use the actual input
     type. */
  if(tocopy)
    {
      out = gal_pointer_allocate(type, 1, 0, __func__, "out");
      memcpy(out, tocopy, gal_type_sizeof(type));
    }

  /* Return. */
  return out;
}





/* CFITSIO doesn't remove the two single quotes around the string value, so
   the strings it reads are like: 'value ', or 'some_very_long_value'. To
   use the value, it is commonly necessary to remove the single quotes (and
   possible extra spaces). This function will modify the string in its own
   allocated space. You can use this to later free the original string (if
   it was allocated). */
void
gal_fits_key_clean_str_value(char *string)
{
  int end;       /* Has to be int because size_t is always >=0. */
  char *c, *cf;

  /* Start from the second last character (the last is a single quote) and
     go down until you hit a non-space character. This will also work when
     there is no space characters between the last character of the value
     and ending single-quote: it will be set to '\0' after this loop. */
  for(end=strlen(string)-2;end>=0;--end)
    if(string[end]!=' ')
      break;

  /* Shift all the characters after the first one (which is a ''' back by
     one and put the string ending characters on the 'end'th element. */
  cf=(c=string)+end; do *c=*(c+1); while(++c<cf);
  *cf='\0';
}




/* Fill the 'tm' structure (defined in 'time.h') with the values derived
   from a FITS format date-string and return the (optional) sub-second
   information as a double.

   The basic FITS string is defined under the 'DATE' keyword in the FITS
   standard. For the more complete format which includes timezones, see the
   W3 standard: https://www.w3.org/TR/NOTE-datetime */
char *
gal_fits_key_date_to_struct_tm(char *fitsdate, struct tm *tp)
{
  int hasT=0, hassq=0, usesdash=0, usesslash=0, hasZ=0;
  char *C, *cc, *c=NULL, *cf, *subsec=NULL, *nosubsec=fitsdate;

  /* Initialize the 'tm' structure to all-zero elements. In particular, The
     FITS standard times are written in UTC, so, the time zone ('tm_zone'
     element, which specifies number of seconds to shift for the time zone)
     has to be zero. The day-light saving flag ('isdst' element) also has
     to be set to zero. */
  tp->tm_sec=tp->tm_min=tp->tm_hour=tp->tm_mday=tp->tm_mon=tp->tm_year=0;
  tp->tm_wday=tp->tm_yday=tp->tm_isdst=tp->tm_gmtoff=0;
  tp->tm_zone=NULL;

  /* According to the FITS standard the 'T' in the middle of the date and
     time of day is optional (the time is not mandatory). */
  cf=(c=fitsdate)+strlen(fitsdate);
  do
    switch(*c)
      {
      case 'T':  hasT=1;      break; /* With 'T' HH:MM:SS are defined.    */
      case '-':  usesdash=1;  break; /* Day definition: YYYY-MM-DD.       */
      case '/':  usesslash=1; break; /* Day definition(old): DD/MM/YY.    */
      case '\'': hassq=1;     break; /* Wholly Wrapped in a single-quote. */
      case 'Z':  hasZ=1;      break; /* When ends in 'Z', means UTC. See  */
                                   /* https://www.w3.org/TR/NOTE-datetime */

      /* In case we have sub-seconds in the string, we need to remove it
         because 'strptime' doesn't recognize sub-second resolution. */
      case '.':
        /* Allocate space (by copying the remaining full string and adding
           a '\0' where necessary. */
        gal_checkset_allocate_copy(c, &subsec);
        gal_checkset_allocate_copy(fitsdate, &nosubsec);

        /* Parse the sub-second part and remove it from the copy. */
        cc=nosubsec+(c-fitsdate);
        for(C=subsec+1;*C!='\0';C++)
          if(!isdigit(*C)) {*cc++=*C; *C='\0';}
        *cc='\0';
      }
  while(++c<cf);

  /* Convert this date into seconds since 1970/01/01, 00:00:00. */
  c = ( (usesdash==0 && usesslash==0)
        ? NULL
        : ( usesdash
            ? ( hasT
                ? ( hasZ
                    ? strptime(nosubsec, hassq?"'%FT%TZ'":"%FT%TZ", tp)
                    : strptime(nosubsec, hassq?"'%FT%T'":"%FT%T", tp) )
                : strptime(nosubsec, hassq?"'%F'"   :"%F"   , tp))
            : ( hasT
                ? ( hasZ
                    ? strptime(nosubsec, hassq?"'%d/%m/%yT%TZ'":"%d/%m/%yT%TZ", tp)
                    : strptime(nosubsec, hassq?"'%d/%m/%yT%T'":"%d/%m/%yT%T", tp))
                : strptime(nosubsec, hassq?"'%d/%m/%y'"   :"%d/%m/%y"   , tp)
                )
            )
        );

  /* The value might have sub-seconds. In that case, 'c' will point to a
     '.' and we'll have to parse it as double. */
  if( c==NULL || (*c!='.' && *c!='\0') )
    error(EXIT_FAILURE, 0, "'%s' isn't in the FITS date format.\n\n"
          "According to the FITS standard, the date must be in one of "
          "these formats:\n"
          "   YYYY-MM-DD\n"
          "   YYYY-MM-DDThh:mm:ss\n"
          "   YYYY-MM-DDThh:mm:ssZ   (Note the 'Z',  see *) \n"
          "   DD/MM/YY               (Note the 'YY', see ^)\n"
          "   DD/MM/YYThh:mm:ss\n"
          "   DD/MM/YYThh:mm:ssZ\n\n"
          "[*]: The 'Z' is interpreted as being in the UTC Timezone.\n"
          "[^]: Gnuastro's FITS library (this program), interprets the "
          "older (two character for year) format, year values 68 to 99 as "
          "the years 1969 to 1999 and values 0 to 68 as the years 2000 to "
          "2068.", fitsdate);

  /* If the subseconds were removed (and a new string was allocated), free
     that extra new string. */
  if(nosubsec!=fitsdate) free(nosubsec);

  /* Return the subsecond value. */
  return subsec;
}





/* Convert the FITS standard date format (as a string, already read from
   the keywords) into number of seconds since 1970/01/01, 00:00:00. Very
   useful to avoid calendar issues like number of days in a different
   months or leap years and etc. The remainder of the format string
   (sub-seconds) will be put into the two pointer arguments: 'subsec' is in
   double-precision floating point format but it will only get a value when
   'subsecstr!=NULL'. */
size_t
gal_fits_key_date_to_seconds(char *fitsdate, char **subsecstr,
                             double *subsec)
{
  time_t t;
  char *tmp;
  struct tm tp;
  size_t seconds;
  void *subsecptr=subsec;

  /* If the string is blank, return a blank value. */
  if( strcmp(fitsdate, GAL_BLANK_STRING)==0 )
    {
      if(subsec) *subsec=NAN;
      if(subsecstr) *subsecstr=NULL;
      return GAL_BLANK_SIZE_T;
    }

  /* Fill in the 'tp' elements with values read from the string. */
  tmp=gal_fits_key_date_to_struct_tm(fitsdate, &tp);

  /* If the user wanted a possible sub-second string/value, then
     'subsecstr!=NULL'. */
  if(subsecstr)
    {
      /* Set the output pointer. Note that it may be NULL if there was no
         sub-second string, but that is fine (and desired because the user
         can use this to check if there was a sub-string or not). */
      *subsecstr=tmp;

      /* If there was a sub-second string, then also read it as a
         double-precision floating point. */
      if(tmp)
        {
          if(subsec)
            if( gal_type_from_string(&subsecptr, tmp, GAL_TYPE_FLOAT64) )
              error(EXIT_FAILURE, 0, "%s: the sub-second portion of '%s' (or "
                    "'%s') couldn't be read as a number", __func__, fitsdate,
                    tmp);
        }
      else { if(subsec) *subsec=NAN; }
    }

  /* Convert the contents of the 'tm' structure to 'time_t' (a positive
     integer) with 'mktime'. Note that by design, the system's timezone is
     included in the returned value of 'mktime' (leading to situations like
     bug #57995). But it writes the given time's timezone (number of
     seconds ahead of UTC) in the 'tm_gmtoff' element of its input.

     IMPORTANT NOTE: the timezone that is calculated by 'mktime' (in
     'tp.tm_gmtoff') belongs to the time that is already within 'tp' (this
     is exactly what we want!). So for example when daylight saving is
     activated at run-time, but at the time inside 'tp', there was no
     daylight saving, the value of 'tp.tm_gmtoff' will be different from
     the 'timezone' global variable. */
  t=mktime(&tp);

  /* Calculate the seconds and return it. */
  seconds = (t == (time_t)(-1)) ? GAL_BLANK_SIZE_T : (t+tp.tm_gmtoff);
  return seconds;
}





/* Read the keyword values from a FITS pointer. The input should be a
   linked list of 'gal_data_t'. Before calling this function, you just have
   to set the 'name' and desired 'type' values of each element in the list
   to the keyword you want it to keep the value of. The given 'name' value
   will be directly passed to CFITSIO to read the desired keyword. This
   function will allocate space to keep the value. Here is one example of
   using this function:

      gal_data_t *keysll=gal_data_array_calloc(N);

      for(i=0;i<N-2;++i) keysll[i]->next=keysll[i+1];

      \\ Put a name and type for each element.

      gal_fits_key_read_from_ptr(fptr, keysll, 0, 0);

      \\ use the values as you like.

      gal_data_array_free(keysll, N, 1);

   If the 'array' pointer of each keyword's dataset is not NULL, then it is
   assumed that the space has already been allocated. If it is NULL, then
   space will be allocated internally here.

   Strings need special consideration: the reason is that generally,
   'gal_data_t' needs to also allow for array of strings (as it supports
   arrays of integers for example). Hence two allocations will be done here
   (one if 'array!=NULL') and 'keysll[i].array' must be interpretted as
   'char **': one allocation for the pointer, one for the actual
   characters. You don't have to worry about the freeing,
   'gal_data_array_free' will free both allocations. So to read a string,
   one easy way would be the following:

      char *str, **strarray;
      strarr = keysll[i].array;
      str    = strarray[0];

   If CFITSIO is unable to read a keyword for any reason the 'status'
   element of the respective 'gal_data_t' will be non-zero. You can check
   the successful reading of the keyword from the 'status' value in each
   keyword's 'gal_data_t'. If it is zero, then the keyword was found and
   succesfully read. Otherwise, it a CFITSIO status value. You can use
   CFITSIO's error reporting tools or 'gal_fits_io_error' for reporting the
   reason. A tip: when the keyword doesn't exist, then CFITSIO's status
   value will be 'KEY_NO_EXIST'.

   CFITSIO will start searching for the keywords from the last place in the
   header that it searched for a keyword. So it is much more efficient if
   the order that you ask for keywords is based on the order they are
   stored in the header.
 */
void
gal_fits_key_read_from_ptr(fitsfile *fptr, gal_data_t *keysll,
                           int readcomment, int readunit)
{
  uint8_t numtype;
  gal_data_t *tmp;
  char **strarray;
  int typewasinvalid;
  void *numptr, *valueptr;

  /* Get the desired keywords. */
  for(tmp=keysll;tmp!=NULL;tmp=tmp->next)
    if(tmp->name)
      {
        /* Initialize the status: */
        tmp->status=0;

        /* For each keyword, this function stores one value currently. So
           set the size and ndim to 1. But first allocate dsize if it
           wasn't already allocated. */
        if(tmp->dsize==NULL)
          tmp->dsize=gal_pointer_allocate(GAL_TYPE_SIZE_T, 1, 0, __func__,
                                          "tmp->dsize");
        tmp->ndim=tmp->size=tmp->dsize[0]=1;

        /* If no type has been given, temporarily set it to a string, we
           will then deduce the type afterwards. */
        typewasinvalid=0;
        if(tmp->type==GAL_TYPE_INVALID)
          {
            typewasinvalid=1;
            tmp->type=GAL_TYPE_STRING;
          }

        /* When the type is a string, 'tmp->array' is an array of pointers
           to a separately allocated piece of memory. So we have to
           allocate that space here. If its not a string, then the
           allocated space above is enough to keep the value. */
        switch(tmp->type)
          {
          case GAL_TYPE_STRING:
            tmp->array=strarray=( tmp->array
                                  ? tmp->array
                                  : gal_pointer_allocate(tmp->type, 1, 0,
                                                         __func__,
                                                         "tmp->array") );
            errno=0;
            valueptr=strarray[0]=malloc(FLEN_VALUE * sizeof *strarray[0]);
            if(strarray[0]==NULL)
              error(EXIT_FAILURE, errno, "%s: %zu bytes for strarray[0]",
                    __func__, FLEN_VALUE * sizeof *strarray[0]);
            break;

          default:
            tmp->array=valueptr=( tmp->array
                                  ? tmp->array
                                  : gal_pointer_allocate(tmp->type, 1, 0,
                                                         __func__,
                                                         "tmp->array") );
          }

        /* Allocate space for the keyword comment if necessary. */
        if(readcomment)
          {
            errno=0;
            tmp->comment=calloc(FLEN_COMMENT, sizeof *tmp->comment);
            if(tmp->comment==NULL)
              error(EXIT_FAILURE, errno, "%s: %zu bytes for tmp->comment",
                    __func__, FLEN_COMMENT * sizeof *tmp->comment);
          }
        else
          tmp->comment=NULL;

        /* Allocate space for the keyword unit if necessary. Note that
           since there is no precise CFITSIO length for units, we will use
           the 'FLEN_COMMENT' length for units too (theoretically, the unit
           might take the full remaining area in the keyword). Also note
           that the unit is only optional, so it needs a separate CFITSIO
           function call which is done here. */
        if(readunit)
          {
            /* Allocate space for the unit and read it in. */
            errno=0;
            tmp->unit=calloc(FLEN_COMMENT, sizeof *tmp->unit);
            if(tmp->unit==NULL)
              error(EXIT_FAILURE, errno, "%s: %zu bytes for tmp->unit",
                    __func__, FLEN_COMMENT * sizeof *tmp->unit);
            fits_read_key_unit(fptr, tmp->name, tmp->unit, &tmp->status);

            /* If the string is empty, free the space and set it to NULL. */
            if(tmp->unit[0]=='\0') {free(tmp->unit); tmp->unit=NULL;}
          }
        else
          tmp->unit=NULL;

        /* Read the keyword and place its value in the pointer. */
        fits_read_key(fptr, gal_fits_type_to_datatype(tmp->type),
                      tmp->name, valueptr, tmp->comment, &tmp->status);

        /* Correct the type if no type was requested and the key has been
           successfully read. */
        if(tmp->status==0 && typewasinvalid)
          {
            /* If the string can be parsed as a number, the number will be
               allocated and placed in 'numptr', otherwise, 'numptr' will
               be NULL. */
            numptr=gal_type_string_to_number(valueptr, &numtype);
            if(numptr)
              {
                free(valueptr);
                free(tmp->array);
                tmp->array=numptr;
                tmp->type=numtype;
              }
          }

        /* If the comment was empty, free the space and set it to NULL. */
        if(tmp->comment && tmp->comment[0]=='\0')
          {free(tmp->comment); tmp->comment=NULL;}
      }
}





/* Same as 'gal_fits_read_keywords_fptr', but accepts the filename and HDU
   as input instead of an already opened CFITSIO 'fitsfile' pointer. */
void
gal_fits_key_read(char *filename, char *hdu, gal_data_t *keysll,
                  int readcomment, int readunit, char *hdu_option_name)
{
  int status=0;
  fitsfile *fptr;

  /* Open the input HDU. */
  fptr=gal_fits_hdu_open(filename, hdu, READONLY, 1, hdu_option_name);

  /* Read the keywords. */
  gal_fits_key_read_from_ptr(fptr, keysll, readcomment, readunit);

  /* Close the FITS file. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
}





/* Add on keyword to the list of header keywords that need to be added
   to a FITS file. In the end, the keywords will have to be freed, so
   it is important to know before hand if they were allocated or
   not. If not, they don't need to be freed.

   NOTE FOR STRINGS: the value should be the pointer to the string its-self
   (char *), not a pointer to a pointer (char **). */
void
gal_fits_key_list_add(gal_fits_list_key_t **list, uint8_t type,
                      char *keyname, int kfree, void *value, int vfree,
                      char *comment, int cfree, char *unit, int ufree)
{
  gal_fits_list_key_t *newnode;

  /* Allocate space for the new node and fill it in. */
  errno=0;
  newnode=malloc(sizeof *newnode);
  if(newnode==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating new node", __func__);

  /* Write the given values into the key structure. */
  newnode->title=NULL;
  newnode->fullcomment=NULL;
  newnode->type=type;
  newnode->keyname=keyname;
  newnode->value=value;
  newnode->comment=comment;
  newnode->unit=unit;
  newnode->kfree=kfree;                /* Free pointers after using them. */
  newnode->vfree=vfree;
  newnode->cfree=cfree;
  newnode->ufree=ufree;

  /* Set the next pointer. */
  newnode->next=*list;
  *list=newnode;
}





void
gal_fits_key_list_add_end(gal_fits_list_key_t **list, uint8_t type,
                          char *keyname, int kfree, void *value, int vfree,
                          char *comment, int cfree, char *unit, int ufree)
{
  gal_fits_list_key_t *tmp, *newnode;

  /* Allocate space for the new node and fill it in. */
  errno=0;
  newnode=malloc(sizeof *newnode);
  if(newnode==NULL)
    error(EXIT_FAILURE, errno, "%s: allocation of new node", __func__);

  /* Write the given values into the key structure. */
  newnode->title=NULL;
  newnode->fullcomment=NULL;
  newnode->type=type;
  newnode->keyname=keyname;
  newnode->value=value;
  newnode->comment=comment;
  newnode->unit=unit;
  newnode->kfree=kfree;            /* Free pointers after using them. */
  newnode->vfree=vfree;
  newnode->cfree=cfree;
  newnode->ufree=ufree;

  /* Set the next pointer. */
  if(*list)         /* List is already full, add this node to the end. */
    {
      /* After this line, tmp points to the last node. */
      tmp=*list; while(tmp->next!=NULL) tmp=tmp->next;
      tmp->next=newnode;
      newnode->next=NULL;
    }
  else                 /* List is empty */
    {
      newnode->next=NULL;
      *list=newnode;
    }
}




/* Only set this key to be a title. */
void
gal_fits_key_list_title_add(gal_fits_list_key_t **list, char *title, int tfree)
{
  gal_fits_list_key_t *newnode;

  /* Allocate space (and initialize to 0/NULL) for the new node and fill it
     in. */
  errno=0;
  newnode=calloc(1, sizeof *newnode);
  if(newnode==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating new node", __func__);

  /* Set the arguments. */
  newnode->title=title;
  newnode->tfree=tfree;

  /* Set the next pointer. */
  newnode->next=*list;
  *list=newnode;
}





/* Put the title keyword in the end. */
void
gal_fits_key_list_title_add_end(gal_fits_list_key_t **list, char *title,
                                int tfree)
{
  gal_fits_list_key_t *tmp, *newnode;

  /* Allocate space (and initialize to 0/NULL) for the new node and fill it
     in. */
  errno=0;
  newnode=calloc(1, sizeof *newnode);
  if(newnode==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating new node", __func__);

  /* Set the arguments. */
  newnode->title=title;
  newnode->tfree=tfree;

  /* Set the next pointer. */
  if(*list)         /* List is already full, add this node to the end. */
    {
      /* After this line, tmp points to the last node. */
      tmp=*list; while(tmp->next!=NULL) tmp=tmp->next;
      tmp->next=newnode;
      newnode->next=NULL;
    }
  else                 /* List is empty */
    {
      newnode->next=*list;
      *list=newnode;
    }
}





/* Only set this key to be a full comment. */
void
gal_fits_key_list_fullcomment_add(gal_fits_list_key_t **list,
                                  char *fullcomment, int fcfree)
{
  gal_fits_list_key_t *newnode;

  /* Allocate space (and initialize to 0/NULL) for the new node and fill it
     in. */
  errno=0;
  newnode=calloc(1, sizeof *newnode);
  if(newnode==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating new node", __func__);

  /* Set the arguments. */
  newnode->fullcomment=fullcomment;
  newnode->fcfree=fcfree;

  /* Set the next pointer. */
  newnode->next=*list;
  *list=newnode;
}





/* Put the title keyword in the end. */
void
gal_fits_key_list_fullcomment_add_end(gal_fits_list_key_t **list,
                                      char *fullcomment, int fcfree)
{
  gal_fits_list_key_t *tmp, *newnode;

  /* Allocate space (and initialize to 0/NULL) for the new node and fill it
     in. */
  errno=0;
  newnode=calloc(1, sizeof *newnode);
  if(newnode==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating new node", __func__);

  /* Set the arguments. */
  newnode->fullcomment=fullcomment;
  newnode->fcfree=fcfree;

  /* Set the next pointer. */
  if(*list)         /* List is already full, add this node to the end. */
    {
      /* After this line, tmp points to the last node. */
      tmp=*list; while(tmp->next!=NULL) tmp=tmp->next;
      tmp->next=newnode;
      newnode->next=NULL;
    }
  else                 /* List is empty */
    {
      newnode->next=*list;
      *list=newnode;
    }
}





void
gal_fits_key_list_reverse(gal_fits_list_key_t **list)
{
  gal_fits_list_key_t *in=*list, *tmp=in, *reversed=NULL;

  /* Only do the reversal if there is more than one element. */
  if(in && in->next)
    {
      /* Fill the 'reversed' list. */
      while(in!=NULL)
        {
          tmp=in->next;
          in->next=reversed;
          reversed=in;
          in=tmp;
        }

      /* Write the reversed list into the input pointer. */
      *list=reversed;
    }
}





/* Write a blank keyword field and a title under it in the specified FITS
   file pointer. */
void
gal_fits_key_write_title_in_ptr(char *title, fitsfile *fptr)
{
  size_t i;
  int status=0;
  char *cp, *cpf, blankrec[80], titlerec[80];

  /* Just in case title is 'NULL'. */
  if(title)
    {
      /* A small sanity check. */
      if( strlen(title) + strlen(GAL_FITS_KEY_TITLE_START) > 78 )
        fprintf(stderr, "%s: FITS keyword title '%s' is too long to be fully "
                "included in the keyword record (80 characters, where the "
                "title is prefixed with %zu space characters)",
                __func__, title, strlen(GAL_FITS_KEY_TITLE_START));

      /* Set the last element of the blank array. */
      cpf=blankrec+79;
      *cpf='\0';
      titlerec[79]='\0';
      cp=blankrec; do *cp=' '; while(++cp<cpf);

      /* Print the blank lines before the title. */
      if(fits_write_record(fptr, blankrec, &status))
        gal_fits_io_error(status, NULL);

      /* Print the title */
      sprintf(titlerec, "%s%s", GAL_FITS_KEY_TITLE_START, title);
      for(i=strlen(titlerec);i<79;++i)
        titlerec[i]=' ';
      if(fits_write_record(fptr, titlerec, &status))
        gal_fits_io_error(status, NULL);
    }
}





/* Write a filename into keyword values. */
void
gal_fits_key_write_filename(char *keynamebase, char *filename,
                            gal_fits_list_key_t **list, int top1end0,
                            int quiet)
{
  char *keyname, *value;
  size_t numkey=1, maxlength;
  size_t i, j, len=strlen(filename), thislen;

  /* When you give string arguments, CFITSIO puts them within two ''s,
     so the actual length available is two less. It seems this length
     also didn't include the null character, so, ultimately you have
     to take three from it. */
  maxlength=FLEN_VALUE-3;

  /* Parse the filename and see when it is necessary to break it. */
  i=0;
  while(i<len)
    {
      /* Set the keyname: */
      errno=0;
      keyname=malloc(FLEN_KEYWORD);
      if(keyname==NULL)
        error(EXIT_FAILURE, errno, "%s: %d bytes for 'keyname'", __func__,
              FLEN_KEYWORD);
      if(len<maxlength)
        strcpy(keyname, keynamebase);
      else
        sprintf(keyname, "%s_%zu", keynamebase, numkey++);

      /* Set the keyword value: */
      errno=0;
      thislen=strlen(&filename[i]);
      value=malloc(maxlength+1);
      if(value==NULL)
        error(EXIT_FAILURE, errno, "%s: allocating %zu bytes", __func__,
              thislen);
      strncpy(value, &filename[i], maxlength);

      /* If the FROM string (=&filename[i]) in strncpy is shorter than
         SIZE (=maxlength), then the rest of the space will be filled
         with null characters. So we can use this to check if the full
         length was copied. */
      if(value[maxlength-1]=='\0')
        {
          if(top1end0)
            gal_fits_key_list_add(list, GAL_TYPE_STRING, keyname, 1,
                                  value, 1, NULL, 0, NULL, 0);
          else
            gal_fits_key_list_add_end(list, GAL_TYPE_STRING, keyname, 1,
                                      value, 1, NULL, 0, NULL, 0);
          break;
        }
      else
        {
          /* Find the last place in the copied array that contains a
             '/' and put j on the next character (so it can be turned
             into a null character. */
          for(j=maxlength-2;j>0;--j)
            if(value[j]=='/')
              {
                value[j+1]='\0';
                break;
              }
          if(j==0)
            {
              /* So the loop doesn't continue after this. */
              i=len+1;

              /* There will only be one keyword, so we'll just use the
                 basename. */
              strcpy(keyname, keynamebase);

              /* Let the user know that the name will be truncated. */
              if(quiet==0)
                error(0,0, "%s: WARNING: the filename '%s' (not "
                      "including directories) is too long to fit into "
                      "a FITS keyword value (max of %zu characters). "
                      "It will therefore be truncated. If you are "
                      "using Gnuastro's programs, this message is "
                      "only about the metadata (keyword that keeps "
                      "name of input), so it won't affect the output "
                      "analysis and data. In this case, you can suppress "
                      "this warning message with a '--quiet' option",
                      __func__, filename, maxlength);
            }

          /* Convert the last useful character and save the file name. */
          if(top1end0)
            gal_fits_key_list_add(list, GAL_TYPE_STRING, keyname, 1,
                                  value, 1, NULL, 0, NULL, 0);
          else
            gal_fits_key_list_add_end(list, GAL_TYPE_STRING, keyname, 1,
                                      value, 1, NULL, 0, NULL, 0);
          i+=j+1;
        }
    }
}





/* A bug was found in WCSLIB that has existed since WCSLIB 5.9 (released on
   2015/07/21) and will be fixed in the version after WCSLIB 7.3.1
   (released in 2020). However, it will take time for many user package
   managers to update their WCSLIB version. So we need to check if that bug
   has occurred here and fix it manually.

   In short the bug was originally seen on a dataset with this input CDELT
   values:

     CDELT1  =            0.0007778 / [deg]
     CDELT2  =            0.0007778 / [deg]
     CDELT3  =        30000.0000000 / [Hz]

   The values are read into the 'wcsprm' structure properly, but upon
   writing into the keyword string structure, the 'CDELT1' and 'CDELT2'
   values are printed as 0. Mark Calabretta (creator of WCSLIB) described
   the issue as follows:

        """wcshdo() tries to find a single sprintf() floating point format
        to use for groups of keywords, such as CDELTj as a group, or
        CRPIXi, PCi_j, and CRVALj, each as separate groups.  It aims to
        present the keyvalues in human-readable form, i.e. with decimal
        points lined up, no unnecessary trailing zeroes, and avoiding
        exponential ('E') format where its use is not warranted.

        The problem here arose because of the large range of CDELT values
        formatted using 'f' format, but not being so large that it would
        force wcshdo() to revert to 'E' format.  There is also the
        troubling possibility that in less extreme cases, precision of the
        CDELT (or other) values could be lost without being noticed."""

   To implement the check we will follow this logic: large dimensional
   differences will not commonly happen in 2D datasets, so we will only
   attempt the check in 3D datasets. We'll read each written CDELT value
   with CFITSIO and if its zero, we'll correct it. */
static void
fits_bug_wrapper_cdelt_zero(fitsfile *fptr, struct wcsprm *wcs, char *keystr)
{
  char *keyname;
  double keyvalue;
  size_t dim=GAL_BLANK_SIZE_T;
  int status=0, datatype=TDOUBLE;

  /* Only do this check when we have more than two dimensions. */
  if(wcs->naxis>2 && !strncmp(keystr, "CDELT", 5))
    {
      /* Find the dimension number and keyword string. This can later be
         improved/generalized by actually parsing the keyword name to
         extract the dimension, but I didn't have time to implement it in
         the first implementation. It will rarely be necessary to go beyond
         the third dimension, so this has almost no extra burden on the
         computer's processing. */
      if(      !strncmp(keystr, "CDELT1", 6) ) { keyname="CDELT1"; dim=0; }
      else if( !strncmp(keystr, "CDELT2", 6) ) { keyname="CDELT2"; dim=1; }
      else if( !strncmp(keystr, "CDELT3", 6) ) { keyname="CDELT3"; dim=2; }
      else if( !strncmp(keystr, "CDELT4", 6) ) { keyname="CDELT4"; dim=3; }
      else if( !strncmp(keystr, "CDELT5", 6) ) { keyname="CDELT5"; dim=4; }
      else if( !strncmp(keystr, "CDELT6", 6) ) { keyname="CDELT6"; dim=5; }
      else if( !strncmp(keystr, "CDELT7", 6) ) { keyname="CDELT7"; dim=6; }
      else if( !strncmp(keystr, "CDELT8", 6) ) { keyname="CDELT8"; dim=7; }
      else if( !strncmp(keystr, "CDELT9", 6) ) { keyname="CDELT9"; dim=8; }
      else
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix "
              "the problem. There appears to be more than 9 dimensions in "
              "the input dataset", __func__, PACKAGE_BUGREPORT);

      /* Read the keyword value. */
      fits_read_key(fptr, datatype, keyname, &keyvalue, NULL, &status);
      gal_fits_io_error(status, NULL);

      /* If the written value is not the same by more than 10 decimals,
         re-write the value. */
      if( fabs( wcs->cdelt[dim] - keyvalue ) > 1e-10  )
        {
          fits_update_key(fptr, datatype, keyname, &wcs->cdelt[dim],
                          NULL, &status);
          gal_fits_io_error(status, NULL);
        }
    }
}





/* Write the WCS header string into a FITS files. */
void
gal_fits_key_write_wcsstr(fitsfile *fptr, struct wcsprm *wcs,
                          char *wcsstr, int nkeyrec)
{
  size_t i;
  int status=0;
  char *keystart;

  /* If the 'wcsstr' string is empty, don't do anything, simply return. */
  if(wcsstr==NULL) return;

  /* Write the title. */
  gal_fits_key_write_title_in_ptr("World Coordinate System (WCS)", fptr);

  /* Write the keywords one by one: */
  for(i=0;i<nkeyrec;++i)
    {
      /* Set the start of this header. */
      keystart=&wcsstr[i*80];

      /* Write it if it isn't blank (first character is a space), or not a
         comment (first 7 characters equal to 'COMMENT'). The reason is
         that WCSLIB adds a blank line and a 'COMMENT' keyword saying its
         own version. But Gnuastro writes the version of WCSLIB as a
         separate keyword along with all other important software, so it is
         redundant and just makes the keywrods hard to read by eye. */
      if( keystart[0]!=' ' && strncmp(keystart, "COMMENT", 7) )
        {
          fits_write_record(fptr, keystart, &status);
          if(wcs) fits_bug_wrapper_cdelt_zero(fptr, wcs, keystart);
        }
    }
  gal_fits_io_error(status, NULL);

   /* WCSLIB is going to write PC+CDELT keywords in any case. But when we
      have a TPV, TNX or ZPX distortion, it is cleaner to use a CD matrix
      (WCSLIB can't convert coordinates properly if the PC matrix is used
      with the TPV distortion). So to help users avoid the potential
      problems with WCSLIB, upon reading the WCS structure, in such cases,
      we set CDELTi=1.0 and 'altlin=2' (signifying that the CD matrix
      should be used). Therefore, effectively the CD matrix and PC matrix
      are equivalent, we just need to convert the keyword names and delete
      the CDELT keywords. Note that zero-valued PC/CD elements may not be
      present, so we'll manually set 'status' to zero to avoid CFITSIO from
      crashing. */
  if(wcs && wcs->altlin==2)
    {
      status=0; fits_delete_str(fptr, "CDELT1", &status);
      status=0; fits_delete_str(fptr, "CDELT2", &status);
      status=0; fits_modify_name(fptr, "PC1_1", "CD1_1", &status);
      status=0; fits_modify_name(fptr, "PC1_2", "CD1_2", &status);
      status=0; fits_modify_name(fptr, "PC2_1", "CD2_1", &status);
      status=0; fits_modify_name(fptr, "PC2_2", "CD2_2", &status);
      if(wcs->naxis==3)
        {
          status=0; fits_delete_str(fptr, "CDELT3", &status);
          status=0; fits_modify_name(fptr, "PC1_3", "CD1_3", &status);
          status=0; fits_modify_name(fptr, "PC2_3", "CD2_3", &status);
          status=0; fits_modify_name(fptr, "PC3_1", "CD3_1", &status);
          status=0; fits_modify_name(fptr, "PC3_2", "CD3_2", &status);
          status=0; fits_modify_name(fptr, "PC3_3", "CD3_3", &status);
        }
      status=0;
    }
}





/* Write the given list of header keywords into the specified HDU of the
   specified FITS file. */
void
gal_fits_key_write(gal_fits_list_key_t **keylist, char *title,
                   char *filename, char *hdu, char *hdu_option_name)
{
  int status=0;
  fitsfile *fptr=gal_fits_hdu_open(filename, hdu, READWRITE, 1,
                                   hdu_option_name);

  /* Write the title. */
  gal_fits_key_write_title_in_ptr(title, fptr);

  /* Write the keywords into the FITS file pointer ('fptr'). */
  gal_fits_key_write_in_ptr(keylist, fptr);

  /* Close the input FITS file. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
}





/* Fits doesn't allow NaN values, so if the type of float or double, we'll
   just check to see if its NaN or not and let the user know the keyword
   name (to help them fix it). */
static void *
gal_fits_key_write_in_ptr_nan_check(gal_fits_list_key_t *tmp)
{
  void *out=NULL;
  int nanwarning=0;

  /* Check the value. */
  switch(tmp->type)
    {
    case GAL_TYPE_FLOAT32:
      if( isnan( ((float *)(tmp->value))[0] ) ) nanwarning=1;
      break;
    case GAL_TYPE_FLOAT64:
      if( isnan( ((double *)(tmp->value))[0] ) ) nanwarning=1;
      break;
    }

  /* Print the warning. */
  if(nanwarning)
    {
      out=gal_pointer_allocate(tmp->type, 1, 0, __func__, "out");
      gal_type_max(tmp->type, out);
      error(EXIT_SUCCESS, 0, "%s: (WARNING) value of '%s' is NaN "
            "and FITS doesn't recognize a NaN key value; instead, "
            "the following value (largest value of the %d-bit "
            "floating point type) will be written: %g", __func__,
            tmp->keyname, tmp->type==GAL_TYPE_FLOAT32 ? 32 : 64,
            ( tmp->type==GAL_TYPE_FLOAT32
              ? ((float  *)(out))[0]
              : ((double *)(out))[0] ) );
    }

  /* Return the allocated array. */
  return out;
}





/* Write the keywords in the gal_fits_list_key_t linked list to the FITS
   file. Every keyword that is written is freed, that is why we need the
   pointer to the linked list (to correct it after we finish). */
void
gal_fits_key_write_in_ptr(gal_fits_list_key_t **keylist, fitsfile *fptr)
{
  int status=0;
  void *ifnan=NULL;
  gal_fits_list_key_t *tmp, *ttmp;

  tmp=*keylist;
  while(tmp!=NULL)
    {
      /* If a title is requested, only put a title. */
      if(tmp->title)
        {
          gal_fits_key_write_title_in_ptr(tmp->title, fptr);
          if(tmp->tfree) free(tmp->title);
        }
      else if (tmp->fullcomment)
        {
          if( fits_write_comment(fptr, tmp->fullcomment, &status) )
            gal_fits_io_error(status, NULL);
          if(tmp->fcfree) free(tmp->fullcomment);
        }
      else
        {
          /* Write the basic key value and comments. */
          if(tmp->value)
            {
              /* Print a warning if the value is NaN. */
              ifnan=gal_fits_key_write_in_ptr_nan_check(tmp);

              /* Write/Update the keyword value. */
              if( fits_update_key(fptr,
                                  gal_fits_type_to_datatype(tmp->type),
                                  tmp->keyname, ifnan?ifnan:tmp->value,
                                  tmp->comment, &status) )
                gal_fits_io_error(status, NULL);
              if(ifnan) free(ifnan);
            }
          else
            {
              if(fits_update_key_null(fptr, tmp->keyname, tmp->comment,
                                      &status))
                gal_fits_io_error(status, NULL);
            }

          /* Write the units if it was given. */
          if( tmp->unit
              && fits_write_key_unit(fptr, tmp->keyname, tmp->unit,
                                     &status) )
            gal_fits_io_error(status, NULL);

          /* Free the value pointer if desired: */
          if(tmp->ufree) free(tmp->unit);
          if(tmp->vfree) free(tmp->value);
          if(tmp->kfree) free(tmp->keyname);
          if(tmp->cfree) free(tmp->comment);
        }

      /* Keep the pointer to the next keyword and free the allocated
         space for this keyword. */
      ttmp=tmp->next;
      free(tmp);
      tmp=ttmp;
    }

  /* Set it to NULL so it isn't mistakenly used later. */
  *keylist=NULL;
}





/* Write the given list of header keywords and version info into the give
   file. */
void
gal_fits_key_write_version(gal_fits_list_key_t **keylist, char *title,
                           char *filename, char *hdu,
                           char *hdu_option_name)
{
  int status=0;
  fitsfile *fptr=gal_fits_hdu_open(filename, hdu, READWRITE, 1,
                                   hdu_option_name);

  /* Write the given keys followed by the versions. */
  gal_fits_key_write_version_in_ptr(keylist, title, fptr);

  /* Close the input FITS file. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
}





void
gal_fits_key_write_version_in_ptr(gal_fits_list_key_t **keylist,
                                  char *title, fitsfile *fptr)
{
  int status=0;
  char *gitdescribe;
  char cfitsioversion[20];

  /* Before WCSLIB 5.0, the wcslib_version function was not
     defined. Sometime in the future were everyone has moved to more
     recent versions of WCSLIB, we can remove this macro and its check
     in configure.ac. */
#if GAL_CONFIG_HAVE_WCSLIB_VERSION == 1
  int wcslibvers[3];
  char wcslibversion[20];
  const char *wcslibversion_const;
#endif

  /* Small sanity check. */
  if(fptr==NULL)
    error(EXIT_FAILURE, 0, "%s: input FITS pointer is NULL", __func__);

  /* If any header keywords are specified, add them: */
  if(keylist && *keylist)
    {
      gal_fits_key_write_title_in_ptr(title?title:"Specific keys", fptr);
      gal_fits_key_write_in_ptr(keylist, fptr);
    }

  /* Print 'Versions and date' title. */
  gal_fits_key_write_title_in_ptr("Versions and date", fptr);

  /* Set the version of CFITSIO as a string: before version 4.0.0 of
     CFITSIO, there were only two numbers in the version (for example
     '3.49' and '3.48'), but from the 4th major release, there are three
     numbers in the version string. The third number corresponds to a new
     'CFITSIO_MICRO' macro. So if it doesn't exist, we'll just print two
     numbers, otherwise, we'll print the three. */
#ifdef CFITSIO_MICRO
  sprintf(cfitsioversion, "%d.%d.%d", CFITSIO_MAJOR,
          CFITSIO_MINOR, CFITSIO_MICRO);
#else
  sprintf(cfitsioversion, "%d.%d", CFITSIO_MAJOR,
          CFITSIO_MINOR);
#endif

  /* Write all the information: */
  fits_write_date(fptr, &status);

  /* Write the version of CFITSIO. */
  fits_update_key(fptr, TSTRING, "CFITSIO", cfitsioversion,
                  "CFITSIO version.", &status);

  /* Write the WCSLIB version. */
#if GAL_CONFIG_HAVE_WCSLIB_VERSION == 1
  wcslibversion_const=wcslib_version(wcslibvers);
  strcpy(wcslibversion, wcslibversion_const);
  fits_update_key(fptr, TSTRING, "WCSLIB", wcslibversion,
                  "WCSLIB version.", &status);
#endif

  /* Write the GSL version. */
  fits_update_key(fptr, TSTRING, "GSL", GSL_VERSION,
                  "GNU Scientific Library version.", &status);

  /* Write the Gnuastro version. */
  fits_update_key(fptr, TSTRING, "GNUASTRO", PACKAGE_VERSION,
                  "GNU Astronomy Utilities version.", &status);

  /* If we are in a version controlled directory and have libgit2
     installed, write the commit description into the FITS file. */
  gitdescribe=gal_git_describe();
  if(gitdescribe)
    {
      fits_update_key(fptr, TSTRING, "COMMIT", gitdescribe,
                      "Git commit in running directory.", &status);
      free(gitdescribe);
    }

  /* Report any error if a problem came up. */
  gal_fits_io_error(status, NULL);
}






/* Write the given keywords into the given extension of the given file,
   ending it with version information. This is primarily intended for
   writing configuration settings of a program into the zero-th extension
   of the FITS file (which is empty when the FITS file is created by
   Gnuastro's program and this library). */
void
gal_fits_key_write_config(gal_fits_list_key_t **keylist, char *title,
                          char *extname, char *filename, char *hdu,
                          char *hdu_option_name)
{
  int status=0;
  fitsfile *fptr=gal_fits_hdu_open(filename, hdu, READWRITE, 1,
                                   hdu_option_name);

  /* Delete the two extra comment lines describing the FITS standard that
     CFITSIO puts in when it creates a new extension. We'll set status to 0
     afterwards so even if they don't exist, the program continues
     normally. */
  fits_delete_key(fptr, "COMMENT", &status);
  fits_delete_key(fptr, "COMMENT", &status);
  status=0;

  /* Put a name for the zero-th extension. */
  if(fits_write_key(fptr, TSTRING, "EXTNAME", extname, "", &status))
    gal_fits_io_error(status, NULL);

  /* Write all the given keywords. */
  gal_fits_key_write_version_in_ptr(keylist, title, fptr);

  /* Close the FITS file. */
  if( fits_close_file(fptr, &status) )
    gal_fits_io_error(status, NULL);
}





/* From an input list of FITS files and a HDU, select those that have a
   certain value(s) in a certain keyword. */
gal_list_str_t *
gal_fits_with_keyvalue(gal_list_str_t *files, char *hdu, char *name,
                       gal_list_str_t *values, char *hdu_option_name)
{
  int status=0;
  fitsfile *fptr;
  char keyvalue[FLEN_VALUE];
  gal_list_str_t *f, *v, *out=NULL;

  /* Go over the list of files and see if they have the requested
     keyword(s). */
  for(f=files; f!=NULL; f=f->next)
    {
      /* Open the file. */
      fptr=gal_fits_hdu_open(f->v, hdu, READONLY, 0, hdu_option_name);

      /* Only attempt to read the value if the requested HDU could be
         opened ('fptr!=NULL'). */
      if(fptr)
        {
          /* Check if the keyword actually exists. */
          if( gal_fits_key_exists_fptr(fptr, name) )
            {
              /* Read the keyword. Note that we aren't checking for the
                 'status' here. If for any reason CFITSIO couldn't read the
                 value and status if non-zero, the next step won't consider
                 the file any more. */
              status=0;
              fits_read_key(fptr, TSTRING, name, &keyvalue, NULL,
                            &status);

              /* If the value corresponds to any of the user's values for
                 this keyword, add it to the list of output names. */
              if(status==0)
                for(v=values; v!=NULL; v=v->next)
                  {
                    if( strcmp(v->v, keyvalue)==0 )
                      { gal_list_str_add(&out, f->v, 1); break; }
                  }
            }

          /* Close the file. */
          if( fits_close_file(fptr, &status) )
            gal_fits_io_error(status, NULL);
        }
    }

  /* Reverse the list to be in same order as input and return. */
  gal_list_str_reverse(&out);
  return out;
}





/* From an input list of FITS files and a HDU, select those that have a
   certain value(s) in a certain keyword. */
gal_list_str_t *
gal_fits_unique_keyvalues(gal_list_str_t *files, char *hdu, char *name,
                          char *hdu_option_name)
{
  fitsfile *fptr;
  int status=0, newvalue;
  char *keyv, keyvalue[FLEN_VALUE];
  gal_list_str_t *f, *v, *out=NULL;

  /* Go over the list of files and see if they have the requested
     keyword(s). */
  for(f=files; f!=NULL; f=f->next)
    {
      /* Open the file. */
      fptr=gal_fits_hdu_open(f->v, hdu, READONLY, 0, hdu_option_name);

      /* Only attempt to read the value if the requested HDU could be
         opened ('fptr!=NULL'). */
      if(fptr)
        {
          /* Check if the keyword actually exists. */
          if( gal_fits_key_exists_fptr(fptr, name) )
            {
              /* Read the keyword. Note that we aren't checking for the
                 'status' here. If for any reason CFITSIO couldn't read the
                 value and status if non-zero, the next step won't consider
                 the file any more. */
              status=0;
              fits_read_key(fptr, TSTRING, name, &keyvalue, NULL,
                            &status);

              /* If the value is new, add it to the list. */
              if(status==0)
                {
                  newvalue=1;
                  keyv=gal_txt_trim_space(keyvalue);
                  for(v=out; v!=NULL; v=v->next)
                    { if( strcmp(v->v, keyv)==0 ) newvalue=0; }
                  if(newvalue) gal_list_str_add(&out, keyv, 1);
                }
            }

          /* Close the file. */
          if( fits_close_file(fptr, &status) )
            gal_fits_io_error(status, NULL);
        }
    }

  /* Reverse the list to be in same order as input and return. */
  gal_list_str_reverse(&out);
  return out;
}





















/*************************************************************
 ***********            Array functions            ***********
 *************************************************************/

/* Note that the FITS standard defines any array as an 'image',
   irrespective of how many dimensions it has. This function will return
   the Gnuastro-type, the number of dimensions and size along each
   dimension of the image along with its name and units if necessary (not
   NULL). Note that '*dsize' will be allocated here, so it must not point
   to any already allocated space. */
void
gal_fits_img_info(fitsfile *fptr, int *type, size_t *ndim, size_t **dsize,
                  char **name, char **unit)
{
  double bscale=NAN;
  size_t i, dsize_key=1;
  char **str, *bzero_str=NULL;
  int bitpix, status=0, naxis;
  gal_data_t *key, *keysll=NULL;
  long naxes[GAL_FITS_MAX_NDIM];

  /* Get the BITPIX, number of dimensions and size of each dimension. */
  if( fits_get_img_param(fptr, GAL_FITS_MAX_NDIM, &bitpix, &naxis,
                         naxes, &status) )
    gal_fits_io_error(status, NULL);
  *ndim=naxis;


  /* Convert bitpix to Gnuastro's known types. */
  *type=gal_fits_bitpix_to_type(bitpix);


  /* Define the names of the possibly existing important keywords about the
     dataset. We are defining these in the opposite order to be read by
     CFITSIO. The way Gnuastro writes the FITS keywords, the output will
     first have 'BZERO', then 'BSCALE', then 'EXTNAME', then, 'BUNIT'.*/
  gal_list_data_add_alloc(&keysll, NULL, GAL_TYPE_STRING, 1, &dsize_key,
                          NULL, 0, -1, 1, "BUNIT", NULL, NULL);
  gal_list_data_add_alloc(&keysll, NULL, GAL_TYPE_STRING, 1, &dsize_key,
                          NULL, 0, -1, 1, "EXTNAME", NULL, NULL);
  gal_list_data_add_alloc(&keysll, NULL, GAL_TYPE_FLOAT64, 1, &dsize_key,
                          NULL, 0, -1, 1, "BSCALE", NULL, NULL);
  gal_list_data_add_alloc(&keysll, NULL, GAL_TYPE_STRING, 1, &dsize_key,
                          NULL, 0, -1, 1, "BZERO", NULL, NULL);
  gal_fits_key_read_from_ptr(fptr, keysll, 0, 0);


  /* Read the special keywords. */
  i=1;
  for(key=keysll;key!=NULL;key=key->next)
    {
      /* Recall that the order is the opposite (this is a last-in-first-out
         list. */
      if(key->status==0)
        {
        switch(i)
          {
          case 4: if(unit) {str=key->array; *unit=*str; *str=NULL;} break;
          case 3: if(name) {str=key->array; *name=*str; *str=NULL;} break;
          case 2: bscale = *(double *)(key->array);                 break;
          case 1: str = key->array; bzero_str = *str;               break;
          default:
            error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s "
                  "to fix the problem. For some reason, there are more "
                  "keywords requested ", __func__, PACKAGE_BUGREPORT);
          }
        }
      ++i;
    }


  /* If a type correction is necessary, then do it. */
  if( !isnan(bscale) || bzero_str )
    fits_type_correct(type, bscale, bzero_str);


  /* Allocate the array to keep the dimension size and fill it in, note
     that its order is the opposite of naxes. */
  *dsize=gal_pointer_allocate(GAL_TYPE_INT64, *ndim, 0, __func__, "dsize");
  for(i=0; i<*ndim; ++i)
    (*dsize)[i]=naxes[*ndim-1-i];


  /* Clean up. Note that bzero_str, gets freed by 'gal_data_free' (which is
     called by 'gal_list_data_free'. */
  gal_list_data_free(keysll);
}





/* Get the basic array info to remove extra dimensions if necessary. */
size_t *
gal_fits_img_info_dim(char *filename, char *hdu, size_t *ndim,
                      char *hdu_option_name)
{
  fitsfile *fptr;
  size_t *dsize=NULL;
  int status=0, type;

  /* Open the given header, read the basic image information and close it
     again. */
  fptr=gal_fits_hdu_open(filename, hdu, READONLY, 1,
                         hdu_option_name);
  gal_fits_img_info(fptr, &type, ndim, &dsize, NULL, NULL);
  if( fits_close_file(fptr, &status) ) gal_fits_io_error(status, NULL);

  return dsize;
}





/* Read a FITS image HDU into a Gnuastro data structure. */
gal_data_t *
gal_fits_img_read(char *filename, char *hdu, size_t minmapsize,
                  int quietmmap, char *hdu_option_name)
{
  void *blank;
  long *fpixel;
  fitsfile *fptr;
  gal_data_t *img;
  size_t i, ndim, *dsize;
  char *name=NULL, *unit=NULL;
  int status=0, type, anyblank;
  char *hduon = hdu_option_name ? hdu_option_name : "--hdu";


  /* Check HDU for realistic conditions: */
  fptr=gal_fits_hdu_open_format(filename, hdu, 0, NULL);


  /* Get the info and allocate the data structure. */
  gal_fits_img_info(fptr, &type, &ndim, &dsize, &name, &unit);


  /* Check if there is any dimensions (the first header can sometimes have
     no images). */
  if(ndim==0)
    error(EXIT_FAILURE, 0, "%s (hdu: %s) has 0 dimensions! The most "
          "common cause for this is a wrongly specified HDU. In some "
          "FITS images, the first HDU doesn't have any data, the data "
          "is in subsequent extensions. So probably reading the second "
          "HDU (with '%s=1') will solve the problem (following CFITSIO's "
          "convention, currently HDU counting starts from 0)", filename,
          hdu, hduon);


  /* Set the fpixel array (first pixel in all dimensions). Note that the
     'long' type will not be larger than 64-bits, so, we'll just assume it
     is 64-bits for space allocation. On 32-bit systems, this won't be a
     problem, the space will be written/read as 32-bit 'long' any way,
     we'll just have a few empty bytes that will be freed anyway at the end
     of this function. */
  fpixel=gal_pointer_allocate(GAL_TYPE_INT64, ndim, 0, __func__, "fpixel");
  for(i=0;i<ndim;++i) fpixel[i]=1;


  /* Allocate the space for the array and for the blank values. */
  img=gal_data_alloc(NULL, type, ndim, dsize, NULL, 0, minmapsize,
                     quietmmap, name, unit, NULL);
  blank=gal_blank_alloc_write(type);
  if(name) free(name);
  if(unit) free(unit);
  free(dsize);


  /* Read the image into the allocated array: */
  fits_read_pix(fptr, gal_fits_type_to_datatype(type), fpixel,
                img->size, blank, img->array, &anyblank, &status);
  if(status) gal_fits_io_error(status, NULL);
  free(fpixel);
  free(blank);


  /* Close the input FITS file. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);


  /* Return the filled data structure. */
  return img;
}





/* The user has specified an input file + extension, and your program needs
   this input to be a special type. For such cases, this function can be
   used to convert the input file to the desired type. */
gal_data_t *
gal_fits_img_read_to_type(char *inputname, char *hdu, uint8_t type,
                          size_t minmapsize, int quietmmap,
                          char *hdu_option_name)
{
  gal_data_t *in, *converted;

  /* Read the specified input image HDU. */
  in=gal_fits_img_read(inputname, hdu, minmapsize, quietmmap,
                       hdu_option_name);

  /* If the input had another type, convert it to float. */
  if(in->type!=type)
    {
      converted=gal_data_copy_to_new_type(in, type);
      gal_data_free(in);
      in=converted;
    }

  /* Return the final structure. */
  return in;
}





gal_data_t *
gal_fits_img_read_kernel(char *filename, char *hdu, size_t minmapsize,
                         int quietmmap, char *hdu_option_name)
{
  size_t i;
  int check=0;
  double sum=0;
  gal_data_t *kernel;
  float *f, *fp, tmp;

  /* Read the image as a float and if it has a WCS structure, free it. */
  kernel=gal_fits_img_read_to_type(filename, hdu, GAL_TYPE_FLOAT32,
                                   minmapsize, quietmmap, hdu_option_name);
  if(kernel->wcs) { wcsfree(kernel->wcs); kernel->wcs=NULL; }

  /* Check if the size along each dimension of the kernel is an odd
     number. If they are all an odd number, then the for each dimension,
     check will be incremented once. */
  for(i=0;i<kernel->ndim;++i)
    check += kernel->dsize[i]%2;
  if(check!=kernel->ndim)
    error(EXIT_FAILURE, 0, "%s: the kernel image has to have an odd number "
          "of pixels in all dimensions (there has to be one element/pixel "
          "in the center). At least one of the dimensions of %s (hdu: %s) "
          "doesn't have an odd number of pixels", __func__, filename, hdu);

  /* If there are any NaN pixels, set them to zero and normalize it. A
     blank pixel in a kernel is going to make a completely blank output. */
  fp=(f=kernel->array)+kernel->size;
  do
    {
      if(isnan(*f)) *f=0.0f;
      else          sum+=*f;
    }
  while(++f<fp);
  kernel->flag |= GAL_DATA_FLAG_BLANK_CH;
  kernel->flag &= ~GAL_DATA_FLAG_HASBLANK;
  f=kernel->array; do *f++ *= 1/sum; while(f<fp);

  /* Flip the kernel about the center (necessary for non-symmetric
     kernels). */
  f=kernel->array;
  for(i=0;i<kernel->size/2;++i)
    {
      tmp=f[i];
      f[i]=f[ kernel->size - i - 1 ];
      f[ kernel->size - i - 1 ]=tmp;
    }

  /* Return the kernel. */
  return kernel;
}





/* This function will write all the data array information (including its
   WCS information) into a FITS file, but will not close it. Instead it
   will pass along the FITS pointer for further modification. */
fitsfile *
gal_fits_img_write_to_ptr(gal_data_t *input, char *filename)
{
  void *blank;
  int64_t *i64;
  char *u64key;
  fitsfile *fptr;
  uint64_t *u64, *u64f;
  long fpixel=1, *naxes;
  size_t i, ndim=input->ndim;
  int hasblank, status=0, datatype=0;
  gal_data_t *i64data, *towrite, *block=gal_tile_block(input);

  /* Small sanity check. */
  if( gal_fits_name_is_fits(filename)==0 )
    error(EXIT_FAILURE, 0, "%s: not a FITS suffix", filename);


  /* If the input is a tile (isn't a contiguous region of memory), then
     copy it into a contiguous region. */
  towrite = input==block ? input : gal_data_copy(input);
  hasblank=gal_blank_present(towrite, 0);


  /* Allocate the naxis area. */
  naxes=gal_pointer_allocate( ( sizeof(long)==8
                                ? GAL_TYPE_INT64
                                : GAL_TYPE_INT32 ), ndim, 0, __func__,
                              "naxes");


  /* Open the file for writing. */
  fptr=gal_fits_open_to_write(filename);


  /* Fill the 'naxes' array (in opposite order, and 'long' type): */
  for(i=0;i<ndim;++i) naxes[ndim-1-i]=towrite->dsize[i];


  /* Create the FITS file. Unfortunately CFITSIO doesn't have a macro for
     UINT64, TLONGLONG is only for (signed) INT64. So if the dataset has
     that type, we'll have to convert it to 'INT64' and in the mean-time
     shift its zero, we will then have to write the BZERO and BSCALE
     keywords accordingly. */
  if(block->type==GAL_TYPE_UINT64)
    {
      /* Allocate the necessary space. */
      i64data=gal_data_alloc(NULL, GAL_TYPE_INT64, ndim, towrite->dsize,
                             NULL, 0, block->minmapsize, block->quietmmap,
                             NULL, NULL, NULL);

      /* Copy the values while making the conversion. */
      i64=i64data->array;
      u64f=(u64=towrite->array)+towrite->size;
      if(hasblank)
        {
          do *i64++ = ( *u64==GAL_BLANK_UINT64
                        ? GAL_BLANK_INT64
                        : (*u64 + INT64_MIN) );
          while(++u64<u64f);
        }
      else
        do *i64++ = (*u64 + INT64_MIN); while(++u64<u64f);

      /* We can now use CFITSIO's signed-int64 type macros. */
      datatype=TLONGLONG;
      fits_create_img(fptr, LONGLONG_IMG, ndim, naxes, &status);
      gal_fits_io_error(status, NULL);

      /* Write the image into the file. */
      fits_write_img(fptr, datatype, fpixel, i64data->size, i64data->array,
                     &status);
      gal_fits_io_error(status, NULL);


      /* We need to write the BZERO and BSCALE keywords manually. VERY
         IMPORTANT: this has to be done after writing the array. We cannot
         write this huge integer as a variable, so we'll simply write the
         full record/card. It is just important that the string be larger
         than 80 characters, CFITSIO will trim the rest of the string. */
      u64key="BZERO   =  9223372036854775808 / Offset of data                                         ";
      fits_write_record(fptr, u64key, &status);
      u64key="BSCALE  =                    1 / Default scaling factor                                 ";
      fits_write_record(fptr, u64key, &status);
      gal_fits_io_error(status, NULL);
    }
  else
    {
      /* Set the datatype. */
      datatype=gal_fits_type_to_datatype(block->type);

      /* Create the FITS file. */
      fits_create_img(fptr, gal_fits_type_to_bitpix(towrite->type),
                      ndim, naxes, &status);
      gal_fits_io_error(status, NULL);

      /* Write the image into the file. */
      fits_write_img(fptr, datatype, fpixel, towrite->size, towrite->array,
                     &status);
      gal_fits_io_error(status, NULL);
    }


  /* Remove the two comment lines put by CFITSIO. Note that in some cases,
     it might not exist. When this happens, the status value will be
     non-zero. We don't care about this error, so to be safe, we will just
     reset the status variable after these calls. */
  fits_delete_key(fptr, "COMMENT", &status);
  fits_delete_key(fptr, "COMMENT", &status);
  status=0;


  /* If we have blank pixels, we need to define a BLANK keyword when we are
     dealing with integer types. */
  if(hasblank)
    switch(towrite->type)
      {
      case GAL_TYPE_FLOAT32:
      case GAL_TYPE_FLOAT64:
        /* Do nothing! Since there are much fewer floating point types
           (that don't need any BLANK keyword), we are checking them. */
        break;

      default:
        blank=gal_fits_key_img_blank(towrite->type);
        if(fits_write_key(fptr, datatype, "BLANK", blank,
                          "Pixels with no data.", &status) )
          gal_fits_io_error(status, "adding the BLANK keyword");
        free(blank);
      }


  /* Write the extension name to the header. */
  if(towrite->name)
    fits_write_key(fptr, TSTRING, "EXTNAME", towrite->name, "", &status);


  /* Write the units to the header. */
  if(towrite->unit)
    fits_write_key(fptr, TSTRING, "BUNIT", towrite->unit, "", &status);


  /* Write comments if they exist. */
  if(towrite->comment)
    fits_write_comment(fptr, towrite->comment, &status);


  /* If a WCS structure is present, write it in the FITS file pointer
     ('fptr'). */
  if(towrite->wcs)
    gal_wcs_write_in_fitsptr(fptr, towrite->wcs);


  /* If there were any errors, report them and return.*/
  free(naxes);
  gal_fits_io_error(status, NULL);
  if(towrite!=input) gal_data_free(towrite);
  return fptr;
}





void
gal_fits_img_write(gal_data_t *data, char *filename,
                   gal_fits_list_key_t *headers, char *program_string)
{
  int status=0;
  fitsfile *fptr;

  /* Write the data array into a FITS file and keep it open: */
  fptr=gal_fits_img_write_to_ptr(data, filename);

  /* Write all the headers and the version information. */
  gal_fits_key_write_version_in_ptr(&headers, program_string, fptr);

  /* Close the FITS file. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
}





void
gal_fits_img_write_to_type(gal_data_t *data, char *filename,
                           gal_fits_list_key_t *headers,
                           char *program_string, int type)
{
  /* If the input dataset is not the correct type, then convert it,
     otherwise, use the input data structure. */
  gal_data_t *towrite = (data->type==type
                         ? data
                         : gal_data_copy_to_new_type(data, type));

  /* Write the converted dataset into an image. */
  gal_fits_img_write(towrite, filename, headers, program_string);

  /* Free the dataset if it was allocated. */
  if(towrite!=data) gal_data_free(towrite);
}





/* This function is mainly useful when you want to make FITS files in
   parallel (from one main WCS structure, with just differing CRPIX) for
   two reasons:

      - When a large number of FITS images (with WCS) need to be created in
        parallel, it can be much more efficient to write the header's WCS
        keywords once at first, write them in the FITS file, then just
        correct the CRPIX values.

      - WCSLIB's header writing function is not thread safe. So when
        writing FITS images in parallel, we can't write the header keywords
        in each thread. */
void
gal_fits_img_write_corr_wcs_str(gal_data_t *input, char *filename,
                                char *wcsstr, int nkeyrec, double *crpix,
                                gal_fits_list_key_t *headers,
                                char *program_string)
{
  int status=0;
  fitsfile *fptr;

  /* The data should not have any WCS structure for this function. */
  if(input->wcs)
    error(EXIT_FAILURE, 0, "%s: input must not have WCS meta-data",
          __func__);

  /* Write the data array into a FITS file and keep it open. */
  fptr=gal_fits_img_write_to_ptr(input, filename);

  /* Write the WCS headers into the FITS file. */
  gal_fits_key_write_wcsstr(fptr, NULL, wcsstr, nkeyrec);

  /* Update the CRPIX keywords. Note that we don't want to change the
     values in the WCS information of gal_data_t. Because, it often happens
     that things are done in parallel, so we don't want to touch the
     original version, we just want to change the copied version. */
  if(crpix)
    {
      fits_update_key(fptr, TDOUBLE, "CRPIX1", &crpix[0], NULL, &status);
      fits_update_key(fptr, TDOUBLE, "CRPIX2", &crpix[1], NULL, &status);
      if(input->ndim==3)
        fits_update_key(fptr, TDOUBLE, "CRPIX3", &crpix[2], NULL, &status);
      gal_fits_io_error(status, NULL);
    }

  /* Write all the headers and the version information. */
  gal_fits_key_write_version_in_ptr(&headers, program_string, fptr);

  /* Close the file and return. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
}




















/**************************************************************/
/**********                 Table                  ************/
/**************************************************************/
/* Get the size of a table HDU. CFITSIO doesn't use size_t, also we want to
   check status here. */
void
gal_fits_tab_size(fitsfile *fitsptr, size_t *nrows, size_t *ncols)
{
  long lnrows;
  int incols, status=0;

  /* Read the sizes and put them in. */
  fits_get_num_rows(fitsptr, &lnrows, &status);
  fits_get_num_cols(fitsptr, &incols, &status);
  *ncols=incols;
  *nrows=lnrows;

  /* Report an error if any was issued. */
  gal_fits_io_error(status, NULL);
}





int
gal_fits_tab_format(fitsfile *fitsptr)
{
  int status=0;
  char value[FLEN_VALUE];

  fits_read_key(fitsptr, TSTRING, "XTENSION", value, NULL, &status);

  if(status==0)
    {
      if(!strcmp(value, "TABLE"))
        return GAL_TABLE_FORMAT_AFITS;
      else if(!strcmp(value, "BINTABLE"))
        return GAL_TABLE_FORMAT_BFITS;
      else
        error(EXIT_FAILURE, 0, "%s: the 'XTENSION' keyword of this FITS "
              "table ('%s') doesn't have a standard value", __func__,
              value);
    }
  else
    {
      if(status==KEY_NO_EXIST)
        error(EXIT_FAILURE, 0, "%s: input fitsfile pointer isn't a table",
              __func__);
      else
        gal_fits_io_error(status, NULL);
    }

  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s so we "
        "can fix it. Control should not have reached the end of this "
        "function", __func__, PACKAGE_BUGREPORT);
  return -1;
}





/* The general format of the TDISPn keywords in FITS is like this: 'Tw.p',
   where 'T' specifies the general format, 'w' is the width to be given to
   this column and 'p' is the precision. For integer types, percision is
   actually the minimum number of integers, for floats, it is the number of
   decimal digits beyond the decimal point. */
static void
set_display_format(char *tdisp, gal_data_t *data, char *filename, char *hdu,
                   char *keyname)
{
  int isanint=0;
  char *tailptr;

  /* First, set the general display format. */
  switch(tdisp[0])
    {
    case 'A':
      data->disp_fmt=GAL_TABLE_DISPLAY_FMT_STRING;
      break;

    case 'I':
      isanint=1;
      data->disp_fmt=GAL_TABLE_DISPLAY_FMT_DECIMAL;
      break;

    case 'O':
      isanint=1;
      data->disp_fmt=GAL_TABLE_DISPLAY_FMT_OCTAL;
      break;

    case 'Z':
      isanint=1;
      data->disp_fmt=GAL_TABLE_DISPLAY_FMT_HEX;
      break;

    case 'F':
      data->disp_fmt=GAL_TABLE_DISPLAY_FMT_FIXED;
      break;

    case 'E':
    case 'D':
      data->disp_fmt=GAL_TABLE_DISPLAY_FMT_EXP;
      break;

    case 'G':
      data->disp_fmt=GAL_TABLE_DISPLAY_FMT_GENERAL;
      break;

    default:
      error(EXIT_FAILURE, 0, "%s (hdu: %s): Format character '%c' in the "
            "value (%s) of the keyword %s not recognized in %s", filename,
            hdu, tdisp[0], tdisp, keyname, __func__);
    }

  /* Parse the rest of the string to see if a width and precision are given
     or not. */
  data->disp_width=strtol(&tdisp[1], &tailptr, 0);
  switch(*tailptr)
    {
    case '.':      /* Width is set, go onto finding the precision. */
      data->disp_precision = strtol(&tailptr[1], &tailptr, 0);
      if(*tailptr!='\0')
        error(EXIT_FAILURE, 0, "%s (hdu: %s): The value '%s' of the "
              "'%s' keyword could not recognized (it doesn't finish "
              "after the precision) in %s", filename, hdu, tdisp,
              keyname, __func__);
      break;

    case '\0':     /* No precision given, use a default value.     */
      data->disp_precision = ( isanint
                               ? GAL_TABLE_DEF_PRECISION_INT
                               : GAL_TABLE_DEF_PRECISION_FLT );
      break;

    default:
      error(EXIT_FAILURE, 0, "%s (hdu: %s): The value '%s' of the "
            "'%s' keyword could not recognized (it doesn't have a '.', "
            "or finish, after the width) in %s", filename, hdu, tdisp,
            keyname, __func__);
    }


}




/* The FITS standard for binary tables (not ASCII tables) does not allow
   unsigned types for short, int and long types, or signed char! So it has
   'TSCALn' and 'TZEROn' to scale the signed types to an unsigned type. It
   does this internally, but since we need to define our data type and
   allocate space for it before actually reading the array, it is necessary
   to do this setting here. */
static void
fits_correct_bin_table_int_types(gal_data_t *allcols, int tfields,
                                 int *tscal, long long *tzero)
{
  size_t i;

  for(i=0;i<tfields;++i)
    {
      /* If TSCALn is not 1, the reason for it isn't to use a different
         signed/unsigned type, so don't change anything. */
      if(tscal[i]!=1) continue;

      /* For a check
      printf("Column %zu initial type: %s (s: %d, z: %lld)\n", i+1,
             gal_data_type_as_string(allcols[i].type, 1), tscal[i],
             tzero[i]);
      */

      /* Correct the type based on the initial read type and the value to
         tzero. If tzero is any other value, then again, its not a type
         conversion, so just ignore it. */
      if(allcols[i].type==GAL_TYPE_UINT8 && tzero[i]==INT8_MIN)
        allcols[i].type = GAL_TYPE_INT8;

      else if ( allcols[i].type==GAL_TYPE_INT16
                && tzero[i] == -(long long)INT16_MIN )
        allcols[i].type = GAL_TYPE_UINT16;

      else if (allcols[i].type==GAL_TYPE_INT32
               && tzero[i] ==  -(long long)INT32_MIN)
        allcols[i].type = GAL_TYPE_UINT32;

      /* For a check
      printf("Column %zu corrected type: %s\n", i+1,
             gal_data_type_as_string(allcols[i].type, 1));
      */
    }
}





/* See the descriptions of 'gal_table_info'. */
gal_data_t *
gal_fits_tab_info(char *filename, char *hdu, size_t *numcols,
                  size_t *numrows, int *tableformat, char *hdu_option_name)
{
  long repeat;
  int tfields;        /* The maximum number of fields in FITS is 999. */
  char *tailptr;
  fitsfile *fptr;
  size_t i, index;
  long long *tzero;
  gal_data_t *allcols;
  int status=0, rkstatus=0, datatype, *tscal;
  char keyname[FLEN_KEYWORD]="XXXXXXXXXXXXX", value[FLEN_VALUE];

  /* Necessary when a keyword can't be written immediately as it is read in
     the FITS header and it actually depends on other data before. */
  gal_list_str_t   *tmp_n, *later_name=NULL;
  gal_list_str_t   *tmp_v, *later_value=NULL;
  gal_list_sizet_t *tmp_i, *later_index=NULL;


  /* Open the FITS file and get the basic information. */
  fptr=gal_fits_hdu_open_format(filename, hdu, 1, hdu_option_name);
  *tableformat=gal_fits_tab_format(fptr);
  gal_fits_tab_size(fptr, numrows, numcols);


  /* Read the total number of fields, then allocate space for the data
     structure array and store the information within it. */
  fits_read_key(fptr, TINT, "TFIELDS", &tfields, NULL, &status);
  allcols=gal_data_array_calloc(tfields);


  /* See comments of 'fits_correct_bin_table_int_types'. Here we are
     allocating the space to keep these values. */
  errno=0;
  tscal=calloc(tfields, sizeof *tscal);
  if(tscal==NULL)
    error(EXIT_FAILURE, errno, "%s: %zu bytes for tscal", __func__,
          tfields*sizeof *tscal);
  errno=0;
  tzero=calloc(tfields, sizeof *tzero);
  if(tzero==NULL)
    error(EXIT_FAILURE, errno, "%s: %zu bytes for tzero", __func__,
          tfields*sizeof *tzero);


  /* Read all the keywords one by one. If they match any of the standard
     Table metadata keywords, then put them in the correct value. Some
     notes about this loop:
       - We are starting from keyword 9 because according to the FITS
         standard, the first 8 keys in a FITS table are reserved.
       - When 'fits_read_keyn' has been able to read the keyword and its
         value, it will return '0'. The returned value is also stored in
         'rkstatus' (Read Keyword STATUS), which we need to check after the
         loop. */
  i=8;
  while( fits_read_keyn(fptr, ++i, keyname, value, NULL, &rkstatus) == 0 )
    {
      /* For string valued keywords, CFITSIO's function above, keeps the
         single quotes around the value string, one before and one
         after. 'gal_fits_key_clean_str_value' will remove these single
         quotes and any possible trailing space within the allocated
         space. */
      if(value[0]=='\'') gal_fits_key_clean_str_value(value);


      /* COLUMN DATA TYPE. According the the FITS standard, the value of
         TFORM is most generally in this format: 'rTa'. 'T' is actually a
         code of the datatype. 'r' is the 'repeat' counter and 'a' is
         depreciated. Currently we can only read repeat==1 cases. When no
         number exists before the defined capital letter, it defaults to 1,
         but if a number exists (for example '5D'), then the repeat is 5
         (there are actually five values in each column). Note that
         value[0] is a single quote. */
      if(strncmp(keyname, "TFORM", 5)==0)
        {
          /* See which column this information was for and add it, if the
             index is larger than the number of columns, then ignore
             the . The FITS standard says there should be no extra TFORM
             keywords beyond the number of columns, but we don't want to be
             that strict here. */
          index = strtoul(&keyname[5], &tailptr, 10) - 1;
          if(index<tfields)  /* Counting from zero was corrected above. */
            {
              /* The FITS standard's value to this option for FITS ASCII
                 and binary files differ. */
              if(*tableformat==GAL_TABLE_FORMAT_AFITS)
                {
                  /* 'repeat' is not defined for ASCII tables in FITS, so
                     it should be 1 (zero means that the column is empty);
                     while we actually have one number in each row. */
                  repeat=1;
                  fits_ascii_tform(value, &datatype, NULL, NULL, &status);
                }
              else
                {
                  /* Read the column's numeric data type. */
                  fits_binary_tform(value, &datatype, &repeat, NULL,
                                    &status);

                  /* Set the repeat flag if necessary (recall that by
                     default 'allcols[index].minmapsize' will be zero! So
                     we need this flag to confirm that the zero there is
                     meaningful. */
                  if(repeat==0)
                    allcols[index].flag
                      |= GAL_TABLEINTERN_FLAG_TFORM_REPEAT_IS_ZERO;
                }

              /* Write the "repeat" element into 'allcols->minmapsize',
                 also activate the repeat flag within the dataset. */
              allcols[index].minmapsize = repeat;

              /* Write the type into the data structure. */
              allcols[index].type=gal_fits_datatype_to_type(datatype, 1);

              /* If we are dealing with a string type, we need to know the
                 number of bytes in both cases for printing later. */
              if( allcols[index].type==GAL_TYPE_STRING )
                {
                  if(*tableformat==GAL_TABLE_FORMAT_AFITS)
                    {
                      repeat=strtol(value+1, &tailptr, 0);
                      if(*tailptr!='\0')
                        error(EXIT_FAILURE, 0, "%s (hdu: %s): the value "
                              "to keyword '%s' ('%s') is not in 'Aw' "
                              "format (for strings) as required by the "
                              "FITS standard in %s", filename, hdu,
                              keyname, value, __func__);
                    }

                  /* TFORM's 'repeat' element can be zero (signifying a
                     column without any data)! In this case, we want the
                     output to be fully blank, so we need space for the
                     blank string. */
                  allcols[index].disp_width=( repeat
                                              ? repeat
                                              : strlen(GAL_BLANK_STRING)+1);
                }
            }
        }


      /* COLUMN SCALE FACTOR. */
      else if(strncmp(keyname, "TSCAL", 5)==0)
        {
          index = strtoul(&keyname[5], &tailptr, 10) - 1;
          if(index<tfields)
            {
              tscal[index]=strtol(value, &tailptr, 0);
              if(*tailptr!='\0')
                error(EXIT_FAILURE, 0, "%s (hdu: %s): value to %s "
                      "keyword ('%s') couldn't be read as a number "
                      "in %s", filename, hdu, keyname, value, __func__);
            }
        }


      /* COLUMN ZERO VALUE (for signed/unsigned types). */
      else if(strncmp(keyname, "TZERO", 5)==0)
        {
          index = strtoul(&keyname[5], &tailptr, 10) - 1;
          if(index<tfields)
            {
              tzero[index]=strtoll(value, &tailptr, 0);
              if(*tailptr!='\0')
                error(EXIT_FAILURE, 0, "%s (hdu: %s): value to %s keyword "
                      "('%s') couldn't be read as a number in %s", filename,
                      hdu, keyname, value, __func__);
            }
        }


      /* COLUMN NAME. All strings in CFITSIO start and finish with single
         quotation marks, CFITSIO puts them in itsself, so if we don't
         remove them here, we might have duplicates later, its easier to
         just remove them to have a simple string that might be used else
         where too (without the single quotes). */
      else if(strncmp(keyname, "TTYPE", 5)==0)
        {
          index = strtoul(&keyname[5], &tailptr, 10) - 1;
          if(index<tfields)
            gal_checkset_allocate_copy(value, &allcols[index].name);
        }


      /* COLUMN UNITS. */
      else if(strncmp(keyname, "TUNIT", 5)==0)
        {
          index = strtoul(&keyname[5], &tailptr, 10) - 1;
          if(index<tfields)
            gal_checkset_allocate_copy(value, &allcols[index].unit);
        }


      /* COLUMN COMMENTS. */
      else if(strncmp(keyname, "TCOMM", 5)==0)
        {
          index = strtoul(&keyname[5], &tailptr, 10) - 1;
          if(index<tfields)
            gal_checkset_allocate_copy(value, &allcols[index].comment);
        }


      /* COLUMN BLANK VALUE. Note that to interpret the blank value the
         type of the column must already have been defined for this column
         in previous keywords. Otherwise, there will be a warning and it
         won't be used. */
      else if(strncmp(keyname, "TNULL", 5)==0)
        {
          index = strtoul(&keyname[5], &tailptr, 10) - 1;
          if(index<tfields )
            {
              if(allcols[index].type==0)
                {
                  gal_list_str_add(&later_name, keyname, 1);
                  gal_list_str_add(&later_value, value, 1);
                  gal_list_sizet_add(&later_index, index);
                }
              else
                {
                  /* Put in the blank value. */
                  gal_tableintern_read_blank(&allcols[index], value);

                  /* This flag is not relevant for FITS tables. */
                  if(allcols[index].flag
                     & GAL_TABLEINTERN_FLAG_ARRAY_IS_BLANK_STRING)
                    {
                      allcols[index].flag
                        &= ~GAL_TABLEINTERN_FLAG_ARRAY_IS_BLANK_STRING;
                      free(allcols[index].array);
                    }
                }
            }
        }


      /* COLUMN DISPLAY FORMAT. */
      else if(strncmp(keyname, "TDISP", 5)==0)
        {
          index = strtoul(&keyname[5], &tailptr, 10) - 1;
          if(index<tfields)
            set_display_format(value, &allcols[index], filename, hdu,
                               keyname);
        }
    }


  /* The loop above finishes as soon as the 'status' of 'fits_read_keyn'
     becomes non-zero. If the status is 'KEY_OUT_BOUNDS', then it shows we
     have reached the end of the keyword list and there is no
     problem. Otherwise, there is an unexpected problem and we should abort
     and inform the user about the problem. */
  if( rkstatus != KEY_OUT_BOUNDS )
    gal_fits_io_error(rkstatus, NULL);


  /* If any columns should be added later because of missing information,
     add them here. */
  if(later_name)
    {
      /* Interpret the necessary 'later_' keys. */
      tmp_i=later_index;
      tmp_v=later_value;
      for(tmp_n=later_name; tmp_n!=NULL; tmp_n=tmp_n->next)
        {
          /* Go over the known types and do the job. */
          if(strncmp(tmp_n->v, "TNULL", 5)==0)
            {
              /* Put in the blank value. */
              gal_tableintern_read_blank(&allcols[tmp_i->v], tmp_v->v);

              /* This flag is not relevant for FITS tables. */
              if(allcols[tmp_i->v].flag
                 & GAL_TABLEINTERN_FLAG_ARRAY_IS_BLANK_STRING)
                {
                  allcols[tmp_i->v].flag
                    &= ~GAL_TABLEINTERN_FLAG_ARRAY_IS_BLANK_STRING;
                  free(allcols[tmp_i->v].array);
                }
            }
          else
            error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s "
                  "to fix the problem. Post-processing of keyword '%s' "
                  "failed", __func__, PACKAGE_BUGREPORT, tmp_n->v);

          /* Increment the other two lists too. */
          tmp_v=tmp_v->next;
          tmp_i=tmp_i->next;
        }

      /* Clean up. */
      gal_list_sizet_free(later_index);
      gal_list_str_free(later_name, 1);
      gal_list_str_free(later_value, 1);
    }


  /* Set the 'next' pointer of each column. */
  for(i=0;i<tfields;++i)
    allcols[i].next = (i==tfields-1) ? NULL : &allcols[i+1];


  /* Correct integer types, then free the allocated arrays. */
  fits_correct_bin_table_int_types(allcols, tfields, tscal, tzero);
  free(tscal);
  free(tzero);


  /* Close the FITS file and report an error if we had any. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
  return allcols;
}





/* Read CFITSIO un-readable (INF, -INF or NAN) floating point values in
   FITS ASCII tables. */
static void
fits_tab_read_ascii_float_special(char *filename, char *hdu,
                                  fitsfile *fptr, gal_data_t *out,
                                  size_t colnum, size_t numrows,
                                  size_t minmapsize, int quietmmap)
{
  double tmp;
  char **strarr;
  char *tailptr;
  gal_data_t *strrows;
  size_t i, colwidth=50;
  int anynul=0, status=0;

  /* Allocate the dataset to keep the string values. */
  strrows=gal_data_alloc(NULL, GAL_TYPE_STRING, 1, &numrows, NULL, 0,
                         minmapsize, quietmmap, NULL, NULL, NULL);

  /* Allocate the space to keep the string values. */
  strarr=strrows->array;
  for(i=0;i<numrows;++i)
    {
      errno=0;
      strarr[i]=calloc(colwidth, sizeof *strarr[i]);
      if(strarr[i]==NULL)
        error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for "
              "strarr[%zu]", __func__, colwidth * sizeof *strarr[i], i);
    }

  /* Read the column as a string. */
  fits_read_col(fptr, TSTRING, colnum, 1, 1, out->size, NULL,
                strrows->array, &anynul, &status);
  gal_fits_io_error(status, NULL);

  /* Convert the strings to float. */
  for(i=0;i<numrows;++i)
    {
      /* Parse the string, if its not readable as a special number (like
         'inf' or 'nan', then just read it as a NaN. */
      tmp=strtod(strarr[i], &tailptr);
      if(tailptr==strarr[i]) tmp=NAN;

      /* Write it into the output dataset. */
      if(out->type==GAL_TYPE_FLOAT32)
        ((float *)(out->array))[i]=tmp;
      else
        ((double *)(out->array))[i]=tmp;
    }

  /* Clean up. */
  gal_data_free(strrows);
}





/* Read one column of the table in parallel. */
struct fits_tab_read_onecol_params
{
  char              *filename;  /* Name of FITS file with table.        */
  char                   *hdu;  /* HDU of input table.                  */
  size_t              numrows;  /* Number of rows in table to read.     */
  size_t              numcols;  /* Number of columns.                   */
  size_t           minmapsize;  /* Minimum space to memory-map.         */
  int               quietmmap;  /* Don't print memory-mapping info.     */
  gal_data_t         *allcols;  /* Information of all columns.          */
  gal_data_t       **colarray;  /* Array of pointers to all columns.    */
  gal_list_sizet_t   *indexll;  /* Index of columns to read.            */
  char       *hdu_option_name;  /* HDU option name (for error message). */
};





static void *
fits_tab_read_onecol(void *in_prm)
{
  /* Low-level definitions to be done first. */
  struct gal_threads_params *tprm=(struct gal_threads_params *)in_prm;
  struct fits_tab_read_onecol_params *p
    = (struct fits_tab_read_onecol_params *)tprm->params;

  /* Subsequent definitions. */
  uint8_t type;
  char **strarr;
  fitsfile *fptr;
  gal_data_t *col;
  size_t dsize[2];
  gal_list_sizet_t *tmp;
  void *blank, *blankuse;
  int isfloat, hdutype, anynul=0, status=0;
  size_t i, j, c, ndim, strw, repeat, indout, indin=GAL_BLANK_SIZE_T;

  /* Open the FITS file. */
  fptr=gal_fits_hdu_open_format(p->filename, p->hdu, 1,
                                p->hdu_option_name);

  /* See if its a Binary or ASCII table (necessary for floating point
     blank values). */
  if (fits_get_hdu_type(fptr, &hdutype, &status) )
    gal_fits_io_error(status, NULL);

  /* Go over all the columns that were assigned to this thread. */
  for(i=0; tprm->indexs[i] != GAL_BLANK_SIZE_T; ++i)
    {
      /* Find the necessary indexs (index of output column 'indout', and
         index of input 'indin'). */
      c=0;
      indout = tprm->indexs[i];
      for(tmp=p->indexll;tmp!=NULL;tmp=tmp->next)
        { if(c==indout) { indin=tmp->v; break; } ++c; }

      /* Allocate the necessary space for this column. */
      type=p->allcols[indin].type;
      repeat=p->allcols[indin].minmapsize;
      if(type!=GAL_TYPE_STRING && repeat>1)
        { ndim=2; dsize[0]=p->numrows; dsize[1]=repeat; }
      else
        { ndim=1; dsize[0]=p->numrows; }
      col=gal_data_alloc(NULL, type, ndim, dsize, NULL, 0,
                         p->minmapsize, p->quietmmap,
                         p->allcols[indin].name,
                         p->allcols[indin].unit,
                         p->allcols[indin].comment);

      /* For a string column, we need an allocated array for each element,
         even in binary values. This value should be stored in the
         disp_width element of the data structure, which is done
         automatically in 'gal_fits_table_info'. */
      if(col->type==GAL_TYPE_STRING)
        {
          /* Since the column may contain blank values, and the blank
             string is pre-defined in Gnuastro, we need to be sure that for
             each row, a blank string can fit. */
          strw = ( strlen(GAL_BLANK_STRING) > p->allcols[indin].disp_width
                   ? strlen(GAL_BLANK_STRING)
                   : p->allcols[indin].disp_width );

          /* Allocate the space for each row's strings. */
          strarr=col->array;
          for(j=0;j<p->numrows;++j)
            {
              errno=0;
              strarr[j]=calloc(strw+1, sizeof *strarr[0]); /* +1 for '\0' */
              if(strarr[j]==NULL)
                error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for "
                      "strarr[%zu]", __func__,
                      (p->allcols[indin].disp_width+1) * sizeof *strarr[j],
                      j);
            }
        }

      /* If this column has a 'repeat' of zero, then just set all its
         elements to its relevant blank type and don't call CFITSIO (there
         is nothing for it to read, and it will crash with "FITSIO status =
         308: bad first element number First element to write is too large:
         1; max allowed value is 0"). */
      if(p->allcols[indin].flag & GAL_TABLEINTERN_FLAG_TFORM_REPEAT_IS_ZERO)
        gal_blank_initialize(col);

      /* The column has non-zero width, read its contents. */
      else
        {
          /* Allocate a blank value for the given type and read/store the
             column using CFITSIO.

             * For binary tables, we only need blank values for integer
               types. For binary floating point types, the FITS standard
               defines blanks as NaN (same as almost any other software
               like Gnuastro). However if a blank value is specified,
               CFITSIO will convert other special numbers like 'inf' to NaN
               also. We want to be able to distinguish 'inf' and NaN here,
               so for floating point types in binary tables, we won't
               define any blank value. In ASCII tables, CFITSIO doesn't
               read the 'NAN' values (that it has written itself) unless we
               specify a blank pointer/value.

             * 'fits_read_col' takes the pointer to the thing that should
               be placed in a blank column (for strings, the 'char *')
               pointer. However, for strings, 'gal_blank_alloc_write' will
               return a 'char **' pointer! So for strings, we need to
               dereference the blank. This is why we need 'blankuse'. */
          isfloat = ( col->type==GAL_TYPE_FLOAT32
                      || col->type==GAL_TYPE_FLOAT64 );
          blank = ( ( hdutype==BINARY_TBL && isfloat )
                    ? NULL
                    : gal_blank_alloc_write(col->type) );
          blankuse = ( col->type==GAL_TYPE_STRING
                       ? *((char **)blank)
                       : blank);
          fits_read_col(fptr, gal_fits_type_to_datatype(col->type),
                        indin+1, 1, 1, col->size, blankuse, col->array,
                        &anynul, &status);

          /* In the ASCII table format some things need to be checked. */
          if( hdutype==ASCII_TBL )
            {
              /* CFITSIO might not be able to read 'INF' or '-INF'. In this
                case, it will set status to 'BAD_C2D' or 'BAD_C2F'. So,
                we'll use our own parser for the column values. */
              if(isfloat && (status==BAD_C2D || status==BAD_C2F) )
                {
                  fits_tab_read_ascii_float_special(p->filename, p->hdu,
                                                    fptr, col, indin+1,
                                                    p->numrows,
                                                    p->minmapsize,
                                                    p->quietmmap);
                  status=0;
                }
            }
          gal_fits_io_error(status, NULL); /* After 'status' correction. */

          /* Clean up and sanity check (just note that the blank value for
             strings, is an array of strings, so we need to free the
             contents before freeing itself). */
          if(col->type==GAL_TYPE_STRING)
            {strarr=blank; free(strarr[0]);}
          if(blank) free(blank);
        }

      /* Everything is fine, put this column in the output array. */
      p->colarray[indout]=col;
    }

  /* Close the FITS file. */
  status=0;
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);

  /* Wait for all the other threads to finish, then return. */
  if(tprm->b) pthread_barrier_wait(tprm->b);
  return NULL;
}





/* Read the column indexs into a dataset. */
gal_data_t *
gal_fits_tab_read(char *filename, char *hdu, size_t numrows,
                  gal_data_t *allcols, gal_list_sizet_t *indexll,
                  size_t numthreads, size_t minmapsize, int quietmmap,
                  char *hdu_option_name)
{
  size_t i;
  gal_data_t *out=NULL;
  gal_list_sizet_t *ind;
  struct fits_tab_read_onecol_params p;

  /* If the 'fits_is_reentrant' function exists, then use it to see if
     CFITSIO was configured in multi-thread mode. Otherwise, just use a
     single thread. */
#if GAL_CONFIG_HAVE_FITS_IS_REENTRANT == 1
  size_t nthreads = fits_is_reentrant() ? numthreads : 1;
#else
  size_t nthreads=1;
#endif

  /* We actually do have columns to read. */
  if(numrows)
    {
      /* Allocate array of output columns (to keep each read column in its
         proper place as they are read in parallel). */
      errno=0;
      p.numcols = gal_list_sizet_number(indexll);
      p.colarray = calloc( p.numcols, sizeof *(p.colarray) );
      if(p.colarray==NULL)
        error(EXIT_FAILURE, 0, "%s: couldn't allocate %zu bytes for "
              "'p.colarray'", __func__, p.numcols*(sizeof *(p.colarray)));

      /* Prepare for parallelization and spin-off the threads. */
      p.hdu = hdu;
      p.allcols = allcols;
      p.numrows = numrows;
      p.indexll = indexll;
      p.filename = filename;
      p.quietmmap = quietmmap;
      p.minmapsize = minmapsize;
      p.hdu_option_name = hdu_option_name;
      gal_threads_spin_off(fits_tab_read_onecol, &p, p.numcols, nthreads,
                           minmapsize, quietmmap);

      /* Put the columns into a single list and free the array of
         pointers. */
      out=p.colarray[0];
      for(i=0;i<p.numcols-1;++i)
        p.colarray[i]->next = p.colarray[i+1];
      free(p.colarray);
    }

  /* There are no rows to read ('numrows==NULL'). Make an empty-sized
     array. */
  else
    {
      /* We are setting a 1-element array to avoid any allocation
         errors. Then we are freeing the allocated spaces and correcting
         the sizes. */
      numrows=1;
      for(ind=indexll; ind!=NULL; ind=ind->next)
        {
          /* Do the allocation. */
          gal_list_data_add_alloc(&out, NULL, allcols[ind->v].type, 1,
                                  &numrows, NULL, 0, minmapsize, quietmmap,
                                  allcols[ind->v].name,
                                  allcols[ind->v].unit,
                                  allcols[ind->v].comment);

          /* Correct the array and sizes. */
          out->size=0;
          out->array=NULL;
          out->dsize[0]=0;
          free(out->array);
        }
    }

  /* Return the output. */
  return out;
}





/* This function will allocate new copies for all elements to have the same
   length as the maximum length and set all trailing elements to '\0' for
   those that are shorter than the length. The return value is the
   allocated space. If the dataset is not a string, the returned value will
   be -1 (largest number of 'size_t'). */
static size_t
fits_string_fixed_alloc_size(gal_data_t *data)
{
  size_t i, j, maxlen=0;
  char *tmp, **strarr=data->array;

  /* Return 0 if the dataset is not a string. */
  if(data->type!=GAL_TYPE_STRING)
    return -1;

  /* Get the maximum length. */
  for(i=0;i<data->size;++i)
    maxlen = strlen(strarr[i])>maxlen ? strlen(strarr[i]) : maxlen;

  /* For all elements, check the length and if they aren't equal to maxlen,
     then allocate a maxlen sized array and put the values in. */
  for(i=0;i<data->size;++i)
    {
      /* Allocate (and clear) the space for the new string. We want it to
         be cleared, so when the strings are smaller, the rest of the space
         is filled with '\0' (ASCII for 0) values. */
      errno=0;
      tmp=calloc(maxlen+1, sizeof *strarr[i]);
      if(tmp==NULL)
        error(EXIT_FAILURE, 0, "%s: %zu bytes for tmp", __func__,
              (maxlen+1)*sizeof *strarr[i]);

      /* Put the old array into the newly allocated space. 'tmp' was
         cleared (all values set to '\0', so we don't need to set the final
         one explicity after the copy. */
      for(j=0;strarr[i][j]!='\0';++j)
        tmp[j]=strarr[i][j];

      /* Free the old array and put in the new one. */
      free(strarr[i]);
      strarr[i]=tmp;
    }

  /* Return the allocated space. */
  return maxlen+1;
}





static void
fits_table_prepare_arrays(gal_data_t *cols, size_t numcols,
                          int tableformat, char ***outtform,
                          char ***outttype, char ***outtunit)
{
  size_t i=0, j;
  gal_data_t *col;
  char fmt[2], lng[3];
  char *blank, **tform, **ttype, **tunit;


  /* Allocate the arrays to keep the 'tform', 'ttype' and 'tunit'
     keywords. */
  errno=0;
  tform=*outtform=malloc(numcols*sizeof *tform);
  if(tform==NULL)
    error(EXIT_FAILURE, 0, "%s: %zu bytes for tform", __func__,
          numcols*sizeof *tform);
  errno=0;
  ttype=*outttype=malloc(numcols*sizeof *ttype);
  if(ttype==NULL)
    error(EXIT_FAILURE, 0, "%s: %zu bytes for ttype", __func__,
          numcols*sizeof *ttype);
  errno=0;
  tunit=*outtunit=malloc(numcols*sizeof *tunit);
  if(tunit==NULL)
    error(EXIT_FAILURE, 0, "%s: %zu bytes for tunit", __func__,
          numcols*sizeof *tunit);


  /* Go over each column and fill in these arrays. */
  for(col=cols; col!=NULL; col=col->next)
    {
      /* Set the 'ttype' and 'tunit' values: */
      if( asprintf(&ttype[i], "%s", col->name ? col->name : "")<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
      if( asprintf(&tunit[i], "%s", col->unit ? col->unit : "")<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);

      /* FITS's TFORM depends on the type of FITS table, so work
         differently. */
      switch(tableformat)
        {
        /* FITS ASCII table. */
        case GAL_TABLE_FORMAT_AFITS:

            /* Fill the printing format. */
            gal_tableintern_col_print_info(col, GAL_TABLE_FORMAT_AFITS,
                                           fmt, lng);

            /* We need to check if the blank value needs is larger than the
               expected width or not. Its initial width is set the output
               of the function above, but if the value is larger,
               'asprintf' (which is used) will make it wider. */
            blank = ( gal_blank_present(col, 0)
                      ? gal_blank_as_string(col->type, col->disp_width)
                      : NULL );

            /* Adjust the width. */
            if(blank)
              {
                col->disp_width = ( strlen(blank) > col->disp_width
                                    ? strlen(blank) : col->disp_width );
                free(blank);
              }

            /* Print the value to be used as TFORMn: */
            switch(col->type)
              {
              case GAL_TYPE_STRING:
              case GAL_TYPE_UINT8:
              case GAL_TYPE_INT8:
              case GAL_TYPE_UINT16:
              case GAL_TYPE_INT16:
              case GAL_TYPE_UINT32:
              case GAL_TYPE_INT32:
              case GAL_TYPE_UINT64:
              case GAL_TYPE_INT64:
                if(asprintf(&tform[i], "%c%d", fmt[0], col->disp_width)<0)
                  error(EXIT_FAILURE, 0, "%s: asprintf allocation",
                        __func__);
                break;

              case GAL_TYPE_FLOAT32:
              case GAL_TYPE_FLOAT64:
                if( asprintf(&tform[i], "%c%d.%d", fmt[0], col->disp_width,
                             col->disp_precision)<0 )
                  error(EXIT_FAILURE, 0, "%s: asprintf allocation",
                        __func__);
                break;

              default:
                error(EXIT_FAILURE, 0, "%s: col->type code %d not "
                      "recognized", __func__, col->type);
              }
          break;

        /* FITS binary table. */
        case GAL_TABLE_FORMAT_BFITS:

          /* If this is a string column, set all the strings to same size,
             then write the value of tform depending on the type. */
          col->disp_width=fits_string_fixed_alloc_size(col);
          fmt[0]=gal_fits_type_to_bin_tform(col->type);
          if( col->type==GAL_TYPE_STRING )
            {
              if( asprintf(&tform[i], "%d%c", col->disp_width, fmt[0])<0 )
                error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
            }
          else
            {
              /* For vector columns, we need to give the number of elements
                 in the vector. Note that a vector column with dsize[1]==1
                 is just a single-valued column and shouldn't be treated
                 like a vector. */
              if( col->ndim==1 || (col->ndim==2 && col->dsize[1]==1) )
                {
                  if( asprintf(&tform[i], "%c", fmt[0])<0 )
                    error(EXIT_FAILURE, 0, "%s: asprintf allocation",
                          __func__);
                }
              else if(col->ndim==2) /* Vector column */
                {
                  if(asprintf(&tform[i], "%zu%c", col->dsize[1], fmt[0])<0)
                    error(EXIT_FAILURE, 0, "%s: asprintf allocation",
                          __func__);
                }
              else
                error(EXIT_FAILURE, 0, "%s: only 1D or 2D data can "
                      "be written as a binary table", __func__);
            }
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: tableformat code %d not recognized",
                __func__, tableformat);
        }

      /* Increment the column index and write the values into the vector
         columns also. */
      if(tableformat==GAL_TABLE_FORMAT_AFITS && col->ndim>1)
        {
          for(j=1;j<col->dsize[1];++j)
            {
              gal_checkset_allocate_copy(tform[i], &tform[i+j]);
              gal_checkset_allocate_copy(tunit[i], &tunit[i+j]);
              gal_checkset_allocate_copy(ttype[i], &ttype[i+j]);
            }
          i+=col->dsize[1];
        }
      else ++i;
    }
}





/* Set the blank value to use for TNULL keyword (necessary in reading and
   writing).

   The blank value must be the raw value within the FITS file (before
   applying 'TZERO' OR 'TSCAL'). Therefore, because the following integer
   types aren't native to the FITS standard, we need to correct TNULL for
   them after applying TZERO. For example for uin16_t, TZERO is 32768, so
   TNULL has to be 32767 (the maximum value of the signed integer with the
   same width). In this way, adding TZERO to the TNULL, will make it the
   actual NULL value we assume in Gnuastro for uint16_t (the largest
   possible number). */
static void *
fits_blank_for_tnull(uint8_t type)
{
  /* Allocate the default blank value. */
  void *blank=gal_blank_alloc_write(type);

  /* For the non-native FITS type, correct the value. */
  switch(type)
    {
    case GAL_TYPE_INT8:   gal_type_min(GAL_TYPE_UINT8, blank); break;
    case GAL_TYPE_UINT16: gal_type_max(GAL_TYPE_INT16, blank); break;
    case GAL_TYPE_UINT32: gal_type_max(GAL_TYPE_INT32, blank); break;
    case GAL_TYPE_UINT64: gal_type_max(GAL_TYPE_INT64, blank); break;
    }

  /* Return the final allocated pointer. */
  return blank;
}





/* Write the TNULLn keywords into the FITS file. Note that this depends on
   the type of the table: for an ASCII table, all the columns need it. For
   a binary table, only the non-floating point ones (even if they don't
   have NULL values) must have it. */
static void
fits_write_tnull_tcomm(fitsfile *fptr, gal_data_t *col, int tableformat,
                       size_t colnum, char *tform)
{
  void *blank;
  int status=0;
  char *c, *keyname, *bcomment;

  /* Write the NULL value. */
  switch(tableformat)
    {
    case GAL_TABLE_FORMAT_AFITS:

      /* Print the keyword and value. */
      if( asprintf(&keyname, "TNULL%zu", colnum)<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
      blank=gal_blank_as_string(col->type, col->disp_width);

      /* When in exponential form ('tform' starting with 'E'), CFITSIO
         writes a NaN value as 'NAN', but when in floating point form
         ('tform' starting with 'F'), it writes it as 'nan'. So in the
         former case, we need to convert the string to upper case. */
      if(tform[0]=='E' || tform[0]=='e')
        for(c=blank; *c!='\0'; ++c) *c=toupper(*c);

      /* Write in the header. */
      fits_write_key(fptr, TSTRING, keyname, blank,
                     "blank value for this column", &status);

      /* Clean up. */
      free(keyname);
      free(blank);
      break;

    case GAL_TABLE_FORMAT_BFITS:

      /* FITS binary tables don't accept NULL values for floating point or
         string columns. For floating point it must be NaN, and for strings
         it is a blank string. */
      if( col->type!=GAL_TYPE_FLOAT32
          && col->type!=GAL_TYPE_FLOAT64
          && col->type!=GAL_TYPE_STRING )
        {
          /* Allocate the blank value to write into the TNULL keyword. */
          blank=fits_blank_for_tnull(col->type);

          /* Prepare the name and write the keyword. */
          if( asprintf(&keyname, "TNULL%zu", colnum)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
          fits_write_key(fptr, gal_fits_type_to_datatype(col->type),
                         keyname, blank, "blank value for this column",
                         &status);
          gal_fits_io_error(status, NULL);
          free(keyname);
          free(blank);
        }
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: tableformat code %d not recognized",
            __func__, tableformat);
    }

  /* Write the comments if there is any. */
  if(col->comment)
    {
      if( asprintf(&keyname, "TCOMM%zu", colnum)<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
      if( asprintf(&bcomment, "comment for field %zu", colnum)<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
      fits_write_key(fptr, TSTRING, keyname, col->comment,
                     bcomment, &status);
      gal_fits_io_error(status, NULL);
      free(keyname);
      free(bcomment);
    }
}





static size_t
fits_tab_write_colvec_ascii(fitsfile *fptr, gal_data_t *vector,
                            size_t colind, char *tform, char *filename)
{
  int status=0;
  char *keyname;
  static int warnasciivec=0;
  gal_data_t *ext, *extracted;
  gal_list_sizet_t *indexs=NULL;
  size_t i, coli=GAL_BLANK_SIZE_T;

  /* Print a warning about the modification that is necessary. */
  if(warnasciivec==0)
    {
      error(EXIT_SUCCESS, 0, "WARNING: vector (multi-valued) columns "
            "are not supported in the FITS ASCII format. Therefore, "
            "vector column(s) will be broken into separate "
            "single-valued columns in '%s'", filename);
      warnasciivec=1;
    }

  /* Build the indexs list and reverse it (adding to a last-in-first-out
     list will result in an inverse list). */
  for(i=0;i<vector->dsize[1];++i) gal_list_sizet_add(&indexs, i);
  gal_list_sizet_reverse(&indexs);

  /* Call the table function to extract the desired components of the
     vector column and clean up the indexs. */
  extracted=gal_table_col_vector_extract(vector, indexs);
  gal_list_sizet_free(indexs);

  /* Write the extracted columns into the output. */
  i=0;
  for(ext=extracted; ext!=NULL; ext=ext->next)
    {
      /* Write the column. */
      coli = colind + i++;
      fits_tab_write_col(fptr, ext, GAL_TABLE_FORMAT_AFITS, &coli,
                         tform, filename);

      /* Set the keyword name. */
      if( asprintf(&keyname, "TTYPE%zu", coli)<0 )
        error(EXIT_FAILURE, 0, "%s: asprintf for 'keyname'", __func__);

      /* Update the keyword with the name of the column (that was just
         copied in 'fits_table_prepare_arrays' from the vector-column's
         name, resulting in similar names). */
      fits_update_key(fptr, TSTRING, keyname, ext->name, NULL, &status);
      gal_fits_io_error(status, NULL);
    }

  /* Return the last column-index (which has been incremented in
     'fits_tab_write_col'). */
  return coli;
}





/* Write a single column into the FITS table. */
static void
fits_tab_write_col(fitsfile *fptr, gal_data_t *col, int tableformat,
                   size_t *colind, char *tform, char *filename)
{
  int status=0;
  char **strarr;
  void *blank=NULL;

  /* If this is a FITS ASCII table, and the column is vector, we need to
     write it as separate single-value columns and write those, then we can
     safely return (no more need to continue). It may happen that a 2D
     column has a second dimension of 1! In this case, it should be saved
     as a normal 1D column. */
  if(tableformat==GAL_TABLE_FORMAT_AFITS && col->ndim==2 && col->dsize[1]>1)
    {
      *colind=fits_tab_write_colvec_ascii(fptr, col, *colind, tform,
                                          filename);
      return;
    }

  /* Write the blank value into the header and return a pointer to
     it. Otherwise, */
  fits_write_tnull_tcomm(fptr, col, tableformat, *colind+1, tform);

  /* Set the blank pointer if its necessary. Note that strings don't need a
     blank pointer in a FITS ASCII table. */
  blank = ( gal_blank_present(col, 0)
            ? fits_blank_for_tnull(col->type) : NULL );
  if(tableformat==GAL_TABLE_FORMAT_AFITS && col->type==GAL_TYPE_STRING)
    { if(blank) free(blank); blank=NULL; }

  /* Manually remove the 'blank' pointer for standard FITS table numeric
     types (types below). We are doing this because as of CFITSIO 3.48,
     CFITSIO crashes for these types when we define our own blank values
     within this pointer, and such values actually exist in the
     column. This is the error message: "Null value for integer table
     column is not defined (FTPCLU)". Generally, for these native FITS
     table types 'blank' is redundant because our blank values are actually
     within their numerical data range. */
  switch(col->type)
    {
    case GAL_TYPE_UINT8:
    case GAL_TYPE_INT16:
    case GAL_TYPE_INT32:
    case GAL_TYPE_INT64:
      free(blank); blank=NULL;
      break;
    }

  /* Write the full column into the table. */
  fits_write_colnull(fptr, gal_fits_type_to_datatype(col->type),
                     *colind+1, 1, 1, col->size, col->array, blank,
                     &status);
  gal_fits_io_error(status, NULL);

  /* Clean up and Increment the column counter. */
  if(blank)
    {
      if(col->type==GAL_TYPE_STRING) {strarr=blank; free(strarr[0]);}
      free(blank);
    }

  /* Increment the 'colind' for the next column. */
  *colind+=1;
}





/* Write the given columns (a linked list of 'gal_data_t') into a FITS
   table.*/
void
gal_fits_tab_write(gal_data_t *cols, gal_list_str_t *comments,
                   int tableformat, char *filename, char *extname,
                   struct gal_fits_list_key_t **keylist)
{
  fitsfile *fptr;
  gal_data_t *col;
  gal_list_str_t *strt;
  char **ttype, **tform, **tunit;
  size_t i, numrows=-1, thisnrows;
  int tbltype, numcols=0, status=0;

  /* Make sure all the input columns have the same number of elements. */
  for(col=cols; col!=NULL; col=col->next)
    {
      thisnrows = col->dsize ? col->dsize[0] : 0;
      if(numrows==-1) numrows=thisnrows;
      else if(thisnrows!=numrows)
        error(EXIT_FAILURE, 0, "%s: the number of records/rows in the "
              "input columns are not equal! The first column "
              "has %zu rows, while column %d has %zu rows",
              __func__, numrows, numcols+1, thisnrows);

      /* ASCII FITS tables don't accept vector columns, so some extra
         columns need to be added. */
      numcols += ( tableformat==GAL_TABLE_FORMAT_AFITS
                   ? (col->ndim==1 ? 1 : col->dsize[1])
                   : 1 );
    }

  /* Open the FITS file for writing. */
  fptr=gal_fits_open_to_write(filename);

  /* Prepare necessary arrays and if integer type columns have blank
     values, write the TNULLn keywords into the FITS file. */
  fits_table_prepare_arrays(cols, numcols, tableformat, &tform, &ttype,
                            &tunit);

  /* Make the FITS file pointer. Note that tableformat was checked in
     'fits_table_prepare_arrays'. */
  tbltype = tableformat==GAL_TABLE_FORMAT_AFITS ? ASCII_TBL : BINARY_TBL;
  fits_create_tbl(fptr, tbltype, numrows, numcols, ttype, tform, tunit,
                  extname, &status);
  gal_fits_io_error(status, NULL);

  /* Write the columns into the file and also write the blank values into
     the header when necessary. */
  i=0;
  for(col=cols; col!=NULL; col=col->next)/*'i' is increment in the func.*/
    fits_tab_write_col(fptr, col, tableformat, &i, tform[i], filename);

  /* Write the requested keywords. */
  if(keylist)
    gal_fits_key_write_in_ptr(keylist, fptr);

  /* Write the comments if there were any. */
  for(strt=comments; strt!=NULL; strt=strt->next)
    fits_write_comment(fptr, strt->v, &status);

  /* Write all the headers and the version information. */
  gal_fits_key_write_version_in_ptr(NULL, NULL, fptr);

  /* Clean up and close the FITS file. Note that each element in the
     'ttype' and 'tunit' arrays just points to the respective string in the
     column data structure, the space for each element of the array wasn't
     allocated. */
  for(i=0;i<numcols;++i) {free(tform[i]); free(ttype[i]); free(tunit[i]);}
  free(tform); free(ttype); free(tunit);
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
}
