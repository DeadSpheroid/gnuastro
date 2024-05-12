#include <gnuastro/data.h>
#define CL_TARGET_OPENCL_VERSION 300
#include <gnuastro/cl-utils.h>
#include <gnuastro/fits.h>

int main(int argc, char **argv)
{
  if(argc != 3) {
    printf("Error, invalid arguments\n");
    exit(1);
  }
  gal_data_t *input_image1 = gal_fits_img_read_to_type (
      argv[1], "1", GAL_TYPE_FLOAT32, -1, -1, "");

  gal_data_t *input_image2 = gal_fits_img_read_to_type (
      argv[2], "1", GAL_TYPE_FLOAT32, -1, -1, "");

  gal_data_t *output_image = gal_data_alloc (
      NULL, GAL_TYPE_FLOAT32, input_image1->ndim, input_image1->dsize,
      input_image1->wcs, 0, input_image1->minmapsize, input_image1->quietmmap,
      NULL, input_image1->unit, NULL);

  cl_context context;
  cl_platform_id platform_id;
  cl_device_id device_id;

  gal_cl_init (CL_DEVICE_TYPE_GPU, &context, &platform_id, &device_id);
  cl_command_queue command_queue
      = gal_cl_create_command_queue (context, device_id);


  gal_data_t *input_image1_gpu
      = gal_cl_copy_data_to_gpu (context, command_queue, input_image1);
  gal_data_t *input_image2_gpu
      = gal_cl_copy_data_to_gpu (context, command_queue, input_image2);
  gal_data_t *output_image_gpu
      = gal_cl_copy_data_to_gpu (context, command_queue, output_image);

  gal_cl_finish_queue (command_queue);

  clprm *sprm = (clprm *)malloc (sizeof (clprm));

  void *kernel_args[] = { (void *)input_image1_gpu, (void *)input_image2_gpu,
                          (void *)output_image_gpu };

  void *svm_ptrs[]
      = { (void *)input_image1_gpu->array, (void *)input_image2_gpu->array,
          (void *)output_image_gpu->array };

  size_t numactions = input_image1->size;

  sprm->kernel_path = "./lib/kernels/add.cl";
  sprm->kernel_name = "add";
  sprm->compiler_opts = "";
  sprm->device_id = device_id;
  sprm->context = context;
  sprm->num_kernel_args = 3;
  sprm->num_svm_args = 3;
  sprm->kernel_args = kernel_args;
  sprm->num_extra_svm_args = 3;
  sprm->extra_svm_args = svm_ptrs;
  sprm->work_dim = 1;
  sprm->global_work_size = &numactions;
  sprm->local_work_size = NULL;

  gal_cl_threads_spinoff (sprm);

  gal_cl_read_data_to_cpu(context, command_queue, output_image_gpu);

  fitsfile *fptr = gal_fits_img_write_to_ptr(output_image_gpu, "added.fits", NULL, 0);
  int status = 0;
  fits_close_file(fptr, &status);

  return 0;
}