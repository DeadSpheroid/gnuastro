/*********************************************************************
MakeCatalog - Make a catalog from an input and labeled image.
MakeCatalog is part of GNU Astronomy Utilities (Gnuastro) package.

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
#include <config.h>

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <gnuastro/git.h>
#include <gnuastro/wcs.h>
#include <gnuastro/data.h>
#include <gnuastro/fits.h>
#include <gnuastro/units.h>
#include <gnuastro/threads.h>
#include <gnuastro/pointer.h>
#include <gnuastro/dimension.h>
#include <gnuastro/statistics.h>
#include <gnuastro/permutation.h>

#include <gnuastro-internal/timing.h>
#include <gnuastro-internal/checkset.h>

#include "main.h"
#include "mkcatalog.h"

#include "ui.h"
#include "parse.h"
#include "columns.h"
#include "upperlimit.h"










/*********************************************************************/
/**************       Manage a single object       *******************/
/*********************************************************************/
static void
mkcatalog_clump_starting_index(struct mkcatalog_passparams *pp)
{
  struct mkcatalogparams *p=pp->p;

  /* Lock the mutex if we are working on more than one thread. NOTE: it is
     very important to keep the number of operations within the mutex to a
     minimum so other threads don't get delayed. */
  if(p->cp.numthreads>1)
    pthread_mutex_lock(&p->mutex);

  /* Put the current total number of rows filled into the output, then
     increment the total number by the number of clumps. */
  pp->clumpstartindex = p->clumprowsfilled;
  p->clumprowsfilled += pp->clumpsinobj;

  /* Unlock the mutex (if it was locked). */
  if(p->cp.numthreads>1)
    pthread_mutex_unlock(&p->mutex);
}




/* Vector allocation (short name is used since it is repeated a lot). */
static void
mkcatalog_vec_alloc(struct mkcatalog_passparams *pp, size_t onum,
                    size_t vnum, uint8_t type)
{
  if( pp->p->oiflag[ onum ] )
    gal_data_initialize(&pp->vector[vnum], 0, type, 1,
                        &(pp->p->objects->dsize[0]), NULL, 1,
                        pp->p->cp.minmapsize, pp->p->cp.quietmmap,
                        NULL, NULL, NULL);
}





/* Allocate all the necessary space. */
static void
mkcatalog_single_object_init(struct mkcatalogparams *p,
                             struct mkcatalog_passparams *pp)
{
  uint8_t *oif=p->oiflag;
  size_t ndim=p->objects->ndim;
  uint8_t i32=GAL_TYPE_INT32, f64=GAL_TYPE_FLOAT64; /* For short lines.*/

  /* Initialize the mkcatalog_passparams elements. */
  pp->p               = p;
  pp->clumpstartindex = 0;
  pp->rng             = p->rng ? gsl_rng_clone(p->rng) : NULL;
  pp->oi              = gal_pointer_allocate(GAL_TYPE_FLOAT64,
                                             OCOL_NUMCOLS, 0, __func__,
                                             "pp->oi");

  /* If we have second order measurements, allocate the array keeping the
     temporary shift values for each object of this thread. Note that the
     clumps catalog (if requested), will have the same measurements, so its
     just enough to check the objects. */
  pp->shift = ( (    oif[ OCOL_GXX ]
                  || oif[ OCOL_GYY ]
                  || oif[ OCOL_GXY ]
                  || oif[ OCOL_VXX ]
                  || oif[ OCOL_VYY ]
                  || oif[ OCOL_VXY ] )
                ? gal_pointer_allocate(GAL_TYPE_SIZE_T, ndim, 0, __func__,
                                       "pp->shift")
                : NULL );

  /* If we have upper-limit mode, then allocate the container to keep the
     values to calculate the standard deviation. */
  if(p->upperlimit)
    {
      /* Allocate the space to keep the upper-limit values. */
      pp->up_vals = gal_data_alloc(NULL, GAL_TYPE_FLOAT32, 1, &p->upnum,
                                  NULL, 0, p->cp.minmapsize,
                                  p->cp.quietmmap, NULL, NULL, NULL);

      /* Set the blank checked flag to 1. By definition, this dataset won't
         have any blank values. Also 'flag' is initialized to '0'. So we
         just have to set the checked flag ('GAL_DATA_FLAG_BLANK_CH') to
         one to inform later steps that there are no blank values. */
      pp->up_vals->flag |= GAL_DATA_FLAG_BLANK_CH;
    }
  else
    pp->up_vals=NULL;

