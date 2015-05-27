/*********************************************************************
SubtractSky - Find and subtract the sky value from an image.
SubtractSky is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <akhlaghi@gnu.org>
Contributing author(s):
Copyright (C) 2015, Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <fitsio.h>

#include "timing.h"	/* Includes time.h and sys/time.h   */
#include "checkset.h"
#include "txtarrayvv.h"
#include "statistics.h"
#include "arraymanip.h"
#include "commonargs.h"
#include "configfiles.h"
#include "fitsarrayvv.h"

#include "main.h"

#include "ui.h"		        /* Needs main.h                   */
#include "args.h"	        /* Needs main.h, includes argp.h. */


/* Set the file names of the places where the default parameters are
   put. */
#define CONFIG_FILE SPACK CONF_POSTFIX
#define SYSCONFIG_FILE SYSCONFIG_DIR "/" CONFIG_FILE
#define USERCONFIG_FILEEND USERCONFIG_DIR CONFIG_FILE
#define CURDIRCONFIG_FILE CURDIRCONFIG_DIR CONFIG_FILE










/**************************************************************/
/**************       Options and parameters    ***************/
/**************************************************************/
void
readconfig(char *filename, struct subtractskyparams *p)
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


      /* Inputs: */
      if(strcmp(name, "hdu")==0)
	{
	  if(cp->hduset) continue;
	  errno=0;
	  cp->hdu=malloc(strlen(value)+1);
	  if(cp->hdu==NULL)
	    error(EXIT_FAILURE, errno, "Space for HDU.");
	  strcpy(cp->hdu, value);
	  cp->hduset=1;
	}
      else if(strcmp(name, "mask")==0)
	{
	  if(up->masknameset) continue;
	  errno=0;
	  up->maskname=malloc(strlen(value)+1);
	  if(up->maskname==NULL)
	    error(EXIT_FAILURE, errno, "Space for mask name.");
	  strcpy(up->maskname, value);
	  up->masknameset=1;
	}
      else if(strcmp(name, "mhdu")==0)
	{
	  if(up->mhduset) continue;
	  errno=0;
	  up->mhdu=malloc(strlen(value)+1);
	  if(up->mhdu==NULL)
	    error(EXIT_FAILURE, errno, "Space for mask HDU.");
	  strcpy(up->mhdu, value);
	  up->mhduset=1;
	}
      else if(strcmp(name, "kernel")==0)
	{
	  if(up->kernelnameset) continue;
	  errno=0;
	  up->kernelname=malloc(strlen(value)+1);
	  if(up->kernelname==NULL)
	    error(EXIT_FAILURE, errno, "Space for kernel name.");
	  strcpy(up->kernelname, value);
	  up->kernelnameset=1;
	}
      else if(strcmp(name, "khdu")==0)
	{
	  if(up->khduset) continue;
	  errno=0;
	  up->khdu=malloc(strlen(value)+1);
	  if(up->khdu==NULL)
	    error(EXIT_FAILURE, errno, "Space for kernel HDU.");
	  strcpy(up->khdu, value);
	  up->khduset=1;
	}



      /* Outputs */
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


      /* Mesh grid: */
      else if(strcmp(name, "meshsize")==0)
	{
	  if(up->meshsizeset) continue;
          sizetlzero(value, &p->mp.meshsize, name, key, SPACK,
                     filename, lineno);
	  up->meshsizeset=1;
	}
      else if(strcmp(name, "nch1")==0)
	{
	  if(up->nch1set) continue;
          sizetlzero(value, &p->mp.nch1, name, key, SPACK,
                     filename, lineno);
	  up->nch1set=1;
	}
      else if(strcmp(name, "nch2")==0)
	{
	  if(up->nch2set) continue;
          sizetlzero(value, &p->mp.nch2, name, key, SPACK,
                     filename, lineno);
	  up->nch2set=1;
	}
      else if(strcmp(name, "lastmeshfrac")==0)
	{
	  if(up->lastmeshfracset) continue;
          floatl0s1(value, &p->mp.lastmeshfrac, name, key, SPACK,
                    filename, lineno);
	  up->lastmeshfracset=1;
	}
      else if(strcmp(name, "mirrordist")==0)
	{
	  if(up->mirrordistset) continue;
          floatl0(value, &p->mp.mirrordist, name, key, SPACK,
                  filename, lineno);
	  up->mirrordistset=1;
	}
      else if(strcmp(name, "minmodeq")==0)
	{
	  if(up->minmodeqset) continue;
          floatl0s1(value, &p->mp.minmodeq, name, key, SPACK,
                  filename, lineno);
	  up->minmodeqset=1;
	}
      else if(strcmp(name, "numnearest")==0)
	{
	  if(up->numnearestset) continue;
          sizetlzero(value, &p->mp.numnearest, name, key, SPACK,
                     filename, lineno);
	  up->numnearestset=1;
	}
      else if(strcmp(name, "smoothwidth")==0)
	{
	  if(up->smoothwidthset) continue;
          sizetpodd(value, &p->mp.smoothwidth, name, key, SPACK,
                    filename, lineno);
	  up->smoothwidthset=1;
	}


      /* Statistics: */
      else if(strcmp(name, "sigclipmultip")==0)
	{
	  if(up->sigclipmultipset) continue;
          floatl0(value, &p->sigclipmultip, name, key, SPACK,
                  filename, lineno);
	  up->sigclipmultipset=1;
	}
      else if(strcmp(name, "sigcliptolerance")==0)
	{
	  if(up->sigcliptoleranceset) continue;
          floatl0s1(value, &p->sigcliptolerance, name, key, SPACK,
                  filename, lineno);
	  up->sigcliptoleranceset=1;
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
printvalues(FILE *fp, struct subtractskyparams *p)
{
  struct uiparams *up=&p->up;
  struct meshparams *mp=&p->mp;
  struct commonparams *cp=&p->cp;

  /* Print all the options that are set. Separate each group with a
     commented line explaining the options in that group. */
  fprintf(fp, "\n# Input:\n");
  if(cp->hduset)
    {
      if(stringhasspace(cp->hdu))
	fprintf(fp, CONF_SHOWFMT"\"%s\"\n", "hdu", cp->hdu);
      else
	fprintf(fp, CONF_SHOWFMT"%s\n", "hdu", cp->hdu);
    }
  if(up->masknameset)
    {
      if(stringhasspace(up->maskname))
	fprintf(fp, CONF_SHOWFMT"\"%s\"\n", "mask", up->maskname);
      else
	fprintf(fp, CONF_SHOWFMT"%s\n", "mask", up->maskname);
    }
  if(up->mhdu)
    {
      if(stringhasspace(up->mhdu))
	fprintf(fp, CONF_SHOWFMT"\"%s\"\n", "mhdu", up->mhdu);
      else
	fprintf(fp, CONF_SHOWFMT"%s\n", "mhdu", up->mhdu);
    }
  if(up->kernelnameset)
    {
      if(stringhasspace(up->kernelname))
	fprintf(fp, CONF_SHOWFMT"\"%s\"\n", "kernel", up->kernelname);
      else
	fprintf(fp, CONF_SHOWFMT"%s\n", "kernel", up->kernelname);
    }
  if(up->khdu)
    {
      if(stringhasspace(up->khdu))
	fprintf(fp, CONF_SHOWFMT"\"%s\"\n", "khdu", up->khdu);
      else
	fprintf(fp, CONF_SHOWFMT"%s\n", "khdu", up->khdu);
    }


  fprintf(fp, "\n# Output:\n");
  if(cp->outputset)
    fprintf(fp, CONF_SHOWFMT"%s\n", "output", cp->output);


  fprintf(fp, "\n# Mesh grid:\n");
  if(up->meshsizeset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "meshsize", mp->meshsize);
  if(up->nch1set)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "nch1", mp->nch1);
  if(up->nch2set)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "nch2", mp->nch2);
  if(up->lastmeshfracset)
    fprintf(fp, CONF_SHOWFMT"%.3f\n", "lastmeshfrac", mp->lastmeshfrac);
  if(up->mirrordistset)
    fprintf(fp, CONF_SHOWFMT"%.3f\n", "mirrordist", p->mp.mirrordist);
  if(up->minmodeqset)
    fprintf(fp, CONF_SHOWFMT"%.3f\n", "minmodeq", p->mp.minmodeq);
  if(up->numnearestset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "numnearest", p->mp.numnearest);
  if(up->smoothwidthset)
    fprintf(fp, CONF_SHOWFMT"%lu\n", "smoothwidth", p->mp.smoothwidth);


  fprintf(fp, "\n# Statistics:\n");
  if(up->sigclipmultipset)
    fprintf(fp, CONF_SHOWFMT"%.3f\n", "sigclipmultip", p->sigclipmultip);
  if(up->sigcliptoleranceset)
    fprintf(fp, CONF_SHOWFMT"%.3f\n", "sigcliptolerance",
            p->sigcliptolerance);
}






