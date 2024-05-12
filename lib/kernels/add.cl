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

__kernel void
add(__global gal_data_t *input_image1,
    __global gal_data_t *input_image2,
    __global gal_data_t *output_image)
{
    int id = get_global_id(0);

    float *input_array1 = (float *)input_image1->array;
    float *input_array2 = (float *)input_image2->array;
    float *output_array = (float *)output_image->array;

    output_array[id] = input_array1[id] + input_array2[id];
    return;
}