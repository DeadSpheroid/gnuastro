#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <gnuastro/cl-utils.h>

/*********************************************************************/
/*************             Initialization          *******************/
/*********************************************************************/
/* Initializes the context and command queue with one of the available
   devices, selected using device_id. Then creates the kernel object
   using the created context for a specific funtion (named function_name)  */
void
gal_cl_init (cl_device_type device_type, cl_context *context,
             cl_platform_id *platform_id, cl_device_id *device_id)
{
  cl_int ret = 0;
  cl_uint num_platforms = -1;

  ret = clGetPlatformIDs (0, NULL, &num_platforms);
  if(ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Unable to fetch number of available platforms",
           __func__, ret);

  cl_platform_id *platforms
      = (cl_platform_id *)malloc (num_platforms * sizeof (cl_platform_id));
  if(platforms==NULL)
    error (EXIT_FAILURE, ENOMEM, "%s: %zu bytes for cl_platform_id[]",
           __func__, num_platforms * sizeof (cl_platform_id));

  ret = clGetPlatformIDs (num_platforms, platforms, NULL);
  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENODEV,
           "%s: CL Error %d: Unable to retrieve available OpenCL platforms",
           __func__, ret);

  uint8_t i;
  for (i = 0; i < num_platforms; i++)
    {
      ret = clGetDeviceIDs (platforms[i], device_type, 1, device_id, NULL);

      if (ret == CL_SUCCESS)
        break;
    }

  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENODEV,
           "%s: CL Error %d: Unable to retrieve requested OpenCL devices",
           __func__, ret);

  *platform_id = platforms[i];

  cl_context_properties properties[]
      = { CL_CONTEXT_PLATFORM, (cl_context_properties)*platform_id, 0 };

  *context
      = clCreateContextFromType (properties, device_type, NULL, NULL, &ret);

  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Unable to create OpenCL context", __func__, ret);

  free (platforms);
}

cl_kernel
gal_cl_kernel_create (char *kernel_name, char *function_name,
                      char *compiler_opts, cl_device_id device_id,
                      cl_context context, cl_command_queue command_queue)
{
  cl_int ret = 0;
  cl_program program;

  FILE *kernelFile = fopen (kernel_name, "r");

  char *result = (char *)malloc (MAX_SOURCE_SIZE);

  size_t kernel_size = fread (result, 1, MAX_SOURCE_SIZE, kernelFile);
  fclose (kernelFile);

  program = clCreateProgramWithSource (context, 1, (const char **)&result,
                                       (const size_t *)&kernel_size, &ret);

  // printf("DEBUG: %s\n", compiler_opts);
  ret = clBuildProgram (program, 1, &device_id,
                        compiler_opts,
                        NULL, NULL);

  if (ret != CL_SUCCESS)
    {
      cl_int ret_clone = ret;
      size_t len;
      char build_opts[1024], build_log[2048];
      cl_build_status bldstatus;

      ret = clGetProgramBuildInfo (program, device_id, CL_PROGRAM_BUILD_STATUS,
                                   sizeof (bldstatus), (void *)&bldstatus,
                                   &len);
      if(ret != CL_SUCCESS)
        fprintf (stderr,
                 "#####\nWarning from '%s'\n#####\n OpenCL failed to "
                 "build the program and querying the build for errors "
                 "is leading to more errors\nThe build status printed "
                 "hereafter may therefore not be accurate\n"
                 "CL Error %d: Failed to query build status of failed program "
                 "build\n",
                 __func__, ret);
      ret = clGetProgramBuildInfo (program, device_id,
                                   CL_PROGRAM_BUILD_OPTIONS,
                                   sizeof (build_opts), build_opts, &len);
      if(ret != CL_SUCCESS)
        fprintf (stderr,
                 "#####\nWarning from '%s'\n#####\n OpenCL failed to "
                 "build the program and querying the build for errors "
                 "has yielded more errors\nThe build options printed "
                 "hereafter may therefore not be accurate\n"
                 "CL Error %d: Failed to query build status of failed program "
                 "build\n",
                 __func__, ret);

      ret = clGetProgramBuildInfo (program, device_id, CL_PROGRAM_BUILD_LOG,
                                   sizeof (build_log), build_log, &len);
      if(ret != CL_SUCCESS)
        fprintf (stderr,
                  "#####\nWarning from '%s'\n#####\n OpenCL failed to "
                  "build the program and querying the build for errors "
                  "is leading to more errors\nThe build log printed "
                  "hereafter may therefore not be accurate\n"
                  "CL Error %d: Failed to query build status of failed program "
                  "build\n",
                  __func__, ret);

      error (EXIT_FAILURE, ENOTRECOVERABLE,
             "%s: CL Error %d: Failed to build OpenCL program kernel\n"
             "Build Status: %d\n"
             "Build Options: %s\n"
             "#####Build Log#####\n%s",
             __func__, ret_clone, bldstatus, build_opts, build_log);
    }

  return clCreateKernel (program, function_name, &ret);
  ;
}

