/*********************************************************************
Functions to that only use WCSLIB functionality.
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
#include <config.h>

#include <time.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include <gsl/gsl_linalg.h>
#include <wcslib/wcsmath.h>
#include <wcslib/wcsprintf.h>

#include <gnuastro/wcs.h>
#include <gnuastro/tile.h>
#include <gnuastro/fits.h>
#include <gnuastro/pointer.h>
#include <gnuastro/dimension.h>
#include <gnuastro/statistics.h>
#include <gnuastro/permutation.h>

#include <gnuastro-internal/checkset.h>

#if GAL_CONFIG_HAVE_WCSLIB_DIS_H
#include <wcslib/dis.h>
#include <gnuastro-internal/wcsdistortion.h>
#endif










/*************************************************************
 ***********               Macros                  ***********
 *************************************************************/
int
gal_wcs_distortion_name_to_id(char *distortion)
{
  if(      !strcmp(distortion,"TPD") ) return GAL_WCS_DISTORTION_TPD;
  else if( !strcmp(distortion,"SIP") ) return GAL_WCS_DISTORTION_SIP;
  else if( !strcmp(distortion,"TPV") ) return GAL_WCS_DISTORTION_TPV;
  else if( !strcmp(distortion,"DSS") ) return GAL_WCS_DISTORTION_DSS;
  else if( !strcmp(distortion,"WAT") ) return GAL_WCS_DISTORTION_WAT;
  else
    error(EXIT_FAILURE, 0, "WCS distortion name '%s' not recognized, "
          "currently recognized names are 'TPD', 'SIP', 'TPV', 'DSS' and "
          "'WAT'", distortion);

  /* Control should not reach here. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return GAL_WCS_DISTORTION_INVALID;
}





char *
gal_wcs_distortion_name_from_id(int distortion)
{
  /* Return the proper literal string. */
  switch(distortion)
    {
    case GAL_WCS_DISTORTION_TPD: return "TPD";
    case GAL_WCS_DISTORTION_SIP: return "SIP";
    case GAL_WCS_DISTORTION_TPV: return "TPV";
    case GAL_WCS_DISTORTION_DSS: return "DSS";
    case GAL_WCS_DISTORTION_WAT: return "WAT";
    default:
      error(EXIT_FAILURE, 0, "WCS distortion id '%d' isn't recognized",
            distortion);
    }

  /* Control should not reach here. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return NULL;
}





int
gal_wcs_coordsys_name_to_id(char *coordsys)
{
  if(      !strcmp(coordsys,"eq-j2000") ) return GAL_WCS_COORDSYS_EQJ2000;
  else if( !strcmp(coordsys,"eq-b1950") ) return GAL_WCS_COORDSYS_EQB1950;
  else if( !strcmp(coordsys,"ec-j2000") ) return GAL_WCS_COORDSYS_ECJ2000;
  else if( !strcmp(coordsys,"ec-b1950") ) return GAL_WCS_COORDSYS_ECB1950;
  else if( !strcmp(coordsys,"galactic") ) return GAL_WCS_COORDSYS_GALACTIC;
  else if( !strcmp(coordsys,"supergalactic") )
    return GAL_WCS_COORDSYS_SUPERGALACTIC;
  else
    error(EXIT_FAILURE, 0, "WCS coordinate system name '%s' not "
          "recognized, currently recognized names are 'eq-j2000', "
          "'eq-b1950', 'galactic' and 'supergalactic'", coordsys);

  /* Control should not reach here. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return GAL_WCS_COORDSYS_INVALID;
}





uint8_t
gal_wcs_projection_name_to_id(char *str)
{
  if(      !strcmp(str, "AZP") ) return GAL_WCS_PROJECTION_AZP;
  else if( !strcmp(str, "SZP") ) return GAL_WCS_PROJECTION_SZP;
  else if( !strcmp(str, "TAN") ) return GAL_WCS_PROJECTION_TAN;
  else if( !strcmp(str, "STG") ) return GAL_WCS_PROJECTION_STG;
  else if( !strcmp(str, "SIN") ) return GAL_WCS_PROJECTION_SIN;
  else if( !strcmp(str, "ARC") ) return GAL_WCS_PROJECTION_ARC;
  else if( !strcmp(str, "ZPN") ) return GAL_WCS_PROJECTION_ZPN;
  else if( !strcmp(str, "ZEA") ) return GAL_WCS_PROJECTION_ZEA;
  else if( !strcmp(str, "AIR") ) return GAL_WCS_PROJECTION_AIR;
  else if( !strcmp(str, "CYP") ) return GAL_WCS_PROJECTION_CYP;
  else if( !strcmp(str, "CEA") ) return GAL_WCS_PROJECTION_CEA;
  else if( !strcmp(str, "CAR") ) return GAL_WCS_PROJECTION_CAR;
  else if( !strcmp(str, "MER") ) return GAL_WCS_PROJECTION_MER;
  else if( !strcmp(str, "SFL") ) return GAL_WCS_PROJECTION_SFL;
  else if( !strcmp(str, "PAR") ) return GAL_WCS_PROJECTION_PAR;
  else if( !strcmp(str, "MOL") ) return GAL_WCS_PROJECTION_MOL;
  else if( !strcmp(str, "AIT") ) return GAL_WCS_PROJECTION_AIT;
  else if( !strcmp(str, "COP") ) return GAL_WCS_PROJECTION_COP;
  else if( !strcmp(str, "COE") ) return GAL_WCS_PROJECTION_COE;
  else if( !strcmp(str, "COD") ) return GAL_WCS_PROJECTION_COD;
  else if( !strcmp(str, "COO") ) return GAL_WCS_PROJECTION_COO;
  else if( !strcmp(str, "BON") ) return GAL_WCS_PROJECTION_BON;
  else if( !strcmp(str, "PCO") ) return GAL_WCS_PROJECTION_PCO;
  else if( !strcmp(str, "TSC") ) return GAL_WCS_PROJECTION_TSC;
  else if( !strcmp(str, "CSC") ) return GAL_WCS_PROJECTION_CSC;
  else if( !strcmp(str, "QSC") ) return GAL_WCS_PROJECTION_QSC;
  else if( !strcmp(str, "HPX") ) return GAL_WCS_PROJECTION_HPX;
  else if( !strcmp(str, "XPH") ) return GAL_WCS_PROJECTION_XPH;
  else                           return GAL_WCS_PROJECTION_INVALID;
}





char *
gal_wcs_projection_name_from_id(uint8_t id)
{
  switch(id)
    {
    case GAL_WCS_PROJECTION_AZP: return "AZP";
    case GAL_WCS_PROJECTION_SZP: return "SZP";
    case GAL_WCS_PROJECTION_TAN: return "TAN";
    case GAL_WCS_PROJECTION_STG: return "STG";
    case GAL_WCS_PROJECTION_SIN: return "SIN";
    case GAL_WCS_PROJECTION_ARC: return "ARC";
    case GAL_WCS_PROJECTION_ZPN: return "ZPN";
    case GAL_WCS_PROJECTION_ZEA: return "ZEA";
    case GAL_WCS_PROJECTION_AIR: return "AIR";
    case GAL_WCS_PROJECTION_CYP: return "CYP";
    case GAL_WCS_PROJECTION_CEA: return "CEA";
    case GAL_WCS_PROJECTION_CAR: return "CAR";
    case GAL_WCS_PROJECTION_MER: return "MER";
    case GAL_WCS_PROJECTION_SFL: return "SFL";
    case GAL_WCS_PROJECTION_PAR: return "PAR";
    case GAL_WCS_PROJECTION_MOL: return "MOL";
    case GAL_WCS_PROJECTION_AIT: return "AIT";
    case GAL_WCS_PROJECTION_COP: return "COP";
    case GAL_WCS_PROJECTION_COE: return "COE";
    case GAL_WCS_PROJECTION_COD: return "COD";
    case GAL_WCS_PROJECTION_COO: return "COO";
    case GAL_WCS_PROJECTION_BON: return "BON";
    case GAL_WCS_PROJECTION_PCO: return "PCO";
    case GAL_WCS_PROJECTION_TSC: return "TSC";
    case GAL_WCS_PROJECTION_CSC: return "CSC";
    case GAL_WCS_PROJECTION_QSC: return "QSC";
    case GAL_WCS_PROJECTION_HPX: return "HPX";
    case GAL_WCS_PROJECTION_XPH: return "XPH";
    default:                     return GAL_BLANK_STRING;
    }

  /* If control reaches here there is a bug! */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to fix "
        "the problem. Control should not have reached this part of "
        "the function", __func__, PACKAGE_BUGREPORT);
  return GAL_BLANK_STRING;
}




















/*************************************************************
 ***********               Read WCS                ***********
 *************************************************************/
/* It may happen that both the PC+CDELT and CD matrices are present in a
   file. But in some cases, they may not result in the same rotation
   matrix. So we need to let the user know about this problem with the FITS
   file, and as a default behavior, we'll disable the PC matrix (which also
   needs a CDELT matrix (that may not have been written). */
static void
wcs_read_correct_pc_cd(struct wcsprm *wcs)
{
  int removepc=0;
  size_t i, j, naxis=wcs->naxis;
  double *cdfrompc=gal_pointer_allocate(GAL_TYPE_FLOAT64, naxis*naxis, 0,
                                        __func__, "cdfrompc");

  /* A small sanity check. */
  if(wcs->cdelt==NULL)
    error(EXIT_FAILURE, 0, "%s: the WCS structure has no 'cdelt' array, "
          "please contact us at %s to see what the problem is", __func__,
          PACKAGE_BUGREPORT);

  /* Multiply the PC matrix with the CDELT matrix. */
  for(i=0;i<naxis;++i)
    for(j=0;j<naxis;++j)
      cdfrompc[i*naxis+j] = wcs->cdelt[i] * wcs->pc[i*naxis+j];

  /* Make sure the file's CD matrix is the same as the CD matrix that is
     derived from the PC+CDELT matrix above. We'll divide the difference by
     the samller value and if the result is larger than 1e-6, then we'll
     consider it different. */
  for(i=0;i<naxis*naxis;++i)
    if( fabs( wcs->cd[i] - cdfrompc[i] )
        / ( fabs(wcs->cd[i]) < fabs(cdfrompc[i])
            ? fabs(wcs->cd[i])
            : fabs(cdfrompc[i]) )
        > 1e-5 )
      { removepc=1; break; }

  /* If the two matrices are different, then print the warning and remove
     the PC+CDELT matrices to only keep the CD matrix. */
  if(removepc)
    {
      /* Let the user know that there is a problem in the file. */
      error(EXIT_SUCCESS, 0, "the WCS structure has both the PC matrix "
            "and CD matrix. However, the two don't match and there is "
            "no way to know which one was intended by the creator of "
            "this file. THIS PROGRAM WILL ASSUME THE CD MATRIX AND "
            "CONTINUE. BUT THIS MAY BE WRONG! To avoid confusion and "
            "wrong results, its best to only use one of them in your "
            "FITS file. You can use Gnuastro's 'astfits' program to "
            "remove any that you want (please run 'info astfits' for "
            "more). For example if you want to delete the PC matrix "
            "you can use this command: 'astfits file.fits --delete=PC1_1 "
            "--delete=PC1_2 --delete=PC2_1 --delete=PC2_2'");

      /* Set the PC matrix to be equal to the CD matrix, and set the CDELTs
         to 1. */
      for(i=0;i<naxis;++i) wcs->cdelt[i] = 1.0f;
      for(i=0;i<naxis*naxis;++i) wcs->pc[i] = wcs->cd[i];
    }

  /* Clean up. */
  free(cdfrompc);
}





/* For the TPV, TNX and ZPX distortions, WCSLIB can't deal with the CDELT
   keys properly and its better to use the CD matrix instead, this function
   will check for this and return 1 if a CD matrix should be used
   (over-riding the user's desired matrix if necessary). */
static int
wcs_use_cd_for_distortion(struct wcsprm *wcs)
{
  return ( wcs
           && wcs->lin.disseq
           && ( !strcmp(   wcs->lin.disseq->dtype[1], "TPV")
                || !strcmp(wcs->lin.disseq->dtype[1], "TNX")
                || !strcmp(wcs->lin.disseq->dtype[1], "ZPX") ) );
}





