/*********************************************************************
Type -- Type information and basic operations.
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
#include <float.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <gnuastro/type.h>
#include <gnuastro/data.h>
#include <gnuastro/list.h>
#include <gnuastro/pointer.h>
#include <gnuastro-internal/checkset.h>






/*************************************************************
 **************         General info           ***************
 *************************************************************/
size_t
gal_type_sizeof(uint8_t type)
{
  /* Allocate space for the array to keep the image. */
  switch(type)
    {
    case GAL_TYPE_BIT:
      error(EXIT_FAILURE, 0, "%s: bit types are not currently supported, "
            "please get in touch with us to implement it", __func__);

      /* The parenthesis after sizeof is not a function, it is actually a
         type cast, so we have put a space between size of and the
         parenthesis to highlight this. In C, 'sizeof' is an operator, not
         a function. */
    case GAL_TYPE_UINT8:     return sizeof (uint8_t);
    case GAL_TYPE_INT8:      return sizeof (int8_t);
    case GAL_TYPE_UINT16:    return sizeof (uint16_t);
    case GAL_TYPE_INT16:     return sizeof (int16_t);
    case GAL_TYPE_UINT32:    return sizeof (uint32_t);
    case GAL_TYPE_INT32:     return sizeof (int32_t);
    case GAL_TYPE_UINT64:    return sizeof (uint64_t);
    case GAL_TYPE_INT64:     return sizeof (int64_t);

    case GAL_TYPE_FLOAT32:
      if( sizeof (float) != 4 )
        error(EXIT_FAILURE, 0, "%s: 'float' is not 32 bits on this machine",
              __func__);
      return sizeof (float);

    case GAL_TYPE_FLOAT64:
      if( sizeof (double) != 8 )
        error(EXIT_FAILURE, 0, "%s: 'double' is not 64 bits on this machine",
              __func__);
      return sizeof (double);

    case GAL_TYPE_COMPLEX32:
      if( sizeof (float) != 4 )
        error(EXIT_FAILURE, 0, "%s: 'float' is not 32 bits on this machine",
              __func__);
      return sizeof (gsl_complex_float);

    case GAL_TYPE_COMPLEX64:
      if( sizeof (double) != 8 )
        error(EXIT_FAILURE, 0, "%s: 'double' is not 64 bits on this machine",
              __func__);
      return sizeof (gsl_complex);

    case GAL_TYPE_STRING:
      return sizeof (char *);

    default:
      error(EXIT_FAILURE, 0, "%s: type value of %d not recognized",
            __func__, type);
    }

  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s so we can find "
        "the cause of the problem. Control should not have reached the end of "
        "this function", __func__, PACKAGE_BUGREPORT);
  return -1;
}





