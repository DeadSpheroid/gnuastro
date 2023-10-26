/*********************************************************************
match -- Functions to match catalogs and WCS.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
     Sachin Kumar Singh <sachinkumarsingh092@gmail.com>
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

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <float.h>
#include <stdlib.h>

#include <gsl/gsl_sort.h>

#include <gnuastro/box.h>
#include <gnuastro/list.h>
#include <gnuastro/blank.h>
#include <gnuastro/binary.h>
#include <gnuastro/kdtree.h>
#include <gnuastro/pointer.h>
#include <gnuastro/threads.h>
#include <gnuastro/statistics.h>
#include <gnuastro/permutation.h>









/**********************************************************************/
/*****************   Sort-Based match custom list   *******************/
/**********************************************************************/
struct match_sfll
{
  float f;
  size_t v;
  struct match_sfll *next;
};





static void
match_add_to_sfll(struct match_sfll **list, size_t value,
                  float fvalue)
{
  struct match_sfll *newnode;

  errno=0;
  newnode=malloc(sizeof *newnode);
  if(newnode==NULL)
    error(EXIT_FAILURE, errno, "%s: new node couldn't be allocated",
          __func__);

  newnode->v=value;
  newnode->f=fvalue;
  newnode->next=*list;
  *list=newnode;
}





static void
match_pop_from_sfll(struct match_sfll **list,
                               size_t *value, float *fvalue)
{
  struct match_sfll *tmp;
  tmp=*list;
  *value=tmp->v;
  *fvalue=tmp->f;
  *list=tmp->next;
  free(tmp);
}




















/**********************************************************************/
/********      Generic functions (for any type of matching)    ********/
/**********************************************************************/
/* Preparations for the desired matching aperture. */
static void
match_aperture_prepare(gal_data_t *A, gal_data_t *B,
                       double *aperture, size_t ndim,
                       double **a, double **b, double *dist,
                       double *c, double *s, int *iscircle)
{
  double semiaxes[3];

  /* These two are common for all dimensions. */
  a[0]=A->array;
  b[0]=B->array;

  /* See if the aperture is a circle or not. */
  switch(ndim)
    {
    case 1:
      *iscircle = 0; /* Irrelevant for 1D. */
      dist[0]=aperture[0];
      break;

    case 2:
      /* Set the main coordinate arrays. */
      a[1]=A->next->array;
      b[1]=B->next->array;

      /* See if the aperture is circular. */
      if( ( *iscircle=(aperture[1]==1)?1:0 )==0 )
        {
          /* Using the box that encloses the aperture, calculate the
             distance along each axis. */
          gal_box_bound_ellipse_extent(aperture[0], aperture[0]*aperture[1],
                                       aperture[2], dist);

          /* Calculate the sin and cos of the given ellipse if necessary
             for ease of processing later. */
          c[0] = cos( aperture[2] * M_PI/180.0 );
          s[0] = sin( aperture[2] * M_PI/180.0 );
        }
      else
        dist[0]=dist[1]=aperture[0];
      break;

    case 3:
      /* Set the main coordinate arrays. */
      a[1]=A->next->array;
      b[1]=B->next->array;
      a[2]=A->next->next->array;
      b[2]=B->next->next->array;

      if( (*iscircle=(aperture[1]==1 && aperture[2]==1)?1:0)==0 )
        {
          /* Using the box that encloses the aperture, calculate the
             distance along each axis. */
          semiaxes[0]=aperture[0];
          semiaxes[1]=aperture[1]*aperture[0];
          semiaxes[2]=aperture[2]*aperture[0];
          gal_box_bound_ellipsoid_extent(semiaxes, &aperture[3], dist);

          /* Calculate the sin and cos of the given ellipse if necessary
             for ease of processing later. */
          c[0] = cos( aperture[3] * M_PI/180.0 );
          s[0] = sin( aperture[3] * M_PI/180.0 );
          c[1] = cos( aperture[4] * M_PI/180.0 );
          s[1] = sin( aperture[4] * M_PI/180.0 );
          c[2] = cos( aperture[5] * M_PI/180.0 );
          s[2] = sin( aperture[5] * M_PI/180.0 );
        }
      else
        dist[0]=dist[1]=dist[2]=aperture[0];
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix "
            "the problem. The value %zu is not recognized for ndim",
            __func__, PACKAGE_BUGREPORT, ndim);
    }
}





static double
match_elliptical_r_2d(double d1, double d2, double *ellipse,
                      double c, double s)
{
  double Xr = d1 * ( c       )     +   d2 * ( s );
  double Yr = d1 * ( -1.0f*s )     +   d2 * ( c );
  return sqrt( Xr*Xr + Yr*Yr/ellipse[1]/ellipse[1] );
}





static double
match_elliptical_r_3d(double *delta, double *ellipsoid,
                      double *c, double *s)
{
  double Xr, Yr, Zr;
  double c1=c[0], s1=s[0];
  double c2=c[1], s2=s[1];
  double c3=c[2], s3=s[2];
  double q1=ellipsoid[1], q2=ellipsoid[2];
  double x=delta[0], y=delta[1], z=delta[2];

  Xr = x*(  c3*c1   - s3*c2*s1 ) + y*( c3*s1   + s3*c2*c1) + z*( s3*s2 );
  Yr = x*( -1*s3*c1 - c3*c2*s1 ) + y*(-1*s3*s1 + c3*c2*c1) + z*( c3*s2 );
  Zr = x*(  s1*s2              ) + y*(-1*s2*c1           ) + z*( c2    );
  return sqrt( Xr*Xr + Yr*Yr/q1/q1 + Zr*Zr/q2/q2 );
}





