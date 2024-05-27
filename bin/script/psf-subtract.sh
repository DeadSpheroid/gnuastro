#!/bin/sh

# Situate a PSF stamp into a given position with the same output image size
# than the original image and subtract it.
#
# Original author:
#   Raul Infante-Sainz <infantesainz@gmail.com>
# Contributing author(s):
#   Mohammad Akhlaghi <mohammad@akhlaghi.org>
#   Zahra Sharbaf <zahra.sharbaf2@gmail.com>
# Copyright (C) 2021-2024 Free Software Foundation, Inc.
#
# Gnuastro is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# Gnuastro is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with Gnuastro. If not, see <http://www.gnu.org/licenses/>.


# Exit the script in the case of failure
set -e

# 'LC_NUMERIC' is responsible for formatting numbers printed by the OS.  It
# prevents floating points like '23,45' instead of '23.45'.
export LC_NUMERIC=C





# Default option values (can be changed with options on the command-line).
hdu=1
psf=""
mode=""
psfhdu=1
quiet=""
scale=""
center=""
keeptmp=0
output=""
tmpdir=""
modelonly=0
version=@VERSION@
scriptname=@SCRIPT_NAME@





# Output of `--usage' and `--help':
print_usage() {
    cat <<EOF
$scriptname: run with '--help' for list of options
EOF
}

