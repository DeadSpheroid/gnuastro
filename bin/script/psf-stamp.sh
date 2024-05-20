#!/bin/sh

# Construct a star stamp to build a Point Spread Function (PSF). This
# script will consider a center position and then it will warp the original
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
sys_lang=$LANG
export LANG=C
sys_lcnumeric=$LC_NUMERIC
export LC_NUMERIC="en_US.UTF-8"





# Default option values (can be changed with options on the command-line).
hdu=1
mode=""
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
        -p|--position-angle)     positionangle="$2";                           check_v "$1" "$positionangle";  shift;shift;;
        -p=*|--position-angle=*) positionangle="${1#*=}";                      check_v "$1" "$positionangle";  shift;;
        -p*)                     positionangle=$(echo "$1"  | sed -e's/-p//'); check_v "$1" "$positionangle";  shift;;

        # Output parameters
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
        *) if [ x"$inputs" = x ]; then inputs="$1"; else inputs="$inputs $1"; fi; shift;;
    esac

done





# Basic sanity checks
# ===================

# If an input image is not given at all.
if [ x"$inputs" = x ]; then
    cat <<EOF
$scriptname: no input FITS image files (outer part of the PSF to unite with an inner part). Run with '--help' for more information on how to run
EOF
    exit 1
elif [ ! -f $inputs ]; then
    cat <<EOF
$scriptname: $inputs, no such file or directory
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
$scriptname: '--center' (or '-c') only takes two values, but $ncenter were given in '$center'
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
$scriptname: the size of the image along the first dimension (NAXIS1) can not be obtained
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
$scriptname: the size of the image along the second dimension (NAXIS2) can not be obtained
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
$scriptname: '--normradii' (or '-n') only take two values, but $nnormradii were given in '$normradii'
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
if [ x"$mode"  = x ]; then
    cat <<EOF
$scriptname: '--mode' (or '-O') is mandatory. It takes one of the following two values: 'img' (for pixel coordinates) or 'wcs' (for celestial coordinates)
EOF
    exit 1
fi
if [ "$mode" = wcs -o "$mode" = img ]; then
    junk=1
else
    cat <<EOF
$scriptname: value to '--mode' (or '-O') is not recognized ('$mode'). This option takes one of the following two values: 'img' (for pixel coordinates) or 'wcs' (for celestial coordinates)
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
objectid="$xcoord"-"$ycoord"
bname_prefix=$(basename "$inputs" | sed 's/\.fits/ /' | awk '{print $1}')
if [ x"$tmpdir" = x ]; then \
  tmpdir=$(pwd)/"$bname_prefix"-stamp-$objectid
fi

# Existance of the temporary directory (delete it if already exists).
if [ -d "$tmpdir" ]; then  rm -r "$tmpdir"; fi
mkdir "$tmpdir"

# Output.
if [ x"$output" = x ]; then
  output="$bname_prefix"_stamp_$objectid.fits
fi





# Compute the center in WCS for Warping
# -------------------------------------
#
# If the original coordinates have been given in WCS or celestial units
# (RA/DEC) they will be used by Warp. Otherwise, here fake WCS information
# is generated to be able to use Warp in the next steps. This is necessary
# for allocating the object with sub-pixel precission at the center of the
# output image. The IMG coordinates of the center are also computed because
# in next steps they are necessary if a segment image is provided (to
# compute the object and clumps label).
if [ "$mode" = wcs ]; then
    xycenter=$(echo "$xcoord,$ycoord" \
                   | asttable  --column='arith $1 $2 wcs-to-img' \
                               --wcsfile=$inputs --wcshdu=$hdu $quiet)

    # Center coordinates in WCS and IMG
    xcwcs=$xcoord
    ycwcs=$ycoord
    xcimg=$(echo "$xycenter" | awk '{print $1}')
    ycimg=$(echo "$xycenter" | awk '{print $2}')
    inputs_wcs=$inputs