/* Read the WCS information from the header. Unfortunately, WCS lib is
   not thread safe, so it needs a mutex. In case you are not using
   multiple threads, just pass a NULL pointer as the mutex.

   After you finish with this WCS, you should free the space with:

   status = wcsvfree(&nwcs,&wcs);

   If the WCS structure is not recognized, then this function will
   return a NULL pointer for the wcsprm structure and a zero for
   nwcs. It will also report the fact to the user in stderr.

   ===================================
   WARNING: wcspih IS NOT THREAD SAFE!
   ===================================
   Don't call this function within a thread or use a mutex.
*/
struct wcsprm *
gal_wcs_read_fitsptr(fitsfile *fptr, int linearmatrix, size_t hstartwcs,
                     size_t hendwcs, int *nwcs)
{
  /* Declaratins: */
  size_t i, fulllen;
  int nkeys=0, status=0;
  int sumcheck, nocomments;
  struct wcsprm *wcs=NULL;
  char *fullheader, *to, *from;
  int fixstatus[NWCSFIX]={0};/* For the various wcsfix checks.          */
  int relax    = WCSHDR_all; /* Macro: use all informal WCS extensions. */
  int ctrl     = 0;          /* Don't report why a keyword wasn't used. */
  int nreject  = 0;          /* Number of keywords rejected for syntax. */
  int fixctrl  = 1;          /* Correct non-standard units in wcsfix.   */
  void *fixnaxis = NULL;     /* For now disable cylfix() with this      */
                             /* (because it depends on image size).     */

  /* In case the user has asked to limit the HDU keyword cards to use for
     WCS reading, also count comment/history/empty lines (so the lines here
     correspond to the output of 'astfits image.fits -h1'). But if no
     limitation is requested, avoid those lines to make the processing
     easier for WCSLIB. */
  nocomments = hendwcs>hstartwcs ? 0 : 1;

  /* CFITSIO function: */
  if( fits_hdr2str(fptr, nocomments, NULL, 0, &fullheader,
                   &nkeys, &status) )
    gal_fits_io_error(status, NULL);

  /* Only consider the header keywords in the current range: */
  if(hendwcs>hstartwcs)
    {
      /* Mark the last character in the desired region. */
      fullheader[hendwcs*(FLEN_CARD-1)]='\0';

      /* For a check:
      printf("%s\n", fullheader);
      */

      /* Shift all the characters to the start of the string. */
      if(hstartwcs)                /* hstartwcs!=0 */
        {
          to=fullheader;
          from=&fullheader[hstartwcs*(FLEN_CARD-1)-1];
          while(*from++!='\0') *to++=*from;
        }

      nkeys=hendwcs-hstartwcs;

      /* For a check:
      printf("\n\n\n###############\n\n\n\n\n\n");
      printf("%s\n", &fullheader[1*(FLEN_CARD-1)]);
      exit(0);
      */
    }

  /* WCSlib function to parse the FITS headers. */
  status=wcspih(fullheader, nkeys, relax, ctrl, &nreject, nwcs, &wcs);
  if(status)
    {
      error(EXIT_SUCCESS, 0, "%s: WCSLIB Warning: wcspih ERROR %d: %s",
            __func__, status, wcs_errmsg[status]);
      wcs=NULL; *nwcs=0;
    }

  /* Set the internal structure: */
  if(wcs)
    {
      /* It may happen that the WCS-related keyword values are stored as
         strings (they have single-quotes around them). In this case,
         WCSLIB will read the CRPIX and CRVAL values as zero. When this
         happens do a small check and abort, while informing the user about
         the problem. */
      sumcheck=0;
      for(i=0;i<wcs->naxis;++i)
        {sumcheck += (wcs->crval[i]==0.0f) + (wcs->crpix[i]==0.0f);}
      if(sumcheck==wcs->naxis*2)
        {
          /* We only care about the first set of characters in each
             80-character row, so we don't need to parse the last few
             characters anyway. */
          fulllen=strlen(fullheader)-12;
          for(i=0;i<fulllen;++i)
            if( strncmp(fullheader+i, "CRVAL1  = '", 11) == 0 )
              fprintf(stderr, "WARNING: WCS Keyword values are not "
                      "numbers.\n\n"
                      "WARNING: The values to the WCS-related keywords "
                      "are enclosed in single-quotes. In the FITS "
                      "standard this is how string values are stored, "
                      "therefore WCSLIB is unable to read them AND WILL "
                      "PUT ZERO IN THEIR PLACE (creating a wrong WCS in "
                      "the output). Please update the respective keywords "
                      "of the input to be numbers (see next line).\n\n"
                      "WARNING: You can do this with Gnuastro's 'astfits' "
                      "program and the '--update' option. The minimal WCS "
                      "keywords that need a numerical value are: "
                      "'CRVAL1', 'CRVAL2', 'CRPIX1', 'CRPIX2', 'EQUINOX' "
                      "and 'CD%%_%%' (or 'PC%%_%%', where the %% are "
                      "integers), please see the FITS standard, and "
                      "inspect your FITS file to identify the full set "
                      "of keywords that you need correct (for example "
                      "PV%%_%% keywords).\n\n");
        }

      /* CTYPE is a mandatory WCS keyword, so if it hasn't been given (its
         '\0'), then the headers didn't have a WCS structure. However,
         WCSLIB still fills in the basic information (for example the
         dimensionality of the dataset). */
      if(wcs->ctype[0][0]=='\0')
        {
          wcsfree(wcs);
          wcs=NULL;
          *nwcs=0;
        }
      else
        {
          /* For a check (we can't use 'wcsprt(wcs)' because this WCS isn't
             yet initialized).
          printf("flag: %d\n", wcs->flag);
          printf("NAXIS: %d\n", wcs->naxis);
          printf("CRPIX: ");
          for(i=0;i<wcs->naxis;++i)
            { printf("%g, ", wcs->crpix[i]); } printf("\n");
          printf("PC: ");
          for(i=0;i<wcs->naxis*wcs->naxis;++i)
            { printf("%g, ", wcs->pc[i]); } printf("\n");
          printf("CDELT: ");
          for(i=0;i<wcs->naxis;++i)
            { printf("%g, ", wcs->cdelt[i]);} printf("\n");
          printf("CD: ");
          for(i=0;i<wcs->naxis*wcs->naxis;++i)
            { printf("%g, ", wcs->cd[i]); } printf("\n");
          printf("CRVAL: ");
          for(i=0;i<wcs->naxis;++i)
            { printf("%g, ", wcs->crval[i]); } printf("\n");
          printf("CUNIT: ");
          for(i=0;i<wcs->naxis;++i)
            { printf("%s, ", wcs->cunit[i]); } printf("\n");
          printf("CTYPE: ");
          for(i=0;i<wcs->naxis;++i)
            { printf("%s, ", wcs->ctype[i]); } printf("\n");
          printf("LONPOLE: %f\n", wcs->lonpole);
          printf("LATPOLE: %f\n", wcs->latpole);
          */

          /* Some datasets may use 'angstroms' (not case-sensitive) in the
             third dimension instead of the standard 'angstrom' (note the
             differing 's'). In this case WCSLIB (atleast until version
             7.3) will not recognize it. We will therefore manually remove
             the 's' before feeding the WCS structure to WCSLIB. */
          if( wcs->naxis==3
              && strlen(wcs->cunit[2])==9
              && !strncasecmp(wcs->cunit[2], "angstroms", 9) )
            wcs->cunit[2][8]='\0';

          /* Fix non-standard WCS features. */
          if( wcsfix(fixctrl, fixnaxis, wcs, fixstatus) )
            {
              if(fixstatus[CDFIX])
                error(0, 0, "%s: (warning) wcsfix status for cdfix: %d",
                      __func__, fixstatus[CDFIX]);
              if(fixstatus[DATFIX])
                error(0, 0, "%s: (warning) wcsfix status for datfix: %d",
                      __func__, fixstatus[DATFIX]);
#if GAL_CONFIG_HAVE_WCSLIB_OBSFIX
              if(fixstatus[OBSFIX])
                error(0, 0, "%s: (warning) wcsfix status for obsfix: %d",
                      __func__, fixstatus[OBSFIX]);
#endif
              if(fixstatus[UNITFIX])
                error(0, 0, "%s: (warning) wcsfix status for unitfix: %d",
                      __func__, fixstatus[UNITFIX]);
              if(fixstatus[SPCFIX])
                error(0, 0, "%s: (warning) wcsfix status for spcfix: %d",
                      __func__, fixstatus[SPCFIX]);
              if(fixstatus[CELFIX])
                error(0, 0, "%s: (warning) wcsfix status for celfix: %d",
                      __func__, fixstatus[CELFIX]);
              if(fixstatus[CYLFIX])
                error(0, 0, "%s: (warning) wcsfix status for cylfix: %d",
                      __func__, fixstatus[CYLFIX]);
            }

          /* Enable wcserr if necessary (can help in debugging situations
             when the default error message isn't enough).  If you confront
             an error, un-comment the two lines below and put
             'wcsperr(wcs,NULL);' after the problematic WCSLIB
             command. This will print a trace-back of the cause.
          wcserr_enable(1);
          wcsprintf_set(stderr);
          */

          /* Set the WCS structure. */
          status=wcsset(wcs);
          if(status)
            {
              error(EXIT_SUCCESS, 0, "%s: WCSLIB warning: wcsset "
                    "error %d: %s", __func__, status, wcs_errmsg[status]);
              wcsfree(wcs);
              wcs=NULL;
              *nwcs=0;
            }
          /* A correctly useful WCS is present. */
          else
            {
              /* According to WCSLIB in discussing 'altlin': "If none of
                 these bits are set the PCi_ja representation results, i.e.
                 wcsprm::pc and wcsprm::cdelt will be used as given". In
                 effect it will also set the PC matrix to unity. So we can
                 safely set it to '1' here because some parts of Gnuastro
                 will later look into this. */
              if(wcs->altlin==0) wcs->altlin=1;

              /* If both the PC and CD matrix have been given, the first
                 two bits of 'altlin' will be '1'. We need to make sure
                 they are the same matrix, and let the user know if they
                 aren't. */
              if( (wcs->altlin & 1) && (wcs->altlin & 2) )
                wcs_read_correct_pc_cd(wcs);
            }
        }
    }

  /* If the distortion requires a CD matrix, or if the user wants it, do
     the conversion here, otherwise, make sure the PC matrix is used. */
  if(wcs_use_cd_for_distortion(wcs) \
     || linearmatrix==GAL_WCS_LINEAR_MATRIX_CD)
    gal_wcs_to_cd(wcs);
  else gal_wcs_decompose_pc_cdelt(wcs);

  /* Clean up and return. */
  status=0;
  if (fits_free_memory(fullheader, &status) )
    gal_fits_io_error(status, "problem in freeing the memory used to "
                      "keep all the headers");
  return wcs;
}





struct wcsprm *
gal_wcs_read(char *filename, char *hdu, int linearmatrix,
             size_t hstartwcs, size_t hendwcs, int *nwcs,
             char *hdu_option_name)
{
  int status=0;
  fitsfile *fptr;
  struct wcsprm *wcs;

  /* Make sure we are dealing with a FITS file. */
  if( gal_fits_file_recognized(filename) == 0 )
    return NULL;

  /* Check HDU for realistic conditions: */
  fptr=gal_fits_hdu_open_format(filename, hdu, 0, hdu_option_name);

  /* Read the WCS information: */
  wcs=gal_wcs_read_fitsptr(fptr, linearmatrix, hstartwcs,
                           hendwcs, nwcs);

  /* Close the FITS file and return. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
  return wcs;
}





struct wcsprm *
gal_wcs_create(double *crpix, double *crval, double *cdelt,
               double *pc, char **cunit, char **ctype,
               size_t ndim, int linearmatrix)
{
  size_t i;
  int status;
  struct wcsprm *wcs;
  double equinox=2000.0f;

  /* Allocate the memory necessary for the wcsprm structure. */
  errno=0;
  wcs=malloc(sizeof *wcs);
  if(wcs==NULL)
    error(EXIT_FAILURE, errno, "%zu for wcs in preparewcs", sizeof *wcs);

  /* Initialize the structure (allocate all its internal arrays). */
  wcs->flag=-1;
  if( (status=wcsini(1, ndim, wcs)) )
    error(EXIT_FAILURE, 0, "wcsini error %d: %s",
          status, wcs_errmsg[status]);

  /* Fill in all the important WCS structure parameters. */
  wcs->altlin  = 0x1;
  wcs->equinox = equinox;
  for(i=0;i<ndim;++i)
    {
      wcs->crpix[i] = crpix[i];
      wcs->crval[i] = crval[i];
      wcs->cdelt[i] = cdelt[i];
      if(cunit[i]) strcpy(wcs->cunit[i], cunit[i]);
      if(ctype[i]) strcpy(wcs->ctype[i], ctype[i]);
    }
  for(i=0;i<ndim*ndim;++i) wcs->pc[i]=pc[i];

  /* Set up the wcs structure with the constants defined above. */
  status=wcsset(wcs);
  if(status)
    error(EXIT_FAILURE, 0, "%s: wcsset error %d: %s", __func__,
          status, wcs_errmsg[status]);

  /* If a CD matrix is desired make it. */
  if(linearmatrix==GAL_WCS_LINEAR_MATRIX_CD)
    gal_wcs_to_cd(wcs);

  /* Return the output WCS. */
  return wcs;
}





void
gal_wcs_free(struct wcsprm *wcs)
{
  int status;

  /* If it is already NULL, then don't continue. */
  if(wcs==NULL) return;

  /* Free the internal structure. */
  status=wcsfree(wcs);
  if(status)
    error(EXIT_FAILURE, 0, "%s: WCSLIB wcsfree ERROR %d: %s",
          __func__, status, wcs_errmsg[status]);

  /* Free the actual structure. */
  free(wcs);
}





/* Extract the dimension name from CTYPE. */
char *
gal_wcs_dimension_name(struct wcsprm *wcs, size_t dimension)
{
  size_t i;
  char *out;

  /* Make sure a WCS pointer actually exists. */
  if(wcs==NULL) return NULL;

  /* Make sure the requested dimension is not larger than the number of
     dimensions in the WCS. */
  if(dimension >= wcs->naxis) return NULL;

  /* Make a copy of the CTYPE value and set the first occurance of '-' to
     '\0', to avoid the projection type. */
  gal_checkset_allocate_copy(wcs->ctype[dimension], &out);
  for(i=0;i<strlen(out);++i) if(out[i]=='-') out[i]='\0';

  /* Return the output array. */
  return out;
}



















/*************************************************************
 ***********               Write WCS               ***********
 *************************************************************/
