#include <config.h>
#include <stdio.h>
#include <time.h>

#include <gnuastro/cl_utils.h>
#include <gnuastro/convolve.h>

gal_data_t *
gal_conv_cl (gal_data_t *input_image, gal_data_t *kernel_image,
             char *kernel_name, cl_context context, cl_device_id device_id)
{

  /* Initializations */
  cl_int ret = 0;
  size_t global_item_size = input_image->size;
  gal_data_t *out;
  cl_command_queue command_queue
      = gal_cl_create_command_queue (context, device_id);

  clock_t start_copy, end_copy;
  double cpu_time_used_copy;

  start_copy = clock ();

  // Allocate SVM Memory for the input image and kernel image
  gal_data_t *input_image_svm = gal_cl_alloc_svm (
      gal_type_sizeof (input_image->type) * input_image->size,
      input_image->ndim * sizeof (*input_image->dsize), context, device_id);

  gal_data_t *kernel_image_svm = gal_cl_alloc_svm (
      gal_type_sizeof (kernel_image->type) * kernel_image->size,
      kernel_image->ndim * sizeof (*kernel_image->dsize), context, device_id);

  // Copy input image and kernel image data to new SVM allocation
  gal_cl_copy_to_svm (input_image, input_image_svm);
  gal_cl_copy_to_svm (kernel_image, kernel_image_svm);


  // Allocate and copy input image to new output image SVM allocation
  out = gal_data_alloc (NULL, input_image_svm->type, input_image_svm->ndim,
                        input_image_svm->dsize, input_image_svm->wcs, 1,
                        input_image_svm->minmapsize,
                        input_image_svm->quietmmap, NULL,
                        input_image_svm->unit, NULL);

  gal_data_t *out_svm = gal_cl_alloc_svm (
      input_image_svm->size * gal_type_sizeof (input_image_svm->type),
      input_image_svm->ndim * sizeof (size_t), context, device_id);

  gal_cl_copy_to_svm (out, out_svm);



  ret = clEnqueueSVMUnmap (command_queue, input_image_svm->array, 0, NULL,
                           NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap (command_queue, input_image_svm->dsize, 0, NULL,
                           NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap (command_queue, input_image_svm, 0, NULL, NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap (command_queue, kernel_image_svm->array, 0, NULL,
                           NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap (command_queue, kernel_image_svm->dsize, 0, NULL,
                           NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap (command_queue, kernel_image_svm, 0, NULL, NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap (command_queue, out_svm->array, 0, NULL, NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap (command_queue, out_svm->dsize, 0, NULL, NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");
  ret = clEnqueueSVMUnmap (command_queue, out_svm, 0, NULL, NULL);
  if (ret != CL_SUCCESS)
    printf ("Failed to unmap svm\n");

  clFinish (command_queue);
  end_copy = clock ();
  cpu_time_used_copy = ((double)(end_copy - start_copy)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying input to device: %f\n",
          cpu_time_used_copy);

  /* Group pointers implicitly referenced in SVM */
  void *svm_ptrs[] = { input_image_svm->array,
                       kernel_image_svm->array,
                       out_svm->array,
                       (void *)input_image_svm->dsize,
                       (void *)kernel_image_svm->dsize,
                       (void *)out_svm->dsize };

  /* Group all input args(directly referenced) */
  void *kernel_args[]
      = { (void *)input_image_svm, (void *)kernel_image_svm, (void *)out_svm };

  /* Spinoff threads on OpenCL Device with specified kernel source and args */
  gal_cl_threads_spinoff (kernel_name, "convolution_svm", device_id, context,
                          3, kernel_args, 6, svm_ptrs, 1, &global_item_size,
                          NULL);

  clock_t start_copy_to, end_copy_to;
  double cpu_time_used_copy_to;

  start_copy_to = clock ();

  /* Map output SVM allocation back to host */
  gal_cl_map_svm (context, device_id, out_svm, sizeof (gal_data_t));

  gal_cl_map_svm (context, device_id, out_svm->array,
                  gal_type_sizeof (input_image_svm->type)
                      * input_image_svm->size);

  gal_cl_map_svm (context, device_id, out_svm->dsize,
                  sizeof (size_t) * input_image_svm->ndim);

  clFinish (command_queue);
  end_copy_to = clock ();
  cpu_time_used_copy_to
      = ((double)(end_copy_to - start_copy_to)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying result to host: %f\n",
          cpu_time_used_copy_to);

  // Clean up
  ret = clReleaseCommandQueue (command_queue);
  return out_svm;
}
