#!/bin/sh
#
# Build a color image using asinh function to manipulate the pixel values
# with the goal of showing the low and high pixel values at the same time.
# Script based in Lupton et al. (2004) http://doi.org/10.1086/382245
#
# Run with '--help' for more information.
#
# Current maintainer:
#     Raul Infante-Sainz <infantesainz@gmail.com>
# Contributing authors:
#     Mohammad Akhlaghi <mohammad@akhlaghi.org>
#     Samane Raji <samaneraji@gmail.com>
# Copyright (C) 2021-2024 Free Software Foundation, Inc.
#
# Gnuastro is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# Gnuastro is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with Gnuastro. If not, see <http://www.gnu.org/licenses/>.



# Exit the script in the case of failure
set -e


# Save the current system language, and then change it to English to avoid
# problems in some parts of the code (AWK with `,' instead of `.' for
# decimal separator).
sys_lang=$LANG
export LANG=C
sys_lcnumeric=$LC_NUMERIC
export LC_NUMERIC="en_US.UTF-8"





# Default option values (can be changed with options on the command-line).
hdu=""
rhdu=1
globalhdu=""

# Minimum, weights, and zeropoint values
weight=""
minimum=""
zeropoint=""

# To control the asinh transformation, set both to 1 (scientific notation)
qbright_default=1.000000e+00
stretch_default=1.000000e+00

# For color, black, and gray regions
regions=""
grayval=""
colorval=""
coloronly=0
graykernelfwhm=0
colorkernelfwhm=0

# Linear range values. When normalizing the images to a new range, use the
# range 'newmin-newmax'. It is obtained by the following operations:
# OldRange = (OldMax - OldMin)
# NewRange = (NewMax - NewMin)
# NewValue = (((OldValue - OldMin) * NewRange) / OldRange) + NewMin
minvalrange=0.000
maxvalrange=100.0

# To enhance the image
bias=0.0
gamma=1.0
contrast=1.0
markoptions=""

quiet=""
tmpdir=""
keeptmp=0
checkparams=0
output="color-faint-gray.pdf"

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
Usage: $scriptname [OPTION] R.fits G.fits B.fits K.fits

This script is part of GNU Astronomy Utilities $version.

This script generates a color image from three different images (RGB). In order
to show the entire range of pixel values, an asinh transformation is used. For
more on the definiton of this algorithm see Lupton et al. 2004:

  https://doi.org/10.1086/382245

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
  -h, --hdu=STR           HDU/extension for the input channels.
  -t, --rhdu=STR          HDU/extension for the regions image.
  -g, --globalhdu=STR/INT Use this HDU for all inputs, ignore '--hdu'.
  -w, --weight=FLT        Relative weight for each input channel.
  -m, --minimum=FLT       Minimum value for each input channel.
  -z, --zeropoint=FLT     Zero point magnitude of each input channel.

 Asinh scaling parameters
  -Q, --qbright=FLT       Parameter for bringing out brighter features.
  -s, --stretch=FLT       Linear stretching parameter for faint features.

 Contrast, bias, and marks
  -b, --bias              Constant (bias) to add to all the pixels (linear).
  -c, --contrast          Change the contrast of the final image (linear).
  -G, --gamma             Gamma (nonlinear, overrides bias/contrast).
      --markoptions=STR   Options to add marks (passed to ConvertType).

 Color and gray parameters
      --coloronly         No grayscale regions, background in color (black).
      --regions=STR       Regions labeled image (color=2, black=1, gray=0).
      --grayval=FLT       Gray threshold (highest value to use grayscale).
      --colorval=FLT      Color threshold (lowest value to have color).
      --graykernelfwhm=FLT  Kernel FWHM for convolving the background image.
      --colorkernelfwhm=FLT Kernel FWHM for color separation ref. image.

 Output:
  -k, --keeptmp           Keep temporary/auxiliar files.
  -o, --output            Output color image name.
      --checkparams       Print distribution of values used to find params.

 Operating mode:
  -h, --help              Print this help list.
  -q, --quiet             Don't print the list.
  -V, --version           Print program version.
      --cite              BibTeX citation for this program.

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
Copyright (C) 2021-2024, Free Software Foundation, Inc.
License GPLv3+: GNU General public license version 3 or later.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written/developed by Raul Infante-Sainz and Mohammad Akhlaghi.
EOF
}


