#!/bin/sh

# Obtain averaged radial profiles, run with `--help', or see description
# under `print_help' (below) for more.
#
# Original author:
#   Raul Infante-Sainz <infantesainz@gmail.com>
# Contributing author(s):
#   Mohammad Akhlaghi <mohammad@akhlaghi.org>
#   Zahra Sharbaf <zahra.sharbaf2@gmail.com>
#   Carlos Morales-Socorro <cmorsoc@gmail.com>
#   Sepideh Eskandarlou <sepideh.eskandarlou@gmail.com>
# Copyright (C) 2020-2024 Free Software Foundation, Inc.
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
rmax=""
polar=0
quiet=""
instd=""
stdhdu=1
center=""
tmpdir=""
output=""
keeptmp=0
mode="img"
azimuth=""
measure=""
precision=0
axisratio=1
zeropoint=""
sigmaclip=""
measuretmp=""
oversample=""
undersample=""
positionangle=0
zeroisnotblank=""
version=@VERSION@
scriptname=@SCRIPT_NAME@




# Define the path work directory.
curdir=$(pwd)





# Output of `--usage' and `--help':
print_usage() {
    cat <<EOF
$scriptname: run with '--help' for list of options
EOF
}

# Short options available for use:
#   b e f g j l n r w x y
#   A B C D E F G H J K L M N S T U W X Y
print_help() {
    cat <<EOF
Usage: $scriptname [OPTION] FITS-files

This script is part of GNU Astronomy Utilities $version.

This script will consider the input image for constructing the radial
profile around a given center with elliptical apertures.

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
  -h, --hdu=STR            HDU/extension of all input FITS files.
  -O, --mode=STR           Coordinate mode: img or wcs.
  -c, --center=FLT,FLT     Coordinate of the center along 2 axes.
  -R, --rmax=FLT           Maximum radius for the radial profile (in pixels).
  -Q, --axis-ratio=FLT     Axis ratio for ellipse profiles (A/B).
  -a, --azimuth=FLT,FLT    Azimuthal range(s) (from the major axis).
  -p, --position-angle=FLT Position angle for ellipse profiles.
  -g, --polar              Polar plot (azimuth angle vs. radius).
  -s, --sigmaclip=FLT,FLT  Sigma-clip multiple and tolerance.
  -z, --zeropoint=FLT      Zeropoint magnitude of input dataset.
  -Z, --zeroisnotblank     0.0 in float or double images are not blank.

 Output:
  -o, --output             Output table with the radial profile.
  -t, --tmpdir             Directory to keep temporary files.
  -k, --keeptmp            Keep temporal/auxiliar files.
  -m, --measure=STR        Measurement operator (mean, sigclip-mean, etc.).
  -P, --precision=INT      Number of digits after decimal point for radius.
  -v, --oversample=INT     Oversample for higher resolution radial profile.
  -u, --undersample=INT    Undersample for lower resolution radial profile.
  -i, --instd=FLT/STR      Sky standard deviation per pixel, as a single
                           number or as the filename.
  -d, --stdhdu=STR         HDU/extension of the sky standard deviation image.

 Operating mode:
  -?, --help               Print this help list.
      --cite               BibTeX citation for this program.
  -q, --quiet              Don't print any extra information in stdout.
  -V, --version            Print program version.

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


print_citation() {
    empty="" # needed for the ascii art!
    cat <<EOF

Thank you for using $scriptname (GNU Astronomy Utilities) $version

Citations and acknowledgement are vital for the continued work on Gnuastro.

Please cite the following record(s) and add the acknowledgement statement below in your work to support us. Please note that different Gnuastro programs may have different corresponding papers. Hence, please check all the programs you used. Don't forget to also include the version as shown above for reproducibility.

Paper introducing this script
-----------------------------
  @ARTICLE{astscript-radial-profile,
         author = {{Infante-Sainz}, Ra{\'u}l and {Akhlaghi}, Mohammad and {Eskandarlou}, Sepideh},
          title = "{Gnuastro: Measuring Radial Profiles from Images}",
        journal = {Research Notes of the American Astronomical Society},
       keywords = {Astronomy software, Astronomical techniques, Astronomy data visualization, Broad band photometry, Open source software, 1855, 1684, 1968, 184, 1866},
           year = 2024,
          month = jan,
         volume = {8},
         number = {1},
            eid = {22},
          pages = {22},
            doi = {10.3847/2515-5172/ad1ee2},
  archivePrefix = {arXiv},
         eprint = {2401.05303},
   primaryClass = {astro-ph.IM},
         adsurl = {https://ui.adsabs.harvard.edu/abs/2024RNAAS...8...22I},
        adsnote = {Provided by the SAO/NASA Astrophysics Data System}
  }

Paper introducing the --polar option
------------------------------------
  @ARTICLE{2024RNAAS...8..168E,
         author = {{Eskandarlou}, Sepideh and {Akhlaghi}, Mohammad},
          title = "{Gnuastro: Generating Polar Plots in Astronomical Images}",
        journal = {Research Notes of the American Astronomical Society},
       keywords = {Astronomy software, Open source software, Astronomical techniques, Spiral pitch angle, Spiral arms, 1855, 1866, 1684, 1561, 1559, Astrophysics - Instrumentation and Methods for Astrophysics, Astrophysics - Astrophysics of Galaxies},
           year = 2024,
          month = jun,
         volume = {8},
         number = {6},
            eid = {168},
          pages = {168},
            doi = {10.3847/2515-5172/ad5a6c},
  archivePrefix = {arXiv},
         eprint = {2406.14619},
   primaryClass = {astro-ph.IM},
         adsurl = {https://ui.adsabs.harvard.edu/abs/2024RNAAS...8..168E},
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
        -h|--hdu)                hdu="$2";                                     check_v "$1" "$hdu";  shift;shift;;
        -h=*|--hdu=*)            hdu="${1#*=}";                                check_v "$1" "$hdu";  shift;;
        -h*)                     hdu=$(echo "$1"  | sed -e's/-h//');           check_v "$1" "$hdu";  shift;;
        -O|--mode)               mode="$2";                                    check_v "$1" "$mode";  shift;shift;;
        -O=*|--mode=*)           mode="${1#*=}";                               check_v "$1" "$mode";  shift;;
        -O*)                     mode=$(echo "$1"  | sed -e's/-O//');          check_v "$1" "$mode";  shift;;
        -c|--center)             center="$2";                                  check_v "$1" "$center";  shift;shift;;
        -c=*|--center=*)         center="${1#*=}";                             check_v "$1" "$center";  shift;;
        -c*)                     center=$(echo "$1"  | sed -e's/-c//');        check_v "$1" "$center";  shift;;
        -R|--rmax)               rmax="$2";                                    check_v "$1" "$rmax";  shift;shift;;
        -R=*|--rmax=*)           rmax="${1#*=}";                               check_v "$1" "$rmax";  shift;;
        -R*)                     rmax=$(echo "$1"  | sed -e's/-R//');          check_v "$1" "$rmax";  shift;;
        -Q|--axis-ratio)         axisratio="$2";                               check_v "$1" "$axisratio";  shift;shift;;
        -Q=*|--axis-ratio=*)     axisratio="${1#*=}";                          check_v "$1" "$axisratio";  shift;;
        -Q*)                     axisratio=$(echo "$1"  | sed -e's/-Q//');     check_v "$1" "$axisratio";  shift;;
        -a|--azimuth)            aztmp="$2";                                   check_v "$1" "$aztmp";  shift;shift;;
        -a=*|--azimuth=*)        aztmp="${1#*=}";                              check_v "$1" "$aztmp";  shift;;
        -a*)                     aztmp=$(echo "$1"  | sed -e's/-a//');         check_v "$1" "$aztmp";  shift;;
        -p|--position-angle)     positionangle="$2";                           check_v "$1" "$positionangle";  shift;shift;;
        -p=*|--position-angle=*) positionangle="${1#*=}";                      check_v "$1" "$positionangle";  shift;;
        -p*)                     positionangle=$(echo "$1"  | sed -e's/-p//'); check_v "$1" "$positionangle";  shift;;
        -s|--sigmaclip)          sigmaclip="$2";                               check_v "$1" "$sigmaclip";  shift;shift;;
        -s=*|--sigmaclip=*)      sigmaclip="${1#*=}";                          check_v "$1" "$sigmaclip";  shift;;
        -s*)                     sigmaclip=$(echo "$1"  | sed -e's/-s//');     check_v "$1" "$sigmaclip";  shift;;
        -z|--zeropoint)          zeropoint="$2";                               check_v "$1" "$zeropoint";  shift;shift;;
        -z=*|--zeropoint=*)      zeropoint="${1#*=}";                          check_v "$1" "$zeropoint";  shift;;
        -z*)                     zeropoint=$(echo "$1"  | sed -e's/-z//');     check_v "$1" "$zeropoint";  shift;;
        -g|--polar)              polar=1; shift;;
        -g*|--polar=*)           on_off_option_error --polar -g;;

        # Output parameters
        -k|--keeptmp)     keeptmp=1; shift;;
        -k*|--keeptmp=*)  on_off_option_error --keeptmp -k;;
        -t|--tmpdir)      tmpdir="$2";                          check_v "$1" "$tmpdir";  shift;shift;;
        -t=*|--tmpdir=*)  tmpdir="${1#*=}";                     check_v "$1" "$tmpdir";  shift;;
        -t*)              tmpdir=$(echo "$1" | sed -e's/-t//'); check_v "$1" "$tmpdir";  shift;;
        -m|--measure)     measuretmp="$2";                      check_v "$1" "$measuretmp";  shift;shift;;
        -m=*|--measure=*) measuretmp="${1#*=}";                 check_v "$1" "$measuretmp";  shift;;
        -m*)              measuretmp=$(echo "$1"  | sed -e's/-m//'); check_v "$1" "$measuretmp";  shift;;
        -P|--precision)   precision="$2";                      check_v "$1" "$precision";  shift;shift;;
        -P=*|--precision=*) precision="${1#*=}";               check_v "$1" "$precision";  shift;;
        -P*)              precision=$(echo "$1"  | sed -e's/-P//'); check_v "$1" "$precision";  shift;;
        -o|--output)      output="$2";                          check_v "$1" "$output"; shift;shift;;
        -o=*|--output=*)  output="${1#*=}";                     check_v "$1" "$output"; shift;;
        -o*)              output=$(echo "$1" | sed -e's/-o//'); check_v "$1" "$output"; shift;;
        -v|--oversample)  oversample="$2";                      check_v "$1" "$oversample"; shift;shift;;
        -v=*|--oversample=*) oversample="${1#*=}";              check_v "$1" "$oversample"; shift;;
        -v*)              oversample=$(echo "$1" | sed -e's/-v//'); check_v "$1" "$oversample"; shift;;
        -u|--undersample) undersample="$2";                     check_v "$1" "$undersample"; shift;shift;;
        -u=*|--undersample=*) undersample="${1#*=}";            check_v "$1" "$undersample"; shift;;
        -u*)              undersample=$(echo "$1" | sed -e's/-u//'); check_v "$1" "$undersample"; shift;;
        -i|--instd)      instd="$2";                            check_v "$1" "$instd";  shift;shift;;
        -i=*|--instd=*)  instd="${1#*=}";                       check_v "$1" "$instd";  shift;;
        -i*)             instd=$(echo "$1"  | sed -e's/-i//');  check_v "$1" "$instd";  shift;;
        -d|--stdhdu)     stdhdu="$2";                           check_v "$1" "$stdhdu";  shift;shift;;
        -d=*|--stdhdu=*) stdhdu="${1#*=}";                      check_v "$1" "$stdhdu";  shift;;
        -d*)             stdhdu=$(echo "$1"  | sed -e's/-d//'); check_v "$1" "$stdhdu";  shift;;

        # Non-operating options.
        -q|--quiet)             quiet="--quiet"; shift;;
        -q*|--quiet=*)          on_off_option_error --quiet -q;;
        -?|--help)              print_help; exit 0;;
        -'?'*|--help=*)         on_off_option_error --help -?;;
        -V|--version)           print_version; exit 0;;
        -V*|--version=*)        on_off_option_error --version -V;;
        --cite)                 print_citation; exit 0;;
        --cite=*)               on_off_option_error --cite;;
        -Z|--zeroisnotblank)    zeroisnotblank="--zeroisnotblank"; shift;;
        -Z*|--zeroisnotblank=*) on_off_option_error --zeroisnotblank -Z;;

        # Unrecognized option:
        -*) echo "$scriptname: unknown option '$1'"; exit 1;;

        # Not an option (not starting with a `-'): assumed to be input FITS
        # file name.
        *) if [ x"$inputs" = x ]; then inputs="$1"; else inputs="$inputs $1"; fi; shift;;
    esac

    # If a measurment or azimuth were given (options that can be called
    # more than once), add their value to previous values.
    if [ x"$measuretmp" != x ]; then
        if [ x"$measure" = x ]; then measure=$measuretmp;
        else                         measure="$measure,$measuretmp";
        fi
        measuretmp=""
    fi
    if [ x"$aztmp" != x ]; then

        # Make sure two values are indeed given to this option.
        naz=$(echo $aztmp \
                  | awk 'BEGIN{FS=","} \
                         {for(i=1;i<=NF;++i) c+=$i!=""} \
                         END{print c}')
        if [ x$naz != x2 ]; then
            printf "$scriptname: '--azimuth' (or '-a') only takes "
            printf "two numbers, but $naz number(s) were given "
            printf "in '$aztmp'"; exit 1
        fi

        # Add the give values to any potentially pre-existing values.
        if [ x"$azimuth" = x ]; then azimuth=$aztmp;
        else                         azimuth="$azimuth:$aztmp";
        fi
        aztmp=""
    fi
