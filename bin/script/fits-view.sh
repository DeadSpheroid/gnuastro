#!/bin/sh

# View the contents of FITS files using DS9 (if there is an image in any of
# the HDUs) or TopCat (when there is a table). This script can be called
# within a '.desktop' file to easily view FITS images or tables in a GUI.
#
# Current maintainer:
#     Mohammad Akhlaghi <mohammad@akhlaghi.org>
# All author(s):
#     Mohammad Akhlaghi <mohammad@akhlaghi.org>
#     Raul Infante-Sainz <infantesainz@gmail.com>
#     Sepideh Eskandarlou <sepideh.eskandarlou@gmail.com>
# Copyright (C) 2020-2024 Free Software Foundation, Inc.
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

# 'LC_NUMERIC' is responsible for formatting numbers printed by the OS.  It
# prevents floating points like '23,45' instead of '23.45'.
export LC_NUMERIC=C





# Default option values (can be changed with options on the
# command-line).
hdu=""
quiet=0
prefix=""
ds9mode=""
ds9scale=""
ds9extra=""
ds9center=""
globalhdu=""
ds9geometry=""
version=@VERSION@
ds9colorbarmulti=""
scriptname=@SCRIPT_NAME@





# Output of '--usage' and '--help':
print_usage() {
    cat <<EOF
$scriptname: run with '--help' for list of options
EOF
}