char *
gal_wcs_write_wcsstr(struct wcsprm *wcs, int *nkeyrec)
{
  char *wcsstr;
  int status=0;

  /* Finalize the linear transformation matrix. Note that some programs may
     have worked on the WCS. So even if 'altlin' is already 2, we'll just
     ensure that the final matrix is CD here. */
  if(wcs->altlin==2 || wcs_use_cd_for_distortion(wcs)) gal_wcs_to_cd(wcs);
  else gal_wcs_decompose_pc_cdelt(wcs);

  /* Clean up small errors in the PC matrix and CDELT values. */
  gal_wcs_clean_errors(wcs);

  /* Convert the WCS information to text. If there was an error, then free
     any allocated space and put zero on 'nkeyrec'. */
  status=wcshdo(WCSHDO_safe, wcs, nkeyrec, &wcsstr);
  if(status)
    {
      error(0, 0, "%s: WARNING: WCSLIB error, no WCS in output.\n"
            "wcshdo ERROR %d: %s", __func__, status, wcs_errmsg[status]);
      if(wcsstr) free(wcsstr);
      *nkeyrec=0;
      wcsstr=NULL;
    }

  /* Return the string. */
  return wcsstr;
}





void
gal_wcs_write_in_fitsptr(fitsfile *fptr, struct wcsprm *wcs)
{
  char *wcsstr;
  int nkeyrec=0;

  /* Convert the WCS structure into FITS keywords as a string. */
  wcsstr=gal_wcs_write_wcsstr(wcs, &nkeyrec);

  /* Write the keywords into the FITS file. */
  gal_fits_key_write_wcsstr(fptr, wcs, wcsstr, nkeyrec);
  free(wcsstr);
}





void
gal_wcs_write(struct wcsprm *wcs, char *filename,
              char *extname, gal_fits_list_key_t *headers,
              char *program_string)
{
  int status=0;
  size_t ndim=0;
  fitsfile *fptr;
  long *naxes=NULL;

  /* Small sanity checks. */
  if(wcs==NULL)
    error(EXIT_FAILURE, 0, "%s: input WCS is NULL", __func__);
  if( gal_fits_file_recognized(filename)==0 )
    error(EXIT_FAILURE, 0, "%s: not a FITS suffix", filename);

  /* Open the file for writing. */
  fptr=gal_fits_open_to_write(filename);

  /* Create the FITS file. */
  fits_create_img(fptr, gal_fits_type_to_bitpix(GAL_TYPE_UINT8),
                  ndim, naxes, &status);
  gal_fits_io_error(status, NULL);

  /* Remove the two comment lines put by CFITSIO. Note that in some cases,
     it might not exist. When this happens, the status value will be
     non-zero. We don't care about this error, so to be safe, we will just
     reset the status variable after these calls. */
  fits_delete_key(fptr, "COMMENT", &status);
  fits_delete_key(fptr, "COMMENT", &status);
  status=0;

  /* If an extension name was requested, add it. */
  if(extname)
    fits_write_key(fptr, TSTRING, "EXTNAME", extname, "", &status);

  /* Write the WCS structure. */
  gal_wcs_write_in_fitsptr(fptr, wcs);

  /* Write all the headers and the version information. */
  gal_fits_key_write_version_in_ptr(&headers, program_string, fptr);

  /* Close the FITS file. */
  fits_close_file(fptr, &status);
  gal_fits_io_error(status, NULL);
}




















/*************************************************************
 ***********           Coordinate system           ***********
 *************************************************************/
/* Identify the coordinate system of the WCS. */
int
gal_wcs_coordsys_identify(struct wcsprm *wcs)
{
  /* Equatorial (we are keeping the dash ('-') to make sure it is a
     standard). */
  if ( !strncmp(wcs->ctype[0], "RA---", 5)
       && !strncmp(wcs->ctype[1], "DEC--", 5) )
    {
      if ( !strncmp(wcs->radesys, "FK4", 3) )
        return GAL_WCS_COORDSYS_EQB1950;
      else if ( !strncmp(wcs->radesys, "FK5", 3) )
        return GAL_WCS_COORDSYS_EQJ2000;
      else
        error(EXIT_FAILURE, 0, "%s: the '%s' value for 'RADESYS' is "
              "not yet implemented! Please contact us at %s to "
              "implement it", __func__, wcs->radesys, PACKAGE_BUGREPORT);
    }

  /* Ecliptic. */
  else if ( !strncmp(wcs->ctype[0], "ELON-", 5)
            && !strncmp(wcs->ctype[1], "ELAT-", 5) )
    if ( !strncmp(wcs->radesys, "FK4", 3) )
      return GAL_WCS_COORDSYS_ECB1950;
    else if ( !strncmp(wcs->radesys, "FK5", 3) )
      return GAL_WCS_COORDSYS_ECJ2000;
    else
      error(EXIT_FAILURE, 0, "%s: the '%s' value for 'RADESYS' is "
            "not yet implemented! Please contact us at %s to "
            "implement it", __func__, wcs->radesys, PACKAGE_BUGREPORT);

  /* Galactic. */
  else if ( !strncmp(wcs->ctype[0], "GLON-", 5)
            && !strncmp(wcs->ctype[1], "GLAT-", 5) )
    return GAL_WCS_COORDSYS_GALACTIC;

  /* SuperGalactic. */
  else if ( !strncmp(wcs->ctype[0], "SLON-", 5)
            && !strncmp(wcs->ctype[1], "SLAT-", 5) )
    return GAL_WCS_COORDSYS_SUPERGALACTIC;

  /* Other. */
  else
    error(EXIT_FAILURE, 0, "%s: the CTYPE values '%s' and '%s' are "
          "not yet implemented! Please contact us at %s to "
          "implement it", __func__, wcs->ctype[0], wcs->ctype[1],
          PACKAGE_BUGREPORT);

  /* Control should not reach here. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
        "problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
  return GAL_WCS_COORDSYS_INVALID;
}





/* Set the pole coordinates (current values taken from the WCSLIB
   manual):
      lng2p1: longitude (in system 2) of the pole of system 1.
      lat2p1: latitude  (in system 2) of the pole of system 1.
      lng1p2: longitude (in system 1) of the pole of system 2.

   Values from NED (inspired by WCSLIB manual's example).
   https://ned.ipac.caltech.edu/coordinate_calculator
   With the following constraints:
    - The "Observation epoch" is the same as "Equinox".
    - The "Position angle (East of North) is set to 0.0.

        longi (deg)  latit (deg)  OUTPUT                 INPUT
        -----        -----        ------                 -----
      (------------, -----------) B1950 equ.  coords. of B1950 equ.  pole.
      (180.31684301, 89.72174782) J2000 equ.  coords. of B1950 equ.  pole.
      (90.000000000, 66.55421111) B1950 ecl.  coords. of B1950 equ.  pole.
      (90.699521110, 66.56068919) J2000 ecl.  coords. of B1950 equ.  pole.
      (123.00000000, 27.40000000) Galactic    coords. of B1950 equ.  pole.
      (26.731537070, 15.64407736) Supgalactic coords. of B1950 equ.  pole.

      (359.68621044, 89.72178502) B1950 equ.  coords. of J2000 equ.  pole.
      (------------, -----------) J2000 equ.  coords. of J2000 equ.  pole.
      (89.300755510, 66.55417728) B1950 ecl.  coords. of J2000 equ.  pole.
      (90.000000000, 66.56070889) J2000 ecl.  coords. of J2000 equ.  pole.
      (122.93200023, 27.12843056) Galactic    coords. of J2000 equ.  pole.
      (26.450516650, 15.70886131) Supgalactic coords. of J2000 equ.  pole.

      (270.00000000, 66.55421111) B1950 equ.  coords. of B1950 ecl.  pole.
      (269.99920697, 66.55421892) J2000 equ.  coords. of B1950 ecl.  pole.
      (------------, -----------) B1950 ecl.  coords. of B1950 ecl.  pole.
      (267.21656404, 89.99350237) J2000 ecl.  coords. of B1950 ecl.  pole.
      (96.376479150, 29.81195400) Galactic    coords. of B1950 ecl.  pole.
      (33.378919140, 38.34766498) Supgalactic coords. of B1950 ecl.  pole.

      (270.00099211, 66.56069675) B1950 equ.  coords. of J2000 ecl.  pole.
      (270.00000000, 66.56070889) J2000 equ.  coords. of J2000 ecl.  pole.
      (86.517962160, 89.99350236) B1950 ecl.  coords. of J2000 ecl.  pole.
      (------------, -----------) J2000 ecl.  coords. of J2000 ecl.  pole.
      (96.383958840, 29.81163604) Galactic    coords. of J2000 ecl.  pole.
      (33.376119480, 38.34154959) Supgalactic coords. of J2000 ecl.  pole.

      (192.25000000, 27.40000000) B1950 equ.  coords. of Galactic    pole.
      (192.85949646, 27.12835323) J2000 equ.  coords. of Galactic    pole.
      (179.32094769, 29.81195400) B1950 ecl.  coords. of Galactic    pole.
      (180.02317894, 29.81153742) J2000 ecl.  coords. of Galactic    pole.
      (------------, -----------) Galactic    coords. of Galactic    pole.
      (90.000000000, 6.320000000) Supgalactic coords. of Galactic    pole.

      (283.18940711, 15.64407736) B1950 equ.  coords. of SupGalactic pole.
      (283.75420420, 15.70894043) J2000 equ.  coords. of SupGalactic pole.
      (286.26975051, 38.34766498) B1950 ecl.  coords. of SupGalactic pole.
      (286.96654469, 38.34158720) J2000 ecl.  coords. of SupGalactic pole.
      (47.370000000, 6.320000000) Galactic    coords. of SupGalactic pole.
      (------------, -----------) Supgalactic coords. of SupGalactic pole.
 */
static void
wcs_coordsys_sys1_pole_in_sys2(int sys1, int sys2, double *lng2p1,
                               double *lat2p1, double *lng1p2)
{
  switch( sys1 )
    {
    case GAL_WCS_COORDSYS_EQB1950:
      switch( sys2)
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2p1=NAN;          *lat2p1=NAN;         *lng1p2=NAN;          return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2p1=180.31684301; *lat2p1=89.72174782; *lng1p2=359.68621044; return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2p1=90.000000000; *lat2p1=66.55421111; *lng1p2=270.00000000; return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2p1=90.699521110; *lat2p1=66.56068919; *lng1p2=270.00099211; return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2p1=123.00000000; *lat2p1=27.40000000; *lng1p2=192.25000000; return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2p1=26.731537070; *lat2p1=15.64407736; *lng1p2=283.18940711; return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input EQB1950)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;
    case GAL_WCS_COORDSYS_EQJ2000:
      switch( sys2)
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2p1=359.68621044; *lat2p1=89.72178502; *lng1p2=180.31684301; return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2p1=NAN;          *lat2p1=NAN;         *lng1p2=NAN;          return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2p1=89.300755510; *lat2p1=66.55417728; *lng1p2=269.99920697; return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2p1=90.000000000; *lat2p1=66.56070889; *lng1p2=270.00000000; return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2p1=122.93200023; *lat2p1=27.12843056; *lng1p2=192.85949646; return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2p1=26.450516650; *lat2p1=15.70886131; *lng1p2=283.75420420; return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input EQJ2000)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;
    case GAL_WCS_COORDSYS_ECB1950:
      switch( sys2)
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2p1=270.00000000; *lat2p1=66.55421111; *lng1p2=90.000000000; return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2p1=269.99920697; *lat2p1=66.55421892; *lng1p2=89.300755510; return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2p1=NAN;          *lat2p1=NAN;         *lng1p2=NAN;          return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2p1=267.21656404; *lat2p1=89.99350237; *lng1p2=86.517962160; return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2p1=96.383958840; *lat2p1=29.81163604; *lng1p2=179.32094769; return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2p1=33.378919140; *lat2p1=38.34766498; *lng1p2=286.26975051; return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input ECB1950)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;
    case GAL_WCS_COORDSYS_ECJ2000:
      switch( sys2)
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2p1=270.00099211; *lat2p1=66.56069675; *lng1p2=90.699521110; return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2p1=270.00000000; *lat2p1=66.56070889; *lng1p2=90.000000000; return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2p1=86.517962160; *lat2p1=89.99350236; *lng1p2=267.21656404; return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2p1=NAN;          *lat2p1=NAN;         *lng1p2=NAN;          return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2p1=96.383958840; *lat2p1=29.81163604; *lng1p2=180.02317894; return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2p1=33.376119480; *lat2p1=38.34154959; *lng1p2=286.96654469; return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input ECJ2000)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;
    case GAL_WCS_COORDSYS_GALACTIC:
      switch( sys2)
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2p1=192.25000000; *lat2p1=27.40000000; *lng1p2=123.00000000; return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2p1=192.85949646; *lat2p1=27.12835323; *lng1p2=122.93200023; return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2p1=179.32094769; *lat2p1=29.81195400; *lng1p2=96.376479150; return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2p1=180.02317894; *lat2p1=29.81153742; *lng1p2=96.383958840; return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2p1=NAN;          *lat2p1=NAN;         *lng1p2=NAN;          return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2p1=90.000000000; *lat2p1=6.320000000; *lng1p2=47.370000000; return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input GALACTIC)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;
    case GAL_WCS_COORDSYS_SUPERGALACTIC:
      switch( sys2)
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2p1=283.18940711; *lat2p1=15.64407736; *lng1p2=26.731537070; return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2p1=283.75420420; *lat2p1=15.70894043; *lng1p2=26.450516650; return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2p1=286.26975051; *lat2p1=38.34766498; *lng1p2=33.378919140; return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2p1=286.96654469; *lat2p1=38.34158720; *lng1p2=33.376119480; return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2p1=47.370000000; *lat2p1=6.320000000; *lng1p2=90.000000000; return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2p1=NAN;          *lat2p1=NAN;         *lng1p2=NAN;          return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input SUPERGALACTIC)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the problem. The code '%d' isn't a recognized WCS "
            "coordinate system ID for 'sys1'", __func__,
            PACKAGE_BUGREPORT, sys1);
    }
}





