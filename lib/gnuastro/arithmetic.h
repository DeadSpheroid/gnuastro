/*********************************************************************
Arithmetic -- Preform arithmetic operations on datasets.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
     Faezeh Bidjarchian <fbidjarchian@gmail.com>
Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#ifndef __GAL_ARITHMETIC_H__
#define __GAL_ARITHMETIC_H__

/* Include other headers if necessary here. Note that other header files
   must be included before the C++ preparations below. */
#include <gnuastro/data.h>


/* When we are within Gnuastro's building process, 'IN_GNUASTRO_BUILD' is
   defined. In the build process, installation information (in particular
   'GAL_CONFIG_ARITH_CHAR' and the rest of the types that we needed in the
   arithmetic function) is kept in 'config.h'. When building a user's
   programs, this information is kept in 'gnuastro/config.h'. Note that all
   '.c' files must start with the inclusion of 'config.h' and that
   'gnuastro/config.h' is only created at installation time (not present
   during the building of Gnuastro). */
#ifndef IN_GNUASTRO_BUILD
#include <gnuastro/config.h>
#endif


/* C++ Preparations */
#undef __BEGIN_C_DECLS
#undef __END_C_DECLS
#ifdef __cplusplus
# define __BEGIN_C_DECLS extern "C" {
# define __END_C_DECLS }
#else
# define __BEGIN_C_DECLS                /* empty */
# define __END_C_DECLS                  /* empty */
#endif
/* End of C++ preparations */



/* Actual header contants (the above were for the Pre-processor). */
__BEGIN_C_DECLS  /* From C++ preparations */



/* Arithmetic on-off bit flags (have to be powers of 2).  */
#define GAL_ARITHMETIC_FLAG_INPLACE  1
#define GAL_ARITHMETIC_FLAG_FREE     2
#define GAL_ARITHMETIC_FLAG_NUMOK    4
#define GAL_ARITHMETIC_FLAG_ENVSEED  8
#define GAL_ARITHMETIC_FLAG_QUIET    16

#define GAL_ARITHMETIC_FLAGS_BASIC ( GAL_ARITHMETIC_FLAG_INPLACE   \
                                     | GAL_ARITHMETIC_FLAG_FREE    \
                                     | GAL_ARITHMETIC_FLAG_NUMOK )

/* Operator fixed strings. */
#define GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX     "load-col-"
#define GAL_ARITHMETIC_OPSTR_LOADCOL_HDU        "-hdu-"
#define GAL_ARITHMETIC_OPSTR_LOADCOL_FILE       "-from-"
#define GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX_LEN strlen(GAL_ARITHMETIC_OPSTR_LOADCOL_PREFIX)
#define GAL_ARITHMETIC_OPSTR_LOADCOL_HDU_LEN    strlen(GAL_ARITHMETIC_OPSTR_LOADCOL_HDU)
#define GAL_ARITHMETIC_OPSTR_LOADCOL_FILE_LEN   strlen(GAL_ARITHMETIC_OPSTR_LOADCOL_FILE)



/* Identifiers for each operator. */
enum gal_arithmetic_operators
{
  GAL_ARITHMETIC_OP_INVALID,      /* Invalid (=0 by C standard).  */

  GAL_ARITHMETIC_OP_PLUS,         /*   +     */
  GAL_ARITHMETIC_OP_MINUS,        /*   -     */
  GAL_ARITHMETIC_OP_MULTIPLY,     /*   x     */
  GAL_ARITHMETIC_OP_DIVIDE,       /*   /     */
  GAL_ARITHMETIC_OP_MODULO,       /*   %     */

  GAL_ARITHMETIC_OP_LT,           /*   <     */
  GAL_ARITHMETIC_OP_LE,           /*   <=    */
  GAL_ARITHMETIC_OP_GT,           /*   >     */
  GAL_ARITHMETIC_OP_GE,           /*   >=    */
  GAL_ARITHMETIC_OP_EQ,           /*   ==    */
  GAL_ARITHMETIC_OP_NE,           /*   !=    */
  GAL_ARITHMETIC_OP_AND,          /*   &&    */
  GAL_ARITHMETIC_OP_OR,           /*   ||    */
  GAL_ARITHMETIC_OP_NOT,          /*   !     */
  GAL_ARITHMETIC_OP_ISBLANK,      /* Similar to isnan() for floats. */
  GAL_ARITHMETIC_OP_ISNOTBLANK,   /* Inverse of 'isblank'. */
  GAL_ARITHMETIC_OP_WHERE,        /*   ?:    */

