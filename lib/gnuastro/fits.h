/*********************************************************************
Functions to convert a FITS array to a C array and vice versa.
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
#ifndef __GAL_FITS_H__
#define __GAL_FITS_H__

/* When we are within Gnuastro's building process, 'IN_GNUASTRO_BUILD' is
   defined. In the build process, installation information (in particular
   'GAL_CONFIG_HAVE_WCSLIB_VERION' that we need in 'fits.c') is kept in
   'config.h'. When building a user's programs, this information is kept in
   'gnuastro/config.h'. Note that all '.c' files must start with the
   inclusion of 'config.h' and that 'gnuastro/config.h' is only created at
   installation time (not present during the building of Gnuastro).*/
#ifndef IN_GNUASTRO_BUILD
#include <gnuastro/config.h>
#endif

/* Include other headers if necessary here. Note that other header files
   must be included before the C++ preparations below */
#include <math.h>
#include <time.h>
#include <float.h>

#include <fitsio.h>
#include <wcslib/wcs.h>
#include <wcslib/wcshdr.h>
#include <wcslib/wcsfix.h>

#include <gnuastro/list.h>
#include <gnuastro/data.h>
/* gnuastro/table.h is included below. */

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



/* Macros. */
#define GAL_FITS_MAX_NDIM 999
#define GAL_FITS_KEY_TITLE_START "                      / "





/* To create a linked list of headers. */
typedef struct gal_fits_list_key_t
{
  char                      *title;   /* !=NULL, only print title. */
  char                *fullcomment;   /* Fully COMMENT (no name).  */
  char                    *keyname;   /* Keyword Name.             */
  void                      *value;   /* Keyword value.            */
  uint8_t                     type;   /* Keyword value type.       */
  char                    *comment;   /* Keyword comment.          */
  char                       *unit;   /* Keyword unit.             */
  int                        tfree;   /* ==1, free title string.   */
  int                       fcfree;   /* ==1, free full comment.   */
  int                        kfree;   /* ==1, free keyword name.   */
  int                        vfree;   /* ==1, free keyword value.  */
  int                        cfree;   /* ==1, free comment.        */
  int                        ufree;   /* ==1, free unit.           */
  struct gal_fits_list_key_t *next;   /* Pointer next keyword.     */
} gal_fits_list_key_t;





/* table.h needs 'gal_fits_list_key_t', so it should be included here, not
   with the other headers above. */
#include <gnuastro/table.h>





/*************************************************************
 **************        Reporting errors:       ***************
 *************************************************************/
void
gal_fits_io_error(int status, char *message);





/*************************************************************
 **************           FITS names           ***************
 *************************************************************/
int
gal_fits_name_is_fits(char *name);

int
gal_fits_suffix_is_fits(char *suffix);

int
gal_fits_file_recognized(char *filename);

char *
gal_fits_name_save_as_string(char *filename, char *hdu);



/*************************************************************
 **************           Type codes           ***************
 *************************************************************/
uint8_t
gal_fits_bitpix_to_type(int bitpix);

int
gal_fits_type_to_bitpix(uint8_t type);

char
gal_fits_type_to_bin_tform(uint8_t type);

int
gal_fits_type_to_datatype(uint8_t type);

uint8_t
gal_fits_datatype_to_type(int datatype, int is_table_column);




/**************************************************************/
/**********                  HDU                   ************/
/**************************************************************/
fitsfile *
gal_fits_open_to_write(char *filename);

size_t
gal_fits_hdu_num(char *filename);

unsigned long
gal_fits_hdu_datasum(char *filename, char *hdu, char *hdu_option_name);

unsigned long
gal_fits_hdu_datasum_ptr(fitsfile *fptr);

int
gal_fits_hdu_format(char *filename, char *hdu, char *hdu_option_name);

int
gal_fits_hdu_is_healpix(fitsfile *fptr);

fitsfile *
gal_fits_hdu_open(char *filename, char *hdu, int iomode, int exitonerror,
                  char *hdu_option_name);

fitsfile *
gal_fits_hdu_open_format(char *filename, char *hdu, int img0_tab1,
                         char *hdu_option_name);




/**************************************************************/
/**********            Header keywords             ************/
/**************************************************************/
int
gal_fits_key_exists_fptr(fitsfile *fptr, char *keyname);

void *
gal_fits_key_img_blank(uint8_t type);

void
gal_fits_key_clean_str_value(char *string);

char *
gal_fits_key_date_to_struct_tm(char *fitsdate, struct tm *tp);

size_t
gal_fits_key_date_to_seconds(char *fitsdate, char **subsecstr,
                             double *subsec);

void
gal_fits_key_read_from_ptr(fitsfile *fptr, gal_data_t *keysll,
                           int readcomment, int readunit);

void
gal_fits_key_read(char *filename, char *hdu, gal_data_t *keysll,
                  int readcomment, int readunit, char *hdu_option_name);

