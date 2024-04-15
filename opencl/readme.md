### This folder is meant to be compiled and run seperately of the main library as the build system is not set for it yet.


### Instructions to run

- make sure to have a data and kernel fits file in the data directory

1. compile cpu version

```gcc -c cpu_conv.c -lgnuastro -lcfitsio```

2. compile gpu version

```gcc -c gpu_utils.c gpu_conv.c -lgnuastro -lcfitsio -lOpenCL```

3. compile main

```gcc main.c cpu_conv.o gpu_utils.o gpu_conv.o -o main -lgnuastro -lcfitsio -lOpenCL```

4. run

```./main cpu```
OR
```./main gpu```


### More Explanation TBA