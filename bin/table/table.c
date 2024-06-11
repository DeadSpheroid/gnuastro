/*********************************************************************
Table - View and manipulate a FITS table structures.
Table is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <gsl/gsl_rng.h>

#include <gnuastro/txt.h>
#include <gnuastro/wcs.h>
#include <gnuastro/fits.h>
#include <gnuastro/list.h>
#include <gnuastro/table.h>
#include <gnuastro/qsort.h>
#include <gnuastro/pointer.h>
#include <gnuastro/polygon.h>
#include <gnuastro/arithmetic.h>
#include <gnuastro/statistics.h>
#include <gnuastro/permutation.h>

#include <gnuastro-internal/checkset.h>

#include "main.h"

#include "ui.h"
#include "arithmetic.h"





/**************************************************************/
/********     Selecting and ordering of columns      **********/
/**************************************************************/
static void
table_error_no_column(char *optionname, char *id)
{
  error(EXIT_FAILURE, 0, "no column could be found with the '%s' "
        "identifier (given to '%s'). The value to this option can "
        "either be a column name or counter (counting from 1). For "
        "more on how to select columns in Gnuastro, please run the "
        "command below (press 'q' to come back to the command-line):\n\n"
        "    info gnuastro \"selecting table columns\"\n",
        id, optionname);
}





static void
table_apply_permutation(gal_data_t *table, size_t *permutation,
                        size_t permsize, int inverse)
{
  gal_data_t *tmp;

  for(tmp=table;tmp!=NULL;tmp=tmp->next)
    {
      /* Apply the permutation. */
      if(inverse)
        {
          if(tmp->ndim==1)
            gal_permutation_apply_inverse(tmp, permutation);
          else
            error(EXIT_FAILURE, 0, "%s: inverse permutation on "
                  "vector columns is not yet supported. Please "
                  "get in touch with us at '%s' to add this "
                  "feature", __func__, PACKAGE_BUGREPORT);
        }
      else
        {
          if(tmp->ndim==1) gal_permutation_apply(tmp, permutation);
          else    gal_permutation_apply_onlydim0(tmp, permutation);
        }

      /* Correct the size. */
      tmp->dsize[0]=permsize;
      tmp->size = tmp->dsize[0] * (tmp->ndim==1 ? 1 : tmp->dsize[1]);
    }
}





static void
table_bring_to_top(gal_data_t *table, gal_data_t *rowids)
{
  char **strarr;
  gal_data_t *col;
  size_t i, n, *ids=rowids->array;

  /* Make sure the rowids are sorted by increasing index.
  gal_statistics_sort_increasing(rowids);
  */

  /* Go over each column and move the desired rows to the top. */
  for(col=table;col!=NULL;col=col->next)
    {
      /* For easy operation if the column is a string or vector. */
      strarr = col->type==GAL_TYPE_STRING ? col->array : NULL;
      n = col->ndim==1 ? 1 : col->dsize[1];

      /* Move the desired rows up to the top. */
      for(i=0;i<rowids->size;++i)
        if( i != ids[i] )
          {
            /* Copy the contents. For strings, its just about freeing and
               copying pointers. */
            if(col->type==GAL_TYPE_STRING)
              {
                free(strarr[i]);
                strarr[i]=strarr[ ids[i] ];
                strarr[ ids[i] ]=NULL;
              }
            else
              {
                memcpy(gal_pointer_increment(col->array, i*n,     col->type),
                       gal_pointer_increment(col->array, ids[i]*n,col->type),
                       n * gal_type_sizeof(col->type));
              }
          }

      /* For string arrays, free the pointers of the remaining rows. */
      if(col->type==GAL_TYPE_STRING)
        for(i=rowids->size;i<col->size;++i)
          if(strarr[i]) free(strarr[i]);

      /* Correct the size (this should be after freeing of the string
         pointers. */
      col->dsize[0] = rowids->size;
      col->size = col->dsize[0] * n;

      /* If there is no elements, free 'array' and set it to NULL. */
      if(col->size==0 && col->array) { free(col->array); col->array=NULL; }
    }
}





static gal_data_t *
table_selection_range(struct tableparams *p, gal_data_t *col)
{
  size_t one=1;
  double *darr;
  int numok=GAL_ARITHMETIC_FLAG_NUMOK;
  int inplace=GAL_ARITHMETIC_FLAG_INPLACE;
  gal_data_t *min=NULL, *max=NULL, *tmp, *ltmin, *gemax=NULL;

  /* First, make sure everything is OK. */
  if(p->range==NULL)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us to fix the "
          "problem at %s. 'p->range' should not be NULL at this point",
          __func__, PACKAGE_BUGREPORT);

  /* Allocations. */
  min=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &one, NULL, 0, -1, 1,
                     NULL, NULL, NULL);
  max=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &one, NULL, 0, -1, 1,
                     NULL, NULL, NULL);

  /* Read the range of values for this column. */
  darr=p->range->array;
  ((double *)(min->array))[0] = darr[0];
  ((double *)(max->array))[0] = darr[1];

  /* Move 'p->range' to the next element in the list and free the current
     one (we have already read its values and don't need it any more). */
  tmp=p->range;
  p->range=p->range->next;
  gal_data_free(tmp);

  /* Find all the elements outside this range (smaller than the minimum,
     larger than the maximum or blank) as separate binary flags. */
  ltmin=gal_arithmetic(GAL_ARITHMETIC_OP_LT, 1, numok, col, min);
  gemax=gal_arithmetic(GAL_ARITHMETIC_OP_GE, 1, numok, col, max);

  /* Merge them both into one array. */
  ltmin=gal_arithmetic(GAL_ARITHMETIC_OP_OR, 1, inplace, ltmin, gemax);

  /* For a check.
  {
    size_t i;
    uint8_t *u=ltmin->array;
    for(i=0;i<ltmin->size;++i) printf("%zu: %u\n", i, u[i]);
    exit(0);
  }
  */

  /* Clean up and return. */
  gal_data_free(gemax);
  gal_data_free(min);
  gal_data_free(max);
  return ltmin;
}





