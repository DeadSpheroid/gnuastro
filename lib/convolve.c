/*********************************************************************
Convolve -- Convolve a dataset with a given kernel.
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

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <stdlib.h>

#include <gnuastro/list.h>
#include <gnuastro/tile.h>
#include <gnuastro/threads.h>
#include <gnuastro/pointer.h>
#include <gnuastro/convolve.h>
#include <gnuastro/dimension.h>

#include <gnuastro-internal/checkset.h>

typedef struct params
{
  gal_data_t *input;
  gal_data_t *kernel;
  gal_data_t *output;
  int noedgecorrection;
  int convoverch;
  int tile_w;
  int tile_h;
} params;

#include "kernels/conv.cl"

void *
convolve_thread(void * in_arg)
{
    struct gal_threads_params *tprm = (struct gal_threads_params *)in_arg;
    struct params *p = (struct params *)tprm->params;

    size_t i, index;

        for (i = 0; tprm->indexs[i] != GAL_BLANK_SIZE_T; ++i)
          {
            /* For easy reading. */
            index = tprm->indexs[i];

            convolution (p->input, p->kernel, p->output, p->tile_w, p->tile_h,
                         p->noedgecorrection, p->convoverch, index);
          }
    if (tprm->b)
      pthread_barrier_wait (tprm->b);
    return NULL;
}

gal_data_t *
gal_convolve_spatial (gal_data_t *input, gal_data_t *kernel, size_t numthreads,
                      size_t *channels, int noedgecorrection, int convoverch,
                      int conv_on_blank)
{
  gal_data_t *out = gal_data_alloc (
      NULL, input->type, input->ndim, input->dsize, input->wcs, 0,
      input->minmapsize, input->quietmmap, NULL, input->unit, NULL);
  params p;
  p.output = out;
  p.kernel = kernel;
  p.input = input->block == NULL? input: input->block;
  p.noedgecorrection = noedgecorrection;
  p.convoverch = convoverch;
  if(channels == NULL)
  {
    p.tile_w = 1;
    p.tile_h = 1;
  }
  else
  {
    p.tile_w = input->dsize[0] / channels[0];
    p.tile_h = input->dsize[1] / channels[1];
    if(channels[0] == 1 && channels[1] == 1) p.convoverch = 1;
  }

  gal_threads_spin_off(convolve_thread, &p, input->size, numthreads, -1, -1);
  return out;
}