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

#include <gnuastro/eps.h>
#include <gnuastro/pdf.h>
#include <gnuastro/jpeg.h>
#include <gnuastro/pointer.h>

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
  int execstat=0;
  size_t w_h_in_pt[2];
  gal_list_str_t *command=NULL;
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
     the error message).  */
  gal_list_str_add(&command, PATH_GHOSTSCRIPT, 0);
  gal_list_str_add(&command, "-q", 0);
  gal_list_str_add(&command, "-o", 0);
  gal_list_str_add(&command, filename, 0);
  gal_list_str_add(&command, devopt, 0);
  gal_list_str_add(&command, devwp, 0);
  gal_list_str_add(&command, devhp, 0);
  gal_list_str_add(&command, "-dPDFFitPage", 0);
  gal_list_str_add(&command, epsname, 0);
  gal_list_str_reverse(&command);
  execstat=gal_checkset_exec(PATH_GHOSTSCRIPT, command);
  if(execstat)
    error(EXIT_FAILURE, 0, "the Ghostscript command (printed at the "
          "end of this message) to convert/compile the EPS file (made "
          "by Gnuastro) to PDF was not successful (Ghostscript returned "
          "with status %d; its error message is shown above)! The EPS "
          "file ('%s') is left if you want to convert it through any "
          "other means (for example the 'epspdf' program). The "
          "Ghostscript command was: %s", execstat, epsname,
          gal_list_str_cat(command, ' '));

  /* Delete the EPS file. */
  errno=0;
  if(unlink(epsname))
    error(EXIT_FAILURE, errno, "%s", epsname);

  /* Clean up the command list and delete the EPS file. */
  free(devhp);
  free(devwp);
  free(devopt);
  free(epsname);
  gal_list_str_free(command, 0);
}
