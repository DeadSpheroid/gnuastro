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
export LC_NUMERIC=C





# Default option values (can be changed with options on the command-line).
hdu=1
psf=""
mode=""
quiet=""
psfhdu=1
center=""
keeptmp=0
tmpdir=""
segment=""
xshifts=""
yshifts=""
normradii=""
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
  -n, --normradii=INT,INT   Minimum and maximum radii (in pixels)
                            for computing the scaling factor value.
  -S, --segment=STR         Output of Segment (OBJECTS and CLUMPS).
  -s, --sigmaclip=FLT,FLT   Sigma-clip multiple and tolerance.
  -x, --xshifts=FLT,FLT,FLT To re-center: min,step,max (in pixels).
  -y, --yshifts=FLT,FLT,FLT To re-center: min,step,max (in pixels).

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

        -x|--xshifts)        xshifts="$2";                               check_v "$1" "$xshifts";  shift;shift;;
        -x=*|--xshifts=*)    xshifts="${1#*=}";                          check_v "$1" "$xshifts";  shift;;
        -x*)                 xshifts=$(echo "$1"  | sed -e's/-x//');     check_v "$1" "$xshifts";  shift;;
        -y|--yshifts)        yshifts="$2";                               check_v "$1" "$yshifts";  shift;shift;;
        -y=*|--yshifts=*)    yshifts="${1#*=}";                          check_v "$1" "$yshifts";  shift;;
        -y*)                 yshifts=$(echo "$1"  | sed -e's/-y//');     check_v "$1" "$yshifts";  shift;;

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

# If x-shifts is provided.
if [ x"$xshifts" != x ]; then
    nxshifts=$(echo $xshifts | awk 'BEGIN{FS=","} \
                                   {for(i=1;i<=NF;++i) c+=$i!=""} \
                                   END{print c}')
    if [ x$nxshifts != x3 ]; then
        cat <<EOF
$scriptname: '--xshifts' (or '-x') takes three values (xmin,xstep,xmax), but $nxshifts were given in '$xshifts'
EOF
        exit 1
    fi
    xstep=""
    xstep=$(echo $xshifts | awk 'BEGIN{FS=","} {if ($2 <= 0.0) print "xstep <= 0.0"}')
    if [ x"$xstep" != x ]; then
        cat <<EOF
$scriptname: the 'xstep' from the option --xshifts=xmin,xstep,xmax (or '-x') is equal or lower than zero ($xshifts), but it has to be a value above zero
EOF
        exit 1
    fi
else
# If x-shifts is not provided, set it to zero with a step different of zero
# to prevent seq chrashing
xshifts="-0.0,0.1,+0.0"
fi

# If y-shifts is provided.
if [ x"$yshifts" != x ]; then
    nyshifts=$(echo $yshifts | awk 'BEGIN{FS=","} \
                                   {for(i=1;i<=NF;++i) c+=$i!=""} \
                                   END{print c}')
    if [ x$nyshifts != x3 ]; then
        cat <<EOF
$scriptname: '--yshifts' (or '-x') takes three values (xmin,xstep,xmax), but $nyshifts were given in '$yshifts'
EOF
        exit 1
    fi
    xstep=""
    xstep=$(echo $yshifts | awk 'BEGIN{FS=","} {if ($2 <= 0.0) print "ystep <= 0.0"}')
    if [ x"$xstep" != x ]; then
        cat <<EOF
$scriptname: the 'ystep' from the option --yshifts=xmin,ystep,xmax (or '-y') is equal or lower than zero ($yshifts), but it has to be a value above zero
EOF
        exit 1
    fi
else
# If y-shifts is not provided, set it to zero with a step different of zero
# to prevent seq chrashing
yshifts="-0.0,0.1,+0.0"
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





# Compute the width of the stamp
# ------------------------------
#
# If the width of the stamp is specified by the user, use that size.
# Otherwise, make the stamp width of the same size than two times the
# maximum radius for computing the flux factor. This is the maximum radius
# that is needed for computing the scaling flux factor.
xwidth=$(astarithmetic $normradiusmax float32 2.0 x 1.0 + --quiet)
ywidth=$(astarithmetic $normradiusmax float32 2.0 x 1.0 + --quiet)





