/*********************************************************************
dimension -- Functions for multi-dimensional operations.
This is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

#include <gnuastro/wcs.h>
#include <gnuastro/binary.h>
#include <gnuastro/pointer.h>
#include <gnuastro/threads.h>
#include <gnuastro/convolve.h>
#include <gnuastro/dimension.h>
#include <gnuastro/statistics.h>
#include <gnuastro/arithmetic.h>





/************************************************************************/
/********************             Info             **********************/
/************************************************************************/
size_t
gal_dimension_total_size(size_t ndim, size_t *dsize)
{
  size_t i, num=1;
  for(i=0;i<ndim;++i) num *= dsize[i];
  return num;
}





int
gal_dimension_is_different(gal_data_t *first, gal_data_t *second)
{
  size_t i;

  /* First make sure that the dimensionality is the same. */
  if(first->ndim!=second->ndim)
    return 1;

  /* If the sizes are not zero, check if each dimension also has the same
     length. */
  if(first->size==0 && first->size==second->size)
    return 0;
  else
    for(i=0;i<first->ndim;++i)
      if( first->dsize[i] != second->dsize[i] )
        return 1;

  /* If it got to here, we know the dimensions have the same length. */
  return 0;
}





/* Calculate the values necessary to increment/decrement along each
   dimension of a dataset with size 'dsize'. */
size_t *
gal_dimension_increment(size_t ndim, size_t *dsize)
{
  int i;
  size_t *out=gal_pointer_allocate(GAL_TYPE_SIZE_T, ndim, 0, __func__, "out");

  /* Along the fastest dimension, it is 1. */
  out[ndim-1]=1;

  /* For the rest of the dimensions, it is the multiple of the faster
     dimension's length and the value for the previous dimension. */
  if(ndim>1)
    for(i=ndim-2;i>=0;--i)
      out[i]=dsize[i+1]*out[i+1];

  /* Return the allocated array. */
  return out;
}





size_t
gal_dimension_num_neighbors(size_t ndim)
{
  if(ndim)
    return pow(3, ndim)-1;
  else
    error(EXIT_FAILURE, 0, "%s: ndim cannot be zero", __func__);
  return 0;
}


















/************************************************************************/
/********************          Coordinates         **********************/
/************************************************************************/
void
gal_dimension_add_coords(size_t *c1, size_t *c2, size_t *out, size_t ndim)
{
  size_t *end=c1+ndim;
  do *out++ = *c1++ + *c2++; while(c1<end);
}





/* Return the index of an element from its coordinates. The index is the
   position in the contiguous array (assuming it is a 1D arrray). */
size_t
gal_dimension_coord_to_index(size_t ndim, size_t *dsize, size_t *coord)
{
  size_t i, d, ind=0, in_all_faster_dim;

  switch(ndim)
    {
    case 0:
      error(EXIT_FAILURE, 0, "%s: doesn't accept 0 dimensional arrays",
            __func__);

    case 1:
      ind=coord[0];
      break;

    case 2:
      ind=coord[0]*dsize[1]+coord[1];
      break;

    default:
      for(d=0;d<ndim;++d)
        {
          /* First, find the number of elements in all dimensions faster
             than this one. */
          in_all_faster_dim=1;
          for(i=d+1;i<ndim;++i)
            in_all_faster_dim *= dsize[i];

          /* Multiply it by the coordinate value of this dimension and add
             to the index. */
          ind += coord[d] * in_all_faster_dim;
        }
    }

  /* Return the derived index. */
  return ind;
}





/* You know the index ('ind') of a point/tile in an n-dimensional ('ndim')
   array which has 'dsize[i]' elements along dimension 'i'. You want to
   know the coordinates of that point along each dimension. The output is
   not actually returned, it must be allocated ('ndim' elements) before
   calling this function. This function will just fill it. The reason for
   this is that this function will often be called with a loop and a single
   allocated space would be enough for the whole loop. */
void
gal_dimension_index_to_coord(size_t index, size_t ndim, size_t *dsize,
                             size_t *coord)
{
  size_t d, *dinc;

  switch(ndim)
    {
    case 0:
      error(EXIT_FAILURE, 0, "%s: a 0-dimensional dataset is not defined",
            __func__);

    /* One dimensional dataset. */
    case 1:
      coord[0] = index;
      break;

    /* 2D dataset. */
    case 2:
      coord[0] = index / dsize[1];
      coord[1] = index % dsize[1];
      break;

    /* Higher dimensional datasets. */
    default:
      /* Set the incrementation values for each dimension. */
      dinc=gal_dimension_increment(ndim, dsize);

      /* We start with the slowest dimension (first in the C standard) and
         continue until (but not including) the fastest dimension. This is
         because except for the fastest (coniguous) dimension, the other
         coordinates can be found by division. */
      for(d=0;d<ndim;++d)
        {
          /* Set the coordinate value for this dimension. */
          coord[d] = index / dinc[d];

          /* Replace the index with its remainder with the number of
             elements in all faster dimensions. */
          index  %= dinc[d];
        }

      /* Clean up. */
      free(dinc);
    }
}




















/************************************************************************/
/********************           Distances          **********************/
/************************************************************************/
float
gal_dimension_dist_manhattan(size_t *a, size_t *b, size_t ndim)
{
  size_t i, out=0;
  for(i=0;i<ndim;++i) out += (a[i] > b[i]) ? (a[i]-b[i]) : (b[i]-a[i]);
  return out;
}





float
gal_dimension_dist_radial(size_t *a, size_t *b, size_t ndim)
{
  size_t i, out=0;
  for(i=0;i<ndim;++i) out += (a[i]-b[i])*(a[i]-b[i]);
  return sqrt(out);
}



/* Elliptical distance of a point from a given center. */
#define DEGREESTORADIANS   M_PI/180.0
float
gal_dimension_dist_elliptical(double *center, double *pa_deg, double *q,
                              size_t ndim, double *point)
{
  double Xr, Yr, Zr;        /* Rotated x, y, z. */
  double q1=q[0], q2;
  double c1=cos( pa_deg[0] * DEGREESTORADIANS ), c2, c3;
  double s1= sin( pa_deg[0] * DEGREESTORADIANS ), s2, s3;
  double x=center[0]-point[0], y=center[1]-point[1], z;

  /* Find the distance depending on the dimension. */
  switch(ndim)
    {
    case 2:
      /* The parenthesis aren't necessary, but help in readability and
         avoiding human induced bugs. */
      Xr = x * ( c1       )     +   y * ( s1 );
      Yr = x * ( -1.0f*s1 )     +   y * ( c1 );
      return sqrt( Xr*Xr + Yr*Yr/q1/q1 );
      break;

    case 3:
      /* Define the necessary parameters. */
      q2=q[1];
      z=center[2]-point[2];
      c2 = cos( pa_deg[1] * DEGREESTORADIANS );
      s2 = sin( pa_deg[1] * DEGREESTORADIANS );
      c3 = cos( pa_deg[2] * DEGREESTORADIANS );
      s3 = sin( pa_deg[2] * DEGREESTORADIANS );

      /* Calculate the distance. */
      Xr = x*( c3*c1   - s3*c2*s1) + y*( c3*s1   + s3*c2*c1) + z*( s3*s2 );
      Yr = x*(-1*s3*c1 - c3*c2*s1) + y*(-1*s3*s1 + c3*c2*c1) + z*( c3*s2 );
      Zr = x*( s1*s2             ) + y*(-1*s2*c1           ) + z*( c2    );
      return sqrt( Xr*Xr + Yr*Yr/q1/q1 + Zr*Zr/q2/q2 );
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: currently only 2 and 3 dimensional "
            "distances are supported, you have asked for an "
            "%zu-dimensional dataset", __func__, ndim);

    }

  /* Control should never reach here. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to address "
        "the problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return NAN;
}



















/************************************************************************/
/********************    Collapsing a dimension    **********************/
/************************************************************************/
enum dimension_collapse_operation
{
 DIMENSION_COLLAPSE_INVALID,    /* ==0 by C standard. */