/* Return the longitude (RA in equatorial) and latitude (Dec in equatorial)
   coordinates of the reference point (on the equator) of system 1 in the
   2nd system. For example, if you want the galactic center's RA and Dec:
   'sys1=GAL_WCS_COORDSYS_GALACTIC' and 'sys2=GAL_WCS_COORDSYS_EQJ2000'.

   Similar to 'wcs_coordsys_sys1_pole_in_sys2', the numbers reported here
   come from NED. Here, the following extra settings were set (they were
   not recorded in the comments when 'wcs_coordsys_sys1_pole_in_sys2' was
   defined).

      - The "Observation epoch" has been set to the same year as the
        equinox.
      - The "Position angle (East of North)" is set to 0.
*/
void
gal_wcs_coordsys_sys1_ref_in_sys2(int sys1, int sys2, double *lng2,
                                  double *lat2)
{
  switch( sys1 )
    {
    case GAL_WCS_COORDSYS_EQB1950:
      switch( sys2 )
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2=NAN;           *lat2=NAN;           return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2=0.64069119;    *lat2=0.27839567;    return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2=0.00000000;    *lat2=0.00000000;    return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2=0.69855979;    *lat2=0.00057803;    return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2=97.74216087;   *lat2=-60.18102400;  return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2=293.11549599;  *lat2=12.69249218;   return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input EQB1950)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;

    case GAL_WCS_COORDSYS_EQJ2000:
      switch( sys2 )
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2=359.35927364;  *lat2=-0.27832018;   return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2=NAN;           *lat2=NAN;           return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2=359.30143791;  *lat2=-0.00041556;   return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2=0.0000000000;  *lat2=0.000000000;   return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2=96.33723581;   *lat2=-60.18845577;   return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2=292.65879220;  *lat2=13.23092157;    return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input EQJ2000)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;

    case GAL_WCS_COORDSYS_ECB1950:
      switch( sys2 )
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2=0.0000000000;  *lat2=0.000000000;   return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2=0.64069119;    *lat2=0.27839567;    return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2=NAN;           *lat2=NAN;           return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2=0.69855979;    *lat2=0.00057803;    return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2=97.74216087;   *lat2=-60.18102400;  return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2=293.11549599;  *lat2=12.69249218;   return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input ECB1950)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;

    case GAL_WCS_COORDSYS_ECJ2000:
      switch( sys2 )
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2=359.35927364;  *lat2=-0.27832018;   return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2=0.0000000000;  *lat2=0.000000000;   return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2=359.30143791;  *lat2=-0.00041556;   return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2=NAN;           *lat2=NAN;           return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2=96.33723581;   *lat2=-60.18845577;  return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2=292.65879220;  *lat2=13.23092157 ;  return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input ECJ2000)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;

    case GAL_WCS_COORDSYS_GALACTIC:
      switch( sys2 )
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2=265.61084403;  *lat2=-28.91679035;  return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2=266.40506655;  *lat2=-28.93616241;  return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2=266.14096542;  *lat2=-5.52979411;   return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2=266.83958692;  *lat2=-5.53630157;   return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2=NAN;           *lat2=NAN;           return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2=185.78610785;  *lat2=42.31028736;   return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input GALACTIC)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;

    case GAL_WCS_COORDSYS_SUPERGALACTIC:
      switch( sys2 )
        {
        case GAL_WCS_COORDSYS_EQB1950:
          *lng2=41.35517115;  *lat2=59.32090820;   return;
        case GAL_WCS_COORDSYS_EQJ2000:
          *lng2=42.30997710;  *lat2=59.52821263;   return;
        case GAL_WCS_COORDSYS_ECB1950:
          *lng2=59.54957645;  *lat2=40.91184040;   return;
        case GAL_WCS_COORDSYS_ECJ2000:
          *lng2=60.24554909;  *lat2=40.91764242;   return;
        case GAL_WCS_COORDSYS_GALACTIC:
          *lng2=137.37000000;  *lat2=0.00000000;   return;
        case GAL_WCS_COORDSYS_SUPERGALACTIC:
          *lng2=NAN;  *lat2=NAN;   return;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
                "fix the problem. The code '%d' isn't a recognized WCS "
                "coordinate system ID for 'sys2' (input SUPERGALACTIC)",
                __func__, PACKAGE_BUGREPORT, sys2);
        }
      break;

    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the problem. The code '%d' isn't a recognized WCS "
            "coordinate system ID for 'sys1'", __func__,
            PACKAGE_BUGREPORT, sys1);
    }
}





static void
wcs_coordsys_ctypes(int coordsys, char **clng, char **clat,
                    char **radesys, double *equinox)
{
  switch( coordsys)
    {
    case GAL_WCS_COORDSYS_EQB1950:
      *clng="RA";   *clat="DEC";  *radesys="FK4"; *equinox=1950; break;
    case GAL_WCS_COORDSYS_EQJ2000:
      *clng="RA";   *clat="DEC";  *radesys="FK5"; *equinox=2000; break;
    case GAL_WCS_COORDSYS_ECB1950:
      *clng="ELON"; *clat="ELAT"; *radesys="FK4"; *equinox=1950; break;
    case GAL_WCS_COORDSYS_ECJ2000:
      *clng="ELON"; *clat="ELAT"; *radesys="FK5"; *equinox=2000; break;
    case GAL_WCS_COORDSYS_GALACTIC:
      *clng="GLON"; *clat="GLAT"; *radesys=NULL;  break;
    case GAL_WCS_COORDSYS_SUPERGALACTIC:
      *clng="SLON"; *clat="SLAT"; *radesys=NULL;  break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the problem. The code '%d' isn't a recognized WCS "
            "coordinate system ID for 'coordsys'", __func__,
            PACKAGE_BUGREPORT, coordsys);
    }
}




/* Convert the coordinate system. */
struct wcsprm *
gal_wcs_coordsys_convert(struct wcsprm *wcs, int outcoordsys)
{
  int incoordsys;
  char *alt=NULL;                 /* Only concerned with primary wcs. */
  double equinox=0.0f;            /* To preserve current value.       */
  struct wcsprm *out=NULL;
  double lng2p1=NAN, lat2p1=NAN, lng1p2=NAN;
  char *clng=NULL, *clat=NULL, *radesys=NULL;

  /* Just incase the input is a NULL pointer. */
  if(wcs==NULL) return NULL;

  /* Get the input's coordinate system and see if it should be converted at
     all or not (if the output coordinate system is different). If its the
     same, just copy the input and return. */
  incoordsys=gal_wcs_coordsys_identify(wcs);
  if(incoordsys==outcoordsys)
    {
      out=gal_wcs_copy(wcs);
      return out;
    }

  /* Find the necessary pole coordinates. Note that we have already
     accounted for the fact that the input and output coordinate systems
     may be the same above, so the NaN outputs will never occur here. */
  wcs_coordsys_sys1_pole_in_sys2(incoordsys, outcoordsys,
                                 &lng2p1, &lat2p1, &lng1p2);

  /* Find the necessary CTYPE names of the output. */
  wcs_coordsys_ctypes(outcoordsys, &clng, &clat, &radesys, &equinox);

  /* Convert the WCS's coordinate system (if 'wcsccs' is available). */
#if GAL_CONFIG_HAVE_WCSLIB_WCSCCS
  out=gal_wcs_copy(wcs);
  wcsccs(out, lng2p1, lat2p1, lng1p2, clng, clat, radesys, equinox, alt);
#else

  /* Just to avoid compiler warnings for 'equinox' and 'alt' (when WCSLIB
     doesn't have the 'wcsccs' function): these will never be used: if
     control comes here the function will abort! So don't worry about this
     command having any kind of effect on the progrma. */
  if(alt) lng2p1+=equinox;

  /* Print error message and abort. */
  error(EXIT_FAILURE, 0, "%s: the 'wcsccs' function isn't available "
        "in the version of WCSLIB that this Gnuastro was built with "
        "('wcsccs' was first available in WCSLIB 7.5, released on "
        "March 2021). Therefore, Gnuastro can't preform the WCS "
        "coordiante system conversion in the WCS. Please update your "
        "WCSLIB and re-build Gnuastro with it to use this feature. "
        "You can follow the instructions here to install the latest "
        "version of WCSLIB:\n"
        "   https://www.gnu.org/software/gnuastro/manual/html_node/"
        "WCSLIB.html\n"
        "And then re-build Gnuastro as described here:\n"
        "   https://www.gnu.org/software/gnuastro/manual/"
        "html_node/Quick-start.html\n\n",
        __func__);
#endif

  /* Return. */
  return out;
}





/* Convert the coordinates of a series of points from one celestial
   coordinate system to another.

   This is based on the equations in this Wikipedia article:
   https://en.wikipedia.org/wiki/Galactic_coordinate_system#Conversion_between_equatorial_and_galactic_coordinates
   A mathematical proof of this equation is present in this StackExchange
   answer:
   https://physics.stackexchange.com/questions/88663/converting-between-galactic-and-ecliptic-coordinates


   The angle names are taken from this solution (for example "BK" is shown
   with the variable 'bk_r', since it is in radians). Intermediate angles
   (that the user never seen by the caller) are in radians and have a '_r'
   suffix. The angles that the caller gives and recieves are in degrees
   with a '_d' suffix.
 */
void
gal_wcs_coordsys_convert_points(int sys1, double *lng1_d, double *lat1_d,
                                int sys2, double *lng2_d, double *lat2_d,
                                size_t number)
{
  size_t i;
  double coslat1, coslat2;
  double lngdiff, coslngdiff, coslat1p2;
  double lng1_r, lat1_r, lng2_r, lat2_r;
  double lng2p1diff_r, lng2p1diff_sin_r, lng2p1diff_cos_r;

  /* Format: 'aaaNcM':
      - aaa either 'lng' (for longitude) or 'lat' (for latitude).
      - N   either 1 or 2 (for the coordinate system of 'aaa').
      - c   either 'p' (for pole) or 'r' (for reference point).
      - M   either 1 or 2 (for the coordinate system of 'c').   */
  double lng1p2_d, lat1p2_d, lng2p1_d;
  double lng1p2_r, lat1p2_r, lng2p1_r;

  /* In case the input and output coordinate systems are the same, just
     copy the first into the second coordinates and return (no need to do
     any conversion). */
  if(sys1==sys2)
    {
      for(i=0;i<number;++i) {lng2_d[i]=lng1_d[1]; lat2_d[i]=lat1_d[1];}
      return;
    }

  /* Get the second system's pole and reference point (on its equator) in
     the first system. */
  wcs_coordsys_sys1_pole_in_sys2(sys2, sys1, &lng1p2_d, &lat1p2_d,
                                 &lng2p1_d);
  lng1p2_r=lng1p2_d*M_PI/180.0f;
  lat1p2_r=lat1p2_d*M_PI/180.0f;
  lng2p1_r=lng2p1_d*M_PI/180.0f;

  /* Loop over all the coordinates. */
  coslat1p2=cos(lat1p2_r);
  for(i=0;i<number;++i)
    {
      /* Convert the input coordinates into radians. */
      lng1_r = lng1_d[i] * M_PI / 180.0f;
      lat1_r = lat1_d[i] * M_PI / 180.0f;

      /* The latitude in the output coordinate is easy to calculate: */
      lngdiff=lng1_r-lng1p2_r;
      coslngdiff=cos(lngdiff);
      lat2_r = asin( sin(lat1p2_r)*sin(lat1_r)
                     + coslat1p2*cos(lat1_r)*coslngdiff );

      /* We can now calculate the angle between 'PG' and 'GR' ('122.9 - l'
         in the webpage above), we'll call it 'pggr_r' here. But there is a
         problem: we need both the sine and cosine of 'pggr_r' to get its
         position across the whole 0 to 360 degrees ('asin()' only returns
         a value from -pi/2 to pi/2 and 'acos()' only returns a value
         between 0 to pi. */
      coslat1 = cos(lat1_r);
      coslat2 = cos(lat2_r);
      lng2p1diff_sin_r = asin( coslat1 * sin(lngdiff) / coslat2 );
      lng2p1diff_cos_r = acos( ( coslat1p2*sin(lat1_r)
                           - sin(lat1p2_r)*coslat1*coslngdiff )
                         / coslat2 );

      /* We have assumed that 'lng2p1diff_r = lng2p1_r - lng2_r'; so
         'lng2_r = lng2p1_r - lng2p1diff_r' (when
         'lng2p1diff_sin_r>0'). When 'lng2p1diff_sin_r<0', the cosine needs
         to be negative (so in the end it becomes 'lng2_r = lng2p1_r +
         lng2p1diff_r'. */
      lng2p1diff_r = ( lng2p1diff_sin_r<0
                       ? -1*lng2p1diff_cos_r
                       : lng2p1diff_cos_r );
      lng2_r = lng2p1_r - lng2p1diff_r;

      /* In case the longitude is larger than 2pi, then we should subtract
         2*pi from it and if it is negative, we should add 2pi to it. */
      if(lng2_r>2*M_PI)  lng2_r=lng2_r-2*M_PI;
      else if (lng2_r<0) lng2_r+=2*M_PI;

      /* Write the values in the output as degrees. */
      lng2_d[i]=lng2_r*180.0f/M_PI;
      lat2_d[i]=lat2_r*180.0f/M_PI;
    }
}


















/*************************************************************
 ***********              Distortions              ***********
 *************************************************************/