/* Read column value of any type as a double for the polygon options. */
static double
selection_polygon_read_point(gal_data_t *col, size_t i)
{
  /* Check and assign the points to the points array. */
  switch(col->type)
    {
    case GAL_TYPE_INT8:    return (( int8_t   *)col->array)[i];
    case GAL_TYPE_UINT8:   return (( uint8_t  *)col->array)[i];
    case GAL_TYPE_UINT16:  return (( uint16_t *)col->array)[i];
    case GAL_TYPE_INT16:   return (( int16_t  *)col->array)[i];
    case GAL_TYPE_UINT32:  return (( uint32_t *)col->array)[i];
    case GAL_TYPE_INT32:   return (( int32_t  *)col->array)[i];
    case GAL_TYPE_UINT64:  return (( uint64_t *)col->array)[i];
    case GAL_TYPE_INT64:   return (( int64_t  *)col->array)[i];
    case GAL_TYPE_FLOAT32: return (( float    *)col->array)[i];
    case GAL_TYPE_FLOAT64: return (( double   *)col->array)[i];
    default:
      error(EXIT_FAILURE, 0, "%s: type code %d not recognized",
            __func__, col->type);
    }

  /* Control should not reach here. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return NAN;
}





/* Mask the rows that are not in the given polygon. */
static gal_data_t *
table_selection_polygon(struct tableparams *p, gal_data_t *col1,
                        gal_data_t *col2, int in1out0)
{
  uint8_t *oarr;
  double point[2];
  gal_data_t *out=NULL;
  size_t i, psize=p->polygon->size/2;

  /* Allocate the output array: This array will have a '0' for the points
     which are inside the polygon and '1' for those that are outside of it
     (to be masked/removed from the input). */
  out=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, col1->dsize, NULL, 0, -1, 1,
                     NULL, NULL, NULL);
  oarr=out->array;

  /* Loop through all the rows in the given columns and check the points. */
  for(i=0; i<col1->size; i++)
    {
      /* Read the column values as a double. */
      point[0]=selection_polygon_read_point(col1, i);
      point[1]=selection_polygon_read_point(col2, i);

      /* For '--inpolygon', if point is inside polygon, put 0, otherwise
         1. Note that we are building a mask for the rows that must be
         discarded, so we want '1' for the points we don't want. */
      oarr[i] = (in1out0
                 ? !gal_polygon_is_inside(p->polygon->array, point, psize)
                 :  gal_polygon_is_inside(p->polygon->array, point, psize));

      /* For a check
      printf("(%f,%f): %s, %u\n", point[0], point[1], oarr[i]);
      */
    }

  /* Return the output column. */
  return out;
}





/* Given a string dataset and a single string, return a 'uint8_t' array
   with the same size as the string dataset that has a '1' for all the
   elements that are equal. */
static gal_data_t *
table_selection_string_eq_ne(gal_data_t *column, char *reference, int e0n1)
{
  gal_data_t *out;
  uint8_t *oarr, comp;
  size_t i, size=column->size;
  char **strarr=column->array;

  /* Allocate the output binary dataset. */
  out=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &size, NULL, 0, -1, 1,
                     NULL, NULL, NULL);
  oarr=out->array;

  /* Parse the values and mark the outputs IN THE OPPOSITE manner (we are
     marking the ones that must be removed). */
  for(i=0;i<size;++i)
    {
      comp=strcmp(strarr[i], reference);
      oarr[i] = e0n1 ? (comp==0) : (comp!=0);
    }

  /* Return. */
  return out;
}





static gal_data_t *
table_selection_equal_or_notequal(struct tableparams *p, gal_data_t *col,
                                  int e0n1)
{
  void *varr;
  char **strarr;
  size_t i, one=1;
  int numok=GAL_ARITHMETIC_FLAG_NUMOK;
  int inplace=GAL_ARITHMETIC_FLAG_INPLACE;
  gal_data_t *eq, *out=NULL, *value=NULL;
  gal_data_t *arg = e0n1 ? p->notequal : p->equal;

  /* Note that this operator is used to make the "masked" array, so when
     'e0n1==0' the operator should be 'GAL_ARITHMETIC_OP_NE' and
     vice-versa.

     For the merging with other elements, when 'e0n1==0', we need the
     'GAL_ARITHMETIC_OP_AND', but for 'e0n1==1', it should be 'OR'. */
  int mergeop  = e0n1 ? GAL_ARITHMETIC_OP_OR : GAL_ARITHMETIC_OP_AND;
  int operator = e0n1 ? GAL_ARITHMETIC_OP_EQ : GAL_ARITHMETIC_OP_NE;

  /* First, make sure everything is OK. */
  if(arg==NULL)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us to fix the "
          "problem at %s. 'p->range' should not be NULL at this point",
          __func__, PACKAGE_BUGREPORT);

  /* To easily parse the given values. */
  strarr=arg->array;

  /* Go through the values given to this call of the option and flag the
     elements. */
  for(i=0;i<arg->size;++i)
    {
      /* Write the value. */
      if(col->type==GAL_TYPE_STRING)
        eq=table_selection_string_eq_ne(col, strarr[i], e0n1);
      else
        {
          /* Allocate the value dataset. */
          value=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &one, NULL, 0,
                               -1, 1, NULL, NULL, NULL);
          varr=value->array;

          /* Read the stored string as a float64. */
          if( gal_type_from_string(&varr, strarr[i], GAL_TYPE_FLOAT64) )
            {
              fprintf(stderr, "%s couldn't be read as a number.\n", strarr[i]);
              exit(EXIT_FAILURE);
            }

          /* Mark the rows that are equal (irrespective of the column's
             original numerical datatype). */
          eq=gal_arithmetic(operator, 1, numok, col, value);
        }

      /* Merge the results with (possible) previous results. */
      if(out)
        {
          out=gal_arithmetic(mergeop, 1, inplace, out, eq);
          gal_data_free(eq);
        }
      else
        out=eq;
    }

  /* For a check.
  {
    uint8_t *u=out->array;
    for(i=0;i<out->size;++i) printf("%zu: %u\n", i, u[i]);
    exit(0);
  }
  */


  /* Move the main pointer to the next possible call of the given
     option. Note that 'arg' already points to 'p->equal' or 'p->notequal',
     so it will automatically be freed with the next step. */
  if(e0n1) p->notequal=p->notequal->next;
  else     p->equal=p->equal->next;

  /* Clean up and return. */
  gal_data_free(value);
  gal_data_free(arg);
  return out;
}





