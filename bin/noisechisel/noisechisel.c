/*********************************************************************
NoiseChisel - Detect signal in a noisy dataset.
NoiseChisel is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2015-2024 Free Software Foundation, Inc.

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
#include <stdlib.h>

#include <gnuastro/fits.h>
#include <gnuastro/blank.h>
#include <gnuastro/convolve.h>

#include <gnuastro-internal/timing.h>
#include <gnuastro-internal/checkset.h>

#include "main.h"

#include "ui.h"
#include "sky.h"
#include "detection.h"
#include "threshold.h"










/***********************************************************************/
/*************  Wrapper functions (for clean high-level) ***************/
/***********************************************************************/
static void
noisechisel_convolve(struct noisechiselparams *p)
{
  struct timeval t1;
  struct gal_tile_two_layer_params *tl=&p->cp.tl;

  /* Convovle with sharper kernel. */
  if(p->conv==NULL)
    {
      /* Do the convolution if a kernel was requested. */
      if(p->kernel)
        {
          /* Make the convolved image. */
          if(!p->cp.quiet) gettimeofday(&t1, NULL);
          p->conv = gal_convolve_spatial(tl->tiles, p->kernel,
                                         p->cp.numthreads, 1,
                                         tl->workoverch, 0);

          /* Report and write check images if necessary. */
          if(!p->cp.quiet)
            {
              if(p->widekernel)
                gal_timing_report(&t1, "Convolved with sharper kernel.", 1);
              else
                gal_timing_report(&t1, "Convolved with given kernel.", 1);
            }
        }
      else
        p->conv=p->input;
    }

  /* Set a fixed name for the convolved image (since it will be used in
     many check images). */
  if(p->conv!=p->input)
    {
      if(p->conv->name) free(p->conv->name);
      gal_checkset_allocate_copy( ( p->widekernel
                                    ? "CONVOLVED-SHARPER"
                                    : "CONVOLVED" ), &p->conv->name);
    }

  /* Save the convolution step if necessary. */
  if(p->detectionname)
    {
      gal_fits_img_write(p->input, p->detectionname, NULL, 0);
      if(p->input!=p->conv)
        gal_fits_img_write(p->conv, p->detectionname, NULL, 0);
    }

  /* Convolve with wider kernel (if requested). */
  if(p->widekernel)
    {
      if(!p->cp.quiet) gettimeofday(&t1, NULL);
      p->wconv=gal_convolve_spatial(tl->tiles, p->widekernel,
                                    p->cp.numthreads, 1,
                                    tl->workoverch, 0);
      gal_checkset_allocate_copy("CONVOLVED-WIDER", &p->wconv->name);

      if(!p->cp.quiet)
        gal_timing_report(&t1, "Convolved with wider kernel.", 1);
    }
}




















/***********************************************************************/
/*************                   Output                  ***************/
/***********************************************************************/
/* Write the output file. */
static void
noisechisel_output(struct noisechiselparams *p)
{
  gal_fits_list_key_t *keys=NULL;

  /* Write the configuration keywords. */
  gal_fits_key_list_title_add_end(&p->cp.ckeys, "Input file", 0);
  gal_fits_key_write_filename("input", p->inputname, &p->cp.ckeys, 0,
                              p->cp.quiet);
  gal_fits_key_write(p->cp.ckeys, p->cp.output, "0", "NONE", 1, 1);


  /* Put a copy of the input into the output (when necessary). */
  if(p->rawoutput==0)
    {
      /* Subtract the Sky value. */
      sky_subtract(p);

      /* Correct the name of the input and write it out. */
      if(p->input->name) free(p->input->name);
      p->input->name="INPUT-NO-SKY";
      gal_fits_img_write(p->input, p->cp.output, NULL, 0);
      p->input->name=NULL;
    }


  /* Write the detected pixels and useful information into it's header. */
  gal_fits_key_list_add(&keys, GAL_TYPE_FLOAT32, "DETSN", 0,
                        &p->detsnthresh, 0,
                        "Minimum S/N of true pseudo-detections", 0,
                        "ratio", 0);
  if(p->label)
    gal_fits_key_list_add(&keys, GAL_TYPE_SIZE_T, "NUMLABS", 0,
                          &p->numdetections, 0, "Total number of labels "
                          "(inclusive)", 0, "counter", 0);
  gal_fits_key_list_reverse(&keys);
  if(p->label)
    {
      p->olabel->name = "DETECTIONS";
      gal_fits_img_write(p->olabel, p->cp.output, keys, 1);
      p->olabel->name=NULL;
    }
  else
    {
      p->binary->name = "DETECTIONS";
      gal_fits_img_write(p->binary, p->cp.output, keys, 1);
      p->binary->name=NULL;
    }
  keys=NULL;


  /* Write the Sky image into the output */
  if(p->sky->name) free(p->sky->name);
  p->sky->name="SKY";
  gal_tile_full_values_write(p->sky, &p->cp.tl, !p->ignoreblankintiles,
                             p->cp.output, NULL, 0);
  p->sky->name=NULL;


  /* Write the Sky standard deviation into the output. */
  p->std->name="SKY_STD";
  gal_fits_key_list_add(&keys, GAL_TYPE_FLOAT32, "MAXSTD", 0, &p->maxstd,
                        0, "Maximum raw tile standard deviation", 0,
                        p->input->unit, 0);
  gal_fits_key_list_add(&keys, GAL_TYPE_FLOAT32, "MINSTD", 0, &p->minstd,
                        0, "Minimum raw tile standard deviation", 0,
                        p->input->unit, 0);
  gal_fits_key_list_add(&keys, GAL_TYPE_FLOAT32, "MEDSTD", 0, &p->medstd,
                        0, "Median raw tile standard deviation", 0,
                        p->input->unit, 0);
  gal_tile_full_values_write(p->std, &p->cp.tl, !p->ignoreblankintiles,
                             p->cp.output, keys, 1);
  p->std->name=NULL;


  /* Let the user know that the output is written. */
  if(!p->cp.quiet)
    printf("  - Output written to '%s'.\n", p->cp.output);
}


















/***********************************************************************/
/*************             High level function           ***************/
/***********************************************************************/
void
noisechisel(struct noisechiselparams *p)
{
  /* Convolve the image. */
  noisechisel_convolve(p);

  /* Do the initial detection. */
  detection_initial(p);

  /* Remove false detections. */
  detection(p);

  /* Find the final Sky and Sky STD values. */
  sky_and_std(p, p->skyname);

  /* Abort if the user only wanted to see until this point.*/
  if(p->skyname && !p->continueaftercheck)
    ui_abort_after_check(p, p->skyname, NULL,
                         "derivation of final Sky (and its STD) value");

  /* Write the output. */
  noisechisel_output(p);
}
