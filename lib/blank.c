/*********************************************************************
blank -- Deal with blank values in a datasets
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2017-2023 Free Software Foundation, Inc.

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

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <gnuastro/data.h>
#include <gnuastro/tile.h>
#include <gnuastro/blank.h>
#include <gnuastro/pointer.h>
#include <gnuastro/statistics.h>

#include <gnuastro-internal/checkset.h>




/* Write the blank value of the type into an already allocate space. Note
   that for STRINGS, pointer should actually be 'char **'. */
void
gal_blank_write(void *ptr, uint8_t type)
{
  switch(type)
    {
    /* Numeric types */
    case GAL_TYPE_UINT8:   *(uint8_t  *)ptr = GAL_BLANK_UINT8;    break;
    case GAL_TYPE_INT8:    *(int8_t   *)ptr = GAL_BLANK_INT8;     break;
    case GAL_TYPE_UINT16:  *(uint16_t *)ptr = GAL_BLANK_UINT16;   break;
    case GAL_TYPE_INT16:   *(int16_t  *)ptr = GAL_BLANK_INT16;    break;
    case GAL_TYPE_UINT32:  *(uint32_t *)ptr = GAL_BLANK_UINT32;   break;
    case GAL_TYPE_INT32:   *(int32_t  *)ptr = GAL_BLANK_INT32;    break;
    case GAL_TYPE_UINT64:  *(uint64_t *)ptr = GAL_BLANK_UINT64;   break;
    case GAL_TYPE_INT64:   *(int64_t  *)ptr = GAL_BLANK_INT64;    break;
    case GAL_TYPE_FLOAT32: *(float    *)ptr = GAL_BLANK_FLOAT32;  break;
    case GAL_TYPE_FLOAT64: *(double   *)ptr = GAL_BLANK_FLOAT64;  break;

    /* String type. */
    case GAL_TYPE_STRING:
      gal_checkset_allocate_copy(GAL_BLANK_STRING, ptr);
      break;

    /* Complex types */
    case GAL_TYPE_COMPLEX32:
    case GAL_TYPE_COMPLEX64:
      error(EXIT_FAILURE, 0, "%s: complex types are not yet supported",
            __func__);

    /* Unrecognized type. */
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized", __func__,
            type);
    }
}





/* Allocate some space for the given type and put the blank value into
   it. */
void *
gal_blank_alloc_write(uint8_t type)
{
  void *out;

  /* Allocate the space to keep the blank value. */
  out=gal_pointer_allocate(type, 1, 0, __func__, "out");

  /* Put the blank value in the allcated space. */
  gal_blank_write(out, type);

  /* Return the allocated space. */
  return out;
}





/* Initialize (set all the values in the array) with the blank value of the
   given type. */
void
gal_blank_initialize(gal_data_t *input)
{
  size_t i;
  char **strarr;

  /* For strings, we will initialize the full array, for numerical data
     types we will consider tiles. */
  if(input->type==GAL_TYPE_STRING)
    {
      strarr=input->array;
      for(i=0;i<input->size;++i)
        {
          if(strarr[i]) free(strarr[i]);
          gal_checkset_allocate_copy(GAL_BLANK_STRING, &strarr[i]);
        }
    }
  else
    {GAL_TILE_PARSE_OPERATE(input, NULL, 0, 0, {*i=b;});}
}





/* Initialize an array to the given type's blank values.*/
void
gal_blank_initialize_array(void *array, size_t size, uint8_t type)
{
  size_t i, w=gal_type_sizeof(type);
  void *b=gal_blank_alloc_write(type);

  /* Set all the elements to blank. */
  for(i=0;i<size;++i)
    memcpy(gal_pointer_increment(array, i, type), b, w);

  /* Clean up. */
  free(b);
}





/* Print the blank value as a string. For the integer types, we'll use the
   PRIxNN keywords of 'inttypes.h' (which is imported into Gnuastro from
   Gnulib, so we don't necessarily rely on the host system having it). */
