#include <stdio.h>
#include <gnuastro/data.h>
#include <CL/cl.h>

#define MAX_SOURCE_SIZE (0x100000)

typedef struct gal_gpu_data_t
{
  /* Basic information on array of data. */
  uint8_t             type;  /* Type of data (see 'gnuastro/type.h').         */
  size_t              ndim;  /* Number of dimensions in the array.            */
  size_t              size;  /* Total number of data-elements.                */
  int            quietmmap;  /* ==1: print a notice whem mmap'ing.            */
  size_t        minmapsize;  /* Minimum number of bytes to mmap the array.    */

  /* WCS information. */
  int                 nwcs;  /* for WCSLIB: no. coord. representations.       */

  /* Content descriptions. */
  uint8_t             flag;  /* Flags: currently 8-bits are enough.           */
  int               status;  /* Context specific value for the dataset.       */

  /* For printing */
  int             disp_fmt;  /* See 'gal_table_diplay_formats'.               */
  int           disp_width;  /* Width of space to print in ASCII.             */
  int       disp_precision;  /* Precision to print in ASCII.                  */



  /* Pointers to other data structures. */
  
  /* void     *restrict array;  /* Array keeping data elements.               */
  /* size_t            *dsize;  /* Size of array along each dimension.        */
  /* char           *mmapname;     File name of the mmap.                     */

  /* struct wcsprm       *wcs;  /* WCS information for this dataset.          */
  
  /* char            *comment;  /* A more detailed description of the data.   */
  /* char               *unit;  /* Units of the data.                         */
  /* char               *name;  /* e.g., EXTNAME, or column, or keyword.      */
  
  /* struct gal_data_t  *next;  /* To use it as a linked list if necessary.   */
  /* struct gal_data_t *block;  /* 'gal_data_t' of hosting block, see above.  */
} gal_gpu_data_t;




/*********************************************************************/
/*************            initialization           *******************/
/*********************************************************************/
cl_kernel 
gal_gpu_kernel_create(char *name, char *function_name, char *core_name, 
                        cl_device_id device_id, cl_context *context, 
                        cl_command_queue *command_queue);





/*********************************************************************/
/*************            data transfer            *******************/
/*********************************************************************/
cl_mem
gal_gpu_copy_array_to_device(gal_data_t * in, cl_mem *input_mem_obj, cl_context context, 
                        cl_command_queue command_queue);

cl_mem
gal_gpu_copy_dsize_to_device(gal_data_t * in, cl_mem *input_mem_obj, cl_context context, 
                        cl_command_queue command_queue);

cl_mem
gal_gpu_copy_struct_to_device(gal_data_t *in, cl_mem *input_mem_obj, cl_context context, 
                        cl_command_queue command_queue);

void
gal_gpu_copy_from_device(gal_data_t *out, cl_mem *output_mem_obj, cl_command_queue command_queue);