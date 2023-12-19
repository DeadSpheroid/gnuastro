/*********************************************************************
Arithmetic - Do arithmetic operations on images.
Arithmetic is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

#include <gnuastro/wcs.h>
#include <gnuastro/fits.h>
#include <gnuastro/array.h>
#include <gnuastro/binary.h>
#include <gnuastro/threads.h>
#include <gnuastro/pointer.h>
#include <gnuastro/dimension.h>
#include <gnuastro/statistics.h>
#include <gnuastro/arithmetic.h>
#include <gnuastro/interpolate.h>

#include <gnuastro-internal/checkset.h>

#include "main.h"

#include "operands.h"
#include "arithmetic.h"














/***************************************************************/
/*************          Internal functions         *************/
/***************************************************************/
#define SET_NUM_OP(CTYPE) {            \
    CTYPE a=*(CTYPE *)(numpop->array); \
    gal_data_free(numpop);             \
    if(a>0) return a;    }

static size_t
pop_number_of_operands(struct arithmeticparams *p, int op,
                       char *token_string, gal_data_t **params)
{
  char *cstring="first";
  size_t c, numparams=0;
  gal_data_t *tmp, *numpop;

  /* See if this operator needs any parameters. If so, pop them. */
  switch(op)
    {
    case GAL_ARITHMETIC_OP_STITCH:
    case GAL_ARITHMETIC_OP_QUANTILE:
      numparams=1;
      break;
    case GAL_ARITHMETIC_OP_MADCLIP_STD:
    case GAL_ARITHMETIC_OP_SIGCLIP_STD:
    case GAL_ARITHMETIC_OP_MADCLIP_MAD:
    case GAL_ARITHMETIC_OP_SIGCLIP_MAD:
    case GAL_ARITHMETIC_OP_MADCLIP_MEAN:
    case GAL_ARITHMETIC_OP_SIGCLIP_MEAN:
    case GAL_ARITHMETIC_OP_MADCLIP_MEDIAN:
    case GAL_ARITHMETIC_OP_SIGCLIP_MEDIAN:
    case GAL_ARITHMETIC_OP_MADCLIP_NUMBER:
    case GAL_ARITHMETIC_OP_SIGCLIP_NUMBER:
    case GAL_ARITHMETIC_OP_MADCLIP_FILL_MAD:
    case GAL_ARITHMETIC_OP_SIGCLIP_FILL_MAD:
    case GAL_ARITHMETIC_OP_MADCLIP_FILL_STD:
    case GAL_ARITHMETIC_OP_SIGCLIP_FILL_STD:
    case GAL_ARITHMETIC_OP_MADCLIP_FILL_MEAN:
    case GAL_ARITHMETIC_OP_SIGCLIP_FILL_MEAN:
    case GAL_ARITHMETIC_OP_MADCLIP_FILL_MEDIAN:
    case GAL_ARITHMETIC_OP_SIGCLIP_FILL_MEDIAN:
    case GAL_ARITHMETIC_OP_MADCLIP_FILL_NUMBER:
    case GAL_ARITHMETIC_OP_SIGCLIP_FILL_NUMBER:
      numparams=2;
      break;
    }

  /* If any parameters should be read, read them. */
  *params=NULL;
  for(c=0;c<numparams;++c)
    {
      /* If it only has one element, save it as floating point and add it
         to the list. */
      tmp=operands_pop(p, token_string);
      if(tmp->size>1)
        error(EXIT_FAILURE, 0, "the %s popped operand of the '%s' "
              "operator must be a single number", cstring, token_string);
      tmp=gal_data_copy_to_new_type_free(tmp, GAL_TYPE_FLOAT32);
      gal_list_data_add(params, tmp);

      /* A small sanity check (none of the parameters for sigma-clipping,
         or quantile estimation can be negative). */
      if( ((float *)(tmp->array))[0]<=0.0 )
        error(EXIT_FAILURE, 0, "the %s popped operand of the '%s' "
              "operator cannot be zero or negative", cstring, token_string);

      /* Increment the counter string. */
      cstring=c?"third":"second";
    }

  /* Check if it is a number. */
  numpop=operands_pop(p, token_string);
  if(numpop->size>1)
    error(EXIT_FAILURE, 0, "the %s popped operand of the '%s' "
          "operator (number of input datasets) must be a number, not an "
          "array", cstring, token_string);

  /* Check its type and return the value. */
  switch(numpop->type)
    {
    /* For the integer types, if they are unsigned, then just pass their
       value, if they are signed, you have to make sure they are zero or
       positive. */
    case GAL_TYPE_UINT8:   SET_NUM_OP(uint8_t);     break;
    case GAL_TYPE_INT8:    SET_NUM_OP(int8_t);      break;
    case GAL_TYPE_UINT16:  SET_NUM_OP(uint16_t);    break;
    case GAL_TYPE_INT16:   SET_NUM_OP(int16_t);     break;
    case GAL_TYPE_UINT32:  SET_NUM_OP(uint32_t);    break;
    case GAL_TYPE_INT32:   SET_NUM_OP(int32_t);     break;
    case GAL_TYPE_UINT64:  SET_NUM_OP(uint64_t);    break;
    case GAL_TYPE_INT64:   SET_NUM_OP(int64_t);     break;

    /* Floating point numbers are not acceptable in this context. */
    case GAL_TYPE_FLOAT32:
    case GAL_TYPE_FLOAT64:
      error(EXIT_FAILURE, 0, "the %s popped operand of the '%s' "
            "operator (number of input datasets) must be an integer type",
            cstring, token_string);

    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, numpop->type);
    }

  /* If control reaches here, then the number must have been a negative
     value, so print an error. */
  error(EXIT_FAILURE, 0, "the %s popped operand of the '%s' operator "
        "cannot be zero or a negative number", cstring,
        token_string);
  return 0;
}




















/**********************************************************************/
/****************         Filtering operators         *****************/
/**********************************************************************/
#define ARITHMETIC_FILTER_DIM 10

struct arithmetic_filter_p
{
  int           operator;       /* The type of filtering.                */
  size_t          *fsize;       /* Filter size.                          */
  size_t        *hpfsize;       /* Positive Half-filter size.            */
  size_t        *hnfsize;       /* Negative Half-filter size.            */
  float     sclip_multip;       /* Sigma multiple in sigma-clipping.     */
  float      sclip_param;       /* Termination critera in sigma-cliping. */
  gal_data_t      *input;       /* Input dataset.                        */
  gal_data_t        *out;       /* Output dataset.                       */
};





