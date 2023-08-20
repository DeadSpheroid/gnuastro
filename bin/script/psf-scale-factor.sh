#!/bin/sh

# Obtain the scale factor by which it is necessary to multiply a given PSF
# image in order to match it to a particular star within a given ring of
# radii.  Run with `--help', or see description under `print_help' (below)
# for more.
#
# Original author:
#   Raul Infante-Sainz <infantesainz@gmail.com>
# Contributing author(s):
#   Mohammad Akhlaghi <mohammad@akhlaghi.org>
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
psf=""
quiet=""
psfhdu=1
center=""
keeptmp=0
tmpdir=""
segment=""
normradii=""
widthinpix=""
nocentering=0
sigmaclip="3,0.1"
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

This script will consider a center position and then compute the factor by
which it is necessary to multiply the specified PSF image in order to match
it with the object at the center (within a ring radii). The main idea is to
have the necessary factor that will be used to allocate the PSF with the
proper flux value for constructing the scattered light field.

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
  -p, --psf=STR           PSF image fits file.
  -P, --psfhdu=STR        HDU/extension of the PSF image.
  -O, --mode=STR          Coordinates mode ('wcs' or 'img').
  -c, --center=FLT,FLT    Center coordinates of the object.
  -W, --widthinpix=INT,INT Width of the stamp in pixels.
  -n, --normradii=INT,INT Minimum and maximum radii (in pixels)
                          for computing the scaling factor value.
  -S, --segment=STR       Output of Segment (with OBJECTS and CLUMPS).
  -s, --sigmaclip=FLT,FLT Sigma-clip multiple and tolerance.

 Output:
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
$scriptname: WARNING: the cropped image could not be generated. Probably because it is out of the input image coverage. Therefore the output stamp will be all NaN. If you later use this stamp (among others) with any stack operator of 'astarithmetic' this image will effectively be ignored, so you can safely continue with stacking all the files
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
        -p|--psf)            psf="$2";                                  check_v "$1" "$psf";  shift;shift;;
        -p=*|--psf=*)        psf="${1#*=}";                             check_v "$1" "$psf";  shift;;
        -p*)                 psf=$(echo "$1"  | sed -e's/-p//');        check_v "$1" "$psf";  shift;;
        -P|--psfhdu)         psfhdu="$2";                               check_v "$1" "$psfhdu";  shift;shift;;
        -P=*|--psfhdu=*)     psfhdu="${1#*=}";                          check_v "$1" "$psfhdu";  shift;;
        -P*)                 psfhdu=$(echo "$1"  | sed -e's/-P//');     check_v "$1" "$psfhdu";  shift;;
        -n|--normradii)      normradii="$2";                            check_v "$1" "$normradii";  shift;shift;;
        -n=*|--normradii=*)  normradii="${1#*=}";                       check_v "$1" "$normradii";  shift;;
        -n*)                 normradii=$(echo "$1"  | sed -e's/-n//');  check_v "$1" "$normradii";  shift;;
        -W|--widthinpix)     widthinpix="$2";                           check_v "$1" "$widthinpix";  shift;shift;;
        -W=*|--widthinpix=*) widthinpix="${1#*=}";                      check_v "$1" "$widthinpix";  shift;;
        -W*)                 widthinpix=$(echo "$1"  | sed -e's/-W//'); check_v "$1" "$widthinpix";  shift;;
        -S|--segment)        segment="$2";                              check_v "$1" "$segment";  shift;shift;;
        -S=*|--segment=*)    segment="${1#*=}";                         check_v "$1" "$segment";  shift;;
        -S*)                 segment=$(echo "$1"  | sed -e's/-S//');    check_v "$1" "$segment";  shift;;
        -O|--mode)           mode="$2";                                 check_v "$1" "$mode";  shift;shift;;
        -O=*|--mode=*)       mode="${1#*=}";                            check_v "$1" "$mode";  shift;;
        -O*)                 mode=$(echo "$1"  | sed -e's/-O//');       check_v "$1" "$mode";  shift;;
        -c|--center)         center="$2";                               check_v "$1" "$center";  shift;shift;;
        -c=*|--center=*)     center="${1#*=}";                          check_v "$1" "$center";  shift;;
        -c*)                 center=$(echo "$1"  | sed -e's/-c//');     check_v "$1" "$center";  shift;;
        -s|--sigmaclip)      sigmaclip="$2";                            check_v "$1" "$sigmaclip";  shift;shift;;
        -s=*|--sigmaclip=*)  sigmaclip="${1#*=}";                       check_v "$1" "$sigmaclip";  shift;;
        -s*)                 sigmaclip=$(echo "$1"  | sed -e's/-s//');  check_v "$1" "$sigmaclip";  shift;;

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
$scriptname: no input FITS image files. Run with '--help' for more information on how to run
EOF
    exit 1