/* Note that numthreads will be used automatically based on the
   configure time. */
void
checkifset(struct subtractskyparams *p)
{
  struct uiparams *up=&p->up;
  struct commonparams *cp=&p->cp;

  int intro=0;
  if(cp->hduset==0)
    REPORT_NOTSET("hdu");
  if(up->khduset==0)
    REPORT_NOTSET("khdu");

  /* Mesh grid: */
  if(up->meshsizeset==0)
    REPORT_NOTSET("meshsize");
  if(up->nch1set==0)
    REPORT_NOTSET("nch1");
  if(up->nch2set==0)
    REPORT_NOTSET("nch2");
  if(up->lastmeshfracset==0)
    REPORT_NOTSET("lastmeshfrac");
  if(up->mirrordistset==0)
    REPORT_NOTSET("mirrordist");
  if(up->minmodeqset==0)
    REPORT_NOTSET("minmodeq");
  if(up->numnearestset==0)
    REPORT_NOTSET("numnearest");
  if(up->smoothwidthset==0)
    REPORT_NOTSET("smoothwidth");

  /* Statistics: */
  if(up->sigclipmultipset==0)
    REPORT_NOTSET("sigclipmultip");
  if(up->sigcliptoleranceset==0)
    REPORT_NOTSET("sigcliptolerance");

  END_OF_NOTSET_REPORT;
}




