static double
match_distance(double *delta, int iscircle, size_t ndim,
               double *aperture, double *c, double *s)
{
  /* For more than one dimension, we'll need to calculate the distance from
     the deltas (differences) in each dimension. */
  switch(ndim)
    {
    case 1:
      return fabs(delta[0]);

    case 2:
      return ( iscircle
               ? sqrt( delta[0]*delta[0] + delta[1]*delta[1] )
               : match_elliptical_r_2d(delta[0], delta[1],
                                       aperture, c[0], s[0]) );

    case 3:
      return ( iscircle
               ? sqrt( delta[0]*delta[0]
                       + delta[1]*delta[1]
                       + delta[2]*delta[2] )
               : match_elliptical_r_3d(delta, aperture, c, s) );

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix "
            "the problem. The value %zu is not recognized for ndim",
            __func__, PACKAGE_BUGREPORT, ndim);
    }

  /* Control should not reach this point. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return NAN;
}





/* In the 'match_XXXX_second_in_first' functions, we made an array of
   lists, here we want to reverse that list to fix the second two issues
   that were discussed there. */
void
match_rearrange(gal_data_t *A, gal_data_t *B, struct match_sfll **bina)
{
  size_t bi;
  float *fp, *fpf, r, *ainb;
  size_t ai, ar=A->size, br=B->size;

  /* Allocate the space for 'ainb' and initialize it to NaN (since zero is
     meaningful in this context; both for indexs and also for floats). This
     is a two column array that will keep the distance and index of the
     closest element in catalog 'a' for each element in catalog b. */
  errno=0; ainb=calloc(2*br, sizeof *ainb);
  if(ainb==NULL)
    error(EXIT_FAILURE, errno, "%s: %zu bytes for 'ainb'", __func__,
          br*sizeof *ainb);
  fpf=(fp=ainb)+2*br; do *fp++=NAN; while(fp<fpf);

  /* Go over each object in catalog 'a' and re-distribute the near objects,
     to find which ones in catalog 'a' are within the search radius of
     catalog b in a sorted manner. Note that we only need the 'ai' with the
     minimum distance to 'bi', the rest are junk. */
  for( ai=0; ai<ar; ++ai )
    while( bina[ai] )	/* As long as its not NULL.            */
      {
	/* Pop out a 'bi' and its distance to this 'ai' from 'bina' (this
           is the nearest B item to this A element). */
	match_pop_from_sfll(&bina[ai], &bi, &r);

	/* If nothing has been put here (the 'isnan' condition below is
	   true), or something exists (the isnan is false, and so it will
	   check the second OR test) with a distance that is larger than
	   this distance then just put this value in. */
	if( isnan(ainb[bi*2]) || r<ainb[bi*2+1] )
	  {
	    ainb[bi*2  ] = ai;
	    ainb[bi*2+1] = r;
	  }
      }

  /* For checking the status of affairs uncomment this block
  {
    printf("\n\nFilled ainb:\n");
    for(bi=0;bi<br;++bi)
      if( !isnan(ainb[bi*2]) )
	printf("bi: %lu: %.0f, %f\n", bi, ainb[bi*2], ainb[bi*2+1]);
  }
  */

  /* Re-fill the bina array, but this time only with the 'bi' that is
     closest to it. Note that bina was fully set to NULL after popping all
     the elements in the loop above. */
  for( bi=0; bi<br; ++bi )
    if( !isnan(ainb[bi*2]) )
      {
	/* Just to keep the same terminology as before and easier
	   reading. */
	r=ainb[bi*2+1];
	ai=(size_t)(ainb[bi*2]);

	/* Check if this is the first time we are associating a 'bi' to
	   this 'ai'. If so, then just allocate a single element
	   list. Otherwise, see if the distance is closer or not. If so,
	   replace the values in the single node. */
	if( bina[ai] )
	  {
	    /* If the distance of this record is smaller than the existing
	       entry, then replace the values. */
	    if( r < bina[ai]->f )
	      {
		bina[ai]->f=r;
		bina[ai]->v=bi;
	      }
	  }
	else
          match_add_to_sfll(&bina[ai], bi, r);
      }

  /* For checking the status of affairs uncomment this block
  {
    size_t bi, counter=0;
    double *a[2]={A->array, A->next->array};
    double *b[2]={B->array, B->next->array};
    printf("\n\nRearranged bina:\n");
    for(ai=0;ai<ar;++ai)
      if(bina[ai])
        {
          ++counter;
          bi=bina[ai]->v;
          printf("A_%lu (%.8f, %.8f) <--> B_%lu (%.8f, %.8f):\n\t%f\n",
                 ai, a[0][ai], a[1][ai], bi, b[0][bi], b[1][bi],
                 bina[ai]->f);
        }
    printf("\n-----------\nMatched: %zu\n", counter);
  }
  exit(0);
  */

  /* Clean up. */
  free(ainb);
}





/* The matching has been done, write the output. */
static gal_data_t *
match_output(gal_data_t *A, gal_data_t *B, size_t *A_perm, size_t *B_perm,
             struct match_sfll **bina, size_t minmapsize, int quietmmap)
{
  float r;
  double *rval;
  gal_data_t *out;
  uint8_t *Bmatched;
  size_t ai, bi, nummatched=0;
  size_t *aind, *bind, match_i, nomatch_i;

  /* Find how many matches there were in total. */
  for(ai=0;ai<A->size;++ai) if(bina[ai]) ++nummatched;


  /* If there aren't any matches, return NULL. */
  if(nummatched==0) return NULL;


