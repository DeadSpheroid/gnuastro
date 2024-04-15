#define CONV_CORE if (row < image_height && col < image_width)                                                          \
{                                                                                                                       \
    float sum = 0;                                                                                                      \
    for (int y = -1; y <= 1; y++)                                                                                       \
    {                                                                                                                   \
        for (int x = -1; x <= 1; x++)                                                                                   \
        {                                                                                                               \
            if (row + y >= 0 && row + y < image_height && col + x >= 0 && col + x < image_width)                        \
            {                                                                                                           \
                sum += (gpu_image_array[(row + y) * image_width + col + x] * gpu_kernel_array[(y + 1) * 3 + x + 1]);    \
            }                                                                                                           \
        }                                                                                                               \
    }                                                                                                                   \
    gpu_output[row * image_width + col] = sum;                                                                          \
}
typedef struct gal_gpu_data_t
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




__kernel void convolution(
    __global gal_data_t * gpu_image,
    __global gal_data_t * gpu_kernel,
	__global float * gpu_image_array,
    __global size_t * gpu_image_dsize,
    __global float * gpu_kernel_array,
    __global size_t * gpu_kernel_dsize,

	__global float * gpu_output
)
{	
    /* correct the pointers */
    gpu_image->dsize = gpu_image_dsize;
    gpu_image->array = gpu_image_array;
    gpu_kernel->dsize = gpu_kernel_dsize;
    gpu_kernel->array = gpu_kernel_array;

    /* get the image size */
    int image_height = gpu_image_dsize[0];
    int image_width = gpu_image_dsize[1];
   
    /* get the local group id */
    int id = get_global_id(0);
    int row = id/image_width;
    int col = id%image_width;



    // @@ conv_core.c @@ 

    CONV_CORE


    if (row < image_height && col < image_width)
    {
        float sum = 0;
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                if (row + y >= 0 && row + y < image_height && col + x >= 0 && col + x < image_width)
                {
                    sum += (gpu_image_array[(row + y) * image_width + col + x] * gpu_kernel_array[(y + 1) * 3 + x + 1]);
                }
            }
        }
        gpu_output[row * image_width + col] = sum;
    }
}