print_help() {
    cat <<EOF
Usage: $scriptname [OPTION] FITS-files

This script is part of GNU Astronomy Utilities $version.

This script will place the given PSF image into a certain central position
with the image and subtract it.

For more information, please run any of the following commands. In
particular the first contains a very comprehensive explanation of this
script's invocation: expected input(s), output(s), and a full description
of all the options.

     Inputs/Outputs and options:           $ info $scriptname
     Full Gnuastro manual/book:            $ info gnuastro

If you couldn't find your answer in the manual, you can get direct help from
experienced Gnuastro users and developers. For more information, please run:

     $ info help-gnuastro

$scriptname options:
 Input:
  -h, --hdu=STR           HDU/extension of all input FITS files.
  -p, --psf=STR           PSF FITS image.
  -P, --psfhdu=STR        HDU/extension of the PSF image.
  -O, --mode=STR          Coordinates mode ('wcs' or 'img').
  -c, --center=FLT,FLT    Center coordinates of the object.
  -s, --scale=FLT         Factor by which the PSF is multiplied.

 Output:
  -o, --output            Output table with the radial profile.
  -t, --tmpdir            Directory to keep temporary files.
  -k, --keeptmp           Keep temporal/auxiliar files.
  -m, --modelonly         Give the model as output (don't subtract it).

 Operating mode:
  -?, --help              Print this help list.
      --cite              BibTeX citation for this program.
  -q, --quiet             Don't print any extra information in stdout.
  -V, --version           Print program version.

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

GNU Astronomy Utilities home page: http://www.gnu.org/software/gnuastro/

Report bugs to bug-gnuastro@gnu.org.
EOF
}





# Output of `--version':
print_version() {
    cat <<EOF
$scriptname (GNU Astronomy Utilities) $version
Copyright (C) 2020-2024 Free Software Foundation, Inc.
License GPLv3+: GNU General public license version 3 or later.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written/developed by Raul Infante-Sainz
EOF
}





# Output of `--cite':
print_citation() {
    empty="" # needed for the ascii art!
    cat <<EOF

Thank you for using $scriptname (GNU Astronomy Utilities) $version

Citations and acknowledgement are vital for the continued work on Gnuastro.

Please cite the following record(s) and add the acknowledgement statement below in your work to support us. Please note that different Gnuastro programs may have different corresponding papers. Hence, please check all the programs you used. Don't forget to also include the version as shown above for reproducibility.

Paper describing the creation of an extended PSF
------------------------------------------------
@ARTICLE{gnuastro-psf,
       author = {{Infante-Sainz}, Ra{\'u}l and {Trujillo}, Ignacio and {Rom{\'a}n}, Javier},
        title = "{The Sloan Digital Sky Survey extended point spread functions}",
      journal = {MNRAS},
         year = 2020,
        month = feb,
       volume = {491},
       number = {4},
        pages = {5317},
          doi = {10.1093/mnras/stz3111},
archivePrefix = {arXiv},
       eprint = {1911.01430},
 primaryClass = {astro-ph.IM},
       adsurl = {https://ui.adsabs.harvard.edu/abs/2020MNRAS.491.5317I},
      adsnote = {Provided by the SAO/NASA Astrophysics Data System}
}

First paper introducing Gnuastro
--------------------------------
  @ARTICLE{gnuastro,
     author = {{Akhlaghi}, M. and {Ichikawa}, T.},
      title = "{Noise-based Detection and Segmentation of Nebulous Objects}",
    journal = {ApJS},
  archivePrefix = "arXiv",
     eprint = {1505.01664},
   primaryClass = "astro-ph.IM",
       year = 2015,
      month = sep,
     volume = 220,
        eid = {1},
      pages = {1},
        doi = {10.1088/0067-0049/220/1/1},
     adsurl = {https://ui.adsabs.harvard.edu/abs/2015ApJS..220....1A},
    adsnote = {Provided by the SAO/NASA Astrophysics Data System}
  }

Acknowledgement
---------------
This work was partly done using GNU Astronomy Utilities (Gnuastro, ascl.net/1801.009) version $version. Work on Gnuastro has been funded by the Japanese Ministry of Education, Culture, Sports, Science, and Technology (MEXT) scholarship and its Grant-in-Aid for Scientific Research (21244012, 24253003), the European Research Council (ERC) advanced grant 339659-MUSICOS, the Spanish Ministry of Economy and Competitiveness (MINECO, grant number AYA2016-76219-P) and the NextGenerationEU grant through the Recovery and Resilience Facility project ICTS-MRR-2021-03-CEFCA.
                                               ,
                                              {|'--.
                                             {{\    \ $empty
      Many thanks from all                   |/\`'--./=.
      Gnuastro developers!                   \`\.---' \`\\
                                                  |\  ||
                                                  | |//
                                                   \//_/|
                                                   //\__/
                                                  //
                   (http://www.chris.com/ascii/) |/

EOF
}





# Functions to check option values and complain if necessary.
on_off_option_error() {
    if [ x"$2" = x ]; then
        echo "$scriptname: '$1' doesn't take any values"
    else
        echo "$scriptname: '$1' (or '$2') doesn't take any values"
    fi
    exit 1
}

check_v() {
    if [ x"$2" = x ]; then
        cat <<EOF
$scriptname: option '$1' requires an argument. Try '$scriptname --help' for more information
EOF
        exit 1;
    fi
}





# Separate command-line arguments from options. Then put the option
# value into the respective variable.
#
# OPTIONS WITH A VALUE:
#
#   Each option has three lines because we want to all common formats: for
#   long option names: `--longname value' and `--longname=value'. For short
#   option names we want `-l value', `-l=value' and `-lvalue' (where `-l'
#   is the short version of the hypothetical `--longname' option).
#
#   The first case (with a space between the name and value) is two
#   command-line arguments. So, we'll need to shift it two times. The
#   latter two cases are a single command-line argument, so we just need to
#   "shift" the counter by one. IMPORTANT NOTE: the ORDER OF THE LATTER TWO
#   cases matters: `-h*' should be checked only when we are sure that its
#   not `-h=*').
#
# OPTIONS WITH NO VALUE (ON-OFF OPTIONS)
#
#   For these, we just want the two forms of `--longname' or `-l'. Nothing
#   else. So if an equal sign is given we should definitely crash and also,
#   if a value is appended to the short format it should crash. So in the
#   second test for these (`-l*') will account for both the case where we
#   have an equal sign and where we don't.
inputs=""
while [ $# -gt 0 ]
do
    case "$1" in
        # Input parameters.
        -h|--hdu)            hdu="$2";                                  check_v "$1" "$hdu";  shift;shift;;
        -h=*|--hdu=*)        hdu="${1#*=}";                             check_v "$1" "$hdu";  shift;;
        -h*)                 hdu=$(echo "$1"  | sed -e's/-h//');        check_v "$1" "$hdu";  shift;;
        -p|--psf)            psf="$2";                                  check_v "$1" "$psf";  shift;shift;;
        -p=*|--psf=*)        psf="${1#*=}";                             check_v "$1" "$psf";  shift;;
        -p*)                 psf=$(echo "$1"  | sed -e's/-p//');        check_v "$1" "$psf";  shift;;
        -P|--psfhdu)         psfhdu="$2";                               check_v "$1" "$psfhdu";  shift;shift;;
        -P=*|--psfhdu=*)     psfhdu="${1#*=}";                          check_v "$1" "$psfhdu";  shift;;
        -P*)                 psfhdu=$(echo "$1"  | sed -e's/-P//');     check_v "$1" "$psfhdu";  shift;;
        -O|--mode)           mode="$2";                                 check_v "$1" "$mode";  shift;shift;;
        -O=*|--mode=*)       mode="${1#*=}";                            check_v "$1" "$mode";  shift;;
        -O*)                 mode=$(echo "$1"  | sed -e's/-O//');       check_v "$1" "$mode";  shift;;
        -c|--center)         center="$2";                               check_v "$1" "$center";  shift;shift;;
        -c=*|--center=*)     center="${1#*=}";                          check_v "$1" "$center";  shift;;
        -c*)                 center=$(echo "$1"  | sed -e's/-c//');     check_v "$1" "$center";  shift;;
        -s|--scale)          scale="$2";                                check_v "$1" "$scale";  shift;shift;;
        -s=*|--scale=*)      scale="${1#*=}";                           check_v "$1" "$scale";  shift;;
        -s*)                 scale=$(echo "$1"  | sed -e's/-f//');      check_v "$1" "$scale";  shift;;

        # Output parameters
        -k|--keeptmp)        keeptmp=1; shift;;
        -k*|--keeptmp=*)     on_off_option_error --keeptmp -k;;
        -m|--modelonly)      modelonly=1; shift;;
        -m*|--modelonly=*)   on_off_option_error --modelonly -m;;
        -t|--tmpdir)         tmpdir="$2";                          check_v "$1" "$tmpdir";  shift;shift;;
        -t=*|--tmpdir=*)     tmpdir="${1#*=}";                     check_v "$1" "$tmpdir";  shift;;
        -t*)                 tmpdir=$(echo "$1" | sed -e's/-t//'); check_v "$1" "$tmpdir";  shift;;
        -o|--output)         output="$2";                          check_v "$1" "$output"; shift;shift;;
        -o=*|--output=*)     output="${1#*=}";                     check_v "$1" "$output"; shift;;
        -o*)                 output=$(echo "$1" | sed -e's/-o//'); check_v "$1" "$output"; shift;;

        # Non-operating options.
        -q|--quiet)       quiet="--quiet"; shift;;
        -q*|--quiet=*)    on_off_option_error --quiet -q;;
        -?|--help)        print_help; exit 0;;
        -'?'*|--help=*)   on_off_option_error --help -?;;
        -V|--version)     print_version; exit 0;;
        -V*|--version=*)  on_off_option_error --version -V;;
        --cite)           print_citation; exit 0;;
        --cite=*)         on_off_option_error --cite;;

        # Unrecognized option:
        -*) echo "$scriptname: unknown option '$1'"; exit 1;;

        # Not an option (not starting with a `-'): assumed to be input FITS
        # file name.
        *) if [ x"$inputs" = x ]; then inputs="$1"; else inputs="$inputs $1"; fi; shift;;
    esac