/* Check the type of distortion present and return the appropriate
   integer based on `enum gal_wcs_distortion`.

   Parameters:
    struct wcsprm *wcs - The wcs parameters of the fits file.

   Return:
    int out_distortion - The type of distortion present. */
int
gal_wcs_distortion_identify(struct wcsprm *wcs)
{
#if GAL_CONFIG_HAVE_WCSLIB_DIS_H
  struct disprm *dispre=NULL;
  struct disprm *disseq=NULL;

  /* Small sanity check. */
  if(wcs==NULL) return GAL_WCS_DISTORTION_INVALID;

  /* To help in reading. */
  disseq=wcs->lin.disseq;
  dispre=wcs->lin.dispre;

  /* Check if distortion present. */
  if( disseq==NULL && dispre==NULL ) return GAL_WCS_DISTORTION_INVALID;

  /* Check the type of distortion.

     As mentioned in the WCS paper IV section 2.4.2 available at
     https://www.atnf.csiro.au/people/mcalabre/WCS/dcs_20040422.pdf, the
     DPja and DQia keywords are used to record the parameters required by
     the prior and sequent distortion functions respectively.

     Now, as mentioned in dis.h file reference section in WCSLIB manual
     given here
     https://www.atnf.csiro.au/people/mcalabre/WCS/wcslib/dis_8h.html, TPV,
     DSS, and WAT are sequent polynomial distortions, while SIP is prior
     polynomial distortion. TPD is a superset of all these distortions and
     hence can be used both as a prior and sequent distortion polynomial.

     References and citations:
     "Representations of distortions in FITS world coordinate systems",
     Calabretta, M.R. et al. (WCS Paper IV, draft dated 2004/04/22)
      */

  if( dispre != NULL )
    {
      if(      !strcmp(*dispre->dtype, "SIP") )
        return GAL_WCS_DISTORTION_SIP;
      else if( !strcmp(*dispre->dtype, "TPD") )
        return GAL_WCS_DISTORTION_TPD;
      else
        return GAL_WCS_DISTORTION_INVALID;
    }
  else if( disseq != NULL )
    {
      if(      !strcmp(*disseq->dtype, "TPV") )
        return GAL_WCS_DISTORTION_TPV;
      else if( !strcmp(*disseq->dtype, "TPD") )
        return GAL_WCS_DISTORTION_TPD;
      else if( !strcmp(*disseq->dtype, "DSS") )
        return GAL_WCS_DISTORTION_DSS;
      else if( !strcmp(*disseq->dtype, "WAT") )
        return GAL_WCS_DISTORTION_WAT;
      else
        return GAL_WCS_DISTORTION_INVALID;
    }

  /* Control should not reach here. */
  error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to fix "
        "the problem. Control should not reach the end of this function",
        __func__, PACKAGE_BUGREPORT);
#else
  /* The 'wcslib/dis.h' isn't present. */
  error(EXIT_FAILURE, 0, "%s: the installed version of WCSLIB on this "
        "system doesn't have the 'dis.h' header! Thus Gnuastro can't do "
        "distortion-related operations on the world coordinate system "
        "(WCS). To use these features, please upgrade your version "
        "of WCSLIB and re-build Gnuastro", __func__);
#endif
  return GAL_WCS_DISTORTION_INVALID;
}










/* Convert a given distrotion type to other.

  Parameters:
    struct wcsprm *wcs - The wcs parameters of the fits file.
    int out_distortion - The desired output distortion.
    size_t* fitsize    - The size of the array along each dimension.

  Return:
    struct wcsprm *outwcs - The transformed wcs parameters in the
                            required distortion type. */
struct wcsprm *
gal_wcs_distortion_convert(struct wcsprm *inwcs, int outdisptype,
                           size_t *fitsize)
{
#if GAL_CONFIG_HAVE_WCSLIB_DIS_H
  struct wcsprm *outwcs=NULL;
  int indisptype=gal_wcs_distortion_identify(inwcs);

  /* Make sure we have a PC+CDELT structure in the input WCS. */
  gal_wcs_decompose_pc_cdelt(inwcs);

  /* If the input and output types are the same, just copy the input,
     otherwise, do the conversion. */
  if(indisptype==outdisptype) outwcs=gal_wcs_copy(inwcs);
  else
    switch(indisptype)
      {
      /* If there is no distortion in the input, just return a
         newly-allocated copy. */
      case GAL_WCS_DISTORTION_INVALID: outwcs=gal_wcs_copy(inwcs); break;

      /* Input's distortion is SIP. */
      case GAL_WCS_DISTORTION_SIP:
        switch(outdisptype)
          {
          case GAL_WCS_DISTORTION_TPV:
            outwcs=gal_wcsdistortion_sip_to_tpv(inwcs);
            break;
          default:
            error(EXIT_FAILURE, 0, "%s: conversion from %s to %s is not "
                  "yet supported. Please contact us at '%s'", __func__,
                  gal_wcs_distortion_name_from_id(indisptype),
                  gal_wcs_distortion_name_from_id(outdisptype),
                  PACKAGE_BUGREPORT);
              }
        break;

      /* Input's distortion is TPV. */
      case GAL_WCS_DISTORTION_TPV:
        switch(outdisptype)
          {
          case GAL_WCS_DISTORTION_SIP:
            if(fitsize==NULL)
              error(EXIT_FAILURE, 0, "%s: the size array is necessary "
                    "for this conversion", __func__);
            outwcs=gal_wcsdistortion_tpv_to_sip(inwcs, fitsize);
            break;
          default:
            error(EXIT_FAILURE, 0, "%s: conversion from %s to %s is not "
                  "yet supported. Please contact us at '%s'", __func__,
                  gal_wcs_distortion_name_from_id(indisptype),
                  gal_wcs_distortion_name_from_id(outdisptype),
                  PACKAGE_BUGREPORT);
          }
        break;

      /* Input's distortion is not yet supported. */
      case GAL_WCS_DISTORTION_TPD:
      case GAL_WCS_DISTORTION_DSS:
      case GAL_WCS_DISTORTION_WAT:
        error(EXIT_FAILURE, 0, "%s: input %s distortion is not yet "
              "supported. Please contact us at '%s'", __func__,
              gal_wcs_distortion_name_from_id(indisptype),
              PACKAGE_BUGREPORT);

      /* A bug! This distortion is not yet recognized. */
      default:
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
              "fix the problem. The identifier '%d' is not recognized "
              "as a distortion", __func__, PACKAGE_BUGREPORT, indisptype);
      }

  /* Return the converted WCS. */
  return outwcs;
#else
  /* The 'wcslib/dis.h' isn't present. */
  error(EXIT_FAILURE, 0, "%s: the installed version of WCSLIB on this "
        "system doesn't have the 'dis.h' header! Thus Gnuastro can't do "
        "distortion-related operations on the world coordinate system "
        "(WCS). To use these features, please upgrade your version "
        "of WCSLIB and re-build Gnuastro", __func__);
  return NULL;
#endif
}





















/**************************************************************/
/**********              Utilities                 ************/
/**************************************************************/
/* Copy a given WSC structure into another one. */
struct wcsprm *
gal_wcs_copy(struct wcsprm *wcs)
{
  struct wcsprm *out;

  /* If the input WCS is NULL, return a NULL WCS. */
  if(wcs)
    {
      /* Allocate the output WCS structure. */
      errno=0;
      out=malloc(sizeof *out);
      if(out==NULL)
        error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for 'out'",
              __func__, sizeof *out);

      /* Initialize the allocated WCS structure. The WCSLIB manual says "On
         the first invokation, and only the first invokation, wcsprm::flag
         must be set to -1 to initialize memory management". */
      out->flag=-1;
      wcsini(1, wcs->naxis, out);

      /* Copy the input WCS to the output WSC structure. */
      wcscopy(1, wcs, out);
    }
  else
    out=NULL;

  /* Return the final output. */
  return out;
}





/* Copy the given WCS into a new one with differnet CRVALs. Because WCSLIB
   keeps internal constructs to speed up operations, we cannot simply
   change these values, we need to write the WCS into a set of keywords,
   then read them as a new one. */
struct wcsprm *
gal_wcs_copy_new_crval(struct wcsprm *in, double *crval)
{
  char *wcsstr;
  double *ocrval;
  int status=0, relax=WCSHDR_all;
  struct wcsprm *incpy, *out=NULL;
  int nkeys, nwcs, ctrl=0, nreject=0;

  /* Copy the input WCS structure into a new one (in case the caller is
     using it in another function). */
  incpy=gal_wcs_copy(in);

  /* Keep the original pointer of CRVAL: for some reason 'wcsprm' cannot
     deal with a 'NULL' CRVAL, so we need to return the original CRVAL to
     the 'wcsprm' before freeing it (we do not want to free the caller's
     CRVAL). */
  ocrval=incpy->crval;
  incpy->crval=crval;

  /* Convert the WCS into a string and read it into a new WCS structure. We
     do not need all the checks in 'gal_wcs_read_fitsptr' because this
     string was written just now. */
  wcsstr=gal_wcs_write_wcsstr(incpy, &nkeys);
  status=wcspih(wcsstr, nkeys, relax, ctrl, &nreject, &nwcs, &out);
  if(status)
    error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to fix the "
          "problem. The internally created WCS string could not be parsed "
          "as a new WCS structure", __func__, PACKAGE_BUGREPORT);

  /* Clean up and return. */
  incpy->crval=ocrval;
  gal_wcs_free(incpy);
  free(wcsstr);
  return out;
}





/* Remove the algorithm part of CTYPE (anything after, and including, a
   '-') if necessary. */
static void
wcs_ctype_noalgorithm(char *str)
{
  size_t i, len=strlen(str);
  for(i=0;i<len;++i) if(str[i]=='-') { str[i]='\0'; break; }
}




/* See if the CTYPE string ends with TAN. */
static int
wcs_ctype_has_tan(char *str)
{
  size_t len=strlen(str);

  return !strcmp(&str[len-3], "TAN");
}





/* Remove dimension. */
#define WCS_REMOVE_DIM_CHECK 0
void
gal_wcs_remove_dimension(struct wcsprm *wcs, size_t fitsdim)
{
  size_t c, i, j, naxis;

  /* If the WCS structure is NULL, just return. */
  if(wcs==NULL) return;

  /* Sanity check. */
  naxis=wcs->naxis;
  if(fitsdim==0 || fitsdim>naxis)
    error(EXIT_FAILURE, 0, "%s: requested dimension (fitsdim=%zu) must be "
          "larger than zero and smaller than the number of dimensions in "
          "the given WCS structure (%zu)", __func__, fitsdim, naxis);

  /**************************************************/
#if WCS_REMOVE_DIM_CHECK
  printf("\n\nfitsdim: %zu\n", fitsdim);
  printf("\n##################\n");
  /*
  wcs->pc[0]=0;   wcs->pc[1]=1;   wcs->pc[2]=2;
  wcs->pc[3]=3;   wcs->pc[4]=4;   wcs->pc[5]=5;
  wcs->pc[6]=6;   wcs->pc[7]=7;   wcs->pc[8]=8;
  */
  for(i=0;i<wcs->naxis;++i)
    {
      for(j=0;j<wcs->naxis;++j)
        printf("%-5g", wcs->pc[i*wcs->naxis+j]);
      printf("\n");
    }
#endif
  /**************************************************/

  /* First loop over the arrays. */
  for(i=0;i<naxis;++i)
    {
      /* The dimensions are in FITS order, but counting starts from 0, so
         we'll have to subtract 1 from 'fitsdim'. */
      if(i>fitsdim-1)
        {
          /* 1-D arrays. */
          if(wcs->crpix) wcs->crpix[i-1] = wcs->crpix[i];
          if(wcs->cdelt) wcs->cdelt[i-1] = wcs->cdelt[i];
          if(wcs->crval) wcs->crval[i-1] = wcs->crval[i];
          if(wcs->crota) wcs->crota[i-1] = wcs->crota[i];
          if(wcs->crder) wcs->crder[i-1] = wcs->crder[i];
          if(wcs->csyer) wcs->csyer[i-1] = wcs->csyer[i];

          /* The strings are all statically allocated, so we don't need to
             check. */
          memcpy(wcs->cunit[i-1], wcs->cunit[i], 72);
          memcpy(wcs->ctype[i-1], wcs->ctype[i], 72);
          memcpy(wcs->cname[i-1], wcs->cname[i], 72);

          /* For 2-D arrays, just bring up all the rows. We'll fix the
             columns in a second loop. */
          for(j=0;j<naxis;++j)
            {
              if(wcs->pc) wcs->pc[ (i-1)*naxis+j ] = wcs->pc[ i*naxis+j ];
              if(wcs->cd) wcs->cd[ (i-1)*naxis+j ] = wcs->cd[ i*naxis+j ];
            }
        }
    }


  /**************************************************/
#if WCS_REMOVE_DIM_CHECK
  printf("\n###### Respective row removed (replaced).\n");
  for(i=0;i<wcs->naxis;++i)
    {
      for(j=0;j<wcs->naxis;++j)
        printf("%-5g", wcs->pc[i*wcs->naxis+j]);
      printf("\n");
    }
#endif
  /**************************************************/


  /* Second loop for 2D arrays. */
  c=0;
  for(i=0;i<naxis;++i)
    for(j=0;j<naxis;++j)
      if(j!=fitsdim-1)
        {
          if(wcs->pc) wcs->pc[ c ] = wcs->pc[ i*naxis+j ];
          if(wcs->cd) wcs->cd[ c ] = wcs->cd[ i*naxis+j ];
          ++c;
        }


  /* Correct the total number of dimensions in the WCS structure. */
  naxis = wcs->naxis -= 1;


  /* The 'TAN' algorithm needs two dimensions. So we need to remove it when
     it can cause confusion. */
  switch(naxis)
    {
    /* The 'TAN' algorithm cannot be used for any single-dimensional
       dataset. So we'll have to remove it if it exists. */
    case 1:
      wcs_ctype_noalgorithm(wcs->ctype[0]);
      break;

    /* For any other dimensionality, 'TAN' should be kept only when exactly
       two dimensions have it. */
    default:

      c=0;
      for(i=0;i<naxis;++i)
        if( wcs_ctype_has_tan(wcs->ctype[i]) )
          ++c;

      if(c!=2)
        for(i=0;i<naxis;++i)
          if( wcs_ctype_has_tan(wcs->ctype[i]) )
            wcs_ctype_noalgorithm(wcs->ctype[i]);
      break;
    }



  /**************************************************/
#if WCS_REMOVE_DIM_CHECK
  printf("\n###### Respective column removed.\n");
  for(i=0;i<naxis;++i)
    {
      for(j=0;j<naxis;++j)
        printf("%-5g", wcs->pc[i*naxis+j]);
      printf("\n");
    }
  printf("\n###### One final string\n");
  for(i=0;i<naxis;++i)
    printf("%s\n", wcs->ctype[i]);
  exit(0);
#endif
  /**************************************************/
}