print_help() {
    cat <<EOF
Usage: $scriptname [OPTION] FITS-file(s)

This script is part of GNU Astronomy Utilities $version.

This script will take any number of FITS inputs and will pass them to
TOPCAT or SAO DS9 depending on the the first input FITS file: if it
contains an image HDU, the files will be passed to SAO DS9, otherwise, to
TOPCAT.

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
  -h, --hdu=STR           Extension name or number of input data.
  -g, --globalhdu=STR     Use this HDU for all inputs, ignore '--hdu'.

 Output:
  -G, --ds9geometry=INTxINT Size of DS9 window, e.g., for HD (800x1000) and
                          for 4K (1800x3000). If not given, the script will
                          attempt to find your screen resolution and find a
                          good match, otherwise, use the default size.
  -e, --ds9extra=STR      Extra options to pass to DS9 after default ones.
  -s, --ds9scale=STR      Custom value to '-scale' option in DS9.
  -c, --ds9center=FLT,FLT Coordinates of center in shown DS9 window.
  -O, --ds9mode=img/wcs   Coord system to interpret '--ds9center' (img/wcs).
  -m, --ds9colorbarmulti  Separate color bars for each loaded image.
  -p, --prefix=STR        Directory containing DS9 or TOPCAT executables.

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





# Output of '--version':
print_version() {
    cat <<EOF
$scriptname (GNU Astronomy Utilities) $version
Copyright (C) 2020-2024 Free Software Foundation, Inc.
License GPLv3+: GNU General public license version 3 or later.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written/developed by Mohammad Akhlaghi and Sepideh Eskandarlou
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
#   long option names: '--longname value' and '--longname=value'. For short
#   option names we want '-l value', '-l=value' and '-lvalue' (where '-l'
#   is the short version of the hypothetical '--longname' option).
#
#   The first case (with a space between the name and value) is two
#   command-line arguments. So, we'll need to shift it two times. The
#   latter two cases are a single command-line argument, so we just need to
#   "shift" the counter by one. IMPORTANT NOTE: the ORDER OF THE LATTER TWO
#   cases matters: '-h*' should be checked only when we are sure that its
#   not '-h=*').
#
# OPTIONS WITH NO VALUE (ON-OFF OPTIONS)
#
#   For these, we just want the two forms of '--longname' or '-l'. Nothing
#   else. So if an equal sign is given we should definitely crash and also,
#   if a value is appended to the short format it should crash. So in the
#   second test for these ('-l*') will account for both the case where we
#   have an equal sign and where we don't.
inputs=""
while [ $# -gt 0 ]
do
    # Put the values in the proper variable.
    case "$1" in

        # Input options
        -h|--hdu)            aux="$2";                             check_v "$1" "$aux"; hdu="$hdu $aux"; shift;shift;;
        -h=*|--hdu=*)        aux="${1#*=}";                        check_v "$1" "$aux"; hdu="$hdu $aux"; shift;;
        -h*)                 aux="$(echo "$1"  | sed -e's/-h//')"; check_v "$1" "$aux"; hdu="$hdu $aux"; shift;;
        -g|--globalhdu)      aux="$2";                             check_v "$1" "$aux"; globalhdu="$aux"; shift;shift;;
        -g=*|--globalhdu=*)  aux="${1#*=}";                        check_v "$1" "$aux"; globalhdu="$aux"; shift;;
        -g*)                 aux="$(echo "$1"  | sed -e's/-g//')"; check_v "$1" "$aux"; globalhdu="$aux"; shift;;

        # Output options
        -G|--ds9geometry)     ds9geometry="$2";                    check_v "$1" "$ds9geometry"; shift;shift;;
        -G=*|--ds9geometry=*) ds9geometry="${1#*=}";               check_v "$1" "$ds9geometry"; shift;;
        -G*)                  ds9geometry=$(echo "$1"  | sed -e's/-g//');  check_v "$1" "$ds9geometry"; shift;;
        -s|--ds9scale)        ds9scale="$2";                       check_v "$1" "$ds9scale"; shift;shift;;
        -s=*|--ds9scale=*)    ds9scale="${1#*=}";                  check_v "$1" "$ds9scale"; shift;;
        -s*)                  ds9scale=$(echo "$1"  | sed -e's/-s//');  check_v "$1" "$ds9scale"; shift;;
        -e|--ds9extra)        ds9extratmp="$2";                    check_v "$1" "$ds9extratmp"; shift;shift;;
        -e=*|--ds9extra=*)    ds9extratmp="${1#*=}";               check_v "$1" "$ds9extratmp"; shift;;
        -e*)                  ds9extratmp=$(echo "$1"  | sed -e's/-s//');  check_v "$1" "$ds9extratmp"; shift;;
        -c|--ds9center)       ds9center="$2";                      check_v "$1" "$ds9center"; shift;shift;;
        -c=*|--ds9center=*)   ds9center="${1#*=}";                 check_v "$1" "$ds9center"; shift;;
        -c*)                  ds9center=$(echo "$1"  | sed -e's/-s//');  check_v "$1" "$ds9center"; shift;;
        -O|--ds9mode)         ds9mode="$2";                       check_v "$1" "$ds9mode"; shift;shift;;
        -O=*|--ds9mode=*)     ds9mode="${1#*=}";                  check_v "$1" "$ds9mode"; shift;;
        -O*)                  ds9mode=$(echo "$1"  | sed -e's/-s//');  check_v "$1" "$ds9mode"; shift;;
        -m|--ds9colorbarmulti)    ds9colorbarmulti=1; shift;;
        -m*|--ds9colorbarmulti=*) on_off_option_error --ds9colorbarmulti -m;;
        -p|--prefix)          prefix="$2";                         check_v "$1" "$prefix"; shift;shift;;
        -p=*|--prefix=*)      prefix="${1#*=}";                    check_v "$1" "$prefix"; shift;;
        -p*)                  prefix=$(echo "$1"  | sed -e's/-p//');  check_v "$1" "$prefix"; shift;;

        # Operating-mode options
        -q|--quiet)       quiet=1; shift;;
        -q*|--quiet=*)    on_off_option_error --quiet -q;;
        -?|--help)        print_help; exit 0;;
        -'?'*|--help=*)   on_off_option_error --help -?;;
        -V|--version)     print_version; exit 0;;
        -V*|--version=*)  on_off_option_error --version -V;;
        --cite)           astfits --cite; exit 0;;
        --cite=*)         on_off_option_error --cite;;

        # Unrecognized option:
        -*) echo "$scriptname: unknown option '$1'"; exit 1;;

        # Not an option (not starting with a '-'): assumed to be input FITS
        # file name.
        *) if [ x"$inputs" = x ]; then inputs="$1"; else inputs="$inputs $1"; fi; shift;;
    esac

    # In case '--ds9extra' was given, add it to any previous values. The
    # reason is that this option can often become very long and it may be
    # easier to break it into multiple calls.
    if [ x"$ds9extratmp" != x ]; then
        ds9extra="$ds9extra $ds9extratmp"
    fi
done





# If center coordinates (--center) is not given at all.
if [ x"$ds9center" != x ]; then

    # Make sure only two values are given.
    ncenter=$(echo $ds9center | awk 'BEGIN{FS=","}END{print NF}')
    if [ x$ncenter != x2 ]; then
        cat <<EOF