 DIMENSION_COLLAPSE_SUM,
 DIMENSION_COLLAPSE_MAX,
 DIMENSION_COLLAPSE_MIN,
 DIMENSION_COLLAPSE_MEAN,
 DIMENSION_COLLAPSE_MEDIAN,
 DIMENSION_COLLAPSE_NUMBER,
 DIMENSION_COLLAPSE_MADCLIP_MAD,
 DIMENSION_COLLAPSE_SIGCLIP_MAD,
 DIMENSION_COLLAPSE_MADCLIP_STD,
 DIMENSION_COLLAPSE_SIGCLIP_STD,
 DIMENSION_COLLAPSE_MADCLIP_MEAN,
 DIMENSION_COLLAPSE_SIGCLIP_MEAN,
 DIMENSION_COLLAPSE_MADCLIP_MEDIAN,
 DIMENSION_COLLAPSE_SIGCLIP_MEDIAN,
 DIMENSION_COLLAPSE_MADCLIP_NUMBER,
 DIMENSION_COLLAPSE_SIGCLIP_NUMBER,
 DIMENSION_COLLAPSE_MADCLIP_FILL_MAD,
 DIMENSION_COLLAPSE_SIGCLIP_FILL_MAD,
 DIMENSION_COLLAPSE_MADCLIP_FILL_STD,
 DIMENSION_COLLAPSE_SIGCLIP_FILL_STD,
 DIMENSION_COLLAPSE_MADCLIP_FILL_MEAN,
 DIMENSION_COLLAPSE_SIGCLIP_FILL_MEAN,
 DIMENSION_COLLAPSE_MADCLIP_FILL_MEDIAN,
 DIMENSION_COLLAPSE_SIGCLIP_FILL_MEDIAN,
 DIMENSION_COLLAPSE_MADCLIP_FILL_NUMBER,
 DIMENSION_COLLAPSE_SIGCLIP_FILL_NUMBER,
};





static gal_data_t *
dimension_collapse_sanity_check(gal_data_t *in, gal_data_t *weight,
                                size_t c_dim, int hasblank, size_t *cnum,
                                double **warr)
{
  gal_data_t *wht=NULL;

  /* The requested dimension to collapse cannot be larger than the input's
     number of dimensions. */
  if( c_dim > (in->ndim-1) )
    error(EXIT_FAILURE, 0, "%s: the input has %zu dimension(s), but you "
          "have asked to collapse dimension %zu", __func__, in->ndim,
          c_dim);

  /* If there is no blank value, there is no point in calculating the
     number of points in each collapsed dataset (when necessary). In that
     case, 'cnum!=0'. */
  if(hasblank==0)
    *cnum=in->dsize[c_dim];

  /* Weight sanity checks. */
  if(weight)
    {
      if( weight->ndim!=1 )
        error(EXIT_FAILURE, 0, "%s: the weight dataset has %zu "
              "dimensions, it must be one-dimensional", __func__,
              weight->ndim);
      if( in->dsize[c_dim]!=weight->size )
        error(EXIT_FAILURE, 0, "%s: the weight dataset has %zu elements, "
              "but the input dataset has %zu elements in dimension %zu",
              __func__, weight->size, in->dsize[c_dim], c_dim);
      wht = ( weight->type == GAL_TYPE_FLOAT64
              ? weight
              : gal_data_copy_to_new_type(weight, GAL_TYPE_FLOAT64) );
      *warr = wht->array;
    }

  /* Return the weight data structure. */
  return wht;
}




/* Set the collapsed output sizes. */
static void
dimension_collapse_sizes(gal_data_t *in, size_t c_dim, size_t *outndim,
                         size_t *outdsize)
{
  size_t i, a=0;

  if(in->ndim==1)
    *outndim=outdsize[0]=1;
  else
    {
      *outndim=in->ndim-1;
      for(i=0;i<in->ndim;++i)
        if(i!=c_dim) outdsize[a++]=in->dsize[i];
    }
}





/* Depending on the operator, write the result into the output. */
#define COLLAPSE_WRITE(OIND,IIND) {                                     \
    /* Sum */                                                           \
    if(farr)                                                            \
      farr[ OIND ] += (warr ? warr[w] : 1) * inarr[ IIND ];             \
                                                                        \
    /* Number */                                                        \
    if(iarr)                                                            \
      {                                                                 \
        if(num->type==GAL_TYPE_UINT8) iarr[ OIND ] = 1;                 \
        else                        ++iarr[ OIND ];                     \
      }                                                                 \
                                                                        \
    /* Sum of weights. */                                               \
    if(wsumarr) wsumarr[ OIND ] += warr[w];                             \
                                                                        \
    /* Minimum or maximum. */                                           \
    if(mmarr)                                                           \
      mmarr[OIND] = ( max1_min0                                         \
                      ? (mmarr[OIND]>inarr[IIND]?mmarr[OIND]:inarr[IIND]) \
                      : (mmarr[OIND]<inarr[IIND]?mmarr[OIND]:inarr[IIND]) ); \
  }


/* Deal properly with blanks. */
#define COLLAPSE_CHECKBLANK(OIND,IIND) {                                \
    if(hasblank)                                                        \
      {                                                                 \
        if(B==B) /* An integer type: blank can be checked with '=='. */ \
          {                                                             \
            if( inarr[IIND] != B )           COLLAPSE_WRITE(OIND,IIND); \
          }                                                             \
        else     /* A floating point type where NAN != NAN. */          \
          {                                                             \
            if( inarr[IIND] == inarr[IIND] ) COLLAPSE_WRITE(OIND,IIND); \
          }                                                             \
      }                                                                 \
    else                                     COLLAPSE_WRITE(OIND,IIND); \
  }