/* Using the block data structure of the tile, add a WCS structure for
   it. In many cases, tiles are created for internal processing, so there
   is no need to keep their WCS. Hence for preformance reasons, when
   creating the tiles they don't have any WCS structure. When needed, this
   function can be used to add a WCS structure to the tile by copying the
   WCS structure of its block and correcting its starting points. If the
   tile already has a WCS structure, this function won't do anything.*/
void
gal_wcs_on_tile(gal_data_t *tile)
{
  size_t i, start_ind, ndim=tile->ndim;
  gal_data_t *block=gal_tile_block(tile);
  size_t *coord=gal_pointer_allocate(GAL_TYPE_SIZE_T, ndim, 0, __func__,
                                     "coord");

  /* If the tile already has a WCS structure, don't do anything. */
  if(tile->wcs) return;
  else
    {
      /* Copy the block's WCS into the tile. */
      tile->wcs=gal_wcs_copy(block->wcs);

      /* Find the coordinates of the tile's starting index. */
      start_ind=gal_pointer_num_between(block->array, tile->array,
                                        block->type);
      gal_dimension_index_to_coord(start_ind, ndim, block->dsize, coord);

      /* Correct the copied WCS structure. Note that crpix is indexed in
         the FITS/Fortran order while coord is ordered in C, it also starts
         counting from 1, not zero. */
      for(i=0;i<ndim;++i)
        tile->wcs->crpix[i] -= coord[ndim-1-i];
      /*
      printf("start_ind: %zu\n", start_ind);
      printf("coord: %zu, %zu\n", coord[1]+1, coord[0]+1);
      printf("CRPIX: %f, %f\n", tile->wcs->crpix[0], tile->wcs->crpix[1]);
      */
    }

  /* Clean up. */
  free(coord);
}





/* Return the Warping matrix of the given WCS structure. This will be the
   final matrix irrespective of the type of storage in the WCS
   structure. Recall that the FITS standard has several methods to store
   the matrix, which is up to this function to account for and return the
   final matrix. The output is an allocated DxD matrix where 'D' is the
   number of dimensions. */
double *
gal_wcs_warp_matrix(struct wcsprm *wcs)
{
  double *out, crota2;
  size_t i, j, size=wcs->naxis*wcs->naxis;

  /* Allocate the necessary array. */
  errno=0;
  out=malloc(size*sizeof *out);
  if(out==NULL)
    error(EXIT_FAILURE, errno, "%s: allocating %zu bytes for 'out'",
          __func__, size*sizeof *out);

  /* Fill in the array. */
  if(wcs->altlin & 0x1)          /* Has a PCi_j array. */
    {
      for(i=0;i<wcs->naxis;++i)
        for(j=0;j<wcs->naxis;++j)
          out[i*wcs->naxis+j] = wcs->cdelt[i] * wcs->pc[i*wcs->naxis+j];
    }
  else if(wcs->altlin & 0x2)     /* Has CDi_j array.   */
    {
      for(i=0;i<size;++i)
        out[i]=wcs->cd[i];
    }
  else if(wcs->altlin & 0x4)     /* Has CROTAi array.   */
    {
      /* Basic sanity checks. */
      if(wcs->naxis!=2)
        error(EXIT_FAILURE, 0, "%s: CROTAi currently on works in 2 "
              "dimensions.", __func__);
      if(wcs->crota[0]!=0.0)
        error(EXIT_FAILURE, 0, "%s: CROTA1 is not zero", __func__);

      /* CROTAi keywords are depreciated in the FITS standard. However, old
         files may still use them. For a full description of CROTAi
         keywords and their history (along with the conversion equations
         here), please see the link below:

         https://fits.gsfc.nasa.gov/users_guide/users_guide/node57.html

         Just note that the equations of the link above convert CROTAi to
         PC. But here we want the "final" matrix (after multiplication by
         the 'CDELT' values). So to speed things up, we won't bother
         dividing and then multiplying by the same CDELT values in the
         off-diagonal elements. */
      crota2=wcs->crota[1];
      out[0] = wcs->cdelt[0] * cos(crota2);
      out[1] = -1 * wcs->cdelt[1] *sin(crota2);
      out[2] = wcs->cdelt[0] * sin(crota2);
      out[3] = wcs->cdelt[1] * cos(crota2);

      /* For a check:
      printf("cdelt: %f, %f\n", wcs->cdelt[0], wcs->cdelt[1]);
      printf("cd:\n%f, %f\n%f, %f\n", out[0], out[1], out[2], out[3]);
      */
    }
  else
    error(EXIT_FAILURE, 0, "%s: currently only PCi_ja and CDi_ja keywords "
          "are recognized", __func__);

  /* Return the result. */
  return out;
}




/* Clean up small/negligible errros that are clearly caused by measurement
   errors in the PC and CDELT elements. */
void
gal_wcs_clean_errors(struct wcsprm *wcs)
{
  double crdcheck=NAN;
  size_t i, crdnum=0, ndim=wcs->naxis;
  double mean, crdsum=0, sum=0, min=FLT_MAX, max=0;
  double *pc=wcs->pc, *cdelt=wcs->cdelt, *crder=wcs->crder;

  /* First clean up CDELT: if the CRDER keyword is set, then we'll use that
     as a reference, if not, we'll use the absolute floating point error
     defined in 'GAL_WCS_FLTERR'. */
  for(i=0; i<ndim; ++i)
    {
      sum+=cdelt[i];
      if(cdelt[i]>max) max=cdelt[i];
      if(cdelt[i]<min) min=cdelt[i];
      if(crder[i]!=UNDEFINED) {++crdnum; crdsum=crder[i];}
    }
  mean=sum/ndim;
  crdcheck = crdnum ? crdsum/crdnum : GAL_WCS_FLTERROR;
  if( (max-min)/mean < crdcheck )
    for(i=0; i<ndim; ++i)
      cdelt[i]=mean;

  /* Now clean up the PC elements. If the diagonal elements are too close
     to 0, 1, or -1, set them to 0 or 1 or -1. */
  if(pc)
    for(i=0;i<ndim*ndim;++i)
      {
        if(      fabs(pc[i] -  0 ) < GAL_WCS_FLTERROR ) pc[i]=0;
        else if( fabs(pc[i] -  1 ) < GAL_WCS_FLTERROR ) pc[i]=1;
        else if( fabs(pc[i] - -1 ) < GAL_WCS_FLTERROR ) pc[i]=-1;
      }
}





/* According to the FITS standard, in the 'PCi_j' WCS formalism, the matrix
   elements m_{ij} are encoded in the 'PCi_j' keywords and the scale
   factors are encoded in the 'CDELTi' keywords. There is also another
   formalism (the 'CDi_j' formalism) which merges the two into one matrix.

   However, WCSLIB's internal operations are apparently done in the 'PCi_j'
   formalism. So its outputs are also all in that format by default. When
   the input is a 'CDi_j', WCSLIB will still read the image into the
   'PCi_j' formalism and the 'CDELTi's are set to 1. This function will
   decompose the two matrices to give a reasonable 'CDELTi' and 'PCi_j' in
   such cases. */
void
gal_wcs_decompose_pc_cdelt(struct wcsprm *wcs)
{
  size_t i, j;
  int status=0;
  double *ps, *warp;

  /* If there is on WCS, then don't do anything. */
  if(wcs==NULL) return;

  /* Get the pixel scale and full warp matrix. */
  warp=gal_wcs_warp_matrix(wcs);
  ps=gal_wcs_pixel_scale(wcs);
  if(ps==NULL) return;

  /* For a check.
  printf("ps: %g, %g\n", ps[0], ps[1]);
  printf("warp: %g, %g, %g, %g\n", warp[0], warp[1],
         warp[2], warp[3]);
  */

  /* Set the CDELTs. */
  for(i=0; i<wcs->naxis; ++i)
    wcs->cdelt[i] = ps[i];

  /* Write the PC matrix. */
  for(i=0;i<wcs->naxis;++i)
    for(j=0;j<wcs->naxis;++j)
      wcs->pc[i*wcs->naxis+j] = warp[i*wcs->naxis+j]/ps[i];

  /* According to the 'wcslib/wcs.h' header: "In particular, wcsset()
     resets wcsprm::cdelt to unity if CDi_ja is present (and no
     PCi_ja).". So apparently, when the input is a 'CDi_j', it might expect
     the 'CDELTi' elements to be 1.0. But we have changed that here, so we
     will correct the 'altlin' element of the WCS structure to make sure
     that WCSLIB only looks into the 'PCi_j' and 'CDELTi' and makes no
     assumptioins about 'CDELTi'. */
  wcs->altlin=1;

  /* Re-run 'wcsset' to update all the internal WCS parameters. */
  status=wcsset(wcs);
  if(status)
    error(EXIT_SUCCESS, 0, "%s: WCSLIB warning: wcsset ERROR %d: %s",
          __func__, status, wcs_errmsg[status]);

  /* Clean up. */
  free(ps);
  free(warp);
}





/* Set the WCS structure to use the CD matrix. */
void
gal_wcs_to_cd(struct wcsprm *wcs)
{
  size_t i, j, n;
  double er=1e-10;

  /* If there is on WCS, then don't do anything. */
  if(wcs==NULL) return;

  /* 'wcs->altlin' identifies which rotation element is being used (PCi_j,
     CDi_J or CROTAi). For PCi_j, the first bit will be 1 (==1), for CDi_j,
     the second bit is 1 (==2) and for CROTAi, the third bit is 1 (==4). */
  n=wcs->naxis;
  switch(wcs->altlin)
    {
   /* PCi_j: Convert it to CDi_j. */
    case 1:

      /* Fill in the CD matrix and correct the PC and CDELT arrays. We have
         to do this because ultimately, WCSLIB will be writing the PC and
         CDELT keywords, even when 'altlin' is 2. So effectively we have to
         multiply the PC and CDELT matrices, then set cdelt=1 in all
         dimensions. This is actually how WCSLIB reads a FITS header with
         only a CD matrix. */
      for(i=0;i<n;++i)
        {
          for(j=0;j<n;++j)
            wcs->cd[i*n+j] = wcs->pc[i*n+j] *= wcs->cdelt[i];
          wcs->cdelt[i]=1;
        }

      /* Set the altlin to be the CD matrix and free the PC matrix. */
      wcs->altlin=2;
      break;

    /* CDi_j: No need to do any conversion. */
    case 2: return; break;

    /* Both PCi_j and CDi_j are present! If they are the same (within
       floating point errors), we'll just set WCS to use the 'CD' matrix
       (as demanded from this function). If they are not the same, then
       print an error. */
    case 3:
      for(i=0;i<n;++i)
        for(j=0;j<n;++j)
          if(wcs->cd[i*n+j] - wcs->pc[i*n+j] * wcs->cdelt[i] > er)
            error(EXIT_FAILURE, 0, "%s: the given WCS has both the "
                  "CDi_j and PCi_j+CDELTi conventions for defining "
                  "the rotation and scale. However, they do not match! "
                  "Please inspect the file and remove the wrong set of "
                  "keywords (you can use 'astfits file.fits "
                  "--delete=KEYNAME' to delete the keyword 'KEYNAME'; "
                  "and you can call '--delete' multiple times). For "
                  "more on the definition of the different "
                  "representations, see the FITS standard: "
                  "https://fits.gsfc.nasa.gov/fits_standard.html",
                  __func__);
      wcs->altlin=2;
      break;

    /* CROTAi: not yet supported. */
    case 4:
      error(0, 0, "%s: WARNING: Conversion of 'CROTAi' keywords to the CD "
            "matrix is not yet supported (for lack of time!), please "
            "contact us at %s to add this feature. But this may not cause a "
            "problem at all, so please check if the output's WCS is "
            "reasonable", __func__, PACKAGE_BUGREPORT);
      break;

    /* The value isn't supported! */
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to fix the "
            "problem. The value %d for wcs->altlin isn't recognized",
            __func__, PACKAGE_BUGREPORT, wcs->altlin);
    }
}





