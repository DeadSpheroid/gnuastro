/*********************************************************************
MakeCatalog - Make a catalog from an input and labeled image.
MakeCatalog is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#include <gnuastro/data.h>
#include <gnuastro/pointer.h>
#include <gnuastro/dimension.h>
#include <gnuastro/statistics.h>

#include "main.h"
#include "mkcatalog.h"

#include "parse.h"









/* Both passes are going to need their starting pointers set, so we'll do
   that here. */
void
parse_initialize(struct mkcatalog_passparams *pp)
{
  struct mkcatalogparams *p=pp->p;

  gal_data_t *vec;
  size_t i, ndim=p->objects->ndim;
  size_t *start_end=pp->start_end_inc;

  /* Initialize the number of clumps in this object. */
  pp->clumpsinobj=0;

  /* Initialize the intermediate values to zero. */
  memset(pp->oi, 0, OCOL_NUMCOLS * sizeof *pp->oi);
  if(pp->vector)
    for(i=0;i<VEC_NUM;++i)
      {
        vec=&(pp->vector[i]);
        if(pp->vector[i].array)
          memset(vec->array, 0, vec->size*gal_type_sizeof(vec->type));
      }

  /* Set the shifts in every dimension to avoid round-off errors in large
     numbers for the non-linear calculations. We are using the first pixel
     of each object's tile as the shift parameter to keep the mean
     (average) reasonably near to the standard deviation. Otherwise, when
     the object is far out in the image (large x and y positions), then
     roundoff errors are going to decrease the accuracy of the second order
     calculations. */
  if(pp->shift)
    {
      /* Get the coordinates of the tile's starting point. */
      gal_dimension_index_to_coord(( (float *)(pp->tile->array)
                                     - (float *)(pp->tile->block->array) ),
                                   ndim, p->objects->dsize, pp->shift);

      /* Change their counting to start from 1, not zero, since we will be
         using them as FITS coordinates. */
      for(i=0;i<ndim;++i) ++pp->shift[i];
    }


  /* Set the starting and ending indexs of this tile/object on all (the
     possible) input arrays. */
  pp->st_o   = gal_tile_start_end_ind_inclusive(pp->tile, p->objects,
                                                start_end);
  pp->st_c   = (p->clumps
                ? (int32_t *)(p->clumps->array) + start_end[0] : NULL);
  pp->st_v   = (p->values
                ? (float *)(p->values->array)   + start_end[0] : NULL);
  pp->st_sky = ( p->sky
                 ? ( p->sky->size==p->objects->size
                     ? (float *)(p->sky->array) + start_end[0]
                     : NULL )
                 : NULL);
  pp->st_std = ( p->std
                 ? ( p->std->size==p->objects->size
                     ? (float *)(p->std->array) + start_end[0]
                     : NULL )
                 : NULL );
}





static size_t *
parse_vector_dim3_prepare(struct mkcatalog_passparams *pp,
                          size_t *start_end_inc, int32_t **st_o,
                          float **st_v, float **st_std)
{
  size_t *tsize;
  gal_data_t *spectile;
  struct mkcatalogparams *p=pp->p;
  size_t coord[3], minmax[6];

  /* Get the coordinates of the spectral tile's starting element, then make
     the tile. */
  gal_dimension_index_to_coord(gal_pointer_num_between(p->objects->array,
                                                       pp->tile->array,
                                                       p->objects->type),
                               p->objects->ndim, p->objects->dsize, coord);
  minmax[0]=0;                             /* Changed to first slice.*/
  minmax[1]=coord[1];
  minmax[2]=coord[2];
  minmax[3]=p->objects->dsize[0]-1;        /* Changed to last slice. */
  minmax[4]=coord[1]+pp->tile->dsize[1]-1;
  minmax[5]=coord[2]+pp->tile->dsize[2]-1;
  spectile=gal_tile_series_from_minmax(p->objects, minmax, 1);

  /* Find the starting (and ending) pointers on each of the datasets. */
  *st_o   = gal_tile_start_end_ind_inclusive(spectile, p->objects,
                                             start_end_inc);
  *st_v   = (float *)(p->values->array) + start_end_inc[0];
  *st_std = ( p->std
                 ? ( p->std->size==p->objects->size
                     ? (float *)(p->std->array) + start_end_inc[0]
                     : NULL )
                 : NULL );

  /* Clean up and return. */
  tsize=spectile->dsize;
  spectile->dsize=NULL;
  gal_data_free(spectile);
  return tsize;
}





static void
parse_vector_dim3(struct mkcatalog_passparams *pp, gal_data_t *xybin)
{
  struct mkcatalogparams *p=pp->p;

  double var;
  int needsvar;
  gal_data_t *vector=pp->vector;
  uint8_t vine=!p->novalinerror;
  float *std=p->std?p->std->array:NULL;
  size_t c[3], *dsize=p->objects->dsize;
  size_t sind=0, pind=0, num_increment=1;
  float sval, *st_v, *st_std, *V=NULL, *ST=NULL;
  uint8_t *xybinarr = xybin ? xybin->array : NULL;
  int32_t *st_o, *O, *OO, *objarr=p->objects->array;
  size_t tid, *tsize, increment=0, start_end_inc[2], ndim=p->objects->ndim;

  /* Pointers to necessary temporary arrays (they will be NULL if they are
     not necessary for the user). */
  double  *suminslice         = vector[ VEC_SUMINSLICE         ].array;
  double  *sumvarinslice      = vector[ VEC_SUMVARINSLICE      ].array;
  double  *sumprojinslice     = vector[ VEC_SUMPROJINSLICE     ].array;
  double  *sumprojvarinslice  = vector[ VEC_SUMPROJVARINSLICE  ].array;
  double  *sumotherinslice    = vector[ VEC_SUMOTHERINSLICE    ].array;
  double  *sumothervarinslice = vector[ VEC_SUMOTHERVARINSLICE ].array;
  int32_t *numinslice         = vector[ VEC_NUMINSLICE         ].array;
  int32_t *numallinslice      = vector[ VEC_NUMALLINSLICE      ].array;
  int32_t *numprojinslice     = vector[ VEC_NUMPROJINSLICE     ].array;
  int32_t *numotherinslice    = vector[ VEC_NUMOTHERINSLICE    ].array;
  int32_t *numallotherinslice = vector[ VEC_NUMALLOTHERINSLICE ].array;

  /* Prepare the parsing information. Also, if tile-id isn't necessary, set
     'tid' to a blank value to cause a crash with a mistake. */
  tsize=parse_vector_dim3_prepare(pp, start_end_inc, &st_o, &st_v,
                                  &st_std);
  tid = (p->std && p->std->size>1 && st_std == NULL)?0:GAL_BLANK_SIZE_T;

  /* Check if we need the variance. */
  needsvar = ( sumvarinslice || sumprojvarinslice || sumothervarinslice
               ? 1 : 0 );
  if(needsvar && p->std==NULL)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to fix "
          "the problem. The requested column requires a variance "
          "estimation, but the input standard deviation image is NULL",
          __func__, PACKAGE_BUGREPORT);

  /* Parse each contiguous patch of memory covered by this object. */
  while( start_end_inc[0] + increment <= start_end_inc[1] )
    {
      /* Set the contiguous range to parse. The pixel-to-pixel counting
         along the fastest dimension will be done over the 'O' pointer. */
      if( p->values        ) V  = st_v   + increment;
      if( p->std && st_std ) ST = st_std + increment;
      OO = ( O = st_o + increment ) + pp->tile->dsize[ndim-1];

      /* Parse the "tile" for this label. */
      do
        {
          /* Counters that don't depend on value. */
          if(numallinslice) ++numallinslice[sind];
          if(*O!=pp->object && numallotherinslice)
            ++numallotherinslice[sind];

          /* Only continue if this voxel is on a label and is useful (it
             isn't NaN). */
          if( !isnan(*V) )
            {
              /* Variance of this voxel (if necessary) */
              if(needsvar)
                {
                  /* If the standard deviation is given on a tile
                     structure, estimate the tile ID. */
                  if(tid != GAL_BLANK_SIZE_T)
                    {
                      gal_dimension_index_to_coord(O-objarr, ndim, dsize,
                                                   c);
                      tid=gal_tile_full_id_from_coord(&p->cp.tl, c);
                    }

                  /* Get the error associated with this voxel. Note that if
                     we are given a variance dataset already, there is no
                     need to use 'st*st', we can directly use 'sval'. */
                  sval = st_std ? *ST : (p->std->size>1?std[tid]:std[0]);
                  var = (p->variance ? sval : sval*sval) + (vine?*V:0);
                }
              else var = NAN;

              /* Only on this label. */
              if(*O==pp->object) /* We are on this object. */
                {
                  if(numinslice)  ++numinslice[sind];
                  if(suminslice)    suminslice[sind]    += *V;
                  if(sumvarinslice) sumvarinslice[sind] += var;
                }

              /* Projected measurements: see if we have a value of '2' in
                 the 'xybin' array (showing that there is atleast one
                 non-blank element there over the whole spectrum.  */
              if(xybin && xybinarr[pind]==2)
                {
                  /* Raw measurements over the projection. */
                  if(numprojinslice)  ++numprojinslice[sind];
                  if(sumprojinslice)    sumprojinslice[sind]    += *V;
                  if(sumprojvarinslice) sumprojvarinslice[sind] += var;

                  /* Other labels over this projection. */
                  if(*O!=pp->object)
                    {
                      if(numotherinslice) ++numotherinslice[sind];
                      if(sumotherinslice)   sumotherinslice[sind]   += *V;
                      if(sumothervarinslice)sumothervarinslice[sind]+=var;
                    }
                }
            }

          /* Values used, increment the pointrs for next voxel. */
          if( xybin            ) ++pind;
          if( p->values        ) ++V;
          if( p->std && st_std ) ++ST;
        }
      while(++O<OO);

      /* Increment to the next contiguous region of this tile. */
      increment += ( gal_tile_block_increment(p->objects, tsize,
                                              num_increment++, NULL) );

      /* If we have reached the end of one slice, increment the slice index
         ('sind'), and reset the projection (2D) index 'pind' if we have
         just finished parsing a slice. Also, set all the sum values
         that didn't have any measurement to NAN. */
      if( (num_increment-1)%pp->tile->dsize[1]==0 )
        {
          /* If there was no measurement, set NaN for the values and their
             errors (zero is meaningful). */
          if(numinslice && numinslice[sind]==0)
            suminslice[sind]=NAN;
          if(numprojinslice && numprojinslice[sind]==0)
            sumprojinslice[sind]=NAN;
          if(numotherinslice && numotherinslice[sind]==0)
            sumotherinslice[sind]=NAN;

          /* Set the projection-index to zero (since it counts on each
             slice), and increment the slice-index. */
          pind=0;
          ++sind;
        }
    }

  /* Clean up and return. */
  free(tsize);
}




