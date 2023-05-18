# Check conversion of coordinates.
#
# See the Tests subsection of the manual for a complete explanation
# (in the Installing gnuastro section).
#
# Original author:
#     Mohammad Akhlaghi <mohammad@akhlaghi.org>
# Contributing author(s):
#     Sepideh Eskandarlou <sepideh.eskandarlou@gmail.com>
# Copyright (C) 2015-2023 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.





# Preliminaries
# =============
#
# Set the variables (The executable is in the build tree). Do the
# basic checks to see if the executable is made or if the defaults
# file exists (basicchecks.sh is in the source tree).
prog=table
execname=../bin/$prog/ast$prog
inimg=convolve_spatial_scaled_noised.fits
intable=$topsrc/tests/mkprof/mkprofcat1.txt




# Skip?
# =====
#
# If the dependencies of the test don't exist, then skip it. There are two
# types of dependencies:
#
#   - The executable was not made (for example due to a configure option),
#
#   - The input data was not made (for example the test that created the
#     data file failed).
if [ ! -f $execname ]; then echo "$execname not created."; exit 77; fi
if [ ! -f $intable  ]; then echo "$intable doesn't exist.";  exit 77; fi
if [ ! -f $inimg    ]; then echo "$inimg doesn't exist.";  exit 77; fi





# Actual test script
# ==================
#
# 'check_with_program' can be something like 'Valgrind' or an empty
# string. Such programs will execute the command if present and help in
# debugging when the developer doesn't have access to the user's system.
#
# This table will be used for checking the zeropoint script. Be carful
# about column metadata.
$check_with_program $execname $intable \
                    --wcsfile=$inimg \
                    --equal=Function,point \
                    --output=table-img-to-wcs.fits \
                    -c'arith X Y img-to-wcs',Magnitude \
                    --colmetadata=2,DEC,deg,"Declination" \
                    --colmetadata=1,RA,deg,"Right Ascension" \
                    --colmetadata=3,Magnitude,log,"Magnitude of profile"