void
gal_fits_key_list_add(gal_fits_list_key_t **list, uint8_t type,
                      char *keyname, int kfree, void *value, int vfree,
                      char *comment, int cfree, char *unit, int ufree);

void
gal_fits_key_list_add_end(gal_fits_list_key_t **list, uint8_t type,
                          char *keyname, int kfree, void *value, int vfree,
                          char *comment, int cfree, char *unit, int ufree);

void
gal_fits_key_list_title_add(gal_fits_list_key_t **list, char *title,
                            int tfree);

void
gal_fits_key_list_title_add_end(gal_fits_list_key_t **list, char *title,
                                int tfree);

void
gal_fits_key_list_fullcomment_add(gal_fits_list_key_t **list,
                                  char *fullcomment, int fcfree);

void
gal_fits_key_list_fullcomment_add_end(gal_fits_list_key_t **list,
                                      char *fullcomment, int fcfree);

void
gal_fits_key_list_add_date(gal_fits_list_key_t **keylist,
                           char *comment);

void
gal_fits_key_list_add_software_versions(gal_fits_list_key_t **keylist);

void
gal_fits_key_list_add_git_commit(gal_fits_list_key_t **keylist);

void
gal_fits_key_list_reverse(gal_fits_list_key_t **list);

void
gal_fits_key_write_title_in_ptr(char *title, fitsfile *fptr);

void
gal_fits_key_write_filename(char *keynamebase, char *filename,
                            gal_fits_list_key_t **list, int top1end0,
                            int quiet);

void
gal_fits_key_write_wcsstr(fitsfile *fptr, struct wcsprm *wcs,
                          char *wcsstr, int nkeyrec);

void
gal_fits_key_write(gal_fits_list_key_t *keylist, char *filename,
                   char *hdu, char *hdu_option_name, int freekeys,
                   int create_fits_not_exists);

void
gal_fits_key_write_in_ptr(gal_fits_list_key_t *keylist,
                          fitsfile *fptr, int freekeys);

gal_list_str_t *
gal_fits_with_keyvalue(gal_list_str_t *files, char *hdu, char *name,
                       gal_list_str_t *values, char *hdu_option_name);

gal_list_str_t *
gal_fits_unique_keyvalues(gal_list_str_t *files, char *hdu, char *name,
                          char *hdu_option_name);





/*************************************************************
 ******************     Array functions      *****************
 *************************************************************/
void
gal_fits_img_info(fitsfile *fptr, int *type, size_t *ndim, size_t **dsize,
                  char **name, char **unit);

size_t *
gal_fits_img_info_dim(char *filename, char *hdu, size_t *ndim,
                      char *hdu_option_name);

gal_data_t *
gal_fits_img_read(char *filename, char *hdu, size_t minmapsize,
                  int quietmmap, char *hdu_option_name);

gal_data_t *
gal_fits_img_read_to_type(char *inputname, char *hdu, uint8_t type,
                          size_t minmapsize, int quietmmap,
                          char *hdu_option_name);

gal_data_t *
gal_fits_img_read_kernel(char *filename, char *hdu, size_t minmapsize,
                         int quietmmap, char *hdu_option_name);

fitsfile *
gal_fits_img_write_to_ptr(gal_data_t *data, char *filename,
                          gal_fits_list_key_t *keylist, int freekeys);

void
gal_fits_img_write(gal_data_t *data, char *filename,
                   gal_fits_list_key_t *keylist, int freekeys);

void
gal_fits_img_write_to_type(gal_data_t *data, char *filename,
                           gal_fits_list_key_t *keylist, int type,
                           int freekeys);

void
gal_fits_img_write_corr_wcs_str(gal_data_t *input, char *filename,
                                char *wcsheader, int nkeyrec, double *crpix,
                                gal_fits_list_key_t *keylist, int freekeys);





/**************************************************************/
/**********                  Table                 ************/
/**************************************************************/
void
gal_fits_tab_size(fitsfile *fitsptr, size_t *nrows, size_t *ncols);

int
gal_fits_tab_format(fitsfile *fptr);

gal_data_t *
gal_fits_tab_info(char *filename, char *hdu, size_t *numcols,
                  size_t *numrows, int *tableformat,
                  char *hdu_option_name);

gal_data_t *
gal_fits_tab_read(char *filename, char *hdu, size_t numrows,
                  gal_data_t *allcols, gal_list_sizet_t *indexll,
                  size_t numthreads, size_t minmapsize, int quietmmap,
                  char *hdu_option_name);

void
gal_fits_tab_write(gal_data_t *cols, gal_list_str_t *comments,
                   int tableformat, char *filename, char *extname,
                   struct gal_fits_list_key_t *keywords, int freekeys);



__END_C_DECLS    /* From C++ preparations */

#endif           /* __GAL_FITS_H__ */