void
parse_objects(struct mkcatalog_passparams *pp)
{
  uint8_t *oif=pp->p->oiflag;
  struct mkcatalogparams *p=pp->p;
  size_t ndim=p->objects->ndim, *dsize=p->objects->dsize;

  double *oi=pp->oi;
  gal_data_t *xybin=NULL;
  size_t *tsize=pp->tile->dsize;
  double minima_v=FLT_MAX, maxima_v=-FLT_MAX;
  size_t d, pind=0, increment=0, num_increment=1;
  int32_t *O, *OO, *C=NULL, *objarr=p->objects->array;
  float var, sval, skyval, *V=NULL, *SK=NULL, *ST=NULL;
  uint8_t *u, *uf, goodvalue, vine=!p->novalinerror, *xybinarr=NULL;
  float *std=p->std?p->std->array:NULL, *sky=p->sky?p->sky->array:NULL;

  /* If tile processing isn't necessary, set 'tid' to a blank value. */
  size_t tid = ( ( (p->sky     && p->sky->size>1 && pp->st_sky == NULL )
                   || ( p->std && p->std->size>1 && pp->st_std == NULL ) )
                 ? 0 : GAL_BLANK_SIZE_T );

  /* Coordinate shift. */
  size_t *sc = ( pp->shift
                 ? gal_pointer_allocate(GAL_TYPE_SIZE_T, ndim, 0, __func__,
                                        "sc")
                 : NULL );

  /* If any coordinate columns are requested. */
  size_t *c = (
               /* Coordinate-related columns. */
               ( oif[    OCOL_GX      ]
                 || oif[ OCOL_GY      ]
                 || oif[ OCOL_GZ      ]
                 || oif[ OCOL_VX      ]
                 || oif[ OCOL_VY      ]
                 || oif[ OCOL_VZ      ]
                 || oif[ OCOL_C_GX    ]
                 || oif[ OCOL_C_GY    ]
                 || oif[ OCOL_C_GZ    ]
                 || oif[ OCOL_MINVX   ]
                 || oif[ OCOL_MAXVX   ]
                 || oif[ OCOL_MINVY   ]
                 || oif[ OCOL_MAXVY   ]
                 || oif[ OCOL_MINVZ   ]
                 || oif[ OCOL_MAXVZ   ]
                 || oif[ OCOL_MINVNUM ]
                 || oif[ OCOL_MAXVNUM ]
                 || sc
                 /* When the sky and its STD are tiles, we'll also need
                    the coordinate to find which tile a pixel belongs
                    to. */
                 || tid==GAL_BLANK_SIZE_T )
               ? gal_pointer_allocate(GAL_TYPE_SIZE_T, ndim, 0, __func__,
                                      "c")
               : NULL );

  /* If any of the projection measurements are necessary, we need to
     allocate an array to keep the projected space. */
  if(    oif[ OCOL_NUMALLXY           ]
      || oif[ OCOL_NUMXY              ]
      || oif[ OCOL_SUMPROJINSLICE     ]
      || oif[ OCOL_NUMPROJINSLICE     ]
      || oif[ OCOL_SUMPROJVARINSLICE  ]
      || oif[ OCOL_NUMOTHERINSLICE    ]
      || oif[ OCOL_SUMOTHERINSLICE    ]
      || oif[ OCOL_SUMOTHERVARINSLICE ]
      || oif[ OCOL_NUMALLOTHERINSLICE ] )
    {
      xybin=gal_data_alloc(NULL, GAL_TYPE_UINT8, 2, &tsize[1], NULL,
                           1, p->cp.minmapsize, p->cp.quietmmap,
                           NULL, NULL, NULL);
      xybinarr=xybin->array;
    }

  /* Parse each contiguous patch of memory covered by this object. */
  while( pp->start_end_inc[0] + increment <= pp->start_end_inc[1] )
    {
      /* Set the contiguous range to parse. The pixel-to-pixel counting
         along the fastest dimension will be done over the 'O' pointer. */
      if( p->clumps            ) C  = pp->st_c   + increment;
      if( p->values            ) V  = pp->st_v   + increment;
      if( p->sky && pp->st_sky ) SK = pp->st_sky + increment;
      if( p->std && pp->st_std ) ST = pp->st_std + increment;
      OO = ( O = pp->st_o + increment ) + tsize[ndim-1];

      /* Parse the tile. */
      do
        {
          /* If this pixel belongs to the requested object then do the
             processing.  */
          if( *O==pp->object )
            {
              /* INTERNAL: Get the number of clumps in this object: it is
                 the largest clump ID over each object. */
              if( p->clumps && *C>0 )
                pp->clumpsinobj = *C > pp->clumpsinobj?*C:pp->clumpsinobj;


              /* Add to the area of this object. */
              if(xybin) xybinarr[ pind ]=1;
              if(oif[ OCOL_NUMALL   ]) oi[ OCOL_NUMALL ]++;


              /* Geometric coordinate measurements. */
              if(c)
                {
                  /* Convert the index to coordinate. */
                  gal_dimension_index_to_coord(O-objarr, ndim, dsize, c);

                  /* If we need tile-ID, get the tile ID now. */
                  if(tid!=GAL_BLANK_SIZE_T)
                    tid=gal_tile_full_id_from_coord(&p->cp.tl, c);

                  /* Do the general geometric (independent of pixel value)
                     calculations. */
                  if(oif[ OCOL_GX ]) oi[ OCOL_GX ] += c[ ndim-1 ]+1;
                  if(oif[ OCOL_GY ]) oi[ OCOL_GY ] += c[ ndim-2 ]+1;
                  if(oif[ OCOL_GZ ]) oi[ OCOL_GZ ] += c[ ndim-3 ]+1;
                  if(pp->shift)
                    {
                      /* Calculate the shifted coordinates for second order
                         calculations. The coordinate is incremented
                         because from now on, the positions are in the FITS
                         standard (starting from one).  */
                      for(d=0;d<ndim;++d) sc[d] = c[d] + 1 - pp->shift[d];

                      /* Include the shifted values, note that the second
                         order moments are never needed independently, they
                         are used together to find the ellipticity
                         parameters. */
                      oi[ OCOL_GXX ] += sc[1] * sc[1];
                      oi[ OCOL_GYY ] += sc[0] * sc[0];
                      oi[ OCOL_GXY ] += sc[1] * sc[0];
                    }
                  if(p->clumps && *C>0)
                    {
                      if(oif[ OCOL_C_NUMALL ]) oi[ OCOL_C_NUMALL ]++;
                      if(oif[ OCOL_C_GX ]) oi[ OCOL_C_GX ] += c[ndim-1]+1;
                      if(oif[ OCOL_C_GY ]) oi[ OCOL_C_GY ] += c[ndim-2]+1;
                      if(oif[ OCOL_C_GZ ]) oi[ OCOL_C_GZ ] += c[ndim-3]+1;
                    }
                }


              /* Value related measurements. */
              goodvalue=0;
              if( p->values && !( p->hasblank && isnan(*V) ) )
                {
                  /* For the standard-deviation measurements later. */
                  goodvalue=1;

                  /* General flux summations. */
                  if(xybin) xybinarr[ pind ]=2;
                  if(oif[ OCOL_NUM ])   oi[ OCOL_NUM   ]++;
                  if(oif[ OCOL_SUM ])   oi[ OCOL_SUM   ] += *V;
                  if(oif[ OCOL_SUMP2 ]) oi[ OCOL_SUMP2 ] += *V * *V;

                  /* Get the necessary clump information. */
                  if(p->clumps && *C>0)
                    {
                      if(oif[ OCOL_C_NUM ]) oi[ OCOL_C_NUM ]++;
                      if(oif[ OCOL_C_SUM ]) oi[ OCOL_C_SUM ] += *V;
                    }

                  /* Get the extrema of the values. Note that if the minima
                     or maxima value's coordinates are requested in any
                     dimension, then 'OCOL_MINVNUM' or 'OCOL_MAXVNUM' will
                     be activated). */
                  if( oif[ OCOL_MINVNUM ] && *V<=minima_v )
                    {
                      /* If the value is smaller than the smallest found so
                         far, reset the counter to one, and reset the sum
                         of positions this one's position. */
                      if( *V<minima_v )
                        {
                          minima_v = *V;
                          oi[ OCOL_MINVNUM ]=1;
                          if(oif[OCOL_MINVX])oi[OCOL_MINVX]=c[ndim-1]+1;
                          if(oif[OCOL_MINVY])oi[OCOL_MINVY]=c[ndim-2]+1;
                          if(oif[OCOL_MINVZ])oi[OCOL_MINVZ]=c[ndim-3]+1;
                        }
                      else
                        {
                          oi[ OCOL_MINVNUM ]++;
                          if(oif[OCOL_MINVX])oi[OCOL_MINVX]+=c[ndim-1]+1;
                          if(oif[OCOL_MINVY])oi[OCOL_MINVY]+=c[ndim-2]+1;
                          if(oif[OCOL_MINVZ])oi[OCOL_MINVZ]+=c[ndim-3]+1;
                        }
                    }
                  if( oif[ OCOL_MAXVNUM ] && *V>=maxima_v )
                    {
                      if( *V>maxima_v )
                        {
                          maxima_v = *V;
                          oi[ OCOL_MAXVNUM ]=1;
                          if(oif[OCOL_MAXVX])oi[OCOL_MAXVX]=c[ndim-1]+1;
                          if(oif[OCOL_MAXVY])oi[OCOL_MAXVY]=c[ndim-2]+1;
                          if(oif[OCOL_MAXVZ])oi[OCOL_MAXVZ]=c[ndim-3]+1;
                        }
                      else
                        {
                          oi[ OCOL_MAXVNUM ]++;
                          if(oif[OCOL_MAXVX])oi[OCOL_MAXVX]+=c[ndim-1]+1;
                          if(oif[OCOL_MAXVY])oi[OCOL_MAXVY]+=c[ndim-2]+1;
                          if(oif[OCOL_MAXVZ])oi[OCOL_MAXVZ]+=c[ndim-3]+1;
                        }
                    }

                  /* For flux weighted centers, we can only use positive
                     values, so do those measurements here. */
                  if( *V > 0.0f )
                    {
                      if(oif[ OCOL_NUMWHT ]) oi[ OCOL_NUMWHT ]++;
                      if(oif[ OCOL_SUMWHT ]) oi[ OCOL_SUMWHT ] += *V;
                      if(oif[ OCOL_VX ]) oi[ OCOL_VX ] += *V*(c[ndim-1]+1);
                      if(oif[ OCOL_VY ]) oi[ OCOL_VY ] += *V*(c[ndim-2]+1);
                      if(oif[ OCOL_VZ ]) oi[ OCOL_VZ ] += *V*(c[ndim-3]+1);
                      if(pp->shift)
                        {
                          oi[ OCOL_VXX    ] += *V * sc[1] * sc[1];
                          oi[ OCOL_VYY    ] += *V * sc[0] * sc[0];
                          oi[ OCOL_VXY    ] += *V * sc[1] * sc[0];
                        }
                      if(p->clumps && *C>0)
                        {
                          if(oif[ OCOL_C_NUMWHT ]) oi[ OCOL_C_NUMWHT ]++;
                          if(oif[ OCOL_C_SUMWHT ]) oi[ OCOL_C_SUMWHT ]+=*V;
                          if(oif[ OCOL_C_VX ])
                            oi[   OCOL_C_VX ] += *V * (c[ ndim-1 ]+1);
                          if(oif[ OCOL_C_VY ])
                            oi[   OCOL_C_VY ] += *V * (c[ ndim-2 ]+1);
                          if(oif[ OCOL_C_VZ ])
                            oi[   OCOL_C_VZ ] += *V * (c[ ndim-3 ]+1);
                        }
                    }
                }


              /* Sky value based measurements. */
              if(p->sky && oif[ OCOL_SUMSKY ])
                {
                  skyval = ( pp->st_sky
                             ? (isnan(*SK)?0:*SK)        /* Full array  */
                             : ( p->sky->size>1
                                 ? (isnan(sky[tid])?0:sky[tid]) /* Tile */
                                 : sky[0] ) );           /* Single value*/
                  if(!isnan(skyval))
                    {
                      oi[ OCOL_NUMSKY  ]++;
                      oi[ OCOL_SUMSKY  ] += skyval;
                    }
                }


              /* Sky standard deviation based measurements.*/
              if(p->std)
                {
                  /* Calculate the variance and save it in the output if
                     necessary. */
                  sval=pp->st_std ? *ST : (p->std->size>1?std[tid]:std[0]);
                  var = p->variance ? sval : sval*sval;
                  if(oif[ OCOL_SUMVAR ] && (!isnan(var)))
                    {
                      oi[ OCOL_NUMVAR  ]++;
                      oi[ OCOL_SUMVAR  ] += var;
                    }

                  /* For each pixel, we have a sky contribution to the
                     counts and the signal's contribution. The standard
                     deviation in the sky is simply 'sval', but the
                     standard deviation of the signal (independent of the
                     sky) is 'sqrt(*V)'. Therefore the total variance of
                     this pixel is the variance of the sky added with the
                     absolute value of its sky-subtracted flux. */
                  if(oif[ OCOL_SUM_VAR ] && goodvalue)
                    {
                      if(!isnan(var))
                        {
                          oi[ OCOL_SUM_VAR_NUM  ]++;
                          oi[ OCOL_SUM_VAR      ] += var + (vine?*V:0);
                        }
                    }
                }
            }

          /* Increment the other pointers. */
          if( xybin                ) ++pind;
          if( p->values            ) ++V;
          if( p->clumps            ) ++C;
          if( p->sky && pp->st_sky ) ++SK;
          if( p->std && pp->st_std ) ++ST;
        }
      while(++O<OO);

      /* Increment to the next contiguous region of this tile. */
      increment += ( gal_tile_block_increment(p->objects, tsize,
                                              num_increment++, NULL) );

      /* If a 2D projection is requested, see if we should initialize (set
         to zero) the projection-index ('pind') not. */
      if(xybin && (num_increment-1)%tsize[1]==0 )
        pind=0;
    }

  /* Write the projected area columns. */
  if(xybin)
    {
      /* Any non-zero pixel must be set for NUMALLXY. */
      uf=(u=xybin->array)+xybin->size;
      do
        if(*u)
          {
            if(oif[ OCOL_NUMALLXY ]          ) oi[ OCOL_NUMALLXY ]++;
            if(oif[ OCOL_NUMXY    ] && *u==2 ) oi[ OCOL_NUMXY    ]++;
          }
      while(++u<uf);

      /* For a check on the projected 2D areas.
      if(xybin && pp->object==1)
        {
          gal_fits_img_write(xybin, "xybin.fits", NULL, NULL);
          printf("Created 'xybin.fits'\n"); exit(0);
        }
      */
    }

  /* Generate the Spectrum. */
  if(    oif[ OCOL_SUMINSLICE         ]
      || oif[ OCOL_NUMINSLICE         ]
      || oif[ OCOL_NUMALLINSLICE      ]
      || oif[ OCOL_SUMVARINSLICE      ]
      || oif[ OCOL_SUMPROJINSLICE     ]
      || oif[ OCOL_NUMOTHERINSLICE    ]
      || oif[ OCOL_SUMOTHERINSLICE    ]
      || oif[ OCOL_SUMPROJVARINSLICE  ]
      || oif[ OCOL_SUMOTHERVARINSLICE ]
      || oif[ OCOL_NUMALLOTHERINSLICE ])
    parse_vector_dim3(pp, xybin);

  /* Clean up. */
  if(c)     free(c);
  if(sc)    free(sc);
  if(xybin) gal_data_free(xybin);
}





