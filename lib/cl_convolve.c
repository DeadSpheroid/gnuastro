#include <config.h>
#include <stdio.h>
#include <time.h>

#include <gnuastro/cl_utils.h>
#include <gnuastro/convolve.h>

gal_data_t *
gal_conv_cl (gal_data_t *input_image, gal_data_t *kernel_image,
             char *kernel_name, cl_context context, cl_device_id device_id,
             size_t global_item_size, size_t local_item_size)
{

  clock_t start_init, end_init;
  double cpu_time_used_init;

  /* initializations */
  cl_int ret = 0;
  gal_data_t *out;
//   cl_mem cl_output;
  cl_device_type device_type = (cl_device_type)NULL;
  cl_command_queue command_queue = NULL;
  cl_kernel kernel;
  cl_event conv_event;
  // cl_mem cl_image_array,cl_image_dsize;
  // cl_mem cl_kernel_array,cl_kernel_dsize;

  start_init = clock ();

  kernel = gal_cl_kernel_create (kernel_name, "convolution", device_id,
                                 context, &command_queue);

  clFinish (command_queue);
  end_init = clock ();
  cpu_time_used_init = ((double)(end_init - start_init)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in initializing: %f\n", cpu_time_used_init);

  clock_t start_copy, end_copy;
  double cpu_time_used_copy;

  start_copy = clock ();
  ret = clGetDeviceInfo (device_id, CL_DEVICE_TYPE, sizeof (cl_device_type),
                         (void *)device_type, NULL);
//   int device = (device_type == CL_DEVICE_TYPE_GPU) ? 1 : 2;

  cl_mem input_array = gal_cl_create_buffer_from_array (
      input_image->array,
      input_image->size * gal_type_sizeof (input_image->type), context,
      device_type);

  cl_mem input_dsize = gal_cl_create_buffer_from_array (
      (void *)input_image->dsize, input_image->ndim * sizeof (size_t), context,
      device_type);

  cl_mem kernel_array = gal_cl_create_buffer_from_array (
      kernel_image->array,
      kernel_image->size * gal_type_sizeof (kernel_image->type), context,
      device_type);

  cl_mem kernel_dsize = gal_cl_create_buffer_from_array (
      (void *)kernel_image->dsize, kernel_image->ndim * sizeof (size_t),
      context, device_type);
  /* prepare the input image for device*/
  //   gal_cl_copy_array_to_device (input_image, &cl_image_array, context,
  //                                command_queue, device);
  //   gal_cl_copy_dsize_to_device (input_image, &cl_image_dsize, context,
  //                                command_queue, device);

  /* prepare the kernel for device*/
  //   gal_cl_copy_array_to_device (kernel_image, &cl_kernel_array, context,
  //                                command_queue, device);
  //   gal_cl_copy_dsize_to_device (kernel_image, &cl_kernel_dsize, context,
  //                                command_queue, device);

  /* allocate and initialize output */
  out = gal_data_alloc (NULL, input_image->type, input_image->ndim,
                        input_image->dsize, input_image->wcs, 1,
                        input_image->minmapsize, input_image->quietmmap, NULL,
                        input_image->unit, NULL);

  cl_mem output_array = gal_cl_create_buffer_from_array (
      out->array, out->size * gal_type_sizeof (out->type), context,
      device_type);

//   gal_cl_copy_array_to_device (out, &cl_output, context, command_queue,
//                                device);

  clFinish (command_queue);
  end_copy = clock ();
  cpu_time_used_copy = ((double)(end_copy - start_copy)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying input to device: %f\n",
          cpu_time_used_copy);

  // NOTE: Using the clock() function to measure the time taken for convolution
  // gives the wrong time, since clock() measures CPU cycles used by the
  // program including any threads launched by it, so it measures the sum of
  // clock cycles used by each thread for convolution, instead of wall time
  // clock_t start_conv, end_conv;
  // double cpu_time_used_conv;

  /* initialize kernel arguments */
  ret = clSetKernelArg (kernel, 0, sizeof (cl_mem), (void *)&input_array);
  ret = clSetKernelArg (kernel, 1, sizeof (cl_mem), (void *)&input_dsize);
  ret = clSetKernelArg (kernel, 2, sizeof (cl_mem), (void *)&kernel_array);
  ret = clSetKernelArg (kernel, 3, sizeof (cl_mem), (void *)&kernel_dsize);
  ret = clSetKernelArg (kernel, 4, sizeof (cl_mem), (void *)&output_array);

  // start_conv = clock();
  /* launch the kernel */
  ret = clEnqueueNDRangeKernel (command_queue, kernel, 1, NULL,
                                &global_item_size, NULL, 0, NULL, &conv_event);
  // clWaitForEvents(1, &conv_event);

  clFinish (command_queue);
  // end_conv = clock();
  if (ret != CL_SUCCESS)
    {
      printf ("Error launching kernel, Error code: %d", ret);
    }
  // cpu_time_used_conv = ((double)(end_conv - start_conv)) / CLOCKS_PER_SEC;

  cl_ulong time_start;
  cl_ulong time_end;

  clGetEventProfilingInfo (conv_event, CL_PROFILING_COMMAND_START,
                           sizeof (time_start), &time_start, NULL);
  clGetEventProfilingInfo (conv_event, CL_PROFILING_COMMAND_END,
                           sizeof (time_end), &time_end, NULL);
  double nanoSeconds = time_end - time_start;
  printf ("  - Time taken in convolution is: %f\n",
          nanoSeconds / 1000000000.0);
  // printf("  - Time taken as per CPU clock is: %f\n", cpu_time_used_conv);

  clock_t start_copy_to, end_copy_to;
  double cpu_time_used_copy_to;

  start_copy_to = clock ();
  /* copy data from gpu buffer to output */
  out->array = gal_cl_read_to_host(&output_array, out->size * gal_type_sizeof(out->type), command_queue);
//   gal_cl_copy_from_device (out, &cl_output, command_queue);

  clFinish (command_queue);
  end_copy_to = clock ();
  cpu_time_used_copy_to
      = ((double)(end_copy_to - start_copy_to)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying result to host: %f\n",
          cpu_time_used_copy_to);

  // Clean up
  ret = clFlush (command_queue);
  ret = clFinish (command_queue);
  ret = clReleaseKernel (kernel);
  ret = clReleaseMemObject (input_array);
  ret = clReleaseMemObject (input_dsize);
  ret = clReleaseMemObject (kernel_array);
  ret = clReleaseMemObject (kernel_dsize);
  ret = clReleaseMemObject (output_array);
  ret = clReleaseCommandQueue (command_queue);
  // clFinish(command_queue);
  return out;
}