  /* Allocate the output list. */
  out=gal_data_alloc(NULL, GAL_TYPE_SIZE_T, 1, &A->size, NULL, 0,
                     minmapsize, quietmmap, "CAT1_ROW", "counter",
                     "Row index in first catalog (counting from 0).");
  out->next=gal_data_alloc(NULL, GAL_TYPE_SIZE_T, 1, &B->size, NULL, 0,
                           minmapsize, quietmmap, "CAT2_ROW", "counter",
                           "Row index in second catalog (counting "
                           "from 0).");
  out->next->next=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &nummatched,
                                 NULL, 0, minmapsize, quietmmap,
                                 "MATCH_DIST", NULL,
                                 "Distance between the match.");


  /* Allocate the 'Bmatched' array which is a flag for which rows of the
     second catalog were matched. The columns that had a match will get a
     value of one while we are parsing them below. */
  Bmatched=gal_pointer_allocate(GAL_TYPE_UINT8, B->size, 1, __func__,
                                "Bmatched");


  /* Initialize the indexs. We want the first 'nummatched' indexs in both
     outputs to be the matching rows. The non-matched rows should start to
     be indexed after the matched ones. So the first non-matched index is
     at the index 'nummatched'. */
  match_i   = 0;
  nomatch_i = nummatched;


  /* Fill in the output arrays. */
  aind = out->array;
  bind = out->next->array;
  rval = out->next->next->array;
  for(ai=0;ai<A->size;++ai)
    {
      /* A match was found. */
      if(bina[ai])
        {
          /* Note that the permutation keeps the original indexs. */
          match_pop_from_sfll(&bina[ai], &bi, &r);
          rval[ match_i   ] = r;
          aind[ match_i   ] = A_perm ? A_perm[ai] : ai;
          bind[ match_i++ ] = B_perm ? B_perm[bi] : bi;

          /* Set a '1' for this object in the second catalog. This will
             later be used to find which rows didn't match to fill in the
             output. */
          Bmatched[ B_perm ? B_perm[bi] : bi ] = 1;
        }

      /* No match found. At this stage, we can only fill the indexs of the
         first input. The second input needs to be matched afterwards. */
      else aind[ nomatch_i++ ] = A_perm ? A_perm[ai] : ai;
    }


  /* Complete the second input's permutation. */
  nomatch_i=nummatched;
  for(bi=0;bi<B->size;++bi)
    if( Bmatched[bi] == 0 )
      bind[ nomatch_i++ ] = bi;


  /* For a check
  printf("\nFirst input's permutation (starred items not matched):\n");
  for(ai=0;ai<A->size;++ai)
    printf("%s%zu\n", ai<nummatched?"  ":"* ", aind[ai]+1);
  printf("\nSecond input's permutation  (starred items not matched):\n");
  for(bi=0;bi<B->size;++bi)
    printf("%s%zu\n", bi<nummatched?"  ":"* ", bind[bi]+1);
  exit(0);
  */

  /* Clean up and return. */
  free(Bmatched);
  return out;
}




















/********************************************************************/
/*************            Sort-Based matching           *************/
/********************************************************************/
/* Since these checks are repetative, its easier to have a separate
   function for both inputs. */
static void
match_sort_based_sanity_check_columns(gal_data_t *coord, char *info,
                                      int inplace, int *allf64)
{
  gal_data_t *tmp;

  for(tmp=coord; tmp!=NULL; tmp=tmp->next)
    {
      if(tmp->type!=GAL_TYPE_FLOAT64)
        {
          if(inplace)
            error(EXIT_FAILURE, 0, "%s: when 'inplace' is activated, "
                  "the input coordinates must have 'float64' type. At "
                  "least one node of the %s list has type of '%s'",
                  __func__, info, gal_type_name(tmp->type, 1));
          else
            *allf64=0;
        }

      if(tmp->ndim!=1)
        error(EXIT_FAILURE, 0, "%s: each input coordinate column must "
              "have a single dimension (be a single column). Atleast "
              "one node of the %s list has %zu dimensions", __func__,
              info, tmp->ndim);

      if(tmp->size!=coord->size)
        error(EXIT_FAILURE, 0, "%s: the nodes of each list of "
              "coordinates must have the same number of elements. "
              "At least one node of the %s list has %zu elements "
              "while the first has %zu elements", __func__, info,
              tmp->size, coord->size);
    }
}





/* To keep the main function clean, we'll do the sanity checks here. */
static void
match_sort_based_sanity_check(gal_data_t *coord1, gal_data_t *coord2,
                               double *aperture, int inplace, int *allf64)
{
  size_t ncoord1=gal_list_data_number(coord1);

  /* Make sure both lists have the same number of datasets. NOTE: they
     don't need to have the same number of elements. */
  if( ncoord1!=gal_list_data_number(coord2) )
    error(EXIT_FAILURE, 0, "%s: the two inputs have different "
          "numbers of datasets (%zu and %zu respectively)",
          __func__, ncoord1, gal_list_data_number(coord2));


  /* This function currently only works for less than 4 dimensions. */
  if(ncoord1>3)
    error(EXIT_FAILURE, 0, "%s: %zu dimension matching requested, "
          "this function currently only matches datasets with a "
          "maximum of 3 dimensions", __func__, ncoord1);

  /* Check the column properties. */
  match_sort_based_sanity_check_columns(coord1, "first", inplace, allf64);
  match_sort_based_sanity_check_columns(coord2, "second", inplace, allf64);

  /* Check the aperture values. */
  if(aperture[0]<=0)
    error(EXIT_FAILURE, 0, "%s: the first value in the aperture (%g) "
          "cannot be zero or negative", __func__, aperture[0]);
  switch(ncoord1)
    {
    case 1:  /* We don't need any checks in a 1D match. */
      break;

    case 2:
      if( (aperture[1]<=0 || aperture[1]>1))
        error(EXIT_FAILURE, 0, "%s: the second value in the aperture "
              "(%g) is the axis ratio, so it must be larger than zero "
              "and less than 1", __func__, aperture[1]);
      break;

    case 3:
      if(aperture[1]<=0 || aperture[1]>1 || aperture[2]<=0 || aperture[2]>1)
        error(EXIT_FAILURE, 0, "%s: at least one of the second or "
              "third values in the aperture (%g and %g respectively) "
              "is smaller than zero or larger than one. In a 3D match, "
              "these are the axis ratios, so they must be larger than "
              "zero and less than 1", __func__, aperture[1], aperture[2]);
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the issue. The value %zu not recognized for 'ndim'",
            __func__, PACKAGE_BUGREPORT, ncoord1);
    }
}