elif [ ! -f "$inputs" ]; then
    echo "$scriptname: $inputs: No such file or directory"
    exit 1
fi

# If a PSF image (--psf) is not given.
if [ x"$psf" = x ]; then
    cat <<EOF
$scriptname: no PSF image or profile provided. A PSF image (profile) has to be specified with '--psf' (or '-p')
EOF
    exit 1
fi

# If center coordinates (--center) is not given at all.
if [ x"$center" = x ]; then
    cat <<EOF
$scriptname: no center coordinates provided. You can specify the object's central coordinate with '--center' ('-c')
EOF
    exit 1
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
fi

# If a normalization range is not given at all.
if [ x"$normradii" = x ]; then
    cat <<EOF
$scriptname: no rign of normalization provided. You can use '--normradii' ('-n') to give the radial interval to define normalization
EOF
    exit 1
else
    nnormradii=$(echo $normradii | awk 'BEGIN{FS=","}END{print NF}')
    if [ x$nnormradii != x2 ]; then
        cat <<EOF
$scriptname: '--normradii' (or '-n') only takes two values, but $nnormradii were given
EOF
        exit 1
    fi
fi

# If mode (--mode) is not given at all.
if [ x"$mode" = x ]; then
    cat <<EOF
$scriptname: no coordinate mode provided. You can use '--mode' (or '-O'), acceptable values are 'img' (for pixel coordinate) or 'wcs' (for celestial coordinates)
EOF
    exit 1

# Make sure the value to '--mode' is either 'wcs' or 'img'. Note: '-o'
# means "or" and is preferred to '[ ] || [ ]' because only a single
# invocation of 'test' is done. Run 'man test' for more.
elif [ "$mode" = wcs     -o     "$mode" = img ]; then
    junk=1
else
    cat <<EOF
$scriptname: wrong value to --mode (-O) provided. Acceptable values are 'img' (for pixel coordinate) or 'wcs' (for celestial coordinates)
EOF
    exit 1
fi





# Basic parameters: coordinates and normalization radii
# -----------------------------------------------------
#
# Obtain the coordinates of the center as well as the normalization radii
# ring from the command line arguments.
xcoord=$(echo "$center" | awk 'BEGIN{FS=","} {print $1}')
ycoord=$(echo "$center" | awk 'BEGIN{FS=","} {print $2}')

normradiusmin=$(echo "$normradii" | awk 'BEGIN{FS=","} {print $1}')
normradiusmax=$(echo "$normradii" | awk 'BEGIN{FS=","} {print $2}')

# With the center coordinates, generate a specific label for the object
# consisting in its coordinates.
objectid="$xcoord"_"$ycoord"





# Define a temporary directory and the final output file
# ------------------------------------------------------
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
# mode will be generated.
bname_prefix=$(basename $inputs | sed 's/\.fits/ /' | awk '{print $1}')
if [ x"$tmpdir" = x ]; then \
    tmpdir=$(pwd)/"$bname_prefix"_psfmodelscalefactor
fi

if [ -d "$tmpdir" ]; then
    junk=1
else
    mkdir -p "$tmpdir"
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





# Compute the width of the stamp
# ------------------------------
#
# If the width of the stamp is specified by the user, use that size.
# Otherwise, make the stamp width of the same size than two times the
# maximum radius for computing the flux factor. This is the maximum radius
# that is needed for computing the flux value.
if [ x"$widthinpix" = x ]; then
    xwidthinpix=$(astarithmetic $normradiusmax float32 2.0 x 1.0 + --quiet)
    ywidthinpix=$(astarithmetic $normradiusmax float32 2.0 x 1.0 + --quiet)
else
    xwidthinpix=$(echo $widthinpix | awk '{print $1}')
    ywidthinpix=$(echo $widthinpix | awk '{print $2}')
fi





# Crop the original image around the object
# -----------------------------------------
#
# Crop the object around its center with the given stamp size width.
cropped=$tmpdir/cropped-$objectid.fits
astcrop $inputs --hdu=$hdu --mode=img \
        --center=$xcenter,$ycenter \
        --width=$xwidthinpix,$ywidthinpix\
        --output=$cropped $quiet

