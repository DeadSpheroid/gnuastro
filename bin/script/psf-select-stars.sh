#!/bin/sh

# Build a catalogue of "good stars" that will be considered for
# constructing an extended and non parametric PSF. Here, "good stars" means
# that they don't have close objects that affect it sourrondings and
# consequently they are not contaminated. The script will construct a
# catalog of stars from reference datasets (Gaia) if the user does not
# provide another one. In addition to this, other parameters like the axis
# ratio are considered to filter the sample and select only proper stars.
#
# Run with '--help' for more information.
#
# Original author:
#     Sepideh Eskandarlou <sepideh.eskandarlou@gmail.com>
# Contributing author:
#     Raul Infante-Sainz <infantesainz@gmail.com>
#     Mohammad Akhlaghi <mohammad@akhlaghi.org>
#     Carlos Morales-Socorro <cmorsoc@gmail.com>
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





# Default parameter's values
hdu=1
quiet=""
output=""
tmpdir=""
catalog=""
keeptmp=""
segmented=""
brightmag=-10
racolumn="ra"
faintmagdiff=4
deccolumn="dec"
minaxisratio=0.9
magnituderange=""
version=@VERSION@
mindistdeg=0.016666667          # one arcmin: 1/60
field="phot_g_mean_mag"
scriptname=@SCRIPT_NAME@
parallaxanderrorcolumn=""
matchaperturedeg=0.002777778    # 10 arcsec: 10/3600
dataset="gaia --dataset=dr3"





# Output of '--usage'
print_usage() {
     cat <<EOF
$scriptname: run with '--help' to list the options.
EOF
}