print_citation() {
    empty="" # needed for the ascii art!
    cat <<EOF

Thank you for using $scriptname (GNU Astronomy Utilities) $version

Citations and acknowledgement are vital for the continued work on Gnuastro.

Please cite the following record(s) and add the acknowledgement statement below in your work to support us. Please note that different Gnuastro programs may have different corresponding papers. Hence, please check all the programs you used. Don't forget to also include the version as shown above for reproducibility.

Paper introducing this script
-----------------------------
  @ARTICLE{astscript-color-faint-gray,
         author = {{Infante-Sainz}, Ra{\'u}l and {Akhlaghi}, Mohammad},
          title = "{Gnuastro: Visualizing the Full Dynamic Range in Color Images}",
        journal = {Research Notes of the American Astronomical Society},
       keywords = {Astronomy software, Astronomy data visualization, Open source software, Low surface brightness galaxies, 1855, 1968, 1866, 940, Astrophysics - Instrumentation and Methods for Astrophysics, Astrophysics - Astrophysics of Galaxies, Computer Science - Computer Vision and Pattern Recognition},
           year = 2024,
          month = jan,
         volume = {8},
         number = {1},
            eid = {10},
          pages = {10},
            doi = {10.3847/2515-5172/ad1aae},
  archivePrefix = {arXiv},
         eprint = {2401.03814},
   primaryClass = {astro-ph.IM},
         adsurl = {https://ui.adsabs.harvard.edu/abs/2024RNAAS...8...10I},
        adsnote = {Provided by the SAO/NASA Astrophysics Data System}
  }

Paper introducing Gnuastro (currently main citation)
----------------------------------------------------
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
    if [ "x$2" = x ]; then
        echo "$scriptname: '$1' doesn't take any values."
    else
        echo "$scriptname: '$1' (or '$2') doesn't take any values."
    fi
    exit 1
}

check_v() {
    if [ x"$2" = x ]; then
        echo "$scriptname: option '$1' requires an argument."
        echo "Try '$scriptname --help' for more information."
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
while [ $# -gt 0 ]
do
    case "$1" in
        # Input parameters.
        -g|--globalhdu)      globalhdu="$2";                            check_v "$1" "$globalhdu";  shift;shift;;
        -g=*|--globalhdu=*)  globalhdu="${1#*=}";                       check_v "$1" "$globalhdu";  shift;;
        -g*)                 globalhdu=$(echo "$1" | sed -e's/-g//');   check_v "$1" "$globalhdu";  shift;;
        -h|--hdu)            aux="$2";                                  check_v "$1" "$aux"; hdu="$hdu $aux"; shift;shift;;
        -h=*|--hdu=*)        aux="${1#*=}";                             check_v "$1" "$aux"; hdu="$hdu $aux"; shift;;
        -h*)                 aux="$(echo "$1"  | sed -e's/-h//')";      check_v "$1" "$aux"; hdu="$hdu $aux"; shift;;
        -t|--rhdu)           rhdu="$2";                                 check_v "$1" "$rhdu";  shift;shift;;
        -t=*|--rhdu=*)       rhdu="${1#*=}";                            check_v "$1" "$rhdu";  shift;;
        -t*)                 rhdu=$(echo "$1" | sed -e's/-t//');        check_v "$1" "$rhdu";  shift;;
        -w|--weight)         aux="$2";                                  check_v "$1" "$aux"; weight="$weight $aux"; shift;shift;;
        -w=*|--weight=*)     aux="${1#*=}";                             check_v "$1" "$aux"; weight="$weight $aux"; shift;;
        -w*)                 aux="$(echo "$1"  | sed -e's/-w//')";      check_v "$1" "$aux"; weight="$weight $aux"; shift;;
        -m|--minimum)        aux="$2";                                  check_v "$1" "$aux"; minimum="$minimum $aux"; shift;shift;;
        -m=*|--minimum=*)    aux="${1#*=}";                             check_v "$1" "$aux"; minimum="$minimum $aux"; shift;;
        -m*)                 aux="$(echo "$1"  | sed -e's/-m//')";      check_v "$1" "$aux"; minimum="$minimum $aux"; shift;;
        -z|--zeropoint)      aux="$2";                                  check_v "$1" "$aux"; zeropoint="$zeropoint $aux"; shift;shift;;
        -z=*|--zeropoint=*)  aux="${1#*=}";                             check_v "$1" "$aux"; zeropoint="$zeropoint $aux"; shift;;
        -z*)                 aux="$(echo "$1"  | sed -e's/-z//')";      check_v "$1" "$aux"; zeropoint="$zeropoint $aux"; shift;;

        -s|--stretch)        stretch="$2";                              check_v "$1" "$stretch";  shift;shift;;
        -s=*|--stretch=*)    stretch="${1#*=}";                         check_v "$1" "$stretch";  shift;;
        -s*)                 stretch=$(echo "$1"  | sed -e's/-s//');    check_v "$1" "$stretch";  shift;;
        -Q|--qbright)        qbright="$2";                              check_v "$1" "$qbright";  shift;shift;;
        -Q=*|--qbright=*)    qbright="${1#*=}";                         check_v "$1" "$qbright";  shift;;
        -Q*)                 qbright=$(echo "$1"  | sed -e's/-Q//');    check_v "$1" "$qbright";  shift;;

        -G|--gamma)          gamma="$2";                                check_v "$1" "$gamma";  shift;shift;;
        -G=*|--gamma=*)      gamma="${1#*=}";                           check_v "$1" "$gamma";  shift;;
        -G*)                 gamma=$(echo "$1"  | sed -e's/-G//');      check_v "$1" "$gamma";  shift;;
        -c|--contrast)       contrast="$2";                             check_v "$1" "$contrast";  shift;shift;;
        -c=*|--contrast=*)   contrast="${1#*=}";                        check_v "$1" "$contrast";  shift;;
        -c*)                 contrast=$(echo "$1"  | sed -e's/-c//');   check_v "$1" "$contrast";  shift;;
        -b|--bias)           bias="$2";                                 check_v "$1" "$bias";  shift;shift;;
        -b=*|--bias=*)       bias="${1#*=}";                            check_v "$1" "$bias";  shift;;
        -b*)                 bias=$(echo "$1"  | sed -e's/-b//');       check_v "$1" "$bias";  shift;;
        --markoptions)       markoptions="$2";                          check_v "$1" "$markoptions";  shift;shift;;
        --markoptions=*)     markoptions="${1#*=}";                     check_v "$1" "$markoptions";  shift;;

        --coloronly)        coloronly=1; shift;;
        --regions)          regions="$2";                              check_v "$1" "$regions";  shift;shift;;
        --regions=*)        regions="${1#*=}";                         check_v "$1" "$regions";  shift;;
        --grayval)          grayval="$2";                              check_v "$1" "$grayval";  shift;shift;;
        --grayval=*)        grayval="${1#*=}";                         check_v "$1" "$grayval";  shift;;
        --colorval)         colorval="$2";                             check_v "$1" "$colorval";  shift;shift;;
        --colorval=*)       colorval="${1#*=}";                        check_v "$1" "$colorval";  shift;;
        --graykernelfwhm)   graykernelfwhm="$2";                       check_v "$1" "$graykernelfwhm";  shift;shift;;
        --graykernelfwhm=*) graykernelfwhm="${1#*=}";                  check_v "$1" "$graykernelfwhm";  shift;;
        --colorkernelfwhm)   colorkernelfwhm="$2";                     check_v "$1" "$colorkernelfwhm";  shift;shift;;
        --colorkernelfwhm=*) colorkernelfwhm="${1#*=}";                check_v "$1" "$colorkernelfwhm";  shift;;

        # Output parameters
        --checkparams)    checkparams=1; shift;;
        -k|--keeptmp)     keeptmp=1; shift;;
        -k*|--keeptmp=*)  on_off_option_error --keeptmp -k;;
        -t|--tmpdir)      tmpdir="$2";                          check_v "$1" "$tmpdir";  shift;shift;;
        -t=*|--tmpdir=*)  tmpdir="${1#*=}";                     check_v "$1" "$tmpdir";  shift;;
        -t*)              tmpdir=$(echo "$1" | sed -e's/-t//'); check_v "$1" "$tmpdir";  shift;;
        -o|--output)      output="$2";                          check_v "$1" "$output"; shift;shift;;
        -o=*|--output=*)  output="${1#*=}";                     check_v "$1" "$output"; shift;;
        -o*)              output=$(echo "$1" | sed -e's/-o//'); check_v "$1" "$output"; shift;;

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
        *) inputs="$inputs $1"; shift;;
    esac
