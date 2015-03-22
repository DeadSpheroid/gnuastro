/*********************************************************************
ImageWarp - Warp images using projective mapping.
ImageWarp is part of GNU Astronomy Utilities (gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2015, Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <math.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <gsl/gsl_sort.h>

#include "main.h"
#include "imgwarp.h"

/***************************************************************/
/**************       Basic operations        ******************/
/***************************************************************/

/* We have a polygon and we want to order its corners in a counter an
   anticlockwise fashion. This is necessary for finding its area
   later. Depending on the transformation, the corners can have
   practically any order even if before the transformation, they were
   ordered.

   The input is an array containing the coordinates (two values) of
   each corner. `n' is the number of corners. So the length of the
   input should be 2*n. The output `ordinds' (ordered indexs) array
   should already be allocated (or defined) outside and should have
   `n+1' elements. There is one extra element because we want to close
   the polygon by coming back to its first element in the end. It will
   keep the index of the corners so that when they are called in the
   following fashion, the input corners will be ordered in a
   counter-clockwise fashion and finish where they started. (for a
   four-cornered polygon for example):

   in[ ordinds[0] * 2 ] --> x of first coordinate(unchanged)
   in[ ordinds[1] * 2 ] --> x of next counter clockwise corner.
   in[ ordinds[2] * 2 ] --> x of next counter clockwise corner.
   in[ ordinds[3] * 2 ] --> x of next counter clockwise corner.
   in[ ordinds[4] * 2 ] --> x of starting point.

   The same for the "y"s, just add a +1.

   The input array will not be changed, it will only be read from.
*/
void
orderedpolygoncorners(double *in, size_t n, size_t *ordinds)
{
  double angles[MAXPOLYGONCORNERS];
  size_t i, tmp, aindexs[MAXPOLYGONCORNERS], tindexs[MAXPOLYGONCORNERS];

  if(n>MAXPOLYGONCORNERS)
    error(EXIT_FAILURE, 0, "Most probably a bug! The number of corners "
          "given to `orderedpolygoncorners' is more than %d. This is an "
          "internal value and cannot be set from the outside. Most probably "
          "Some bug has caused this un-normal value. Please contact us at "
          PACKAGE_BUGREPORT" so we can solve this problem.",
          MAXPOLYGONCORNERS);

  /* Just a check:
  printf("\n\nInitial values:\n");
  for(i=0;i<n;++i)
    printf("%lu: %.3f, %.3f\n", i, in[i*2], in[i*2+1]);
  printf("\n");
  */

  /* Find the point with the smallest Y (if there is two of them, the
     one with the smallest X too.). This is necessary because if the
     angles are not found relative to this point, the ordering of the
     corners might not be correct in non-trivial cases. The number of
     points is usually very small (less than 10), so this is
     insignificant in the grand scheme of processes that have to take
     place in warping the image. */
  gsl_sort_index(ordinds, in+1, 2, n);
  if( in[ ordinds[0]*2+1 ] == in[ ordinds[1]*2+1 ]
     && in[ ordinds[0]*2] > in[ ordinds[1]*2 ])
    {
      tmp=ordinds[0];
      ordinds[0]=ordinds[1];
      ordinds[1]=tmp;
    }


  /* We only have `n-1' more elements to sort, use the angle of the
     line between the three remaining points and the first point. */
  for(i=0;i<n-1;++i)
    angles[i]=atan2( in[ ordinds[i+1]*2+1 ] - in[ ordinds[0]*2+1 ],
                     in[ ordinds[i+1]*2 ]   - in[ ordinds[0]*2   ] );
  /* For a check:
  for(i=0;i<n-1;++i)
    printf("%lu: %.3f degrees\n", ordinds[i+1], angles[i]*180/M_PI);
  printf("\n");
  */

  /* Sort the angles into the correct order, we need an extra array to
     temporarily keep the newly angle-ordered indexs. Without it we
     are going to loose half of the ordinds indexs! */
  gsl_sort_index(aindexs, angles, 1, n-1);
  for(i=0;i<n-1;++i) tindexs[i]=ordinds[aindexs[i]+1];
  for(i=0;i<n-1;++i) ordinds[i+1]=tindexs[i];

  /* Set the last element: */
  ordinds[n]=ordinds[0];

  /* For a check:
  printf("\nAfter sorting:\n");
  for(i=0;i<n+1;++i) printf("%lu\n", ordinds[i]);
  printf("\n");
  */
}