# Output of '--help'
print_help() {
   cat <<EOF
Usage: $scriptname [OPTIONS] image.fits

Build a catalogue of "good stars" that will be considered for constructing
an extended and non parametric PSF. Here, "good stars" means that they
don't have close objects that affect it sourrondings and consequently they
are not contaminated. The script will construct a catalog of stars from
reference datasets (Gaia) if the user does not provide another one. In
addition to this, other parameters like the axis ratio are considered to
filter the sample and select only proper stars.

$scriptname options:
 Input:
  -h, --hdu=STR/INT       HDU/Extension name of number of the input file.
  -S, --segmented=STR     Segmentation file obtained by Segment (astsegment).
  -D, --dataset=STR       Query dataset ("gaia --dataset=edr3", etc.).
  -r, --racolumn=STR      Right Ascension (R.A.) column name.
  -d, --deccolumn=STR     Declination (Dec) column name.
  -f, --field=STR         Magnitude column name ("phot_rp_mean_mag", for Gaia).
  -p, --parallaxanderrorcolumn=STR,STR The name of the parallax column.
  -m, --magnituderange=FLT,FLT The range of magnitude.
  -Q, --minaxisratio=FLT  Minimum axis ratio to be accepted (default to 0.9).
  -M, --mindistdeg=FLT    Minimum distance to more bright neighbour stars.
                          to be accepted, in degrees.
  -c, --catalog=STR       Catalog of stars containing:
                          ra, dec, magnitude, parrallax, parrallax_error.
  -a, --matchaperturedeg=FLT Aperture, in pixels, to match catalogue ra and
                          dec coordinates with clumps' ra and dec.
  -F, --faintmagdiff      The difference from the faintest star which the user
                          will be determined the faintest star in
                          '--magnituderange' option.
  -b, --brightmag         The limit for selecting wider range of bright stars.

 Output:
  -o, --output            Output table with the object coordinates.
  -t, --tmpdir            Directory to keep temporary files.
  -k, --keeptmp           Keep temporal/auxiliar files.

 Operating mode:
  -h, --help              Print this help.
      --cite              BibTeX citation for this program.
  -q, --quiet             Don't print any extra information in stdout.
  -V, --version           Print program version.

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponfing short options.

GNU Astronomy Utilities home page: http://www.gnu.org/software/gnuastro/

Report bugs to bug-gnuastro@gnu.org
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

Written/developed by Sepideh Eskandarlou.
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





# Separate command-line arguments from options and put the option values
# into the respective variables.
#
# OPTIONS WITH A VALUE:
#
#   Each option has three lines because we take into account the three common
#   formats:
#   For long option names, '--longname value' and '--longname=value'.
#   For short option names, '-l value', '-l=value' and '-lvalue'
#   (where '-l' is the short version of the hypothetical '--longname option').
#
#   The first case (with a space between the name and value) is two
#   command-line arguments. So, we'll need to shift it twice. The
#   latter two cases are a single command-line argument, so we just need to
#   "shift" the counter by one.
#
#   IMPORTANT NOTE: the ORDER OF THE LATTER TWO cases matters: '-h*' should be
#   checked only when we are sure that its not '-h=*').
#
# OPTIONS WITH NO VALUE (ON-OFF OPTIONS)
#
#   For these, we just want the forms of '--longname' or '-l'. Nothing
#   else. So if an equal sign is given we should definitely crash and also,
#   if a value is appended to the short format it should crash. So in the
#   second test for these ('-l*') will account for both the case where we
#   have an equal sign and where we don't.
inputs=""
while [ $# -gt 0 ]
do
   case "$1" in
   # Input parameters.
       -h|--hdu)               hdu="$2";                                     check_v "$1" "$hdu";  shift;shift;;
       -h=*|--hdu=*)           hdu="${1#*=}";                                check_v "$1" "$hdu";  shift;;
       -h*)                    hdu=$(echo "$1" | sed -e's/-h//');            check_v "$1" "$hdu";  shift;;
       -S|--segmented)         segmented="$2";                               check_v "$1" "$segmented";  shift;shift;;
       -S=*|--segmented=*)     segmented="${1#*=}";                          check_v "$1" "$segmented";  shift;;
       -S*)                    segmented=$(echo "$1" | sed -e's/-S//');      check_v "$1" "$segmented";  shift;;
       -r|--racolumn)          racolumn="$2";                                check_v "$1" "$racolumn";  shift;shift;;
       -r=*|--racolumn=*)      racolumn="${1#*=}";                           check_v "$1" "$racolumn";  shift;;
       -r*)                    racolumn=$(echo "$1" | sed -e's/-r//');       check_v "$1" "$racolumn";  shift;;
       -d|--deccolumn)         deccolumn="$2";                               check_v "$1" "$deccolumn";  shift;shift;;
       -d=*|--deccolumn=*)     deccolumn="${1#*=}";                          check_v "$1" "$deccolumn";  shift;;
       -d*)                    deccolumn=$(echo "$1" | sed -e's/-d//');      check_v "$1" "$deccolumn";  shift;;
       -p|--parallaxanderrorcolumn) parallaxanderrorcolumn="$2";            check_v "$1" "$parallaxanderrorcolumn";  shift;shift;;
       -p=*|--parallaxanderrorcolumn=*) parallaxanderrorcolumn="${1#*=}";    check_v "$1" "$parallaxanderrorcolumn";  shift;;
       -p*)                    parallaxanderrorcolumn=$(echo "$1" | sed -e's/-p//'); check_v "$1" "$parallaxanderrorcolumn";  shift;;
       -D|--dataset)           dataset="$2";                                 check_v "$1" "$dataset";  shift;shift;;
       -D=*|--dataset=*)       dataset="${1#*=}";                            check_v "$1" "$dataset";  shift;;
       -D*)                    dataset=$(echo "$1" | sed -e's/-D//');        check_v "$1" "$dataset";  shift;;
       -c|--catalog)           catalog="$2";                                 check_v "$1" "$catalog";  shift;shift;;
       -c=*|--catalog=*)       catalog="${1#*=}";                            check_v "$1" "$catalog";  shift;;
       -c*)                    catalog=$(echo "$1" | sed -e's/-c//');        check_v "$1" "$catalog";  shift;;
       -f|--field)             field="$2";                                   check_v "$1" "$field";  shift;shift;;
       -f=*|--field=*)         field="${1#*=}";                              check_v "$1" "$field";  shift;;
       -f*)                    field=$(echo "$1" | sed -e's/-f//');          check_v "$1" "$field";  shift;;
       -F|--faintmagdiff)      faintmagdiff="$2";                            check_v "$1" "$faintmagdiff";  shift;shift;;
       -F=*|--faintmagdiff=*)  faintmagdiff="${1#*=}";                       check_v "$1" "$faintmagdiff";  shift;;
       -F*)                    faintmagdiff=$(echo "$1" | sed -e's/-F//');   check_v "$1" "$faintmagdiff";  shift;;
       -b|--brightmag)         brightmag="$2";                               check_v "$1" "$brightmag";  shift;shift;;
       -b=*|--brightmag=*)     brightmag="${1#*=}";                          check_v "$1" "$brightmag";  shift;;
       -b*)                    brightmag=$(echo "$1" | sed -e's/-b//');      check_v "$1" "$brightmag";  shift;;
       -m|--magnituderange)       magnituderange="$2";                          check_v "$1" "$magnituderange";  shift;shift;;
       -m=*|--magnituderange=*)   magnituderange="${1#*=}";                    check_v "$1" "$magnituderange";  shift;;
       -m*)                       magnituderange=$(echo "$1" | sed -e's/-m//'); check_v "$1" "$magnituderange";  shift;;
       -a|--matchaperturedeg)     matchaperturedeg="$2";                        check_v "$1" "$matchaperturedeg";shift;shift;;
       -a=*|--matchaperturedeg=*) matchaperturedeg="${1#*=}";                check_v "$1" "$matchaperturedeg";  shift;;
       -a*)                       matchaperturedeg=$(echo "$1" | sed -e's/-a//');  check_v "$1" "$matchaperturedeg";  shift;;
       -Q|--minaxisratio)      minaxisratio="$2";                            check_v "$1" "$minaxisratio";  shift;shift;;
       -Q=*|--minaxisratio=*)  minaxisratio="${1#*=}";                       check_v "$1" "$minaxisratio";  shift;;
       -Q*)                    minaxisratio=$(echo "$1" | sed -e's/-Q//');   check_v "$1" "$minaxisratio";  shift;;
       -M|--mindistdeg)        mindistdeg="$2";                              check_v "$1" "$mindistdeg";  shift;shift;;
       -M=*|--mindistdeg=*)    mindistdeg="${1#*=}";                         check_v "$1" "$mindistdeg";  shift;;
       -M*)                    mindistdeg=$(echo "$1" | sed -e's/-M//');     check_v "$1" "$mindistdeg";  shift;;

   # Output parameters
       -k|--keeptmp)           keeptmp=1; shift;;
       -k*|--keeptmp=*)        on_off_option_error --keeptmp -k;;
       -t|--tmpdir)            tmpdir="$2";                                  check_v "$1" "$tmpdir";  shift;shift;;
       -t=*|--tmpdir=*)        tmpdir="${1#*=}";                             check_v "$1" "$tmpdir";  shift;;
       -t*)                    tmpdir=$(echo "$1" | sed -e's/-t//');         check_v "$1" "$tmpdir";  shift;;
       -o|--output)            output="$2";                                  check_v "$1" "$output"; shift;shift;;
       -o=*|--output=*)        output="${1#*=}";                             check_v "$1" "$output"; shift;;
       -o*)                    output=$(echo "$1" | sed -e's/-o//');         check_v "$1" "$output"; shift;;

   # Non-operating options.
       -q|--quiet)             quiet=" -q"; shift;;
       -q*|--quiet=*)          on_off_option_error --quiet -q;;
       -?|--help)              print_help; exit 0;;
       -'?'*|--help=*)         on_off_option_error --help -?;;
       -V|--version)           print_version; exit 0;;
       -V*|--version=*)        on_off_option_error --version -V;;
       --cite)                 print_citation; exit 0;;
       --cite=*)               on_off_option_error --cite;;

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