done





# Basic sanity checks
# ===================

# If an input image is not given at all.
if [ x"$inputs" = x ]; then
    echo "$scriptname: no input FITS image files."
    echo "Run with '--help' for more information on how to run."
    exit 1
elif [ ! -f $inputs ]; then
    echo "$scriptname: $inputs: No such file or directory."
    exit 1
fi

# If a '--center' has been given, make sure it only has two numbers.
if [ x"$center" != x ]; then
    ncenter=$(echo $center | awk 'BEGIN{FS=","} \
                                  {for(i=1;i<=NF;++i) c+=$i!=""} \
                                  END{print c}')
    if [ x$ncenter != x2 ]; then
        echo "$scriptname: '--center' (or '-c') only takes two values, but $ncenter were given in '$center'"
        exit 1
    fi
fi

# Make sure the value to '--mode' is either 'wcs' or 'img'. Note: '-o'
# means "or" and is preferred to '[ ] || [ ]' because only a single
# invocation of 'test' is done. Run 'man test' for more.
if [ "$mode" = wcs   -o    $mode = "img" ]; then
    junk=1
else
    echo "$scriptname: value to '--mode' ('-m') should be 'wcs' or 'img'"
    exit 1
fi

# The precision should be an integer, smaller than 6.
pcheck=$(echo $precision \
             | awk 'int($1)==$1 && $1>=0 && $1<=6 {print 1}')
