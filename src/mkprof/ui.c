/*********************************************************************
mkprof (MakeProfiles) - Create mock astronomical profiles.
MakeProfiles is part of GNU Astronomy Utilities (AstrUtils) package.

Copyright (C) 2013-2015 Mohammad Akhlaghi
Tohoku University Astronomical Institute, Sendai, Japan.
http://astr.tohoku.ac.jp/~akhlaghi/

AstrUtils is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

AstrUtils is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with AstrUtils. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <time.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>

#include "timing.h"	        /* Includes time.h and sys/time.h */
#include "checkset.h"
#include "txtarrayvv.h"
#include "commonargs.h"
#include "configfiles.h"
#include "fitsarrayvv.h"

#include "main.h"
#include "ellipse.h"

#include "ui.h"			/* Needs main.h.                  */
#include "args.h"		/* Needs main.h, includes argp.h. */


/* Set the file names of the places where the default parameters are
   put. */
#define CONFIG_FILE SPACK CONF_POSTFIX
#define SYSCONFIG_FILE SYSCONFIG_DIR CONFIG_FILE
#define USERCONFIG_FILEEND USERCONFIG_DIR CONFIG_FILE
#define CURDIRCONFIG_FILE CURDIRCONFIG_DIR CONFIG_FILE







/**************************************************************/
/**************       Options and parameters    ***************/
/**************************************************************/
void
readconfig(char *filename, struct mkprofparams *p)
{
  FILE *fp;
  size_t lineno=0, len=200;
  char *line, *name, *value;
  struct uiparams *up=&p->up;
  struct commonparams *cp=&p->cp;
  char key='a';	/* Not used, just a place holder. */

  /* When the file doesn't exist or can't be opened, it is ignored. It
     might be intentional, so there is no error. If a parameter is
     missing, it will be reported after all defaults are read. */
  fp=fopen(filename, "r");
  if (fp==NULL) return;


  /* Allocate some space for `line` with `len` elements so it can
     easily be freed later on. The value of `len` is arbitarary at
     this point, during the run, getline will change it along with the
     pointer to line. */
  errno=0;
  line=malloc(len*sizeof *line);
  if(line==NULL)
    error(EXIT_FAILURE, errno, "ui.c: %lu bytes in readdefaults",
	  len * sizeof *line);

  /* Read the tokens in the file:  */
  while(getline(&line, &len, fp) != -1)
    {
      /* Prepare the "name" and "value" strings, also set lineno. */
      STARTREADINGLINE;

      /* Profiles: */
      if(strcmp(name, "tunitinp")==0)
	{
	  if(up->tunitinpset) continue;
	  intzeroorone(value, &p->up.tunitinp, name, key, SPACK,
		       filename, lineno);
	  up->tunitinpset=1;
	}
      else if(strcmp(name, "tolerance")==0)
	{
	  if(up->toleranceset) continue;
	  floatl0(value, &p->tolerance, name, key, SPACK, filename, lineno);
	  up->toleranceset=1;
	}
      else if(strcmp(name, "zeropoint")==0)
	{
	  if(up->zeropointset) continue;
	  floatl0(value, &p->zeropoint, name, key, SPACK, filename, lineno);
	  up->zeropointset=1;
	}
      else if(strcmp(name, "prepforconv")==0)
	{
	  if(up->prepforconvset) continue;
	  intzeroorone(value, &p->up.prepforconv, name, key, SPACK,
		       filename, lineno);
	  up->prepforconvset=1;
	}
      else if(strcmp(name, "xshift")==0)
	{
	  if(up->xshiftset) continue;
	  sizetelzero(value, &p->up.xshift, name, key, SPACK,
		      filename, lineno);
	  up->xshiftset=1;
	}
      else if(strcmp(name, "yshift")==0)
	{
	  if(up->yshiftset) continue;
	  sizetelzero(value, &p->up.yshift, name, key, SPACK,
		      filename, lineno);
	  up->yshiftset=1;
	}




      /* Catalog: */
      else if(strcmp(name, "xcol")==0)
	{
	  if(up->xcolset) continue;
	  sizetelzero(value, &p->xcol, name, key, SPACK, filename, lineno);
	  up->xcolset=1;
	}
      else if(strcmp(name, "ycol")==0)
	{
	  if(up->ycolset) continue;
	  sizetelzero(value, &p->ycol, name, key, SPACK, filename, lineno);
	  up->ycolset=1;
	}
      else if(strcmp(name, "fcol")==0)
	{
	  if(up->fcolset) continue;
	  sizetelzero(value, &p->fcol, name, key, SPACK, filename, lineno);
	  up->fcolset=1;
	}
      else if(strcmp(name, "rcol")==0)
	{
	  if(up->rcolset) continue;
	  sizetelzero(value, &p->rcol, name, key, SPACK, filename, lineno);
	  up->rcolset=1;
	}
      else if(strcmp(name, "ncol")==0)
	{
	  if(up->ncolset) continue;
	  sizetelzero(value, &p->ncol, name, key, SPACK, filename, lineno);
	  up->ncolset=1;
	}
      else if(strcmp(name, "pcol")==0)
	{
	  if(up->pcolset) continue;
	  sizetelzero(value, &p->pcol, name, key, SPACK, filename, lineno);
	  up->pcolset=1;
	}
      else if(strcmp(name, "qcol")==0)
	{
	  if(up->qcolset) continue;
	  sizetelzero(value, &p->qcol, name, key, SPACK, filename, lineno);
	  up->qcolset=1;
	}
      else if(strcmp(name, "mcol")==0)
	{
	  if(up->mcolset) continue;
	  sizetelzero(value, &p->mcol, name, key, SPACK, filename, lineno);
	  up->mcolset=1;
	}
      else if(strcmp(name, "tcol")==0)
	{
	  if(up->tcolset) continue;
	  sizetelzero(value, &p->tcol, name, key, SPACK, filename, lineno);
	  up->tcolset=1;
	}



      /* Outputs: */
      else if(strcmp(name, "output")==0)
	{
	  if(cp->outputset) continue;
	  errno=0;
	  cp->output=malloc(strlen(value)+1);
	  if(cp->output==NULL)
	    error(EXIT_FAILURE, errno, "Space for output");
	  strcpy(cp->output, value);
	  cp->outputset=1;
	}
      else if(strcmp(name, "naxis1")==0)
	{
	  if(up->naxis1set) continue;
	  sizetlzero(value, &p->s1, name, key, SPACK, filename, lineno);
	  up->naxis1set=1;
	}
      else if(strcmp(name, "naxis2")==0)
	{
	  if(up->naxis2set) continue;
	  sizetlzero(value, &p->s0, name, key, SPACK, filename, lineno);
	  up->naxis2set=1;
	}
      else if(strcmp(name, "oversample")==0)
	{
	  if(up->oversampleset) continue;
	  sizetlzero(value, &p->oversample, name, key, SPACK,
		     filename, lineno);
	  up->oversampleset=1;
	}




      /* Operating modes: */
      else if(strcmp(name, "numthreads")==0)
	{
	  if(cp->numthreadsset) continue;
	  sizetlzero(value, &cp->numthreads, name, key, SPACK,
		     filename, lineno);
	  cp->numthreadsset=1;
	}


      else
	error_at_line(EXIT_FAILURE, 0, filename, lineno,
		      "`%s` not recognized.\n", name);
    }

  free(line);
  fclose(fp);
}