  /* If any vector measurements are necessary, do the necessary
     allocations: first the general array, to keep all that are necessary,
     then the individual ones. */
  pp->vector = ( (    oif[ OCOL_NUMINSLICE         ]
                   || oif[ OCOL_SUMINSLICE         ]
                   || oif[ OCOL_NUMALLINSLICE      ]
                   || oif[ OCOL_SUMVARINSLICE      ]
                   || oif[ OCOL_SUMPROJINSLICE     ]
                   || oif[ OCOL_NUMPROJINSLICE     ]
                   || oif[ OCOL_SUMPROJVARINSLICE  ]
                   || oif[ OCOL_NUMOTHERINSLICE    ]
                   || oif[ OCOL_SUMOTHERINSLICE    ]
                   || oif[ OCOL_SUMOTHERVARINSLICE ]
                   || oif[ OCOL_NUMALLOTHERINSLICE ] )
                 ? gal_data_array_calloc(VEC_NUM)
                 : NULL );
  mkcatalog_vec_alloc(pp, OCOL_NUMINSLICE,      VEC_NUMINSLICE,     i32);
  mkcatalog_vec_alloc(pp, OCOL_NUMALLINSLICE,   VEC_NUMALLINSLICE,  i32);
  mkcatalog_vec_alloc(pp, OCOL_NUMPROJINSLICE,  VEC_NUMPROJINSLICE, i32);
  mkcatalog_vec_alloc(pp, OCOL_NUMOTHERINSLICE, VEC_NUMOTHERINSLICE,i32);
  mkcatalog_vec_alloc(pp, OCOL_SUMINSLICE,      VEC_SUMINSLICE,     f64);
  mkcatalog_vec_alloc(pp, OCOL_SUMVARINSLICE,   VEC_SUMVARINSLICE,  f64);
  mkcatalog_vec_alloc(pp, OCOL_SUMPROJINSLICE,  VEC_SUMPROJINSLICE, f64);
  mkcatalog_vec_alloc(pp, OCOL_SUMOTHERINSLICE, VEC_SUMOTHERINSLICE,f64);
  mkcatalog_vec_alloc(pp, OCOL_SUMOTHERVARINSLICE, VEC_SUMOTHERVARINSLICE,
                      f64);
  mkcatalog_vec_alloc(pp, OCOL_NUMALLOTHERINSLICE, VEC_NUMALLOTHERINSLICE,
                      i32);
  mkcatalog_vec_alloc(pp, OCOL_SUMPROJVARINSLICE, VEC_SUMPROJVARINSLICE,
                      f64);
}





/* Each thread will call this function once. It will go over all the
   objects that are assigned to it. */