if [ x$pcheck = x ]; then
    echo "$scriptname: value of '--precision' ($precision) must be an integer between 0 to 6 (inclusive)"
    exit 1
fi

# If no specific measurement has been requested, use the mean.
if [ x"$measure" = x ]; then measure=mean; fi

# Without azimuthal angle, we can not create a polar-plot.
if [ x"$azimuth" = x ] && [ x$polar = x1 ]; then
    azimuth="0,360"
fi

# If the user call the '--undersample' and '--oversample' option. On the
# other hand, they use '--polar' option with these options
if [ x$polar = x1 ] && [ x"$undersample" != x ] ; then
    echo "$scriptname: the polar plot does not yet support --undersample; please get in touch with us at 'bug-gnuastro@gnu.org' for its possible addition"; exit 1
fi

if [ x$polar = x1 ] && [ x"$oversample" != x ] ; then
    echo "$scriptname: the polar plot does not yet support --oversample; please get in touch with us at 'bug-gnuastro@gnu.org' for its possible addition"; exit 1
fi





# Finalize the center value
# -------------------------
#
# Beyond this point, we know the image-based, central coordinate for the
# radial profile as two values (one along each dimension).
if [ x"$center" = x ]; then

    # No center has been given: we thus assume that the object is already
    # centered on the input image and will set the center to the central
    # pixel in the image. In the FITS standard, pixels are counted from 1,
    # and the integers are in the center of the pixel. So after dividing
    # the pixel size of the image by 2, we should add it with 0.5 to be the
    # `center' of the image.
    xcenter=$(astfits $inputs --hdu=$hdu | awk '/^NAXIS1/{print $3/2+0.5}')
    ycenter=$(astfits $inputs --hdu=$hdu | awk '/^NAXIS2/{print $3/2+0.5}')