void
printvalues(FILE *fp, struct mkprofparams *p)
{
  struct uiparams *up=&p->up;

  fprintf(fp, "\n# Profiles:\n");
  if(up->tunitinpset)
    fprintf(fp, CONF_SHOWFMT"%d\n", "tunitinp", p->up.tunitinp);
  if(up->toleranceset)
    fprintf(fp, CONF_SHOWFMT"%.2f\n", "tolerance", p->tolerance);
  if(up->zeropointset)
    fprintf(fp, CONF_SHOWFMT"%.2f\n", "zeropoint", p->zeropoint);

  fprintf(fp, "\n# Catalog:\n");
  if(up->xcolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "xcol", p->xcol);
  if(up->ycolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "ycol", p->ycol);
  if(up->fcolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "fcol", p->fcol);
  if(up->rcolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "rcol", p->rcol);
  if(up->ncolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "ncol", p->ncol);
  if(up->pcolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "pcol", p->pcol);
  if(up->qcolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "qcol", p->qcol);
  if(up->mcolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "mcol", p->mcol);
  if(up->mcolset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "tcol", p->tcol);

  fprintf(fp, "\n# Output:\n");
  if(p->cp.outputset)
    fprintf(fp, CONF_SHOWFMT"%s\n", "output", p->cp.output);
  if(up->naxis1set)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "naxis1", p->s1);
  if(up->naxis2set)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "naxis2", p->s0);
  if(up->oversampleset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "oversample", p->oversample);

  fprintf(fp, "\n# Operating modes:\n");
  /* Number of threads doesn't need to be checked, it is set by
     default */
  fprintf(fp, CONF_SHOWFMT"%lu\n", "numthreads", p->cp.numthreads);
}