/* To keep things clean, the sorting of each input array will be done in
   this function. */
static size_t *
match_sort_based_prepare_sort(gal_data_t *coords, size_t minmapsize)
{
  size_t i;
  double *darr;
  gal_data_t *tmp;
  size_t *permutation=gal_pointer_allocate(GAL_TYPE_SIZE_T, coords->size,
                                           0, __func__, "permutation");

  /* Unfortunately 'gsl_sort_index' doesn't account for NaN elements. So we
     need to set them to the maximum possible floating point value. */
  if( gal_blank_present(coords, 1) )
    {
      darr=coords->array;
      for(i=0;i<coords->size;++i)
        if( isnan(darr[i]) ) darr[i]=FLT_MAX;
    }

  /* Get the permutation necessary to sort all the columns (based on the
     first column). */
  gsl_sort_index(permutation, coords->array, 1, coords->size);

  /* For a check.
  if(coords->size>1)
    for(size_t i=0; i<coords->size; ++i) printf("%zu\n", permutation[i]);
  */

  /* Sort all the coordinates. */
  for(tmp=coords; tmp!=NULL; tmp=tmp->next)
    gal_permutation_apply(tmp, permutation);

  /* For a check.
  if(coords->size>1)
    {
      for(i=0;i<coords->size;++i)
        {
          for(tmp=coords; tmp!=NULL; tmp=tmp->next)
            {
              printf("%f ", ((double *)(tmp->array))[i]);
            }
          printf("\n");
        }
      exit(0);
    }
  */

  /* Return the permutation. */
  return permutation;
}





/* Do the preparations for matching of coordinates. */
static void
match_sort_based_prepare(gal_data_t *coord1, gal_data_t *coord2,
                          int sorted_by_first, int inplace, int allf64,
                          gal_data_t **A_out, gal_data_t **B_out,
                          size_t **A_perm, size_t **B_perm,
                          size_t minmapsize)
{
  gal_data_t *c, *tmp, *A=NULL, *B=NULL;

  /* Sort the datasets if they aren't sorted. If the dataset is already
     sorted, then 'inplace' is irrelevant. */
  if(sorted_by_first && allf64)
    {
      *A_out=coord1;
      *B_out=coord2;
    }
  else
    {
      /* Allocating a new list is only necessary when 'inplace==0' or all
         the columns are double. */
      if( inplace && allf64 )
        {
          *A_out=coord1;
          *B_out=coord2;
        }
      else
        {
          /* Copy the first list. */
          for(tmp=coord1; tmp!=NULL; tmp=tmp->next)
            {
              c=gal_data_copy(tmp);
              c->next=NULL;
              gal_list_data_add(&A, c);
            }

          /* Copy the second list. */
          for(tmp=coord2; tmp!=NULL; tmp=tmp->next)
            {
              c=gal_data_copy(tmp);
              c->next=NULL;
              gal_list_data_add(&B, c);
            }

          /* Reverse both lists: the copying process reversed the order. */
          gal_list_data_reverse(&A);
          gal_list_data_reverse(&B);

          /* Set the output pointers. */
          *A_out=A;
          *B_out=B;
        }

      /* Sort each dataset by the first coordinate. */
      *A_perm = match_sort_based_prepare_sort(*A_out, minmapsize);
      *B_perm = match_sort_based_prepare_sort(*B_out, minmapsize);
    }
}





/* Go through both catalogs and find which records/rows in the second
   catalog (catalog b) are within the acceptable distance of each record in
   the first (a). */
static void
match_sort_based_second_in_first(gal_data_t *A, gal_data_t *B,
                                  double *aperture,
                                  struct match_sfll **bina)
{
  /* To keep things easy to read, all variables related to catalog 1 start
     with an 'a' and things related to catalog 2 are marked with a 'b'. The
     redundant variables (those that equal a previous value) are only
     defined to make it easy to read the code.*/
  int iscircle=0;
  size_t i, ar=A->size, br=B->size;
  size_t ai, bi, blow=0, prevblow=0;
  size_t ndim=gal_list_data_number(A);
  double r, c[3]={NAN, NAN, NAN}, s[3]={NAN, NAN, NAN};
  double dist[3]={NAN, NAN, NAN}, delta[3]={NAN, NAN, NAN};
  double *a[3]={NULL, NULL, NULL}, *b[3]={NULL, NULL, NULL};

  /* Necessary preperations. */
  match_aperture_prepare(A, B, aperture,  ndim, a, b, dist,
                         c, s, &iscircle);