char *
gal_blank_as_string(uint8_t type, int width)
{
  char *blank=NULL, *fmt;

  /* Print the given value. */
  switch(type)
    {
    case GAL_TYPE_BIT:
      error(EXIT_FAILURE, 0, "%s: bit types are not implemented yet",
            __func__);
      break;

    case GAL_TYPE_STRING:
      if(width)
        {
          if( asprintf(&blank, "%*s", width,  GAL_BLANK_STRING)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, "%s", GAL_BLANK_STRING)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_UINT8:
      fmt = width ? "%*"PRIu8 : "%"PRIu8;
      if(width)
        {
          if( asprintf(&blank, fmt, width, (uint8_t)GAL_BLANK_UINT8)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, fmt, (uint8_t)GAL_BLANK_UINT8)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_INT8:
      fmt = width ? "%*"PRId8 : "%"PRId8;
      if(width)
        {
          if( asprintf(&blank, fmt, width, (int8_t)GAL_BLANK_INT8)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, fmt, (int8_t)GAL_BLANK_INT8)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_UINT16:
      fmt = width ? "%*"PRIu16 : "%"PRIu16;
      if(width)
        {
          if( asprintf(&blank, fmt, width, (uint16_t)GAL_BLANK_UINT16)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, fmt, (uint16_t)GAL_BLANK_UINT16)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_INT16:
      fmt = width ? "%*"PRId16 : "%"PRId16;
      if(width)
        {
          if( asprintf(&blank, fmt, width, (int16_t)GAL_BLANK_INT16)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, fmt, (int16_t)GAL_BLANK_INT16)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_UINT32:
      fmt = width ? "%*"PRIu32 : "%"PRIu32;
      if(width)
        {
          if( asprintf(&blank, fmt, width, (uint32_t)GAL_BLANK_UINT32)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, fmt, (uint32_t)GAL_BLANK_UINT32)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_INT32:
      fmt = width ? "%*"PRId32 : "%"PRId32;
      if(width)
        {
          if( asprintf(&blank, fmt, width, (int32_t)GAL_BLANK_INT32)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, fmt, (int32_t)GAL_BLANK_INT32)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_UINT64:
      fmt = width ? "%*"PRIu64 : "%"PRIu64;
      if(width)
        {
          if( asprintf(&blank, fmt, width, (uint64_t)GAL_BLANK_UINT64)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, fmt, (uint64_t)GAL_BLANK_UINT64)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_INT64:
      fmt = width ? "%*"PRId64 : "%"PRId64;
      if(width)
        {
          if( asprintf(&blank, fmt, width, (int64_t)GAL_BLANK_INT64)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, fmt, (int64_t)GAL_BLANK_INT64)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_FLOAT32:
      if(width)
        {
          if( asprintf(&blank, "%*f", width,  (float)GAL_BLANK_FLOAT32)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, "%f", (float)GAL_BLANK_FLOAT32)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    case GAL_TYPE_FLOAT64:
      if(width)
        {
          if( asprintf(&blank, "%*f", width,  (double)GAL_BLANK_FLOAT64)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      else
        {
          if( asprintf(&blank, "%f", (double)GAL_BLANK_FLOAT64)<0 )
            error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
        }
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized", __func__,
            type);
    }
  return blank;
}






/* Return 1 if the contents of the pointer (with the given type) is
   blank. */
int
gal_blank_is(void *pointer, uint8_t type)
{
  /* A small sanity check. */
  if(pointer==NULL)
    error(EXIT_FAILURE, 0, "%s: input pointer is NULL", __func__);

  /* Do the checks based on the type. */
  switch(type)
    {
    /* Numeric types */
    case GAL_TYPE_UINT8:     return *(uint8_t  *)pointer==GAL_BLANK_UINT8;
    case GAL_TYPE_INT8:      return *(int8_t   *)pointer==GAL_BLANK_INT8;
    case GAL_TYPE_UINT16:    return *(uint16_t *)pointer==GAL_BLANK_UINT16;
    case GAL_TYPE_INT16:     return *(int16_t  *)pointer==GAL_BLANK_INT16;
    case GAL_TYPE_UINT32:    return *(uint32_t *)pointer==GAL_BLANK_UINT32;
    case GAL_TYPE_INT32:     return *(int32_t  *)pointer==GAL_BLANK_INT32;
    case GAL_TYPE_UINT64:    return *(uint64_t *)pointer==GAL_BLANK_UINT64;
    case GAL_TYPE_INT64:     return *(int64_t  *)pointer==GAL_BLANK_INT64;
    case GAL_TYPE_FLOAT32:   return isnan( *(float *)(pointer) );
    case GAL_TYPE_FLOAT64:   return isnan( *(double *)(pointer) );

    /* String. */
    case GAL_TYPE_STRING:
      if(!strcmp(pointer,GAL_BLANK_STRING)) return 1; else return 0;

    /* Complex types */
    case GAL_TYPE_COMPLEX32:
    case GAL_TYPE_COMPLEX64:
      error(EXIT_FAILURE, 0, "%s: complex types are not yet supported",
            __func__);

    /* Bit. */
    case GAL_TYPE_BIT:
      error(EXIT_FAILURE, 0, "%s: bit type datasets are not yet supported",
            __func__);

    default:
      error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
            __func__, type);
    }

  /* Control should not reach here, so print an error if it does, then
     return a 0 (just to avoid compiler warnings). */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to address the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return 0;
}





/* Return 1 if the dataset has a blank value and zero if it doesn't. Before
   checking the dataset, this function will look at its flags. If the
   'GAL_DATA_FLAG_HASBLANK' or 'GAL_DATA_FLAG_DONT_CHECK_ZERO' bits of
   'input->flag' are set to 1, this function will not do any check and will
   just use the information in the flags.

   If you want to re-check a dataset which has non-zero flags, then
   explicitly set the appropriate flag to zero before calling this
   function. When there are no other flags, you can just set 'input->flags'
   to zero, otherwise you can use this expression:

       input->flags &= ~ (GAL_DATA_FLAG_HASBLANK | GAL_DATA_FLAG_USE_ZERO);

   This function has no side-effects on the dataset: it will not toggle the
   flags, to avoid repeating parsing of the full dataset multiple times
   (when it occurs), please toggle the flags your self after the first
   check. */
#define HAS_BLANK(IT) {                                                 \
    IT b, *a=input->array, *af=a+input->size, *start;                   \
    gal_blank_write(&b, block->type);                                   \
                                                                        \
    /* If this is a tile, not a full block. */                          \
    if(input!=block)                                                    \
      start=gal_tile_start_end_ind_inclusive(input, block, start_end_inc); \
                                                                        \
    /* Go over all the elements. */                                     \
    while( start_end_inc[0] + increment <= start_end_inc[1] )           \
      {                                                                 \
        /* Necessary when we are on a tile. */                          \
        if(input!=block)                                                \
          af = ( a = start + increment ) + input->dsize[input->ndim-1]; \
                                                                        \
        /* Check for blank values. */                                   \
        if(b==b) do if(*a==b)  { hasblank=1; break; } while(++a<af);    \
        else     do if(*a!=*a) { hasblank=1; break; } while(++a<af);    \
                                                                        \
        /* Necessary when we are on a tile. */                          \
        if(input!=block)                                                \
          increment += gal_tile_block_increment(block, input->dsize,    \
                                                num_increment++, NULL); \
        else break;                                                     \
      }                                                                 \
  }
int
gal_blank_present(gal_data_t *input, int updateflag)
{
  int hasblank=0;
  char **str, **strf;
  size_t increment=0, num_increment=1;
  gal_data_t *block=gal_tile_block(input);
  size_t start_end_inc[2]={0,block->size-1}; /* -1: this is INCLUSIVE. */

  /* If there is nothing in the array (its size is zero), then return 0 (no
     blank is present. */
  if(input->size==0) return 0;

  /* From the user's flags, you can tell if the dataset has already been
     checked for blank values or not. If it has, then just return the
     checked result. */
  if( input->flag & GAL_DATA_FLAG_BLANK_CH )
    return input->flag & GAL_DATA_FLAG_HASBLANK;

  /* Go over the pixels and check: */
  switch(block->type)
    {
    /* Numeric types */
    case GAL_TYPE_UINT8:     HAS_BLANK( uint8_t  );    break;
    case GAL_TYPE_INT8:      HAS_BLANK( int8_t   );    break;
    case GAL_TYPE_UINT16:    HAS_BLANK( uint16_t );    break;
    case GAL_TYPE_INT16:     HAS_BLANK( int16_t  );    break;
    case GAL_TYPE_UINT32:    HAS_BLANK( uint32_t );    break;
    case GAL_TYPE_INT32:     HAS_BLANK( int32_t  );    break;
    case GAL_TYPE_UINT64:    HAS_BLANK( uint64_t );    break;
    case GAL_TYPE_INT64:     HAS_BLANK( int64_t  );    break;
    case GAL_TYPE_FLOAT32:   HAS_BLANK( float    );    break;
    case GAL_TYPE_FLOAT64:   HAS_BLANK( double   );    break;

    /* String. */
    case GAL_TYPE_STRING:
      if(input!=block)
        error(EXIT_FAILURE, 0, "%s: tile mode is currently not "
              "supported for strings", __func__);
      strf = (str=input->array) + input->size;
      do
        if(*str==NULL || !strcmp(*str,GAL_BLANK_STRING)) return 1;
      while(++str<strf);
      break;

    /* Complex types */
    case GAL_TYPE_COMPLEX32:
    case GAL_TYPE_COMPLEX64:
      error(EXIT_FAILURE, 0, "%s: complex types are not yet supported",
            __func__);

    /* Bit */
    case GAL_TYPE_BIT:
      error(EXIT_FAILURE, 0, "%s: bit type datasets are not yet supported",
            __func__);

    default:
      error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
            __func__, block->type);
    }

  /* Update the flag if requested. */
  if(updateflag)
    {
      input->flag |= GAL_DATA_FLAG_BLANK_CH;
      if(hasblank) input->flag |= GAL_DATA_FLAG_HASBLANK;
      else         input->flag &= ~GAL_DATA_FLAG_HASBLANK;
    }

  /* If there was a blank value, then the function would have returned with
     a value of 1. So if it reaches here, then we can be sure that there
     was no blank values, hence, return 0. */
  return hasblank;
}





/* Return the number of blank elements in the dataset. */
size_t
gal_blank_number(gal_data_t *input, int updateflag)
{
  size_t nblank;
  char **strarr;
  gal_data_t *number;
  size_t i, num_not_blank;

  if(input)
    {
      if( gal_blank_present(input, updateflag) )
        {
          if(input->type==GAL_TYPE_STRING)
            {
              nblank=0;
              strarr=input->array;
              for(i=0;i<input->size;++i)
                {
                  if( strarr[i]==NULL
                      || strcmp(strarr[i], GAL_BLANK_STRING)==0 )
                    ++nblank;
                }
              return nblank;
            }
          else
            {
              number=gal_statistics_number(input);
              num_not_blank=((size_t *)(number->array))[0];
              gal_data_free(number);
              return input->size - num_not_blank;
            }
        }
      else
        return 0;
    }
  else
    return GAL_BLANK_SIZE_T;
}







/* Create a dataset of the the same size as the input, but with an uint8_t
   type that has a value of 1 for data that are blank and 0 for those that
   aren't. */
#define FLAG_BLANK(IT) {                                                \
    IT b, *a=input->array;                                              \
    gal_blank_write(&b, input->type);                                   \
    if(b==b) /* Blank value can be checked with the equal. */           \
      do {*o = blank1_not0 ? *a==b  : *a!=b;  ++a;} while(++o<of);      \
    else     /* Blank value will fail with the equal comparison. */     \
      do {*o = blank1_not0 ? *a!=*a : *a==*a; ++a;} while(++o<of);      \
  }
static gal_data_t *
blank_flag(gal_data_t *input, int blank1_not0)
{
  uint8_t *o, *of;
  gal_data_t *out;
  char **str=input->array, **strf=str+input->size;

  /* The datasets may be empty. In this case, the output should also be
     empty, but with the standard 'uint8' type of a flag (we can have
     tables and images with 0 rows or pixels!). */
  if(input->size==0 || input->array==NULL)
    {
      out=gal_data_alloc_empty(input->ndim, input->minmapsize,
                               input->quietmmap);
      out->type=GAL_TYPE_UINT8;
      return out;
    }

  /* Do all the checks and allocations if a blank is actually present! */
  if( gal_blank_present(input, 0) )
    {
      /* Allocate a non-cleared output array, we are going to parse the
         input and fill in each element. */
      out=gal_data_alloc(NULL, GAL_TYPE_UINT8, input->ndim, input->dsize,
                         input->wcs, 0, input->minmapsize,
                         input->quietmmap, NULL, "bool", NULL);

      /* Set the pointers for easy looping. */
      of=(o=out->array)+input->size;

      /* Go over the pixels and set the output values. */
      switch(input->type)
        {
        /* Numeric types */
        case GAL_TYPE_UINT8:     FLAG_BLANK( uint8_t  );    break;
        case GAL_TYPE_INT8:      FLAG_BLANK( int8_t   );    break;
        case GAL_TYPE_UINT16:    FLAG_BLANK( uint16_t );    break;
        case GAL_TYPE_INT16:     FLAG_BLANK( int16_t  );    break;
        case GAL_TYPE_UINT32:    FLAG_BLANK( uint32_t );    break;
        case GAL_TYPE_INT32:     FLAG_BLANK( int32_t  );    break;
        case GAL_TYPE_UINT64:    FLAG_BLANK( uint64_t );    break;
        case GAL_TYPE_INT64:     FLAG_BLANK( int64_t  );    break;
        case GAL_TYPE_FLOAT32:   FLAG_BLANK( float    );    break;
        case GAL_TYPE_FLOAT64:   FLAG_BLANK( double   );    break;

        /* String. */
        case GAL_TYPE_STRING:
          do *o++ = !strcmp(*str,GAL_BLANK_STRING); while(++str<strf);
          break;

        /* Currently unsupported types. */
        case GAL_TYPE_BIT:
        case GAL_TYPE_COMPLEX32:
        case GAL_TYPE_COMPLEX64:
          error(EXIT_FAILURE, 0, "%s: %s type not yet supported",
                __func__, gal_type_name(input->type, 1));

        /* Bad input. */
        default:
          error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
                __func__, input->type);
        }
    }

  /* Input had no blanks */
  else
    {
      /* Allocate the output data structure (if the caller wants to flag
         blanks, then we will "clear" the output also). */
      out=gal_data_alloc(NULL, GAL_TYPE_UINT8, input->ndim, input->dsize,
                         input->wcs, blank1_not0 ? 1 : 0,
                         input->minmapsize, input->quietmmap,
                         NULL, "bool", NULL);

      /* If the caller wants to flag the non-blank elements, we should now
         set all the output values to 1 (the input had no blanks!). */
      if(blank1_not0==0)
        { of=(o=out->array)+out->size; do *o++=1; while(o<of); }
    }

  /* Return */
  return out;
}





gal_data_t *
gal_blank_flag(gal_data_t *input)
{
  return blank_flag(input, 1);
}





gal_data_t *
gal_blank_flag_not(gal_data_t *input)
{
  return blank_flag(input, 0);
}





/* Write a blank value in the input anywhere that the flag dataset is not
   zero or not blank. */
#define BLANK_FLAG_APPLY(IT) {                                          \
    IT *i=input->array, b;                                              \
    gal_blank_write(&b, input->type);                                   \
    do {if(*f && *f!=GAL_BLANK_UINT8) *i=b; ++i;} while(++f<ff);        \
  }
void
gal_blank_flag_apply(gal_data_t *input, gal_data_t *flag)
{
  size_t j;
  char **strarr=input->array;
  uint8_t *f=flag->array, *ff=f+flag->size;

  /* Sanity check. */
  if(flag->type!=GAL_TYPE_UINT8)
    error(EXIT_FAILURE, 0, "%s: the 'flag' argument has a '%s' type, it "
          "must have an unsigned 8-bit type", __func__,
          gal_type_name(flag->type, 1));
  if(gal_dimension_is_different(input, flag))
    error(EXIT_FAILURE, 0, "%s: the 'flag' argument doesn't have the same "
          "size as the 'input' argument", __func__);

  /* Write the blank values. */
  switch(input->type)
    {
    /* Basic numeric types. */
    case GAL_TYPE_UINT8:     BLANK_FLAG_APPLY( uint8_t  );    break;
    case GAL_TYPE_INT8:      BLANK_FLAG_APPLY( int8_t   );    break;
    case GAL_TYPE_UINT16:    BLANK_FLAG_APPLY( uint16_t );    break;
    case GAL_TYPE_INT16:     BLANK_FLAG_APPLY( int16_t  );    break;
    case GAL_TYPE_UINT32:    BLANK_FLAG_APPLY( uint32_t );    break;
    case GAL_TYPE_INT32:     BLANK_FLAG_APPLY( int32_t  );    break;
    case GAL_TYPE_UINT64:    BLANK_FLAG_APPLY( uint64_t );    break;
    case GAL_TYPE_INT64:     BLANK_FLAG_APPLY( int64_t  );    break;
    case GAL_TYPE_FLOAT32:   BLANK_FLAG_APPLY( float    );    break;
    case GAL_TYPE_FLOAT64:   BLANK_FLAG_APPLY( double   );    break;

    /* Strings. */
    case GAL_TYPE_STRING:
      for(j=0; j<input->size; ++j)
        {
          if(*f && *f!=GAL_BLANK_UINT8)
            {
              free(strarr[j]);
              gal_checkset_allocate_copy(GAL_BLANK_STRING, &strarr[j]);
            }
          ++f;
        }
      break;

    /* Currently unsupported types. */
    case GAL_TYPE_BIT:
    case GAL_TYPE_COMPLEX32:
    case GAL_TYPE_COMPLEX64:
      error(EXIT_FAILURE, 0, "%s: %s type not yet supported",
            __func__, gal_type_name(input->type, 1));

    /* Bad input. */
    default:
      error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
            __func__, input->type);
    }

  /* Update the blank flags. */
  gal_blank_present(input, 1);
}





/* Remove flagged elements from a dataset (which may not necessarily
   blank), convert it to a 1D dataset and adjust the size properly. In
   practice this function doesn't 'realloc' the input array, all it does is
   to shift the blank eleemnts to the end and adjust the size elements of
   the 'gal_data_t'. */
#define BLANK_FLAG_REMOVE(IT) {                                         \
    IT *a=input->array, *af=a+input->size, *o=input->array;             \
    do {                                                                \
      if(*f==0) {++num; *o++=*a; }                                      \
      ++f;                                                              \
    }                                                                   \
    while(++a<af);                                                      \
  }
void
gal_blank_flag_remove(gal_data_t *input, gal_data_t *flag)
{
  char **strarr;
  size_t i, num=0;
  uint8_t *f=flag->array;

  /* Sanity check. */
  if(flag->type!=GAL_TYPE_UINT8)
    error(EXIT_FAILURE, 0, "%s: the 'flag' argument has a '%s' type, it "
          "must have an unsigned 8-bit type", __func__,
          gal_type_name(flag->type, 1));
  if(gal_dimension_is_different(input, flag))
    error(EXIT_FAILURE, 0, "%s: the 'flag' argument doesn't have the same "
          "size as the 'input' argument", __func__);

  /* If there is no elements in the input or the flag (we have already
     confirmed that they are the same size), then just return (nothing is
     necessary to do). */
  if(flag->size==0 || flag->array==NULL) return;

  /* Shift all non-blank elements to the start of the array. */
  switch(input->type)
    {
    case GAL_TYPE_UINT8:    BLANK_FLAG_REMOVE( uint8_t  );    break;
    case GAL_TYPE_INT8:     BLANK_FLAG_REMOVE( int8_t   );    break;
    case GAL_TYPE_UINT16:   BLANK_FLAG_REMOVE( uint16_t );    break;
    case GAL_TYPE_INT16:    BLANK_FLAG_REMOVE( int16_t  );    break;
    case GAL_TYPE_UINT32:   BLANK_FLAG_REMOVE( uint32_t );    break;
    case GAL_TYPE_INT32:    BLANK_FLAG_REMOVE( int32_t  );    break;
    case GAL_TYPE_UINT64:   BLANK_FLAG_REMOVE( uint64_t );    break;
    case GAL_TYPE_INT64:    BLANK_FLAG_REMOVE( int64_t  );    break;
    case GAL_TYPE_FLOAT32:  BLANK_FLAG_REMOVE( float    );    break;
    case GAL_TYPE_FLOAT64:  BLANK_FLAG_REMOVE( double   );    break;
    case GAL_TYPE_STRING:
      strarr=input->array;
      for(i=0;i<input->size;++i)
        {
          if( *f && *f!=GAL_BLANK_UINT8 )    /* Flagged to be removed */
            { free(strarr[i]); strarr[i]=NULL; }
          else strarr[num++]=strarr[i];      /* Keep. */
          ++f;
        }
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, input->type);
    }

  /* Adjust the size elements of the dataset. */
  input->ndim=1;
  input->dsize[0]=input->size=num;
}





/* Remove flagged elements from a dataset (which may not necessarily
   blank), convert it to a 1D dataset and adjust the size properly. In
   practice this function doesn't 'realloc' the input array, all it does is
   to shift the blank eleemnts to the end and adjust the size elements of
   the 'gal_data_t'. */
#define BLANK_FLAG_REMOVE_D0(IT) {                                      \
    IT *a=input->array, *af=a+input->size, *o=input->array;             \
    do {                                                                \
      printf("\n%s: HERE\n", __func__);                                   \
      if(*f) a+=nelem;                                                  \
      else {num++; for(i=0;i<nelem;++i) {*o++=*a; ++a; printf("%zu ", i);} } \
      ++f;                                                              \
    }                                                                   \
    while(a<af);                                                        \
  }
static void
blank_flag_remove_dim0(gal_data_t *input, gal_data_t *flag)
{
  size_t i, num=0;
  uint8_t it=input->type;
  uint8_t *f=flag->array;
  size_t nelem = input->ndim==1 ? 1 : input->size/input->dsize[0];

  /* Sanity check. */
  if(flag->type!=GAL_TYPE_UINT8)
    error(EXIT_FAILURE, 0, "%s: the 'flag' argument has a '%s' type, it "
          "must have an unsigned 8-bit type", __func__,
          gal_type_name(flag->type, 1));
  if(input->dsize[0]!=flag->dsize[0])
    error(EXIT_FAILURE, 0, "%s: the 'flag' argument doesn't have the same "
          "size as the 'input' argument", __func__);

  /* This is a special function! */
  if(flag->ndim!=1)
    error(EXIT_FAILURE, 0, "%s: this function's 'flag' should only "
          "have a single dimension", __func__);
  if(input->ndim==flag->ndim)
    error(EXIT_FAILURE, 0, "%s: this function is only for scenarios "
          "where 'flag' is single-dimensional and the input is "
          "multi-dimensional. Please use 'gal_blank_flag_remove' when "
          "the input and flag have the same number of dimensions",
          __func__);

  /* If there is no elements in the input or the flag (we have already
     confirmed that they are the same size), then just return (nothing is
     necessary to do). */
  if(flag->size==0 || flag->array==NULL) return;

  /* Move all the non-flagged elements to the start of the array. */
  for(i=0;i<flag->size;++i)
    if(f[i]==0)
      memmove(gal_pointer_increment(input->array, (num++)*nelem, it),
              gal_pointer_increment(input->array, i*nelem,       it),
              nelem*gal_type_sizeof(it));

  /* Shift all non-blank elements to the start of the array.
  switch(input->type)
    {
    case GAL_TYPE_UINT8:    BLANK_FLAG_REMOVE_D0( uint8_t  );    break;
    case GAL_TYPE_INT8:     BLANK_FLAG_REMOVE_D0( int8_t   );    break;
    case GAL_TYPE_UINT16:   BLANK_FLAG_REMOVE_D0( uint16_t );    break;
    case GAL_TYPE_INT16:    BLANK_FLAG_REMOVE_D0( int16_t  );    break;
    case GAL_TYPE_UINT32:   BLANK_FLAG_REMOVE_D0( uint32_t );    break;
    case GAL_TYPE_INT32:    BLANK_FLAG_REMOVE_D0( int32_t  );    break;
    case GAL_TYPE_UINT64:   BLANK_FLAG_REMOVE_D0( uint64_t );    break;
    case GAL_TYPE_INT64:    BLANK_FLAG_REMOVE_D0( int64_t  );    break;
    case GAL_TYPE_FLOAT32:  BLANK_FLAG_REMOVE_D0( float    );    break;
    case GAL_TYPE_FLOAT64:  BLANK_FLAG_REMOVE_D0( double   );    break;
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, input->type);
    }
  */

  /* Adjust the size elements of the dataset. */
  input->size=1;
  input->dsize[0]=num;
  for(i=0;i<input->ndim;++i) input->size*=input->dsize[i];
}





/* Remove blank elements from a dataset, convert it to a 1D dataset and
   adjust the size properly. In practice this function doesn't 'realloc'
   the input array, all it does is to shift the blank eleemnts to the end
   and adjust the size elements of the 'gal_data_t'. */
#define BLANK_REMOVE(IT) {                                              \
    IT b, *a=input->array, *af=a+input->size, *o=input->array;          \
    gal_blank_write(&b, input->type);                                   \
    if(b==b)       /* Blank value can be be checked with equal. */      \
      do if(*a!=b)  { ++num; *o++=*a; } while(++a<af);                  \
    else           /* Blank value will fail on equal comparison. */     \
      do if(*a==*a) { ++num; *o++=*a; } while(++a<af);                  \
  }