void
checkifset(struct mkprofparams *p)
{
  struct uiparams *up=&p->up;
  struct commonparams *cp=&p->cp;

  int intro=0;
  if(up->tunitinpset==0)
    REPORT_NOTSET("tunitinp");
  if(up->toleranceset==0)
    REPORT_NOTSET("tolerance");
  if(up->zeropointset==0)
    REPORT_NOTSET("zeropoint");
  if(up->xcolset==0)
    REPORT_NOTSET("xcol");
  if(up->ycolset==0)
    REPORT_NOTSET("ycol");
  if(up->fcolset==0)
    REPORT_NOTSET("fcol");
  if(up->rcolset==0)
    REPORT_NOTSET("rcol");
  if(up->ncolset==0)
    REPORT_NOTSET("ncol");
  if(up->pcolset==0)
    REPORT_NOTSET("pcol");
  if(up->qcolset==0)
    REPORT_NOTSET("qcol");
  if(up->mcolset==0)
    REPORT_NOTSET("mcol");
  if(cp->outputset==0)
    REPORT_NOTSET("output");
  if(up->naxis1set==0)
    REPORT_NOTSET("naxis1");
  if(up->naxis2set==0)
    REPORT_NOTSET("naxis2");
  if(up->oversampleset==0)
    REPORT_NOTSET("oversample");
  END_OF_NOTSET_REPORT;
}




















/**************************************************************/
/***************       Sanity Check         *******************/
/**************************************************************/
void
sanitycheck(struct mkprofparams *p)
{
  double trunc;
  size_t i, j, columns[9], sum;

  /* Check if over-sampling is an odd number, then convert the
     oversampling rate into the double type. */
  if(p->oversample%2==0) ++p->oversample;
  p->dos=p->oversample;

  /* If the column numbers are not equal. */
  columns[0]=p->xcol; columns[1]=p->ycol; columns[2]=p->fcol;
  columns[3]=p->rcol; columns[4]=p->ncol; columns[5]=p->pcol;
  columns[6]=p->qcol; columns[7]=p->mcol; columns[8]=p->tcol;
  for(i=0;i<9;++i)
    for(j=0;j<9;++j)
      if(i!=j && columns[i]==columns[j])
	error(EXIT_FAILURE, 0, "At least two of the specified columns "
	      "are set to %lu! By adding the `-P` or `--printparams` "
	      "option you can check the final column numbers. They "
	      "all have to be different.", columns[i]);

  /* If all the columns are within the catalog: */
  CHECKCOLINCAT(p->xcol, "xcol");
  CHECKCOLINCAT(p->ycol, "ycol");
  CHECKCOLINCAT(p->fcol, "fcol");
  CHECKCOLINCAT(p->rcol, "rcol");
  CHECKCOLINCAT(p->ncol, "ncol");
  CHECKCOLINCAT(p->pcol, "pcol");
  CHECKCOLINCAT(p->qcol, "qcol");
  CHECKCOLINCAT(p->mcol, "mcol");
  CHECKCOLINCAT(p->tcol, "tcol");

  /* If prepforconv is called, then xshift and yshift should be zero. Also
     a Moffat or Gaussian profile should exist in the image. */
  if( (p->up.xshift==0 && p->up.yshift==0) && p->up.prepforconv)
    {
      /* Make sure there is at least one Moffat or Gaussian profile. */
      j=0;
      for(i=0;i<p->cs0;++i)
	if(p->cat[i*p->cs1+p->fcol]==1 || p->cat[i*p->cs1+p->fcol]==2)
	  {
	    j=i;
	    break;
	  }

      /* If there is no PSF in the catalog, then you can ignore
	 prepforconv. */
      if(i<p->cs0)
	{
	  /* Find the correct xshift and yshift using the first Moffat
	     or Gaussian profile (in row 'j'). Note that the output of
	     encloseellipse will be the total width, we only want half
	     of it for the shift.*/
	  if(p->up.tunitinp)
	    trunc=p->dos*p->cat[j*p->cs1+p->tcol];
	  else
	    trunc=p->dos*p->cat[j*p->cs1+p->tcol]*p->cat[j*p->cs1+p->rcol]/2;
	  encloseellipse(trunc, p->cat[j*p->cs1+p->qcol]*trunc,
			 p->cat[j*p->cs1+p->pcol], &p->up.yshift,
			 &p->up.xshift);
	  p->up.xshift/=2;
	  p->up.yshift/=2;
	}
    }

  /* If any of xshift or yshift is non-zero, the other should be too!
     Note that conditional operators return 1 if true and 0 if false,
     so if one is non-zero while the other is zero, then sum will be
     1. Otherwise the sum will either be 0 or 2.*/
  sum = (p->up.xshift!=0) + (p->up.yshift!=0);
  if(sum==1)
    error(EXIT_FAILURE, 0, "At least one of `--xshift` (`-X`) or "
	  "`--yshift` (`-Y`) are zero! ");


}




