done





# Basic sanity checks
# -------------------
#
# If no input is given, let the user know.
if [ x"$inputs" = x ]; then
    echo "$scriptname: no input FITS files."
    echo "Run with '--help' for more information on how to run."
    exit 1
fi


# Inputs. Make sure three or four inputs have been given.
ninputs=$(echo "$inputs" | awk '{print NF}')
if [ $ninputs != 3 ] && [ $ninputs != 4 ]; then
    cat <<EOF
$scriptname: $ninputs inputs given, but 3 (or 4) inputs are necessary: one for each of the R,G,B,(K) channels, respectively. Run with '--help' for more information on how to run.
EOF
    exit 1
else
    rimage=$(echo $inputs | awk '{print $1}')
    gimage=$(echo $inputs | awk '{print $2}')
    bimage=$(echo $inputs | awk '{print $3}')
    kimage=$(echo $inputs | awk '{print $4}')
fi


# HDU. If the user provides a globalhdu use it, otherwise check that HDU
# number matches with input images
if [ x"$globalhdu" != x ]; then
    rhdu=$globalhdu
    ghdu=$globalhdu
    bhdu=$globalhdu
    khdu=$globalhdu
else
    nhdu=$(echo "$hdu" | awk '{print NF}')
    if [ x"$nhdu" != x"$ninputs" ]; then
        cat <<EOF
$scriptname: number of inputs and HDUs does not match. Every input FITS image needs a HDU (identified by name or number, counting from zero). You can use multiple calls to the '--hdu' ('-h') option for each input FITS image (in the same order as the input FITS files), or use '--globalhdu' ('-g') once when the same HDU should be used for all of them. Run with '--help' for more information on how to run.
EOF
        exit 1
    fi
    rhdu=$(echo "$hdu" | awk '{print $1}')
    ghdu=$(echo "$hdu" | awk '{print $2}')
    bhdu=$(echo "$hdu" | awk '{print $3}')
    khdu=$(echo "$hdu" | awk '{print $4}')
fi


# Minimum. If the user provides a single value for --minimum, use it for
# all channels. Otherwise, check that the number of minimums matches with
# the number of inputs.
nminimum=$(echo "$minimum" | awk '{print NF}')
if [ x$nminimum = x1 ]; then
    rmin=$minimum
    gmin=$minimum
    bmin=$minimum
    kmin=$minimum
elif [ $nminimum -gt 1 ] && [ x"$nminimum" != x"$ninputs" ]; then
        cat <<EOF
$scriptname: number of inputs ($ninputs) and minimum values ($nminimum) does not match. Every input FITS image needs a minimum value. You can use multiple calls to the '--minimum' ('-m') option for each input FITS image (in the same order as the input FITS files). If a single value is provided, it will be used for all channels. Run with '--help' for more information on how to run.
EOF
        exit 1