/* To keep the main function easier to read. */
static void *
parse_init_extrema(uint8_t *cif, uint8_t type, size_t num, int max1min0)
{
  void *out;
  double *out_d;
  size_t i, *out_s;

  /* Allocate the array. */
  out=gal_pointer_allocate(type, num, 0, __func__, "out");

  /* Initialize the array. */
  switch(type)
    {
    case GAL_TYPE_FLOAT64:
      out_d=out;
      for(i=0;i<num;++i) out_d[i]= max1min0 ? -FLT_MAX : FLT_MAX;
      break;
    case GAL_TYPE_SIZE_T:
      out_s=out;
      for(i=0;i<num;++i) out_s[i]= max1min0 ? 0 : GAL_BLANK_SIZE_T;
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix "
            "the problem. Type code %d isn't recognized", __func__,
            PACKAGE_BUGREPORT, type);
    }

  /* Return the allocated array. */
  return out;
}






/* Macro to help in finding the minimum and maximum coordinates. */
#define CMIN(COL, DIM) ( ci[ CCOL_NUMALL ]==1.0f                        \
                         ? (c[ DIM ]+1)                                 \
                         : ( (c[ DIM ]+1) < ci[ COL ]                   \
                             ? (c[ DIM ]+1) : ci[ COL ] ) )
