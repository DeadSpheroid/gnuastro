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
void gal_cl_init (cl_device_type device_type, cl_context *context,
                  cl_platform_id *platform_id, cl_device_id *device_id);

cl_kernel gal_cl_kernel_create (char *kernel_name, char *function_name,
                                cl_device_id device_id, cl_context context,
                                cl_command_queue command_queue);

cl_command_queue gal_cl_create_command_queue (cl_context context,
                                              cl_device_id device_id);

/*********************************************************************/
/*************            retrieve info            *******************/
/*********************************************************************/
char *gal_cl_get_device_name (cl_device_id device_id);

char *gal_cl_get_platform_name (cl_platform_id platform_id);





/*********************************************************************/
/*************            data transfer            *******************/
/*********************************************************************/
void gal_cl_copy_array_to_device (gal_data_t *in, cl_mem *input_mem_obj,
                                  cl_context context,
                                  cl_command_queue command_queue, int device);

void gal_cl_copy_dsize_to_device (gal_data_t *in, cl_mem *input_mem_obj,
                                  cl_context context,
                                  cl_command_queue command_queue, int device);

void gal_cl_copy_from_device (gal_data_t *out, cl_mem *output_mem_obj,
                              cl_command_queue command_queue);





/*********************************************************************/
/*************                Buffer               *******************/
/*********************************************************************/
cl_mem gal_cl_create_buffer_from_array (void *array, size_t size,
                                        cl_context context,
                                        cl_device_info device_type);

void gal_cl_write_to_device (cl_mem *buffer, void *mapped_ptr,
                             cl_command_queue command_queue);

void *gal_cl_read_to_host (cl_mem *buffer, size_t size,
                           cl_command_queue command_queue);





/*********************************************************************/
/*************                 SVM                 *******************/
/*********************************************************************/
gal_data_t *gal_cl_alloc_svm (size_t size_of_array, size_t size_of_dsize,
                              cl_context context, cl_device_id device_id);

void gal_cl_map_svm (cl_context context, cl_device_id device_id, void *svm_ptr,
                     size_t size);

void gal_cl_copy_to_svm (gal_data_t *input, gal_data_t *svm);






/*********************************************************************/
/*************         Exec On OpenCL Device       *******************/
/*********************************************************************/
void gal_cl_threads_spinoff (char *kernel_path, char *kernel_name,
                             cl_device_id device_id, cl_context context,
                             int num_kernel_args, void *kernel_args[],
                             int num_extra_svm_args, void *extra_svm_args[],
                             int work_dim, size_t *global_work_size,
                             size_t *local_work_size);
#endif