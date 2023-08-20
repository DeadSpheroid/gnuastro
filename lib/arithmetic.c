/*********************************************************************
Arithmetic operations on data structures
This is part of GNU Astronomy Utilities (Gnuastro) package.

Corresponding author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
     Faezeh Bidjarchian <fbidjarchian@gmail.com>
     Pedram Ashofteh-Ardakani <pedramardakani@pm.me>
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

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <wcslib/wcs.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_const_num.h>
#include <gsl/gsl_const_mks.h>

#include <gnuastro/box.h>
#include <gnuastro/wcs.h>
#include <gnuastro/list.h>
#include <gnuastro/pool.h>
#include <gnuastro/blank.h>
#include <gnuastro/units.h>
#include <gnuastro/qsort.h>
#include <gnuastro/pointer.h>
#include <gnuastro/threads.h>
#include <gnuastro/dimension.h>
#include <gnuastro/statistics.h>
#include <gnuastro/arithmetic.h>

#include <gnuastro-internal/checkset.h>
#include <gnuastro-internal/arithmetic-internal.h>

/* Headers for each binary operator. Since they heavily involve macros,
   their compilation can be very large if they are in a single function and
   file. So there is a separate C source and header file for each of these
   functions. */
#include <gnuastro-internal/arithmetic-lt.h>
#include <gnuastro-internal/arithmetic-le.h>
#include <gnuastro-internal/arithmetic-gt.h>
#include <gnuastro-internal/arithmetic-ge.h>
#include <gnuastro-internal/arithmetic-eq.h>
#include <gnuastro-internal/arithmetic-ne.h>
#include <gnuastro-internal/arithmetic-or.h>
#include <gnuastro-internal/arithmetic-and.h>
#include <gnuastro-internal/arithmetic-plus.h>
#include <gnuastro-internal/arithmetic-minus.h>
#include <gnuastro-internal/arithmetic-bitor.h>
#include <gnuastro-internal/arithmetic-bitand.h>
#include <gnuastro-internal/arithmetic-bitxor.h>
#include <gnuastro-internal/arithmetic-bitlsh.h>
#include <gnuastro-internal/arithmetic-bitrsh.h>
#include <gnuastro-internal/arithmetic-modulo.h>
#include <gnuastro-internal/arithmetic-divide.h>
#include <gnuastro-internal/arithmetic-multiply.h>










/***********************************************************************/
/***************             Internal checks              **************/
/***********************************************************************/
/* Some functions are only for a floating point operand, so if the input
   isn't floating point, inform the user to change the type explicitly,
   doing it implicitly/internally puts too much responsability on the
   program.
static void
arithmetic_check_float_input(gal_data_t *in, int operator, char *numstr)
{
  switch(in->type)
    {
    case GAL_TYPE_FLOAT32:
    case GAL_TYPE_FLOAT64:
      break;
    default:
      error(EXIT_FAILURE, 0, "the %s operator can only accept single or "
            "double precision floating point numbers as its operand. The "
            "%s operand has type %s. You can use the 'float' or 'double' "
            "operators before this operator to explicitly convert to the "
            "desired precision floating point type. If the operand was "
            "originally a typed number (string of characters), add an 'f' "
            "after it so it is directly read into the proper precision "
            "floating point number (based on the number of non-zero "
            "decimals it has)", gal_arithmetic_operator_string(operator),
            numstr, gal_type_name(in->type, 1));
    }
}
*/



















/***********************************************************************/
/***************        Unary functions/operators         **************/
/***********************************************************************/
/* Change input data structure type. */
static gal_data_t *
arithmetic_change_type(gal_data_t *data, int operator, int flags)
{
  int type=-1;
  gal_data_t *out;

  /* Set the output type. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_TO_UINT8:    type=GAL_TYPE_UINT8;    break;
    case GAL_ARITHMETIC_OP_TO_INT8:     type=GAL_TYPE_INT8;     break;
    case GAL_ARITHMETIC_OP_TO_UINT16:   type=GAL_TYPE_UINT16;   break;
    case GAL_ARITHMETIC_OP_TO_INT16:    type=GAL_TYPE_INT16;    break;
    case GAL_ARITHMETIC_OP_TO_UINT32:   type=GAL_TYPE_UINT32;   break;
    case GAL_ARITHMETIC_OP_TO_INT32:    type=GAL_TYPE_INT32;    break;
    case GAL_ARITHMETIC_OP_TO_UINT64:   type=GAL_TYPE_UINT64;   break;
    case GAL_ARITHMETIC_OP_TO_INT64:    type=GAL_TYPE_INT64;    break;
    case GAL_ARITHMETIC_OP_TO_FLOAT32:  type=GAL_TYPE_FLOAT32;  break;
    case GAL_ARITHMETIC_OP_TO_FLOAT64:  type=GAL_TYPE_FLOAT64;  break;
    default:
      error(EXIT_FAILURE, 0, "%s: operator value of %d not recognized",
            __func__, operator);
    }

  /* Copy to the new type. */
  out=gal_data_copy_to_new_type(data, type);

  /* Delete the input structure if the user asked for it. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    gal_data_free(data);

  /* Return */
  return out;
}





/* Return an array of value 1 for any zero valued element and zero for any
   non-zero valued element. */
#define TYPE_CASE_FOR_NOT(CTYPE) {                                      \
    CTYPE *a=data->array, *af=a+data->size;                             \
    do *o++ = !(*a); while(++a<af);                                     \
  }

static gal_data_t *
arithmetic_not(gal_data_t *data, int flags)
{
  uint8_t *o;
  gal_data_t *out;

  /* The dataset may be empty. In this case the output should also be empty
     (we can have tables and images with 0 rows or pixels!). */
  if(data->size==0 || data->array==NULL) return data;


  /* Allocate the output array. */
  out=gal_data_alloc(NULL, GAL_TYPE_UINT8, data->ndim, data->dsize,
                     data->wcs, 0, data->minmapsize, data->quietmmap,
                     data->name, data->unit, data->comment);
  o=out->array;


  /* Go over the pixels and set the output values. */
  switch(data->type)
    {
    case GAL_TYPE_UINT8:   TYPE_CASE_FOR_NOT( uint8_t  );   break;
    case GAL_TYPE_INT8:    TYPE_CASE_FOR_NOT( int8_t   );   break;
    case GAL_TYPE_UINT16:  TYPE_CASE_FOR_NOT( uint16_t );   break;
    case GAL_TYPE_INT16:   TYPE_CASE_FOR_NOT( int16_t  );   break;
    case GAL_TYPE_UINT32:  TYPE_CASE_FOR_NOT( uint32_t );   break;
    case GAL_TYPE_INT32:   TYPE_CASE_FOR_NOT( int32_t  );   break;
    case GAL_TYPE_UINT64:  TYPE_CASE_FOR_NOT( uint64_t );   break;
    case GAL_TYPE_INT64:   TYPE_CASE_FOR_NOT( int64_t  );   break;
    case GAL_TYPE_FLOAT32: TYPE_CASE_FOR_NOT( float    );   break;
    case GAL_TYPE_FLOAT64: TYPE_CASE_FOR_NOT( double   );   break;

    case GAL_TYPE_BIT:
      error(EXIT_FAILURE, 0, "%s: bit datatypes are not yet supported, "
            "please get in touch with us to implement it.", __func__);

    default:
      error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
            __func__, data->type);
    }

  /* Delete the input structure if the user asked for it. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    gal_data_free(data);

  /* Return */
  return out;
}





/* Bitwise not operator. */
static gal_data_t *
arithmetic_bitwise_not(int flags, gal_data_t *in)
{
  gal_data_t *o;
  uint8_t    *iu8  = in->array,  *iu8f  = iu8  + in->size,   *ou8;
  int8_t     *ii8  = in->array,  *ii8f  = ii8  + in->size,   *oi8;
  uint16_t   *iu16 = in->array,  *iu16f = iu16 + in->size,   *ou16;
  int16_t    *ii16 = in->array,  *ii16f = ii16 + in->size,   *oi16;
  uint32_t   *iu32 = in->array,  *iu32f = iu32 + in->size,   *ou32;
  int32_t    *ii32 = in->array,  *ii32f = ii32 + in->size,   *oi32;
  uint64_t   *iu64 = in->array,  *iu64f = iu64 + in->size,   *ou64;
  int64_t    *ii64 = in->array,  *ii64f = ii64 + in->size,   *oi64;

  /* The dataset may be empty. In this case, the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if(in->size==0 || in->array==NULL) return in;

  /* Check the type. */
  switch(in->type)
    {
    case GAL_TYPE_FLOAT32:
    case GAL_TYPE_FLOAT64:
      error(EXIT_FAILURE, 0, "%s: bitwise not (one's complement) "
            "operator can only work on integer types", __func__);
    }

  /* If we want inplace output, set the output pointer to the input
     pointer, for every pixel, the operation will be independent. */
  if(flags & GAL_ARITHMETIC_FLAG_INPLACE)
    o = in;
  else
    o = gal_data_alloc(NULL, in->type, in->ndim, in->dsize, in->wcs,
                       0, in->minmapsize, in->quietmmap, NULL, NULL, NULL);

  /* Start setting the types. */
  switch(in->type)
    {
    case GAL_TYPE_UINT8:
      ou8=o->array;   do  *ou8++ = ~(*iu8++);    while(iu8<iu8f);     break;

    case GAL_TYPE_INT8:
      oi8=o->array;   do  *oi8++ = ~(*ii8++);    while(ii8<ii8f);     break;

    case GAL_TYPE_UINT16:
      ou16=o->array;  do *ou16++ = ~(*iu16++);   while(iu16<iu16f);   break;

    case GAL_TYPE_INT16:
      oi16=o->array;  do *oi16++ = ~(*ii16++);   while(ii16<ii16f);   break;

    case GAL_TYPE_UINT32:
      ou32=o->array;  do *ou32++ = ~(*iu32++);   while(iu32<iu32f);   break;

    case GAL_TYPE_INT32:
      oi32=o->array;  do *oi32++ = ~(*ii32++);   while(ii32<ii32f);   break;

    case GAL_TYPE_UINT64:
      ou64=o->array;  do *ou64++ = ~(*iu64++);   while(iu64<iu64f);   break;

    case GAL_TYPE_INT64:
      oi64=o->array;  do *oi64++ = ~(*ii64++);   while(ii64<ii64f);   break;

    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, in->type);
    }


  /* Clean up (if necessary). */
  if( (flags & GAL_ARITHMETIC_FLAG_FREE) && o!=in)
    gal_data_free(in);

  /* Return */
  return o;
}





/* We don't want to use the standard function for unary functions in the
   case of the absolute operator. This is because there are multiple
   versions of this function in the C library for different types, which
   can greatly improve speed. */
#define ARITHMETIC_ABS_SGN(CTYPE, FUNC) {                          \
    CTYPE *o=out->array, *a=in->array, *af=a+in->size;             \
    do *o++ = FUNC(*a); while(++a<af);                             \
  }
static gal_data_t *
arithmetic_abs(int flags, gal_data_t *in)
{
  gal_data_t *out;

  /* The dataset may be empty. In this case, the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if(in->size==0 || in->array==NULL) return in;

  /* Set the output array. */
  if(flags & GAL_ARITHMETIC_FLAG_INPLACE)
    out=in;
  else
    out = gal_data_alloc(NULL, in->type, in->ndim, in->dsize,
                         in->wcs, 0, in->minmapsize, in->quietmmap,
                         in->name, in->unit, in->comment);

  /* Put the absolute value depending on the type. */
  switch(in->type)
    {
    /* Unsigned types are already positive, so if the input is not to be
       freed (the output must be a separate array), just copy the whole
       array. */
    case GAL_TYPE_UINT8:
    case GAL_TYPE_UINT16:
    case GAL_TYPE_UINT32:
    case GAL_TYPE_UINT64:
      if(out!=in) gal_data_copy_to_allocated(in, out);
      break;

    /* For the signed types, we actually have to go over the data and
       calculate the absolute value. There are unique functions for
       different types, so we will be using them. */
    case GAL_TYPE_INT8:    ARITHMETIC_ABS_SGN( int8_t,  abs   );  break;
    case GAL_TYPE_INT16:   ARITHMETIC_ABS_SGN( int16_t, abs   );  break;
    case GAL_TYPE_INT32:   ARITHMETIC_ABS_SGN( int32_t, labs  );  break;
    case GAL_TYPE_INT64:   ARITHMETIC_ABS_SGN( int64_t, llabs );  break;
    case GAL_TYPE_FLOAT32: ARITHMETIC_ABS_SGN( float,   fabsf );  break;
    case GAL_TYPE_FLOAT64: ARITHMETIC_ABS_SGN( double,  fabs  );  break;
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, in->type);
    }

  /* Clean up and return. */
  if( (flags & GAL_ARITHMETIC_FLAG_FREE) && out!=in)
    gal_data_free(in);
  return out;
}





/* Wrapper functions for RA/Dec strings. */
static char *
arithmetic_units_degree_to_ra(double decimal)
{ return gal_units_degree_to_ra(decimal, 0); }

static char *
arithmetic_units_degree_to_dec(double decimal)
{ return gal_units_degree_to_dec(decimal, 0); }

#define UNIFUNC_RUN_FUNCTION_ON_ELEMENT(OT, IT, OP, BEFORE, AFTER){     \
    OT *oa=o->array;                                                    \
    IT *ia=in->array, *iaf=ia + in->size;                               \
    do *oa++ = OP( *ia++ BEFORE ) AFTER; while(ia<iaf);                 \
  }