#define CMAX(COL, DIM) ( ci[ CCOL_NUMALL ]==1.0f                        \
                         ? (c[ DIM ]+1)                                 \
                         : ( (c[ DIM ]+1) > ci[ COL ]                   \
                             ? (c[ DIM ]+1) : ci[ COL ] ) )

/* Parse over the clumps within an object.  */
void
parse_clumps(struct mkcatalog_passparams *pp)
{
  struct mkcatalogparams *p=pp->p;
  size_t ndim=p->objects->ndim, *dsize=p->objects->dsize;

  double *ci, *cir;
  gal_data_t *xybin=NULL;
  uint8_t vine=!p->novalinerror;
  int32_t *O, *OO, *C=NULL, nlab;
  size_t cind, *tsize=pp->tile->dsize;
  double *minima_v=NULL, *maxima_v=NULL;
  uint8_t *u, *uf, goodvalue, *cif=p->ciflag;
  size_t nngb=gal_dimension_num_neighbors(ndim);
  size_t i, ii, d, pind=0, increment=0, num_increment=1;
  float var, sval, skyval, *V=NULL, *SK=NULL, *ST=NULL;
  int32_t *objects=p->objects->array, *clumps=p->clumps->array;
  float *std=p->std?p->std->array:NULL, *sky=p->sky?p->sky->array:NULL;

  /* If tile processing isn't necessary, set 'tid' to a blank value. */
  size_t tid = ( ( (p->sky     && p->sky->size>1 && pp->st_sky == NULL )
                   || ( p->std && p->std->size>1 && pp->st_std == NULL ) )
                 ? 0 : GAL_BLANK_SIZE_T );

  /* Coordinate shift. */
  size_t *sc = ( pp->shift
                 ? gal_pointer_allocate(GAL_TYPE_SIZE_T, ndim, 0,
                                        __func__, "sc")
                 : NULL );

  /* If any coordinate columns are requested. */
  size_t *c = ( ( cif[    CCOL_GX    ]
                  || cif[ CCOL_GY    ]
                  || cif[ CCOL_GZ    ]
                  || cif[ CCOL_VX    ]
                  || cif[ CCOL_VY    ]
                  || cif[ CCOL_VZ    ]
                  || cif[ CCOL_MINX  ]
                  || cif[ CCOL_MAXX  ]
                  || cif[ CCOL_MINY  ]
                  || cif[ CCOL_MAXY  ]
                  || cif[ CCOL_MINZ  ]
                  || cif[ CCOL_MAXZ  ]
                  || cif[ CCOL_MINVX ]
                  || cif[ CCOL_MAXVX ]
                  || cif[ CCOL_MINVY ]
                  || cif[ CCOL_MAXVY ]
                  || cif[ CCOL_MINVZ ]
                  || cif[ CCOL_MAXVZ ]
                  || cif[ CCOL_MINVNUM ]
                  || cif[ CCOL_MAXVNUM ]
                  || sc
                  || tid==GAL_BLANK_SIZE_T )
                ? gal_pointer_allocate(GAL_TYPE_SIZE_T, ndim, 0,
                                       __func__, "c")
                : NULL );

  /* Preparations for neighbor parsing. */
  int32_t *ngblabs=( ( cif[    CCOL_RIV_NUM     ]
                       || cif[ CCOL_RIV_SUM     ]
                       || cif[ CCOL_RIV_SUM_VAR ] )
                     ? gal_pointer_allocate(GAL_TYPE_INT32, nngb, 0,
                                             __func__, "ngblabs")
                     : NULL );
  size_t *dinc = ngblabs ? gal_dimension_increment(ndim, dsize) : NULL;

  /* If an XY projection area is requested, we'll need to allocate an array
     to keep the projected space.*/
  if( cif[    CCOL_NUMALLXY ]
      || cif[ CCOL_NUMXY    ] )
    {
      xybin=gal_data_array_calloc(pp->clumpsinobj);
      for(i=0;i<pp->clumpsinobj;++i)
        gal_data_initialize(&xybin[i], NULL, GAL_TYPE_UINT8, 2, &tsize[1],
                            NULL, 1, p->cp.minmapsize, p->cp.quietmmap,
                            NULL, NULL, NULL);
    }

  /* For the extrema columns. */
  if( cif[    CCOL_MINVNUM ] || cif[ CCOL_MINVX ]
      || cif[ CCOL_MINVY   ] || cif[ CCOL_MINVZ ] )
    minima_v=parse_init_extrema(cif, GAL_TYPE_FLOAT64, pp->clumpsinobj, 0);
  if( cif[    CCOL_MAXVNUM ] || cif[ CCOL_MAXVX ]
      || cif[ CCOL_MAXVY   ] || cif[ CCOL_MAXVZ ] )
    maxima_v=parse_init_extrema(cif, GAL_TYPE_FLOAT64, pp->clumpsinobj, 1);

  /* Parse each contiguous patch of memory covered by this object. */
  while( pp->start_end_inc[0] + increment <= pp->start_end_inc[1] )
    {
      /* Set the contiguous range to parse. The pixel-to-pixel counting
         along the fastest dimension will be done over the 'O' pointer. */
      C = pp->st_c + increment;
      if( p->values            ) V  = pp->st_v   + increment;
      if( p->sky && pp->st_sky ) SK = pp->st_sky + increment;
      if( p->std && pp->st_std ) ST = pp->st_std + increment;
      OO = ( O = pp->st_o + increment ) + tsize[ndim-1];

      /* Parse the tile */
      do
        {
          /* If this pixel belongs to the requested object then do the
             processing. */
          if( *O==pp->object )
            {
              /* We are on a clump. */
              if(p->clumps && *C>0)
                {
                  /* Pointer to make things easier. Note that the clump
                     labels start from 1, but the array indexs from 0.*/
                  cind = *C-1;
                  ci=&pp->ci[ cind * CCOL_NUMCOLS ];

                  /* Add to the area of this object. */
                  if( cif[ CCOL_NUMALL ]
                      || cif[ CCOL_MINX ] || cif[ CCOL_MAXX ]
                      || cif[ CCOL_MINY ] || cif[ CCOL_MAXY ]
                      || cif[ CCOL_MINZ ] || cif[ CCOL_MAXZ ] )
                    ci[ CCOL_NUMALL ]++;
                  if(cif[ CCOL_NUMALLXY ])
                    ((uint8_t *)(xybin[cind].array))[ pind ] = 1;

                  /* Raw-position related measurements. */
                  if(c)
                    {
                      /* Get "C" the coordinates of this point. */
                      gal_dimension_index_to_coord(O-objects, ndim,
                                                   dsize, c);

                      /* Position extrema measurements. */
                      if(cif[ CCOL_MINX ])
                        ci[CCOL_MINX]=CMIN(CCOL_MINX, ndim-1);
                      if(cif[ CCOL_MAXX ])
                        ci[CCOL_MAXX]=CMAX(CCOL_MAXX, ndim-1);
                      if(cif[ CCOL_MINY ])
                        ci[CCOL_MINY]=CMIN(CCOL_MINY, ndim-2);
                      if(cif[ CCOL_MAXY ])
                        ci[CCOL_MAXY]=CMAX(CCOL_MAXY, ndim-2);
                      if(cif[ CCOL_MINZ ])
                        ci[CCOL_MINZ]=CMIN(CCOL_MINZ, ndim-3);
                      if(cif[ CCOL_MAXZ ])
                        ci[CCOL_MAXZ]=CMAX(CCOL_MAXZ, ndim-3);

                      /* If we need tile-ID, get the tile ID now. */
                      if(tid!=GAL_BLANK_SIZE_T)
                        tid=gal_tile_full_id_from_coord(&p->cp.tl, c);

                      /* General geometric (independent of pixel value)
                         calculations. */
                      if(cif[ CCOL_GX ]) ci[ CCOL_GX ] += c[ ndim-1 ]+1;
                      if(cif[ CCOL_GY ]) ci[ CCOL_GY ] += c[ ndim-2 ]+1;
                      if(cif[ CCOL_GZ ]) ci[ CCOL_GZ ] += c[ ndim-3 ]+1;
                      if(pp->shift)
                        {
                          /* Shifted coordinates for second order moments,
                             see explanations in the first pass.*/
                          for(d=0;d<ndim;++d) sc[d] = c[d]+1-pp->shift[d];

                          /* Raw second-order measurements. */
                          ci[ CCOL_GXX ] += sc[1] * sc[1];
                          ci[ CCOL_GYY ] += sc[0] * sc[0];
                          ci[ CCOL_GXY ] += sc[1] * sc[0];
                        }
                    }

                  /* Value related measurements, see 'parse_objects' for
                     comments. */
                  goodvalue=0;
                  if( p->values && !( p->hasblank && isnan(*V) ) )
                    {
                      /* For the standard-deviation measurement. */
                      goodvalue=1;

                      /* Fill in the necessary information. */
                      if(cif[ CCOL_NUM   ]) ci[ CCOL_NUM   ]++;
                      if(cif[ CCOL_SUM   ]) ci[ CCOL_SUM   ] += *V;
                      if(cif[ CCOL_SUMP2 ]) ci[ CCOL_SUMP2 ] += *V * *V;
                      if(cif[ CCOL_NUMXY ])
                        ((uint8_t *)(xybin[cind].array))[ pind ] = 2;

                      /* Minimum/maximum pixel positions. */
                      if( cif[ CCOL_MINVNUM ] && *V<=minima_v[cind] )
                        {
                          if( *V<minima_v[cind] )
                            {
                              minima_v[cind] = *V;
                              ci[ CCOL_MINVNUM ]=1;
                              if(cif[CCOL_MINVX])
                                ci[ CCOL_MINVX ] = c[ ndim-1 ]+1;
                              if(cif[CCOL_MINVY])
                                ci[ CCOL_MINVY ] = c[ ndim-2 ]+1;
                              if(cif[CCOL_MINVZ])
                                ci[ CCOL_MINVZ ] = c[ ndim-3 ]+1;
                            }
                          else
                            {
                              ci[ CCOL_MINVNUM ]++;
                              if(cif[CCOL_MINVX])
                                ci[  CCOL_MINVX ] += c[ ndim-1 ]+1;
                              if(cif[CCOL_MINVY])
                                ci[  CCOL_MINVY ] += c[ ndim-2 ]+1;
                              if(cif[CCOL_MINVZ])
                                ci[  CCOL_MINVZ ] += c[ ndim-3 ]+1;
                            }
                        }
                      if( cif[ CCOL_MAXVNUM ] && *V>=maxima_v[cind] )
                        {
                          if( *V>maxima_v[cind] )
                            {
                              maxima_v[cind] = *V;
                              ci[ CCOL_MAXVNUM ]=1;
                              if(cif[CCOL_MAXVX])
                                ci[  CCOL_MAXVX ] = c[ ndim-1 ]+1;
                              if(cif[CCOL_MAXVY])
                                ci[  CCOL_MAXVY ] = c[ ndim-2 ]+1;
                              if(cif[CCOL_MAXVZ])
                                ci[  CCOL_MAXVZ ] = c[ ndim-3 ]+1;
                            }
                          else
                            {
                              ci[ CCOL_MAXVNUM ]++;
                              if(cif[CCOL_MAXVX])
                                ci[  CCOL_MAXVX ] += c[ ndim-1 ]+1;
                              if(cif[CCOL_MAXVY])
                                ci[  CCOL_MAXVY ] += c[ ndim-2 ]+1;
                              if(cif[CCOL_MAXVZ])
                                ci[  CCOL_MAXVZ ] += c[ ndim-3 ]+1;
                            }
                        }

                      /* Columns that need positive values. */
                      if( *V > 0.0f )
                        {
                          if(cif[ CCOL_NUMWHT ]) ci[ CCOL_NUMWHT ]++;
                          if(cif[ CCOL_SUMWHT ]) ci[ CCOL_SUMWHT ] += *V;
                          if(cif[ CCOL_VX ])
                            ci[   CCOL_VX ] += *V * (c[ ndim-1 ]+1);
                          if(cif[ CCOL_VY ])
                            ci[   CCOL_VY ] += *V * (c[ ndim-2 ]+1);
                          if(cif[ CCOL_VZ ])
                            ci[   CCOL_VZ ] += *V * (c[ ndim-3 ]+1);
                          if(pp->shift)
                            {
                              ci[ CCOL_VXX ] += *V * sc[1] * sc[1];
                              ci[ CCOL_VYY ] += *V * sc[0] * sc[0];
                              ci[ CCOL_VXY ] += *V * sc[1] * sc[0];
                            }
                        }
                    }

                  /* Sky based measurements. */
                  if(p->sky && cif[ CCOL_SUMSKY ])
                    {
                      skyval = ( pp->st_sky
                                 ? *SK             /* Full. */
                                 : ( p->sky->size>1
                                     ? sky[tid]    /* Tile. */
                                     : sky[0] ) ); /* 1 value. */
                      if(!isnan(skyval))
                        {
                          ci[ CCOL_NUMSKY  ]++;
                          ci[ CCOL_SUMSKY  ] += skyval;
                        }
                    }

                  /* Sky Standard deviation based measurements, see
                     'parse_objects' for comments. */
                  if(p->std)
                    {
                      sval = ( pp->st_std
                               ? *ST
                               : (p->std->size>1 ? std[tid] : std[0]) );
                      var = p->variance ? sval : sval*sval;
                      if(cif[ CCOL_SUMVAR  ] && (!isnan(var)))
                        {
                          ci[ CCOL_NUMVAR ]++;
                          ci[ CCOL_SUMVAR ] += var;
                        }
                      if(cif[ CCOL_SUM_VAR ] && goodvalue)
                        {
                          if(!isnan(var))
                            {
                              ci[ CCOL_SUM_VAR_NUM ]++;
                              ci[ CCOL_SUM_VAR     ] += var+(vine?*V:0);
                            }
                        }
                    }
                }

              /* This pixel is on the diffuse region (and the object
                 actually has clumps). If any river-based measurements are
                 necessary check to see if it is touching a clump or not,
                 but only if this object actually has any clumps. */
              else if(ngblabs && pp->clumpsinobj)
                {
                  /* We are on a diffuse (possibly a river) pixel. So the
                     value of this pixel has to be added to any of the
                     clumps in touches. But since it might touch a labeled
                     region more than once, we use 'ngblabs' to keep track
                     of which label we have already added its value
                     to. 'ii' is the number of different labels this river
                     pixel has already been considered for. 'ngblabs' will
                     keep the list labels. */
                  ii=0;
                  memset(ngblabs, 0, nngb*sizeof *ngblabs);

                  /* Go over the neighbors and see if this pixel is
                     touching a clump or not. */
                  GAL_DIMENSION_NEIGHBOR_OP(O-objects, ndim, dsize, ndim,
                                            dinc,
                     {
                       /* Neighbor's label (mainly for easy reading). */
                       nlab=clumps[nind];

                       /* We only want neighbors that are a clump and part
                          of this object and part of the same object. */
                       if( nlab>0 && objects[nind]==pp->object)
                         {
                           /* Go over all already checked labels and make
                              sure this clump hasn't already been
                              considered. */
                           for(i=0;i<ii;++i) if(ngblabs[i]==nlab) break;

                           /* It hasn't been considered yet: */
                           if(i==ii)
                             {
                               /* Make sure it won't be considered any
                                  more. */
                               ngblabs[ii++] = nlab;

                               /* To help in reading. */
                               cir=&pp->ci[ (nlab-1) * CCOL_NUMCOLS ];

                               /* Write in the necessary values. */
                               if(cif[ CCOL_RIV_NUM  ])
                                 cir[ CCOL_RIV_NUM ]++;

                               /* Total sum of values in river. */
                               if(cif[ CCOL_RIV_SUM  ])
                                 cir[ CCOL_RIV_SUM ] += *V;

                               /* Minimum river value. */
                               if(cif[CCOL_RIV_MIN])
                                 if(cir[CCOL_RIV_NUM]==1
                                    || *V < cir[CCOL_RIV_MIN])
                                   cir[CCOL_RIV_MIN]=*V;

                               /* Maximum river value. */
                               if(cif[CCOL_RIV_MAX])
                                 if(cir[CCOL_RIV_NUM]==1
                                    || *V > cir[CCOL_RIV_MAX])
                                   cir[CCOL_RIV_MAX]=*V;

                               /* Sum of variances within river. */
                               if(cif[ CCOL_RIV_SUM_VAR  ])
                                 {
                                   sval = ( pp->st_std
                                            ? *ST
                                            : ( p->std->size>1
                                                ? std[tid]
                                                : std[0] )     );
                                   cir[ CCOL_RIV_SUM_VAR ] +=
                                     (p->variance ? sval : sval*sval)
                                     + (vine?*V:0);
                                 }
                             }
                         }
                     });
                }
            }

          /* Increment the other pointers. */
          ++C;
          if( xybin                ) ++pind;
          if( p->values            ) ++V;
          if( p->sky && pp->st_sky ) ++SK;
          if( p->std && pp->st_std ) ++ST;
        }
      while(++O<OO);

      /* Increment to the next contiguous region of this tile. */
      increment += ( gal_tile_block_increment(p->objects, tsize,
                                              num_increment++, NULL) );

      /* If a 2D projection is requested, see if we should initialize (set
         to zero) the projection-index ('pind') not. */
      if(xybin && (num_increment-1) % tsize[1]==0 )
        pind=0;
    }


  /* Write the higher-level columns. */
  for(i=0;i<pp->clumpsinobj;++i)
    {
      /* Pointer to make things easier. */
      ci=&pp->ci[ i * CCOL_NUMCOLS ];

      /* Write the XY projection columns. */
      if(xybin)
        {
          /* Any non-zero pixel must be set for NUMALLXY. */
          uf=(u=xybin[i].array)+xybin[i].size;
          do
            if(*u)
              {
                if(cif[ CCOL_NUMALLXY ]          ) ci[ CCOL_NUMALLXY ]++;
                if(cif[ CCOL_NUMXY    ] && *u==2 ) ci[ CCOL_NUMXY    ]++;
              }
          while(++u<uf);

          /* For a check on the projected 2D areas. */
          if(xybin && pp->object==2)
            gal_fits_img_write(&xybin[i], "xybin.fits", NULL, 0);

        }
    }


  /* Clean up. */
  if(c) free(c);
  if(sc) free(sc);
  if(dinc) free(dinc);
  if(ngblabs) free(ngblabs);
  if(minima_v) free(minima_v);
  if(maxima_v) free(maxima_v);
  if(xybin) gal_data_array_free(xybin, pp->clumpsinobj, 1);
}





