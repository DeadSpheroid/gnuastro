/*********************************************************************
Fits - View and manipulate FITS extensions and/or headers.
Fits is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
     Pedram Ashofteh-Ardakani <pedramardakani@pm.me>
Copyright (C) 2016-2024 Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#ifndef ARGS_H
#define ARGS_H






/* Array of acceptable options. */
struct argp_option program_options[] =
  {
    {
      "arguments",
      UI_KEY_ARGUMENTS,
      "STR",
      0,
      "plain-text file with list of input files.",
      GAL_OPTIONS_GROUP_INPUT,
      &p->arguments,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },



    {
      0, 0, 0, 0,
      "HDU (extension) information:",
      UI_GROUP_EXTENSION_INFORMATION
    },
    {
      "numhdus",
      UI_KEY_NUMHDUS,
      0,
      0,
      "Print number of HDUs in the given FITS file.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->numhdus,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "datasum",
      UI_KEY_DATASUM,
      0,
      0,
      "Calculate HDU's datasum and print in stdout.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->datasum,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "datasum-encoded",
      UI_KEY_DATASUMENCODED,
      0,
      0,
      "16 character string encoding of '--datasum'.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->datasumencoded,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "pixelscale",
      UI_KEY_PIXELSCALE,
      0,
      0,
      "Return the pixel-scale of the HDU's WCS.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->pixelscale,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "pixelareaarcsec2",
      UI_KEY_PIXELAREAARCSEC2,
      0,
      0,
      "Pixel area in arc-seconds squared.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->pixelareaarcsec2,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "skycoverage",
      UI_KEY_SKYCOVERAGE,
      0,
      0,
      "Image coverage in the WCS coordinates.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->skycoverage,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "hastablehdu",
      UI_KEY_HASTABLEHDU,
      0,
      0,
      "File has at least one table HDU.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->hastablehdu,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "hasimagehdu",
      UI_KEY_HASIMAGEHDU,
      0,
      0,
      "File has at least one image HDU.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->hasimagehdu,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "listtablehdus",
      UI_KEY_LISTTABLEHDUS,
      0,
      0,
      "List all table HDUs within the file.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->listtablehdus,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "listimagehdus",
      UI_KEY_LISTIMAGEHDUS,
      0,
      0,
      "List all image HDUs within the file.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->listimagehdus,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "listallhdus",
      UI_KEY_LISTALLHDUS,
      0,
      0,
      "List all HDUs within the file.",
      UI_GROUP_EXTENSION_INFORMATION,
      &p->listallhdus,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    {
      0, 0, 0, 0,
      "HDU (extension) manipulation:",
      UI_GROUP_EXTENSION_MANIPULATION
    },
    {
      "remove",
      UI_KEY_REMOVE,
      "STR/INT",
      0,
      "Remove extension from input file.",
      UI_GROUP_EXTENSION_MANIPULATION,
      &p->remove,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "copy",
      UI_KEY_COPY,
      "STR/INT",
      0,
      "Copy extension to output file.",
      UI_GROUP_EXTENSION_MANIPULATION,
      &p->copy,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "cut",
      UI_KEY_CUT,
      "STR/INT",
      0,
      "Copy extension to output and remove from input.",
      UI_GROUP_EXTENSION_MANIPULATION,
      &p->cut,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "primaryimghdu",
      UI_KEY_PRIMARYIMGHDU,
      0,
      0,
      "Copy/cut image HDUs to primary/zero-th HDU.",
      UI_GROUP_EXTENSION_MANIPULATION,
      &p->primaryimghdu,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    {
      0, 0, 0, 0,
      "Keywords (in one HDU):",
      UI_GROUP_KEYWORD
    },
    {
      "asis",
      UI_KEY_ASIS,
      "STR",
      0,
      "Write value as-is (may corrupt FITS file).",
      UI_GROUP_KEYWORD,
      &p->asis,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "keyvalue",
      UI_KEY_KEYVALUE,
      "STR[,STR,...]",
      0,
      "Only print the value of requested keyword(s).",
      UI_GROUP_KEYWORD,
      &p->keyvalue,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "delete",
      UI_KEY_DELETE,
      "STR",
      0,
      "Delete a keyword from the header.",
      UI_GROUP_KEYWORD,
      &p->delete,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "rename",
      UI_KEY_RENAME,
      "STR,STR",
      0,
      "Rename keyword, keeping value and comments.",
      UI_GROUP_KEYWORD,
      &p->rename,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "update",
      UI_KEY_UPDATE,
      "STR,STR",
      0,
      "Update a keyword value or comments.",
      UI_GROUP_KEYWORD,
      &p->update,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "write",
      UI_KEY_WRITE,
      "STR",
      0,
      "Write a keyword (with value, comments and units).",
      UI_GROUP_KEYWORD,
      &p->write,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "history",
      UI_KEY_HISTORY,
      "STR",
      0,
      "Add HISTORY keyword, any length is ok.",
      UI_GROUP_KEYWORD,
      &p->history,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "comment",
      UI_KEY_COMMENT,
      "STR",
      0,
      "Add COMMENT keyword, any length is ok.",
      UI_GROUP_KEYWORD,
      &p->comment,
      GAL_TYPE_STRLL,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "date",
      UI_KEY_DATE,
      0,
      0,
      "Set the DATE keyword to the current time.",
      UI_GROUP_KEYWORD,
      &p->date,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "verify",
      UI_KEY_VERIFY,
      0,
      0,
      "Verify the CHECKSUM and DATASUM keywords.",
      UI_GROUP_KEYWORD,
      &p->verify,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "printallkeys",
      UI_KEY_PRINTALLKEYS,
      0,
      0,
      "Print all keywords in the selected HDU.",
      UI_GROUP_KEYWORD,
      &p->printallkeys,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "printkeynames",
      UI_KEY_PRINTKEYNAMES,
      0,
      0,
      "Print all keyword names.",
      UI_GROUP_KEYWORD,
      &p->printkeynames,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "copykeys",
      UI_KEY_COPYKEYS,
      "INT:INT/STR,STR",
      0,
      "Range/Names of keywords to copy in output.",
      UI_GROUP_KEYWORD,
      &p->copykeys,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "datetosec",
      UI_KEY_DATETOSEC,
      "STR",
      0,
      "FITS date to sec from 1970/01/01T00:00:00",
      UI_GROUP_KEYWORD,
      &p->datetosec,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "wcsdistortion",
      UI_KEY_WCSDISTORTION,
      "STR",
      0,
      "Convert WCS distortion to another type.",
      UI_GROUP_KEYWORD,
      &p->wcsdistortion,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "wcscoordsys",
      UI_KEY_WCSCOORDSYS,
      "STR",
      0,
      "Convert WCS coordinate system.",
      UI_GROUP_KEYWORD,
      &p->wcscoordsys,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },




    {
      0, 0, 0, 0,
      "Meta-image creation:",
      UI_GROUP_META
    },
    {
      "pixelareaonwcs",
      UI_KEY_PIXELAREAONWCS,
      0,
      0,
      "Image where pixels show their area in sky (WCS).",
      UI_GROUP_META,
      &p->pixelareaonwcs,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "edgesampling",
      UI_KEY_EDGESAMPLING,
      "INT",
      0,
      "Number of extra samplings in pixel sides.",
      UI_GROUP_META,
      &p->edgesampling,
      GAL_TYPE_SIZE_T,
      GAL_OPTIONS_RANGE_GE_0,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    /* Output options. */
    {
      "outhdu",
      UI_KEY_OUTHDU,
      "STR",
      0,
      "HDU/extension in output for --copykeys.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->outhdu,
      GAL_TYPE_STRING,
      GAL_OPTIONS_RANGE_ANY,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },
    {
      "colinfoinstdout",
      UI_KEY_COLINFOINSTDOUT,
      0,
      0,
      "Column info/metadata when printing to stdout.",
      GAL_OPTIONS_GROUP_OUTPUT,
      &p->colinfoinstdout,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },





    /* Operating mode options. */
    {
      "quitonerror",
      UI_KEY_QUITONERROR,
      0,
      0,
      "Quit if there is an error on any action.",
      GAL_OPTIONS_GROUP_OPERATING_MODE,
      &p->quitonerror,
      GAL_OPTIONS_NO_ARG_TYPE,
      GAL_OPTIONS_RANGE_0_OR_1,
      GAL_OPTIONS_NOT_MANDATORY,
      GAL_OPTIONS_NOT_SET
    },



    {0}
  };





/* Define the child argp structure. */
struct argp
gal_options_common_child = {gal_commonopts_options,
                            gal_options_common_argp_parse,
                            NULL, NULL, NULL, NULL, NULL};

/* Use the child argp structure in list of children (only one for now). */
struct argp_child
children[]=
{
  {&gal_options_common_child, 0, NULL, 0},
  {0, 0, 0, 0}
};

/* Set all the necessary argp parameters. */
struct argp
thisargp = {program_options, parse_opt, args_doc, doc, children, NULL, NULL};
#endif
