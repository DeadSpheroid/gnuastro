#include <stdio.h>
#include <stdlib.h>

#define MAX_SOURCE_SIZE (0x100000)


char *
gal_merge_files(char * kernel, char * core)
{
    FILE *coreFile = fopen("conv_core.h", "r");
    if (coreFile == NULL) {
        perror("Error opening conv_core.h");
        return 1;
    }


    FILE *kernelFile = fopen("conv.cl", "r");
    if (kernelFile == NULL) {
        perror("Error opening conv.cl");
        fclose(coreFile);
        return 1;
    }



    
    char *result = (char *)malloc(MAX_SOURCE_SIZE); // +1 for null-terminator

    size_t core_size = fread(result, 1, MAX_SOURCE_SIZE, coreFile);
    size_t kernel_size = fread(result + core_size, 1, MAX_SOURCE_SIZE, kernelFile);

    // fread(result, 1, coreSize, coreFile);

    // *result[coreSize] = '\n';

    // fread(result + coreSize + 1, 1, kernelSize, kernelFile);

    // *result[coreSize + kernelSize] = '\0';

    fclose(coreFile);
    fclose(kernelFile);

    return result;
}


int main() {
    
    // char * final_str = gal_merge_files("conv.cl", "conv_core.h");

    

    FILE *coreFile = fopen("conv_core.h", "r");
    if (coreFile == NULL) {
        perror("Error opening conv_core.h");
        return 1;
    }


    FILE *kernelFile = fopen("conv.cl", "r");
    if (kernelFile == NULL) {
        perror("Error opening conv.cl");
        fclose(coreFile);
        return 1;
    }


    char *result = (char *)malloc(MAX_SOURCE_SIZE); // +1 for null-terminator
    size_t core_size = fread(result, 1, MAX_SOURCE_SIZE, coreFile);
    
    // append a new line in result
    result[core_size] = '\n';
    size_t kernel_size = fread(result + core_size+1, 1, MAX_SOURCE_SIZE, kernelFile);

    for(int i = 0; i < core_size + kernel_size+10; i++) {
        printf("%c", result[i]);
    }


    return 0;
}