# Warp the PSF image
# ------------------
#
# In principle, the PSF does not have WCS information. Here, fake WCS info
# is included in order to use warp and center/re-size the PSF to the same
# size than the warped image. Fake WCS is constructed as follow: reference
# pixel is the center pixel. This pixel is assigned to be RA, Dec = 180.0,
# 0.0 deg. Pixel scale is set to 1.0 arcsec/pixel in both axis.
crval1=180.0
crval2=0.0
cdelt1=1.0
cdelt2=1.0
psfnaxis1=$(astfits $psf --hdu=$psfhdu --keyval NAXIS1 --quiet)
psfnaxis2=$(astfits $psf --hdu=$psfhdu --keyval NAXIS2 --quiet)
psfcenter1=$(echo $psfnaxis1 | awk '{print $1/2+0.5}')
psfcenter2=$(echo $psfnaxis2 | awk '{print $1/2+0.5}')

# Create the fake WCS image. No data, just WCS.
psf_fake_wcs=$tmpdir/psf-fake-wcs.fits
echo "1 $psfcenter1 $psfcenter2 1 1 1 1 1 1 1" \
    | astmkprof --oversample=1 \
                --type=uint8 \
                --output=$psf_fake_wcs \
                --cunit="arcsec,arcsec" \
                --cdelt=$cdelt1,$cdelt2 \
                --crval=$crval1,$crval2 \
                --crpix=$psfcenter1,$psfcenter2 \
                --mergedsize=$psfnaxis1,$psfnaxis2

# Inject the WCS header into the image.
psf_wcs=$tmpdir/psf-wcsed.fits
astarithmetic $psf --hdu=$psfhdu \
              --wcsfile=$psf_fake_wcs --output=$psf_wcs

# Transform the center in img coordinates to center in wcs
psfxywcs=$(echo "$psfcenter1,$psfcenter2" \
                | asttable  --column='arith $1 $2 img-to-wcs' \
                            --wcsfile=$psf_wcs --wcshdu=1 $quiet \
                            | awk '{printf "%.10e,%.10e\n", $1, $2}')

# Warp the PSF image around its center with the same size than the image
psf_warped=$tmpdir/psf-warped.fits
astwarp $psf_wcs \
        --hdu=1 \
        --widthinpix \
        --center=$psfxywcs \
        --width=$xwidth,$ywidth \
        --output=$psf_warped $quiet \





# Build a radial profile image
# ----------------------------
#
# It will be used to only select pixels within the requested radial range.
xradcenter=$(astfits $psf_warped --keyval NAXIS1 --quiet \
                     | awk '{print $1/2+0.5}')
yradcenter=$(astfits $psf_warped --keyval NAXIS2 --quiet \
                     | awk '{print $1/2+0.5}')
maxradius=$(printf "$xwidth\n$ywidth" \
                   | aststatistics --maximum --quiet)
rad_warped=$tmpdir/warped-radial-$objectid.fits
echo "1 $xradcenter $yradcenter 7 $maxradius 0 0 1 1 1" \
     | astmkprof --background=$psf_warped --clearcanvas \
                 --oversample=1 --output=$rad_warped $quiet





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

    # Center coordinates in WCS and IMG
    xcwcs=$xcoord
    ycwcs=$ycoord
    xcimg=$(echo "$xycenter" | awk '{print $1}')
    ycimg=$(echo "$xycenter" | awk '{print $2}')
    inputs_wcs=$inputs

else
    # Fake WCS for the image is taken from the already constructed fake WCS
    # of the PSF image. The only difference is that the crpix correspond to
    # the center of the image (instead of the PSF image).
    naxis1=$(astfits $inputs --hdu=$hdu --keyval NAXIS1 --quiet)
    naxis2=$(astfits $inputs --hdu=$hdu --keyval NAXIS2 --quiet)
    center1=$(echo $naxis1 | awk '{print $1/2}')
    center2=$(echo $naxis2 | awk '{print $1/2}')

    fake_wcs=$tmpdir/fake-wcs-$objectid.fits
    echo "1 $center1 $center2 1 1 1 1 1 1 1" \
        | astmkprof --oversample=1 \
                    --type=uint8 \
                    --output=$fake_wcs \
                    --cdelt=$cdelt1,$cdelt2 \
                    --crval=$crval1,$crval2 \
                    --crpix=$center1,$center2 \
                    --cunit="arcsec,arcsec" \
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