/* Main filtering work function. */
static void *
arithmetic_filter(void *in_prm)
{
  struct gal_threads_params *tprm=(struct gal_threads_params *)in_prm;
  struct arithmetic_filter_p *afp=(struct arithmetic_filter_p *)tprm->params;
  gal_data_t *input=afp->input;

  size_t sind=-1;
  int clipflags=0;
  size_t ind, index, one=1;
  gal_data_t *sigclip, *result=NULL;
  size_t *hpfsize=afp->hpfsize, *hnfsize=afp->hnfsize;
  size_t *tsize, *dsize=input->dsize, *fsize=afp->fsize;
  size_t i, j, coord[ARITHMETIC_FILTER_DIM], ndim=input->ndim;
  size_t start[ARITHMETIC_FILTER_DIM], end[ARITHMETIC_FILTER_DIM];
  gal_data_t *tile=gal_data_alloc(NULL, input->type, ndim, afp->fsize, NULL,
                                  0, -1, 1, NULL, NULL, NULL);

  /* Prepare the tile. */
  free(tile->array);
  tsize=tile->dsize;
  tile->block=input;


  /* Go over all the pixels that were assigned to this thread. */
  for(i=0; tprm->indexs[i] != GAL_BLANK_SIZE_T; ++i)
    {
      /* For easy reading, put the index in 'ind'. */
      ind=tprm->indexs[i];

      /* Get the coordinate of the pixel. */
      gal_dimension_index_to_coord(ind, ndim, dsize, coord);

      /* See which dimensions need trimming. */
      tile->size=1;
      for(j=0;j<ndim;++j)
        {
          /* Estimate the coordinate of the filter's starting point. Note
             that we are dealing with size_t (unsigned int) type here, so
             there are no negatives. A negative result will produce an
             extremely large number, so instead of checking for negative,
             we can just see if the result of a subtraction is less than
             the width of the input. */
          if( (coord[j] - hnfsize[j] > dsize[j])
              || (coord[j] + hpfsize[j] >= dsize[j]) )
            {
              start[j] = ( (coord[j] - hnfsize[j] > dsize[j])
                           ? 0 : coord[j] - hnfsize[j] );
              end[j]   = ( (coord[j] + hpfsize[j] >= dsize[j])
                           ? dsize[j]
                           : coord[j] + hpfsize[j] + 1);
              tsize[j] = end[j] - start[j];
            }
          else  /* NOT on the edge (given requested filter width). */
            {
              tsize[j] = fsize[j];
              start[j] = coord[j] - hnfsize[j];
            }
          tile->size *= tsize[j];
        }

      /* For a test.
         printf("coord: %zu, %zu\n", coord[1]+1, coord[0]+1);
         printf("\tstart: %zu, %zu\n", start[1]+1, start[0]+1);
         printf("\ttsize: %zu, %zu\n", tsize[1], tsize[0]);
      */

      /* Set the tile's starting pointer. */
      index=gal_dimension_coord_to_index(ndim, dsize, start);
      tile->array=gal_pointer_increment(input->array, index, input->type);

      /* Do the necessary calculation. */
      switch(afp->operator)
        {
        case ARITHMETIC_OP_FILTER_MEDIAN:
          result=gal_statistics_median(tile, 0);
          break;


        case ARITHMETIC_OP_FILTER_MEAN:
          result=gal_statistics_mean(tile);
          break;


        case ARITHMETIC_OP_FILTER_SIGCLIP_MEAN:
        case ARITHMETIC_OP_FILTER_SIGCLIP_MEDIAN:
          /* The median is always available with a sigma-clip, but the mean
             needs to be explicitly requested. */
          if(afp->operator == ARITHMETIC_OP_FILTER_SIGCLIP_MEAN)
            clipflags = GAL_STATISTICS_CLIP_OUTCOL_OPTIONAL_MEAN;
          sigclip=gal_statistics_clip_sigma(tile, afp->sclip_multip,
                                            afp->sclip_param, clipflags,
                                            0, 1);

          /* Set the required index. */
          switch(afp->operator)
            {
            case ARITHMETIC_OP_FILTER_SIGCLIP_MEAN:
              sind = GAL_STATISTICS_CLIP_OUTCOL_MEAN; break;
            case ARITHMETIC_OP_FILTER_SIGCLIP_MEDIAN:
              sind = GAL_STATISTICS_CLIP_OUTCOL_MEDIAN; break;
            default:
              error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at "
                    "%s to fix the problem. The 'afp->operator' value "
                    "%d is not recognized as sigma-clipped median or "
                    "mean", __func__, PACKAGE_BUGREPORT, afp->operator);
            }

          /* Allocate the output and write the value into it. */
          result=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL,
                                0, -1, 1, NULL, NULL, NULL);
          ((float *)(result->array))[0] =
            ((float *)(sigclip->array))[sind];

          /* Clean up. */
          gal_data_free(sigclip);
          break;


        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s "
                "to fix the problem. 'afp->operator' code %d is not "
                "recognized", PACKAGE_BUGREPORT, __func__,
                afp->operator);
        }

      /* Make sure the output array type and result's type are the
         same. */
      if(result->type!=afp->out->type)
        result=gal_data_copy_to_new_type_free(result, afp->out->type);


      /* Copy the result into the output array. */
      memcpy(gal_pointer_increment(afp->out->array, ind, afp->out->type),
             result->array, gal_type_sizeof(afp->out->type));

      /* Clean up for this pixel. */
      gal_data_free(result);
    }


  /* Clean up for this thread. */
  tile->array=NULL;
  tile->block=NULL;
  gal_data_free(tile);


  /* Wait for all the other threads to finish, then return. */
  if(tprm->b) pthread_barrier_wait(tprm->b);
  return NULL;
}





static void
wrapper_for_filter(struct arithmeticparams *p, char *token, int operator)
{
  int type=GAL_TYPE_INVALID;
  size_t i=0, ndim, nparams, one=1;
  struct arithmetic_filter_p afp={0};
  size_t fsize[ARITHMETIC_FILTER_DIM];
  gal_data_t *tmp, *tmp2, *zero, *comp, *params_list=NULL;
  size_t hnfsize[ARITHMETIC_FILTER_DIM], hpfsize[ARITHMETIC_FILTER_DIM];
  int issigclip=(operator==ARITHMETIC_OP_FILTER_SIGCLIP_MEAN
                 || operator==ARITHMETIC_OP_FILTER_SIGCLIP_MEDIAN);


  /* Get the input's number of dimensions. */
  afp.input=operands_pop(p, token);
  afp.operator=operator;
  ndim=afp.input->ndim;
  afp.hnfsize=hnfsize;
  afp.hpfsize=hpfsize;
  afp.fsize=fsize;


  /* A small sanity check. */
  if(ndim>ARITHMETIC_FILTER_DIM)
    error(EXIT_FAILURE, 0, "%s: currently only datasets with less than "
          "%d dimensions are acceptable. The input has %zu dimensions",
          __func__, ARITHMETIC_FILTER_DIM, ndim);


  /* A zero value for checking the value of input widths. */
  zero=gal_data_alloc(NULL, GAL_TYPE_INT32, 1, &one, NULL, 1, -1, 1, NULL,
                      NULL, NULL);


  /* Based on the first popped operand's dimensions and the operator, of
     pop the necessary number of operands. */
  nparams = ndim + (issigclip ? 2 : 0 );
  for(i=0;i<nparams;++i)
    {
      /* Add this to the list of parameters. */
      gal_list_data_add(&params_list, operands_pop(p, token));

      /* Make sure it only has a single element. */
      if(params_list->size!=1)
        error(EXIT_FAILURE, 0, "the parameters given to the filtering "
              "operators can only be numbers. Operand number %zu after "
              "the main input has %zu elements, so its an array", i,
              params_list->size);
    }


  /* If this is a sigma-clipping filter, the top two operands are the
     sigma-clipping parameters. */
  if(issigclip)
    {
      /* Read the sigma-clipping multiple (first element in the list). */
      tmp=gal_list_data_pop(&params_list);
      tmp=gal_data_copy_to_new_type_free(tmp, GAL_TYPE_FLOAT32);
      afp.sclip_multip=*(float *)(tmp->array);
      gal_data_free(tmp);

      /* Read the sigma-clipping termination parameter. */
      tmp=gal_list_data_pop(&params_list);
      tmp=gal_data_copy_to_new_type_free(tmp, GAL_TYPE_FLOAT32);
      afp.sclip_param=*(float *)(tmp->array);
      gal_data_free(tmp);
    }

  /* If the input only has one element, it is most probably and error (the
     user has confused the parameters with the input dataset). So report a
     warning filtering makes no sense, so don't waste time, just add the
     input onto the stack. */
  if(afp.input->size==1)
    {
      /* Inform the user that this is suspicious. */
      if(p->cp.quiet==0)
        error(EXIT_SUCCESS, 0, "WARNING: the first popped operand to the "
              "filtering operators has a single element! This is most "
              "probably a typo in the order of operands! Recall that the "
              "filtering operators need the main input image/cube as the "
              "first popped operand");

      /* Set the input as the output. */
      afp.out=afp.input;
    }
  else
    {
      /* Allocate an array for the size of the filter and fill it in. The
         values must be written in the inverse order since the user gives
         dimensions with the FITS standard. */
      i=ndim-1;
      for(tmp=params_list; tmp!=NULL; tmp=tmp->next)
        {
          /* Make sure the user has given an integer type. */
          if(tmp->type==GAL_TYPE_FLOAT32 || tmp->type==GAL_TYPE_FLOAT64)
            error(EXIT_FAILURE, 0, "lengths of filter along dimensions "
                  "must be integer values, not floats. The given length "
                  "along dimension %zu is a float", ndim-i);

          /* Make sure it isn't negative. */
          comp=gal_arithmetic(GAL_ARITHMETIC_OP_GT, 1, 0, tmp, zero);
          if( *(uint8_t *)(comp->array) == 0 )
            error(EXIT_FAILURE, 0, "lengths of filter along dimensions "
                  "must be positive. The given length in dimension %zu"
                  "is either zero or negative", ndim-i);
          gal_data_free(comp);

          /* Convert the input into size_t and put it into the array that
             keeps the filter size. */
          tmp2=gal_data_copy_to_new_type(tmp, GAL_TYPE_SIZE_T);
          fsize[ i ] = *(size_t *)(tmp2->array);
          gal_data_free(tmp2);

          /* If the width is larger than the input's size, change the width
             to the input's size. */
          if( fsize[i] > afp.input->dsize[i] )
            error(EXIT_FAILURE, 0, "%s: the filter size along dimension "
                  "%zu (%zu) is greater than the input's length in that "
                  "dimension (%zu)", __func__, i, fsize[i],
                  afp.input->dsize[i]);

          /* Go onto the previous dimension. */
          --i;
        }


      /* Set the half filter sizes. Note that when the size is an odd
         number, the number of pixels before and after the actual pixel are
         equal, but for an even number, we will look into one element more
         when looking before than the ones after. */
      for(i=0;i<ndim;++i)
        {
          if( fsize[i]%2 )
            hnfsize[i]=hpfsize[i]=fsize[i]/2;
          else
            { hnfsize[i]=fsize[i]/2; hpfsize[i]=fsize[i]/2-1; }
        }

      /* For a test.
      printf("fsize: %zu, %zu\n", fsize[0], fsize[1]);
      printf("hnfsize: %zu, %zu\n", hnfsize[0], hnfsize[1]);
      printf("hpfsize: %zu, %zu\n", hpfsize[0], hpfsize[1]);
      */


      /* Set the type of the output dataset. */
      switch(operator)
        {
        case ARITHMETIC_OP_FILTER_MEDIAN:
        case ARITHMETIC_OP_FILTER_SIGCLIP_MEDIAN:
          type=afp.input->type;
          break;

        case ARITHMETIC_OP_FILTER_MEAN:
        case ARITHMETIC_OP_FILTER_SIGCLIP_MEAN:
          type=GAL_TYPE_FLOAT64;
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s to "
                "fix the problem. The 'operator' code %d is not recognized",
                PACKAGE_BUGREPORT, __func__, operator);
        }


      /* Allocate the output dataset. Note that filtering doesn't change
         the units of the dataset. */
      afp.out=gal_data_alloc(NULL, type, ndim, afp.input->dsize,
                             afp.input->wcs, 0, afp.input->minmapsize,
                             afp.input->quietmmap, NULL, afp.input->unit,
                             NULL);


      /* Spin off threads for each pixel. */
      gal_threads_spin_off(arithmetic_filter, &afp, afp.input->size,
                           p->cp.numthreads, p->cp.minmapsize,
                           p->cp.quietmmap);
    }


  /* Add the output to the top of the stack. */
  operands_add(p, NULL, afp.out);

  /* Clean up and add the output on top of the stack. */
  gal_data_free(zero);
  gal_list_data_free(params_list);
  if(afp.input!=afp.out) gal_data_free(afp.input); /* Single-element. */
}




