char *
gal_type_name(uint8_t type, int long_name)
{
  switch(type)
    {
    case GAL_TYPE_BIT:
      if(long_name) return "bit";                 else return "b";

    case GAL_TYPE_UINT8:
      if(long_name) return "uint8";               else return "u8";

    case GAL_TYPE_INT8:
      if(long_name) return "int8";                else return "i8";

    case GAL_TYPE_UINT16:
      if(long_name) return "uint16";              else return "u16";

    case GAL_TYPE_INT16:
      if(long_name) return "int16";               else return "i16";

    case GAL_TYPE_UINT32:
      if(long_name) return "uint32";              else return "u32";

    case GAL_TYPE_INT32:
      if(long_name) return "int32";               else return "i32";

    case GAL_TYPE_UINT64:
      if(long_name) return "uint64";              else return "u64";

    case GAL_TYPE_INT64:
      if(long_name) return "int64";               else return "i64";

    case GAL_TYPE_FLOAT32:
      if(long_name) return "float32";             else return "f32";

    case GAL_TYPE_FLOAT64:
      if(long_name) return "float64";             else return "f64";

    case GAL_TYPE_COMPLEX32:
      if(long_name) return "complex32";           else return "c32";

    case GAL_TYPE_COMPLEX64:
      if(long_name) return "complex64";           else return "c64";

    case GAL_TYPE_STRING:
      if(long_name) return "string";              else return "str";

    case GAL_TYPE_STRLL:
      if(long_name) return "string linked list";  else return "strll";

    default:
      error(EXIT_FAILURE, 0, "%s: type value of %d not recognized",
            __func__, type);
    }

  /* Any of the cases above should return this function, so if control
     reaches here, there is a bug. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s so we can "
        "address the problem. Control should not have reached the end of "
        "this function", __func__, PACKAGE_BUGREPORT);
  return NULL;
}





uint8_t
gal_type_from_name(char *str)
{
  if(      !strcmp(str, "b")     || !strcmp(str, "bit") )
    return GAL_TYPE_BIT;

  else if( !strcmp(str, "u8")    || !strcmp(str, "uint8") )
    return GAL_TYPE_UINT8;

  else if( !strcmp(str, "i8")    || !strcmp(str, "int8") )
    return GAL_TYPE_INT8;

  else if( !strcmp(str, "u16")   || !strcmp(str, "uint16") )
    return GAL_TYPE_UINT16;

  else if( !strcmp(str, "i16")   || !strcmp(str, "int16") )
    return GAL_TYPE_INT16;

  else if( !strcmp(str, "u32")   || !strcmp(str, "uint32") )
    return GAL_TYPE_UINT32;

  else if( !strcmp(str, "i32")   || !strcmp(str, "int32") )
    return GAL_TYPE_INT32;

  else if( !strcmp(str, "u64")   || !strcmp(str, "uint64") )
    return GAL_TYPE_UINT64;

  else if( !strcmp(str, "i64")   || !strcmp(str, "int64") )
    return GAL_TYPE_INT64;

  else if( !strcmp(str, "f32")   || !strcmp(str, "float32") )
    return GAL_TYPE_FLOAT32;

  else if( !strcmp(str, "f64")   || !strcmp(str, "float64") )
    return GAL_TYPE_FLOAT64;

  else if( !strcmp(str, "c32")   || !strcmp(str, "complex32") )
    return GAL_TYPE_COMPLEX32;

  else if( !strcmp(str, "c64")   || !strcmp(str, "complex64") )
    return GAL_TYPE_COMPLEX64;

  else if( !strcmp(str, "str")   || !strcmp(str, "string") )
    return GAL_TYPE_STRING;

  else
    return GAL_TYPE_INVALID;

  /* Any of the cases above should return this function, so if control
     reaches here, there is a bug. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s so we can "
        "address the problem. Control must not have reached the end of this "
        "function", __func__, PACKAGE_BUGREPORT);
  return 0;
}





/* Put the minimum (or maximum for the 'gal_type_max') value for the
   type in the space (that must already be allocated before the call to
   this function) pointed to by in. */
void
gal_type_min(uint8_t type, void *in)
{
  switch(type)
    {
    case GAL_TYPE_UINT8:        *(uint8_t *)  in = 0;            break;
    case GAL_TYPE_INT8:         *(int8_t *)   in = INT8_MIN;     break;
    case GAL_TYPE_UINT16:       *(uint16_t *) in = 0;            break;
    case GAL_TYPE_INT16:        *(int16_t *)  in = INT16_MIN;    break;
    case GAL_TYPE_UINT32:       *(uint32_t *) in = 0;            break;
    case GAL_TYPE_INT32:        *(int32_t *)  in = INT32_MIN;    break;
    case GAL_TYPE_UINT64:       *(uint64_t *) in = 0;            break;
    case GAL_TYPE_INT64:        *(int64_t *)  in = INT64_MIN;    break;
    case GAL_TYPE_FLOAT32:      *(float *)    in = -FLT_MAX;     break;
    case GAL_TYPE_FLOAT64:      *(double *)   in = -DBL_MAX;     break;
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized", __func__, type);
    }
}





void
gal_type_max(uint8_t type, void *in)
{
  switch(type)
    {
    case GAL_TYPE_UINT8:        *(uint8_t *)  in = UINT8_MAX;    break;
    case GAL_TYPE_INT8:         *(int8_t *)   in = INT8_MAX;     break;
    case GAL_TYPE_UINT16:       *(uint16_t *) in = UINT16_MAX;   break;
    case GAL_TYPE_INT16:        *(int16_t *)  in = INT16_MAX;    break;
    case GAL_TYPE_UINT32:       *(uint32_t *) in = UINT32_MAX;   break;
    case GAL_TYPE_INT32:        *(int32_t *)  in = INT32_MAX;    break;
    case GAL_TYPE_UINT64:       *(uint64_t *) in = UINT64_MAX;   break;
    case GAL_TYPE_INT64:        *(int64_t *)  in = INT64_MAX;    break;
    case GAL_TYPE_FLOAT32:      *(float *)    in = FLT_MAX;      break;
    case GAL_TYPE_FLOAT64:      *(double *)   in = DBL_MAX;      break;
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized", __func__,
            type);
    }
}





