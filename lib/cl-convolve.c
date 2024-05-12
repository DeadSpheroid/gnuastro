#include <config.h>
#include <stdio.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <gnuastro/cl-utils.h>
#include <gnuastro/convolve.h>

gal_data_t *
gal_convolve_cl (gal_data_t *input_image, gal_data_t *kernel_image,
             char *kernel_name, int noedgecorrect, size_t *channels,
             int convoverch, cl_context context, cl_device_id device_id)
{
  cl_int ret = 0;
  if (input_image->ndim != 2)
    error (EXIT_FAILURE, EINVAL,
           "%s: CL Error : Convolution not implemented for 3D Images, please "
           "use 2D Images",
           __func__);
  /* Initializations */
  int block_size = 16;
  int padded_w = (block_size - (input_image->dsize[0] % block_size)) % block_size;
  int padded_l = (block_size - (input_image->dsize[1] % block_size)) % block_size;
  size_t global_item_size[] = {input_image->dsize[0] + padded_w, input_image->dsize[1] + padded_l};
  // printf("DEBUG: Total threads launched:%ld * %ld\n", global_item_size[0], global_item_size[1]);
  size_t local_item_size[] = {block_size, block_size};
  gal_data_t *out;
  clprm *sprm = (clprm *)malloc(sizeof(clprm));
  cl_command_queue command_queue
      = gal_cl_create_command_queue (context, device_id);

  char compiler_opts[256] = "";
  clock_t start_copy, end_copy;
  double cpu_time_used_copy;
  int ignore_ch = ((channels[0] == 1) && (channels[1] == 1)); 
  cl_int tile_w = input_image->dsize[0] / channels[0];
  cl_int tile_h = input_image->dsize[1] / channels[1];
  cl_mem kernel_array;
  start_copy = clock ();

  // Allocate GPU Memory for the gal_data_t's
  gal_data_t *input_image_svm
      = gal_cl_copy_data_to_gpu (context, command_queue, input_image);

  out = gal_data_alloc (NULL, input_image_svm->type, input_image_svm->ndim,
                        input_image_svm->dsize, input_image_svm->wcs, 1,
                        input_image_svm->minmapsize,
                        input_image_svm->quietmmap, NULL,
                        input_image_svm->unit, NULL);

  gal_data_t *out_svm
      = gal_cl_copy_data_to_gpu (context, command_queue, out);

  kernel_array = clCreateBuffer (
      context, CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR,
      sizeof (cl_float) * kernel_image->size, kernel_image->array, &ret);

  if (ret != CL_SUCCESS)
    error (EXIT_FAILURE, ENOTRECOVERABLE,
           "%s: CL Error %d: Failed to create a constant buffer", __func__,
           ret);

  gal_cl_finish_queue (command_queue);

  end_copy = clock ();
  cpu_time_used_copy = ((double)(end_copy - start_copy)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying input to device: %f\n",
          cpu_time_used_copy);

  /* Group internal struct pointers used in kernel */
  void *svm_ptrs[] = { input_image_svm->array,
                       out_svm->array,
                       (void *)input_image_svm->dsize,
                       (void *)out_svm->dsize };

  /* Group all input args(directly referenced) */
  void *kernel_args[]
      = { (void *)input_image_svm, (void *)out_svm, (void *)&kernel_array,
          (void *)&tile_w, (void *)&tile_h };

  size_t kernel_args_sizes[]
      = { sizeof (cl_mem), sizeof (cl_int), sizeof (cl_int) };

  if(noedgecorrect)
    strcat(compiler_opts, " -DNO_EDGE_CORRECT ");

  if(convoverch || ignore_ch)
    strcat(compiler_opts, " -DIS_SAME_CH(x1,x2,y1,y2,tw,th)=true");

  char* kernelmacro = malloc(64 * sizeof(char));
  if (asprintf (&kernelmacro, " -DKERNEL_SIZE=%ld -DKERNEL_RADIUS=%d",
                kernel_image->dsize[0], (int)kernel_image->dsize[0] / 2)
      == -1)
    error (EXIT_FAILURE, ENOMEM, "%s: Error trying to allocate memory",
           __func__);

  strcat(compiler_opts, kernelmacro);

  sprm->kernel_path = kernel_name;
  sprm->kernel_name = "convolution";
  sprm->compiler_opts = compiler_opts;
  sprm->device_id = device_id;
  sprm->context = context;
  sprm->num_kernel_args = 5;
  sprm->num_svm_args = 2;
  sprm->kernel_args = kernel_args;
  sprm->kernel_args_sizes = kernel_args_sizes;
  sprm->num_extra_svm_args = 4;
  sprm->extra_svm_args = svm_ptrs;
  sprm->work_dim = 2;
  sprm->global_work_size = global_item_size;
  sprm->local_work_size = local_item_size;

  /* Spinoff threads on OpenCL Device with specified kernel source and args */
  gal_cl_threads_spinoff (sprm);

  clock_t start_copy_to, end_copy_to;
  double cpu_time_used_copy_to;

  start_copy_to = clock ();

  /* Map output SVM allocation back to host */
  gal_cl_read_data_to_cpu(context, command_queue, out_svm);

  gal_cl_finish_queue (command_queue);

  end_copy_to = clock ();
  cpu_time_used_copy_to
      = ((double)(end_copy_to - start_copy_to)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying result to host: %f\n",
          cpu_time_used_copy_to);

  // Clean up
  gal_cl_free_queue(command_queue);
  free(sprm);
  free(kernelmacro);
  return out_svm;
}