  /* For each row/record of catalog 'a', make a list of the nearest records
     in catalog b within the maximum distance. Note that both catalogs are
     sorted by their first axis coordinate.*/
  for(ai=0;ai<ar;++ai)
    if( !isnan(a[0][ai]) && blow<br)
      {
        /* Initialize 'bina'. */
        bina[ai]=NULL;

        /* Find the first (lowest first axis value) row/record in catalog
           'b' that is within the search radius for this record of catalog
           'a'. 'blow' is the index of the first element to start searching
           in the catalog 'b' for a match to 'a[][ai]' (the record in
           catalog a that is currently being searched). 'blow' is only
           based on the first coordinate, not the second.

           Both catalogs are sorted by their first coordinate, so the
           'blow' to search for the next record in catalog 'a' will be
           larger or equal to that of the previous catalog 'a' record. To
           account for possibly large distances between the records, we do
           a search here to change 'blow' if necessary before doing further
           searching.*/
        for( blow=prevblow; blow<br && b[0][blow] < a[0][ai]-dist[0];
             ++blow)
          { /* This can be blank, the 'for' does all we need :-). */ }

        /* 'blow' is now found for this 'ai' and will be used unchanged to
           the end of the loop. So keep its value to help the search for
           the next entry in catalog 'a'. */
        prevblow=blow;

        /* Go through catalog 'b' (starting at 'blow') with a first axis
           value smaller than the maximum acceptable range for 'si'. */
        for( bi=blow; bi<br && b[0][bi] <= a[0][ai] + dist[0]; ++bi )
          {
            /* Only consider records with a second axis value in the
               correct range, note that unlike the first axis, the second
               axis is no longer sorted. so we have to do both lower and
               higher limit checks for each item.

               Positions can have an accuracy to a much higher order of
               magnitude than the search radius. Therefore, it is
               meaning-less to sort the second axis (after having sorted
               the first). In other words, very rarely can two first axis
               coordinates have EXACTLY the same floating point value as
               each other to easily define an independent sorting in the
               second axis. */
            if( ndim<2
                || (    b[1][bi] >= a[1][ai]-dist[1]
                     && b[1][bi] <= a[1][ai]+dist[1] ) )
              {
                /* Now, 'bi' is within the rectangular range of 'ai'. But
                   this is not enough to consider the two objects matched
                   for the following reasons:

                   1) Until now we have avoided calculations other than
                   larger or smaller on double precision floating point
                   variables for efficiency. So the 'bi' is within a square
                   of side 'dist[0]*dist[1]' around 'ai' (not within a
                   fixed radius).

                   2) Other objects in the 'b' catalog may be closer to
                   'ai' than this 'bi'.

                   3) The closest 'bi' to 'ai' might be closer to another
                   catalog 'a' record.

                   To address these problems, we will use a linked list to
                   keep the indexes of the 'b's near 'ai', along with their
                   distance. We only add the 'bi's to this list that are
                   within the acceptable distance.

                   Since we are dealing with much fewer objects at this
                   stage, it is justified to do complex mathematical
                   operations like square root and multiplication. This
                   fixes the first problem.

                   The next two problems will be solved with the list after
                   parsing of the whole catalog is complete.*/
                if( ndim<3
                    || ( b[2][bi] >= a[2][ai]-dist[2]
                         && b[2][bi] <= a[2][ai]+dist[2] ) )
                  {
                    for(i=0;i<ndim;++i) delta[i]=b[i][bi]-a[i][ai];
                    r=match_distance(delta, iscircle, ndim, aperture,
                                     c, s);
                    if(r<aperture[0])
                      match_add_to_sfll(&bina[ai], bi, r);
                  }
              }
          }

        /* If there was no objects within the acceptable distance, then the
           linked list pointer will be NULL, so go on to the next 'ai'. */
        if(bina[ai]==NULL)
          continue;

        /* For checking the status of affairs uncomment this block
        {
          struct match_sfll *tmp;
          printf("\n\nai: %lu:\n", ai);
          printf("ax: %f (%f -- %f)\n", a[0][ai], a[0][ai]-dist[0],
                 a[0][ai]+dist[0]);
          printf("ay: %f (%f -- %f)\n", a[1][ai], a[1][ai]-dist[1],
                 a[1][ai]+dist[1]);
          for(tmp=bina[ai];tmp!=NULL;tmp=tmp->next)
            printf("%lu: %f\n", tmp->v, tmp->f);
        }
        */
      }
}





/* Match two positions: the two inputs ('coord1' and 'coord2') should be
   lists of coordinates (each is a list of datasets). To speed up the
   search, this function will sort the inputs by their first column. If
   both are already sorted, give a non-zero value to
   'sorted_by_first'. When sorting is necessary and 'inplace' is non-zero,
   the actual inputs will be sorted. Otherwise, an internal copy of the
   inputs will be made which will be used (sorted) and later
   freed. Therefore when 'inplace==0', the input's won't be changed.

   IMPORTANT NOTE: the output permutations will correspond to the initial
   inputs. Therefore, even when 'inplace' is non-zero (and this function
   changes the inputs' order), the output permutation will correspond to
   original inputs.

   The output is a list of 'gal_data_t's with the following columns:

       Node 1: First catalog index (counting from zero).
       Node 2: Second catalog index (counting from zero).
       Node 3: Distance between the match.                    */
