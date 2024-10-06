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

#if GAL_CONFIG_HAVE_OPENCL
#include <CL/cl.h>
#endif

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
__BEGIN_C_DECLS /* From C++ preparations */

gal_data_t *
gal_convolve_spatial (gal_data_t *input, gal_data_t *kernel,
                        size_t numthreads, size_t *channels,
                        int noedgecorrection, int convoverch, int conv_on_blank);

#if GAL_CONFIG_HAVE_OPENCL

gal_data_t *
gal_convolve_cl (gal_data_t *input_image, gal_data_t *kernel_image,
               char *kernel_name, int edgecorrect, size_t *channels,
               int convoverch, cl_context context,
               cl_device_id device);

gal_data_t *
gal_convolve_cl_unopt (gal_data_t *input_image, gal_data_t *kernel_image,
             char *kernel_name, int noedgecorrect, size_t *channels,
             int convoverch, cl_context context, cl_device_id device_id);

#endif
__END_C_DECLS    /* From C++ preparations */
#endif