#define UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(IT, OP, BEFORE, AFTER)    \
  switch(o->type)                                                       \
    {                                                                   \
    case GAL_TYPE_UINT8:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(uint8_t,  IT, OP, BEFORE, AFTER)  \
        break;                                                          \
    case GAL_TYPE_INT8:                                                 \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(int8_t,   IT, OP, BEFORE, AFTER)  \
        break;                                                          \
    case GAL_TYPE_UINT16:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(uint16_t, IT, OP, BEFORE, AFTER)  \
        break;                                                          \
    case GAL_TYPE_INT16:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(int16_t,  IT, OP, BEFORE, AFTER)  \
        break;                                                          \
    case GAL_TYPE_UINT32:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(uint32_t, IT, OP, BEFORE, AFTER)  \
        break;                                                          \
    case GAL_TYPE_INT32:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(int32_t,  IT, OP, BEFORE, AFTER)  \
        break;                                                          \
    case GAL_TYPE_UINT64:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(uint64_t, IT, OP, BEFORE, AFTER)  \
        break;                                                          \
    case GAL_TYPE_INT64:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(int64_t,  IT, OP, BEFORE, AFTER)  \
        break;                                                          \
    case GAL_TYPE_FLOAT32:                                              \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(float,    IT, OP, BEFORE, AFTER)  \
      break;                                                            \
    case GAL_TYPE_FLOAT64:                                              \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(double,   IT, OP, BEFORE, AFTER)  \
      break;                                                            \
    default:                                                            \
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",         \
            "UNIARY_FUNCTION_ON_ELEMENT", in->type);                    \
    }

#define UNIARY_FUNCTION_ON_ELEMENT_OUTPUT_STRING(OP)     \
  switch(in->type)                                                      \
    {                                                                   \
    case GAL_TYPE_UINT8:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, uint8_t,  OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_INT8:                                                 \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, int8_t,   OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_UINT16:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, uint16_t, OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_INT16:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, int16_t,  OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_UINT32:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, uint32_t, OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_INT32:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, int32_t,  OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_UINT64:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, uint64_t, OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_INT64:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, int64_t,  OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_FLOAT32:                                              \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, float,    OP, +0, +0)     \
        break;                                                          \
    case GAL_TYPE_FLOAT64:                                              \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT(char *, double,   OP, +0, +0)     \
        break;                                                          \
    default:                                                            \
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",         \
            "UNIARY_FUNCTION_ON_ELEMENT_OUTPUT_STRING", in->type);      \
    }

#define UNIARY_FUNCTION_ON_ELEMENT(OP, BEFORE, AFTER)                   \
  switch(in->type)                                                      \
    {                                                                   \
    case GAL_TYPE_UINT8:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(uint8_t,  OP, BEFORE, AFTER) \
        break;                                                          \
    case GAL_TYPE_INT8:                                                 \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(int8_t,   OP, BEFORE, AFTER) \
        break;                                                          \
    case GAL_TYPE_UINT16:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(uint16_t, OP, BEFORE, AFTER) \
        break;                                                          \
    case GAL_TYPE_INT16:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(int16_t,  OP, BEFORE, AFTER) \
        break;                                                          \
    case GAL_TYPE_UINT32:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(uint32_t, OP, BEFORE, AFTER) \
        break;                                                          \
    case GAL_TYPE_INT32:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(int32_t,  OP, BEFORE, AFTER) \
        break;                                                          \
    case GAL_TYPE_UINT64:                                               \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(uint64_t, OP, BEFORE, AFTER) \
        break;                                                          \
    case GAL_TYPE_INT64:                                                \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(int64_t,  OP, BEFORE, AFTER) \
        break;                                                          \
    case GAL_TYPE_FLOAT32:                                              \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(float,    OP, BEFORE, AFTER) \
      break;                                                            \
    case GAL_TYPE_FLOAT64:                                              \
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_INSET(double,   OP, BEFORE, AFTER) \
      break;                                                            \
    default:                                                            \
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",         \
            "UNIARY_FUNCTION_ON_ELEMENT", in->type);                    \
    }

#define UNIFUNC_RUN_FUNCTION_ON_ELEMENT_SEXAGESIMAL(OT, OP){            \
    OT *oa=o->array;                                                    \
    char **ia, **iaf;                                                   \
    /* If the input dataset isn't a string (it was parsed as a */       \
    /* number, stop the program and let the user know what to do. */    \
    /* This can happen with the Arithmetic program, for example if */   \
    /* this command is called: 'astarithmetic 16 ra-to-deg'. */         \
    if(in->type!=GAL_TYPE_STRING)                                       \
      error(EXIT_FAILURE, 0, "%s: the input should be a string in "     \
            "the format of %s (where the '_' are place-holders for "    \
            "numbers). This error may happen when you are calling a "   \
            "command like \"astarithmetic 16 %s\" or \"echo %s | "      \
            "asttable -c'arith $1 %s'\". In such cases, please use "    \
            "this command: "                                            \
            "\"echo %s | asttable\" (by default, when a string type "   \
            "isn't specified for a column, and it conforms to the "     \
            "pattern above, 'asttable' does the conversion "            \
            "internally; this is the cause of the second type of "      \
            "error above). The '%s' operator is only relevant for "     \
            "many sexagesimal values in a string-type table column",    \
            gal_arithmetic_operator_string(operator),                   \
            ( operator==GAL_ARITHMETIC_OP_RA_TO_DEGREE                  \
              ? "'_h_m_s' or '_h_m_'"                                   \
              : "'_d_m_s' or '_d_m_'" ),                                \
            gal_arithmetic_operator_string(operator),                   \
            ( operator==GAL_ARITHMETIC_OP_RA_TO_DEGREE                  \
              ? "16h0m0" : "16d0m0" ),                                  \
            gal_arithmetic_operator_string(operator),                   \
            ( operator==GAL_ARITHMETIC_OP_RA_TO_DEGREE                  \
              ? "16h0m0" : "16d0m0" ),                                  \
            gal_arithmetic_operator_string(operator));                  \
    iaf = (ia=in->array) + in->size;                                    \
    do *oa++ = OP(*ia++); while(ia<iaf);                                \
}

static gal_data_t *
arithmetic_function_unary(int operator, int flags, gal_data_t *in)
{
  uint8_t otype;
  int inplace=0;
  gal_data_t *o;

  /* The dataset may be empty. In this case, the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if(in->size==0 || in->array==NULL) return in;

  /* See if the operation should be done in place. The output of these
     operators is defined in the floating point space. So even if the input
     is an integer type and user requested in place operation, if it's not
     a floating point type, it will not be in place. */
  if( (flags & GAL_ARITHMETIC_FLAG_INPLACE)
      && ( in->type==GAL_TYPE_FLOAT32 || in->type==GAL_TYPE_FLOAT64 )
      && ( operator != GAL_ARITHMETIC_OP_RA_TO_DEGREE
      &&   operator != GAL_ARITHMETIC_OP_DEC_TO_DEGREE
      &&   operator != GAL_ARITHMETIC_OP_DEGREE_TO_RA
      &&   operator != GAL_ARITHMETIC_OP_DEGREE_TO_DEC ) )
    inplace=1;

  /* Set the output pointer. */
  if(inplace)
    {
      o = in;
      otype=in->type;
    }
  else
    {
      /* Check for operators which have fixed output types. */
      if(         operator == GAL_ARITHMETIC_OP_RA_TO_DEGREE
               || operator == GAL_ARITHMETIC_OP_DEC_TO_DEGREE )
        otype = GAL_TYPE_FLOAT64;
      else if(    operator == GAL_ARITHMETIC_OP_DEGREE_TO_RA
               || operator == GAL_ARITHMETIC_OP_DEGREE_TO_DEC )
        otype = GAL_TYPE_STRING;
      else
        otype = ( in->type==GAL_TYPE_FLOAT64
                  ? GAL_TYPE_FLOAT64
                  : GAL_TYPE_FLOAT32 );

      /* Set the final output type. */
      o = gal_data_alloc(NULL, otype, in->ndim, in->dsize, in->wcs,
                         0, in->minmapsize, in->quietmmap,
                         NULL, NULL, NULL);
    }

  /* Start setting the operator and operands. The mathematical constant
     'PI' is imported from the GSL as M_PI. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_SQRT:
      UNIARY_FUNCTION_ON_ELEMENT( sqrt,  +0, +0);         break;
    case GAL_ARITHMETIC_OP_LOG:
      UNIARY_FUNCTION_ON_ELEMENT( log,   +0, +0);         break;
    case GAL_ARITHMETIC_OP_LOG10:
      UNIARY_FUNCTION_ON_ELEMENT( log10, +0, +0);         break;
    case GAL_ARITHMETIC_OP_SIN:
      UNIARY_FUNCTION_ON_ELEMENT( sin,   *M_PI/180.0f, +0); break;
    case GAL_ARITHMETIC_OP_COS:
      UNIARY_FUNCTION_ON_ELEMENT( cos,   *M_PI/180.0f, +0); break;
    case GAL_ARITHMETIC_OP_TAN:
      UNIARY_FUNCTION_ON_ELEMENT( tan,   *M_PI/180.0f, +0); break;
    case GAL_ARITHMETIC_OP_ASIN:
      UNIARY_FUNCTION_ON_ELEMENT( asin,  +0, *180.0f/M_PI); break;
    case GAL_ARITHMETIC_OP_ACOS:
      UNIARY_FUNCTION_ON_ELEMENT( acos,  +0, *180.0f/M_PI); break;
    case GAL_ARITHMETIC_OP_ATAN:
      UNIARY_FUNCTION_ON_ELEMENT( atan,  +0, *180.0f/M_PI); break;
    case GAL_ARITHMETIC_OP_SINH:
      UNIARY_FUNCTION_ON_ELEMENT( sinh,  +0, +0);         break;
    case GAL_ARITHMETIC_OP_COSH:
      UNIARY_FUNCTION_ON_ELEMENT( cosh,  +0, +0);         break;
    case GAL_ARITHMETIC_OP_TANH:
      UNIARY_FUNCTION_ON_ELEMENT( tanh,  +0, +0);         break;
    case GAL_ARITHMETIC_OP_ASINH:
      UNIARY_FUNCTION_ON_ELEMENT( asinh, +0, +0);         break;
    case GAL_ARITHMETIC_OP_ACOSH:
      UNIARY_FUNCTION_ON_ELEMENT( acosh, +0, +0);         break;
    case GAL_ARITHMETIC_OP_ATANH:
      UNIARY_FUNCTION_ON_ELEMENT( atanh, +0, +0);         break;
    case GAL_ARITHMETIC_OP_MAG_TO_JY:
      UNIARY_FUNCTION_ON_ELEMENT( gal_units_mag_to_jy, +0, +0); break;
    case GAL_ARITHMETIC_OP_JY_TO_MAG:
      UNIARY_FUNCTION_ON_ELEMENT( gal_units_jy_to_mag, +0, +0); break;
    case GAL_ARITHMETIC_OP_AU_TO_PC:
      UNIARY_FUNCTION_ON_ELEMENT( gal_units_au_to_pc, +0, +0); break;
    case GAL_ARITHMETIC_OP_PC_TO_AU:
      UNIARY_FUNCTION_ON_ELEMENT( gal_units_pc_to_au, +0, +0); break;
    case GAL_ARITHMETIC_OP_LY_TO_PC:
      UNIARY_FUNCTION_ON_ELEMENT( gal_units_ly_to_pc, +0, +0); break;
    case GAL_ARITHMETIC_OP_PC_TO_LY:
      UNIARY_FUNCTION_ON_ELEMENT( gal_units_pc_to_ly, +0, +0); break;
    case GAL_ARITHMETIC_OP_LY_TO_AU:
      UNIARY_FUNCTION_ON_ELEMENT( gal_units_ly_to_au, +0, +0); break;
    case GAL_ARITHMETIC_OP_AU_TO_LY:
      UNIARY_FUNCTION_ON_ELEMENT( gal_units_au_to_ly, +0, +0); break;
    case GAL_ARITHMETIC_OP_RA_TO_DEGREE:
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_SEXAGESIMAL(double, gal_units_ra_to_degree);
      break;
    case GAL_ARITHMETIC_OP_DEC_TO_DEGREE:
      UNIFUNC_RUN_FUNCTION_ON_ELEMENT_SEXAGESIMAL(double, gal_units_dec_to_degree);
      break;
    case GAL_ARITHMETIC_OP_DEGREE_TO_RA:
      UNIARY_FUNCTION_ON_ELEMENT_OUTPUT_STRING(arithmetic_units_degree_to_ra);
      break;
    case GAL_ARITHMETIC_OP_DEGREE_TO_DEC:
      UNIARY_FUNCTION_ON_ELEMENT_OUTPUT_STRING(arithmetic_units_degree_to_dec);
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: operator code %d not recognized",
            __func__, operator);
    }

  /* Clean up. Note that if the input arrays can be freed, and any of right
     or left arrays needed conversion, 'UNIFUNC_CONVERT_TO_COMPILED_TYPE'
     has already freed the input arrays, and we only have 'r' and 'l'
     allocated in any case. Alternatively, when the inputs shouldn't be
     freed, the only allocated spaces are the 'r' and 'l' arrays if their
     types weren't compiled for binary operations, we can tell this from
     the pointers: if they are different from the original pointers, they
     were allocated. */
  if( (flags & GAL_ARITHMETIC_FLAG_FREE) && o!=in)
    gal_data_free(in);

  /* Return */
  return o;
}





/* Call functions in the 'gnuastro/statistics' library. */
static gal_data_t *
arithmetic_from_statistics(int operator, int flags, gal_data_t *input)
{
  gal_data_t *out=NULL;
  int ip= (    (flags & GAL_ARITHMETIC_FLAG_INPLACE)
            || (flags & GAL_ARITHMETIC_FLAG_FREE)    );

  /* The dataset may be empty. In this case, the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if(input->size==0 || input->array==NULL) return input;

  /* Do the operation. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_MINVAL:   out=gal_statistics_minimum(input);break;
    case GAL_ARITHMETIC_OP_MAXVAL:   out=gal_statistics_maximum(input);break;
    case GAL_ARITHMETIC_OP_NUMBERVAL:out=gal_statistics_number(input); break;
    case GAL_ARITHMETIC_OP_SUMVAL:   out=gal_statistics_sum(input);    break;
    case GAL_ARITHMETIC_OP_MEANVAL:  out=gal_statistics_mean(input);   break;
    case GAL_ARITHMETIC_OP_STDVAL:   out=gal_statistics_std(input);    break;
    case GAL_ARITHMETIC_OP_MEDIANVAL:
      out=gal_statistics_median(input, ip); break;
    default:
      error(EXIT_FAILURE, 0, "%s: operator code %d not recognized",
            __func__, operator);
    }

  /* If the input is to be freed, then do so and return the output. */
  if( flags & GAL_ARITHMETIC_FLAG_FREE ) gal_data_free(input);
  return out;
}





/* Call functions in the 'gnuastro/statistics' library. */
static gal_data_t *
arithmetic_to_oned(int operator, int flags, gal_data_t *input)
{
  gal_data_t *out=NULL;
  int inplace=flags & GAL_ARITHMETIC_FLAG_FREE;

  switch(operator)
    {
    case GAL_ARITHMETIC_OP_UNIQUE:
      out=gal_statistics_unique(input, inplace);
      break;
    case GAL_ARITHMETIC_OP_NOBLANK:
      out = inplace ? input : gal_data_copy(input);
      gal_blank_remove(out);
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! please contact us at "
            "'%s' to fix the problem. The value '%d' to "
            "'operator' is not recognized", __func__,
            PACKAGE_BUGREPORT, operator);
    }

  /* Return the output dataset. */
  return out;
}




















/***********************************************************************/
/***************               Adding noise               **************/
/***********************************************************************/

static gsl_rng *
arithmetic_gsl_initialize(int flags, const char **rng_name,
                          unsigned long *rng_seed, char *operator_str)
{
  gsl_rng *rng;

  /* Column counter in case '--envseed' is given and we have multiple
     columns. */
  static unsigned long colcounter=0;

  /* Setup the random number generator. For 'envseed', we want to pass a
     boolean value: either 0 or 1. However, when we say 'flags &
     GAL_ARITHMETIC_ENVSEED', the returned value is the integer positioning
     of the envseed bit (for example if it's on the fourth bit, the value
     will be 8). This can cause problems if it is on the 8th bit (or any
     multiple of 8). So to avoid issues with the bit-positioning of the
     'ENVSEED', we will return the conditional to see if the result of the
     bit-wise '&' is larger than 0 or not (binary). */
  rng=gal_checkset_gsl_rng( (flags & GAL_ARITHMETIC_FLAG_ENVSEED)>0,
                            rng_name, rng_seed);

  /* If '--envseed' was called, we need to add the column counter to the
     requested seed (so it is not the same for all columns. */
  if(flags & GAL_ARITHMETIC_FLAG_ENVSEED)
    {
      *rng_seed += colcounter++;
      gsl_rng_set(rng, *rng_seed);
    }

  /* Print the basic RNG information if requested. */
  if( (flags & GAL_ARITHMETIC_FLAG_QUIET)==0 )
    {
      printf(" - Parameters used for '%s':\n", operator_str);
      printf("   - Random number generator name: %s\n", *rng_name);
      printf("   - Random number generator seed: %lu\n", *rng_seed);
    }

  /* Return the GSL random number generator. */
  return rng;
}





/* The size operator. Reports the size along a given dimension. */
static gal_data_t *
arithmetic_mknoise(int operator, int flags, gal_data_t *in,
                   gal_data_t *arg)
{
  size_t i;
  gsl_rng *rng;
  uint32_t *u32;
  const char *rng_name;
  unsigned long rng_seed;
  gal_data_t *out, *targ;
  int otype=GAL_TYPE_INVALID;
  double *id, *od, *aarr, arg_v;

  /* The dataset may be empty. In this case, the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if(in->size==0 || in->array==NULL) return in;

  /* Sanity checks. */
  if(arg->size!=1 && arg->size!=in->size)
    error(EXIT_FAILURE, 0, "the first popped operand to the '%s' "
          "operator should either be a single number or have the "
          "same number of elements as the input data. Recall that "
          "it specifies the fixed sigma, or background level for "
          "Poisson noise). However, it has %zu elements, in %zu "
          "dimension(s)", gal_arithmetic_operator_string(operator),
          arg->size, arg->ndim);
  if(in->type==GAL_TYPE_STRING)
    error(EXIT_FAILURE, 0, "the input dataset to the '%s' operator "
          "should have a numerical data type (integer or float), but "
          "it has a string type",
          gal_arithmetic_operator_string(operator));

  /* Prepare the output dataset (for the Poisson distribution, it should
     be 32-bit integers). */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_MKNOISE_SIGMA:
    case GAL_ARITHMETIC_OP_MKNOISE_UNIFORM:
    case GAL_ARITHMETIC_OP_MKNOISE_SIGMA_FROM_MEAN:
      otype=GAL_TYPE_FLOAT64; break;
    case GAL_ARITHMETIC_OP_MKNOISE_POISSON: otype=GAL_TYPE_UINT32;  break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the problem. The operator code %d isn't recognized "
            "in this function", __func__, PACKAGE_BUGREPORT, operator);
    }
  in=gal_data_copy_to_new_type_free(in, GAL_TYPE_FLOAT64);
  out = ( in->type==otype
          ? in
          : gal_data_alloc(NULL, otype, in->ndim, in->dsize, in->wcs,
                           0, in->minmapsize, in->quietmmap, NULL, NULL,
                           NULL) );
  targ=gal_data_copy_to_new_type(arg, GAL_TYPE_FLOAT64);
  aarr=targ->array;

  /* Initialize the GSL random number generator. */
  rng=arithmetic_gsl_initialize(flags, &rng_name, &rng_seed,
                                gal_arithmetic_operator_string(operator));

  /* Add the noise. */
  id=in->array;
  od=out->array;
  for(i=0;i<out->size;++i)
    {
      /* Set the argument value. */
      arg_v = arg->size==1 ? aarr[0] : aarr[i];

      /* Make sure the noise identifier is positive. */
      if(arg_v<0)
        error(EXIT_FAILURE, 0, "the noise identifier (sigma for "
              "'mknoise-sigma', background for 'mknoise-poisson', or "
              "range for 'mknoise-uniform') must be positive (it is %g)",
              arg_v);

      /* Do the necessary operation. */
      switch(operator)
        {
        case GAL_ARITHMETIC_OP_MKNOISE_SIGMA:
          od[i] = id[i] + gsl_ran_gaussian(rng, arg_v);
          break;
        case GAL_ARITHMETIC_OP_MKNOISE_SIGMA_FROM_MEAN:
          od[i] = id[i] + arg_v + gsl_ran_gaussian(rng, sqrt(arg_v+id[i]));
          break;
        case GAL_ARITHMETIC_OP_MKNOISE_POISSON:
          u32=out->array;
          u32[i] = gsl_ran_poisson(rng, arg_v + id[i]);
          break;
        case GAL_ARITHMETIC_OP_MKNOISE_UNIFORM:
          od[i] = id[i] + gsl_rng_uniform(rng)*arg_v - arg_v/2;
          break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The operator code %d isn't recognized "
                "in this function", __func__, PACKAGE_BUGREPORT, operator);
        }
    }

  /* Clean up and return */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    { gal_data_free(arg); if(in!=out) gal_data_free(in);}
  gal_data_free(targ);
  gsl_rng_free(rng);
  return out;
}





/* Sanity checks for 'random_from_hist'. */
static void
arithmetic_random_from_hist_sanity(gal_data_t **inhist, gal_data_t **inbinc,
                                   gal_data_t *in, int operator)
{
  size_t i;
  double *d, diff, binwidth, binwidth_e;
  gal_data_t *hist=*inhist, *binc=*inbinc;

  /* The 'hist' and 'bincenter' arrays should be 1-dimensional and both
     should have the same size. */
  if(hist->ndim!=1)
    error(EXIT_FAILURE, 0, "the first popped (nearest) operand to the "
          "'%s' operator should only have a single dimension. However, "
          "it has %zu dimensions",
          gal_arithmetic_operator_string(operator), hist->ndim);
  if(binc->ndim!=1)
    error(EXIT_FAILURE, 0, "the second popped (second nearest) operand "
          "to the '%s' operator should only have a single "
          "dimension. However, it has %zu dimensions",
          gal_arithmetic_operator_string(operator), binc->ndim);
  if(hist->size!=binc->size)
    error(EXIT_FAILURE, 0, "the first and second popped operands to the "
          "'%s' operator must have the same size! However, "
          "they have %zu and %zu elements respectively",
          gal_arithmetic_operator_string(operator), hist->size,
          binc->size);

  /* The input type shouldn't be a string. */
  if( in->type==GAL_TYPE_STRING )
    error(EXIT_FAILURE, 0, "the input dataset to the '%s' "
          "operator should have a numerical data type (integer or "
          "float), but it has a string type",
          gal_arithmetic_operator_string(operator));

  /* The histogram should be in float64 type, we'll also set the bin
     centers to float64 to enable easy sanity checks. */
  *inhist=gal_data_copy_to_new_type_free(hist, GAL_TYPE_FLOAT64);
  binc=*inbinc=gal_data_copy_to_new_type_free(binc, GAL_TYPE_FLOAT64);

  /* For the 'random-from-hist' operator, we will need to assume that the
     bins are equally spaced and that they are ascending. */
  if(operator==GAL_ARITHMETIC_OP_RANDOM_FROM_HIST)
    {
      d=binc->array;
      binwidth=d[1]-d[0];
      if(binwidth<0)
        error(EXIT_FAILURE, 0, "the bins given to the '%s' operator "
              "should be in ascending order, but the second row "
              "(%g) is smaller than the first (%g)",
              gal_arithmetic_operator_string(operator), d[1], d[0]);

      /* Make sure the bins are in ascending order and have a fixed bin
         width (within floating point errors: hence where 1e-6 comes
         from). */
      binwidth_e=binwidth*1e-6;
      for(i=0;i<binc->size-1;++i)
        {
          diff=d[i+1]-d[i];
          if(diff>binwidth+binwidth_e || diff<binwidth-binwidth_e)
            error(EXIT_FAILURE, 0, "the bins given to the '%s' operator "
                  "should be in ascending order, but row %zu (with value "
                  "%g) is larger than the next row's value (%g)",
                  gal_arithmetic_operator_string(operator), i+1, d[i],
                  d[i+1]);
        }
    }
}





/* Build a custom noise distribution. */
static gal_data_t *
arithmetic_random_from_hist(int operator, int flags, gal_data_t *in,
                            gal_data_t *binc, gal_data_t *hist)
{
  size_t rind;
  const char *rng_name;
  gal_data_t *out=NULL;
  unsigned long rng_seed;
  gsl_rng *rng=NULL, *rngu=NULL;
  gsl_ran_discrete_t *disc=NULL;
  double *b, *d, *df, binwidth, halfbinwidth;

  /* The dataset may be empty. In this case, the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if(in->size==0 || in->array==NULL) return in;

  /* Basic sanity checks. */
  arithmetic_random_from_hist_sanity(&hist, &binc, in,
                                     GAL_ARITHMETIC_OP_RANDOM_FROM_HIST);

  /* Allocate the output dataset (based on the size and dimensions of the
     input), then free the input. */
  out=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, in->ndim, in->dsize, in->wcs,
                     0, in->minmapsize, in->quietmmap, "RANDOM_FROM_HIST",
                     hist->unit, "Random values from custom distribution");

  /* Initialize the GSL random number generator. */
  rng=arithmetic_gsl_initialize(flags, &rng_name, &rng_seed,
                                gal_arithmetic_operator_string(operator));
  if(operator==GAL_ARITHMETIC_OP_RANDOM_FROM_HIST)
    rngu=arithmetic_gsl_initialize(flags, &rng_name, &rng_seed,
                                   "uniform component of 'random-"
                                   "from-hist'");

  /* Pre-process the descrete random number generator. */
  disc=gsl_ran_discrete_preproc(hist->size, hist->array);

  /* Apply the random number generator over the output. Note that
     'gsl_ran_discrete' returns the index to the bin centers table that we
     can return. */
  b=binc->array;
  binwidth=b[1]-b[0];
  halfbinwidth=binwidth/2;
  df=(d=out->array)+out->size;
  do
    {
      rind=gsl_ran_discrete(rng, disc);
      *d = ( operator==GAL_ARITHMETIC_OP_RANDOM_FROM_HIST
             ? ((b[rind]-halfbinwidth) + gsl_rng_uniform(rngu)*binwidth)
             : b[rind] );
    }
  while(++d<df);

  /* Clean up and return. */
  gsl_rng_free(rng);
  if(rngu) gsl_rng_free(rngu);
  gsl_ran_discrete_free(disc);
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    {
      gal_data_free(in);
      gal_data_free(hist);
      gal_data_free(binc);
    }
  return out;
}

