int
gal_type_is_int(uint8_t type)
{
  switch(type)
    {
    case GAL_TYPE_UINT8:  return 1;
    case GAL_TYPE_INT8:   return 1;
    case GAL_TYPE_UINT16: return 1;
    case GAL_TYPE_INT16:  return 1;
    case GAL_TYPE_UINT32: return 1;
    case GAL_TYPE_INT32:  return 1;
    case GAL_TYPE_UINT64: return 1;
    case GAL_TYPE_INT64:  return 1;
    default:              return 0;
    }
}





/* Since linked lists need a different process than arrays, for functions
   that work on both, it is convenient to simiplify the check with this
   function. */
int
gal_type_is_list(uint8_t type)
{
  return type==GAL_TYPE_STRLL;
}





int
gal_type_out(int first_type, int second_type)
{
  return first_type > second_type ? first_type : second_type;
}




















/*************************************************************
 **************         To/from string         ***************
 *************************************************************/
/* Write the bit (0 or 1) contents of 'in' into a string ready for
   printing. 'size' is used to determine the number of bytes to print. The
   output string will be dynamically allocated within this function. This
   can be useful for easy checking of bit flag values, for example in an
   expression like below:

      printf("flag: %s\n", gal_type_bit_string(&flag, sizeof flag) );    */
char *
gal_type_bit_string(void *in, size_t size)
{
  size_t i;
  char *byte=in;
  char *str=gal_pointer_allocate(GAL_TYPE_UINT8, 8*size+1, 0, __func__,
                                 "str");

  /* Print the bits into the allocated string. This was inspired from

     http://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format */
  for(i=0;i<size;++i)
    sprintf(str+i*8, "%c%c%c%c%c%c%c%c ",
           (byte[i] & 0x80 ? '1' : '0'), (byte[i] & 0x40 ? '1' : '0'),
           (byte[i] & 0x20 ? '1' : '0'), (byte[i] & 0x10 ? '1' : '0'),
           (byte[i] & 0x08 ? '1' : '0'), (byte[i] & 0x04 ? '1' : '0'),
           (byte[i] & 0x02 ? '1' : '0'), (byte[i] & 0x01 ? '1' : '0') );

  /* Return the allocated and filled string. */
  return str;
}





/* Write the contents of memory that 'ptr' points to as a string of type
   'type'.*/
#define TO_STRING(CTYPE, FMT) {                                         \
  if( asprintf(&str, FMT, *(CTYPE *)ptr)<0 )                            \
    error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__); }