static void
table_select_by_value(struct tableparams *p)
{
  gal_data_t *rowids;
  size_t i, *s, ngood=0;
  struct list_select *tmp;
  uint8_t *u, *uf, *ustart;
  int inplace=GAL_ARITHMETIC_FLAG_INPLACE;
  gal_data_t *mask, *col, *blmask, *addmask=NULL;

  /* It may happen that the input table is empty! In such cases, just
     return and don't bother with this step. */
  if(p->table->size==0 || p->table->array==NULL || p->table->dsize==NULL)
    return;

  /* Allocate datasets for the necessary numbers and write them in. */
  mask=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &p->table->dsize[0],
                      NULL, 1, p->cp.minmapsize, p->cp.quietmmap,
                      NULL, NULL, NULL);

  /* Go over each selection criteria and remove the necessary elements. */
  for(tmp=p->selectcol;tmp!=NULL;tmp=tmp->next)
    {
      /* Make sure the selection column isn't a vector column. */
      if(tmp->col->ndim!=1)
        error(EXIT_FAILURE, 0, "row selection by value (for example with "
              "'--range', '--inpolygon', '--equal' or '--noblank') is "
              "currently not available for vector columns. If you need "
              "this feature, please get in touch with us at '%s' to add "
              "it", PACKAGE_BUGREPORT);

      /* Make sure all the to-be-used table columns at this point have the
         same number of rows as the selection column (can be different when
         transposing). */
      for(col=p->table;col!=NULL;col=col->next)
        if(tmp->col->dsize[0]!=col->dsize[0])
          error(EXIT_FAILURE, 0, "the number of rows in the column given "
                "for selection by value (for example '--range' or '--equal') "
                "is not the same as the number of rows in the table when "
                "they are applied. This may happen due to operators like "
                "'--transpose'. In such cases, you probably want row-based "
                "operators to have precedence over column-based operators. "
                "If this is your case, please call '--rowfirst'");

      /* Do the specific type of selection. */
      switch(tmp->type)
        {
        case SELECT_TYPE_RANGE:
          addmask=table_selection_range(p, tmp->col);
          break;

        /* '--inpolygon' and '--outpolygon' need two columns. */
        case SELECT_TYPE_INPOLYGON:
        case SELECT_TYPE_OUTPOLYGON:
          addmask=table_selection_polygon(p, tmp->col, tmp->next->col,
                                          tmp->type==SELECT_TYPE_INPOLYGON);
          tmp=tmp->next;
          break;

        case SELECT_TYPE_EQUAL:
          addmask=table_selection_equal_or_notequal(p, tmp->col, 0);
          break;

        case SELECT_TYPE_NOTEQUAL:
          addmask=table_selection_equal_or_notequal(p, tmp->col, 1);
          break;

        case SELECT_TYPE_NOBLANK:
          addmask = gal_arithmetic(GAL_ARITHMETIC_OP_ISBLANK, 1, 0,
                                   tmp->col);
          break;

        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s "
                "to fix the problem. The code %d is not a recognized "
                "range identifier", __func__, PACKAGE_BUGREPORT,
                tmp->type);
        }

      /* Remove any blank elements (incase we are on a noblank column. */
      if(tmp->type!=SELECT_TYPE_NOBLANK && gal_blank_present(tmp->col, 1))
        {
          blmask = gal_arithmetic(GAL_ARITHMETIC_OP_ISBLANK, 1, 0,
                                  tmp->col);
          addmask=gal_arithmetic(GAL_ARITHMETIC_OP_OR, 1, inplace,
                                 addmask, blmask);
          gal_data_free(blmask);
        }

      /* Add this mask array to the cumulative mask array (of all
         selections). */
      mask=gal_arithmetic(GAL_ARITHMETIC_OP_OR, 1, inplace, mask, addmask);

      /* For a check.
         {
           float *f=ref->array;
           uint8_t *m=mask->array;
           uint8_t *u=addmask->array, *uf=u+addmask->size;
           printf("\n\nInput column: %s\n", ref->name?ref->name:"No Name");
           printf("Range: %g, %g\n", rarr[0], rarr[1]);
           printf("%-20s%-20s%-20s\n", "Value", "This mask",
           "Including previous");
           do printf("%-20f%-20u%-20u\n", *f++, *u++, *m++); while(u<uf);
           exit(0);
           }
        */

      /* Final clean up. */
      gal_data_free(addmask);
    }

  /* Find the final number of elements to print and allocate the array to
     keep them. */
  uf=(u=mask->array)+mask->size;
  ngood=0; do if(*u==0) ++ngood; while(++u<uf);
  rowids=gal_data_alloc(NULL, GAL_TYPE_SIZE_T, 1, &ngood, NULL, 0,
                        p->cp.minmapsize, p->cp.quietmmap, NULL,
                        NULL, NULL);

  /* Fill-in the finally desired row-IDs. */
  s=rowids->array;
  ustart=mask->array;
  uf=(u=mask->array)+mask->size;
  do if(*u==0) *s++ = u-ustart; while(++u<uf);

  /* Move the desired rows to the top of the table. */
  table_bring_to_top(p->table, rowids);

  /* If the sort column is not in the table (the proper range has already
     been applied to it), and we need to sort the resulting columns
     afterwards, we should also apply the permutation on the sort
     column. */
  if(p->sortcol && p->sortin==0) table_bring_to_top(p->sortcol, rowids);

  /* Clean up. */
  i=0;
  for(tmp=p->selectcol;tmp!=NULL;tmp=tmp->next)
    { if(p->freeselect[i]) {gal_data_free(tmp->col); tmp->col=NULL;} ++i; }
  ui_list_select_free(p->selectcol, 0);
  free(p->freeselect);
  gal_data_free(mask);
  gal_data_free(rowids);
}