void
gal_blank_remove(gal_data_t *input)
{
  char **strarr;
  size_t i, num=0;

  /* This function currently assumes a contiguous patch of memory. */
  if(input->block && input->ndim!=1 )
    error(EXIT_FAILURE, 0, "%s: tiles in datasets with more dimensions "
          "than 1 are not yet supported. Your input has %zu dimensions",
          __func__, input->ndim);

  /* If the dataset doesn't have blank values, then just get the size. */
  if( gal_blank_present(input, 0) )
    {
      /* Shift all non-blank elements to the start of the array. */
      switch(input->type)
        {
        case GAL_TYPE_UINT8:    BLANK_REMOVE( uint8_t  );    break;
        case GAL_TYPE_INT8:     BLANK_REMOVE( int8_t   );    break;
        case GAL_TYPE_UINT16:   BLANK_REMOVE( uint16_t );    break;
        case GAL_TYPE_INT16:    BLANK_REMOVE( int16_t  );    break;
        case GAL_TYPE_UINT32:   BLANK_REMOVE( uint32_t );    break;
        case GAL_TYPE_INT32:    BLANK_REMOVE( int32_t  );    break;
        case GAL_TYPE_UINT64:   BLANK_REMOVE( uint64_t );    break;
        case GAL_TYPE_INT64:    BLANK_REMOVE( int64_t  );    break;
        case GAL_TYPE_FLOAT32:  BLANK_REMOVE( float    );    break;
        case GAL_TYPE_FLOAT64:  BLANK_REMOVE( double   );    break;
        case GAL_TYPE_STRING:
          strarr=input->array;
          for(i=0;i<input->size;++i)
            if( strcmp(strarr[i], GAL_BLANK_STRING) ) /* Not blank. */
              { strarr[num++]=strarr[i]; }
            else { free(strarr[i]); strarr[i]=NULL; }  /* Is blank. */
          break;
        default:
          error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
                __func__, input->type);
        }
    }
  else num=input->size;

  /* Adjust the size elements of the dataset. */
  input->ndim=1;
  input->dsize[0]=input->size=num;

  /* Set the flags to mark that there is no blanks. */
  input->flag |=  GAL_DATA_FLAG_BLANK_CH;
  input->flag &= ~GAL_DATA_FLAG_HASBLANK;
}