  GAL_ARITHMETIC_OP_BITAND,       /*   &     */
  GAL_ARITHMETIC_OP_BITOR,        /*   |     */
  GAL_ARITHMETIC_OP_BITXOR,       /*   ^     */
  GAL_ARITHMETIC_OP_BITLSH,       /*   <<    */
  GAL_ARITHMETIC_OP_BITRSH,       /*   >>    */
  GAL_ARITHMETIC_OP_BITNOT,       /*   ~     */

  GAL_ARITHMETIC_OP_ABS,          /* abs()   */
  GAL_ARITHMETIC_OP_POW,          /* pow()   */
  GAL_ARITHMETIC_OP_SQRT,         /* sqrt()  */
  GAL_ARITHMETIC_OP_LOG,          /* log()   */
  GAL_ARITHMETIC_OP_LOG10,        /* log10() */

  GAL_ARITHMETIC_OP_SIN,          /* sine (input in deg).    */
  GAL_ARITHMETIC_OP_COS,          /* cosine (input in deg).  */
  GAL_ARITHMETIC_OP_TAN,          /* tangent (input in deg). */
  GAL_ARITHMETIC_OP_ASIN,         /* Inverse sine (output in deg). */
  GAL_ARITHMETIC_OP_ACOS,         /* Inverse cosine (output in deg). */
  GAL_ARITHMETIC_OP_ATAN,         /* Inverse tangent (output in deg). */
  GAL_ARITHMETIC_OP_ATAN2,        /* Inv. atan (preserves quad, in deg). */
  GAL_ARITHMETIC_OP_SINH,         /* Hyperbolic sine. */
  GAL_ARITHMETIC_OP_COSH,         /* Hyperbolic cosine. */
  GAL_ARITHMETIC_OP_TANH,         /* Hyperbolic tangent. */
  GAL_ARITHMETIC_OP_ASINH,        /* Inverse hyperbolic sine. */
  GAL_ARITHMETIC_OP_ACOSH,        /* Inverse hyperbolic cosine. */
  GAL_ARITHMETIC_OP_ATANH,        /* Inverse hyperbolic tangent. */

  GAL_ARITHMETIC_OP_E,            /* The base of natural logirithm. */
  GAL_ARITHMETIC_OP_PI,           /* Circle circumference by diameter. */
  GAL_ARITHMETIC_OP_C,            /* The speed of light. */
  GAL_ARITHMETIC_OP_G,            /* The gravitational constant. */
  GAL_ARITHMETIC_OP_H,            /* Plank's constant. */
  GAL_ARITHMETIC_OP_AU,           /* Astronomical Unit (in meters). */
  GAL_ARITHMETIC_OP_LY,           /* Light years (in meters). */
  GAL_ARITHMETIC_OP_AVOGADRO,     /* Avogadro's number. */
  GAL_ARITHMETIC_OP_FINESTRUCTURE,/* Fine-structure constant. */