static void
table_sort(struct tableparams *p)
{
  gal_data_t *perm;
  size_t c=0, *s, *sf, dsize0=p->table->dsize[0];
  int (*qsortfn)(const void *, const void *)=NULL;

  /* In case there are no columns to sort, skip this function. */
  if(p->table->size==0 || p->table->array==NULL || p->table->dsize==NULL)
    return;

  /* Allocate the permutation array and fill it. Note that we need 'dsize0'
     because the first column may be a vector column (which is 2D). */
  perm=gal_data_alloc(NULL, GAL_TYPE_SIZE_T, 1, &dsize0, NULL, 0,
                      p->cp.minmapsize, p->cp.quietmmap, NULL, NULL, NULL);
  sf=(s=perm->array)+perm->size; do *s=c++; while(++s<sf);

  /* For string columns, print a descriptive message. Note that some FITS
     tables were found that do actually have numbers stored in string
     types! */
  if(p->sortcol->type==GAL_TYPE_STRING)
    error(EXIT_FAILURE, 0, "sort column has a string type, but it can "
          "(currently) only work on numbers.\n\n"
          "TIP: if you know the columns contents are all numbers that are "
          "just stored as strings, you can use this program to save the "
          "table as a text file, modify the column meta-data (for example "
          "to type 'i32' or 'f32' instead of 'strN'), then use this "
          "program again to save it as a FITS table.\n\n"
          "For more on column metadata in plain text format, please run "
          "the following command (or see the 'Gnuastro text table format "
          "section of the book/manual):\n\n"
          "    $ info gnuastro \"gnuastro text table format\"");

  /* Set the proper qsort function. */
  if(p->descending)
    switch(p->sortcol->type)
      {
      case GAL_TYPE_UINT8:   qsortfn=gal_qsort_index_single_uint8_d;   break;
      case GAL_TYPE_INT8:    qsortfn=gal_qsort_index_single_int8_d;    break;
      case GAL_TYPE_UINT16:  qsortfn=gal_qsort_index_single_uint16_d;  break;
      case GAL_TYPE_INT16:   qsortfn=gal_qsort_index_single_int16_d;   break;
      case GAL_TYPE_UINT32:  qsortfn=gal_qsort_index_single_uint32_d;  break;
      case GAL_TYPE_INT32:   qsortfn=gal_qsort_index_single_int32_d;   break;
      case GAL_TYPE_UINT64:  qsortfn=gal_qsort_index_single_uint64_d;  break;
      case GAL_TYPE_INT64:   qsortfn=gal_qsort_index_single_int64_d;   break;
      case GAL_TYPE_FLOAT32: qsortfn=gal_qsort_index_single_float32_d; break;
      case GAL_TYPE_FLOAT64: qsortfn=gal_qsort_index_single_float64_d; break;
      default:
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix "
              "the problem. The code '%u' wasn't recognized as a data type",
              __func__, PACKAGE_BUGREPORT, p->sortcol->type);
      }
  else
    switch(p->sortcol->type)
      {
      case GAL_TYPE_UINT8:   qsortfn=gal_qsort_index_single_uint8_i;   break;
      case GAL_TYPE_INT8:    qsortfn=gal_qsort_index_single_int8_i;    break;
      case GAL_TYPE_UINT16:  qsortfn=gal_qsort_index_single_uint16_i;  break;
      case GAL_TYPE_INT16:   qsortfn=gal_qsort_index_single_int16_i;   break;
      case GAL_TYPE_UINT32:  qsortfn=gal_qsort_index_single_uint32_i;  break;
      case GAL_TYPE_INT32:   qsortfn=gal_qsort_index_single_int32_i;   break;
      case GAL_TYPE_UINT64:  qsortfn=gal_qsort_index_single_uint64_i;  break;
      case GAL_TYPE_INT64:   qsortfn=gal_qsort_index_single_int64_i;   break;
      case GAL_TYPE_FLOAT32: qsortfn=gal_qsort_index_single_float32_i; break;
      case GAL_TYPE_FLOAT64: qsortfn=gal_qsort_index_single_float64_i; break;
      default:
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix "
              "the problem. The code '%u' wasn't recognized as a data type",
              __func__, PACKAGE_BUGREPORT, p->sortcol->type);
      }

  /* Sort the indexs from the values. */
  gal_qsort_index_single=p->sortcol->array;
  qsort(perm->array, perm->size, sizeof *s, qsortfn);

  /* For a check (only on float32 type 'sortcol'):
  {
    float *f=p->sortcol->array;
    sf=(s=perm->array)+perm->size;
    do printf("%f\n", f[*s]); while(++s<sf);
    exit(0);
  }
  */

  /* Sort all the output columns with this permutation. */
  table_apply_permutation(p->table, perm->array, perm->size, 0);

  /* Clean up. */
  gal_data_free(perm);
  if(p->freesort) gal_data_free(p->sortcol);
}





/* Apply random row selection. If the returned value is 'EXIT_SUCCESS',
   then, it was successful. Otherwise, it will return 'EXIT_FAILURE' and
   the input won't be touched. */
static int
table_random_rows(gal_data_t *table, gsl_rng *rng, size_t numrandom,
                  size_t minmapsize, int quietmmap)
{
  int bad;
  gal_data_t *rowids;
  size_t i, j, *ids, ind;

  /* Sanity check. */
  if(numrandom>table->size) return EXIT_FAILURE;

  /* Allocate space for the list of rows to use. */
  rowids=gal_data_alloc(NULL, GAL_TYPE_SIZE_T, 1, &numrandom, NULL, 0,
                        minmapsize, quietmmap, NULL, NULL, NULL);

  /* Select the row numbers. */
  ids=rowids->array;
  for(i=0;i<numrandom;++i)
    {
      /* Select a random index and make sure it is new. */
      bad=1;
      while(bad)
        {
          ind = gsl_rng_uniform(rng) * table->size;
          for(j=0;j<i;++j) if(ids[j]==ind) break;
          if(i==j) bad=0;
        }
      ids[i]=ind;
    }

  /* Move the desired rows to the top. */
  table_bring_to_top(table, rowids);

  /* Clean up and return. */
  gal_data_free(rowids);
  return EXIT_SUCCESS;
}





static void
table_select_by_position(struct tableparams *p)
{
  char **strarr;
  gal_data_t *col;
  size_t i, start, end, nelem;
  double *darr = p->rowrange ? p->rowrange->array : NULL;

  /* If the table is already empty, then don't bother continuing. */
  if(p->table->size==0 || p->table->array==NULL || p->table->dsize==NULL)
    return;

  /* If the head or tail values are given and are larger than the number of
     rows, just set them to the number of rows (print the all the final
     rows). This is how the 'head' and 'tail' programs of GNU Coreutils
     operate. Note that some columns may be vector (multi-value per
     column), in this case, they will be 2D. So we should use 'dsize[0]'
     for the generic way to find the number of rows. */
  p->head = ( ( (p->head!=GAL_BLANK_SIZE_T)
                && (p->head > p->table->dsize[0]) )
              ? p->table->dsize[0]
              : p->head );
  p->tail = ( ( (p->tail!=GAL_BLANK_SIZE_T)
                && (p->tail > p->table->dsize[0]) )
              ? p->table->dsize[0]
              : p->tail );

  /* Random row selection (by position, not value). This step is
     independent of the other operations of this function, so as soon as
     its finished return. */
  if(p->rowrandom)
    {
      if( table_random_rows(p->table, p->rng, p->rowrandom,
                            p->cp.minmapsize, p->cp.quietmmap)
          == EXIT_FAILURE && p->cp.quiet==0 )
        error(EXIT_SUCCESS, 0, "'--rowrandom' not activated because "
              "the number of rows in the table at this stage (%zu) "
              "is smaller than the number of requested random rows "
              "(%zu). You can suppress this message with '--quiet'",
              p->table->size, p->rowrandom);
      return;
    }

  /* Make sure the given values to '--rowrange' are within the number of
     rows until this point. */
  if(p->rowrange)
    {
      if(darr[0]>=p->table->dsize[0])
        error(EXIT_FAILURE, 0, "the first value to '--rowrange' (%g) "
              "is larger than the number of rows (%zu)",
              darr[0]+1, p->table->dsize[0]);
      else if( darr[1]>=p->table->dsize[0] )
        error(EXIT_FAILURE, 0, "the second value to '--rowrange' (%g) "
              "is larger than the number of rows (%zu)",
              darr[1]+1, p->table->dsize[0]);
    }

  /* Go over all the columns and make the necessary corrections. */
  for(col=p->table; col!=NULL; col=col->next)
    {
      /* Set the increment (number of elements in this column). For vector
         columns, this is the number of elements in each row, and for
         normal columns, this is 1. */
      nelem=col->size/col->dsize[0];

      /* FOR STRING: we'll need to free the individual strings that will
         not be used (outside the allocated array directly
         'gal_data_t'). We don't have to worry about the space for the
         actual pointers (they will be free'd by 'free' in any case, since
         they are in the initially allocated array). */
      if(col->type==GAL_TYPE_STRING)
        {
          /* Parse the rows and free extra pointers. */
          strarr=col->array;
          if(p->rowrange)
            {
              /* Note that the given values to '--rowrange' started from 1,
                 but in 'ui.c' we subtracted one from it (so at this stage,
                 it starts from 0). */
              start = darr[0];
              end   = darr[1];
              for(i=0;i<p->table->dsize[0];++i)
                if(i<start || i>end) { free(strarr[i]); strarr[i]=NULL; }
            }
          else
            {
              /* Set the starting and ending indexs to free the allocated
                 space of each string. */
              start = p->head!=GAL_BLANK_SIZE_T ? p->head : 0;
              end   = ( p->head!=GAL_BLANK_SIZE_T
                        ? p->table->dsize[0]
                        : p->table->dsize[0] - p->tail );
              for(i=start; i<end; ++i) { free(strarr[i]); strarr[i]=NULL; }
            }
        }

      /* Make the final adjustment. */
      if(p->rowrange)
        {
          /* Move the values up to the top and correct the size. */
          col->dsize[0]=darr[1]-darr[0]+1;
          memmove(col->array,
                  gal_pointer_increment(col->array, darr[0]*nelem,
                                        col->type),
                  (darr[1]-darr[0]+1)*nelem*gal_type_sizeof(col->type));
        }
      else
        {
          /* For '--tail', we'll need to bring the last columns to the
             start. Note that we are using 'memmove' because we want to be
             safe with overlap. */
          if(p->tail!=GAL_BLANK_SIZE_T)
            memmove(col->array,
                    gal_pointer_increment(col->array,
                                          (col->dsize[0]-p->tail)*nelem,
                                          col->type),
                    p->tail*nelem*gal_type_sizeof(col->type));

          /* In any case (head or tail), the new number of rows, then
             update the total number of elements (may be vector). */
          col->dsize[0] = p->head!=GAL_BLANK_SIZE_T ? p->head : p->tail;
        }

      /* The 'dsize[0]' component was set above, we should not update the
         total size. */
      col->size = col->dsize[0] * (col->ndim==1 ? 1 : col->dsize[1]);
    }
}





