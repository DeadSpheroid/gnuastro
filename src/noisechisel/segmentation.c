/*********************************************************************
NoiseChisel - Detect and segment signal in noise.
NoiseChisel is part of GNU Astronomy Utilities (Gnuastro) package.

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

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

#include "fitsarrayvv.h"

#include "main.h"

#include "label.h"
#include "clumps.h"
#include "segmentation.h"






/******************************************************************/
/*****************         Segmentation         *******************/
/******************************************************************/
void *
segmentonthread(void *inparam)
{
  struct segmentationparams *sp=(struct segmentationparams *)inparam;
  struct noisechiselparams *p=sp->p;

  size_t i;

  /* Go over all the initial detections that were assigned to this
     thread. */
  for(i=0;sp->indexs[i]!=NONTHRDINDEX;++i)
    {
      /* Keep the initial label of this detection. */
      sp->thisinitlab=sp->indexs[i];


    }

  /* Wait until all the threads finish: */
  if(p->cp.numthreads>1)
    pthread_barrier_wait(sp->b);
  return NULL;
}





void
segmentdetections(struct noisechiselparams *p, size_t numobjsinit,
                  size_t *labareas, size_t **labinds)
{
  int err;
  pthread_t t; /* We don't use the thread id, so all are saved here. */
  pthread_attr_t attr;
  pthread_barrier_t b;
  struct segmentationparams *sp;
  size_t i, nb, *indexs, thrdcols;
  size_t numthreads=p->cp.numthreads;


  /* Allocate the arrays to keep the thread and parameters for each
     thread. */
  errno=0; sp=malloc(numthreads*sizeof *sp);
  if(sp==NULL)
    error(EXIT_FAILURE, errno, "%lu bytes in segmentdetections "
          "(segmentation.c) for sp", numthreads*sizeof *sp);


  /* Distribute the initial labels between all the threads. */
  distinthreads(numobjsinit, numthreads, &indexs, &thrdcols);


  /* Spin off the threads to work on each object if more than one
     thread will be used. If not, simply start working on all the
     detections. */
  if(numthreads==1)
    {
      sp[0].p=p;
      sp[0].id=0;
      sp[0].indexs=indexs;
      sp[0].labinds=labinds;
      sp[0].labareas=labareas;
      segmentonthread(&sp[0]);
    }
  else
    {
      /* Initialize the attributes. Note that this running thread
	 (that spinns off the nt threads) is also a thread, so the
	 number the barrier should be one more than the number of
	 threads spinned off. */
      if(numobjsinit<numthreads) nb=numobjsinit+1;
      else                       nb=numthreads+1;
      attrbarrierinit(&attr, &b, nb);

      /* Spin off the threads: */
      for(i=0;i<numthreads;++i)
	if(indexs[i*thrdcols]!=NONTHRDINDEX)
	  {
            sp[i].p=p;
            sp[i].id=i;
            sp[i].b=&b;
            sp[i].labinds=labinds;
            sp[i].labareas=labareas;
            sp[i].indexs=&indexs[i*thrdcols];
	    err=pthread_create(&t, &attr, segmentonthread, &sp[i]);
	    if(err) error(EXIT_FAILURE, 0, "Can't create thread %lu.", i);
	  }

      /* Wait for all threads to finish and free the spaces. */
      pthread_barrier_wait(&b);
      pthread_attr_destroy(&attr);
      pthread_barrier_destroy(&b);
    }

  free(sp);
  free(indexs);
}




















/******************************************************************/
/*****************         Main function        *******************/
/******************************************************************/
void
segmentation(struct noisechiselparams *p)
{
  float *f, *fp;
  char *extname=NULL;
  long *l, *c, *lp, *forfits=NULL;
  size_t i, s0=p->smp.s0, s1=p->smp.s1;
  char *segmentationname=p->segmentationname;
  size_t *labareas, **labinds, numobjsinit=p->numobjects;

  /* Start off the counter for the number of objects and clumps. The
     value to these variables will be the label that is given to the
     next clump or object found. Note that we stored a copy of the
     initial number of objects in the numobjsinit variable above.*/
  p->numclumps=1;
  p->numobjects=1;


  /* Start the steps image: */
  if(segmentationname)
    {
      arraytofitsimg(segmentationname, "Input-SkySubtracted",
                     FLOAT_IMG, p->img, s0, s1, p->numblank,
                     p->wcs, NULL, SPACK_STRING);
      arraytofitsimg(segmentationname, "Convolved-SkySubtracted",
                     FLOAT_IMG, p->conv, s0, s1, p->numblank,
                     p->wcs, NULL, SPACK_STRING);
      arraytofitsimg(segmentationname, "InitialLabels",
                     LONG_IMG, p->olab, s0, s1, 0, p->wcs,
                     NULL, SPACK_STRING);
    }

  /* All possible NaN pixels should be given the largest possible
     float flux in the convolved image (which is used for
     over-segmentation). NOTE that the convolved image is used for
     relative pixel values, not absolute ones. This is because NaN
     pixels might be in the centers of stars or bright objects (they
     might slice through a connected region). So we can't allow them
     to cut our objects. The safest approach is to start segmentation
     of each object or noise mesh with the NaNs it might contain. A
     NaN region will never be calculated in any flux measurement any
     way and if it is connecting two bright objects, they will be
     segmented because on the two sides of the NaN region, they have
     different fluxes.*/
  if(p->numblank)
    {
      fp=(f=p->conv)+s0*s1;
      do if(isnan(*f)) *f=FLT_MAX; while(++f<fp);
    }


  /* Find the true clump S/N threshold andput it in p->lmp.garray1. */
  p->b0f1=0;
  clumpsngrid(p);


  /* Save the indexs of all the detected labels. We will be working on
     each label independently: */
  labindexs(p->olab, s0*s1, numobjsinit, &labareas, &labinds);


  /* We don't need the labels in the p->olab and p->clab any
     more. From now on, they will be used to keep the final values. */
  c=p->clab; lp=(l=p->olab)+s0*s1; do *l=*c++=0; while(++l<lp);


  /* Segment the detections. When the viewer wants to check the steps,
     the operations have to be repeated multiple times to output each
     step. */
  if(p->segmentationname)
    {
      p->stepnum=1;
      while(p->stepnum<6)
	{
	  memset(p->clab, 0, s0*s1*sizeof *p->clab);
          segmentdetections(p, numobjsinit, labareas, labinds);
          switch(p->stepnum)
            {
            case 1:
              extname="Over-segmentation"; forfits=p->clab; break;
            case 2:
              extname="Successful clumps"; forfits=p->clab; break;
            case 3:
              extname="Clumps grown"; forfits=p->olab; break;
            case 4:
              extname="Objects found"; forfits=p->olab; break;
            case 5:
              extname="Objects grown"; forfits=p->olab; break;
            default:
              error(EXIT_FAILURE, 0, "A bug! Please contact us at %s to "
                    "fix the problem. For some reason, the variable "
                    "p->stepnum in segmenation (segmentation.c) has the "
                    "unrecognized value of %d.", PACKAGE_BUGREPORT,
                    p->stepnum);
            }
          arraytofitsimg(p->segmentationname, extname, LONG_IMG, forfits,
                         s0, s1, 0, p->wcs, NULL, SPACK_STRING);
          ++p->stepnum;
        }
    }
  else
    segmentdetections(p, numobjsinit, labareas, labinds);


  /* Clean up */
  for(i=0;i<numobjsinit;++i) free(labinds[i]);
  free(labareas);
  free(labinds);
}