/* Do all the preparations.

   Make the output array. We transform the four corners of the image
   into the output space. To find the four sides of the image.

   About fpixel and lpixel. The point is that we don't want to spend
   time, transforming any pixels which we know will not be in the
   input image.

   Find the proper order of transformed pixel corners from the output
   array to the input array. The order is fixed for all the pixels in
   the image altough the scale might change.
*/
void
imgwarppreparations(struct imgwarpparams *p)
{
  size_t i;
  long brd[4];
  double *d, *df, output[8];
  double ocrn[8]={0,0,1,0,0,1,1,1}, icrn[8]={0,0,0,0,0,0,0,0};
  double xmin=DBL_MAX, xmax=-DBL_MAX, ymin=DBL_MAX, ymax=-DBL_MAX;
  double input[8]={0.0f, 0.0f, p->is1, 0.0f, 0.0f, p->is0, p->is1, p->is0};

  for(i=0;i<4;++i)
    {
      mappoint(&input[i*2], p->matrix, &output[i*2]);
      if(output[i*2]<xmin) xmin=output[i*2];
      if(output[i*2]>xmax) xmax=output[i*2];
      if(output[i*2+1]<ymin) ymin=output[i*2+1];
      if(output[i*2+1]>ymax) ymax=output[i*2+1];
    }
  /* For a check:
  for(i=0;i<4;++i)
      printf("(%.3f, %.3f) --> (%.3f, %.3f)\n",
             input[i*2], input[i*2+1], output[i*2], output[i*2+1]);
  printf("xmin: %.3f\nxmax: %.3f\nymin: %.3f\nymax: %.3f\n",
         xmin, xmax, ymin, ymax);
  */

  /* integer range of output image. If the maximum values have no
     fractional parts, then the last pixel should be used. */
  brd[0]=floor(xmin);
  brd[1] = floor(xmax)==ceil(xmax) ? floor(xmax)-1 : floor(xmax);
  brd[2]=floor(ymin);
  brd[3] = floor(ymax)==ceil(ymax) ? floor(ymax)-1 : floor(ymax);
  /* For a test:
  for(i=0;i<4;++i) printf("%ld, ", brd[i]); printf("\n");
  */

  /* Set the final size of the image. */
  p->os0=brd[1]-brd[0]+1;
  p->os1=brd[3]-brd[2]+1;
  p->outfpixval[0]=brd[0];
  p->outfpixval[1]=brd[2];
  /* For a test:
  printf("os0: %lu, os1: %lu\n\n", p->os0, p->os1);
  */

  /* In case there is translation, then (in the x axis) either xmin
     will be larger than zero or xmax will be smaller than zero. The
     same goes for the y axis.

     NOTE: The integer value of a pixel is its bottom left side.
     NOTE II: brd[3] and brd[1] are negative when used here.
  */
  if(brd[0]>=0) { p->os0 += brd[0];    p->outfpixval[0] -= brd[0]; }
  if(brd[1]<0)    p->os0 += -1*brd[1];
  if(brd[2]>=0) { p->os1 += brd[2];    p->outfpixval[1] -= brd[2]; }
  if(brd[3]<0)    p->os1 += -1*brd[2];
  /* For a test:
  printf("os0: %lu\nos1: %lu\n\n", p->os0, p->os1);
  */

  /* We now know the size of the output and the starting and ending
     coordinates in the output image (bottom left corners of pixels)
     for the transformation. */
  errno=0;
  p->output=malloc(p->os0*p->os1*sizeof *p->output);
  if(p->output==NULL)
    error(EXIT_FAILURE, errno, "%lu bytes for the output array",
          p->os0*p->os1*sizeof *p->output);
  df=(d=p->output)+p->os0*p->os1; do *d++=NAN; while(d<df);


  /* Order the corners of the inverse-transformed pixel (from the
     output to the input) in an anti-clockwise transformation. In a
     general homographic transform, the scales of the output pixels
     may change, but the relative positions of the corners will
     not. */
  for(i=0;i<4;++i)
    mappoint(&ocrn[i*2], p->inverse, &icrn[i*2]);
  orderedpolygoncorners(icrn, 4, p->oplygncrn);
}




















/***************************************************************/
/**************      Processing function      ******************/
/***************************************************************/
void *
imgwarponthread(void *inparam)
{
  struct iwpparams *iwp=(struct iwpparams*)inparam;
  struct imgwarpparams *p=iwp->p;




  /* Wait until all other threads finish. */
  if(p->cp.numthreads>1)
    pthread_barrier_wait(iwp->b);

  return NULL;
}



















/***************************************************************/
/**************       Outside function        ******************/
/***************************************************************/
void
imgwarp(struct imgwarpparams *p)
{
  int err;
  pthread_t t;          /* All thread ids saved in this, not used. */
  pthread_attr_t attr;
  pthread_barrier_t b;
  struct iwpparams *iwp;
  size_t nt=p->cp.numthreads;
  size_t i, nb, *indexs, thrdcols;


  /* Array keeping thread parameters for each thread. */
  errno=0;
  iwp=malloc(nt*sizeof *iwp);
  if(iwp==NULL)
    error(EXIT_FAILURE, errno, "%lu bytes in imgwarp "
          "(imgwarp.c) for iwp", nt*sizeof *iwp);


  /* Prepare the output array and all the necessary things: */
  imgwarppreparations(p);


  /* Distribute the output pixels into the threads: */
  distinthreads(p->os0*p->os1, nt, &indexs, &thrdcols);


  /* Start the convolution. */
  if(nt==1)
    {
      iwp[0].p=p;
      iwp[0].indexs=indexs;
      imgwarponthread(&iwp[0]);
    }
  else
    {
      /* Initialize the attributes. Note that this running thread
	 (that spinns off the nt threads) is also a thread, so the
	 number the barrier should be one more than the number of
	 threads spinned off. */
      if(p->os0*p->os1<nt) nb=p->os0*p->os1+1;
      else nb=nt+1;
      attrbarrierinit(&attr, &b, nb);

      /* Spin off the threads: */
      for(i=0;i<nt;++i)
        if(indexs[i*thrdcols]!=NONTHRDINDEX)
          {
            iwp[i].p=p;
            iwp[i].b=&b;
            iwp[i].indexs=&indexs[i*thrdcols];
	    err=pthread_create(&t, &attr, imgwarponthread, &iwp[i]);
	    if(err)
	      error(EXIT_FAILURE, 0, "Can't create thread %lu.", i);
          }

      /* Wait for all threads to finish and free the spaces. */
      pthread_barrier_wait(&b);
      pthread_attr_destroy(&attr);
      pthread_barrier_destroy(&b);
    }

  /* Free the allocated spaces: */
  free(iwp);
  free(indexs);
  free(p->output);
}