else
    # Fake WCS information to be injected as header to the original image.
    # The center is assumed to be at RA=180,DEC=0 with a pixel scale of 1
    # arcsec in both axeses. Warp will interpret this for warping.
    crval1=180.0
    crval2=0.0
    cdelt1=1.0
    cdelt2=1.0
    naxis1=$(astfits $inputs --hdu=$hdu --keyval NAXIS1 --quiet)
    naxis2=$(astfits $inputs --hdu=$hdu --keyval NAXIS2 --quiet)
    fake_wcs=$tmpdir/fake-wcs-$objectid.fits
    echo "1 $xcoord $ycoord 1 1 1 1 1 1 1" \
        | astmkprof --type=uint8 \
                    --oversample=1 \
                    --output=$fake_wcs \
                    --cunit="arcsec,arcsec" \
                    --cdelt=$cdelt1,$cdelt2 \
                    --crval=$crval1,$crval2 \
                    --crpix=$xcoord,$ycoord \
                    --mergedsize=$naxis1,$naxis2

    # Inject the fake WCS into the original image, its HDU becomes 1.
    hdu=1
    inputs_wcs=$tmpdir/input-wcsed-$objectid.fits
    astarithmetic $inputs --hdu $hdu \
                  --wcsfile=$fake_wcs --output $inputs_wcs

    # Center coordinates in IMG and WCS
    xycenter=$(echo "$xcoord,$ycoord" \
                   | asttable  --column='arith $1 $2 img-to-wcs' \
                               --wcsfile=$inputs_wcs --wcshdu=1 $quiet)
    xcimg=$xcoord
    ycimg=$ycoord
    xcwcs=$(echo "$xycenter" | awk '{print $1}')
    ycwcs=$(echo "$xycenter" | awk '{print $2}')

fi





# Warp the original image around the object
# -----------------------------------------
#
# Warp the object around its center with the given stamp size width. It may
# happen that the given coordinate is fully outside of the image (within
# the requested stamp-width). In this case Warp won't generate any output,
# so we are checking the existance of the '$warped' file. If not created
# we will create a fully NaN-valued image that can be ignored in stacks.
warped=$tmpdir/warped-$objectid.fits
astwarp $inputs_wcs \
        --hdu=1 \
        --widthinpix \
        --center=$xcwcs,$ycwcs \
        --output=$warped $quiet \
        --width=$xwidthinpix,$ywidthinpix
if ! [ -f $warped ]; then

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






# Warp and unlabel the segmentation image
# ---------------------------------------
#
# If the user provides a segmentation image, treat it appropiately in order
# to mask all objects that are not the central one. If not, just consider
# that the warped and masked image is the warped (not masked) image. The
# process is as follow:
#   - Compute the central clump and object labels. This is done with the
#     option '--oneelemstdout' of Crop.
#   - Warp the CLUMPS image.
#   - Warp the OBJECTS image.
#   - On the original warped image, convert pixels belonging to the central
#     object to zeros. By doing this, the central object becomes as sky.
#   - Mask all non zero pixels in the mask image as nan values.
if [ x"$segment" != x ]; then
    # Find the object and clump labels of the target.
    clab=$(astcrop $segment --hdu=CLUMPS --mode=img --width=1,1 \
                   --oneelemstdout --center=$xcimg,$ycimg --quiet)
    olab=$(astcrop $segment --hdu=OBJECTS --mode=img --width=1,1 \
                   --oneelemstdout --center=$xcimg,$ycimg --quiet)

    # If for any reason, a clump or object label couldn't be initialized at
    # the given coordiante, simply ignore this step. But print a warning so
    # the user is informed of the situation (and that this is a bug: 'clab'
    # should be initialized!).
    if [ x"$clab" = x   -o   x"$olab" = x ]; then
        if [ x"$quiet" = x ]; then
            cat <<EOF