gal_data_t *
gal_match_sort_based(gal_data_t *coord1, gal_data_t *coord2,
                      double *aperture, int sorted_by_first,
                      int inplace, size_t minmapsize, int quietmmap,
                      size_t *nummatched)
{
  int allf64=1;
  gal_data_t *A, *B, *out;
  size_t *A_perm=NULL, *B_perm=NULL;
  struct match_sfll **bina;

  /* Do a small sanity check and make the preparations. After this point,
     we'll call the two arrays 'a' and 'b'.*/
  match_sort_based_sanity_check(coord1, coord2, aperture, inplace,
                                 &allf64);
  match_sort_based_prepare(coord1, coord2, sorted_by_first, inplace,
                            allf64, &A, &B, &A_perm, &B_perm,
                            minmapsize);


  /* Allocate the 'bina' array (an array of lists). Let's call the first
     catalog 'a' and the second 'b'. This array has 'a->size' elements
     (pointers) and for each, it keeps a list of 'b' elements that are
     nearest to it. */
  errno=0;
  bina=calloc(A->size, sizeof *bina);
  if(bina==NULL)
    error(EXIT_FAILURE, errno, "%s: %zu bytes for 'bina'", __func__,
          A->size*sizeof *bina);


  /* All records in 'b' that match each 'a' (possibly duplicate). */
  match_sort_based_second_in_first(A, B, aperture, bina);


  /* Two re-arrangings will fix the issue. */
  match_rearrange(A, B, bina);


  /* The match is done, write the output. */
  out=match_output(A, B, A_perm, B_perm, bina, minmapsize, quietmmap);


  /* Clean up. */
  free(bina);
  if(A!=coord1)
    {
      gal_list_data_free(A);
      gal_list_data_free(B);
    }
  if(A_perm) free(A_perm);
  if(B_perm) free(B_perm);


  /* Set 'nummatched' and return output. */
  *nummatched = out ?  out->next->next->size : 0;
  return out;
}




















/********************************************************************/
/*************             k-d tree matching            *************/
/********************************************************************/
struct match_kdtree_params
{
  /* Input arguments. */
  gal_data_t             *A;  /* 1st coordinate list of 'gal_data_t's */
  gal_data_t             *B;  /* 2nd coordinate list of 'gal_data_t's */
  size_t               ndim;  /* The number of dimensions.            */
  double          *aperture;  /* Acceptable aperture for match.       */
  size_t        kdtree_root;  /* Index (counting from 0) of root.     */
  gal_data_t      *A_kdtree;  /* k-d tree of first coordinate.        */

  /* Internal parameters for easy aperture checking. For example there is
     no need to calculate the fixed 'cos()' and 'sin()' functions every
     time. So we calculate them once and store the results here to just use
     their values for every check. */
  int              iscircle;  /* If the aperture is circular.         */
  double               c[3];  /* Fixed cos(), for elliptical dist.    */
  double               s[3];  /* Fixed sin(), for elliptical dist.    */

  /* Internal items. */
  double              *a[3];  /* Direct pointers to column arrays.    */
  double              *b[3];  /* Direct pointers to column arrays.    */
  struct match_sfll  **bina;  /* Second cat. items in first.          */
  gal_data_t        *Aexist;  /* If any element of A exists in bins.  */
  double         *Abinwidth;  /* Width of bins along each dimension.  */
  double              *Amin;  /* Minimum value of A along each dim.   */
  double              *Amax;  /* Maximum value of A along each dim.   */
};





/* Find the "coverage" of A along each dimension to help in rejecting
   non-matches without even calling the k-d tree function.

   The 'MATCH_KDTREE_COVERAGE_MAXBINS' is currently just a place-holder to
   get the other parts of the algorithm going. But most probably there is a
   way to optimally select the maximum number automatically. */
#define MATCH_KDTREE_COVERAGE_MAXBINS 10000
static void
match_kdtree_A_coverage(struct match_kdtree_params *p)
{
  double *d, min, max;
  size_t *s, *sf, dim, two=2, numbins;
  gal_data_t *tmp, *stat, *hist, *range=NULL, *bins=NULL;

  /* Allocate the space to keep the range of first input dimensions. */
  p->Amin=gal_pointer_allocate(GAL_TYPE_FLOAT64, p->ndim, 0,
                               __func__, "p->Amin");
  p->Amax=gal_pointer_allocate(GAL_TYPE_FLOAT64, p->ndim, 0,
                               __func__, "p->Amax");
  p->Abinwidth=gal_pointer_allocate(GAL_TYPE_FLOAT64, p->ndim, 0,
                                    __func__, "p->Abinwidth");

  /* Set the coverage along each dimension. */
  dim=0;
  p->Aexist=NULL;
  for(tmp=p->A; tmp!=NULL; tmp=tmp->next)
    {
      /* Find the number of bins based on the range and aperture size. */
      stat=gal_statistics_minimum(tmp);
      min=((double *)(stat->array))[0];
      gal_data_free(stat);
      stat=gal_statistics_maximum(tmp);
      max=((double *)(stat->array))[0];
      gal_data_free(stat);

      /* Set the generic constants. */
      p->Amin[dim] = min - p->aperture[0];
      p->Amax[dim] = max + p->aperture[0];
      numbins=(p->Amax[dim] - p->Amin[dim])/p->aperture[0];
      if(numbins>MATCH_KDTREE_COVERAGE_MAXBINS)
        numbins=MATCH_KDTREE_COVERAGE_MAXBINS;
      if(numbins==0) numbins=1;

      /*************************/
      //numbins=1;
      /*************************/

      /* Generate the 'Aexist' list for this dimension. Note that if we
         have a single bin in this dimension, we can just set everything
         automatically. */
      if(numbins==1)
        {
          /* We only have one bin, so set the width and a single-element
             histogram. */
          p->Abinwidth[dim] = p->Amax[dim] - p->Amin[dim];
          hist=gal_data_alloc(NULL, GAL_TYPE_UINT8, 1, &numbins, NULL,
                              0, -1, 1, NULL, NULL, NULL);
          ((uint8_t *)(hist->array))[0]=1;
        }
      else
        {
          /* Set the 'range' for the bins. */
          range=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &two, NULL,
                               0, -1, 1, NULL, NULL, NULL);
          d=range->array;
          d[0]=p->Amin[dim];
          d[1]=p->Amax[dim];

          /* Generate the histogram of elements in this dimension. */
          bins=gal_statistics_regular_bins(tmp, range, numbins, NAN);
          hist=gal_statistics_histogram(tmp, bins, 0, 0);

          /* Set all histograms with atleast one element to 1 and convert
             it to 8-bit unsigned integer. */
          sf = (s=hist->array) + hist->size; do *s=*s>0; while(++s<sf);
          hist=gal_data_copy_to_new_type_free(hist, GAL_TYPE_UINT8);

          /* Dilate the binary histogram to avoid bin-edge-effect (missing
             a match because the two points are on opposite sides of the
             bin boundary). Note that we know that the bins are
             equal/larger than ther user's given aperture and that these
             bins are only for rejecting points before the k-d tree (they
             aren't used within the k-d tree matching). */
          gal_binary_dilate(hist, 1, 1, 1);

          /* Set the general bin properties along this dimension. */
          d=bins->array;
          p->Abinwidth[dim] = d[1]-d[0];
          p->Amin[dim]      = d[ 0            ] - p->Abinwidth[dim]/2;
          p->Amax[dim]      = d[ bins->size-1 ] + p->Abinwidth[dim]/2;
        }

      /* For a check.
      {
        size_t i;
        double *d;
        uint8_t *u=hist->array;
        printf("\ndim: %zu\n", dim);
        printf("min: %g\n", p->Amin[dim]);
        printf("max: %g\n", p->Amax[dim]);
        printf("binwidth: %g\n", p->Abinwidth[dim]);
        printf("----------------\n");
        if(bins)
          {
            d=bins->array;
            for(i=0;i<bins->size;++i)
              printf("%zu: %-15.8f%-15.8f%u\n", i, d[i]-p->Abinwidth[dim]/2,
                     d[i]+p->Abinwidth[dim]/2, u[i]);
          }
        else
          printf("0: %-15.8f%-15.8f%u\n", p->Amin[dim],
                 p->Amax[dim], u[0]);
        printf("----------------\n");
      } */

      /* Add the histogram to the list and increment the dimensionality. */
      gal_list_data_add(&p->Aexist, hist);
      ++dim;

      /* Clean up (these are done here in case the 'For a check' is
         uncommented, and we want to debug things). */
      if(bins)  { gal_data_free(bins);  bins=NULL;  }
      if(range) { gal_data_free(range); range=NULL; }
    }

  //printf("%s: Good!\n", __func__); exit(0);

  /* Reverse the list to be in the proper dimensional order. */
  gal_list_data_reverse(&p->Aexist);
}