  GAL_ARITHMETIC_OP_RA_TO_DEGREE, /* right ascension to decimal. */
  GAL_ARITHMETIC_OP_DEC_TO_DEGREE,/* declination to decimal. */
  GAL_ARITHMETIC_OP_DEGREE_TO_RA, /* right ascension to decimal. */
  GAL_ARITHMETIC_OP_DEGREE_TO_DEC,/* declination to decimal. */
  GAL_ARITHMETIC_OP_COUNTS_TO_MAG,/* Counts to magnitude. */
  GAL_ARITHMETIC_OP_MAG_TO_COUNTS,/* Magnitude to counts. */
  GAL_ARITHMETIC_OP_MAG_TO_SB,    /* Magnitude to Surface Brightness. */
  GAL_ARITHMETIC_OP_SB_TO_MAG,    /* Surface Brightness to Magnitude. */
  GAL_ARITHMETIC_OP_COUNTS_TO_SB, /* Counts to Surface Brightness. */
  GAL_ARITHMETIC_OP_SB_TO_COUNTS, /* Surface Brightness to counts. */
  GAL_ARITHMETIC_OP_COUNTS_TO_JY, /* Counts to Janskys (AB zeropoint). */
  GAL_ARITHMETIC_OP_JY_TO_COUNTS, /* Janskys to Counts (AB zeropoint). */
  GAL_ARITHMETIC_OP_MAG_TO_JY,    /* Magnitude to Janskys. */
  GAL_ARITHMETIC_OP_JY_TO_MAG,    /* Janskys to Magnitude. */
  GAL_ARITHMETIC_OP_ZEROPOINT_CHANGE, /* Change the zero point. */
  GAL_ARITHMETIC_OP_COUNTS_TO_NANOMAGGY,/* Counts to SDSS nanomaggies. */
  GAL_ARITHMETIC_OP_NANOMAGGY_TO_COUNTS,/* SDSS nanomaggies to counts. */
  GAL_ARITHMETIC_OP_AU_TO_PC,     /* Astronomical units (AU) to Parsecs (PC). */
  GAL_ARITHMETIC_OP_PC_TO_AU,     /* Parsecs (PC) to Astronomical units (AU). */
  GAL_ARITHMETIC_OP_LY_TO_PC,     /* Astronomical units (AU) to Parsecs (PC). */
  GAL_ARITHMETIC_OP_PC_TO_LY,     /* Parsecs (PC) to Astronomical units (AU). */
  GAL_ARITHMETIC_OP_LY_TO_AU,     /* Light-years to Astronomical units (AU). */
  GAL_ARITHMETIC_OP_AU_TO_LY,     /* Astronomical units (AU) to Light-years. */

  GAL_ARITHMETIC_OP_MINVAL,       /* Minimum value of array.               */
  GAL_ARITHMETIC_OP_MAXVAL,       /* Maximum value of array.               */
  GAL_ARITHMETIC_OP_NUMBERVAL,    /* Number of (non-blank) elements.       */
  GAL_ARITHMETIC_OP_SUMVAL,       /* Sum of (non-blank) elements.          */
  GAL_ARITHMETIC_OP_MEANVAL,      /* Mean value of array.                  */
  GAL_ARITHMETIC_OP_STDVAL,       /* Standard deviation value of array.    */
  GAL_ARITHMETIC_OP_MEDIANVAL,    /* Median value of array.                */
  GAL_ARITHMETIC_OP_UNIQUE,       /* Only return unique elements.          */
  GAL_ARITHMETIC_OP_NOBLANK,      /* Only keep non-blank elements.         */

  GAL_ARITHMETIC_OP_MIN,          /* Minimum per pixel of multiple arrays. */
  GAL_ARITHMETIC_OP_MAX,          /* Maximum per pixel of multiple arrays. */
  GAL_ARITHMETIC_OP_NUMBER,       /* Non-blank number of pixels in arrays. */
  GAL_ARITHMETIC_OP_SUM,          /* Sum per pixel of multiple arrays.     */
  GAL_ARITHMETIC_OP_MEAN,         /* Mean per pixel of multiple arrays.    */
  GAL_ARITHMETIC_OP_STD,          /* STD per pixel of multiple arrays.     */
  GAL_ARITHMETIC_OP_MAD,          /* MAD per pixel of multiple arrays.     */
  GAL_ARITHMETIC_OP_MEDIAN,       /* Median per pixel of multiple arrays.  */
  GAL_ARITHMETIC_OP_QUANTILE,     /* Quantile per pixel of multiple arrays.*/
  GAL_ARITHMETIC_OP_SIGCLIP_MEAN, /* Sigma-clipped mean of multiple arrays.*/
  GAL_ARITHMETIC_OP_SIGCLIP_MEDIAN,/* Sigma-clipped median of mult. arrays.*/
  GAL_ARITHMETIC_OP_SIGCLIP_STD,  /* Sigma-clipped STD of multiple arrays. */
  GAL_ARITHMETIC_OP_SIGCLIP_MAD,  /* Sigma-clipped STD of multiple arrays. */
  GAL_ARITHMETIC_OP_SIGCLIP_MASKFILLED, /* Mask clipped elem. of each in.  */
  GAL_ARITHMETIC_OP_MADCLIP_MEAN,  /* MAD-clipped mean of multiple arrays. */
  GAL_ARITHMETIC_OP_MADCLIP_MEDIAN,/* MAD-clipped median of mult. arrays.  */
  GAL_ARITHMETIC_OP_MADCLIP_STD,   /* MAD-clipped STD of multiple arrays.  */
  GAL_ARITHMETIC_OP_MADCLIP_MAD,   /* MAD-clipped STD of multiple arrays.  */
  GAL_ARITHMETIC_OP_MADCLIP_MASKFILLED, /* Mask clipped elem. of each in.  */