static void *
mkcatalog_single_object(void *in_prm)
{
  struct gal_threads_params *tprm=(struct gal_threads_params *)in_prm;
  struct mkcatalogparams *p=(struct mkcatalogparams *)(tprm->params);

  size_t i;
  struct mkcatalog_passparams pp;

  /* Initialize and allocate all the necessary values. */
  mkcatalog_single_object_init(p, &pp);

  /* Fill the desired columns for all the objects given to this thread. */
  for(i=0; tprm->indexs[i]!=GAL_BLANK_SIZE_T; ++i)
    {
      /* For easy reading. Note that the object IDs start from one while
         the array positions start from 0. */
      pp.ci       = NULL;
      pp.object   = ( p->outlabs
                      ? p->outlabs[ tprm->indexs[i] ]
                      : tprm->indexs[i] + 1 );
      pp.tile     = &p->tiles[   tprm->indexs[i] ];

      /* Initialize the parameters for this object/tile. */
      parse_initialize(&pp);

      /* Get the first pass information. */
      parse_objects(&pp);

      /* Currently the second pass is only necessary when there is a clumps
         image. */
      if(p->clumps)
        {
          /* Allocate space for the properties of each clump. */
          pp.ci = gal_pointer_allocate(GAL_TYPE_FLOAT64,
                                       pp.clumpsinobj * CCOL_NUMCOLS, 1,
                                       __func__, "pp.ci");

          /* Get the starting row of this object's clumps in the final
             catalog. This index is also necessary for the unique random
             number generator seeds of each clump. */
          mkcatalog_clump_starting_index(&pp);

          /* Get the second pass information. */
          parse_clumps(&pp);
        }

      /* If an order-based calculation is requested, another pass is
         necessary. */
      if(    p->oiflag[ OCOL_MEDIAN        ]
          || p->oiflag[ OCOL_MAXIMUM       ]
          || p->oiflag[ OCOL_HALFMAXSUM    ]
          || p->oiflag[ OCOL_HALFMAXNUM    ]
          || p->oiflag[ OCOL_HALFSUMNUM    ]
          || p->oiflag[ OCOL_SIGCLIPNUM    ]
          || p->oiflag[ OCOL_SIGCLIPSTD    ]
          || p->oiflag[ OCOL_SIGCLIPMEAN   ]
          || p->oiflag[ OCOL_FRACMAX1NUM   ]
          || p->oiflag[ OCOL_FRACMAX2NUM   ]
          || p->oiflag[ OCOL_SIGCLIPMEDIAN ])
        parse_order_based(&pp);

      /* Calculate the upper limit magnitude (if necessary). */
      if(p->upperlimit) upperlimit_calculate(&pp);

      /* Write the pass information into the columns. */
      columns_fill(&pp);

      /* Clean up for this object. */
      if(pp.ci) free(pp.ci);
    }

  /* Clean up. */
  free(pp.oi);
  free(pp.shift);
  gal_data_free(pp.up_vals);
  if(pp.rng) gsl_rng_free(pp.rng);
  gal_data_array_free(pp.vector, VEC_NUM, 1);

  /* Wait until all the threads finish and return. */
  if(tprm->b) pthread_barrier_wait(tprm->b);
  return NULL;
}




















/*********************************************************************/
/********         Processing after threads finish        *************/
/*********************************************************************/
/* Convert internal image coordinates to WCS for table.

   Note that from the beginning (during the passing steps), we saved FITS
   coordinates. Also note that we are doing the conversion in place. */