/***************************************************************/
/*************            Other functions          *************/
/***************************************************************/
static int
arithmetic_binary_sanity_checks(gal_data_t *in, gal_data_t *conn,
                                char *operator)
{
  int conn_int;

  /* Do proper sanity checks on 'conn'. */
  if(conn->size!=1)
    error(EXIT_FAILURE, 0, "the first popped operand to '%s' must be a "
          "single number. However, it has %zu elements", operator,
          conn->size);
  if(conn->type==GAL_TYPE_FLOAT32 || conn->type==GAL_TYPE_FLOAT64)
    error(EXIT_FAILURE, 0, "the first popped operand to '%s' is the "
          "connectivity (a value between 1 and the number of dimensions) "
          "therefore, it must NOT be a floating point", operator);

  /* Convert the connectivity value to a 32-bit integer and read it in and
     make sure it is not larger than the number of dimensions. */
  conn=gal_data_copy_to_new_type_free(conn, GAL_TYPE_INT32);
  conn_int = *((int32_t *)(conn->array));
  if(conn_int>in->ndim)
    error(EXIT_FAILURE, 0, "the first popped operand of '%s' (%d) is "
          "larger than the number of dimensions in the second-popped "
          "operand (%zu)", operator, conn_int, in->ndim);

  /* Make sure the array has an unsigned 8-bit type. */
  if(in->type!=GAL_TYPE_UINT8)
    error(EXIT_FAILURE, 0, "the second popped operand of '%s' has a type "
          "of %s. However, it must be a binary dataset (only being equal "
          "to zero is checked). You can use the 'uint8' operator for type "
          "conversion, alternatively, if all your values are positive "
          "and floating point, you can use '0 gt', if you want non-blank "
          "values you can use 'isblank not' and many other operators that "
          "produce a binary output", operator, gal_type_name(in->type, 1));

  /* Clean up and return the integer value of 'conn'. */
  gal_data_free(conn);
  return conn_int;
}





static void
arithmetic_erode_dilate(struct arithmeticparams *p, char *token, int op)
{
  int conn_int;

  /* Pop the two necessary operands. */
  gal_data_t *conn = operands_pop(p, token);
  gal_data_t *in   = operands_pop(p, token);

  /* Do the sanity checks. */
  conn_int=arithmetic_binary_sanity_checks(in, conn, token);

  /* Do the operation. */
  switch(op)
    {
    case ARITHMETIC_OP_ERODE:  gal_binary_erode(in,  1, conn_int, 1); break;
    case ARITHMETIC_OP_DILATE: gal_binary_dilate(in, 1, conn_int, 1); break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
            "problem. The operator code %d not recognized", __func__,
            PACKAGE_BUGREPORT, op);
    }

  /* Push the result onto the stack. */
  operands_add(p, NULL, in);
}





static void
arithmetic_number_neighbors(struct arithmeticparams *p, char *token, int op)
{
  int conn_int;

  /* Pop the two necessary operands. */
  gal_data_t *conn = operands_pop(p, token);
  gal_data_t *in   = operands_pop(p, token);

  /* Do the sanity checks and do the job. */
  conn_int=arithmetic_binary_sanity_checks(in, conn, token);
  in=gal_binary_number_neighbors(in, conn_int, 1);

  /* Push the result onto the stack. */
  operands_add(p, NULL, in);
}





static void
arithmetic_connected_components(struct arithmeticparams *p, char *token)
{
  int conn_int;
  gal_data_t *out=NULL;

  /* Pop the two necessary operands. */
  gal_data_t *conn = operands_pop(p, token);
  gal_data_t *in   = operands_pop(p, token);

  /* Basic sanity checks. */
  conn_int=arithmetic_binary_sanity_checks(in, conn, token);

  /* Do the connected components labeling. */
  gal_binary_connected_components(in, &out, conn_int);

  /* Push the result onto the stack. */
  operands_add(p, NULL, out);

  /* Clean up ('conn' was freed in the sanity check). */
  gal_data_free(in);
}





static void
arithmetic_fill_holes(struct arithmeticparams *p, char *token)
{
  int conn_int;

  /* Pop the two necessary operands. */
  gal_data_t *conn = operands_pop(p, token);
  gal_data_t *in   = operands_pop(p, token);

  /* Basic sanity checks. */
  conn_int=arithmetic_binary_sanity_checks(in, conn, token);

  /* Fill the holes. */
  gal_binary_holes_fill(in, conn_int, -1);

  /* Push the result onto the stack. */
  operands_add(p, NULL, in);
}





static void
arithmetic_invert(struct arithmeticparams *p, char *token)
{
  gal_data_t *in = operands_pop(p, token);

  uint8_t *u8  = in->array, *u8f  = u8  + in->size;
  uint8_t *u16 = in->array, *u16f = u16 + in->size;
  uint8_t *u32 = in->array, *u32f = u32 + in->size;
  uint8_t *u64 = in->array, *u64f = u64 + in->size;

  /* Do the inversion based on type. */
  switch(in->type)
    {
    case GAL_TYPE_UINT8:  do *u8  = UINT8_MAX-*u8;   while(++u8<u8f);   break;
    case GAL_TYPE_UINT16: do *u16 = UINT16_MAX-*u16; while(++u16<u16f); break;
    case GAL_TYPE_UINT32: do *u32 = UINT32_MAX-*u32; while(++u32<u32f); break;
    case GAL_TYPE_UINT64: do *u64 = UINT64_MAX-*u64; while(++u64<u64f); break;
    default:
      error(EXIT_FAILURE, 0, "'invert' operand has %s type. 'invert' can "
            "only take unsigned integer types.\n\nYou can use any of the "
            "'uint8', 'uint16', 'uint32', or 'uint64' operators to chage "
            "the type before calling 'invert'",
            gal_type_name(in->type, 1));
    }

  /* Push the result onto the stack. */
  operands_add(p, NULL, in);
}