# A center is given.
else

    # The central position is in image mode; we should just separate each
    # coordinate.
    if [ "$mode" = img ]; then
        xcenter=$(echo "$center" | awk 'BEGIN{FS=","} {print $1}')
        ycenter=$(echo "$center" | awk 'BEGIN{FS=","} {print $2}')

    # The center is in WCS coordinates. We should thus convert them to
    # image coordinates at this point. To do that, WCS information from the
    # input header image is used.
    else
        xy=$(echo "$center" \
                  | asttable -c'arith $1 $2 wcs-to-img' \
                             --wcsfile=$inputs --wcshdu=$hdu)
        xcenter=$(echo $xy | awk '{print $1}');
        ycenter=$(echo $xy | awk '{print $2}');
    fi
fi





# Calculate the maximum radius
# ----------------------------
#
# If the user didn't set the '--rmax' parameter, then compute the maximum
# radius possible on the image.
#
# If the user has not given any maximum radius, we give the most reliable
# maximum radius (where the full circumference will be within the
# image). If the radius goes outside the image, then the measurements and
# calculations can be biased, so when the user has not provided any maximum
# radius, we should only confine ourselves to a radius where the results
# are reliable.
#
#             Y--------------
#              |            |       The maximum radius (to ensure the profile
#            y |........*   |       lies within the image) is the smallest
#              |        .   |       one of these values:
#              |        .   |              x, y, X-x, Y-y
#              --------------
#              0        x   X
#
if [ x"$rmax" = x ]; then
  rmax=$(astfits $inputs --hdu=$hdu \
             | awk '/^NAXIS1/{X=$3} /^NAXIS2/{Y=$3} \
                    END{ x='$xcenter'; y='$ycenter'; \
                         printf("%s\n%s\n%s\n%s", x, y, X-x, Y-y); }' \
             | aststatistics --minimum )
fi





# Define the final output file and temporal directory
# ---------------------------------------------------
#
# Here, it is defined the final output file containing the radial profile.
# If the user has defined a specific path/name for the output, it will be
# used for saving the output file. If the user does not specify a output
# name, then a default value containing the center and mode will be
# generated.
bname_prefix=$(basename $inputs | sed 's/\.fits/ /' | awk '{print $1}')
defaultname=$(pwd)/"$bname_prefix"_radial_profile_$mode"_$xcenter"_"$ycenter"
if [ x"$output" = x ]; then output="$defaultname.fits"; fi

# Construct the temporary directory. If the user does not specify any
# directory, then a default one with the base name of the input image will
# be constructed.  If the user set the directory, then make it. This
# directory will be deleted at the end of the script if the user does not
# want to keep it (with the `--keeptmp' option).
if [ x"$tmpdir" = x ]; then tmpdir=$defaultname; fi
if [ -d "$tmpdir" ]; then junk=1; else mkdir $tmpdir; fi





# Crop image and instd
# --------------------
#
# Crop the input image and instd image, if --sn is requested, around the
# desired point so we can continue processing only on those pixels (we do
# not need the other pixels).
#
# The crop's output always has the range of pixels from the original image
# used in the `ICF1PIX' keyword value. So, to find the new center
# (important if it is sub-pixel precission), we can simply get the first
# and third value of that string, and convert to the cropped coordinate
# system. Note that because FITS pixel couting starts from 1, we need to
# subtract `1'.
#
# For the standard deviation, besides being empty or not, we first need to
# make sure it is actually a file (note that it can also be a number). If
# its a file, we'll crop it and set 'instd' to be the new cropped file.
cropbase=crop.fits
crop=$tmpdir/$cropbase
cropstdbase=crop-std.fits
cropstd=$tmpdir/$cropstdbase
cropwidth=$(echo $rmax | awk '{print $1*2+1}')
astcrop $inputs --hdu=$hdu --center=$xcenter,$ycenter --mode=img \
        --width=$cropwidth $zeroisnotblank --output=$crop $quiet