done





# Basic sanity checks
# ===================

# If an input image is not given at all.
if [ x"$inputs" = x ]; then
    cat <<EOF
$scriptname: no input FITS image files. Run with '--help' for more information on how to run
EOF
    exit 1
elif [ ! -f $inputs ]; then
    cat <<EOF
$scriptname: $inputs, no such file or directory
EOF
    exit 1
fi

# If a PSF image (--psf) is not given at all.
if [ x"$psf" = x ]; then
    cat <<EOF
$scriptname: no PSF image provided. The PSF image should be provided with '--psf' (or '-p')
EOF
    exit 1
elif [ ! -f $psf ]; then
    cat <<EOF
$scriptname: $psf, no such file or directory
EOF
    exit 1
fi

# If a scale factor (--scale) is not given at all.
if [ x"$scale" = x ]; then
    cat <<EOF
$scriptname: no scale factor provided. It has to be given with '--scale' (or '-s'). You can derive this value with the 'astscript-psf-model-scale-factor'
EOF
    exit 1
fi

# If center coordinates (--center) is not given at all.
if [ x"$center" = x ]; then
    cat <<EOF
$scriptname: no center coordinates provided (for the star that should be subtracted). You can use '--center' ('-c')
EOF
    exit 1
else
    ncenter=$(echo $center | awk 'BEGIN{FS=","} \
                                  {for(i=1;i<=NF;++i) c+=$i!=""} \
                                  END{print c}')
    if [ x$ncenter != x2 ]; then
        cat <<EOF
$scriptname: '--center' (or '-c') only takes two values, but $ncenter were given in 'center'
EOF
        exit 1
    fi
fi
# If mode (--mode) is not given at all.
modeerrorinfo="Depending on the nature of the star's coordinates, please give either 'img' (for pixel coordinates) or 'wcs' (for RA,Dec) to the '--mode' (or '-O') option"
if [ x"$mode" = x ]; then
    cat <<EOF
