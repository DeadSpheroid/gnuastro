/*********************************************************************
Convolve - Convolve input data with a given kernel.
Convolve is part of GNU Astronomy Utilities (gnuastro) package.

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
#ifndef MAIN_H
#define MAIN_H




#include "commonparams.h"



/* Progarm name macros: */
#define SPACK_VERSION   "0.1"
#define SPACK           "astconvolve" /* Subpackage executable name. */
#define SPACK_NAME      "Convolve" /* Subpackage full name.       */
#define SPACK_STRING    SPACK_NAME" ("PACKAGE_STRING") "SPACK_VERSION




struct uiparams
{
  char        *inputname;   /* Name of input file.                   */
  char       *kernelname;   /* Name of kernel file.                  */
  char             *khdu;   /* HDU of input Kernel.                  */

  int      kernelnameset;
  int            khduset;
  int     blankweightset;
};





struct convolveparams
{
  struct uiparams     up;   /* Pointer to user interface structure.   */
  struct commonparams cp;   /* Pointer to the commonparams structure. */

  /* Inputs: */
  float           *input;   /* Input image array.                     */
  float          *kernel;   /* Input Kernel array.                    */
  size_t             is0;   /* Input image size along C's first axis. */
  size_t             is1;   /* Input image size along C's second axis.*/
  size_t             ks0;   /* Kernel size along C's first axis.      */
  size_t             ks1;   /* Kernel size along C's second axis.     */
  int        inputhasnan;   /* ==1: The input image has nan pixels.   */
  int         kernelflip;   /* ==1: Flip the kernel.                  */
  int         kernelnorm;   /* ==1: Normalize the kernel.             */
  int               nwcs;   /* Number of WCS headers.                 */
  struct wcsprm     *wcs;   /* WCS structure.                         */

  /* Outputs: */
  float      blankweight;   /* Maximum weight of blank pixels.        */

  /* internal: */
  time_t         rawtime;   /* Starting time of the program.          */
};

#endif