if [ x"$instd" != x ] && [ -f "$instd" ]; then
    astcrop $instd --hdu="$stdhdu" --center=$xcenter,$ycenter \
            --mode=img --width=$cropwidth $zeroisnotblank \
            --output=$cropstd $quiet
fi





# Correct the central position (to cropped image)
# -----------------------------------------------
dxy=$(astfits $crop -h0 \
          | grep ICF1PIX \
          | sed -e"s/'/ /g" -e's/\:/ /g' -e's/,/ /' \
          | awk '{print $3-1, $5-1}')
xcenter=$(echo "$xcenter $cropwidth $dxy" \
              | awk '{ if($1>int($2/2)) print $1-$3; \
                       else             print int($2/2)+$1-int($1) }')
ycenter=$(echo "$ycenter $cropwidth $dxy" \
              | awk '{ if($1>int($2/2)) print $1-$4; \
                       else             print int($2/2)+$1-int($1) }')





# Over-sample the input if necessary
# ----------------------------------
#
# With the goal of having higher spatial resolution, here the input image
# is over-sampled. This is only done if the user request this with the
# option --oversample.
valuesbase=values.fits
values=$tmpdir/$valuesbase
valuesstdbase=valuesstd.fits
valuesstd=$tmpdir/$valuesstdbase
if [ x"$oversample" = x ]; then
    cd $tmpdir; ln -fs $cropbase $valuesbase; cd $curdir
    if [ x"$instd" != x   -a   -f "$instd" ]; then
        cd $tmpdir; ln -fs $cropstdbase $valuesstdbase; cd $curdir
    fi
else
    # For the central coordinate, we can't simply multiply the X,Y by the
    # oversample factor otherwise the over-sampled pixel with distance of 0
    # will be on the edge of the original pixel. To make sure that the
    # central oversampled pixel is in the center of the original pixel, we
    # need to shift by half the oversampling factor (as an integer).
    os=$oversample # To shorten the next two commands!.
    xcenter=$(echo $xcenter | awk '{print '$os'*$1-int('$os'/2)}')
    ycenter=$(echo $ycenter | awk '{print '$os'*$1-int('$os'/2)}')
    rmax=$(echo    $rmax    | awk '{print '$oversample'*$1}')
    astwarp $crop --scale=$oversample,$oversample -o$values
    if [ x"$instd" != x    -a    -f "$instd" ]; then
        astwarp $cropstd --scale=$oversample,$oversample -o$valuesstd
    fi
fi





# Define the standard deviation option to MakeCatalog
# ---------------------------------------------------
#
# The standard deviation input can be a file or a single number. If it is a
# file, it has been cropped or warped (in the oversample scenario) in the
# previous blocks of the code. In this part, we set the option string to
# pass onto MakeCatalog in all possibilities (being a single number, a file
# or not defined).
#
# If the user has not given 'instd', then just pass an empty string to
# MakeCatalog.
if [ x"$instd" = x ]; then
    finalinstd=""
else
     # When 'instd' is a file, put the 'valuesstd' as the option value
     # (which has been warped/cropped and created in the previous section).
     if [ -f "$instd" ]; then
         finalinstd="--instd=$valuesstd --stdhdu=1"

     # When 'instd' is a number, simply give the number as the value.
     else
         finalinstd="--instd=$instd"
     fi
fi





# Generate the apertures image
# ----------------------------
#
# The apertures image is generated using MakeProfiles with the parameters
# specified in the echo statement:
#
# 1             -- ID of profile (irrelevant here!)
# xcenter       -- X center position (in pixels).
# ycenter       -- Y center position (in pixels).
# 7             -- Type of the profiles (radial distance).
# 1             -- The Sersic or Moffat index (irrelevant here!).
# positionangle -- position angle.
# axisratio     -- axis ratio.
# rmax          -- magnitude of the profile within the truncation radius (rmax).
# 1             -- Truncation in radius unit.
#
# After making the apertures image, we will set the central pixel to have
# the radius 1 (to avoid missing the central pixel in later phases.
radialaperturesbase=radial-raw.fits
radialapertures=$tmpdir/$radialaperturesbase
precfactor=$(astarithmetic 10 $precision pow --quiet)
radialaperturesholed=$tmpdir/radial-raw-with-hole.fits
echo "1 $xcenter $ycenter 7 $rmax 1 $positionangle $axisratio 1 1" \
     | astmkprof --background=$values --backhdu=1 --mforflatpix \
                 --mode=img --clearcanvas --type=float32 $quiet \
                 --circumwidth=1 --replace --output=$radialaperturesholed
astarithmetic $radialaperturesholed set-i \
              i 0 ne 1 fill-holes set-good \
              i good i 1 + where $precfactor x uint32 \
              $quiet --output $radialapertures
rm $radialaperturesholed