# Check that 'segmented' is the output of 'astsegment'.
if [ x"$segmented" != x ]; then
    nhdu=$(astfits $segmented --listimagehdus \
               | grep 'OBJECTS\|CLUMPS' \
               | wc -l)
    if [ $nhdu != 2 ]; then
        cat <<EOF
$scriptname: the file given to '--segmented' does not have 'CLUMPS' and 'OBJECTS' HDUs. Please give an output from 'astsegment'
EOF
        exit 1
    fi
elif [ ! -f $segmented ]; then
    cat <<EOF
$scriptname: $segmented, no such file or directory"
EOF
    exit 1
fi

# If the brighter and fainter range of magnitude are not given at all.
if [ x$magnituderange = x ]; then
    cat<<EOF
$scriptname: no magnitude range provided. Values to '--magnituderange' (or '-m') should be provided"
EOF
    exit 1
else
    nmagrng=$(echo $magnituderange | awk 'BEGIN{FS=","} \
                                          {for(i=1;i<=NF;++i) c+=$i!=""} \
                                          END{print c}')
    if [ x$nmagrng != x2 ]; then
        cat<<EOF
$scriptname: '--magnituderange' (or '-m') only takes two values, but $nmagrng were given
EOF
        exit 1
    fi
fi

# If the parallax and parallax error columns are not given at all.
if [ x$parallaxanderrorcolumn = x ]; then
    columnquery=$racolumn,$deccolumn,$field