$scriptname: no coordinate mode provided. $modeerrorinfo
EOF
    exit 1
# Make sure the value to '--mode' is either 'wcs' or 'img'.
elif [ "$mode" = wcs    -o    "$mode" = img ]; then
    junk=1
else
    cat <<EOF
$scriptname: '$mode' not acceptable for '--mode' (or '-O'). $modeerrorinfo
EOF
    exit 1
fi





# Center coordinates and default object label
# -------------------------------------------
#
# Obtain the coordinates of the center.
xcoord=$(echo "$center" | awk 'BEGIN{FS=","} {print $1}')
ycoord=$(echo "$center" | awk 'BEGIN{FS=","} {print $2}')

# With the center coordinates, generate a specific label for the object
# consisting in its coordinates.
objectid="$xcoord"_"$ycoord"





# Define a temporal directory and thefinal output file
# ----------------------------------------------------
#
# Construct the temporary directory. If the user does not specify any
# directory, then a default one with the base name of the input image will
# be constructed.  If the user set the directory, then make it. This
# directory will be deleted at the end of the script if the user does not
# want to keep it (with the `--keeptmp' option).

# The final output stamp is also defined here if the user does not provide
# an explicit name. If the user has defined a specific path/name for the
# output, it will be used for saving the output file. If the user does not
# specify a output name, then a default value containing the center and
# mode will be generated. Moreover, if the user specify the option
# --modelonly, then the output will be the modeled PSF (not subtracted from
# the original image).
bname_prefix=$(basename $inputs | sed 's/\.fits/ /' | awk '{print $1}')
if [ x"$tmpdir" = x ]; then \
  tmpdir=$(pwd)/"$bname_prefix"_subtracted
fi

if [ -d "$tmpdir" ]; then
  junk=1
else
  mkdir -p "$tmpdir"
fi

# Output, subtract or model only
if [ x"$output" = x ]; then
  if [ x"$modelonly" = x1 ]; then
    output="$bname_prefix"_model_$objectid.fits
  else
    output="$bname_prefix"_subtracted_$objectid.fits
  fi
fi





# Transform WCS to IMG center coordinates
# ---------------------------------------
#
# If the original coordinates have been given in WCS or celestial units
# (RA/DEC), then transform them to IMG (pixel). Here, this is done by using
# the WCS information from the original input image. If the original
# coordinates were done in IMG, then just use them.
if [ "$mode" = wcs ]; then
  xycenter=$(echo "$xcoord,$ycoord" \
                  | asttable  --column='arith $1 $2 wcs-to-img' \
                              --wcsfile=$inputs --wcshdu=$hdu $quiet)
  xcenter=$(echo "$xycenter" | awk '{print $1}')
  ycenter=$(echo "$xycenter" | awk '{print $2}')
else
  xcenter=$xcoord
  ycenter=$ycoord
fi





# Scale the PSF with the given factor
# -----------------------------------
#
# The PSF absolute pixel values are not relevant since they are used to be
# normalized somehow. Here, the input PSF is scaled (multiplied) by the
# specified factor (--fluxfactor) in order to appropiately obtain the PSF
# brightness.
psffluxscaled=$tmpdir/psf-flux-scaled-$objectid.fits
astarithmetic $psf --hdu=$psfhdu $scale float32 x \
              --output=$psffluxscaled $quiet





# Get the original inputs size
# ----------------------------
#
# In order to have the final PSF with the same size than the input image,
# it is necessary to compute the original size of the input image. It is
# possible that the input image is compressed as '.fz' file. In these
# situations, the keywords with the original dimesion of the array are
# ZNAXIS1 and ZNAXIS2, instead of NAXIS1 and NAXIS2. The former are the
# real size of the array (as it were not compressed), the latter are the
# size after compressing the data (as a table). If there are not ZNAXIS
# keywords, output values are "n/a". So, they are removed with sed, and
# then count the number of remaining values. Overall, when there are 4
# remaining keywords, the data is compressed, otherwise use the simple
# NAXIS1 and NAXIS2 keywords (that are always into the header).
axises=$(astfits $inputs --hdu=$hdu --quiet \
                 --keyvalue ZNAXIS1,ZNAXIS2,NAXIS1,NAXIS2)
naxises=$(echo $axises \
               | sed 's/n\/a//g' \
               | awk '{print NF}')
if [ x$naxises = x4 ]; then
    xaxis=$(echo $axises | awk '{print $1}')
    yaxis=$(echo $axises | awk '{print $2}')
