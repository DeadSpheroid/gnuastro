/* Main data structure.

   mmap (keep data outside of RAM)
   -------------------------------

     'mmap' is C's facility to keep the data on the HDD/SSD instead of
     inside the RAM. This can be very useful for large data sets which can
     be very memory intensive. Ofcourse, the speed of operation greatly
     decreases when defining not using RAM, but that is worth it because
     increasing RAM might not be possible. So in 'gal_data_t' when the size
     of the requested array (in bytes) is larger than a certain minimum
     size (in bytes), Gnuastro won't write the array in memory but on
     non-volatile memory (like HDDs and SSDs) as a file in the
     './.gnuastro' directory of the directory it was run from.

         - If mmapname==NULL, then the array is allocated (using malloc, in
           the RAM), otherwise its is mmap'd (is actually a file on the
           ssd/hdd).

         - minmapsize is stored in the data structure to allow any
           derivative data structures to follow the same number and decide
           if they should be mmap'd or allocated.

         - 'minmapsize' ==  0: array is definitely mmap'd.

         - 'minmapsize' == -1: array is definitely in RAM.


   block (work with only a subset of the data)
   -------------------------------------------

     In many contexts, it is desirable to slice the data set into subsets
     or tiles, not necessarily overlapping. In such a way that you can work
     on each independently. One option is to copy that region to a separate
     allocated space, but in many contexts this isn't necessary and infact
     can be a big burden on CPU/Memory usage. The 'block' pointer in
     'gal_data_t' is defined for situations where allocation is not
     necessary: you just want to read the data or write to it
     independently, or in coordination with, other regions of the
     dataset. Added with parallel processing, this can greatly improve the
     time/memory consumption.

     See the figure below for example: assume the larger box is a
     contiguous block of memory that you are interpretting as a 2D
     array. But you only want to work on the region marked by the smaller
     box.

                    tile->block = larger
                ---------------------------
                |                         |
                |           tile          |
                |        ----------       |
                |        |        |       |
                |        |_       |       |
                |        |*|      |       |
                |        ----------       |
                |_                        |
                |*|                       |
                ---------------------------

     To use 'gal_data_t's block concept, you allocate a 'gal_data_t *tile'
     which is initialized with the pointer to the first element in the
     sub-array (as its 'array' argument). Note that this is not necessarily
     the first element in the larger array. You can set the size of the
     tile along with the initialization as you please. Recall that, when
     given a non-NULL pointer as 'array', 'gal_data_initialize' (and thus
     'gal_data_alloc') do not allocate any space and just uses the given
     pointer for the new 'array' element of the 'gal_data_t'. So your
     'tile' data structure will not be pointing to a separately allocated
     space.

     After the allocation is done, you just point 'tile->block' to the
     'gal_data_t' which hosts the larger array. Afterwards, the programs
     that take in the 'sub' array can check the 'block' pointer to see how
     to deal with dimensions and increments (strides) while working on the
     'sub' datastructure. For example to increment along the vertical
     dimension, the program must look at index i+W (where 'W' is the width
     of the larger array, not the tile).

     Since the block structure is defined as a pointer, arbitrary levels of
     tesselation/griding are possible. Therefore, just like a linked list,
     it is important to have the 'block' pointer of the largest dataset set
     to NULL. Normally, you won't have to worry about this, because
     'gal_data_initialize' (and thus 'gal_data_alloc') will set it to NULL
     by default (just remember not to change it). You can then only change
     the 'block' element for the tiles you define over the allocated space.

     In Gnuastro, there is a separate library for tiling operations called
     'tile.h', see the functions there for tools to effectively use this
     feature. This approach to dealing with parts of a larger block was
     inspired from the way the GNU Scientific Library does it in the
     "Vectors and Matrices" chapter.
*/

typedef struct gal_data_t
{
  /* Basic information on array of data. */
  void     *restrict array;  /* Array keeping data elements.               */
  uint8_t             type;  /* Type of data (see 'gnuastro/type.h').      */
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
  uint8_t             flag;  /* Flags: currently 8-bits are enough.        */
  int               status;  /* Context specific value for the dataset.    */
  char               *name;  /* e.g., EXTNAME, or column, or keyword.      */
  char               *unit;  /* Units of the data.                         */
  char            *comment;  /* A more detailed description of the data.   */

  /* For printing */
  int             disp_fmt;  /* See 'gal_table_diplay_formats'.            */
  int           disp_width;  /* Width of space to print in ASCII.          */
  int       disp_precision;  /* Precision to print in ASCII.               */

#if GAL_CONFIG_HAVE_OPENCL
  cl_context       context;
  int                  svm;
#endif


  /* Pointers to other data structures. */
  struct gal_data_t  *next;  /* To use it as a linked list if necessary.   */
  struct gal_data_t *block;  /* 'gal_data_t' of hosting block, see above.  */
} gal_data_t;