gal_data_t *
gal_convolve_cl_unopt (gal_data_t *input_image, gal_data_t *kernel_image,
             char *kernel_name, int noedgecorrect, size_t *channels,
             int convoverch, cl_context context, cl_device_id device_id)
{
  /* Initializations */
  size_t global_item_size = input_image->size;
  gal_data_t *out;
  clprm *sprm = (clprm *)malloc(sizeof(clprm));
  cl_command_queue command_queue
      = gal_cl_create_command_queue (context, device_id);

  cl_int tile_w = input_image->dsize[0] / channels[0];
  cl_int tile_h = input_image->dsize[1] / channels[1];
  cl_int noedgecorrection = noedgecorrect;
  cl_int convoverchannels = convoverch;

  clock_t start_copy, end_copy;
  double cpu_time_used_copy;

  start_copy = clock ();

  // Allocate GPU Memory for the gal_data_t's
  gal_data_t *input_image_svm
      = gal_cl_copy_data_to_gpu (context, command_queue, input_image);

  gal_data_t *kernel_image_svm
      = gal_cl_copy_data_to_gpu (context, command_queue, kernel_image);

  out = gal_data_alloc (NULL, input_image_svm->type, input_image_svm->ndim,
                        input_image_svm->dsize, input_image_svm->wcs, 1,
                        input_image_svm->minmapsize,
                        input_image_svm->quietmmap, NULL,
                        input_image_svm->unit, NULL);

  gal_data_t *out_svm
      = gal_cl_copy_data_to_gpu (context, command_queue, out);

  gal_cl_finish_queue (command_queue);

  end_copy = clock ();
  cpu_time_used_copy = ((double)(end_copy - start_copy)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying input to device: %f\n",
          cpu_time_used_copy);

  /* Group internal struct pointers used in kernel */
  void *svm_ptrs[] = { input_image_svm->array,
                       kernel_image_svm->array,
                       out_svm->array,
                       (void *)input_image_svm->dsize,
                       (void *)kernel_image_svm->dsize,
                       (void *)out_svm->dsize };

  /* Group all input args(directly referenced) */
  void *kernel_args[] = { (void *)input_image_svm, (void *)kernel_image_svm,
                          (void *)out_svm,         (void *)&tile_w,
                          (void *)&tile_h,         (void *)&noedgecorrection,
                          (void *)&convoverchannels };

  size_t kernel_args_sizes[]
      = { sizeof (cl_int), sizeof (cl_int), sizeof (cl_int), sizeof (cl_int) };
  // printf("Here\n");
  sprm->kernel_path = kernel_name;
  sprm->kernel_name = "convolution";
  sprm->compiler_opts = "";
  sprm->device_id = device_id;
  sprm->context = context;
  sprm->num_kernel_args = 7;
  sprm->kernel_args = kernel_args;
  sprm->kernel_args_sizes = kernel_args_sizes;
  sprm->num_svm_args = 3;
  sprm->num_extra_svm_args = 6;
  sprm->extra_svm_args = svm_ptrs;
  sprm->work_dim = 1;
  sprm->global_work_size = &global_item_size;
  sprm->local_work_size = NULL;

  /* Spinoff threads on OpenCL Device with specified kernel source and args */
  double execution_time = gal_cl_threads_spinoff (sprm);

  printf("  - Time taken to execute kernel: %f\n", execution_time);

  clock_t start_copy_to, end_copy_to;
  double cpu_time_used_copy_to;

  start_copy_to = clock ();

  /* Map output SVM allocation back to host */
  gal_cl_read_data_to_cpu(context, command_queue, out_svm);

  gal_cl_finish_queue (command_queue);

  end_copy_to = clock ();
  cpu_time_used_copy_to
      = ((double)(end_copy_to - start_copy_to)) / CLOCKS_PER_SEC;
  printf ("  - Time taken in copying result to host: %f\n",
          cpu_time_used_copy_to);

  // Clean up
  gal_cl_free_queue(command_queue);
  free(sprm);
  return out_svm;
}