static void
mkcatalog_wcs_conversion(struct mkcatalogparams *p)
{
  gal_data_t *c;
  gal_data_t *column;

  /* Flux weighted center positions for clumps and objects. */
  if(p->wcs_vo)
    {
      gal_wcs_img_to_world(p->wcs_vo, p->objects->wcs, 1);
      if(p->wcs_vc)
        gal_wcs_img_to_world(p->wcs_vc, p->objects->wcs, 1);
    }


  /* Geometric center positions for clumps and objects. */
  if(p->wcs_go)
    {
      gal_wcs_img_to_world(p->wcs_go, p->objects->wcs, 1);
      if(p->wcs_gc)
        gal_wcs_img_to_world(p->wcs_gc, p->objects->wcs, 1);
    }


  /* All clumps flux weighted center. */
  if(p->wcs_vcc)
    gal_wcs_img_to_world(p->wcs_vcc, p->objects->wcs, 1);


  /* All clumps geometric center. */
  if(p->wcs_gcc)
    gal_wcs_img_to_world(p->wcs_gcc, p->objects->wcs, 1);


  /* Go over all the object columns and fill in the values. */
  for(column=p->objectcols; column!=NULL; column=column->next)
    {
      /* Definitions */
      c=NULL;

      /* Set 'c' for the columns that must be corrected. Note that this
         'switch' statement doesn't need any 'default', because there are
         probably columns that don't need any correction. */
      switch(column->status)
        {
        case UI_KEY_W1:           c=p->wcs_vo;                break;
        case UI_KEY_W2:           c=p->wcs_vo->next;          break;
        case UI_KEY_W3:           c=p->wcs_vo->next->next;    break;
        case UI_KEY_GEOW1:        c=p->wcs_go;                break;
        case UI_KEY_GEOW2:        c=p->wcs_go->next;          break;
        case UI_KEY_GEOW3:        c=p->wcs_go->next->next;    break;
        case UI_KEY_CLUMPSW1:     c=p->wcs_vcc;               break;
        case UI_KEY_CLUMPSW2:     c=p->wcs_vcc->next;         break;
        case UI_KEY_CLUMPSW3:     c=p->wcs_vcc->next->next;   break;
        case UI_KEY_CLUMPSGEOW1:  c=p->wcs_gcc;               break;
        case UI_KEY_CLUMPSGEOW2:  c=p->wcs_gcc->next;         break;
        case UI_KEY_CLUMPSGEOW3:  c=p->wcs_gcc->next->next;   break;
        }

      /* Copy the elements into the output column. */
      if(c)
        memcpy(column->array, c->array,
               column->size*gal_type_sizeof(c->type));
    }


  /* Go over all the clump columns and fill in the values. */
  for(column=p->clumpcols; column!=NULL; column=column->next)
    {
      /* Definitions */
      c=NULL;

      /* Set 'c' for the columns that must be corrected. Note that this
         'switch' statement doesn't need any 'default', because there are
         probably columns that don't need any correction. */
      switch(column->status)
        {
        case UI_KEY_W1:           c=p->wcs_vc;                break;
        case UI_KEY_W2:           c=p->wcs_vc->next;          break;
        case UI_KEY_W3:           c=p->wcs_vc->next->next;    break;
        case UI_KEY_GEOW1:        c=p->wcs_gc;                break;
        case UI_KEY_GEOW2:        c=p->wcs_gc->next;          break;
        case UI_KEY_GEOW3:        c=p->wcs_gc->next->next;    break;
        }

      /* Copy the elements into the output column. */
      if(c)
        memcpy(column->array, c->array,
               column->size*gal_type_sizeof(c->type));
    }
}





void
mkcatalog_outputs_keys_numeric(gal_fits_list_key_t **keylist, void *number,
                               uint8_t type, char *nameliteral,
                               char *commentliteral, char *unitliteral)
{
  void *value;
  value=gal_pointer_allocate(type, 1, 0, __func__, "value");
  memcpy(value, number, gal_type_sizeof(type));
  gal_fits_key_list_add_end(keylist, type, nameliteral, 0,
                            value, 1, commentliteral, 0,
                            unitliteral, 0);
}





void
mkcatalog_outputs_keys_infiles(struct mkcatalogparams *p,
                               gal_fits_list_key_t **keylist)
{
  int quiet=p->cp.quiet;
  char *stdname, *stdhdu, *stdvalcom;

  gal_fits_key_list_title_add_end(keylist,
                                  "Input files and/or configuration", 0);

  /* Object labels. */
  gal_fits_key_write_filename("INLAB", p->objectsfile, keylist, 0,
                              quiet);
  gal_fits_key_write_filename("INLABHDU", p->cp.hdu, keylist, 0,
                              quiet);

  /* Clump labels. */
  if(p->clumps)
    {
      gal_fits_key_write_filename("INCLU", p->usedclumpsfile, keylist, 0,
                                  quiet);
      gal_fits_key_write_filename("INCLUHDU", p->clumpshdu, keylist, 0,
                                  quiet);
    }

  /* Values image. */
  if(p->values)
    {
      gal_fits_key_write_filename("INVAL", p->usedvaluesfile, keylist, 0,
                                  quiet);
      gal_fits_key_write_filename("INVALHDU", p->valueshdu, keylist, 0,
                                  quiet);
    }

  /* Sky image/value. */
  if(p->sky)
    {
      if(p->sky->size==1)
        mkcatalog_outputs_keys_numeric(keylist, p->sky->array,
                                       p->sky->type, "INSKYVAL",
                                       "Value of Sky used (a single "
                                       "number).", NULL);
      else
        {
          gal_fits_key_write_filename("INSKY", p->usedskyfile, keylist, 0,
                                      quiet);
          gal_fits_key_write_filename("INSKYHDU", p->skyhdu, keylist, 0,
                                      quiet);
        }
    }

  /* Standard deviation (or variance) image. */
  if(p->variance)
    {
      stdname="INVAR"; stdhdu="INVARHDU";
      stdvalcom="Value of Sky variance (a single number).";
    }
  else
    {
      stdname="INSTD"; stdhdu="INSTDHDU";
      stdvalcom="Value of Sky STD (a single number).";
    }
  if(p->std)
    {
      if(p->std->size==1)
        mkcatalog_outputs_keys_numeric(keylist, p->std->array, p->std->type,
                                       stdname, stdvalcom, NULL);
      else
        {
          gal_fits_key_write_filename(stdname, p->usedstdfile, keylist, 0,
                                      quiet);
          gal_fits_key_write_filename(stdhdu, p->stdhdu, keylist, 0,
                                      quiet);
        }
    }

  /* Upper limit mask. */
  if(p->upmaskfile)
    {
      gal_fits_key_write_filename("INUPM", p->upmaskfile, keylist, 0,
                                  quiet);
      gal_fits_key_write_filename("INUPMHDU", p->upmaskhdu, keylist, 0,
                                  quiet);
    }
}