static void
match_kdtree_sanity_check(struct match_kdtree_params *p)
{
  gal_data_t *tmp;

  /* Make sure all coordinates and the k-d tree have the same number of
     rows. */
  p->ndim=gal_list_data_number(p->A);
  if( p->ndim != gal_list_data_number(p->B) )
    error(EXIT_FAILURE, 0, "%s: the 'coord1' and 'coord2' arguments "
          "should have the same number of nodes/columns (elements "
          "in a simply linked list). But they each respectively "
          "have %zu, %zu and %zu nodes/columns", __func__, p->ndim,
          gal_list_data_number(p->B),
          gal_list_data_number(p->A_kdtree));

  /* Make sure that the k-d tree only has two columns. */
  if( gal_list_data_number(p->A_kdtree)!=2 )
    error(EXIT_FAILURE, 0, "%s: the 'kdtree' argument should only "
          "two nodes/columns (elements in a simply linked list), "
          "but it has %zu nodes/columns", __func__,
          gal_list_data_number(p->A_kdtree));

  /* Make sure the coordinates have a 'double' type and that the k-d tree
     has an unsigned 32-bit integer type.*/
  for(tmp=p->A; tmp!=NULL; tmp=tmp->next)
    if( tmp->type!=GAL_TYPE_FLOAT64 )
      error(EXIT_FAILURE, 0, "%s: the type of all columns in 'coord1' "
            "should be 'double', but at least one of them is '%s'",
            __func__, gal_type_name(tmp->type, 1));
  for(tmp=p->B; tmp!=NULL; tmp=tmp->next)
    if( tmp->type!=GAL_TYPE_FLOAT64 )
      error(EXIT_FAILURE, 0, "%s: the type of all columns in 'coord2' "
            "should be 'double', but at least one of them is '%s'",
            __func__, gal_type_name(tmp->type, 1));
  for(tmp=p->A_kdtree; tmp!=NULL; tmp=tmp->next)
    if( tmp->type!=GAL_TYPE_UINT32 )
      error(EXIT_FAILURE, 0, "%s: the type of both columns in "
            "'coord1_kdtree' should be 'uint32', but it is '%s'",
            __func__, gal_type_name(tmp->type, 1));

  /* Allocate and initialize the 'bina' array (an array of lists). Let's
     call the first catalog 'a' and the second 'b'. This array has
     'a->size' elements (pointers) and for each, it keeps a list of 'b'
     elements that are nearest to it. */
  errno=0;
  p->bina=calloc(p->A->size, sizeof *p->bina);
  if(p->bina==NULL)
    error(EXIT_FAILURE, errno, "%s: %zu bytes for 'bina'",
          __func__, p->A->size*sizeof *p->bina);

  /* Pointers to the input column arrays for easy parsing later. */
  p->a[0]=p->A->array;
  p->b[0]=p->B->array;
  if( p->A->next )
    {
      p->a[1]=p->A->next->array;
      if( p->A->next->next ) p->a[2]=p->A->next->next->array;
    }
  if( p->B->next )
    {
      p->b[1]=p->B->next->array;
      if( p->B->next->next ) p->b[2]=p->B->next->next->array;
    }

  /* Find the bins of the first input along all its dimensions and select
     those that contain data. This is very important in optimal k-d tree
     based matching because confirming a non-match in a k-d tree is very
     computationally expensive. */
  match_kdtree_A_coverage(p);
}





