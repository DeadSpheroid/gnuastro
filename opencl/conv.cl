typedef struct __attribute__((packed)) gal_gpu_data_t
{
  /* Basic information on array of data. */
  uint8             type;  /* Type of data (see 'gnuastro/type.h').         */
  size_t              ndim;  /* Number of dimensions in the array.            */
  size_t              size;  /* Total number of data-elements.                */
  int            quietmmap;  /* ==1: print a notice whem mmap'ing.            */
  size_t        minmapsize;  /* Minimum number of bytes to mmap the array.    */

  /* WCS information. */
  int                 nwcs;  /* for WCSLIB: no. coord. representations.       */

  /* Content descriptions. */
  uint8             flag;  /* Flags: currently 8-bits are enough.           */
  int               status;  /* Context specific value for the dataset.       */

  /* For printing */
  int             disp_fmt;  /* See 'gal_table_diplay_formats'.               */
  int           disp_width;  /* Width of space to print in ASCII.             */
  int       disp_precision;  /* Precision to print in ASCII.                  */
} gal_gpu_data_t;

typedef struct gal_data_t
{
  /* Basic information on array of data. */
  void     *restrict array;  /* Array keeping data elements.               */
  uint8             type;  /* Type of data (see 'gnuastro/type.h').      */
  size_t              ndim;  /* Number of dimensions in the array.         */
  size_t            *dsize;  /* Size of array along each dimension.        */
  size_t              size;  /* Total number of data-elements.             */
  int            quietmmap;  /* ==1: print a notice whem mmap'ing.         */
  char           *mmapname;  /* File name of the mmap.                     */
  size_t        minmapsize;  /* Minimum number of bytes to mmap the array. */

  /* WCS information. */
  int                 nwcs;  /* for WCSLIB: no. coord. representations.    */
  struct wcsprm       *wcs;  /* WCS information for this dataset.          */

  /* Content descriptions. */
  uint8             flag;  /* Flags: currently 8-bits are enough.        */
  int               status;  /* Context specific value for the dataset.    */
  char               *name;  /* e.g., EXTNAME, or column, or keyword.      */
  char               *unit;  /* Units of the data.                         */
  char            *comment;  /* A more detailed description of the data.   */

  /* For printing */
  int             disp_fmt;  /* See 'gal_table_diplay_formats'.            */
  int           disp_width;  /* Width of space to print in ASCII.          */
  int       disp_precision;  /* Precision to print in ASCII.               */

  /* Pointers to other data structures. */
  struct gal_data_t  *next;  /* To use it as a linked list if necessary.   */
  struct gal_data_t *block;  /* 'gal_data_t' of hosting block, see above.  */
} gal_data_t;

#include "conv_core.h"


__kernel void convolution(
    __global gal_data_t * image,
    __global gal_data_t * kernell,
	__global float * image_array,
    __global size_t * image_dsize,
    __global float * kernell_array,
    __global size_t * kernell_dsize,

	__global float * output
)
{	
    // /* correct the pointers */
    // image->dsize = image_dsize;
    // image->array = image_array;
    // kernell->dsize = kernell_dsize;
    // kernell->array = kernell_array;

    /* get the image and kernel size */
    int image_height = image_dsize[0];
    int image_width = image_dsize[1];
    int kernell_height = kernell_dsize[0];
    int kernell_width = kernell_dsize[1];
   
    /* get the local group id */
    int id = get_global_id(0);
    int row = id/image_width;
    int col = id%image_width;



    // @@ conv_core.c @@ 

    CONV_CORE
}