$scriptname: '--ds9center' (or '-c') only takes two values, but $ncenter value(s) where given in '$ds9center'
EOF
        exit 1
    fi

    # With '--ds9center', its also necessary to give a mode (--ds9mode).
    modeerrorinfo="Depending on the nature of the central coordinates, please give either 'img' (for pixel coordinates) or 'wcs' (for RA,Dec) to the '--ds9mode' (or '-O') option."
    if [ x"$ds9mode" = x ]; then
        cat <<EOF
$scriptname: no coordinate mode provided! $modeerrorinfo
EOF
        exit 1

    # Make sure the value to '--mode' is either 'wcs' or 'img'. Note: '-o'
    # means "or" and is preferred to '[ ] || [ ]' because only a single
    # invocation of 'test' is done. Run 'man test' for more.
    elif [ "$ds9mode" = wcs   -o    "$ds9mode" = img ]; then
        junk=1
    else
        cat <<EOF
$scriptname: '$ds9mode' not acceptable for '--ds9mode' (or '-O'). $modeerrorinfo
EOF
        exit 1
    fi
fi

# If HDUs are given, make sure they are the same number as images.
if ! [ x"$hdu" = x ]; then
    nhdu=$(echo $hdu | wc -w)
    nin=$(echo $inputs | wc -w)
    if [ $nhdu -lt $nin  ]; then
        cat <<EOF
$scriptname: number of input HDUs (values to '--hdu' or '-h') is less than the number of inputs. If all the HDUs are the same, you can use the '--globalhdu' or '-g' option
EOF
        exit 1
    fi
fi





# Set a good geometry if the user hasn't given any. So far, we just depend
# on 'xrandr' to find the screen resolution, if it doesn't exist, then we
# won't set this option at all and let DS9 use its default size.
ds9geoopt=""
if [ x"$ds9geometry" = x ]; then
    if command -v xrandr > /dev/null 2>/dev/null; then

        # The line with a '*' is the resolution of the used screen. We will
        # then extract the height (second value) and set DS9's geometry
        # based on that.
        ds9geometry=$(xrandr \
                        | grep '*' \
                        | sed 's/x/ /' \
                        | awk 'NR==1{w=0.75*$2; printf "%dx%d\n", w, $2}')
    fi
fi
ds9geoopt="-geometry $ds9geometry"





# Set the DS9 '-scale' option value. If nothing is given, set the mode to
# 'zscale'. Also set some aliases to simplify usage on the command-line
# (and let the user give '--ds9scale=minmax' instead of '--ds9scale="mode
# minmax"').
#
#    zscale  --> mode zscale
#    minmax  --> mode minmax
#
# Note: '-o' means "or" and is preferred to '[ ] || [ ]' because only a
# single invocation of 'test' is done. Run 'man test' for more.
if [ x"$ds9scale" = x    -o    "$ds9scale" = "zscale" ]; then
    ds9scale="mode zscale"
elif [ "$ds9scale" = "minmax" ]; then
     ds9scale="mode minmax"
fi
ds9scaleopt="-scale $ds9scale"





# Set the DS9 pan option.
ds9pan=""
if [ x"$ds9center" != x ]; then
    ds9pancent=$(echo $ds9center | awk 'BEGIN{FS=","} {print $1, $2}')
    if [ $ds9mode = wcs ]; then
        ds9pan="-pan to $ds9pancent wcs fk5"
    else
        ds9pan="-pan to $ds9pancent image"
    fi
fi





# If '--prefix' is given, add it to the PATH for the remainder of the
# commands in this script.
if [ "x$prefix" != x ]; then
    ds9exec="$prefix/ds9"
    topcatexec="$prefix/topcat"
else
    ds9exec=ds9
    topcatexec=topcat
fi





# Color map contrast and bias for the default SLS color map designed to
# make the color of the largest value red, not white (which is the color of
# the default background). These are currently not configurable because
# generally, the colormap ('sls') is not yet configurable.
cmap=sls
cmap_bias=0.5634
cmap_contrast=0.8955





# To allow generic usage, if no input file is given (the `if' below is
# true), then just open an empty ds9.
if [ x"$inputs" = x ]; then
    $ds9exec $ds9geoopt
