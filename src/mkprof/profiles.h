/*********************************************************************
MakeProfiles - Create mock astronomical profiles.
MakeProfiles is part of GNU Astronomy Utilities (Gnuastro) package.

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
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#ifndef PROFILES_H
#define PROFILES_H

double
Gaussian(struct mkonthread *mkp);

double
totgaussian(double q);

double
Moffat(struct mkonthread *mkp);

double
moffat_alpha(double fwhm, double beta);

double
totmoffat(double alpha, double beta, double q);

double
Sersic(struct mkonthread *mkp);

double
sersic_b(double n);

double
totsersic(double n, double re, double b, double q);

double
Circumference(struct mkonthread *mkp);

double
Flat(struct mkonthread *mkp);

#endif