/**************************************************************/
/***************       Preparations         *******************/
/**************************************************************/
void
preparearrays(struct mkprofparams *p)
{
  double *cat=p->cat;
  size_t i, cs1=p->cs1;
  struct uiparams *up=&p->up;

  /* Allocate space for the log file: */
  p->log=malloc(p->cs0*LOGNUMCOLS*sizeof *p->log);
  if(p->log==NULL)
    error(EXIT_FAILURE, 0, "Allocating %lu bytes for log file.",
	  p->cs0*LOGNUMCOLS*sizeof *p->log);

  /* Correct the X and Y positions and set the truncation radius in
     units of pixels. Note that in sanity check we checked that X and
     Y shifts are either both zero or both non-zero. So we don't need
     to check for y shift*/
  if(up->xshift || up->tunitinp==0)
    for(i=0;i<p->cs0;++i)
      {
	cat[i*cs1+p->xcol]+=up->xshift;
	cat[i*cs1+p->ycol]+=up->yshift;
	if(up->tunitinp==0)
	  cat[i*cs1+p->tcol]*=cat[i*cs1+p->rcol];
      }

  /* Correct the output image size based on the x and y shifts. Note
     the difference between the FITS and C axises. */
  p->s0+=2*up->yshift;
  p->s1+=2*up->xshift;
}



















/**************************************************************/
/************         Set the parameters          *************/
/**************************************************************/
void
setparams(int argc, char *argv[], struct mkprofparams *p)
{
  struct commonparams *cp=&p->cp;

  /* Set the non-zero initial values, the structure was initialized to
     have a zero value for all elements. */
  cp->spack         = SPACK;
  cp->verb          = 1;
  cp->numthreads    = DP_NUMTHREADS;
  cp->removedirinfo = 1;

  /* Read the arguments. */
  errno=0;
  if(argp_parse(&thisargp, argc, argv, 0, 0, p))
    error(EXIT_FAILURE, errno, "Parsing arguments");

  /* Add the user default values and save them if asked. */
  CHECKSETCONFIG;

  /* Check if all the required parameters are set. */
  checkifset(p);

  /* Print the values for each parameter. */
  if(cp->printparams)
    REPORT_PARAMETERS_SET;

  /* Read catalog if given. */
  if(p->up.catname)
    txttoarray(p->up.catname, &p->cat, &p->cs0, &p->cs1);

  /* Do a sanity check, then remove the possibly existing log file
     created by txttoarray. */
  sanitycheck(p);
  checkremovefile(TXTARRAYVVLOG, 0);

  /* Prepare the necessary arrays: */
  preparearrays(p);

  /* Everything is ready, notify the user of the program starting. */
  if(cp->verb)
    printf(SPACK_NAME" started on %s", ctime(&p->rawtime));
}




















/**************************************************************/
/************      Free allocated, report         *************/
/**************************************************************/
void
freeandreport(struct mkprofparams *p, struct timeval *t1)
{
  /* Free all the allocated arrays. */
  free(p->cat);
  free(p->cp.hdu);
  free(p->cp.output); /* If not set, it is NULL, and free is OK with NULL */

  /* Print the final message. */
  reporttiming(t1, SPACK_NAME" finished in: ", 0);
}