else
    # Select the first input.
    input1=$(echo $inputs | awk '{print $1}')

    # Do the tests only if the given file exists.
    if [ -f $input1 ]; then

        # If the first input exists. Continue if it is also a FITS file.
        if astfits $input1 -h0 > /dev/null 2>&1; then

            # Input is image or table.
            type=$(astfits $input1 --hasimagehdu)

            # If the file was a image, then  `check` will be 1.
            if [ "$type" = 1 ]; then

                # If a HDU is given, add it to all the input file names
                # (within square brackets for DS9).
                if [ x"$hdu" = x ] && [ x"$globalhdu" = x ]; then
                    inwithhdu="$inputs"
                else
                    c=1
                    inwithhdu=""
                    for i in $inputs; do
                        if [ x"$hdu" = x ]; then
                            h="$globalhdu"
                        else
                            h=$(echo $hdu | awk -vc=$c '{print $c}')
                        fi
                        inwithhdu="$inwithhdu $i[$h]"
                        c=$((c+1))
                    done
                fi

                # Read the number of dimensions.
                n0=$(astfits $input1 -h0 | awk '$1=="NAXIS"{print $3}')

                # Find the number of dimensions.
                if [ "$n0" = 0 ]; then
                    ndim=$(astfits $input1 -h1 | awk '$1=="NAXIS"{print $3}')
                else
                    ndim=$n0
                fi;

                # If multiple colorbars should be used.
                if [ x"$ds9colorbarmulti" = x ]; then multicolorbars=no;
                else                                  multicolorbars=yes;
                fi

                # Open DS9 based on the number of dimension.
                if [ "$ndim" = 2 ]; then

                    # If a HDU is specified, ignore other HDUs (recall that
                    # with '-mecube' we are viewing all the HDUs as a single
                    # cube.
                    if [ x"$hdu" = x ]; then mecube="-mecube";
                    else                     mecube="";
                    fi

                    # 2D multi-extension file: use the "Cube" window to
                    # flip/slide through the extensions.
                    execom="$ds9exec $ds9scaleopt \
                                     $ds9geoopt \
                                     $mecube \
                                     $inwithhdu \
                                     -zoom to fit \
                                     -wcs degrees \
                                     -cmap $cmap \
                                     -cmap $cmap_contrast $cmap_bias \
                                     -match frame image \
                                     -match frame colorbar \
                                     -frame lock image \
                                     -colorbar lock yes \
                                     -view multi $multicolorbars \
                                     -lock slice image \
                                     $ds9pan \
                                     $ds9extra"
                else

                    # If a HDU is given, don't use multi-frame.
                    if [ x"$hdu" = x ]; then mframe="-multiframe";
                    else                     mframe="";
                    fi

                    # 3D multi-extension file: The "Cube" window will slide
                    # between the slices of a single extension. To flip
                    # through the extensions (not the slices), press the
                    # top row "frame" button and from the last four buttons
                    # of the bottom row ("first", "previous", "next" and
                    # "last") can be used to switch through the extensions
                    # (while keeping the same slice).
                    execom="$ds9exec $ds9scaleopt \
                                     $ds9geoopt -wcs degrees \
                                     $mframe \
                                     $inwithhdu \
                                     -lock slice image \
                                     -lock frame image \
                                     -zoom to fit \
                                     -cmap sls \
                                     -match frame colorbar \
                                     -colorbar lock yes \
                                     -view multi $multicolorbars \
                                     $ds9pan \
                                     $ds9extra"
                fi

            # When input was table.
            else

                # If a HDU is given, add it to all the input file names
                # (with a '#' between the filename and HDU in TOPCAT).
                if [ x"$hdu" = x ]; then
                inwithhdu="$inputs"
                else
                    c=1
                    inwithhdu=""
                    for i in $inputs; do
                        h=$(echo $hdu | awk -vc=$c '{print $c}')
                        inwithhdu="$inwithhdu $i#$h"
                        c=$((c+1))
                    done
                fi

                # TOPCAT command.
                execom="$topcatexec $inwithhdu"
            fi

            # Run the final command and print it if not in '--quiet' mode.
            if [ $quiet = 0 ]; then
                echo "Running: $(echo $execom | sed 's|* | |')";
            fi
            $execom

            # 'astfits' couldn't open the file.
        else
            echo "$scriptname:$input1: not a FITS file"
        fi

    # File does not exist.
    else
        echo "$scriptname:$input1: does not exist"
    fi
fi
