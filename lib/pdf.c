/*********************************************************************
pdf -- functions to write PDF files.
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
#include <config.h>

#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <gnuastro/eps.h>
#include <gnuastro/pdf.h>
#include <gnuastro/jpeg.h>

#include <gnuastro-internal/checkset.h>




/*************************************************************
 **************      Acceptable PDF names      ***************
 *************************************************************/
int
gal_pdf_name_is_pdf(char *name)
{
  size_t len;

  if(name)
    {
      len=strlen(name);
      if (strcmp(&name[len-3], "pdf") == 0
          || strcmp(&name[len-3], "PDF") == 0)
        return 1;
      else
        return 0;
    }
  else return 0;
}





int
gal_pdf_suffix_is_pdf(char *name)
{
  if(name)
    {
      if (strcmp(name, "pdf") == 0 || strcmp(name, ".pdf") == 0
          || strcmp(name, "PDF") == 0 || strcmp(name, ".PDF") == 0)
        return 1;
      else
        return 0;
    }
  else return 0;
}




















/*************************************************************
 **************        Write a PDF image       ***************
 *************************************************************/
void
gal_pdf_write(gal_data_t *in, char *filename, float widthincm,
              uint32_t borderwidth, uint8_t bordercolor,
              int dontoptimize, gal_data_t *marks)
{
  pid_t pid;
  int childstat=0;
  size_t w_h_in_pt[2];
  char *device, *devwp, *devhp, *devopt;
  char *epsname=gal_checkset_malloc_cat(filename, ".ps");

  /* Write the EPS file. */
  gal_eps_write(in, epsname, widthincm, borderwidth,
                bordercolor, 0, dontoptimize, 0, marks);

  /* Get the size of the image in 'pt' units. */
  gal_eps_to_pt(widthincm, in->dsize, w_h_in_pt);

  /* Set the device from the file name. */
  if(gal_jpeg_name_is_jpeg(filename)) device="jpeg";
  else                                device="pdfwrite";

  /* Build the necessary strings. */
  if( asprintf(&devwp, "-dDEVICEWIDTHPOINTS=%zu",
               w_h_in_pt[0]+2*borderwidth)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation error", __func__);
  if( asprintf(&devhp, "-dDEVICEHEIGHTPOINTS=%zu",
               w_h_in_pt[1]+2*borderwidth)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation error", __func__);
  if( asprintf(&devopt, "-sDEVICE=%s", device)<0 )
    error(EXIT_FAILURE, 0, "%s: asprintf allocation error", __func__);

  /* Run Ghostscript (if the command changes, also change the command in
     the error message). We are not using the 'system()' command because
     that call '/bin/sh', which may not always be usable within the running
     environment; see https://savannah.nongnu.org/bugs/?65677.

     For a very nice summary on fork/execl, see the first answer in the
     link below.
     https://stackoverflow.com/questions/4204915/please-explain-the-exec-function-and-its-family

     In summary: the child gets 'pid=0' and the parent gets the process ID
     of the child. The parent then waits for the child to finish and
     continues. */
  pid=fork();
  if(pid<0) error(EXIT_FAILURE, 0, "%s: could not build fork", __func__);
  if(pid==0)
    execl(PATH_GHOSTSCRIPT, "gs", "-q", "-o", filename, devopt,
          devwp, devhp, "-dPDFFitPage", epsname, NULL);
  else waitpid(pid, &childstat, 0);
  if(childstat)
    error(EXIT_FAILURE, 0, "the Ghostscript command (printed at the "
          "end of this message) to convert/compile the EPS file (made "
          "by Gnuastro) to PDF was not successful (Ghostscript returned "
          "with status %d; its error message is shown above)! The EPS "
          "file ('%s') is left if you want to convert it through any "
          "other means (for example the 'epspdf' program). The "
          "Ghostscript command was: %s -q -o %s %s %s %s -dPDFFitPage "
          "%s", childstat, epsname, PATH_GHOSTSCRIPT, filename, devopt,
          devwp, devhp, epsname);

  /* Delete the EPS file. */
  errno=0;
  if(unlink(epsname))
    error(EXIT_FAILURE, errno, "%s", epsname);

  /* Clean up. */
  free(devhp);
  free(devwp);
  free(devopt);
  free(epsname);
}