/* The distance (along a great circle) on a sphere between two points
   is calculated here. Since the pixel sides are usually very small,
   we won't be using the direct formula:

   cos(distance)=sin(d1)*sin(d2)+cos(d1)*cos(d2)*cos(r1-r2)

   We will be using the haversine formula which better considering
   floating point errors (from Wikipedia:)

   sin^2(distance)/2=sin^2( (d1-d2)/2 )+cos(d1)*cos(d2)*sin^2( (r1-r2)/2 )

   Inputs and outputs are all in degrees.
*/
double
gal_wcs_angular_distance_deg(double r1, double d1, double r2, double d2)
{
  /* Convert degrees to radians. */
  double r1r=r1*M_PI/180, d1r=d1*M_PI/180;
  double r2r=r2*M_PI/180, d2r=d2*M_PI/180;

  /* To make things easier to read: */
  double a=sin( (d1r-d2r)/2 );
  double b=sin( (r1r-r2r)/2 );

  /* Return the result: */
  return 2*asin( sqrt( a*a + cos(d1r)*cos(d2r)*b*b) ) * 180/M_PI;
}





/* Calculate the vertices of a box in the given center. The output should
   have 8 allocated spaces ready to be written into:
      out[0]=bottom-left-ra
      out[1]=bottom-left-dec
      out[2]=bottom-right-ra
      out[3]=bottom-right-dec
      out[4]=top-right-ra
      out[5]=top-right-dec
      out[6]=top-left-ra
      out[7]=top-left-dec

  The 'ra_delta' and 'dec_delta' should be the full length of the box along
  the respective axis (not half of it!). */
void
gal_wcs_box_vertices_from_center(double ra_center, double dec_center,
                                 double ra_delta,  double dec_delta,
                                 double *out)
{
  double corr;

  /* The bottom vertices, note that the positive right ascension is on
     the left side. */
  out[1] = out[3] = dec_center - dec_delta/2;
  corr=1/(2*cos(out[1]*M_PI/180));
  out[0] = ra_center + ra_delta*corr;
  out[2] = ra_center - ra_delta*corr;

  /* The top vertices. */
  out[5] = out[7] = dec_center + dec_delta/2;
  corr=1/(2*cos(out[5]*M_PI/180));
  out[4] = ra_center + ra_delta*corr;
  out[6] = ra_center - ra_delta*corr;
}





/* Return the pixel scale of the dataset in units of the WCS. */
double *
gal_wcs_pixel_scale(struct wcsprm *wcs)
{
  gsl_vector S;
  gsl_matrix A, V;
  int warning_printed;
  gal_data_t *pixscale;
  size_t i, j, n, maxj, *permutation;
  double jvmax, *a, *out, *v, maxrow, minrow;

  /* Only continue if a WCS exists. */
  if(wcs==NULL) return NULL;


  /* Write the full WCS rotation matrix into an array, irrespective of what
     style it was stored in the wcsprm structure ('PCi_j' style or 'CDi_j'
     style). */
  a=gal_wcs_warp_matrix(wcs);


  /* Now that everything is good, we can allocate the necessary memory. */
  n=wcs->naxis;
  v=gal_pointer_allocate(GAL_TYPE_FLOAT64, n*n, 0, __func__, "v");
  permutation=gal_pointer_allocate(GAL_TYPE_SIZE_T, n, 0, __func__,
                                   "permutation");
  pixscale=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &n, NULL,
                          0, -1, 1, NULL, NULL, NULL);


  /* To avoid confusing issues with floating point errors being written in
     the non-diagonal elements of the FITS header PC or CD matrices, we
     need to check if the minimum and maximum values in each row are not
     several orders of magnitude apart.

     Note that in some cases (for example a spectrum), one axis might be in
     degrees (value around 1e-5) and the other in angestroms (value around
     1e-10). So we can't look at the minimum and maximum of the whole
     matrix. However, in such cases, people will probably not warp/rotate
     the image to mix the coordinates. So the important thing to check is
     the minimum and maximum (non-zero) values in each row. */
  warning_printed=0;
  for(i=0;i<n;++i)
    {
      /* Find the minimum and maximum values in each row. */
      minrow=FLT_MAX;
      maxrow=-FLT_MAX;
      for(j=0;j<n;++j)
        if(a[i*n+j]!=0.0) /* We aren't concerned with 0 valued elements. */
          {
            /* We have to use absolutes because in cases like RA, the
               diagonal values in different rows may have different signs. */
            if(fabs(a[i*n+j])<minrow) minrow=fabs(a[i*n+j]);
            if(fabs(a[i*n+j])>maxrow) maxrow=fabs(a[i*n+j]);
          }

      /* Do the check, print warning and make correction. */
      if(maxrow!=minrow
         && maxrow/minrow>1e5 /* The difference between elements is large. */
         && maxrow/minrow<GAL_WCS_FLTERROR
         && warning_printed==0)
        {
          fprintf(stderr, "\nWARNING: The input WCS matrix (possibly taken "
                  "from the FITS header keywords starting with 'CD' or 'PC') "
                  "contains values with very different scales (more than "
                  "10^5 different). This is probably due to floating point "
                  "errors. These values might bias the pixel scale (and "
                  "subsequent) calculations.\n\n"
                  "You can see the respective matrix with one of the "
                  "following two commands (depending on how the FITS file "
                  "was written). Recall that if the desired extension/HDU "
                  "isn't the default, you can choose it with the '--hdu' "
                  "(or '-h') option before the '|' sign in these commands."
                  "\n\n"
                  "    $ astfits file.fits -p | grep 'PC._.'\n"
                  "    $ astfits file.fits -p | grep 'CD._.'\n\n"
                  "You can delete the ones with obvious floating point "
                  "error values using the following command (assuming you "
                  "want to delete 'CD1_2' and 'CD2_1'). Afterwards, you can "
                  "re-run your original command to remove this warning "
                  "message and possibly correct errors that it might have "
                  "caused.\n\n"
                  "    $ astfits file.fits --delete=CD1_2 --delete=CD2_1\n\n"
                  );
          warning_printed=1;
        }
    }


  /* Fill in the necessary GSL vector and Matrix structures. */
  S.size=n;     S.stride=1;                S.data=pixscale->array;
  V.size1=n;    V.size2=n;    V.tda=n;     V.data=v;
  A.size1=n;    A.size2=n;    A.tda=n;     A.data=a;


  /* Run GSL's Singular Value Decomposition, using one-sided Jacobi
     orthogonalization which computes the singular (scale) values to a
     higher relative accuracy. */
  gsl_linalg_SV_decomp_jacobi(&A, &V, &S);


  /* The raw pixel scale array produced from the singular value
     decomposition above is ordered based on values, not the input. So when
     the pixel scales in all the dimensions aren't the same (the units of
     the dimensions differ), the order of the values in 'pixelscale' will
     not necessarily correspond to the input's dimensions.

     To correct the order, we can use the 'V' matrix to find the original
     position of the pixel scale values and then use permutation to
     re-order it correspondingly. The column with the largest (absolute)
     value will be taken as the one to be used for each row. */
  for(i=0;i<n;++i)
    {
      /* Find the column with the maximum value. */
      maxj=-1;
      jvmax=-FLT_MAX;
      for(j=0;j<n;++j)
        if(fabs(v[i*n+j])>jvmax)
          {
            maxj=j;
            jvmax=fabs(v[i*n+j]);
          }

      /* Use the column with the maximum value for this dimension. */
      permutation[i]=maxj;
    }


  /* Apply the permutation described above. */
  gal_permutation_apply(pixscale, permutation);


  /* Clean up and return. */
  free(a);
  free(v);
  free(permutation);
  out=pixscale->array;
  pixscale->array=NULL;
  gal_data_free(pixscale);
  return out;
}





/* Report the arcsec^2 area of the pixels in the image based on the
   WCS information in that image. */
double
gal_wcs_pixel_area_arcsec2(struct wcsprm *wcs)
{
  double out;
  double *pixscale;

  /* Some basic sanity checks. */
  if(wcs==NULL) return NAN;
  if(wcs->naxis==1) return NAN;

  /* Check if the units of the axis are degrees or not. Currently all FITS
     images I have worked with use 'deg' for degrees. If other alternatives
     exist, we can add corrections later. */
  if( strcmp("deg", wcs->cunit[0]) || strcmp("deg", wcs->cunit[1]) )
    return NAN;

  /* Get the pixel scales along each axis in degrees, then multiply. */
  pixscale=gal_wcs_pixel_scale(wcs);
  if(pixscale==NULL) return NAN;

  /* Clean up and return the result. */
  out = pixscale[0] * pixscale[1] * 3600.0f * 3600.0f;
  free(pixscale);
  return out;
}





int
gal_wcs_coverage(char *filename, char *hdu, size_t *ondim,
                 double **ocenter, double **owidth, double **omin,
                 double **omax, char *hdu_option_name)
{
  fitsfile *fptr;
  struct wcsprm *wcs;
  int nwcs=0, type, status=0;
  char *name=NULL, *unit=NULL;
  gal_data_t *fc, *tmp, *coords=NULL;
  size_t i, ndim, *dsize=NULL, numrows;
  size_t fullcircledim=GAL_BLANK_SIZE_T;
  double *x=NULL, *y=NULL, *z=NULL, *min, *max, *fcarr, *center, *width;

  /* Read the desired WCS (note that the linear matrix is irrelevant here,
     we'll just select PC because its the default WCS mode. */
  wcs=gal_wcs_read(filename, hdu, GAL_WCS_LINEAR_MATRIX_PC,
                   0, 0, &nwcs, hdu_option_name);

  /* If a WCS doesn't exist, return NULL. */
  if(wcs==NULL) return 0;

  /* Make sure the input HDU is an image. */
  if( gal_fits_hdu_format(filename, hdu, hdu_option_name) != IMAGE_HDU )
    error(EXIT_FAILURE, 0, "%s (hdu %s): is not an image HDU, the "
          "'--skycoverage' option only applies to image extensions",
          filename, hdu);

  /* Get the array information of the image. */
  fptr=gal_fits_hdu_open(filename, hdu, READONLY, 1,
                         hdu_option_name);
  gal_fits_img_info(fptr, &type, ondim, &dsize, &name, &unit);
  fits_close_file(fptr, &status);
  ndim=*ondim;

  /* Abort if we have more than 3 dimensions. */
  if(ndim==1 || ndim>3) return 0;

  /* Allocate the output datasets. */
  center=*ocenter=gal_pointer_allocate(GAL_TYPE_FLOAT64, ndim, 0, __func__,
                                       "ocenter");
  width=*owidth=gal_pointer_allocate(GAL_TYPE_FLOAT64, ndim, 0, __func__,
                                     "owidth");
  min=*omin=gal_pointer_allocate(GAL_TYPE_FLOAT64, ndim, 0, __func__,
                                 "omin");
  max=*omax=gal_pointer_allocate(GAL_TYPE_FLOAT64, ndim, 0, __func__,
                                 "omax");

  /* Now that we have the number of dimensions in the image, allocate the
     space needed for the coordinates. */
  numrows = (ndim==2) ? 5 : 9;
  for(i=0;i<ndim;++i)
    {
      tmp=gal_data_alloc(NULL, GAL_TYPE_FLOAT64, 1, &numrows, NULL, 0,
                         -1, 1, NULL, NULL, NULL);
      tmp->next=coords;
      coords=tmp;
    }

  /* Fill in the coordinate arrays, Note that 'dsize' is ordered in C
     dimensions, for the WCS conversion, we need to have the dimensions
     ordered in FITS/Fortran order. */
  switch(ndim)
    {
    case 2:
      x=coords->array;  y=coords->next->array;
      x[0] = 1;         y[0] = 1;
      x[1] = dsize[1];  y[1] = 1;
      x[2] = 1;         y[2] = dsize[0];
      x[3] = dsize[1];  y[3] = dsize[0];
      x[4] = dsize[1]/2 + (dsize[1]%2 ? 1 : 0.5f);
      y[4] = dsize[0]/2 + (dsize[0]%2 ? 1 : 0.5f);
      break;
    case 3:
      x=coords->array; y=coords->next->array; z=coords->next->next->array;
      x[0] = 1;        y[0] = 1;              z[0]=1;
      x[1] = dsize[2]; y[1] = 1;              z[1]=1;
      x[2] = 1;        y[2] = dsize[1];       z[2]=1;
      x[3] = dsize[2]; y[3] = dsize[1];       z[3]=1;
      x[4] = 1;        y[4] = 1;              z[4]=dsize[0];
      x[5] = dsize[2]; y[5] = 1;              z[5]=dsize[0];
      x[6] = 1;        y[6] = dsize[1];       z[6]=dsize[0];
      x[7] = dsize[2]; y[7] = dsize[1];       z[7]=dsize[0];
      x[8] = dsize[2]/2 + (dsize[2]%2 ? 1 : 0.5f);
      y[8] = dsize[1]/2 + (dsize[1]%2 ? 1 : 0.5f);
      z[8] = dsize[0]/2 + (dsize[0]%2 ? 1 : 0.5f);
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to "
            "fix the problem. 'ndim' of %zu is not recognized.",
            __func__, PACKAGE_BUGREPORT, ndim);
    }

  /* For a check:
  printf("IMAGE COORDINATES:\n");
  for(i=0;i<numrows;++i)
    if(ndim==2)
      printf("%-15g%-15g\n", x[i], y[i]);
    else
      printf("%-15g%-15g%-15g\n", x[i], y[i], z[i]);
  */

  /* Convert to the world coordinate system. */
  gal_wcs_img_to_world(coords, wcs, 1);

  /* For a check:
  printf("\nWORLD COORDINATES:\n");
  for(i=0;i<numrows;++i)
    if(ndim==2)
      printf("%-15g%-15g\n", x[i], y[i]);
    else
      printf("%-15g%-15g%-15g\n", x[i], y[i], z[i]);
  */

  /* Get the minimum and maximum values in each dimension. */
  tmp=gal_statistics_minimum(coords);
  min[0] = ((double *)(tmp->array))[0];      gal_data_free(tmp);
  tmp=gal_statistics_maximum(coords);
  max[0] = ((double *)(tmp->array))[0];      gal_data_free(tmp);
  tmp=gal_statistics_minimum(coords->next);
  min[1] = ((double *)(tmp->array))[0];      gal_data_free(tmp);
  tmp=gal_statistics_maximum(coords->next);
  max[1] = ((double *)(tmp->array))[0];      gal_data_free(tmp);
  if(ndim>2)
    {
      tmp=gal_statistics_minimum(coords->next->next);
      min[2] = ((double *)(tmp->array))[0];      gal_data_free(tmp);
      tmp=gal_statistics_maximum(coords->next->next);
      max[2] = ((double *)(tmp->array))[0];      gal_data_free(tmp);
    }

  /* Write the center and width. */
  width[0]=max[0]-min[0];
  width[1]=max[1]-min[1];
  switch(ndim)
    {
    case 2:
      center[0]=x[4];  center[1]=y[4];
      break;
    case 3:
      width[2]=max[2]-min[2];
      center[0]=x[8]; center[1]=y[8]; center[2]=z[8];
      break;
    default:
      error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at %s to solve "
            "the problem. The value %zu is not a recognized dimension",
            __func__, PACKAGE_BUGREPORT, ndim);
    }

  /* It may happen that the image passes the RA=0 hour circle. In that
     case, the width along the first WCS dimension (RA) will be larger than
     180. Therefore, we need to re-calculate the minimum and maximums. An
     example dataset is available in https://savannah.gnu.org/bugs/?63257
     (Gnuastro bug #63257):

        $ jplusdr2url=http://archive.cefca.es/catalogues/vo/siap/jplus-dr2
        $ wget $jplusdr2url/get_fits?id=71811 -Ojplus-dr2-71811.fits.fz

     But first, we need to find the dimension that has a full-circle
     dimension: this is usually the first WCS dimension, but it may happen
     that it isn't. Also, note that the WCS may not be celestial/spherical
     at all (hence why we are only doing this for pre-defined celestial
     'CTYPE's). */
  for(i=0;i<ndim;++i)
    if(   strncmp(wcs->ctype[i], "RA-",   3)==0 /* Equatorial */
       || strncmp(wcs->ctype[i], "GLON-", 5)==0 /* Galactic */
       || strncmp(wcs->ctype[i], "SLON-", 5)==0 /* Super Galactic */
       || strncmp(wcs->ctype[i], "ELON-", 5)==0 /* Ecliptic */ )
      { fullcircledim=i; break; }
  if( fullcircledim!=GAL_BLANK_SIZE_T && width[fullcircledim]>180.0f )
    {
      /* Set the proper pointer to the coordinate that should be
         checked. */
      switch(fullcircledim)
        {
        case 0: fc=coords;             break;
        case 1: fc=coords->next;       break;
        case 2: fc=coords->next->next; break;
        default:
          error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at "
                "'%s' to fix the problem. The value '%zu' isn't "
                "recognized for 'fullcircledim'", __func__,
                PACKAGE_BUGREPORT, fullcircledim);
        }

      /* Shift all the coordinates after 0, by 360, while keeping those
         that are close to 360 untouched, then find the minimum and
         maximums. */
      fcarr=fc->array;
      for(i=0;i<coords->size;++i) if(fcarr[i]<180) fcarr[i]+=360.0f;
      tmp=gal_statistics_minimum(fc);
      min[fullcircledim] = ((double *)(tmp->array))[0]; gal_data_free(tmp);
      tmp=gal_statistics_maximum(fc);
      max[fullcircledim] = ((double *)(tmp->array))[0]; gal_data_free(tmp);

      /* Calculate the correct width, maximum is guarateed to be larger
         than 360, so subtract 360 from it. */
      width[fullcircledim]=max[fullcircledim]-min[fullcircledim];
      max[fullcircledim]-=360.0f;
    }

  /* Clean up and return success. */
  free(dsize);
  if(name) free(name);
  if(unit) free(unit);
  wcsfree(wcs); free(wcs);
  gal_list_data_free(coords);
  return 1;
}



















