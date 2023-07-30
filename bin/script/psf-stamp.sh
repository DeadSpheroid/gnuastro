#!/bin/sh

# Construct a star stamp to build a Point Spread Function (PSF). This
# script will consider a center position and then it will crop the original
# image around that center with the specified size. After that, it will
# compute the radial profile to obtain a normalization value. Finally, the
# output stamp will be normalized by dividing by the normalization value.
# More options like providing a segmentation map are also available. The
# main goal of this script is to generate stamps to combine them and get an
# extended PSF from the stamp of stars. Run with `--help', or see
# description under `print_help' (below) for more.
#
# Original author:
#   Raul Infante-Sainz <infantesainz@gmail.com>
# Contributing author(s):
#   Mohammad Akhlaghi <mohammad@akhlaghi.org>
#   Carlos Morales-Socorro <cmorsoc@gmail.com>
#   Sepideh Eskandarlou <sepideh.eskandarlou@gmail.com>
# Copyright (C) 2021-2023 Free Software Foundation, Inc.
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
quiet=""
center=""
keeptmp=0
output=""
tmpdir=""
segment=""
snthresh=""
axisratio=1
normradii=""
sigmaclip=""
widthinpix=""
nocentering=0
normop="median"
positionangle=0
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

This script will consider a center position to make a normalized stamp. It
is intendeed to generate several stamps and combine them to obtain a PSF.

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
  -O, --mode=STR          Coordinates mode ('wcs' or 'img').
  -c, --center=FLT,FLT    Center coordinates of the object.
  -W, --widthinpix=INT,INT Width of the stamp in pixels.
  -T, --snthresh=FLT      Mask pixels below this S/N (only when
                          '--segment' is given).
  -n, --normradii=INT,INT Minimum and maximum radii (in pixels)
                          for computing the normalization value.
  -S, --segment=STR       Output of Segment (with OBJECTS and CLUMPS).
  -N, --normop=STR        Operator for computing the normalization value
                          (mean, sigclip-mean, etc.).
  -Q, --axis-ratio=FLT    Axis ratio for ellipse maskprofile (A/B).
  -p, --position-angle=FLT Position angle for ellipse mask profile.
  -s, --sigmaclip=FLT,FLT Sigma-clip multiple and tolerance.
  -d, --nocentering       Don't warp the center coord to central pixel.

 Output:
  -o, --output            Output table with the radial profile.
  -t, --tmpdir            Directory to keep temporary files.
  -k, --keeptmp           Keep temporal/auxiliar files.

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
Copyright (C) 2020-2023 Free Software Foundation, Inc.
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