/* Write the output keywords. */
static gal_fits_list_key_t *
mkcatalog_outputs_keys(struct mkcatalogparams *p, int o0c1)
{
  float pixarea=NAN, fvalue;
  gal_fits_list_key_t *keylist=NULL;

  /* First, add the file names. */
  mkcatalog_outputs_keys_infiles(p, &keylist);

  /* Type of catalog. */
  gal_fits_key_list_add_end(&keylist, GAL_TYPE_STRING, "CATTYPE", 0,
                            o0c1 ? "clumps" : "objects", 0,
                            "Type of catalog ('object' or 'clump').", 0,
                            NULL, 0);

  /* Add project commit information when in a Git-controlled directory and
     the output isn't a FITS file (in FITS, this will be automatically
     included). */
  if(p->cp.tableformat==GAL_TABLE_FORMAT_TXT && gal_git_describe())
    gal_fits_key_list_add_end(&keylist, GAL_TYPE_STRING, "COMMIT", 0,
                              gal_git_describe(), 1,
                              "Git commit in running directory.", 0,
                              NULL, 0);

  /* Pixel area. */
  if(p->objects->wcs)
    {
      pixarea=gal_wcs_pixel_area_arcsec2(p->objects->wcs);
      if( isnan(pixarea)==0 )
        mkcatalog_outputs_keys_numeric(&keylist, &pixarea,
                                       GAL_TYPE_FLOAT32, "PIXAREA",
                                       "Pixel area of input image.",
                                       "arcsec^2");
    }

  /* Zeropoint magnitude. */
  if( !isnan(p->zeropoint) )
    mkcatalog_outputs_keys_numeric(&keylist, &p->zeropoint,
                                   GAL_TYPE_FLOAT32, "ZEROPNT",
                                   "Zeropoint used for magnitude.",
                                   "mag");

  /* Add the title for the keywords. */
  gal_fits_key_list_title_add_end(&keylist, "Surface brightness limit "
                                  "(SBL)", 0);

  /* Print surface brightness limit. */
  if( !isnan(p->medstd) && !isnan(p->sfmagnsigma) )
    {
      /* Used noise value (per pixel) and multiple of sigma. */
      mkcatalog_outputs_keys_numeric(&keylist, &p->medstd,
                                     GAL_TYPE_FLOAT32, "SBLSTD",
                                     "Pixel STD for surface brightness "
                                     "limit.", NULL);
      mkcatalog_outputs_keys_numeric(&keylist, &p->sfmagnsigma,
                                     GAL_TYPE_FLOAT32, "SBLNSIG",
                                     "Sigma multiple for surface "
                                     "brightness limit.", NULL);

      /* Only print magnitudes if a zeropoint is given. */
      if( !isnan(p->zeropoint) )
        {
          /* Per pixel, Surface brightness limit magnitude. */
          fvalue=gal_units_counts_to_mag(p->sfmagnsigma * p->medstd,
                                         p->zeropoint);
          mkcatalog_outputs_keys_numeric(&keylist, &fvalue,
                                         GAL_TYPE_FLOAT32, "SBLMAGPX",
                                         "Surface brightness limit per "
                                         "pixel.", "mag/pix");

          /* Only print the SBL in fixed area if a WCS is present and a
             pixel area could be deduced. */
          if( !isnan(pixarea) )
            {
              /* Area used for measuring SBL. */
              mkcatalog_outputs_keys_numeric(&keylist, &p->sfmagarea,
                                             GAL_TYPE_FLOAT32, "SBLAREA",
                                             "Area for surface brightness "
                                             "limit.", "arcsec^2");

              /* Per area, Surface brightness limit magnitude. */
              fvalue=gal_units_counts_to_mag(p->sfmagnsigma
                                             * p->medstd
                                             / sqrt( p->sfmagarea
                                                     * pixarea),
                                             p->zeropoint);
              mkcatalog_outputs_keys_numeric(&keylist, &fvalue,
                                             GAL_TYPE_FLOAT32, "SBLMAG",
                                             "Surf. bright. limit in "
                                             "SBLAREA.",
                                             "mag/arcsec^2");
            }
          else
            gal_fits_key_list_fullcomment_add_end(&keylist, "Can't "
                   "write surface brightness limiting magnitude (SBLM) "
                   "values in fixed area ('SBLAREA' and 'SBLMAG' "
                   "keywords) because input doesn't have a world "
                   "coordinate system (WCS), or the first two "
                   "coordinates of the WCS weren't angular positions "
                   "in units of degrees.", 0);
        }
      else
        gal_fits_key_list_fullcomment_add_end(&keylist, "Can't write "
               "surface brightness limiting magnitude values (e.g., "
               "'SBLMAG' or 'SBLMAGPX' keywords) because no "
               "'--zeropoint' has been given.", 0);
    }
  else
    {
      gal_fits_key_list_fullcomment_add_end(&keylist, "No surface "
             "brightness calcuations (e.g., 'SBLMAG' or 'SBLMAGPX' "
             "keywords) because STD image didn't have the 'MEDSTD' "
             "keyword. There are two solutions: 1) Call with "
             "'--forcereadstd'. 2) Measure the median noise level "
             "manually (possibly with Gnuastro's Arithmetic program) "
             "and put the value in the 'MEDSTD' keyword of the STD "
             "image.", 0);
      gal_fits_key_list_fullcomment_add_end(&keylist, "", 0);
    }

  /* The count-per-second correction. */
  if(p->cpscorr>1.0f)
    mkcatalog_outputs_keys_numeric(&keylist, &p->cpscorr,
                                   GAL_TYPE_FLOAT32, "CPSCORR",
                                   "Counts-per-second correction.",
                                   NULL);

  /* Print upper-limit parameters. */
  if(p->upperlimit)
    upperlimit_write_keys(p, &keylist, 1);

  /* In plain-text outputs, put a title for column metadata. */
  if(p->cp.tableformat==GAL_TABLE_FORMAT_TXT)
    gal_fits_key_list_title_add_end(&keylist, "Column metadata", 0);

  /* Return the list of keywords. */
  return keylist;
}