/***********************************************************************/
/***************                  Metadata                **************/
/***********************************************************************/

/* The size operator. Reports the size along a given dimension. */
static gal_data_t *
arithmetic_size(int operator, int flags, gal_data_t *in, gal_data_t *arg)
{
  size_t one=1, arg_val;
  gal_data_t *usearg=NULL, *out=NULL;


  /* Sanity checks on argument (dimension number): it should be an integer,
     and have a size of 1. */
  if(arg->type==GAL_TYPE_FLOAT32 || arg->type==GAL_TYPE_FLOAT64)
    error(EXIT_FAILURE, 0, "%s: size operator's dimension argument"
          "must have an integer type", __func__);
  if(arg->size!=1)
    error(EXIT_FAILURE, 0, "%s: size operator's dimension argument"
          "must be a single number, but it has %zu elements", __func__,
          arg->size);


  /* Convert 'arg' to 'size_t' and read it. Note that we can only free the
     'arg' array (while changing its type), when the freeing flag has been
     set. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    {
      arg=gal_data_copy_to_new_type_free(arg, GAL_TYPE_SIZE_T);
      arg_val=*(size_t *)(arg->array);
      gal_data_free(arg);
    }
  else
    {
      usearg=gal_data_copy_to_new_type(arg, GAL_TYPE_SIZE_T);
      arg_val=*(size_t *)(usearg->array);
      gal_data_free(usearg);
    }


  /* Sanity checks on the value of the given argument. */
  if(arg_val>in->ndim)
    error(EXIT_FAILURE, 0, "%s: size operator's dimension argument "
          "(given %zu) cannot be larger than the dimensions of the "
          "given input (%zu)", __func__, arg_val, in->ndim);
  if(arg_val==0)
    error(EXIT_FAILURE, 0, "%s: size operator's dimension argument "
          "(given %zu) cannot be zero: dimensions are counted from 1",
          __func__, arg_val);


  /* Allocate the output array and write the desired dimension. Note that
     'dsize' is in the C order, while the output must be in FITS/Fortran
     order. Also that C order starts from 0, while the FITS order starts
     from 1. */
  out=gal_data_alloc(NULL, GAL_TYPE_SIZE_T, 1, &one, NULL, 0,
                     in->minmapsize, 0, NULL, NULL, NULL);
  *(size_t *)(out->array)=in->dsize[in->ndim-arg_val];


  /* Clean up and return. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    gal_data_free(in);
  return out;
}





/* Stitch multiple operands along a given dimension. */
static gal_data_t *
arithmetic_to_1d(int flags, gal_data_t *input)
{
  size_t i;

  /* Set absurd value to cause a crash if values that shouldn't be used
     are used! */
  for(i=1;i<input->ndim;++i) input->dsize[i]=GAL_BLANK_UINT64;

  /* Reset the metadata and return.*/
  input->ndim=1;
  input->dsize[0]=input->size;
  return input;
}





static size_t
arithmetic_stitch_sanity_check(gal_data_t *list, gal_data_t *fdim)
{
  float *fitsdim;
  gal_data_t *tmp;
  size_t c, dim, otherdim;

  /* Currently we only have the stitch operator for 2D datasets. */
  if(list->ndim>2)
    error(EXIT_FAILURE, 0, "currently the 'stitch' operator only "
          "works with 2D datasets (images) but you have given a "
          "%zu dimensional dataset. Please get in touch with us "
          "at '%s' to add this feature", list->ndim, PACKAGE_BUGREPORT);

  /* Make sure that the dimention is an integer. */
  fitsdim=fdim->array;
  if( fitsdim[0] != ceil(fitsdim[0]) )
    error(EXIT_FAILURE, 0, "the dimension operand (first popped "
          "operand) to 'stitch' should be an integer, but it is "
          "'%g'", fitsdim[0]);

  /* Make sure that the requested dimension is not larger than the input's
     number of dimensions. */
  if(fitsdim[0]>list->ndim)
    error(EXIT_FAILURE, 0, "the dimension operand (first popped "
          "operand) to 'stitch' is %g, but your input datasets "
          "only have %zu dimensions", fitsdim[0], list->ndim);

  /* Convert the given dimension (in FITS standard) to C standard. */
  dim = list->ndim - fitsdim[0];

  /* Go through the list of datasets and make sure the dimensionality is
     fine. */
  c=0;
  for(tmp=list; tmp!=NULL; tmp=tmp->next)
    {
      /* Increment the counter. */
      ++c;

      /* Make sure they have the same numerical data type. */
      if(tmp->type!=list->type)
        error(EXIT_FAILURE, 0, "input dataset number %zu has a "
              "numerical data type of '%s' while the first "
              "input has a type of '%s'",
              c, gal_type_name(tmp->type,1), gal_type_name(list->ndim,1));

      /* Make sure they have the same number of dimensions. */
      if(tmp->ndim!=list->ndim)
        error(EXIT_FAILURE, 0, "input dataset number %zu has %zu "
              "dimensions, while the first input has %zu dimensions",
              c, tmp->ndim, list->ndim);

      /* Make sure the length along the non-requested dimension in all the
         inputs are the same. Recall that we currently only support 2D
         datasets, so 'dim' is either '1' or '0' (we are not using '!dim'
         because some compilers can give the 'logical-not-parentheses'
         warning). */
      otherdim = dim ? 0 : 1;
      if( tmp->dsize[otherdim]!=list->dsize[otherdim])
        error(EXIT_FAILURE, 0, "input dataset number %zu has %zu "
              "elements along dimension number %d, while the first "
              "has %zu elements along that dimension", c,
              tmp->dsize[otherdim], otherdim==0 ? 2 : 1,
              list->dsize[otherdim]);
    }

  /* Return the dimension number that must be used. */
  return dim;
}





/* Stitch multiple operands along a given dimension. */
static gal_data_t *
arithmetic_stitch(int flags, gal_data_t *list, gal_data_t *fdim)
{
  void *oarr;
  gal_data_t *tmp, *out;
  uint8_t type=list->type;
  size_t i, dim, sum, dsize[2]={GAL_BLANK_SIZE_T, GAL_BLANK_SIZE_T};

  /* In case we are dealing with a single-element list, just return it! */
  if(list->next==NULL) return list;

  /* Make sure everything is good! */
  dim=arithmetic_stitch_sanity_check(list, fdim);

  /* Find the size of the final output dataset. */
  sum=0; for(tmp=list; tmp!=NULL; tmp=tmp->next) sum+=tmp->dsize[dim];
  dsize[0] = dim==0 ? sum : list->dsize[0];
  if(list->ndim>1) dsize[1] = dim==0 ? list->dsize[1] : sum;

  /* Allocate the output dataset. */
  out=gal_data_alloc(NULL, list->type, list->ndim, dsize, list->wcs,
                     0, list->minmapsize, list->quietmmap, "Stitched",
                     list->unit, "Stitched dataset");

  /* Write the individual datasets into the output. Note that 'dim' is the
     dimension counter of the C standard. */
  oarr=out->array;
  switch(list->ndim)
    {
    case 1: /* 1D stitching. */
      sum=0;
      for(tmp=list; tmp!=NULL; tmp=tmp->next)
        {
          memcpy(gal_pointer_increment(oarr, sum, type),
                 tmp->array, gal_type_sizeof(type)*tmp->size);
          sum+=tmp->size;
        }
      break;

    case 2: /* 2D stitching. */
      for(tmp=list; tmp!=NULL; tmp=tmp->next)
        {
          switch(dim)
            {

            /* Vertical stitching (second FITS axis is the vertical
               axis). */
            case 0:
              memcpy(oarr,tmp->array, gal_type_sizeof(type)*tmp->size);
              oarr += gal_type_sizeof(type)*tmp->size;
              break;

            /* Horizontal stitching (first FITS axis is the horizontal
               axis). */
            case 1:

              /* Copy row-by-row. */
              for(i=0;i<dsize[0];++i)
                memcpy(gal_pointer_increment(oarr, i*dsize[1], type),
                       gal_pointer_increment(tmp->array, i*tmp->dsize[1],
                                             type),
                       gal_type_sizeof(type)*tmp->dsize[1]);

              /* Copying has finished, increment the start for the next
                 image. Note that in this scenario, the starting pixel for the
                 next image is on the first row, but tmp->dsize[1] pixels
                 away. */
              oarr += gal_type_sizeof(type)*tmp->dsize[1];
              break;

            default:
              error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
                    "to find and fix the problem. The value of 'dim' is "
                    "'%zu' is not understood", __func__, PACKAGE_BUGREPORT,
                    dim);
            }
        }
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
            "fix the problem. The value %zu of 'ndim' should have been "
            "checked before this point!", __func__, PACKAGE_BUGREPORT,
            list->ndim);
    }

  /* Clean up and return. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    gal_list_data_free(list);
  return out;
}



















/***********************************************************************/
/***************                  Where                   **************/
/***********************************************************************/
/* When the 'iftrue' dataset only has one element and the element is blank,
   then it will be replaced with the blank value of the type of the output
   data. */
#define DO_WHERE_OPERATION(ITT, OT) {                                   \
    ITT *it=iftrue->array;                                              \
    OT b, *o=out->array, *of=o+out->size;                               \
    gal_blank_write(&b, out->type);                                     \
    if(iftrue->size==1)                                                 \
      {                                                                 \
        if( gal_blank_is(it, iftrue->type) )                            \
          {                                                             \
            do{*o = (chb && *c==cb) ? *o : (*c ? b   : *o); ++c;      } \
            while(++o<of);                                              \
          }                                                             \
        else                                                            \
          do  {*o = (chb && *c==cb) ? *o : (*c ? *it : *o); ++c;      } \
          while(++o<of);                                                \
      }                                                                 \
    else                                                                \
      do      {*o = (chb && *c==cb) ? *o : (*c ? *it : *o); ++it; ++c;} \
      while(++o<of);                                                    \
}





#define WHERE_OUT_SET(OT)                                               \
  switch(iftrue->type)                                                  \
    {                                                                   \
    case GAL_TYPE_UINT8:    DO_WHERE_OPERATION( uint8_t,  OT);  break;  \
    case GAL_TYPE_INT8:     DO_WHERE_OPERATION( int8_t,   OT);  break;  \
    case GAL_TYPE_UINT16:   DO_WHERE_OPERATION( uint16_t, OT);  break;  \
    case GAL_TYPE_INT16:    DO_WHERE_OPERATION( int16_t,  OT);  break;  \
    case GAL_TYPE_UINT32:   DO_WHERE_OPERATION( uint32_t, OT);  break;  \
    case GAL_TYPE_INT32:    DO_WHERE_OPERATION( int32_t,  OT);  break;  \
    case GAL_TYPE_UINT64:   DO_WHERE_OPERATION( uint64_t, OT);  break;  \
    case GAL_TYPE_INT64:    DO_WHERE_OPERATION( int64_t,  OT);  break;  \
    case GAL_TYPE_FLOAT32:  DO_WHERE_OPERATION( float,    OT);  break;  \
    case GAL_TYPE_FLOAT64:  DO_WHERE_OPERATION( double,   OT);  break;  \
    default:                                                            \
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized for the " \
            "'iftrue' dataset", "WHERE_OUT_SET", iftrue->type);         \
    }





static void
arithmetic_where(int flags, gal_data_t *out, gal_data_t *cond,
                 gal_data_t *iftrue)
{
  size_t i;
  int chb;    /* Read as: "Condition-Has-Blank" */
  unsigned char *c=cond->array, cb=GAL_BLANK_UINT8;

  /* The datasets may be empty. In this case the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if(    out->size==0    || out->array==NULL
      || cond->size==0   || cond->array==NULL
      || iftrue->size==0 || iftrue->array==NULL )
    {
      if(flags & GAL_ARITHMETIC_FLAG_FREE)
        { gal_data_free(cond); gal_data_free(iftrue); }
      if(out->array) {free(out->array); out->array=NULL;}
      if(out->dsize) for(i=0;i<out->ndim;++i) out->dsize[i]=0;
      out->size=0; return;
    }

  /* The condition operator has to be unsigned char. */
  if(cond->type!=GAL_TYPE_UINT8)
    error(EXIT_FAILURE, 0, "%s: the condition operand must be an "
          "'uint8' type, but the given condition operand has a "
          "'%s' type", __func__, gal_type_name(cond->type, 1));

  /* The dimension and sizes of the out and condition data sets must be the
     same. */
  if( gal_dimension_is_different(out, cond) )
    error(EXIT_FAILURE, 0, "%s: the output and condition datasets "
          "must have the same size", __func__);

  /* See if the condition array has blank values. */
  chb=gal_blank_present(cond, 0);

  /* Do the operation. */
  switch(out->type)
    {
    case GAL_TYPE_UINT8:         WHERE_OUT_SET( uint8_t  );      break;
    case GAL_TYPE_INT8:          WHERE_OUT_SET( int8_t   );      break;
    case GAL_TYPE_UINT16:        WHERE_OUT_SET( uint16_t );      break;
    case GAL_TYPE_INT16:         WHERE_OUT_SET( int16_t  );      break;
    case GAL_TYPE_UINT32:        WHERE_OUT_SET( uint32_t );      break;
    case GAL_TYPE_INT32:         WHERE_OUT_SET( int32_t  );      break;
    case GAL_TYPE_UINT64:        WHERE_OUT_SET( uint64_t );      break;
    case GAL_TYPE_INT64:         WHERE_OUT_SET( int64_t  );      break;
    case GAL_TYPE_FLOAT32:       WHERE_OUT_SET( float    );      break;
    case GAL_TYPE_FLOAT64:       WHERE_OUT_SET( double   );      break;
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized for the 'out'",
            __func__, out->type);
    }

  /* Clean up if necessary. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    {
      gal_data_free(cond);
      gal_data_free(iftrue);
    }
}




















/***********************************************************************/
/***************        Multiple operand operators        **************/
/***********************************************************************/
struct multioperandparams
{
  gal_data_t      *list;        /* List of input datasets.           */
  gal_data_t       *out;        /* Output dataset.                   */
  size_t           dnum;        /* Number of input dataset.          */
  int          operator;        /* Operator to use.                  */
  uint8_t     *hasblank;        /* Array of 0s or 1s for each input. */
  float              p1;        /* Sigma-cliping parameter 1.        */
  float              p2;        /* Sigma-cliping parameter 2.        */
};





#define MULTIOPERAND_MIN(TYPE) {                                        \
    size_t n, j=0;                                                      \
    TYPE t, max, *o=p->out->array;                                      \
    gal_type_max(p->list->type, &max);                                  \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        t=max;                                                          \
        j=tprm->indexs[tind];                                           \
                                                                        \
        for(i=0;i<p->dnum;++i)  /* Loop over each array. */             \
          {   /* Only for integer types, b==b. */                       \
            if( p->hasblank[i] && b==b)                                 \
              {                                                         \
                if( a[i][j] != b )                                      \
                  { t = a[i][j] < t ? a[i][j] : t; ++n; }               \
              }                                                         \
            else { t = a[i][j] < t ? a[i][j] : t; ++n; }                \
          }                                                             \
        o[j] = n ? t : b;  /* No usable elements: set to blank. */      \
      }                                                                 \
  }





#define MULTIOPERAND_MAX(TYPE) {                                        \
    size_t n, j=0;                                                      \
    TYPE t, min, *o=p->out->array;                                      \
    gal_type_min(p->list->type, &min);                                  \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        t=min;                                                          \
        j=tprm->indexs[tind];                                           \
                                                                        \
        for(i=0;i<p->dnum;++i)  /* Loop over each array. */             \
          {   /* Only for integer types, b==b. */                       \
            if( p->hasblank[i] && b==b)                                 \
              {                                                         \
                if( a[i][j] != b )                                      \
                  { t = a[i][j] > t ? a[i][j] : t; ++n; }               \
              }                                                         \
            else { t = a[i][j] > t ? a[i][j] : t; ++n; }                \
          }                                                             \
        o[j] = n ? t : b;  /* No usable elements: set to blank. */      \
      }                                                                 \
  }





#define MULTIOPERAND_NUM {                                              \
    int use;                                                            \
    size_t n, j;                                                        \
    uint32_t *o=p->out->array;                                          \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        j=tprm->indexs[tind];                                           \
                                                                        \
        for(i=0;i<p->dnum;++i)  /* Loop over each array. */             \
          {                                                             \
            /* Only integers and non-NaN floats: v==v is 1. */          \
            if(p->hasblank[i])                                          \
              use = ( b==b                                              \
                      ? ( a[i][j]!=b       ? 1 : 0 )      /* Integer */ \
                      : ( a[i][j]==a[i][j] ? 1 : 0 ) );   /* Float   */ \
            else use=1;                                                 \
                                                                        \
            /* Increment counter if necessary. */                       \
            if(use) ++n;                                                \
          }                                                             \
        o[j] = n;                                                       \
      }                                                                 \
  }





#define MULTIOPERAND_SUM {                                              \
    int use;                                                            \
    double sum;                                                         \
    size_t n, j;                                                        \
    float *o=p->out->array;                                             \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        sum=0.0f;                                                       \
        j=tprm->indexs[tind];                                           \
                                                                        \
        for(i=0;i<p->dnum;++i)  /* Loop over each array. */             \
          {                                                             \
            /* Only integers and non-NaN floats: v==v is 1. */          \
            if(p->hasblank[i])                                          \
              use = ( b==b                                              \
                      ? ( a[i][j]!=b       ? 1 : 0 )     /* Integer */  \
                      : ( a[i][j]==a[i][j] ? 1 : 0 ) );  /* Float   */  \
            else use=1;                                                 \
                                                                        \
            /* Use in sum if necessary. */                              \
            if(use) { sum += a[i][j]; ++n; }                            \
          }                                                             \
        o[j] = n ? sum : NAN; /* Not using 'b', because input type */   \
      }           /* may be integer, while output is always float. */   \
  }





#define MULTIOPERAND_MEAN {                                             \
    int use;                                                            \
    double sum;                                                         \
    size_t n, j;                                                        \
    float *o=p->out->array;                                             \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        sum=0.0f;                                                       \
        j=tprm->indexs[tind];                                           \
                                                                        \
        for(i=0;i<p->dnum;++i)  /* Loop over each array. */             \
          {                                                             \
            /* Only integers and non-NaN floats: v==v is 1. */          \
            if(p->hasblank[i])                                          \
              use = ( b==b                                              \
                      ? ( a[i][j]!=b       ? 1 : 0 )     /* Integer */  \
                      : ( a[i][j]==a[i][j] ? 1 : 0 ) );  /* Float   */  \
            else use=1;                                                 \
                                                                        \
            /* Calculate the mean if necessary. */                      \
            if(use) { sum += a[i][j]; ++n; }                            \
          }                                                             \
        o[j] = n ? sum/n : NAN; /* Not using 'b', because input type */ \
      }             /* may be integer, while output is always float. */ \
  }





#define MULTIOPERAND_STD {                                              \
    int use;                                                            \
    size_t n, j;                                                        \
    double sum, sum2;                                                   \
    float *o=p->out->array;                                             \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        sum=sum2=0.0f;                                                  \
        j=tprm->indexs[tind];                                           \
                                                                        \
        for(i=0;i<p->dnum;++i)  /* Loop over each array. */             \
          {                                                             \
            /* Only integers and non-NaN floats: v==v is 1. */          \
            if(p->hasblank[i])                                          \
              use = ( b==b                                              \
                      ? ( a[i][j]!=b       ? 1 : 0 )     /* Integer */  \
                      : ( a[i][j]==a[i][j] ? 1 : 0 ) );  /* Float   */  \
            else use=1;                                                 \
                                                                        \
            /* Calculate the necessary parameters if necessary. */      \
            if(use)                                                     \
              {                                                         \
                sum2 += a[i][j] * a[i][j];                              \
                sum  += a[i][j];                                        \
                ++n;                                                    \
              }                                                         \
          }                                                             \
        o[j] = n ? sqrt( (sum2-sum*sum/n)/n ) : NAN; /* Not using 'b' */ \
      } /* because input may be integer, but output is always float. */ \
  }