# Shifts grid
# -----------
#
# Here, a grid of center positions are defined. The image will be warped
# using each center, at the end, the one that has the smaller standard
# deviation within the ring of computing the flux scaling factor will be
# provided as the best option. The shifts are specified in pixels, so, they
# need to be transformed to WCS shifts by using the pixel scale in deg/pix.
pscales=$(astfits $inputs_wcs --hdu $hdu --pixelscale --quiet)
xpscale=$(echo $pscales | awk '{print $1}')
ypscale=$(echo $pscales | awk '{print $2}')
xshiftswcs=$(echo $xshifts \
                  | awk -v p="$xpscale" -F ","\
                    '{printf "%.10f %.10f %.10f\n", p*$1,p*$2,p*$3}')
yshiftswcs=$(echo $yshifts \
                  | awk -v p="$ypscale" -F ","\
                    '{printf "%.10f %.10f %.10f\n", p*$1,p*$2,p*$3}')

for xs in $(seq $xshiftswcs); do
    for ys in $(seq $yshiftswcs); do
        # Compute the shifts in WCS
	xspix=$(astarithmetic $xs $xpscale / --quiet)
	yspix=$(astarithmetic $ys $ypscale / --quiet)

        # New shifted center in wcs
        xcwcs_shift=$(astarithmetic $xcwcs $xs + --quiet)
        ycwcs_shift=$(astarithmetic $ycwcs $ys + --quiet)

	# To ease the filename reading, put the offsets in pixels
        label_shift=xs_"$xspix"_ys_"$yspix"




        # Warp (WCS) the original image around the object
        # -----------------------------------------------
        #
        # Warp the object around its center with the given stamp size width.
        warped=$tmpdir/warped-$objectid-$label_shift.fits
        astwarp $inputs_wcs \
                --hdu=$hdu\
                --widthinpix \
                --width=$xwidth,$ywidth \
                --output=$warped $quiet \
                --center=$xcwcs_shift,$ycwcs_shift


        # If the warped image is not generated, it may happen that it does not
        # overlap with the input image. Save a nan value as the output and
        # continue.
        if ! [ -f $warped ]; then

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
	#   - In the original cropped mask, convert pixels belonging to the central
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

        	# Warp the object and clump images to same size as desired stamp.
        	# If mode is in img (no wcs info), it is necessary to inject the
        	# fake WCS information in advance.
                clumps_wcs=$tmpdir/clumps-wcs-$objectid-$label_shift.fits
                objects_wcs=$tmpdir/objects-wcs-$objectid-$label_shift.fits
                if [ "$mode" = wcs ]; then
                    astfits $segment --copy CLUMPS --output $clumps_wcs
                    astfits $segment --copy OBJECTS --output $objects_wcs
                else
        	    astarithmetic $segment --hdu CLUMPS \
                                  --wcsfile=$fake_wcs --output=$clumps_wcs
        	    astarithmetic $segment --hdu OBJECTS \
                                  --wcsfile=$fake_wcs --output=$objects_wcs
                fi

                warped_clp=$tmpdir/warped-clumps-$objectid-$label_shift.fits
                warped_obj=$tmpdir/warped-objects-$objectid-$label_shift.fits
                astwarp $clumps_wcs \
                        --widthinpix \
                        --width=$xwidth,$ywidth \
                        --output=$warped_clp $quiet \
                        --center=$xcwcs_shift,$ycwcs_shift
                astwarp $objects_wcs \
                        --widthinpix \
                        --width=$xwidth,$ywidth \
                        --output=$warped_obj $quiet \
                        --center=$xcwcs_shift,$ycwcs_shift

                # Mask all the undesired regions.
                warped_masked=$tmpdir/warped-masked-$objectid-$label_shift.fits
                astarithmetic $warped     --hdu=1 set-i  \
                              $warped_obj --hdu=1 set-o \
                              $warped_clp --hdu=1 set-c \
                              \
                              c o $olab ne 0 where c $clab eq -1 where 0 gt set-cmask \
                              o o $olab eq 0 where set-omask \
                              i omask cmask or nan where \
                              --output=$warped_masked $quiet
            fi
        else
            warped_masked=$warped
        fi