else
    rmin=$(echo "$minimum" | awk '{print $1}')
    gmin=$(echo "$minimum" | awk '{print $2}')
    bmin=$(echo "$minimum" | awk '{print $3}')
    kmin=$(echo "$minimum" | awk '{print $4}')

fi


# Zeropoint. If the user provides a single value for --zeropoint, use it
# for all channels. Otherwise, check that the number of zeropoints matches
# with the number of inputs.
nzeropoint=$(echo "$zeropoint" | awk '{print NF}')
if [ x$nzeropoint = x1 ]; then
    rzero=$zeropoint
    gzero=$zeropoint
    bzero=$zeropoint
    kzero=$zeropoint
elif [ $nzeropoint -gt 1 ] && [ x"$nzeropoint" != x"$ninputs" ]; then
        cat <<EOF
$scriptname: number of inputs ($ninputs) and zeropoint values ($nzeropoint) does not match. Every input FITS image needs a zeropoint value. You can use multiple calls to the '--zeropoint' ('-z') option for each input FITS image (in the same order as the input FITS files). If a single value is provided, it will be used for all channels. Run with '--help' for more information on how to run.
EOF
        exit 1
else
    rzero=$(echo "$zeropoint" | awk '{print $1}')
    gzero=$(echo "$zeropoint" | awk '{print $2}')
    bzero=$(echo "$zeropoint" | awk '{print $3}')
    kzero=$(echo "$zeropoint" | awk '{print $4}')
fi


# Weight. If the does not provide a value for weight, set it to 1.0. If
# user provides a single value for --weight, use it for all channels.
# Otherwise, check that the number of weights matches
# with the number of inputs.
nweight=$(echo "$weight" | awk '{print NF}')
if [ x$nweight = x0 ]; then
    rweight=1.0
    gweight=1.0
    bweight=1.0
elif [ x$nweight = x1 ]; then
    rweight=$weight
    gweight=$weight
    bweight=$weight
elif [ $nweight -gt 1 ] && [ x"$nweight" != x"$ninputs" ]; then
        cat <<EOF
$scriptname: number of inputs ($ninputs) and weight values ($nweight) does not match. Every input FITS image needs a weight value. You can use multiple calls to the '--weight' ('-w') option for each input FITS image (in the same order as the input FITS files). If a single value is provided, it will be used for all channels. Run with '--help' for more information on how to run.
EOF
        exit 1
else
    rweight=$(echo "$weight" | awk '{print $1}')
    gweight=$(echo "$weight" | awk '{print $2}')
    bweight=$(echo "$weight" | awk '{print $3}')
fi


# Bright. If the user provides --qbright, make sure it is not equal to zero
# (with 8 decimals), that would crash the asinh transformation.
if [ x$qbright != x ]; then
    qbright_check=$(echo "$qbright" | awk 'BEGIN{FS=","} {printf "%.8f", $1}')
    if [ x$qbright_check = x0.00000000 ]; then
        echo "$scriptname: --qbright value ($qbright) cannot be zero (8 decimals)."
        exit 1
    fi
else
    qbright=$qbright_default
fi


# Stretch. If the user provides --stretch, make sure it is not equal to
# zero (with 8 decimals), that would crash the asinh transformation.
if [ x$stretch != x ]; then
    stretch_check=$(echo "$stretch" | awk 'BEGIN{FS=","} {printf "%.8f", $1}')
    if [ x$stretch_check = x0.00000000 ]; then
        echo "$scriptname: --stretch value ($stretch) cannot be zero (8 decimals)."
        exit 1
    fi
else
    stretch=$stretch_default
fi






# Define the temporary directory
# -----------------------------
#
# Construct the temporary directory. If the user does not specify any
# directory, then a default one with will be constructed.  If the user
# set the directory, then make it. This directory will be deleted at
# the end of the script if the user does not want to keep it (with the
# `--keeptmp' option).
defaulttmpdir="color-faint-gray-tmp"
if [ x$tmpdir = x ]; then tmpdir=$defaulttmpdir; fi
if [ -d $tmpdir ]; then junk=1; else mkdir $tmpdir; fi





# Clip pixel values below the specified limit
# -------------------------------------------
#
# If the user specify a given value below which the pixel values are
# wanted to be zero, then put all of those pixels to zero. By default,
# no clipping will be done (a symbolic link will be put to the input).
rclipped="$tmpdir/R_clipped.fits"
gclipped="$tmpdir/G_clipped.fits"
bclipped="$tmpdir/B_clipped.fits"
kclipped="$tmpdir/K_clipped.fits"

if [ x"$rmin" = x ]; then
    cp $rimage $rclipped
else
    astarithmetic $rimage --hdu=$rhdu set-i $quiet \
                  i i $rmin lt 0 where --output=$rclipped
    rhdu=1
fi

if [ x"$gmin" = x ]; then
    cp $gimage $gclipped
else
    astarithmetic $gimage --hdu=$ghdu set-i $quiet \
                  i i $gmin lt 0 where --output=$gclipped
    ghdu=1
fi

if [ x"$bmin" = x ]; then
    cp $bimage $bclipped
else
    astarithmetic $bimage --hdu=$bhdu set-i $quiet \
                  i i $bmin lt 0 where --output=$bclipped
    bhdu=1
fi

# kclipped is constructed only if a fourth image has been given.
if [ x"$kmin" = x ] && [ x$kimage != x ]; then
    cp $kimage $kclipped