cl_command_queue
gal_cl_create_command_queue (cl_context context, cl_device_id device_id)
{
  cl_int ret = 0;
  cl_queue_properties properties[]
      = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };

  cl_command_queue command_queue = clCreateCommandQueueWithProperties (
      context, device_id, properties, &ret);
  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Failed to create command queue with requested "
           "properties",
           __func__, ret);
  return command_queue;
}

void
gal_cl_finish_queue(cl_command_queue command_queue)
{
  cl_int ret = 0;

  ret = clFlush(command_queue);
  if(ret != CL_SUCCESS)
    error(EXIT_FAILURE, ENOTRECOVERABLE,
          "%s: CL Error %d: Failed to flush commands to device",
          __func__, ret);

  ret = clFinish(command_queue);
  if(ret != CL_SUCCESS)
    error(EXIT_FAILURE, ENOTRECOVERABLE,
          "%s: CL Error %d: Failed to finish commands enqueued",
          __func__, ret);
}

void
gal_cl_free_queue(cl_command_queue command_queue)
{
  cl_int ret = 0;

  ret = clReleaseCommandQueue(command_queue);
  if(ret != CL_SUCCESS)
    error(EXIT_FAILURE, ENOTRECOVERABLE,
          "%s: CL Error %d: Failed to release command queue",
          __func__, ret);
}




/*********************************************************************/
/*************            Query Devices            *******************/
/*********************************************************************/
char *
gal_cl_get_device_name (cl_device_id device_id)
{
  cl_int ret = 0;
  char *device_name = (char *)malloc(256);
  ret = clGetDeviceInfo (device_id, CL_DEVICE_NAME, 256,
                                 (void *)device_name, NULL);

  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, EINVAL,
           "%s: CL Error %d: Failed to query device name", __func__, ret);

  return device_name;
}

char *
gal_cl_get_platform_name (cl_platform_id platform_id)
{
  cl_int ret = 0;
  char *platform_name = (char *)malloc(256);
  ret = clGetPlatformInfo (platform_id, CL_PLATFORM_NAME, 256,
                                   (void *)platform_name, NULL);

  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, EINVAL,
           "%s: CL Error %d: Failed to query platform name", __func__, ret);

  return platform_name;
}