else
    xaxis=$(echo $axises | awk '{print $3}')
    yaxis=$(echo $axises | awk '{print $4}')
fi





# Get the center of the PSF
# -------------------------
#
# The center of the PSF is assumed to be at the center of the PSF image.
# So, the center of the PSF is computed here from the size of the PSF image
# along the two axis (in pixels). Dimension values are computed as
# described above.
psfaxises=$(astfits $psf --hdu=$psfhdu --quiet \
                    --keyvalue ZNAXIS1,ZNAXIS2,NAXIS1,NAXIS2)
psfnaxises=$(echo $psfaxises \
                  | sed 's/n\/a//g' \
                  | awk '{print NF}')
if [ x$psfnaxises = x4 ]; then
    xpsfaxis=$(echo $psfaxises | awk '{print $1}')
    ypsfaxis=$(echo $psfaxises | awk '{print $2}')
else
    xpsfaxis=$(echo $psfaxises | awk '{print $3}')
    ypsfaxis=$(echo $psfaxises | awk '{print $4}')
fi

# To obtain the center of the PSF (in pixels), just divide by 2 the size of
# the PSF image.
xpsfcenter=$(astarithmetic $xpsfaxis float32 2.0 / --quiet)
ypsfcenter=$(astarithmetic $ypsfaxis float32 2.0 / --quiet)





# Translate the PSF image
# -----------------------
#
# In order to allocate the PSF into the center coordinates provided by the
# user, it is necessary to compute the appropiate offsets along the X and Y
# axis. After that, the PSF image is warped using that offsets.
xdiff=$(astarithmetic $xcenter float32 $xpsfcenter float32 - --quiet)
ydiff=$(astarithmetic $ycenter float32 $ypsfcenter float32 - --quiet)

psftranslated=$tmpdir/psf-translated-$objectid.fits
astwarp $psffluxscaled --translate=$xdiff,$ydiff \
                       --output=$psftranslated $quiet





# Crop the PSF image
# ------------------
#
# Once the PSF has been situated appropiately into the right position, it
# is necessary to crop it with a sub-pixel precision as well as with the
# same size than the original input image. Otherwise the subtraction of the
# PSF or scattered light field model would not be possible. Here, this
# cropping is done.
xrange=$(echo "$xdiff $xaxis $xcenter" \
              | awk '{if($1<0) \
                       {i=int($1); \
                        d=-1*i+1; \
                        min=d; max=$2+d-1;} \
                      else {min=1; max=$2} \
                      i=int($3); {min=min+1; max=max+1}
                      printf "%d:%d", min, max}')

yrange=$(echo "$ydiff $yaxis $ycenter" \
              | awk '{if($1<0) \
                       {i=int($1); \
                        d=-1*i+1; \
                        min=d; max=$2+d-1;} \
                      else {min=1; max=$2} \
                      i=int($3); {min=min+1; max=max+1}
                      printf "%d:%d", min, max}')

# Once the necessary crop parameters have been computed, use the option
# '--section' to get the PSF image with the correct size and the center
# situated where the user has specified.
psfcropped=$tmpdir/psf-cropped-$objectid.fits
astcrop $psftranslated --mode=img --section=$xrange,$yrange \
        --output=$psfcropped $quiet





# Final output: PSF-subtracted or PSF-model star
# ----------------------------------------------
#
# Here, the final output is generated. By default, the final result is the
# PSF-subtracted image. But, if the user uses the option --modelonly, then
# the output will be the modeled PSF (not subtraction of the PSF is done).
# In the vast majority of the cases, having zeros in the outer parts of the
# astronomical images is not a good idea. It is better to have them as nan
# values. However, for the scattered light field modeling, it is better to
# have the pixels where there are no flux as zero values. By having them as
# zero values, it is possible to make the addition of different modeled
# stars (using the PSF) or the subtraction without having nan values in the
# final subtracted image.
if [ x"$modelonly" = x0 ]; then
  astarithmetic $psfcropped --hdu=1 set-i \
                i i isblank 0 where set-psf \
                $inputs --hdu=$hdu psf - --output=$output $quiet
else
  astarithmetic $psfcropped --hdu=1 set-i \
                i i isblank 0 where --output=$output $quiet

fi





# Remove temporary files
# ----------------------
#
# If the user does not specify to keep the temporal files with the option
# `--keeptmp', then remove the whole directory.
if [ $keeptmp = 0 ]; then
    rm -r $tmpdir
fi