/* Import columns from another file/table into the working table. */
static void
table_catcolumn(struct tableparams *p)
{
  size_t counter=1;
  gal_list_str_t *filell, *hdull;
  char *tmpname, *hdu=NULL, cstr[100];
  gal_data_t *col, *tocat, *final, *newcol;
  struct gal_options_common_params *cp=&p->cp;

  /* Go over all the given files. */
  hdull=p->catcolumnhdu;
  for(filell=p->catcolumnfile; filell!=NULL; filell=filell->next)
    {
      /* Set the HDU (not necessary for non-FITS tables). */
      if(gal_fits_file_recognized(filell->v))
        {
          if(hdull) { hdu=hdull->v; hdull=hdull->next; }
          else
            error(EXIT_FAILURE, 0, "not enough '--catcolumnhdu's (or "
                  "'-u'). For every FITS table given to "
                  "'--catcolumnfile'. A call to '--catcolumnhdu' is "
                  "necessary to identify its HDU/extension");
        }
      else hdu=NULL;

      /* Read the catcolumn table. */
      tocat=gal_table_read(filell->v, hdu, NULL, p->catcolumns,
                           cp->searchin, cp->ignorecase,
                           cp->numthreads, cp->minmapsize,
                           p->cp.quietmmap, NULL, "--catcolumnhdu");

      /* Check the number of rows. */
      if(tocat->dsize[0]!=p->table->dsize[0])
        error(EXIT_FAILURE, 0, "%s: incorrect number of rows. The "
              "table given to '--catcolumn' must have the same number "
              "of rows as the main argument (after all row-selections "
              "have been applied), but they have %zu and %zu rows "
              "respectively",
              gal_fits_name_save_as_string(filell->v, hdu), tocat->dsize[0],
              p->table->dsize[0]);

      /* Append a counter to the column names because this option is most
         often used with columns that have a similar name and it would help
         the user if the output doesn't have multiple columns with same
         name. */
      if(p->catcolumnrawname==0)
        for(newcol=tocat; newcol!=NULL; newcol=newcol->next)
          if(newcol->name)
            {
              /* If the new column's name is identical (case-insensitive)
                 to an existing name in the existing table so far, we need
                 to add a suffix. */
              for(col=p->table; col!=NULL; col=col->next)
                if( col->name && strcasecmp(col->name, newcol->name)==0 )
                  {
                    /* Add the counter suffix to the column name. */
                    sprintf(cstr, "-%zu", counter);
                    tmpname=gal_checkset_malloc_cat(newcol->name, cstr);

                    /* Free the old name and put in the new one. */
                    free(newcol->name);
                    newcol->name=tmpname;
                  }
            }

      /* Find the final column of the main table and add this table. */
      final=gal_list_data_last(p->table);
      final->next=tocat;
      ++counter;
    }
}





static void
table_transpose(struct tableparams *p)
{
  gal_data_t *tmp;
  size_t refwidth=p->table->dsize[1];

  /* Temporary error. */
  if(p->colpack)
    error(EXIT_FAILURE, 0, "'--transpose' is currently not supported "
          "with Column Arithmetic. Please pipe the transposed table "
          "into a new 'asttable' command for doing column arithmetic "
          "on the new columns");

  /* Basic sanity checks and counting of final output columns. */
  for(tmp=p->table; tmp!=NULL; tmp=tmp->next)
    {
      /* Inputs should be vectors. */
      if(tmp->ndim!=2)
        error(EXIT_FAILURE, 0, "only vector columns should be present "
              "in the table when using '--transpose'");
      if(tmp->dsize[1] != refwidth)
        error(EXIT_FAILURE, 0, "all vector columns given to '--transpose' "
              "should have the same length (number of tokens/elements)");

      /* Apply the transposition. */
      gal_permutation_transpose_2d(tmp);

      /* Remove extra dimensions if necessary. */
      if(tmp->dsize[1]==1) tmp->ndim=1;
    }
}