#define MULTIOPERAND_MEDIAN(TYPE, QSORT_F) {                            \
    int use;                                                            \
    size_t n, j;                                                        \
    float *o=p->out->array;                                             \
    TYPE *pixs=gal_pointer_allocate(p->list->type, p->dnum, 0,          \
                                    __func__, "pixs");                  \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        j=tprm->indexs[tind];                                           \
                                                                        \
        /* Loop over each array: 'i' is input dataset's index. */       \
        for(i=0;i<p->dnum;++i)                                          \
          {                                                             \
            /* Only integers and non-NaN floats: v==v is 1. */          \
            if(p->hasblank[i])                                          \
              use = ( b==b                                              \
                      ? ( a[i][j]!=b       ? 1 : 0 )     /* Integer */  \
                      : ( a[i][j]==a[i][j] ? 1 : 0 ) );  /* Float   */  \
            else use=1;                                                 \
                                                                        \
            /* Put the value into the array of values. */               \
            if(use) pixs[n++]=a[i][j];                                  \
          }                                                             \
                                                                        \
        /* Sort all the values for this pixel and return the median. */ \
        if(n)                                                           \
          {                                                             \
            qsort(pixs, n, sizeof *pixs, QSORT_F);                      \
            o[j] = n%2 ? pixs[n/2] : (pixs[n/2] + pixs[n/2-1])/2 ;      \
          }                                                             \
        else                                                            \
          o[j]=NAN; /* Not using 'b' because input may be integer */    \
      }                             /* but output is always float.*/    \
                                                                        \
    /* Clean up. */                                                     \
    free(pixs);                                                         \
  }





#define MULTIOPERAND_QUANTILE(TYPE) {                                   \
    size_t n, j;                                                        \
    gal_data_t *quantile;                                               \
    TYPE *o=p->out->array;                                              \
    TYPE *pixs=gal_pointer_allocate(p->list->type, p->dnum, 0,          \
                                    __func__, "pixs");                  \
    gal_data_t *cont=gal_data_alloc(pixs, p->list->type, 1, &p->dnum,   \
                                    NULL, 0, -1, 1, NULL, NULL, NULL);  \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        j=tprm->indexs[tind];                                           \
                                                                        \
        /* Read the necessay values from each input. */                 \
        for(i=0;i<p->dnum;++i) pixs[n++]=a[i][j];                       \
                                                                        \
        /* If there are any elements, do the measurement. */            \
        if(n)                                                           \
          {                                                             \
            /* Calculate the quantile and put it in the output. */      \
            quantile=gal_statistics_quantile(cont, p->p1, 1);           \
            memcpy(&o[j], quantile->array,                              \
                   gal_type_sizeof(p->list->type));                     \
            gal_data_free(quantile);                                    \
                                                                        \
            /* Since we are doing sigma-clipping in place, the size, */ \
            /* and flags need to be reset. */                           \
            cont->flag=0;                                               \
            cont->size=cont->dsize[0]=p->dnum;                          \
          }                                                             \
        else                                                            \
          o[j]=b;                                                       \
      }                                                                 \
                                                                        \
    /* Clean up (note that 'pixs' is inside of 'cont'). */              \
    gal_data_free(cont);                                                \
  }





#define MULTIOPERAND_SIGCLIP(TYPE) {                                    \
    size_t n, j;                                                        \
    gal_data_t *sclip;                                                  \
    uint32_t *N=p->out->array;                                          \
    float *sarr, *o=p->out->array;                                      \
    TYPE *pixs=gal_pointer_allocate(p->list->type, p->dnum, 0,          \
                                    __func__, "pixs");                  \
    gal_data_t *cont=gal_data_alloc(pixs, p->list->type, 1, &p->dnum,   \
                                    NULL, 0, -1, 1, NULL, NULL, NULL);  \
                                                                        \
    /* Go over all the pixels assigned to this thread. */               \
    for(tind=0; tprm->indexs[tind] != GAL_BLANK_SIZE_T; ++tind)         \
      {                                                                 \
        /* Initialize, 'j' is desired pixel's index. */                 \
        n=0;                                                            \
        j=tprm->indexs[tind];                                           \
                                                                        \
        /* Read the necessay values from each input. */                 \
        for(i=0;i<p->dnum;++i) pixs[n++]=a[i][j];                       \
                                                                        \
        /* If there are any usable elements, do the measurement. */     \
        if(n)                                                           \
          {                                                             \
            /* Calculate the sigma-clip and write it in. */             \
            sclip=gal_statistics_sigma_clip(cont, p->p1, p->p2, 1, 1);  \
            sarr=sclip->array;                                          \
            switch(p->operator)                                         \
              {                                                         \
              case GAL_ARITHMETIC_OP_SIGCLIP_STD:    o[j]=sarr[3]; break;\
              case GAL_ARITHMETIC_OP_SIGCLIP_MEAN:   o[j]=sarr[2]; break;\
              case GAL_ARITHMETIC_OP_SIGCLIP_MEDIAN: o[j]=sarr[1]; break;\
              case GAL_ARITHMETIC_OP_SIGCLIP_NUMBER: N[j]=sarr[0]; break;\
              default:                                                  \
                error(EXIT_FAILURE, 0, "%s: a bug! the code %d is not " \
                      "valid for sigma-clipping results", __func__,     \
                      p->operator);                                     \
              }                                                         \
            gal_data_free(sclip);                                       \
                                                                        \
            /* Since we are doing sigma-clipping in place, the size, */ \
            /* and flags need to be reset. */                           \
            cont->flag=0;                                               \
            cont->size=cont->dsize[0]=p->dnum;                          \
          }                                                             \
        else                                                            \
          o[j] = ( p->operator==GAL_ARITHMETIC_OP_SIGCLIP_NUMBER        \
                   ? 0.0    /* Not using 'b' because input can be an */ \
                   : NAN );   /* integer but output is always float. */ \
      }                                                                 \
                                                                        \
    /* Clean up (note that 'pixs' is inside of 'cont'). */              \
    gal_data_free(cont);                                                \
  }





#define MULTIOPERAND_TYPE_SET(TYPE, QSORT_F) {                          \
    TYPE b, **a;                                                        \
    gal_data_t *tmp;                                                    \
    size_t i=0, tind;                                                   \
                                                                        \
    /* Allocate space to keep the pointers to the arrays of each. */    \
    /* Input data structure. The operators will increment these */      \
    /* pointers while parsing them. */                                  \
    errno=0;                                                            \
    a=malloc(p->dnum*sizeof *a);                                        \
    if(a==NULL)                                                         \
      error(EXIT_FAILURE, 0, "%s: %zu bytes for 'a'",                   \
            "MULTIOPERAND_TYPE_SET", p->dnum*sizeof *a);                \
                                                                        \
    /* Fill in the array pointers and the blank value for this type. */ \
    gal_blank_write(&b, p->list->type);                                 \
    for(tmp=p->list;tmp!=NULL;tmp=tmp->next)                            \
      a[i++]=tmp->array;                                                \
                                                                        \
    /* Do the operation. */                                             \
    switch(p->operator)                                                 \
      {                                                                 \
      case GAL_ARITHMETIC_OP_MIN:                                       \
        MULTIOPERAND_MIN(TYPE);                                         \
        break;                                                          \
                                                                        \
      case GAL_ARITHMETIC_OP_MAX:                                       \
        MULTIOPERAND_MAX(TYPE);                                         \
        break;                                                          \
                                                                        \
      case GAL_ARITHMETIC_OP_NUMBER:                                    \
        MULTIOPERAND_NUM;                                               \
        break;                                                          \
                                                                        \
      case GAL_ARITHMETIC_OP_SUM:                                       \
        MULTIOPERAND_SUM;                                               \
        break;                                                          \
                                                                        \
      case GAL_ARITHMETIC_OP_MEAN:                                      \
        MULTIOPERAND_MEAN;                                              \
        break;                                                          \
                                                                        \
      case GAL_ARITHMETIC_OP_STD:                                       \
        MULTIOPERAND_STD;                                               \
        break;                                                          \
                                                                        \
      case GAL_ARITHMETIC_OP_MEDIAN:                                    \
        MULTIOPERAND_MEDIAN(TYPE, QSORT_F);                             \
        break;                                                          \
                                                                        \
      case GAL_ARITHMETIC_OP_QUANTILE:                                  \
        MULTIOPERAND_QUANTILE(TYPE);                                    \
        break;                                                          \
                                                                        \
      case GAL_ARITHMETIC_OP_SIGCLIP_STD:                               \
      case GAL_ARITHMETIC_OP_SIGCLIP_MEAN:                              \
      case GAL_ARITHMETIC_OP_SIGCLIP_MEDIAN:                            \
      case GAL_ARITHMETIC_OP_SIGCLIP_NUMBER:                            \
        MULTIOPERAND_SIGCLIP(TYPE);                                     \
        break;                                                          \
                                                                        \
      default:                                                          \
        error(EXIT_FAILURE, 0, "%s: operator code %d not recognized",   \
              "MULTIOPERAND_TYPE_SET", p->operator);                    \
      }                                                                 \
                                                                        \
    /* Clean up. */                                                     \
    free(a);                                                            \
  }





/* Worker function on each thread. */
static void *
multioperand_on_thread(void *in_prm)
{
  /* Low-level definitions to be done first. */
  struct gal_threads_params *tprm=(struct gal_threads_params *)in_prm;
  struct multioperandparams *p=(struct multioperandparams *)tprm->params;

  /* Do the operation on each thread. */
  switch(p->list->type)
    {
    case GAL_TYPE_UINT8:
      MULTIOPERAND_TYPE_SET(uint8_t,   gal_qsort_uint8_i);
      break;
    case GAL_TYPE_INT8:
      MULTIOPERAND_TYPE_SET(int8_t,    gal_qsort_int8_i);
      break;
    case GAL_TYPE_UINT16:
      MULTIOPERAND_TYPE_SET(uint16_t,  gal_qsort_uint16_i);
      break;
    case GAL_TYPE_INT16:
      MULTIOPERAND_TYPE_SET(int16_t,   gal_qsort_int16_i);
      break;
    case GAL_TYPE_UINT32:
      MULTIOPERAND_TYPE_SET(uint32_t,  gal_qsort_uint32_i);
      break;
    case GAL_TYPE_INT32:
      MULTIOPERAND_TYPE_SET(int32_t,   gal_qsort_int32_i);
      break;
    case GAL_TYPE_UINT64:
      MULTIOPERAND_TYPE_SET(uint64_t,  gal_qsort_uint64_i);
      break;
    case GAL_TYPE_INT64:
      MULTIOPERAND_TYPE_SET(int64_t,   gal_qsort_int64_i);
      break;
    case GAL_TYPE_FLOAT32:
      MULTIOPERAND_TYPE_SET(float,     gal_qsort_float32_i);
      break;
    case GAL_TYPE_FLOAT64:
      MULTIOPERAND_TYPE_SET(double,    gal_qsort_float64_i);
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, p->list->type);
    }

  /* Wait for all the other threads to finish, then return. */
  if(tprm->b) pthread_barrier_wait(tprm->b);
  return NULL;
}





/* The single operator in this function is assumed to be a linked list. The
   number of operators is determined from the fact that the last node in
   the linked list must have a NULL pointer as its 'next' element. */
static gal_data_t *
arithmetic_multioperand(int operator, int flags, gal_data_t *list,
                        gal_data_t *params, size_t numthreads)
{
  size_t i=0, dnum=1;
  float p1=NAN, p2=NAN;
  struct multioperandparams p;
  gal_data_t *out, *tmp, *ttmp;
  uint8_t *hasblank, otype=GAL_TYPE_INVALID;


  /* For generality, 'list' can be a NULL pointer, in that case, this
     function will return a NULL pointer and avoid further processing. */
  if(list==NULL) return NULL;


  /* If any parameters are given, prepare them. */
  for(tmp=params; tmp!=NULL; tmp=tmp->next)
    {
      /* Basic sanity checks. */
      if(tmp->size>1)
        error(EXIT_FAILURE, 0, "%s: parameters must be a single number",
              __func__);
      if(tmp->type!=GAL_TYPE_FLOAT32)
        error(EXIT_FAILURE, 0, "%s: parameters must be float32 type",
              __func__);

      /* Write them */
      if(isnan(p1)) p1=((float *)(tmp->array))[0];
      else          p2=((float *)(tmp->array))[0];

      /* Operator specific, parameter sanity checks. */
      switch(operator)
        {
        case GAL_ARITHMETIC_OP_QUANTILE:
          if(p1<0 || p1>1)
            error(EXIT_FAILURE, 0, "%s: the parameter given to the "
                  "'quantile' operator must be between (and including) "
                  "0 and 1. The given value is: %g", __func__, p1);
          break;
        }
    }


  /* Do a simple sanity check, comparing the operand on top of the list to
     the rest of the operands within the list. */
  for(tmp=list->next;tmp!=NULL;tmp=tmp->next)
    {
      /* Increment the number of structures. */
      ++dnum;

      /* Check the types. */
      if(tmp->type!=list->type)
        error(EXIT_FAILURE, 0, "%s: the types of all operands to the '%s' "
              "operator must be same", __func__,
              gal_arithmetic_operator_string(operator));

      /* Make sure they actually have any data. */
      if(tmp->size==0 || tmp->array==NULL)
        error(EXIT_FAILURE, 0, "%s: atleast one input operand doesn't "
              "have any data", __func__);

      /* Check the sizes. */
      if( gal_dimension_is_different(list, tmp) )
        error(EXIT_FAILURE, 0, "%s: the sizes of all operands to the '%s' "
              "operator must be same", __func__,
              gal_arithmetic_operator_string(operator));
    }


  /* Set the output dataset type. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_MIN:            otype=list->type;       break;
    case GAL_ARITHMETIC_OP_MAX:            otype=list->type;       break;
    case GAL_ARITHMETIC_OP_NUMBER:         otype=GAL_TYPE_UINT32;  break;
    case GAL_ARITHMETIC_OP_SUM:            otype=GAL_TYPE_FLOAT32; break;
    case GAL_ARITHMETIC_OP_MEAN:           otype=GAL_TYPE_FLOAT32; break;
    case GAL_ARITHMETIC_OP_STD:            otype=GAL_TYPE_FLOAT32; break;
    case GAL_ARITHMETIC_OP_MEDIAN:         otype=GAL_TYPE_FLOAT32; break;
    case GAL_ARITHMETIC_OP_QUANTILE:       otype=list->type;       break;
    case GAL_ARITHMETIC_OP_SIGCLIP_STD:    otype=GAL_TYPE_FLOAT32; break;
    case GAL_ARITHMETIC_OP_SIGCLIP_MEAN:   otype=GAL_TYPE_FLOAT32; break;
    case GAL_ARITHMETIC_OP_SIGCLIP_MEDIAN: otype=GAL_TYPE_FLOAT32; break;
    case GAL_ARITHMETIC_OP_SIGCLIP_NUMBER: otype=GAL_TYPE_UINT32;  break;
    default:
      error(EXIT_FAILURE, 0, "%s: operator code %d isn't recognized",
            __func__, operator);
    }


  /* Set the output data structure. */
  if( (flags & GAL_ARITHMETIC_FLAG_INPLACE) && otype==list->type)
    out = list;                 /* The top element in the list. */
  else
    out = gal_data_alloc(NULL, otype, list->ndim, list->dsize,
                         list->wcs, 0, list->minmapsize, list->quietmmap,
                         NULL, NULL, NULL);


  /* hasblank is used to see if a blank value should be checked for each
     list element or not. */
  hasblank=gal_pointer_allocate(GAL_TYPE_UINT8, dnum, 0, __func__,
                                "hasblank");
  for(tmp=list;tmp!=NULL;tmp=tmp->next)
    hasblank[i++]=gal_blank_present(tmp, 0);


  /* Set the parameters necessary for multithreaded operation and spin them
     off to do apply the operator. */
  p.p1=p1;
  p.p2=p2;
  p.out=out;
  p.list=list;
  p.dnum=dnum;
  p.operator=operator;
  p.hasblank=hasblank;
  gal_threads_spin_off(multioperand_on_thread, &p, out->size, numthreads,
                       list->minmapsize, list->quietmmap);


  /* Clean up and return. Note that the operation might have been done in
     place. In that case, a list element was used. So we need to check
     before freeing each data structure. If we are on the designated output
     dataset, we should set its 'next' pointer to NULL so it isn't treated
     as a list any more by future functions. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    {
      tmp=list;
      while(tmp!=NULL)
        {
          ttmp=tmp->next;
          if(tmp==out) tmp->next=NULL; else gal_data_free(tmp);
          tmp=ttmp;
        }
      if(params) gal_list_data_free(params);
    }
  free(hasblank);
  return out;
}




















/**********************************************************************/
/****************           Binary operators          *****************/
/**********************************************************************/
/* See if we should check for blanks. When both types are floats, blanks
   don't need to be checked (the floating point standard will do the job
   for us). It is also not necessary to check blanks in bitwise operators,
   but bitwise operators have their own macro
   ('BINARY_OP_INCR_OT_RT_LT_SET') which doesn' use 'checkblanks'. */
int
gal_arithmetic_binary_checkblank(gal_data_t *l, gal_data_t *r)
{
  return ((((l->type!=GAL_TYPE_FLOAT32    && l->type!=GAL_TYPE_FLOAT64)
            || (r->type!=GAL_TYPE_FLOAT32 && r->type!=GAL_TYPE_FLOAT64))
           && (gal_blank_present(l, 1) || gal_blank_present(r, 1)))
          ? 1 : 0 );
}





static int
arithmetic_binary_out_type(int operator, gal_data_t *l, gal_data_t *r)
{
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_LT:
    case GAL_ARITHMETIC_OP_LE:
    case GAL_ARITHMETIC_OP_GT:
    case GAL_ARITHMETIC_OP_GE:
    case GAL_ARITHMETIC_OP_EQ:
    case GAL_ARITHMETIC_OP_NE:
    case GAL_ARITHMETIC_OP_AND:
    case GAL_ARITHMETIC_OP_OR:
      return GAL_TYPE_UINT8;

    default:
      return gal_type_out(l->type, r->type);
    }
  return -1;
}





/* Binary arithmetic's type checks: According to C's implicity/automatic
   type conversion in binary operators, the unsigned types have higher
   precedence for the same width. See the description of
   'gal_type_string_to_number' in the Gnuastro book for an example.

   To avoid this situation, it is therefore necessary to print a message
   and let the user know that strange situations like above may occur. Just
   note that this won't happen if 'a' and 'b' have different widths: such
   that this will work fine: 'int8_t a=-1; uint16_t b=50000'. */
static void
arithmetic_binary_int_sanity_check(gal_data_t *l, gal_data_t *r,
                                   int operator)
{
  /* Variables to simplify the checks. */
  int l_is_signed=0, r_is_signed=0;

  /* Warning only necessary for same-width types. */
  if( gal_type_sizeof(l->type)==gal_type_sizeof(r->type) )
    {
      /* Warning not needed when one of the inputs is a float. */
      if(    l->type==GAL_TYPE_FLOAT32 || l->type==GAL_TYPE_FLOAT64
          || r->type==GAL_TYPE_FLOAT32 || r->type==GAL_TYPE_FLOAT64 )
        return;
      else
        {
          /* Warning not needed if both have (or don't have) a sign. */
          if(    l->type==GAL_TYPE_INT8  || l->type==GAL_TYPE_INT16
              || l->type==GAL_TYPE_INT32 || l->type==GAL_TYPE_INT64 )
            l_is_signed=1;
          if(    r->type==GAL_TYPE_INT8  || r->type==GAL_TYPE_INT16
              || r->type==GAL_TYPE_INT32 || r->type==GAL_TYPE_INT64 )
            r_is_signed=1;
          if( l_is_signed!=r_is_signed )
            error(EXIT_SUCCESS, 0, "warning: the two integer operands "
                  "given to '%s' have the same width (number of bits), "
                  "but a different sign: the first popped operand (that "
                  "is closer to the operator, or the \"right\" operand) "
                  "has type '%s' and the second (or \"left\" operand) "
                  "has type '%s'. This may create wrong results, for "
                  "example when the signed input contains negative "
                  "values. To address this problem there are two "
                  "options: 1) if you know that the signed input can "
                  "only have positive values, use Arithmetic's type "
                  "conversion operators to convert it to an un-signed "
                  "type of the same width (e.g., 'uint8', 'uint16', "
                  "'uint32' or 'uint64'). 2) Convert the unsigned "
                  "input to a signed one of the next largest width "
                  "using the type conversion operators (e.g., 'int16', "
                  "'int32' or 'int64'). For more, see the \"Integer "
                  "benefits and pitfalls\" section of Gnuastro's "
                  "manual with this command: 'info gnuastro integer'. "
                  "This warning can be removed with '--quiet' (or "
                  "'-q')", gal_arithmetic_operator_string(operator),
                  gal_type_name(r->type, 1), gal_type_name(l->type, 1));
        }
    }
}