# Generate an azimuthal profile
# -----------------------------
#
# Create an azimuthal apertures image if the user specifies a range of
# angles over which compute the radial profile. The azimuthal radial
# profile is combined with the radial apertures generated above, to only
# consider the desired portions of the image. If the azimuth option is not
# called, then the aperture image is a symbolic link to the radial aperture
# image constructed above.
aperturesrawbase=apertures-raw.fits
aperturesraw=$tmpdir/$aperturesrawbase
if [ x"$azimuth" != x ]; then

    # Function to normalize angles to the range [0, 360) For example, if
    # --azimuth=337,367, then the angle to be used should be azimuth=337,7.
    # Here, this conversion is done. Negative angles are also considered.
    transform_angle() {
        angle=$1
        new_angle=$(awk "BEGIN { print ($angle % 360 + 360) % 360 }")
        echo $new_angle
    }

    # Generate the azimuthal apertures over the full profile.
    azraw=$tmpdir/azimuth-raw.fits
    azone=$tmpdir/azimuth-one.fits
    echo "1 $xcenter $ycenter 9 $rmax 1 $positionangle $axisratio 1 1" \
        | astmkprof --background=$values --backhdu=1 --mforflatpix \
                    --mode=img --clearcanvas --type=float32 $quiet \
                    --circumwidth=1 --replace --output=$azraw

    # Loop over all the pairs. But first, make sure that the final file
    # (with all azimuthal ranges) is not present (from a previous run for
    # example).
    rm -f $aperturesraw
    for p in $(echo "$azimuth" | tr ':' ' '); do

        # Get the initial and final azimuth angles
        raw_azimuth_ini=$(echo "$p" | awk 'BEGIN{FS=","} {print $1}')
        raw_azimuth_fin=$(echo "$p" | awk 'BEGIN{FS=","} {print $2}')

        # Transform the azimuth angles to [0, 360) range
        azimuth_ini=$(transform_angle $raw_azimuth_ini)
        azimuth_fin=$(transform_angle $raw_azimuth_fin)

        # From the azimuthal aperture image, consider only that portion
        # that are in between (or outside) the two specified angles. Set
        # the rest of the pixels to zero values (on the radial apertures
        # image).
        #
        # There are two ways that the angle range can be defined:
        #
        #   - The first is smaller than the second (for example
        #     '--azimuth=10,20'). In this case, we will use an 'and'
        #     between the regions of each angle so the user gets the region
        #     between the two angles.
        #
        #   - The first is greater than the second (for example
        #     '--azimuth=355,5') In this case, we assume that the user
        #     wants an azimuthal range outside the two angles. In the
        #     example above, its the 10 degrees around the azimuthal angle
        #     0. For this case, we should use 'or' between the two regions.
        #
        #   - The central pixel contains all azimuth angles, it should
        #     therefore be included in azimthal ranges. By default
        #     MakeProfiles is forced to give it a single azimuth angle
        #     value, so without explicitly adding the central pixel to all
        #     azimuthal profiles, it the radius of 1 will be ignored in
        #     some azimuthal ranges and kept in others. But this is only
        #     because it is smaller than our sampling rate, in practice,
        #     the central pixel contains all azimuth angles. So when
        #     defining the 'arc', we are also adding 'radial 1 eq or' to
        #     add that central pixel.
        condition=$(echo $azimuth_ini $azimuth_fin \
                         | awk ' {if ($1<$2) print "and"; \
                                  else       print "or"}')
        astarithmetic --output=$azone $quiet \
                      $radialapertures  -h1 uint16 set-radial \
                      $azraw            -h1 uint16 set-azimuth \
                      azimuth $azimuth_ini  uint16 ge \
                      azimuth $azimuth_fin  uint16 le $condition \
                      radial 1 eq or set-arc \
                      radial arc 1 uint8 ne 0 uint8 where

        # Put the output of this azimuthal range into the intermediate
        # file.
        if [ -f $aperturesraw ]; then
            aperturesrawtmp=$tmpdir/azimuth-intermediate-tmp.fits
            astarithmetic $aperturesraw set-i $azone set-o \
                          i i 0 eq o where -g1 -o$aperturesrawtmp
            mv $aperturesrawtmp $aperturesraw
        else
            mv $azone $aperturesraw
        fi
    done
else
    cd $tmpdir; ln -fs $radialaperturesbase $aperturesrawbase; cd $curdir
fi





# MakeCatalog configuration options
# ---------------------------------
#
# If not given, don't use anything and just let MakeCatalog use its default
# values.
if [ x"$sigmaclip" = x ]; then finalsigmaclip=""
else                           finalsigmaclip="--sigmaclip=$sigmaclip";
fi
if [ x"$zeropoint" = x ]; then finalzp=""
else                           finalzp="--zeropoint=$zeropoint";
fi
if [ x$polar = x0 ]; then polaropt=""
else
    azclumps=$tmpdir/azimuth-clumps.fits
    astarithmetic $azraw $aperturesraw 0 eq 0 where -g1 \
                  uint16 --output=$azclumps
    polaropt="--clumpsfile=$azclumps --clumpshdu=1 --clumpscat"
fi