all_nan_warning() {
    cat <<EOF
$scriptname: WARNING: all the pixels in the requested normalization radius are NaN. Therefore the output stamp will be all NaN. If you later use this stamp (among others) with any stack operator of 'astarithmetic' this image will effectively be ignored, so you can safely continue with stacking all the files
EOF
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
        -n|--normradii)      normradii="$2";                            check_v "$1" "$normradii";  shift;shift;;
        -n=*|--normradii=*)  normradii="${1#*=}";                       check_v "$1" "$normradii";  shift;;
        -n*)                 normradii=$(echo "$1"  | sed -e's/-n//');  check_v "$1" "$normradii";  shift;;
        -W|--widthinpix)     widthinpix="$2";                           check_v "$1" "$widthinpix";  shift;shift;;
        -W=*|--widthinpix=*) widthinpix="${1#*=}";                      check_v "$1" "$widthinpix";  shift;;
        -W*)                 widthinpix=$(echo "$1"  | sed -e's/-W//'); check_v "$1" "$widthinpix";  shift;;
        -T|--snthresh)       snthresh="$2";                             check_v "$1" "$snthresh";  shift;shift;;
        -T=*|--snthresh=*)   snthresh="${1#*=}";                        check_v "$1" "$snthresh";  shift;;
        -T*)                 snthresh=$(echo "$1"  | sed -e's/-W//');   check_v "$1" "$snthresh";  shift;;
        -S|--segment)        segment="$2";                              check_v "$1" "$segment";  shift;shift;;
        -S=*|--segment=*)    segment="${1#*=}";                         check_v "$1" "$segment";  shift;;
        -S*)                 segment=$(echo "$1"  | sed -e's/-S//');    check_v "$1" "$segment";  shift;;
        -O|--mode)           mode="$2";                                 check_v "$1" "$mode";  shift;shift;;
        -O=*|--mode=*)       mode="${1#*=}";                            check_v "$1" "$mode";  shift;;
        -O*)                 mode=$(echo "$1"  | sed -e's/-O//');       check_v "$1" "$mode";  shift;;
        -c|--center)         center="$2";                               check_v "$1" "$center";  shift;shift;;
        -c=*|--center=*)     center="${1#*=}";                          check_v "$1" "$center";  shift;;
        -c*)                 center=$(echo "$1"  | sed -e's/-c//');     check_v "$1" "$center";  shift;;
        -N|--normop)         normop="$2";                               check_v "$1" "$normop";  shift;shift;;
        -N=*|--normop=*)     normop="${1#*=}";                          check_v "$1" "$normop";  shift;;
        -N*)                 normop=$(echo "$1"  | sed -e's/-N//');     check_v "$1" "$normop";  shift;;
        -s|--sigmaclip)      sigmaclip="$2";                            check_v "$1" "$sigmaclip";  shift;shift;;
        -s=*|--sigmaclip=*)  sigmaclip="${1#*=}";                       check_v "$1" "$sigmaclip";  shift;;
        -s*)                 sigmaclip=$(echo "$1"  | sed -e's/-s//');  check_v "$1" "$sigmaclip";  shift;;
        -Q|--axis-ratio)     axisratio="$2";                            check_v "$1" "$axisratio";  shift;shift;;
        -Q=*|--axis-ratio=*) axisratio="${1#*=}";                       check_v "$1" "$axisratio";  shift;;
        -Q*)                 axisratio=$(echo "$1"  | sed -e's/-Q//');  check_v "$1" "$axisratio";  shift;;
        -p|--position-angle)       positionangle="$2";                                  check_v "$1" "$positionangle";  shift;shift;;
        -p=*|--position-angle=*)   positionangle="${1#*=}";                             check_v "$1" "$positionangle";  shift;;
        -p*)                       positionangle=$(echo "$1"  | sed -e's/-p//');        check_v "$1" "$positionangle";  shift;;



        # Output parameters
        -k|--keeptmp)     keeptmp=1; shift;;
        -k*|--keeptmp=*)  on_off_option_error --keeptmp -k;;
        -t|--tmpdir)      tmpdir="$2";                          check_v "$1" "$tmpdir";  shift;shift;;
        -t=*|--tmpdir=*)  tmpdir="${1#*=}";                     check_v "$1" "$tmpdir";  shift;;
        -t*)              tmpdir=$(echo "$1" | sed -e's/-t//'); check_v "$1" "$tmpdir";  shift;;
        -d|--nocentering)     nocentering=1; shift;;
        -d*|--nocentering=*)  on_off_option_error --nocentering -d;;
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
        *) if [ x"$inputs" = x ]; then inputs="$1"; else inputs="$inputs $1"; fi; shift;;
    esac

done





# Basic sanity checks
# ===================

# If an input image is not given at all.
if [ x"$inputs" = x ]; then
    cat <<EOF
$scriptname: ERROR: no input FITS image files (outer part of the PSF to unite with an inner part). Run with '--help' for more information on how to run
EOF
    exit 1
elif [ ! -f $inputs ]; then
    cat <<EOF
$scriptname: ERROR: $inputs, no such file or directory
EOF
    exit 1
fi

# Find the size of the image and check if it is an odd or even number
xsize=$(astfits $inputs --hdu=$hdu --keyvalue=NAXIS1 --quiet)
ysize=$(astfits $inputs --hdu=$hdu --keyvalue=NAXIS2 --quiet)
xsizetype=$(echo $xsize | awk '{if($1%2) print "odd"; else print "even"}')
ysizetype=$(echo $ysize | awk '{if($1%2) print "odd"; else print "even"}')