/**************************************************************/
/***************       Sanity Check         *******************/
/**************************************************************/
void
sanitycheck(struct subtractskyparams *p)
{
  /* Set the maskname and mask hdu accordingly: */
  CHECKMASKNAMEANDHDU(SPACK);

  /* Set the output name: */
  if(p->cp.output)
    checkremovefile(p->cp.output, p->cp.dontdelete);
  else
    automaticoutput(p->up.inputname, "_skysubed.fits", p->cp.removedirinfo,
		p->cp.dontdelete, &p->cp.output);

  /* Set the sky image name: */

  /* Set the check image names: */
  if(p->meshname)
    {
      p->meshname=NULL;           /* Was not allocated before!  */
      automaticoutput(p->up.inputname, "_mesh.fits", p->cp.removedirinfo,
                      p->cp.dontdelete, &p->meshname);
    }
  if(p->interpname)
    {
      p->interpname=NULL;         /* Was not allocated before!  */
      automaticoutput(p->up.inputname, "_interp.fits", p->cp.removedirinfo,
                      p->cp.dontdelete, &p->interpname);
    }
  if(p->convname)
    {
      p->convname=NULL;         /* Was not allocated before!  */
      automaticoutput(p->up.inputname, "_conv.fits", p->cp.removedirinfo,
                      p->cp.dontdelete, &p->convname);
    }
  if(p->skyname)
    {
      p->skyname=NULL;            /* Was not allocated before!  */
      automaticoutput(p->up.inputname, "_sky.fits", p->cp.removedirinfo,
                      p->cp.dontdelete, &p->skyname);
    }


  /* Other checks: */
  if(p->mp.numnearest<MINACCEPTABLENEAREST)
    error(EXIT_FAILURE, 0, "The smallest possible number for `--numnearest' "
          "(`-n') is %d. You have asked for: %lu.", MINACCEPTABLENEAREST,
          p->mp.numnearest);

  /* Set the constants in the meshparams structure. */
  p->mp.params=p;
  p->mp.numthreads=p->cp.numthreads;
}



