elif [ x$kimage != x ]; then
    astarithmetic $kimage --hdu=$khdu set-i $quiet \
                  i i $kmin lt 0 where --output=$kclipped
    khdu=1
fi





# Scale the images: weighting and converting to micro Janskys
# -----------------------------------------------------------
#
# All input images are scaled if different weights of 1 are provided. This
# step allow to easily weight the different bands differently. The
# weighting is done considering the specified weight values and setting the
# maximum value to 1. That means that the image with the highest weighting
# value will be not modified, while the other bands will be scaled
# appropiately. In addition to this, if zero points are provided, the
# images will be transformed to have micro Jansky values, this helps makes
# a more physically meaningful comparison between the filters.
rscaled="$tmpdir/R_scaled.fits"
gscaled="$tmpdir/G_scaled.fits"
bscaled="$tmpdir/B_scaled.fits"

# Compute normalized weights
weight_sum=$(astarithmetic $rweight f32 $gweight f32 $bweight f32 3 sum --type=f32 -q)
rweight_sum=$(astarithmetic $rweight f32 $weight_sum / -q | awk '{printf "%.4f", $1}')
gweight_sum=$(astarithmetic $gweight f32 $weight_sum / -q | awk '{printf "%.4f", $1}')
bweight_sum=$(astarithmetic $bweight f32 $weight_sum / -q | awk '{printf "%.4f", $1}')

wmax=$(astarithmetic $rweight_sum $gweight_sum $bweight_sum 3 max --type=f32 -q)

rweight_norm=$(astarithmetic $rweight_sum $wmax / -q | awk '{printf "%.4f", $1}')
gweight_norm=$(astarithmetic $gweight_sum $wmax / -q | awk '{printf "%.4f", $1}')
bweight_norm=$(astarithmetic $bweight_sum $wmax / -q | awk '{printf "%.4f", $1}')


# If all weights are 1.0000 and no zeropoints are provided, then symbolic link.
if [ x$rweight = x"1.0000" ] &&
   [ x$gweight = x"1.0000" ] &&
   [ x$bweight = x"1.0000" ] &&
   [ x"$rzero" = x ]; then
    ln -sf $rclipped $rscaled
    ln -sf $gclipped $gscaled
    ln -sf $bclipped $bscaled

# If diferent weights but same zeropoints, then scale the images.
elif [ x$rweight != x"1.0000" ] ||
     [ x$gweight != x"1.0000" ] ||
     [ x$bweight != x"1.0000" ] ||
     [ x"$rzero" = x ]; then
    # Just weight the images appropiately
    astarithmetic $rclipped -h$rhdu $rweight_norm x -o$rscaled $quiet
    astarithmetic $gclipped -h$ghdu $gweight_norm x -o$gscaled $quiet
    astarithmetic $bclipped -h$bhdu $bweight_norm x -o$bscaled $quiet
    rhdu=1; ghdu=1; bhdu=1

# Else, use weights and zeropoints to scale and transform to micro Jansky.
else
    # Consider zeropoints as float (8 decimals) numbers to homogeneize
    rzerof=$(echo $rzero | awk '{printf "%.8f", $1}')
    gzerof=$(echo $gzero | awk '{printf "%.8f", $1}')
    bzerof=$(echo $bzero | awk '{printf "%.8f", $1}')

    # Transform to pixel unit to micro Jansky
    astarithmetic $rclipped -h$rhdu $rzerof counts-to-jy 1e6 x \
                  $rweight_norm x --output=$rscaled $quiet
    astarithmetic $gclipped -h$ghdu $gzerof counts-to-jy 1e6 x \
                  $gweight_norm x --output=$gscaled $quiet
    astarithmetic $bclipped -h$bhdu $bzerof counts-to-jy 1e6 x \
                  $bweight_norm x --output=$bscaled $quiet
    rhdu=1; ghdu=1; bhdu=1
fi





# Stacked image: i_rgb = (R+G+B)/3
# --------------------------------
#
# The asinh transformation is done on the mean of RGB images. Here, this
# image is obtained by stacking them using the 'mean' operator. If the
# stacked image has exact zero pixel values, change them to nan values.
i_rgb_stack="$tmpdir/rgb_mean.fits"
astarithmetic $rscaled --hdu=$rhdu \
              $gscaled --hdu=$ghdu \
              $bscaled --hdu=$bhdu 3 mean \
               --output=$i_rgb_stack $quiet





# ASINH transformation
# --------------------
#
# Once the necessary parameters have been obtained, here the asinh
# transformation is done over the mean of RGB images. After the
# transformation is done, it is normalized by 'i_rgb'. Finally, the range
# of pixel values is linear transformed to [minvalrange - maxvalrange].
i_rgb_asinh="$tmpdir/rgb_mean-asinh.fits"
astarithmetic $i_rgb_stack -h1 set-i_rgb \
              i_rgb $qbright x $stretch x set-i \
              i asinh $qbright / --output=$i_rgb_asinh $quiet
i_rgb_asinh_norm="$tmpdir/rgb_mean-asinh-norm.fits"
astarithmetic $i_rgb_stack -h1 set-stack \
              $i_rgb_asinh -h1 set-asinh \
              asinh stack / --output=$i_rgb_asinh_norm $quiet