/*********************************************************************/
/*************          Buffer Operations          *******************/
/*********************************************************************/
void
gal_cl_copy_array_to_device (gal_data_t *in, cl_mem *input_mem_obj,
                             cl_context context,
                             cl_command_queue command_queue, int device)
{
  int ret = 0;
  if (device == 2)
    {
      *input_mem_obj = clCreateBuffer (
          context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
          in->size * gal_type_sizeof (in->type), (void *)in->array, &ret);

      if (ret != CL_SUCCESS)
        error (EXIT_FAILURE, ENOTRECOVERABLE,
               "%s: CL Error %d: Failed to create OpenCL Buffer", __func__,
               ret);
    }
  else
    {
      *input_mem_obj
          = clCreateBuffer (context, CL_MEM_READ_ONLY,
                            in->size * gal_type_sizeof (in->type), NULL, &ret);

      if (ret != CL_SUCCESS)
        error (EXIT_FAILURE, ENOTRECOVERABLE,
               "%s: CL Error %d: Failed to create OpenCL Buffer", __func__,
               ret);

      ret = clEnqueueWriteBuffer (command_queue, *input_mem_obj, CL_TRUE, 0,
                                  in->size * gal_type_sizeof (in->type),
                                  (float *)in->array, 0, NULL, NULL);

      if (ret != CL_SUCCESS)
        error (EXIT_FAILURE, ENOTRECOVERABLE,
               "%s: CL Error %d: Failed to write OpenCL Buffer to device",
               __func__, ret);
    }
}

void
gal_cl_copy_dsize_to_device (gal_data_t *in, cl_mem *input_mem_obj,
                             cl_context context,
                             cl_command_queue command_queue, int device)
{
  int ret = 0;
  if (device == 2)
    {
      *input_mem_obj
          = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                            3 * sizeof (size_t), (void *)in->dsize, &ret);

      if (ret != CL_SUCCESS)
        error (EXIT_FAILURE, ENOTRECOVERABLE,
               "%s: CL Error %d: Failed to create OpenCL Buffer", __func__,
               ret);
    }
  else
    {
      *input_mem_obj = clCreateBuffer (context, CL_MEM_READ_ONLY,
                                       3 * sizeof (size_t), NULL, &ret);

      if (ret != CL_SUCCESS)
        error (EXIT_FAILURE, ENOTRECOVERABLE,
               "%s: CL Error %d: Failed to create OpenCL Buffer", __func__,
               ret);

      ret = clEnqueueWriteBuffer (command_queue, *input_mem_obj, CL_TRUE, 0,
                                  3 * sizeof (size_t), in->dsize, 0, NULL,
                                  NULL);

      if (ret != CL_SUCCESS)
        error (EXIT_FAILURE, ENOTRECOVERABLE,
               "%s: CL Error %d: Failed to write OpenCL Buffer to device",
               __func__, ret);
    }
}

void
gal_cl_copy_from_device (gal_data_t *out, cl_mem *output_mem_obj,
                         cl_command_queue command_queue)
{
  int ret = clEnqueueReadBuffer (command_queue, *output_mem_obj, CL_TRUE, 0,
                                 out->size * gal_type_sizeof (out->type),
                                 (float *)out->array, 0, NULL, NULL);

  if (ret != CL_SUCCESS)
        error (EXIT_FAILURE, ENOTRECOVERABLE,
               "%s: CL Error %d: Failed to read OpenCL Buffer from device",
               __func__, ret);
}





/*********************************************************************/
/*************            Map Operations           *******************/
/*********************************************************************/
cl_mem
gal_cl_create_buffer_from_array (void *array, size_t size, cl_context context,
                                 cl_device_info device_type)
{
  cl_int ret = 0;

  cl_mem buffer
      = clCreateBuffer (context,
                        CL_MEM_READ_WRITE | (device_type == CL_DEVICE_TYPE_CPU)
                            ? CL_MEM_USE_HOST_PTR
                            : 0,
                        size, (void *)array, &ret);

  if (ret != CL_SUCCESS)
        error (EXIT_FAILURE, ENOTRECOVERABLE,
               "%s: CL Error %d: Failed to create OpenCL Buffer",
               __func__, ret);
  return buffer;
}

void
gal_cl_write_to_device (cl_mem *buffer, void *mapped_ptr,
                        cl_command_queue command_queue)
{
  cl_int ret = 0;
  clEnqueueUnmapMemObject (command_queue, *buffer, mapped_ptr, 0, NULL, NULL);
  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Failed to unmap OpenCL buffer to device",
           __func__, ret);
}