char *
gal_type_to_string(void *ptr, uint8_t type, int quote_if_str_has_space)
{
  char *c, *str=NULL;
  switch(type)
    {
    /* For a string we might need to make sure it has no white space
       characters, if it does, it can be printed it within quotation
       signs. */
    case GAL_TYPE_STRING:
      if(quote_if_str_has_space)
        {
          c=*(char **)ptr; while(*c!='\0') if(isspace(*c++)) break;
          if(*c=='\0')
            {
              if( asprintf(&str, "%s", *(char **)ptr)<0 )
                error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
            }
          else
            {
              if( asprintf(&str, "\"%s\" ", *(char **)ptr)<0 )
                error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
            }
        }
      else
        if( asprintf(&str, "%s", *(char **)ptr)<0 )
          error(EXIT_FAILURE, 0, "%s: asprintf allocation", __func__);
      break;

    case GAL_TYPE_UINT8:   TO_STRING( uint8_t,  "%"PRIu8  );  break;
    case GAL_TYPE_INT8:    TO_STRING( int8_t,   "%"PRId8  );  break;
    case GAL_TYPE_UINT16:  TO_STRING( uint16_t, "%"PRIu16 );  break;
    case GAL_TYPE_INT16:   TO_STRING( int16_t,  "%"PRId16 );  break;
    case GAL_TYPE_UINT32:  TO_STRING( uint32_t, "%"PRIu32 );  break;
    case GAL_TYPE_INT32:   TO_STRING( int32_t,  "%"PRId32 );  break;
    case GAL_TYPE_UINT64:  TO_STRING( uint64_t, "%"PRIu64 );  break;
    case GAL_TYPE_INT64:   TO_STRING( int64_t,  "%"PRId64 );  break;

    /* We aren't using '%g' for floating points because it can remove
       statisically significant digits in some scenarios and its result is
       generally not easily predictable: can be fixed-point or exponential
       depending on printed length! But the printed length of a number can
       hide statisical significance. See the discussion in
       'bin/table/asttable.conf' for the values used here. */
    case GAL_TYPE_FLOAT32: TO_STRING( float,  "%.6e" ); break;
    case GAL_TYPE_FLOAT64: TO_STRING( double, "%.14e"); break;

    /* Unknown type! */
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, type);
    }
  return str;
}





/* Read a string as a given data type and put a the pointer to it in
   *out. When the input '*out!=NULL', then it is assumed to be allocated
   and the value will be simply put there. If '*out==NULL', then space will
   be allocated for the given type and the string's value (in the given
   type) will be stored there.

   Note that when we are dealing with a string type, '*out' should be
   interpretted as 'char **' (one element in an array of pointers to
   different strings). In other words, 'out' should be 'char ***'.

   This function can be used to fill in arrays of numbers from strings (in
   an already allocated data structure), or add nodes to a linked list. For
   an array, you have to pass the pointer to the 'i'th element where you
   want the value to be stored, for example &(array[i]).

   If parsing was successful, it will return a 0. If there was a problem,
   it will return 1. */
int
gal_type_from_string(void **out, char *string, uint8_t type)
{
  long l;
  double d;
  void *value;
  char *tailptr;
  int status=0, allocated=0;

  /* If the output is NULL, then allocate the necessary space if we are not
     dealing with a linked list. In a linked list, a NULL value is
     meaningful (it is the end of the list). */
  if( *out==NULL && !gal_type_is_list(type) )
    {
      allocated=1;
      *out=gal_pointer_allocate(type, 1, 0, __func__, "out");
    }
  value=*out;

  /* Read the string depending on the type. */
  switch(type)
    {

    /* Linked lists, currently only string linked lists. */
    case GAL_TYPE_STRLL:
      gal_list_str_add( (struct gal_list_str_t **)out, string, 1);
      break;

    /* String, just allocate and copy the string and keep its pointer in
       the place '*out' points to (for strings, '*out' is 'char **'). */
    case GAL_TYPE_STRING:
      gal_checkset_allocate_copy(string, value);
      break;

    /* Floating point: Read it as a double or long, then put it in the
       array. When the conversion can't be done (the string isn't a number
       for example), then just assume no blank value was given. */
    case GAL_TYPE_FLOAT32:
    case GAL_TYPE_FLOAT64:
      d=strtod(string, &tailptr);
      if(*tailptr!='\0')
        status=1;
      else
        {
          if(type==GAL_TYPE_FLOAT32) *(float  *) value=d;
          else                       *(double *) value=d;
        }
      break;

    /* Integers. */
    default:
      l=strtol(string, &tailptr, 0);
      if(*tailptr!='\0')
        status=1;
      else
        switch(type)
          {
          /* The signed values can easily be put in. */
          case GAL_TYPE_INT8:  *(int8_t  *) value = l; break;
          case GAL_TYPE_INT16: *(int16_t *) value = l; break;
          case GAL_TYPE_INT32: *(int32_t *) value = l; break;
          case GAL_TYPE_INT64: *(int64_t *) value = l; break;

          /* For the unsigned types, the value has to be positive, so if
             the input was negative, then just return a status of one and
             don't store the value. */
          default:
            if(l<0)
              status=1;
            else
              switch(type)
                {
                case GAL_TYPE_UINT8:  *(uint8_t  *) value=l;   break;
                case GAL_TYPE_UINT16: *(uint16_t *) value=l;   break;
                case GAL_TYPE_UINT32: *(uint32_t *) value=l;   break;
                case GAL_TYPE_UINT64: *(uint64_t *) value=l;   break;
                default:
                  error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
                        __func__, type);
                }
          }
    }

  /* If reading was unsuccessful, then free the space if it was allocated,
     then return the status, don't touch the pointer. */
  if(status && allocated)
    {
      free(*out);
      *out=NULL;
    }
  return status;
}