else
    nmparallax=$(echo $parallaxanderrorcolumn \
                      | awk 'BEGIN{FS=","} \
                                  {for(i=1;i<=NF;++i) c+=$i!=""} \
                                  END{print c}')
    if [ x$nmparallax != x2 ]; then
        cat<<EOF
$scriptname: '--parallaxanderrorcolumn' (or '-p') only takes two values, but $nmparallax were given
EOF
        exit 1
    else
        columnquery=$racolumn,$deccolumn,$field,$parallaxanderrorcolumn
    fi
fi

# If the minimum axis ratio is not given at all.
if [ x$minaxisratio = x ]; then
    cat<<EOF
$scriptname: no minimum axis ratio provided. Value to '--minaxisratio' (or '-Q') should be provided
EOF
    exit 1
fi

# If the minimum distance between contaminants is not given at all.
if [ x$mindistdeg = x ]; then
    cat<<EOF
$scriptname: no minimum distance (for rejecting neighbors) provided. Value to '--mindistdeg' (or '-M') should be provided
EOF
    exit 1
fi

# If the match aperture radius is not given at all.
if [ x$matchaperturedeg = x ]; then
    cat<<EOF
$scriptname: no aperture matching radius provided. Value to '--matchaperturedeg' (or '-a') should be provided
EOF
    exit 1
fi





# Basic parameters: magnitude range, parallax and parallax error
# --------------------------------------------------------------
#
# Obtain the magnitude range from the command line arguments.
brightmag=$(echo "$magnituderange" | awk 'BEGIN{FS=","} {print $1}')
faintmag=$(echo "$magnituderange" | awk 'BEGIN{FS=","} {print $2}')

# Range of magnitude which are in the wider range of brighter and fainter
# magnitude.
fainter=$(astarithmetic $faintmag float32 $faintmagdiff float32 + --quiet)

# Obtain the parallax and parallax error column names.
parallax=$(echo "$parallaxanderrorcolumn" \
                | awk 'BEGIN{FS=","} {print $1}')
parallax_error=$(echo "$parallaxanderrorcolumn" \
                      | awk 'BEGIN{FS=","} {print $2}')





# Define a temporary directory and the final output file
# ------------------------------------------------------
#
# Construct the temporary directory. If the user does not specify any
# directory, then a default one with the base name of the input image will
# be constructed.  If the user set the directory, then make it. This
# directory will be deleted at the end of the script if the user does not
# want to keep it (with the `--keeptmp' option).

# The final catalog is also defined here if the user does not provide an
# explicit name. If the user has defined a specific path/name for the
# output, it will be used for saving the output file. If the user does not
# specify an output name, then a default value containing the field, min,
# and max magnitudes will will be generated.
bname_prefix=$(basename $inputs | sed 's/\.fits/ /' | awk '{print $1}')
if [ x"$tmpdir" = x ]; then \
  tmpdir=$(pwd)/"$bname_prefix"_psfcreateselectstar_"$field"_"$brightmag"_"$faintmag"
fi

if [ -d "$tmpdir" ]; then
  junk=1
else
  mkdir -p "$tmpdir"
fi

# Default output catalog file
if [ x"$output" = x ]; then
  output="$bname_prefix"_psfcreateselectstar_"$field"_"$brightmag"_"$faintmag".fits
fi





# Obtain the main catalog of stars
# --------------------------------
#
# Here, a catalogue of stars is constructed. If the user provides the
# catalog, then check that the objects overlap with the input image and
# filter the objects to have only the ones with proper brightness. If the
# user does not provide a catalogue, obtain it using the program
# 'astquery'.
#
# The output name of the catalog of stars in the range of the magnitude
# between smaller than the brighter and larger than the fainter.
catalog_main=$tmpdir/catalog-main-$brightmag-$fainter.fits

# The output name of the sky coverage.
skycoverage=$tmpdir/skycoverage.fits