#define COLLAPSE_DIM(IT) {                                              \
    IT m, B, *inarr=in->array, *mmarr=minmax?minmax->array:NULL;        \
    if(hasblank) gal_blank_write(&B, in->type);                         \
                                                                        \
    /* Initialize the array for minimum or maximum. */                  \
    if(mmarr)                                                           \
      {                                                                 \
        if(max1_min0) gal_type_min(in->type, &m);                       \
        else          gal_type_max(in->type, &m);                       \
        for(i=0;i<minmax->size;++i) mmarr[i]=m;                         \
      }                                                                 \
                                                                        \
    /* Collapse the dataset. */                                         \
    switch(in->ndim)                                                    \
      {                                                                 \
      /* 1D input dataset. */                                           \
      case 1:                                                           \
        for(i=0;i<in->dsize[0];++i)                                     \
          {                                                             \
            if(weight) w=i;                                             \
            COLLAPSE_CHECKBLANK(0,i);                                   \
          }                                                             \
        break;                                                          \
                                                                        \
      /* 2D input dataset. */                                           \
      case 2:                                                           \
        for(i=0;i<in->dsize[0];++i)                                     \
          for(j=0;j<in->dsize[1];++j)                                   \
            {                                                           \
              /* In a more easy to understand format:                   \
                 dim==0 --> a=j;                                        \
                 dim==1 --> a=i; */                                     \
              a = c_dim==0 ? j : i;                                     \
              if(weight) w = c_dim == 0 ? i : j;                        \
              COLLAPSE_CHECKBLANK(a, i*in->dsize[1] + j);               \
            }                                                           \
        break;                                                          \
                                                                        \
      /* 3D input dataset. */                                           \
      case 3:                                                           \
        slice=in->dsize[1]*in->dsize[2];                                \
        for(i=0;i<in->dsize[0];++i)                                     \
          for(j=0;j<in->dsize[1];++j)                                   \
            for(k=0;k<in->dsize[2];++k)                                 \
              {                                                         \
                /* In a more easy to understand format:                 \
                   dim==0 --> a=j; b=k;                                 \
                   dim==1 --> a=i; b=k;                                 \
                   dim==2 --> a=i; b=j;   */                            \
                a = c_dim==0 ? j : i;                                   \
                b = c_dim==2 ? j : k;                                   \
                if(weight) w = c_dim==0 ? i : (c_dim==1 ? j : k);       \
                COLLAPSE_CHECKBLANK(a*outdsize[1]+b,                    \
                                    i*slice + j*in->dsize[2] + k);      \
              }                                                         \
        break;                                                          \
                                                                        \
        /* Input dataset's dimensionality not yet supported. */         \
      default:                                                          \
        error(EXIT_FAILURE, 0, "%s: %zu-dimensional datasets not yet "  \
              "supported, please contact us at %s to add this feature", \
              __func__, in->ndim, PACKAGE_BUGREPORT);                   \
      }                                                                 \
                                                                        \
    /* For minimum or maximum, elements with no input must be blank. */ \
    if(mmarr && iarr)                                                   \
      for(i=0;i<minmax->size;++i) if(iarr[i]==0) mmarr[i]=B;            \
  }





gal_data_t *
gal_dimension_collapse_sum(gal_data_t *in, size_t c_dim, gal_data_t *weight)
{
  int max1_min0=0;
  double *wsumarr=NULL;
  uint8_t *ii, *iarr=NULL;
  size_t a, b, i, j, k, w=-1, cnum=0;
  size_t outdsize[10], slice, outndim;
  int hasblank=gal_blank_present(in, 0);
  double *dd, *df, *warr=NULL, *farr=NULL;
  gal_data_t *sum=NULL, *wht=NULL, *num=NULL, *minmax=NULL;

  /* Basic sanity checks. */
  wht=dimension_collapse_sanity_check(in, weight, c_dim, hasblank,
                                      &cnum, &warr);

  /* Set the size of the collapsed output. */
  dimension_collapse_sizes(in, c_dim, &outndim, outdsize);

  /* Allocate the sum (output) dataset. */
  sum=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, outndim, outdsize, in->wcs,
                     1, in->minmapsize, in->quietmmap, NULL, NULL, NULL);

  /* The number dataset (when there are blank values). */
  if(hasblank)
    num=gal_data_alloc(NULL, GAL_TYPE_INT8, outndim, outdsize, NULL,
                       1, in->minmapsize, in->quietmmap, NULL, NULL, NULL);

  /* Set the array pointers. */
  if(sum) farr=sum->array;
  if(num) iarr=num->array;

  /* Parse the dataset. */
  switch(in->type)
    {
    case GAL_TYPE_UINT8:     COLLAPSE_DIM( uint8_t  );   break;
    case GAL_TYPE_INT8:      COLLAPSE_DIM( int8_t   );   break;
    case GAL_TYPE_UINT16:    COLLAPSE_DIM( uint16_t );   break;
    case GAL_TYPE_INT16:     COLLAPSE_DIM( int16_t  );   break;
    case GAL_TYPE_UINT32:    COLLAPSE_DIM( uint32_t );   break;
    case GAL_TYPE_INT32:     COLLAPSE_DIM( int32_t  );   break;
    case GAL_TYPE_UINT64:    COLLAPSE_DIM( uint64_t );   break;
    case GAL_TYPE_INT64:     COLLAPSE_DIM( int64_t  );   break;
    case GAL_TYPE_FLOAT32:   COLLAPSE_DIM( float    );   break;
    case GAL_TYPE_FLOAT64:   COLLAPSE_DIM( double   );   break;
    default:
      error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
            __func__, in->type);
    }

  /* If 'num' is zero on any element, set its sum to NaN. */
  if(num)
    {
      ii = num->array;
      df = (dd=sum->array) + sum->size;
      do if(*ii++==0) *dd=NAN; while(++dd<df);
    }

  /* Remove the respective dimension in the WCS structure also (if any
     exists). Note that 'sum->ndim' has already been changed. So we'll use
     'in->wcs'. */
  gal_wcs_remove_dimension(sum->wcs, in->ndim-c_dim);

  /* Clean up and return. */
  if(wht!=weight) gal_data_free(wht);
  if(num) gal_data_free(num);
  return sum;
}





