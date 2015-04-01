# Apply a general homographic transformation to an image.
#
# See the Tests subsection of the manual for a complete explanation
# (in the Installing gnuastro section).
#
# Original author:
#     Mohammad Akhlaghi <akhlaghi@gnu.org>
# Contributing author(s):
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.





# Preliminaries:
################
# Set the variabels (The executable is in the build tree). Do the
# basic checks to see if the executable is made or if the defaults
# file exists (basicchecks.sh is in the source tree).
prog=imgwarp
execname=../src/$prog/ast$prog





# If the executable was not made (the user chose to not install this
# package), skip this test:
if [ ! -f $execname ]; then
    exit 77
fi





# Actual test script:
#####################
img=convolve_spatial.fits
$execname $img --output=homographic.fits \
          --matrix="0.707106781,-0.707106781,0, 0.707106781, 0.707106781,0,      0.001,0.0001,1"