# If center coordinates (--center) is not given, assume it is centered.
if [ x"$center" = x ]; then
    # Find the center of the image
    xcoord=$(echo $xsize | awk '{if($1%2) print int($1/2); else print int($1/2)+1}')
    ycoord=$(echo $ysize | awk '{if($1%2) print int($1/2); else print int($1/2)+1}')

    # The center has been computed in pixels, assume pixel coordinates mode
    mode=img
    if [ x"$quiet" = x ]; then
    cat <<EOF
$scriptname: WARNING: no center provided ('--center' or '-c'). Considering that the center of the image is $xcoord,$ycoord
EOF
    fi
else
    ncenter=$(echo $center | awk 'BEGIN{FS=","} \
                                  {for(i=1;i<=NF;++i) c+=$i!=""} \
                                  END{print c}')
    if [ x$ncenter != x2 ]; then
        cat <<EOF
$scriptname: ERROR: '--center' (or '-c') only takes two values, but $ncenter were given in '$center'
EOF
        exit 1
    fi
    # Obtain the coordinates of the center.
    xcoord=$(echo "$center" | awk 'BEGIN{FS=","} {print $1}')
    ycoord=$(echo "$center" | awk 'BEGIN{FS=","} {print $2}')
fi

# If a stamp width (--widthinpix) is not given, assume the same size than
# the input.  If the input image has an odd number of pixels, it is fine.
# Otherwise, if the input image has an even number of pixels, then it has
# to be cropped around the center position and the size will be set to an
# odd number of pixels. In this case print a warnings.
if [ x"$widthinpix" = x ]; then

    if [ x"$xsizetype" = xeven ]; then
        xwidthinpix=$(echo $xsize | awk '{print $1+1}')
        if [ x"$quiet" = x ]; then
            cat <<EOF
$scriptname: WARNING: the image along the first dimension (NAXIS1) has an even number of pixels ($xsize). This may cause some problems with the centering of the stamp. Please, check and consider providing an image with an odd number of pixels
EOF
        fi
    elif [ x"$xsizetype" = xodd ]; then
        xwidthinpix=$xsize
    else
        cat <<EOF
$scriptname: ERROR: the size of the image along the first dimension (NAXIS1) can not be obtained
EOF
        exit 1
    fi
    if [ x"$ysizetype" = xeven ]; then
        ywidthinpix=$(echo $ysize | awk '{print $1+1}')
        if [ x"$quiet" = x ]; then
            cat <<EOF
$scriptname: WARNING: the image along the second dimension (NAXIS2) has an even number of pixels ($ysize). This may cause some problems with the centering of the stamp. Please, check and consider providing an image with an odd number of pixels
EOF
        fi
    elif [ x"$ysizetype" = xodd ]; then
        ywidthinpix=$ysize
    else
        cat <<EOF
$scriptname: ERROR: the size of the image along the second dimension (NAXIS2) can not be obtained
EOF
        exit 1
    fi
    if [ x"$quiet" = x ]; then
        cat <<EOF
$scriptname: WARNING: no stamp width provided ('--widthinpix' or '-W'). Considering that the stamp width is $xwidthinpix x $ywidthinpix
EOF
    fi
else
    nwidthinpix=$(echo $widthinpix | awk 'BEGIN{FS=","}END{print NF}')
    if [ x$nwidthinpix != x2 ]; then
        cat <<EOF
$scriptname: '--widthinpix' (or '-W') only take two values, but $nwidthinpix were given in '$widthinpix'
EOF
        exit 1
    fi
    # Obtain the coordinates of the center.
    xwidthinpix=$(echo "$widthinpix" | awk 'BEGIN{FS=","} {print $1}')
    ywidthinpix=$(echo "$widthinpix" | awk 'BEGIN{FS=","} {print $2}')
fi