static void
table_fromvector(struct tableparams *p)
{
  size_t i, *iarr;
  gal_list_sizet_t *indexs=NULL;
  gal_data_t *tmp, *vector=NULL, *ext;

  /* Parse the values given to this option. */
  for(tmp=p->fromvector;tmp!=NULL;tmp=tmp->next)
    {
      /* Extract the name and element counters. */
      vector=gal_list_data_select_by_id(p->table, tmp->name, NULL);
      if(vector==NULL) table_error_no_column("--fromvector", tmp->name);

      /* If we don't need this vector column any more, remove it from the
         table. */
      if(p->keepvectfin==0) gal_list_data_remove(&p->table, vector);

      /* Make sure the selected column is actually a vector. */
      if(vector->ndim!=2)
        error(EXIT_FAILURE, 0, "column '%s' (given to '--fromvector') "
              "is not a vector", tmp->name);

      /* Loop over the values and make sure they are within the range. */
      indexs=NULL;
      iarr=tmp->array;
      for(i=0;i<tmp->size;++i)
        {
          /* Check if it is reasonable. */
          if(iarr[i]>vector->dsize[1])
            error(EXIT_FAILURE, 0, "column '%s' (given to "
                  "'--fromvector') only has a length of %zu, but you "
                  "have asked for element %zu", tmp->name,
                  vector->dsize[1], iarr[i]);

          /* Make sure the user didn't give a value of 0. */
          if(iarr[i]==0)
            error(EXIT_FAILURE, 0, "integers given to '--fromvector' "
                  "must be larger than 1, but you have given '0'");

          /* Add it to the list of indexs. Note that the user has given a
             "counter" (starting from 1), while we want an "index"
             (counting from 0). */
          gal_list_sizet_add(&indexs, iarr[i]-1);
        }

      /* Reverse the list of indexes to be the same as the requested. */
      gal_list_sizet_reverse(&indexs);

      /* Extract the columns and append them to the end of the table. */
      ext=gal_table_col_vector_extract(vector, indexs);
      gal_list_data_last(p->table)->next=ext;

      /* Clean up (remove the vector column, if requested). */
      gal_list_sizet_free(indexs);
      if(p->keepvectfin==0) gal_data_free(vector);
    }
}





static void
table_tovector(struct tableparams *p)
{
  size_t i;
  char **strarr;
  gal_list_str_t *tstr;
  gal_data_t *ids, *col, *tcol, *list, *vector, **torm=NULL;

  /* Loop over all the calls to this option. */
  for(tstr=p->tovector;tstr!=NULL;tstr=tstr->next)
    {
      /* Extract the separate csv. */
      ids=gal_options_parse_csv_strings_to_data(tstr->v, NULL, 0);

      /* Allocate an array of dataset pointers to keep the columns that
         should be removed. */
      if(p->keepvectfin==0)
        {
          errno=0;
          torm=malloc(ids->size*sizeof *torm);
          if(torm==NULL)
            error(EXIT_FAILURE, errno, "%s: allocating %zu bytes",
                  __func__, ids->size*sizeof *torm);
        }

      /* Parse the given values, and extract them from the table. */
      list=NULL;
      strarr=ids->array;
      for(i=0;i<ids->size;++i)
        {
          /* Extract this column. */
          tcol=gal_list_data_select_by_id(p->table, strarr[i], NULL);
          if(tcol==NULL) table_error_no_column("--tovector", strarr[i]);

          /* Keep a copy of the column and put it in the list of columns to
             add. */
          col=gal_data_copy(tcol); col->next=NULL;
          gal_list_data_add(&list, col);

          /* Keep the pointer to this column for removal (if necessary). */
          if(p->keepvectfin==0) torm[i]=tcol;
        }

      /* Reverse the list to be in the same order as the input, and convert
         it to a vector. */
      gal_list_data_reverse(&list);
      vector=gal_table_cols_to_vector(list);
      gal_list_data_free(list);

      /* Add this vector column to the output. */
      gal_list_data_last(p->table)->next=vector;

      /* Free the input columns if the user wanted to. */
      if(p->keepvectfin==0)
        for(i=0;i<ids->size;++i)
          gal_list_data_remove(&p->table, torm[i]);

      /* Clean up. */
      gal_data_free(ids);
      if(torm) free(torm);
    }
}




/* Find the HDU of the table to read. */
static char *
table_catrows_findhdu(char *filename, gal_list_str_t **hdull)
{
  char *hdu=NULL;

  /* Set the HDU (not necessary for non-FITS tables). */
  if(gal_fits_file_recognized(filename))
    {
      if(*hdull) { hdu=(*hdull)->v; *hdull=(*hdull)->next; }
      else
        error(EXIT_FAILURE, 0, "not enough '--catrowhdu's (or "
              "'-X'). For every FITS table given to '--catrowfile'. "
              "A call to '--catrowhdu' is necessary to identify "
              "its HDU/extension");
    }

  /* Return the HDU. */
  return hdu;
}





/* Preparations for adding rows: allocate final table, copy input table
   into it, and free the input table (while checking if enough HDUs are
   given for all the tables whose rows should be added). */
static size_t
table_catrows_prepare(struct tableparams *p)
{
  char **strarr;
  char *hdu=NULL;
  int tableformat;
  gal_data_t *ocol, *tmp;
  gal_list_str_t *filell, *hdull;
  size_t i, dsize[2], nrows=p->table->size;
  size_t numcols, numrows, filledrows=p->table->size;

  /* Go over all the given tables and find the final number of rows. */
  hdull=p->catrowhdu;
  for(filell=p->catrowfile; filell!=NULL; filell=filell->next)
    {
      hdu=table_catrows_findhdu(filell->v, &hdull);
      gal_table_info(filell->v, hdu, NULL, &numcols, &numrows,
                     &tableformat, "--catrowhdu");
      nrows+=numrows;
    }

  /* Change the 'array' component of each column (to preserve the column's
     'gal_data_t' pointer; since previous preparations have kept that
     pointer for next steps in some cases like column arithmetic). To do
     this, we will first allocate a full 'gal_data_t', copy the contents,
     free the contents of the original column, then replace them with the
     larger array, and free the temporary 'gal_data_t'. */
  for(tmp=p->table; tmp!=NULL; tmp=tmp->next)
    {
      /* Set the final amount of allocated space for this column. */
      dsize[0]=nrows;
      if(tmp->ndim==2) dsize[1]=tmp->dsize[1];

      /* Allocate a temporary dataset (we just want its allocated space,
         not the actual 'gal_data_t' pointer)! */
      ocol=gal_data_alloc(NULL, tmp->type, tmp->ndim, dsize, NULL,
                          0, p->cp.minmapsize, p->cp.quietmmap,
                          tmp->name, tmp->unit, tmp->comment);

      /* Put the full contents of the existing column into the new
         column: this will be the first set of rows. */
      memcpy(ocol->array, tmp->array, tmp->size*gal_type_sizeof(tmp->type));

      /* If the column type is a string, we should set the input pointers
         to NULL to avoid freeing them later. */
      if(tmp->type==GAL_TYPE_STRING)
        {
          strarr=tmp->array;
          for(i=0;i<tmp->size;++i) strarr[i]=NULL;
        }

      /* Free the contents of the current column (while keeping its
         pointer), and replace those of the 'ocol' dataset. */
      gal_data_free_contents(tmp);
      tmp->comment=ocol->comment; ocol->comment=NULL;
      tmp->array=ocol->array;     ocol->array=NULL;
      tmp->dsize=ocol->dsize;     ocol->dsize=NULL;
      tmp->name=ocol->name;       ocol->name=NULL;
      tmp->unit=ocol->unit;       ocol->unit=NULL;
      tmp->size=ocol->size;

      /* Free the 'ocol' dataset. */
      gal_data_free(ocol);
    }

  /* Clean up and return. */
  return filledrows;
}