# If the catalog and the image overlap, then select the stars
# in the range of magnitude between bright and faint stars.
if [ x"$catalog" != x ]; then

    # Find minimum and maximum RA/Dec values of the input image.
    minraimg=$(astfits $inputs --hdu=$hdu --skycoverage \
                   | grep 'RA' | awk '{print $2}')
    maxraimg=$(astfits $inputs --hdu=$hdu --skycoverage \
                   | grep 'RA' | awk '{print $3}')

    mindecimg=$(astfits $inputs --hdu=$hdu --skycoverage \
                    | grep 'DEC' | awk '{print $2}')
    maxdecimg=$(astfits $inputs --hdu=$hdu --skycoverage \
                    | grep 'DEC' | awk '{print $3}')

    # Find the overlap area between catalog and image.
    asttable $catalog --inpolygon=$racolumn,$deccolumn \
             --polygon="$minraimg,$mindecimg \
                        : $minraimg,$maxdecimg \
                        : $maxraimg,$mindecimg \
                        : $maxraimg,$maxdecimg" \
             --output=$skycoverage

    # The number of stars in the overlaping area.
    number=$(asttable $skycoverage | wc -l)

    # If catalog overlap the image, select stars with magnitudes between
    # the fainter and larger than bright values.
    if [ "$number" = 0 ]; then

        # Stop if the catalog doesn't overlap with the image.
        echo "Image and catalog do not overlap."
        echo "Please provide an image and a catalog that overlap together."
        exit 1
    else

        # Make a file for output of 'asttable'.
        if [ -f "$catalog_main" ]; then
            echo "External Catalog already exists "
        else

            # Select stars with magnitude between brighter to fainter.
            asttable $catalog \
                     --range=$field,$brightmag,$fainter --sort=$field \
                     --range=$racolumn,$minraimg,$maxraimg \
                     --range=$deccolumn,$mindecimg,$maxdecimg \
                     --output=$catalog_main $quiet

            # If there are no stars in the range of magnitude
            numstars=$(asttable $catalog_main -i \
			   | grep 'Number of rows' | awk '{print $4}')
            if [ "$numstars" = 0 ]; then
                rm -rf $tmpdir
                echo "There were no stars in magnitude range $brightmag to $faintmag"
                exit 2
            fi
        fi
    fi

# If the user doesn't provide a catalog, use 'astquery' to obtain it.
else
    # Check if the catalog already exists.
    if [ -f $catalog_main ]; then
        echo "Queried Cataloge already exists."
    else
	# Download the catalog with the necessary parameters.
        astquery $dataset --output=$catalog_main \
		 --overlapwith=$inputs --hdu=$hdu \
                 --column=$columnquery \
		 --range=$field,$brightmag,$fainter \
                 --sort=$field $quiet
    fi
fi





# Target and main catalogs
# ------------------------
#
# There are two catalogs:

# - Main catalog: wider catalog with extra objects (fainter and brighter)
# than the ones the user is looking for. This is necessary for identifying
# the contaminant objects that fall out of the range of magnitude of the
# target stars. This is the one provided, or downloaded by this script.

# - Target catalog: catalog of stars with no contaminated stars and proper
# parameters (magnitude, parallaxes, etc.). It is a subset of the main
# catalog.

catalog_target=$tmpdir/catalog-gaia-$brightmag-$faintmag.fits
asttable $catalog_main \
         --range=$field,$brightmag,$faintmag \
         --output=$catalog_target $quiet





# Select stars with good parallax
# -------------------------------
#
# Only stars with small parallaxes are allowed. Here, stars with high value
# of parallax are removed. More in particular, with some options of
# 'asttable' such as '--noblank', stars that have parallax larger than
# three times the parallax-error are removed.
goodparallax=$tmpdir/parallax-good.fits
if [ x"$parallaxanderrorcolumn" != x ]; then
    asttable $catalog_target -c$racolumn,$deccolumn -c$field \
             --range=$field,$brightmag,$faintmag \
             --colinfoinstdout --noblank=4 \
             -c'arith '$parallax' '$parallax' abs \
                      '$parallax_error' 3 x lt nan where ' \
             | asttable -c$racolumn,$deccolumn -c$field \
                        --output=$goodparallax $quiet
else
    cp $catalog_target $goodparallax
fi





# Find circular objects
# ---------------------
#
# Consider only objects with an axis ratio above 'minaxisrario'
# (minaxisratio=1 <--> circular shape). To do that, a catalogue is computed
# using 'astmkcatalog' using as input the segmented image. Then, that
# catalogue is matched with the Gaia catalog and filtered by the
# 'minaxisratio' criteria.
circular=$tmpdir/circular-objects.fits
if [ -f $circular ]; then
    echo "Catalog of circular objects already exists"