# If a normalization range is not given at all.
if [ x"$normradii" = x ]; then
    if [ x"$quiet" = x ]; then
        cat <<EOF
$scriptname: WARNING: no ring of normalization provided ('--normradii' or '-n'). The stamp won't be normalized
EOF
    fi
else
    nnormradii=$(echo $normradii | awk 'BEGIN{FS=","}END{print NF}')
    if [ x$nnormradii != x2 ]; then
        cat <<EOF
$scriptname: ERROR: '--normradii' (or '-n') only take two values, but $nnormradii were given in '$normradii'
EOF
        exit 1
    fi
    # Obtain the minimum/maximum normalization radii.
    normradiusmin=$(echo "$normradii" | awk 'BEGIN{FS=","} {print $1}')
    normradiusmax=$(echo "$normradii" | awk 'BEGIN{FS=","} {print $2}')
fi

# Make sure the value to '--mode' is either 'wcs' or 'img'. Note: '-o'
# means "or" and is preferred to '[ ] || [ ]' because only a single
# invocation of 'test' is done. Run 'man test' for more.
if [ "$mode" = wcs     -o      $mode = "img" ]; then
    junk=1
else
    cat <<EOF
$scriptname: ERROR: value to '--mode' (or '-O') is not recognized ('$mode'). This option takes one of the following two values: 'img' (for pixel coordinates) or 'wcs' (for celestial coordinates)
EOF
    exit 1
fi





# Define a temporal directory and the final output file
# -----------------------------------------------------
#
# Construct the temporary directory. If the user does not specify any
# directory, then a default one with the base name of the input image will
# be constructed.  If the user set the directory, then make it. This
# directory will be deleted at the end of the script if the user does not
# want to keep it (with the `--keeptmp' option).

# The final output stamp is also defined here if the user does not provide
# an explicit name. If the user has defined a specific path/name for the
# output, it will be used for saving the output file. If the user does not
# specify a output name, then a default value containing the center will be
# generated.
objectid="$xcoord"_"$ycoord"
bname_prefix=$(basename "$inputs" | sed 's/\.fits/ /' | awk '{print $1}')
if [ x"$tmpdir" = x ]; then \
  tmpdir=$(pwd)/"$bname_prefix"_stamp
fi

if [ -d "$tmpdir" ]; then
  junk=1
else
  mkdir -p "$tmpdir"
fi

# Output
if [ x"$output" = x ]; then
  output="$bname_prefix"_stamp_$objectid.fits
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





# Crop the original image around the object
# -----------------------------------------
#
# Crop the object around its center with the given stamp size width. It may
# happen that the given coordinate is fully outside of the image (within
# the requested stamp-width). In this case Crop won't generate any output,
# so we are checking the existance of the '$cropped' file. If not created
# we will create a fully NaN-valued image that can be ignored in stacks.
#
# We are setting '--checkcenter=0' in Crop so it doesn't check if the
# central pixel is covered or not (we may be interested in the outer parts
# of the PSF of a star that is centered outside of the image).
cropped=$tmpdir/cropped-$objectid.fits
astcrop $inputs --hdu=$hdu --mode=img --checkcenter=0 \
        --width=$xwidthinpix,$ywidthinpix \
        --center=$xcenter,$ycenter \
        --output=$cropped $quiet
if ! [ -f $cropped ]; then

    # 'makenew' will generate a fully 0-valued image. Adding (with '+') any
    # number by NaN will be NaN. Therefore the output of the Arithmetic
    # command below will be a fully NaN-valued image.
    astarithmetic $xwidthinpix $ywidthinpix 2 makenew float32 \
                  nan + $quiet --output=$output

    # Write an empty 'NORMVAL' keyword (as is expected from this program).
    astfits $output --write=NORMVAL,nan,"Normalization value"

    # Let the user know what happened.
    if [ x"$quiet" = x ]; then all_nan_warning; fi

    # Return successfully and don't continue
    exit 0
fi