# If the cropped image is not generated, it may happen that it does not
# overlap with the input image. Save a nan value as the output and
# continue.
if ! [ -f $cropped ]; then

    multifactor=nan

    # Let the user know what happened.
    if [ x"$quiet" = x ]; then all_nan_warning; fi

    # Print the multiplication factor in standard output or in a given file
    if [ x"$output" = x ]; then echo $multifactor
    else                        echo $multifactor > $output
    fi

    # If the user does not specify to keep the temporal files with the option
    # `--keeptmp', then remove the whole directory.
    if [ $keeptmp = 0 ]; then
        rm -r $tmpdir
    fi

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
        if [ x"$quiet" = x ]; then
            cat <<EOF
$scriptname: WARNING: no clump or object label could be found in '$segment' for the coordinate $center
EOF
        fi
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
                --width=$xwidthinpix,$ywidthinpix \
                --center=$xcenter,$ycenter \
                --output=$cropobj $quiet
        astcrop $segment --hdu=CLUMPS --mode=img \
                --width=$xwidthinpix,$ywidthinpix \
                --center=$xcenter,$ycenter \
                --output=$cropclp $quiet

        # Mask all the undesired regions.
        cropped_masked=$tmpdir/cropped-masked-$objectid.fits
        astarithmetic $cropped --hdu=1 set-i --output=$cropped_masked \
                      $cropobj --hdu=1 set-o \
                      $cropclp --hdu=1 set-c \
                      \
                      c o $olab ne 0 where c $clab eq -1 where 0 gt set-cmask \
                      o o $olab eq 0 where set-omask \
                      i omask cmask or nan where $quiet
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
$scriptname: a bug! Please contact with us at bug-gnuastro@gnu.org. The value of 'dim' is "$dim" and not recognized
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
    overlaprange=$(astfits $cropped -h1 --keyvalue=ICF1PIX --quiet \
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
    astwarp $cropped_masked --translate=$DXY --output=$warpped $quiet

    # Crop image based on the calculated shift (second component of the
    # output above).
    centermsk=$tmpdir/cropped-masked-centered-$objectid.fits
    CXY=$(echo "$warpcoord" | awk '{print $2}')
    astcrop $warpped -h1 \
            --mode=img --output=$centermsk $quiet \
            --center=$CXY --width=$xwidthinpix,$ywidthinpix
else
    # If the user did not want to correct the center of image, we'll use
    # the raw crop as input for the next step.
    centermsk=$cropped_masked
fi





# Crop the PSF image with the same size.
psfcropped=$tmpdir/cropped-psf-$objectid.fits
psfxcenter=$(astfits $psf -h$psfhdu --keyvalue=NAXIS1 --quiet \
                     | awk '{print $1/2+1}')
psfycenter=$(astfits $psf -h$psfhdu --keyvalue=NAXIS2 --quiet \
                     | awk '{print $1/2+1}')
astcrop $psf --hdu=$psfhdu --mode=img \
        --center=$psfxcenter,$psfycenter \
        --width=$xwidthinpix,$ywidthinpix \
        --output=$psfcropped $quiet





# Build a radial profile image. It will be used to only select pixels
# within the requested radial range.
xradcenter=$(echo $xwidthinpix | awk '{print $1/2+1}')
yradcenter=$(echo $ywidthinpix | awk '{print $1/2+1}')
maxradius=$(printf "$xwidthinpix\n$ywidthinpix" \
                   | aststatistics --maximum --quiet)
radcropped=$tmpdir/cropped-radial-$objectid.fits
echo "1 $xradcenter $yradcenter 7 $maxradius 0 0 1 1 1" \
     | astmkprof --background=$psfcropped --clearcanvas \
                 --oversample=1 --output=$radcropped $quiet





# Find the multiplication factor
multipimg=$tmpdir/for-factor-$objectid.fits
astarithmetic $centermsk -h1 set-i \
              $psfcropped -h1 set-p \
              $radcropped -h1 set-r \
              r $normradiusmin lt r $normradiusmax ge or set-m \
              i p / m nan where --output $multipimg $quiet
multifactor=$(aststatistics $multipimg --sigclip-median \
                            --sclipparams=$sigmaclip --quiet)





# Print the multiplication factor in standard output or in a given file
if [ x"$output" = x ]; then echo $multifactor
else                        echo $multifactor > $output
fi





# Remove temporary files
# ----------------------
#
# If the user does not specify to keep the temporal files with the option
# `--keeptmp', then remove the whole directory.
if [ $keeptmp = 0 ]; then
    rm -r $tmpdir
fi