static gal_data_t *
arithmetic_binary(int operator, int flags, gal_data_t *l, gal_data_t *r)
{
  /* Read the variable arguments. 'lo' and 'ro' keep the original data, in
     case their type isn't built (based on configure options are configure
     time). */
  int32_t otype;
  gal_data_t *o=NULL;
  size_t out_size, minmapsize;
  int quietmmap=l->quietmmap && r->quietmmap;


  /* The datasets may be empty. In this case, the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if( l->size==0 || l->array==NULL || r->size==0 || r->array==NULL )
    {
      if(l->array==0 || l->array==NULL)
        {   if(flags & GAL_ARITHMETIC_FLAG_FREE) gal_data_free(r); return l;}
      else {if(flags & GAL_ARITHMETIC_FLAG_FREE) gal_data_free(l); return r;}
    }


  /* Simple sanity check on the input sizes. */
  if( !( (flags & GAL_ARITHMETIC_FLAG_NUMOK) && (l->size==1 || r->size==1))
      && gal_dimension_is_different(l, r) )
    error(EXIT_FAILURE, 0, "%s: the non-number inputs to '%s' don't "
          "have the same dimension/size", __func__,
          gal_arithmetic_operator_string(operator));


  /* Print a warning if the inputs are both integers, but have different
     signs (the user needs to know that the output may not be what they
     expect!).*/
  if( (flags & GAL_ARITHMETIC_FLAG_QUIET)==0 )
    arithmetic_binary_int_sanity_check(l, r, operator);


  /* Set the output type. For the comparison operators, the output type is
     either 0 or 1, so we will set the output type to 'unsigned char' for
     efficient memory and CPU usage. Since the number of operators without
     a fixed output type (like the conditionals) is less, by 'default' we
     will set the output type to 'unsigned char', and if any of the other
     operatrs are given, it will be chosen based on the input types. */
  otype=arithmetic_binary_out_type(operator, l, r);


  /* Set the output sizes. */
  minmapsize = ( l->minmapsize < r->minmapsize
                 ? l->minmapsize : r->minmapsize );
  out_size = l->size > r->size ? l->size : r->size;


  /* If we want inplace output, set the output pointer to one input. Note
     that the output type can be different from both inputs. */
  if(flags & GAL_ARITHMETIC_FLAG_INPLACE)
    {
      if     (l->type==otype && out_size==l->size)   o = l;
      else if(r->type==otype && out_size==r->size)   o = r;
    }


  /* If the output pointer was not set above for any of the possible
     reasons, allocate it. For 'mmapsize', note that since its 'size_t', it
     will always be positive. The '-1' that is recommended to give when you
     want the value in RAM is actually the largest possible memory
     location. So we just have to choose the smaller minmapsize of the two
     to decide if the output array should be in RAM or not. */
  if(o==NULL)
    o = gal_data_alloc(NULL, otype,
                       l->size>1 ? l->ndim  : r->ndim,
                       l->size>1 ? l->dsize : r->dsize,
                       l->size>1 ? l->wcs   : r->wcs,
                       0, minmapsize, quietmmap, NULL, NULL, NULL );


  /* Call the proper function for the operator. Since they heavily involve
     macros, their compilation can be very large if they are in a single
     function and file. So there is a separate C source and header file for
     each of these functions. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_PLUS:     arithmetic_plus(l, r, o);     break;
    case GAL_ARITHMETIC_OP_MINUS:    arithmetic_minus(l, r, o);    break;
    case GAL_ARITHMETIC_OP_MULTIPLY: arithmetic_multiply(l, r, o); break;
    case GAL_ARITHMETIC_OP_DIVIDE:   arithmetic_divide(l, r, o);   break;
    case GAL_ARITHMETIC_OP_LT:       arithmetic_lt(l, r, o);       break;
    case GAL_ARITHMETIC_OP_LE:       arithmetic_le(l, r, o);       break;
    case GAL_ARITHMETIC_OP_GT:       arithmetic_gt(l, r, o);       break;
    case GAL_ARITHMETIC_OP_GE:       arithmetic_ge(l, r, o);       break;
    case GAL_ARITHMETIC_OP_EQ:       arithmetic_eq(l, r, o);       break;
    case GAL_ARITHMETIC_OP_NE:       arithmetic_ne(l, r, o);       break;
    case GAL_ARITHMETIC_OP_AND:      arithmetic_and(l, r, o);      break;
    case GAL_ARITHMETIC_OP_OR:       arithmetic_or(l, r, o);       break;
    case GAL_ARITHMETIC_OP_BITAND:   arithmetic_bitand(l, r, o);   break;
    case GAL_ARITHMETIC_OP_BITOR:    arithmetic_bitor(l, r, o);    break;
    case GAL_ARITHMETIC_OP_BITXOR:   arithmetic_bitxor(l, r, o);   break;
    case GAL_ARITHMETIC_OP_BITLSH:   arithmetic_bitlsh(l, r, o);   break;
    case GAL_ARITHMETIC_OP_BITRSH:   arithmetic_bitrsh(l, r, o);   break;
    case GAL_ARITHMETIC_OP_MODULO:   arithmetic_modulo(l, r, o);   break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s to address "
            "the problem. %d is not a valid operator code", __func__,
            PACKAGE_BUGREPORT, operator);
    }


  /* Clean up if necessary. Note that if the operation was requested to be
     in place, then the output might be one of the inputs. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    {
      if     (o==l)       gal_data_free(r);
      else if(o==r)       gal_data_free(l);
      else              { gal_data_free(l); gal_data_free(r); }
    }


  /* Return */
  return o;
}





#define BINFUNC_RUN_FUNCTION(OT, RT, LT, OP, AFTER){                    \
    LT *la=l->array;                                                    \
    RT *ra=r->array;                                                    \
    OT *oa=o->array, *of=oa + o->size;                                  \
    if(l->size==r->size) do *oa = OP(*la++, *ra++) AFTER; while(++oa<of); \
    else if(l->size==1)  do *oa = OP(*la,   *ra++) AFTER; while(++oa<of); \
    else                 do *oa = OP(*la++, *ra  ) AFTER; while(++oa<of); \
  }


#define BINFUNC_F_OPERATOR_LEFT_RIGHT_SET(RT, LT, OP, AFTER)            \
  switch(o->type)                                                       \
    {                                                                   \
    case GAL_TYPE_FLOAT32:                                              \
      BINFUNC_RUN_FUNCTION(float, RT, LT, OP, AFTER);                   \
      break;                                                            \
    case GAL_TYPE_FLOAT64:                                              \
      BINFUNC_RUN_FUNCTION(double, RT, LT, OP, AFTER);                  \
      break;                                                            \
    default:                                                            \
      error(EXIT_FAILURE, 0, "%s: type %d not recognized for o->type ", \
            "BINFUNC_F_OPERATOR_LEFT_RIGHT_SET", o->type);              \
    }


#define BINFUNC_F_OPERATOR_LEFT_SET(LT, OP, AFTER)                      \
  switch(r->type)                                                       \
    {                                                                   \
    case GAL_TYPE_FLOAT32:                                              \
      BINFUNC_F_OPERATOR_LEFT_RIGHT_SET(float, LT, OP, AFTER);          \
      break;                                                            \
    case GAL_TYPE_FLOAT64:                                              \
      BINFUNC_F_OPERATOR_LEFT_RIGHT_SET(double, LT, OP, AFTER);         \
      break;                                                            \
    default:                                                            \
      error(EXIT_FAILURE, 0, "%s: type %d not recognized for r->type",  \
            "BINFUNC_F_OPERATOR_LEFT_SET", r->type);                    \
    }


#define BINFUNC_F_OPERATOR_SET(OP, AFTER)                               \
  switch(l->type)                                                       \
    {                                                                   \
    case GAL_TYPE_FLOAT32:                                              \
      BINFUNC_F_OPERATOR_LEFT_SET(float, OP, AFTER);                    \
      break;                                                            \
    case GAL_TYPE_FLOAT64:                                              \
      BINFUNC_F_OPERATOR_LEFT_SET(double, OP, AFTER);                   \
      break;                                                            \
    default:                                                            \
      error(EXIT_FAILURE, 0, "%s: type %d not recognized for l->type",  \
            "BINFUNC_F_OPERATOR_SET", l->type);                         \
    }


static gal_data_t *
arithmetic_function_binary_flt(int operator, int flags, gal_data_t *il,
                               gal_data_t *ir)
{
  int final_otype;
  size_t out_size, minmapsize;
  gal_data_t *l, *r, *o=NULL;
  int quietmmap=il->quietmmap && ir->quietmmap;


  /* The datasets may be empty. In this case, the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if( il->size==0 || il->array==NULL || ir->size==0 || ir->array==NULL )
    {
      if(il->array==0 || il->array==NULL)
        {   if(flags & GAL_ARITHMETIC_FLAG_FREE) gal_data_free(ir); return il;}
      else {if(flags & GAL_ARITHMETIC_FLAG_FREE) gal_data_free(il); return ir;}
    }


  /* Simple sanity check on the input sizes. */
  if( !( (flags & GAL_ARITHMETIC_FLAG_NUMOK) && (il->size==1 || ir->size==1))
      && gal_dimension_is_different(il, ir) )
    error(EXIT_FAILURE, 0, "%s: the input datasets don't have the same "
          "dimension/size", __func__);


  /* Convert the values to double precision floating point if they are
     integer. */
  l = ( (il->type==GAL_TYPE_FLOAT32 || il->type==GAL_TYPE_FLOAT64)
         ? il : gal_data_copy_to_new_type(il, GAL_TYPE_FLOAT64) );
  r = ( (ir->type==GAL_TYPE_FLOAT32 || ir->type==GAL_TYPE_FLOAT64)
         ? ir : gal_data_copy_to_new_type(ir, GAL_TYPE_FLOAT64) );


  /* Set the output type. */
  final_otype = gal_type_out(l->type, r->type);


  /* Set the output sizes. */
  minmapsize = ( l->minmapsize < r->minmapsize
                 ? l->minmapsize
                 : r->minmapsize );
  out_size = l->size > r->size ? l->size : r->size;


  /* If we want inplace output, set the output pointer to one input. Note
     that the output type can be different from both inputs. */
  if(flags & GAL_ARITHMETIC_FLAG_INPLACE)
    {
      if     (l->type==final_otype && out_size==l->size)   o = l;
      else if(r->type==final_otype && out_size==r->size)   o = r;
    }


  /* If the output pointer was not set for any reason, allocate it. For
     'mmapsize', note that since its 'size_t', it will always be
     Positive. The '-1' that is recommended to give when you want the value
     in RAM is actually the largest possible memory location. So we just
     have to choose the smaller minmapsize of the two to decide if the
     output array should be in RAM or not. */
  if(o==NULL)
    o = gal_data_alloc(NULL, final_otype,
                       l->size>1 ? l->ndim  : r->ndim,
                       l->size>1 ? l->dsize : r->dsize,
                       l->size>1 ? l->wcs : r->wcs, 0, minmapsize,
                       quietmmap, NULL, NULL, NULL);


  /* Start setting the operator and operands. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_POW:
      BINFUNC_F_OPERATOR_SET( pow,   +0 );         break;
    case GAL_ARITHMETIC_OP_ATAN2:
      BINFUNC_F_OPERATOR_SET( atan2, *180.0f/M_PI ); break;
    case GAL_ARITHMETIC_OP_SB_TO_MAG:
      BINFUNC_F_OPERATOR_SET( gal_units_sb_to_mag, +0 ); break;
    case GAL_ARITHMETIC_OP_MAG_TO_SB:
      BINFUNC_F_OPERATOR_SET( gal_units_mag_to_sb, +0 ); break;
    case GAL_ARITHMETIC_OP_COUNTS_TO_MAG:
      BINFUNC_F_OPERATOR_SET( gal_units_counts_to_mag, +0 ); break;
    case GAL_ARITHMETIC_OP_MAG_TO_COUNTS:
      BINFUNC_F_OPERATOR_SET( gal_units_mag_to_counts, +0 ); break;
    case GAL_ARITHMETIC_OP_COUNTS_TO_JY:
      BINFUNC_F_OPERATOR_SET( gal_units_counts_to_jy, +0 ); break;
    case GAL_ARITHMETIC_OP_JY_TO_COUNTS:
      BINFUNC_F_OPERATOR_SET( gal_units_jy_to_counts, +0 ); break;
    case GAL_ARITHMETIC_OP_COUNTS_TO_NANOMAGGY:
      BINFUNC_F_OPERATOR_SET( gal_units_counts_to_nanomaggy, +0 ); break;
    case GAL_ARITHMETIC_OP_NANOMAGGY_TO_COUNTS:
      BINFUNC_F_OPERATOR_SET( gal_units_nanomaggy_to_counts, +0 ); break;
    default:
      error(EXIT_FAILURE, 0, "%s: operator code %d not recognized",
            __func__, operator);
    }


  /* Clean up. Note that if the input arrays can be freed, and any of right
     or left arrays needed conversion, 'BINFUNC_CONVERT_TO_COMPILED_TYPE'
     has already freed the input arrays, and we only have 'r' and 'l'
     allocated in any case. Alternatively, when the inputs shouldn't be
     freed, the only allocated spaces are the 'r' and 'l' arrays if their
     types weren't compiled for binary operations, we can tell this from
     the pointers: if they are different from the original pointers, they
     were allocated. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    {
      /* Clean the main used (temporarily allocated) datasets. */
      if     (o==l)       gal_data_free(r);
      else if(o==r)       gal_data_free(l);
      else              { gal_data_free(l); gal_data_free(r); }

      /* Clean the raw inputs, if they weren't equal to the datasets. */
      if     (o==il) { if(ir!=r) gal_data_free(ir); }
      else if(o==ir) { if(il!=l) gal_data_free(il); }
      else           { if(il!=l) gal_data_free(il);
                       if(ir!=r) gal_data_free(ir); }
    }
  else
    {
      /* Input datasets should be kept, but we don't want the temporary
         datasets, so if they were allocated (they don't equal the input
         pointers, free them). */
      if (l!=il) gal_data_free(l);
      if (r!=ir) gal_data_free(r);
    }

  /* Return */
  return o;
}





/* The list of arguments are:
     d1: Input data (counts or SB).
     d2: Zeropoint.
     d3: Area.      */
static gal_data_t *
arithmetic_counts_to_from_sb(int operator, int flags, gal_data_t *d1,
                             gal_data_t *d2, gal_data_t *d3)
{
  gal_data_t *tmp, *out=NULL;

  switch(operator)
    {
    case GAL_ARITHMETIC_OP_COUNTS_TO_SB:
      tmp=arithmetic_function_binary_flt(GAL_ARITHMETIC_OP_COUNTS_TO_MAG,
                                         flags, d1, d2); /* d2=zeropoint */
      out=arithmetic_function_binary_flt(GAL_ARITHMETIC_OP_MAG_TO_SB,
                                         flags, tmp, d3); /* d3=area */
      break;

    case GAL_ARITHMETIC_OP_SB_TO_COUNTS:
      tmp=arithmetic_function_binary_flt(GAL_ARITHMETIC_OP_SB_TO_MAG,
                                         flags, d1, d3); /* d3-->area */
      out=arithmetic_function_binary_flt(GAL_ARITHMETIC_OP_MAG_TO_COUNTS,
                                         flags, tmp, d2); /* d2=zeropoint */
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
            "to find and fix the problem. The value of '%d' isn't "
            "recognized for the 'operator' variable", __func__,
            PACKAGE_BUGREPORT, operator);
    }

  return out;
}





static gal_data_t *
arithmetic_box_around_ellipse(gal_data_t *a_data, gal_data_t *b_data,
                              gal_data_t *pa_data, int flags)
{
  size_t i;
  double *a=a_data->array, *b=b_data->array, *pa=pa_data->array, out[2];

  /* Loop over all the elements. */
  for(i=0;i<a_data->size;++i)
    {
      /* If the minor axis has a larger value, print a warning and set the
         value to NaN. */
      if(b[i]>a[i])
        {
          if( (flags & GAL_ARITHMETIC_FLAG_QUIET)==0 )
            error(EXIT_SUCCESS, 0, "%s: the minor axis (%g) is larger "
                  "than the major axis (%g), output for this element "
                  "will be NaN", __func__, b[i], a[i]);
          out[0]=out[1]=NAN;
        }
      else gal_box_bound_ellipse_extent(a[i], b[i], pa[i], out);

      /* Write the output in the inputs (we don't need the inputs any
         more). */
      a[i]=out[0];
      b[i]=out[1];
    }

  /* Make the output as a list and return it. */
  b_data->next=a_data;
  return b_data;
}





/* Calculate the vertices of a box from its center and width. The longitude
   coordinate/width is specified with a 'lon' and the latitude
   coordinate/width is specified with a 'lat'. */
static gal_data_t *
arithmetic_box_vertices_on_sphere(gal_data_t *d1, gal_data_t *d2,
                                  gal_data_t *d3, gal_data_t *d4,
                                  int flags)
{
  size_t i;
  double vertices[8];
  double *clon=d1->array, *clat=d2->array;
  double *dlon=d3->array, *dlat=d4->array;

  /* Output datasets. */
  double *blr, *bld, *brr, *brd, *tlr, *tld, *trr, *trd;
  gal_data_t *blrd, *bldd, *brrd, *brdd, *tlrd, *tldd, *trrd, *trdd;

  /* Allocate the extra output datasets (four of them will be the same as
     the input (which will be over-written after usage) and set the array
     pointers. */
  blrd=d1; bldd=d2; brrd=d3; brdd=d4;
  tlrd=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, d1->ndim, d1->dsize, NULL,
                      0, d1->minmapsize, d1->quietmmap, NULL, NULL, NULL);
  tldd=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, d1->ndim, d1->dsize, NULL,
                      0, d1->minmapsize, d1->quietmmap, NULL, NULL, NULL);
  trrd=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, d1->ndim, d1->dsize, NULL,
                      0, d1->minmapsize, d1->quietmmap, NULL, NULL, NULL);
  trdd=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, d1->ndim, d1->dsize, NULL,
                      0, d1->minmapsize, d1->quietmmap, NULL, NULL, NULL);
  blr=blrd->array; bld=bldd->array; brr=brrd->array; brd=brdd->array;
  tlr=tlrd->array; tld=tldd->array; trr=trrd->array; trd=trdd->array;

  /* Loop over the elements. */
  for(i=0;i<d1->size;++i)
    {
      /* Compute the positions of the vertices. */
      gal_wcs_box_vertices_from_center(clon[i], clat[i], dlon[i],
                                       dlat[i], vertices);

      /* Write the results into the the output arrays. */
      blr[i]=vertices[0];     bld[i]=vertices[1];
      brr[i]=vertices[2];     brd[i]=vertices[3];
      tlr[i]=vertices[4];     tld[i]=vertices[5];
      trr[i]=vertices[6];     trd[i]=vertices[7];
    }

  /* Create the output list, note that it has to in the inverse order of
     the final output order. In the end, we want the order of the corners
     to be bottom_left_ra -> bottom_left_dec -> bottom_right_ra ->
     bottom_right_dec -> top_right_ra -> top_right_dec -> top_left_ra ->
     top_left_dec. */
  blrd->next=bldd;
  bldd->next=brrd;
  brrd->next=brdd;
  brdd->next=trrd;
  trrd->next=trdd;
  trdd->next=tlrd;
  tlrd->next=tldd;
  gal_list_data_reverse(&blrd);
  return blrd;
}




