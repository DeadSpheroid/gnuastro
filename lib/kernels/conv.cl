#include <CL/cl.h>
#include <stddef.h>

typedef struct gal_data_t
{
  /* Basic information on array of data. */
  void *restrict array; /* Array keeping data elements.               */
  uint8_t type;         /* Type of data (see 'gnuastro/type.h').      */
  size_t ndim;          /* Number of dimensions in the array.         */
  size_t *dsize;        /* Size of array along each dimension.        */
  size_t size;          /* Total number of data-elements.             */
  int quietmmap;        /* ==1: print a notice whem mmap'ing.         */
  char *mmapname;       /* File name of the mmap.                     */
  size_t minmapsize;    /* Minimum number of bytes to mmap the array. */

  /* WCS information. */
  int nwcs;           /* for WCSLIB: no. coord. representations.    */
  struct wcsprm *wcs; /* WCS information for this dataset.          */

  /* Content descriptions. */
  uint8_t flag;  /* Flags: currently 8-bits are enough.        */
  int status;    /* Context specific value for the dataset.    */
  char *name;    /* e.g., EXTNAME, or column, or keyword.      */
  char *unit;    /* Units of the data.                         */
  char *comment; /* A more detailed description of the data.   */

  /* For printing */
  int disp_fmt;       /* See 'gal_table_diplay_formats'.            */
  int disp_width;     /* Width of space to print in ASCII.          */
  int disp_precision; /* Precision to print in ASCII.               */

  cl_context context;
  int svm;

  /* Pointers to other data structures. */
  struct gal_data_t *next;  /* To use it as a linked list if necessary.   */
  struct gal_data_t *block; /* 'gal_data_t' of hosting block, see above.  */
} gal_data_t;

__kernel void
convolution_svm (__global gal_data_t *input_image,
                 __global gal_data_t *kernel_image,
                 __global gal_data_t *output_image)
{
  // /* correct the pointers */
  // image->dsize = image_dsize;
  // image->array = image_array;
  // kernell->dsize = kernell_dsize;
  // kernell->array = kernell_array;

  /* get the image and kernel size */
  int image_height = input_image->dsize[0];
  int image_width = input_image->dsize[1];
  int kernell_height = kernel_image->dsize[0];
  int kernell_width = kernel_image->dsize[1];

  float *image_array = (float *)input_image->array;
  float *kernell_array = (float *)kernel_image->array;
  float *output = (float *)output_image->array;
  /* get the local group id */
  int id = get_global_id (0);
  int row = id / image_width;
  int col = id % image_width;

  if (row < image_height && col < image_width)
    {

      float sum = 0;
      for (int y = -kernell_height / 2; y <= kernell_height / 2; y++)
        {
          for (int x = -kernell_width / 2; x <= kernell_width / 2; x++)
            {
              if (row + y >= 0 && row + y < image_height && col + x >= 0
                  && col + x < image_width)
                {
                  sum += (image_array[(row + y) * image_width + col + x]
                          * kernell_array[(y + kernell_height / 2)
                                              * kernell_width
                                          + x + kernell_width / 2]);
                }
            }
        }
      output[row * image_width + col] = sum;
    }
}
