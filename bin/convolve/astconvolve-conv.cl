typedef struct  __attribute__((aligned(4))) gal_data_t
{
  /* Basic information on array of data. */
  void *restrict array; /* Array keeping data elements.               */
  uchar type;         /* Type of data (see 'gnuastro/type.h').      */
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
  uchar flag;  /* Flags: currently 8-bits are enough.        */
  int status;    /* Context specific value for the dataset.    */
  char *name;    /* e.g., EXTNAME, or column, or keyword.      */
  char *unit;    /* Units of the data.                         */
  char *comment; /* A more detailed description of the data.   */

  /* For printing */
  int disp_fmt;       /* See 'gal_table_diplay_formats'.            */
  int disp_width;     /* Width of space to print in ASCII.          */
  int disp_precision; /* Precision to print in ASCII.               */

  void *context;
  int svm;

  /* Pointers to other data structures. */
  struct gal_data_t *next;  /* To use it as a linked list if necessary.   */
  struct gal_data_t *block; /* 'gal_data_t' of hosting block, see above.  */
} gal_data_t;

// Defines a function-like macro to check for the channels
// Expands to either a function call or to '1'
#ifndef IS_SAME_CH
bool is_same_channel(int x1, int x2, int y1, int y2, int tile_w, int tile_h)
{
  int tile_x_1 = x1 / tile_w;
  int tile_x_2 = x2 / tile_w;
  int tile_y_1 = y1 / tile_h;
  int tile_y_2 = y2 / tile_h;

  return (tile_x_1 == tile_x_2) && (tile_y_1 == tile_y_2);
}
#define IS_SAME_CH(x1, x2, y1, y2, tile_w, tile_h) is_same_channel(x1, x2, y1, y2, tile_w, tile_h)
#endif

__kernel void
convolution (__global gal_data_t *input_image,
              __global gal_data_t *output_image,
              __constant float *kernell_array,
              int tile_w,
              int tile_h)
{
  int image_height = input_image->dsize[0];
  int image_width = input_image->dsize[1];
  int kernell_height = KERNEL_SIZE;
  int kernell_width = KERNEL_SIZE;

  float *image_array = (float *)input_image->array;
  float *output = (float *)output_image->array;

  __local float shared_memory[16 + KERNEL_RADIUS * 2][16 + KERNEL_RADIUS * 2];

  int gid0 = get_global_id (0);
  int gid1 = get_global_id(1);
  int lid0 = get_local_id(0);
  int lid1 = get_local_id(1);
  int wid0 = get_group_id(0);
  int wid1 = get_group_id(1);
  int gLoc = gid0 * image_width + gid1;

  int row = gid0;
  int col = gid1;
  float w = 0.0;

  // TOP LEFT
  int x = row - KERNEL_RADIUS;
  int y = col - KERNEL_RADIUS;
  if ( x < 0 || y < 0 || x >= image_height || y >= image_width)
    shared_memory[lid0][lid1] = NAN;
  else
    shared_memory[lid0][lid1] = image_array[x * image_width + y];

  // TOP RIGHT
  x = row + KERNEL_RADIUS;
  y = col - KERNEL_RADIUS;
  if ( x < 0 || y < 0 || x >= image_height || y >= image_width)
    shared_memory[lid0 + KERNEL_RADIUS * 2][lid1] = NAN;
  else
    shared_memory[lid0 + KERNEL_RADIUS * 2][lid1] = image_array[x * image_width + y];

  // BOTTOM LEFT
  x = row - KERNEL_RADIUS;
  y = col + KERNEL_RADIUS;
  if ( x < 0 || y < 0 || x >= image_height || y >= image_width)
    shared_memory[lid0][lid1 + KERNEL_RADIUS * 2] = NAN;
  else
    shared_memory[lid0][lid1 + KERNEL_RADIUS * 2] = image_array[x * image_width + y];


  // BOTTOM RIGHT
  x = row + KERNEL_RADIUS;
  y = col + KERNEL_RADIUS;
  if ( x < 0 || y < 0 || x >= image_height || y >= image_width)
    shared_memory[lid0 + KERNEL_RADIUS * 2][lid1 + KERNEL_RADIUS * 2] = NAN;
  else
    shared_memory[lid0 + KERNEL_RADIUS * 2][lid1 + KERNEL_RADIUS * 2] = image_array[x * image_width + y]; 
  
  barrier(CLK_LOCAL_MEM_FENCE);

  // printf("%d,%d,%d,%d,%d,%d\n", gid0, gid1, lid0, lid1, wid0, wid1);

  
  float sum = 0;
  x = KERNEL_RADIUS + lid0;
  y = KERNEL_RADIUS + lid1;
  if(row < image_height && col < image_width)
  {
    for (int i = -KERNEL_RADIUS; i <= KERNEL_RADIUS; i++)
      for (int j = -KERNEL_RADIUS; j <= KERNEL_RADIUS; j++)
      {
        if(IS_SAME_CH(gid0, gid0 + i, gid1, gid1 + j, tile_w, tile_h) && !isnan(shared_memory[x+i][y+j]))
        {
          sum += shared_memory[x + i][y + j] * kernell_array[(KERNEL_RADIUS + i) * kernell_width + j + KERNEL_RADIUS];

#ifndef NO_EDGE_CORRECT
          w += kernell_array[(KERNEL_RADIUS + i) * kernell_width + j + KERNEL_RADIUS];
#endif

        }
      }

#ifdef NO_EDGE_CORRECT
    output[gLoc] = sum;
#else
    output[gLoc] = sum / w;
#endif
  }

  barrier(CLK_GLOBAL_MEM_FENCE);
}