# Crop and unlabel the segmentation image
# ---------------------------------------
#
# If the user provides a segmentation image, treat it appropiately in order
# to mask all objects that are not the central one. If not, just consider
# that the cropped and masked image is the cropped (not masked) image. The
# process is as follow:
#   - Compute the central clump and object labels. This is done with the
#     option '--oneelemstdout' of Crop.
#   - Crop the original mask image.
#   - Crop the original mask image to a central region (core), in order to
#     compute what is the central object id. This is necessary to unmask
#     this object.
#   - Compute what is the central object value, using the median value.
#   - In the original cropped mask, convert all pixels belonging to the
#     central object to zeros. By doing this, the central object becomes as
#     sky.
#   - Mask all non zero pixels in the mask image as nan values.
if [ x"$segment" != x ]; then

    # Find the object and clump labels of the target.
    clab=$(astcrop $segment --hdu=CLUMPS --mode=img --width=1,1 \
                   --oneelemstdout --center=$xcenter,$ycenter \
                   --quiet)
    olab=$(astcrop $segment --hdu=OBJECTS --mode=img --width=1,1 \
                   --oneelemstdout --center=$xcenter,$ycenter \
                   --quiet)

    # If for any reason, a clump or object label couldn't be initialized at
    # the given coordiante, simply ignore this step. But print a warning so
    # the user is informed of the situation (and that this is a bug: 'clab'
    # should be initialized!).
    if [ x"$clab" = x   -o   x"$olab" = x ]; then
        cat <<EOF
$scriptname: WARNING: no clump or object label could be found in '$segment' for the coordinate $center
EOF
        cropped_masked=$cropped

    else
        # To help in debugging (when '--quiet' is not called)
        if [ x"$quiet" = x ]; then
            echo "$scriptname: $segment: at $center, found clump $clab in object $olab"
        fi

        # Crop the object and clump image to same size as desired stamp.
        cropclp=$tmpdir/cropped-clumps-$objectid.fits
        cropobj=$tmpdir/cropped-objects-$objectid.fits
        astcrop $segment --hdu=OBJECTS --mode=img \
                --center=$xcenter,$ycenter \
                --width=$xwidthinpix,$ywidthinpix --output=$cropobj $quiet
        astcrop $segment --hdu=CLUMPS --mode=img \
                --center=$xcenter,$ycenter \
                --width=$xwidthinpix,$ywidthinpix --output=$cropclp $quiet

        # Mask all the undesired regions.
        cropped_masked=$tmpdir/cropped-masked-$objectid.fits
        astarithmetic $cropped --hdu=1 set-i --output=$cropped_masked $quiet \
                      $cropobj --hdu=1 set-o \
                      $cropclp --hdu=1 set-c \
                      \
                      c o $olab ne 0 where c $clab eq -1 where 0 gt \
                      1 dilate set-cmask \
                      o o $olab eq 0 where set-omask \
                      i omask cmask or nan where
    fi

    # Apply the signal-to-noise threshold if the user provided one.
    if [ x"$snthresh" != x ]; then
	cropsnt=$tmpdir/cropped-snt-$objectid.fits
	cropstd=$tmpdir/cropped-std-$objectid.fits
	astcrop $segment --hdu=SKY_STD --mode=img \
                --center=$xcenter,$ycenter \
                --width=$xwidthinpix,$ywidthinpix \
		--output=$cropstd $quiet
	astarithmetic $cropped_masked -h1 set-v \
		      $cropstd -h1        set-s \
		      v s /               set-sn \
		      v sn $snthresh lt \
		      2 dilate nan where -o$cropsnt
	mv $cropsnt $cropped_masked
    fi

else
    cropped_masked=$cropped
fi