gal_data_t *
gal_dimension_collapse_mean(gal_data_t *in, size_t c_dim,
                            gal_data_t *weight)
{
  int max1_min0=0;
  double wsum=NAN;
  double *wsumarr=NULL;
  int32_t *ii, *iarr=NULL;
  size_t a, b, i, j, k, w=-1, cnum=0;
  size_t outdsize[10], slice, outndim;
  int hasblank=gal_blank_present(in, 0);
  double *dd, *dw, *df, *warr=NULL, *farr=NULL;
  gal_data_t *sum=NULL, *wht=NULL, *num=NULL, *minmax=NULL;


  /* Basic sanity checks. */
  wht=dimension_collapse_sanity_check(in, weight, c_dim, hasblank,
                                      &cnum, &warr);

  /* Set the size of the collapsed output. */
  dimension_collapse_sizes(in, c_dim, &outndim, outdsize);

  /* The sum array. */
  sum=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, outndim, outdsize, in->wcs,
                     1, in->minmapsize, in->quietmmap, NULL, NULL, NULL);

  /* If a weighted mean is requested. */
  if( weight )
    {
      /* There are blank values, so we'll need to keep the sums of the
         weights for each collapsed dimension. */
      if( hasblank )
        wsumarr=gal_pointer_allocate(GAL_TYPE_FLOAT64, sum->size, 1,
                                     __func__, "wsumarr");

      /* There aren't any blank values, so one summation over the
         weights is enough to calculate the weighted mean. */
      else
        {
          wsum=0.0f;
          df=(dd=weight->array)+weight->size;
          do wsum += *dd++; while(dd<df);
        }
    }
  /* No weight is given, so we'll need the number of elements. */
  else if( hasblank )
    num=gal_data_alloc(NULL, GAL_TYPE_INT32, outndim, outdsize, NULL,
                       1, in->minmapsize, in->quietmmap, NULL, NULL, NULL);

  /* Set the array pointers. */
  if(sum) farr=sum->array;
  if(num) iarr=num->array;

  /* Parse the dataset. */
  switch(in->type)
    {
    case GAL_TYPE_UINT8:     COLLAPSE_DIM( uint8_t  );   break;
    case GAL_TYPE_INT8:      COLLAPSE_DIM( int8_t   );   break;
    case GAL_TYPE_UINT16:    COLLAPSE_DIM( uint16_t );   break;
    case GAL_TYPE_INT16:     COLLAPSE_DIM( int16_t  );   break;
    case GAL_TYPE_UINT32:    COLLAPSE_DIM( uint32_t );   break;
    case GAL_TYPE_INT32:     COLLAPSE_DIM( int32_t  );   break;
    case GAL_TYPE_UINT64:    COLLAPSE_DIM( uint64_t );   break;
    case GAL_TYPE_INT64:     COLLAPSE_DIM( int64_t  );   break;
    case GAL_TYPE_FLOAT32:   COLLAPSE_DIM( float    );   break;
    case GAL_TYPE_FLOAT64:   COLLAPSE_DIM( double   );   break;
    default:
      error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
            __func__, in->type);
    }

  /* If 'num' is zero on any element, set its sum to NaN. */
  if(num)
    {
      ii = num->array;
      df = (dd=sum->array) + sum->size;
      do if(*ii++==0) *dd=NAN; while(++dd<df);
    }

  /* Divide the sum by the number. */
  df = (dd=sum->array) + sum->size;
  if(weight)
    {
      if(hasblank) { dw=wsumarr;  do *dd /= *dw++; while(++dd<df); }
      else                        do *dd /= wsum;  while(++dd<df);
    }
  else
    if(num) { ii = num->array;    do *dd /= *ii++; while(++dd<df); }
    else                          do *dd /= cnum;  while(++dd<df);

  /* Correct the WCS, clean up and return. */
  gal_wcs_remove_dimension(sum->wcs, in->ndim-c_dim);
  if(wht!=weight) gal_data_free(wht);
  if(wsumarr) free(wsumarr);
  gal_data_free(num);
  return sum;
}





gal_data_t *
gal_dimension_collapse_number(gal_data_t *in, size_t c_dim)
{
  int max1_min0=0;
  double *wsumarr=NULL;
  double *warr=NULL, *farr=NULL;
  int32_t *ii, *iif, *iarr=NULL;
  size_t a, b, i, j, k, w, cnum=0;
  size_t outdsize[10], slice, outndim;
  int hasblank=gal_blank_present(in, 0);
  gal_data_t *weight=NULL, *wht=NULL, *num=NULL, *minmax=NULL;

  /* Basic sanity checks. */
  wht=dimension_collapse_sanity_check(in, weight, c_dim, hasblank,
                                      &cnum, &warr);

  /* Set the size of the collapsed output. */
  dimension_collapse_sizes(in, c_dim, &outndim, outdsize);

  /* The number dataset (when there are blank values).*/
  num=gal_data_alloc(NULL, GAL_TYPE_INT32, outndim, outdsize, in->wcs,
                     1, in->minmapsize, in->quietmmap, NULL, NULL, NULL);

  /* Set the array pointers. */
  iarr=num->array;

  /* Parse the input dataset (if necessary). */
  if(hasblank)
    switch(in->type)
      {
      case GAL_TYPE_UINT8:     COLLAPSE_DIM( uint8_t  );   break;
      case GAL_TYPE_INT8:      COLLAPSE_DIM( int8_t   );   break;
      case GAL_TYPE_UINT16:    COLLAPSE_DIM( uint16_t );   break;
      case GAL_TYPE_INT16:     COLLAPSE_DIM( int16_t  );   break;
      case GAL_TYPE_UINT32:    COLLAPSE_DIM( uint32_t );   break;
      case GAL_TYPE_INT32:     COLLAPSE_DIM( int32_t  );   break;
      case GAL_TYPE_UINT64:    COLLAPSE_DIM( uint64_t );   break;
      case GAL_TYPE_INT64:     COLLAPSE_DIM( int64_t  );   break;
      case GAL_TYPE_FLOAT32:   COLLAPSE_DIM( float    );   break;
      case GAL_TYPE_FLOAT64:   COLLAPSE_DIM( double   );   break;
      default:
        error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
              __func__, in->type);
      }
  else
    {
      iif=(ii=num->array)+num->size;
      do *ii++ = cnum; while(ii<iif);
    }

  /* Remove the respective dimension in the WCS structure also (if any
     exists). Note that 'sum->ndim' has already been changed. So we'll use
     'in->wcs'. */
  gal_wcs_remove_dimension(num->wcs, in->ndim-c_dim);

  /* Return. */
  if(wht!=weight) gal_data_free(wht);
  return num;
}





gal_data_t *
gal_dimension_collapse_minmax(gal_data_t *in, size_t c_dim, int max1_min0)
{
  uint8_t *iarr=NULL;
  double *wsumarr=NULL;
  double *warr=NULL, *farr=NULL;
  size_t a, b, i, j, k, w, cnum=0;
  size_t outdsize[10], slice, outndim;
  int hasblank=gal_blank_present(in, 0);
  gal_data_t *weight=NULL, *wht=NULL, *num=NULL, *minmax=NULL;

  /* Basic sanity checks. */
  wht=dimension_collapse_sanity_check(in, weight, c_dim, hasblank,
                                      &cnum, &warr);

  /* Set the size of the collapsed output. */
  dimension_collapse_sizes(in, c_dim, &outndim, outdsize);

  /* Allocate the necessary datasets. If there are blank pixels, we'll need
     to count how many elements whent into the calculation so we can set
     them to blank. */
  minmax=gal_data_alloc(NULL, in->type, outndim, outdsize, in->wcs,
                        0, in->minmapsize, in->quietmmap, NULL, NULL, NULL);
  if(hasblank)
    {
      num=gal_data_alloc(NULL, GAL_TYPE_UINT8, outndim, outdsize, in->wcs,
                         1, in->minmapsize, in->quietmmap, NULL, NULL,
                         NULL);
      iarr=num->array;
    }

  /* Parse the input dataset (if necessary). */
  switch(in->type)
    {
    case GAL_TYPE_UINT8:     COLLAPSE_DIM( uint8_t  );   break;
    case GAL_TYPE_INT8:      COLLAPSE_DIM( int8_t   );   break;
    case GAL_TYPE_UINT16:    COLLAPSE_DIM( uint16_t );   break;
    case GAL_TYPE_INT16:     COLLAPSE_DIM( int16_t  );   break;
    case GAL_TYPE_UINT32:    COLLAPSE_DIM( uint32_t );   break;
    case GAL_TYPE_INT32:     COLLAPSE_DIM( int32_t  );   break;
    case GAL_TYPE_UINT64:    COLLAPSE_DIM( uint64_t );   break;
    case GAL_TYPE_INT64:     COLLAPSE_DIM( int64_t  );   break;
    case GAL_TYPE_FLOAT32:   COLLAPSE_DIM( float    );   break;
    case GAL_TYPE_FLOAT64:   COLLAPSE_DIM( double   );   break;
    default:
      error(EXIT_FAILURE, 0, "%s: type value (%d) not recognized",
            __func__, in->type);
    }

  /* Remove the respective dimension in the WCS structure also (if any
     exists). Note that 'minmax->ndim' has already been changed. So we'll
     use 'in->wcs'. */
  gal_wcs_remove_dimension(minmax->wcs, in->ndim-c_dim);

  /* Clean up and return. */
  if(wht!=weight) gal_data_free(wht);
  if(num) gal_data_free(num);
  return minmax;
}





