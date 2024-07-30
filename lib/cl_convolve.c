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

  kernel = gal_cl_kernel_create (kernel_name, "convolution_svm", device_id,
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

  gal_data_t *input_image_svm = gal_cl_alloc_svm (
      gal_type_sizeof (input_image->type) * input_image->size,
      input_image->ndim * sizeof (*input_image->dsize), context);

  gal_data_t *kernel_image_svm = gal_cl_alloc_svm (
      gal_type_sizeof (kernel_image->type) * kernel_image->size,
      kernel_image->ndim * sizeof (*kernel_image->dsize), context);

  gal_cl_copy_to_svm(input_image, input_image_svm);
  gal_cl_copy_to_svm(kernel_image, kernel_image_svm);

  out = gal_data_alloc (NULL, input_image_svm->type, input_image_svm->ndim,
                        input_image_svm->dsize, input_image_svm->wcs, 1,
                        input_image_svm->minmapsize, input_image_svm->quietmmap, NULL,
                        input_image_svm->unit, NULL);
  // free(input_image->array);

  gal_data_t *out_svm = gal_cl_alloc_svm (
      input_image_svm->size * gal_type_sizeof (input_image_svm->type),
      input_image_svm->ndim * sizeof (size_t), context);

  gal_cl_copy_to_svm(out, out_svm);
  // free(out->array);
  // printf("There\n");
  ret = clEnqueueSVMUnmap(command_queue, input_image_svm->array, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap(command_queue, input_image_svm->dsize, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap(command_queue, input_image_svm, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap(command_queue, kernel_image_svm->array, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap(command_queue, kernel_image_svm->dsize, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap(command_queue, kernel_image_svm, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap(command_queue, out_svm->array, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap(command_queue, out_svm->dsize, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap(command_queue, out_svm, 0, NULL, NULL);
  if(ret != CL_SUCCESS) printf("Failed to unmap svm\n");


  clFinish (command_queue);
  end_copy = clock ();
  cpu_time_used_copy = ((double)(end_copy - start_copy)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying input to device: %f\n",
          cpu_time_used_copy);

  // NOTE: Using the clock() function to measure the time taken for convolution
  // gives the wrong time, since clock() measures CPU cycles used by the
  // program including any threads launched by it, so it measures the sum of
  // clock cycles used by each thread for convolution, instead of wall time

  clSetKernelArgSVMPointer(kernel, 0, (void *)input_image_svm);
  clSetKernelArgSVMPointer(kernel, 1, (void *)kernel_image_svm);
  clSetKernelArgSVMPointer(kernel, 2, (void *)out_svm);
  void* svm_ptrs[] = {
    input_image_svm->array, kernel_image_svm->array, out_svm->array, (void *)input_image_svm->dsize, (void*)kernel_image_svm->dsize, (void*)out_svm->dsize
  };
  ret = clSetKernelExecInfo(kernel, CL_KERNEL_EXEC_INFO_SVM_PTRS, 6 * sizeof(void *), svm_ptrs);
  if(ret != CL_SUCCESS) printf("Error exec inf %d\n", ret);

  /* launch the kernel */
  ret = clEnqueueNDRangeKernel (command_queue, kernel, 1, NULL,
                                &global_item_size, NULL, 0, NULL, &conv_event);

  clFinish (command_queue);
  if (ret != CL_SUCCESS)
    {
      printf ("Error launching kernel, Error code: %d", ret);
    }

  cl_ulong time_start;
  cl_ulong time_end;

  clGetEventProfilingInfo (conv_event, CL_PROFILING_COMMAND_START,
                           sizeof (time_start), &time_start, NULL);
  clGetEventProfilingInfo (conv_event, CL_PROFILING_COMMAND_END,
                           sizeof (time_end), &time_end, NULL);
  double nanoSeconds = time_end - time_start;
  printf ("  - Time taken in convolution is: %f\n",
          nanoSeconds / 1000000000.0);

  clock_t start_copy_to, end_copy_to;
  double cpu_time_used_copy_to;

  start_copy_to = clock ();
  /* copy data from gpu buffer to output */
  ret = clEnqueueSVMMap (command_queue, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE,
                         out_svm, sizeof(gal_data_t), 0, NULL, NULL);
  if (ret != CL_SUCCESS)
    printf ("Error mapping svm %d\n", ret);
  ret = clEnqueueSVMMap (command_queue, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE,
                         out_svm->array, input_image_svm->size * gal_type_sizeof (input_image_svm->type),
                         0, NULL, NULL);
  if (ret != CL_SUCCESS)
    printf ("Error mapping svm %d\n", ret);

  ret = clEnqueueSVMMap (command_queue, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE,
                         out_svm->dsize, input_image_svm->ndim * sizeof (size_t),
                         0, NULL, NULL);
  if (ret != CL_SUCCESS)
    printf ("Error mapping svm %d\n", ret);
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
  ret = clReleaseCommandQueue (command_queue);
  // clFinish(command_queue);
  return out_svm;
}
