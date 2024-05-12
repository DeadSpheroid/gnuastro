#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <gnuastro/convolve.h>
#include <gnuastro/cl_utils.h>

gal_data_t *
gal_conv_cl(gal_data_t *input_image, gal_data_t *kernel_image, 
            char * cl_kernel_name, char * function_name, char * core_name,
            size_t global_item_size, size_t local_item_size, int device)
{

    clock_t start_init, end_init;
    double cpu_time_used_init;


    /* initializations */
    int ret=0;
    gal_data_t * out;
    cl_mem cl_output;
    cl_context context = NULL;
    cl_device_id device_id = NULL;
    cl_command_queue command_queue = NULL;
    cl_kernel kernel;
    cl_event conv_event;
    cl_mem cl_image_array,cl_image_dsize;
    cl_mem cl_kernel_array,cl_kernel_dsize;

    printf("  - Using OpenCL kernel: %s\n", cl_kernel_name);
    start_init = clock();

    kernel = gal_cl_kernel_create(cl_kernel_name, function_name,core_name, 
                                                device_id, &context, &command_queue, device);
    clFinish(command_queue);
    end_init = clock();
    cpu_time_used_init = ((double)(end_init - start_init)) / CLOCKS_PER_SEC;
    printf("  - Time taken in initializing: %f\n", cpu_time_used_init);



    clock_t start_copy, end_copy;
    double cpu_time_used_copy;

    start_copy = clock();

    /* prepare the input image for device*/
    gal_cl_copy_array_to_device(input_image, &cl_image_array, context, command_queue, device);
    gal_cl_copy_dsize_to_device(input_image, &cl_image_dsize, context, command_queue, device);

    /* prepare the kernel for device*/
    gal_cl_copy_array_to_device(kernel_image, &cl_kernel_array, context, command_queue, device);
    gal_cl_copy_dsize_to_device(kernel_image, &cl_kernel_dsize, context, command_queue, device);

    /* allocate and initialize output */
    out = gal_data_alloc(NULL, input_image->type, input_image->ndim, input_image->dsize,
                         input_image->wcs, 1, input_image->minmapsize, input_image->quietmmap,
                         NULL, input_image->unit, NULL);
    gal_cl_copy_array_to_device(out, &cl_output, context, command_queue, device);

    clFinish(command_queue);
    end_copy = clock();
    cpu_time_used_copy = ((double)(end_copy - start_copy)) / CLOCKS_PER_SEC;
    printf("  - Time taken in copying input to device: %f\n", cpu_time_used_copy);




    // clock_t start_conv, end_conv;
    // double cpu_time_used_conv;

    /* initialize kernel arguments */
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&cl_image_array);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&cl_image_dsize);
    ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&cl_kernel_array);
    ret = clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&cl_kernel_dsize);
    ret = clSetKernelArg(kernel, 4, sizeof(cl_mem), (void *)&cl_output);

    // start_conv = clock();
    /* launch the kernel */
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global_item_size, 
                                    NULL, 0, NULL, &conv_event);
    // clWaitForEvents(1, &conv_event);
    clFinish(command_queue);
    // end_conv = clock();
    if(ret != CL_SUCCESS)
    {
        printf("Error launching kernel, Error code: %d", ret);
    }
    // cpu_time_used_conv = ((double)(end_conv - start_conv)) / CLOCKS_PER_SEC;

    cl_ulong time_start;
    cl_ulong time_end;

    clGetEventProfilingInfo(conv_event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(conv_event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);
    double nanoSeconds = time_end-time_start;
    printf("  - Time taken in convolution is: %f\n",nanoSeconds / 1000000000.0);
    // printf("  - Time taken as per CPU clock is: %f\n", cpu_time_used_conv);


    clock_t start_copy_to, end_copy_to;
    double cpu_time_used_copy_to;

    start_copy_to = clock();
    /* copy data from gpu buffer to output */
    gal_cl_copy_from_device(out, &cl_output, command_queue);

    clFinish(command_queue);
    end_copy_to = clock();
    cpu_time_used_copy_to = ((double)(end_copy_to - start_copy_to)) / CLOCKS_PER_SEC;
    printf("  - Time taken in copying result to host: %f\n", cpu_time_used_copy_to);



    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseMemObject(cl_image_array);
    ret = clReleaseMemObject(cl_image_dsize);
    ret = clReleaseMemObject(cl_kernel_array);
    ret = clReleaseMemObject(cl_kernel_dsize);
    ret = clReleaseMemObject(cl_output);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);    
    //clFinish(command_queue);
    return out;
}


