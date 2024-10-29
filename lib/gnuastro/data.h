/*********************************************************************
data -- Structure and functions to represent/work with data
This is part of GNU Astronomy Utilities (Gnuastro) package.

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
#ifndef __GAL_DATA_H__
#define __GAL_DATA_H__

/* Include other headers if necessary here. Note that other header files
   must be included before the C++ preparations below */
#include <math.h>
#include <wcslib/wcs.h>

/* When we are within Gnuastro's building process, 'IN_GNUASTRO_BUILD' is
   defined. In the build process, installation information (in particular
   the 'restrict' replacement) is kept in 'config.h' (top source
   directory). When building a user's programs, this information is kept in
   'gnuastro/config.h'. Note that all '.c' files in Gnuastro's source must
   start with the inclusion of 'config.h' and that 'gnuastro/config.h' is
   only created at installation time (not present during the building of
   Gnuastro). */
#ifndef IN_GNUASTRO_BUILD
#include <gnuastro/config.h>
#endif

#if GAL_CONFIG_HAVE_OPENCL
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#endif

#include <gnuastro/type.h>



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





/* Flag values for the dataset. Note that these are bit-values, so to be
   more clear, we'll use hexadecimal notation: '0x1' (=1), '0x2' (=2),
   '0x4' (=4), '0x8' (=8), '0x10' (=16), '0x20' (=32) and so on. */

/* Number of bytes in the unsigned integer hosting the bit-flags ('flag'
   element) of 'gal_data_t'. */
#define GAL_DATA_FLAG_SIZE         1

/* Bit 0: The has-blank flag has been checked, so a flag value of 0 for the
          blank flag is trustable. This can be very useful to avoid
          repetative checks when the necessary value of the bit is 0. */
#define GAL_DATA_FLAG_BLANK_CH     0x1

/* Bit 1: Dataset contains blank values. */
#define GAL_DATA_FLAG_HASBLANK     0x2

/* Bit 2: Sorted flags have been checked, see GAL_DATA_FLAG_BLANK_CH. */
#define GAL_DATA_FLAG_SORT_CH      0x4

/* Bit 3: Dataset is sorted and increasing. */
#define GAL_DATA_FLAG_SORTED_I     0x8

/* Bit 4: Dataset is sorted and decreasing. */
#define GAL_DATA_FLAG_SORTED_D     0x10

/* Maximum internal flag value. Higher-level flags can be defined with the
   bitwise shift operators on this value to define internal flags for
   libraries/programs that depend on Gnuastro without causing any possible
   conflict with the internal flags or having to check the values manually
   on every release. */
#define GAL_DATA_FLAG_MAXFLAG      GAL_DATA_FLAG_SORTED_D

#include <gnuastro/data-t.h>





/*********************************************************************/
/*************              allocation             *******************/
/*********************************************************************/
gal_data_t *
gal_data_alloc(void *array, uint8_t type, size_t ndim, size_t *dsize,
               struct wcsprm *wcs, int clear, size_t minmapsize,
               int quietmmap, char *name, char *unit, char *comment);

void
gal_data_initialize(gal_data_t *data, void *array, uint8_t type,
                    size_t ndim, size_t *dsize, struct wcsprm *wcs,
                    int clear, size_t minmapsize, int quietmmap,
                    char *name, char *unit, char *comment);

gal_data_t *
gal_data_alloc_empty(size_t ndim, size_t minmapsize, int quietmmap);

void
gal_data_free_contents(gal_data_t *data);

void
gal_data_free(gal_data_t *data);





/*********************************************************************/
/*************        Array of data structures      ******************/
/*********************************************************************/
gal_data_t *
gal_data_array_calloc(size_t size);

void
gal_data_array_free(gal_data_t *dataarr, size_t num, int free_array);

gal_data_t **
gal_data_array_ptr_calloc(size_t size);

void
gal_data_array_ptr_free(gal_data_t **dataptr, size_t size, int free_array);




/*************************************************************
 **************            Copying             ***************
 *************************************************************/
gal_data_t *
gal_data_copy(gal_data_t *in);

gal_data_t *
gal_data_copy_to_new_type(gal_data_t *in, uint8_t newtype);

gal_data_t *
gal_data_copy_to_new_type_free(gal_data_t *in, uint8_t newtype);

void
gal_data_copy_to_allocated(gal_data_t *in, gal_data_t *out);

gal_data_t *
gal_data_copy_string_to_number(char *string);


__END_C_DECLS    /* From C++ preparations */

#endif           /* __GAL_DATA_H__ */