static size_t
parse_frac_find(gal_data_t *sorted_d, double value, double frac, int dosum)
{
  size_t i;
  double check=0.0f;
  double *sorted=sorted_d->array;

  /* Parse over the sorted array and find the index. */
  for(i=0;i<sorted_d->size;++i)
    if(dosum)
      { if( (check+=sorted[i]) > value*frac ) break; }
    else
      { if(         sorted[i]  < value*frac ) break; }

  /* Return the final value. Note that if the index is zero, we should
     actually return 1, because we are starting with the maximum. */
  return i==0 ? 1 : i;
}





static double
parse_frac_sum(gal_data_t *sorted_d, double value, double frac, int dosum)
{
  double sum=0.0f, *sorted=sorted_d->array;
  size_t i, ind=parse_frac_find(sorted_d, value, frac, 0);

  for(i=0;i<ind;++i) sum+=sorted[i];
  return sum;
}





static void
parse_area_of_frac_sum(struct mkcatalog_passparams *pp, gal_data_t *values,
                       double *outarr, int o1c0)
{
  struct mkcatalogparams *p=pp->p;

  double max, *sorted;
  gal_data_t *sorted_d;
  uint8_t *flag = o1c0 ? p->oiflag : p->ciflag;
  double *fracmax = p->fracmax ? p->fracmax->array : NULL;
  double sumlab = o1c0 ? outarr[OCOL_SUM] : outarr[CCOL_SUM];

  /* Allocate the array to use. */
  sorted_d = ( values->type==GAL_TYPE_FLOAT64
                ? values
                : gal_data_copy_to_new_type(values, GAL_TYPE_FLOAT64) );

  /* Sort the desired labels and find the number of elements where we reach
     half the total sum. */
  gal_statistics_sort_decreasing(sorted_d);

  /* Set the required fractions. */
  if(flag[ o1c0 ? OCOL_HALFSUMNUM : CCOL_HALFSUMNUM ])
    outarr[ o1c0 ? OCOL_HALFSUMNUM : CCOL_HALFSUMNUM ]
      = parse_frac_find(sorted_d, sumlab, 0.5f, 1);

  /* Values related to the maximum. */
  if( flag[    o1c0 ? OCOL_MAXIMUM     : CCOL_MAXIMUM     ]
      || flag[ o1c0 ? OCOL_HALFMAXNUM  : CCOL_HALFMAXNUM  ]
      || flag[ o1c0 ? OCOL_HALFMAXSUM  : CCOL_HALFMAXSUM  ]
      || flag[ o1c0 ? OCOL_FRACMAX1NUM : CCOL_FRACMAX1NUM ]
      || flag[ o1c0 ? OCOL_FRACMAX1SUM : CCOL_FRACMAX1SUM ]
      || flag[ o1c0 ? OCOL_FRACMAX2NUM : CCOL_FRACMAX2NUM ]
      || flag[ o1c0 ? OCOL_FRACMAX2SUM : CCOL_FRACMAX2SUM ] )
    {
      /* Set the array and maximum value. We'll use the median of the top
         three pixels for the maximum (to avoid noise) */
      sorted=sorted_d->array;
      max = ( sorted_d->size>3
              ? (sorted[0]+sorted[1]+sorted[2])/3
              : sorted[0] );

      /* If we want the maximum value, then write it in. */
      if(flag[ o1c0 ? OCOL_MAXIMUM : CCOL_MAXIMUM ])
        outarr[ o1c0 ? OCOL_MAXIMUM : CCOL_MAXIMUM ] = max;

      /* Number of pixels within half the maximum. */
      if(flag[ o1c0 ? OCOL_HALFMAXNUM : CCOL_HALFMAXNUM ])
        outarr[ o1c0 ? OCOL_HALFMAXNUM : CCOL_HALFMAXNUM ]
          = parse_frac_find(sorted_d, max, 0.5f, 0);

      /* Number of pixels within the first requested fraction of maximum */
      if(flag[ o1c0 ? OCOL_FRACMAX1NUM : CCOL_FRACMAX1NUM ])
        outarr[ o1c0 ? OCOL_FRACMAX1NUM : CCOL_FRACMAX1NUM ]
          = parse_frac_find(sorted_d, max, fracmax[0], 0);

      /* Number of pixels within the first requested fraction of maximum */
      if(flag[ o1c0 ? OCOL_FRACMAX2NUM : CCOL_FRACMAX2NUM ])
        outarr[ o1c0 ? OCOL_FRACMAX2NUM : CCOL_FRACMAX2NUM ]
          = parse_frac_find(sorted_d, max, fracmax[1], 0);

      /* Sum of the pixels within the given fraction of the maximum. */
      if( flag[ o1c0 ? OCOL_HALFMAXSUM : CCOL_HALFMAXSUM ] )
        outarr[ o1c0 ? OCOL_HALFMAXSUM : CCOL_HALFMAXSUM ]
          = parse_frac_sum(sorted_d, max, 0.5f, 0);

      /* Sum of the pixels within the 1st given fraction of the maximum. */
      if( flag[ o1c0 ? OCOL_FRACMAX1SUM : CCOL_FRACMAX1SUM ] )
        outarr[ o1c0 ? OCOL_FRACMAX1SUM : CCOL_FRACMAX1SUM ]
          = parse_frac_sum(sorted_d, max, fracmax[0], 0);

      /* Sum of the pixels within the 1st given fraction of the maximum. */
      if( flag[ o1c0 ? OCOL_FRACMAX2SUM : CCOL_FRACMAX2SUM ] )
        outarr[ o1c0 ? OCOL_FRACMAX2SUM : CCOL_FRACMAX2SUM ]
          = parse_frac_sum(sorted_d, max, fracmax[1], 0);
    }

  /* Clean up and return. */
  if(sorted_d!=values) gal_data_free(sorted_d);
}





