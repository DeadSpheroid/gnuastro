/*********************************************************************
ImageCrop - Crop a given size from one or multiple images.
ImageCrop is part of GNU Astronomy Utilities (gnuastro) package.

Copyright (C) 2013-2015 Mohammad Akhlaghi
Tohoku University Astronomical Institute, Sendai, Japan.
http://astr.tohoku.ac.jp/~akhlaghi/

gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "timing.h"
#include "checkset.h"
#include "fitsarrayvv.h"
#include "astrthreads.h"

#include "main.h"

#include "crop.h"
#include "wcsmode.h"




void *
imgmodecrop(void *inparam)
{
  struct cropparams *crp=(struct cropparams *)inparam;
  struct imgcropparams *p=crp->p;
  struct commonparams *cp=&p->cp;

  size_t i;
  int status;
  struct inputimgs *img;
  struct imgcroplog *log;
  char msg[VERBMSGLENGTH_V];

  /* In image mode, we always only have one image. */
  crp->imgindex=0;

  /* The whole catalog is from one image, so you can get the
     information here:*/
  img=&p->imgs[crp->imgindex];
  readfitshdu(img->name, cp->hdu, IMAGE_HDU, &crp->infits);

  /* Go over all the outputs that are assigned to this thread: */
  for(i=0;crp->indexs[i]!=NONTHRDINDEX;++i)
    {
      /* Set all the output parameters: */
      crp->outindex=crp->indexs[i];
      log=&p->log[crp->outindex];
      crp->outfits=NULL;
      log->numimg=0;
      cropname(crp);

      /* Crop the image. */
      onecrop(crp);

      /* Check the final output: */
      if(log->numimg)
	{
	  log->centerfilled=iscenterfilled(crp);

	  /* Add the final headers and close output FITS image: */
	  copyrightandend(crp->outfits, SPACK_STRING);
	  status=0;
	  if( fits_close_file(crp->outfits, &status) )
	    fitsioerror(status, "CFITSIO could not close the opened file.");

	  /* Remove the output image if its center was not filled. */
	  if(log->centerfilled==0 && p->keepblankcenter==0)
	    {
	      errno=0;
	      if(unlink(log->name))
		error(EXIT_FAILURE, errno, "%s", log->name);
	    }
	}
      else log->centerfilled=0;

      /* Write the log entry for this crop, in this mode, each output
	 image was only cropped from one image. Then print the result
	 on the terminal, if the user askd for it. */
      if(cp->verb)
	{
	  sprintf(msg, "%-30s %lu %d", log->name, log->numimg,
		  log->centerfilled);
	  reporttiming(NULL, msg, 2);
	}
    }

  /* Close the input image. */
  status=0;
  if( fits_close_file(crp->infits, &status) )
    fitsioerror(status, "imgmode.c: imgcroponthreads could "
		"not close FITS file.");

  /* Wait until all other threads finish. */
  if(cp->numthreads>1)
    pthread_barrier_wait(crp->b);

  return NULL;
}





void *
wcsmodecrop(void *inparam)
{
  struct cropparams *crp=(struct cropparams *)inparam;
  struct imgcropparams *p=crp->p;

  size_t i;
  int status, tcatset=0;
  struct imgcroplog *log;
  char msg[VERBMSGLENGTH_V];

  /* Go over all the output objects for this thread. */
  for(i=0;crp->indexs[i]!=NONTHRDINDEX;++i)
    {
      /* Set all the output parameters: */
      crp->outindex=crp->indexs[i];
      log=&p->log[crp->outindex];
      crp->outfits=NULL;
      log->name=NULL;
      log->numimg=0;


      /* Set the sides of the crop in RA and Dec */
      setcsides(crp);


      /* Go over all the images to see if this target is within their
	 range or not. */
      crp->imgindex=0;
      do
	if(radecoverlap(crp))
	  {
	    readfitshdu(p->imgs[crp->imgindex].name, p->cp.hdu,
			IMAGE_HDU, &crp->infits);

	    if(log->name==NULL) cropname(crp);

	    onecrop(crp);

	    status=0;
	    if( fits_close_file(crp->infits, &status) )
	      fitsioerror(status, "imgmode.c: imgcroponthreads could "
			  "not close FITS file.");
	  }
      while ( ++(crp->imgindex) < p->numimg );


      /* Check the final output: */
      if(log->numimg)
	{
	  log->centerfilled=iscenterfilled(crp);

	  copyrightandend(crp->outfits, SPACK_STRING);
	  status=0;
	  if( fits_close_file(crp->outfits, &status) )
	    fitsioerror(status, "CFITSIO could not close the opened file.");

	  if(log->centerfilled==0 && p->keepblankcenter==0)
	    {
	      errno=0;
	      if(unlink(log->name))
		error(EXIT_FAILURE, errno, "%s", log->name);
	    }
	}
      else
	{
	  if(p->up.catset==0)	/* Trick cropname into making a catalog */
	    {			/* So we have a name for log report. */
	      tcatset=1;
	      p->up.catset=1;
	    }
	  cropname(crp);
	  if(tcatset) p->up.catset=0;
	  log->centerfilled=0;
	}

      /* Write the log entry for this crop, in this mode, each output
	 image was only cropped from one image. Then print the result
	 on the terminal, if the user askd for it. */
      if(p->cp.verb)
	{
	  sprintf(msg, "%-30s %lu %d", log->name, log->numimg,
		  log->centerfilled);
	  reporttiming(NULL, msg, 2);
	}
    }

  /* Wait until all other threads finish. */
  if(p->cp.numthreads>1)
    pthread_barrier_wait(crp->b);

  return NULL;
}




