void *
gal_cl_read_to_host (cl_mem buffer, size_t size,
                     cl_command_queue command_queue)
{
  cl_int ret = 0;

  void *mapped_ptr = clEnqueueMapBuffer (command_queue, buffer, CL_TRUE,
                                         CL_MAP_READ | CL_MAP_WRITE, 0, size,
                                         0, NULL, NULL, &ret);

  if (mapped_ptr == NULL || ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Failed to map %zu bytes of OpenCL Buffer to host",
           __func__, ret, size);
  return mapped_ptr;
}





/*********************************************************************/
/*************                 SVM                 *******************/
/*********************************************************************/
gal_data_t *
gal_cl_alloc_svm (size_t size_of_array, size_t size_of_dsize,
                  cl_context context, cl_command_queue command_queue)
{
  gal_data_t *out;

  out = (gal_data_t *)clSVMAlloc (context, CL_MEM_READ_WRITE, sizeof *out, 0);
  if (out == NULL)
    error (EXIT_FAILURE, ENOMEM,
           "%s: CL SVM Alloc Error: %zu bytes for gal_data_t", __func__,
           sizeof *out);

  gal_cl_map_svm_to_cpu (context, command_queue, (void *)out, sizeof *out);

  out->array
      = (void *)clSVMAlloc (context, CL_MEM_READ_WRITE, size_of_array, 0);
  if (out->array == NULL)
    error (EXIT_FAILURE, ENOMEM,
           "%s: CL SVM Alloc Error: %zu bytes for gal_data_t->array", __func__,
           size_of_array);

  gal_cl_map_svm_to_cpu (context, command_queue, (void *)(out->array), size_of_array);

  out->dsize
      = (size_t *)clSVMAlloc (context, CL_MEM_READ_WRITE, size_of_dsize, 0);
  if (out->dsize == NULL)
    error (EXIT_FAILURE, ENOMEM,
           "%s: CL SVM Alloc Error: %zu bytes for gal_data_t->dsize", __func__,
           size_of_dsize);

  gal_cl_map_svm_to_cpu (context, command_queue, (void *)(out->dsize), size_of_dsize);

  return out;
}

void
gal_cl_map_svm_to_cpu (cl_context context, cl_command_queue command_queue,
                void *svm_ptr, size_t size)
{
  cl_int ret = 0;

  ret = clEnqueueSVMMap (command_queue, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE,
                         svm_ptr, size, 0, NULL, NULL);

  if(ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Unable to map SVM allocation to host", __func__,
           ret);

}

void
gal_cl_unmap_svm_to_gpu (cl_context context, cl_command_queue command_queue,
                  void *svm_ptr)
{
  cl_int ret = 0;
  ret = clEnqueueSVMUnmap(command_queue, svm_ptr, 0, NULL, NULL);
  if(ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Failed to unmap SVM allocation to device",
           __func__, ret);
}

void
gal_cl_copy_to_svm(gal_data_t *input, gal_data_t *svm)
{
  float *svm_array = (float *)svm->array;
  float *inp_array = (float *)input->array;
  for(int i = 0; i < input->size; i++)
  {
    svm_array[i] = inp_array[i];
  }
  svm->type = input->type;
  svm->ndim = input->ndim;
  svm->size = input->size;
  svm->quietmmap = input->quietmmap;
  svm->mmapname = input->mmapname;
  svm->minmapsize = input->minmapsize;
  svm->wcs = input->wcs;
  svm->nwcs = input->nwcs;
  svm->flag = input->flag;
  svm->status = input->status;
  svm->name = input->name;
  svm->unit = input->unit;
  svm->comment = input->comment;
  svm->disp_fmt = input->disp_fmt;
  svm->disp_width = input->disp_width;
  svm->disp_precision = input->disp_precision;

  svm->context = input->context;
  svm->svm = input->svm;
  svm->next = input->next;
  svm->block = input->block;
  for(int i = 0; i < svm->ndim; i++)
  {
    svm->dsize[i] = input->dsize[i];
  }
}

