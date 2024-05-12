/*********************************************************************
Convolve -- Convolve the dataset with a given kernel.
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
#ifndef __GAL_CONVOLVE_H__
#define __GAL_CONVOLVE_H__

/* Include other headers if necessary here. Note that other header files
   must be included before the C++ preparations below */
#include <gnuastro/data.h>
#ifndef IN_GNUASTRO_BUILD
#include <gnuastro/config.h>
#endif

/* C++ Preparations */
#undef __BEGIN_C_DECLS
#undef __END_C_DECLS
#ifdef __cplusplus
# define __BEGIN_C_DECLS extern "C" {
# define __END_C_DECLS }
#else
# define __BEGIN_C_DECLS                /* empty */
# define __END_C_DECLS                  /* empty */
#endif
/* End of C++ preparations */

/* Actual header contants (the above were for the Pre-processor). */
__BEGIN_C_DECLS  /* From C++ preparations */



gal_data_t *
gal_convolve_spatial(gal_data_t *tiles, gal_data_t *kernel,
                     size_t numthreads, int edgecorrection,
                     int convoverch, int conv_on_blank);


void
gal_convolve_spatial_correct_ch_edge(gal_data_t *tiles, gal_data_t *kernel,
                                     size_t numthreads, int edgecorrection,
                                     int conv_on_blank,
                                     gal_data_t *tocorrect);

#if GAL_CONFIG_HAVE_OPENCL

gal_data_t *
gal_conv_cl(gal_data_t *input_image, gal_data_t *kernel_image, 
            char * cl_kernel_name, char * function_name, char * core_name,
            size_t global_item_size, size_t local_item_size, int device);


__END_C_DECLS    /* From C++ preparations */
#endif
#endif