# Function to simplify checks in each dimension
# ---------------------------------------------
#
# If the bottom-left edge of the crop is outside the image, the minimum
# coordinate needs to be modified from the value in the 'ICF1PIX' keyword
# of the output of the Crop program.
first_pix_in_img () {

    # This function takes a single argument: the dimension to work on.
    dim=$1

    # Find the minimim and maximum from the cropped image.
    if [ $dim = x ]; then
        width=$xwidthinpix
        min=$(echo "$overlaprange" | awk '{print $1}')
        max=$(echo "$overlaprange" | awk '{print $2}')
    elif [ $dim = y ]; then
        width=$ywidthinpix
        min=$(echo "$overlaprange" | awk '{print $3}')
        max=$(echo "$overlaprange" | awk '{print $4}')
    else
        cat <<EOF
$scriptname: ERROR: a bug! Please contact with us at bug-gnuastro@gnu.org. The value of 'dim' is "$dim" and not recognized
EOF
        exit 1
    fi

    # Check if it is necessary to correct the position. If the bottom-left
    # corner of the image (which defines the zero coordinate along both
    # dimensions) has fallen outside of the image, then the minimum value
    # of 'ICF1PIX' in that dimension will be '1'.
    #
    # Therefore, if the minimum value is not '1', then everything is fine
    # and no correction is necessary! We'll just return the minimum
    # value. Otherwise, we need to return a negative minimum pixel value to
    # obtain the correct position of the object from 'ICF1PIX' (which only
    # has the region of the input that overlapped with the output). It is
    # necessary to add '1' because the first pixel has a coordiante of 1,
    # not 0.
    if [ $min = 1 ]; then
        echo "$max $width" | awk '{if($1==$2) print 1; else print $1-$2+1}'
    else
        echo $min
    fi
}





# In the FITS standard, the integer coordinates are defined on the center
# of each pixel. On the other hand, the centers of objects can be anywhere
# (not exactly in the center of the pixel!). In this scenario, we should
# move the center of the object to the center of the pixel with a sub-pixel
# warp. In this part of the script we will calculate the necessary
# sub-pixel translation and use Warp to center the object on the pixel
# grid. The user can disable this with the '--nocentering' option.
if [ $nocentering = 0 ]; then

    # Read the overlap range from the 'ICF1PIX' keyword (which is printed
    # in all outputs of Crop).
    overlaprange=$(astfits $cropped -h1 --keyvalue=ICF1PIX -q \
                       | sed -e's|:| |g' -e's|,| |')

    # Calculate the position of the bottom-left pixel of the cropped image
    # in relation to the input image.
    minx=$(first_pix_in_img x)
    miny=$(first_pix_in_img y)

    # Due to cropping, the image coordinate should be shifted by a certain
    # number (integer) of pixels (leaving the sub-pixel center intact).
    stmcoord=$(echo "$xcenter $ycenter $minx $miny" \
                   | awk '{printf "%g %g\n", $1-$3+1, $2-$4+1}')

    # Calculate the displacement (or value to give to '--translate' in
    # Warp), and the new center of the star after translation (or value of
    # '--center' in Crop):
    #  1. At first we extract the sub-pixel displacement ('x' and 'y').
    #  2. If the displacement is larger than 0.5, then after the warp, the
    #     center will be shifted by one pixel.  Otherwise, if the
    #     displacement is smaller than 0.5, we shift the grid backwards by
    #     mutiplying it by -1 and don't add any shift.
    #  3. Add 'xshift' and 'yshit' valut to integer of x and y coordinate
    #     and add final value to one.
    warpcoord=$(echo "$stmcoord" \
          | awk '{x=$1-int($1); y=$2-int($2); \
                  if(x>0.5){x=1-x; xshift=1} else {x*=-1; xshift=0}; \
                  if(y>0.5){y=1-y; yshift=1} else {y*=-1; yshift=0}; \
                  printf("%f,%f %d,%d\n", x, y, \
                  int($1)+xshift+1, int($2)+yshift+1)}')

    # Warp image based on the measured displacement (first component of the
    # output above).
    warpped=$tmpdir/cropped-masked-warpforcenter-$objectid.fits
    DXY=$(echo "$warpcoord" | awk '{print $1}')
    astwarp $cropped_masked --translate=$DXY --output=$warpped

    # Crop image based on the calculated shift (second component of the
    # output above).
    centermsk=$tmpdir/cropped-masked-centered-$objectid.fits
    CXY=$(echo "$warpcoord" | awk '{print $2}')
    astcrop $warpped -h1 \
            --mode=img --output=$centermsk \
            --center=$CXY --width=$xwidthinpix,$ywidthinpix