#define INTERPOLATE_REGION(TYPE,OP,FUNC) {                              \
    TYPE mm, b, *a=in->array, *m=minmax->array;                         \
    FUNC(in->type, &mm);                                                \
    gal_blank_write(&b, in->type);                                      \
    for(i=0;i<minmax->size;++i) m[i]=mm;                                \
    for(i=0;i<in->size;++i)                                             \
      {                                                                 \
        if( l[i]>0 )                                                    \
          GAL_DIMENSION_NEIGHBOR_OP(i, in->ndim, in->dsize, in->ndim,   \
                                    dinc,                               \
            {                                                           \
              if(in->type==GAL_TYPE_FLOAT32 || in->type==GAL_TYPE_FLOAT64) \
                { if( a[nind] OP m[l[i]] ) m[l[i]]=a[nind]; }           \
              else                                                      \
                { if( a[nind]!=b && a[nind] OP m[l[i]] ) m[l[i]]=a[nind]; } \
            });                                                         \
      }                                                                 \
    for(i=0;i<in->size;++i) if( l[i]>0 ) { a[i]=m[l[i]]; }              \
}
#define INTERPOLATE_REGION_OP(TYPE) {                                   \
    switch(operator)                                                    \
      {                                                                 \
      case ARITHMETIC_OP_INTERPOLATE_MINOFREGION:                       \
        INTERPOLATE_REGION(TYPE,<,gal_type_max); break;                 \
      case ARITHMETIC_OP_INTERPOLATE_MAXOFREGION:                       \
        INTERPOLATE_REGION(TYPE,>,gal_type_min); break;                 \
      default:                                                          \
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to " \
              "fix the problem. The operator code %d isn't recognized", \
              __func__, PACKAGE_BUGREPORT, operator);                   \
      }                                                                 \
  }
static void
arithmetic_interpolate_region(struct arithmeticparams *p,
                              int operator, char *token)
{
  /* Pop the dataset to interpolate. */
  int32_t *l;
  gal_data_t *minmax;
  gal_data_t *lab=NULL, *flag;
  size_t i, *con, *dinc, numlabs;

  /* First pop the number of nearby neighbors.*/
  gal_data_t *connectivity = operands_pop(p, token);

  /* Then pop the actual dataset to interpolate. */
  gal_data_t *in = operands_pop(p, token);

  /* Do proper sanity checks on 'con'. */
  if(connectivity->size!=1)
    error(EXIT_FAILURE, 0, "the first popped operand to the "
          "'interpolate-XXXofregion' operators must be a single number. "
          "However, it has %zu elements", connectivity->size);
  if( connectivity->type==GAL_TYPE_FLOAT32
      || connectivity->type==GAL_TYPE_FLOAT64)
    error(EXIT_FAILURE, 0, "the first popped operand to "
          "'interpolate-XXXofregion' operators is the connectivity to "
          "define connected blank regions (a counter, an integer, with "
          "a maximum of the number of dimensions of the input). It must "
          "NOT be a floating point.\n\n"
          "If its already an integer, but in a floating point container, "
          "you can use the 'int32' operator to convert it to a 32-bit "
          "integer for example");

  /* Convert connectivity to an integer type and make sure its not larger
     than dimensions of the input. */
  connectivity=gal_data_copy_to_new_type_free(connectivity,
                                              GAL_TYPE_SIZE_T);
  con=connectivity->array;
  if(con[0]>in->ndim)
    error(EXIT_FAILURE, 0, "the first popped operand to "
          "'interpolate-XXXofregion' operators must not be larger than "
          "the number of dimensions of the input. The connectivity is "
          "used to define connected blank regions. For example in a "
          "2D dataset, a connectivity of 1 corresponds to 4-connected "
          "neighbors and connectivity 2 corresponds to 8-connected "
          "neighbors");

  /* First make sure the input has blank values. If it doesn't, don't waste
     time, just return it. */
  if( gal_blank_present(in, 1)==0 )
    { operands_add(p, NULL, in); return; }

  /* Build a binary image with the blank regions masked and label them,
     then free the flagged array. */
  flag=gal_blank_flag(in);
  numlabs=gal_binary_connected_components(flag, &lab, con[0]);
  gal_data_free(flag);

  /* Allocate array to keep maximum values for each region. Just note that
     because we count the regions from 1, but the indexs from 0, we'll
     allocate one extra point. */
  ++numlabs;
  minmax=gal_data_alloc(NULL, in->type, 1, &numlabs, NULL, 0, in->minmapsize,
                        in->quietmmap, NULL, NULL, NULL);

  /* Parse the dataset elements for NaNs. */
  l=lab->array;
  dinc=gal_dimension_increment(in->ndim, in->dsize);
  switch(in->type)
    {
    case GAL_TYPE_UINT8:   INTERPOLATE_REGION_OP(uint8_t);  break;
    case GAL_TYPE_INT8:    INTERPOLATE_REGION_OP(int8_t);   break;
    case GAL_TYPE_UINT16:  INTERPOLATE_REGION_OP(uint16_t); break;
    case GAL_TYPE_INT16:   INTERPOLATE_REGION_OP(int16_t);  break;
    case GAL_TYPE_UINT32:  INTERPOLATE_REGION_OP(uint32_t); break;
    case GAL_TYPE_INT32:   INTERPOLATE_REGION_OP(int32_t);  break;
    case GAL_TYPE_FLOAT32: INTERPOLATE_REGION_OP(float);    break;
    case GAL_TYPE_FLOAT64: INTERPOLATE_REGION_OP(double);   break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the problem. The code %d is not a recognized data "
            "type", __func__, PACKAGE_BUGREPORT, in->type);
    }

  /* For tests.
  gal_fits_img_write(lab, "test-out.fits", NULL, NULL);
  gal_fits_img_write(in, "test-out.fits", NULL, NULL);
  printf("\n...%s...\n", __func__); exit(0);
  */

  /* Clean up. */
  free(dinc);
  gal_data_free(lab);
  gal_data_free(minmax);
  gal_data_free(connectivity);

  /* Push the interpolated dataset onto the stack. */
  operands_add(p, NULL, in);
}





static void
arithmetic_interpolate(struct arithmeticparams *p, int operator, char *token)
{
  gal_data_t *interpolated;
  int num_int, interpop=GAL_ARITHMETIC_OP_INVALID;

  /* First pop the number of nearby neighbors. */
  gal_data_t *num = operands_pop(p, token);

  /* Then pop the actual dataset to interpolate. */
  gal_data_t *in = operands_pop(p, token);

  /* Do proper sanity checks on 'num'. */
  if(num->size!=1)
    error(EXIT_FAILURE, 0, "the first popped operand to "
          "'interpolate-XXXXXngb' operators must be a single number. "
          "However, it has %zu elements", num->size);
  if(num->type==GAL_TYPE_FLOAT32 || num->type==GAL_TYPE_FLOAT64)
    error(EXIT_FAILURE, 0, "the first popped operand to "
          "'interpolate-XXXXXngb' operators is the number of nearby "
          "neighbors (a counter, an integer). It must NOT be a floating "
          "point.\n\n"
          "If its already an integer, but in a floating point container, "
          "you can use the 'int32' operator to convert it to a 32-bit "
          "integer for example");

  /* Convert the given number to a 32-bit integer and read it in. */
  num=gal_data_copy_to_new_type_free(num, GAL_TYPE_INT32);
  num_int = *((int32_t *)(num->array));

  /* Set the interpolation operator. */
  switch(operator)
    {
    case ARITHMETIC_OP_INTERPOLATE_MINNGB:
      interpop=GAL_INTERPOLATE_NEIGHBORS_FUNC_MIN;
      break;
    case ARITHMETIC_OP_INTERPOLATE_MAXNGB:
      interpop=GAL_INTERPOLATE_NEIGHBORS_FUNC_MAX;
      break;
    case ARITHMETIC_OP_INTERPOLATE_MEANNGB:
      interpop=GAL_INTERPOLATE_NEIGHBORS_FUNC_MEAN;
      break;
    case ARITHMETIC_OP_INTERPOLATE_MEDIANNGB:
      interpop=GAL_INTERPOLATE_NEIGHBORS_FUNC_MEDIAN;
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the problem. The operator code %d isn't recognized",
            __func__, PACKAGE_BUGREPORT, operator);
    }

  /* Call the interpolation function. */
  interpolated=gal_interpolate_neighbors(in, NULL, p->cp.interpmetric,
                                         num_int, p->cp.numthreads,
                                         1, 0, interpop);

  /* Clean up and push the interpolated array onto the stack. */
  gal_data_free(in);
  gal_data_free(num);
  operands_add(p, NULL, interpolated);
}





static void
arithmetic_collapse_single_value(gal_data_t *input, char *counter,
                                 char *opstr, char *description,
                                 int checkint)
{
  if( input->ndim!=1 || input->size!=1)
    error(EXIT_FAILURE, 0, "%s popped operand of 'collapse-%s*' "
          "operators (%s) must be a single number (single-element, "
          "one-dimensional dataset). But it has %zu dimension(s) and "
          "%zu element(s).", counter, opstr, description, input->ndim,
          input->size);
  if(checkint)
    if(input->type==GAL_TYPE_FLOAT32 || input->type==GAL_TYPE_FLOAT64)
      error(EXIT_FAILURE, 0, "%s popped operand of 'collapse-%s*' "
            "operators (%s) must have an integer type, but it has a "
            "floating point type ('%s')", counter, opstr, description,
            gal_type_name(input->type,1));
}