/* This structure can keep all information you want to pass onto the worker
   function on each thread. */
struct dimension_sortbased_p
{
  gal_data_t      *in;    /* Dataset to print values of.       */
  gal_data_t     *out;    /* Dataset to print values of.       */
  size_t        c_dim;    /* Dimension to collapse over.       */
  int        operator;    /* Operator to use.                  */
  float   sclipmultip;    /* Sigma clip multiple.              */
  float    sclipparam;    /* Sigma clip parameter.             */
  size_t   minmapsize;    /* Minimum size for memorymapping.   */
  int       quietmmap;    /* If memorymapping should be quiet. */
};





static void
dimension_csb_copy(gal_data_t *in, size_t from, gal_data_t *work,
                   size_t to)
{
  memcpy(gal_pointer_increment(work->array, to,   in->type),
         gal_pointer_increment(in->array,   from, in->type),
         gal_type_sizeof(in->type));
}





static gal_data_t *
dimension_collapse_sortbased_operation(struct dimension_sortbased_p *p,
                                       gal_data_t *work, uint8_t clipflags,
                                       uint8_t isfill, size_t wdsize)
{
  gal_data_t *out=NULL;

  /* Do the operation. */
  switch(p->operator)
    {
    case DIMENSION_COLLAPSE_MEDIAN:
      out=gal_statistics_median(work, 1);
      break;

    case DIMENSION_COLLAPSE_MADCLIP_MAD:
    case DIMENSION_COLLAPSE_MADCLIP_STD:
    case DIMENSION_COLLAPSE_MADCLIP_MEAN:
    case DIMENSION_COLLAPSE_MADCLIP_MEDIAN:
    case DIMENSION_COLLAPSE_MADCLIP_NUMBER:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MAD:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_STD:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MEAN:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MEDIAN:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_NUMBER:
      /* When we are dealing with a clip-fill operator, the order of
         the elements matter, so we don't want it to be done inplace.*/
      out=gal_statistics_clip_mad(work, p->sclipmultip,
                                  p->sclipparam, clipflags,
                                  isfill?0:1, 1);
      break;

    case DIMENSION_COLLAPSE_SIGCLIP_MAD:
    case DIMENSION_COLLAPSE_SIGCLIP_STD:
    case DIMENSION_COLLAPSE_SIGCLIP_MEAN:
    case DIMENSION_COLLAPSE_SIGCLIP_MEDIAN:
    case DIMENSION_COLLAPSE_SIGCLIP_NUMBER:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MAD:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_STD:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEAN:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEDIAN:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_NUMBER:
      /* When we are dealing with a clip-fill operator, the order of
         the elements matter, so we don't want it to be done inplace.*/
      out=gal_statistics_clip_sigma(work, p->sclipmultip,
                                     p->sclipparam, clipflags,
                                     isfill?0:1, 1);
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
            "to fix the problem. The operator code %d isn't "
            "recognized", __func__, PACKAGE_BUGREPORT, p->operator);
    }

  /* All the statistical operators that are based on sorting will free the
     input array if it doesn't have any elements! But we are re-using the
     'work' array many times. So we need to re-allocate the array here. */
  if(work->array==NULL)
    work->array=gal_pointer_allocate(work->type, wdsize, 0, __func__,
                                     "work->array");

  /* Return the output. */
  return out;
}





