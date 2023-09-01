/*********************************************************************
Functions to that only use WCSLIB's functions, not related to FITS.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2015-2023 Free Software Foundation, Inc.

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
#ifndef __GAL_WCS_H__
#define __GAL_WCS_H__

/* Include other headers if necessary here. Note that other header files
   must be included before the C++ preparations below */
#include <fitsio.h>
#include <wcslib/wcs.h>

#include <gnuastro/data.h>
#include <gnuastro/fits.h>

/* Assumed floating point error in the WCS-related functionality. */
#define GAL_WCS_FLTERROR 1e-12

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





/*************************************************************
 **************           Constants            ***************
 *************************************************************/
/* Macros to identify the type of distortion for conversions. */
enum gal_wcs_distortions
{
  GAL_WCS_DISTORTION_INVALID,         /* Invalid (=0 by C standard).    */

  GAL_WCS_DISTORTION_TPD,             /* The TPD polynomial distortion. */
  GAL_WCS_DISTORTION_SIP,             /* The SIP polynomial distortion. */
  GAL_WCS_DISTORTION_TPV,             /* The TPV polynomial distortion. */
  GAL_WCS_DISTORTION_DSS,             /* The DSS polynomial distortion. */
  GAL_WCS_DISTORTION_WAT,             /* The WAT polynomial distortion. */
};

/* Macros to identify coordinate system for convesions. */
enum gal_wcs_coordsys
{
  GAL_WCS_COORDSYS_INVALID,           /* Invalid (=0 by C standard).    */

  GAL_WCS_COORDSYS_EQB1950,           /* Equatorial B1950 */
  GAL_WCS_COORDSYS_EQJ2000,           /* Equatorial J2000 */
  GAL_WCS_COORDSYS_ECB1950,           /* Ecliptic B1950 */
  GAL_WCS_COORDSYS_ECJ2000,           /* Ecliptic J2000 */
  GAL_WCS_COORDSYS_GALACTIC,          /* Galactic */
  GAL_WCS_COORDSYS_SUPERGALACTIC,     /* Super-galactic */
};

/* Macros to identify the type of distortion for conversions. */
enum gal_wcs_linear_matrix
{
  GAL_WCS_LINEAR_MATRIX_INVALID,      /* Invalid (=0 by C standard).    */

  GAL_WCS_LINEAR_MATRIX_PC,
  GAL_WCS_LINEAR_MATRIX_CD,
};





/*************************************************************
 ***********               Read WCS                ***********
 *************************************************************/
struct wcsprm *
gal_wcs_read_fitsptr(fitsfile *fptr, int linearmatrix, size_t hstartwcs,
                     size_t hendwcs, int *nwcs);

struct wcsprm *
gal_wcs_read(char *filename, char *hdu, int linearmatrix,
             size_t hstartwcs, size_t hendwcs, int *nwcs,
             char *hdu_option_name);

struct wcsprm *
gal_wcs_create(double *crpix, double *crval, double *cdelt,
               double *pc, char **cunit, char **ctype, size_t ndim,
               int linearmatrix);

void
gal_wcs_free(struct wcsprm *wcs);

char *
gal_wcs_dimension_name(struct wcsprm *wcs, size_t dimension);



/*************************************************************
 ***********               Write WCS               ***********
 *************************************************************/
void
gal_wcs_write(struct wcsprm *wcs, char *filename,
              char *extname, gal_fits_list_key_t *headers,
              char *program_string);

char *
gal_wcs_write_wcsstr(struct wcsprm *wcs, int *nkeyrec);

void
gal_wcs_write_in_fitsptr(fitsfile *fptr, struct wcsprm *wcs);



/*************************************************************
 ***********              Distortions              ***********
 *************************************************************/
int
gal_wcs_coordsys_from_string(char *coordsys);

int
gal_wcs_coordsys_identify(struct wcsprm *inwcs);

struct wcsprm *
gal_wcs_coordsys_convert(struct wcsprm *inwcs, int coordsysid);



/*************************************************************
 ***********              Distortions              ***********
 *************************************************************/
int
gal_wcs_distortion_from_string(char *distortion);

char *
gal_wcs_distortion_to_string(int distortion);

int
gal_wcs_distortion_identify(struct wcsprm *wcs);

struct wcsprm *
gal_wcs_distortion_convert(struct wcsprm *inwcs,
                           int out_distortion, size_t *fitsize);





/**************************************************************/
/**********              Utilities                 ************/
/**************************************************************/
struct wcsprm *
gal_wcs_copy(struct wcsprm *wcs);

struct wcsprm *
gal_wcs_copy_new_crval(struct wcsprm *in, double *crval);

void
gal_wcs_remove_dimension(struct wcsprm *wcs, size_t fitsdim);

void
gal_wcs_on_tile(gal_data_t *tile);

double *
gal_wcs_warp_matrix(struct wcsprm *wcs);

void
gal_wcs_clean_errors(struct wcsprm *wcs);

void
gal_wcs_decompose_pc_cdelt(struct wcsprm *wcs);

void
gal_wcs_to_cd(struct wcsprm *wcs);

double
gal_wcs_angular_distance_deg(double r1, double d1, double r2, double d2);

void
gal_wcs_box_vertices_from_center(double ra_center, double dec_center,
                                 double ra_delta,  double dec_delta,
                                 double *out);

double *
gal_wcs_pixel_scale(struct wcsprm *wcs);

double
gal_wcs_pixel_area_arcsec2(struct wcsprm *wcs);

int
gal_wcs_coverage(char *filename, char *hdu, size_t *ondim,
                 double **ocenter, double **owidth, double **omin,
                 double **omax, char *hdu_option_name);





/**************************************************************/
/**********              Conversion                ************/
/**************************************************************/
gal_data_t *
gal_wcs_world_to_img(gal_data_t *coords, struct wcsprm *wcs, int inplace);

gal_data_t *
gal_wcs_img_to_world(gal_data_t *coords, struct wcsprm *wcs, int inplace);





__END_C_DECLS    /* From C++ preparations */

#endif           /* __GAL_WCS_H__ */
