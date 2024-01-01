#!/bin/sh

# Convert the given two columns into a ds9 region file.
#
# Original author:
#     Mohammad Akhlaghi <mohammad@akhlaghi.org>
# Contributing author(s):
#     Samane Raji <samaneraji@protonmail.com>
#     Raul Infante-Sainz <infantesainz@gmail.com>
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





# Default option values (can be changed with options on the
# command-line).
hdu=1
col=""
width=1
mode=wcs
radius=""
command=""
namecol=""
out=ds9.reg
color=green
dontdelete=0
version=@VERSION@
scriptname=@SCRIPT_NAME@





# Output of '--usage' and '--help':
print_usage() {
    cat <<EOF
$scriptname: run with '--help' for list of options
EOF
}

print_help() {
    cat <<EOF
Usage: $scriptname [OPTION] FITS-files

This script is part of GNU Astronomy Utilities $version.

This script will take two column names (or numbers) and return a "region
file" (with one region over each entry) for easy inspection of catalog
entries in this SAO DS9.

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
  -c, --column=STR,STR    Columns to use as coordinates (name or number).
  -m, --mode=wcs|img      Coordinates in WCS or image (default: $mode)
  -n, --namecol=STR       ID of each region (name or number of a column)

 Output:
  -C, --color             Color for the regions (read by DS9).
  -w, --width             Line thickness of the regions (in DS9).
  -r, --radius            Radius of each region (arcseconds if in WCS mode).
  -D, --dontdelete        Don't delete output if it exists.
  -o, --output=STR        Name of output file.
      --command="STR"     DS9 command to run after making region.

 Operating mode:
  -?, --help              Print this help list.
      --cite              BibTeX citation for this program.
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
Copyright (C) 2015-2024 Free Software Foundation, Inc.
License GPLv3+: GNU General public license version 3 or later.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.

Written/developed by Mohammad Akhlaghi
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
    # Initialize 'tcol':
    tcol=""

    # Put the values in the proper variable.
    case "$1" in
        # Input parameters.
        -h|--hdu)           hdu="$2";                            check_v "$1" "$hdu";   shift;shift;;
        -h=*|--hdu=*)       hdu="${1#*=}";                       check_v "$1" "$hdu";   shift;;
        -h*)                hdu=$(echo "$1"  | sed -e's/-h//');  check_v "$1" "$hdu";   shift;;
        -c|--column)        tcol="$2";                           check_v "$1" "$tcol";  shift;shift;;
        -c=*|--column=*)    tcol="${1#*=}";                      check_v "$1" "$tcol";  shift;;
        -c*)                tcol=$(echo "$1"  | sed -e's/-c//'); check_v "$1" "$tcol";  shift;;
        -m|--mode)          mode="$2";                           check_v "$1" "$mode";  shift;shift;;
        -m=*|--mode=*)      mode="${1#*=}";                      check_v "$1" "$mode";  shift;;
        -m*)                mode=$(echo "$1"  | sed -e's/-m//'); check_v "$1" "$mode";  shift;;
        -n|--namecol)       namecol="$2";                        check_v "$1" "$namecol"; shift;shift;;
        -n=*|--namecol=*)   namecol="${1#*=}";                   check_v "$1" "$namecol"; shift;;
        -n*)                namecol=$(echo "$1"  | sed -e's/-n//'); check_v "$1" "$namecol"; shift;;

        # Output parameters
        -C|--color)         color="$2";                            check_v "$1" "$color";   shift;shift;;
        -C=*|--color=*)     color="${1#*=}";                       check_v "$1" "$color";   shift;;
        -C*)                color=$(echo "$1"  | sed -e's/-C//');  check_v "$1" "$color";   shift;;
        -w|--width)         width="$2";                            check_v "$1" "$width";   shift;shift;;
        -w=*|--width=*)     width="${1#*=}";                       check_v "$1" "$width";   shift;;
        -w*)                width=$(echo "$1"  | sed -e's/-w//');  check_v "$1" "$width";   shift;;
        -r|--radius)        radius="$2";                           check_v "$1" "$radius";  shift;shift;;
        -r=*|--radius=*)    radius="${1#*=}";                      check_v "$1" "$radius";  shift;;
        -r*)                radius=$(echo "$1"  | sed -e's/-r//'); check_v "$1" "$radius";  shift;;
        -D|--dontdelete)    dontdelete=1; shift;;
        -D*|--dontdelete=*) on_off_option_error --dontdelete -D;;
        -o|--output)        out="$2";                              check_v "$1" "$out";     shift;shift;;
        -o=*|--output=*)    out="${1#*=}";                         check_v "$1" "$out";     shift;;
        -o*)                out=$(echo "$1"  | sed -e's/-o//');    check_v "$1" "$out";     shift;;
        --command)          command="$2";                          check_v "$1" "$command"; shift;shift;;
        --command=*)        command="${1#*=}";                     check_v "$1" "$command"; shift;;

        # Non-operating options.
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

    # If a column was given, add it to possibly existing previous columns
    # into a comma-separate list.
    if [ x"$tcol" != x ]; then
        if [ x"$col" != x ]; then col="$col,$tcol"; else col=$tcol; fi
    fi