static void
arithmetic_collapse(struct arithmeticparams *p, char *token, int operator)
{
  long dim;
  float p1=NAN, p2=NAN;
  int qmm=p->cp.quietmmap;
  gal_data_t *dimension=NULL, *input=NULL;
  size_t nt=p->cp.numthreads, mms=p->cp.minmapsize;
  gal_data_t *collapsed=NULL, *param1=NULL, *param2=NULL;


  /* First popped operand is the dimension. */
  dimension = operands_pop(p, token);


  /* If we are on any of the sigma-clipping operators, we need to pop two
     more operands. */
  if(    operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_MAD
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_STD
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_MEAN
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_MEDIAN
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_NUMBER
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_MAD
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_STD
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_MEAN
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_MEDIAN
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_NUMBER
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MAD
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_STD
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MEAN
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MEDIAN
      || operator==ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_NUMBER
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MAD
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_STD
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MEAN
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MEDIAN
      || operator==ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_NUMBER )
    {
      param2=operands_pop(p, token); /* Termination criteria. */
      param1=operands_pop(p, token); /* Multiple of sigma. */
    }


  /* Final popped operand is the desired input dataset. */
  input = operands_pop(p, token);


  /* Sanity checks. */
  arithmetic_collapse_single_value(dimension, "first", "",
                                   "dimension to collapse", 1);
  dimension=gal_data_copy_to_new_type_free(dimension, GAL_TYPE_LONG);
  dim=((long *)(dimension->array))[0];
  if(dim<=0)
    error(EXIT_FAILURE, 0, "first popped operand of 'collapse-*' "
          "operators (dimension to collapse) must be positive (larger "
          "than zero), it is %ld", dim);
  if(dim > input->ndim)
    error(EXIT_FAILURE, 0, "input dataset to '%s' has %zu dimension(s), "
          "but you have asked to collapse along dimension %ld", token,
          input->ndim, dim);
  if(param1)
    {
      arithmetic_collapse_single_value(param1, "third", "sigclip-",
                                       "sigma-clip's multiple of sigma",
                                       0);
      arithmetic_collapse_single_value(param2, "second", "sigclip-",
                                       "sigma-clip's termination param",
                                       0);
      param1=gal_data_copy_to_new_type_free(param1, GAL_TYPE_FLOAT32);
      param2=gal_data_copy_to_new_type_free(param2, GAL_TYPE_FLOAT32);
      p1=((float *)(param1->array))[0];
      p2=((float *)(param2->array))[0];
      if(p1<=0 || p2<=0)
        error(EXIT_FAILURE, 0, "third and second popped operands of "
              "'collapse-sigclip-*' operators (sigma multiple and "
              "termination criteria) must be positive (larger than "
              "zero), but they are %g and %g", p1, p2);
    }

  /* If a WCS structure has been read, we'll need to pass it to
     'gal_dimension_collapse', so it modifies it respectively. */
  if(p->wcs_collapsed==0)
    {
      p->wcs_collapsed=1;
      input->wcs=p->refdata.wcs;
    }


  /* Run the relevant library function. */
  switch(operator)
    {
    case ARITHMETIC_OP_COLLAPSE_SUM:
      collapsed=gal_dimension_collapse_sum(input, input->ndim-dim, NULL);
      break;

    case ARITHMETIC_OP_COLLAPSE_MEAN:
      collapsed=gal_dimension_collapse_mean(input, input->ndim-dim, NULL);
      break;

    case ARITHMETIC_OP_COLLAPSE_NUMBER:
      collapsed=gal_dimension_collapse_number(input, input->ndim-dim);
      break;

    case ARITHMETIC_OP_COLLAPSE_MIN:
      collapsed=gal_dimension_collapse_minmax(input, input->ndim-dim, 0);
      break;

    case ARITHMETIC_OP_COLLAPSE_MAX:
      collapsed=gal_dimension_collapse_minmax(input, input->ndim-dim, 1);
      break;

    case ARITHMETIC_OP_COLLAPSE_MEDIAN:
      collapsed=gal_dimension_collapse_median(input, input->ndim-dim,
                                              nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_MAD:
      collapsed=gal_dimension_collapse_mclip_std(input, input->ndim-dim,
                                                 p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MAD:
      collapsed=gal_dimension_collapse_mclip_fill_std(input,
                                input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_STD:
      collapsed=gal_dimension_collapse_mclip_std(input, input->ndim-dim,
                                                 p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_STD:
      collapsed=gal_dimension_collapse_mclip_fill_std(input,
                                input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_MEAN:
      collapsed=gal_dimension_collapse_mclip_mean(input, input->ndim-dim,
                                                  p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MEAN:
      collapsed=gal_dimension_collapse_mclip_fill_mean(input,
                                 input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_MEDIAN:
      collapsed=gal_dimension_collapse_mclip_median(input, input->ndim-dim,
                                                    p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MEDIAN:
      collapsed=gal_dimension_collapse_mclip_fill_median(input,
                                   input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_NUMBER:
      collapsed=gal_dimension_collapse_mclip_number(input, input->ndim-dim,
                                                    p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_NUMBER:
      collapsed=gal_dimension_collapse_mclip_fill_number(input,
                                   input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_MAD:
      collapsed=gal_dimension_collapse_sclip_std(input, input->ndim-dim,
                                                 p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MAD:
      collapsed=gal_dimension_collapse_sclip_fill_std(input,
                                 input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_STD:
      collapsed=gal_dimension_collapse_sclip_std(input, input->ndim-dim,
                                                 p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_STD:
      collapsed=gal_dimension_collapse_sclip_fill_std(input,
                                input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_MEAN:
      collapsed=gal_dimension_collapse_sclip_mean(input, input->ndim-dim,
                                                  p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MEAN:
      collapsed=gal_dimension_collapse_sclip_fill_mean(input,
                                 input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_MEDIAN:
      collapsed=gal_dimension_collapse_sclip_median(input, input->ndim-dim,
                                                    p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MEDIAN:
      collapsed=gal_dimension_collapse_sclip_fill_median(input,
                                   input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_NUMBER:
      collapsed=gal_dimension_collapse_sclip_number(input, input->ndim-dim,
                                                    p1, p2, nt, mms, qmm);
      break;

    case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_NUMBER:
      collapsed=gal_dimension_collapse_sclip_fill_number(input,
                                   input->ndim-dim, p1, p2, nt, mms, qmm);
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the problem. The operator code %d is not recognized",
            __func__, PACKAGE_BUGREPORT, operator);
    }


  /* If a WCS structure existed, a modified WCS is now present in
     'collapsed->wcs'. So we'll let the freeing of 'input' free the old
     'p->refdata.wcs' structure and we'll put the new one there, then we'll
     set 'collapsed->wcs' to 'NULL', so the new one isn't freed. */
  if(collapsed->wcs)
    {
      p->refdata.wcs = collapsed->wcs;
      collapsed->wcs = NULL;
    }


  /* We'll also need to correct the size of the reference dataset if it
     hasn't been corrected yet. We'll use 'memcpy' to write the new 'dsize'
     values into the old ones. The dimensions have decreased, so we won't
     be writing outside of allocated space that 'p->refdata.dsize' points
     to. */
  if( p->refdata.ndim != collapsed->ndim )
    {
      p->refdata.ndim -= 1;
      memcpy( p->refdata.dsize, collapsed->dsize,
              p->refdata.ndim * (sizeof *p->refdata.dsize) );
    }


  /* Clean up and add the collapsed dataset to the top of the operands. */
  gal_data_free(input);
  gal_data_free(dimension);
  operands_add(p, NULL, collapsed);
}





void
arithmetic_tofile(struct arithmeticparams *p, char *token, int freeflag)
{
  /* Pop the top dataset. */
  gal_data_t *popped = operands_pop(p, token);
  char *filename = &token[ freeflag
                           ? OPERATOR_PREFIX_LENGTH_TOFILEFREE
                           : OPERATOR_PREFIX_LENGTH_TOFILE     ];

  /* Make sure that if the file exists, it is deleted. */
  gal_checkset_writable_remove(filename, NULL, p->cp.keep,
                               p->cp.dontdelete);

  /* Save it to a file. */
  popped->wcs=p->refdata.wcs;
  if(popped->ndim==1 && p->onedasimage==0)
    gal_table_write(popped, NULL, NULL, p->cp.tableformat, filename,
                    "ARITHMETIC", 0);
  else
    gal_fits_img_write(popped, filename, NULL, PROGRAM_NAME);
  if(!p->cp.quiet)
    printf(" - Write: %s\n", filename);

  /* Reset the WCS to NULL and put it back on the stack. */
  popped->wcs=NULL;
  if(freeflag) gal_data_free(popped);
  else         operands_add(p, NULL, popped);
}





void
arithmetic_add_dimension(struct arithmeticparams *p, char *token,
                         int operator)
{
  size_t one=1;
  gal_data_t *ref=NULL, *out=NULL;
  size_t i, j, num, dsize[3], nbytes=0;
  gal_data_t *tmp = operands_pop(p, token);

  /* Make sure the first operand is a number. */
  if(tmp->size!=1)
    error(EXIT_FAILURE, 0, "first popped operand to '%s' must be a "
          "number (specifying how many datasets to use)", token);

  /* Put the first-popped value into 'num'. */
  tmp=gal_data_copy_to_new_type_free(tmp, GAL_TYPE_SIZE_T);
  num=*(size_t *)(tmp->array);
  gal_data_free(tmp);

  /* Pop all the datasets and put them in a list. */
  for(i=0;i<num;++i)
    {
      /* Pop the operand. */
      tmp=operands_pop(p, token);

      /* Things that differ from the first dataset and the rest. */
      if(out) /* Not the first. */
        {
          /* All entries have to have the same data type. */
          if(tmp->type!=out->type)
            error(EXIT_FAILURE, 0, "the operands to '%s' must have "
                  "the same data type (the first popped operand has a "
                  "'%s' type, but the popped operand number %zu has "
                  "type '%s')", token, gal_type_name(out->type, 1), i+1,
                  gal_type_name(tmp->type, 1));

          /* All entries should also have the same dimension. */
          if( gal_dimension_is_different(ref, tmp) )
            error(EXIT_FAILURE, 0, "the operands to '%s' must have the "
                  "same size in all dimensions", token);
        }
      else  /* First popped operand. */
        {
          /* First popped operand, do necessary basic checks here. */
          switch(operator)
            {
            case ARITHMETIC_OP_ADD_DIMENSION_SLOW:
              dsize[0]=num;
              dsize[1]=tmp->dsize[0];
              dsize[2]=tmp->dsize[1];
              nbytes=gal_type_sizeof(tmp->type)*tmp->size;
              if(tmp->ndim!=2)
                error(EXIT_FAILURE, 0, "currently only 2-dimensional "
                      "datasets are acceptable for '%s', please get in "
                      "touch with us at %s so we add functionality for "
                      "different dimensions", token, PACKAGE_BUGREPORT);
              break;
            case ARITHMETIC_OP_ADD_DIMENSION_FAST:
              dsize[1]=num;
              dsize[0]=tmp->dsize[0];
              nbytes=gal_type_sizeof(tmp->type);
              if(tmp->ndim!=1)
                error(EXIT_FAILURE, 0, "currently only 1-dimensional "
                      "datasets are acceptable for '%s', please get in "
                      "touch with us at %s so we add functionality for "
                      "different dimensions", token, PACKAGE_BUGREPORT);
              break;
            default:
              error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at "
                    "%s to address the problem. The operator code '%d' "
                    "isn't recognized", __func__, PACKAGE_BUGREPORT,
                    operator);
            }

          /* Allocate the size-reference dataset to simplify checking of
             the sizes for each new dataset. This is done because this
             dataset that is actually being popped will be freed right
             after its values have been written into the output. We don't
             care about any data from this dataset, and to avoid wasting
             memory, we will simply allocate a one-element dataset. But
             we'll many set its sizes to the size of the popped dataset. */
          ref=gal_data_alloc(NULL, tmp->type, 1, &one, NULL, 0, -1, 1,
                             NULL, NULL, NULL);
          free(ref->dsize);
          ref->ndim=tmp->ndim;
          ref->dsize=gal_pointer_allocate(GAL_TYPE_SIZE_T, ref->ndim, 0,
                                          __func__, "ref->dsize");
          for(j=0;j<ref->ndim;++j) ref->dsize[j]=tmp->dsize[j];

          /* Allocate the output dataset. */
          out = gal_data_alloc(NULL, tmp->type, tmp->ndim+1, dsize, NULL,
                               0, p->cp.minmapsize, p->cp.quietmmap, NULL,
                               NULL, NULL);
        }

      /* Copy the dataset into the higher-dimensional output. */
      switch(operator)
        {
        case ARITHMETIC_OP_ADD_DIMENSION_SLOW:
          memcpy(gal_pointer_increment(out->array, (num-1-i)*tmp->size,
                                       tmp->type),
                 tmp->array, nbytes);
          break;
        case ARITHMETIC_OP_ADD_DIMENSION_FAST:
          for(j=0;j<tmp->size;++j)
            memcpy(gal_pointer_increment(out->array, j*num+num-i-1, out->type),
                   gal_pointer_increment(tmp->array, j, out->type), nbytes);

          break;
        }

      /* Clean up. */
      gal_data_free(tmp);
    }

  /* Put the higher-dimensional output on the operands stack. */
  operands_add(p, NULL, out);

  /* Clean up. */
  gal_data_free(ref);
}





void
arithmetic_repeat(struct arithmeticparams *p, char *token, int operator)
{
  size_t i, num;
  gal_data_t *ttmp, *tmp = operands_pop(p, token);

  /* Make sure the first operand is a number. */
  if(tmp->size!=1)
    error(EXIT_FAILURE, 0, "first popped operand to '%s' must be a "
          "number (specifying how many datasets to use)", token);

  /* Put the first-popped value into 'num'. */
  tmp=gal_data_copy_to_new_type_free(tmp, GAL_TYPE_SIZE_T);
  num=*(size_t *)(tmp->array);
  gal_data_free(tmp);

  /* Pop the dataset to be copied. */
  tmp=operands_pop(p, token);

  /* Add it to the stack for the desired number of times. */
  for(i=0; i<num; ++i)
    {
      ttmp=gal_data_copy(tmp);
      operands_add(p, NULL, ttmp);
    }

  /* Clean up. */
  gal_data_free(tmp);
}


















/***************************************************************/
/*************      Reverse Polish algorithm       *************/
/***************************************************************/
static int
arithmetic_set_operator(char *string, size_t *num_operands, int *inlib)
{
  /* Use the library's main function for its own operators. */
  int op = gal_arithmetic_set_operator(string, num_operands);

  /* Mark if this operator is in the library or not. */
  *inlib = op==GAL_ARITHMETIC_OP_INVALID ? 0 : 1;

  /* If its not a library operator, check if its an internal operator. */
  if(op==GAL_ARITHMETIC_OP_INVALID)
    {
      /* See if its a custom operator to the Arithmetic program. */
      if      (!strcmp(string, "filter-mean"))
        { op=ARITHMETIC_OP_FILTER_MEAN;           *num_operands=0; }
      else if (!strcmp(string, "filter-median"))
        { op=ARITHMETIC_OP_FILTER_MEDIAN;         *num_operands=0; }
      else if (!strcmp(string, "filter-sigclip-mean"))
        { op=ARITHMETIC_OP_FILTER_SIGCLIP_MEAN;   *num_operands=0; }
      else if (!strcmp(string, "filter-sigclip-median"))
        { op=ARITHMETIC_OP_FILTER_SIGCLIP_MEDIAN; *num_operands=0; }
      else if (!strcmp(string, "erode"))
        { op=ARITHMETIC_OP_ERODE;                 *num_operands=0; }
      else if (!strcmp(string, "dilate"))
        { op=ARITHMETIC_OP_DILATE;                *num_operands=0; }
      else if (!strcmp(string, "number-neighbors"))
        { op=ARITHMETIC_OP_NUMBER_NEIGHBORS;      *num_operands=0; }
      else if (!strcmp(string, "connected-components"))
        { op=ARITHMETIC_OP_CONNECTED_COMPONENTS;  *num_operands=0; }
      else if (!strcmp(string, "fill-holes"))
        { op=ARITHMETIC_OP_FILL_HOLES;            *num_operands=0; }
      else if (!strcmp(string, "invert"))
        { op=ARITHMETIC_OP_INVERT;                *num_operands=0; }
      else if (!strcmp(string, "interpolate-minngb"))
        { op=ARITHMETIC_OP_INTERPOLATE_MINNGB;    *num_operands=0; }
      else if (!strcmp(string, "interpolate-maxngb"))
        { op=ARITHMETIC_OP_INTERPOLATE_MAXNGB;    *num_operands=0; }
      else if (!strcmp(string, "interpolate-meanngb"))
        { op=ARITHMETIC_OP_INTERPOLATE_MEANNGB;   *num_operands=0; }
      else if (!strcmp(string, "interpolate-medianngb"))
        { op=ARITHMETIC_OP_INTERPOLATE_MEDIANNGB; *num_operands=0; }
      else if (!strcmp(string, "interpolate-minofregion"))
        { op=ARITHMETIC_OP_INTERPOLATE_MINOFREGION; *num_operands=0; }
      else if (!strcmp(string, "interpolate-maxofregion"))
        { op=ARITHMETIC_OP_INTERPOLATE_MAXOFREGION; *num_operands=0; }
      else if (!strcmp(string, "collapse-sum"))
        { op=ARITHMETIC_OP_COLLAPSE_SUM;          *num_operands=0; }
      else if (!strcmp(string, "collapse-min"))
        { op=ARITHMETIC_OP_COLLAPSE_MIN;          *num_operands=0; }
      else if (!strcmp(string, "collapse-max"))
        { op=ARITHMETIC_OP_COLLAPSE_MAX;          *num_operands=0; }
      else if (!strcmp(string, "collapse-mean"))
        { op=ARITHMETIC_OP_COLLAPSE_MEAN;         *num_operands=0; }
      else if (!strcmp(string, "collapse-number"))
        { op=ARITHMETIC_OP_COLLAPSE_MEDIAN;       *num_operands=0; }
      else if (!strcmp(string, "collapse-median"))
        { op=ARITHMETIC_OP_COLLAPSE_MEDIAN;       *num_operands=0; }
      else if (!strcmp(string, "collapse-madclip-mad"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_MAD;  *num_operands=0; }
      else if (!strcmp(string, "collapse-madclip-std"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_STD;  *num_operands=0; }
      else if (!strcmp(string, "collapse-madclip-mean"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_MEAN; *num_operands=0; }
      else if (!strcmp(string, "collapse-madclip-median"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_MEDIAN;*num_operands=0;}
      else if (!strcmp(string, "collapse-madclip-number"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_NUMBER;*num_operands=0;}
      else if (!strcmp(string, "collapse-sigclip-mad"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_MAD;  *num_operands=0; }
      else if (!strcmp(string, "collapse-sigclip-std"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_STD;  *num_operands=0; }
      else if (!strcmp(string, "collapse-sigclip-mean"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_MEAN; *num_operands=0; }
      else if (!strcmp(string, "collapse-sigclip-median"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_MEDIAN;*num_operands=0;}
      else if (!strcmp(string, "collapse-sigclip-number"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_NUMBER;*num_operands=0;}
      else if (!strcmp(string, "collapse-madclip-fill-mad"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MAD;  *num_operands=0; }
      else if (!strcmp(string, "collapse-madclip-fill-std"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_STD;  *num_operands=0; }
      else if (!strcmp(string, "collapse-madclip-fill-mean"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MEAN; *num_operands=0; }
      else if (!strcmp(string, "collapse-madclip-fill-median"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MEDIAN;*num_operands=0;}
      else if (!strcmp(string, "collapse-madclip-fill-number"))
        { op=ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_NUMBER;*num_operands=0;}
      else if (!strcmp(string, "collapse-sigclip-fill-mad"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MAD;  *num_operands=0; }
      else if (!strcmp(string, "collapse-sigclip-fill-std"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_STD;  *num_operands=0; }
      else if (!strcmp(string, "collapse-sigclip-fill-mean"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MEAN; *num_operands=0; }
      else if (!strcmp(string, "collapse-sigclip-fill-median"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MEDIAN;*num_operands=0;}
      else if (!strcmp(string, "collapse-sigclip-fill-number"))
        { op=ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_NUMBER;*num_operands=0;}
      else if (!strcmp(string, "add-dimension-slow"))
        { op=ARITHMETIC_OP_ADD_DIMENSION_SLOW;    *num_operands=0; }
      else if (!strcmp(string, "add-dimension-fast"))
        { op=ARITHMETIC_OP_ADD_DIMENSION_FAST;    *num_operands=0; }
      else if (!strcmp(string, "repeat"))
        { op=ARITHMETIC_OP_REPEAT;                *num_operands=0; }
      else
        error(EXIT_FAILURE, 0, "the argument '%s' could not be "
              "interpretted as a file name, named dataset, number, "
              "or operator", string);
    }

  /* Return the operator code. */
  return op;
}





static gal_data_t *
arithmetic_prepare_meta(gal_data_t *d1, gal_data_t *d2, gal_data_t *d3)
{
  size_t i;
  gal_data_t *out;

  /* A small sanity check (these operators only need a single
     operator). */
  if(d2 || d3)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
          "find and fix the problem. Meta operators should only "
          "take a single operand, but for some reason, this "
          "hasn't occurred here", __func__, PACKAGE_BUGREPORT);

  /* Allocate the output dataset. */
  out=gal_data_alloc_empty(d1->ndim, d1->minmapsize, d1->quietmmap);

  /* Fill the output dataset and return. */
  for(i=0;i<d1->ndim;++i) out->dsize[i]=d1->dsize[i];
  out->size=d1->size;
  return out;
}





static void
arithmetic_operator_run(struct arithmeticparams *p, int operator,
                        char *operator_string, size_t num_operands,
                        int inlib)
{
  size_t i;
  unsigned int numop;
  int flags = GAL_ARITHMETIC_FLAGS_BASIC;
  gal_data_t *d1=NULL, *d2=NULL, *d3=NULL, *d4=NULL, *d5=NULL;

  /* Set the operating-mode flags if necessary. */
  if(p->cp.quiet) flags |= GAL_ARITHMETIC_FLAG_QUIET;
  if(p->envseed)  flags |= GAL_ARITHMETIC_FLAG_ENVSEED;

  /* If this operator is in the library, we should pop everything here.  */
  if(inlib)
    {
      /* Pop the necessary number of operators. Note that the
         operators are poped from a linked list (which is
         last-in-first-out). So for the operators which need a
         specific order, the first poped operand is actally the
         last (right most, in in-fix notation) input operand.*/
      switch(num_operands)
        {
        case 0:
          break;

        case 1:
          d1=operands_pop(p, operator_string);
          break;

        case 2:
          d2=operands_pop(p, operator_string);
          d1=operands_pop(p, operator_string);
          break;

        case 3:
          d3=operands_pop(p, operator_string);
          d2=operands_pop(p, operator_string);
          d1=operands_pop(p, operator_string);
          break;

        case 4:
          d4=operands_pop(p, operator_string);
          d3=operands_pop(p, operator_string);
          d2=operands_pop(p, operator_string);
          d1=operands_pop(p, operator_string);
          break;

        case 5:
          d5=operands_pop(p, operator_string);
          d4=operands_pop(p, operator_string);
          d3=operands_pop(p, operator_string);
          d2=operands_pop(p, operator_string);
          d1=operands_pop(p, operator_string);
          break;

        case -1:
          /* This case is when the number of operands is itself an
             operand. So except for operators that have high-level
             parameters (like sigma-clipping), the first popped operand
             must be an integer number, we will use that to construct a
             linked list of any number of operands within the single 'd1'
             pointer. */
          numop=pop_number_of_operands(p, operator, operator_string, &d2);
          for(i=0;i<numop;++i)
            gal_list_data_add(&d1, operands_pop(p, operator_string));
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s "
                "to fix the problem. '%zu' is not recognized as an "
                "operand counter (with '%s')", __func__,
                PACKAGE_BUGREPORT, num_operands, operator_string);
        }

      /* Operators with special attention. */
      switch(operator)
        {
         /* Meta operators (where only the information about the pixels is
            relevant, not the pixel values themselves, like the 'index'
            operator). For these (that only accept one operand), we should
            add the first popped operand back on the stack. */
        case GAL_ARITHMETIC_OP_INDEX:
        case GAL_ARITHMETIC_OP_COUNTER:
          operands_add(p, NULL, d1);
          d1=arithmetic_prepare_meta(d1, d2, d3);
          break;

        /* Operators that also modify the reference WCS. */
        case GAL_ARITHMETIC_OP_TRIM:
          d1->wcs=gal_wcs_copy(p->refdata.wcs);
          break;

        /* Operators that need/modify the WCS. */
        case GAL_ARITHMETIC_OP_POOLMAX:
        case GAL_ARITHMETIC_OP_POOLMIN:
        case GAL_ARITHMETIC_OP_POOLSUM:
        case GAL_ARITHMETIC_OP_POOLMEAN:
        case GAL_ARITHMETIC_OP_POOLMEDIAN:

          /* Print warning in case of existing WCS. */
          if(p->refdata.wcs)
            {
              if(p->cp.quiet==0)
                error(EXIT_SUCCESS, 0, "WARNING: the WCS is not currently "
                      "supported for the pooling operators (the output "
                      "will not contain WCS). If it is necessary in your "
                      "usage, please get in touch with us at '%s'. You "
                      "can suppress this warning with the '--quiet' option",
                      PACKAGE_BUGREPORT);
              gal_wcs_free(p->refdata.wcs);
              p->refdata.wcs=NULL;
            }

          /* In case you want to pass the WCS to the library function ...

          d1->wcs=gal_wcs_copy(p->refdata.wcs);

          ... uncomment the line above and comment the whole
          'if(p->refdata.wcs)' check above (so the wcs is not freed).*/
          break;
        }

      /* Run the arithmetic operation. Note that 'gal_arithmetic'
         is a variable argument function (like printf). So the
         number of arguments it uses depend on the operator. So
         when the operator doesn't need three operands, the extra
         arguments will be ignored. */
      operands_add(p, NULL, gal_arithmetic(operator, p->cp.numthreads,
                                           flags, d1, d2, d3, d4, d5));

      /* Operators with special attention afterwards. */
      switch(operator)
        {
        case GAL_ARITHMETIC_OP_TRIM:
          gal_wcs_free(p->refdata.wcs);
          p->refdata.wcs=gal_wcs_copy(p->operands->data->wcs);
          break;
        }
    }

  /* No need to call the arithmetic library, call the proper wrappers
     directly. */
  else
    {
      switch(operator)
        {
        case ARITHMETIC_OP_FILTER_MEAN:
        case ARITHMETIC_OP_FILTER_MEDIAN:
        case ARITHMETIC_OP_FILTER_SIGCLIP_MEAN:
        case ARITHMETIC_OP_FILTER_SIGCLIP_MEDIAN:
          wrapper_for_filter(p, operator_string, operator);
          break;

        case ARITHMETIC_OP_ERODE:
        case ARITHMETIC_OP_DILATE:
          arithmetic_erode_dilate(p, operator_string, operator);
          break;

        case ARITHMETIC_OP_NUMBER_NEIGHBORS:
          arithmetic_number_neighbors(p, operator_string, operator);
          break;

        case ARITHMETIC_OP_CONNECTED_COMPONENTS:
          arithmetic_connected_components(p, operator_string);
          break;

        case ARITHMETIC_OP_FILL_HOLES:
          arithmetic_fill_holes(p, operator_string);
          break;

        case ARITHMETIC_OP_INVERT:
          arithmetic_invert(p, operator_string);
          break;

        case ARITHMETIC_OP_INTERPOLATE_MINNGB:
        case ARITHMETIC_OP_INTERPOLATE_MAXNGB:
        case ARITHMETIC_OP_INTERPOLATE_MEANNGB:
        case ARITHMETIC_OP_INTERPOLATE_MEDIANNGB:
          arithmetic_interpolate(p, operator, operator_string);
          break;

        case ARITHMETIC_OP_INTERPOLATE_MINOFREGION:
        case ARITHMETIC_OP_INTERPOLATE_MAXOFREGION:
          arithmetic_interpolate_region(p, operator, operator_string);
          break;

        case ARITHMETIC_OP_COLLAPSE_SUM:
        case ARITHMETIC_OP_COLLAPSE_MIN:
        case ARITHMETIC_OP_COLLAPSE_MAX:
        case ARITHMETIC_OP_COLLAPSE_MEAN:
        case ARITHMETIC_OP_COLLAPSE_MEDIAN:
        case ARITHMETIC_OP_COLLAPSE_NUMBER:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_MAD:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_MAD:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_STD:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_STD:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_MEAN:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_MEAN:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_MEDIAN:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_MEDIAN:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_NUMBER:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_NUMBER:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MAD:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MAD:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_STD:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_STD:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MEAN:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MEAN:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_MEDIAN:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_MEDIAN:
        case ARITHMETIC_OP_COLLAPSE_MADCLIP_FILL_NUMBER:
        case ARITHMETIC_OP_COLLAPSE_SIGCLIP_FILL_NUMBER:
          arithmetic_collapse(p, operator_string, operator);
          break;

        case ARITHMETIC_OP_ADD_DIMENSION_SLOW:
        case ARITHMETIC_OP_ADD_DIMENSION_FAST:
          arithmetic_add_dimension(p, operator_string, operator);
          break;

        case ARITHMETIC_OP_REPEAT:
          arithmetic_repeat(p, operator_string, operator);
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: a bug! please contact us at "
                "%s to fix the problem. The code %d is not "
                "recognized for 'op'", __func__, PACKAGE_BUGREPORT,
                operator);
        }
    }
}





/* Used by the 'set-' operator. */
static int
arithmetic_set_name_used_later(void *in, char *name)
{
  struct gal_arithmetic_set_params *p=(struct gal_arithmetic_set_params *)in;
  gal_list_str_t *token, *tokens = (gal_list_str_t *)(p->tokens);

  size_t counter=0;

  /* If the name exists after the current token, then return 1. */
  for(token=tokens;token!=NULL;token=token->next)
    if( counter++ > p->tokencounter && !strcmp(token->v, name) )
      return 1;

  /* If we get to this point, it means that the name doesn't exist. */
  return 0;
}





static void
arithmetic_final_read_file(struct arithmeticparams *p,
                           struct operand *operand)
{
  gal_data_t *out=NULL;
  char *hdu, *filename;

  /* Read the desired image and report it if necessary. */
  hdu=operand->hdu;
  filename=operand->filename;
  if( gal_fits_file_recognized(filename) )
    {
      /* Read the data, note that the WCS has already been set. */
      out=gal_array_read_one_ch(filename, hdu, NULL,
                                p->cp.minmapsize,
                                p->cp.quietmmap, "--hdu");
      out->ndim=gal_dimension_remove_extra(out->ndim, out->dsize,
                                            NULL);
      if(!p->cp.quiet) printf(" - %s (hdu %s) is read.\n", filename, hdu);
    }
  else
    error(EXIT_FAILURE, 0, "%s: a bug! please contact us at %s to fix "
          "the problem. While 'operands->data' is NULL, the filename "
          "('%s') is not recognized as a FITS file", __func__,
          PACKAGE_BUGREPORT, filename);

  /* Keep the read dataset (note that we don't need to free 'filename' or
     'hdu' since their pointers are simply copied into the operand, they
     are not copied.*/
  operand->data=out;
}





/* Extract all the datasets of the remaining operands. */
static gal_data_t *
arithmetic_final_data(struct arithmeticparams *p)
{
  struct operand *op;
  gal_data_t *out=NULL;

  /* Go over the operands, extract their dataset and add it to the list. */
  for(op=p->operands; op!=NULL; op=op->next)
    {
      gal_list_data_add(&out, op->data);
      out->wcs=gal_wcs_copy(p->refdata.wcs);
    }

  /* Reverse the list so it goes in the same order as the operands, then
     return it. */
  gal_list_data_reverse(&out);
  return out;
}





/* This function implements the reverse polish algorithm as explained
   in the Wikipedia page.

   NOTE that in ui.c, the input linked list of tokens was ordered to
   have the same order as what the user provided. */
void
reversepolish(struct arithmeticparams *p)
{
  char *printnum;
  struct operand *otmp;
  size_t num_operands=0;
  gal_list_str_t *token;
  gal_data_t *tmp, *data, *col;
  struct gal_options_common_params *cp=&p->cp;
  int inlib, operator=GAL_ARITHMETIC_OP_INVALID;

  /* Prepare the processing: */
  p->popcounter=0;
  p->operands=NULL;
  p->setprm.params=p;
  p->setprm.tokencounter=0;
  p->setprm.tokens=p->tokens;
  p->setprm.pop=operands_pop_wrapper_set;
  p->setprm.used_later=arithmetic_set_name_used_later;

  /* Go over each input token and do the work. */
  for(token=p->tokens;token!=NULL;token=token->next)
    {
      /* The 'tofile-' operator's string can end in a '.fits', similar to a
         FITS file input file. So, it needs to be checked before checking
         for a filename. If we have a name or number, then add it to the
         operands linked list. Otherwise, pull out two members and do the
         specified operation on them. */
      operator=GAL_ARITHMETIC_OP_INVALID;
      if( !strncmp(OPERATOR_PREFIX_TOFILE, token->v,
                   OPERATOR_PREFIX_LENGTH_TOFILE) )
        arithmetic_tofile(p, token->v, 0);
      else if( !strncmp(OPERATOR_PREFIX_TOFILEFREE, token->v,
                        OPERATOR_PREFIX_LENGTH_TOFILEFREE) )
        arithmetic_tofile(p, token->v, 1);
      else if( !strncmp(token->v, GAL_ARITHMETIC_SET_PREFIX,
                        GAL_ARITHMETIC_SET_PREFIX_LENGTH) )
        gal_arithmetic_set_name(&p->setprm, token->v);
      else if( (col=gal_arithmetic_load_col(token->v, cp->searchin,
                                            cp->ignorecase,
                                            cp->minmapsize,
                                            cp->quietmmap))!=NULL )
        operands_add(p, NULL, col);
      else if(    gal_array_file_recognized(token->v)
               || gal_arithmetic_set_is_name(p->setprm.named, token->v) )
        operands_add(p, token->v, NULL);
      else if( (data=gal_data_copy_string_to_number(token->v)) )
        {
          /* The 'minmapsize' and 'quietmmap' parameters should be passed
             onto the library within numbers also (since they are the
             only things that go in the library sometimes). */
          data->quietmmap=p->cp.quietmmap;
          data->minmapsize=p->cp.minmapsize;
          operands_add(p, NULL, data);
        }

      /* Last option is an operator: the program will abort if the token
         isn't an operator. */
      else
        {
          operator=arithmetic_set_operator(token->v, &num_operands, &inlib);
          arithmetic_operator_run(p, operator, token->v, num_operands, inlib);
        }

      /* Increment the token counter. */
      ++p->setprm.tokencounter;
    }


  /* If there aren't any more operands (a variable has been set but not
     used), then there is nothing to create. */
  if(p->operands==NULL)
    error(EXIT_FAILURE, 0, "no operands on the stack to write (as output)");


  /* If there is more than one node in the operands stack then the user has
     given too many operands which is an error. */
  if(p->writeall==0 && p->operands->next!=NULL)
    error(EXIT_FAILURE, 0, "too many operands");


  /* If the final operand has a filename, but its 'data' element is NULL,
     then the file hasn't actually be read yet. In this case, we need to
     read the contents of the file and put the resulting dataset into the
     operands 'data' element. This can happen for example if no operators
     are called and there is only one filename as an argument (which can
     happen in scripts).*/
  for(otmp=p->operands; otmp!=NULL; otmp=otmp->next)
    if(otmp->data==NULL && otmp->filename)
      arithmetic_final_read_file(p, otmp);


  /* If the final data structure has more than one element, write it as a
     FITS file. Otherwise, if the user didn't call '--output', print it in
     the standard output. */
  data=arithmetic_final_data(p);
  if(data->next==NULL && data->size==1 && data->ndim==1
     && p->outnamerequested==0)
    {
      /* Make the string to print the number. */
      printnum=gal_type_to_string(data->array, data->type, 0);
      printf("%s\n", printnum);

      /* Clean up. */
      free(printnum);
    }
  else
    {
      /* If the user has requested a name, units or comments for the FITS
         file, insert them into the dataset here. */
      if(p->metaname)
        { if(data->name) free(data->name);
          gal_checkset_allocate_copy(p->metaname, &data->name); }
      if(p->metaunit)
        { if(data->unit) free(data->unit);
          gal_checkset_allocate_copy(p->metaunit, &data->unit); }
      if(p->metacomment)
        { if(data->comment) free(data->comment);
          gal_checkset_allocate_copy(p->metacomment, &data->comment); }

      /* Put a copy of the WCS structure from the reference image, it
         will be freed while freeing 'data'. */
      if(data->ndim==1 && p->onedasimage==0)
        gal_table_write(data, NULL, NULL, p->cp.tableformat,
                        p->onedonstdout ? NULL : p->cp.output,
                        "ARITHMETIC", 0);
      else
        for(tmp=data; tmp!=NULL; tmp=tmp->next)
          gal_fits_img_write(tmp, p->cp.output, NULL, PROGRAM_NAME);

      /* Let the user know that the job is done. */
      if(!p->cp.quiet)
        printf(" - Write (final): %s\n", p->cp.output);
    }


  /* Clean up, note that above, we copied the pointer to 'refdata->wcs'
     into 'data', so it is freed when freeing 'data'. */
  gal_data_free(data);
  free(p->refdata.dsize);
  gal_list_data_free(p->setprm.named);


  /* Clean up. Note that the tokens were taken from the command-line
     arguments, so the string within each token linked list must not be
     freed. */
  gal_list_str_free(p->tokens, 0);
  free(p->operands);
}



















/***************************************************************/
/*************             Top function            *************/
/***************************************************************/
void
arithmetic(struct arithmeticparams *p)
{
  /* Parse the arguments */
  reversepolish(p);
}