/* Import rows from another set of table(s). */
static void
table_catrows(struct tableparams *p)
{
  size_t increment;
  char *hdu=NULL, **strarr;
  gal_data_t *new, *ttmp, *tmp;
  gal_list_str_t *filell, *hdull;
  size_t i, colcount, ncols, ncolstest, filledrows;

  /* Make sure enough HDUs are given, and allocate the final output table,
     while filling the initiall table rows into it. */
  filledrows=table_catrows_prepare(p);

  /* Go over all the given tables and extract the same set of columns that
     were extracted from the input table. */
  hdull=p->catrowhdu;
  ncols=gal_list_data_number(p->table);
  for(filell=p->catrowfile; filell!=NULL; filell=filell->next)
    {
      /* Read the columns of the new table. */
      hdu=table_catrows_findhdu(filell->v, &hdull);
      new=gal_table_read(filell->v, hdu, NULL, p->columns,
                         p->cp.searchin, p->cp.ignorecase,
                         p->cp.numthreads, p->cp.minmapsize,
                         p->cp.quietmmap, NULL, "--catrowhdu");

      /* Make sure that the same number of columns were extracted from this
         table as they were from the original table. */
      ncolstest=gal_list_data_number(new);
      if(ncolstest!=ncols)
        error(EXIT_FAILURE, 0, "%s: %zu column(s) were matched with "
              "your requested columns. However, the final table "
              "before adding rows contains %zu column(s). For "
              "concatenating (adding) rows, the final number of "
              "columns in all input tables should be the same. "
              "Note that adding columns is done before adding "
              "rows", gal_fits_name_save_as_string(filell->v, hdu),
              ncolstest, ncols);

      /* Parse all the new columns and add their contents to the already
         allocated space of the output. */
      colcount=1;
      ttmp=p->table;
      for(tmp=new; tmp!=NULL; tmp=tmp->next)
        {
          /* See if this column has the same type as the same column in the
             input table. */
          if(tmp->type!=ttmp->type)
            error(EXIT_FAILURE, 0, "%s: column %zu has a data type of "
                  "'%s'. However, in the final table (before adding "
                  "rows) this column has a type of '%s'. For "
                  "concatenating (adding) rows, the columns must have "
                  "the same data type. Note that adding columns is "
                  "done before adding rows. If you haven't added columns "
                  "you can use Table's column arithmetic to change the "
                  "data type of this column in the inputs",
                  gal_fits_name_save_as_string(filell->v, hdu), colcount,
                  gal_type_name(tmp->type, 1), gal_type_name(ttmp->type, 1));

          /* Make sure the two columns have the same dimensions (vector or
             single element). */
          if(tmp->ndim!=ttmp->ndim)
            error(EXIT_FAILURE, 0, "%s: column %zu is a %s column. "
                  "However, in the final table (before adding rows) this "
                  "column is a %s column",
                  gal_fits_name_save_as_string(filell->v, hdu), colcount,
                  tmp->ndim==1?"single-valued":"vector",
                  ttmp->ndim==1?"single-valued":"vector");

          /* If the column is vector, make sure it has the same number of
             elements. */
          if(tmp->ndim==2 && tmp->dsize[1]!=ttmp->dsize[1])
            error(EXIT_FAILURE, 0, "%s: vector column %zu has %zu elements "
                  "However, in the final table (before adding rows) this "
                  "vector column has %zu elements",
                  gal_fits_name_save_as_string(filell->v, hdu), colcount,
                  tmp->dsize[1], ttmp->dsize[1]);

          /* Set the increment on the existing table (column may be
             vector). */
          increment = filledrows * ( tmp->ndim==1 ? 1 : tmp->dsize[1] );

          /* Add the new rows and incremenet the counter. */
          memcpy(gal_pointer_increment(ttmp->array, increment, ttmp->type),
                 tmp->array, tmp->size*gal_type_sizeof(tmp->type));

          /* If the column type is a string, we should set the input
             pointers to NULL to avoid freeing them later. */
          if(tmp->type==GAL_TYPE_STRING)
            {
              strarr=tmp->array;
              for(i=0;i<tmp->size;++i) strarr[i]=NULL;
            }

          /* Take 'ttmp' to the next column and increment the counter. */
          ttmp=ttmp->next;
          ++colcount;
        }

      /* Clean up the columns of the table and increment 'filledrows'. */
      filledrows += new->dsize[0];
      gal_list_data_free(new);
    }
}





void
table_colmetadata(struct tableparams *p)
{
  char **strarr;
  gal_data_t *meta, *col;
  size_t counter, *colnum;

  /* Loop through all the given updates and implement them. */
  for(meta=p->colmetadata;meta!=NULL;meta=meta->next)
    {
      /* If the given column specifier is a name (not parse-able as a
         number), then this condition will fail. */
      colnum=NULL;
      if( gal_type_from_string((void **)(&colnum), meta->name,
                               GAL_TYPE_SIZE_T) )
        {
          /* We have been given a string, so find the first column that has
             the same name. */
          for(col=p->table; col!=NULL; col=col->next)
            if(!strcasecmp(col->name, meta->name)) break;
        }
      /* The column specifier is a number. */
      else
        {
          /* Go over the columns and find the one with this counter. */
          counter=1;
          for(col=p->table; col!=NULL; col=col->next)
            if(counter++==colnum[0]) break;

          /* Clean up the space that was allocated for 'colnum' (its not
             allocated when the given value was a string). */
          free(colnum);
        }

      /* If a match was found, then 'col' should not be NULL. */
      if(col==NULL)
        error(EXIT_FAILURE, 0, "no column found for '%s' (given to "
              "'--colmetadata'). Columns can either be specified by "
              "their position in the output table (integer counter, "
              "starting from 1), or their name (the first column "
              "found with the given name will be used)", meta->name);

      /* The matching column is found and we know that at least one value is
         already given (otherwise 'gal_options_parse_name_and_values' would
         abort the program). The first given string is the new name. */
      strarr=meta->array;
      if(col->name) free(col->name);
      gal_checkset_allocate_copy(strarr[0], &col->name);

      /* If more than one string is given, the second one is the new
         unit. */
      if(meta->size>1)
        {
          /* Replace the unit. */
          if(col->unit) free(col->unit);
          gal_checkset_allocate_copy(strarr[1], &col->unit);

          /* The next element is the comment of the column. */
          if(meta->size>2)
            {
              if(col->comment) free(col->comment);
              gal_checkset_allocate_copy(strarr[2], &col->comment);
            }
        }
    }
}