#################################################################
        vback=0.0
        back=1
        if [ x$back = x1 ]; then
        # Generate the profiles
        rprofile_psf=$tmpdir/rprofile-psf-$objectid-$label_shift.fits
        rprofile_img=$tmpdir/rprofile-img-$objectid-$label_shift.fits
        astscript-radial-profile $psf_warped --output $rprofile_psf
        astscript-radial-profile $warped_masked --output $rprofile_img

        for i in $(seq $(asttable $rprofile_img --tail 1 -cRADIUS)); do
            for j in $(seq $(asttable $rprofile_psf --tail 1 -cRADIUS)); do

            psf_i=$(asttable $rprofile_psf --equal=RADIUS,$i -c2)
            psf_j=$(asttable $rprofile_psf --equal=RADIUS,$j -c2)
            star_i=$(asttable $rprofile_img --equal=RADIUS,$i -c2)
            star_j=$(asttable $rprofile_img --equal=RADIUS,$j -c2)

            f_ij=$(astarithmetic $star_i $star_j - $psf_i $psf_j - / --quiet)
            c_ij=$(astarithmetic $star_i $psf_i $f_ij x - --quiet)

            stats=$tmpdir/rprofile-stats-$objectid-$label_shift-R_$i-$j.txt
            echo $i $j $star_i $star_j $psf_i $psf_j $f_ij $c_ij > $stats

            done
        done
        fi

        statsfits=$tmpdir/rprofile-stats.fits
        cat $tmpdir/rprofile-stats-$objectid-$label_shift-R_*.txt \
            | asttable --output $statsfits
        vback=$(aststatistics $statsfits -c8 --sigclip-mean --quiet)
        vbackstd=$(aststatistics $statsfits -c8 --sigclip-std --quiet)

	# Mask all but the wanted pixels of the ring. Subtract the
	# background value computed above (or vback=0.0 if not estimated)
        multipimg=$tmpdir/for-factor-$objectid-$label_shift.fits
        astarithmetic $warped_masked -h1 $vback - set-i \
                      $psf_warped    -h1          set-p \
                      $rad_warped    -h1          set-r \
                      r $normradiusmin lt r $normradiusmax ge or set-m \
                      i p / m nan where --output $multipimg $quiet

        # Find the multiplication factor, and the STD
        stats=$(aststatistics $multipimg --quiet \
                              --sclipparams=$sigmaclip \
                              --sigclip-median --sigclip-std)

#################################################################

	# Set the used the center position for the output. If the original
	# mode requested was IMG, the current center postion (that is in
	# WCS) needs to be transformed to IMG.
        xc_out=$xcwcs_shift
        yc_out=$ycwcs_shift
        values=$tmpdir/stats-$objectid-$label_shift.txt
	if [ x"$mode" = x"img" ]; then
            xcyc_shift=$(echo "$xcwcs_shift,$ycwcs_shift" \
                           | asttable  --column='arith $1 $2 wcs-to-img' \
                                       --wcsfile=$inputs_wcs --wcshdu=1 $quiet)
            xc_out=$(echo "$xcyc_shift" | awk '{print $1}')
            yc_out=$(echo "$xcyc_shift" | awk '{print $2}')
        fi

        # Save the data: center position, stats, and shifts.
        echo "#xcenter ycenter fscale fscalestd backval backstd   xshift yshift xshiftpix yshiftpix"   > $values
        echo "$xc_out  $yc_out $stats           $vback  $vbackstd $xs    $ys    $xspix    $yspix    " >> $values

    done
done





# Collect the statistics of all shifted images
stats_all=$tmpdir/stats-all-$objectid.fits
cat $tmpdir/stats-$objectid-*.txt \
    | asttable --sort=4 --output $stats_all

# Get the shift with the minimum STD value (4th column)
minstd=$(aststatistics $stats_all -c4 --minimum --quiet)

# Keep the new coordiantes and the multiplicative flux factor
outvalues=$(asttable $stats_all --equal=4,$minstd --quiet)





# Print the multiplication factor in standard output or in a given file
if [ x"$output" = x ]; then echo $outvalues
else                        echo $outvalues > $output
fi





# Remove temporary files
# ----------------------
#
# If the user does not specify to keep the temporal files with the option
# `--keeptmp', then remove the whole directory.
if [ $keeptmp = 0 ]; then
    rm -r $tmpdir
fi