static gal_data_t *
arithmetic_box(gal_data_t *d1, gal_data_t *d2, gal_data_t *d3,
               gal_data_t *d4, int operator, int flags)
{
  size_t i;
  gal_data_t *out=NULL;
  gal_data_t *d1d=NULL, *d2d=NULL, *d3d=NULL, *d4d=NULL;

  /* Basic sanity check. */
  if( d1->size != d2->size || d1->size != d3->size
      || (d4 && d1->size!=d4->size) )
    error(EXIT_FAILURE, 0, "%s: the operands to this function "
          "don't have the same number of elements", __func__);

  /* The datasets may be empty. In this case the output should also be
     empty (we can have tables and images with 0 rows or pixels!). */
  if(    d1->size==0 || d1->array==NULL
      || d2->size==0 || d2->array==NULL
      || d3->size==0 || d3->array==NULL
      || (d4 && (d4->size==0 || d4->array==NULL) ) )
    {
      if(flags & GAL_ARITHMETIC_FLAG_FREE)
        { gal_data_free(d2); gal_data_free(d3); gal_data_free(d4); }
      if(d1->array) {free(d1->array); d1->array=NULL;}
      if(d1->dsize) for(i=0;i<d1->ndim;++i) d1->dsize[i]=0;
      d1->size=0; return d1;
    }

 /* Convert the inputs into double. Note that if the user doesn't want to
    free the inputs, we should make a copy of 'a_data' and 'b_data' because
    the output will also be written in them. */
  d1d=( d1->type==GAL_TYPE_FLOAT64
        ? d1
        : gal_data_copy_to_new_type(d1, GAL_TYPE_FLOAT64) );
  d2d=( d2->type==GAL_TYPE_FLOAT64
        ? d2
        : gal_data_copy_to_new_type(d2, GAL_TYPE_FLOAT64) );
  d3d=( d3->type==GAL_TYPE_FLOAT64
        ? d3
        : gal_data_copy_to_new_type(d3, GAL_TYPE_FLOAT64) );
  if(d4)
    d4d=( d4->type==GAL_TYPE_FLOAT64
          ? d4
          : gal_data_copy_to_new_type(d4, GAL_TYPE_FLOAT64) );

  /* Call the appropriate function. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_BOX_AROUND_ELLIPSE:
      out=arithmetic_box_around_ellipse(d1d, d2d, d3d, flags);
      break;
    case GAL_ARITHMETIC_OP_BOX_VERTICES_ON_SPHERE:
      out=arithmetic_box_vertices_on_sphere(d1d, d2d, d3d, d4d, flags);
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
            "fix the problem. The code '%d' is not a recognized "
            "operator for this function", __func__, PACKAGE_BUGREPORT,
            operator);
    }

  /* Clean up. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE)
    {
      if(d1d!=d1) gal_data_free(d1);
      if(d2d!=d2) gal_data_free(d2);
      if(d3d!=d3) gal_data_free(d3);
    }
  if(operator==GAL_ARITHMETIC_OP_BOX_AROUND_ELLIPSE)
    { gal_data_free(d3d); gal_data_free(d4d); }

  /* Return the output. */
  return out;
}





static gal_data_t *
arithmetic_constants_standard(int operator)
{
  size_t one=1;

  /* Allocate the output dataset. */
  gal_data_t *out=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &one,
                                 NULL, 0, -1, 0, NULL, NULL, NULL);

  /* Set the pointer to the value. */
  double *value=out->array;

  /* Write the desired constant (from GSL). */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_E:            /* Base of natural logarithm. */
      value[0]=M_E; break;
    case GAL_ARITHMETIC_OP_PI:    /* Circle circumfernece to diameter. */
      value[0]=M_PI; break;
    case GAL_ARITHMETIC_OP_C:                   /* The speed of light. */
      value[0]=GSL_CONST_MKS_SPEED_OF_LIGHT;
    case GAL_ARITHMETIC_OP_G:           /* The gravitational constant. */
      value[0]=GSL_CONST_MKS_GRAVITATIONAL_CONSTANT;
    case GAL_ARITHMETIC_OP_H:                     /* Plank's constant. */
      value[0]=GSL_CONST_MKS_PLANCKS_CONSTANT_H;
    case GAL_ARITHMETIC_OP_AU:       /* Astronomical Unit (in meters). */
      value[0]=GSL_CONST_MKS_ASTRONOMICAL_UNIT;
    case GAL_ARITHMETIC_OP_LY:             /* Light years (in meters). */
      value[0]=GSL_CONST_MKS_LIGHT_YEAR;
    case GAL_ARITHMETIC_OP_AVOGADRO:             /* Avogadro's number. */
      value[0]=GSL_CONST_NUM_AVOGADRO;
    case GAL_ARITHMETIC_OP_FINESTRUCTURE:  /* Fine-structure constant. */
      value[0]=GSL_CONST_NUM_FINE_STRUCTURE;

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
            "fix the problem. The value '%d' isn't recognized for the "
            "variable 'operator'", __func__, PACKAGE_BUGREPORT, operator);
    }

  /* Return the allocated dataset. */
  return out;
}




















/**********************************************************************/
/****************             New datasets            *****************/
/**********************************************************************/
static gal_data_t *
arithmetic_makenew(gal_data_t *sizes)
{
  gal_data_t *out, *tmp, *ttmp;
  int quietmmap=sizes->quietmmap;
  size_t minmapsize=sizes->minmapsize;
  size_t i, *dsize, ndim=gal_list_data_number(sizes);

  /* Make sure all the elements are a single, integer number. */
  for(tmp=sizes; tmp!=NULL; tmp=tmp->next)
    {
      if(tmp->size!=1)
        error(EXIT_FAILURE, 0, "%s: operands given to 'makenew' operator "
              "should only be a single number. However, at least one of "
              "the input operands has %zu elements", __func__, tmp->size);

      if( tmp->type==GAL_TYPE_FLOAT32 || tmp->type==GAL_TYPE_FLOAT64)
        error(EXIT_FAILURE, 0, "%s: operands given to 'makenew' operator "
              "should have integer types. However, at least one of "
              "the input operands is floating point", __func__);
    }

  /* Fill the 'dsize' array based on the given values. */
  i=ndim-1;
  tmp=sizes;
  dsize=gal_pointer_allocate(GAL_TYPE_SIZE_T, ndim, 1, __func__, "dsize");
  while(tmp!=NULL)
    {
      /* Set the next pointer and conver this one to size_t. */
      ttmp=tmp->next;
      tmp=gal_data_copy_to_new_type_free(tmp, GAL_TYPE_SIZE_T);

      /* Write the dimension's length into 'dsize'. */
      dsize[i--] = ((size_t *)(tmp->array))[0];

      /* Free 'tmp' and re-set it to the next element. */
      free(tmp);
      tmp=ttmp;
    }

  /* allocate the necessary dataset. */
  out=gal_data_alloc(NULL, GAL_TYPE_UINT8, ndim, dsize, NULL, 1,
                     minmapsize, quietmmap, NULL, NULL, NULL);

  /* Clean up and return. */
  free(dsize);
  return out;
}





/* Build a dataset with the index of each element. */
#define ARITHMETIC_FILL_INDEX(IT) {                                     \
    IT i=0, *a=out->array, *af=a+out->size; do *a = i++; while(++a<af); }
#define ARITHMETIC_FILL_COUNTER(IT) {                                   \
    IT i=0, *a=out->array, *af=a+out->size; do *a = ++i; while(++a<af); }
static gal_data_t *
arithmetic_index_counter(gal_data_t *input, int operator, int flags)
{
  uint8_t otype;
  gal_data_t *out;
  size_t isize=input->size;

  /* Find the best type for the output. */
  if(      isize<UINT8_MAX  ) otype=GAL_TYPE_UINT8;
  else if( isize<UINT16_MAX ) otype=GAL_TYPE_UINT16;
  else if( isize<UINT32_MAX ) otype=GAL_TYPE_UINT32;
  else                        otype=GAL_TYPE_UINT64;

  /* Allocate the necessary dataset. */
  out=gal_data_alloc(NULL, otype, input->ndim, input->dsize, NULL,
                     0, input->minmapsize, input->quietmmap, NULL,
                     NULL, NULL);

  /* Do the respective operation. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_INDEX:
    case GAL_ARITHMETIC_OP_INDEXONLY:
      switch(otype)
        {
        case GAL_TYPE_UINT8:  ARITHMETIC_FILL_INDEX( uint8_t  ); break;
        case GAL_TYPE_UINT16: ARITHMETIC_FILL_INDEX( uint16_t ); break;
        case GAL_TYPE_UINT32: ARITHMETIC_FILL_INDEX( uint32_t ); break;
        case GAL_TYPE_UINT64: ARITHMETIC_FILL_INDEX( uint64_t ); break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
                "to find and fix the problem. The type code '%d' isn't "
                "recognized for the 'index' operator", __func__,
                PACKAGE_BUGREPORT, otype);
        }
      break;
    case GAL_ARITHMETIC_OP_COUNTER:
    case GAL_ARITHMETIC_OP_COUNTERONLY:
      switch(otype)
        {
        case GAL_TYPE_UINT8:  ARITHMETIC_FILL_COUNTER( uint8_t  ); break;
        case GAL_TYPE_UINT16: ARITHMETIC_FILL_COUNTER( uint16_t ); break;
        case GAL_TYPE_UINT32: ARITHMETIC_FILL_COUNTER( uint32_t ); break;
        case GAL_TYPE_UINT64: ARITHMETIC_FILL_COUNTER( uint64_t ); break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
                "to find and fix the problem. The type code '%d' isn't "
                "recognized for the 'counter' operator", __func__,
                PACKAGE_BUGREPORT, otype);
        }
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
            "find and fix the problem. The code '%d' isn't recognized "
            "for the 'operator' variable", __func__, PACKAGE_BUGREPORT,
            operator);
    }

  /* Clean up and return. */
  if(flags & GAL_ARITHMETIC_FLAG_FREE) gal_data_free(input);
  return out;
}





static gal_data_t *
arithmetic_constant(gal_data_t *input, gal_data_t *constant, int operator,
                    int flags)
{
  gal_data_t *out;
  size_t i, tsize;

  /* Sanity check. */
  if(constant->size!=1)
    error(EXIT_FAILURE, 0, "%s: the constant operand should only contain "
          "a single element", __func__);

  /* Allocate the necessary dataset. */
  out = ( input->type==constant->type
          ? input
          : gal_data_alloc(NULL, constant->type, input->ndim, input->dsize,
                           NULL, 0, input->minmapsize, input->quietmmap,
                           NULL, NULL, NULL) );

  /* Fill the output with the constant's value. */
  tsize=gal_type_sizeof(out->type);
  for(i=0;i<input->size;++i)
    memcpy(gal_pointer_increment(out->array, i, out->type),
           constant->array, tsize);

  /* Clean up and return. */
  if(out!=input && (flags & GAL_ARITHMETIC_FLAG_FREE))
    gal_data_free(input);
  return out;
}





static gal_data_t *
arithmetic_pool(gal_data_t *input, gal_data_t *psize, gal_data_t *stride,
                int operator, size_t numthreads, int flags)
{
  gal_data_t *out=NULL;
  size_t *pstrarr, *psizearr;


  /* The pool size and the stride must have a single element. */
  if(psize->size!=1)
    error(EXIT_FAILURE, 0, "%s: the pooling operand should only "
          "contain a single element.", __func__);
  if(stride->size!=1)
    error(EXIT_FAILURE, 0, "%s: the stride operand should only "
        "contain a single element.", __func__);

  /* This function is only for a integer operand, so make sure the user
     has given an integer type for the poolsize and the stride. */
  if(psize->type==GAL_TYPE_FLOAT32 || psize->type==GAL_TYPE_FLOAT64)
    error(EXIT_FAILURE, 0, "lengths of pooling along dimensions "
          "must be integer values, not floats. The given length "
          "along dimension %zu is a float.", psize->dsize[0]);
  if(stride->type==GAL_TYPE_FLOAT32 || stride->type==GAL_TYPE_FLOAT64)
    error(EXIT_FAILURE, 0, "the stride of pooling along dimensions "
          "must be integer values, not floats. The given value is a "
          "float.");

  /* Convert the type of the poolsize and the stride into size_t. */
  psize=( psize->type==GAL_TYPE_SIZE_T
        ? psize
        : gal_data_copy_to_new_type_free(psize, GAL_TYPE_SIZE_T) );
  stride=( stride->type==GAL_TYPE_SIZE_T
        ? stride
        : gal_data_copy_to_new_type_free(stride, GAL_TYPE_SIZE_T) );


  psizearr=psize->array;
  pstrarr=stride->array;

  /* Call the separate functions. */
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_POOLMAX:
      out=gal_pool_max(input, psizearr[0], pstrarr[0], numthreads);
      break;
    case GAL_ARITHMETIC_OP_POOLMIN:
      out=gal_pool_min(input, psizearr[0], pstrarr[0], numthreads);
      break;
    case GAL_ARITHMETIC_OP_POOLSUM:
      out=gal_pool_sum(input, psizearr[0], pstrarr[0], numthreads);
      break;
    case GAL_ARITHMETIC_OP_POOLMEAN:
      out=gal_pool_mean(input, psizearr[0], pstrarr[0], numthreads);
      break;
    case GAL_ARITHMETIC_OP_POOLMEDIAN:
      out=gal_pool_median(input, psizearr[0], pstrarr[0], numthreads);
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! please contact us at "
            "'%s' to fix the problem. The value '%d' to "
            "'operator' is not recognized", __func__,
            PACKAGE_BUGREPORT, operator);
    }

  /* Clean up and return. */
  if(out!=input && (flags & GAL_ARITHMETIC_FLAG_FREE))
    gal_data_free(input);
  return out;
}





gal_data_t *
gal_arithmetic_load_col(char *str, int searchin, int ignorecase,
                        size_t minmapsize, int quietmmap)
{
  gal_data_t *out=NULL;
  gal_list_str_t *colid=NULL;
  char *c, *cf, *copy, *hdu=NULL, *filename=NULL;
  size_t numthreads=1; /* We only want to read a single column! */

  /* This is the shortest possible string (with each component being given
     a one character value). Recall that in C, simply putting literal
     strings after each other, will merge them together into one literal
     string. */
  char *checkstr = ( GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX   "a"
                     GAL_ARITHMETIC_OPSTR_LOADCOL_FILE "a" );

  /* This function is called on every call of Arithmetic, so before going
     into any further tests, first make sure the string is long enough, and
     that it starts with the fixed format string 'FMTCOL'. */
  if( strlen(str)<strlen(checkstr)
      || strncmp(str, GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX,
                 GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX_LEN) ) return NULL;

  /* To separate the components, we need to put '\0's within the input
     string. But we don't want to ruin the input string (in case it isn't
     for this purpose), so for the rest of the steps we'll make a copy of
     the input string and work on that. */
  gal_checkset_allocate_copy(str, &copy);

  /* Parse the string and separate the components. */
  gal_list_str_add(&colid,
                   &copy[GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX_LEN], 0);
  cf=copy+strlen(str);
  c=colid->v+1;     /* colID has at least one character long, so we'll */
  while(c<cf)           /* start the parsing from the next character */
    {
      /* If we are on the file-name component, then we can set the end of
         the column ID component, and set the start of the HDU. But this is
         only valid if 'hdu' hasn't already been set. */
      if( !strncmp(c, GAL_ARITHMETIC_OPSTR_LOADCOL_FILE,
                   GAL_ARITHMETIC_OPSTR_LOADCOL_FILE_LEN ) )
        {
          /* If 'filename' or 'hdu' have already been set, then this
             doesn't conform to the format, and we should leave this
             function. */
          if(filename || hdu) { free(copy); return NULL; };

          /* Set the current position to '\0' (to end the column name). */
          *c='\0';

          /* Set the HDU's starting pointer. */
          filename=c+GAL_ARITHMETIC_OPSTR_LOADCOL_FILE_LEN;
          c=filename+1; /* Similar to 'colid->v' above. */
        }

      /* If we are on a HDU component, steps are very similar to the
         filename steps above. */
      else if( !strncmp(c, GAL_ARITHMETIC_OPSTR_LOADCOL_HDU,
                        GAL_ARITHMETIC_OPSTR_LOADCOL_HDU_LEN) )
        {
          if(hdu) { free(copy); return NULL; }
          *c='\0'; hdu=c+GAL_ARITHMETIC_OPSTR_LOADCOL_HDU_LEN;
          c=hdu+1;
        }

      /* If there was no match with HDU or file strings, then simply
         increment the pointer. */
      else ++c;
    }

  /* If a file-name couldn't be identified, then return NULL. */
  if(filename==NULL) { free(copy); return NULL; }

  /* If a HDU wasn't given and the file is a FITS file, print a warning and
     use the default "1". */
  if(hdu==NULL && gal_fits_name_is_fits(filename))
    error(EXIT_FAILURE, 0, "WARNING: '%s' is a FITS file, but no HDU "
          "has been given (recall that a FITS file can contain "
          "multiple HDUs). Please add a '-hdu-AAA' suffix to your "
          "'"GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX"' operator to specify "
          "the HDU (where 'AAA' is your HDU name or counter, counting "
          "from zero); like this: '"GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX
          "%s"GAL_ARITHMETIC_OPSTR_LOADCOL_FILE"%s"
          GAL_ARITHMETIC_OPSTR_LOADCOL_HDU"AAA'",
          filename, colid->v, filename);

  /* Read the column from the table. */
  out=gal_table_read(filename, hdu, NULL, colid, searchin, ignorecase,
                     numthreads, minmapsize, quietmmap, NULL,
                     "WITHIN-LOAD-COL");

  /* Make sure that only a single column matched. */
  if(out->next)
    error(EXIT_FAILURE, 0, "%s: '%s' matches more than one column! "
          "To load columns during arithmetic, it is important that "
          "'load-col' returns only a single column", colid->v,
          gal_fits_name_save_as_string(filename, hdu));

  /* Clean up and return. */
  gal_list_str_free(colid, 0);
  free(copy);
  return out;
}




