/* Since all the measurements were done in parallel (and we didn't know the
   number of clumps per object a-priori), the clumps informtion is just
   written in as they are measured. Here, we'll sort the clump columns by
   object ID. There is an option to disable this. */
static void
sort_clumps_by_objid(struct mkcatalogparams *p)
{
  gal_data_t *col;
  size_t o, i, j, *permute, *rowstart;

  /* Make sure everything is fine. */
  if(p->hostobjid_c==NULL || p->numclumps_c==NULL)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
          "problem. 'p->hostobjid_c' and 'p->numclumps_c' must not be "
          "NULL.", __func__, PACKAGE_BUGREPORT);


  /* Allocate the necessary arrays. */
  rowstart=gal_pointer_allocate(GAL_TYPE_SIZE_T, p->numobjects, 0, __func__,
                                 "rowstart");
  permute=gal_pointer_allocate(GAL_TYPE_SIZE_T, p->numclumps, 0, __func__,
                                "permute");


  /* The objects array is already sorted by object ID. So we should just
     add up the number of clumps to find the row where each object's clumps
     should start from in the final sorted clumps catalog. */
  rowstart[0] = 0;
  for(i=1;i<p->numobjects;++i)
    rowstart[i] = p->numclumps_c[i-1] + rowstart[i-1];

  /* Fill the permutation array. Note that WE KNOW that all the objects for
     one clump are after each other.*/
  i=0;
  while(i<p->numclumps)
    {
      o = ( p->outlabsinv
            ? (p->outlabsinv[ p->hostobjid_c[i] ] + 1)
            : p->hostobjid_c[i] ) - 1;
      for(j=0; j<p->numclumps_c[o]; ++j)
        permute[i++] = rowstart[o] + j;
    }

  /* Permute all the clump columns. */
  for(col=p->clumpcols; col!=NULL; col=col->next)
    gal_permutation_apply_inverse(col, permute);

  /* Clean up */
  free(permute);
  free(rowstart);
}