else
    if [ x"$segmented" = x ]; then
	cp $goodparallax $circular
        echo "No segmented image given to use '--minaxisratio'"
    else
	# Intermediate files.
	qraw=$tmpdir/axisratio-raw.fits
	qmatch=$tmpdir/axisratio-match.fits

	# Measure axis ratios
	astmkcatalog $segmented --ids --ra --dec --axis-ratio \
                     --position-angle --fwhm --clumpscat \
                     --output=$qraw $quiet

	# Match with downloaded catalog
	astmatch $goodparallax --ccol1=$racolumn,$deccolumn \
		 $qraw --hdu2=CLUMPS --ccol2=RA,DEC \
		 --aperture=$matchaperturedeg \
		 --output=$qmatch $quiet \
		 --outcols=a$racolumn,a$deccolumn,a$field,bAXIS_RATIO,bPOSITION_ANGLE,bFWHM

	# Select circular objects
	asttable $qmatch --range=AXIS_RATIO,$minaxisratio,1 \
		 --output=$circular $quiet
    fi
fi





# Determine the number of neighbors around each object
# -----------------------------------------------------
#
# To have a good catalogue of stars to construct a PSF, it is necessary
# that they are not contaminated by bright near objects. Here, those
# objects that are contaminated by the presence of a nearby object are
# rejected.

# First of all based on ra and dec of circular stars determine the distance
# between the circular star and brigther stars in minimumm distance and
# based on '--noblank' and 'where' remove stars tha have distance larger
# than minimum distance. Then remove each circular stars that have more
# than 9 neighbors.
lessneighbor=$tmpdir/less-than-2-stars.txt
moreneighbor=$tmpdir/more-than-2-stars.txt
echo "# Column 1: ra     [deg,f64] Right ascension" >  $lessneighbor
echo "# Column 2: dec    [deg,f64] Declination"     >> $lessneighbor
echo "# Column 3: $field [mag,f64] Magnitude"       >> $lessneighbor

echo "# Column 1: ra     [deg,f64] Right ascension" >  $moreneighbor
echo "# Column 2: dec    [deg,f64] Declination"     >> $moreneighbor
echo "# Column 3: $field [mag,f64] Magnitude"       >> $moreneighbor

# LOOP over the target catalog. For each star, check the the neighbors.
asttable $circular -c$racolumn,$deccolumn,$field \
         | while read r d mag; do

   # Make a file for number of neighbourhood.
   numberneighbor=$tmpdir/"$r_$d"th-star-neighbor.fits

   # Find number of neighborhod for each star.
   asttable $catalog_main -c$racolumn,$deccolumn -c$field \
	    -c'arith '$racolumn' '$deccolumn' '$r' '$d' distance-on-sphere \
                     set-i i i '$mindistdeg' gt nan where' \
            --noblankend=4 \
	    --colinfoinstdout \
            --output=$numberneighbor $quiet

   # Check number of neighborhod and remove each star that has more than 2
   # neighbors: 1 means match with itself; >2 means that there are nearby
   # contaminant objects).
   lof=$(asttable $numberneighbor | wc -l)
   if [ $lof -lt 2 ]; then
       echo $r $d $mag >> $lessneighbor
   else
       echo $r $d $mag >> $moreneighbor
   fi
done





# Create the final output catalogue
# ---------------------------------
#
# This is the catalog with the proper stars once all filtering parameters
# have been taken into account and nearby contaminants have been rejected.
# Just convert the '.txt' catalog from the above loop to a '.fits' file.
asttable $lessneighbor --output=$output

# The contaminated objects catalog may be useful for the user for checking
# objects to be rejected or not.
contaminated_objects=$tmpdir/catalog_contaminated_objects.txt
asttable $moreneighbor --output=$contaminated_objects




# Print the statistics of the generated catalogue
# -----------------------------------------------
#
# If the user do not want to see all steps of run, the 'quiet' option must
# be used. Without this option user can see all steps of run.
if [ x"$quiet" = x ]; then
   stats_nstars_total=$(asttable $catalog_main | wc -l)
   stats_nstars_noneighbors=$(asttable $output | wc -l)
   stats_nstars_goodparallax=$(asttable $goodparallax | wc -l)
   echo "Number of stars in wider mag-range: $stats_nstars_total"
   echo "Number of stars with good parallax: $stats_nstars_goodparallax"
   echo "Number of stars with no neighbors:  $stats_nstars_noneighbors"
   if [ ! -$segmented ]; then
      stats_nstars_matched=$(asttable $qmatch)
      echo "Number of clumps matched stars: $stats_nstars_matched"
   fi
fi





# Remove temporary directory
# --------------------------
#
# If user does not specify to keep build file with the option of
# --keeptmp', then the directory will be removed.
if [ x"$keeptmp" = x ]; then
   rm -rf $tmpdir
fi