/**********************************************************************/
/****************         High-level functions        *****************/
/**********************************************************************/
/* Order is the same as in the manual. */
int
gal_arithmetic_set_operator(char *string, size_t *num_operands)
{
  int op;

  /* Simple arithmetic operators. */
  if      (!strcmp(string, "+" ))
    { op=GAL_ARITHMETIC_OP_PLUS;              *num_operands=2;  }
  else if (!strcmp(string, "-" ))
    { op=GAL_ARITHMETIC_OP_MINUS;             *num_operands=2;  }
  else if (!strcmp(string, "x" ))
    { op=GAL_ARITHMETIC_OP_MULTIPLY;          *num_operands=2;  }
  else if (!strcmp(string, "/" ))
    { op=GAL_ARITHMETIC_OP_DIVIDE;            *num_operands=2;  }
  else if (!strcmp(string, "%" ))
    { op=GAL_ARITHMETIC_OP_MODULO;            *num_operands=2;  }

  /* Mathematical Operators. */
  else if (!strcmp(string, "abs"))
    { op=GAL_ARITHMETIC_OP_ABS;               *num_operands=1;  }
  else if (!strcmp(string, "pow"))
    { op=GAL_ARITHMETIC_OP_POW;               *num_operands=2;  }
  else if (!strcmp(string, "sqrt"))
    { op=GAL_ARITHMETIC_OP_SQRT;              *num_operands=1;  }
  else if (!strcmp(string, "log"))
    { op=GAL_ARITHMETIC_OP_LOG;               *num_operands=1;  }
  else if (!strcmp(string, "log10"))
    { op=GAL_ARITHMETIC_OP_LOG10;             *num_operands=1;  }

  /* Trigonometric functions. */
  else if( !strcmp(string, "sin"))
    { op=GAL_ARITHMETIC_OP_SIN;               *num_operands=1; }
  else if( !strcmp(string, "cos"))
    { op=GAL_ARITHMETIC_OP_COS;               *num_operands=1; }
  else if( !strcmp(string, "tan"))
    { op=GAL_ARITHMETIC_OP_TAN;               *num_operands=1; }
  else if( !strcmp(string, "asin"))
    { op=GAL_ARITHMETIC_OP_ASIN;              *num_operands=1; }
  else if( !strcmp(string, "acos"))
    { op=GAL_ARITHMETIC_OP_ACOS;              *num_operands=1; }
  else if( !strcmp(string, "atan"))
    { op=GAL_ARITHMETIC_OP_ATAN;              *num_operands=1; }
  else if( !strcmp(string, "atan2"))
    { op=GAL_ARITHMETIC_OP_ATAN2;             *num_operands=2; }
  else if( !strcmp(string, "sinh"))
    { op=GAL_ARITHMETIC_OP_SINH;              *num_operands=1; }
  else if( !strcmp(string, "cosh"))
    { op=GAL_ARITHMETIC_OP_COSH;              *num_operands=1; }
  else if( !strcmp(string, "tanh"))
    { op=GAL_ARITHMETIC_OP_TANH;              *num_operands=1; }
  else if( !strcmp(string, "asinh"))
    { op=GAL_ARITHMETIC_OP_ASINH;             *num_operands=1; }
  else if( !strcmp(string, "acosh"))
    { op=GAL_ARITHMETIC_OP_ACOSH;             *num_operands=1; }
  else if( !strcmp(string, "atanh"))
    { op=GAL_ARITHMETIC_OP_ATANH;             *num_operands=1; }

  /* Units conversion functions. */
  else if (!strcmp(string, "ra-to-degree"))
    { op=GAL_ARITHMETIC_OP_RA_TO_DEGREE;      *num_operands=1;  }
  else if (!strcmp(string, "dec-to-degree"))
    { op=GAL_ARITHMETIC_OP_DEC_TO_DEGREE;     *num_operands=1;  }
  else if (!strcmp(string, "degree-to-ra"))
    { op=GAL_ARITHMETIC_OP_DEGREE_TO_RA;      *num_operands=1;  }
  else if (!strcmp(string, "degree-to-dec"))
    { op=GAL_ARITHMETIC_OP_DEGREE_TO_DEC;     *num_operands=1;  }
  else if (!strcmp(string, "counts-to-mag"))
    { op=GAL_ARITHMETIC_OP_COUNTS_TO_MAG;     *num_operands=2;  }
  else if (!strcmp(string, "mag-to-counts"))
    { op=GAL_ARITHMETIC_OP_MAG_TO_COUNTS;     *num_operands=2;  }
  else if (!strcmp(string, "sb-to-mag"))
    { op=GAL_ARITHMETIC_OP_SB_TO_MAG;         *num_operands=2;  }
  else if (!strcmp(string, "mag-to-sb"))
    { op=GAL_ARITHMETIC_OP_MAG_TO_SB;         *num_operands=2;  }
  else if (!strcmp(string, "counts-to-sb"))
    { op=GAL_ARITHMETIC_OP_COUNTS_TO_SB;      *num_operands=3;  }
  else if (!strcmp(string, "sb-to-counts"))
    { op=GAL_ARITHMETIC_OP_SB_TO_COUNTS;      *num_operands=3;  }
  else if (!strcmp(string, "counts-to-jy"))
    { op=GAL_ARITHMETIC_OP_COUNTS_TO_JY;      *num_operands=2;  }
  else if (!strcmp(string, "jy-to-counts"))
    { op=GAL_ARITHMETIC_OP_JY_TO_COUNTS;      *num_operands=2;  }
  else if (!strcmp(string, "counts-to-nanomaggy"))
    { op=GAL_ARITHMETIC_OP_COUNTS_TO_NANOMAGGY; *num_operands=2;}
  else if (!strcmp(string, "nanomaggy-to-counts"))
    { op=GAL_ARITHMETIC_OP_NANOMAGGY_TO_COUNTS; *num_operands=2;}
  else if (!strcmp(string, "mag-to-jy"))
    { op=GAL_ARITHMETIC_OP_MAG_TO_JY;         *num_operands=1;  }
  else if (!strcmp(string, "jy-to-mag"))
    { op=GAL_ARITHMETIC_OP_JY_TO_MAG;         *num_operands=1;  }
  else if( !strcmp(string, "au-to-pc"))
    { op=GAL_ARITHMETIC_OP_AU_TO_PC;          *num_operands=1;  }
  else if( !strcmp(string, "pc-to-au"))
    { op=GAL_ARITHMETIC_OP_PC_TO_AU;          *num_operands=1;  }
  else if( !strcmp(string, "ly-to-pc"))
    { op=GAL_ARITHMETIC_OP_LY_TO_PC;          *num_operands=1;  }
  else if( !strcmp(string, "pc-to-ly"))
    { op=GAL_ARITHMETIC_OP_PC_TO_LY;          *num_operands=1;  }
  else if( !strcmp(string, "ly-to-au"))
    { op=GAL_ARITHMETIC_OP_LY_TO_AU;          *num_operands=1;  }
  else if( !strcmp(string, "au-to-ly"))
    { op=GAL_ARITHMETIC_OP_AU_TO_LY;          *num_operands=1;  }

  /* Statistical/higher-level operators. */
  else if (!strcmp(string, "minvalue"))
    { op=GAL_ARITHMETIC_OP_MINVAL;            *num_operands=1;  }
  else if (!strcmp(string, "maxvalue"))
    { op=GAL_ARITHMETIC_OP_MAXVAL;            *num_operands=1;  }
  else if (!strcmp(string, "numbervalue"))
    { op=GAL_ARITHMETIC_OP_NUMBERVAL;         *num_operands=1;  }
  else if (!strcmp(string, "sumvalue"))
    { op=GAL_ARITHMETIC_OP_SUMVAL;            *num_operands=1;  }
  else if (!strcmp(string, "meanvalue"))
    { op=GAL_ARITHMETIC_OP_MEANVAL;           *num_operands=1;  }
  else if (!strcmp(string, "stdvalue"))
    { op=GAL_ARITHMETIC_OP_STDVAL;            *num_operands=1;  }
  else if (!strcmp(string, "medianvalue"))
    { op=GAL_ARITHMETIC_OP_MEDIANVAL;         *num_operands=1;  }
  else if (!strcmp(string, "min"))
    { op=GAL_ARITHMETIC_OP_MIN;               *num_operands=-1; }
  else if (!strcmp(string, "max"))
    { op=GAL_ARITHMETIC_OP_MAX;               *num_operands=-1; }
  else if (!strcmp(string, "number"))
    { op=GAL_ARITHMETIC_OP_NUMBER;            *num_operands=-1; }
  else if (!strcmp(string, "sum"))
    { op=GAL_ARITHMETIC_OP_SUM;               *num_operands=-1; }
  else if (!strcmp(string, "mean"))
    { op=GAL_ARITHMETIC_OP_MEAN;              *num_operands=-1; }
  else if (!strcmp(string, "std"))
    { op=GAL_ARITHMETIC_OP_STD;               *num_operands=-1; }
  else if (!strcmp(string, "median"))
    { op=GAL_ARITHMETIC_OP_MEDIAN;            *num_operands=-1; }
  else if (!strcmp(string, "quantile"))
    { op=GAL_ARITHMETIC_OP_QUANTILE;          *num_operands=-1; }
  else if (!strcmp(string, "sigclip-number"))
    { op=GAL_ARITHMETIC_OP_SIGCLIP_NUMBER;    *num_operands=-1; }
  else if (!strcmp(string, "sigclip-mean"))
    { op=GAL_ARITHMETIC_OP_SIGCLIP_MEAN;      *num_operands=-1; }
  else if (!strcmp(string, "sigclip-median"))
    { op=GAL_ARITHMETIC_OP_SIGCLIP_MEDIAN;    *num_operands=-1; }
  else if (!strcmp(string, "sigclip-std"))
    { op=GAL_ARITHMETIC_OP_SIGCLIP_STD;       *num_operands=-1; }

  /* To one-dimension (only based on values). */
  else if (!strcmp(string, "unique"))
    { op=GAL_ARITHMETIC_OP_UNIQUE;            *num_operands=1;  }
  else if (!strcmp(string, "noblank"))
    { op=GAL_ARITHMETIC_OP_NOBLANK;           *num_operands=1;  }

  /* Adding noise operators. */
  else if (!strcmp(string, "mknoise-sigma"))
    { op=GAL_ARITHMETIC_OP_MKNOISE_SIGMA;     *num_operands=2; }
  else if (!strcmp(string, "mknoise-sigma-from-mean"))
    { op=GAL_ARITHMETIC_OP_MKNOISE_SIGMA_FROM_MEAN; *num_operands=2; }
  else if (!strcmp(string, "mknoise-poisson"))
    { op=GAL_ARITHMETIC_OP_MKNOISE_POISSON;   *num_operands=2; }
  else if (!strcmp(string, "mknoise-uniform"))
    { op=GAL_ARITHMETIC_OP_MKNOISE_UNIFORM;   *num_operands=2; }
  else if (!strcmp(string, "random-from-hist"))
    { op=GAL_ARITHMETIC_OP_RANDOM_FROM_HIST;  *num_operands=3; }
  else if (!strcmp(string, "random-from-hist-raw"))
    { op=GAL_ARITHMETIC_OP_RANDOM_FROM_HIST_RAW; *num_operands=3; }

  /* Dimensionality changing */
  else if (!strcmp(string, "to-1d"))
    { op=GAL_ARITHMETIC_OP_TO1D;              *num_operands=1;  }
  else if (!strcmp(string, "stitch"))
    { op=GAL_ARITHMETIC_OP_STITCH;            *num_operands=-1; }
  else if (!strcmp(string, "trim"))
    { op=GAL_ARITHMETIC_OP_TRIM;              *num_operands=1; }

  /* Conditional operators. */
  else if (!strcmp(string, "lt" ))
    { op=GAL_ARITHMETIC_OP_LT;                *num_operands=2;  }
  else if (!strcmp(string, "le"))
    { op=GAL_ARITHMETIC_OP_LE;                *num_operands=2;  }
  else if (!strcmp(string, "gt" ))
    { op=GAL_ARITHMETIC_OP_GT;                *num_operands=2;  }
  else if (!strcmp(string, "ge"))
    { op=GAL_ARITHMETIC_OP_GE;                *num_operands=2;  }
  else if (!strcmp(string, "eq"))
    { op=GAL_ARITHMETIC_OP_EQ;                *num_operands=2;  }
  else if (!strcmp(string, "ne"))
    { op=GAL_ARITHMETIC_OP_NE;                *num_operands=2;  }
  else if (!strcmp(string, "and"))
    { op=GAL_ARITHMETIC_OP_AND;               *num_operands=2;  }
  else if (!strcmp(string, "or"))
    { op=GAL_ARITHMETIC_OP_OR;                *num_operands=2;  }
  else if (!strcmp(string, "not"))
    { op=GAL_ARITHMETIC_OP_NOT;               *num_operands=1;  }
  else if (!strcmp(string, "isblank"))
    { op=GAL_ARITHMETIC_OP_ISBLANK;           *num_operands=1;  }
  else if (!strcmp(string, "isnotblank"))
    { op=GAL_ARITHMETIC_OP_ISNOTBLANK;        *num_operands=1;  }
  else if (!strcmp(string, "where"))
    { op=GAL_ARITHMETIC_OP_WHERE;             *num_operands=3;  }

  /* Bitwise operators. */
  else if (!strcmp(string, "bitand"))
    { op=GAL_ARITHMETIC_OP_BITAND;            *num_operands=2;  }
  else if (!strcmp(string, "bitor"))
    { op=GAL_ARITHMETIC_OP_BITOR;             *num_operands=2;  }
  else if (!strcmp(string, "bitxor"))
    { op=GAL_ARITHMETIC_OP_BITXOR;            *num_operands=2;  }
  else if (!strcmp(string, "lshift"))
    { op=GAL_ARITHMETIC_OP_BITLSH;            *num_operands=2;  }
  else if (!strcmp(string, "rshift"))
    { op=GAL_ARITHMETIC_OP_BITRSH;            *num_operands=2;  }
  else if (!strcmp(string, "bitnot"))
    { op=GAL_ARITHMETIC_OP_BITNOT;            *num_operands=1;  }

  /* Type conversion. */
  else if (!strcmp(string, "uint8")   || !strcmp(string, "u8"))
    { op=GAL_ARITHMETIC_OP_TO_UINT8;          *num_operands=1;  }
  else if (!strcmp(string, "int8")    || !strcmp(string, "i8"))
    { op=GAL_ARITHMETIC_OP_TO_INT8;           *num_operands=1;  }
  else if (!strcmp(string, "uint16")  || !strcmp(string, "u16"))
    { op=GAL_ARITHMETIC_OP_TO_UINT16;         *num_operands=1;  }
  else if (!strcmp(string, "int16")   || !strcmp(string, "i16"))
    { op=GAL_ARITHMETIC_OP_TO_INT16;          *num_operands=1;  }
  else if (!strcmp(string, "uint32")  || !strcmp(string, "u32"))
    { op=GAL_ARITHMETIC_OP_TO_UINT32;         *num_operands=1;  }
  else if (!strcmp(string, "int32")   || !strcmp(string, "i32"))
    { op=GAL_ARITHMETIC_OP_TO_INT32;          *num_operands=1;  }
  else if (!strcmp(string, "uint64")  || !strcmp(string, "u64"))
    { op=GAL_ARITHMETIC_OP_TO_UINT64;         *num_operands=1;  }
  else if (!strcmp(string, "int64")   || !strcmp(string, "i64"))
    { op=GAL_ARITHMETIC_OP_TO_INT64;          *num_operands=1;  }
  else if (!strcmp(string, "float32") || !strcmp(string, "f32"))
    { op=GAL_ARITHMETIC_OP_TO_FLOAT32;        *num_operands=1;  }
  else if (!strcmp(string, "float64") || !strcmp(string, "f64"))
    { op=GAL_ARITHMETIC_OP_TO_FLOAT64;        *num_operands=1;  }

  /* Constants. */
  else if (!strcmp(string, "e"))
    { op=GAL_ARITHMETIC_OP_E;                 *num_operands=0;  }
  else if (!strcmp(string, "pi"))
    { op=GAL_ARITHMETIC_OP_PI;                *num_operands=0;  }
  else if (!strcmp(string, "c"))
    { op=GAL_ARITHMETIC_OP_C;                 *num_operands=0;  }
  else if (!strcmp(string, "G"))
    { op=GAL_ARITHMETIC_OP_G;                 *num_operands=0;  }
  else if (!strcmp(string, "h"))
    { op=GAL_ARITHMETIC_OP_H;                 *num_operands=0;  }
  else if (!strcmp(string, "AU"))
    { op=GAL_ARITHMETIC_OP_AU;                *num_operands=0;  }
  else if (!strcmp(string, "ly"))
    { op=GAL_ARITHMETIC_OP_LY;                *num_operands=0;  }
  else if (!strcmp(string, "avogadro"))
    { op=GAL_ARITHMETIC_OP_AVOGADRO;          *num_operands=0;  }
  else if (!strcmp(string, "fine-structure"))
    { op=GAL_ARITHMETIC_OP_FINESTRUCTURE;     *num_operands=0;  }

  /* Surrounding box. */
  else if (!strcmp(string, "box-around-ellipse"))
    { op=GAL_ARITHMETIC_OP_BOX_AROUND_ELLIPSE;*num_operands=3;  }
  else if (!strcmp(string, "box-vertices-on-sphere"))
    { op=GAL_ARITHMETIC_OP_BOX_VERTICES_ON_SPHERE; *num_operands=4; }

  /* Size and position operators. */
  else if (!strcmp(string, "size"))
    { op=GAL_ARITHMETIC_OP_SIZE;              *num_operands=2;  }
  else if (!strcmp(string, "makenew"))
    { op=GAL_ARITHMETIC_OP_MAKENEW;           *num_operands=-1; }
  else if (!strcmp(string, "index"))
    { op=GAL_ARITHMETIC_OP_INDEX;             *num_operands=1;  }
   else if (!strcmp(string, "counter"))
    { op=GAL_ARITHMETIC_OP_COUNTER;           *num_operands=1;  }
  else if (!strcmp(string, "indexonly"))
    { op=GAL_ARITHMETIC_OP_INDEXONLY;         *num_operands=1;  }
  else if (!strcmp(string, "counteronly"))
    { op=GAL_ARITHMETIC_OP_COUNTERONLY;       *num_operands=1;  }
  else if (!strcmp(string, "swap"))
    { op=GAL_ARITHMETIC_OP_SWAP;              *num_operands=2;  }
  else if (!strcmp(string, "constant"))
    { op=GAL_ARITHMETIC_OP_CONSTANT;          *num_operands=2;  }

  /* Pooling operators. */
  else if (!strcmp(string, "pool-max"))
    { op=GAL_ARITHMETIC_OP_POOLMAX;           *num_operands=3;  }
  else if (!strcmp(string, "pool-min"))
    { op=GAL_ARITHMETIC_OP_POOLMIN;           *num_operands=3;  }
  else if (!strcmp(string, "pool-sum"))
    { op=GAL_ARITHMETIC_OP_POOLSUM;           *num_operands=3;  }
  else if (!strcmp(string, "pool-mean"))
    { op=GAL_ARITHMETIC_OP_POOLMEAN;          *num_operands=3;  }
  else if (!strcmp(string, "pool-median"))
    { op=GAL_ARITHMETIC_OP_POOLMEDIAN;        *num_operands=3;  }

  /* Operator not defined. */
  else
    { op=GAL_ARITHMETIC_OP_INVALID; *num_operands=GAL_BLANK_INT; }
  return op;
}





