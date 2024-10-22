#ifndef __GAL_CL_UTILS_H__
#define __GAL_CL_UTILS_H__

#define CL_TARGET_OPENCL_VERSION 300
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

/* Struct containing params for cl_spinoff_threads */
typedef struct clprm
{
  char               *kernel_path; /* Path to kernel.cl file */
  char               *kernel_name; /* Name of __kernel function */
  char             *compiler_opts; /* Additional compiler options */
  cl_device_id          device_id; /* Device to be targeted */
  cl_context              context; /* Context of OpenCL in use */
  int             num_kernel_args; /* Number of total kernel arguments */
  int                num_svm_args; /* Number of SVM args*/
  void              **kernel_args; /* Array of pointers to kernel args */
  size_t       *kernel_args_sizes; /* Sizes of non SVM args */
  int          num_extra_svm_args; /* Number of implicit SVM args */
  void           **extra_svm_args; /* Array of pointers to these args */
  int                    work_dim; /* Work dimension of job - 1,2,3 */
  size_t        *global_work_size; /* Array of global sizes of size work_dim */
  size_t         *local_work_size; /* Array of local sizes of size work_dim */
} clprm;


/*********************************************************************/
/*************            Initialization           *******************/
/*********************************************************************/
void
gal_cl_init (cl_device_type device_type, cl_context *context,
                cl_platform_id *platform_id, cl_device_id *device_id);

cl_kernel gal_cl_kernel_create (char *kernel_name, char *function_name,
                                char *compiler_opts, cl_device_id device_id,
                                cl_context context,
                                cl_command_queue command_queue);

cl_command_queue
gal_cl_create_command_queue (cl_context context,
                                cl_device_id device_id);

void
gal_cl_finish_queue(cl_command_queue command_queue);

void
gal_cl_free_queue(cl_command_queue command_queue);





/*********************************************************************/
/*************            Query Devices            *******************/
/*********************************************************************/
char *
gal_cl_get_device_name (cl_device_id device_id);

char *
gal_cl_get_platform_name (cl_platform_id platform_id);





/*********************************************************************/
/*************          Buffer Operations          *******************/
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
gal_cl_copy_from_device (gal_data_t *out, cl_mem *output_mem_obj,
                          cl_command_queue command_queue);





/*********************************************************************/
/*************            Map Operations           *******************/
/*********************************************************************/
cl_mem
gal_cl_create_buffer_from_array (void *array, size_t size,
                                        cl_context context,
                                        cl_device_info device_type);

void
gal_cl_write_to_device (cl_mem *buffer, void *mapped_ptr,
                             cl_command_queue command_queue);

void *
gal_cl_read_to_host (cl_mem buffer, size_t size,
                           cl_command_queue command_queue);





/*********************************************************************/
/*************                 Mem                 *******************/
/*********************************************************************/
gal_data_t *
gal_cl_alloc_svm (size_t size_of_array, size_t size_of_dsize,
                  cl_context context, cl_command_queue command_queue);

void
gal_cl_map_svm_to_cpu (cl_context context, cl_command_queue command_queue, 
                void *svm_ptr, size_t size);

void
gal_cl_unmap_svm_to_gpu (cl_context context, cl_command_queue command_queue,
                  void *svm_ptr);

void
gal_cl_copy_to_svm (gal_data_t *input, gal_data_t *svm);

void
gal_cl_write_data_to_gpu (cl_context context,
                            cl_command_queue command_queue,
                            gal_data_t *input);

void
gal_cl_read_data_to_cpu (cl_context context,
                          cl_command_queue command_queue,
                          gal_data_t *input);

gal_data_t *
gal_cl_copy_data_to_gpu(cl_context context,
                          cl_command_queue command_queue,
                          gal_data_t *input);





/*********************************************************************/
/*************         Exec On OpenCL Device       *******************/
/*********************************************************************/
double
gal_cl_threads_spinoff (clprm *sprm);
#endif