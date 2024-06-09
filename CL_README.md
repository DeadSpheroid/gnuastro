# Information about building Gnuastro with OpenCL enabled

## Testing if OpenCL is correctly configured

You can check to see if OpenCL is successfully set up on your system with `clinfo`
```bash
sudo apt install clinfo
clinfo | grep -C 1 "Device Name"
```
It should display something like
```bash
Number of devices                                 1
  Device Name                                     NVIDIA GeForce RTX 4050 Laptop GPU
  Device Vendor                                   NVIDIA Corporation
--
Number of devices                                 1
  Device Name                                     pthread-13th Gen Intel(R) Core(TM) i7-13620H
  Device Vendor                                   GenuineIntel
```

This ensures the OpenCL runtime is available for all your devices and the loader is working properly.

For development purposes, you will need the libOpenCL.so library, check for it with

```bash
gcc -lOpenCL
```

You should see output like this,

```bash
/usr/bin/ld: /usr/lib/gcc/x86_64-linux-gnu/11/../../../x86_64-linux-gnu/Scrt1.o: in function `_start':
(.text+0x1b): undefined reference to `main'
collect2: error: ld returned 1 exit status
```

## Getting OpenCL
Firstly for **Nvidia** GPUs, 
```bash
sudo apt install nvidia-cuda-toolkit ocl-icd-opencl-dev
```

This alone is enough, but if you want CPU support too,

For **Intel** CPUs, 
```bash
sudo apt install pocl-opencl-icd libpocl-dev libpocl2 libpocl2-common
```

For **AMD** CPUs and GPUs consider building POCL from source, or using an older version of the AMD SDK.


## Building Gnuastro

The normal build procedure is followed with,
```bash
./configure | grep OpenCL
make -j8
make check -j8
sudo make install 
```
Replace -j8 with -j16 or whatever number of cores your cpu has.

The `grep` is necessary to verify that OpenCL was found without having to scroll through large text.

You should see output like this
```bash
checking for libOpenCL... yes
checking how to link with libOpenCL... -lOpenCL
linking flags (LDADD) ... -lgit2 -ltiff -ljpeg -lwcs -lcfitsio -lz -lgsl -lgslcblas -lOpenCL -lm 
```

Then after installing, run the `test-cl-convolve.sh` script
```bash
chmod +x test-cl-convolve.sh
./test-cl-convolve.sh
```

The output you get will vary, but will look like this,

```bash
----------------------OpenCL CPU----------------------
Convolve started on Sun Jun  9 22:53:48 2024
  - Input: data/arithmetic.fits (hdu 1)
  - Kernel: data/kernel.fits (hdu 1)
  - Mode of Operation: OpenCL CPU
  - Using OpenCL kernel: /usr/local/include/gnuastro/astconvolve-conv.cl
  - Using platform: Portable Computing Language
  - Using device: pthread-13th Gen Intel(R) Core(TM) i7-13620H

  - Time taken in initializing: 0.054573
  - Time taken in copying input to device: 0.000037
  - Time taken in convolution is: 2.199336
  - Time taken in copying result to host: 0.000048
  - Output: cl_cpu_conv.fits
Convolve finished in:  4.361111 seconds
----------------------OpenCL GPU----------------------
Convolve started on Sun Jun  9 22:53:52 2024
  - Input: data/arithmetic.fits (hdu 1)
  - Kernel: data/kernel.fits (hdu 1)
  - Mode of Operation: OpenCL GPU
  - Using OpenCL kernel: /usr/local/include/gnuastro/astconvolve-conv.cl
  - Using platform: NVIDIA CUDA
  - Using device: NVIDIA GeForce RTX 4050 Laptop GPU

  - Time taken in initializing: 0.094025
  - Time taken in copying input to device: 0.647133
  - Time taken in convolution is: 0.039234
  - Time taken in copying result to host: 0.384328
  - Output: cl_gpu_conv.fits
Convolve finished in:  3.925250 seconds
----------------------Pthread CPU----------------------
Convolve started on Sun Jun  9 22:53:56 2024
  - Input: data/arithmetic.fits (hdu 1)
  - Kernel: data/kernel.fits (hdu 1)
  - Mode of Operation: pthread CPU
  - Using 16 CPU threads.
  - Output: pthread_cpu_conv.fits
Convolve finished in:  9.125372 seconds
```

Afterward run the `cl-compare.sh` script, to compare the outputs of astconvolve across various devices and check that they are the same.

```bash
chmod +x cl-compare.sh
./cl-compare.sh
```

Again, output will vary, but look like this,
```bash
----------------------OpenCL CPU versus OpenCL GPU----------------------
Statistics (GNU Astronomy Utilities) UNKNOWN
-------
Input: diff_cl_cpu_vs_cl_gpu.fits (hdu: 1)
-------
  Number of elements:                      225000000
  Minimum:                                 -3.814697e-06
  Maximum:                                 3.814697e-06
  Median:                                  0.000000e+00
  Mean:                                    -8.289600022e-12
  Standard deviation:                      2.29137086e-07
-------
Histogram:
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                * *  *                                
 |                              * ****** *                              
 |*   *   *    *   * ******************************** *   *    *        
 |----------------------------------------------------------------------


----------------------Pthread CPU versus OpenCL GPU----------------------
Statistics (GNU Astronomy Utilities) UNKNOWN
-------
Input: diff_pthread_cpu_vs_cl_gpu.fits (hdu: 1)
-------
  Number of elements:                      225000000
  Minimum:                                 -4.768372e-06
  Maximum:                                 4.768372e-06
  Median:                                  0.000000e+00
  Mean:                                    -3.277167298e-11
  Standard deviation:                      3.299786198e-07
-------
Histogram:
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                  *                                   
 |                                 ****                                 
 |                               * **** *                               
 |                               * **** *                               
 |*     *   *  *   * ** ***************************** *  *   *  *       
 |----------------------------------------------------------------------


----------------------Pthread CPU versus OpenCL CPU----------------------
Statistics (GNU Astronomy Utilities) UNKNOWN
-------
Input: diff_pthread_cpu_vs_cl_cpu.fits (hdu: 1)
-------
  Number of elements:                      225000000
  Minimum:                                 -5.722046e-06
  Maximum:                                 4.768372e-06
  Median:                                  0.000000e+00
  Mean:                                    -2.448207296e-11
  Standard deviation:                      3.240690197e-07
-------
Histogram:
 |                                      *                               
 |                                      *                               
 |                                      *                               
 |                                      *                               
 |                                      *                               
 |                                      *                               
 |                                      *                               
 |                                    ****                              
 |                                  * **** *                            
 |                                  * **** *                            
 |*     *     *  *   *  ** * ************************ * *  *  *  *     *
 |----------------------------------------------------------------------

```

Notice the **mean** and **standard deviation** have near-zero values, indicating that the output of the opencl implementation is same as the existing pthread implementation.

