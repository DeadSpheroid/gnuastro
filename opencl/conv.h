#include <gnuastro/fits.h>

gal_data_t *
gal_conv_cl(gal_data_t *input_image, gal_data_t *kernel_image, 
                char * kernel_name, char * function_name, char *core_name,
                size_t global_item_size, size_t local_item_size, int device);
