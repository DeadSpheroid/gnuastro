#define CONV_CORE if (row < image_height && col < image_width)                                                          \
{                                                                                                                       \
    float sum = 0;                                                                                                      \
    for (int y = -kernell_height/2; y <= kernell_height/2; y++)                                                         \
    {                                                                                                                   \
        for (int x = -kernell_width/2; x <= kernell_width/2; x++)                                                       \
        {                                                                                                               \
            if (row + y >= 0 && row + y < image_height && col + x >= 0 && col + x < image_width)                        \
            {                                                                                                           \
                sum += ( image_array[(row + y) * image_width + col + x]                                                 \
                        *                                                                                               \
                         kernell_array[(y + kernell_height/2) * kernell_width + x + kernell_width/2] );                 \
            }                                                                                                           \
        }                                                                                                               \
    }                                                                                                                   \
    output[row * image_width + col] = sum;                                                                              \
}