/* If the data structure was correctly created (the string was a number),
   then return its pointer. Otherwise, return NULL. */
void *
gal_type_string_to_number(char *string, uint8_t *type)
{
  long int l;
  size_t digits;
  void *ptr, *out;
  uint8_t forcedfloat=0;
  char *c, *tailptr, *cp;

  /* Define initial spaces to keep the value. */
  uint8_t   u8;   int8_t   i8;      uint16_t u16;   int16_t i16;
  uint32_t u32;   int32_t i32;      uint64_t u64;   int64_t i64;
  float      f;   double    d;

  /* First see if the number is a double (the most generic). */
  d=strtod(string, &tailptr);
  if(*tailptr=='f') { if(tailptr[1]=='\0') forcedfloat=1; else return NULL; }
  else if (*tailptr!='\0')  return NULL;

  /* The number has been parsed successfully as a number. But if it
     contains a '.', then it must a "forced" float also. This won't be a
     problem in scenarios like '.2', but people may use '2.' or '2.0' to
     force a float and this loop is necessary in such cases. */
  for(c=string; *c!='\0'; ++c) if(*c=='.') { forcedfloat=1; break; }

  /* Read the number as an integer if 1) we aren't in forced-float mode, 2)
     the number is actually an integer ('ceil(d)==d'), and 3) the number
     fits within interger limits: the maximum value of an unsigned 64-bit
     integer is almost 1.8e19, and the minimum value of a signed 64-bit
     integer is almost -9.2e18), see the "Numeric data types" section of
     Gnuastro's book. */
  if( forcedfloat==0 && ceil(d) == d && d<1.8e19f && d>-9.2e18f )
    {
      /* We know the number is an integer, so we should re-read it again,
         but this time, as an integer, because: 1) floating point numbers
         can only preserve a certain number of decimals precisely after a
         certain number of decimals, they loose precision. 2) Integer
         comparisons (that are done below) are faster, but this is
         secondary because the parsing itself takes more time!

         The string is being parsed as an integer in base-10 (third
         argument of 'strtol'). This should not be '0', otherwise 'strtol'
         will parse strings starting with '0' in octal radix and strings
         starting in '0x' in hexagesimal radix (these aren't used in data
         analysis, only in computer science). */
      l=strtol(string, &tailptr, 10);
      if(*tailptr!='\0')
        {
          /* If 'tailptr' is simply an 'e', the input string was in
             scientific notation (for example '1e5'). In such cases, the
             number of decimals is (usually!) enough to fit in a double,
             and we can simply put the value of 'd' in 'l'. Note that with
             'ceil(d)==d' we have confirmed that the number is actually an
             integer (and not a float). */
          if(*tailptr=='e') l=d;
          else return NULL;
        }

      /* If the number is negative, put it in the signed types (based on
         its value). If its zero or positive, then put it in the unsigned
         types. */
      if( l < 0 )
        {
          if     (l>INT8_MIN)    { i8=l;  ptr=&i8;  *type=GAL_TYPE_INT8;  }
          else if(l>INT16_MIN)   { i16=l; ptr=&i16; *type=GAL_TYPE_INT16; }
          else if(l>INT32_MIN)   { i32=l; ptr=&i32; *type=GAL_TYPE_INT32; }
          else                   { i64=l; ptr=&i64; *type=GAL_TYPE_INT64; }
        }
      else
        {
          /* Note that the blank values are set to the maximum values in
             unsigned types. A blank value should be given as a blank
             string to this function ('GAL_BLANK_STRING'). So, to avoid
             confusing situations (for example when the user gives 255), if
             the value is equal to the given maximum of the given type,
             we'll assign it to a larger type. In other words, we won't be
             using the '<=MAX', but '<MAX'.

             Even though they are positive, we should give priority to the
             signed types if the number fits in the range of signed type
             for that width: this is the way that C's internal automatic
             type conversion works (which is used by Arithmetic's binary
             operators for example). */
          if     (l<UINT8_MAX)
            {  if(l>INT8_MAX)  { u8=l;  ptr=&u8;  *type=GAL_TYPE_UINT8; }
               else            { i8=l;  ptr=&i8;  *type=GAL_TYPE_INT8;  } }
          else if(l<UINT16_MAX)
            {  if(l>INT16_MAX) { u16=l; ptr=&u16; *type=GAL_TYPE_UINT16; }
               else            { i16=l; ptr=&i16; *type=GAL_TYPE_INT16;  } }
          else if(l<UINT32_MAX)
            {  if(l>INT32_MAX) { u32=l; ptr=&u32; *type=GAL_TYPE_UINT32; }
               else            { i32=l; ptr=&i32; *type=GAL_TYPE_INT32;  } }
          else
            {  if(l>INT64_MAX) { u64=l; ptr=&u64; *type=GAL_TYPE_UINT64; }
               else            { i64=l; ptr=&i64; *type=GAL_TYPE_INT64;  } }
        }
    }
  else
    {
      /* Start counting the number of digits from the start of the string
         (while ignoring any '0's at the start). */
      digits=0;
      for(cp=string;*cp!='\0';++cp)
        {
          if(isdigit(*cp))
            { if(!(digits==0 && *cp=='0')) ++digits; }
          if(*cp=='e') break;
        }

      /* In the previous loop, we went to the end of the string (or the 'e'
         character in an exponential), so 'cp' now points to its end. We
         just have to iterate backwards and stop when we hit a non-zero
         character. */
      for(;cp!=string;--cp)
        if(isdigit(*cp))
          {
            if(*cp=='0') --digits;
            else break;
          }

      /* Calculate the number of decimal digits and decide if it the number
         should be a float or a double.

         The maximum number of decimal digits to store 32-bit floating
         point is 7.22 (see "Printing floating point numbers" section of
         the book). We will round this to 7 to be on the safe side. If the
         given number has more than 7 decimal digits, or is outside the
         range of possible values for a 32-bit float, it should be saved as
         a 64-bit float. */
      if( digits > 7 || fabs(d)>FLT_MAX || fabs(d)<FLT_MIN )
        {      ptr=&d; *type=GAL_TYPE_FLOAT64; }
      else
        { f=d; ptr=&f; *type=GAL_TYPE_FLOAT32; }

      /* For a check:
      printf("%s:%s: %zu %s\n", __func__, string, digits,
             gal_type_name(*type, 1));
      printf("%s: GOOD\n", __func__); exit(0);
      */
    }

  /* Allocate a one-element dataset, then copy the number into it. */
  out=gal_pointer_allocate(*type, 1, 0, __func__, "out");
  memcpy(out, ptr, gal_type_sizeof(*type));
  return out;
}
