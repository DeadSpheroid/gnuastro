#!/bin/sh

# Separate input datasets into multiple nights, run with '--help', or see
# description under 'print_help' (below) for more.
#
# Original author:
#     Mohammad Akhlaghi <mohammad@akhlaghi.org>
# Contributing author(s):
# Copyright (C) 2019-2023 Free Software Foundation, Inc.
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
copy=0
link=0
hour=11
quiet=0
key=DATE
prefix=./
version=@VERSION@
stdintime=10000000
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

This script will look into a HDU/extension for a header keyword in the
given FITS files and interpret the value as a date. The inputs will be
separated by "night"s. The definition a "nights" is set with the '--hour'
option (just note that the FITS time may be recorded in UTC, not local
time)! It will then print a list of all the input files along with the
following two columns: night number and file number in that night (sorted
by time). With '--link' a symbolic link (one for each input) will be made
with names including the night classifier. With '--copy' instead of a link,
a copy of the inputs will be made.

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
  -k, --key=STR           Header keyword specifying date to use.
  -H, --hour=FLT          Hour in next day to be included in night.
                          Be aware of time zones with this argument!

 Output:
  -l, --link              list of inputs with night and file number.
  -c, --copy              Copy the files, don't make symbolic links.
  -p, --prefix=STR        Prefix for outputs of '--copy' and '--link'.

 Operating mode:
  -?, --help              Print this help list.
      --cite              BibTeX citation for this program.
  -q, --quiet             Don't print any extra information in stdout.
  -V, --version           Print program version.
      --stdintimeout      Micro-seconds to wait for standard input
                          (used for operations within this script).

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
Copyright (C) 2015-2023 Free Software Foundation, Inc.
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
    case "$1" in
        # Input parameters.
        -h|--hdu)         hdu="$2";                           check_v "$1" "$hdu";  shift;shift;;
        -h=*|--hdu=*)     hdu="${1#*=}";                      check_v "$1" "$hdu";  shift;;
        -h*)              hdu=$(echo "$1"  | sed -e's/-h//'); check_v "$1" "$hdu";  shift;;
        -k|--key)         key="$2";                           check_v "$1" "$key";  shift;shift;;
        -k=*|--key=*)     key="${1#*=}";                      check_v "$1" "$key";  shift;;
        -k*)              key=$(echo "$1"  | sed -e's/-k//'); check_v "$1" "$key";  shift;;
        -H|--hour)        hour="$2";                          check_v "$1" "$hour"; shift;shift;;
        -H=*|--hour=*)    hour="${1#*=}";                     check_v "$1" "$hour"; shift;;
        -H*)              hour=$(echo "$1" | sed -e's/-H//'); check_v "$1" "$hour"; shift;;

        # Output parameters
        -l|--link)        link=1; shift;;
        -l*|--link=*)     on_off_option_error --link -l;;
        -c|--copy)        copy=1; shift;;
        -c*|--copy=*)     on_off_option_error --copy -c;;
        -p|--prefix)      prefix="$2";                          check_v "$1" "$prefix"; shift;shift;;
        -p=*|--prefix=*)  prefix="${1#*=}";                     check_v "$1" "$prefix"; shift;;
        -p*)              prefix=$(echo "$1" | sed -e's/-p//'); check_v "$1" "$prefix"; shift;;

        # Operating mode options
        --stdintimeout)   stdintime="$2";                       check_v "$1" "$stdintime"; shift;shift;;
        --stdintimeout=*) stdintime="${1#*=}";                  check_v "$1" "$stdintime"; shift;;

        # Non-operating options.
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
done





# Basic sanity checks on arguments.
if [ x"$inputs" = x ]; then
    echo "$scriptname: no input FITS files."
    echo "Run with '--help' for more information on how to run."
    exit 1
fi

if [ $copy = 1 ] && [ $link = 1 ]; then
    echo "$scriptname: '--copy' and '--link' cannot be called together"
    exit 1
fi





# Find the night number (including until 9:00 a.m of the following
# day) of each image.
#
# To do this, we'll convert the date into Unix epoch time (seconds
# since 1970-01-01,00:00:00) and keep that with the filename.
#
# A simple AWK expression for what we are doing here inside of Table
# (where the first input column is the filename and the second is the
# date in seconds):
#
# awk '{h='$hour'; d=int($2/86400); \
#       if(int($2)%86400<(h*3600)) n=d-1; else n=d; \
#       print $1, $2, n }'
#
# Finally, we are sorting all the images with the unix-second column
# to make sure that they are ordered in observation order later.
list=$(astfits --keyvalue=$key --hdu=$hdu $inputs --colinfoinstdout \
               | asttable -cFILENAME \
                          -c'arith '$key' date-to-sec' \
                          -c'arith '$key' date-to-sec set-sec \
                                   sec 86400 / int32 set-day \
                                   day \
                                     sec int32 86400 % '$hour' 3600  x lt \
                                     day 1 - \
                                   where' \
                          --colmetadata=3,NIGHT,counter,"Observing night." \
                          --colinfoinstdout --stdintimeout=$stdintime \
               | asttable --sort=UNIXSEC --colinfoinstdout \
                          --stdintimeout=$stdintime)





# To see the result of the step above, uncomment the next line. You
# can use a similar line to inspect the steps for later variables also
# (just changing the variable name). IMPORTANT NOTE: Just don't forget
# the double-quotes around the variable name, otherwise the lines
# won't be separated.
#echo "$list"; exit 1





# Get the uniqe nights from the previous step and give each night a
# counter starting from 1. The output of this step will be two
# columns: a counter, and the night number.
#
# We are using 'asttable' here to avoid issues with spaces in
# directory names in the first line.
unique=$(echo "$list" \
             | asttable --stdintimeout=$stdintime -cNIGHT \
             | sort \
             | uniq \
             | cat -n)
#echo "check: $unique"; exit 1





# Find the FITS files of every unique day and sort them by observing
# time within that day. We'll also initialize the night-counter to 1.
echo "$unique" | while read l; do

    # Find all input files (and their Unix epoch time).
    night_to=$(echo $l | awk '{print $1}')
    night_from=$(echo $l | awk '{print $2}')
    in_this_night=$(echo "$list" \
                        | asttable --stdintimeout=$stdintime \
                                   -cFILENAME --equal=NIGHT,$night_from)

    # Now that we know this night's files, sorted by time, we can take
    # the proper action (simply list, or copy or make links).
    exposure_num=1
    echo "$in_this_night" | while read infile; do

        # Make the outputs
        outfile=$prefix"n"$night_to-$exposure_num.fits
        if   [ $copy = 1 ]; then   cp     $infile $outfile
        elif [ $link = 1 ]; then   ln -fs $infile $outfile
        else                       echo "$infile $night_to $exposure_num"
        fi

        # Increment the exposure number
        exposure_num=$((exposure_num+1))

    done
done