void
parse_order_based(struct mkcatalog_passparams *pp)
{
  struct mkcatalogparams *p=pp->p;

  float *V;
  double *ci;
  float *sigcliparr;
  gal_data_t *result;
  uint8_t clipflags=0;
  int32_t *O, *OO, *C=NULL;
  size_t i, increment=0, num_increment=1;
  gal_data_t *objvals=NULL, **clumpsvals=NULL;
  size_t *tsize=pp->tile->dsize, ndim=p->objects->ndim;
  size_t counter=0, *ccounter=NULL, tmpsize=pp->oi[OCOL_NUM];

  /* It may happen that there are no usable pixels for this object (and
     thus its possible clumps). In this case `tmpsize' will be zero and we
     can just write NaN values for the necessary columns. */
  if(tmpsize==0)
    {
      if(p->oiflag[OCOL_MEDIAN       ]) pp->oi[ OCOL_MEDIAN       ] = NAN;
      if(p->oiflag[OCOL_MAXIMUM      ]) pp->oi[ OCOL_MAXIMUM      ] = NAN;
      if(p->oiflag[OCOL_HALFMAXSUM   ]) pp->oi[ OCOL_HALFMAXSUM   ] = NAN;
      if(p->oiflag[OCOL_HALFMAXNUM   ]) pp->oi[ OCOL_HALFMAXNUM   ] = 0;
      if(p->oiflag[OCOL_HALFSUMNUM   ]) pp->oi[ OCOL_HALFSUMNUM   ] = 0;
      if(p->oiflag[OCOL_FRACMAX1NUM  ]) pp->oi[ OCOL_FRACMAX1NUM  ] = 0;
      if(p->oiflag[OCOL_FRACMAX2NUM  ]) pp->oi[ OCOL_FRACMAX2NUM  ] = 0;
      if(p->oiflag[OCOL_SIGCLIPNUM   ]) pp->oi[ OCOL_SIGCLIPNUM   ] = 0;
      if(p->oiflag[OCOL_SIGCLIPSTD   ]) pp->oi[ OCOL_SIGCLIPSTD   ] = 0;
      if(p->oiflag[OCOL_SIGCLIPMEAN  ]) pp->oi[ OCOL_SIGCLIPMEAN  ] = NAN;
      if(p->oiflag[OCOL_SIGCLIPMEDIAN]) pp->oi[ OCOL_SIGCLIPMEDIAN] = NAN;
      if(p->clumps)
        for(i=0;i<pp->clumpsinobj;++i)
          {
            ci=&pp->ci[ i * CCOL_NUMCOLS ];
            if(p->ciflag[CCOL_MEDIAN     ])   ci[ CCOL_MEDIAN      ] = NAN;
            if(p->ciflag[CCOL_MAXIMUM    ])   ci[ CCOL_MAXIMUM     ] = NAN;
            if(p->ciflag[CCOL_HALFMAXSUM ])   ci[ CCOL_HALFMAXSUM  ] = NAN;
            if(p->ciflag[CCOL_HALFMAXNUM ])   ci[ CCOL_HALFMAXNUM  ] = 0;
            if(p->ciflag[CCOL_HALFSUMNUM ])   ci[ CCOL_HALFSUMNUM  ] = 0;
            if(p->ciflag[CCOL_FRACMAX1NUM])   ci[ CCOL_FRACMAX1NUM ] = 0;
            if(p->ciflag[CCOL_FRACMAX2NUM])   ci[ CCOL_FRACMAX2NUM ] = 0;
            if(p->ciflag[CCOL_SIGCLIPNUM ])   ci[ CCOL_SIGCLIPNUM  ] = 0;
            if(p->ciflag[CCOL_SIGCLIPSTD ])   ci[ CCOL_SIGCLIPSTD  ] = 0;
            if(p->ciflag[CCOL_SIGCLIPMEAN])   ci[ CCOL_SIGCLIPMEAN ] = NAN;
            if(p->ciflag[CCOL_SIGCLIPMEDIAN]) ci[ CCOL_SIGCLIPMEDIAN]=NAN;
          }
      return;
    }

  /* We know we have pixels to use, so allocate space for the values within
     the object. */
  objvals=gal_data_alloc(NULL, p->values->type, 1, &tmpsize, NULL, 0,
                         p->cp.minmapsize, p->cp.quietmmap, NULL, NULL,
                         NULL);

  /* Clump preparations. */
  if(p->clumps)
    {
      /* Allocate the necessary space. */
      errno=0;
      clumpsvals=malloc(pp->clumpsinobj * sizeof *clumpsvals);
      if(clumpsvals==NULL)
        error(EXIT_FAILURE, errno, "%s: couldn't allocate 'clumpsvals' "
              "for %zu clumps", __func__, pp->clumpsinobj);

      /* Allocate the array necessary to keep the values of each clump. */
      ccounter=gal_pointer_allocate(GAL_TYPE_SIZE_T, pp->clumpsinobj, 1,
                                    __func__, "ccounter");
      for(i=0;i<pp->clumpsinobj;++i)
        {
          tmpsize=pp->ci[ i * CCOL_NUMCOLS + CCOL_NUM ];
          clumpsvals[i] = ( tmpsize
                            ? gal_data_alloc(NULL, p->values->type, 1,
                                             &tmpsize, NULL, 0,
                                             p->cp.minmapsize,
                                             p->cp.quietmmap,
                                             NULL, NULL, NULL)
                            : NULL );
        }
    }


  /* Parse each contiguous patch of memory covered by this object. */
  while( pp->start_end_inc[0] + increment <= pp->start_end_inc[1] )
    {
      /* Set the contiguous range to parse. The pixel-to-pixel counting
         along the fastest dimension will be done over the 'O' pointer. */
      V = pp->st_v + increment;
      if(p->clumps) C = pp->st_c + increment;
      OO = ( O = pp->st_o + increment ) + tsize[ndim-1];

      /* Parse the next contiguous region of this tile. */
      do
        {
          /* If this pixel belongs to the requested object, then do the
             processing. 'hasblank' is constant, so when the values doesn't
             have any blank values, the 'isnan' will never be checked. */
          if( *O==pp->object && !( p->hasblank && isnan(*V) ) )
            {
              /* Copy the value for the whole object. */
              memcpy( gal_pointer_increment(objvals->array, counter++,
                                             p->values->type), V,
                      gal_type_sizeof(p->values->type) );

              /* We are also on a clump. */
              if(p->clumps && *C>0 && clumpsvals[*C-1]!=NULL)
                memcpy( gal_pointer_increment(clumpsvals[*C-1]->array,
                                              ccounter[*C-1]++,
                                              p->values->type), V,
                        gal_type_sizeof(p->values->type) );
            }

          /* Increment the other pointers. */
          ++V;
          if(p->clumps) ++C;
        }
      while(++O<OO);

      /* Increment to the next contiguous region of this tile. */
      increment += ( gal_tile_block_increment(p->objects, tsize,
                                              num_increment++, NULL) );
    }


  /* Calculate the necessary values for the objects. */
  if(p->oiflag[ OCOL_MEDIAN ])
    {
      result=gal_data_copy_to_new_type_free(gal_statistics_median(objvals,
                                                                  1),
                                            GAL_TYPE_FLOAT64);
      pp->oi[OCOL_MEDIAN]=*((double *)(result->array));
      gal_data_free(result);
    }
  if(p->oiflag[ OCOL_SIGCLIPNUM ]
     || p->oiflag[ OCOL_SIGCLIPSTD ]
     || p->oiflag[ OCOL_SIGCLIPMEAN ]
     || p->oiflag[ OCOL_SIGCLIPMEDIAN ])
    {
      /* See which optional clipping measurements are necessary and run the
         clipping. */
      clipflags=0;
      if(p->oiflag[ OCOL_SIGCLIPSTD ])
        clipflags |= GAL_STATISTICS_CLIP_OUTCOL_OPTIONAL_STD;
      if(p->oiflag[ OCOL_SIGCLIPMEAN ])
        clipflags |= GAL_STATISTICS_CLIP_OUTCOL_OPTIONAL_MEAN;
      result=gal_statistics_clip_sigma(objvals, p->sigmaclip[0],
                                       p->sigmaclip[1], clipflags,
                                       1, 1);
      sigcliparr=result->array;
      if(p->oiflag[ OCOL_SIGCLIPNUM ])
        pp->oi[OCOL_SIGCLIPNUM]
          = sigcliparr[GAL_STATISTICS_CLIP_OUTCOL_NUMBER_USED];
      if(p->oiflag[ OCOL_SIGCLIPSTD ])
        pp->oi[OCOL_SIGCLIPSTD]
          = sigcliparr[GAL_STATISTICS_CLIP_OUTCOL_STD];
      if(p->oiflag[ OCOL_SIGCLIPMEAN ])
        pp->oi[OCOL_SIGCLIPMEAN]
          = sigcliparr[GAL_STATISTICS_CLIP_OUTCOL_MEAN];
      if(p->oiflag[ OCOL_SIGCLIPMEDIAN ])
        pp->oi[OCOL_SIGCLIPMEDIAN]
          = sigcliparr[GAL_STATISTICS_CLIP_OUTCOL_MEDIAN];

      /* Clean up the sigma-clipped values. */
      gal_data_free(result);
    }

  /* Fractional values. */
  if( p->oiflag[    OCOL_MAXIMUM     ]
      || p->oiflag[ OCOL_HALFMAXNUM  ]
      || p->oiflag[ OCOL_HALFMAXSUM  ]
      || p->oiflag[ OCOL_HALFSUMNUM  ]
      || p->oiflag[ OCOL_FRACMAX1NUM ]
      || p->oiflag[ OCOL_FRACMAX2NUM ] )
    parse_area_of_frac_sum(pp, objvals, pp->oi, 1);

  /* Clean up the object values. */
  gal_data_free(objvals);


  /* Calculate the necessary value for clumps. */
  if(p->clumps)
    {
      for(i=0;i<pp->clumpsinobj;++i)
        {
          /* Set the main row to fill and initialize. */
          ci=&pp->ci[ i * CCOL_NUMCOLS ];

          /* Median. */
          if(p->ciflag[ CCOL_MEDIAN ])
            {
              if(clumpsvals[i])
                {
                  result=gal_statistics_median(clumpsvals[i], 1);
                  result=gal_data_copy_to_new_type_free(result,
                                                        GAL_TYPE_FLOAT64);
                  ci[ CCOL_MEDIAN ] = ( *((double *)(result->array))
                                        - (   ci[ CCOL_RIV_SUM ]
                                            / ci[ CCOL_RIV_NUM ]) );
                  gal_data_free(result);
                }
              else ci[ CCOL_MEDIAN ] = NAN;
            }

          /* Sigma-clipping measurements. */
          if(p->ciflag[ CCOL_SIGCLIPNUM ]
             || p->ciflag[ CCOL_SIGCLIPSTD ]
             || p->ciflag[ CCOL_SIGCLIPMEAN ]
             || p->ciflag[ CCOL_SIGCLIPMEDIAN ])
            {
              if(clumpsvals[i])
                {
                  clipflags=0;
                  if(p->oiflag[ OCOL_SIGCLIPSTD ])
                    clipflags |= GAL_STATISTICS_CLIP_OUTCOL_OPTIONAL_STD;
                  if(p->oiflag[ OCOL_SIGCLIPMEAN ])
                    clipflags |= GAL_STATISTICS_CLIP_OUTCOL_OPTIONAL_MEAN;
                  result=gal_statistics_clip_sigma(clumpsvals[i],
                                                   p->sigmaclip[0],
                                                   p->sigmaclip[1],
                                                   clipflags, 1, 1);
                  sigcliparr=result->array;
                  if(p->ciflag[ CCOL_SIGCLIPNUM ])
                    ci[CCOL_SIGCLIPNUM]
                      = sigcliparr[GAL_STATISTICS_CLIP_OUTCOL_NUMBER_USED];
                  if(p->ciflag[ CCOL_SIGCLIPSTD ])
                    ci[CCOL_SIGCLIPSTD]
                      = ( sigcliparr[GAL_STATISTICS_CLIP_OUTCOL_STD]
                          - (   ci[ CCOL_RIV_SUM ]
                              / ci[ CCOL_RIV_NUM ]));
                  if(p->ciflag[ CCOL_SIGCLIPMEAN ])
                    ci[CCOL_SIGCLIPMEAN]
                      = ( sigcliparr[GAL_STATISTICS_CLIP_OUTCOL_MEAN]
                          - (   ci[ CCOL_RIV_SUM ]
                              / ci[ CCOL_RIV_NUM ]));
                  if(p->ciflag[ CCOL_SIGCLIPMEDIAN ])
                    ci[CCOL_SIGCLIPMEDIAN]
                      = ( sigcliparr[GAL_STATISTICS_CLIP_OUTCOL_MEDIAN]
                          - (   ci[ CCOL_RIV_SUM ]
                              / ci[ CCOL_RIV_NUM ]));
                  gal_data_free(result);
                }
              else
                {
                  if(p->ciflag[ CCOL_SIGCLIPNUM    ])
                    ci[ CCOL_SIGCLIPNUM  ]=NAN;
                  if(p->ciflag[ CCOL_SIGCLIPSTD    ])
                    ci[ CCOL_SIGCLIPSTD  ]=NAN;
                  if(p->ciflag[ CCOL_SIGCLIPMEAN   ])
                    ci[ CCOL_SIGCLIPMEAN ]=NAN;
                  if(p->ciflag[ CCOL_SIGCLIPMEDIAN ])
                    ci[CCOL_SIGCLIPMEDIAN]=NAN;
                }
            }

          /* Estimate half of the total sum. */
          if( p->ciflag[    CCOL_MAXIMUM     ]
              || p->ciflag[ CCOL_HALFMAXNUM  ]
              || p->ciflag[ CCOL_HALFMAXSUM  ]
              || p->ciflag[ CCOL_HALFSUMNUM  ]
              || p->ciflag[ CCOL_FRACMAX1NUM ]
              || p->ciflag[ CCOL_FRACMAX1SUM ]
              || p->ciflag[ CCOL_FRACMAX2NUM ]
              || p->ciflag[ CCOL_FRACMAX2SUM ] )
            {
              if(clumpsvals[i])
                parse_area_of_frac_sum(pp, clumpsvals[i], ci, 0);
              else
                {
                  if(p->ciflag[CCOL_MAXIMUM    ]) ci[CCOL_MAXIMUM    ]=NAN;
                  if(p->ciflag[CCOL_HALFMAXNUM ]) ci[CCOL_HALFMAXNUM ]=NAN;
                  if(p->ciflag[CCOL_HALFMAXSUM ]) ci[CCOL_HALFMAXSUM ]=NAN;
                  if(p->ciflag[CCOL_HALFSUMNUM ]) ci[CCOL_HALFSUMNUM ]=NAN;
                  if(p->ciflag[CCOL_FRACMAX1NUM]) ci[CCOL_FRACMAX1NUM]=NAN;
                  if(p->ciflag[CCOL_FRACMAX1SUM]) ci[CCOL_FRACMAX1SUM]=NAN;
                  if(p->ciflag[CCOL_FRACMAX2NUM]) ci[CCOL_FRACMAX2NUM]=NAN;
                  if(p->ciflag[CCOL_FRACMAX2SUM]) ci[CCOL_FRACMAX2SUM]=NAN;
                }
            }

          /* Clean up this clump's values. */
          gal_data_free(clumpsvals[i]);
        }
      free(clumpsvals);
      free(ccounter);
    }
}