done




# Basic sanity checks
# ===================

# Make sure only two columns are given, then set the columns.
if [ x"$col" = x ]; then
    echo "$scriptname: no columns specified, you can use '--column' (or '-c')"
    exit 1
else
    ncols=$(echo $col | awk 'BEGIN{FS=","} \
                             {for(i=1;i<=NF;++i) c+=$i!=""} \
                             END{print c}')
    if [ x$ncols != x2 ]; then
        echo "$scriptname: only two columns should be given with '--column' (or '-c'), but $ncols were given"
        exit 1
    fi
fi

# Make sure the value to '--mode' is either 'wcs' or 'img'. Note: '-o'
# means "or" and is preferred to '[ ] || [ ]' because only a single
# invocation of 'test' is done. Run 'man test' for more.
if [ "$mode" = wcs   -o   $mode = "img" ]; then
    junk=1
else
    echo "$scriptname: value to '--mode' ('-m') should be 'wcs' or 'img'"
    exit 1
fi

# If the output already exists and the user doesn't want to overwrite, then
# abort with an error.
if [ -f $out ]; then
    if [ $dontdelete = 1 ]; then
        echo "$scriptname: '$out' already exists! Aborting due to '--dontdelete'"
        exit 1
    fi
fi

# Make sure a single column is given to '--namecol':
if [ x"$namecol" != x ]; then
    ncols=$(echo $namecol | awk 'BEGIN{FS=","}END{print NF}')
    if [ x$ncols != x1 ]; then
        echo "$scriptname: only one column should be given to '--namecol'"
        exit 1
    fi
fi





# Initalize the radius value if not given. If we are in WCS mode, select a
# 3 arcsecond radius and if we are in image mode, select a 5 pixel radius.
if [ x"$radius" = x ]; then
    if [ "$mode" = wcs ]; then radius=1
    else                       radius=3
    fi
fi

# Set the units of the radius.
if [ x"$mode" = xwcs ]; then unit="\""; else unit=""; fi





# Write the metadata in the output.
printf "# Region file format: DS9 version 4.1\n" > $out
printf "# Created by $scriptname (GNU Astronomy Utilities) $version\n" >> $out
if [ x"$inputs" = x ]; then printf "# Input from stdin\n" >> $out
else          printf "# Input file: $inputs (hdu $hdu)\n" >> $out
fi
printf "# Columns: $col\n" >> $out
if [ x"$namecol" != x ]; then
    printf "# Region name (or label) column: $namecol\n" >> $out
fi
printf "global color=%s width=%d\n" $color $width >> $out
if [ "$mode" = wcs ]; then  printf "fk5\n" >> $out
else                        printf "image\n" >> $out;   fi





# Write each region's results (when no input file is given, read from the
# standard input). Note that the 'unit' string will be empty in image-mode
# coordinates. As a result, it we don't enclose it in double-quotations,
# 'printf' will not see anything and use the next variable where the unit
# should be printed! See https://savannah.gnu.org/bugs/index.php?64153
if [ x"$namecol" = x ]; then
    if [ x"$inputs" = x ]; then
        cat /dev/stdin \
            | asttable --column=$col \
            | while read a b; do \
                  printf "circle(%f,%f,%f%s)\n" \
                         $a $b $radius "$unit" >> $out; \
              done
    else
        asttable $inputs --hdu=$hdu --column=$col \
            | while read a b; do \
                  printf "circle(%f,%f,%f%s)\n" \
                         $a $b $radius "$unit" >> $out; \
              done
    fi
else
    if [ x"$inputs" = x ]; then
        cat /dev/stdin \
            | asttable --column=$col --column=$namecol \
            | while read a b c; do \
                  printf "circle(%f,%f,%f%s) # text={%g}\n" \
                         $a $b $radius "$unit" $c >> $out; \
              done
    else
        asttable $inputs --hdu=$hdu --column=$col --column=$namecol \
            | while read a b c; do \
                  printf "circle(%f,%f,%f%s) # text={%g}\n" \
                         $a $b $radius "$unit" $c >> $out; \
              done
    fi
fi





# Run the user's command (while appending the region).
if [ x"$command" != x ]; then
    $command -regions $out
    if [ $dontdelete = 0 ]; then rm $out; fi
fi