char *
gal_arithmetic_operator_string(int operator)
{
  switch(operator)
    {
    case GAL_ARITHMETIC_OP_PLUS:            return "+";
    case GAL_ARITHMETIC_OP_MINUS:           return "-";
    case GAL_ARITHMETIC_OP_MULTIPLY:        return "x";
    case GAL_ARITHMETIC_OP_DIVIDE:          return "/";
    case GAL_ARITHMETIC_OP_MODULO:          return "%";

    case GAL_ARITHMETIC_OP_LT:              return "lt";
    case GAL_ARITHMETIC_OP_LE:              return "le";
    case GAL_ARITHMETIC_OP_GT:              return "gt";
    case GAL_ARITHMETIC_OP_GE:              return "ge";
    case GAL_ARITHMETIC_OP_EQ:              return "eq";
    case GAL_ARITHMETIC_OP_NE:              return "ne";
    case GAL_ARITHMETIC_OP_AND:             return "and";
    case GAL_ARITHMETIC_OP_OR:              return "or";
    case GAL_ARITHMETIC_OP_NOT:             return "not";
    case GAL_ARITHMETIC_OP_ISBLANK:         return "isblank";
    case GAL_ARITHMETIC_OP_ISNOTBLANK:      return "isnotblank";
    case GAL_ARITHMETIC_OP_WHERE:           return "where";

    case GAL_ARITHMETIC_OP_BITAND:          return "bitand";
    case GAL_ARITHMETIC_OP_BITOR:           return "bitor";
    case GAL_ARITHMETIC_OP_BITXOR:          return "bitxor";
    case GAL_ARITHMETIC_OP_BITLSH:          return "lshift";
    case GAL_ARITHMETIC_OP_BITRSH:          return "rshift";
    case GAL_ARITHMETIC_OP_BITNOT:          return "bitnot";

    case GAL_ARITHMETIC_OP_ABS:             return "abs";
    case GAL_ARITHMETIC_OP_POW:             return "pow";
    case GAL_ARITHMETIC_OP_SQRT:            return "sqrt";
    case GAL_ARITHMETIC_OP_LOG:             return "log";
    case GAL_ARITHMETIC_OP_LOG10:           return "log10";

    case GAL_ARITHMETIC_OP_SIN:             return "sin";
    case GAL_ARITHMETIC_OP_COS:             return "cos";
    case GAL_ARITHMETIC_OP_TAN:             return "tan";
    case GAL_ARITHMETIC_OP_ASIN:            return "asin";
    case GAL_ARITHMETIC_OP_ACOS:            return "acos";
    case GAL_ARITHMETIC_OP_ATAN:            return "atan";
    case GAL_ARITHMETIC_OP_SINH:            return "sinh";
    case GAL_ARITHMETIC_OP_COSH:            return "cosh";
    case GAL_ARITHMETIC_OP_TANH:            return "tanh";
    case GAL_ARITHMETIC_OP_ASINH:           return "asinh";
    case GAL_ARITHMETIC_OP_ACOSH:           return "acosh";
    case GAL_ARITHMETIC_OP_ATANH:           return "atanh";
    case GAL_ARITHMETIC_OP_ATAN2:           return "atan2";

    case GAL_ARITHMETIC_OP_RA_TO_DEGREE:    return "ra-to-degree";
    case GAL_ARITHMETIC_OP_DEC_TO_DEGREE:   return "dec-to-degree";
    case GAL_ARITHMETIC_OP_DEGREE_TO_RA:    return "degree-to-ra";
    case GAL_ARITHMETIC_OP_DEGREE_TO_DEC:   return "degree-to-dec";
    case GAL_ARITHMETIC_OP_COUNTS_TO_MAG:   return "counts-to-mag";
    case GAL_ARITHMETIC_OP_MAG_TO_COUNTS:   return "mag-to-counts";
    case GAL_ARITHMETIC_OP_SB_TO_MAG:       return "sb-to-mag";
    case GAL_ARITHMETIC_OP_MAG_TO_SB:       return "mag-to-sb";
    case GAL_ARITHMETIC_OP_COUNTS_TO_SB:    return "counts-to-sb";
    case GAL_ARITHMETIC_OP_SB_TO_COUNTS:    return "sb-to-counts";
    case GAL_ARITHMETIC_OP_COUNTS_TO_JY:    return "counts-to-jy";
    case GAL_ARITHMETIC_OP_JY_TO_COUNTS:    return "jy-to-counts";
    case GAL_ARITHMETIC_OP_COUNTS_TO_NANOMAGGY: return "counts-to-nanomaggy";
    case GAL_ARITHMETIC_OP_NANOMAGGY_TO_COUNTS: return "nanomaggy-to-counts";
    case GAL_ARITHMETIC_OP_MAG_TO_JY:       return "mag-to-jy";
    case GAL_ARITHMETIC_OP_JY_TO_MAG:       return "jy-to-mag";
    case GAL_ARITHMETIC_OP_AU_TO_PC:        return "au-to-pc";
    case GAL_ARITHMETIC_OP_PC_TO_AU:        return "pc-to-au";
    case GAL_ARITHMETIC_OP_LY_TO_PC:        return "ly-to-pc";
    case GAL_ARITHMETIC_OP_PC_TO_LY:        return "pc-to-ly";
    case GAL_ARITHMETIC_OP_LY_TO_AU:        return "ly-to-au";
    case GAL_ARITHMETIC_OP_AU_TO_LY:        return "au-to-ly";

    case GAL_ARITHMETIC_OP_MINVAL:          return "minvalue";
    case GAL_ARITHMETIC_OP_MAXVAL:          return "maxvalue";
    case GAL_ARITHMETIC_OP_NUMBERVAL:       return "numbervalue";
    case GAL_ARITHMETIC_OP_SUMVAL:          return "sumvalue";
    case GAL_ARITHMETIC_OP_MEANVAL:         return "meanvalue";
    case GAL_ARITHMETIC_OP_STDVAL:          return "stdvalue";
    case GAL_ARITHMETIC_OP_MEDIANVAL:       return "medianvalue";

    case GAL_ARITHMETIC_OP_UNIQUE:          return "unique";
    case GAL_ARITHMETIC_OP_NOBLANK:         return "noblank";

    case GAL_ARITHMETIC_OP_MIN:             return "min";
    case GAL_ARITHMETIC_OP_MAX:             return "max";
    case GAL_ARITHMETIC_OP_NUMBER:          return "number";
    case GAL_ARITHMETIC_OP_SUM:             return "sum";
    case GAL_ARITHMETIC_OP_MEAN:            return "mean";
    case GAL_ARITHMETIC_OP_STD:             return "std";
    case GAL_ARITHMETIC_OP_MEDIAN:          return "median";
    case GAL_ARITHMETIC_OP_QUANTILE:        return "quantile";
    case GAL_ARITHMETIC_OP_SIGCLIP_NUMBER:  return "sigclip-number";
    case GAL_ARITHMETIC_OP_SIGCLIP_MEDIAN:  return "sigclip-median";
    case GAL_ARITHMETIC_OP_SIGCLIP_MEAN:    return "sigclip-mean";
    case GAL_ARITHMETIC_OP_SIGCLIP_STD:     return "sigclip-number";

    case GAL_ARITHMETIC_OP_MKNOISE_SIGMA:   return "mknoise-sigma";
    case GAL_ARITHMETIC_OP_MKNOISE_SIGMA_FROM_MEAN: return "mknoise-sigma-from-mean";
    case GAL_ARITHMETIC_OP_MKNOISE_POISSON: return "mknoise-poisson";
    case GAL_ARITHMETIC_OP_MKNOISE_UNIFORM: return "mknoise-uniform";
    case GAL_ARITHMETIC_OP_RANDOM_FROM_HIST:return "random-from-hist";
    case GAL_ARITHMETIC_OP_RANDOM_FROM_HIST_RAW:return "random-from-hist-raw";

    case GAL_ARITHMETIC_OP_STITCH:          return "stitch";

    case GAL_ARITHMETIC_OP_TO_UINT8:        return "uchar";
    case GAL_ARITHMETIC_OP_TO_INT8:         return "char";
    case GAL_ARITHMETIC_OP_TO_UINT16:       return "ushort";
    case GAL_ARITHMETIC_OP_TO_INT16:        return "short";
    case GAL_ARITHMETIC_OP_TO_UINT32:       return "uint";
    case GAL_ARITHMETIC_OP_TO_INT32:        return "int";
    case GAL_ARITHMETIC_OP_TO_UINT64:       return "ulong";
    case GAL_ARITHMETIC_OP_TO_INT64:        return "long";
    case GAL_ARITHMETIC_OP_TO_FLOAT32:      return "float32";
    case GAL_ARITHMETIC_OP_TO_FLOAT64:      return "float64";

    case GAL_ARITHMETIC_OP_E:               return "e";
    case GAL_ARITHMETIC_OP_PI:              return "pi";
    case GAL_ARITHMETIC_OP_C:               return "c";
    case GAL_ARITHMETIC_OP_G:               return "G";
    case GAL_ARITHMETIC_OP_H:               return "h";
    case GAL_ARITHMETIC_OP_AU:              return "au";
    case GAL_ARITHMETIC_OP_LY:              return "ly";
    case GAL_ARITHMETIC_OP_AVOGADRO:        return "avogadro";
    case GAL_ARITHMETIC_OP_FINESTRUCTURE:   return "fine-structure";

    case GAL_ARITHMETIC_OP_BOX_AROUND_ELLIPSE: return "box-around-ellipse";
    case GAL_ARITHMETIC_OP_BOX_VERTICES_ON_SPHERE: return "vertices-on-sphere";

    case GAL_ARITHMETIC_OP_SIZE:            return "size";
    case GAL_ARITHMETIC_OP_MAKENEW:         return "makenew";
    case GAL_ARITHMETIC_OP_INDEX:           return "index";
    case GAL_ARITHMETIC_OP_COUNTER:         return "counter";
    case GAL_ARITHMETIC_OP_INDEXONLY:       return "indexonly";
    case GAL_ARITHMETIC_OP_COUNTERONLY:     return "counteronly";
    case GAL_ARITHMETIC_OP_SWAP:            return "swap";
    case GAL_ARITHMETIC_OP_CONSTANT:        return "constant";

    case GAL_ARITHMETIC_OP_POOLMAX:         return "pool-max";
    case GAL_ARITHMETIC_OP_POOLMIN:         return "pool-min";
    case GAL_ARITHMETIC_OP_POOLSUM:         return "pool-sum";
    case GAL_ARITHMETIC_OP_POOLMEAN:        return "pool-mean";
    case GAL_ARITHMETIC_OP_POOLMEDIAN:      return "pool-median";

    default:                                return NULL;
    }
  return NULL;
}





gal_data_t *
gal_arithmetic(int operator, size_t numthreads, int flags, ...)
{
  va_list va;
  gal_data_t *d1, *d2, *d3, *d4, *out=NULL;

  /* Prepare the variable arguments (starting after the flags argument). */
  va_start(va, flags);

  /* Depending on the operator, do the job: */
  switch(operator)
    {
    /* Binary operators with any data type. */
    case GAL_ARITHMETIC_OP_PLUS:
    case GAL_ARITHMETIC_OP_MINUS:
    case GAL_ARITHMETIC_OP_MULTIPLY:
    case GAL_ARITHMETIC_OP_DIVIDE:
    case GAL_ARITHMETIC_OP_LT:
    case GAL_ARITHMETIC_OP_LE:
    case GAL_ARITHMETIC_OP_GT:
    case GAL_ARITHMETIC_OP_GE:
    case GAL_ARITHMETIC_OP_EQ:
    case GAL_ARITHMETIC_OP_NE:
    case GAL_ARITHMETIC_OP_AND:
    case GAL_ARITHMETIC_OP_OR:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=arithmetic_binary(operator, flags, d1, d2);
      break;

    case GAL_ARITHMETIC_OP_NOT:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_not(d1, flags);
      break;

    case GAL_ARITHMETIC_OP_ISBLANK:
    case GAL_ARITHMETIC_OP_ISNOTBLANK:
      d1 = va_arg(va, gal_data_t *);
      out = ( operator==GAL_ARITHMETIC_OP_ISBLANK
              ? gal_blank_flag(d1)
              : gal_blank_flag_not(d1) );
      if(flags & GAL_ARITHMETIC_FLAG_FREE) gal_data_free(d1);
      break;

    case GAL_ARITHMETIC_OP_WHERE:
      d1 = va_arg(va, gal_data_t *);    /* To modify value/array.     */
      d2 = va_arg(va, gal_data_t *);    /* Condition (unsigned char). */
      d3 = va_arg(va, gal_data_t *);    /* If true value/array.       */
      arithmetic_where(flags, d1, d2, d3);
      out=d1;
      break;

    /* Unary function operators. */
    case GAL_ARITHMETIC_OP_SQRT:
    case GAL_ARITHMETIC_OP_LOG:
    case GAL_ARITHMETIC_OP_LOG10:
    case GAL_ARITHMETIC_OP_SIN:
    case GAL_ARITHMETIC_OP_COS:
    case GAL_ARITHMETIC_OP_TAN:
    case GAL_ARITHMETIC_OP_ASIN:
    case GAL_ARITHMETIC_OP_ACOS:
    case GAL_ARITHMETIC_OP_ATAN:
    case GAL_ARITHMETIC_OP_SINH:
    case GAL_ARITHMETIC_OP_COSH:
    case GAL_ARITHMETIC_OP_TANH:
    case GAL_ARITHMETIC_OP_ASINH:
    case GAL_ARITHMETIC_OP_ACOSH:
    case GAL_ARITHMETIC_OP_ATANH:
    case GAL_ARITHMETIC_OP_AU_TO_PC:
    case GAL_ARITHMETIC_OP_PC_TO_AU:
    case GAL_ARITHMETIC_OP_LY_TO_PC:
    case GAL_ARITHMETIC_OP_PC_TO_LY:
    case GAL_ARITHMETIC_OP_LY_TO_AU:
    case GAL_ARITHMETIC_OP_AU_TO_LY:
    case GAL_ARITHMETIC_OP_MAG_TO_JY:
    case GAL_ARITHMETIC_OP_JY_TO_MAG:
    case GAL_ARITHMETIC_OP_RA_TO_DEGREE:
    case GAL_ARITHMETIC_OP_DEC_TO_DEGREE:
    case GAL_ARITHMETIC_OP_DEGREE_TO_RA:
    case GAL_ARITHMETIC_OP_DEGREE_TO_DEC:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_function_unary(operator, flags, d1);
      break;

    /* Binary function operators. */
    case GAL_ARITHMETIC_OP_POW:
    case GAL_ARITHMETIC_OP_ATAN2:
    case GAL_ARITHMETIC_OP_MAG_TO_SB:
    case GAL_ARITHMETIC_OP_SB_TO_MAG:
    case GAL_ARITHMETIC_OP_JY_TO_COUNTS:
    case GAL_ARITHMETIC_OP_COUNTS_TO_JY:
    case GAL_ARITHMETIC_OP_COUNTS_TO_MAG:
    case GAL_ARITHMETIC_OP_MAG_TO_COUNTS:
    case GAL_ARITHMETIC_OP_NANOMAGGY_TO_COUNTS:
    case GAL_ARITHMETIC_OP_COUNTS_TO_NANOMAGGY:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=arithmetic_function_binary_flt(operator, flags, d1, d2);
      break;

    /* More complex operators. */
    case GAL_ARITHMETIC_OP_COUNTS_TO_SB:
    case GAL_ARITHMETIC_OP_SB_TO_COUNTS:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      d3 = va_arg(va, gal_data_t *);
      out=arithmetic_counts_to_from_sb(operator, flags, d1, d2, d3);

      break;

    /* Statistical operators that return one value. */
    case GAL_ARITHMETIC_OP_MINVAL:
    case GAL_ARITHMETIC_OP_MAXVAL:
    case GAL_ARITHMETIC_OP_NUMBERVAL:
    case GAL_ARITHMETIC_OP_SUMVAL:
    case GAL_ARITHMETIC_OP_MEANVAL:
    case GAL_ARITHMETIC_OP_STDVAL:
    case GAL_ARITHMETIC_OP_MEDIANVAL:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_from_statistics(operator, flags, d1);
      break;

    /* Return 1D array (only values). */
    case GAL_ARITHMETIC_OP_UNIQUE:
    case GAL_ARITHMETIC_OP_NOBLANK:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_to_oned(operator, flags, d1);
      break;

    /* Absolute operator. */
    case GAL_ARITHMETIC_OP_ABS:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_abs(flags, d1);
      break;

    /* Multi-operand operators */
    case GAL_ARITHMETIC_OP_MIN:
    case GAL_ARITHMETIC_OP_MAX:
    case GAL_ARITHMETIC_OP_NUMBER:
    case GAL_ARITHMETIC_OP_SUM:
    case GAL_ARITHMETIC_OP_MEAN:
    case GAL_ARITHMETIC_OP_STD:
    case GAL_ARITHMETIC_OP_MEDIAN:
    case GAL_ARITHMETIC_OP_QUANTILE:
    case GAL_ARITHMETIC_OP_SIGCLIP_STD:
    case GAL_ARITHMETIC_OP_SIGCLIP_MEAN:
    case GAL_ARITHMETIC_OP_SIGCLIP_MEDIAN:
    case GAL_ARITHMETIC_OP_SIGCLIP_NUMBER:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=arithmetic_multioperand(operator, flags, d1, d2, numthreads);
      break;

    /* Binary operators that only work on integer types. */
    case GAL_ARITHMETIC_OP_BITAND:
    case GAL_ARITHMETIC_OP_BITOR:
    case GAL_ARITHMETIC_OP_BITXOR:
    case GAL_ARITHMETIC_OP_BITLSH:
    case GAL_ARITHMETIC_OP_BITRSH:
    case GAL_ARITHMETIC_OP_MODULO:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=arithmetic_binary(operator, flags, d1, d2);
      break;
    case GAL_ARITHMETIC_OP_BITNOT:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_bitwise_not(flags, d1);
      break;

    /* Random steps. */
    case GAL_ARITHMETIC_OP_MKNOISE_SIGMA:
    case GAL_ARITHMETIC_OP_MKNOISE_POISSON:
    case GAL_ARITHMETIC_OP_MKNOISE_UNIFORM:
    case GAL_ARITHMETIC_OP_RANDOM_FROM_HIST:
    case GAL_ARITHMETIC_OP_RANDOM_FROM_HIST_RAW:
    case GAL_ARITHMETIC_OP_MKNOISE_SIGMA_FROM_MEAN:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      if(operator==GAL_ARITHMETIC_OP_RANDOM_FROM_HIST
         || operator==GAL_ARITHMETIC_OP_RANDOM_FROM_HIST_RAW)
        {
          d3 = va_arg(va, gal_data_t *);
          out=arithmetic_random_from_hist(operator, flags, d1, d2, d3);
        }
      else
        out=arithmetic_mknoise(operator, flags, d1, d2);
      break;

    /* Dimensionality changing operators. */
    case GAL_ARITHMETIC_OP_TO1D:
    case GAL_ARITHMETIC_OP_TRIM:
      d1 = va_arg(va, gal_data_t *);
      out = ( operator==GAL_ARITHMETIC_OP_TO1D
              ? arithmetic_to_1d(flags, d1)
              : gal_blank_trim(d1, flags & GAL_ARITHMETIC_FLAG_FREE) );
      break;
    case GAL_ARITHMETIC_OP_STITCH:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=arithmetic_stitch(flags, d1, d2);
      break;

    /* Conversion operators. */
    case GAL_ARITHMETIC_OP_TO_UINT8:
    case GAL_ARITHMETIC_OP_TO_INT8:
    case GAL_ARITHMETIC_OP_TO_UINT16:
    case GAL_ARITHMETIC_OP_TO_INT16:
    case GAL_ARITHMETIC_OP_TO_UINT32:
    case GAL_ARITHMETIC_OP_TO_INT32:
    case GAL_ARITHMETIC_OP_TO_UINT64:
    case GAL_ARITHMETIC_OP_TO_INT64:
    case GAL_ARITHMETIC_OP_TO_FLOAT32:
    case GAL_ARITHMETIC_OP_TO_FLOAT64:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_change_type(d1, operator, flags);
      break;

    /* Constants. */
    case GAL_ARITHMETIC_OP_E:
    case GAL_ARITHMETIC_OP_C:
    case GAL_ARITHMETIC_OP_G:
    case GAL_ARITHMETIC_OP_H:
    case GAL_ARITHMETIC_OP_AU:
    case GAL_ARITHMETIC_OP_LY:
    case GAL_ARITHMETIC_OP_PI:
    case GAL_ARITHMETIC_OP_AVOGADRO:
    case GAL_ARITHMETIC_OP_FINESTRUCTURE:
      out=arithmetic_constants_standard(operator);
      break;

    /* Calculate the width and height of a box surrounding an ellipse with
       a certain major axis, minor axis and position angle. */
    case GAL_ARITHMETIC_OP_BOX_AROUND_ELLIPSE:
    case GAL_ARITHMETIC_OP_BOX_VERTICES_ON_SPHERE:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      d3 = va_arg(va, gal_data_t *);
      if(operator==GAL_ARITHMETIC_OP_BOX_AROUND_ELLIPSE)
        out=arithmetic_box(d1, d2, d3, NULL, operator, flags);
      else
        {
          d4=va_arg(va, gal_data_t *);
          out=arithmetic_box(d1, d2, d3, d4, operator, flags);
        }
      break;

    /* Size and position operators. */
    case GAL_ARITHMETIC_OP_SIZE:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=arithmetic_size(operator, flags, d1, d2);
      break;
    case GAL_ARITHMETIC_OP_MAKENEW:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_makenew(d1);
      break;
    case GAL_ARITHMETIC_OP_INDEX:
    case GAL_ARITHMETIC_OP_COUNTER:
    case GAL_ARITHMETIC_OP_INDEXONLY:
    case GAL_ARITHMETIC_OP_COUNTERONLY:
      d1 = va_arg(va, gal_data_t *);
      out=arithmetic_index_counter(d1, operator, flags);
      break;
    case GAL_ARITHMETIC_OP_SWAP:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      d1->next=d2; d2->next=NULL;  out=d1;
      break;
    case GAL_ARITHMETIC_OP_CONSTANT:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      out=arithmetic_constant(d1, d2, operator, flags);
      break;

    /* Pooling operators. */
    case GAL_ARITHMETIC_OP_POOLMAX:
    case GAL_ARITHMETIC_OP_POOLMIN:
    case GAL_ARITHMETIC_OP_POOLSUM:
    case GAL_ARITHMETIC_OP_POOLMEAN:
    case GAL_ARITHMETIC_OP_POOLMEDIAN:
      d1 = va_arg(va, gal_data_t *);
      d2 = va_arg(va, gal_data_t *);
      d3 = va_arg(va, gal_data_t *);
      out=arithmetic_pool(d1, d2, d3, operator, numthreads, flags);
      break;

    /* When the operator is not recognized. */
    default:
      error(EXIT_FAILURE, 0, "%s: the argument '%d' could not be "
            "interpretted as an operator", __func__, operator);
    }

  /* End the variable argument structure and return. */
  va_end(va);
  return out;
}
