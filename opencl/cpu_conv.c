#include <gnuastro/threads.h>
#include "conv.h"
#include "cpu_conv.h"


void 
convolve(int row, int col, int image_height, int image_width, int kernell_height, int kernell_width,
         float *image_array, float *kernell_array, float *output)
{                                                                  
    CONV_CORE
} 

void *
convolve_thread(void * in_arg)
{
    struct gal_threads_params *tprm = (struct gal_threads_params *)in_arg;
    struct params *p = (struct params *)tprm->params;

    float * input_array = p->input;
    float * input_kernel = p->kernel;
    float * output = p->result;

    int image_height = p->input_height;
    int image_width = p->input_width;
    int kerenl_height = p->kernel_height;
    int kernel_width = p->kernel_width;


    size_t i, index;
    
    for (i = 0; tprm->indexs[i] != GAL_BLANK_SIZE_T; ++i)
    {
        /* For easy reading. */
        index = tprm->indexs[i];

        int row = index / image_width ;
        int col = index % image_width ;

        convolve(row, col, image_height, image_width,kerenl_height, kernel_width, input_array, input_kernel, output);
    }

    if (tprm->b)
        pthread_barrier_wait(tprm->b);
    return NULL;
    
}


gal_data_t *
gal_conv_cpu(gal_data_t * input_image, gal_data_t * kernel, size_t nthreads)
{
    struct params p;
    p.input = (float *)input_image->array;
    p.kernel = (float *)kernel->array;
    float * result = (float *)malloc(sizeof(float)*input_image->size);
    
    
    p.result = result;
    p.input_height = input_image->dsize[0];
    p.input_width = input_image->dsize[1];
    p.kernel_height = kernel->dsize[0];
    p.kernel_width = kernel->dsize[1];


    gal_threads_spin_off(convolve_thread, &p, p.input_width*p.input_height, nthreads,
                         -1,-1);

    gal_data_t * out;
    out=gal_data_alloc((void *)result, GAL_TYPE_FLOAT32, input_image->ndim, input_image->dsize,
                         input_image->wcs, 0, input_image->minmapsize, input_image->quietmmap,
                         NULL, input_image->unit, NULL);

    
    return out;
}





























































/* TEST */

// int  main()
// {
//     size_t numthreads = gal_threads_number();

//     clock_t start_read, end_read;
//     double cpu_time_used_read;

//     start_read = clock();

//     //reading input image and kernel
//     gal_data_t * input = gal_fits_img_read_to_type("test3.fits","1",GAL_TYPE_FLOAT32,-1,-1);
//     gal_data_t * kernel = gal_fits_img_read_kernel("kernel.fits", "1", -1, -1);
    
//         end_read = clock();
//     cpu_time_used_read = ((double)(end_read - start_read)) / CLOCKS_PER_SEC;
//     printf("Time taken in readinng: %f\n", cpu_time_used_read);
    










//     clock_t start_convolve, end_convolve;
//     double cpu_time_used_convolve;

//     start_convolve = clock();

//     gal_data_t *result = conolve(input, kernel, numthreads);
//     end_convolve = clock();

//     cpu_time_used_convolve = ((double)(end_convolve - start_convolve)) / CLOCKS_PER_SEC;
//     printf("Time taken in convolution: %f\n", cpu_time_used_convolve);






//     clock_t start_write, end_write;
//     double cpu_time_used_write;

//     start_write = clock();
//     fitsfile *fptr = gal_fits_img_write_to_ptr(result,"test_cpu_conv.fits");
//     int status = 0;
//     fits_close_file(fptr, &status);


//     end_write = clock();
//     cpu_time_used_write = ((double)(end_write - start_write)) / CLOCKS_PER_SEC;
//     printf("Time taken in writing: %f\n", cpu_time_used_write);

// }