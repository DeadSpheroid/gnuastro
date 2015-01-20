# Crop from a catalog using x and y coordinates in Image mode.
#
# See the Tests subsection of the manual for a complete explanation
# (in the Installing AstrUtils section).





# Preliminaries:
################
# Set the variabels (The executable is in the build tree). Do the
# basic checks to see if the executable is made or if the defaults
# file exists (basicchecks.sh is in the source tree).
prog=imgcrop
execname=../src/$prog/astr$prog
source $topsrc/tests/basicchecks.sh





# Actual test script:
#####################
img=mkprofcat1.fits
cat=$topsrc/tests/$prog/cat.txt
$execname $img $cat --imgmode --suffix=_imgcat.fits --zeroisnotblank