/* Main k-d tree matching function. */
static void *
match_kdtree_worker(void *in_prm)
{
  /* Low-level definitions to be done first. */
  struct gal_threads_params *tprm=(struct gal_threads_params *)in_prm;
  struct match_kdtree_params *p=(struct match_kdtree_params *)tprm->params;

  /* High level definitions. */
  int iscovered;
  uint8_t *existA;
  double r, delta[3];
  size_t i, j, ai, bi, h_i;
  gal_data_t *ccol, *Aexist;
  double po, *point=NULL, least_dist;

  /* Allocate space for all the matching points (based on the number of
     dimensions). */
  point=gal_pointer_allocate(GAL_TYPE_FLOAT64, p->ndim, 0, __func__,
                             "point");

  /* Go over all the rows in the second catalog that were assigned to this
     thread. */
  for(i=0; tprm->indexs[i] != GAL_BLANK_SIZE_T; ++i)
    {
      /* Set the easy-to-read indexs: this is the index in the second
         catalog, hence 'bi'. */
      bi = tprm->indexs[i];

      /* Fill the 'point' for this thread. But first, check if each of its
         dimensions fall within the coverage of A. */
      j=0;
      iscovered=1;
      Aexist=p->Aexist;
      for(ccol=p->B; ccol!=NULL; ccol=ccol->next)
        {
          if(iscovered)
            {
              /* Fill the point location in this dimension and set the
                 pointer. */
              existA=Aexist->array;
              po = point[j] = ((double *)(ccol->array))[ bi ];

              /* Make sure it covers the range of A (following the same set
                 of tests as in 'gal_statistics_histogram'). */
              if( po >= p->Amin[j] && po <= p->Amax[j] )
                {
                  h_i=(po-p->Amin[j])/p->Abinwidth[j];
                  if( existA[ h_i - (h_i==p->Aexist->size ? 1 : 0) ] == 0 )
                    iscovered=0;
                }
              else
                iscovered=0;
            }

          /* Increment the dimensionality counter. */
          ++j;
          Aexist=Aexist->next;
        }

      /* Continue with the match if the point is in-range. */
      if(iscovered)
        {
          /* Find the index of the nearest neighbor in the first catalog to
             this point in the second catalog. */
          ai = gal_kdtree_nearest_neighbour(p->A, p->A_kdtree,
                                            p->kdtree_root, point,
                                            &least_dist);

          /* If nothing was found within the least distance, then the 'ai'
             will be 'GAL_BLANK_SIZE_T'. */
          if(ai!=GAL_BLANK_SIZE_T)
            {
              /* Make sure the matched point is within the given aperture
                 (which may be elliptical). */
              for(j=0;j<p->ndim;++j)
                delta[j]=p->b[j][bi] - p->a[j][ai];
              r=match_distance(delta, p->iscircle, p->ndim, p->aperture,
                               p->c, p->s);

              /* If the radial distance is smaller than the radial measure,
                 then add this item to a match with 'ai'. */
              if(r<p->aperture[0])
                match_add_to_sfll(&p->bina[ai], bi, r);
            }

          /* For a check:
          if(ai==GAL_BLANK_SIZE_T)
            printf("second[%zu] matched with first[%zu].\n", bi);
          else
            printf("second[%zu] DIDN'T match with first.\n", bi, bi);
          */
        }
    }

  /* Clean up. */
  free(point);

  /* Wait for all the other threads to finish, then return. */
  if(tprm->b) pthread_barrier_wait(tprm->b);
  return NULL;
}





static void
match_kdtree_second_in_first(struct match_kdtree_params *p,
                             size_t numthreads, size_t minmapsize,
                             int quietmmap)
{
  double dist[3]; /* Just a place-holder in 'aperture_prepare'. */

  /* Prepare the aperture-related checks. */
  match_aperture_prepare(p->A, p->B, p->aperture,
                         p->ndim, p->a, p->b, dist, p->c,
                         p->s, &p->iscircle);

  /* Distribute the jobs in multiple threads. */
  gal_threads_spin_off(match_kdtree_worker, p, p->B->size,
                       numthreads, minmapsize, quietmmap);
}





gal_data_t *
gal_match_kdtree(gal_data_t *coord1, gal_data_t *coord2,
                 gal_data_t *coord1_kdtree, size_t kdtree_root,
                 double *aperture, size_t numthreads, size_t minmapsize,
                 int quietmmap, size_t *nummatched)
{
  gal_data_t *out=NULL;
  struct match_kdtree_params p;

  /* In case the 'k-d' tree is empty, just return a NULL pointer and the
     number of matches to zero. */
  if(coord1_kdtree==NULL) { *nummatched=0; return NULL; }

  /* Write the parameters into the structure. */
  p.A=coord1;
  p.B=coord2;
  p.aperture=aperture;
  p.A_kdtree=coord1_kdtree;
  p.kdtree_root=kdtree_root;

  /* Basic sanity checks. */
  match_kdtree_sanity_check(&p);

  /* Find all of the second catalog points that are within the acceptable
     radius of the first. */
  match_kdtree_second_in_first(&p, numthreads, minmapsize, quietmmap);

  /* Find the best match for each item (from possibly multiple matches). */
  match_rearrange(p.A, p.B, p.bina);

  /* The match is done, write the output. */
  out=match_output(p.A, p.B, NULL, NULL, p.bina, minmapsize, quietmmap);

  /* Set 'nummatched' and return output. */
  *nummatched = out ?  out->next->next->size : 0;

  /* Clean up and return. */
  free(p.bina);
  free(p.Amin);
  free(p.Amax);
  free(p.Abinwidth);
  gal_list_data_free(p.Aexist);
  return out;
}
