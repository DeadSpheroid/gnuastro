/*********************************************************************
Settings for VizieR.
Query is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

#include <gnuastro-internal/checkset.h>

#include "main.h"

#include "ui.h"
#include "tap.h"


static void
vizier_sanity_checks(struct queryparams *p)
{
  /* VizieR specific: if the user has asked for '--information', but
     without '--limitinfo', print a notice to introduce 'limitinfo'. */
  if(p->datasetstr==NULL && p->information && p->limitinfo==NULL)
    {
      fprintf(stderr, "\n--------------------\n");
      error(EXIT_SUCCESS, 0, "WARNING: The full VizieR metadata "
            "(information) is more than 20Mb, and contains tens of "
            "thousands entries. You can use '--limitinfo=XXXX' to "
            "constrain the downloaded and displayed metadata to "
            "those that have 'XXXX' in the description (for example "
            "a certain author, or a certain project name). This will "
            "greatly improve the speed of your search");
      fprintf(stderr, "--------------------\n");
    }

  /* Set the summarized names. */
  if(p->datasetstr)
    {
      /* Check if the dataset name is a known summary. */
      if( !strcmp(p->datasetstr, "2mass") )
        gal_checkset_allocate_copy("II/246/out", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "akarifis") )
        gal_checkset_allocate_copy("II/298/fis", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "allwise") )
        gal_checkset_allocate_copy("II/328/allwise", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "apass9") )
        gal_checkset_allocate_copy("II/336/apass9", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "des1") )
        gal_checkset_allocate_copy("II/357/des_dr1", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "gaiaedr3") )
        gal_checkset_allocate_copy("I/350/gaiaedr3", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "gaiadr3") )
        gal_checkset_allocate_copy("I/355/gaiadr3", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "galex5") )
        gal_checkset_allocate_copy("II/312/ais", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "nomad") )
        gal_checkset_allocate_copy("I/297/out", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "panstarrs1") )
        gal_checkset_allocate_copy("II/349/ps1", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "pmx1") )
        gal_checkset_allocate_copy("I/317/sample", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "usnob1") )
        gal_checkset_allocate_copy("I/284/out", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "ucac5") )
        gal_checkset_allocate_copy("I/340/ucac5", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "unwise") )
        gal_checkset_allocate_copy("II/363/unwise", &p->datasetuse);
      else if( !strcmp(p->datasetstr, "catwise") )
        {
          if(p->ra_name==NULL) p->ra_name="RA_ICRS";
          if(p->dec_name==NULL) p->dec_name="DE_ICRS";
          gal_checkset_allocate_copy("II/365/catwise", &p->datasetuse);
        }
      else if( !strcmp(p->datasetstr, "gaiadr2") )
        {
          if(p->ra_name==NULL) p->ra_name="ra_epoch2000";
          if(p->dec_name==NULL) p->dec_name="dec_epoch2000";
          gal_checkset_allocate_copy("I/345/gaia2", &p->datasetuse);
        }
      else if( !strcmp(p->datasetstr, "sdss12") )
        {
          if(p->ra_name==NULL) p->ra_name="RA_ICRS";
          if(p->dec_name==NULL) p->dec_name="DE_ICRS";
          gal_checkset_allocate_copy("V/147/sdss12", &p->datasetuse);
        }

      /* The given dataset is not a known summary. */
      else
        gal_checkset_allocate_copy(p->datasetstr, &p->datasetuse);
    }

  /* Currently we assume VizieR only uses TAP. */
  p->usetap=1;
}





void
vizier_prepare(struct queryparams *p)
{
  /* VizieR-specific. */
  vizier_sanity_checks(p);

  /* Set the URLs, note that this is a simply-linked list, so we need to
     reverse it in the end (with 'gal_list_str_reverse') to have the same
     order here.

     Other possible VizieR TAP servers that don't seem to be working now
     (extracted from the 'visquery' script):
        http://vizier.cfa.harvard.edu/TAPVizieR/tap/sync
        http://vizier.nao.ac.jp/TAPVizieR/tap/sync
        http://data.bao.ac.cn/TAPVizieR/tap/sync
        http://vizier.ast.cam.ac.uk/TAPVizieR/tap/sync
        http://www.ukirt.jach.hawaii.edu/TAPVizieR/tap/sync
        http://vizier.inasan.ru/TAPVizieR/tap/sync */
  gal_list_str_add(&p->urls,
                   "http://tapvizier.u-strasbg.fr/TAPVizieR/tap/sync", 0);

  /* Name of default RA Dec columns. */
  if(p->ra_name==NULL)  p->ra_name="RAJ2000";
  if(p->dec_name==NULL) p->dec_name="DEJ2000";

  /* Basic sanity checks. */
  tap_sanity_checks(p);
}