# Compute the r, g, b images
# --------------------------
#
# Each input image is multiplied by the transformed and normalized averaged
# image (i_rgb_asinh_norm).
i_r="$tmpdir/r.fits"
i_g="$tmpdir/g.fits"
i_b="$tmpdir/b.fits"
astarithmetic $rscaled --hdu=$rhdu $i_rgb_asinh_norm x $quiet --output=$i_r
astarithmetic $gscaled --hdu=$ghdu $i_rgb_asinh_norm x $quiet --output=$i_g
astarithmetic $bscaled --hdu=$bhdu $i_rgb_asinh_norm x $quiet --output=$i_b





# Normalize by the maximum value of R, G, B
# -----------------------------------------
#
# Compute the maximum of the R, G, B images and divide each image by that
# value. This step is done in order to represent the true color.
i_r_norm="$tmpdir/r_norm.fits"
i_g_norm="$tmpdir/g_norm.fits"
i_b_norm="$tmpdir/b_norm.fits"
maxr=$(aststatistics $i_r --maximum -q)
maxg=$(aststatistics $i_g --maximum -q)
maxb=$(aststatistics $i_b --maximum -q)
maxrgb=$(astarithmetic $maxr float64 $maxg float64 $maxb \
                       float64 3 max -q)
astarithmetic $i_r $maxrgb / $maxvalrange x float32 -o$i_r_norm $quiet
astarithmetic $i_g $maxrgb / $maxvalrange x float32 -o$i_g_norm $quiet
astarithmetic $i_b $maxrgb / $maxvalrange x float32 -o$i_b_norm $quiet





# Transformations to enhance the image
# ------------------------------------
#
# Modify the images that are going to be used for obtaining the COLOR
# image. There are two possible options: linear and NON-linear
# transformation. For the linear transformation, two parameters are used,
# contrast and bias:
#
#  OUTPUT = contrast * INPUT + bias
#
# In the NON-linear transformation only one parameter is used, gamma:
#
#  OUTPUT = INPUT**gamma
#
# After this transformation is done, all values are clipped by the
# maxvalrange value. So, all pixels above maxrangevalue become equal to
# maxrangevalue.
i_r_enhanced="$tmpdir/r_enhanced.fits"
i_g_enhanced="$tmpdir/g_enhanced.fits"
i_b_enhanced="$tmpdir/b_enhanced.fits"
if [ x"$gamma" != x"1.0" ]; then
  astarithmetic $i_r_norm $maxvalrange / $gamma pow $maxvalrange x set-t \
                t t $maxvalrange gt $maxvalrange where \
                --output=$i_r_enhanced $quiet
  astarithmetic $i_g_norm $maxvalrange / $gamma pow $maxvalrange x set-t \
                t t $maxvalrange gt $maxvalrange where \
                --output=$i_g_enhanced $quiet
  astarithmetic $i_b_norm $maxvalrange / $gamma pow $maxvalrange x set-t \
                t t $maxvalrange gt $maxvalrange where \
                --output=$i_b_enhanced $quiet

elif [ x"$contrast" != x"1.0" ] || [ x$bias != x"0.0" ]; then
  astarithmetic $i_r_norm $contrast x $bias + set-t \
                t t $maxvalrange gt $maxvalrange where \
                --output=$i_r_enhanced $quiet
  astarithmetic $i_g_norm $contrast x $bias + set-t \
                t t $maxvalrange gt $maxvalrange where \
                --output=$i_g_enhanced $quiet
  astarithmetic $i_b_norm $contrast x $bias + set-t \
                t t $maxvalrange gt $maxvalrange where \
                --output=$i_b_enhanced $quiet

else
    cp $i_r_norm $i_r_enhanced    # Not using 'ln -s' because there will
    cp $i_g_norm $i_g_enhanced    # be too many links and 'realpath' is
    cp $i_b_norm $i_b_enhanced    # not portable on non-GNU OSs.
fi





# If the user only wants colored pixels (with black background).
if [ x$coloronly = x1 ]; then

    # Make the color figure
    # ---------------------
    #
    # Once all the previous treatment has been done for each image, then combine
    # all with Convert program to obtain the colored image. Limit fluxes used
    # here correspond to the gray-computed ones in order to obtain exactly the
    # same color as the gray-background image (for those pixels that are not
    # background).
    astconvertt $i_r_enhanced -h1 \
                $i_g_enhanced -h1 \
                $i_b_enhanced -h1 \
                $markoptions      \
                --output=$output $quiet

    # Remove images
    if [ $keeptmp = 0 ]; then
      rm $rscaled $gscaled $bscaled \
         $rclipped $gclipped $bclipped
    fi


