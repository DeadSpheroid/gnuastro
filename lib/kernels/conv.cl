__kernel void
convolution(
            __global float *image_array, __global size_t *image_dsize,
            __global float *kernell_array, __global size_t *kernell_dsize,

            __global float *output) {
  // /* correct the pointers */
  // image->dsize = image_dsize;
  // image->array = image_array;
  // kernell->dsize = kernell_dsize;
  // kernell->array = kernell_array;

  /* get the image and kernel size */
  int image_height = image_dsize[0];
  int image_width = image_dsize[1];
  int kernell_height = kernell_dsize[0];
  int kernell_width = kernell_dsize[1];

  /* get the local group id */
  int id = get_global_id(0);
  int row = id / image_width;
  int col = id % image_width;

  if (row < image_height && col < image_width) {

    float sum = 0;
    for (int y = -kernell_height / 2; y <= kernell_height / 2; y++) {
      for (int x = -kernell_width / 2; x <= kernell_width / 2; x++) {
        if (row + y >= 0 && row + y < image_height && col + x >= 0 &&
            col + x < image_width) {
          sum += (image_array[(row + y) * image_width + col + x] *
                  kernell_array[(y + kernell_height / 2) * kernell_width + x +
                                kernell_width / 2]);
        }
      }
    }
    output[row * image_width + col] = sum;
  }
}
