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
cl_kernel
gal_cl_kernel_create(char *kernel_name, char *function_name, char *core_name,
                     cl_device_id device_id, cl_context *context,
                     cl_command_queue *command_queue, int device)
{
    /* initializations */
    int ret = 0;
    cl_program program;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_uint num_platforms = -1;
    cl_platform_id platform_id = NULL;

    // Get total num of platforms
    ret = clGetPlatformIDs(0, NULL, &num_platforms);

    // Allocate space for platform info array
    cl_platform_id *platforms =
        (cl_platform_id *)malloc(num_platforms * sizeof(cl_platform_id));

    // printf("No of platforms available on your device%d\n\n", num_platforms);

    // Get all available platforms
    ret = clGetPlatformIDs(num_platforms, platforms, NULL);
    // just put one availbale device inside device_id
    cl_uint i;
    for (i = 0; i < num_platforms; i++)
    {
        if (device == 1)
        {
            // Searching for a gpu
            ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
        }
        else
        {
            // Searching for a cpu
            ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_CPU, 1, &device_id, &ret_num_devices);
        }
        if (ret == CL_SUCCESS)
        {
            break;
        }
    }
    if (ret != CL_SUCCESS)
    {
        printf("Error finding devices\n");
    }

    char buffer[256];
    ret = clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(buffer), (void *)buffer, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error getting platform name\n");
    }
    else
    {
        printf("  - Using platform: %s\n", buffer);
    }

    ret = clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(buffer), (void *)buffer, NULL);
    if (ret != CL_SUCCESS)
    {
        printf("Error getting device name\n");
    }
    else
    {
        printf("  - Using device: %s\n\n", buffer);
    }

    *context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    printf("There\n");
    *command_queue = clCreateCommandQueueWithProperties(*context, device_id, CL_QUEUE_PROFILING_ENABLE, &ret);
    if(ret != CL_SUCCESS){
        printf("Error in creating command queue, code %d", ret);
    }
    printf("Here");
    // FILE *coreFile = fopen(core_name, "r");
    // FILE *kernelFile = fopen(kernel_name, "r");

    // char *result = (char *)malloc(MAX_SOURCE_SIZE);
    // size_t core_size = fread(result, 1, MAX_SOURCE_SIZE, coreFile);

    // result[core_size] = '\n';
    // size_t kernel_size = fread(result + core_size+1, 1, MAX_SOURCE_SIZE, kernelFile);
    // result[core_size + kernel_size] = '\0';

    // fclose(coreFile);
    // fclose(kernelFile);

    FILE *kernelFile = fopen(kernel_name, "r");

    char *result = (char *)malloc(MAX_SOURCE_SIZE);

    size_t kernel_size = fread(result, 1, MAX_SOURCE_SIZE, kernelFile);
    fclose(kernelFile);
    
    program = clCreateProgramWithSource(*context, 1,
                                        (const char **)&result,
                                        (const size_t *)&kernel_size, &ret);

    ret = clBuildProgram(program, 1, &device_id, "-I .", NULL, NULL);

    if (ret != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];
        cl_build_status bldstatus;
        printf("\nError %d: Failed to build program executable [ ]\n", ret);
        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_STATUS, sizeof(bldstatus), (void *)&bldstatus, &len);
        printf("Build Status %d: \n", ret);
        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_OPTIONS, sizeof(buffer), buffer, &len);
        printf("Build Options %d: \n", ret);
        printf("INFO: %s\n", buffer);
        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("Build Log %d:\n", ret);
        printf("%s\n", buffer);
        exit(1);
    }

    cl_kernel kernel = clCreateKernel(program, function_name, &ret);

    return kernel;
}

// void
// gal_gpu_context_prepare(cl_device_id device_id, cl_context *context, cl_command_queue *command_queue)
// {

//     cl_platform_id platform_id = NULL;
//     cl_uint ret_num_devices;
//     cl_uint ret_num_platforms;

//     cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
//     ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);

//     *context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);

//     *command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

// }

cl_mem
gal_cl_copy_array_to_device(gal_data_t *in, cl_mem *input_mem_obj, cl_context context,
                            cl_command_queue command_queue, int device)
{
    int ret = 0;
    if(device == 2){
        *input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                    in->size * gal_type_sizeof(in->type), (void*)in->array, &ret);
    }
    else{
        *input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                    in->size * gal_type_sizeof(in->type), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, *input_mem_obj, CL_TRUE, 0,
                                in->size * gal_type_sizeof(in->type), (float *)in->array, 0, NULL, NULL);
    }
}

cl_mem
gal_cl_copy_dsize_to_device(gal_data_t *in, cl_mem *input_mem_obj, cl_context context,
                            cl_command_queue command_queue, int device)
{
    int ret = 0;
    if(device == 2){
        *input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                    3 * sizeof(size_t), (void*)in->dsize, &ret);
    }
    else{
        *input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                    3 * sizeof(size_t), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, *input_mem_obj, CL_TRUE, 0,
                                3 * sizeof(size_t), in->dsize, 0, NULL, NULL);
    }
    

}

cl_mem
gal_cl_copy_struct_to_device(gal_data_t *in, cl_mem *input_mem_obj, cl_context context,
                             cl_command_queue command_queue, int device)
{
    int ret = 0;
    if(device == 2){
        *input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR,
                                        sizeof(in), (void*)in, &ret);
    }
    else{
        *input_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                        sizeof(in), NULL, &ret);
        ret = clEnqueueWriteBuffer(command_queue, *input_mem_obj, CL_TRUE, 0,
                                sizeof(in), in, 0, NULL, NULL);
    }

}

void gal_cl_copy_from_device(gal_data_t *out, cl_mem *output_mem_obj, cl_command_queue command_queue)
{
    clEnqueueReadBuffer(command_queue, *output_mem_obj, CL_TRUE, 0,
                        out->size * gal_type_sizeof(out->type), (float *)out->array, 0, NULL, NULL);
}