  GAL_ARITHMETIC_OP_MKNOISE_SIGMA,/* Fixed-sigma noise to every element.   */
  GAL_ARITHMETIC_OP_MKNOISE_SIGMA_FROM_MEAN, /* Sigma calculated from mean.*/
  GAL_ARITHMETIC_OP_MKNOISE_POISSON,/* Poission noise on every element.    */
  GAL_ARITHMETIC_OP_MKNOISE_UNIFORM,/* Uniform noise on every element.     */
  GAL_ARITHMETIC_OP_RANDOM_FROM_HIST,/* Randoms from a histogram (uniform).*/
  GAL_ARITHMETIC_OP_RANDOM_FROM_HIST_RAW,/* Randoms from a histogram (raw).*/

  GAL_ARITHMETIC_OP_STITCH,       /* Stitch multiple datasets together.    */
  GAL_ARITHMETIC_OP_TO1D,         /* Make they output into a 1D array.     */
  GAL_ARITHMETIC_OP_TRIM,         /* Trim blank rows or columns in image.  */

  GAL_ARITHMETIC_OP_TO_UINT8,     /* Convert to uint8_t.                   */
  GAL_ARITHMETIC_OP_TO_INT8,      /* Convert to int8_t.                    */
  GAL_ARITHMETIC_OP_TO_UINT16,    /* Convert to uint16_t.                  */
  GAL_ARITHMETIC_OP_TO_INT16,     /* Convert to int16_t.                   */
  GAL_ARITHMETIC_OP_TO_UINT32,    /* Convert to uint32_t.                  */
  GAL_ARITHMETIC_OP_TO_INT32,     /* Convert to int32_t.                   */
  GAL_ARITHMETIC_OP_TO_UINT64,    /* Convert to uint64_t.                  */
  GAL_ARITHMETIC_OP_TO_INT64,     /* Convert to int64_t.                   */
  GAL_ARITHMETIC_OP_TO_FLOAT32,   /* Convert to float32.                   */
  GAL_ARITHMETIC_OP_TO_FLOAT64,   /* Convert to float64.                   */

  GAL_ARITHMETIC_OP_ROTATE_COORD, /* Return input coords after rotation.   */
  GAL_ARITHMETIC_OP_BOX_AROUND_ELLIPSE, /* Width/Height of box over ellipse*/
  GAL_ARITHMETIC_OP_BOX_VERTICES_ON_SPHERE, /* Vert. from center and width */

  /* Meta operators */
  GAL_ARITHMETIC_OP_SIZE,         /* Size of the dataset along an axis.    */
  GAL_ARITHMETIC_OP_MAKENEW,      /* Build a new dataset, containing zeros.*/
  GAL_ARITHMETIC_OP_INDEX,        /* New with the index (counting from 0). */
  GAL_ARITHMETIC_OP_COUNTER,      /* New with the index (counting from 0). */
  GAL_ARITHMETIC_OP_INDEXONLY,    /* New with the index (counting from 0). */
  GAL_ARITHMETIC_OP_COUNTERONLY,  /* New with the index (counting from 1). */
  GAL_ARITHMETIC_OP_SWAP,         /* Swap the top two operands.            */
  GAL_ARITHMETIC_OP_CONSTANT,     /* Make a row with given constant.       */