static gal_data_t *
dimension_collapse_sortbased_fill(struct dimension_sortbased_p *p,
                                  gal_data_t *fstat, gal_data_t *work,
                                  gal_data_t *conv, uint8_t clipflags,
                                  size_t wdsize, int check)
{
  int std1_mad0=0;
  uint8_t *u, *uf;
  size_t one=1, sumflag;
  float *farr=fstat->array;
  gal_data_t *formask=conv?conv:work, *out=NULL;
  gal_data_t *tmp, *multip, *upper, *lower, *center, *spread;
  int aflags=GAL_ARITHMETIC_FLAG_NUMOK; /* Don't free the inputs. */

  /* Basic checks. */
  switch(p->operator)
    {
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MAD:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_STD:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MEAN:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MEDIAN:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_NUMBER:
      std1_mad0=0; break;
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MAD:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_STD:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEAN:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEDIAN:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_NUMBER:
      std1_mad0=1; break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
            "fix the problem. The operator code '%d' is not recognized",
            __func__, PACKAGE_BUGREPORT, p->operator);
    }

  /* Convert the first clipped statistics ('fstat') into two datasets with
     the center and spread. */
  center=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL, 0, -1, 1,
                        NULL, NULL, NULL);
  spread=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL, 0, -1, 1,
                        NULL, NULL, NULL);
  ((float *)(center->array))[0]=farr[GAL_STATISTICS_CLIP_OUTCOL_MEDIAN];
  ((float *)(spread->array))[0]=farr[ std1_mad0
                                      ? GAL_STATISTICS_CLIP_OUTCOL_STD
                                      : GAL_STATISTICS_CLIP_OUTCOL_MAD ];

  /* Find the upper and lower thresholds based on the user's desired
     multiple. */
  multip=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &one, NULL, 0, -1, 1,
                        NULL, NULL, NULL);
  ((float *)(multip->array))[0]=p->sclipmultip;
  tmp=gal_arithmetic(GAL_ARITHMETIC_OP_MULTIPLY, 1, aflags,
                     spread, multip);
  upper=gal_arithmetic(GAL_ARITHMETIC_OP_PLUS, 1, aflags,
                       center, tmp);
  lower=gal_arithmetic(GAL_ARITHMETIC_OP_MINUS, 1, aflags,
                       center, tmp);
  gal_data_free(tmp);

  /* Build a flag with bad elements having a value of 1. For a 1D dataset,
     the largest possible "hole" size should be smaller than higher
     dimensionality data, because two elements that are very far from each
     other can create a very large "hole".*/
  tmp=gal_arithmetic(GAL_ARITHMETIC_OP_OR, 1, aflags,
                     gal_arithmetic(GAL_ARITHMETIC_OP_GT, 1, aflags,
                                    formask, upper),
                     gal_arithmetic(GAL_ARITHMETIC_OP_LE, 1, aflags,
                                    formask, lower));

  /* For a check (define 'int check' as an argument, and when calling this
     function, set 'index==XXX').
  if(check)
    {
      size_t i;
      double *f=formask->array;
      uint8_t *u=tmp->array;
      for(i=0;i<formask->size;++i)
        printf("%-5zu %-5u %f\n", i, u[i], f[i]);
      printf("%s: upper:%f, lower: %f\n", __func__,
             ((float *)(upper->array))[0],
             ((float *)(lower->array))[0] );
    }
  */

  /* For a 1D array, do erosion because after filling holes, two single
     elements outside the range can mask a very large portion of the
     input. */
  tmp = ( formask->ndim==1
          ? gal_binary_erode(tmp, 1, 1, 1)
          : gal_binary_dilate(tmp, 1, 1, 1) );
  gal_binary_holes_fill(tmp, formask->ndim, -1);
  tmp=gal_binary_erode(tmp, 2, formask->ndim, 1);
  tmp=gal_binary_dilate(tmp, formask->ndim==1?4:2, formask->ndim, 1);

  /* If the covered area is larger than a certain fraction of the
     area, flag the whole dataset. */
  sumflag=0;
  uf=(u=tmp->array)+tmp->size; do sumflag+=*u; while(++u<uf);
  if(sumflag>0.95*tmp->size)
    {uf=(u=tmp->array)+tmp->size; do *u=1; while(++u<uf);}

  /* Apply the flag onto the input (to set the pixels to NaN). */
  gal_blank_flag_apply(work, tmp);

  /* For a check (define 'int check' as an argument, and when calling this
     function, set 'index==XXX').
  if(check)
    {
      size_t i;
      double *f=work->array; // CHECK THE TYPE OF YOUR INPUT
      uint8_t *u=tmp->array;
      for(i=0;i<work->size;++i)
        printf("%-5zu %-5u %f\n", i, u[i], f[i]);
      printf("%s: GOOD\n", __func__); exit(0);
    }
  */

  /* Apply the operation: note that 'isfill' is zero here because we don't
     care about the order of elements any more ('isfill' is only used to do
     the operation inplace or not). */
  out=dimension_collapse_sortbased_operation(p, work, clipflags, 0,
                                             wdsize);

  /* Clean up and return. */
  gal_data_free(tmp);
  gal_data_free(fstat);
  gal_data_free(upper);
  gal_data_free(lower);
  gal_data_free(multip);
  gal_data_free(center);
  gal_data_free(spread);
  return out;
}





gal_data_t *
dimension_collapse_sortbased_conv(gal_data_t *work /*, int check*/)
{
  float *karr;
  gal_data_t *in, *out, *kernel;
  size_t i, ksize[3]={3,3,3};  /* Extra dimensions will not be used. */

  /* Set the input. */
  in = ( work->type==GAL_TYPE_FLOAT32
         ? work
         : gal_data_copy_to_new_type(work, GAL_TYPE_FLOAT32) );

  /* Build a kernel and fill it with equal values (so their sum is 1). */
  kernel=gal_data_alloc(NULL, GAL_TYPE_FLOAT32, work->ndim, ksize,
                        NULL, 0, -1, 1, NULL, NULL, NULL);
  karr=kernel->array;
  for(i=0;i<kernel->size;++i) karr[i]=1.0/kernel->size;

  /* Convolve the work array with the given kernel. */
  out=gal_convolve_spatial(in, kernel, 1, 1, 0, 1);

  /* For a check.
  if(check)
    {
      float *ia=in->array, *oa=out->array;
      for(i=0;i<in->size;++i)
        printf("%-5zu %-10.3f %-10.3f\n", i, ia[i], oa[i]);
    }
  */

  /* Clean up and return. */
  if(in!=work) gal_data_free(in);
  gal_data_free(kernel);
  return out;
}