void
table_noblankend_check_add(struct tableparams *p,
                           gal_list_sizet_t **column_indexs, size_t colind)
{
  size_t i=0;
  gal_data_t *tmp;
  static int warningprinted=0;

  /* Before adding, be sure that the column is not a vector column. */
  for(tmp=p->table;tmp!=NULL;tmp=tmp->next)
    if(i++==colind)
      {
        if( tmp->ndim==1 ) gal_list_sizet_add(column_indexs, colind);
        else if(p->cp.quiet==0 && warningprinted==0)
          {
            warningprinted=1;
            error(EXIT_SUCCESS, 0, "WARNING: vector columns will be "
                  "ignored for the '--noblankend' option. To remove "
                  "this warning, run with '--quiet' (or '-q')");
          }
      }
}





void
table_noblankend(struct tableparams *p)
{
  int found;
  gal_list_str_t *tmp;
  size_t i, j, *index;
  gal_data_t *tcol, *flag;
  gal_list_sizet_t *column_indexs=NULL;

  /* Merge all possible calls to '--noblankend' into one list. */
  gal_options_merge_list_of_csv(&p->noblankend);

  /* See if all columns should be checked, or just a select few. */
  if( gal_list_str_number(p->noblankend)==1
      && !strcmp(p->noblankend->v,"_all") )
    {
      for(i=0;i<gal_list_data_number(p->table);++i)
        table_noblankend_check_add(p, &column_indexs, i);
    }

  /* Only certain columns should be checked, so find/add their index. */
  else
    for(tmp=p->noblankend; tmp!=NULL; tmp=tmp->next)
      {
        /* First go through the column names and if they match, add
           them. Note that we don't want to stop once a name is found, in
           this scenario, if multiple columns have the same name, we should
           use all. */
        j=0;
        found=0;
        for(tcol=p->table; tcol!=NULL; tcol=tcol->next)
          {
            if( tcol->name && !strcasecmp(tcol->name, tmp->v) )
              {
                found=1;
                table_noblankend_check_add(p, &column_indexs, j);
              }
            ++j;
          }

        /* If the given string didn't match any column name, it must be a
           number, so parse it as a number and use that number. */
        if(found==0)
          {
            /* Parse the given index. */
            index=NULL;
            if( gal_type_from_string((void **)(&index), tmp->v,
                                     GAL_TYPE_SIZE_T) )
              error(EXIT_FAILURE, 0, "column '%s' didn't match any of the "
                    "final column names and can't be parsed as a column "
                    "counter (starting from 1) either", tmp->v);

            /* Make sure its not zero (the user counts from 1). */
            if(*index==0)
              error(EXIT_FAILURE, 0, "the column number (given to the "
                    "'--noblankend' option) should start from 1, but you "
                    "have given 0");

            /* Make sure that the index falls within the number (note that
               it still counts from 1). */
            if(*index > gal_list_data_number(p->table))
              error(EXIT_FAILURE, 0, "the final output table only has "
                    "%zu columns, but you have given column %zu to "
                    "'--noblankend'. Recall that '--noblankend' operates "
                    "at the end (on the output columns) and that you "
                    "can also use output column names (if they have "
                    "any). In case you meant a column from the input "
                    "table, you should use '--noblank'",
                    gal_list_data_number(p->table), *index);

            /* Everything is fine, add the index to the list of columns to
               check. */
            table_noblankend_check_add(p, &column_indexs, *index-1);

            /* Clean up. */
            free(index);
          }

        /* For a check.
           printf("%zu\n", column_indexs->v);
        */
      }

  /* Remove all blank rows from the output table, note that we don't need
     the flags of the removed columns here. So we can just free it up.

     Vector columns are currently ignored in '--noblankend', so if the user
     only asks for no blanks in a vector column, in effect, no rows should
     be removed. */
  if(column_indexs)
    {
      flag=gal_blank_remove_rows(p->table, column_indexs, 1);
      gal_data_free(flag);
    }
}





static void
table_txt_formats(struct tableparams *p)
{
  gal_data_t *tmp;

  for(tmp=p->table; tmp!=NULL; tmp=tmp->next)
    {
      switch(tmp->type)
        {
        case GAL_TYPE_FLOAT32:
          if(p->txtf32format)    tmp->disp_fmt=p->txtf32format;
          if(p->txtf32precision!=GAL_BLANK_INT)
            tmp->disp_precision=p->txtf32precision;
          break;
        case GAL_TYPE_FLOAT64:
          if(p->txtf64format)    tmp->disp_fmt=p->txtf64format;
          if(p->txtf64precision!=GAL_BLANK_INT)
            tmp->disp_precision=p->txtf64precision;
          break;
        }
    }
}

















void
table_column(struct tableparams *p)
{
  /* Concatenate the columns of tables (if required). */
  if(p->catcolumnfile) table_catcolumn(p);

  /* Extract columns from vector. */
  if(p->fromvector) table_fromvector(p);

  /* Merge columns into a vector column. */
  if(p->tovector) table_tovector(p);

  /* If any arithmetic operations are needed, do them. */
  if(p->colpack) arithmetic_operate(p);
}





void
table_row(struct tableparams *p)
{
  /* Concatenate the rows of multiple tables (if required). */
  if(p->catrowfile) table_catrows(p);

  /* Apply ranges based on row values (if required). */
  if(p->selection) table_select_by_value(p);

  /* Sort it (if required). */
  if(p->sort) table_sort(p);

  /* If the output number of rows is limited, apply them. */
  if(    p->rowrange
      || p->rowrandom
      || p->head!=GAL_BLANK_SIZE_T
      || p->tail!=GAL_BLANK_SIZE_T )
    table_select_by_position(p);

  /* Transpose the input table. */
  if(p->transpose) table_transpose(p);
}





/***************************************************************/
/*************         Top-level function          *************/
/***************************************************************/
void
table(struct tableparams *p)
{
  /* Do the requested operations. */
  if(p->rowfirst) { table_row(p);    table_column(p); }
  else            { table_column(p); table_row(p);    }

  /* Last steps (independent of '--rowfirst'). */
  if(p->colmetadata) table_colmetadata(p);
  if(p->noblankend) table_noblankend(p);

  /* Write the output or a warning/error (it can become NULL!) */
  if(p->table)
    {
      table_txt_formats(p);
      gal_table_write(p->table, NULL, NULL, p->cp.tableformat,
                      p->cp.output, "TABLE", p->colinfoinstdout, 0);
    }
  else
    error(EXIT_FAILURE, 0, "no output columns");
}