/**************************************************************/
/**********            Array conversion            ************/
/**************************************************************/
/* Some sanity checks for the WCS conversion functions. */
static void
wcs_convert_sanity_check_alloc(gal_data_t *coords, struct wcsprm *wcs,
                               const char *func, int **stat, double **phi,
                               double **theta, double **world,
                               double **pixcrd, double **imgcrd)
{
  gal_data_t *tmp;
  size_t ndim=0, firstsize=0, size=coords->size;

  /* Make sure a WCS structure is actually given. */
  if(wcs==NULL)
    error(EXIT_FAILURE, 0, "%s: input WCS structure is NULL", func);

  for(tmp=coords; tmp!=NULL; tmp=tmp->next)
    {
      /* Count how many coordinates are given. */
      ++ndim;

      /* Check the type of the input. */
      if(tmp->type!=GAL_TYPE_FLOAT64)
        error(EXIT_FAILURE, 0, "%s: input coordinates must have 'float64' "
              "type", func);

      /* Make sure it has a single dimension. */
      if(tmp->ndim!=1)
        error(EXIT_FAILURE, 0, "%s: input coordinates for each dimension "
              "must each be one dimensional. Coordinate dataset %zu of the "
              "inputs has %zu dimensions", func, ndim, tmp->ndim);

      /* See if all inputs have the same size. */
      if(ndim==1) firstsize=tmp->size;
      else
        if(firstsize!=tmp->size)
          error(EXIT_FAILURE, 0, "%s: all input coordinates must have the "
                "same number of elements. Coordinate dataset %zu has %zu "
                "elements while the first coordinate has %zu", func, ndim,
                tmp->size, firstsize);
    }

  /* See if the number of coordinates given corresponds to the dimensions
     of the WCS structure. */
  if(ndim!=wcs->naxis)
    error(EXIT_FAILURE, 0, "%s: the number of input coordinates (%zu) does "
          "not match the dimensions of the input WCS structure (%d)", func,
          ndim, wcs->naxis);

  /* Allocate all the necessary arrays. */
  *phi    = gal_pointer_allocate( GAL_TYPE_FLOAT64, size,      0, __func__,
                                  "phi");
  *stat   = gal_pointer_allocate( GAL_TYPE_INT,     size,      1, __func__,
                                  "stat");
  *theta  = gal_pointer_allocate( GAL_TYPE_FLOAT64, size,      0, __func__,
                                  "theta");
  *world  = gal_pointer_allocate( GAL_TYPE_FLOAT64, ndim*size, 0, __func__,
                                  "world");
  *imgcrd = gal_pointer_allocate( GAL_TYPE_FLOAT64, ndim*size, 0, __func__,
                                  "imgcrd");
  *pixcrd = gal_pointer_allocate( GAL_TYPE_FLOAT64, ndim*size, 0, __func__,
                                  "pixcrd");
}





/* In Gnuastro, each column (coordinate for WCS conversion) is treated as a
   separate array in a 'gal_data_t' that are linked through a linked
   list. But in WCSLIB, the input is a single array (with multiple
   columns). This function will convert between the two. */
static void
wcs_convert_list_to_from_array(gal_data_t *list, double *array, int *stat,
                               size_t ndim, int to0from1)
{
  size_t i, d=0;
  gal_data_t *tmp;

  for(tmp=list; tmp!=NULL; tmp=tmp->next)
    {
      /* Put all this coordinate's values into the single array that is
         input into or output from WCSLIB. */
      for(i=0;i<list->size;++i)
        {
          if(to0from1)
            ((double *)(tmp->array))[i] = stat[i] ? NAN : array[i*ndim+d];
          else
            array[i*ndim+d] = ((double *)(tmp->array))[i];
        }

      /* Increment the dimension. */
      ++d;
    }
}





/* Prepare the output of the WCS conversion functions. */
static gal_data_t *
wcs_convert_prepare_out(gal_data_t *coords, struct wcsprm *wcs, int inplace)
{
  size_t i;
  gal_data_t *out=NULL;

  if(inplace)
    out=coords;
  else
    for(i=0;i<wcs->naxis;++i)
      gal_list_data_add_alloc(&out, NULL, GAL_TYPE_FLOAT64, 1,
                              &coords->size, NULL, 0, coords->minmapsize,
                              coords->quietmmap, wcs->ctype[i],
                              wcs->cunit[i], NULL);
  return out;
}





/* Convert world coordinates to image coordinates given the input WCS
   structure. The input must be a linked list of data structures of float64
   ('double') type. The top element of the linked list must be the first
   coordinate and etc. If 'inplace' is non-zero, then the output will be
   written into the input's allocated space. */
gal_data_t *
gal_wcs_world_to_img(gal_data_t *coords, struct wcsprm *wcs, int inplace)
{
  gal_data_t *out;
  int *stat=NULL, ncoord=coords->size, nelem;
  double *phi=NULL, *theta=NULL, *world=NULL, *pixcrd=NULL, *imgcrd=NULL;

  /* It can happen that the input datasets are empty. In this case, simply
     return them. */
  if(coords->size==0 || coords->array==NULL)
    {
      if(inplace) return coords;
      else error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at "
                 "'%s' to fix the problem. The input has no data and "
                 "'inplace' is not called", __func__, PACKAGE_BUGREPORT);
    }


  /* Some sanity checks. */
  wcs_convert_sanity_check_alloc(coords, wcs, __func__, &stat, &phi,
                                 &theta, &world, &pixcrd, &imgcrd);
  nelem=wcs->naxis; /* We have to make sure a WCS is given first. */


  /* Write the values from the input list of separate columns into a single
     array (WCSLIB input). */
  wcs_convert_list_to_from_array(coords, world, stat, wcs->naxis, 0);


  /* Use WCSLIB's wcss2p for the conversion. We are ignoring the over-all
     status here, because later we will use the 'stat' array to set all bad
     coordinates to NaN. */
  wcss2p(wcs, ncoord, nelem, world, phi, theta, imgcrd, pixcrd, stat);


  /* For a sanity check.
  {
    size_t i;
    printf("\n\n%s sanity check:\n", __func__);
    for(i=0;i<coords->size;++i)
      printf("(%g, %g) --> phi:%g, theta: %g --> (%g, %g) --> (%g, %g) "
             "[stat: %d]\n",
             world[i*2],  world[i*2+1],
             phi[i*2],    theta[i*2],
             imgcrd[i*2], imgcrd[i*2+1],
             pixcrd[i*2], pixcrd[i*2+1], stat[i]);
    printf("stat: %d\n", stat[4]);
  }
  */


  /* Allocate the output arrays if they were not already allocated. */
  out=wcs_convert_prepare_out(coords, wcs, inplace);


  /* Write the output from a single array (WCSLIB output) into the output
     list of this function. */
  wcs_convert_list_to_from_array(out, pixcrd, stat, wcs->naxis, 1);


  /* Clean up. */
  free(phi);
  free(stat);
  free(theta);
  free(world);
  free(imgcrd);
  free(pixcrd);

  /* Return the output list of coordinates. */
  return out;
}





/* Similar to 'gal_wcs_world_to_img'. */
gal_data_t *
gal_wcs_img_to_world(gal_data_t *coords, struct wcsprm *wcs, int inplace)
{
  gal_data_t *out;
  int *stat=NULL, ncoord=coords->size, nelem;
  double *phi=NULL, *theta=NULL, *world=NULL, *pixcrd=NULL, *imgcrd=NULL;

  /* Some sanity checks. */
  wcs_convert_sanity_check_alloc(coords, wcs, __func__, &stat, &phi, &theta,
                                 &world, &pixcrd, &imgcrd);
  nelem=wcs->naxis; /* We have to make sure a WCS is given first. */


  /* Write the values from the input list of separate columns into a single
     array (WCSLIB input). */
  wcs_convert_list_to_from_array(coords, pixcrd, stat, wcs->naxis, 0);


  /* Use WCSLIB's wcsp2s for the conversion. We are ignoring the over-all
     status here, because later we will use the 'stat' array to set all bad
     coordinates to NaN. */
  wcsp2s(wcs, ncoord, nelem, pixcrd, imgcrd, phi, theta, world, stat);


  /* For a check.
  {
    size_t i;
    printf("\n\n%s sanity check (%d dimensions):\n", __func__, nelem);
    for(i=0;i<coords->size;++i)
      switch(nelem)
        {
        case 2:
          printf("(%-10g %-10g) --> (%-10g %-10g), [stat: %d]\n",
                 pixcrd[i*2], pixcrd[i*2+1],
                 world[i*2],  world[i*2+1], stat[i]);
          break;
        case 3:
          printf("(%g, %g, %g) --> (%g, %g, %g), [stat: %d]\n",
                 pixcrd[i*3], pixcrd[i*3+1], pixcrd[i*3+2],
                 world[i*3],  world[i*3+1],  world[i*3+2], stat[i]);
          break;
        }
  }
  */


  /* Allocate the output arrays if they were not already allocated. */
  out=wcs_convert_prepare_out(coords, wcs, inplace);


  /* Write the output from a single array (WCSLIB output) into the output
     list of this function. */
  wcs_convert_list_to_from_array(out, world, stat, wcs->naxis, 1);


  /* Clean up. */
  free(phi);
  free(stat);
  free(theta);
  free(world);
  free(imgcrd);
  free(pixcrd);


  /* Return the output list of coordinates. */
  return out;
}