else
    # If the user did not want to correct the center of image, we'll use
    # the raw crop as input for the next step.
    centermsk=$cropped_masked
fi





# Compute the radial profile and the normalization value
# ------------------------------------------------------
#
# Only if the the user has specified a ring of normalization (--normradii).
# Otherwise set the normalization value equal to 1.0 (no normalization).
if [ x"$normradiusmin" != x   -a   x"$normradiusmax" != x ]; then

    # Generate the radial profile of the stamp, since it has been already
    # centered on the center of the object, it is not necessary to give the
    # center coordinates. If the user specifies a maximum radius, use it.
    # Otherwise, compute the radial profile up to the outer part of the
    # ring for the normalization (to not wast CPU time). If the user
    # specifies sigma clip parameters, use them.
    radialprofile=$tmpdir/rprofile-$objectid.fits
    maxr=$(echo "$normradiusmax" | awk '{print $1+1}')
    if [ x"$sigmaclip" = x ]; then finalsigmaclip=""
    else                           finalsigmaclip="--sigmaclip=$sigmaclip";
    fi
    astscript-radial-profile $centermsk --hdu=1 --rmax=$maxr \
                             --measure=$normop $finalsigmaclip \
                             --position-angle=$positionangle \
                             --tmpdir=$tmpdir --keeptmp \
                             --axis-ratio=$axisratio \
                             --output=$radialprofile $quiet

    # The normalization value is computed from the radial profile in between
    # the two radius specified by the user. In this case, the option to give
    # sigmaclip parameters to 'aststatistics' is different.
    if [ x"$sigmaclip" = x ]; then finalsigmaclip=""
    else                           finalsigmaclip="--sclipparams=$sigmaclip";
    fi

    # Select the values within the requested radial range.
    values=$(asttable $radialprofile $quiet \
                      --range=1,$normradiusmin,$normradiusmax)
    if ! normvalue=$(echo "$values" | aststatistics --column=2 --$normop \
                                                    $finalsigmaclip -q 2> /dev/null); then
        normvalue=nan
    fi
else
    normvalue=1.0
fi





# Normalize the stamp
# -------------------
#
# Before applying the normalization factor to the masked stamps, we should
# make sure the the normalization value has a 'float32' type. This is
# because Arithmetic will interpret its type based on the number of digits
# it has. Therefore something like 1.7204 will be read as float32 (1.7204),
# but 1.868915 will be read as 'float64'. In the latter case, the output
# will become 'float64' also, which will cause problems later when we want
# to stack them together (some images will be 'float32', some 'float64').
astarithmetic $centermsk --hdu=1 $normvalue float32 / \
              float32 --output=$output $quiet





# Keeping meta-data
# -----------------
#
# Some meta-data are useful for later analysis and because of that, they
# are kept into the header of the output file. The data kept are: object
# and clump that were not masked and the normalization value. If not masks
# were applied, then set the clump and object labels to 'none'.
if [ x"$clab" = x ]; then
    astfits $output --write=NORMVAL,$normvalue,"Normalization value"
else
    astfits $output --write=NORMVAL,$normvalue,"Normalization value" \
            --write=CLABEL,$clab,"CLUMP label that was not masked" \
            --write=OLABEL,$olab,"OBJECT label that was not masked"
fi





# Print warning. We are printing this warning here (at the end of the
# script) because we dont want it to be mixed with the outputs of the
# previus commands.
if [ x"$quiet" = x ] && [ $normvalue = nan ]; then all_nan_warning; fi




# Remove temporary files
# ----------------------
#
# If the user does not specify to keep the temporal files with the option
# `--keeptmp', then remove the whole directory.
if [ $keeptmp = 0 ]; then
    rm -r $tmpdir
fi