# Undersampling the aperture image
# --------------------------------
#
# The undersampling is for making the size of the apertures larger so the
# number of pixels on each aperture (at each radius) will be larger. Most
# of times this option is good to average over a larger number of pixels
# and increase the signal-to-noise ratio of the measurement.
aperturesbase=apertures.fits
apertures=$tmpdir/$aperturesbase
if [ x"$undersample" != x ]; then

    # Divide by the undersampling factor (multiplied by the precision
    # factor: multiple of 10).
    astarithmetic $aperturesraw $undersample $precfactor  x / \
                  $quiet int16 --output $apertures.fits

    # The integer division above will re-create the 0-valued hole in the
    # center! So we need to fill it like above.
    astarithmetic $apertures.fits set-i \
                  i 0 ne 1 fill-holes set-good \
                  i good i 1 + where \
                  $quiet --output $apertures
else
    cd $tmpdir; ln -fs $aperturesrawbase $aperturesbase; cd $curdir
fi





# Extract each measurement column(s)
# ----------------------------------
#
# The user gives each desired MakeCatalog option name as a value to the
# '--measure' option here as a comma-separated list of values. But we want
# to feed them into MakeCatalog (which needs each one of them to be
# prefixed with '--' and separated by a space).
finalmeasure=$(echo "$measure" \
                   | awk 'BEGIN{FS=","} \
                          END{for(i=1;i<=NF;++i) printf "--%s ", $i}')





# Obtain the radial profile
# -------------------------
#
# The radial profile is obtained using MakeCatalog. In practice, we obtain
# a catalogue using the segmentation image previously generated (the
# elliptical apertures) and the original input image for measuring the
# values.
#
# --spatialresolution=0.0: in a radial profile, the areas used for each
#   measurement are pre-determined analytically, so the error in area
#   (relevant for the estimation of the surface brightness error for
#   example) should be considered as zero. The default value is '2.0' which
#   will give over-estimated error bars in the center.
#
#   We tested this by making a mock profile and adding different noise
#   realizations (just changing the random number generator seed). The
#   radial profiles of each noise realization were then measured and their
#   scatter was compared with the '--sb-error' column of MakeCatalog when
#   given to this radial profile script. We saw that without
#   '--spatialresolution=0.0', the measured errors were overestimated in
#   the center, but after adding this, the '--sb-error' column was nicely
#   consistent with the scatter in the mock profiles at central radii.
cat=$tmpdir/catalog.fits
astmkcatalog $apertures --hdu=1 --valuesfile=$values --valueshdu=1 \
             --ids $finalinstd $finalmeasure $finalsigmaclip $finalzp \
             $polaropt --spatialresolution=0.0 --output=$cat $quiet





# Final radii of each row
# -----------------------
#
# In the steps above many operations were done on the labels. It is now
# necessary to revert them all and obtain the final radius column that will
# go into the output.
radraw=$tmpdir/radii.fits
if [ x"$oversample" != x ]; then

    asttable $cat -o$radraw \
             --colmetadata=1,RADIUS,pix,"Radial distance" \
             -c'arith OBJ_ID float32 '$precfactor' / 1 - '$oversample' /'

elif [ x"$undersample" != x ]; then

    # Under sampling corrects for the 100 multiplication factor before
    # MakeCatalog.
    asttable $cat -c'arith OBJ_ID float32 1 - '$undersample' x' \
             -o$radraw

else

    asttable $cat -c'arith OBJ_ID float32 '$precfactor' / 1 -' \
             -o$radraw --colmetadata=1,RADIUS,pix,"Radial distance"

fi






# Prepare the final output columns
# --------------------------------
#
# Add the radius column calculated above to the measured columns. If the
# user has asked for it, build the output in the format of '--customtable'
# in MakeProfiles. But before anything, we need to set the options to print
# the other columns untouched (we only want to change the first column).
outraw=$tmpdir/out-raw.fits
restcols=$(astfits $cat -h1 \
               | awk '/^TFIELDS/{for(i=2;i<=$3;++i) \
                                  printf "--catcolumns=%d ", i}')
asttable $radraw --catcolumnfile=$cat $restcols --output=$outraw \
         --colmetadata=1,RADIUS,pix,"Radial distance"





# Correct possibly extra rows
# ---------------------------
#
# Due to a possible position angle and axis ratio, it may happen that at
# least one extra radial aperture (that is given to
# MakeCatalog). Therefore, once the final radii are set (accounting for
# over/under sampling), we should make sure that the final output doesn't
# contain radii larger than what the user asked for.
#
# We are not using '$output' here, because the user may have asked to make
# a polar plot and we may confront some bugs in the middle. It is better
# that the final output only be built after everything is complete.
#
# When a polar plot is not requested, we can immediately write the output,
# otherwise, we need to add a second HDU, so we will not write the final
# output yet.
if [ x$polar = x0 ]; then outputraw=$output
else
    osuffix=$(echo $output | awk 'BEGIN{FS="."}{print $NF}')
    outputraw=$tmpdir/output.$osuffix
fi
asttable $outraw --range=RADIUS,0,$rmax --output=$outputraw