static void *
dimension_collapse_sortbased_worker(void *in_prm)
{
  /* Low-level definitions to be done first. */
  struct gal_threads_params *tprm=(struct gal_threads_params *)in_prm;
  struct dimension_sortbased_p *p=(struct dimension_sortbased_p *)tprm->params;

  /* Input dataset (also used in other variable definitions). */
  gal_data_t *in=p->in;

  /* Subsequent definitions. */
  uint8_t clipflags=0, isfill=0;
  size_t a, b, c, sind=GAL_BLANK_SIZE_T;
  gal_data_t *work, *conv=NULL, *stat=NULL;
  size_t i, j, index, c_dim=p->c_dim, wdsize=in->dsize[c_dim];

  /* Allocate the dataset that will be sorted. */
  work=gal_data_alloc(NULL, in->type, 1, &wdsize, NULL, 0,
                      p->minmapsize, p->quietmmap, NULL, NULL, NULL);

  /* Some things are unique for fill-based operators. */
  switch(p->operator)
    {
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MAD:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MAD:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_STD:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_STD:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MEAN:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEAN:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MEDIAN:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEDIAN:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_NUMBER:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_NUMBER:
      isfill=1;
      break;
    }

  /* Go over all the actions (pixels in this case) that were assigned to
     this thread. */
  for(i=0; tprm->indexs[i] != GAL_BLANK_SIZE_T; ++i)
    {
      /* For easy reading. */
      index = tprm->indexs[i];

      /* Reset the sizes (which may have been changed during the
         statistical calculation), and flags (so the possible existance or
         non-existance of blank values in one run doesn't affect the
         next). */
      work->flag=0;
      work->size=work->dsize[0]=wdsize;

      /* Extract the necessary components into an array. */
      switch(in->ndim)
        {
        /* One-dimensional data. */
        case 1:
          memcpy(work->array, in->array,
                 in->size*gal_type_sizeof(in->type));
          break;

        /* Two dimensional data. */
        case 2:
          a=in->dsize[0];
          b=in->dsize[1];
          if(c_dim) /* c_dim==1 dim. to collapse, already contiguous. */
            memcpy(work->array,
                   gal_pointer_increment(in->array,   index*b, in->type),
                   b*gal_type_sizeof(in->type));
          else      /* c_dim==0 */
            for(j=0;j<a;++j) dimension_csb_copy(in, j*b+index, work, j);
          break;

        /* Three dimensional data. */
        case 3:
          a=in->dsize[0];
          b=in->dsize[1];
          c=in->dsize[2];
          switch(c_dim)
            {
            case 0:
              for(j=0;j<a;++j)
                dimension_csb_copy(in, j*b*c+index, work, j);
              break;

            case 1:
              for(j=0;j<b;++j)
                dimension_csb_copy(in, (index/c)*b*c+j*c+(index%c),
                                   work, j);
              break;

            case 2: /* Fastest dimension: contiguous in memory. */
              memcpy(work->array,
                     gal_pointer_increment(in->array,
                                           (index/b)*b*c+(index%b)*c,
                                           in->type),
                     c*gal_type_sizeof(in->type));
              break;

            default:
              error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at "
                    "'%s' to solve the problem. The dimension counter "
                    "%zu isn't recognized for a 3D dataset", __func__,
                    PACKAGE_BUGREPORT, c_dim);
            }
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
                "to find the cause. This function doesn't support %zu "
                "dimensions", __func__, PACKAGE_BUGREPORT, in->ndim);
        }

      /* For a check
      if(index==250)
      {
        double *f=work->array;
        for(j=0;j<wdsize;++j)
          printf("%zu  %f\n", j, f[j]);
        printf("%s: GOOD\n", __func__); //exit(0);
      }
      */

      /* Set the necessary flag for extra calculation during sigma-clipping
         (this is not necessary for some). */
      switch(p->operator)
        {
        case DIMENSION_COLLAPSE_MADCLIP_STD:
        case DIMENSION_COLLAPSE_MADCLIP_FILL_STD:
          clipflags=GAL_STATISTICS_CLIP_OUTCOL_OPTIONAL_STD;  break;
        case DIMENSION_COLLAPSE_SIGCLIP_MAD:
        case DIMENSION_COLLAPSE_SIGCLIP_FILL_MAD:
          clipflags=GAL_STATISTICS_CLIP_OUTCOL_OPTIONAL_MAD;  break;
        case DIMENSION_COLLAPSE_MADCLIP_MEAN:
        case DIMENSION_COLLAPSE_SIGCLIP_MEAN:
        case DIMENSION_COLLAPSE_MADCLIP_FILL_MEAN:
        case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEAN:
          clipflags=GAL_STATISTICS_CLIP_OUTCOL_OPTIONAL_MEAN; break;
        }

      /* Create a convolved image (currently disabled, because 'work' will
         always be non-NULL, kept for further investigation later!). */
      conv=work?NULL:dimension_collapse_sortbased_conv(work);

      /* Do the necessary statistical operation. */
      stat=dimension_collapse_sortbased_operation(p, conv?conv:work,
                                                  clipflags, isfill,
                                                  wdsize);

      /* If this is a "filling" operation, then repeat the operation with
         the fill. */
      if(isfill)
        stat=dimension_collapse_sortbased_fill(p, stat, work, conv,
                                               clipflags, wdsize,
                                               index==-1);

      /* Set the index in the output 'stat' array. These can't be set in
         the main operation 'switch' because the functions are different,
         while 'sind' is the same. Note also that this should be done after
         the actual operation because some operators need to correct the
         type. */
      switch(p->operator)
        {
        case DIMENSION_COLLAPSE_MEDIAN:
          stat=gal_data_copy_to_new_type_free(stat, GAL_TYPE_FLOAT32);
          sind=0; break;
        case DIMENSION_COLLAPSE_MADCLIP_MAD:
        case DIMENSION_COLLAPSE_SIGCLIP_MAD:
        case DIMENSION_COLLAPSE_MADCLIP_FILL_MAD:
        case DIMENSION_COLLAPSE_SIGCLIP_FILL_MAD:
          sind=GAL_STATISTICS_CLIP_OUTCOL_MAD; break;
        case DIMENSION_COLLAPSE_MADCLIP_STD:
        case DIMENSION_COLLAPSE_SIGCLIP_STD:
        case DIMENSION_COLLAPSE_MADCLIP_FILL_STD:
        case DIMENSION_COLLAPSE_SIGCLIP_FILL_STD:
          sind=GAL_STATISTICS_CLIP_OUTCOL_STD; break;
        case DIMENSION_COLLAPSE_MADCLIP_MEAN:
        case DIMENSION_COLLAPSE_SIGCLIP_MEAN:
        case DIMENSION_COLLAPSE_MADCLIP_FILL_MEAN:
        case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEAN:
          sind=GAL_STATISTICS_CLIP_OUTCOL_MEAN; break;
        case DIMENSION_COLLAPSE_MADCLIP_MEDIAN:
        case DIMENSION_COLLAPSE_SIGCLIP_MEDIAN:
        case DIMENSION_COLLAPSE_MADCLIP_FILL_MEDIAN:
        case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEDIAN:
          sind=GAL_STATISTICS_CLIP_OUTCOL_MEDIAN; break;
        case DIMENSION_COLLAPSE_MADCLIP_NUMBER:
        case DIMENSION_COLLAPSE_SIGCLIP_NUMBER:
        case DIMENSION_COLLAPSE_MADCLIP_FILL_NUMBER:
        case DIMENSION_COLLAPSE_SIGCLIP_FILL_NUMBER:
          stat=gal_data_copy_to_new_type_free(stat, GAL_TYPE_UINT32);
          sind=GAL_STATISTICS_CLIP_OUTCOL_NUMBER_USED; break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
                "to fix the problem. The operator code '%d' is not "
                "recognized when writing the output", __func__,
                PACKAGE_BUGREPORT, p->operator);
        }

      /* Copy the result from the statistics output into the output array
         on the desired index, then free the 'stat' array. */
      memcpy(gal_pointer_increment(p->out->array, index, p->out->type),
             gal_pointer_increment(stat->array,   sind,  stat->type),
             gal_type_sizeof(stat->type));
      gal_data_free(stat);
    }

  /* Clean up. */
  gal_data_free(work);

  /* Wait for all the other threads to finish, then return. */
  if(tprm->b) pthread_barrier_wait(tprm->b);
  return NULL;
}