/* Similar to 'gal_blank_remove', but also reallocates/frees the extra
   space. */
void
gal_blank_remove_realloc(gal_data_t *input)
{
  /* Remove the blanks and fix the size of the dataset. */
  gal_blank_remove(input);

  /* Run realloc to shrink the allocated space. */
  input->array=realloc(input->array,
                       input->size*gal_type_sizeof(input->type));
  if(input->array==NULL)
    error(EXIT_FAILURE, 0, "%s: couldn't reallocate memory", __func__);
}





static gal_data_t *
blank_remove_in_list_merge_flags(gal_data_t *thisdata, gal_data_t *flag,
                                 int onlydim0)
{
  size_t i;
  uint8_t *u, *tu;
  gal_data_t *flagtmp;
  static int warningprinted=0;

  /* Ignore the dataset if it has more than one dimension and 'onlydim0' is
     called*/
  if(onlydim0 && thisdata->ndim>1 && warningprinted==0)
    {
      warningprinted=1;
      error(EXIT_SUCCESS, 0, "%s: WARNING: multi-dimensional columns "
            "are not supported when 'onlydim0' is non-zero", __func__);
      return NULL;
    }

  /* Build the flag of blank elements for this column. */
  flagtmp=gal_blank_flag(thisdata);

  /* If this is the first column, then use flagtmp as flag. */
  if(flag)
    {
      u=flag->array;
      tu=flagtmp->array;
      for(i=0;i<flag->size;++i) u[i] = u[i] || tu[i];
      gal_data_free(flagtmp);
    }
  else
    flag=flagtmp;

  /* For a check.
  u=flag->array;
  double *d=thisdata->array;
  for(i=0;i<flag->size;++i) printf("%f, %u\n", d[i], u[i]);
  printf("\n");
  */

  /* Return the flag dataset. */
  return flag;
}





