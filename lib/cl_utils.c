#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#include <gnuastro/cl_utils.h>

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

  cl_platform_id *platforms
      = (cl_platform_id *)malloc (num_platforms * sizeof (cl_platform_id));

  ret = clGetPlatformIDs (num_platforms, platforms, NULL);

  if (ret != CL_SUCCESS)
    {
      printf ("Error finding platforms\n");
      exit (1);
    }

  uint8_t i;
  for (i = 0; i < num_platforms; i++)
    {
      ret = clGetDeviceIDs (platforms[i], device_type, 1, device_id, NULL);

      if (ret == CL_SUCCESS)
        break;
    }

  if (ret != CL_SUCCESS)
    {
      printf ("Error finding devices\n");
      exit (1);
    }
  *platform_id = platforms[i];

  cl_context_properties properties[]
      = { CL_CONTEXT_PLATFORM, (cl_context_properties)*platform_id, 0 };

  *context
      = clCreateContextFromType (properties, device_type, NULL, NULL, &ret);

  if (ret != CL_SUCCESS)
    {
      printf ("Unable to create OpenCL Context");
      exit (1);
    }

  free (platforms);
}

cl_kernel
gal_cl_kernel_create (char *kernel_name, char *function_name,
                      cl_device_id device_id, cl_context context,
                      cl_command_queue *command_queue)
{
  cl_int ret = 0;
  cl_program program;
  cl_queue_properties properties[]
      = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
  *command_queue = clCreateCommandQueueWithProperties (context, device_id,
                                                       properties, &ret);
  if (ret != CL_SUCCESS)
    {
      printf ("Error in creating command queue\nError code: %d\n", ret);
    }

  FILE *kernelFile = fopen (kernel_name, "r");

  char *result = (char *)malloc (MAX_SOURCE_SIZE);

  size_t kernel_size = fread (result, 1, MAX_SOURCE_SIZE, kernelFile);
  fclose (kernelFile);

  program = clCreateProgramWithSource (context, 1, (const char **)&result,
                                       (const size_t *)&kernel_size, &ret);

  ret = clBuildProgram (program, 1, &device_id, "-I .", NULL, NULL);

  if (ret != CL_SUCCESS)
    {
      size_t len;
      char buffer[2048];
      cl_build_status bldstatus;
      printf ("\nError %d: Failed to build program executable [ ]\n", ret);

      ret = clGetProgramBuildInfo (program, device_id, CL_PROGRAM_BUILD_STATUS,
                                   sizeof (bldstatus), (void *)&bldstatus,
                                   &len);

      printf ("Build Status %d: \n", ret);

      ret = clGetProgramBuildInfo (program, device_id,
                                   CL_PROGRAM_BUILD_OPTIONS, sizeof (buffer),
                                   buffer, &len);

      printf ("Build Options %d: \n", ret);
      printf ("INFO: %s\n", buffer);

      ret = clGetProgramBuildInfo (program, device_id, CL_PROGRAM_BUILD_LOG,
                                   sizeof (buffer), buffer, &len);

      printf ("Build Log %d:\n", ret);
      printf ("%s\n", buffer);
      exit (1);
    }

  return clCreateKernel (program, function_name, &ret);
  ;
}

/*********************************************************************/
/*************            retrieve info            *******************/
/*********************************************************************/
void
gal_cl_get_device_name (cl_device_id device_id, char *device_name)
{
  cl_int return_code = 0;
  return_code = clGetDeviceInfo (device_id, CL_DEVICE_NAME, 256,
                                 (void *)device_name, NULL);

  if (return_code != CL_SUCCESS)
    {
      exit (1);
    }
}

void
gal_cl_get_platform_name (cl_platform_id platform_id, char *platform_name)
{
  cl_int return_code = 0;
  return_code = clGetPlatformInfo (platform_id, CL_PLATFORM_NAME, 256,
                                   (void *)platform_name, NULL);

  if (return_code != CL_SUCCESS)
    {
      exit (1);
    }
}

/*********************************************************************/
/*************            data transfer            *******************/
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
        {
          printf (
              "Error in creating OpenCL buffer for array, Error code: %d\n",
              ret);
        }
    }
  else
    {
      *input_mem_obj
          = clCreateBuffer (context, CL_MEM_READ_ONLY,
                            in->size * gal_type_sizeof (in->type), NULL, &ret);

      if (ret != CL_SUCCESS)
        {
          printf (
              "Error in creating OpenCL buffer for array, Error code: %d\n",
              ret);
        }

      ret = clEnqueueWriteBuffer (command_queue, *input_mem_obj, CL_TRUE, 0,
                                  in->size * gal_type_sizeof (in->type),
                                  (float *)in->array, 0, NULL, NULL);

      if (ret != CL_SUCCESS)
        {
          printf ("Error in writing array to OpenCL buffer on GPU, Error "
                  "code: %d\n",
                  ret);
        }
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
        {
          printf (
              "Error in creating OpenCL buffer for dsize, Error code: %d\n",
              ret);
        }
    }
  else
    {
      *input_mem_obj = clCreateBuffer (context, CL_MEM_READ_ONLY,
                                       3 * sizeof (size_t), NULL, &ret);

      if (ret != CL_SUCCESS)
        {
          printf (
              "Error in creating OpenCL buffer for dsize, Error code: %d\n",
              ret);
        }

      ret = clEnqueueWriteBuffer (command_queue, *input_mem_obj, CL_TRUE, 0,
                                  3 * sizeof (size_t), in->dsize, 0, NULL,
                                  NULL);

      if (ret != CL_SUCCESS)
        {
          printf ("Error in writing dsize to OpenCL buffer on GPU, Error "
                  "code: %d\n",
                  ret);
        }
    }
}

void
gal_cl_copy_struct_to_device (gal_data_t *in, cl_mem *input_mem_obj,
                              cl_context context,
                              cl_command_queue command_queue, int device)
{
  int ret = 0;
  if (device == 2)
    {
      *input_mem_obj
          = clCreateBuffer (context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                            sizeof (in), (void *)in, &ret);
    }
  else
    {
      *input_mem_obj = clCreateBuffer (context, CL_MEM_READ_ONLY, sizeof (in),
                                       NULL, &ret);
      ret = clEnqueueWriteBuffer (command_queue, *input_mem_obj, CL_TRUE, 0,
                                  sizeof (in), in, 0, NULL, NULL);
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
    {
      printf (
          "Error in reading array to OpenCL buffer from GPU, Error code: %d\n",
          ret);
    }
}