static gal_data_t *
dimension_collapse_sortbased(gal_data_t *in, size_t c_dim, int operator,
                             float sclipmultip, float sclipparam,
                             size_t numthreads, size_t minmapsize,
                             int quietmmap)
{
  size_t cnum=0;
  gal_data_t *out;
  double *warr=NULL;
  int otype=GAL_TYPE_INVALID;
  size_t outdsize[10], outndim;
  struct dimension_sortbased_p p;

  /* During the median calculation, blank elements will automatically be
     removed. */
  int hasblank=0;

  /* Basic sanity checks. */
  if( dimension_collapse_sanity_check(in, NULL, c_dim, hasblank,
                                      &cnum, &warr)!=NULL )
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to fix "
          "the problem. This functions should always return NULL here",
          __func__, PACKAGE_BUGREPORT);

  /* Set the size of the collapsed output. */
  dimension_collapse_sizes(in, c_dim, &outndim, outdsize);

  /* The output array (and its type). */
  switch(operator)
    {
    case DIMENSION_COLLAPSE_MADCLIP_NUMBER:
    case DIMENSION_COLLAPSE_SIGCLIP_NUMBER:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_NUMBER:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_NUMBER:
      otype=GAL_TYPE_UINT32;  break;
    case DIMENSION_COLLAPSE_MEDIAN:
    case DIMENSION_COLLAPSE_MADCLIP_MAD:
    case DIMENSION_COLLAPSE_SIGCLIP_MAD:
    case DIMENSION_COLLAPSE_MADCLIP_STD:
    case DIMENSION_COLLAPSE_SIGCLIP_STD:
    case DIMENSION_COLLAPSE_MADCLIP_MEAN:
    case DIMENSION_COLLAPSE_SIGCLIP_MEAN:
    case DIMENSION_COLLAPSE_MADCLIP_MEDIAN:
    case DIMENSION_COLLAPSE_SIGCLIP_MEDIAN:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MAD:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MAD:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_STD:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_STD:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MEAN:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEAN:
    case DIMENSION_COLLAPSE_MADCLIP_FILL_MEDIAN:
    case DIMENSION_COLLAPSE_SIGCLIP_FILL_MEDIAN:
      otype=GAL_TYPE_FLOAT32; break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' "
            "to fix the problem. The operator code %d is not a "
            "recognized operator ID", __func__, PACKAGE_BUGREPORT,
            operator);
    }
  out=gal_data_alloc(NULL, otype, outndim, outdsize, in->wcs,
                     1, in->minmapsize, in->quietmmap, NULL, NULL, NULL);

  /* Spin-off the threads and do the processing on each thread. */
  p.in=in;
  p.out=out;
  p.c_dim=c_dim;
  p.operator=operator;
  p.quietmmap=quietmmap;
  p.minmapsize=minmapsize;
  p.sclipparam=sclipparam;
  p.sclipmultip=sclipmultip;
  gal_threads_spin_off(dimension_collapse_sortbased_worker, &p,
                       out->size, numthreads,
                       minmapsize, quietmmap);

  /* Remove the respective dimension in the WCS structure also (if any
     exists). Note that 'out->ndim' has already been changed. So we'll use
     'in->wcs'. */
  gal_wcs_remove_dimension(out->wcs, in->ndim-c_dim);

  /* Return the collapsed dataset. */
  return out;
}





gal_data_t *
gal_dimension_collapse_median(gal_data_t *in, size_t c_dim,
                              size_t numthreads, size_t minmapsize,
                              int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MEDIAN;
  return dimension_collapse_sortbased(in, c_dim, op, NAN, NAN,
                                      numthreads, minmapsize,
                                      quietmmap);
}




gal_data_t *
gal_dimension_collapse_mclip_mad(gal_data_t *in, size_t c_dim,
                                 float multip, float param,
                                 size_t numthreads, size_t minmapsize,
                                 int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_MAD;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_fill_mad(gal_data_t *in, size_t c_dim,
                                      float multip, float param,
                                      size_t numthreads, size_t minmapsize,
                                      int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_FILL_MAD;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_std(gal_data_t *in, size_t c_dim,
                                 float multip, float param,
                                 size_t numthreads, size_t minmapsize,
                                 int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_STD;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_fill_std(gal_data_t *in, size_t c_dim,
                                      float multip, float param,
                                      size_t numthreads, size_t minmapsize,
                                      int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_FILL_STD;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_mean(gal_data_t *in, size_t c_dim,
                                  float multip, float param,
                                  size_t numthreads, size_t minmapsize,
                                  int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_MEAN;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_fill_mean(gal_data_t *in, size_t c_dim,
                                       float multip, float param,
                                       size_t numthreads, size_t minmapsize,
                                       int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_FILL_MEAN;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_median(gal_data_t *in, size_t c_dim,
                                    float multip, float param,
                                    size_t numthreads, size_t minmapsize,
                                    int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_MEDIAN;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_fill_median(gal_data_t *in, size_t c_dim,
                                         float multip, float param,
                                         size_t numthreads,
                                         size_t minmapsize, int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_FILL_MEDIAN;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_number(gal_data_t *in, size_t c_dim,
                                    float multip, float param,
                                    size_t numthreads, size_t minmapsize,
                                    int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_NUMBER;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_mclip_fill_number(gal_data_t *in, size_t c_dim,
                                         float multip, float param,
                                         size_t numthreads,
                                         size_t minmapsize, int quietmmap)
{
  int op=DIMENSION_COLLAPSE_MADCLIP_FILL_NUMBER;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_mad(gal_data_t *in, size_t c_dim,
                                 float multip, float param,
                                 size_t numthreads, size_t minmapsize,
                                 int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_MAD;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_fill_mad(gal_data_t *in, size_t c_dim,
                                      float multip, float param,
                                      size_t numthreads, size_t minmapsize,
                                      int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_FILL_MAD;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_std(gal_data_t *in, size_t c_dim,
                                 float multip, float param,
                                 size_t numthreads, size_t minmapsize,
                                 int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_STD;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_fill_std(gal_data_t *in, size_t c_dim,
                                      float multip, float param,
                                      size_t numthreads, size_t minmapsize,
                                      int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_FILL_STD;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_mean(gal_data_t *in, size_t c_dim,
                                  float multip, float param,
                                  size_t numthreads, size_t minmapsize,
                                  int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_MEAN;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_fill_mean(gal_data_t *in, size_t c_dim,
                                       float multip, float param,
                                       size_t numthreads, size_t minmapsize,
                                       int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_FILL_MEAN;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_median(gal_data_t *in, size_t c_dim,
                                    float multip, float param,
                                    size_t numthreads, size_t minmapsize,
                                    int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_MEDIAN;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_fill_median(gal_data_t *in, size_t c_dim,
                                         float multip, float param,
                                         size_t numthreads,
                                         size_t minmapsize, int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_FILL_MEDIAN;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_number(gal_data_t *in, size_t c_dim,
                                    float multip, float param,
                                    size_t numthreads, size_t minmapsize,
                                    int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_NUMBER;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_dimension_collapse_sclip_fill_number(gal_data_t *in, size_t c_dim,
                                         float multip, float param,
                                         size_t numthreads,
                                         size_t minmapsize, int quietmmap)
{
  int op=DIMENSION_COLLAPSE_SIGCLIP_FILL_NUMBER;
  return dimension_collapse_sortbased(in, c_dim, op, multip, param,
                                      numthreads, minmapsize, quietmmap);
}




















/************************************************************************/
/********************             Other            **********************/
/************************************************************************/
size_t
gal_dimension_remove_extra(size_t ndim, size_t *dsize, struct wcsprm *wcs)
{
  size_t i, j, size=1;

  /* When there is only one element in the dataset, "extra" dimensions are
     not defined. This is because we define "extra" by dimensions of length
     1, so when there is only a single element everything will be "extra"
     and the function will return 0 dimensions!  See
     https://savannah.gnu.org/bugs/index.php?65833 for example.  Therefore,
     the dimensions should be preserved and this function should not do
     anything. */
  for(i=0;i<ndim;++i) size*=dsize[i];
  if(size>1)
    {
      for(i=0;i<ndim;++i)
        if(dsize[i]==1)
          {
            /* Correct the WCS. */
            if(wcs) gal_wcs_remove_dimension(wcs, ndim-i);

            /* Shift all subsequent dimensions to replace this one. */
            for(j=i;j<ndim-1;++j) dsize[j]=dsize[j+1];

            /* Decrement the 'i' and the total number of dimension. */
            --i;
            --ndim;
          }
    }

  /* Return the number of dimensions. */
  return ndim;
}