# If user wants the gray background image
else

    # In this case, the color map is the following:
    #   Bright pixels are shown in color.
    #   Fainter pixels are shown in black.
    #   Background pixels are shown in gray.
    # As a consequence, two different thresholds need to be defined:
    # colorval and grayval. They are defined from the threshold image.

    # Regions:
    # Threshold image:
    # Final channel images:
    # ----------------------------------------------
    # bright-color    | faint-black  |  back-gray
    # 0   20   40     |    60  70    |  80  100
    # R: 90 80 10 ... |    0 0 0     | ... 10 50 100
    # G: 80 70 20 ... |    0 0 0     | ... 10 50 100
    # B: 90 60 30 ... |    0 0 0     | ... 10 50 100

    # In words. The bright pixels are formed from the three channels, each
    # with different values for the same pixel.

    # The faint region (in black) can be shown in color but dark because
    # the pixels have lower values in the three channels. These pixels go
    # close to zero in the channels.

    # Then, the background is shown in gray. To do it, the same pixel value
    # are set for the different channels. In this case, lower but equal
    # pixel values mean dark gray, while high and same pixel values mean
    # white.

    # In the situation above there is no pure black regions because the
    # transition from color to gray is smooth ('colorval'='grayval'). But
    # it is possible to define a region of pure black pixels. They are set
    # from the --colorval and --grayval thresholds. The pixels between
    # these values in the threshold image are set to zero in the three
    # channels.


    # Background image
    # ----------------
    #
    # If three images are provided, we use the stacked and
    # asinh-transformed image for the background. Otherwise, if four images
    # are provided, we use the fourth image with no modifications for the
    # background.
    i_colorgray_threshold="$tmpdir/colorgray_threshold.fits"
    if [ x$ninputs = x3 ]; then
        i_back=$i_rgb_asinh
        khdu=1

        # Convolve the background image
        # -----------------------------
        #
        # If the user wants to convolve the background image.
        i_back_convolved="$tmpdir/back_convolved.fits"
        if [ $graykernelfwhm = 0 ]; then
          cp $i_back $i_back_convolved
        else
          i_back_kernel="$tmpdir/back_kernel.fits"
          astmkprof --kernel=gaussian,$graykernelfwhm,3 \
                    --oversample=1 --output=$i_back_kernel $quiet
          astconvolve $i_back --hdu=$khdu --kernel=$i_back_kernel \
                      --domain=spatial --output=$i_back_convolved $quiet
        fi

        # Change pixel values to the wanted range
        astarithmetic $i_back_convolved -h1 set-image \
                      image minvalue set-oldmin \
                      image maxvalue set-oldmax \
                      $minvalrange set-newmin \
                      $maxvalrange set-newmax \
                      oldmax oldmin - set-oldrange \
                      newmax newmin - set-newrange \
                      image oldmin - newrange x oldrange / newmin + \
                      float32 --output=$i_colorgray_threshold $quiet

    else
        i_back_convolved=$kclipped
        ln -sf $kclipped $i_colorgray_threshold
    fi





    # Find the thresholds
    # -------------------
    #
    # The color image with the gray background is constructed by separating the
    # original image into two regimes: COLOR and GRAY. Here, the pixel value
    # used for separate these two regions is computed. It is obtained from the
    # asinh-transformed image. If the user does not provide a value then use
    # ghe computed one (guessed). If the user provide a value, then use it
    # directly. Note that the guessed value is computed in any case.
    colorval_default=$(aststatistics $i_colorgray_threshold \
                                       --median --quiet)
    if [ x$colorval = x"" ]; then
      colorval=$colorval_default
    fi

    grayval_default=$(aststatistics $i_colorgray_threshold \
                                      --median --quiet)
    if [ x$grayval = x"" ]; then
      grayval=$grayval_default
    fi





    # Regions labeled image
    # ---------------------
    #
    # The TOTAL_MASK consists of a labeled image whose pixel values
    # correspond to the three regions. Labels are defined as follows:
    #   2 will be shown in color
    #   1 will be shown in pure black
    #   0 will be shown in gray
    # This image is computed by default from the colorval and grayval
    # parameters. Alternatively, it can be provided by the user.
    if [ x$regions = x ]; then
        total_mask="$tmpdir/total_mask-2color-1black-0gray.fits"
        astarithmetic $i_colorgray_threshold                     set-i \
                      i $colorval gt                   2 uint8 x set-c \
                      i $colorval lt i $grayval gt and 1 uint8 x set-b \
                      i $colorval lt                   0 uint8 x set-g \
                      c b g 3 sum uint8 --output $total_mask
        rhdu=1
   else
       grayval="$regions"
       colorval="$regions"
       total_mask=$regions
   fi





    # Gray background image
    # ---------------------
    #
    # We use the specified gray background image after some modifications to
    # obtain the GRAY part. For futher highlight the interesting diffuse flux,
    # we need to invert it the brigher pixels are darker. Here, the color
    # region is masked (pixels=2 in TOTAL_MASK)
    #
    # Here is the logic of the steps we take:
    #   1. We call the convolved image 'values'.
    #   2. We call the mask image 'mask'.
    #   3. Pixels equal to 1 in 'mask' are set to nan in 'values': 'masked'.
    #   4. The min value of this image is 'oldmin'.
    #   5. The max value of this image is 'oldmax'.
    #   6. The new min value of the final image is 'newmin'.
    #   7. The new max value of the final image is 'newmax'.
    #   (Note that they are equal but inverted of the general range values).
    #   7. The old range (oldmax - oldmin) is 'oldrange'.
    #   8. The new range (newmax - newmin) is 'newrange'.
    #   9. The 'masked' image is transformed to have the new range of values.
    #   (Note that 'grayscale' will transform the pixel value in case of any
    #   function is specified. E.g., log, sqrt, asinh, etc.)
    grayscale=""
    i_gray_colormasked="$tmpdir/gray_colormasked.fits"
    astarithmetic $i_back_convolved -h1          set-values \
                  $total_mask -h$rhdu 2 uint8 eq set-mask \
                  values mask nan where $grayscale set-masked \
                  masked minvalue set-oldmin \
                  masked maxvalue set-oldmax \
                  $maxvalrange set-newmin \
                  $minvalrange set-newmax \
                  oldmax oldmin - set-oldrange \
                  newmax newmin - set-newrange \
                  masked oldmin - newrange x oldrange / newmin + \
                  set-transformed \
                  transformed float32 --output=$i_gray_colormasked $quiet





    # Set the pure black region to zero pixel values
    # ----------------------------------------------
    #
    # Put black pixels (pixels=1 in TOTAL_MASK) equal to zero. By doing
    # this, those pixels will be set to pure black color.
    i_gray_colormasked_zeroblack="$tmpdir/gray_colormasked_zeroblack.fits"
    astarithmetic $i_gray_colormasked -h1 set-i \
                  $total_mask -h$rhdu 1 uint8 eq -h1 set-b \
                  i b 0.0 where float32 \
                  --output=$i_gray_colormasked_zeroblack





    # Use color for high S/N and gray for low S/N
    # -------------------------------------------
    #
    # Once the gray-mask has been obtained, use it for replace all pixels less
    # than the gray-threshold with the pixels of the reference image. This is
    # done for each input image (band).  Output images end with -gray.fits,
    # generated into the for loop.
    i_r_black_gray="$tmpdir/r_black_gray.fits"
    i_g_black_gray="$tmpdir/g_black_gray.fits"
    i_b_black_gray="$tmpdir/b_black_gray.fits"
    for f in $i_r_enhanced $i_g_enhanced $i_b_enhanced; do
      outputname=$(echo "$f" | sed -e's/_enhanced.fits/_black_gray.fits/')
      astarithmetic $f \
                    $i_gray_colormasked_zeroblack isblank not \
                    $i_gray_colormasked_zeroblack \
                    where -g1 --output=$outputname $quiet;
    done





    # Make the color-gray image
    # -------------------------
    #
    # Once all the previous treatment has been done for each image, then
    # combine all with Convert program to obtain the colored image (with the
    # low S/N regions in gray).
    astconvertt $i_r_black_gray -h1 \
                $i_g_black_gray -h1 \
                $i_b_black_gray -h1 \
                $markoptions      \
                --output=$output $quiet