$scriptname: WARNING: no clump or object label could be found in '$segment' for the coordinate $center
EOF
        fi
        warped_masked=$warped
    else
        # To help in debugging (when '--quiet' is not called)
        if [ x"$quiet" = x ]; then
            echo "$scriptname: $segment: at $center, found clump $clab in object $olab"
        fi

	# In order to warp the objects and clumps image, WCS
	# information is needed. If mode is wcs, just copy the HDU.
	# If mode is img (no wcs info), it is necessary to inject
	# the fake WCS information in advance.
        clumps_wcs=$tmpdir/clumps-wcs-$objectid.fits
        objects_wcs=$tmpdir/objects-wcs-$objectid.fits
        if [ "$mode" = wcs ]; then
            astfits $segment --copy CLUMPS --output $clumps_wcs
            astfits $segment --copy OBJECTS --output $objects_wcs
        else
	    astarithmetic $segment --hdu CLUMPS \
                          --wcsfile=$fake_wcs --output=$clumps_wcs
	    astarithmetic $segment --hdu OBJECTS \
                          --wcsfile=$fake_wcs --output=$objects_wcs
        fi

        # Maske the detection map of Objects and Clumps of target in image.
        cropped_masked=$tmpdir/cropped-masked.fits
        astarithmetic $segment  --hdu=INPUT-NO-SKY set-i \
                      $segment  --hdu=CLUMPS       set-c \
                      $segment  --hdu=OBJECTS      set-o \
                      \
                      c o $olab ne 0 where c $clab eq -1 where 0 gt \
                      1 dilate set-mask \
                      o o $olab eq 0 where set-omask \
                      mask omask mask or nan where -o$cropped_masked

        # Warp the mask image.
        cropped_masked_warp=$tmpdir/cropped-masked-warp.fits
        astwarp $cropped_masked \
                --gridfile=$warped $quiet \
                --output=$cropped_masked_warp

        # Mask extra objects in warp image.
        warped_masked=$tmpdir/warped-masked-$objectid.fits
        astarithmetic $warped              -h1 set-w \
                      $cropped_masked_warp -h1 set-m \
                      w m isblank nan where -o$warped_masked
    fi

    # Apply the signal-to-noise threshold if the user provided one, and
    # only keep the central object.
    if [ x"$snthresh" != x ]; then

        # Mask all the pixels that are below the threshold based on the
        # signal to noise value which is obtained form the SKY_STD. Finally
        # label the separate regions.
        warpsnt=$tmpdir/warped-snt.fits
        warpstd=$tmpdir/warped-std.fits
        warplab=$tmpdir/warped-lab.fits
        astwarp $segment --hdu=SKY_STD \
                --gridfile=$warped \
                --output=$warpstd $quiet

        # Fill the NAN pixels with maximum value of the image plus one (so
        # the fill value is not present in the image).
        fillval=$(astarithmetic $warped_masked maxvalue 1 + -q)
        fill=$tmpdir/warped-masked-fill-nan-pix.fits
        astarithmetic $warped_masked set-i i i isblank $fillval \
                      where --output=$fill

        # Apply the threshold and label the regions above the threshold.
        astarithmetic $fill -h1      set-v \
                      $warpstd -h1   set-s \
                      v s /          set-sn \
                      v sn $snthresh lt \
                      2 dilate nan where set-all \
                      all tofilefree-$warpsnt \
                      all isnotblank 2 connected-components \
                      --output=$warplab

        # Extract the label of the central coordinate.
        id=$(astcrop $warplab -h1 --mode=$mode \
                     --center=$xcimg,$ycimg \
                     --widthinpix --width=1 --oneelemstdout -q)

        # Mask all the pixels that do not belong to the main object and set
        # all the originally NaN-valued pixels to NaN again.
        msk=$tmpdir/mask.fits
        astarithmetic $warpsnt $warplab $id ne nan where set-i \
                      i i $fillval eq nan where -g1 --output=$msk

        # Set the masked image to the desired output of this step.
        mv $msk $warped_masked
    fi

# No '--segment' provided.
else
    warped_masked=$warped
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
    radialprofile=$tmpdir/rprofile.fits
    maxr=$(echo "$normradiusmax" | awk '{print $1+1}')
    if [ x"$sigmaclip" = x ]; then finalsigmaclip=""
    else                           finalsigmaclip="--sigmaclip=$sigmaclip";
    fi
    astscript-radial-profile $warped_masked --hdu=1 --rmax=$maxr \
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
    if ! normvalue=$(echo "$values" \
                         | aststatistics --column=2 --$normop \
                                         $finalsigmaclip -q \
                                         2> /dev/null); then
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
astarithmetic $warped_masked --hdu=1 $normvalue float32 / \
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





# The script has finished, reset the original language to the system's
# default language.
export LANG=$sys_lang
export LC_NUMERIC=$sys_lcnumeric