# Polar plot
# ----------
#
# Generate the polar plot if it was requested.
if [ x$polar = x1 ]; then

    # Set metadata on the polar-plot catalog for easy operations below (and
    # for debugging). To include the maximum radius (99 in case of
    # '--rmax=100'), because Table's '--range' option doesn't include the
    # maximum number and we are dealing with integers, we need to add
    # 'rmax' with one.
    polarcat=$tmpdir/polar-catalog.fits
    rmaxpone=$(astarithmetic $rmax uint32 1 + --quiet)
    asttable $cat -hCLUMPS --range=HOST_OBJ_ID,0,$rmaxpone \
             --colmetadata=HOST_OBJ_ID,RADIUS,pix,"Radial distance" \
             --colmetadata=ID_IN_HOST_OBJ,AZIMUTH,degree,"Azimuth angle" \
             --output=$polarcat

    # Find the maximum value of radius and azimuth angel.
    maxrad=$(aststatistics  $polarcat -h1 -cRADIUS  --maximum)
    maxazim=$(aststatistics $polarcat -h1 -cAZIMUTH --maximum)
    minazim=$(aststatistics $polarcat -h1 -cAZIMUTH --minimum)

    # If the maximum value is 360, make sure we have more than one row with
    # an azimuth of 360. If there is only one element at an azimuth of 360,
    # it was just the central pixel and the largest azimuthal value was not
    # 360.
    if [ $maxazim = 360 ]; then
        if [ $(asttable $polarcat --equal=AZIMUTH,360 | wc -l) = 1 ]; then
            maxazim=$(asttable $polarcat -cAZIMUTH --range=AZIMUTH,0,360 \
                               -O | aststatistics --maximum)
        fi
    fi

    # In the end, many azimuth angles close to radius zero are not going to
    # be filled at all! We need to set those to NaN within the radial
    # profile script (because there was no data there and we want to
    # preserve the zero-valued pixels in our radial profiles). So here
    # (before calling MakeProfiles), we replace any possible zeros in the
    # input table with a pre-defined number (the maximum value in the image
    # plus one), and later, rever this specific number back to zero.
    polarcatz=$tmpdir/polar-catalog-no-zeros.fits
    numzero=$(asttable $polarcat --equal=3,0 | wc -l)
    if [ $numzero = 0 ]; then
        ln -sf $polarcat $polarcatz
    else
        maxmesure=$(aststatistics $polarcat -h1 -c3 --maximum -q)
        placeholder=$(astarithmetic $maxmesure 1 + -q)
        asttable $polarcat -cRADIUS,AZIMUTH --output=$polarcatz \
                 -c'arith $3 set-m m m 0 eq '$placeholder' where'
    fi

    # Generate the raw polar plot. When the user sets a non-zero minimum
    # azimuthal angle, we need the full range of azimuthal angles here, so
    # the azimuthal width is effectively just the maximum azimuthal
    # angle. If the minimum is non-zero, we will later crop the output to
    # remove the all-zero columns of the output image.
    ppraw=$tmpdir/polar-plot-raw.fits
    asttable $polarcat -h1 -cAZIMUTH,RADIUS,3  \
        | awk '{print(1, $1, $2, 4, 0, 0, 0, 0, $3, 0)}' \
        | astmkprof --mforflatpix --mode=img --oversample=1 \
                    --mergedsize=$maxazim,$maxrad --clearcanvas \
                    --output=$ppraw $quiet \
                    --crpix=1,1 \
                    --pc=1,0,0,1 \
                    --cdelt=1,1 \
                    --cunit=deg,pix \
                    --ctype=AZI,RAD \
                    --crval=1,0

    # If the minimum azimuthal angle is not zero, then we need to crop the
    # zero-valued regions (as described above).
    ppcrop=$tmpdir/polar-plot-crop.fits
    if [ $minazim = 0 ]; then
        ln -sf $ppraw $ppcrop
    else
        astcrop $ppraw --mode=img --section=$minazim:$maxazim,: \
                --output=$ppcrop $quiet
    fi

    # Any zero-valued pixel in the output of MakeProfiles means that there
    # was no input data there so we should set them to Nan! Recall that
    # above, if there was any actual zeros, they were replaced and we set
    # them back to zero after setting the NaNs.
    ppout=$tmpdir/polar-plot.fits
    if  [ $numzero = 0 ]; then
        astarithmetic $ppcrop set-p p p 0 eq nan where \
                      --output=$ppout $quiet
    else
        astarithmetic $ppcrop                       set-p \
                      p p 0            eq nan where set-z \
                      z z $placeholder eq 0   where \
                      --output=$ppout $quiet
    fi

    # If the output is a FITS file, then add a new HDU. Otherwise (for
    # example output is plain-text), we need to make a new file.
    if astfits $outputraw > /dev/null 2>&1; then
        exthdu=2
        polarfile=$outputraw
    else
        # Set its properties.
        exthdu=1
        polarfile=$(echo $output | sed -e's|.'$osuffix'|-polar.fits|')

        # In case it already exists, delete it (because we later
        # "--copy"). Recall that we are in the scenario that the actual
        # radial profile was not FITS.
        if [ -f $polarfile ]; then rm $polarfile; fi
    fi

    # Copy the image into the desired output file and set its HDU name.
    astfits $ppout --copy=1 --output=$polarfile
    astfits $polarfile --hdu=$exthdu --update=EXTNAME,POLAR-PLOT

    # Copy the 'rawoutput' into the final output.
    cp $outputraw $output
fi





# Remove temporary files
# ----------------------
#
# If the user does not specify to keep the temporal files with the option
# `--keeptmp', then remove the whole directory.
if [ $keeptmp = 0 ]; then
    rm -r $tmpdir
fi