fi





# Information to help and warn the user
# -------------------------------------
#
# If the quiet mode is not activated, then print valuable information to
# help the user in guessing appropiate parameters for making the color
# image. In the same way, if the images come with astrometric information
# (WCS) the relative variations in pixel scales and alignment of them are
# provided. This is useful to know if color-gradients on the color image
# could be due to a bad alignment of the images or different pixel scales.

if [ ! x$quiet = x"--quiet" ]; then

  # Print on the command line the statistics of the image and the parameters
  # that are going to be used. This will helps to guess appropiate parameters
  if [ x$checkparams = x1 ]; then
  echo "                   "
  echo "For asinh-transformation ('--strech' and '--qbright' parameters)."
  aststatistics $i_rgb_stack

  echo "                   "
  echo "For color and gray thresholds."
  echo "separation between color and black regions (--colorval)"
  echo "separation between black and gray regions (--grayval)"
  aststatistics $i_colorgray_threshold

  fi

  cat <<EOF

TIPS:

  # If there are exact zero-valued pixels (e.g., in the border/outer parts
    of the images), convert them to NaN: zero is statistically meaningful
    and can interfere with the statistics of this script; see the tutorial.
  # Use the default options to estimate the parameters.
  # Select a good background value of the images:
      A minimum value of zero could be a good option: '--minimum=0.0'
  # Focus on the bright regions and tweak '--qbright' and '--stretch':
      First, try low values of '--qbright' to show the bright parts.
      Then, adjust '--stretch' to show the fainter regions around bright parts.
      Overall, play with these two parameters to show the color regions appropriately.
  # Change '--colorval' to separate the color and black regions:
      This is the lowest value of the threshold image that is shown in color.
  # Change '--grayval' to separate the black and gray regions:
      This is highest value of the threshold image that is shown in gray.
  # Schema for these parameters:
      Output regions:   |  GRAY  |   BLACK   |   COLOR   |
      Threshold values: 0 --- grayval --- colorval --- 100
  # Use '--checkparams' to check the pixel value distributions.
  # Use '--keeptmp' to not remove the threshold image and check it:
      '$i_colorgray_threshold'
EOF
  echo
  printf "%-17s %-15s %-15s\n"   Option       Default           Used
  printf "  %-15s %-15g %-15g\n" "--qbright"  $qbright_default  $qbright
  printf "  %-15s %-15g %-15g\n" "--stretch"  $stretch_default  $stretch
  if [ x$regions = x ]; then
      printf "  %-15s %-15g %-15g\n" "--colorval" $colorval_default $colorval
      printf "  %-15s %-15g %-15g\n" "--grayval"  $grayval_default  $grayval
  fi
  echo; echo "Output written to '$output'."
fi





# Remove temporary files
# ---------------------
#
# If the user does not specify to keep the temporary files with the option
# `--keeptmp', then remove the whole directory.
if [ $keeptmp = 0 ]; then
    rm -rf $tmpdir
fi





# The script has finished, reset the original language to the system's
# default language.
export LANG=$sys_lang
export LC_NUMERIC=$sys_lcnumeric