/* Remove any row that has a blank in any of the given columns. */
gal_data_t *
gal_blank_remove_rows(gal_data_t *columns, gal_list_sizet_t *column_indexs,
                      int onlydim0)
{
  size_t i;
  gal_list_sizet_t *tcol;
  gal_data_t *tmp, *flag=NULL;

  /* If any columns are requested, only use the given columns for the
     flags, otherwise use all the input columns. */
  if(column_indexs)
    for(tcol=column_indexs; tcol!=NULL; tcol=tcol->next)
      {
        /* Find the correct column in the full list. */
        i=0;
        for(tmp=columns; tmp!=NULL; tmp=tmp->next)
          if(i++==tcol->v) break;

        /* If the given index is larger than the number of elements in the
           input list, then print an error. */
        if(tmp==NULL)
          error(EXIT_FAILURE, 0, "%s: input list has %zu elements, but "
                "the column %zu (counting from zero) has been requested",
                __func__, gal_list_data_number(columns), tcol->v);

        /* Build the flag of blank elements for this column. */
        flag=blank_remove_in_list_merge_flags(tmp, flag, onlydim0);
      }
  else
    for(tmp=columns; tmp!=NULL; tmp=tmp->next)
      flag=blank_remove_in_list_merge_flags(tmp, flag, onlydim0);

  /* Now that the flags have been set, remove the rows. */
  for(tmp=columns; tmp!=NULL; tmp=tmp->next)
    {
      if(tmp->ndim==1 || onlydim0==0) gal_blank_flag_remove(tmp, flag);
      else blank_flag_remove_dim0(tmp, flag);
    }

  /* For a check.
  double *d1=columns->array, *d2=columns->next->array;
  for(i=0;i<columns->size;++i)
    printf("%f, %f\n", d2[i], d1[i]);
  printf("\n");
  */

  /* Return the flag. */
  return flag;
}