void
gal_cl_write_data_to_gpu(cl_context context, 
                          cl_command_queue command_queue,
                          gal_data_t *input)
{
  gal_cl_unmap_svm_to_gpu(context, command_queue, input->array);
  gal_cl_unmap_svm_to_gpu(context, command_queue, input->dsize);
  gal_cl_unmap_svm_to_gpu(context, command_queue, input);
}

void
gal_cl_read_data_to_cpu(cl_context context, 
                          cl_command_queue command_queue,
                          gal_data_t *input)
{
  gal_cl_map_svm_to_cpu (context, command_queue, input, sizeof (gal_data_t));
  gal_cl_map_svm_to_cpu (context, command_queue, input->array,
                         gal_type_sizeof (input->type) * input->size);
  gal_cl_map_svm_to_cpu (context, command_queue, input->dsize,
                         sizeof (size_t) * input->ndim);
}

gal_data_t *
gal_cl_copy_data_to_gpu(cl_context context,
                          cl_command_queue command_queue,
                          gal_data_t *input)
{
  gal_data_t *output = gal_cl_alloc_svm (
      gal_type_sizeof (input->type) * input->size,
      input->ndim * sizeof (size_t), context,
      command_queue);

  gal_cl_copy_to_svm(input, output);

  gal_cl_write_data_to_gpu(context, command_queue, output);
  
  return output;
}





/*********************************************************************/
/*************         Exec On OpenCL Device       *******************/
/*********************************************************************/
double gal_cl_threads_spinoff (clprm *sprm)
{
  int i = 0;
  cl_int ret = 0;
  cl_event spinoff_event;
  cl_command_queue command_queue
      = gal_cl_create_command_queue (sprm->context, sprm->device_id);
  cl_kernel kernel = gal_cl_kernel_create (
      sprm->kernel_path, sprm->kernel_name, sprm->compiler_opts,
      sprm->device_id, sprm->context, command_queue);
  clFinish (command_queue);
  for (; i < sprm->num_svm_args; i++)
    {
      clSetKernelArgSVMPointer (kernel, i, sprm->kernel_args[i]);
    }
  for (; i < sprm->num_kernel_args; i++)
    {
      clSetKernelArg (kernel, i,
                      sprm->kernel_args_sizes[i - sprm->num_svm_args],
                      sprm->kernel_args[i]);
    }

  ret = clSetKernelExecInfo (kernel, CL_KERNEL_EXEC_INFO_SVM_PTRS,
                             sprm->num_extra_svm_args * sizeof (void *),
                             sprm->extra_svm_args);
  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, EINVAL,
           "%s: CL Error: %d Setting implicit SVM pointers for kernel",
           __func__, ret);

  ret = clEnqueueNDRangeKernel (command_queue, kernel, sprm->work_dim, NULL,
                                sprm->global_work_size, sprm->local_work_size,
                                0, NULL, &spinoff_event);

  clFinish (command_queue);
  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, EINVAL,
           "%s: CL Error %d: Unable to launch kernel at %s named %s with "
           "work_dim=%d",
           __func__, ret, sprm->kernel_path, sprm->kernel_name,
           sprm->work_dim);

  /*
    Using the clock() function to measure the time taken for convolution
    gives the wrong time, since clock() measures CPU cycles used by the
    program including any threads launched by it, so it measures the sum of
    clock cycles used by each thread for convolution, instead of wall time
  */
  cl_ulong time_start;
  cl_ulong time_end;
  clGetEventProfilingInfo (spinoff_event, CL_PROFILING_COMMAND_START,
                           sizeof (time_start), &time_start, NULL);
  clGetEventProfilingInfo (spinoff_event, CL_PROFILING_COMMAND_END,
                           sizeof (time_end), &time_end, NULL);
  double nanoSeconds = time_end - time_start;
  ret = clReleaseCommandQueue (command_queue);
  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Invalid release of command queue", __func__, ret);

  return nanoSeconds / 1000000000.0;
}