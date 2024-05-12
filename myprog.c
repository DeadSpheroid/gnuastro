#include <gnuastro/convolve.h>
#include <gnuastro/fits.h>
#include <string.h>
int main(int argc, char **argv)
{

    if (argc != 2)
    {
        // printf("invalid argumnets\n");
        // exit(1);
    }

    clock_t start_load, end_load;
    double cpu_time_used_load;

    start_load = clock();

    gal_data_t *input_image = gal_fits_img_read_to_type("data/arithmetic.fits", "1", GAL_TYPE_FLOAT32, -1, -1, "");
    gal_data_t *kernel_image = gal_fits_img_read_kernel("data/kernel.fits", "1", -1, -1, "");
    gal_data_t *output_image;

    end_load = clock();
    cpu_time_used_load = ((double)(end_load - start_load)) / CLOCKS_PER_SEC;
    // printf("Time taken in loadng input from Disk to CPU: %f\n", cpu_time_used_load);

    clock_t start_total, end_total;
    double cpu_time_used_total;

    start_total = clock();
    if (strcmp(argv[1], "gpu") == 0)
    {
        output_image = gal_conv_cl(input_image, kernel_image, "conv.cl", "convolution", "conv_core.h", input_image->size, 128, 1);
    }
    else if (strcmp(argv[1], "cpu") == 0)
    {
        output_image = gal_conv_cl(input_image, kernel_image, "conv.cl", "convolution", "conv_core.h", input_image->size, 128, 2);
    }
    else
    {
        printf("invalid argumnets\n");
        exit(1);
    }
    end_total = clock();
    cpu_time_used_total = ((double)(end_total - start_total)) / CLOCKS_PER_SEC;
    printf("\nTime taken for all operations : %f\n", cpu_time_used_total);

    clock_t start_savi, end_savi;
    double cpu_time_used_savi;

    start_savi = clock();

    char res_file_name[20] = "conv_";
    strcat(res_file_name, argv[1]);
    strcat(res_file_name, "_.fits");
    fitsfile *fptr = gal_fits_img_write_to_ptr(output_image, res_file_name, NULL, 0);
    int status = 0;
    fits_close_file(fptr, &status);

    gal_data_free(input_image);
    gal_data_free(kernel_image);
    gal_data_free(output_image);

    end_savi = clock();
    cpu_time_used_savi = ((double)(end_savi - start_savi)) / CLOCKS_PER_SEC;
    // printf("Time taken in saving output from CPU to disk: %f\n", cpu_time_used_savi);

    return 0;
}