/* Write the produced columns into the output */
static void
mkcatalog_write_outputs(struct mkcatalogparams *p)
{
  gal_fits_list_key_t *keylist;
  gal_list_str_t *comments=NULL;
  int outisfits=gal_fits_name_is_fits(p->objectsout);

  /* If a catalog is to be generated. */
  if(p->objectcols)
    {
      /* OBJECT catalog */
      keylist=mkcatalog_outputs_keys(p, 0);

      /* Reverse the comments list (so it is printed in the same order
         here), write the objects catalog and free the comments. */
      gal_list_str_reverse(&comments);
      gal_table_write(p->objectcols, &keylist, NULL, p->cp.tableformat,
                      p->objectsout, "OBJECTS", 0);
      gal_list_str_free(comments, 1);


      /* CLUMPS catalog */
      if(p->clumps)
        {
          /* Make the comments. */
          keylist=mkcatalog_outputs_keys(p, 1);

          /* Write objects catalog
             ---------------------

             Reverse the comments list (so it is printed in the same order
             here), write the objects catalog and free the comments. */
          gal_list_str_reverse(&comments);
          gal_table_write(p->clumpcols, NULL, comments, p->cp.tableformat,
                          p->clumpsout, "CLUMPS", 0);
          gal_list_str_free(comments, 1);
        }
    }

  /* Configuration information. */
  if(outisfits)
    {
      gal_fits_key_write_filename("input", p->objectsfile, &p->cp.okeys,
                                  1, p->cp.quiet);
      gal_fits_key_write_config(&p->cp.okeys, "MakeCatalog configuration",
                                "MKCATALOG-CONFIG", p->objectsout, "0");
    }


  /* Inform the user */
  if(!p->cp.quiet && p->objectcols)
    {
      if(p->clumpsout && strcmp(p->clumpsout,p->objectsout))
        {
          printf("  - Output objects catalog: %s\n", p->objectsout);
          if(p->clumps)
            printf("  - Output clumps catalog: %s\n", p->clumpsout);
        }
      else
        printf("  - Catalog written to %s\n", p->objectsout);
    }
}




















/*********************************************************************/
/*****************       Top-level function        *******************/
/*********************************************************************/
void
mkcatalog(struct mkcatalogparams *p)
{
  /* When more than one thread is to be used, initialize the mutex: we need
     it to assign a column to the clumps in the final catalog. */
  if( p->cp.numthreads > 1 ) pthread_mutex_init(&p->mutex, NULL);

  /* Do the processing on each thread. */
  gal_threads_spin_off(mkcatalog_single_object, p, p->numobjects,
                       p->cp.numthreads, p->cp.minmapsize,
                       p->cp.quietmmap);

  /* Post-thread processing, for example to convert image coordinates to RA
     and Dec. */
  mkcatalog_wcs_conversion(p);

  /* If the columns need to be sorted (by object ID), then some adjustments
     need to be made (possibly to both the objects and clumps catalogs). */
  if(p->hostobjid_c)
    sort_clumps_by_objid(p);

  /* Write the filled columns into the output. */
  mkcatalog_write_outputs(p);

  /* Destroy the mutex. */
  if( p->cp.numthreads>1 ) pthread_mutex_destroy(&p->mutex);
}
