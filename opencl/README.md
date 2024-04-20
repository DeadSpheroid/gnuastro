### This folder is meant to be compiled and run seperately of the main library as the build system is not set for it yet.

### Instructions to generate input image and kernel

```shell
astarithmetic 5000 5000 2 makenew 10 mknoise-sigma
astmkprof --kernel=gaussian,2,3 --oversample=1
```
## Or

    make input

### Instructions to run

- Input image file and Kernel file should be place in /data/ with this folder and named `arithmetic.fits` and `kernel.fits`

1. Compile and run both cpu, gpu versions

    ```make both```

2. Only compile main

    ```make```

3. Run only cpu version

    ```make cpu```

4. Run only gpu version

    ```make gpu```

5. Clean .o and output files

    ```make clean```

6. Open output images along side input images for comparison

    ```make view```

7. Check OpenCL availability on your device

    ```make cl-check```

8. Generate random input
    ```make input ```

### Results on a 5000x5000 image with standard kernel on various devices
```shell
Using platform: Portable Computing Language
Using device: pthread-Intel(R) Core(TM) i5-10300H CPU @ 2.50GHz

Time taken in initializing: 0.113599
Time taken in copying input to device: 0.000058
Time taken in convolution: 4.662804
Time taken in copying result to host: 0.000051

Time taken for all operations : 4.776571

```
***
```shell
Using platform: NVIDIA CUDA
Using device: NVIDIA GeForce GTX 1650

Time taken in initializing: 0.152243
Time taken in copying input to device: 0.046491
Time taken in convolution: 0.011786
Time taken in copying result to host: 0.068962

Time taken for all operations : 0.279955
```
***
```shell
Using platform: Intel(R) OpenCL
Using device: Intel(R) Core(TM) i5-10300H CPU @ 2.50GHz

Time taken in initializing: 0.277849
Time taken in copying input to device: 0.135501
Time taken in convolution: 5.447107
Time taken in copying result to host: 0.232176

Time taken for all operations : 6.105039
```