  /* Pooling operators. */
  GAL_ARITHMETIC_OP_POOLMAX,      /* The pool-max of desired pixels.       */
  GAL_ARITHMETIC_OP_POOLMIN,      /* The pool-min of desired pixels.       */
  GAL_ARITHMETIC_OP_POOLSUM,      /* The pool-sum of desired pixels.       */
  GAL_ARITHMETIC_OP_POOLMEAN,     /* The pool-mean of desired pixels.      */
  GAL_ARITHMETIC_OP_POOLMEDIAN,   /* The pool-median of desired pixels.    */

  /* Celestial coordinate conversions (names are clear, no comment). */
  GAL_ARITHMETIC_OP_EQB1950_TO_EQJ2000,
  GAL_ARITHMETIC_OP_EQB1950_TO_ECB1950,
  GAL_ARITHMETIC_OP_EQB1950_TO_ECJ2000,
  GAL_ARITHMETIC_OP_EQB1950_TO_GALACTIC,
  GAL_ARITHMETIC_OP_EQB1950_TO_SUPERGALACTIC,
  GAL_ARITHMETIC_OP_EQJ2000_TO_EQB1950,
  GAL_ARITHMETIC_OP_EQJ2000_TO_ECB1950,
  GAL_ARITHMETIC_OP_EQJ2000_TO_ECJ2000,
  GAL_ARITHMETIC_OP_EQJ2000_TO_GALACTIC,
  GAL_ARITHMETIC_OP_EQJ2000_TO_SUPERGALACTIC,
  GAL_ARITHMETIC_OP_ECB1950_TO_EQB1950,
  GAL_ARITHMETIC_OP_ECB1950_TO_EQJ2000,
  GAL_ARITHMETIC_OP_ECB1950_TO_ECJ2000,
  GAL_ARITHMETIC_OP_ECB1950_TO_GALACTIC,
  GAL_ARITHMETIC_OP_ECB1950_TO_SUPERGALACTIC,
  GAL_ARITHMETIC_OP_ECJ2000_TO_EQB1950,
  GAL_ARITHMETIC_OP_ECJ2000_TO_EQJ2000,
  GAL_ARITHMETIC_OP_ECJ2000_TO_ECB1950,
  GAL_ARITHMETIC_OP_ECJ2000_TO_GALACTIC,
  GAL_ARITHMETIC_OP_ECJ2000_TO_SUPERGALACTIC,
  GAL_ARITHMETIC_OP_GALACTIC_TO_EQB1950,
  GAL_ARITHMETIC_OP_GALACTIC_TO_EQJ2000,
  GAL_ARITHMETIC_OP_GALACTIC_TO_ECB1950,
  GAL_ARITHMETIC_OP_GALACTIC_TO_ECJ2000,
  GAL_ARITHMETIC_OP_GALACTIC_TO_SUPERGALACTIC,
  GAL_ARITHMETIC_OP_SUPERGALACTIC_TO_EQB1950,
  GAL_ARITHMETIC_OP_SUPERGALACTIC_TO_EQJ2000,
  GAL_ARITHMETIC_OP_SUPERGALACTIC_TO_ECB1950,
  GAL_ARITHMETIC_OP_SUPERGALACTIC_TO_ECJ2000,
  GAL_ARITHMETIC_OP_SUPERGALACTIC_TO_GALACTIC,

  /* Memory operations. */
  GAL_ARITHMETIC_OP_FREE,

  /* Counter for number of operators. */
  GAL_ARITHMETIC_OP_LAST_CODE,    /* Last code of the library operands.    */
};

char *
gal_arithmetic_operator_string(int operator);

int
gal_arithmetic_set_operator(char *string, size_t *num_operands);

gal_data_t *
gal_arithmetic_load_col(char *str, int searchin, int ignorecase,
                        size_t minmapsize, int quietmmap);

gal_data_t *
gal_arithmetic(int operator, size_t numthreads, int flags, ...);



__END_C_DECLS    /* From C++ preparations */

#endif           /* __GAL_ARITHMETIC_H__ */