/**************************************************************/
/***************       Preparations         *******************/
/**************************************************************/
void
preparearrays(struct subtractskyparams *p)
{
  struct meshparams *mp=&p->mp;

  /* Read the input image. */
  filetofloat(p->up.inputname, p->up.maskname, p->cp.hdu, p->up.mhdu,
              (float **)&p->mp.img, &p->bitpix, &p->numblank, &mp->s0,
              &mp->s1);
  readfitswcs(p->up.inputname, p->cp.hdu, &p->nwcs, &p->wcs);

  /* Read the kernel: */
  if(p->up.kernelnameset)
    prepfloatkernel(p->up.kernelname, p->up.khdu, &mp->kernel,
                    &mp->ks0, &mp->ks1);

  /* Check if the input sizes and channel sizes are exact
     multiples. */
  if( mp->s0%mp->nch2 || mp->s1%mp->nch1 )
    error(EXIT_FAILURE, 0, "The input image size (%lu x %lu) is not an "
          "exact multiple of the number of the given channels (%lu, %lu) "
          "in the respective axis.", mp->s1, mp->s0, mp->nch1, mp->nch2);
}



















/**************************************************************/
/************         Set the parameters          *************/
/**************************************************************/
void
setparams(int argc, char *argv[], struct subtractskyparams *p)
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

  /* Do a sanity check. */
  sanitycheck(p);

  /* Make the array of input images. */
  preparearrays(p);

  /* Everything is ready, notify the user of the program starting. */
  if(cp->verb)
    {
      printf(SPACK_NAME" started on %s", ctime(&p->rawtime));
      printf("  - Using %lu CPU threads.\n", p->cp.numthreads);
      printf("  - Input: %s (hdu: %s)\n", p->up.inputname, p->cp.hdu);
      if(p->up.maskname)
        printf("  - Mask: %s (hdu: %s)\n", p->up.maskname, p->up.mhdu);
      if(p->up.kernelnameset)
        printf("  - Kernel: %s (hdu: %s)\n", p->up.kernelname, p->up.khdu);
    }
}




















/**************************************************************/
/************      Free allocated, report         *************/
/**************************************************************/
void
freeandreport(struct subtractskyparams *p, struct timeval *t1)
{
  /* Free the allocated arrays: */
  free(p->mp.img);
  free(p->cp.hdu);
  free(p->up.khdu);
  free(p->up.mhdu);
  free(p->cp.output);

  /* Free all the allocated names: */
  if(p->meshname) free(p->meshname);
  if(p->interpname) free(p->interpname);
  if(p->up.kernelnameset) free(p->up.kernelname);

  /* Free the mask image name. Note that p->up.inputname was not
     allocated, but given to the program by the operating system. */
  if(p->up.maskname && p->up.maskname!=p->up.inputname)
    free(p->up.maskname);

  /* Free the WCS structure: */
  if(p->wcs)
    wcsvfree(&p->nwcs, &p->wcs);

  /* Print the final message. */
  reporttiming(t1, SPACK_NAME" finished in: ", 0);
}