/*******************************************************************/
/**************           Output function           ****************/
/*******************************************************************/
/* Main function for the Image Mode. It is assumed that if only one
   crop box from each input image is desired, the first and last
   pixels are already set, irrespective of how the user specified that
   box.  */
void
imgcrop(struct imgcropparams *p)
{
  int err;
  pthread_t t; /* We don't use the thread id, so all are saved here. */
  pthread_attr_t attr;
  pthread_barrier_t b;
  struct cropparams *crp;
  size_t i, *indexs, thrdcols;
  void *(*modefunction)(void *);
  size_t nt=p->cp.numthreads, nb;


  /* Set the function to run: */
  if(p->imgmode)
    modefunction=&imgmodecrop;
  else if(p->wcsmode)
    modefunction=&wcsmodecrop;
  else
    error(EXIT_FAILURE, 0, "A bug! Somehow in imgcrop (imgcrop.c), "
	  "neither the imgmode is on or the wcsmode! Please contact us "
	  "so we can fix it, thanks.");


  /* Allocate the arrays to keep the thread and parameters for each
     thread. */
  errno=0;
  crp=malloc(nt*sizeof *crp);
  if(crp==NULL)
    error(EXIT_FAILURE, errno,
	  "%lu bytes in imgcrop (imgcrop.c) for crp", nt*sizeof *crp);


  /* Get the length of the output, no reasonable integer can have more
     than 50 characters! Since this is fixed for all the threads and
     images, we will just find it once here. */
  crp[0].outlen=strlen(p->cp.output)+strlen(p->suffix)+50;


  /* Distribute the indexs into the threads (this is needed even if we
     only have one object where p->cs0 is not defined): */
  if(p->up.catset)
    distinthreads(p->cs0, nt, &indexs, &thrdcols);
  else
    distinthreads(1, nt, &indexs, &thrdcols);



  /* Run the job, if there is only one thread, don't go through the
     trouble of spinning off a thread! */
  if(nt==1)
    {
      crp[0].p=p;
      crp[0].indexs=indexs;
      modefunction(&crp[0]);
    }
  else
    {
      /* Initialize the attributes. Note that this running thread
	 (that spinns off the nt threads) is also a thread, so the
	 number the barrier should be one more than the number of
	 threads spinned off. */
      if(p->cs0<nt) nb=p->cs0+1;
      else nb=nt+1;
      attrbarrierinit(&attr, &b, nb);

      /* Spin off the threads: */
      for(i=0;i<nt;++i)
	if(indexs[i*thrdcols]!=NONTHRDINDEX)
	  {
	    crp[i].p=p;
	    crp[i].b=&b;
	    crp[i].outlen=crp[0].outlen;
	    crp[i].indexs=&indexs[i*thrdcols];
	    err=pthread_create(&t, &attr, modefunction, &crp[i]);
	    if(err)
	      error(EXIT_FAILURE, 0, "Can't create thread %lu.", i);
	  }

      /* Wait for all threads to finish and free the spaces. */
      pthread_barrier_wait(&b);
      pthread_attr_destroy(&attr);
      pthread_barrier_destroy(&b);
    }


  /* Print the log file: */
  printlog(p);

  free(crp);
  free(indexs);
}
