#include <gnuastro/threads.h>
#include "conv_core.h"

struct params
{
    float *input;
    float *kernel;
    float *result;
    int input_height;
    int input_width;
    int kernel_height;
    int kernel_width;
};
