#ifndef __GAL_CL_UTILS_H__
#define __GAL_CL_UTILS_H__

// #define CL_TARGET_OPENCL_VERSION 300
#include <gnuastro/data.h>

#include <CL/cl.h>
#define MAX_SOURCE_SIZE (0x100000)

#ifndef IN_GNUASTRO_BUILD
#include <gnuastro/config.h>
#endif

#undef __BEGIN_C_DECLS
#undef __END_C_DECLS
#ifdef __cplusplus
#define __BEGIN_C_DECLS                                                       \
  extern "C"                                                                  \
  {
#define __END_C_DECLS }
#else
#define __BEGIN_C_DECLS /* empty */
#define __END_C_DECLS   /* empty */
#endif

/*********************************************************************/
/*************            initialization           *******************/
/*********************************************************************/
void 
gal_cl_init (cl_device_type device_type, cl_context *context,
                  cl_platform_id *platform_id, cl_device_id *device_id);

cl_kernel 
gal_cl_kernel_create (char *kernel_name, char *function_name,
                                cl_device_id device_id, cl_context context,
                                cl_command_queue *command_queue);




/*********************************************************************/
/*************            retrieve info            *******************/
/*********************************************************************/
void 
gal_cl_get_device_name(cl_device_id device_id, char *device_name);

void
gal_cl_get_platform_name(cl_platform_id platform_id, char *platform_name);





/*********************************************************************/
/*************            data transfer            *******************/
/*********************************************************************/
void 
gal_cl_copy_array_to_device (gal_data_t *in, cl_mem *input_mem_obj,
                                cl_context context,
                                cl_command_queue command_queue, int device);

void 
gal_cl_copy_dsize_to_device (gal_data_t *in, cl_mem *input_mem_obj,
                                cl_context context,
                                cl_command_queue command_queue, int device);

void 
gal_cl_copy_struct_to_device (gal_data_t *in, cl_mem *input_mem_obj,
                                cl_context context,
                                cl_command_queue command_queue, int device);

void 
gal_cl_copy_from_device (gal_data_t *out, cl_mem *output_mem_obj,
                            cl_command_queue command_queue);

#endif